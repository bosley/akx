#ifndef AKX_REPL_H
#define AKX_REPL_H

#include "akx_core.h"
#include "akx_rt.h"

typedef struct {
  volatile int *keep_running;
  volatile int *signal_count;
} akx_repl_signal_ctx_t;

int akx_repl_start(akx_core_t *core, akx_runtime_ctx_t *runtime,
                   akx_repl_signal_ctx_t *signal_ctx);

#endif
