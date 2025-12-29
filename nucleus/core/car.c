akx_cell_t *car_impl(akx_runtime_ctx_t *rt, akx_cell_t *args) {
  if (akx_rt_list_length(args) != 1) {
    akx_rt_error(rt, "car requires exactly 1 argument");
    return NULL;
  }

  akx_cell_t *list_cell = akx_rt_list_nth(args, 0);
  akx_cell_t *list = akx_rt_eval(rt, list_cell);
  if (!list) {
    return NULL;
  }

  akx_type_t list_type = akx_rt_cell_get_type(list);
  if (list_type != AKX_TYPE_LIST && list_type != AKX_TYPE_LIST_SQUARE &&
      list_type != AKX_TYPE_LIST_CURLY && list_type != AKX_TYPE_LIST_TEMPLE) {
    akx_rt_error(rt, "car: argument must be a list");
    akx_rt_free_cell(rt, list);
    return NULL;
  }

  akx_cell_t *list_head = akx_rt_cell_as_list(list);
  if (!list_head) {
    akx_rt_error(rt, "car: list is empty");
    akx_rt_free_cell(rt, list);
    return NULL;
  }

  akx_cell_t *result = akx_cell_clone(list_head);
  if (!result) {
    akx_rt_error(rt, "car: failed to clone first element");
    akx_rt_free_cell(rt, list);
    return NULL;
  }

  result->next = NULL;

  akx_rt_free_cell(rt, list);

  return result;
}
