#include <stddef.h>

akx_cell_t *form_add_affordance(akx_runtime_ctx_t *rt, akx_cell_t *args) {
  if (akx_rt_list_length(args) < 3) {
    akx_rt_error(rt, "form-add-affordance requires at least 3 arguments "
                     "(:form-name :affordance-name lambda)");
    return NULL;
  }

  akx_cell_t *form_name_cell = akx_rt_list_nth(args, 0);
  akx_cell_t *affordance_name_cell = akx_rt_list_nth(args, 1);
  akx_cell_t *lambda_cell = akx_rt_list_nth(args, 2);

  if (form_name_cell->type != AKX_TYPE_SYMBOL) {
    akx_rt_error(rt, "form name must be a keyword");
    return NULL;
  }

  const char *form_name = akx_rt_cell_as_symbol(form_name_cell);
  if (!form_name || form_name[0] != ':') {
    akx_rt_error(rt, "form name must be a keyword (symbol starting with ':')");
    return NULL;
  }
  form_name = form_name + 1;

  if (affordance_name_cell->type != AKX_TYPE_SYMBOL) {
    akx_rt_error(rt, "affordance name must be a keyword");
    return NULL;
  }

  const char *affordance_name = akx_rt_cell_as_symbol(affordance_name_cell);
  if (!affordance_name || affordance_name[0] != ':') {
    akx_rt_error(
        rt, "affordance name must be a keyword (symbol starting with ':')");
    return NULL;
  }
  affordance_name = affordance_name + 1;

  akx_cell_t *lambda_evaled = akx_rt_eval_and_assert(
      rt, lambda_cell, AKX_TYPE_LAMBDA, "implementation must be a lambda");
  if (!lambda_evaled) {
    return NULL;
  }

  ak_lambda_t *lambda = akx_rt_cell_as_lambda(lambda_evaled);

  int result = akx_rt_form_add_affordance(rt, form_name, affordance_name,
                                          lambda, NULL, NULL);

  if (result != 0) {
    return NULL;
  }

  akx_cell_t *success = akx_rt_alloc_cell(rt, AKX_TYPE_INTEGER_LITERAL);
  akx_rt_set_int(rt, success, 1);
  return success;
}
