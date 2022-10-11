#pragma once

#include <grpc++/grpc++.h>
#include <grpcpp/alarm.h>
#include <synthesis.grpc.pb.h>

#include <string>
#include <thread>

namespace smartspeech::grpc {
class abstract_connection;
struct grpc_event_tag {
  enum class cause { start_call, write, read, writes_done, finish, alarm };
  cause cause;
  abstract_connection *connection;
  grpc_event_tag(enum cause c, abstract_connection *ptr)
      : cause(c)
      , connection(ptr) {}
};
class abstract_connection {
 public:
  explicit abstract_connection(const std::shared_ptr<::grpc::Channel> &channel);
  virtual ~abstract_connection() = default;
  virtual void proceed(enum grpc_event_tag::cause cause, bool ok) = 0;

 protected:
  ::grpc::ClientContext context_;
  ::grpc::Status status_;
};

namespace synthesis {
struct result {
  bool end;
  std::vector<uint8_t> buffer;
};

class connection : public abstract_connection {
 public:
  using on_result = std::function<void(const result &)>;
  struct params {
    std::string text;
  };

  connection(const std::shared_ptr<::grpc::Channel> &channel, ::grpc::CompletionQueue &cq, const params &p,
             on_result &&cb);

  virtual void proceed(enum grpc_event_tag::cause cause, bool ok) override;

 private:
  void read();
  void on_read();

 private:
  std::unique_ptr<smartspeech::synthesis::v1::SmartSpeech::Stub> stub_;
  std::unique_ptr<::grpc::ClientAsyncReader<smartspeech::synthesis::v1::SynthesisResponse>> responder_;
  ::grpc::CompletionQueue &cq_;
  smartspeech::synthesis::v1::SynthesisResponse response_;
  params params_;
  on_result on_result_cb_;
};
}  // namespace synthesis

class client {
 public:
  struct params {
    std::string host;
    std::string token;
    std::string root_ca;
  };
  explicit client(const params &p);
  ~client();
  std::unique_ptr<synthesis::connection> start_synth_connection(const synthesis::connection::params &p,
                                                                      synthesis::connection::on_result &&cb);

 private:
  ::grpc::CompletionQueue cq_;
  std::shared_ptr<::grpc::Channel> channel_;
  std::thread worker_thread_;
};
}  // namespace smartspeech::grpc