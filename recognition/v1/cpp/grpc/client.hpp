#pragma once

#include <grpc++/grpc++.h>
#include <grpcpp/alarm.h>
#include <recognition.grpc.pb.h>

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
  std::unique_ptr<smartspeech::recognition::v1::SmartSpeech::Stub> stub_;
  ::grpc::ClientContext context_;
  ::grpc::Status status_;
};

namespace recognition {
struct result {
  bool eou;
  std::string words;
  std::string normalized;
};

class connection : public abstract_connection {
 public:
  using on_result = std::function<void(const result &)>;
  struct params {
    uint16_t audio_encoding_flag;
    int32_t sample_rate;
    std::string model;
    int32_t hypotheses_count;
    bool enable_multi_utterance;
    bool enable_partial_results;
    size_t no_speech_timeout_ms;
    size_t max_speech_timeout_ms;
  };

  connection(const std::shared_ptr<::grpc::Channel> &channel, ::grpc::CompletionQueue &cq, const params &p,
             on_result &&cb);
  void feed(uint8_t *buffer, size_t size);
  void writes_done();

  virtual void proceed(enum grpc_event_tag::cause cause, bool ok) override;

 private:
  void send_initial_settings();
  std::vector<uint8_t> get_prepared_chunk();
  void send_audio_chunk(std::vector<uint8_t> &&chunk);
  void send_writes_done();
  void arm_alarm();

  void read();
  void on_read();

 private:
  std::unique_ptr<::grpc::ClientAsyncReaderWriter<smartspeech::recognition::v1::RecognitionRequest,
                                                  smartspeech::recognition::v1::RecognitionResponse>>
      responder_;
  ::grpc::CompletionQueue &cq_;
  smartspeech::recognition::v1::RecognitionResponse response_;
  ::grpc::Alarm alarm_;

  std::mutex m_;
  std::vector<uint8_t> internal_buffer_;

  params params_;
  on_result on_result_cb_;


  bool write_pending_;
  std::atomic_bool writes_done_;
};
}  // namespace recognition

class client {
 public:
  struct params {
    std::string host;
    std::string token;
    std::string root_ca;
  };
  explicit client(const params &p);
  ~client();
  std::unique_ptr<recognition::connection> start_recognition_connection(const recognition::connection::params &p,
                                                        recognition::connection::on_result &&cb);

 private:
  ::grpc::CompletionQueue cq_;
  std::shared_ptr<::grpc::Channel> channel_;
  std::thread worker_thread_;
};
}  // namespace smartspeech::grpc