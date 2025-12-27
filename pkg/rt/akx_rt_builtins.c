#include "akx_rt_builtins.h"
#include "akx_rt_compiler.h"
#include <ak24/context.h>
#include <ak24/intern.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static void akx_lambda_context_free(void *ctx) {
  if (!ctx) {
    return;
  }
  akx_lambda_context_t *lambda_ctx = (akx_lambda_context_t *)ctx;
  if (lambda_ctx->param_names) {
    AK24_FREE(lambda_ctx->param_names);
  }
  if (lambda_ctx->body) {
    akx_cell_free(lambda_ctx->body);
  }
  AK24_FREE(lambda_ctx);
}

static void akx_lambda_invoke_impl(void *captured_ctx, void *invoke_args) {
  if (!captured_ctx) {
    return;
  }

  akx_lambda_context_t *lambda_ctx = (akx_lambda_context_t *)captured_ctx;
  akx_runtime_ctx_t *rt = lambda_ctx->rt;
  akx_cell_t *args = (akx_cell_t *)invoke_args;

  lambda_ctx->result = NULL;

  size_t arg_count = akx_rt_list_length(args);
  if (arg_count != lambda_ctx->param_count) {
    akx_rt_error_fmt(rt, "lambda: expected %zu arguments, got %zu",
                     lambda_ctx->param_count, arg_count);
    return;
  }

  for (size_t i = 0; i < lambda_ctx->param_count; i++) {
    akx_cell_t *arg = akx_rt_list_nth(args, i);
    akx_cell_t *evaled = akx_rt_eval(rt, arg);
    if (!evaled) {
      return;
    }
    akx_rt_scope_set(rt, lambda_ctx->param_names[i], evaled);
  }

  akx_cell_t *result = NULL;
  akx_cell_t *current = lambda_ctx->body;
  while (current) {
    if (result) {
      akx_cell_free(result);
    }
    result = akx_rt_eval(rt, current);
    if (!result) {
      return;
    }
    current = current->next;
  }

  lambda_ctx->result = result;
}

static akx_cell_t *builtin_lambda(akx_runtime_ctx_t *rt, akx_cell_t *args) {
  akx_cell_t *params_cell = akx_rt_list_nth(args, 0);
  if (!params_cell || params_cell->type != AKX_TYPE_LIST_SQUARE) {
    akx_rt_error(rt, "lambda: first argument must be a square-bracket list of "
                     "parameter names");
    return NULL;
  }

  akx_cell_t *params = params_cell->value.list_head;
  size_t param_count = akx_rt_list_length(params);

  const char **param_names = NULL;
  if (param_count > 0) {
    param_names = AK24_ALLOC(sizeof(char *) * param_count);
    if (!param_names) {
      akx_rt_error(rt, "lambda: failed to allocate parameter names");
      return NULL;
    }

    akx_cell_t *current = params;
    for (size_t i = 0; i < param_count; i++) {
      if (!current || current->type != AKX_TYPE_SYMBOL) {
        AK24_FREE(param_names);
        akx_rt_error(rt, "lambda: all parameters must be symbols");
        return NULL;
      }
      param_names[i] = akx_rt_cell_as_symbol(current);
      current = current->next;
    }
  }

  akx_cell_t *body = akx_rt_list_nth(args, 1);
  if (!body) {
    if (param_names) {
      AK24_FREE(param_names);
    }
    akx_rt_error(rt, "lambda: missing body");
    return NULL;
  }

  akx_cell_t *body_clone = NULL;
  akx_cell_t *body_tail = NULL;
  akx_cell_t *current = body;
  while (current) {
    akx_cell_t *cloned = akx_cell_clone(current);
    if (!cloned) {
      if (param_names) {
        AK24_FREE(param_names);
      }
      if (body_clone) {
        akx_cell_free(body_clone);
      }
      akx_rt_error(rt, "lambda: failed to clone body");
      return NULL;
    }
    cloned->next = NULL;
    if (!body_clone) {
      body_clone = cloned;
      body_tail = cloned;
    } else {
      body_tail->next = cloned;
      body_tail = cloned;
    }
    current = current->next;
  }

  akx_lambda_context_t *lambda_ctx = AK24_ALLOC(sizeof(akx_lambda_context_t));
  if (!lambda_ctx) {
    if (param_names) {
      AK24_FREE(param_names);
    }
    if (body_clone) {
      akx_cell_free(body_clone);
    }
    akx_rt_error(rt, "lambda: failed to allocate lambda context");
    return NULL;
  }

  lambda_ctx->rt = rt;
  lambda_ctx->param_names = param_names;
  lambda_ctx->param_count = param_count;
  lambda_ctx->body = body_clone;
  lambda_ctx->result = NULL;

  ak_lambda_t *lambda = ak_lambda_new(akx_lambda_invoke_impl, lambda_ctx,
                                      akx_lambda_context_free);
  if (!lambda) {
    akx_lambda_context_free(lambda_ctx);
    akx_rt_error(rt, "lambda: failed to create lambda");
    return NULL;
  }

  akx_cell_t *lambda_cell = akx_rt_alloc_cell(rt, AKX_TYPE_LAMBDA);
  if (!lambda_cell) {
    ak_lambda_free(lambda);
    akx_rt_error(rt, "lambda: failed to allocate lambda cell");
    return NULL;
  }

  akx_rt_set_lambda(rt, lambda_cell, lambda);

  return lambda_cell;
}

