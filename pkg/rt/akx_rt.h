#ifndef AKX_RT_H
#define AKX_RT_H

#include "akx_cell.h"
#include "akx_sv.h"
#include <ak24/context.h>
#include <ak24/kernel.h>
#include <ak24/list.h>

typedef struct akx_rt_error_ctx_t akx_rt_error_ctx_t;

typedef struct akx_runtime_ctx_t akx_runtime_ctx_t;

typedef list_t(akx_cell_t *) akx_cell_list_t;

akx_runtime_ctx_t *akx_runtime_init(void);

void akx_runtime_deinit(akx_runtime_ctx_t *ctx);

int akx_runtime_start(akx_runtime_ctx_t *ctx, akx_cell_list_t *cells);

ak_context_t *akx_runtime_get_current_scope(akx_runtime_ctx_t *ctx);

#endif
