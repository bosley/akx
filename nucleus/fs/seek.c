#include "fs_handles_internal.h"
#include <stdlib.h>
#include <string.h>

akx_cell_t *seek_impl(akx_runtime_ctx_t *rt, akx_cell_t *args) {
  if (!args) {
    akx_rt_error(rt, "fs/seek: requires :fid, :offset, and :whence arguments");
    return NULL;
  }

  int fid = -1;
  int offset = 0;
  int whence = SEEK_SET;

  akx_cell_t *current = args;
  while (current) {
    if (current->type != AKX_TYPE_SYMBOL) {
      akx_rt_error(rt, "fs/seek: expected keyword");
      return NULL;
    }

    const char *keyword = akx_rt_cell_as_symbol(current);
    if (keyword[0] != ':') {
      akx_rt_error_fmt(rt, "fs/seek: keywords must start with ':', got: %s",
                       keyword);
      return NULL;
    }

    current = akx_rt_cell_next(current);
    if (!current) {
      akx_rt_error_fmt(rt, "fs/seek: missing value for keyword %s", keyword);
      return NULL;
    }

    if (strcmp(keyword, ":fid") == 0) {
      akx_cell_t *fid_cell =
          akx_rt_eval_and_assert(rt, current, AKX_TYPE_INTEGER_LITERAL,
                                 "fs/seek: fid must be an integer");
      if (!fid_cell)
        return NULL;
      fid = akx_rt_cell_as_int(fid_cell);
      akx_rt_free_cell(rt, fid_cell);
    } else if (strcmp(keyword, ":offset") == 0) {
      akx_cell_t *offset_cell =
          akx_rt_eval_and_assert(rt, current, AKX_TYPE_INTEGER_LITERAL,
                                 "fs/seek: offset must be an integer");
      if (!offset_cell)
        return NULL;
      offset = akx_rt_cell_as_int(offset_cell);
      akx_rt_free_cell(rt, offset_cell);
    } else if (strcmp(keyword, ":whence") == 0) {
      // Don't evaluate - args are passed unevaluated
      if (current->type != AKX_TYPE_SYMBOL) {
        akx_rt_error(rt, "fs/seek: whence must be a symbol (set, cur, or end)");
        return NULL;
      }
      const char *whence_str = akx_rt_cell_as_symbol(current);
      if (strcmp(whence_str, "set") == 0) {
        whence = SEEK_SET;
      } else if (strcmp(whence_str, "cur") == 0) {
        whence = SEEK_CUR;
      } else if (strcmp(whence_str, "end") == 0) {
        whence = SEEK_END;
      } else {
        akx_rt_error_fmt(
            rt, "fs/seek: invalid whence value '%s' (must be set, cur, or end)",
            whence_str);
        return NULL;
      }
    } else {
      akx_rt_error_fmt(rt, "fs/seek: unknown keyword: %s", keyword);
      return NULL;
    }

    current = akx_rt_cell_next(current);
  }

  if (fid < 0) {
    akx_rt_error(rt, "fs/seek: :fid keyword is required");
    return NULL;
  }

  file_handle_t *handle = akx_rt_scope_get(rt, fid_to_key(fid));
  if (!handle) {
    akx_rt_error_fmt(rt, "fs/seek: invalid file ID %d", fid);
    return NULL;
  }

  if (fseek(handle->fp, offset, whence) != 0) {
    akx_rt_error_fmt(rt, "fs/seek: failed to seek to offset %d with whence %d",
                     offset, whence);
    return NULL;
  }

  long new_pos = ftell(handle->fp);
  handle->position = new_pos;

  akx_cell_t *result = akx_rt_alloc_cell(rt, AKX_TYPE_INTEGER_LITERAL);
  akx_rt_set_int(rt, result, (int)new_pos);
  return result;
}
