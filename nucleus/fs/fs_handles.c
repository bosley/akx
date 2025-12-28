#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FS_READ_BUFFER_SIZE 4096
#define MAX_FID 1024

typedef struct {
  FILE *fp;
  char *path;
  char *mode;
  long position;
} file_handle_t;

static int next_fid = 1;

static char *fid_to_key(int fid) {
  static char key[32];
  snprintf(key, sizeof(key), "__fs_handle_%d", fid);
  return key;
}

akx_cell_t *open_impl(akx_runtime_ctx_t *rt, akx_cell_t *args) {
  if (!args) {
    akx_rt_error(rt, "fs/open: requires path and mode arguments");
    return NULL;
  }

  const char *path = NULL;
  const char *mode = "r";
  char *expanded_path = NULL;

  akx_cell_t *current = args;
  while (current) {
    if (current->type != AKX_TYPE_SYMBOL) {
      akx_rt_error(rt, "fs/open: expected keyword (symbol starting with ':')");
      goto cleanup_error;
    }

    const char *keyword = akx_rt_cell_as_symbol(current);
    if (keyword[0] != ':') {
      akx_rt_error_fmt(rt, "fs/open: keywords must start with ':', got: %s",
                       keyword);
      goto cleanup_error;
    }

    current = akx_rt_cell_next(current);
    if (!current) {
      akx_rt_error_fmt(rt, "fs/open: missing value for keyword %s", keyword);
      goto cleanup_error;
    }

    if (strcmp(keyword, ":path") == 0) {
      akx_cell_t *path_cell =
          akx_rt_eval_and_assert(rt, current, AKX_TYPE_STRING_LITERAL,
                                 "fs/open: path must be a string");
      if (!path_cell)
        goto cleanup_error;
      path = akx_rt_cell_as_string(path_cell);
      expanded_path = akx_rt_expand_env_vars(path);
      akx_rt_free_cell(rt, path_cell);
    } else if (strcmp(keyword, ":mode") == 0) {
      akx_cell_t *mode_cell =
          akx_rt_eval_and_assert(rt, current, AKX_TYPE_STRING_LITERAL,
                                 "fs/open: mode must be a string");
      if (!mode_cell)
        goto cleanup_error;
      mode = akx_rt_cell_as_string(mode_cell);
      akx_rt_free_cell(rt, mode_cell);
    } else {
      akx_rt_error_fmt(rt, "fs/open: unknown keyword: %s", keyword);
      goto cleanup_error;
    }

    current = akx_rt_cell_next(current);
  }

  if (!path) {
    akx_rt_error(rt, "fs/open: :path keyword is required");
    goto cleanup_error;
  }

  const char *actual_path = expanded_path ? expanded_path : path;

  FILE *fp = fopen(actual_path, mode);
  if (!fp) {
    akx_rt_error_fmt(rt, "fs/open: failed to open file '%s' with mode '%s'",
                     actual_path, mode);
    goto cleanup_error;
  }

  file_handle_t *handle = malloc(sizeof(file_handle_t));
  if (!handle) {
    fclose(fp);
    akx_rt_error(rt, "fs/open: memory allocation failed");
    goto cleanup_error;
  }

  handle->fp = fp;
  handle->path = strdup(actual_path);
  handle->mode = strdup(mode);
  handle->position = 0;

  int fid = next_fid++;
  akx_rt_scope_set(rt, fid_to_key(fid), handle);

  if (expanded_path)
    free(expanded_path);

  akx_cell_t *result = akx_rt_alloc_cell(rt, AKX_TYPE_INTEGER_LITERAL);
  akx_rt_set_int(rt, result, fid);
  return result;

cleanup_error:
  if (expanded_path)
    free(expanded_path);
  return NULL;
}

akx_cell_t *close_impl(akx_runtime_ctx_t *rt, akx_cell_t *args) {
  if (!args) {
    akx_rt_error(rt, "fs/close: requires a file ID");
    return NULL;
  }

  int fid = -1;
  akx_cell_t *current = args;
  while (current) {
    if (current->type != AKX_TYPE_SYMBOL) {
      akx_rt_error(rt, "fs/close: expected keyword");
      return NULL;
    }

    const char *keyword = akx_rt_cell_as_symbol(current);
    if (keyword[0] != ':') {
      akx_rt_error_fmt(rt, "fs/close: keywords must start with ':', got: %s",
                       keyword);
      return NULL;
    }

    current = akx_rt_cell_next(current);
    if (!current) {
      akx_rt_error_fmt(rt, "fs/close: missing value for keyword %s", keyword);
      return NULL;
    }

    if (strcmp(keyword, ":fid") == 0) {
      akx_cell_t *fid_cell =
          akx_rt_eval_and_assert(rt, current, AKX_TYPE_INTEGER_LITERAL,
                                 "fs/close: fid must be an integer");
      if (!fid_cell)
        return NULL;
      fid = akx_rt_cell_as_int(fid_cell);
      akx_rt_free_cell(rt, fid_cell);
    } else {
      akx_rt_error_fmt(rt, "fs/close: unknown keyword: %s", keyword);
      return NULL;
    }

    current = akx_rt_cell_next(current);
  }

  if (fid < 0) {
    akx_rt_error(rt, "fs/close: :fid keyword is required");
    return NULL;
  }

  file_handle_t *handle = akx_rt_scope_get(rt, fid_to_key(fid));
  if (!handle) {
    akx_rt_error_fmt(rt, "fs/close: invalid file ID %d", fid);
    return NULL;
  }

  if (handle->fp)
    fclose(handle->fp);
  if (handle->path)
    free(handle->path);
  if (handle->mode)
    free(handle->mode);
  free(handle);

  akx_rt_scope_set(rt, fid_to_key(fid), NULL);

  akx_cell_t *result = akx_rt_alloc_cell(rt, AKX_TYPE_SYMBOL);
  akx_rt_set_symbol(rt, result, "true");
  return result;
}

akx_cell_t *fs_handles_impl(akx_runtime_ctx_t *rt, akx_cell_t *args) {
  akx_cell_t *result = akx_rt_alloc_cell(rt, AKX_TYPE_SYMBOL);
  akx_rt_set_symbol(rt, result, "fs-handles-loaded");
  return result;
}
