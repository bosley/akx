#include "akx_rt_compiler.h"
#include <string.h>

int akx_compiler_extract_string_list(akx_runtime_ctx_t *rt,
                                     akx_cell_t *list_cell,
                                     const char ***out_strings,
                                     size_t *out_count) {
  if (!list_cell || list_cell->type != AKX_TYPE_LIST_CURLY) {
    akx_rt_error(rt, "expected curly-brace list");
    return -1;
  }

  akx_cell_t *items = list_cell->value.list_head;
  size_t count = akx_rt_list_length(items);

  if (count == 0) {
    *out_strings = NULL;
    *out_count = 0;
    return 0;
  }

  const char **strings = AK24_ALLOC(sizeof(char *) * count);
  if (!strings) {
    akx_rt_error(rt, "failed to allocate string list");
    return -1;
  }

  akx_cell_t *current = items;
  size_t index = 0;
  while (current) {
    akx_cell_t *evaled = akx_rt_eval_and_assert(
        rt, current, AKX_TYPE_STRING_LITERAL, "list items must be strings");
    if (!evaled) {
      for (size_t i = 0; i < index; i++) {
        AK24_FREE((void *)strings[i]);
      }
      AK24_FREE(strings);
      return -1;
    }

    const char *str = akx_rt_cell_as_string(evaled);
    size_t len = strlen(str);
    char *dup = AK24_ALLOC(len + 1);
    if (!dup) {
      akx_rt_free_cell(rt, evaled);
      for (size_t i = 0; i < index; i++) {
        AK24_FREE((void *)strings[i]);
      }
      AK24_FREE(strings);
      return -1;
    }
    strcpy(dup, str);
    strings[index] = dup;
    akx_rt_free_cell(rt, evaled);
    index++;
    current = akx_rt_cell_next(current);
  }

  *out_strings = strings;
  *out_count = count;
  return 0;
}

int akx_compiler_extract_define_list(akx_runtime_ctx_t *rt,
                                     akx_cell_t *list_cell,
                                     const char ***out_names,
                                     const char ***out_values,
                                     size_t *out_count) {
  if (!list_cell || list_cell->type != AKX_TYPE_LIST_CURLY) {
    akx_rt_error(rt, "expected curly-brace list for defines");
    return -1;
  }

  akx_cell_t *items = list_cell->value.list_head;
  size_t count = akx_rt_list_length(items);

  if (count == 0) {
    *out_names = NULL;
    *out_values = NULL;
    *out_count = 0;
    return 0;
  }

  const char **names = AK24_ALLOC(sizeof(char *) * count);
  const char **values = AK24_ALLOC(sizeof(char *) * count);
  if (!names || !values) {
    if (names)
      AK24_FREE(names);
    if (values)
      AK24_FREE(values);
    akx_rt_error(rt, "failed to allocate define lists");
    return -1;
  }

  akx_cell_t *current = items;
  size_t index = 0;
  while (current) {
    if (current->type != AKX_TYPE_LIST_SQUARE) {
      akx_rt_error(rt, "define must be a square-bracket list [name value]");
      AK24_FREE(names);
      AK24_FREE(values);
      return -1;
    }

    akx_cell_t *pair = current->value.list_head;
    if (akx_rt_list_length(pair) != 2) {
      akx_rt_error(rt, "define must have exactly 2 elements [name value]");
      AK24_FREE(names);
      AK24_FREE(values);
      return -1;
    }

    akx_cell_t *name_cell = akx_rt_list_nth(pair, 0);
    akx_cell_t *value_cell = akx_rt_list_nth(pair, 1);

    akx_cell_t *name_evaled = akx_rt_eval_and_assert(
        rt, name_cell, AKX_TYPE_STRING_LITERAL, "define name must be a string");
    if (!name_evaled) {
      AK24_FREE(names);
      AK24_FREE(values);
      return -1;
    }

    akx_cell_t *value_evaled =
        akx_rt_eval_and_assert(rt, value_cell, AKX_TYPE_STRING_LITERAL,
                               "define value must be a string");
    if (!value_evaled) {
      akx_rt_free_cell(rt, name_evaled);
      AK24_FREE(names);
      AK24_FREE(values);
      return -1;
    }

    const char *name_str = akx_rt_cell_as_string(name_evaled);
    const char *value_str = akx_rt_cell_as_string(value_evaled);

    size_t name_len = strlen(name_str);
    size_t value_len = strlen(value_str);

    char *name_dup = AK24_ALLOC(name_len + 1);
    char *value_dup = AK24_ALLOC(value_len + 1);

    if (!name_dup || !value_dup) {
      if (name_dup)
        AK24_FREE(name_dup);
      if (value_dup)
        AK24_FREE(value_dup);
      akx_rt_free_cell(rt, name_evaled);
      akx_rt_free_cell(rt, value_evaled);
      for (size_t i = 0; i < index; i++) {
        AK24_FREE((void *)names[i]);
        AK24_FREE((void *)values[i]);
      }
      AK24_FREE(names);
      AK24_FREE(values);
      return -1;
    }

    strcpy(name_dup, name_str);
    strcpy(value_dup, value_str);

    names[index] = name_dup;
    values[index] = value_dup;
    akx_rt_free_cell(rt, name_evaled);
    akx_rt_free_cell(rt, value_evaled);

    index++;
    current = akx_rt_cell_next(current);
  }

  *out_names = names;
  *out_values = values;
  *out_count = count;
  return 0;
}

