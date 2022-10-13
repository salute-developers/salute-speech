#pragma once

#include <functional>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct event_loop_msg_t event_loop_msg_t;

#ifdef __cplusplus
}
#endif

struct event_loop_msg_t {
  void *func;
};