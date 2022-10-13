#include "engine.hpp"

#include "cimpl.h"
#include "event_loop.hpp"

#ifdef __cplusplus
extern "C" {
#endif

#include <mrcp_engine_impl.h>

#ifdef __cplusplus
}
#endif

#include "channel.hpp"

#ifdef __cplusplus
extern "C" {
#endif

static apt_bool_t event_loop_proceed(apt_task_t *task, apt_task_msg_t *msg);

static apt_bool_t smartspeech_mrcp_synthesis_engine_open(mrcp_engine_t *mrcp_engine);
static apt_bool_t smartspeech_mrcp_synthesis_engine_destroy(mrcp_engine_t *mrcp_engine);
static apt_bool_t smartspeech_mrcp_synthesis_engine_close(mrcp_engine_t *mrcp_engine);
static mrcp_engine_channel_t *smartspeech_mrcp_synthesis_channel_create(mrcp_engine_t *mrcp_engine, apr_pool_t *pool);

static struct mrcp_engine_method_vtable_t smartspeech_recognize_engine_vtable = {
    smartspeech_mrcp_synthesis_engine_destroy, smartspeech_mrcp_synthesis_engine_open,
    smartspeech_mrcp_synthesis_engine_close, smartspeech_mrcp_synthesis_channel_create};

#ifdef __cplusplus
}
#endif

// event loop
apt_bool_t event_loop_proceed(apt_task_t *task, apt_task_msg_t *msg) {
  auto *event_loop_msg = reinterpret_cast<event_loop_msg_t *>(msg->data);
  auto *func = static_cast<std::function<void(void)> *>(event_loop_msg->func);
  (*func)();
  delete func;

  return TRUE;
}

apt_bool_t smartspeech_mrcp_synthesis_engine_open(mrcp_engine_t *mrcp_engine) {
  if (!mrcp_engine->config) {
    apt_log(APT_LOG_MARK, APT_PRIO_ERROR, "unimrcp config not found");
    return FALSE;
  }
  apr_table_t *engine_config = mrcp_engine->config->params;

  const char *smartspeech_url = apr_table_get(engine_config, "smartspeech-url");
  if (!smartspeech_url) {
    apt_log(APT_LOG_MARK, APT_PRIO_ERROR, "smartspeech url not found");
    return FALSE;
  }
  const char *smartmarket_url = apr_table_get(engine_config, "smartmarket-url");
  if (!smartmarket_url) {
    apt_log(APT_LOG_MARK, APT_PRIO_ERROR, "smartmarket url not found");
    return FALSE;
  }

  const char *smartmarket_client_id = apr_table_get(engine_config, "smartmarket-client-id");
  if (!smartmarket_client_id) {
    apt_log(APT_LOG_MARK, APT_PRIO_ERROR, "smartmarket client id not found");
    return FALSE;
  }

  auto *smartspeech_synthesis_engine = static_cast<smartspeech::mrcp::synthesis::engine *>(mrcp_engine->obj);
  if (!smartspeech_synthesis_engine) {
    apt_log(APT_LOG_MARK, APT_PRIO_ERROR, "smartspeech mrcp synthesis engine not found");
    return FALSE;
  }

  const char *smartmarket_secret = apr_table_get(engine_config, "smartmarket-secret");
  if (!smartmarket_secret) {
    apt_log(APT_LOG_MARK, APT_PRIO_ERROR, "smartmarket secret not found");
    return FALSE;
  }

  const char *smartmarket_scope = apr_table_get(engine_config, "smartmarket-scope");
  const char *smartspeech_token = apr_table_get(engine_config, "smartspeech-token");

  smartspeech::mrcp::synthesis::engine::config config{};
  config.smartspeech_url = smartspeech_url;
  config.smartmarket_url = smartmarket_url;
  config.smartmarket_client_id = smartmarket_client_id;
  config.smartmarket_secret = smartmarket_secret;
  config.smartmarket_scope = (smartmarket_scope) ? smartmarket_scope : "SMART_SPEECH";
  config.smartspeech_token = (smartspeech_token) ? smartspeech_token : "";

  smartspeech_synthesis_engine->set_config(config);

  apt_log(APT_LOG_MARK, APT_PRIO_INFO, "SmartSpeech synthesis starting service!");
  if (!smartspeech_synthesis_engine->start_service()) {
    apt_log(APT_LOG_MARK, APT_PRIO_ERROR, "Cant get token for smartspeech");
    return FALSE;
  }
   
  apt_log(APT_LOG_MARK, APT_PRIO_INFO, "SmartSpeech synthesis engine opened!");
  return mrcp_engine_open_respond(mrcp_engine, TRUE);
}