static akx_cell_t *builtin_let(akx_runtime_ctx_t *rt, akx_cell_t *args) {
  akx_cell_t *symbol_cell = akx_rt_list_nth(args, 0);
  if (!symbol_cell || symbol_cell->type != AKX_TYPE_SYMBOL) {
    akx_rt_error(rt, "let: first argument must be a symbol");
    return NULL;
  }

  const char *symbol = akx_rt_cell_as_symbol(symbol_cell);

  ak_context_t *scope = akx_rt_get_scope(rt);
  if (ak_context_has_local(scope, symbol)) {
    akx_rt_error_fmt(rt, "let: symbol '%s' already defined in current scope",
                     symbol);
    return NULL;
  }

  akx_cell_t *value_cell = akx_rt_list_nth(args, 1);
  if (!value_cell) {
    akx_rt_error(rt, "let: missing value argument");
    return NULL;
  }

  akx_cell_t *evaled = akx_rt_eval(rt, value_cell);
  if (!evaled) {
    return NULL;
  }

  akx_rt_scope_set(rt, symbol, evaled);

  akx_cell_t *returned = akx_cell_clone(evaled);
  if (!returned) {
    akx_rt_error(rt, "let: failed to clone value for return");
    return NULL;
  }

  return returned;
}

static akx_cell_t *builtin_set(akx_runtime_ctx_t *rt, akx_cell_t *args) {
  akx_cell_t *symbol_cell = akx_rt_list_nth(args, 0);
  if (!symbol_cell || symbol_cell->type != AKX_TYPE_SYMBOL) {
    akx_rt_error(rt, "set: first argument must be a symbol");
    return NULL;
  }

  const char *symbol = akx_rt_cell_as_symbol(symbol_cell);

  ak_context_t *scope = akx_rt_get_scope(rt);
  if (!ak_context_has(scope, symbol)) {
    akx_rt_error_fmt(rt, "set: symbol '%s' is not defined", symbol);
    return NULL;
  }

  akx_cell_t *value_cell = akx_rt_list_nth(args, 1);
  if (!value_cell) {
    akx_rt_error(rt, "set: missing value argument");
    return NULL;
  }

  akx_cell_t *evaled = akx_rt_eval(rt, value_cell);
  if (!evaled) {
    return NULL;
  }

  ak_context_t *containing = ak_context_get_containing_context(scope, symbol);
  if (containing) {
    void *old_value = ak_context_get_local(containing, symbol);
    if (old_value) {
      akx_cell_free((akx_cell_t *)old_value);
    }
    ak_context_set(containing, symbol, evaled);
  }

  return evaled;
}

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

  int result = akx_compiler_load_builtin_ex(rt, name, root_path, &opts);

  if (root_path)
    AK24_FREE((void *)root_path);
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
  akx_compiler_free_compile_opts(&opts);
  return NULL;
}

