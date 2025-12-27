akx_cell_t *import_impl(akx_runtime_ctx_t *rt, akx_cell_t *args) {
  akx_cell_t *path_cell = akx_rt_list_nth(args, 0);
  if (!path_cell) {
    akx_rt_error(rt, "import: missing file path argument");
    return NULL;
  }

  akx_cell_t *evaled_path = akx_rt_eval(rt, path_cell);
  if (!evaled_path) {
    return NULL;
  }

  if (akx_rt_cell_get_type(evaled_path) != AKX_TYPE_STRING_LITERAL) {
    akx_rt_error(rt, "import: file path must be a string");
    akx_cell_free(evaled_path);
    return NULL;
  }

  const char *file_path = akx_rt_cell_as_string(evaled_path);
  if (!file_path) {
    akx_rt_error(rt, "import: invalid file path");
    akx_cell_free(evaled_path);
    return NULL;
  }

  akx_parse_result_t result = akx_cell_parse_file(file_path);

  if (result.errors) {
    akx_parse_error_t *err = result.errors;
    while (err) {
      char error_msg[512];
      snprintf(error_msg, sizeof(error_msg), "import: parse error in '%s': %s",
               file_path, err->message);
      akx_rt_error(rt, error_msg);
      err = err->next;
    }
    akx_parse_result_free(&result);
    akx_cell_free(evaled_path);
    return NULL;
  }

  if (list_count(&result.cells) == 0) {
    akx_parse_result_free(&result);
    akx_cell_free(evaled_path);
    akx_cell_t *t_cell = akx_rt_alloc_cell(rt, AKX_TYPE_SYMBOL);
    if (t_cell) {
      akx_rt_set_symbol(rt, t_cell, "t");
    }
    return t_cell;
  }

  akx_cell_t *last_result = NULL;
  list_iter_t iter = list_iter(&result.cells);
  akx_cell_t **cell_ptr;
  while ((cell_ptr = list_next(&result.cells, &iter))) {
    if (last_result) {
      akx_cell_free(last_result);
    }
    last_result = akx_rt_eval(rt, *cell_ptr);
    if (!last_result) {
      akx_parse_result_free(&result);
      akx_cell_free(evaled_path);
      return NULL;
    }
  }

  akx_parse_result_free(&result);
  akx_cell_free(evaled_path);

  if (!last_result) {
    last_result = akx_rt_alloc_cell(rt, AKX_TYPE_SYMBOL);
    if (last_result) {
      akx_rt_set_symbol(rt, last_result, "t");
    }
  }

  return last_result;
}

