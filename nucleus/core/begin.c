akx_cell_t *begin_impl(akx_runtime_ctx_t *rt, akx_cell_t *args) {
  if (akx_rt_list_length(args) == 0) {
    akx_cell_t *nil = akx_rt_alloc_cell(rt, AKX_TYPE_SYMBOL);
    akx_rt_set_symbol(rt, nil, "nil");
    return nil;
  }

  akx_cell_t *result = NULL;
  akx_cell_t *current = args;

  while (current) {
    if (result && akx_rt_cell_get_type(result) != AKX_TYPE_LAMBDA) {
      akx_rt_free_cell(rt, result);
    }

    result = akx_rt_eval(rt, current);
    if (!result) {
      return NULL;
    }

    current = akx_rt_cell_next(current);
  }

  return result;
}
