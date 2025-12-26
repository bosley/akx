akx_cell_t *config_demo(akx_runtime_ctx_t *rt, akx_cell_t *args) {
  (void)args;

  akx_cell_t *result = akx_rt_alloc_cell(rt, AKX_TYPE_STRING_LITERAL);

#ifdef DEBUG_MODE
  akx_rt_set_string(rt, result, "Debug mode enabled");
#else
  akx_rt_set_string(rt, result, "Release mode");
#endif

  return result;
}
