#ifndef AKX_REPL_H
#define AKX_REPL_H

#include "akx_core.h"
#include "akx_rt.h"

extern volatile int keep_running;
extern volatile int signal_count;

int akx_repl_start(akx_core_t *core, akx_runtime_ctx_t *runtime);

#endif
