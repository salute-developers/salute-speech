#include "channel.hpp"

#include "event_loop.hpp"

#ifdef __cplusplus
extern "C" {
#endif

#include <mrcp_engine_impl.h>
#include <mrcp_generic_header.h>
#include <mrcp_recog_header.h>
#include <mrcp_recog_resource.h>

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
extern "C" {
#endif

static apt_bool_t smartspeech_mrcp_recognition_channel_destroy(mrcp_engine_channel_t *mrcp_channel);
static apt_bool_t smartspeech_mrcp_recognition_channel_open(mrcp_engine_channel_t *mrcp_channel);
static apt_bool_t smartspeech_mrcp_recognition_close(mrcp_engine_channel_t *mrcp_channel);
static apt_bool_t smartspeech_mrcp_recognition_process(mrcp_engine_channel_t *mrcp_channel, mrcp_message_t *request);

static const struct mrcp_engine_channel_method_vtable_t smartspeech_mrcp_recognition_channel_vtable = {
    smartspeech_mrcp_recognition_channel_destroy, smartspeech_mrcp_recognition_channel_open,
    smartspeech_mrcp_recognition_close, smartspeech_mrcp_recognition_process};

static apt_bool_t smartspeech_mrcp_recognition_audio_stream_destroy(mpf_audio_stream_t *stream);
static apt_bool_t smartspeech_mrcp_recognition_audio_stream_open(mpf_audio_stream_t *stream, mpf_codec_t *codec);
static apt_bool_t smartspeech_mrcp_recognition_audio_stream_close(mpf_audio_stream_t *stream);
static apt_bool_t smartspeech_mrcp_recognition_audio_stream_write(mpf_audio_stream_t *stream, const mpf_frame_t *frame);

const mpf_audio_stream_vtable_t smartspeech_mrcp_synthesis_audio_stream_vtable = {
    smartspeech_mrcp_recognition_audio_stream_destroy,
    NULL,
    NULL,
    NULL,
    smartspeech_mrcp_recognition_audio_stream_open,
    smartspeech_mrcp_recognition_audio_stream_close,
    smartspeech_mrcp_recognition_audio_stream_write,
    NULL};

#ifdef __cplusplus
}
#endif

apt_bool_t smartspeech_mrcp_recognition_channel_destroy(mrcp_engine_channel_t *mrcp_channel) {
  auto *channel = static_cast<smartspeech::mrcp::recognition::channel *>(mrcp_channel->method_obj);
  delete channel;

  return TRUE;
}

apt_bool_t smartspeech_mrcp_recognition_channel_open(mrcp_engine_channel_t *mrcp_channel) {
  auto *channel = static_cast<smartspeech::mrcp::recognition::channel *>(mrcp_channel->method_obj);
  if (!channel) {
    return FALSE;
  }
  return channel->on_open();
}
apt_bool_t smartspeech_mrcp_recognition_close(mrcp_engine_channel_t *mrcp_channel) {
  auto *channel = static_cast<smartspeech::mrcp::recognition::channel *>(mrcp_channel->method_obj);
  if (!channel) {
    return FALSE;
  }
  return channel->on_close();
}
apt_bool_t smartspeech_mrcp_recognition_process(mrcp_engine_channel_t *mrcp_channel, mrcp_message_t *request) {
  auto *channel = static_cast<smartspeech::mrcp::recognition::channel *>(mrcp_channel->method_obj);
  if (!channel) {
    return FALSE;
  }
  return channel->on_message(request);
}

apt_bool_t smartspeech_mrcp_recognition_audio_stream_destroy(mpf_audio_stream_t *stream) {
  return TRUE;
}
apt_bool_t smartspeech_mrcp_recognition_audio_stream_open(mpf_audio_stream_t *stream, mpf_codec_t *codec) {
  return TRUE;
}
apt_bool_t smartspeech_mrcp_recognition_audio_stream_close(mpf_audio_stream_t *stream) {
  return TRUE;
}
apt_bool_t smartspeech_mrcp_recognition_audio_stream_write(mpf_audio_stream_t *stream, const mpf_frame_t *frame) {
  auto *channel = static_cast<smartspeech::mrcp::recognition::channel *>(stream->obj);
  if (channel) {
    channel->on_voice(static_cast<uint8_t *>(frame->codec_frame.buffer), frame->codec_frame.size);
  }
  return TRUE;
}

namespace smartspeech::mrcp::recognition {
channel::channel(const params &params)
    : pool_(params.pool)
    , event_loop_(params.event_loop)
    , smartspeech_grpc_client_(params.smartspeech_grpc_client)
    , state_(state::idle)
    , define_grammar_request_(nullptr)
    , recognize_request_(nullptr) {
  mrcp_params_.no_input_timeout_ms = 5000;
  mrcp_params_.max_speech_timeout_ms = 15000;

  /* create termination for audio */
  mpf_stream_capabilities_t *capabilities;
  mpf_termination_t *termination;

  capabilities = mpf_sink_stream_capabilities_create(params.pool);
  mpf_codec_capabilities_add(&capabilities->codecs, MPF_SAMPLE_RATE_8000 | MPF_SAMPLE_RATE_16000, "LPCM");
  termination = mrcp_engine_audio_termination_create(this, &smartspeech_mrcp_synthesis_audio_stream_vtable,
                                                     capabilities, params.pool);

  mrcp_channel_ = mrcp_engine_channel_create(params.mrcp_engine, &smartspeech_mrcp_recognition_channel_vtable, this,
                                             termination, params.pool);
}

channel::~channel() {}

mrcp_engine_channel_t *channel::mrcp_channel() const {
  return mrcp_channel_;
}

void channel::on_voice(uint8_t *voice_buffer, size_t length) {
  if (state_ == state::idle) {
    return;
  }
  audio_buffer_.insert(audio_buffer_.end(), voice_buffer, voice_buffer + length);
  if (smartspeech_grpc_recognition_connection_ && audio_buffer_.size() >= 1600) {
    smartspeech_grpc_recognition_connection_->feed(audio_buffer_.data(), audio_buffer_.size());
    audio_buffer_.clear();
  }
}

apt_bool_t channel::on_open() {
  apt_task_t *task = apt_consumer_task_base_get(event_loop_);
  apt_task_msg_t *msg = apt_task_msg_get(task);
  if (msg) {
    msg->type = TASK_MSG_USER;
    auto event_loop_msg = reinterpret_cast<event_loop_msg_t *>(msg->data);
    event_loop_msg->func = new std::function<void(void)>{[mrcp_channel = mrcp_channel_] {
      mrcp_engine_channel_open_respond(mrcp_channel, TRUE);
    }};
    apt_task_msg_signal(task, msg);
  }

  return TRUE;
}

apt_bool_t channel::on_close() {
  apt_task_t *task = apt_consumer_task_base_get(event_loop_);
  apt_task_msg_t *msg = apt_task_msg_get(task);
  if (msg) {
    msg->type = TASK_MSG_USER;
    auto event_loop_msg = reinterpret_cast<event_loop_msg_t *>(msg->data);
    event_loop_msg->func = new std::function<void(void)>{[mrcp_channel = mrcp_channel_] {
      mrcp_engine_channel_close_respond(mrcp_channel);
    }};
    apt_task_msg_signal(task, msg);
  }

  return TRUE;
}

apt_bool_t channel::on_message(mrcp_message_t *request) {
  switch (request->start_line.method_id) {
    case RECOGNIZER_SET_PARAMS: {
      process_set_params(request);
      break;
    }
    case RECOGNIZER_DEFINE_GRAMMAR: {
      process_define_grammar(request);
      break;
    }
    case RECOGNIZER_RECOGNIZE: {
      process_recognize(request);
      break;
    }
    case RECOGNIZER_STOP: {
      process_stop(request);
      break;
    }
    default:
      auto default_response = mrcp_response_create(request, request->pool);
      send_mrcp_response(default_response);
      break;
  }

  return TRUE;
}

void channel::on_result(const smartspeech::grpc::recognition::result &result) {
  if (state_ == state::waiting_vad) {
    send_start_of_input_event();
    state_ = state::recognizing;
  }
  if (state_ == state::recognizing && result.eou) {
    shedule_stop_recognize();
    send_result_event(result.words);
  }
}

void channel::on_error(const std::string &error_msg) {
  shedule_stop_recognize();
  send_error_event(error_msg);
}

void channel::send_mrcp_response(mrcp_message_t *response) {
  apt_task_t *task = apt_consumer_task_base_get(event_loop_);
  apt_task_msg_t *msg = apt_task_msg_get(task);
  if (msg) {
    msg->type = TASK_MSG_USER;
    auto event_loop_msg = reinterpret_cast<event_loop_msg_t *>(msg->data);
    event_loop_msg->func = new std::function<void(void)>{[mrcp_channel = mrcp_channel_, response] {
      mrcp_engine_channel_message_send(mrcp_channel, response);
    }};
    apt_task_msg_signal(task, msg);
  }
}

void channel::send_start_of_input_event() {
  auto response = mrcp_event_create(recognize_request_, RECOGNIZER_START_OF_INPUT, recognize_request_->pool);
  send_mrcp_response(response);
}

void channel::send_result_event(const std::string &result_message) {
  mrcp_message_t *response =
      mrcp_event_create(recognize_request_, RECOGNIZER_RECOGNITION_COMPLETE, recognize_request_->pool);
  response->start_line.request_state = MRCP_REQUEST_STATE_COMPLETE;
  apt_str_t *content_id = &recognize_request_->body;

  char *body_str = apr_psprintf(response->pool,
                                "<?xml version=\"1.0\"?>\n"
                                "<result>\n"
                                "  <interpretation grammar=\"%s\" confidence=\"%d\">\n"
                                "    <input mode=\"%s\">%s</input>\n"
                                "    <instance>%s</instance>\n"
                                "  </interpretation>\n"
                                "</result>\n",
                                content_id->buf, 1, "speech", result_message.c_str(), result_message.c_str());
  apt_string_set(&response->body, body_str);

  mrcp_generic_header_t *generic_header = NULL;
  generic_header = mrcp_generic_header_prepare(response);
  if (generic_header) {
    apt_string_assign(&generic_header->content_type, "application/x-nlsml", response->pool);
    mrcp_generic_header_property_add(response, GENERIC_HEADER_CONTENT_TYPE);
  }

  send_mrcp_response(response);
}

void channel::send_error_event(const std::string &error_msg) {
  mrcp_message_t *response =
      mrcp_event_create(recognize_request_, RECOGNIZER_RECOGNITION_COMPLETE, recognize_request_->pool);
  response->start_line.request_state = MRCP_REQUEST_STATE_COMPLETE;
  mrcp_recog_header_t *resource_header = NULL;
  resource_header = static_cast<mrcp_recog_header_t *>(mrcp_resource_header_prepare(response));
  resource_header->completion_cause = RECOGNIZER_COMPLETION_CAUSE_ERROR;
  mrcp_resource_header_property_add(response, RECOGNIZER_HEADER_COMPLETION_CAUSE);

  if (!error_msg.empty()) {
    apt_string_assign(&resource_header->completion_reason, error_msg.c_str(), response->pool);
    mrcp_resource_header_property_add(response, RECOGNIZER_HEADER_COMPLETION_REASON);
  }

  send_mrcp_response(response);
}

void channel::update_mrcp_params(mrcp_message_t *request) {
  auto *recognition_header = static_cast<mrcp_recog_header_t *>(mrcp_resource_header_get(request));
  if (recognition_header) {
    if (mrcp_resource_header_property_check(request, RECOGNIZER_HEADER_NO_INPUT_TIMEOUT) &&
        recognition_header->no_input_timeout > 0) {
      mrcp_params_.no_input_timeout_ms = (recognition_header->no_input_timeout >= 1000)
                                             ? recognition_header->no_input_timeout
                                             : recognition_header->no_input_timeout * 1000;
    }
    if (mrcp_resource_header_property_check(request, RECOGNIZER_HEADER_RECOGNITION_TIMEOUT) &&
        recognition_header->recognition_timeout > 0) {
      mrcp_params_.max_speech_timeout_ms = (recognition_header->recognition_timeout >= 1000)
                                               ? recognition_header->recognition_timeout
                                               : recognition_header->recognition_timeout * 1000;
    }
  }
}

void channel::start_recognize() {
  smartspeech::grpc::recognition::connection::params p{};
  p.model = "ivr";
  p.sample_rate = 8000;
  p.enable_multi_utterance = false;
  p.enable_partial_results = true;
  p.hypotheses_count = 1;
  p.no_speech_timeout_ms = mrcp_params_.no_input_timeout_ms;
  p.max_speech_timeout_ms = mrcp_params_.max_speech_timeout_ms;

  smartspeech_grpc_recognition_connection_ = smartspeech_grpc_client_
                                                 ->start_recognition_connection(
                                                     p,
                                                     [this](const smartspeech::grpc::recognition::result &result) {
                                                       this->on_result(result);
                                                     },
                                                     [this](const std::string &error_msg) {
                                                       this->on_error(error_msg);
                                                     })
                                                 .release();
  state_ = state::waiting_vad;
}

void channel::shedule_stop_recognize() {
  apt_task_t *task = apt_consumer_task_base_get(event_loop_);
  apt_task_msg_t *msg = apt_task_msg_get(task);
  if (msg) {
    msg->type = TASK_MSG_USER;
    auto event_loop_msg = reinterpret_cast<event_loop_msg_t *>(msg->data);
    event_loop_msg->func = new std::function<void(void)>{[this] {
      this->stop_recognize();
    }};
    apt_task_msg_signal(task, msg);
  }
}

void channel::stop_recognize() {
  if (smartspeech_grpc_recognition_connection_) {
    smartspeech_grpc_recognition_connection_->writes_done();
    smartspeech_grpc_recognition_connection_->disable();
  }
}

void channel::process_set_params(mrcp_message_t *request) {
  update_mrcp_params(request);
  mrcp_message_t *response = mrcp_response_create(request, request->pool);
  send_mrcp_response(response);
}

void channel::process_define_grammar(mrcp_message_t *request) {
  define_grammar_request_ = request;
  mrcp_message_t *response = mrcp_response_create(request, request->pool);
  send_mrcp_response(response);
}

void channel::process_recognize(mrcp_message_t *request) {
  update_mrcp_params(request);
  recognize_request_ = request;
  start_recognize();
  mrcp_message_t *response = mrcp_response_create(request, request->pool);
  response->start_line.request_state = MRCP_REQUEST_STATE_INPROGRESS;
  send_mrcp_response(response);
}

void channel::process_stop(mrcp_message_t *request) {
  stop_recognize();
  mrcp_message_t *response = mrcp_response_create(request, response->pool);

  apt_task_t *task = apt_consumer_task_base_get(event_loop_);
  apt_task_msg_t *msg = apt_task_msg_get(task);
  if (msg) {
    msg->type = TASK_MSG_USER;
    auto event_loop_msg = reinterpret_cast<event_loop_msg_t *>(msg->data);
    event_loop_msg->func = new std::function<void(void)>{[mrcp_channel = mrcp_channel_, response] {
      mrcp_engine_channel_message_send(mrcp_channel, response);
    }};
    apt_task_msg_signal(task, msg);
  }
}
}  // namespace smartspeech::mrcp::recognition
