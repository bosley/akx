typedef struct {
  int count;
} counter_state_t;

void counter_init(akx_runtime_ctx_t *rt) {
  counter_state_t *state = malloc(sizeof(counter_state_t));
  state->count = 0;
  akx_rt_module_set_data(rt, state);
  printf("[counter] Initialized with count=0\n");
}

void counter_deinit(akx_runtime_ctx_t *rt) {
  counter_state_t *state = akx_rt_module_get_data(rt);
  if (state) {
    printf("[counter] Deinitializing with final count=%d\n", state->count);
    free(state);
  }
}

void counter_reload(akx_runtime_ctx_t *rt, void *old_state) {
  counter_state_t *old = (counter_state_t *)old_state;
  counter_state_t *new_state = malloc(sizeof(counter_state_t));
  new_state->count = old->count;
  akx_rt_module_set_data(rt, new_state);
  printf("[counter] Reloaded, preserving count=%d\n", new_state->count);
  free(old);
}

akx_cell_t *counter(akx_runtime_ctx_t *rt, akx_cell_t *args) {
  (void)args;
  counter_state_t *state = akx_rt_module_get_data(rt);
  if (!state) {
    akx_rt_error(rt, "counter state not initialized");
    return NULL;
  }

  state->count++;

  akx_cell_t *result = akx_rt_alloc_cell(rt, AKX_TYPE_INTEGER_LITERAL);
  akx_rt_set_int(rt, result, state->count);
  return result;
}
