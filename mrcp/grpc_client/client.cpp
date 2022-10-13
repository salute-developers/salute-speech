#include "client.hpp"

#include <fstream>

namespace smartspeech::grpc {

class authenticator : public ::grpc::MetadataCredentialsPlugin {
 public:
  authenticator(smartspeech::token_resolver &token_resolver)
      : token_resolver_(token_resolver) {}

  ::grpc::Status GetMetadata(::grpc::string_ref service_url, ::grpc::string_ref method_name,
                             const ::grpc::AuthContext &channel_auth_context,
                             std::multimap<::grpc::string, ::grpc::string> *metadata) override {
    ::grpc::string auth = ::grpc::string("Bearer ") + token_resolver_.get_token();
    metadata->insert(std::make_pair("authorization", auth));
    return ::grpc::Status::OK;
  }

 private:
  smartspeech::token_resolver &token_resolver_;
};

abstract_connection::abstract_connection(const std::shared_ptr<::grpc::Channel> &channel)
    : stub_(smartspeech::recognition::v1::SmartSpeech::NewStub(channel))
    , active_(true) {}

void abstract_connection::disable() {
  active_ = false;
}

bool abstract_connection::active() const {
  return active_;
}

client::client(const params &p) 
  : cq_(nullptr)
  , channel_(nullptr)  
{
  cq_.reset(new ::grpc::CompletionQueue());
  ::grpc::SslCredentialsOptions ssl_opts;
  if (!p.root_ca.empty()) {
    ssl_opts.pem_root_certs = p.root_ca;
  }
  channel_ = ::grpc::CreateChannel(
      p.host, ::grpc::CompositeChannelCredentials(
                  ::grpc::SslCredentials(ssl_opts),
                  ::grpc::MetadataCredentialsFromPlugin(
                      std::unique_ptr<::grpc::MetadataCredentialsPlugin>(new authenticator(p.token_resolver)))));

  worker_thread_ = std::thread([this] {
    void *tag = nullptr;
    bool ok = false;

    while (cq_->Next(&tag, &ok)) {
      if (tag) {
        auto event_tag = static_cast<grpc_event_tag *>(tag);
        if (event_tag->connection->active()) {
          event_tag->connection->proceed(event_tag->cause, ok);
        } else {
          // todo: fix grpc pure virtual call
          // delete event_tag->connection;
        }
        delete event_tag;
      }
    }
  });
}

client::~client() {
  cq_->Shutdown();
  if (worker_thread_.joinable()) {
    worker_thread_.join();
  }
}

std::unique_ptr<recognition::connection> client::start_recognition_connection(
    const recognition::connection::params &p, recognition::connection::on_result &&result_cb,
    recognition::connection::on_error &&error_cb) {
  return std::make_unique<recognition::connection>(channel_, *cq_, p, std::move(result_cb), std::move(error_cb));
}

std::unique_ptr<synthesis::connection> client::start_synth_connection(const synthesis::connection::params &p,
                                                                      synthesis::connection::on_result &&result_cb,
                                                                      synthesis::connection::on_error &&error_cb) {
  return std::make_unique<synthesis::connection>(channel_, *cq_, p, std::move(result_cb), std::move(error_cb));
}

recognition::connection::connection(const std::shared_ptr<::grpc::Channel> &channel, ::grpc::CompletionQueue &cq,
                                    const params &p, on_result &&result_cb, on_error &&error_cb)
    : abstract_connection(channel)
    , cq_(cq)
    , params_(p)
    , on_result_cb_(std::move(result_cb))
    , on_error_cb_(std::move(error_cb))
    , write_pending_(false)
    , writes_done_(false) {
  auto *event_tag = new grpc_event_tag(grpc_event_tag::cause::start_call, this);
  responder_ = stub_->AsyncRecognize(&context_, &cq, event_tag);
}

void recognition::connection::writes_done() {
  writes_done_ = true;
}

void recognition::connection::feed(uint8_t *buffer, size_t size) {
  std::lock_guard<std::mutex> l(m_);
  internal_buffer_.insert(internal_buffer_.end(), buffer, buffer + size);
}

void recognition::connection::proceed(enum grpc_event_tag::cause cause, bool ok) {
  // check status after Finish() call
  if (cause == grpc_event_tag::cause::finish && !status_.ok()) {
    std::cerr << "smartspeech grpc error: " << status_.error_message() << std::endl;
    on_error_cb_(status_.error_message());
    return;
  }

  // there are no more messages to be received from the server (earlier call to AsyncReaderInterface::Read that yielded
  // a failed result cq->Next(&read_tag, &ok) filled in 'ok' with 'false'
  if (cause == grpc_event_tag::cause::read && !ok) {
    auto event_tag = new grpc_event_tag(grpc_event_tag::cause::finish, this);
    responder_->Finish(&status_, event_tag);
    return;
  }

  switch (cause) {
    case grpc_event_tag::cause::start_call:
      send_initial_settings();
      read();
      break;
    case grpc_event_tag::cause::write:
      write_pending_ = false;
    case grpc_event_tag::cause::alarm: {
      if (!write_pending_) {
        auto chunk = get_prepared_chunk();
        if (!chunk.empty()) {
          write_pending_ = true;
          send_audio_chunk(std::move(chunk));
        } else if (writes_done_) {
          send_writes_done();
        } else {
          arm_alarm();
        }
      }
    } break;
    case grpc_event_tag::cause::read:
      on_read();
      read();
      break;
    case grpc_event_tag::cause::writes_done: {
      auto event_tag = new grpc_event_tag(grpc_event_tag::cause::finish, this);
      responder_->Finish(&status_, event_tag);
    } break;
    case grpc_event_tag::cause::finish:
    default:
      break;
  }
}

void recognition::connection::send_initial_settings() {
  smartspeech::recognition::v1::RecognitionRequest request;
  auto *options = new smartspeech::recognition::v1::RecognitionOptions();

  options->set_audio_encoding(smartspeech::recognition::v1::RecognitionOptions_AudioEncoding_PCM_S16LE);
  options->set_model(params_.model);
  options->set_sample_rate(params_.sample_rate);
  options->set_enable_multi_utterance(params_.enable_multi_utterance);
  options->set_enable_partial_results(params_.enable_partial_results);
  request.set_allocated_options(options);

  write_pending_ = true;
  auto event_tag = new grpc_event_tag(grpc_event_tag::cause::write, this);
  responder_->Write(request, event_tag);
}

std::vector<uint8_t> recognition::connection::get_prepared_chunk() {
  std::vector<uint8_t> chunk;
  std::lock_guard<std::mutex> l(m_);
  if (!internal_buffer_.empty()) {
    std::swap(chunk, internal_buffer_);
  }
  return std::move(chunk);
}

void recognition::connection::send_audio_chunk(std::vector<uint8_t> &&chunk) {
  smartspeech::recognition::v1::RecognitionRequest request;
  request.set_audio_chunk(chunk.data(), chunk.size());
  auto event_tag = new grpc_event_tag(grpc_event_tag::cause::write, this);
  responder_->Write(request, event_tag);
}

void recognition::connection::send_writes_done() {
  auto event_tag = new grpc_event_tag(grpc_event_tag::cause::writes_done, this);
  responder_->WritesDone(event_tag);
}

void recognition::connection::arm_alarm() {
  auto event_tag = new grpc_event_tag(grpc_event_tag::cause::alarm, this);
  alarm_.Set(&cq_, std::chrono::system_clock::now() + std::chrono::milliseconds(100), event_tag);
}

void recognition::connection::read() {
  auto event_tag = new grpc_event_tag(grpc_event_tag::cause::read, this);
  responder_->Read(&response_, event_tag);
}

void recognition::connection::on_read() {
  recognition::result r{};
  r.eou = response_.eou();
  r.words = response_.results()[0].text();
  r.normalized = response_.results()[0].normalized_text();

  on_result_cb_(r);
}

synthesis::connection::connection(const std::shared_ptr<::grpc::Channel> &channel, ::grpc::CompletionQueue &cq,
                                  const params &p, on_result &&result_cb, on_error &&error_cb)
    : abstract_connection(channel)
    , stub_(smartspeech::synthesis::v1::SmartSpeech::NewStub(channel))
    , cq_(cq)
    , params_(p)
    , on_result_cb_(std::move(result_cb))
    , on_error_cb_(std::move(error_cb)) {
  auto *event_tag = new grpc_event_tag(grpc_event_tag::cause::start_call, this);

  smartspeech::synthesis::v1::SynthesisRequest request;
  request.set_text(p.text);
  request.set_audio_encoding(smartspeech::synthesis::v1::SynthesisRequest_AudioEncoding_PCM_S16LE);
  request.set_language("ru-RU");
  auto content_type = (p.is_ssml) ? smartspeech::synthesis::v1::SynthesisRequest_ContentType_SSML
                                  : smartspeech::synthesis::v1::SynthesisRequest_ContentType_TEXT;
  request.set_content_type(content_type);
  request.set_voice("Bys_8000");

  responder_ = stub_->AsyncSynthesize(&context_, request, &cq, event_tag);
}

void synthesis::connection::proceed(enum grpc_event_tag::cause cause, bool ok) {
  // check status after Finish() call
  if (cause == grpc_event_tag::cause::finish && !status_.ok()) {
    std::cerr << "smartspeech grpc error: " << status_.error_message() << std::endl;
    on_error_cb_(status_.error_message());
    return;
  }

  // there are no more messages to be received from the server (earlier call to AsyncReaderInterface::Read that yielded
  // a failed result cq->Next(&read_tag, &ok) filled in 'ok' with 'false'
  if (cause == grpc_event_tag::cause::read && !ok) {
    auto event_tag = new grpc_event_tag(grpc_event_tag::cause::finish, this);
    responder_->Finish(&status_, event_tag);
    return;
  }

  switch (cause) {
    case grpc_event_tag::cause::start_call:
      read();
      break;
    case grpc_event_tag::cause::read:
      on_read();
      read();
      break;
    case grpc_event_tag::cause::finish:
      if (!status_.ok()) {
        std::cerr << "FIN err: " << status_.error_message() << ": " << status_.error_details();
      }
      {
        synthesis::result r{};
        r.end = true;
        on_result_cb_(r);
      }
    default:
      break;
  }
}

void synthesis::connection::read() {
  auto event_tag = new grpc_event_tag(grpc_event_tag::cause::read, this);
  responder_->Read(&response_, event_tag);
}

void synthesis::connection::on_read() {
  synthesis::result r{};
  r.end = false;
  r.buffer.assign(response_.data().data(), response_.data().data() + response_.data().size());
  on_result_cb_(r);
}

}  // namespace smartspeech::grpc
