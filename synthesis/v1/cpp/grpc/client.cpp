#include "client.hpp"

#include <fstream>

namespace smartspeech::grpc {

abstract_connection::abstract_connection(const std::shared_ptr<::grpc::Channel> &channel) {}

client::client(const params &p) {
  ::grpc::SslCredentialsOptions ssl_opts;
  if (!p.root_ca.empty()) {
    ssl_opts.pem_root_certs = p.root_ca;
  }
  channel_ = ::grpc::CreateChannel(
      p.host,
      ::grpc::CompositeChannelCredentials(::grpc::SslCredentials(ssl_opts), ::grpc::AccessTokenCredentials(p.token)));

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

std::unique_ptr<synthesis::connection> client::start_synth_connection(const synthesis::connection::params &p,
                                                                            synthesis::connection::on_result &&cb) {
  return std::make_unique<synthesis::connection>(channel_, cq_, p, std::move(cb));
}

synthesis::connection::connection(const std::shared_ptr<::grpc::Channel> &channel, ::grpc::CompletionQueue &cq,
                                  const params &p, on_result &&cb)
    : abstract_connection(channel)
    , stub_(smartspeech::synthesis::v1::SmartSpeech::NewStub(channel))
    , cq_(cq)
    , params_(p)
    , on_result_cb_(std::move(cb)) {
  auto *event_tag = new grpc_event_tag(grpc_event_tag::cause::start_call, this);

  smartspeech::synthesis::v1::SynthesisRequest request;
  request.set_text(p.text);
  request.set_audio_encoding(smartspeech::synthesis::v1::SynthesisRequest_AudioEncoding_PCM_S16LE);
  request.set_language("ru-RU");
  request.set_content_type(smartspeech::synthesis::v1::SynthesisRequest_ContentType_TEXT);
  request.set_voice("May_24000");

  responder_ = stub_->AsyncSynthesize(&context_, request, &cq, event_tag);
}

void synthesis::connection::proceed(enum grpc_event_tag::cause cause, bool ok) {
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
      read();
      break;
    case grpc_event_tag::cause::read:
      on_read();
      read();
      break;
    case grpc_event_tag::cause::finish:
      if (status_.ok()) {
        std::cout << "FIN\n";
      } else {
        std::cout << "FIN err: " << status_.error_message() << ": " << status_.error_details();
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
  r.buffer.assign(response_.data().data(), response_.data().data() + response_.data().size());
  on_result_cb_(r);
}

}  // namespace smartspeech::grpc