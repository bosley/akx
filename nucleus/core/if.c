#include <stddef.h>

akx_cell_t *if_impl(akx_runtime_ctx_t *rt, akx_cell_t *args) {
  size_t arg_count = akx_rt_list_length(args);
  if (arg_count < 2 || arg_count > 3) {
    akx_rt_error(rt, "if: requires 2 or 3 arguments (condition then-branch "
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

  int is_true = 0;
  if (condition->type == AKX_TYPE_INTEGER_LITERAL &&
      condition->value.integer_literal == 1) {
    is_true = 1;
  }

  if (condition->type != AKX_TYPE_LAMBDA) {
    akx_rt_free_cell(rt, condition);
  }

  if (is_true) {
    return akx_rt_eval(rt, then_branch);
  } else if (else_branch) {
    return akx_rt_eval(rt, else_branch);
  } else {
    akx_cell_t *nil = akx_rt_alloc_cell(rt, AKX_TYPE_SYMBOL);
    akx_rt_set_symbol(rt, nil, "nil");
    return nil;
  }
}
