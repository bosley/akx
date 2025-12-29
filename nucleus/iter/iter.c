#include <string.h>

static size_t utf8_char_length(const char *str) {
  unsigned char c = (unsigned char)str[0];
  if ((c & 0x80) == 0) return 1;
  if ((c & 0xE0) == 0xC0) return 2;
  if ((c & 0xF0) == 0xE0) return 3;
  if ((c & 0xF8) == 0xF0) return 4;
  return 1;
}

static akx_cell_t *iter_list(akx_runtime_ctx_t *rt, akx_cell_t *list, akx_cell_t *lambda_cell) {
  akx_cell_t *result_head = NULL;
  akx_cell_t *result_tail = NULL;
  
  akx_cell_t *current = list;
  while (current) {
    akx_cell_t *item_clone = akx_cell_clone(current);
    if (item_clone) {
      item_clone->next = NULL;
    }
    
    akx_cell_t *transformed = akx_rt_invoke_lambda(rt, lambda_cell, item_clone);
    if (item_clone) {
      akx_cell_free(item_clone);
    }
    
    if (!transformed) {
      if (result_head) {
        akx_cell_free(result_head);
      }
      return NULL;
    }
    
    if (!result_head) {
      result_head = transformed;
      result_tail = transformed;
    } else {
      result_tail->next = transformed;
      result_tail = transformed;
    }
    
    current = akx_rt_cell_next(current);
  }
  
  if (!result_head) {
    result_head = akx_rt_alloc_cell(rt, AKX_TYPE_SYMBOL);
    akx_rt_set_symbol(rt, result_head, "nil");
    return result_head;
  }
  
  // Wrap the result list in a LIST cell
  akx_cell_t *list_cell = akx_rt_alloc_cell(rt, AKX_TYPE_LIST);
  list_cell->value.list_head = result_head;
  list_cell->next = NULL;
  
  return list_cell;
}

static akx_cell_t *iter_string(akx_runtime_ctx_t *rt, const char *str, akx_cell_t *lambda_cell) {
  size_t str_len = strlen(str);
  size_t result_capacity = str_len * 4 + 1;
  char *result_str = AK24_ALLOC(result_capacity);
  if (!result_str) {
    akx_rt_error(rt, "iter: memory allocation failed");
    return NULL;
  }
  result_str[0] = '\0';
  size_t result_pos = 0;
  
  size_t pos = 0;
  while (pos < str_len) {
    size_t char_len = utf8_char_length(str + pos);
    if (pos + char_len > str_len) {
      char_len = str_len - pos;
    }
    
    char utf8_char[5] = {0};
    strncpy(utf8_char, str + pos, char_len);
    utf8_char[char_len] = '\0';
    
    akx_cell_t *char_cell = akx_rt_alloc_cell(rt, AKX_TYPE_STRING_LITERAL);
    akx_rt_set_string(rt, char_cell, utf8_char);
    char_cell->next = NULL;
    
    akx_cell_t *transformed = akx_rt_invoke_lambda(rt, lambda_cell, char_cell);
    
    if (!transformed) {
      akx_cell_free(char_cell);
      AK24_FREE(result_str);
      return NULL;
    }
    
    if (akx_rt_cell_get_type(transformed) == AKX_TYPE_STRING_LITERAL) {
      const char *new_char = akx_rt_cell_as_string(transformed);
      if (new_char) {
        size_t new_len = strlen(new_char);
        if (result_pos + new_len >= result_capacity) {
          result_capacity = (result_pos + new_len + 1) * 2;
          char *new_result = AK24_ALLOC(result_capacity);
          if (!new_result) {
            akx_rt_free_cell(rt, char_cell);
            akx_rt_free_cell(rt, transformed);
            AK24_FREE(result_str);
            akx_rt_error(rt, "iter: memory allocation failed");
            return NULL;
          }
          memcpy(new_result, result_str, result_pos);
          AK24_FREE(result_str);
          result_str = new_result;
        }
        memcpy(result_str + result_pos, new_char, new_len);
        result_pos += new_len;
        result_str[result_pos] = '\0';
      }
    }
    
    akx_cell_free(char_cell);
    akx_rt_free_cell(rt, transformed);
    pos += char_len;
  }
  
  akx_cell_t *result = akx_rt_alloc_cell(rt, AKX_TYPE_STRING_LITERAL);
  akx_rt_set_string(rt, result, result_str);
  AK24_FREE(result_str);
  
  return result;
}

static akx_cell_t *iter_integer(akx_runtime_ctx_t *rt, int value, akx_cell_t *lambda_cell) {
  int result = 0;
  
  for (int bit_pos = 0; bit_pos < 32; bit_pos++) {
    int bit_value = (value >> bit_pos) & 1;
    
    akx_cell_t *bit_cell = akx_rt_alloc_cell(rt, AKX_TYPE_INTEGER_LITERAL);
    akx_rt_set_int(rt, bit_cell, bit_value);
    bit_cell->next = NULL;
    
    akx_cell_t *transformed = akx_rt_invoke_lambda(rt, lambda_cell, bit_cell);
    
    if (!transformed) {
      akx_cell_free(bit_cell);
      return NULL;
    }
    
    if (akx_rt_cell_get_type(transformed) == AKX_TYPE_INTEGER_LITERAL) {
      int new_bit = akx_rt_cell_as_int(transformed) ? 1 : 0;
      result = (result & ~(1 << bit_pos)) | (new_bit << bit_pos);
    }
    
    akx_cell_free(bit_cell);
    akx_rt_free_cell(rt, transformed);
  }
  
  akx_cell_t *result_cell = akx_rt_alloc_cell(rt, AKX_TYPE_INTEGER_LITERAL);
  akx_rt_set_int(rt, result_cell, result);
  return result_cell;
}

