#include <string.h>

akx_cell_t *str_split_impl(akx_runtime_ctx_t *rt, akx_cell_t *args) {
  size_t arg_count = akx_rt_list_length(args);
  if (arg_count < 3) {
    akx_rt_error(rt, "str/split requires at least 3 arguments (string :idx N "
                     "or string :sub \"substr\")");
    return NULL;
  }

  akx_cell_t *str_arg = akx_rt_list_nth(args, 0);
  akx_cell_t *evaled_str =
      akx_rt_eval_and_assert(rt, str_arg, AKX_TYPE_STRING_LITERAL,
                             "str/split: first argument must be a string");
  if (!evaled_str) {
    return NULL;
  }

  const char *input_str = akx_rt_cell_as_string(evaled_str);
  if (!input_str) {
    akx_rt_free_cell(rt, evaled_str);
    akx_rt_error(rt, "str/split: invalid string");
    return NULL;
  }

  akx_cell_t *mode_arg = akx_rt_list_nth(args, 1);
  if (!mode_arg || akx_rt_cell_get_type(mode_arg) != AKX_TYPE_SYMBOL) {
    akx_rt_free_cell(rt, evaled_str);
    akx_rt_error(rt, "str/split: second argument must be :idx or :sub");
    return NULL;
  }

  const char *keyword = akx_rt_cell_as_symbol(mode_arg);
  if (!keyword || keyword[0] != ':') {
    akx_rt_free_cell(rt, evaled_str);
    akx_rt_error(rt, "str/split: keywords must start with ':'");
    return NULL;
  }

  const char *mode = keyword + 1;
  akx_cell_t *result = NULL;

  if (strcmp(mode, "idx") == 0) {
    akx_cell_t *idx_arg = akx_rt_list_nth(args, 2);
    akx_cell_t *evaled_idx =
        akx_rt_eval_and_assert(rt, idx_arg, AKX_TYPE_INTEGER_LITERAL,
                               "str/split :idx requires integer index");
    if (!evaled_idx) {
      akx_rt_free_cell(rt, evaled_str);
      return NULL;
    }

    int split_idx = akx_rt_cell_as_int(evaled_idx);
    size_t str_len = strlen(input_str);

    if (split_idx < 0 || (size_t)split_idx > str_len) {
      akx_rt_free_cell(rt, evaled_str);
      akx_rt_free_cell(rt, evaled_idx);
      akx_rt_error(rt, "str/split: index out of bounds");
      return NULL;
    }

    char *left = AK24_ALLOC(split_idx + 1);
    char *right = AK24_ALLOC(str_len - split_idx + 1);
    if (!left || !right) {
      if (left)
        AK24_FREE(left);
      if (right)
        AK24_FREE(right);
      akx_rt_free_cell(rt, evaled_str);
      akx_rt_free_cell(rt, evaled_idx);
      akx_rt_error(rt, "str/split: memory allocation failed");
      return NULL;
    }

    strncpy(left, input_str, split_idx);
    left[split_idx] = '\0';
    strcpy(right, input_str + split_idx);

    akx_cell_t *left_cell = akx_rt_alloc_cell(rt, AKX_TYPE_STRING_LITERAL);
    akx_rt_set_string(rt, left_cell, left);

    akx_cell_t *right_cell = akx_rt_alloc_cell(rt, AKX_TYPE_STRING_LITERAL);
    akx_rt_set_string(rt, right_cell, right);

    left_cell->next = right_cell;
    result = left_cell;

    AK24_FREE(left);
    AK24_FREE(right);
    akx_rt_free_cell(rt, evaled_idx);

  } else if (strcmp(mode, "sub") == 0) {
    akx_cell_t *sub_arg = akx_rt_list_nth(args, 2);
    akx_cell_t *evaled_sub =
        akx_rt_eval_and_assert(rt, sub_arg, AKX_TYPE_STRING_LITERAL,
                               "str/split :sub requires string substring");
    if (!evaled_sub) {
      akx_rt_free_cell(rt, evaled_str);
      return NULL;
    }

    const char *substring = akx_rt_cell_as_string(evaled_sub);
    const char *found = strstr(input_str, substring);

    if (!found) {
      akx_cell_t *full_cell = akx_rt_alloc_cell(rt, AKX_TYPE_STRING_LITERAL);
      akx_rt_set_string(rt, full_cell, input_str);
      result = full_cell;
    } else {
      size_t left_len = found - input_str;
      size_t sub_len = strlen(substring);
      size_t right_len = strlen(found + sub_len);

      char *left = AK24_ALLOC(left_len + 1);
      char *right = AK24_ALLOC(right_len + 1);
      if (!left || !right) {
        if (left)
          AK24_FREE(left);
        if (right)
          AK24_FREE(right);
        akx_rt_free_cell(rt, evaled_str);
        akx_rt_free_cell(rt, evaled_sub);
        akx_rt_error(rt, "str/split: memory allocation failed");
        return NULL;
      }

      strncpy(left, input_str, left_len);
      left[left_len] = '\0';
      strcpy(right, found + sub_len);

      akx_cell_t *left_cell = akx_rt_alloc_cell(rt, AKX_TYPE_STRING_LITERAL);
      akx_rt_set_string(rt, left_cell, left);

      akx_cell_t *right_cell = akx_rt_alloc_cell(rt, AKX_TYPE_STRING_LITERAL);
      akx_rt_set_string(rt, right_cell, right);

      left_cell->next = right_cell;
      result = left_cell;

      AK24_FREE(left);
      AK24_FREE(right);
    }

    akx_rt_free_cell(rt, evaled_sub);

  } else {
    akx_rt_free_cell(rt, evaled_str);
    akx_rt_error_fmt(rt, "str/split: unknown keyword: %s", keyword);
    return NULL;
  }

  akx_rt_free_cell(rt, evaled_str);

  return result;
}
