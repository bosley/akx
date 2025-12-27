#include <ak24/buffer.h>
#include <string.h>

akx_cell_t *return_quote(akx_runtime_ctx_t *rt, akx_cell_t *args) {
  (void)args;
  akx_cell_t *quoted = akx_rt_alloc_cell(rt, AKX_TYPE_QUOTED);
  if (!quoted) {
    return NULL;
  }
  
  const char *content = "test-symbol";
  size_t len = strlen(content);
  quoted->value.quoted_literal = ak_buffer_new(len + 1);
  if (!quoted->value.quoted_literal) {
    akx_rt_free_cell(rt, quoted);
    return NULL;
  }
  
  ak_buffer_copy_to(quoted->value.quoted_literal, (uint8_t *)content, len);
  return quoted;
}

