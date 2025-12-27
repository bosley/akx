akx_cell_t *defined_impl(akx_runtime_ctx_t *rt, akx_cell_t *args) {
  if (akx_rt_list_length(args) != 1) {
    akx_rt_error(rt, "?defined requires exactly 1 argument");
    return NULL;
  }

  akx_cell_t *symbol_cell = akx_rt_list_nth(args, 0);
  if (!symbol_cell || akx_rt_cell_get_type(symbol_cell) != AKX_TYPE_SYMBOL) {
    akx_rt_error(rt, "?defined: argument must be a symbol");
    return NULL;
  }

  const char *sym = akx_rt_cell_as_symbol(symbol_cell);
  void *value = akx_rt_scope_get(rt, sym);

  akx_cell_t *ret = akx_rt_alloc_cell(rt, AKX_TYPE_INTEGER_LITERAL);
  akx_rt_set_int(rt, ret, value ? 1 : 0);
  return ret;
}
