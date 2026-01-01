#include "akx_rt.h"
#include "akx_rt_builtins.h"
#include "builtin_registry.h"
#include <ak24/buffer.h>
#include <ak24/filepath.h>
#include <ak24/intern.h>
#include <ak24/lambda.h>
#include <ak24/map.h>
#include <ctype.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

struct akx_rt_error_ctx_t {
  int error_count;
  akx_parse_error_t *errors;
};

struct akx_runtime_ctx_t {
  int initialized;
  akx_rt_error_ctx_t *error_ctx;
  ak_context_t *current_context;
  map_void_t builtins;
  list_t(ak_cjit_unit_t *) cjit_units;
  const char *current_module_name;
  int in_tail_position;
  int script_argc;
  char **script_argv;
};

akx_runtime_ctx_t *akx_runtime_init(void) {
  akx_runtime_ctx_t *ctx = AK24_ALLOC(sizeof(akx_runtime_ctx_t));
  if (!ctx) {
    AK24_LOG_ERROR("Failed to allocate runtime context");
    return NULL;
  }

  ctx->initialized = 1;
  ctx->error_ctx = AK24_ALLOC(sizeof(akx_rt_error_ctx_t));
  if (!ctx->error_ctx) {
    AK24_LOG_ERROR("Failed to allocate error context");
    AK24_FREE(ctx);
    return NULL;
  }

  ctx->error_ctx->error_count = 0;
  ctx->error_ctx->errors = NULL;

  ctx->current_context = ak_context_new();
  if (!ctx->current_context) {
    AK24_LOG_ERROR("Failed to create runtime context");
    AK24_FREE(ctx->error_ctx);
    AK24_FREE(ctx);
    return NULL;
  }

  map_init_generic(&ctx->builtins, sizeof(char *), map_hash_str, map_cmp_str);
  list_init(&ctx->cjit_units);
  ctx->current_module_name = NULL;
  ctx->in_tail_position = 0;
  ctx->script_argc = 0;
  ctx->script_argv = NULL;

  akx_rt_register_bootstrap_builtins(ctx);
  akx_rt_register_compiled_nuclei(ctx);

  akx_cell_t *nil_cell = akx_rt_alloc_cell(ctx, AKX_TYPE_SYMBOL);
  akx_rt_set_symbol(ctx, nil_cell, "nil");
  akx_rt_scope_set(ctx, "nil", nil_cell);

  AK24_LOG_TRACE("AKX runtime initialized");

  return ctx;
}

void akx_runtime_deinit(akx_runtime_ctx_t *ctx) {
  if (!ctx) {
    return;
  }

  map_iter_t iter = map_iter(&ctx->builtins);
  const char **key_ptr;
  while ((key_ptr = (const char **)map_next_generic(&ctx->builtins, &iter))) {
    void **info_ptr = map_get_generic(&ctx->builtins, key_ptr);
    if (info_ptr && *info_ptr) {
      akx_builtin_info_t *info = (akx_builtin_info_t *)*info_ptr;

      if (info->deinit_fn) {
        ctx->current_module_name = info->module_name;
        info->deinit_fn(ctx);
        ctx->current_module_name = NULL;
      }

      if (info->unit) {
        ak_cjit_unit_free(info->unit);
      }

      if (info->source_path) {
        AK24_FREE(info->source_path);
      }
      AK24_FREE(info);
    }
  }
  map_deinit(&ctx->builtins);

  list_iter_t list_iter = list_iter(&ctx->cjit_units);
  ak_cjit_unit_t **unit_ptr;
  while ((unit_ptr = list_next(&ctx->cjit_units, &list_iter))) {
    if (*unit_ptr) {
      ak_cjit_unit_free(*unit_ptr);
    }
  }
  list_deinit(&ctx->cjit_units);

  while (ctx->current_context) {
    ak_context_t *parent = ctx->current_context->parent;
    ak_context_free(ctx->current_context);
    ctx->current_context = parent;
  }

  if (ctx->error_ctx) {
    akx_parse_error_t *err = ctx->error_ctx->errors;
    while (err) {
      akx_parse_error_t *next = err->next;
      AK24_FREE(err);
      err = next;
    }
    AK24_FREE(ctx->error_ctx);
  }

  AK24_LOG_TRACE("AKX runtime deinitialized");

  AK24_FREE(ctx);
}

