#include "channel.hpp"
#include <mutex>

#include "event_loop.hpp"

#ifdef __cplusplus
extern "C" {
#endif

#include <mrcp_engine_impl.h>
#include <mrcp_generic_header.h>
#include <mrcp_synth_header.h>
#include <mrcp_synth_resource.h>

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
extern "C" {
#endif

static apt_bool_t smartspeech_mrcp_synthesis_channel_destroy(mrcp_engine_channel_t *mrcp_channel);
static apt_bool_t smartspeech_mrcp_synthesis_channel_open(mrcp_engine_channel_t *mrcp_channel);
static apt_bool_t smartspeech_mrcp_synthesis_close(mrcp_engine_channel_t *mrcp_channel);
static apt_bool_t smartspeech_mrcp_synthesis_process(mrcp_engine_channel_t *mrcp_channel, mrcp_message_t *request);

static const struct mrcp_engine_channel_method_vtable_t smartspeech_mrcp_synthesis_channel_vtable = {
    smartspeech_mrcp_synthesis_channel_destroy, smartspeech_mrcp_synthesis_channel_open,
    smartspeech_mrcp_synthesis_close, smartspeech_mrcp_synthesis_process};

static apt_bool_t smartspeech_mrcp_synthesis_audio_stream_destroy(mpf_audio_stream_t *stream);
static apt_bool_t smartspeech_mrcp_synthesis_audio_stream_open(mpf_audio_stream_t *stream, mpf_codec_t *codec);
static apt_bool_t smartspeech_mrcp_synthesis_audio_stream_close(mpf_audio_stream_t *stream);
static apt_bool_t smartspeech_mrcp_synthesis_audio_stream_read(mpf_audio_stream_t *stream, mpf_frame_t *frame);

const mpf_audio_stream_vtable_t smartspeech_mrcp_synthesis_audio_stream_vtable = {
    smartspeech_mrcp_synthesis_audio_stream_destroy,
    NULL,
    NULL,
    smartspeech_mrcp_synthesis_audio_stream_read,
    smartspeech_mrcp_synthesis_audio_stream_open,
    smartspeech_mrcp_synthesis_audio_stream_close,
    NULL,
    NULL};

#ifdef __cplusplus
}
#endif

apt_bool_t smartspeech_mrcp_synthesis_channel_destroy(mrcp_engine_channel_t *mrcp_channel) {
  auto *channel = static_cast<smartspeech::mrcp::synthesis::channel *>(mrcp_channel->method_obj);
  delete channel;

  return TRUE;
}

apt_bool_t smartspeech_mrcp_synthesis_channel_open(mrcp_engine_channel_t *mrcp_channel) {
  auto *channel = static_cast<smartspeech::mrcp::synthesis::channel *>(mrcp_channel->method_obj);
  if (!channel) {
    return FALSE;
  }
  return channel->on_open();
}
apt_bool_t smartspeech_mrcp_synthesis_close(mrcp_engine_channel_t *mrcp_channel) {
  auto *channel = static_cast<smartspeech::mrcp::synthesis::channel *>(mrcp_channel->method_obj);
  if (!channel) {
    return FALSE;
  }
  return channel->on_close();
}
apt_bool_t smartspeech_mrcp_synthesis_process(mrcp_engine_channel_t *mrcp_channel, mrcp_message_t *request) {
  auto *channel = static_cast<smartspeech::mrcp::synthesis::channel *>(mrcp_channel->method_obj);
  if (!channel) {
    return FALSE;
  }
  return channel->on_message(request);
}

apt_bool_t smartspeech_mrcp_synthesis_audio_stream_destroy(mpf_audio_stream_t *stream) {
  return TRUE;
}
apt_bool_t smartspeech_mrcp_synthesis_audio_stream_open(mpf_audio_stream_t *stream, mpf_codec_t *codec) {
  return TRUE;
}
apt_bool_t smartspeech_mrcp_synthesis_audio_stream_close(mpf_audio_stream_t *stream) {
  return TRUE;
}
apt_bool_t smartspeech_mrcp_synthesis_audio_stream_read(mpf_audio_stream_t *stream, mpf_frame_t *frame) {
  auto *channel = static_cast<smartspeech::mrcp::synthesis::channel *>(stream->obj);
  if (channel) {
    channel->feel_voice_buffer(frame->codec_frame.buffer, frame->codec_frame.size);
    frame->type |= MEDIA_FRAME_TYPE_AUDIO;
    channel->check_synth_comlete();
  }
  return TRUE;
}

namespace smartspeech::mrcp::synthesis {
channel::channel(const params &params)
    : pool_(params.pool)
    , event_loop_(params.event_loop)
    , smartspeech_grpc_client_(params.smartspeech_grpc_client)
    , state_(state::idle)
    , synthesis_request_(nullptr)
    , synth_complete_flag_(false){
  mrcp_params_.voice_name = "May_LQ";

  /* create termination for audio */
  mpf_stream_capabilities_t *capabilities;
  mpf_termination_t *termination;

  capabilities = mpf_source_stream_capabilities_create(params.pool);
  mpf_codec_capabilities_add(&capabilities->codecs, MPF_SAMPLE_RATE_8000, "LPCM");
  termination = mrcp_engine_audio_termination_create(this, &smartspeech_mrcp_synthesis_audio_stream_vtable,
                                                     capabilities, params.pool);

  mrcp_channel_ = mrcp_engine_channel_create(params.mrcp_engine, &smartspeech_mrcp_synthesis_channel_vtable, this,
                                             termination, params.pool);
}

channel::~channel() {
  debug_pcm_file_.close();
}

mrcp_engine_channel_t *channel::mrcp_channel() const {
  return mrcp_channel_;
}

void channel::feel_voice_buffer(void *buffer, size_t length) {
  if (state_ == state::synthesis) {
    std::lock_guard<std::mutex> l(voice_buffer_m_);
    if (voice_buffer_.size() > length) {
      memcpy(buffer, voice_buffer_.data(), length);
      voice_buffer_.erase(voice_buffer_.begin(), voice_buffer_.begin() + length);
    } else {
      size_t available = voice_buffer_.size();
      memcpy(buffer, voice_buffer_.data(), available);
      voice_buffer_.clear();
    }
  }
}

void channel::check_synth_comlete() {
  if (state_ == state::synthesis) {
    std::lock_guard<std::mutex> l(voice_buffer_m_);
    if (synth_complete_flag_ && voice_buffer_.empty()) {
      shedule_stop_synthesis();
      send_result_event();
    }
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
    case SYNTHESIZER_SET_PARAMS: {
      process_set_params(request);
      break;
    }
    case SYNTHESIZER_SPEAK: {
      process_synthesis(request);
      break;
    }
    case SYNTHESIZER_STOP:
    case SYNTHESIZER_BARGE_IN_OCCURRED:
      process_stop(request);
      break;
    default:
      auto default_response = mrcp_response_create(request, request->pool);
      send_mrcp_response(default_response);
      break;
  }

  return TRUE;
}

void channel::on_voice(const smartspeech::grpc::synthesis::result &result) {
  if (!result.end) {
    synth_complete_flag_ = false;
    std::lock_guard<std::mutex> l(voice_buffer_m_);
    voice_buffer_.insert(voice_buffer_.end(), result.buffer.begin(), result.buffer.end());
  } else {
    synth_complete_flag_ = true;
  }
}

void channel::on_error(const std::string &error_msg) {
  shedule_stop_synthesis();
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

void channel::send_result_event() {
  if (!synthesis_request_) {
    std::cerr << "error: send_result_event: synthesis_request == null\n";
    return;
  }
  auto response = mrcp_event_create(synthesis_request_, SYNTHESIZER_SPEAK_COMPLETE, synthesis_request_->pool);
  response->start_line.request_state = MRCP_REQUEST_STATE_COMPLETE;
  auto header = static_cast<mrcp_synth_header_t *>(mrcp_resource_header_prepare(response));
  if (header) {
    header->completion_cause = SYNTHESIZER_COMPLETION_CAUSE_NORMAL;
    mrcp_resource_header_property_add(response, SYNTHESIZER_HEADER_COMPLETION_CAUSE);
  }
  send_mrcp_response(response);
}

void channel::send_error_event(const std::string &error_msg) { 
  if (!synthesis_request_) {
    std::cerr << "error: send_error_event: synthesis_request == null\n";
    return;
  }
  mrcp_message_t *response =
      mrcp_event_create(synthesis_request_, SYNTHESIZER_SPEAK_COMPLETE, synthesis_request_->pool);
  response->start_line.request_state = MRCP_REQUEST_STATE_COMPLETE;
  mrcp_synth_header_t *resource_header = NULL;
  resource_header = static_cast<mrcp_synth_header_t *>(mrcp_resource_header_prepare(response));
  resource_header->completion_cause = SYNTHESIZER_COMPLETION_CAUSE_ERROR;
  mrcp_resource_header_property_add(response, SYNTHESIZER_HEADER_COMPLETION_CAUSE);

  if (!error_msg.empty()) {
    apt_string_assign(&resource_header->completion_reason, error_msg.c_str(), response->pool);
    mrcp_resource_header_property_add(response, SYNTHESIZER_HEADER_COMPLETION_REASON);
  }

  send_mrcp_response(response);
}

void channel::update_mrcp_params(mrcp_message_t *request) {
  // todo: get voice name from request
}

void channel::start_synthesis(const std::string &text, bool is_ssml) {
  smartspeech::grpc::synthesis::connection::params p{};
  p.text = text;
  p.is_ssml = is_ssml;
  smartspeech_grpc_synthesis_connection_ = smartspeech_grpc_client_
                                               ->start_synth_connection(
                                                   p,
                                                   [this](const smartspeech::grpc::synthesis::result &result) {
                                                     this->on_voice(result);
                                                   },
                                                   [this](const std::string &error_msg) {
                                                     this->on_error(error_msg);
                                                   })
                                               .release();
  state_ = state::synthesis;
}

void channel::shedule_stop_synthesis() {
  apt_task_t *task = apt_consumer_task_base_get(event_loop_);
  apt_task_msg_t *msg = apt_task_msg_get(task);
  if (msg) {
    msg->type = TASK_MSG_USER;
    auto event_loop_msg = reinterpret_cast<event_loop_msg_t *>(msg->data);
    event_loop_msg->func = new std::function<void(void)>{[this] {
      this->stop_synthesis();
    }};
    apt_task_msg_signal(task, msg);
  }
}

void channel::stop_synthesis() {
  if (smartspeech_grpc_synthesis_connection_) {
    smartspeech_grpc_synthesis_connection_->disable();
  }
  state_ = state::idle;
}

void channel::process_set_params(mrcp_message_t *request) {
  update_mrcp_params(request);
  mrcp_message_t *response = mrcp_response_create(request, request->pool);
  send_mrcp_response(response);
}

void channel::process_synthesis(mrcp_message_t *request) {
  update_mrcp_params(request);
  synthesis_request_ = request;
  std::string text = request->body.buf;
  bool is_ssml = false;
  mrcp_generic_header_t *generic_header = mrcp_generic_header_get(request);
  if (generic_header && mrcp_generic_header_property_check(request, GENERIC_HEADER_CONTENT_TYPE) == TRUE) {
    const char *content_type = generic_header->content_type.buf;
    is_ssml = (strstr(content_type, "ssml")) ? true : false;
  }
  start_synthesis(text, is_ssml);
  mrcp_message_t *response = mrcp_response_create(request, request->pool);
  response->start_line.request_state = MRCP_REQUEST_STATE_INPROGRESS;
  send_mrcp_response(response);
}

void channel::process_stop(mrcp_message_t *request) {
  stop_synthesis();
  mrcp_message_t *response = mrcp_response_create(request, request->pool);

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
}  // namespace smartspeech::mrcp::synthesis
