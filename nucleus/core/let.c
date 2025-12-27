akx_cell_t *let_impl(akx_runtime_ctx_t *rt, akx_cell_t *args) {
  akx_cell_t *symbol_cell = akx_rt_list_nth(args, 0);
  if (!symbol_cell || akx_rt_cell_get_type(symbol_cell) != AKX_TYPE_SYMBOL) {
    akx_rt_error(rt, "let: first argument must be a symbol");
    return NULL;
  }

  const char *symbol = akx_rt_cell_as_symbol(symbol_cell);

  ak_context_t *scope = akx_rt_get_scope(rt);
  if (ak_context_has_local(scope, symbol)) {
    akx_rt_error_fmt(rt, "let: symbol '%s' already defined in current scope",
                     symbol);
    return NULL;
  }

  akx_cell_t *value_cell = akx_rt_list_nth(args, 1);
  if (!value_cell) {
    akx_rt_error(rt, "let: missing value argument");
    return NULL;
  }

  akx_cell_t *evaled = akx_rt_eval(rt, value_cell);
  if (!evaled) {
    return NULL;
  }

  akx_rt_scope_set(rt, symbol, evaled);

  akx_cell_t *returned = akx_cell_clone(evaled);
  if (!returned) {
    akx_rt_error(rt, "let: failed to clone value for return");
    return NULL;
  }

  return returned;
}
