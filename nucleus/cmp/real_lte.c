akx_cell_t *real_lte_impl(akx_runtime_ctx_t *rt, akx_cell_t *args) {
  if (akx_rt_list_length(args) != 2) {
    akx_rt_error(rt, "real/lte requires exactly 2 arguments");
    return NULL;
  }

  akx_cell_t *first_arg = akx_rt_list_nth(args, 0);
  akx_cell_t *evaled_first = akx_rt_eval_and_assert(
      rt, first_arg, AKX_TYPE_REAL_LITERAL, "real/lte requires real arguments");
  if (!evaled_first) {
    return NULL;
  }

  akx_cell_t *second_arg = akx_rt_list_nth(args, 1);
  akx_cell_t *evaled_second =
      akx_rt_eval_and_assert(rt, second_arg, AKX_TYPE_REAL_LITERAL,
                             "real/lte requires real arguments");
  if (!evaled_second) {
    akx_rt_free_cell(rt, evaled_first);
    return NULL;
  }

  double first_val = akx_rt_cell_as_real(evaled_first);
  double second_val = akx_rt_cell_as_real(evaled_second);

  int is_lte = first_val <= second_val;

  akx_rt_free_cell(rt, evaled_first);
  akx_rt_free_cell(rt, evaled_second);

  akx_cell_t *result = akx_rt_alloc_cell(rt, AKX_TYPE_INTEGER_LITERAL);
  akx_rt_set_int(rt, result, is_lte);
  return result;
}
