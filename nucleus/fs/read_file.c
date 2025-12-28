akx_cell_t *read_file_impl(akx_runtime_ctx_t *rt, akx_cell_t *args) {
  if (!args) {
    akx_rt_error(rt, "fs/read-file: requires a file path");
    return NULL;
  }

  akx_cell_t *path_cell = akx_rt_eval(rt, args);
  if (!path_cell || !akx_rt_cell_is_type(path_cell, AKX_TYPE_STRING_LITERAL)) {
    akx_rt_error(rt, "fs/read-file: path must be a string");
    if (path_cell)
      akx_rt_free_cell(rt, path_cell);
    return NULL;
  }

  const char *path = akx_rt_cell_as_string(path_cell);
  char *expanded_path = akx_rt_expand_env_vars(path);
  const char *actual_path = expanded_path ? expanded_path : path;

  ak_buffer_t *buffer = ak_buffer_from_file(actual_path);
  
  if (expanded_path) {
    free(expanded_path);
  }

  if (!buffer) {
    akx_rt_error_fmt(rt, "fs/read-file: failed to read file '%s'", actual_path);
    akx_rt_free_cell(rt, path_cell);
    return NULL;
  }

  uint8_t *data = ak_buffer_data(buffer);
  size_t size = ak_buffer_count(buffer);

  akx_cell_t *result = akx_rt_alloc_cell(rt, AKX_TYPE_STRING_LITERAL);
  
  char *content = (char *)malloc(size + 1);
  if (!content) {
    akx_rt_error(rt, "fs/read-file: memory allocation failed");
    ak_buffer_free(buffer);
    akx_rt_free_cell(rt, path_cell);
    akx_rt_free_cell(rt, result);
    return NULL;
  }
  
  memcpy(content, data, size);
  content[size] = '\0';
  
  akx_rt_set_string(rt, result, content);
  free(content);
  
  ak_buffer_free(buffer);
  akx_rt_free_cell(rt, path_cell);
  
  return result;
}

