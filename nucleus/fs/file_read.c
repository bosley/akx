#include "fs_module.h"
#include <stdlib.h>
#include <string.h>

akx_cell_t *file_read_impl(akx_runtime_ctx_t *rt, akx_cell_t *args) {
  if (!args) {
    akx_rt_error(rt, "fs/read: requires a file ID");
    return NULL;
  }

  int fid = -1;
  int bytes_to_read = -1;
  int lines_to_read = -1;
  long offset = -1;
  int line_start = -1;
  int line_end = -1;

  akx_cell_t *current = args;
  while (current) {
    if (current->type != AKX_TYPE_SYMBOL) {
      akx_rt_error(rt, "fs/read: expected keyword (symbol starting with ':')");
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
      if (!fid_cell) {
        return NULL;
      }
      fid = akx_rt_cell_as_int(fid_cell);
      akx_rt_free_cell(rt, fid_cell);
    } else if (strcmp(keyword, ":bytes") == 0) {
      akx_cell_t *bytes_cell =
          akx_rt_eval_and_assert(rt, current, AKX_TYPE_INTEGER_LITERAL,
                                 "fs/read: bytes must be an integer");
      if (!bytes_cell) {
        return NULL;
      }
      bytes_to_read = akx_rt_cell_as_int(bytes_cell);
      akx_rt_free_cell(rt, bytes_cell);
    } else if (strcmp(keyword, ":lines") == 0) {
      akx_cell_t *lines_cell =
          akx_rt_eval_and_assert(rt, current, AKX_TYPE_INTEGER_LITERAL,
                                 "fs/read: lines must be an integer");
      if (!lines_cell) {
        return NULL;
      }
      lines_to_read = akx_rt_cell_as_int(lines_cell);
      akx_rt_free_cell(rt, lines_cell);
    } else if (strcmp(keyword, ":offset") == 0) {
      akx_cell_t *offset_cell =
          akx_rt_eval_and_assert(rt, current, AKX_TYPE_INTEGER_LITERAL,
                                 "fs/read: offset must be an integer");
      if (!offset_cell) {
        return NULL;
      }
      offset = akx_rt_cell_as_int(offset_cell);
      akx_rt_free_cell(rt, offset_cell);
    } else if (strcmp(keyword, ":line-start") == 0) {
      akx_cell_t *line_start_cell =
          akx_rt_eval_and_assert(rt, current, AKX_TYPE_INTEGER_LITERAL,
                                 "fs/read: line-start must be an integer");
      if (!line_start_cell) {
        return NULL;
      }
      line_start = akx_rt_cell_as_int(line_start_cell);
      akx_rt_free_cell(rt, line_start_cell);
    } else if (strcmp(keyword, ":line-end") == 0) {
      akx_cell_t *line_end_cell =
          akx_rt_eval_and_assert(rt, current, AKX_TYPE_INTEGER_LITERAL,
                                 "fs/read: line-end must be an integer");
      if (!line_end_cell) {
        return NULL;
      }
      line_end = akx_rt_cell_as_int(line_end_cell);
      akx_rt_free_cell(rt, line_end_cell);
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

  file_handle_t *handle = fs_get_handle(rt, fid);
  if (!handle) {
    akx_rt_error_fmt(rt, "fs/read: invalid file ID %d", fid);
    return NULL;
  }

  if (offset >= 0) {
    if (fseek(handle->fp, offset, SEEK_SET) != 0) {
      akx_rt_error_fmt(rt, "fs/read: failed to seek to offset %ld", offset);
      return NULL;
    }
  }

  char *result_str = NULL;
  size_t result_len = 0;

  if (bytes_to_read > 0) {
    result_str = AK24_ALLOC(bytes_to_read + 1);
    if (!result_str) {
      akx_rt_error(rt, "fs/read: memory allocation failed");
      return NULL;
    }
    size_t read = fread(result_str, 1, bytes_to_read, handle->fp);
    result_str[read] = '\0';
    result_len = read;
  } else if (lines_to_read > 0 || (line_start >= 0 && line_end >= 0)) {
    size_t capacity = FS_READ_BUFFER_SIZE;
    result_str = AK24_ALLOC(capacity);
    if (!result_str) {
      akx_rt_error(rt, "fs/read: memory allocation failed");
      return NULL;
    }

    int current_line = 0;
    int target_start = line_start >= 0 ? line_start : 0;
    int target_end = line_end >= 0 ? line_end : (lines_to_read - 1);
    char line_buffer[FS_READ_BUFFER_SIZE];

    while (fgets(line_buffer, sizeof(line_buffer), handle->fp)) {
      if (current_line >= target_start && current_line <= target_end) {
        size_t line_len = strlen(line_buffer);
        if (result_len + line_len >= capacity) {
          capacity *= 2;
          char *new_str = AK24_REALLOC(result_str, capacity);
          if (!new_str) {
            AK24_FREE(result_str);
            akx_rt_error(rt, "fs/read: memory reallocation failed");
            return NULL;
          }
          result_str = new_str;
        }
        strcpy(result_str + result_len, line_buffer);
        result_len += line_len;
      }
      current_line++;
      if (current_line > target_end) {
        break;
      }
    }
    result_str[result_len] = '\0';
  } else {
    size_t capacity = FS_READ_BUFFER_SIZE;
    result_str = AK24_ALLOC(capacity);
    if (!result_str) {
      akx_rt_error(rt, "fs/read: memory allocation failed");
      return NULL;
    }

    char buffer[FS_READ_BUFFER_SIZE];
    size_t read;
    while ((read = fread(buffer, 1, sizeof(buffer), handle->fp)) > 0) {
      if (result_len + read >= capacity) {
        capacity *= 2;
        char *new_str = AK24_REALLOC(result_str, capacity);
        if (!new_str) {
          AK24_FREE(result_str);
          akx_rt_error(rt, "fs/read: memory reallocation failed");
          return NULL;
        }
        result_str = new_str;
      }
      memcpy(result_str + result_len, buffer, read);
      result_len += read;
    }
    result_str[result_len] = '\0';
  }

  handle->position = ftell(handle->fp);

  akx_cell_t *result = akx_rt_alloc_cell(rt, AKX_TYPE_STRING_LITERAL);
  akx_rt_set_string(rt, result, result_str);
  AK24_FREE(result_str);

  return result;
}