int akx_runtime_start(akx_runtime_ctx_t *ctx, akx_cell_list_t *cells) {
  if (!ctx) {
    AK24_LOG_ERROR("Runtime context is NULL");
    return -1;
  }

  if (!cells) {
    AK24_LOG_ERROR("Cell list is NULL");
    return -1;
  }

  list_iter_t iter = list_iter(cells);
  akx_cell_t **cell_ptr;
  while ((cell_ptr = list_next(cells, &iter))) {
    akx_cell_t *result = akx_rt_eval(ctx, *cell_ptr);
    if (!result) {
      if (ctx->error_ctx && ctx->error_ctx->error_count > 0) {
        akx_parse_error_t *err = ctx->error_ctx->errors;
        while (err) {
          AK24_LOG_ERROR("Runtime error: %s", err->message);
          err = err->next;
        }
      }
      return -1;
    }
    akx_cell_free(result);
  }

  return 0;
}

ak_context_t *akx_runtime_get_current_scope(akx_runtime_ctx_t *ctx) {
  if (!ctx) {
    return NULL;
  }
  return ctx->current_context;
}

akx_parse_error_t *akx_runtime_get_errors(akx_runtime_ctx_t *ctx) {
  if (!ctx || !ctx->error_ctx) {
    return NULL;
  }
  return ctx->error_ctx->errors;
}

akx_cell_t *akx_rt_alloc_cell(akx_runtime_ctx_t *rt, akx_type_t type) {
  (void)rt;
  akx_cell_t *cell = AK24_ALLOC(sizeof(akx_cell_t));
  if (!cell) {
    return NULL;
  }
  memset(cell, 0, sizeof(akx_cell_t));
  cell->type = type;
  cell->next = NULL;
  cell->sourceloc = NULL;
  return cell;
}

void akx_rt_free_cell(akx_runtime_ctx_t *rt, akx_cell_t *cell) {
  (void)rt;
  akx_cell_free(cell);
}

void akx_rt_set_symbol(akx_runtime_ctx_t *rt, akx_cell_t *cell,
                       const char *sym) {
  (void)rt;
  if (!cell || !sym) {
    return;
  }
  cell->value.symbol = ak_intern(sym);
}

void akx_rt_set_int(akx_runtime_ctx_t *rt, akx_cell_t *cell, int value) {
  (void)rt;
  if (!cell) {
    return;
  }
  cell->value.integer_literal = value;
}

void akx_rt_set_real(akx_runtime_ctx_t *rt, akx_cell_t *cell, double value) {
  (void)rt;
  if (!cell) {
    return;
  }
  cell->value.real_literal = value;
}

void akx_rt_set_string(akx_runtime_ctx_t *rt, akx_cell_t *cell,
                       const char *str) {
  (void)rt;
  if (!cell || !str) {
    return;
  }
  size_t len = strlen(str);
  cell->value.string_literal = ak_buffer_new(len + 1);
  if (cell->value.string_literal) {
    ak_buffer_copy_to(cell->value.string_literal, (uint8_t *)str, len);
  }
}

void akx_rt_set_list(akx_runtime_ctx_t *rt, akx_cell_t *cell,
                     akx_cell_t *head) {
  (void)rt;
  if (!cell) {
    return;
  }
  cell->value.list_head = head;
}

void akx_rt_set_lambda(akx_runtime_ctx_t *rt, akx_cell_t *cell,
                       ak_lambda_t *lambda) {
  (void)rt;
  if (!cell) {
    return;
  }
  cell->value.lambda = lambda;
}

int akx_rt_cell_is_type(akx_cell_t *cell, akx_type_t type) {
  if (!cell) {
    return 0;
  }
  return cell->type == type;
}

akx_type_t akx_rt_cell_get_type(akx_cell_t *cell) {
  if (!cell) {
    return AKX_TYPE_SYMBOL;
  }
  return cell->type;
}