static akx_cell_t *iter_real(akx_runtime_ctx_t *rt, double value, akx_cell_t *lambda_cell) {
  union {
    double d;
    uint64_t u;
  } converter;
  converter.d = value;
  uint64_t bits = converter.u;
  
  uint64_t result = 0;
  
  for (int bit_pos = 0; bit_pos < 64; bit_pos++) {
    int bit_value = (bits >> bit_pos) & 1;
    
    akx_cell_t *bit_cell = akx_rt_alloc_cell(rt, AKX_TYPE_INTEGER_LITERAL);
    akx_rt_set_int(rt, bit_cell, bit_value);
    bit_cell->next = NULL;
    
    akx_cell_t *transformed = akx_rt_invoke_lambda(rt, lambda_cell, bit_cell);
    
    if (!transformed) {
      akx_cell_free(bit_cell);
      return NULL;
    }
    
    if (akx_rt_cell_get_type(transformed) == AKX_TYPE_INTEGER_LITERAL) {
      uint64_t new_bit = akx_rt_cell_as_int(transformed) ? 1ULL : 0ULL;
      result = (result & ~(1ULL << bit_pos)) | (new_bit << bit_pos);
    }
    
    akx_cell_free(bit_cell);
    akx_rt_free_cell(rt, transformed);
  }
  
  converter.u = result;
  
  akx_cell_t *result_cell = akx_rt_alloc_cell(rt, AKX_TYPE_REAL_LITERAL);
  akx_rt_set_real(rt, result_cell, converter.d);
  return result_cell;
}

akx_cell_t *iter_impl(akx_runtime_ctx_t *rt, akx_cell_t *args) {
  if (akx_rt_list_length(args) != 2) {
    akx_rt_error(rt, "iter: requires exactly 2 arguments (value lambda)");
    return NULL;
  }
  
  akx_cell_t *value_arg = akx_rt_list_nth(args, 0);
  akx_cell_t *evaled_value = akx_rt_eval(rt, value_arg);
  if (!evaled_value) {
    return NULL;
  }
  
  akx_cell_t *lambda_arg = akx_rt_list_nth(args, 1);
  akx_cell_t *lambda_cell = akx_rt_eval(rt, lambda_arg);
  if (!lambda_cell) {
    if (akx_rt_cell_get_type(evaled_value) != AKX_TYPE_LAMBDA) {
      akx_rt_free_cell(rt, evaled_value);
    }
    return NULL;
  }
  
  if (akx_rt_cell_get_type(lambda_cell) != AKX_TYPE_LAMBDA) {
    if (akx_rt_cell_get_type(evaled_value) != AKX_TYPE_LAMBDA) {
      akx_rt_free_cell(rt, evaled_value);
    }
    akx_rt_error(rt, "iter: second argument must be a lambda");
    return NULL;
  }
  
  akx_type_t value_type = akx_rt_cell_get_type(evaled_value);
  akx_cell_t *result = NULL;
  
  // Check if it's nil (empty list)
  if (value_type == AKX_TYPE_SYMBOL) {
    const char *sym = akx_rt_cell_as_symbol(evaled_value);
    if (sym && strcmp(sym, "nil") == 0) {
      // Empty list - return nil
      result = akx_rt_alloc_cell(rt, AKX_TYPE_SYMBOL);
      akx_rt_set_symbol(rt, result, "nil");
      if (value_type != AKX_TYPE_LAMBDA) {
        akx_rt_free_cell(rt, evaled_value);
      }
      return result;
    }
  }
  
  switch (value_type) {
  case AKX_TYPE_LIST:
  case AKX_TYPE_LIST_SQUARE:
  case AKX_TYPE_LIST_CURLY:
  case AKX_TYPE_LIST_TEMPLE: {
    akx_cell_t *list_head = akx_rt_cell_as_list(evaled_value);
    result = iter_list(rt, list_head, lambda_cell);
    break;
  }
  
  case AKX_TYPE_STRING_LITERAL: {
    const char *str = akx_rt_cell_as_string(evaled_value);
    if (str) {
      result = iter_string(rt, str, lambda_cell);
    } else {
      akx_rt_error(rt, "iter: invalid string");
    }
    break;
  }
  
  case AKX_TYPE_INTEGER_LITERAL: {
    int value = akx_rt_cell_as_int(evaled_value);
    result = iter_integer(rt, value, lambda_cell);
    break;
  }
  
  case AKX_TYPE_REAL_LITERAL: {
    double value = akx_rt_cell_as_real(evaled_value);
    result = iter_real(rt, value, lambda_cell);
    break;
  }
  
  case AKX_TYPE_LAMBDA:
  case AKX_TYPE_SYMBOL:
  case AKX_TYPE_QUOTED:
  case AKX_TYPE_CONTINUATION:
    akx_rt_error_fmt(rt, "iter: cannot iterate over type %d (lambda, symbol, quoted, or continuation)", value_type);
    break;
    
  default:
    akx_rt_error_fmt(rt, "iter: unsupported type %d", value_type);
    break;
  }
  
  if (value_type != AKX_TYPE_LAMBDA) {
    akx_rt_free_cell(rt, evaled_value);
  }
  
  return result;
}

