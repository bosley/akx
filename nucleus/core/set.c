akx_cell_t *set_impl(akx_runtime_ctx_t *rt, akx_cell_t *args) {
  akx_cell_t *symbol_cell = akx_rt_list_nth(args, 0);
  if (!symbol_cell || akx_rt_cell_get_type(symbol_cell) != AKX_TYPE_SYMBOL) {
    akx_rt_error(rt, "set: first argument must be a symbol");
    return NULL;
  }

  const char *symbol = akx_rt_cell_as_symbol(symbol_cell);

  ak_context_t *scope = akx_rt_get_scope(rt);
  if (!ak_context_has(scope, symbol)) {
    akx_rt_error_fmt(rt, "set: symbol '%s' is not defined", symbol);
    return NULL;
  }

  akx_cell_t *value_cell = akx_rt_list_nth(args, 1);
  if (!value_cell) {
    akx_rt_error(rt, "set: missing value argument");
    return NULL;
  }

  akx_cell_t *evaled = akx_rt_eval(rt, value_cell);
  if (!evaled) {
    return NULL;
  }

  ak_context_t *containing = ak_context_get_containing_context(scope, symbol);
  if (containing) {
    void *old_value = ak_context_get_local(containing, symbol);
    if (old_value) {
      akx_cell_free((akx_cell_t *)old_value);
    }
    ak_context_set(containing, symbol, evaled);
  }

  akx_cell_t *returned = akx_cell_clone(evaled);
  if (!returned) {
    akx_rt_error(rt, "set: failed to clone value for return");
    return NULL;
  }

  return returned;
}
