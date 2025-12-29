#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

akx_cell_t *stat_impl(akx_runtime_ctx_t *rt, akx_cell_t *args) {
  if (!args) {
    akx_rt_error(rt, "fs/stat: requires a file path");
    return NULL;
  }

  akx_cell_t *path_cell = akx_rt_eval(rt, args);
  if (!path_cell || !akx_rt_cell_is_type(path_cell, AKX_TYPE_STRING_LITERAL)) {
    akx_rt_error(rt, "fs/stat: path must be a string");
    if (path_cell)
      akx_rt_free_cell(rt, path_cell);
    return NULL;
  }

  const char *path = akx_rt_cell_as_string(path_cell);
  char *expanded_path = akx_rt_expand_env_vars(path);
  const char *actual_path = expanded_path ? expanded_path : path;

  struct stat st;
  int result = stat(actual_path, &st);

  if (result != 0) {
    akx_rt_error_fmt(rt, "fs/stat: failed to stat '%s'", actual_path);
    if (expanded_path)
      free(expanded_path);
    akx_rt_free_cell(rt, path_cell);
    return NULL;
  }

  if (expanded_path)
    free(expanded_path);
  akx_rt_free_cell(rt, path_cell);

  akx_cell_t *result_list = NULL;
  akx_cell_t *tail = NULL;

  akx_cell_t *size_key = akx_rt_alloc_cell(rt, AKX_TYPE_SYMBOL);
  akx_rt_set_symbol(rt, size_key, ":size");
  akx_cell_t *size_val = akx_rt_alloc_cell(rt, AKX_TYPE_INTEGER_LITERAL);
  akx_rt_set_int(rt, size_val, (int)st.st_size);
  size_key->next = size_val;

  result_list = size_key;
  tail = size_val;

  akx_cell_t *type_key = akx_rt_alloc_cell(rt, AKX_TYPE_SYMBOL);
  akx_rt_set_symbol(rt, type_key, ":type");
  akx_cell_t *type_val = akx_rt_alloc_cell(rt, AKX_TYPE_SYMBOL);
  if (S_ISDIR(st.st_mode)) {
    akx_rt_set_symbol(rt, type_val, "dir");
  } else if (S_ISREG(st.st_mode)) {
    akx_rt_set_symbol(rt, type_val, "file");
  } else {
    akx_rt_set_symbol(rt, type_val, "other");
  }
  tail->next = type_key;
  type_key->next = type_val;
  tail = type_val;

  akx_cell_t *mtime_key = akx_rt_alloc_cell(rt, AKX_TYPE_SYMBOL);
  akx_rt_set_symbol(rt, mtime_key, ":mtime");
  akx_cell_t *mtime_val = akx_rt_alloc_cell(rt, AKX_TYPE_INTEGER_LITERAL);
  akx_rt_set_int(rt, mtime_val, (int)st.st_mtime);
  tail->next = mtime_key;
  mtime_key->next = mtime_val;
  tail = mtime_val;

  akx_cell_t *mode_key = akx_rt_alloc_cell(rt, AKX_TYPE_SYMBOL);
  akx_rt_set_symbol(rt, mode_key, ":mode");
  akx_cell_t *mode_val = akx_rt_alloc_cell(rt, AKX_TYPE_INTEGER_LITERAL);
  akx_rt_set_int(rt, mode_val, (int)(st.st_mode & 0777));
  tail->next = mode_key;
  mode_key->next = mode_val;

  return result_list;
}