apt_bool_t smartspeech_mrcp_synthesis_engine_destroy(mrcp_engine_t *mrcp_engine) {
  auto *smartspeech_synthesis_engine = static_cast<smartspeech::mrcp::synthesis::engine *>(mrcp_engine->obj);

  delete smartspeech_synthesis_engine;

  return TRUE;
}

apt_bool_t smartspeech_mrcp_synthesis_engine_close(mrcp_engine_t *mrcp_engine) {
  auto *smartspeech_synthesis_engine = static_cast<smartspeech::mrcp::synthesis::engine *>(mrcp_engine->obj);
  if (!smartspeech_synthesis_engine) {
    apt_log(APT_LOG_MARK, APT_PRIO_ERROR, "smartspeech mrcp synthesis engine not found");
    return FALSE;
  }
  smartspeech_synthesis_engine->stop_service();

  return mrcp_engine_close_respond(mrcp_engine);
}

mrcp_engine_channel_t *smartspeech_mrcp_synthesis_channel_create(mrcp_engine_t *mrcp_engine, apr_pool_t *pool) {
  auto *smartspeech_synthesis_engine = static_cast<smartspeech::mrcp::synthesis::engine *>(mrcp_engine->obj);
  if (!smartspeech_synthesis_engine) {
    apt_log(APT_LOG_MARK, APT_PRIO_ERROR, "smartspeech mrcp synthesis engine not found");
    return FALSE;
  }
  auto *smartspeech_synthesis_channel = smartspeech_synthesis_engine->create_channel(pool);

  return smartspeech_synthesis_channel->mrcp_channel();
}

namespace smartspeech::mrcp::synthesis {
engine::engine(apr_pool_t *pool)
    : pool_(pool)
    , event_loop_(nullptr)
    , mrcp_engine_(nullptr) {
  smartspeech::token_resolver::global_init();
  // start event loop thread and dispatcher
  auto *msg_pool = apt_task_msg_pool_create_dynamic(sizeof(event_loop_msg_t), pool);
  event_loop_ = apt_consumer_task_create(this, msg_pool, pool);
  auto *task = apt_consumer_task_base_get(event_loop_);
  apt_task_name_set(task, "synthesis");
  auto *vtable = apt_task_vtable_get(task);
  if (vtable) {
    vtable->process_msg = event_loop_proceed;
  }
  // start engine
  mrcp_engine_ = mrcp_engine_create(MRCP_SYNTHESIZER_RESOURCE, this, &::smartspeech_recognize_engine_vtable, pool);
}

engine::~engine() {
  if (event_loop_) {
    apt_task_t *task = apt_consumer_task_base_get(event_loop_);
    apt_task_destroy(task);
  }
  smartspeech::token_resolver::global_cleanup();
}

mrcp_engine_t *engine::mrcp_engine() const {
  return mrcp_engine_;
}

void engine::set_config(const config &config) {
  config_ = config;
}

bool engine::start_service() {
  token_resolver_ = std::make_unique<smartspeech::token_resolver>(
      config_.smartmarket_url, config_.smartmarket_client_id, config_.smartmarket_secret, config_.smartmarket_scope);

  smartspeech::grpc::client::params p{.host = config_.smartspeech_url, .token_resolver = *token_resolver_};
  smartspeech_grpc_client_ = std::make_shared<smartspeech::grpc::client>(p);
  
  if (event_loop_) {
    auto *task = apt_consumer_task_base_get(event_loop_);
    apt_task_start(task);
  }

  return true;
}

void engine::stop_service() {
  if (event_loop_) {
    apt_task_t *task = apt_consumer_task_base_get(event_loop_);
    apt_task_terminate(task, TRUE);
  }
}

channel *engine::create_channel(apr_pool_t *pool) {
  channel::params params{};
  params.mrcp_engine = mrcp_engine_;
  params.pool = pool;
  params.event_loop = event_loop_;
  params.smartspeech_grpc_client = smartspeech_grpc_client_;

  return new channel(params);
}
}  // namespace smartspeech::mrcp::synthesis
