#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

akx_cell_t *is_file_impl(akx_runtime_ctx_t *rt, akx_cell_t *args) {
  if (!args) {
    akx_rt_error(rt, "fs/?is-file: requires a path");
    return NULL;
  }

  akx_cell_t *path_cell = akx_rt_eval(rt, args);
  if (!path_cell || !akx_rt_cell_is_type(path_cell, AKX_TYPE_STRING_LITERAL)) {
    akx_rt_error(rt, "fs/?is-file: path must be a string");
    if (path_cell)
      akx_rt_free_cell(rt, path_cell);
    return NULL;
  }

  const char *path = akx_rt_cell_as_string(path_cell);
  char *expanded_path = akx_rt_expand_env_vars(path);
  const char *actual_path = expanded_path ? expanded_path : path;

  struct stat st;
  int result = stat(actual_path, &st);
  int is_file = (result == 0 && S_ISREG(st.st_mode));

  if (expanded_path)
    free(expanded_path);
  akx_rt_free_cell(rt, path_cell);

  akx_cell_t *result_cell = akx_rt_alloc_cell(rt, AKX_TYPE_INTEGER_LITERAL);
  akx_rt_set_int(rt, result_cell, is_file ? 1 : 0);
  return result_cell;
}
