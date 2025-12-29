akx_cell_t *os_args_impl(akx_runtime_ctx_t *rt, akx_cell_t *args) {
  (void)args;

  int script_argc = akx_rt_get_script_argc(rt);
  char **script_argv = akx_rt_get_script_argv(rt);

  if (script_argc <= 0 || !script_argv) {
    akx_cell_t *nil = akx_rt_alloc_cell(rt, AKX_TYPE_SYMBOL);
    akx_rt_set_symbol(rt, nil, "nil");
    return nil;
  }

  akx_cell_t *result = NULL;
  akx_cell_t *tail = NULL;

  for (int i = 0; i < script_argc; i++) {
    akx_cell_t *arg_cell = akx_rt_alloc_cell(rt, AKX_TYPE_STRING_LITERAL);
    akx_rt_set_string(rt, arg_cell, script_argv[i]);

    if (!result) {
      result = arg_cell;
      tail = arg_cell;
    } else {
      tail->next = arg_cell;
      tail = arg_cell;
    }
  }

  return result ? result : akx_rt_alloc_cell(rt, AKX_TYPE_SYMBOL);
}
