akx_cell_t *eqtest(akx_runtime_ctx_t *rt, akx_cell_t *args) {
  if (akx_rt_list_length(args) != 2) {
    akx_rt_error(rt, "= requires exactly 2 arguments");
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
    akx_rt_free_cell(rt, left);
    return NULL;
  }

  int equal = 0;
  if (left->type == right->type) {
    if (left->type == AKX_TYPE_INTEGER_LITERAL) {
      equal = (akx_rt_cell_as_int(left) == akx_rt_cell_as_int(right));
    } else if (left->type == AKX_TYPE_REAL_LITERAL) {
      equal = (akx_rt_cell_as_real(left) == akx_rt_cell_as_real(right));
    } else if (left->type == AKX_TYPE_STRING_LITERAL) {
      const char *ls = akx_rt_cell_as_string(left);
      const char *rs = akx_rt_cell_as_string(right);
      equal = (strcmp(ls, rs) == 0);
    } else if (left->type == AKX_TYPE_SYMBOL) {
      const char *ls = akx_rt_cell_as_symbol(left);
      const char *rs = akx_rt_cell_as_symbol(right);
      equal = (strcmp(ls, rs) == 0);
    }
  }

  akx_rt_free_cell(rt, left);
  akx_rt_free_cell(rt, right);

  akx_cell_t *result = akx_rt_alloc_cell(rt, AKX_TYPE_SYMBOL);
  akx_rt_set_symbol(rt, result, equal ? "true" : "false");
  return result;
}
