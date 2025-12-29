#include <stddef.h>

akx_cell_t *form_matches(akx_runtime_ctx_t *rt, akx_cell_t *args) {
  if (akx_rt_list_length(args) != 2) {
    akx_rt_error(rt, "form-matches requires 2 arguments (value :form-name)");
    return NULL;
  }

  akx_cell_t *value_cell = akx_rt_list_nth(args, 0);
  akx_cell_t *form_name_cell = akx_rt_list_nth(args, 1);

  akx_cell_t *value_evaled = akx_rt_eval(rt, value_cell);
  if (!value_evaled) {
    return NULL;
  }

  if (form_name_cell->type != AKX_TYPE_SYMBOL) {
    akx_rt_free_cell(rt, value_evaled);
    akx_rt_error(rt, "form name must be a keyword");
    return NULL;
  }

  const char *form_name = akx_rt_cell_as_symbol(form_name_cell);
  if (!form_name || form_name[0] != ':') {
    akx_rt_free_cell(rt, value_evaled);
    akx_rt_error(rt, "form name must be a keyword (symbol starting with ':')");
    return NULL;
  }

  form_name = form_name + 1;
  int matches = akx_rt_cell_matches_form(rt, value_evaled, form_name);

  akx_rt_free_cell(rt, value_evaled);

  akx_cell_t *result = akx_rt_alloc_cell(rt, AKX_TYPE_INTEGER_LITERAL);
  akx_rt_set_int(rt, result, matches ? 1 : 0);
  return result;
}
