#include <string.h>

static int cells_equal(akx_cell_t *a, akx_cell_t *b) {
  if (!a || !b)
    return 0;

  if (akx_rt_cell_is_type(a, AKX_TYPE_INTEGER_LITERAL) &&
      akx_rt_cell_is_type(b, AKX_TYPE_INTEGER_LITERAL)) {
    return akx_rt_cell_as_int(a) == akx_rt_cell_as_int(b);
  }
  if (akx_rt_cell_is_type(a, AKX_TYPE_REAL_LITERAL) &&
      akx_rt_cell_is_type(b, AKX_TYPE_REAL_LITERAL)) {
    return akx_rt_cell_as_real(a) == akx_rt_cell_as_real(b);
  }
  if (akx_rt_cell_is_type(a, AKX_TYPE_STRING_LITERAL) &&
      akx_rt_cell_is_type(b, AKX_TYPE_STRING_LITERAL)) {
    return strcmp(akx_rt_cell_as_string(a), akx_rt_cell_as_string(b)) == 0;
  }
  if (akx_rt_cell_is_type(a, AKX_TYPE_SYMBOL) &&
      akx_rt_cell_is_type(b, AKX_TYPE_SYMBOL)) {
    return strcmp(akx_rt_cell_as_symbol(a), akx_rt_cell_as_symbol(b)) == 0;
  }
  return 0;
}

static akx_cell_t *clone_cell(akx_runtime_ctx_t *rt, akx_cell_t *cell) {
  if (!cell)
    return NULL;

  if (akx_rt_cell_is_type(cell, AKX_TYPE_INTEGER_LITERAL)) {
    akx_cell_t *result = akx_rt_alloc_cell(rt, AKX_TYPE_INTEGER_LITERAL);
    akx_rt_set_int(rt, result, akx_rt_cell_as_int(cell));
    return result;
  } else if (akx_rt_cell_is_type(cell, AKX_TYPE_REAL_LITERAL)) {
    akx_cell_t *result = akx_rt_alloc_cell(rt, AKX_TYPE_REAL_LITERAL);
    akx_rt_set_real(rt, result, akx_rt_cell_as_real(cell));
    return result;
  } else if (akx_rt_cell_is_type(cell, AKX_TYPE_STRING_LITERAL)) {
    akx_cell_t *result = akx_rt_alloc_cell(rt, AKX_TYPE_STRING_LITERAL);
    akx_rt_set_string(rt, result, akx_rt_cell_as_string(cell));
    return result;
  } else if (akx_rt_cell_is_type(cell, AKX_TYPE_SYMBOL)) {
    akx_cell_t *result = akx_rt_alloc_cell(rt, AKX_TYPE_SYMBOL);
    akx_rt_set_symbol(rt, result, akx_rt_cell_as_symbol(cell));
    return result;
  }
  return cell;
}

akx_cell_t *load_store(akx_runtime_ctx_t *rt, akx_cell_t *args) {
  size_t argc = akx_rt_list_length(args);

  if (argc == 0) {
    akx_rt_error(rt, "load_store requires at least one argument");
    return NULL;
  }

  akx_cell_t *first = akx_rt_list_nth(args, 0);
  if (!first) {
    akx_rt_error(rt, "load_store requires at least one argument");
    return NULL;
  }

  if (!akx_rt_cell_is_type(first, AKX_TYPE_SYMBOL)) {
    akx_rt_error(rt, "load_store requires symbol as first argument");
    return NULL;
  }

  const char *sym = akx_rt_cell_as_symbol(first);

  if (sym[0] == ':') {
    akx_rt_error_fmt(rt, "Special operations not yet implemented: %s", sym);
    return NULL;
  }

  if (argc == 1) {
    void *value = akx_rt_scope_get(rt, sym);
    if (!value) {
      akx_cell_t *result = akx_rt_alloc_cell(rt, AKX_TYPE_SYMBOL);
      akx_rt_set_symbol(rt, result, "nil");
      return result;
    }
    akx_cell_t *stored = (akx_cell_t *)value;
    return clone_cell(rt, stored);
  } else if (argc == 2) {
    akx_cell_t *value_cell = akx_rt_list_nth(args, 1);
    akx_cell_t *evaled_value = akx_rt_eval(rt, value_cell);
    if (!evaled_value) {
      return NULL;
    }
    akx_cell_t *to_store = clone_cell(rt, evaled_value);
    int result = akx_rt_scope_set(rt, sym, to_store);
    akx_cell_t *ret = akx_rt_alloc_cell(rt, AKX_TYPE_INTEGER_LITERAL);
    akx_rt_set_int(rt, ret, result == 0 ? 1 : 0);
    return ret;
  } else if (argc == 3) {
    akx_cell_t *new_value_cell = akx_rt_list_nth(args, 1);
    akx_cell_t *old_value_cell = akx_rt_list_nth(args, 2);
    akx_cell_t *evaled_new = akx_rt_eval(rt, new_value_cell);
    akx_cell_t *evaled_old = akx_rt_eval(rt, old_value_cell);
    if (!evaled_new || !evaled_old) {
      return NULL;
    }
    void *current_value = akx_rt_scope_get(rt, sym);
    int swapped = 0;
    if (current_value) {
      akx_cell_t *current = (akx_cell_t *)current_value;
      if (cells_equal(current, evaled_old)) {
        akx_cell_t *to_store = clone_cell(rt, evaled_new);
        akx_rt_scope_set(rt, sym, to_store);
        swapped = 1;
      }
    }
    akx_cell_t *ret = akx_rt_alloc_cell(rt, AKX_TYPE_INTEGER_LITERAL);
    akx_rt_set_int(rt, ret, swapped);
    return ret;
  } else {
    akx_rt_error(rt, "load_store expects 1-3 arguments");
    return NULL;
  }
}