int akx_compiler_parse_linker_options(akx_runtime_ctx_t *rt,
                                      akx_cell_t *linker_cell,
                                      akx_builtin_compile_opts_t *opts) {
  if (!linker_cell || linker_cell->type != AKX_TYPE_LIST_CURLY) {
    akx_rt_error(rt, ":linker value must be a curly-brace list");
    return -1;
  }

  akx_cell_t *linker_items = linker_cell->value.list_head;
  akx_cell_t *current = linker_items;

  while (current) {
    if (current->type != AKX_TYPE_SYMBOL) {
      akx_rt_error(rt, "expected keyword in :linker block");
      return -1;
    }

    const char *keyword = akx_rt_cell_as_symbol(current);
    if (keyword[0] != ':') {
      akx_rt_error(rt, "linker keywords must start with ':'");
      return -1;
    }

    current = akx_rt_cell_next(current);
    if (!current) {
      akx_rt_error_fmt(rt, "missing value for keyword %s", keyword);
      return -1;
    }

    if (strcmp(keyword, ":library-paths") == 0) {
      if (akx_compiler_extract_string_list(rt, current, &opts->library_paths,
                                           &opts->library_path_count) != 0) {
        return -1;
      }
    } else if (strcmp(keyword, ":libraries") == 0) {
      if (akx_compiler_extract_string_list(rt, current, &opts->libraries,
                                           &opts->library_count) != 0) {
        return -1;
      }
    } else if (strcmp(keyword, ":defines") == 0) {
      if (akx_compiler_extract_define_list(rt, current, &opts->define_names,
                                           &opts->define_values,
                                           &opts->define_count) != 0) {
        return -1;
      }
    } else {
      akx_rt_error_fmt(rt, "unknown linker keyword: %s", keyword);
      return -1;
    }

    current = akx_rt_cell_next(current);
  }

  return 0;
}

void akx_compiler_free_compile_opts(akx_builtin_compile_opts_t *opts) {
  if (!opts) {
    return;
  }

  if (opts->include_paths) {
    for (size_t i = 0; i < opts->include_path_count; i++) {
      AK24_FREE((void *)opts->include_paths[i]);
    }
    AK24_FREE(opts->include_paths);
  }

  if (opts->impl_files) {
    for (size_t i = 0; i < opts->impl_file_count; i++) {
      AK24_FREE((void *)opts->impl_files[i]);
    }
    AK24_FREE(opts->impl_files);
  }

  if (opts->library_paths) {
    for (size_t i = 0; i < opts->library_path_count; i++) {
      AK24_FREE((void *)opts->library_paths[i]);
    }
    AK24_FREE(opts->library_paths);
  }

  if (opts->libraries) {
    for (size_t i = 0; i < opts->library_count; i++) {
      AK24_FREE((void *)opts->libraries[i]);
    }
    AK24_FREE(opts->libraries);
  }

  if (opts->define_names) {
    for (size_t i = 0; i < opts->define_count; i++) {
      AK24_FREE((void *)opts->define_names[i]);
    }
    AK24_FREE(opts->define_names);
  }

  if (opts->define_values) {
    for (size_t i = 0; i < opts->define_count; i++) {
      AK24_FREE((void *)opts->define_values[i]);
    }
    AK24_FREE(opts->define_values);
  }
}
