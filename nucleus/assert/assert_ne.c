akx_cell_t *assert_ne_impl(akx_runtime_ctx_t *rt, akx_cell_t *args) {
  akx_cell_t *left_cell = akx_rt_list_nth(args, 0);
  akx_cell_t *right_cell = akx_rt_list_nth(args, 1);

  if (!left_cell || !right_cell) {
    akx_rt_error(rt, "assert/ne: requires two arguments");
    return NULL;
  }

  akx_cell_t *left = akx_rt_eval(rt, left_cell);
  if (!left) {
    return NULL;
  }

  akx_cell_t *right = akx_rt_eval(rt, right_cell);
  if (!right) {
    akx_cell_free(left);
    return NULL;
  }

  int equal = 0;

  if (akx_rt_cell_get_type(left) != akx_rt_cell_get_type(right)) {
    equal = 0;
  } else {
    switch (akx_rt_cell_get_type(left)) {
    case AKX_TYPE_INTEGER_LITERAL:
      equal = (left->value.integer_literal == right->value.integer_literal);
      break;
    case AKX_TYPE_REAL_LITERAL:
      equal = (left->value.real_literal == right->value.real_literal);
      break;
    case AKX_TYPE_STRING_LITERAL: {
      const char *left_str = akx_rt_cell_as_string(left);
      const char *right_str = akx_rt_cell_as_string(right);
      equal = (strcmp(left_str, right_str) == 0);
      break;
    }
    case AKX_TYPE_SYMBOL: {
      const char *left_sym = akx_rt_cell_as_symbol(left);
      const char *right_sym = akx_rt_cell_as_symbol(right);
      equal = (strcmp(left_sym, right_sym) == 0);
      break;
    }
    default:
      equal = 0;
      break;
    }
  }

  if (equal) {
    akx_cell_t *message_cell = akx_rt_list_nth(args, 2);
    if (message_cell) {
      akx_cell_t *msg_evaled = akx_rt_eval(rt, message_cell);
      if (msg_evaled &&
          akx_rt_cell_get_type(msg_evaled) == AKX_TYPE_STRING_LITERAL) {
        const char *msg = akx_rt_cell_as_string(msg_evaled);
        AK24_LOG_ERROR("Assertion failed: %s", msg);
        akx_cell_free(msg_evaled);
      } else {
        AK24_LOG_ERROR("Assertion failed: values are equal");
        if (msg_evaled && akx_rt_cell_get_type(msg_evaled) != AKX_TYPE_LAMBDA) {
          akx_cell_free(msg_evaled);
        }
      }
    } else {
      AK24_LOG_ERROR("Assertion failed: values are equal");
    }
    if (akx_rt_cell_get_type(left) != AKX_TYPE_LAMBDA) {
      akx_cell_free(left);
    }
    if (akx_rt_cell_get_type(right) != AKX_TYPE_LAMBDA) {
      akx_cell_free(right);
    }
    raise(SIGABRT);
  }

  if (akx_rt_cell_get_type(left) != AKX_TYPE_LAMBDA) {
    akx_cell_free(left);
  }
  if (akx_rt_cell_get_type(right) != AKX_TYPE_LAMBDA) {
    akx_cell_free(right);
  }

  akx_cell_t *ret = akx_rt_alloc_cell(rt, AKX_TYPE_SYMBOL);
  akx_rt_set_symbol(rt, ret, "t");
  return ret;
}
