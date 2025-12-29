akx_cell_t *path_join_impl(akx_runtime_ctx_t *rt, akx_cell_t *args) {
  if (!args) {
    akx_rt_error(rt, "fs/path-join: requires at least one path component");
    return NULL;
  }

  size_t count = akx_rt_list_length(args);
  const char **paths = (const char **)malloc(count * sizeof(char *));
  if (!paths) {
    akx_rt_error(rt, "fs/path-join: memory allocation failed");
    return NULL;
  }

  akx_cell_t *current = args;
  size_t i = 0;
  while (current && i < count) {
    akx_cell_t *path_cell = akx_rt_eval(rt, current);
    if (!path_cell ||
        !akx_rt_cell_is_type(path_cell, AKX_TYPE_STRING_LITERAL)) {
      akx_rt_error(rt, "fs/path-join: all arguments must be strings");
      for (size_t j = 0; j < i; j++) {
        free((void *)paths[j]);
      }
      free(paths);
      if (path_cell)
        akx_rt_free_cell(rt, path_cell);
      return NULL;
    }

    const char *path_str = akx_rt_cell_as_string(path_cell);
    paths[i] = strdup(path_str);
    akx_rt_free_cell(rt, path_cell);

    current = akx_rt_cell_next(current);
    i++;
  }

  ak_buffer_t *joined = NULL;
  if (count == 1) {
    joined = ak_filepath_join(1, paths[0]);
  } else if (count == 2) {
    joined = ak_filepath_join(2, paths[0], paths[1]);
  } else if (count == 3) {
    joined = ak_filepath_join(3, paths[0], paths[1], paths[2]);
  } else if (count == 4) {
    joined = ak_filepath_join(4, paths[0], paths[1], paths[2], paths[3]);
  } else if (count == 5) {
    joined =
        ak_filepath_join(5, paths[0], paths[1], paths[2], paths[3], paths[4]);
  } else {
    akx_rt_error(rt, "fs/path-join: maximum 5 path components supported");
    for (size_t j = 0; j < count; j++) {
      free((void *)paths[j]);
    }
    free(paths);
    return NULL;
  }

  for (size_t j = 0; j < count; j++) {
    free((void *)paths[j]);
  }
  free(paths);

  if (!joined) {
    akx_rt_error(rt, "fs/path-join: failed to join paths");
    return NULL;
  }

  uint8_t *data = ak_buffer_data(joined);
  size_t size = ak_buffer_count(joined);

  char *result_str = (char *)malloc(size + 1);
  if (!result_str) {
    akx_rt_error(rt, "fs/path-join: memory allocation failed");
    ak_buffer_free(joined);
    return NULL;
  }

  memcpy(result_str, data, size);
  result_str[size] = '\0';

  akx_cell_t *result = akx_rt_alloc_cell(rt, AKX_TYPE_STRING_LITERAL);
  akx_rt_set_string(rt, result, result_str);

  free(result_str);
  ak_buffer_free(joined);

  return result;
}