static int is_truthy(akx_cell_t *cell) {
  if (!cell) {
    return 0;
  }

  if (cell->type == AKX_TYPE_SYMBOL) {
    const char *sym = cell->value.symbol;
    if (strcmp(sym, "nil") == 0 || strcmp(sym, "false") == 0) {
      return 0;
    }
    return 1;
  }

  if (cell->type == AKX_TYPE_INTEGER_LITERAL) {
    return cell->value.integer_literal != 0;
  }

  if (cell->type == AKX_TYPE_REAL_LITERAL) {
    return cell->value.real_literal != 0.0;
  }

  return 1;
}

static akx_cell_t *builtin_assert_true(akx_runtime_ctx_t *rt,
                                       akx_cell_t *args) {
  akx_cell_t *condition_cell = akx_rt_list_nth(args, 0);
  if (!condition_cell) {
    akx_rt_error(rt, "assert-true: missing condition argument");
    return NULL;
  }

  akx_cell_t *evaled = akx_rt_eval(rt, condition_cell);
  if (!evaled) {
    return NULL;
  }

  int is_true = is_truthy(evaled);

  if (evaled->type != AKX_TYPE_LAMBDA) {
    akx_cell_free(evaled);
  }

  if (!is_true) {
    akx_cell_t *message_cell = akx_rt_list_nth(args, 1);
    if (message_cell) {
      akx_cell_t *msg_evaled = akx_rt_eval(rt, message_cell);
      if (msg_evaled && msg_evaled->type == AKX_TYPE_STRING_LITERAL) {
        const char *msg = akx_rt_cell_as_string(msg_evaled);
        AK24_LOG_ERROR("Assertion failed: %s", msg);
        akx_cell_free(msg_evaled);
      } else {
        AK24_LOG_ERROR("Assertion failed: condition is false");
        if (msg_evaled && msg_evaled->type != AKX_TYPE_LAMBDA) {
          akx_cell_free(msg_evaled);
        }
      }
    } else {
      AK24_LOG_ERROR("Assertion failed: condition is false");
    }
    raise(SIGABRT);
  }

  akx_cell_t *ret = akx_rt_alloc_cell(rt, AKX_TYPE_SYMBOL);
  akx_rt_set_symbol(rt, ret, "t");
  return ret;
}

static akx_cell_t *builtin_assert_false(akx_runtime_ctx_t *rt,
                                        akx_cell_t *args) {
  akx_cell_t *condition_cell = akx_rt_list_nth(args, 0);
  if (!condition_cell) {
    akx_rt_error(rt, "assert-false: missing condition argument");
    return NULL;
  }

  akx_cell_t *evaled = akx_rt_eval(rt, condition_cell);
  if (!evaled) {
    return NULL;
  }

  int is_true = is_truthy(evaled);

  if (evaled->type != AKX_TYPE_LAMBDA) {
    akx_cell_free(evaled);
  }

  if (is_true) {
    akx_cell_t *message_cell = akx_rt_list_nth(args, 1);
    if (message_cell) {
      akx_cell_t *msg_evaled = akx_rt_eval(rt, message_cell);
      if (msg_evaled && msg_evaled->type == AKX_TYPE_STRING_LITERAL) {
        const char *msg = akx_rt_cell_as_string(msg_evaled);
        AK24_LOG_ERROR("Assertion failed: %s", msg);
        akx_cell_free(msg_evaled);
      } else {
        AK24_LOG_ERROR("Assertion failed: condition is true");
        if (msg_evaled && msg_evaled->type != AKX_TYPE_LAMBDA) {
          akx_cell_free(msg_evaled);
        }
      }
    } else {
      AK24_LOG_ERROR("Assertion failed: condition is true");
    }
    raise(SIGABRT);
  }

  akx_cell_t *ret = akx_rt_alloc_cell(rt, AKX_TYPE_SYMBOL);
  akx_rt_set_symbol(rt, ret, "t");
  return ret;
}

