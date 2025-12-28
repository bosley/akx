#include "fs_module.h"
#include <stdlib.h>
#include <string.h>

akx_cell_t *file_write_impl(akx_runtime_ctx_t *rt, akx_cell_t *args) {
  if (!args) {
    akx_rt_error(rt, "fs/write: requires file ID and content");
    return NULL;
  }

  int fid = -1;
  const char *content = NULL;
  long offset = -1;
  int append_mode = 0;

  akx_cell_t *current = args;
  while (current) {
    if (current->type != AKX_TYPE_SYMBOL) {
      akx_rt_error(rt, "fs/write: expected keyword (symbol starting with ':')");
      return NULL;
    }

    const char *keyword = akx_rt_cell_as_symbol(current);
    if (keyword[0] != ':') {
      akx_rt_error_fmt(rt, "fs/write: keywords must start with ':', got: %s",
                       keyword);
      return NULL;
    }

    current = akx_rt_cell_next(current);
    if (!current) {
      akx_rt_error_fmt(rt, "fs/write: missing value for keyword %s", keyword);
      return NULL;
    }

    if (strcmp(keyword, ":fid") == 0) {
      akx_cell_t *fid_cell =
          akx_rt_eval_and_assert(rt, current, AKX_TYPE_INTEGER_LITERAL,
                                 "fs/write: fid must be an integer");
      if (!fid_cell) {
        return NULL;
      }
      fid = akx_rt_cell_as_int(fid_cell);
      akx_rt_free_cell(rt, fid_cell);
    } else if (strcmp(keyword, ":content") == 0) {
      akx_cell_t *content_cell =
          akx_rt_eval_and_assert(rt, current, AKX_TYPE_STRING_LITERAL,
                                 "fs/write: content must be a string");
      if (!content_cell) {
        return NULL;
      }
      content = akx_rt_cell_as_string(content_cell);
      akx_rt_free_cell(rt, content_cell);
    } else if (strcmp(keyword, ":offset") == 0) {
      akx_cell_t *offset_cell =
          akx_rt_eval_and_assert(rt, current, AKX_TYPE_INTEGER_LITERAL,
                                 "fs/write: offset must be an integer");
      if (!offset_cell) {
        return NULL;
      }
      offset = akx_rt_cell_as_int(offset_cell);
      akx_rt_free_cell(rt, offset_cell);
    } else if (strcmp(keyword, ":append") == 0) {
      akx_cell_t *append_cell = akx_rt_eval(rt, current);
      if (!append_cell) {
        return NULL;
      }
      if (append_cell->type == AKX_TYPE_SYMBOL) {
        const char *sym = akx_rt_cell_as_symbol(append_cell);
        append_mode = (strcmp(sym, "true") == 0 || strcmp(sym, "t") == 0);
      } else if (append_cell->type == AKX_TYPE_INTEGER_LITERAL) {
        append_mode = (akx_rt_cell_as_int(append_cell) != 0);
      }
      akx_rt_free_cell(rt, append_cell);
    } else {
      akx_rt_error_fmt(rt, "fs/write: unknown keyword: %s", keyword);
      return NULL;
    }

    current = akx_rt_cell_next(current);
  }

  if (fid < 0) {
    akx_rt_error(rt, "fs/write: :fid keyword is required");
    return NULL;
  }

  if (!content) {
    akx_rt_error(rt, "fs/write: :content keyword is required");
    return NULL;
  }

  file_handle_t *handle = fs_get_handle(rt, fid);
  if (!handle) {
    akx_rt_error_fmt(rt, "fs/write: invalid file ID %d", fid);
    return NULL;
  }

  if (append_mode) {
    if (fseek(handle->fp, 0, SEEK_END) != 0) {
      akx_rt_error(rt, "fs/write: failed to seek to end of file");
      return NULL;
    }
  } else if (offset >= 0) {
    if (fseek(handle->fp, offset, SEEK_SET) != 0) {
      akx_rt_error_fmt(rt, "fs/write: failed to seek to offset %ld", offset);
      return NULL;
    }
  }

  size_t content_len = strlen(content);
  size_t written = fwrite(content, 1, content_len, handle->fp);

  if (written != content_len) {
    akx_rt_error_fmt(rt,
                     "fs/write: failed to write all content (wrote %zu of %zu "
                     "bytes)",
                     written, content_len);
    return NULL;
  }

  fflush(handle->fp);
  handle->position = ftell(handle->fp);

  akx_cell_t *result = akx_rt_alloc_cell(rt, AKX_TYPE_INTEGER_LITERAL);
  akx_rt_set_int(rt, result, (int)written);
  return result;
}
