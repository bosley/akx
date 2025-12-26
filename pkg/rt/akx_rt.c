#include "akx_rt.h"
#include <ak24/buffer.h>
#include <ak24/cjit.h>
#include <ak24/intern.h>
#include <ak24/map.h>
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
  ak_context_t *context;
  map_void_t builtins;
  list_t(ak_cjit_unit_t *) cjit_units;
  const char *current_module_name;
};

static akx_cell_t *builtin_cjit_load_builtin(akx_runtime_ctx_t *rt,
                                             akx_cell_t *args) {
  if (akx_rt_list_length(args) != 2) {
    akx_rt_error(rt, "cjit-load-builtin requires 2 arguments: name and path");
    return NULL;
  }

  akx_cell_t *name_cell = akx_rt_list_nth(args, 0);
  if (!name_cell || name_cell->type != AKX_TYPE_SYMBOL) {
    akx_rt_error(rt, "first argument must be a symbol");
    return NULL;
  }

  akx_cell_t *path_cell = akx_rt_eval_and_assert(
      rt, akx_rt_list_nth(args, 1), AKX_TYPE_STRING_LITERAL,
      "second argument must be a string");
  if (!path_cell) {
    return NULL;
  }

  const char *name = akx_rt_cell_as_symbol(name_cell);
  const char *path = akx_rt_cell_as_string(path_cell);

  int result = akx_runtime_load_builtin(rt, name, path);

  akx_rt_free_cell(rt, path_cell);

  akx_cell_t *ret = akx_rt_alloc_cell(rt, AKX_TYPE_SYMBOL);
  if (result == 0) {
    akx_rt_set_symbol(rt, ret, "t");
  } else {
    akx_rt_set_symbol(rt, ret, "nil");
  }
  return ret;
}

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

  ctx->context = ak_context_new();
  if (!ctx->context) {
    AK24_LOG_ERROR("Failed to create runtime context");
    AK24_FREE(ctx->error_ctx);
    AK24_FREE(ctx);
    return NULL;
  }

  map_init_generic(&ctx->builtins, sizeof(char *), map_hash_str, map_cmp_str);
  list_init(&ctx->cjit_units);
  ctx->current_module_name = NULL;

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
    map_set_generic(&ctx->builtins, &name, bootstrap_info);
    AK24_LOG_TRACE("Registered native builtin: cjit-load-builtin");
  }

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

  if (ctx->context) {
    ak_context_free(ctx->context);
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
  return ctx->context;
}

akx_parse_error_t *akx_runtime_get_errors(akx_runtime_ctx_t *ctx) {
  if (!ctx || !ctx->error_ctx) {
    return NULL;
  }
  return ctx->error_ctx->errors;
}