static akx_cell_t *builtin_assert_eq(akx_runtime_ctx_t *rt, akx_cell_t *args) {
  akx_cell_t *left_cell = akx_rt_list_nth(args, 0);
  akx_cell_t *right_cell = akx_rt_list_nth(args, 1);

  if (!left_cell || !right_cell) {
    akx_rt_error(rt, "assert-eq: requires two arguments");
    return NULL;
  }

  akx_cell_t *left = akx_rt_eval(rt, left_cell);
  if (!left) {
    return NULL;
  }

  akx_cell_t *right = akx_rt_eval(rt, right_cell);
  if (!right) {
    akx_cell_free(left);
    return NULL;
  }

  int equal = 0;

  if (left->type != right->type) {
    equal = 0;
  } else {
    switch (left->type) {
    case AKX_TYPE_INTEGER_LITERAL:
      equal = (left->value.integer_literal == right->value.integer_literal);
      break;
    case AKX_TYPE_REAL_LITERAL:
      equal = (left->value.real_literal == right->value.real_literal);
      break;
    case AKX_TYPE_STRING_LITERAL: {
      const char *left_str = akx_rt_cell_as_string(left);
      const char *right_str = akx_rt_cell_as_string(right);
      equal = (strcmp(left_str, right_str) == 0);
      break;
    }
    case AKX_TYPE_SYMBOL: {
      const char *left_sym = akx_rt_cell_as_symbol(left);
      const char *right_sym = akx_rt_cell_as_symbol(right);
      equal = (strcmp(left_sym, right_sym) == 0);
      break;
    }
    default:
      equal = 0;
      break;
    }
  }

  if (!equal) {
    akx_cell_t *message_cell = akx_rt_list_nth(args, 2);
    if (message_cell) {
      akx_cell_t *msg_evaled = akx_rt_eval(rt, message_cell);
      if (msg_evaled && msg_evaled->type == AKX_TYPE_STRING_LITERAL) {
        const char *msg = akx_rt_cell_as_string(msg_evaled);
        AK24_LOG_ERROR("Assertion failed: %s", msg);
        akx_cell_free(msg_evaled);
      } else {
        AK24_LOG_ERROR("Assertion failed: values not equal");
        if (msg_evaled && msg_evaled->type != AKX_TYPE_LAMBDA) {
          akx_cell_free(msg_evaled);
        }
      }
    } else {
      AK24_LOG_ERROR("Assertion failed: values not equal");
    }
    if (left->type != AKX_TYPE_LAMBDA) {
      akx_cell_free(left);
    }
    if (right->type != AKX_TYPE_LAMBDA) {
      akx_cell_free(right);
    }
    raise(SIGABRT);
  }

  if (left->type != AKX_TYPE_LAMBDA) {
    akx_cell_free(left);
  }
  if (right->type != AKX_TYPE_LAMBDA) {
    akx_cell_free(right);
  }

  akx_cell_t *ret = akx_rt_alloc_cell(rt, AKX_TYPE_SYMBOL);
  akx_rt_set_symbol(rt, ret, "t");
  return ret;
}

