#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <apt_consumer_task.h>
#include <mrcp_engine_types.h>

#ifdef __cplusplus
}
#endif

#include <memory>
#include <string>

#include "channel.hpp"
#include "grpc_client/client.hpp"
#include "grpc_client/token_resolver.hpp"

namespace smartspeech::mrcp::recognition {
class engine {
 public:
  struct config {
    std::string smartspeech_url;
    std::string smartmarket_url;
    std::string smartmarket_client_id;
    std::string smartmarket_secret;
    std::string smartmarket_scope;
    std::string smartspeech_token;
  };

 public:
  explicit engine(apr_pool_t *pool);
  ~engine();

  mrcp_engine_t *mrcp_engine() const;
  void set_config(const config &config);

  bool start_service();
  void stop_service();

  channel *create_channel(apr_pool_t *pool);

 private:
  apr_pool_t *pool_;
  apt_consumer_task_t *event_loop_;
  mrcp_engine_t *mrcp_engine_;

  config config_;

  std::unique_ptr<smartspeech::token_resolver> token_resolver_;
  std::shared_ptr<smartspeech::grpc::client> smartspeech_grpc_client_;
};
}  // namespace smartspeech::mrcp::recognition
