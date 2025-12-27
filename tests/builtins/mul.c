akx_cell_t *mul(akx_runtime_ctx_t *rt, akx_cell_t *args) {
  if (akx_rt_list_length(args) < 2) {
    akx_rt_error(rt, "* requires at least 2 arguments");
    return NULL;
  }

  int product = 1;
  akx_cell_t *current = args;

  while (current) {
    akx_cell_t *evaled = akx_rt_eval_and_assert(
        rt, current, AKX_TYPE_INTEGER_LITERAL, "* requires integer arguments");
    if (!evaled) {
      return NULL;
    }

    product *= akx_rt_cell_as_int(evaled);
    akx_rt_free_cell(rt, evaled);
    current = akx_rt_cell_next(current);
  }

  akx_cell_t *result = akx_rt_alloc_cell(rt, AKX_TYPE_INTEGER_LITERAL);
  akx_rt_set_int(rt, result, product);
  return result;
}