static akx_cell_t *builtin_assert_ne(akx_runtime_ctx_t *rt, akx_cell_t *args) {
  akx_cell_t *left_cell = akx_rt_list_nth(args, 0);
  akx_cell_t *right_cell = akx_rt_list_nth(args, 1);

  if (!left_cell || !right_cell) {
    akx_rt_error(rt, "assert-ne: requires two arguments");
    return NULL;
  }

  akx_cell_t *left = akx_rt_eval(rt, left_cell);
  if (!left) {
    return NULL;
  }

  akx_cell_t *right = akx_rt_eval(rt, right_cell);
  if (!right) {
    akx_cell_free(left);
    return NULL;
  }

  int equal = 0;

  if (left->type != right->type) {
    equal = 0;
  } else {
    switch (left->type) {
    case AKX_TYPE_INTEGER_LITERAL:
      equal = (left->value.integer_literal == right->value.integer_literal);
      break;
    case AKX_TYPE_REAL_LITERAL:
      equal = (left->value.real_literal == right->value.real_literal);
      break;
    case AKX_TYPE_STRING_LITERAL: {
      const char *left_str = akx_rt_cell_as_string(left);
      const char *right_str = akx_rt_cell_as_string(right);
      equal = (strcmp(left_str, right_str) == 0);
      break;
    }
    case AKX_TYPE_SYMBOL: {
      const char *left_sym = akx_rt_cell_as_symbol(left);
      const char *right_sym = akx_rt_cell_as_symbol(right);
      equal = (strcmp(left_sym, right_sym) == 0);
      break;
    }
    default:
      equal = 0;
      break;
    }
  }

  if (equal) {
    akx_cell_t *message_cell = akx_rt_list_nth(args, 2);
    if (message_cell) {
      akx_cell_t *msg_evaled = akx_rt_eval(rt, message_cell);
      if (msg_evaled && msg_evaled->type == AKX_TYPE_STRING_LITERAL) {
        const char *msg = akx_rt_cell_as_string(msg_evaled);
        AK24_LOG_ERROR("Assertion failed: %s", msg);
        akx_cell_free(msg_evaled);
      } else {
        AK24_LOG_ERROR("Assertion failed: values are equal");
        if (msg_evaled && msg_evaled->type != AKX_TYPE_LAMBDA) {
          akx_cell_free(msg_evaled);
        }
      }
    } else {
      AK24_LOG_ERROR("Assertion failed: values are equal");
    }
    if (left->type != AKX_TYPE_LAMBDA) {
      akx_cell_free(left);
    }
    if (right->type != AKX_TYPE_LAMBDA) {
      akx_cell_free(right);
    }
    raise(SIGABRT);
  }

  if (left->type != AKX_TYPE_LAMBDA) {
    akx_cell_free(left);
  }
  if (right->type != AKX_TYPE_LAMBDA) {
    akx_cell_free(right);
  }

  akx_cell_t *ret = akx_rt_alloc_cell(rt, AKX_TYPE_SYMBOL);
  akx_rt_set_symbol(rt, ret, "t");
  return ret;
}

