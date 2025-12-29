#include <stddef.h>

akx_cell_t *form_invoke(akx_runtime_ctx_t *rt, akx_cell_t *args) {
  if (akx_rt_list_length(args) < 2) {
    akx_rt_error(rt, "form/invoke requires at least 2 arguments "
                     "(value :affordance-name [additional-args...])");
    return NULL;
  }

  akx_cell_t *value_cell = akx_rt_list_nth(args, 0);
  akx_cell_t *affordance_name_cell = akx_rt_list_nth(args, 1);

  akx_cell_t *value = akx_rt_eval(rt, value_cell);
  if (!value) {
    return NULL;
  }

  if (affordance_name_cell->type != AKX_TYPE_SYMBOL) {
    akx_rt_free_cell(rt, value);
    akx_rt_error(rt, "affordance name must be a keyword");
    return NULL;
  }

  const char *affordance_name = akx_rt_cell_as_symbol(affordance_name_cell);
  if (!affordance_name || affordance_name[0] != ':') {
    akx_rt_free_cell(rt, value);
    akx_rt_error(
        rt, "affordance name must be a keyword (symbol starting with ':')");
    return NULL;
  }
  affordance_name = affordance_name + 1;

  map_void_t *forms_map = akx_rt_get_forms(rt);
  if (!forms_map) {
    akx_rt_free_cell(rt, value);
    akx_rt_error(rt, "forms registry not available");
    return NULL;
  }

  ak_affect_t *found_affordance = NULL;

  map_iter_t iter = map_iter(forms_map);
  const char **form_name_ptr;
  while ((form_name_ptr = (const char **)map_next_generic(forms_map, &iter))) {
    const char *form_name = *form_name_ptr;

    if (akx_rt_cell_matches_form(rt, value, form_name)) {
      ak_affect_t *affordance =
          akx_rt_form_get_affordance(rt, form_name, affordance_name);
      if (affordance) {
        found_affordance = affordance;
        break;
      }
    }
  }

  if (!found_affordance) {
    akx_rt_free_cell(rt, value);
    akx_rt_error_fmt(rt, "no form matching the value has affordance '%s'",
                     affordance_name);
    return NULL;
  }

  ak_lambda_t *lambda = found_affordance->lambda;
  if (!lambda) {
    akx_rt_free_cell(rt, value);
    akx_rt_error(rt, "affordance has no implementation");
    return NULL;
  }

  akx_cell_t *lambda_cell = akx_rt_alloc_cell(rt, AKX_TYPE_LAMBDA);
  akx_rt_set_lambda(rt, lambda_cell, lambda);

  akx_cell_t *invoke_args = value;
  if (akx_rt_list_length(args) > 2) {
    akx_cell_t *current = value;
    for (size_t i = 2; i < akx_rt_list_length(args); i++) {
      akx_cell_t *arg = akx_rt_list_nth(args, i);
      akx_cell_t *evaled_arg = akx_rt_eval(rt, arg);
      if (!evaled_arg) {
        akx_rt_free_cell(rt, value);
        return NULL;
      }
      current->next = evaled_arg;
      current = evaled_arg;
    }
  }

  akx_cell_t *result = akx_rt_invoke_lambda(rt, lambda_cell, invoke_args);

  akx_rt_free_cell(rt, value);

  return result;
}
