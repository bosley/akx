akx_cell_t *write_file_impl(akx_runtime_ctx_t *rt, akx_cell_t *args) {
  if (!args) {
    akx_rt_error(rt, "fs/write-file: requires path and content arguments");
    return NULL;
  }

  akx_cell_t *path_cell = akx_rt_eval(rt, args);
  if (!path_cell || !akx_rt_cell_is_type(path_cell, AKX_TYPE_STRING_LITERAL)) {
    akx_rt_error(rt, "fs/write-file: path must be a string");
    if (path_cell)
      akx_rt_free_cell(rt, path_cell);
    return NULL;
  }

  akx_cell_t *content_arg = akx_rt_cell_next(args);
  if (!content_arg) {
    akx_rt_error(rt, "fs/write-file: requires content argument");
    akx_rt_free_cell(rt, path_cell);
    return NULL;
  }

  akx_cell_t *content_cell = akx_rt_eval(rt, content_arg);
  if (!content_cell || !akx_rt_cell_is_type(content_cell, AKX_TYPE_STRING_LITERAL)) {
    akx_rt_error(rt, "fs/write-file: content must be a string");
    akx_rt_free_cell(rt, path_cell);
    if (content_cell)
      akx_rt_free_cell(rt, content_cell);
    return NULL;
  }

  const char *path = akx_rt_cell_as_string(path_cell);
  const char *content = akx_rt_cell_as_string(content_cell);
  
  char *expanded_path = akx_rt_expand_env_vars(path);
  const char *actual_path = expanded_path ? expanded_path : path;

  FILE *file = fopen(actual_path, "w");
  
  if (expanded_path) {
    free(expanded_path);
  }

  if (!file) {
    akx_rt_error_fmt(rt, "fs/write-file: failed to open file '%s' for writing", actual_path);
    akx_rt_free_cell(rt, path_cell);
    akx_rt_free_cell(rt, content_cell);
    return NULL;
  }

  size_t content_len = strlen(content);
  size_t written = fwrite(content, 1, content_len, file);
  fclose(file);

  akx_rt_free_cell(rt, path_cell);
  akx_rt_free_cell(rt, content_cell);

  if (written != content_len) {
    akx_rt_error(rt, "fs/write-file: failed to write all content");
    return NULL;
  }

  akx_cell_t *result = akx_rt_alloc_cell(rt, AKX_TYPE_INTEGER_LITERAL);
  akx_rt_set_int(rt, result, (int)written);
  return result;
}

