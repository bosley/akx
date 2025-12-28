akx_cell_t *exists_impl(akx_runtime_ctx_t *rt, akx_cell_t *args) {
  if (!args) {
    akx_rt_error(rt, "fs/?exists: requires a file path");
    return NULL;
  }

  akx_cell_t *path_cell = akx_rt_eval(rt, args);
  if (!path_cell || !akx_rt_cell_is_type(path_cell, AKX_TYPE_STRING_LITERAL)) {
    akx_rt_error(rt, "fs/?exists: path must be a string");
    if (path_cell)
      akx_rt_free_cell(rt, path_cell);
    return NULL;
  }

  const char *path = akx_rt_cell_as_string(path_cell);
  char *expanded_path = akx_rt_expand_env_vars(path);
  const char *actual_path = expanded_path ? expanded_path : path;

  FILE *file = fopen(actual_path, "r");
  int exists = (file != NULL);

  if (file) {
    fclose(file);
  }

  if (expanded_path) {
    free(expanded_path);
  }

  akx_rt_free_cell(rt, path_cell);

  akx_cell_t *result = akx_rt_alloc_cell(rt, AKX_TYPE_SYMBOL);
  akx_rt_set_symbol(rt, result, exists ? "true" : "false");
  return result;
}
