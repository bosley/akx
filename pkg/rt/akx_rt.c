#include "akx_rt.h"

struct akx_rt_error_ctx_t {
  int error_count;
  akx_parse_error_t *errors;
};

struct akx_runtime_ctx_t {
  int initialized;
  akx_rt_error_ctx_t *error_ctx;
  ak_context_t *context;
};

akx_runtime_ctx_t *akx_runtime_init(void) {
  akx_runtime_ctx_t *ctx = AK24_ALLOC(sizeof(akx_runtime_ctx_t));
  if (!ctx) {
    AK24_LOG_ERROR("Failed to allocate runtime context");
    return NULL;
  }

  ctx->initialized = 1;
  ctx->error_ctx = AK24_ALLOC(sizeof(akx_rt_error_ctx_t));
  if (!ctx->error_ctx) {
    AK24_LOG_ERROR("Failed to allocate error context");
    AK24_FREE(ctx);
    return NULL;
  }

  ctx->error_ctx->error_count = 0;
  ctx->error_ctx->errors = NULL;

  ctx->context = ak_context_new();
  if (!ctx->context) {
    AK24_LOG_ERROR("Failed to create runtime context");
    AK24_FREE(ctx->error_ctx);
    AK24_FREE(ctx);
    return NULL;
  }

  AK24_LOG_INFO("AKX runtime initialized");

  return ctx;
}

void akx_runtime_deinit(akx_runtime_ctx_t *ctx) {
  if (!ctx) {
    return;
  }

  if (ctx->context) {
    ak_context_free(ctx->context);
  }

  if (ctx->error_ctx) {
    akx_parse_error_t *err = ctx->error_ctx->errors;
    while (err) {
      akx_parse_error_t *next = err->next;
      AK24_FREE(err);
      err = next;
    }
    AK24_FREE(ctx->error_ctx);
  }

  AK24_LOG_INFO("AKX runtime deinitialized");

  AK24_FREE(ctx);
}

int akx_runtime_start(akx_runtime_ctx_t *ctx, akx_cell_list_t *cells) {
  if (!ctx) {
    AK24_LOG_ERROR("Runtime context is NULL");
    return -1;
  }

  if (!cells) {
    AK24_LOG_ERROR("Cell list is NULL");
    return -1;
  }

  AK24_LOG_INFO("Runtime start called with %zu cells", list_count(cells));

  return 0;
}

ak_context_t *akx_runtime_get_current_scope(akx_runtime_ctx_t *ctx) {
  if (!ctx) {
    return NULL;
  }
  return ctx->context;
}
