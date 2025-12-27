#include "akx_rt_compiler.h"
#include <ak24/buffer.h>
#include <ak24/cjit.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

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

const char *akx_compiler_generate_abi_header(void) {
  return "#include <stddef.h>\n"
         "#include <stdint.h>\n"
         "#include <stdio.h>\n"
         "#include <stdlib.h>\n"
         "#include <string.h>\n"
         "\n"
         "typedef struct akx_runtime_ctx_t akx_runtime_ctx_t;\n"
         "typedef struct akx_cell_t akx_cell_t;\n"
         "typedef struct ak_context_t ak_context_t;\n"
         "typedef struct ak_lambda_t ak_lambda_t;\n"
         "\n"
         "typedef enum {\n"
         "  AKX_TYPE_SYMBOL = 0,\n"
         "  AKX_TYPE_STRING_LITERAL = 1,\n"
         "  AKX_TYPE_INTEGER_LITERAL = 2,\n"
         "  AKX_TYPE_REAL_LITERAL = 3,\n"
         "  AKX_TYPE_LIST = 4,\n"
         "  AKX_TYPE_LIST_SQUARE = 5,\n"
         "  AKX_TYPE_LIST_CURLY = 6,\n"
         "  AKX_TYPE_LIST_TEMPLE = 7,\n"
         "  AKX_TYPE_QUOTED = 8,\n"
         "  AKX_TYPE_LAMBDA = 9\n"
         "} akx_type_t;\n"
         "\n"
         "typedef akx_cell_t* (*akx_builtin_fn)(akx_runtime_ctx_t*, "
         "akx_cell_t*);\n"
         "\n"
         "extern akx_type_t akx_rt_cell_get_type(akx_cell_t *cell);\n"
         "\n"
         "extern akx_cell_t* akx_rt_alloc_cell(akx_runtime_ctx_t *rt, "
         "akx_type_t type);\n"
         "extern void akx_rt_free_cell(akx_runtime_ctx_t *rt, akx_cell_t "
         "*cell);\n"
         "\n"
         "extern void akx_rt_set_symbol(akx_runtime_ctx_t *rt, akx_cell_t "
         "*cell, const char *sym);\n"
         "extern void akx_rt_set_int(akx_runtime_ctx_t *rt, akx_cell_t *cell, "
         "int value);\n"
         "extern void akx_rt_set_real(akx_runtime_ctx_t *rt, akx_cell_t *cell, "
         "double value);\n"
         "extern void akx_rt_set_string(akx_runtime_ctx_t *rt, akx_cell_t "
         "*cell, const char *str);\n"
         "extern void akx_rt_set_list(akx_runtime_ctx_t *rt, akx_cell_t *cell, "
         "akx_cell_t *head);\n"
         "extern void akx_rt_set_lambda(akx_runtime_ctx_t *rt, akx_cell_t "
         "*cell, ak_lambda_t *lambda);\n"
         "\n"
         "extern int akx_rt_cell_is_type(akx_cell_t *cell, akx_type_t type);\n"
         "extern const char* akx_rt_cell_as_symbol(akx_cell_t *cell);\n"
         "extern int akx_rt_cell_as_int(akx_cell_t *cell);\n"
         "extern double akx_rt_cell_as_real(akx_cell_t *cell);\n"
         "extern const char* akx_rt_cell_as_string(akx_cell_t *cell);\n"
         "extern akx_cell_t* akx_rt_cell_as_list(akx_cell_t *cell);\n"
         "extern ak_lambda_t* akx_rt_cell_as_lambda(akx_cell_t *cell);\n"
         "extern akx_cell_t* akx_rt_cell_next(akx_cell_t *cell);\n"
         "\n"
         "extern size_t akx_rt_list_length(akx_cell_t *list);\n"
         "extern akx_cell_t* akx_rt_list_nth(akx_cell_t *list, size_t n);\n"
         "extern akx_cell_t* akx_rt_list_append(akx_runtime_ctx_t *rt, "
         "akx_cell_t *list, akx_cell_t *item);\n"
         "\n"
         "extern ak_context_t* akx_rt_get_scope(akx_runtime_ctx_t *rt);\n"
         "extern int akx_rt_scope_set(akx_runtime_ctx_t *rt, const char *key, "
         "void *value);\n"
         "extern void* akx_rt_scope_get(akx_runtime_ctx_t *rt, const char "
         "*key);\n"
         "extern void akx_rt_push_scope(akx_runtime_ctx_t *rt);\n"
         "extern void akx_rt_pop_scope(akx_runtime_ctx_t *rt);\n"
         "\n"
         "extern void akx_rt_error(akx_runtime_ctx_t *rt, const char "
         "*message);\n"
         "extern void akx_rt_error_fmt(akx_runtime_ctx_t *rt, const char *fmt, "
         "...);\n"
         "\n"
         "extern akx_cell_t* akx_rt_eval(akx_runtime_ctx_t *rt, akx_cell_t "
         "*expr);\n"
         "extern akx_cell_t* akx_rt_eval_list(akx_runtime_ctx_t *rt, "
         "akx_cell_t *list);\n"
         "extern akx_cell_t* akx_rt_eval_and_assert(akx_runtime_ctx_t *rt, "
         "akx_cell_t *expr, akx_type_t expected_type, const char *error_msg);\n"
         "\n"
         "extern akx_cell_t* akx_rt_invoke_lambda(akx_runtime_ctx_t *rt, "
         "akx_cell_t *lambda_cell, akx_cell_t *args);\n"
         "\n"
         "extern void akx_rt_module_set_data(akx_runtime_ctx_t *rt, void "
         "*data);\n"
         "extern void* akx_rt_module_get_data(akx_runtime_ctx_t *rt);\n"
         "\n";
}

