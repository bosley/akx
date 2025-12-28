#include "fs_module.h"

akx_cell_t *fs_module_init_impl(akx_runtime_ctx_t *rt, akx_cell_t *args) {
  fs_ensure_init(rt);
  akx_cell_t *result = akx_rt_alloc_cell(rt, AKX_TYPE_SYMBOL);
  akx_rt_set_symbol(rt, result, "fs-initialized");
  return result;
}