const char *akx_rt_cell_as_symbol(akx_cell_t *cell) {
  if (!cell || cell->type != AKX_TYPE_SYMBOL) {
    return NULL;
  }
  return cell->value.symbol;
}

int akx_rt_cell_as_int(akx_cell_t *cell) {
  if (!cell || cell->type != AKX_TYPE_INTEGER_LITERAL) {
    return 0;
  }
  return cell->value.integer_literal;
}

double akx_rt_cell_as_real(akx_cell_t *cell) {
  if (!cell || cell->type != AKX_TYPE_REAL_LITERAL) {
    return 0.0;
  }
  return cell->value.real_literal;
}

const char *akx_rt_cell_as_string(akx_cell_t *cell) {
  if (!cell || cell->type != AKX_TYPE_STRING_LITERAL) {
    return NULL;
  }
  if (!cell->value.string_literal) {
    return NULL;
  }
  return (const char *)ak_buffer_data(cell->value.string_literal);
}

akx_cell_t *akx_rt_cell_as_list(akx_cell_t *cell) {
  if (!cell) {
    return NULL;
  }
  if (cell->type != AKX_TYPE_LIST && cell->type != AKX_TYPE_LIST_SQUARE &&
      cell->type != AKX_TYPE_LIST_CURLY && cell->type != AKX_TYPE_LIST_TEMPLE) {
    return NULL;
  }
  return cell->value.list_head;
}

ak_lambda_t *akx_rt_cell_as_lambda(akx_cell_t *cell) {
  if (!cell || cell->type != AKX_TYPE_LAMBDA) {
    return NULL;
  }
  return cell->value.lambda;
}

akx_cell_t *akx_rt_cell_next(akx_cell_t *cell) {
  if (!cell) {
    return NULL;
  }
  return cell->next;
}

size_t akx_rt_list_length(akx_cell_t *list) {
  size_t count = 0;
  akx_cell_t *current = list;
  while (current) {
    count++;
    current = current->next;
  }
  return count;
}

akx_cell_t *akx_rt_list_nth(akx_cell_t *list, size_t n) {
  akx_cell_t *current = list;
  size_t index = 0;
  while (current) {
    if (index == n) {
      return current;
    }
    index++;
    current = current->next;
  }
  return NULL;
}

akx_cell_t *akx_rt_list_append(akx_runtime_ctx_t *rt, akx_cell_t *list,
                               akx_cell_t *item) {
  (void)rt;
  if (!item) {
    return list;
  }
  if (!list) {
    return item;
  }
  akx_cell_t *current = list;
  while (current->next) {
    current = current->next;
  }
  current->next = item;
  return list;
}

ak_context_t *akx_rt_get_scope(akx_runtime_ctx_t *rt) {
  if (!rt) {
    return NULL;
  }
  return rt->current_context;
}

int akx_rt_scope_set(akx_runtime_ctx_t *rt, const char *key, void *value) {
  if (!rt || !rt->current_context || !key) {
    return -1;
  }
  return ak_context_set(rt->current_context, key, value);
}

void *akx_rt_scope_get(akx_runtime_ctx_t *rt, const char *key) {
  if (!rt || !rt->current_context || !key) {
    return NULL;
  }
  return ak_context_get(rt->current_context, key);
}

void akx_rt_push_scope(akx_runtime_ctx_t *rt) {
  if (!rt || !rt->current_context) {
    return;
  }
  rt->current_context = ak_context_push(rt->current_context);
}

void akx_rt_pop_scope(akx_runtime_ctx_t *rt) {
  if (!rt || !rt->current_context) {
    return;
  }
  rt->current_context = ak_context_pop(rt->current_context);
}

void akx_rt_error(akx_runtime_ctx_t *rt, const char *message) {
  if (!rt || !rt->error_ctx || !message) {
    return;
  }

  akx_parse_error_t *error = AK24_ALLOC(sizeof(akx_parse_error_t));
  if (!error) {
    return;
  }

  memset(error, 0, sizeof(akx_parse_error_t));
  snprintf(error->message, sizeof(error->message), "%s", message);
  error->next = NULL;

  if (!rt->error_ctx->errors) {
    rt->error_ctx->errors = error;
  } else {
    akx_parse_error_t *current = rt->error_ctx->errors;
    while (current->next) {
      current = current->next;
    }
    current->next = error;
  }
  rt->error_ctx->error_count++;
}

