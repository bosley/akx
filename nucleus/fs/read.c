#include "fs_handles_internal.h"
#include <stdlib.h>
#include <string.h>

#define READ_BUFFER_SIZE 4096

akx_cell_t *file_read_impl(akx_runtime_ctx_t *rt, akx_cell_t *args) {
  if (!args) {
    akx_rt_error(rt, "fs/read: requires :fid argument");
    return NULL;
  }

  int fid = -1;
  int bytes = -1;
  int lines = -1;
  int offset = -1;
  int line_start = -1;
  int line_end = -1;

  akx_cell_t *current = args;
  while (current) {
    if (current->type != AKX_TYPE_SYMBOL) {
      akx_rt_error(rt, "fs/read: expected keyword");
      return NULL;
    }

    const char *keyword = akx_rt_cell_as_symbol(current);
    if (keyword[0] != ':') {
      akx_rt_error_fmt(rt, "fs/read: keywords must start with ':', got: %s",
                       keyword);
      return NULL;
    }

    current = akx_rt_cell_next(current);
    if (!current) {
      akx_rt_error_fmt(rt, "fs/read: missing value for keyword %s", keyword);
      return NULL;
    }

    if (strcmp(keyword, ":fid") == 0) {
      akx_cell_t *fid_cell =
          akx_rt_eval_and_assert(rt, current, AKX_TYPE_INTEGER_LITERAL,
                                 "fs/read: fid must be an integer");
      if (!fid_cell)
        return NULL;
      fid = akx_rt_cell_as_int(fid_cell);
      akx_rt_free_cell(rt, fid_cell);
    } else if (strcmp(keyword, ":bytes") == 0) {
      akx_cell_t *bytes_cell =
          akx_rt_eval_and_assert(rt, current, AKX_TYPE_INTEGER_LITERAL,
                                 "fs/read: bytes must be an integer");
      if (!bytes_cell)
        return NULL;
      bytes = akx_rt_cell_as_int(bytes_cell);
      akx_rt_free_cell(rt, bytes_cell);
    } else if (strcmp(keyword, ":lines") == 0) {
      akx_cell_t *lines_cell =
          akx_rt_eval_and_assert(rt, current, AKX_TYPE_INTEGER_LITERAL,
                                 "fs/read: lines must be an integer");
      if (!lines_cell)
        return NULL;
      lines = akx_rt_cell_as_int(lines_cell);
      akx_rt_free_cell(rt, lines_cell);
    } else if (strcmp(keyword, ":offset") == 0) {
      akx_cell_t *offset_cell =
          akx_rt_eval_and_assert(rt, current, AKX_TYPE_INTEGER_LITERAL,
                                 "fs/read: offset must be an integer");
      if (!offset_cell)
        return NULL;
      offset = akx_rt_cell_as_int(offset_cell);
      akx_rt_free_cell(rt, offset_cell);
    } else if (strcmp(keyword, ":line-start") == 0) {
      akx_cell_t *ls_cell =
          akx_rt_eval_and_assert(rt, current, AKX_TYPE_INTEGER_LITERAL,
                                 "fs/read: line-start must be an integer");
      if (!ls_cell)
        return NULL;
      line_start = akx_rt_cell_as_int(ls_cell);
      akx_rt_free_cell(rt, ls_cell);
    } else if (strcmp(keyword, ":line-end") == 0) {
      akx_cell_t *le_cell =
          akx_rt_eval_and_assert(rt, current, AKX_TYPE_INTEGER_LITERAL,
                                 "fs/read: line-end must be an integer");
      if (!le_cell)
        return NULL;
      line_end = akx_rt_cell_as_int(le_cell);
      akx_rt_free_cell(rt, le_cell);
    } else {
      akx_rt_error_fmt(rt, "fs/read: unknown keyword: %s", keyword);
      return NULL;
    }

    current = akx_rt_cell_next(current);
  }

  if (fid < 0) {
    akx_rt_error(rt, "fs/read: :fid keyword is required");
    return NULL;
  }

  file_handle_t *handle = akx_rt_scope_get(rt, fid_to_key(fid));
  if (!handle) {
    akx_rt_error_fmt(rt, "fs/read: invalid file ID %d", fid);
    return NULL;
  }

  if (offset >= 0) {
    if (fseek(handle->fp, offset, SEEK_SET) != 0) {
      akx_rt_error_fmt(rt, "fs/read: failed to seek to offset %d", offset);
      return NULL;
    }
  }

  char *result_str = NULL;
  size_t result_len = 0;

  if (line_start >= 0 || line_end >= 0) {
    if (line_start < 0)
      line_start = 0;
    if (line_end < 0)
      line_end = line_start;

    if (offset < 0) {
      fseek(handle->fp, 0, SEEK_SET);
    }

    char *buffer = malloc(READ_BUFFER_SIZE);
    if (!buffer) {
      akx_rt_error(rt, "fs/read: memory allocation failed");
      return NULL;
    }

    int current_line = 0;
    size_t capacity = READ_BUFFER_SIZE;
    result_str = malloc(capacity);
    if (!result_str) {
      free(buffer);
      akx_rt_error(rt, "fs/read: memory allocation failed");
      return NULL;
    }
    result_str[0] = '\0';

    while (fgets(buffer, READ_BUFFER_SIZE, handle->fp)) {
      if (current_line >= line_start && current_line <= line_end) {
        size_t line_len = strlen(buffer);
        if (result_len + line_len + 1 > capacity) {
          capacity *= 2;
          char *new_result = realloc(result_str, capacity);
          if (!new_result) {
            free(buffer);
            free(result_str);
            akx_rt_error(rt, "fs/read: memory allocation failed");
            return NULL;
          }
          result_str = new_result;
        }
        strcat(result_str, buffer);
        result_len += line_len;
      }
      current_line++;
      if (current_line > line_end)
        break;
    }
    free(buffer);
  } else if (lines > 0) {
    if (offset < 0) {
      fseek(handle->fp, 0, SEEK_SET);
    }

    char *buffer = malloc(READ_BUFFER_SIZE);
    if (!buffer) {
      akx_rt_error(rt, "fs/read: memory allocation failed");
      return NULL;
    }

    size_t capacity = READ_BUFFER_SIZE;
    result_str = malloc(capacity);
    if (!result_str) {
      free(buffer);
      akx_rt_error(rt, "fs/read: memory allocation failed");
      return NULL;
    }
    result_str[0] = '\0';

    int lines_read = 0;
    while (lines_read < lines && fgets(buffer, READ_BUFFER_SIZE, handle->fp)) {
      size_t line_len = strlen(buffer);
      if (result_len + line_len + 1 > capacity) {
        capacity *= 2;
        char *new_result = realloc(result_str, capacity);
        if (!new_result) {
          free(buffer);
          free(result_str);
          akx_rt_error(rt, "fs/read: memory allocation failed");
          return NULL;
        }
        result_str = new_result;
      }
      strcat(result_str, buffer);
      result_len += line_len;
      lines_read++;
    }
    free(buffer);
  } else if (bytes > 0) {
    result_str = malloc(bytes + 1);
    if (!result_str) {
      akx_rt_error(rt, "fs/read: memory allocation failed");
      return NULL;
    }
    size_t read_count = fread(result_str, 1, bytes, handle->fp);
    result_str[read_count] = '\0';
    result_len = read_count;
  } else {
    if (offset < 0) {
      fseek(handle->fp, 0, SEEK_SET);
    }

    long start_pos = ftell(handle->fp);
    fseek(handle->fp, 0, SEEK_END);
    long end_pos = ftell(handle->fp);
    long file_size = end_pos - start_pos;
    fseek(handle->fp, start_pos, SEEK_SET);

    result_str = malloc(file_size + 1);
    if (!result_str) {
      akx_rt_error(rt, "fs/read: memory allocation failed");
      return NULL;
    }
    size_t read_count = fread(result_str, 1, file_size, handle->fp);
    result_str[read_count] = '\0';
    result_len = read_count;
  }

  akx_cell_t *result = akx_rt_alloc_cell(rt, AKX_TYPE_STRING_LITERAL);
  akx_rt_set_string(rt, result, result_str);
  free(result_str);
  return result;
}
