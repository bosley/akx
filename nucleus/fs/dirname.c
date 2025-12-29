akx_cell_t *dirname_impl(akx_runtime_ctx_t *rt, akx_cell_t *args) {
  if (!args) {
    akx_rt_error(rt, "fs/dirname: requires a path");
    return NULL;
  }

  akx_cell_t *path_cell = akx_rt_eval(rt, args);
  if (!path_cell || !akx_rt_cell_is_type(path_cell, AKX_TYPE_STRING_LITERAL)) {
    akx_rt_error(rt, "fs/dirname: path must be a string");
    if (path_cell)
      akx_rt_free_cell(rt, path_cell);
    return NULL;
  }

  const char *path = akx_rt_cell_as_string(path_cell);
  ak_buffer_t *dirname = ak_filepath_dirname(path);

  if (!dirname) {
    akx_rt_error(rt, "fs/dirname: failed to extract dirname");
    akx_rt_free_cell(rt, path_cell);
    return NULL;
  }

  uint8_t *data = ak_buffer_data(dirname);
  size_t size = ak_buffer_count(dirname);

  char *result_str = (char *)malloc(size + 1);
  if (!result_str) {
    akx_rt_error(rt, "fs/dirname: memory allocation failed");
    ak_buffer_free(dirname);
    akx_rt_free_cell(rt, path_cell);
    return NULL;
  }

  memcpy(result_str, data, size);
  result_str[size] = '\0';

  akx_cell_t *result = akx_rt_alloc_cell(rt, AKX_TYPE_STRING_LITERAL);
  akx_rt_set_string(rt, result, result_str);

  free(result_str);
  ak_buffer_free(dirname);
  akx_rt_free_cell(rt, path_cell);

  return result;
}