int akx_compiler_load_builtin_ex(akx_runtime_ctx_t *rt, const char *name,
                                 const char *c_function_name,
                                 const char *root_path,
                                 const akx_builtin_compile_opts_t *opts) {
  if (!rt || !name || !root_path) {
    AK24_LOG_ERROR("Invalid arguments to akx_compiler_load_builtin_ex");
    return -1;
  }

  AK24_LOG_DEBUG("Creating CJIT unit");
  ak_cjit_unit_t *unit = ak_cjit_unit_new(NULL, NULL, NULL);
  if (!unit) {
    AK24_LOG_ERROR("Failed to create CJIT unit");
    return -1;
  }
  AK24_LOG_DEBUG("CJIT unit created");

  if (opts) {
    for (size_t i = 0; i < opts->include_path_count; i++) {
      AK24_LOG_DEBUG("Adding include path: %s", opts->include_paths[i]);
      if (ak_cjit_add_include_path(unit, opts->include_paths[i]) !=
          AK_CJIT_OK) {
        AK24_LOG_ERROR("Failed to add include path: %s",
                       opts->include_paths[i]);
        ak_cjit_unit_free(unit);
        return -1;
      }
    }

    for (size_t i = 0; i < opts->library_path_count; i++) {
      AK24_LOG_DEBUG("Adding library path: %s", opts->library_paths[i]);
      if (ak_cjit_add_library_path(unit, opts->library_paths[i]) !=
          AK_CJIT_OK) {
        AK24_LOG_ERROR("Failed to add library path: %s",
                       opts->library_paths[i]);
        ak_cjit_unit_free(unit);
        return -1;
      }
    }

    for (size_t i = 0; i < opts->library_count; i++) {
      AK24_LOG_DEBUG("Adding library: %s", opts->libraries[i]);
      if (ak_cjit_add_library(unit, opts->libraries[i]) != AK_CJIT_OK) {
        AK24_LOG_ERROR("Failed to add library: %s", opts->libraries[i]);
        ak_cjit_unit_free(unit);
        return -1;
      }
    }

    for (size_t i = 0; i < opts->define_count; i++) {
      AK24_LOG_DEBUG("Adding define: %s=%s", opts->define_names[i],
                     opts->define_values[i]);
      if (ak_cjit_define(unit, opts->define_names[i], opts->define_values[i]) !=
          AK_CJIT_OK) {
        AK24_LOG_ERROR("Failed to add define: %s", opts->define_names[i]);
        ak_cjit_unit_free(unit);
        return -1;
      }
    }
  }

  AK24_LOG_DEBUG("Reading root source file: %s", root_path);
  ak_buffer_t *source_buf = ak_buffer_from_file(root_path);
  if (!source_buf) {
    AK24_LOG_ERROR("Failed to read root source file: %s", root_path);
    ak_cjit_unit_free(unit);
    return -1;
  }
  AK24_LOG_DEBUG("Root source file read successfully (%zu bytes)",
                 ak_buffer_count(source_buf));

  AK24_LOG_DEBUG("Generating ABI header");
  const char *abi_header = akx_compiler_generate_abi_header();
  size_t abi_len = strlen(abi_header);
  size_t source_len = ak_buffer_count(source_buf);
  size_t total_len = abi_len + source_len + 1;

  AK24_LOG_DEBUG("Allocating %zu bytes for combined source", total_len);
  char *combined_source = AK24_ALLOC(total_len);
  if (!combined_source) {
    AK24_LOG_ERROR("Failed to allocate memory for combined source");
    ak_buffer_free(source_buf);
    ak_cjit_unit_free(unit);
    return -1;
  }

  AK24_LOG_DEBUG("Combining ABI header and root source");
  memcpy(combined_source, abi_header, abi_len);
  memcpy(combined_source + abi_len, ak_buffer_data(source_buf), source_len);
  combined_source[abi_len + source_len] = '\0';

  ak_buffer_free(source_buf);

  AK24_LOG_DEBUG("Adding symbols to CJIT unit");

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wpedantic"

  ak_cjit_add_symbol(unit, "printf", printf);
  ak_cjit_add_symbol(unit, "malloc", malloc);
  ak_cjit_add_symbol(unit, "free", free);
  ak_cjit_add_symbol(unit, "akx_rt_alloc_cell", akx_rt_alloc_cell);
  ak_cjit_add_symbol(unit, "akx_rt_free_cell", akx_rt_free_cell);
  ak_cjit_add_symbol(unit, "akx_rt_set_symbol", akx_rt_set_symbol);
  ak_cjit_add_symbol(unit, "akx_rt_set_int", akx_rt_set_int);
  ak_cjit_add_symbol(unit, "akx_rt_set_real", akx_rt_set_real);
  ak_cjit_add_symbol(unit, "akx_rt_set_string", akx_rt_set_string);
  ak_cjit_add_symbol(unit, "akx_rt_set_list", akx_rt_set_list);
  ak_cjit_add_symbol(unit, "akx_rt_set_lambda", akx_rt_set_lambda);
  ak_cjit_add_symbol(unit, "akx_rt_cell_is_type", akx_rt_cell_is_type);
  ak_cjit_add_symbol(unit, "akx_rt_cell_get_type", akx_rt_cell_get_type);
  ak_cjit_add_symbol(unit, "akx_rt_cell_as_symbol", akx_rt_cell_as_symbol);
  ak_cjit_add_symbol(unit, "akx_rt_cell_as_int", akx_rt_cell_as_int);
  ak_cjit_add_symbol(unit, "akx_rt_cell_as_real", akx_rt_cell_as_real);
  ak_cjit_add_symbol(unit, "akx_rt_cell_as_string", akx_rt_cell_as_string);
  ak_cjit_add_symbol(unit, "akx_rt_cell_as_list", akx_rt_cell_as_list);
  ak_cjit_add_symbol(unit, "akx_rt_cell_as_lambda", akx_rt_cell_as_lambda);
  ak_cjit_add_symbol(unit, "akx_rt_cell_next", akx_rt_cell_next);
  ak_cjit_add_symbol(unit, "akx_rt_list_length", akx_rt_list_length);
  ak_cjit_add_symbol(unit, "akx_rt_list_nth", akx_rt_list_nth);
  ak_cjit_add_symbol(unit, "akx_rt_list_append", akx_rt_list_append);
  ak_cjit_add_symbol(unit, "akx_rt_get_scope", akx_rt_get_scope);
  ak_cjit_add_symbol(unit, "akx_rt_scope_set", akx_rt_scope_set);
  ak_cjit_add_symbol(unit, "akx_rt_scope_get", akx_rt_scope_get);
  ak_cjit_add_symbol(unit, "akx_rt_push_scope", akx_rt_push_scope);
  ak_cjit_add_symbol(unit, "akx_rt_pop_scope", akx_rt_pop_scope);
  ak_cjit_add_symbol(unit, "akx_rt_error", akx_rt_error);
  ak_cjit_add_symbol(unit, "akx_rt_error_fmt", akx_rt_error_fmt);
  ak_cjit_add_symbol(unit, "akx_rt_eval", akx_rt_eval);
  ak_cjit_add_symbol(unit, "akx_rt_eval_list", akx_rt_eval_list);
  ak_cjit_add_symbol(unit, "akx_rt_eval_and_assert", akx_rt_eval_and_assert);
  ak_cjit_add_symbol(unit, "akx_rt_invoke_lambda", akx_rt_invoke_lambda);
  ak_cjit_add_symbol(unit, "akx_rt_module_set_data", akx_rt_module_set_data);
  ak_cjit_add_symbol(unit, "akx_rt_module_get_data", akx_rt_module_get_data);

#pragma clang diagnostic pop

  AK24_LOG_DEBUG("All symbols added");

  AK24_LOG_DEBUG("Adding root source to CJIT unit");
  if (ak_cjit_add_source(unit, combined_source, root_path) != AK_CJIT_OK) {
    AK24_LOG_ERROR("Failed to add root source to CJIT unit");
    ak_cjit_unit_free(unit);
    AK24_FREE(combined_source);
    return -1;
  }
  AK24_LOG_DEBUG("Root source added successfully");

  AK24_FREE(combined_source);

  if (opts && opts->impl_file_count > 0) {
    char root_dir[1024];
    const char *last_slash = strrchr(root_path, '/');
    if (last_slash) {
      size_t dir_len = last_slash - root_path;
      if (dir_len >= sizeof(root_dir)) {
        AK24_LOG_ERROR("Root path too long");
        ak_cjit_unit_free(unit);
        return -1;
      }
      memcpy(root_dir, root_path, dir_len);
      root_dir[dir_len] = '\0';
    } else {
      strcpy(root_dir, ".");
    }

    for (size_t i = 0; i < opts->impl_file_count; i++) {
      char impl_path[2048];
      snprintf(impl_path, sizeof(impl_path), "%s/%s", root_dir,
               opts->impl_files[i]);

      AK24_LOG_DEBUG("Reading implementation file: %s", impl_path);
      ak_buffer_t *impl_buf = ak_buffer_from_file(impl_path);
      if (!impl_buf) {
        AK24_LOG_ERROR("Failed to read implementation file: %s", impl_path);
        ak_cjit_unit_free(unit);
        return -1;
      }

      const char *impl_source = (const char *)ak_buffer_data(impl_buf);
      AK24_LOG_DEBUG("Adding implementation file to CJIT unit: %s", impl_path);
      if (ak_cjit_add_source(unit, impl_source, impl_path) != AK_CJIT_OK) {
        AK24_LOG_ERROR("Failed to add implementation file: %s", impl_path);
        ak_buffer_free(impl_buf);
        ak_cjit_unit_free(unit);
        return -1;
      }

      ak_buffer_free(impl_buf);
      AK24_LOG_DEBUG("Implementation file added: %s", impl_path);
    }
  }

  AK24_LOG_DEBUG("Compiling builtin");
  if (ak_cjit_relocate(unit) != AK_CJIT_OK) {
    AK24_LOG_ERROR("Failed to compile builtin: %s", root_path);
    ak_cjit_unit_free(unit);
    return -1;
  }
  AK24_LOG_DEBUG("Compilation successful");

  const char *lookup_name = c_function_name ? c_function_name : name;
  AK24_LOG_DEBUG("Looking up symbol '%s'", lookup_name);
  akx_builtin_fn function =
      (akx_builtin_fn)ak_cjit_get_symbol(unit, lookup_name);
  if (!function) {
    AK24_LOG_ERROR("Failed to find symbol '%s' in compiled builtin",
                   lookup_name);
    ak_cjit_unit_free(unit);
    return -1;
  }
  AK24_LOG_DEBUG("Symbol found");

  char init_name[256];
  char deinit_name[256];
  char reload_name[256];
  snprintf(init_name, sizeof(init_name), "%s_init", lookup_name);
  snprintf(deinit_name, sizeof(deinit_name), "%s_deinit", lookup_name);
  snprintf(reload_name, sizeof(reload_name), "%s_reload", lookup_name);

  void (*init_fn)(akx_runtime_ctx_t *) =
      (void (*)(akx_runtime_ctx_t *))ak_cjit_get_symbol(unit, init_name);
  void (*deinit_fn)(akx_runtime_ctx_t *) =
      (void (*)(akx_runtime_ctx_t *))ak_cjit_get_symbol(unit, deinit_name);
  void (*reload_fn)(akx_runtime_ctx_t *, void *) =
      (void (*)(akx_runtime_ctx_t *, void *))ak_cjit_get_symbol(unit,
                                                                reload_name);

  return akx_rt_register_builtin(rt, name, root_path, function, unit, init_fn,
                                 deinit_fn, reload_fn);
}
