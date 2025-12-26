akx_cell_t *simple(akx_runtime_ctx_t *rt, akx_cell_t *args) {
  (void)args;
  akx_cell_t *result = akx_rt_alloc_cell(rt, AKX_TYPE_STRING_LITERAL);
  akx_rt_set_string(rt, result, "Simple builtin without lifecycle hooks");
  return result;
}
