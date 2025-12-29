#include <dirent.h>
#include <stdlib.h>
#include <string.h>

akx_cell_t *list_dir_impl(akx_runtime_ctx_t *rt, akx_cell_t *args) {
  if (!args) {
    akx_rt_error(rt, "fs/list-dir: requires a directory path");
    return NULL;
  }

  akx_cell_t *path_cell = akx_rt_eval(rt, args);
  if (!path_cell || !akx_rt_cell_is_type(path_cell, AKX_TYPE_STRING_LITERAL)) {
    akx_rt_error(rt, "fs/list-dir: path must be a string");
    if (path_cell)
      akx_rt_free_cell(rt, path_cell);
    return NULL;
  }

  const char *path = akx_rt_cell_as_string(path_cell);
  char *expanded_path = akx_rt_expand_env_vars(path);
  const char *actual_path = expanded_path ? expanded_path : path;

  DIR *dir = opendir(actual_path);
  if (!dir) {
    akx_rt_error_fmt(rt, "fs/list-dir: failed to open directory '%s'",
                     actual_path);
    if (expanded_path)
      free(expanded_path);
    akx_rt_free_cell(rt, path_cell);
    return NULL;
  }

  akx_cell_t *result = NULL;
  akx_cell_t *tail = NULL;
  struct dirent *entry;

  while ((entry = readdir(dir)) != NULL) {
    if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
      continue;
    }

    akx_cell_t *name_cell = akx_rt_alloc_cell(rt, AKX_TYPE_STRING_LITERAL);
    akx_rt_set_string(rt, name_cell, entry->d_name);

    if (!result) {
      result = name_cell;
      tail = name_cell;
    } else {
      tail->next = name_cell;
      tail = name_cell;
    }
  }

  closedir(dir);

  if (expanded_path)
    free(expanded_path);
  akx_rt_free_cell(rt, path_cell);

  if (!result) {
    result = akx_rt_alloc_cell(rt, AKX_TYPE_SYMBOL);
    akx_rt_set_symbol(rt, result, "nil");
  }

  return result;
}