void akx_rt_error_at(akx_runtime_ctx_t *rt, akx_cell_t *cell,
                     const char *message) {
  if (!rt || !rt->error_ctx || !message) {
    return;
  }

  akx_parse_error_t *error = AK24_ALLOC(sizeof(akx_parse_error_t));
  if (!error) {
    return;
  }

  memset(error, 0, sizeof(akx_parse_error_t));
  snprintf(error->message, sizeof(error->message), "%s", message);
  error->next = NULL;

  if (cell && cell->sourceloc) {
    error->location = cell->sourceloc->start;
  }

  if (!rt->error_ctx->errors) {
    rt->error_ctx->errors = error;
  } else {
    akx_parse_error_t *current = rt->error_ctx->errors;
    while (current->next) {
      current = current->next;
    }
    current->next = error;
  }
  rt->error_ctx->error_count++;
}

void akx_rt_error_fmt(akx_runtime_ctx_t *rt, const char *fmt, ...) {
  if (!rt || !rt->error_ctx || !fmt) {
    return;
  }

  akx_parse_error_t *error = AK24_ALLOC(sizeof(akx_parse_error_t));
  if (!error) {
    return;
  }

  memset(error, 0, sizeof(akx_parse_error_t));

  va_list args;
  va_start(args, fmt);
  vsnprintf(error->message, sizeof(error->message), fmt, args);
  va_end(args);

  error->next = NULL;

  if (!rt->error_ctx->errors) {
    rt->error_ctx->errors = error;
  } else {
    akx_parse_error_t *current = rt->error_ctx->errors;
    while (current->next) {
      current = current->next;
    }
    current->next = error;
  }
  rt->error_ctx->error_count++;
}

akx_cell_t *akx_rt_eval(akx_runtime_ctx_t *rt, akx_cell_t *expr) {
  if (!rt || !expr) {
    return NULL;
  }

  switch (expr->type) {
  case AKX_TYPE_INTEGER_LITERAL:
  case AKX_TYPE_REAL_LITERAL:
  case AKX_TYPE_STRING_LITERAL:
    return akx_cell_clone(expr);

  case AKX_TYPE_SYMBOL: {
    const char *sym = expr->value.symbol;
    void *value = akx_rt_scope_get(rt, sym);
    if (value) {
      akx_cell_t *stored = (akx_cell_t *)value;
      if (stored->type == AKX_TYPE_LAMBDA) {
        return stored;
      }
      return akx_cell_clone(stored);
    }
    char error_msg[256];
    snprintf(error_msg, sizeof(error_msg), "undefined symbol: %s", sym);
    akx_rt_error_at(rt, expr, error_msg);
    return NULL;
  }

  case AKX_TYPE_LIST:
  case AKX_TYPE_LIST_SQUARE:
  case AKX_TYPE_LIST_CURLY:
  case AKX_TYPE_LIST_TEMPLE: {
    akx_cell_t *head = expr->value.list_head;
    if (!head) {
      akx_cell_t *nil = akx_rt_alloc_cell(rt, AKX_TYPE_SYMBOL);
      akx_rt_set_symbol(rt, nil, "nil");
      return nil;
    }

    if (head->type == AKX_TYPE_SYMBOL) {
      const char *func_name = head->value.symbol;

      void **builtin_ptr = map_get_generic(&rt->builtins, &func_name);
      if (builtin_ptr && *builtin_ptr) {
        akx_builtin_info_t *info = (akx_builtin_info_t *)*builtin_ptr;
        akx_cell_t *args = head->next;
        rt->current_module_name = info->module_name;
        akx_cell_t *result = info->function(rt, args);
        rt->current_module_name = NULL;
        return result;
      }

      void *value = akx_rt_scope_get(rt, func_name);
      if (value) {
        akx_cell_t *func_cell = (akx_cell_t *)value;
        if (func_cell->type == AKX_TYPE_LAMBDA) {
          return akx_rt_invoke_lambda(rt, func_cell, head->next);
        }
      }

      char error_msg[256];
      snprintf(error_msg, sizeof(error_msg), "undefined function: %s",
               func_name);
      akx_rt_error_at(rt, head, error_msg);
      return NULL;
    }

    akx_cell_t *evaled_head = akx_rt_eval(rt, head);
    if (evaled_head && evaled_head->type == AKX_TYPE_LAMBDA) {
      akx_cell_t *result = akx_rt_invoke_lambda(rt, evaled_head, head->next);
      akx_cell_free(evaled_head);
      return result;
    }
    if (evaled_head) {
      akx_cell_free(evaled_head);
    }

    akx_rt_error_at(rt, expr, "cannot evaluate list - not a function call");
    return NULL;
  }

  case AKX_TYPE_QUOTED:
    return akx_cell_unwrap_quoted(expr);

  default:
    akx_rt_error(rt, "unknown cell type in eval");
    return NULL;
  }
}

