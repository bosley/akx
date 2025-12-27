akx_cell_t *assert_false_impl(akx_runtime_ctx_t *rt, akx_cell_t *args) {
  akx_cell_t *condition_cell = akx_rt_list_nth(args, 0);
  if (!condition_cell) {
    akx_rt_error(rt, "assert-false: missing condition argument");
    return NULL;
  }

  akx_cell_t *evaled = akx_rt_eval(rt, condition_cell);
  if (!evaled) {
    return NULL;
  }

  int is_true = is_truthy(evaled);

  if (akx_rt_cell_get_type(evaled) != AKX_TYPE_LAMBDA) {
    akx_cell_free(evaled);
  }

  if (is_true) {
    akx_cell_t *message_cell = akx_rt_list_nth(args, 1);
    if (message_cell) {
      akx_cell_t *msg_evaled = akx_rt_eval(rt, message_cell);
      if (msg_evaled && akx_rt_cell_get_type(msg_evaled) == AKX_TYPE_STRING_LITERAL) {
        const char *msg = akx_rt_cell_as_string(msg_evaled);
        AK24_LOG_ERROR("Assertion failed: %s", msg);
        akx_cell_free(msg_evaled);
      } else {
        AK24_LOG_ERROR("Assertion failed: condition is true");
        if (msg_evaled && akx_rt_cell_get_type(msg_evaled) != AKX_TYPE_LAMBDA) {
          akx_cell_free(msg_evaled);
        }
      }
    } else {
      AK24_LOG_ERROR("Assertion failed: condition is true");
    }
    raise(SIGABRT);
  }

  akx_cell_t *ret = akx_rt_alloc_cell(rt, AKX_TYPE_SYMBOL);
  akx_rt_set_symbol(rt, ret, "t");
  return ret;
}
