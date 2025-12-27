akx_cell_t *is_sym_impl(akx_runtime_ctx_t *rt, akx_cell_t *args) {
  if (akx_rt_list_length(args) != 1) {
    akx_rt_error(rt, "?sym requires exactly 1 argument");
    return NULL;
  }

  akx_cell_t *arg = akx_rt_list_nth(args, 0);
  akx_cell_t *evaled = akx_rt_eval(rt, arg);

  int result = 0;
  if (evaled) {
    result = (akx_rt_cell_get_type(evaled) == AKX_TYPE_SYMBOL);
  }

  if (akx_rt_cell_get_type(evaled) != AKX_TYPE_LAMBDA) {
    akx_rt_free_cell(rt, evaled);
  }

  akx_cell_t *ret = akx_rt_alloc_cell(rt, AKX_TYPE_INTEGER_LITERAL);
  akx_rt_set_int(rt, ret, result);
  return ret;
}