akx_cell_t *akx_rt_eval_tail(akx_runtime_ctx_t *rt, akx_cell_t *expr) {
  if (!rt || !expr) {
    return NULL;
  }

  int saved_tail_flag = rt->in_tail_position;
  rt->in_tail_position = 1;

  akx_cell_t *result = NULL;
  switch (expr->type) {
  case AKX_TYPE_INTEGER_LITERAL:
  case AKX_TYPE_REAL_LITERAL:
  case AKX_TYPE_STRING_LITERAL:
    result = akx_cell_clone(expr);
    break;

  case AKX_TYPE_SYMBOL: {
    const char *sym = expr->value.symbol;
    void *value = akx_rt_scope_get(rt, sym);
    if (value) {
      akx_cell_t *stored = (akx_cell_t *)value;
      if (stored->type == AKX_TYPE_LAMBDA) {
        result = stored;
      } else {
        result = akx_cell_clone(stored);
      }
    } else {
      char error_msg[256];
      snprintf(error_msg, sizeof(error_msg), "undefined symbol: %s", sym);
      akx_rt_error_at(rt, expr, error_msg);
    }
    break;
  }

  case AKX_TYPE_LIST:
  case AKX_TYPE_LIST_SQUARE:
  case AKX_TYPE_LIST_CURLY:
  case AKX_TYPE_LIST_TEMPLE: {
    akx_cell_t *head = expr->value.list_head;
    if (!head) {
      akx_cell_t *nil = akx_rt_alloc_cell(rt, AKX_TYPE_SYMBOL);
      akx_rt_set_symbol(rt, nil, "nil");
      result = nil;
      break;
    }

    if (head->type == AKX_TYPE_SYMBOL) {
      const char *func_name = head->value.symbol;

      void **builtin_ptr = map_get_generic(&rt->builtins, &func_name);
      if (builtin_ptr && *builtin_ptr) {
        akx_builtin_info_t *info = (akx_builtin_info_t *)*builtin_ptr;
        akx_cell_t *args = head->next;
        rt->current_module_name = info->module_name;
        result = info->function(rt, args);
        rt->current_module_name = NULL;
        break;
      }

      void *value = akx_rt_scope_get(rt, func_name);
      if (value) {
        akx_cell_t *func_cell = (akx_cell_t *)value;
        if (func_cell->type == AKX_TYPE_LAMBDA) {
          result = akx_rt_alloc_continuation(rt, func_cell, head->next);
          break;
        }
      }

      char error_msg[256];
      snprintf(error_msg, sizeof(error_msg), "undefined function: %s",
               func_name);
      akx_rt_error_at(rt, head, error_msg);
      break;
    }

    akx_cell_t *evaled_head = akx_rt_eval(rt, head);
    if (evaled_head && evaled_head->type == AKX_TYPE_LAMBDA) {
      result = akx_rt_alloc_continuation(rt, evaled_head, head->next);
    } else {
      if (evaled_head) {
        akx_cell_free(evaled_head);
      }
      akx_rt_error_at(rt, expr, "cannot evaluate list - not a function call");
    }
    break;
  }

  case AKX_TYPE_QUOTED:
    result = akx_cell_unwrap_quoted(expr);
    break;

  default:
    akx_rt_error(rt, "unknown cell type in eval");
    break;
  }

  rt->in_tail_position = saved_tail_flag;
  return result;
}

