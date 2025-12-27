#include "akx_rt_builtins.h"
#include "akx_rt_compiler.h"
#include <ak24/context.h>
#include <ak24/intern.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static akx_cell_t *builtin_cjit_load_builtin(akx_runtime_ctx_t *rt,
                                             akx_cell_t *args) {
  akx_cell_t *name_cell = akx_rt_list_nth(args, 0);
  if (!name_cell || name_cell->type != AKX_TYPE_SYMBOL) {
    akx_rt_error(rt, "first argument must be a symbol (builtin name)");
    return NULL;
  }

  const char *name = akx_rt_cell_as_symbol(name_cell);

  akx_builtin_compile_opts_t opts;
  memset(&opts, 0, sizeof(opts));

  const char *root_path = NULL;
  const char *c_function_name = NULL;

  akx_cell_t *current = akx_rt_list_nth(args, 1);
  while (current) {
    if (current->type != AKX_TYPE_SYMBOL) {
      akx_rt_error(rt, "expected keyword (symbol starting with ':')");
      goto cleanup_error;
    }

    const char *keyword = akx_rt_cell_as_symbol(current);
    if (keyword[0] != ':') {
      akx_rt_error_fmt(rt, "keywords must start with ':', got: %s", keyword);
      goto cleanup_error;
    }

    current = akx_rt_cell_next(current);
    if (!current) {
      akx_rt_error_fmt(rt, "missing value for keyword %s", keyword);
      goto cleanup_error;
    }

    if (strcmp(keyword, ":root") == 0) {
      akx_cell_t *root_evaled = akx_rt_eval_and_assert(
          rt, current, AKX_TYPE_STRING_LITERAL, ":root value must be a string");
      if (!root_evaled) {
        goto cleanup_error;
      }
      const char *root_str = akx_rt_cell_as_string(root_evaled);
      size_t root_len = strlen(root_str);
      char *root_dup = AK24_ALLOC(root_len + 1);
      if (!root_dup) {
        akx_rt_free_cell(rt, root_evaled);
        akx_rt_error(rt, "failed to allocate root path");
        goto cleanup_error;
      }
      strcpy(root_dup, root_str);
      root_path = root_dup;
      akx_rt_free_cell(rt, root_evaled);
    } else if (strcmp(keyword, ":as") == 0) {
      akx_cell_t *as_evaled = akx_rt_eval_and_assert(
          rt, current, AKX_TYPE_STRING_LITERAL, ":as value must be a string");
      if (!as_evaled) {
        goto cleanup_error;
      }
      const char *as_str = akx_rt_cell_as_string(as_evaled);
      size_t as_len = strlen(as_str);
      char *as_dup = AK24_ALLOC(as_len + 1);
      if (!as_dup) {
        akx_rt_free_cell(rt, as_evaled);
        akx_rt_error(rt, "failed to allocate C function name");
        goto cleanup_error;
      }
      strcpy(as_dup, as_str);
      c_function_name = as_dup;
      akx_rt_free_cell(rt, as_evaled);
    } else if (strcmp(keyword, ":include-paths") == 0) {
      if (akx_compiler_extract_string_list(rt, current, &opts.include_paths,
                                           &opts.include_path_count) != 0) {
        goto cleanup_error;
      }
    } else if (strcmp(keyword, ":implementation-files") == 0) {
      if (akx_compiler_extract_string_list(rt, current, &opts.impl_files,
                                           &opts.impl_file_count) != 0) {
        goto cleanup_error;
      }
    } else if (strcmp(keyword, ":linker") == 0) {
      if (akx_compiler_parse_linker_options(rt, current, &opts) != 0) {
        goto cleanup_error;
      }
    } else {
      akx_rt_error_fmt(rt, "unknown keyword: %s", keyword);
      goto cleanup_error;
    }

    current = akx_rt_cell_next(current);
  }

  if (!root_path) {
    akx_rt_error(rt, ":root keyword is required");
    goto cleanup_error;
  }

  int result =
      akx_compiler_load_builtin_ex(rt, name, c_function_name, root_path, &opts);

  if (root_path)
    AK24_FREE((void *)root_path);
  if (c_function_name)
    AK24_FREE((void *)c_function_name);
  akx_compiler_free_compile_opts(&opts);

  akx_cell_t *ret = akx_rt_alloc_cell(rt, AKX_TYPE_SYMBOL);
  if (result == 0) {
    akx_rt_set_symbol(rt, ret, "t");
  } else {
    akx_rt_set_symbol(rt, ret, "nil");
  }
  return ret;

cleanup_error:
  if (root_path)
    AK24_FREE((void *)root_path);
  if (c_function_name)
    AK24_FREE((void *)c_function_name);
  akx_compiler_free_compile_opts(&opts);
  return NULL;
}

void akx_rt_register_bootstrap_builtins(akx_runtime_ctx_t *rt) {
  if (!rt) {
    return;
  }

  akx_builtin_info_t *bootstrap_info = AK24_ALLOC(sizeof(akx_builtin_info_t));
  if (bootstrap_info) {
    bootstrap_info->function = builtin_cjit_load_builtin;
    bootstrap_info->source_path = NULL;
    bootstrap_info->load_time = time(NULL);
    bootstrap_info->init_fn = NULL;
    bootstrap_info->deinit_fn = NULL;
    bootstrap_info->reload_fn = NULL;
    bootstrap_info->module_data = NULL;
    bootstrap_info->module_name = ak_intern("cjit-load-builtin");
    bootstrap_info->unit = NULL;
    const char *name = "cjit-load-builtin";
    akx_rt_add_builtin(rt, name, bootstrap_info);
    AK24_LOG_TRACE("Registered bootstrap builtin: cjit-load-builtin");
  }
}
