#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <apr_pools.h>
#include <mrcp_engine_types.h>

mrcp_engine_t *smartspeech_recognize_engine_create(apr_pool_t *pool);
#ifdef __cplusplus
}
#endif