akx_cell_t *akx_rt_eval_list(akx_runtime_ctx_t *rt, akx_cell_t *list) {
  if (!rt || !list) {
    return NULL;
  }

  akx_cell_t *result_head = NULL;
  akx_cell_t *result_tail = NULL;

  akx_cell_t *current = list;
  while (current) {
    akx_cell_t *evaled = akx_rt_eval(rt, current);
    if (!evaled) {
      if (result_head) {
        akx_cell_free(result_head);
      }
      return NULL;
    }

    if (!result_head) {
      result_head = evaled;
      result_tail = evaled;
    } else {
      result_tail->next = evaled;
      result_tail = evaled;
    }

    current = current->next;
  }

  return result_head;
}

akx_cell_t *akx_rt_eval_and_assert(akx_runtime_ctx_t *rt, akx_cell_t *expr,
                                   akx_type_t expected_type,
                                   const char *error_msg) {
  if (!rt || !expr) {
    return NULL;
  }

  akx_cell_t *result = akx_rt_eval(rt, expr);
  if (!result) {
    return NULL;
  }

  if (result->type != expected_type) {
    if (error_msg) {
      akx_rt_error(rt, error_msg);
    }
    akx_cell_free(result);
    return NULL;
  }

  return result;
}

akx_cell_t *akx_rt_alloc_continuation(akx_runtime_ctx_t *rt,
                                      akx_cell_t *lambda_cell,
                                      akx_cell_t *args) {
  (void)rt;
  akx_cell_t *cont_cell = akx_rt_alloc_cell(rt, AKX_TYPE_CONTINUATION);
  if (!cont_cell) {
    return NULL;
  }

  akx_continuation_t *cont = AK24_ALLOC(sizeof(akx_continuation_t));
  if (!cont) {
    akx_cell_free(cont_cell);
    return NULL;
  }

  cont->lambda_cell = lambda_cell;
  cont->args = args;
  cont_cell->value.continuation = cont;

  return cont_cell;
}

int akx_rt_is_continuation(akx_cell_t *cell) {
  return cell && cell->type == AKX_TYPE_CONTINUATION;
}

akx_cell_t *akx_rt_invoke_lambda(akx_runtime_ctx_t *rt, akx_cell_t *lambda_cell,
                                 akx_cell_t *args) {
  if (!rt || !lambda_cell || lambda_cell->type != AKX_TYPE_LAMBDA) {
    return NULL;
  }

  akx_cell_t *current_lambda = lambda_cell;
  akx_cell_t *current_args = args;
  akx_cell_t *result = NULL;

  while (1) {
    if (!current_lambda || current_lambda->type != AKX_TYPE_LAMBDA) {
      if (result) {
        akx_cell_free(result);
      }
      akx_rt_error(rt, "invalid lambda cell in trampoline");
      return NULL;
    }

    ak_lambda_t *lambda = current_lambda->value.lambda;
    if (!lambda) {
      if (result) {
        akx_cell_free(result);
      }
      akx_rt_error(rt, "invalid lambda cell - no lambda data");
      return NULL;
    }

    akx_rt_push_scope(rt);
    ak_lambda_invoke(lambda, current_args);

    akx_lambda_context_t *lambda_ctx =
        (akx_lambda_context_t *)ak_lambda_get_context(lambda);
    result = lambda_ctx ? lambda_ctx->result : NULL;

    akx_rt_pop_scope(rt);

    if (!result) {
      result = akx_rt_alloc_cell(rt, AKX_TYPE_SYMBOL);
      akx_rt_set_symbol(rt, result, "nil");
      return result;
    }

    if (result->type != AKX_TYPE_CONTINUATION) {
      return result;
    }

    akx_continuation_t *cont = result->value.continuation;
    if (!cont) {
      akx_cell_free(result);
      akx_rt_error(rt, "invalid continuation - no data");
      return NULL;
    }

    current_lambda = cont->lambda_cell;
    current_args = cont->args;

    cont->lambda_cell = NULL;
    cont->args = NULL;
    akx_cell_free(result);
    result = NULL;
  }
}