static const char *generate_abi_header(void) {
  return "#include <stddef.h>\n"
         "#include <stdint.h>\n"
         "#include <stdio.h>\n"
         "#include <stdlib.h>\n"
         "\n"
         "typedef struct akx_runtime_ctx_t akx_runtime_ctx_t;\n"
         "typedef struct akx_cell_t akx_cell_t;\n"
         "typedef struct ak_context_t ak_context_t;\n"
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
         "  AKX_TYPE_QUOTED = 8\n"
         "} akx_type_t;\n"
         "\n"
         "typedef akx_cell_t* (*akx_builtin_fn)(akx_runtime_ctx_t*, "
         "akx_cell_t*);\n"
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
         "\n"
         "extern int akx_rt_cell_is_type(akx_cell_t *cell, akx_type_t type);\n"
         "extern const char* akx_rt_cell_as_symbol(akx_cell_t *cell);\n"
         "extern int akx_rt_cell_as_int(akx_cell_t *cell);\n"
         "extern double akx_rt_cell_as_real(akx_cell_t *cell);\n"
         "extern const char* akx_rt_cell_as_string(akx_cell_t *cell);\n"
         "extern akx_cell_t* akx_rt_cell_as_list(akx_cell_t *cell);\n"
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
         "extern void akx_rt_module_set_data(akx_runtime_ctx_t *rt, void "
         "*data);\n"
         "extern void* akx_rt_module_get_data(akx_runtime_ctx_t *rt);\n"
         "\n";
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

int akx_rt_cell_is_type(akx_cell_t *cell, akx_type_t type) {
  if (!cell) {
    return 0;
  }
  return cell->type == type;
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
  return rt->context;
}

int akx_rt_scope_set(akx_runtime_ctx_t *rt, const char *key, void *value) {
  if (!rt || !rt->context || !key) {
    return -1;
  }
  return ak_context_set(rt->context, key, value);
}

void *akx_rt_scope_get(akx_runtime_ctx_t *rt, const char *key) {
  if (!rt || !rt->context || !key) {
    return NULL;
  }
  return ak_context_get(rt->context, key);
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
      return (akx_cell_t *)value;
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

      char error_msg[256];
      snprintf(error_msg, sizeof(error_msg), "undefined function: %s",
               func_name);
      akx_rt_error_at(rt, head, error_msg);
      return NULL;
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

int akx_runtime_load_builtin(akx_runtime_ctx_t *rt, const char *name,
                             const char *source_path) {
  if (!rt || !name || !source_path) {
    AK24_LOG_ERROR("Invalid arguments to akx_runtime_load_builtin");
    return -1;
  }

  AK24_LOG_DEBUG("Reading source file: %s", source_path);
  ak_buffer_t *source_buf = ak_buffer_from_file(source_path);
  if (!source_buf) {
    AK24_LOG_ERROR("Failed to read source file: %s", source_path);
    return -1;
  }
  AK24_LOG_DEBUG("Source file read successfully (%zu bytes)",
                 ak_buffer_count(source_buf));

  AK24_LOG_DEBUG("Generating ABI header");
  const char *abi_header = generate_abi_header();
  size_t abi_len = strlen(abi_header);
  size_t source_len = ak_buffer_count(source_buf);
  size_t total_len = abi_len + source_len + 1;

  AK24_LOG_DEBUG("Allocating %zu bytes for combined source", total_len);
  char *combined_source = AK24_ALLOC(total_len);
  if (!combined_source) {
    AK24_LOG_ERROR("Failed to allocate memory for combined source");
    ak_buffer_free(source_buf);
    return -1;
  }

  AK24_LOG_DEBUG("Combining ABI header and source");
  memcpy(combined_source, abi_header, abi_len);
  memcpy(combined_source + abi_len, ak_buffer_data(source_buf), source_len);
  combined_source[abi_len + source_len] = '\0';

  ak_buffer_free(source_buf);

  AK24_LOG_DEBUG("Creating CJIT unit");
  ak_cjit_unit_t *unit = ak_cjit_unit_new(NULL, NULL, NULL);
  if (!unit) {
    AK24_LOG_ERROR("Failed to create CJIT unit");
    AK24_FREE(combined_source);
    return -1;
  }
  AK24_LOG_DEBUG("CJIT unit created");

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
  ak_cjit_add_symbol(unit, "akx_rt_cell_is_type", akx_rt_cell_is_type);
  ak_cjit_add_symbol(unit, "akx_rt_cell_as_symbol", akx_rt_cell_as_symbol);
  ak_cjit_add_symbol(unit, "akx_rt_cell_as_int", akx_rt_cell_as_int);
  ak_cjit_add_symbol(unit, "akx_rt_cell_as_real", akx_rt_cell_as_real);
  ak_cjit_add_symbol(unit, "akx_rt_cell_as_string", akx_rt_cell_as_string);
  ak_cjit_add_symbol(unit, "akx_rt_cell_as_list", akx_rt_cell_as_list);
  ak_cjit_add_symbol(unit, "akx_rt_cell_next", akx_rt_cell_next);
  ak_cjit_add_symbol(unit, "akx_rt_list_length", akx_rt_list_length);
  ak_cjit_add_symbol(unit, "akx_rt_list_nth", akx_rt_list_nth);
  ak_cjit_add_symbol(unit, "akx_rt_list_append", akx_rt_list_append);
  ak_cjit_add_symbol(unit, "akx_rt_get_scope", akx_rt_get_scope);
  ak_cjit_add_symbol(unit, "akx_rt_scope_set", akx_rt_scope_set);
  ak_cjit_add_symbol(unit, "akx_rt_scope_get", akx_rt_scope_get);
  ak_cjit_add_symbol(unit, "akx_rt_error", akx_rt_error);
  ak_cjit_add_symbol(unit, "akx_rt_error_fmt", akx_rt_error_fmt);
  ak_cjit_add_symbol(unit, "akx_rt_eval", akx_rt_eval);
  ak_cjit_add_symbol(unit, "akx_rt_eval_list", akx_rt_eval_list);
  ak_cjit_add_symbol(unit, "akx_rt_eval_and_assert", akx_rt_eval_and_assert);
  ak_cjit_add_symbol(unit, "akx_rt_module_set_data", akx_rt_module_set_data);
  ak_cjit_add_symbol(unit, "akx_rt_module_get_data", akx_rt_module_get_data);

#pragma clang diagnostic pop

  AK24_LOG_DEBUG("All symbols added");

  AK24_LOG_DEBUG("Adding source to CJIT unit");
  if (ak_cjit_add_source(unit, combined_source, source_path) != AK_CJIT_OK) {
    AK24_LOG_ERROR("Failed to add source to CJIT unit");
    ak_cjit_unit_free(unit);
    AK24_FREE(combined_source);
    return -1;
  }
  AK24_LOG_DEBUG("Source added successfully");

  AK24_FREE(combined_source);

  AK24_LOG_DEBUG("Compiling builtin");
  if (ak_cjit_relocate(unit) != AK_CJIT_OK) {
    AK24_LOG_ERROR("Failed to compile builtin: %s", source_path);
    ak_cjit_unit_free(unit);
    return -1;
  }
  AK24_LOG_DEBUG("Compilation successful");

  AK24_LOG_DEBUG("Looking up symbol '%s'", name);
  akx_builtin_fn function = (akx_builtin_fn)ak_cjit_get_symbol(unit, name);
  if (!function) {
    AK24_LOG_ERROR("Failed to find symbol '%s' in compiled builtin", name);
    ak_cjit_unit_free(unit);
    return -1;
  }
  AK24_LOG_DEBUG("Symbol found");

  char init_name[256];
  char deinit_name[256];
  char reload_name[256];
  snprintf(init_name, sizeof(init_name), "%s_init", name);
  snprintf(deinit_name, sizeof(deinit_name), "%s_deinit", name);
  snprintf(reload_name, sizeof(reload_name), "%s_reload", name);

  void (*init_fn)(akx_runtime_ctx_t *) =
      (void (*)(akx_runtime_ctx_t *))ak_cjit_get_symbol(unit, init_name);
  void (*deinit_fn)(akx_runtime_ctx_t *) =
      (void (*)(akx_runtime_ctx_t *))ak_cjit_get_symbol(unit, deinit_name);
  void (*reload_fn)(akx_runtime_ctx_t *, void *) =
      (void (*)(akx_runtime_ctx_t *, void *))ak_cjit_get_symbol(unit,
                                                                reload_name);

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
    existing_info->source_path = AK24_ALLOC(strlen(source_path) + 1);
    if (existing_info->source_path) {
      strcpy(existing_info->source_path, source_path);
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
      ak_cjit_unit_free(unit);
      return -1;
    }

    info->function = function;
    info->source_path = AK24_ALLOC(strlen(source_path) + 1);
    if (info->source_path) {
      strcpy(info->source_path, source_path);
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

  AK24_LOG_TRACE("Successfully loaded builtin '%s' from %s", name, source_path);
  return 0;
}
