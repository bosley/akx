akx_cell_t *cons_impl(akx_runtime_ctx_t *rt, akx_cell_t *args) {
  if (akx_rt_list_length(args) != 2) {
    akx_rt_error(rt, "cons requires exactly 2 arguments");
    return NULL;
  }

  akx_cell_t *element_cell = akx_rt_list_nth(args, 0);
  akx_cell_t *list_cell = akx_rt_list_nth(args, 1);

  akx_cell_t *element = akx_rt_eval(rt, element_cell);
  if (!element) {
    return NULL;
  }

  akx_cell_t *list = akx_rt_eval(rt, list_cell);
  if (!list) {
    akx_rt_free_cell(rt, element);
    return NULL;
  }

  akx_type_t list_type = akx_rt_cell_get_type(list);
  if (list_type != AKX_TYPE_LIST && list_type != AKX_TYPE_LIST_SQUARE &&
      list_type != AKX_TYPE_LIST_CURLY && list_type != AKX_TYPE_LIST_TEMPLE) {
    akx_rt_error(rt, "cons: second argument must be a list");
    akx_rt_free_cell(rt, element);
    akx_rt_free_cell(rt, list);
    return NULL;
  }

  akx_cell_t *list_head = akx_rt_cell_as_list(list);
  akx_cell_t *element_clone = akx_cell_clone(element);
  if (!element_clone) {
    akx_rt_error(rt, "cons: failed to clone element");
    akx_rt_free_cell(rt, element);
    akx_rt_free_cell(rt, list);
    return NULL;
  }

  element_clone->next = NULL;

  if (list_head) {
    akx_cell_t *list_head_clone = akx_cell_clone(list_head);
    if (!list_head_clone) {
      akx_rt_error(rt, "cons: failed to clone list");
      akx_cell_free(element_clone);
      akx_rt_free_cell(rt, element);
      akx_rt_free_cell(rt, list);
      return NULL;
    }
    element_clone->next = list_head_clone;
  }

  akx_cell_t *result = akx_rt_alloc_cell(rt, AKX_TYPE_LIST);
  if (!result) {
    akx_rt_error(rt, "cons: failed to allocate result cell");
    akx_cell_free(element_clone);
    akx_rt_free_cell(rt, element);
    akx_rt_free_cell(rt, list);
    return NULL;
  }

  akx_rt_set_list(rt, result, element_clone);

  akx_rt_free_cell(rt, element);
  akx_rt_free_cell(rt, list);

  return result;
}