void akx_rt_module_set_data(akx_runtime_ctx_t *rt, void *data) {
  if (!rt || !rt->current_module_name) {
    return;
  }

  void **info_ptr = map_get_generic(&rt->builtins, &rt->current_module_name);
  if (info_ptr && *info_ptr) {
    akx_builtin_info_t *info = (akx_builtin_info_t *)*info_ptr;
    info->module_data = data;
  }
}

void *akx_rt_module_get_data(akx_runtime_ctx_t *rt) {
  if (!rt || !rt->current_module_name) {
    return NULL;
  }

  void **info_ptr = map_get_generic(&rt->builtins, &rt->current_module_name);
  if (info_ptr && *info_ptr) {
    akx_builtin_info_t *info = (akx_builtin_info_t *)*info_ptr;
    return info->module_data;
  }

  return NULL;
}

void akx_rt_add_builtin(akx_runtime_ctx_t *rt, const char *name,
                        akx_builtin_info_t *info) {
  if (!rt || !name || !info) {
    return;
  }
  map_set_generic(&rt->builtins, &name, info);
}

int akx_rt_register_builtin(akx_runtime_ctx_t *rt, const char *name,
                            const char *root_path, akx_builtin_fn function,
                            ak_cjit_unit_t *unit,
                            void (*init_fn)(akx_runtime_ctx_t *),
                            void (*deinit_fn)(akx_runtime_ctx_t *),
                            void (*reload_fn)(akx_runtime_ctx_t *, void *)) {
  if (!rt || !name) {
    return -1;
  }

  AK24_LOG_DEBUG("Checking for existing builtin with name='%s'", name);
  void **existing_info_ptr = map_get_generic(&rt->builtins, &name);

  if (existing_info_ptr != NULL) {
    akx_builtin_info_t *existing_info =
        (akx_builtin_info_t *)*existing_info_ptr;
    AK24_LOG_TRACE("Hot-reloading builtin: %s", name);

    void *old_state = existing_info->module_data;
    rt->current_module_name = existing_info->module_name;

    if (reload_fn) {
      AK24_LOG_DEBUG("Calling reload hook for %s", name);
      reload_fn(rt, old_state);
    } else if (existing_info->deinit_fn) {
      AK24_LOG_DEBUG("Calling deinit hook for old %s", name);
      existing_info->deinit_fn(rt);
    }

    if (existing_info->source_path) {
      AK24_FREE(existing_info->source_path);
    }

    if (existing_info->unit) {
      ak_cjit_unit_free(existing_info->unit);
    }

    existing_info->function = function;
    if (root_path) {
      existing_info->source_path = AK24_ALLOC(strlen(root_path) + 1);
      if (existing_info->source_path) {
        strcpy(existing_info->source_path, root_path);
      }
    } else {
      existing_info->source_path = NULL;
    }
    existing_info->load_time = time(NULL);
    existing_info->init_fn = init_fn;
    existing_info->deinit_fn = deinit_fn;
    existing_info->reload_fn = reload_fn;
    existing_info->unit = unit;

    if (!reload_fn && init_fn) {
      AK24_LOG_DEBUG("Calling init hook for new %s", name);
      init_fn(rt);
    }

    rt->current_module_name = NULL;
  } else {
    akx_builtin_info_t *info = AK24_ALLOC(sizeof(akx_builtin_info_t));
    if (!info) {
      AK24_LOG_ERROR("Failed to allocate builtin info");
      if (unit) {
        ak_cjit_unit_free(unit);
      }
      return -1;
    }

    info->function = function;
    if (root_path) {
      info->source_path = AK24_ALLOC(strlen(root_path) + 1);
      if (info->source_path) {
        strcpy(info->source_path, root_path);
      }
    } else {
      info->source_path = NULL;
    }
    info->load_time = time(NULL);
    info->init_fn = init_fn;
    info->deinit_fn = deinit_fn;
    info->reload_fn = reload_fn;
    info->module_data = NULL;
    info->module_name = ak_intern(name);
    info->unit = unit;

    AK24_LOG_DEBUG("Storing builtin info in map with name='%s'", name);
    map_set_generic(&rt->builtins, &name, info);
    AK24_LOG_DEBUG("Builtin info stored");

    if (init_fn) {
      rt->current_module_name = info->module_name;
      AK24_LOG_DEBUG("Calling init hook for %s", name);
      init_fn(rt);
      rt->current_module_name = NULL;
    }
  }

  AK24_LOG_TRACE("Successfully registered builtin '%s'", name);
  return 0;
}

