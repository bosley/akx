akx_cell_t *ifcond(akx_runtime_ctx_t *rt, akx_cell_t *args) {
  if (akx_rt_list_length(args) < 2) {
    akx_rt_error(rt, "if requires at least 2 arguments (condition then-branch "
                     "[else-branch])");
    return NULL;
  }

  akx_cell_t *condition_cell = akx_rt_list_nth(args, 0);
  akx_cell_t *then_branch = akx_rt_list_nth(args, 1);
  akx_cell_t *else_branch = akx_rt_list_nth(args, 2);

  akx_cell_t *condition = akx_rt_eval(rt, condition_cell);
  if (!condition) {
    return NULL;
  }

  int is_truthy = 0;
  if (condition->type == AKX_TYPE_SYMBOL) {
    const char *sym = akx_rt_cell_as_symbol(condition);
    if (sym && strcmp(sym, "nil") != 0 && strcmp(sym, "false") != 0) {
      is_truthy = 1;
    }
  } else if (condition->type == AKX_TYPE_INTEGER_LITERAL) {
    is_truthy = (akx_rt_cell_as_int(condition) != 0);
  } else if (condition->type == AKX_TYPE_REAL_LITERAL) {
    is_truthy = (akx_rt_cell_as_real(condition) != 0.0);
  } else {
    is_truthy = 1;
  }

  akx_rt_free_cell(rt, condition);

  if (is_truthy) {
    return akx_rt_eval(rt, then_branch);
  } else if (else_branch) {
    return akx_rt_eval(rt, else_branch);
  } else {
    akx_cell_t *nil = akx_rt_alloc_cell(rt, AKX_TYPE_SYMBOL);
    akx_rt_set_symbol(rt, nil, "nil");
    return nil;
  }
}
