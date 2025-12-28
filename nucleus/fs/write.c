#include "fs_handles_internal.h"
#include <stdlib.h>
#include <string.h>

akx_cell_t *file_write_impl(akx_runtime_ctx_t *rt, akx_cell_t *args) {
  if (!args) {
    akx_rt_error(rt, "fs/write: requires :fid and :content arguments");
    return NULL;
  }

  int fid = -1;
  char *content = NULL;
  int offset = -1;
  int append = 0;

  akx_cell_t *current = args;
  while (current) {
    if (current->type != AKX_TYPE_SYMBOL) {
      akx_rt_error(rt, "fs/write: expected keyword");
      if (content)
        free(content);
      return NULL;
    }

    const char *keyword = akx_rt_cell_as_symbol(current);
    if (keyword[0] != ':') {
      akx_rt_error_fmt(rt, "fs/write: keywords must start with ':', got: %s",
                       keyword);
      if (content)
        free(content);
      return NULL;
    }

    current = akx_rt_cell_next(current);
    if (!current) {
      akx_rt_error_fmt(rt, "fs/write: missing value for keyword %s", keyword);
      if (content)
        free(content);
      return NULL;
    }

    if (strcmp(keyword, ":fid") == 0) {
      akx_cell_t *fid_cell =
          akx_rt_eval_and_assert(rt, current, AKX_TYPE_INTEGER_LITERAL,
                                 "fs/write: fid must be an integer");
      if (!fid_cell) {
        if (content)
          free(content);
        return NULL;
      }
      fid = akx_rt_cell_as_int(fid_cell);
      akx_rt_free_cell(rt, fid_cell);
    } else if (strcmp(keyword, ":content") == 0) {
      akx_cell_t *content_cell =
          akx_rt_eval_and_assert(rt, current, AKX_TYPE_STRING_LITERAL,
                                 "fs/write: content must be a string");
      if (!content_cell) {
        if (content)
          free(content);
        return NULL;
      }
      content = strdup(akx_rt_cell_as_string(content_cell));
      akx_rt_free_cell(rt, content_cell);
    } else if (strcmp(keyword, ":offset") == 0) {
      akx_cell_t *offset_cell =
          akx_rt_eval_and_assert(rt, current, AKX_TYPE_INTEGER_LITERAL,
                                 "fs/write: offset must be an integer");
      if (!offset_cell) {
        if (content)
          free(content);
        return NULL;
      }
      offset = akx_rt_cell_as_int(offset_cell);
      akx_rt_free_cell(rt, offset_cell);
    } else if (strcmp(keyword, ":append") == 0) {
      akx_cell_t *append_cell = akx_rt_eval(rt, current);
      if (!append_cell) {
        if (content)
          free(content);
        return NULL;
      }
      // Check for truthy values: 1, "true", or non-zero integers
      if (append_cell->type == AKX_TYPE_INTEGER_LITERAL) {
        append = (akx_rt_cell_as_int(append_cell) != 0);
      } else if (append_cell->type == AKX_TYPE_SYMBOL) {
        const char *sym = akx_rt_cell_as_symbol(append_cell);
        append = (strcmp(sym, "true") == 0);
      }
      akx_rt_free_cell(rt, append_cell);
    } else {
      akx_rt_error_fmt(rt, "fs/write: unknown keyword: %s", keyword);
      if (content)
        free(content);
      return NULL;
    }

    current = akx_rt_cell_next(current);
  }

  if (fid < 0) {
    akx_rt_error(rt, "fs/write: :fid keyword is required");
    if (content)
      free(content);
    return NULL;
  }

  if (!content) {
    akx_rt_error(rt, "fs/write: :content keyword is required");
    return NULL;
  }

  file_handle_t *handle = akx_rt_scope_get(rt, fid_to_key(fid));
  if (!handle) {
    akx_rt_error_fmt(rt, "fs/write: invalid file ID %d", fid);
    free(content);
    return NULL;
  }

  if (offset >= 0) {
    if (fseek(handle->fp, offset, SEEK_SET) != 0) {
      akx_rt_error_fmt(rt, "fs/write: failed to seek to offset %d", offset);
      free(content);
      return NULL;
    }
  } else if (append) {
    fseek(handle->fp, 0, SEEK_END);
  }

  size_t content_len = strlen(content);
  size_t written = fwrite(content, 1, content_len, handle->fp);
  fflush(handle->fp);

  free(content);

  akx_cell_t *result = akx_rt_alloc_cell(rt, AKX_TYPE_INTEGER_LITERAL);
  akx_rt_set_int(rt, result, (int)written);
  return result;
}