void akx_rt_register_bootstrap_builtins(akx_runtime_ctx_t *rt) {
  if (!rt) {
    return;
  }

  akx_builtin_info_t *let_info = AK24_ALLOC(sizeof(akx_builtin_info_t));
  if (let_info) {
    let_info->function = builtin_let;
    let_info->source_path = NULL;
    let_info->load_time = time(NULL);
    let_info->init_fn = NULL;
    let_info->deinit_fn = NULL;
    let_info->reload_fn = NULL;
    let_info->module_data = NULL;
    let_info->module_name = ak_intern("let");
    let_info->unit = NULL;
    akx_rt_add_builtin(rt, "let", let_info);
    AK24_LOG_TRACE("Registered native builtin: let");
  }

  akx_builtin_info_t *set_info = AK24_ALLOC(sizeof(akx_builtin_info_t));
  if (set_info) {
    set_info->function = builtin_set;
    set_info->source_path = NULL;
    set_info->load_time = time(NULL);
    set_info->init_fn = NULL;
    set_info->deinit_fn = NULL;
    set_info->reload_fn = NULL;
    set_info->module_data = NULL;
    set_info->module_name = ak_intern("set");
    set_info->unit = NULL;
    akx_rt_add_builtin(rt, "set", set_info);
    AK24_LOG_TRACE("Registered native builtin: set");
  }

  akx_builtin_info_t *lambda_info = AK24_ALLOC(sizeof(akx_builtin_info_t));
  if (lambda_info) {
    lambda_info->function = builtin_lambda;
    lambda_info->source_path = NULL;
    lambda_info->load_time = time(NULL);
    lambda_info->init_fn = NULL;
    lambda_info->deinit_fn = NULL;
    lambda_info->reload_fn = NULL;
    lambda_info->module_data = NULL;
    lambda_info->module_name = ak_intern("lambda");
    lambda_info->unit = NULL;
    akx_rt_add_builtin(rt, "lambda", lambda_info);
    AK24_LOG_TRACE("Registered native builtin: lambda");
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
    AK24_LOG_TRACE("Registered native builtin: cjit-load-builtin");
  }

  akx_builtin_info_t *assert_true_info = AK24_ALLOC(sizeof(akx_builtin_info_t));
  if (assert_true_info) {
    assert_true_info->function = builtin_assert_true;
    assert_true_info->source_path = NULL;
    assert_true_info->load_time = time(NULL);
    assert_true_info->init_fn = NULL;
    assert_true_info->deinit_fn = NULL;
    assert_true_info->reload_fn = NULL;
    assert_true_info->module_data = NULL;
    assert_true_info->module_name = ak_intern("assert-true");
    assert_true_info->unit = NULL;
    akx_rt_add_builtin(rt, "assert-true", assert_true_info);
    AK24_LOG_TRACE("Registered native builtin: assert-true");
  }

  akx_builtin_info_t *assert_false_info =
      AK24_ALLOC(sizeof(akx_builtin_info_t));
  if (assert_false_info) {
    assert_false_info->function = builtin_assert_false;
    assert_false_info->source_path = NULL;
    assert_false_info->load_time = time(NULL);
    assert_false_info->init_fn = NULL;
    assert_false_info->deinit_fn = NULL;
    assert_false_info->reload_fn = NULL;
    assert_false_info->module_data = NULL;
    assert_false_info->module_name = ak_intern("assert-false");
    assert_false_info->unit = NULL;
    akx_rt_add_builtin(rt, "assert-false", assert_false_info);
    AK24_LOG_TRACE("Registered native builtin: assert-false");
  }

  akx_builtin_info_t *assert_eq_info = AK24_ALLOC(sizeof(akx_builtin_info_t));
  if (assert_eq_info) {
    assert_eq_info->function = builtin_assert_eq;
    assert_eq_info->source_path = NULL;
    assert_eq_info->load_time = time(NULL);
    assert_eq_info->init_fn = NULL;
    assert_eq_info->deinit_fn = NULL;
    assert_eq_info->reload_fn = NULL;
    assert_eq_info->module_data = NULL;
    assert_eq_info->module_name = ak_intern("assert-eq");
    assert_eq_info->unit = NULL;
    akx_rt_add_builtin(rt, "assert-eq", assert_eq_info);
    AK24_LOG_TRACE("Registered native builtin: assert-eq");
  }

  akx_builtin_info_t *assert_ne_info = AK24_ALLOC(sizeof(akx_builtin_info_t));
  if (assert_ne_info) {
    assert_ne_info->function = builtin_assert_ne;
    assert_ne_info->source_path = NULL;
    assert_ne_info->load_time = time(NULL);
    assert_ne_info->init_fn = NULL;
    assert_ne_info->deinit_fn = NULL;
    assert_ne_info->reload_fn = NULL;
    assert_ne_info->module_data = NULL;
    assert_ne_info->module_name = ak_intern("assert-ne");
    assert_ne_info->unit = NULL;
    akx_rt_add_builtin(rt, "assert-ne", assert_ne_info);
    AK24_LOG_TRACE("Registered native builtin: assert-ne");
  }
}
