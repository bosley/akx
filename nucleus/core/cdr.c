akx_cell_t *cdr_impl(akx_runtime_ctx_t *rt, akx_cell_t *args) {
  if (akx_rt_list_length(args) != 1) {
    akx_rt_error(rt, "cdr requires exactly 1 argument");
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
    akx_rt_error(rt, "cdr: argument must be a list");
    akx_rt_free_cell(rt, list);
    return NULL;
  }

  akx_cell_t *list_head = akx_rt_cell_as_list(list);
  if (!list_head) {
    akx_cell_t *nil = akx_rt_alloc_cell(rt, AKX_TYPE_SYMBOL);
    akx_rt_set_symbol(rt, nil, "nil");
    akx_rt_free_cell(rt, list);
    return nil;
  }

  akx_cell_t *rest = list_head->next;
  if (!rest) {
    akx_cell_t *nil = akx_rt_alloc_cell(rt, AKX_TYPE_SYMBOL);
    akx_rt_set_symbol(rt, nil, "nil");
    akx_rt_free_cell(rt, list);
    return nil;
  }

  akx_cell_t *rest_clone = akx_cell_clone(rest);
  if (!rest_clone) {
    akx_rt_error(rt, "cdr: failed to clone rest of list");
    akx_rt_free_cell(rt, list);
    return NULL;
  }

  akx_cell_t *result = akx_rt_alloc_cell(rt, AKX_TYPE_LIST);
  if (!result) {
    akx_rt_error(rt, "cdr: failed to allocate result cell");
    akx_cell_free(rest_clone);
    akx_rt_free_cell(rt, list);
    return NULL;
  }

  akx_rt_set_list(rt, result, rest_clone);

  akx_rt_free_cell(rt, list);

  return result;
}
