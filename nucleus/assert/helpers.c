static int is_truthy(akx_cell_t *cell) {
  if (!cell) {
    return 0;
  }

  if (akx_rt_cell_get_type(cell) == AKX_TYPE_SYMBOL) {
    const char *sym = cell->value.symbol;
    if (strcmp(sym, "nil") == 0 || strcmp(sym, "false") == 0) {
      return 0;
    }
    return 1;
  }

  if (akx_rt_cell_get_type(cell) == AKX_TYPE_INTEGER_LITERAL) {
    return cell->value.integer_literal != 0;
  }

  if (akx_rt_cell_get_type(cell) == AKX_TYPE_REAL_LITERAL) {
    return cell->value.real_literal != 0.0;
  }

  return 1;
}
