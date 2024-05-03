#pragma once

#include <rime_api.h>

#ifdef __cplusplus
extern "C" {
#endif

#define RIME_MSGPACK_TYPE void

typedef struct rime_msgpack_api_t {
    int data_size;

    void (*commit_msgpack)(RimeSessionId session_id, RIME_MSGPACK_TYPE* commit_data);
    void (*context_msgpack)(RimeSessionId session_id, RIME_MSGPACK_TYPE* context_data);
    void (*status_msgpack)(RimeSessionId session_id, RIME_MSGPACK_TYPE* status_data);
} RimeMsgpackApi;

#ifdef __cplusplus
}
#endif
