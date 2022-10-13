#include "cimpl.h"

#include "engine.hpp"

mrcp_engine_t *smartspeech_recognize_engine_create(apr_pool_t *pool) {
  auto *e = new smartspeech::mrcp::recognition::engine(pool);
  return e->mrcp_engine();
}