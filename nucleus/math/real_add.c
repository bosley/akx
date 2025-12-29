akx_cell_t *real_add(akx_runtime_ctx_t *rt, akx_cell_t *args) {
  if (akx_rt_list_length(args) < 2) {
    akx_rt_error(rt, "real/+ requires at least 2 arguments");
    return NULL;
  }

  double sum = 0.0;
  akx_cell_t *current = args;

  while (current) {
    akx_cell_t *evaled = akx_rt_eval_and_assert(
        rt, current, AKX_TYPE_REAL_LITERAL, "real/+ requires real arguments");
    if (!evaled) {
      return NULL;
    }

    sum += akx_rt_cell_as_real(evaled);
    akx_rt_free_cell(rt, evaled);
    current = akx_rt_cell_next(current);
  }

  akx_cell_t *result = akx_rt_alloc_cell(rt, AKX_TYPE_REAL_LITERAL);
  akx_rt_set_real(rt, result, sum);
  return result;
}