char *akx_rt_expand_env_vars(const char *path) {
  if (!path) {
    return NULL;
  }

  const char *dollar = strchr(path, '$');
  if (!dollar) {
    size_t len = strlen(path);
    char *result = AK24_ALLOC(len + 1);
    if (result) {
      memcpy(result, path, len + 1);
    }
    return result;
  }

  size_t estimated_len = strlen(path) + 256;
  char *result = AK24_ALLOC(estimated_len);
  if (!result) {
    return NULL;
  }

  const char *current = path;
  size_t result_pos = 0;
  ak_buffer_t *temp_default_path = NULL;

  while (*current) {
    if (*current == '$') {
      const char *var_start = current + 1;
      const char *var_end = var_start;

      while (*var_end && (isalnum(*var_end) || *var_end == '_')) {
        var_end++;
      }

      if (var_end > var_start) {
        size_t var_len = var_end - var_start;
        char *var_name = AK24_ALLOC(var_len + 1);
        if (!var_name) {
          AK24_FREE(result);
          if (temp_default_path) {
            ak_buffer_free(temp_default_path);
          }
          return NULL;
        }
        memcpy(var_name, var_start, var_len);
        var_name[var_len] = '\0';

        const char *var_value = getenv(var_name);
        if (!var_value && strcmp(var_name, "AKX_HOME") == 0) {
          const char *home = getenv("HOME");
          if (home) {
            temp_default_path = ak_filepath_join(2, home, ".akx");
            if (temp_default_path) {
              var_value = (const char *)ak_buffer_data(temp_default_path);
            }
          }
        }

        if (var_value) {
          size_t val_len = strlen(var_value);
          if (result_pos + val_len >= estimated_len) {
            estimated_len = result_pos + val_len + 256;
            char *new_result = AK24_ALLOC(estimated_len);
            if (!new_result) {
              AK24_FREE(result);
              AK24_FREE(var_name);
              if (temp_default_path) {
                ak_buffer_free(temp_default_path);
              }
              return NULL;
            }
            memcpy(new_result, result, result_pos);
            AK24_FREE(result);
            result = new_result;
          }
          memcpy(result + result_pos, var_value, val_len);
          result_pos += val_len;
        }

        AK24_FREE(var_name);
        current = var_end;
        continue;
      }
    }

    if (result_pos + 1 >= estimated_len) {
      estimated_len = result_pos + 256;
      char *new_result = AK24_ALLOC(estimated_len);
      if (!new_result) {
        AK24_FREE(result);
        if (temp_default_path) {
          ak_buffer_free(temp_default_path);
        }
        return NULL;
      }
      memcpy(new_result, result, result_pos);
      AK24_FREE(result);
      result = new_result;
    }
    result[result_pos++] = *current;
    current++;
  }

  result[result_pos] = '\0';

  if (temp_default_path) {
    ak_buffer_free(temp_default_path);
  }

  return result;
}

map_void_t *akx_rt_get_builtins(akx_runtime_ctx_t *rt) {
  if (!rt) {
    return NULL;
  }
  return &rt->builtins;
}

void akx_runtime_set_script_args(akx_runtime_ctx_t *ctx, int argc,
                                 char **argv) {
  if (!ctx) {
    return;
  }
  ctx->script_argc = argc;
  ctx->script_argv = argv;
}

int akx_rt_get_script_argc(akx_runtime_ctx_t *rt) {
  if (!rt) {
    return 0;
  }
  return rt->script_argc;
}

char **akx_rt_get_script_argv(akx_runtime_ctx_t *rt) {
  if (!rt) {
    return NULL;
  }
  return rt->script_argv;
}
