akx_cell_t *sub(akx_runtime_ctx_t *rt, akx_cell_t *args) {
  if (akx_rt_list_length(args) != 2) {
    akx_rt_error(rt, "- requires exactly 2 arguments");
    return NULL;
  }

  akx_cell_t *left_cell = akx_rt_list_nth(args, 0);
  akx_cell_t *right_cell = akx_rt_list_nth(args, 1);

  akx_cell_t *left = akx_rt_eval_and_assert(
      rt, left_cell, AKX_TYPE_INTEGER_LITERAL, "- requires integer arguments");
  if (!left) {
    return NULL;
  }

  akx_cell_t *right = akx_rt_eval_and_assert(
      rt, right_cell, AKX_TYPE_INTEGER_LITERAL, "- requires integer arguments");
  if (!right) {
    akx_rt_free_cell(rt, left);
    return NULL;
  }

  int result_val = akx_rt_cell_as_int(left) - akx_rt_cell_as_int(right);

  akx_rt_free_cell(rt, left);
  akx_rt_free_cell(rt, right);

  akx_cell_t *result = akx_rt_alloc_cell(rt, AKX_TYPE_INTEGER_LITERAL);
  akx_rt_set_int(rt, result, result_val);
  return result;
}
