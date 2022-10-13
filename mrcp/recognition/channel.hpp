#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <apt_consumer_task.h>
#include <mpf_stream.h>
#include <mrcp_engine_types.h>
#include <mrcp_message.h>

#ifdef __cplusplus
}
#endif

#include <memory>
#include <vector>

#include "grpc_client/client.hpp"

namespace smartspeech::mrcp::recognition {
class channel {
 public:
  struct params {
    mrcp_engine_t *mrcp_engine;
    apr_pool_t *pool;
    apt_consumer_task_t *event_loop;
    std::shared_ptr<smartspeech::grpc::client> smartspeech_grpc_client;
  };

 public:
  explicit channel(const params &params);
  ~channel();

  mrcp_engine_channel_t *mrcp_channel() const;

  void on_voice(uint8_t *voice_buffer, size_t length);
  apt_bool_t on_open();
  apt_bool_t on_close();
  apt_bool_t on_message(mrcp_message_t *request);
  void on_result(const smartspeech::grpc::recognition::result &result);
  void on_error(const std::string &error_msg);

 private:
  void send_mrcp_response(mrcp_message_t *response);
  void send_start_of_input_event();
  void send_result_event(const std::string &result_message);
  void send_error_event(const std::string &error_msg);
  void update_mrcp_params(mrcp_message_t *request);
  void start_recognize();
  void shedule_stop_recognize();
  void stop_recognize();

  void process_set_params(mrcp_message_t *request);
  void process_define_grammar(mrcp_message_t *request);
  void process_recognize(mrcp_message_t *request);
  void process_stop(mrcp_message_t *request);

 private:
  apr_pool_t *pool_;
  apt_consumer_task_t *event_loop_;
  mrcp_engine_channel_t *mrcp_channel_;

  std::shared_ptr<smartspeech::grpc::client> smartspeech_grpc_client_;
  smartspeech::grpc::recognition::connection *smartspeech_grpc_recognition_connection_;

  enum class state { idle, waiting_vad, recognizing } state_;

  struct mrcp_params {
    int32_t no_input_timeout_ms;
    int32_t max_speech_timeout_ms;
  };
  mrcp_params mrcp_params_;

  /* active requests */
  mrcp_message_t *define_grammar_request_;
  mrcp_message_t *recognize_request_;

  std::vector<uint8_t> audio_buffer_;
};
}  // namespace smartspeech::mrcp::recognition
