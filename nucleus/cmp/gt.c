akx_cell_t *gt_impl(akx_runtime_ctx_t *rt, akx_cell_t *args) {
  if (akx_rt_list_length(args) != 2) {
    akx_rt_error(rt, "gt requires exactly 2 arguments");
    return NULL;
  }

  akx_cell_t *left_cell = akx_rt_list_nth(args, 0);
  akx_cell_t *right_cell = akx_rt_list_nth(args, 1);

  akx_cell_t *left = akx_rt_eval(rt, left_cell);
  if (!left) {
    return NULL;
  }

  akx_cell_t *right = akx_rt_eval(rt, right_cell);
  if (!right) {
    if (akx_rt_cell_get_type(left) != AKX_TYPE_LAMBDA) {
      akx_rt_free_cell(rt, left);
    }
    return NULL;
  }

  akx_type_t left_type = akx_rt_cell_get_type(left);
  akx_type_t right_type = akx_rt_cell_get_type(right);

  int result = 0;
  if (left_type != right_type) {
    akx_rt_error(rt, "gt: both operands must be the same type");
  } else if (left_type == AKX_TYPE_INTEGER_LITERAL) {
    result = (akx_rt_cell_as_int(left) > akx_rt_cell_as_int(right));
  } else if (left_type == AKX_TYPE_REAL_LITERAL) {
    result = (akx_rt_cell_as_real(left) > akx_rt_cell_as_real(right));
  } else {
    akx_rt_error(rt, "gt: operands must be integer or real");
  }

  int has_error = (result == 0 && (left_type != right_type || 
                   (left_type != AKX_TYPE_INTEGER_LITERAL && 
                    left_type != AKX_TYPE_REAL_LITERAL)));

  if (akx_rt_cell_get_type(left) != AKX_TYPE_LAMBDA) {
    akx_rt_free_cell(rt, left);
  }
  if (akx_rt_cell_get_type(right) != AKX_TYPE_LAMBDA) {
    akx_rt_free_cell(rt, right);
  }

  if (has_error) {
    return NULL;
  }

  akx_cell_t *ret = akx_rt_alloc_cell(rt, AKX_TYPE_INTEGER_LITERAL);
  akx_rt_set_int(rt, ret, result);
  return ret;
}

