#include "fs_handles_internal.h"
#include <stdlib.h>
#include <string.h>

akx_cell_t *tell_impl(akx_runtime_ctx_t *rt, akx_cell_t *args) {
  if (!args) {
    akx_rt_error(rt, "fs/tell: requires :fid argument");
    return NULL;
  }

  int fid = -1;

  akx_cell_t *current = args;
  while (current) {
    if (current->type != AKX_TYPE_SYMBOL) {
      akx_rt_error(rt, "fs/tell: expected keyword");
      return NULL;
    }

    const char *keyword = akx_rt_cell_as_symbol(current);
    if (keyword[0] != ':') {
      akx_rt_error_fmt(rt, "fs/tell: keywords must start with ':', got: %s",
                       keyword);
      return NULL;
    }

    current = akx_rt_cell_next(current);
    if (!current) {
      akx_rt_error_fmt(rt, "fs/tell: missing value for keyword %s", keyword);
      return NULL;
    }

    if (strcmp(keyword, ":fid") == 0) {
      akx_cell_t *fid_cell =
          akx_rt_eval_and_assert(rt, current, AKX_TYPE_INTEGER_LITERAL,
                                 "fs/tell: fid must be an integer");
      if (!fid_cell)
        return NULL;
      fid = akx_rt_cell_as_int(fid_cell);
      akx_rt_free_cell(rt, fid_cell);
    } else {
      akx_rt_error_fmt(rt, "fs/tell: unknown keyword: %s", keyword);
      return NULL;
    }

    current = akx_rt_cell_next(current);
  }

  if (fid < 0) {
    akx_rt_error(rt, "fs/tell: :fid keyword is required");
    return NULL;
  }

  file_handle_t *handle = akx_rt_scope_get(rt, fid_to_key(fid));
  if (!handle) {
    akx_rt_error_fmt(rt, "fs/tell: invalid file ID %d", fid);
    return NULL;
  }

  long pos = ftell(handle->fp);
  if (pos < 0) {
    akx_rt_error(rt, "fs/tell: failed to get file position");
    return NULL;
  }

  handle->position = pos;

  akx_cell_t *result = akx_rt_alloc_cell(rt, AKX_TYPE_INTEGER_LITERAL);
  akx_rt_set_int(rt, result, (int)pos);
  return result;
}
