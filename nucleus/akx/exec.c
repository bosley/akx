akx_cell_t *exec_impl(akx_runtime_ctx_t *rt, akx_cell_t *args) {
  akx_cell_t *path_cell = akx_rt_list_nth(args, 0);
  if (!path_cell) {
    akx_rt_error(rt, "akx/exec: missing file path argument");
    return NULL;
  }

  akx_cell_t *evaled_path = akx_rt_eval(rt, path_cell);
  if (!evaled_path) {
    return NULL;
  }

  if (akx_rt_cell_get_type(evaled_path) != AKX_TYPE_STRING_LITERAL) {
    akx_rt_error(rt, "akx/exec: file path must be a string");
    akx_cell_free(evaled_path);
    return NULL;
  }

  const char *file_path = akx_rt_cell_as_string(evaled_path);
  if (!file_path) {
    akx_rt_error(rt, "akx/exec: invalid file path");
    akx_cell_free(evaled_path);
    return NULL;
  }

  size_t arg_count = akx_rt_list_length(args);
  char **script_argv = NULL;
  int script_argc = (int)arg_count;

  script_argv = AK24_ALLOC(sizeof(char *) * script_argc);
  if (!script_argv) {
    akx_rt_error(rt, "akx/exec: failed to allocate argv array");
    akx_cell_free(evaled_path);
    return NULL;
  }

  size_t file_path_len = strlen(file_path);
  script_argv[0] = AK24_ALLOC(file_path_len + 1);
  if (!script_argv[0]) {
    akx_rt_error(rt, "akx/exec: failed to allocate script path");
    AK24_FREE(script_argv);
    akx_cell_free(evaled_path);
    return NULL;
  }
  memcpy(script_argv[0], file_path, file_path_len + 1);

  if (arg_count > 1) {
    akx_cell_t *current_arg = akx_rt_cell_next(args);
    int arg_idx = 1;
    while (current_arg && arg_idx < script_argc) {
      akx_cell_t *evaled_arg = akx_rt_eval(rt, current_arg);
      if (!evaled_arg) {
        for (int i = 0; i < arg_idx; i++) {
          AK24_FREE(script_argv[i]);
        }
        AK24_FREE(script_argv);
        akx_cell_free(evaled_path);
        return NULL;
      }

      akx_type_t arg_type = akx_rt_cell_get_type(evaled_arg);
      char *arg_str = NULL;

      switch (arg_type) {
      case AKX_TYPE_STRING_LITERAL: {
        const char *str = akx_rt_cell_as_string(evaled_arg);
        if (str) {
          size_t len = strlen(str);
          arg_str = AK24_ALLOC(len + 1);
          if (arg_str) {
            memcpy(arg_str, str, len + 1);
          }
        }
        break;
      }
      case AKX_TYPE_INTEGER_LITERAL: {
        int val = akx_rt_cell_as_int(evaled_arg);
        arg_str = AK24_ALLOC(32);
        if (arg_str) {
          snprintf(arg_str, 32, "%d", val);
        }
        break;
      }
      case AKX_TYPE_REAL_LITERAL: {
        double val = akx_rt_cell_as_real(evaled_arg);
        arg_str = AK24_ALLOC(64);
        if (arg_str) {
          snprintf(arg_str, 64, "%f", val);
        }
        break;
      }
      case AKX_TYPE_SYMBOL: {
        const char *sym = akx_rt_cell_as_symbol(evaled_arg);
        if (sym) {
          size_t len = strlen(sym);
          arg_str = AK24_ALLOC(len + 1);
          if (arg_str) {
            memcpy(arg_str, sym, len + 1);
          }
        }
        break;
      }
      default: {
        arg_str = AK24_ALLOC(16);
        if (arg_str) {
          snprintf(arg_str, 16, "<value>");
        }
        break;
      }
      }

      akx_rt_free_cell(rt, evaled_arg);

      if (!arg_str) {
        akx_rt_error(rt, "akx/exec: failed to convert argument to string");
        for (int i = 0; i < arg_idx; i++) {
          AK24_FREE(script_argv[i]);
        }
        AK24_FREE(script_argv);
        akx_cell_free(evaled_path);
        return NULL;
      }

      script_argv[arg_idx++] = arg_str;
      current_arg = akx_rt_cell_next(current_arg);
    }
  }

  int saved_argc = akx_rt_get_script_argc(rt);
  char **saved_argv = akx_rt_get_script_argv(rt);

  akx_runtime_set_script_args(rt, script_argc, script_argv);

  char *expanded_path = akx_rt_expand_env_vars(file_path);
  if (!expanded_path) {
    akx_rt_error(rt, "akx/exec: failed to expand path");
    akx_runtime_set_script_args(rt, saved_argc, saved_argv);
    if (script_argv) {
      for (int i = 0; i < script_argc; i++) {
        AK24_FREE(script_argv[i]);
      }
      AK24_FREE(script_argv);
    }
    akx_cell_free(evaled_path);
    return NULL;
  }

  akx_parse_result_t result = akx_cell_parse_file(expanded_path);
  AK24_FREE(expanded_path);

  if (result.errors) {
    akx_parse_error_t *err = result.errors;
    while (err) {
      char error_msg[512];
      snprintf(error_msg, sizeof(error_msg),
               "akx/exec: parse error in '%s': %s", file_path, err->message);
      akx_rt_error(rt, error_msg);
      err = err->next;
    }
    akx_parse_result_free(&result);
    akx_runtime_set_script_args(rt, saved_argc, saved_argv);
    if (script_argv) {
      for (int i = 0; i < script_argc; i++) {
        AK24_FREE(script_argv[i]);
      }
      AK24_FREE(script_argv);
    }
    akx_cell_free(evaled_path);
    return NULL;
  }

  if (list_count(&result.cells) == 0) {
    akx_parse_result_free(&result);
    akx_runtime_set_script_args(rt, saved_argc, saved_argv);
    if (script_argv) {
      for (int i = 0; i < script_argc; i++) {
        AK24_FREE(script_argv[i]);
      }
      AK24_FREE(script_argv);
    }
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
      akx_runtime_set_script_args(rt, saved_argc, saved_argv);
      if (script_argv) {
        for (int i = 0; i < script_argc; i++) {
          AK24_FREE(script_argv[i]);
        }
        AK24_FREE(script_argv);
      }
      akx_cell_free(evaled_path);
      return NULL;
    }
  }

  akx_parse_result_free(&result);
  akx_runtime_set_script_args(rt, saved_argc, saved_argv);
  if (script_argv) {
    for (int i = 0; i < script_argc; i++) {
      AK24_FREE(script_argv[i]);
    }
    AK24_FREE(script_argv);
  }
  akx_cell_free(evaled_path);

  if (!last_result) {
    last_result = akx_rt_alloc_cell(rt, AKX_TYPE_SYMBOL);
    if (last_result) {
      akx_rt_set_symbol(rt, last_result, "t");
    }
  }

  return last_result;
}
