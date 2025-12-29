akx_cell_t *basename_impl(akx_runtime_ctx_t *rt, akx_cell_t *args) {
  if (!args) {
    akx_rt_error(rt, "fs/basename: requires a path");
    return NULL;
  }

  akx_cell_t *path_cell = akx_rt_eval(rt, args);
  if (!path_cell || !akx_rt_cell_is_type(path_cell, AKX_TYPE_STRING_LITERAL)) {
    akx_rt_error(rt, "fs/basename: path must be a string");
    if (path_cell)
      akx_rt_free_cell(rt, path_cell);
    return NULL;
  }

  const char *path = akx_rt_cell_as_string(path_cell);
  ak_buffer_t *basename = ak_filepath_basename(path);

  if (!basename) {
    akx_rt_error(rt, "fs/basename: failed to extract basename");
    akx_rt_free_cell(rt, path_cell);
    return NULL;
  }

  uint8_t *data = ak_buffer_data(basename);
  size_t size = ak_buffer_count(basename);

  char *result_str = (char *)malloc(size + 1);
  if (!result_str) {
    akx_rt_error(rt, "fs/basename: memory allocation failed");
    ak_buffer_free(basename);
    akx_rt_free_cell(rt, path_cell);
    return NULL;
  }

  memcpy(result_str, data, size);
  result_str[size] = '\0';

  akx_cell_t *result = akx_rt_alloc_cell(rt, AKX_TYPE_STRING_LITERAL);
  akx_rt_set_string(rt, result, result_str);

  free(result_str);
  ak_buffer_free(basename);
  akx_rt_free_cell(rt, path_cell);

  return result;
}
