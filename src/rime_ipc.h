#pragma once

#include "rime_msgpack.h"

typedef struct rime_ipc_response_t {
    CommitType commit;
    ContextType context;
    StatusType status;

    template<typename T>
    void pack(T &pack) {
        pack(commit, context, status);
    }
} RimeIpcResponce;
