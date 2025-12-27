akx_cell_t *print(akx_runtime_ctx_t *rt, akx_cell_t *args) {
  akx_cell_t *current = args;

  while (current) {
    akx_cell_t *evaled = akx_rt_eval(rt, current);
    if (!evaled)
      return NULL;

    if (akx_rt_cell_is_type(evaled, AKX_TYPE_STRING_LITERAL)) {
      printf("%s", akx_rt_cell_as_string(evaled));
    } else if (akx_rt_cell_is_type(evaled, AKX_TYPE_INTEGER_LITERAL)) {
      printf("%d", akx_rt_cell_as_int(evaled));
    } else if (akx_rt_cell_is_type(evaled, AKX_TYPE_REAL_LITERAL)) {
      printf("%.6f", akx_rt_cell_as_real(evaled));
    } else if (akx_rt_cell_is_type(evaled, AKX_TYPE_SYMBOL)) {
      printf("%s", akx_rt_cell_as_symbol(evaled));
    } else if (akx_rt_cell_is_type(evaled, AKX_TYPE_LAMBDA)) {
      printf("<lambda>");
    }

    if (evaled->type != AKX_TYPE_LAMBDA) {
      akx_rt_free_cell(rt, evaled);
    }

    current = akx_rt_cell_next(current);
  }

  akx_cell_t *nil = akx_rt_alloc_cell(rt, AKX_TYPE_SYMBOL);
  akx_rt_set_symbol(rt, nil, "nil");
  return nil;
}
