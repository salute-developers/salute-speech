#include "client.hpp"

#include <fstream>

namespace smartspeech::grpc {

abstract_connection::abstract_connection(const std::shared_ptr<::grpc::Channel> &channel)
    : stub_(smartspeech::recognition::v1::SmartSpeech::NewStub(channel)) {}

client::client(const params &p) {
  ::grpc::SslCredentialsOptions ssl_opts;
  if (!p.root_ca.empty()) {
    ssl_opts.pem_root_certs = p.root_ca;
  }
  channel_ = ::grpc::CreateChannel(p.host, ::grpc::CompositeChannelCredentials(::grpc::SslCredentials(ssl_opts),
                                                                               ::grpc::AccessTokenCredentials(p.token)));

  worker_thread_ = std::thread([this] {
    void *tag = nullptr;
    bool ok = false;

    while (cq_.Next(&tag, &ok)) {
      if (tag) {
        auto event_tag = static_cast<grpc_event_tag *>(tag);
        event_tag->connection->proceed(event_tag->cause, ok);
        delete event_tag;
      }
    }
  });
}

client::~client() {
  cq_.Shutdown();
  if (worker_thread_.joinable()) {
    worker_thread_.join();
  }
}

std::unique_ptr<recognition::connection> client::start_recognition_connection(const recognition::connection::params &p,
                                                              recognition::connection::on_result &&cb) {
  return std::make_unique<recognition::connection>(channel_, cq_, p, std::move(cb));
}

recognition::connection::connection(const std::shared_ptr<::grpc::Channel> &channel, ::grpc::CompletionQueue &cq,
                                    const params &p, on_result &&cb)
    : abstract_connection(channel)
    , cq_(cq)
    , params_(p)
    , on_result_cb_(std::move(cb))
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
  if (!status_.ok()) {
    std::cerr << "proceed error: " << status_.error_message() << std::endl;
  }

  if (!ok) {
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
    case grpc_event_tag::cause::writes_done:
      writes_done_ = true;
      break;
    case grpc_event_tag::cause::finish:
      if (status_.ok()) {
        std::cout << "FIN\n";
      } else {
        std::cout << "FIN err: " << status_.error_message() << ": " << status_.error_details();
      }
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

}  // namespace smartspeech::grpc