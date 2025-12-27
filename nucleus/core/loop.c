akx_cell_t *loop_impl(akx_runtime_ctx_t *rt, akx_cell_t *args) {
  if (akx_rt_list_length(args) < 2) {
    akx_rt_error(rt, "loop: requires at least 2 arguments (condition body...)");
    return NULL;
  }

  akx_cell_t *condition_cell = akx_rt_list_nth(args, 0);
  akx_cell_t *body = akx_rt_list_nth(args, 1);

  akx_cell_t *last_result = NULL;

  while (1) {
    akx_cell_t *condition = akx_rt_eval(rt, condition_cell);
    if (!condition) {
      if (last_result && akx_rt_cell_get_type(last_result) != AKX_TYPE_LAMBDA) {
        akx_rt_free_cell(rt, last_result);
      }
      return NULL;
    }

    int should_continue = 0;
    if (akx_rt_cell_get_type(condition) == AKX_TYPE_INTEGER_LITERAL &&
        akx_rt_cell_as_int(condition) == 1) {
      should_continue = 1;
    }

    if (akx_rt_cell_get_type(condition) != AKX_TYPE_LAMBDA) {
      akx_rt_free_cell(rt, condition);
    }

    if (!should_continue) {
      break;
    }

    akx_rt_push_scope(rt);

    if (last_result && akx_rt_cell_get_type(last_result) != AKX_TYPE_LAMBDA) {
      akx_rt_free_cell(rt, last_result);
    }

    akx_cell_t *current = body;
    while (current) {
      last_result = akx_rt_eval(rt, current);
      if (!last_result) {
        akx_rt_pop_scope(rt);
        return NULL;
      }
      current = akx_rt_cell_next(current);
    }

    akx_rt_pop_scope(rt);
  }

  if (!last_result) {
    last_result = akx_rt_alloc_cell(rt, AKX_TYPE_SYMBOL);
    akx_rt_set_symbol(rt, last_result, "nil");
  }

  return last_result;
}
