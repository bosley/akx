#include "akx_cell.h"
#include <ctype.h>
#include <string.h>

typedef struct parse_context_t {
  akx_parse_error_t *errors;
  akx_parse_error_t *errors_tail;
} parse_context_t;

static void add_parse_error(parse_context_t *ctx, ak_source_loc_t *loc,
                            const char *message) {
  for (akx_parse_error_t *err = ctx->errors; err; err = err->next) {
    if (err->location.file == loc->file &&
        err->location.offset == loc->offset) {
      return;
    }
  }

  akx_parse_error_t *error = AK24_ALLOC(sizeof(akx_parse_error_t));
  if (!error) {
    return;
  }

  error->location = *loc;
  snprintf(error->message, sizeof(error->message), "%s", message);
  error->next = NULL;

  if (!ctx->errors) {
    ctx->errors = error;
    ctx->errors_tail = error;
  } else {
    ctx->errors_tail->next = error;
    ctx->errors_tail = error;
  }
}

static akx_cell_t *create_cell(akx_type_t type, ak_source_range_t *range) {
  akx_cell_t *cell = AK24_ALLOC(sizeof(akx_cell_t));
  if (!cell) {
    return NULL;
  }

  memset(cell, 0, sizeof(akx_cell_t));
  cell->type = type;
  cell->next = NULL;

  if (range) {
    cell->sourceloc = AK24_ALLOC(sizeof(ak_source_range_t));
    if (cell->sourceloc) {
      memcpy(cell->sourceloc, range, sizeof(ak_source_range_t));
    }
  } else {
    cell->sourceloc = NULL;
  }

  return cell;
}

void akx_cell_free(akx_cell_t *cell) {
  while (cell) {
    akx_cell_t *next = cell->next;

    if (cell->type == AKX_TYPE_STRING_LITERAL && cell->value.string_literal) {
      ak_buffer_free(cell->value.string_literal);
    } else if ((cell->type == AKX_TYPE_LIST ||
                cell->type == AKX_TYPE_LIST_SQUARE ||
                cell->type == AKX_TYPE_LIST_CURLY ||
                cell->type == AKX_TYPE_LIST_TEMPLE) &&
               cell->value.list_head) {
      akx_cell_free(cell->value.list_head);
    } else if (cell->type == AKX_TYPE_QUOTED && cell->value.quoted_literal) {
      ak_buffer_free(cell->value.quoted_literal);
    }

    if (cell->sourceloc) {
      AK24_FREE(cell->sourceloc);
    }

    AK24_FREE(cell);
    cell = next;
  }
}

static akx_cell_t *parse_argument(ak_scanner_t *scanner,
                                  ak_source_file_t *source_file,
                                  parse_context_t *ctx);
static akx_cell_t *parse_explicit_list(ak_scanner_t *scanner,
                                       ak_source_file_t *source_file,
                                       parse_context_t *ctx);
static akx_cell_t *parse_virtual_list(ak_scanner_t *scanner,
                                      ak_source_file_t *source_file,
                                      parse_context_t *ctx);
static akx_cell_t *parse_quoted(ak_scanner_t *scanner,
                                ak_source_file_t *source_file,
                                parse_context_t *ctx);

static akx_cell_t *parse_static_type(ak_scanner_t *scanner,
                                     ak_source_file_t *source_file,
                                     parse_context_t *ctx) {
  (void)ctx;
  size_t start_pos = scanner->position;

  ak_scanner_static_type_result_t result =
      ak_scanner_read_static_base_type(scanner, NULL);

  if (!result.success) {
    return NULL;
  }

  ak_source_loc_t start_loc = ak_source_loc_from_offset(source_file, start_pos);
  ak_source_loc_t end_loc =
      ak_source_loc_from_offset(source_file, scanner->position);
  ak_source_range_t range = ak_source_range_new(start_loc, end_loc);

  akx_cell_t *cell = NULL;

  switch (result.data.base) {
  case AK24_STATIC_BASE_INTEGER: {
    cell = create_cell(AKX_TYPE_INTEGER_LITERAL, &range);
    if (cell) {
      char temp[32];
      size_t len = result.data.byte_length < 31 ? result.data.byte_length : 31;
      memcpy(temp, result.data.data, len);
      temp[len] = '\0';
      cell->value.integer_literal = atoi(temp);
    }
    break;
  }

  case AK24_STATIC_BASE_REAL: {
    cell = create_cell(AKX_TYPE_REAL_LITERAL, &range);
    if (cell) {
      char temp[64];
      size_t len = result.data.byte_length < 63 ? result.data.byte_length : 63;
      memcpy(temp, result.data.data, len);
      temp[len] = '\0';
      cell->value.real_literal = atof(temp);
    }
    break;
  }

  case AK24_STATIC_BASE_SYMBOL: {
    cell = create_cell(AKX_TYPE_SYMBOL, &range);
    if (cell) {
      cell->value.symbol =
          ak_intern_n((const char *)result.data.data, result.data.byte_length);
    }
    break;
  }

  default:
    break;
  }

  return cell;
}

static akx_cell_t *parse_string_literal(ak_scanner_t *scanner,
                                        ak_source_file_t *source_file,
                                        parse_context_t *ctx) {
  size_t start_pos = scanner->position;

  uint8_t escape = '\\';
  ak_scanner_find_group_result_t result =
      ak_scanner_find_group(scanner, '"', '"', &escape, false);

  if (!result.success) {
    ak_source_loc_t error_loc =
        ak_source_loc_from_offset(source_file, start_pos);
    add_parse_error(ctx, &error_loc, "unterminated string literal");
    return NULL;
  }

  ak_source_loc_t start_loc = ak_source_loc_from_offset(source_file, start_pos);
  ak_source_loc_t end_loc = ak_source_loc_from_offset(
      source_file, result.index_of_closing_symbol + 1);
  ak_source_range_t range = ak_source_range_new(start_loc, end_loc);

  akx_cell_t *cell = create_cell(AKX_TYPE_STRING_LITERAL, &range);
  if (!cell) {
    return NULL;
  }

  size_t content_start = result.index_of_start_symbol + 1;
  size_t content_len = result.index_of_closing_symbol - content_start;

  cell->value.string_literal = ak_buffer_new(content_len + 1);
  if (!cell->value.string_literal) {
    akx_cell_free(cell);
    return NULL;
  }

  uint8_t *buf_data = ak_buffer_data(scanner->buffer);
  ak_buffer_copy_to(cell->value.string_literal, buf_data + content_start,
                    content_len);

  scanner->position = result.index_of_closing_symbol + 1;

  return cell;
}

static akx_cell_t *parse_explicit_list_generic(
    ak_scanner_t *scanner, ak_source_file_t *source_file, uint8_t open_delim,
    uint8_t close_delim, akx_type_t list_type, parse_context_t *ctx) {
  size_t start_pos = scanner->position;

  ak_scanner_find_group_result_t result =
      ak_scanner_find_group(scanner, open_delim, close_delim, NULL, true);

  if (!result.success) {
    size_t scan_pos = scanner->position + 1;
    uint8_t *buf_data = ak_buffer_data(scanner->buffer);
    size_t buf_len = ak_buffer_count(scanner->buffer);
    size_t last_open_pos = start_pos;
    int depth = 0;

    for (size_t i = scan_pos; i < buf_len; i++) {
      uint8_t c = buf_data[i];
      if (c == open_delim) {
        last_open_pos = i;
        depth++;
      } else if (c == close_delim) {
        if (depth > 0) {
          depth--;
        }
      }
    }

    ak_source_loc_t error_loc =
        ak_source_loc_from_offset(source_file, last_open_pos);
    char error_msg[128];
    snprintf(error_msg, sizeof(error_msg),
             "unterminated list - missing closing '%c'", close_delim);
    add_parse_error(ctx, &error_loc, error_msg);
    return NULL;
  }

  ak_source_loc_t start_loc = ak_source_loc_from_offset(source_file, start_pos);
  ak_source_loc_t end_loc = ak_source_loc_from_offset(
      source_file, result.index_of_closing_symbol + 1);
  ak_source_range_t range = ak_source_range_new(start_loc, end_loc);

  akx_cell_t *list_cell = create_cell(list_type, &range);
  if (!list_cell) {
    return NULL;
  }

  size_t content_start = result.index_of_start_symbol + 1;
  size_t content_len = result.index_of_closing_symbol - content_start;

  if (content_len > 0) {
    int bytes_copied = 0;
    ak_buffer_t *sub_buf = ak_buffer_sub_buffer(scanner->buffer, content_start,
                                                content_len, &bytes_copied);

    if (sub_buf && bytes_copied > 0) {
      ak_scanner_t *sub_scanner = ak_scanner_new(sub_buf, 0);
      if (sub_scanner) {
        akx_cell_t *head = NULL;
        akx_cell_t *tail = NULL;

        while (sub_scanner->position < ak_buffer_count(sub_buf)) {
          if (!ak_scanner_skip_whitespace_and_comments(sub_scanner)) {
            break;
          }

          if (sub_scanner->position >= ak_buffer_count(sub_buf)) {
            break;
          }

          akx_cell_t *arg = parse_argument(sub_scanner, source_file, ctx);
          if (!arg) {
            break;
          }

          if (!head) {
            head = arg;
            tail = arg;
          } else {
            tail->next = arg;
            tail = arg;
          }

          while (tail->next) {
            tail = tail->next;
          }
        }

        list_cell->value.list_head = head;
        ak_scanner_free(sub_scanner);
      }
      ak_buffer_free(sub_buf);
    }
  } else {
    list_cell->value.list_head = NULL;
  }

  scanner->position = result.index_of_closing_symbol + 1;

  return list_cell;
}

static akx_cell_t *parse_explicit_list(ak_scanner_t *scanner,
                                       ak_source_file_t *source_file,
                                       parse_context_t *ctx) {
  return parse_explicit_list_generic(scanner, source_file, '(', ')',
                                     AKX_TYPE_LIST, ctx);
}

static akx_cell_t *parse_explicit_list_square(ak_scanner_t *scanner,
                                              ak_source_file_t *source_file,
                                              parse_context_t *ctx) {
  return parse_explicit_list_generic(scanner, source_file, '[', ']',
                                     AKX_TYPE_LIST_SQUARE, ctx);
}

static akx_cell_t *parse_explicit_list_curly(ak_scanner_t *scanner,
                                             ak_source_file_t *source_file,
                                             parse_context_t *ctx) {
  return parse_explicit_list_generic(scanner, source_file, '{', '}',
                                     AKX_TYPE_LIST_CURLY, ctx);
}

static akx_cell_t *parse_explicit_list_temple(ak_scanner_t *scanner,
                                              ak_source_file_t *source_file,
                                              parse_context_t *ctx) {
  return parse_explicit_list_generic(scanner, source_file, '<', '>',
                                     AKX_TYPE_LIST_TEMPLE, ctx);
}

static int is_newline(uint8_t c) { return c == '\n' || c == '\r'; }

static akx_cell_t *parse_virtual_list(ak_scanner_t *scanner,
                                      ak_source_file_t *source_file,
                                      parse_context_t *ctx) {
  size_t start_pos = scanner->position;

  akx_cell_t *first_symbol = parse_static_type(scanner, source_file, ctx);
  if (!first_symbol || first_symbol->type != AKX_TYPE_SYMBOL) {
    if (first_symbol) {
      akx_cell_free(first_symbol);
    }
    return NULL;
  }

  akx_cell_t *head = first_symbol;
  akx_cell_t *tail = first_symbol;

  while (scanner->position < ak_buffer_count(scanner->buffer)) {
    size_t pos_before_ws = scanner->position;

    if (!ak_scanner_goto_next_non_white(scanner)) {
      break;
    }

    uint8_t *buf_data = ak_buffer_data(scanner->buffer);
    for (size_t i = pos_before_ws;
         i < scanner->position && i < ak_buffer_count(scanner->buffer); i++) {
      if (is_newline(buf_data[i])) {
        goto end_virtual_list;
      }
    }

    if (scanner->position >= ak_buffer_count(scanner->buffer)) {
      break;
    }

    if (buf_data[scanner->position] == ';') {
      break;
    }

    if (is_newline(buf_data[scanner->position])) {
      break;
    }

    akx_cell_t *arg = parse_argument(scanner, source_file, ctx);
    if (!arg) {
      break;
    }

    tail->next = arg;
    tail = arg;

    while (tail->next) {
      tail = tail->next;
    }
  }

end_virtual_list:;

  ak_source_loc_t start_loc = ak_source_loc_from_offset(source_file, start_pos);
  ak_source_loc_t end_loc =
      ak_source_loc_from_offset(source_file, scanner->position);
  ak_source_range_t range = ak_source_range_new(start_loc, end_loc);

  akx_cell_t *list_cell = create_cell(AKX_TYPE_LIST, &range);
  if (!list_cell) {
    akx_cell_free(head);
    return NULL;
  }

  list_cell->value.list_head = head;

  return list_cell;
}

static akx_cell_t *parse_quoted(ak_scanner_t *scanner,
                                ak_source_file_t *source_file,
                                parse_context_t *ctx) {
  size_t start_pos = scanner->position;

  scanner->position++;

  akx_cell_t *inner = parse_argument(scanner, source_file, ctx);
  if (!inner) {
    ak_source_loc_t error_loc =
        ak_source_loc_from_offset(source_file, start_pos);
    add_parse_error(ctx, &error_loc, "invalid expression after quote");
    return NULL;
  }

  size_t content_start = start_pos + 1;
  size_t content_end = scanner->position;

  ak_source_loc_t start_loc = ak_source_loc_from_offset(source_file, start_pos);
  ak_source_loc_t end_loc = ak_source_loc_from_offset(source_file, content_end);
  ak_source_range_t range = ak_source_range_new(start_loc, end_loc);

  akx_cell_t *quoted_cell = create_cell(AKX_TYPE_QUOTED, &range);
  if (!quoted_cell) {
    akx_cell_free(inner);
    return NULL;
  }

  uint8_t *buf_data = ak_buffer_data(scanner->buffer);
  size_t content_len = content_end - content_start;
  quoted_cell->value.quoted_literal = ak_buffer_new(content_len + 1);
  if (!quoted_cell->value.quoted_literal) {
    akx_cell_free(quoted_cell);
    akx_cell_free(inner);
    return NULL;
  }

  ak_buffer_copy_to(quoted_cell->value.quoted_literal, buf_data + content_start,
                    content_len);

  akx_cell_free(inner);

  return quoted_cell;
}

static akx_cell_t *parse_argument(ak_scanner_t *scanner,
                                  ak_source_file_t *source_file,
                                  parse_context_t *ctx) {
  if (!ak_scanner_skip_whitespace_and_comments(scanner)) {
    return NULL;
  }

  if (scanner->position >= ak_buffer_count(scanner->buffer)) {
    return NULL;
  }

  uint8_t *buf_data = ak_buffer_data(scanner->buffer);
  uint8_t current = buf_data[scanner->position];

  switch (current) {
  case '(':
    return parse_explicit_list(scanner, source_file, ctx);
  case '[':
    return parse_explicit_list_square(scanner, source_file, ctx);
  case '{':
    return parse_explicit_list_curly(scanner, source_file, ctx);
  case '<':
    return parse_explicit_list_temple(scanner, source_file, ctx);
  case '"':
    return parse_string_literal(scanner, source_file, ctx);
  case '\'':
    return parse_quoted(scanner, source_file, ctx);
  default:
    return parse_static_type(scanner, source_file, ctx);
  }
}

akx_parse_result_t akx_cell_parse_buffer(ak_buffer_t *buf,
                                         const char *filename) {
  akx_parse_result_t result;
  list_init(&result.cells);
  result.errors = NULL;
  result.source_file = NULL;

  if (!buf || !filename) {
    return result;
  }

  parse_context_t ctx = {NULL, NULL};

  ak_source_file_t *source_file = ak_source_file_new(
      filename, (const char *)ak_buffer_data(buf), ak_buffer_count(buf));
  if (!source_file) {
    AK24_LOG_ERROR("Failed to create source file for %s", filename);
    return result;
  }

  ak_scanner_t *scanner = ak_scanner_new(buf, 0);
  if (!scanner) {
    AK24_LOG_ERROR("Failed to create scanner");
    ak_source_file_release(source_file);
    return result;
  }

  while (scanner->position < ak_buffer_count(buf)) {
    if (!ak_scanner_skip_whitespace_and_comments(scanner)) {
      break;
    }

    if (scanner->position >= ak_buffer_count(buf)) {
      break;
    }

    uint8_t *buf_data = ak_buffer_data(buf);
    uint8_t current = buf_data[scanner->position];

    akx_cell_t *expr = NULL;

    switch (current) {
    case '(':
      expr = parse_explicit_list(scanner, source_file, &ctx);
      break;
    case '[':
      expr = parse_explicit_list_square(scanner, source_file, &ctx);
      break;
    case '{':
      expr = parse_explicit_list_curly(scanner, source_file, &ctx);
      break;
    case '<':
      expr = parse_explicit_list_temple(scanner, source_file, &ctx);
      break;
    case '\'':
      expr = parse_quoted(scanner, source_file, &ctx);
      break;
    default:
      expr = parse_virtual_list(scanner, source_file, &ctx);
      break;
    }

    if (!expr) {
      break;
    }

    akx_cell_t *cell_iter = expr;
    while (cell_iter) {
      akx_cell_t *next = cell_iter->next;
      cell_iter->next = NULL;
      list_push(&result.cells, cell_iter);
      cell_iter = next;
    }
  }

  ak_scanner_free(scanner);

  result.errors = ctx.errors;
  result.source_file = source_file;
  return result;
}

akx_parse_result_t akx_cell_parse_file(const char *path) {
  akx_parse_result_t result;
  list_init(&result.cells);
  result.errors = NULL;
  result.source_file = NULL;

  if (!path) {
    return result;
  }

  ak_buffer_t *buf = ak_buffer_from_file(path);
  if (!buf) {
    AK24_LOG_ERROR("Failed to read file: %s", path);
    return result;
  }

  result = akx_cell_parse_buffer(buf, path);
  ak_buffer_free(buf);

  return result;
}

void akx_parse_result_free(akx_parse_result_t *result) {
  if (!result) {
    return;
  }

  list_iter_t iter = list_iter(&result->cells);
  akx_cell_t **cell_ptr;
  while ((cell_ptr = list_next(&result->cells, &iter))) {
    akx_cell_free(*cell_ptr);
  }
  list_deinit(&result->cells);

  akx_parse_error_t *err = result->errors;
  while (err) {
    akx_parse_error_t *next = err->next;
    AK24_FREE(err);
    err = next;
  }
  result->errors = NULL;

  if (result->source_file) {
    ak_source_file_release(result->source_file);
    result->source_file = NULL;
  }
}

akx_cell_t *akx_cell_unwrap_quoted(akx_cell_t *quoted_cell) {
  if (!quoted_cell || quoted_cell->type != AKX_TYPE_QUOTED) {
    return NULL;
  }

  if (!quoted_cell->value.quoted_literal) {
    return NULL;
  }

  const char *filename = "<quoted>";
  if (quoted_cell->sourceloc && quoted_cell->sourceloc->start.file) {
    filename = ak_source_file_name(quoted_cell->sourceloc->start.file);
  }

  akx_parse_result_t result =
      akx_cell_parse_buffer(quoted_cell->value.quoted_literal, filename);

  akx_cell_t *first = list_count(&result.cells) > 0
                          ? *((akx_cell_t **)list_get(&result.cells, 0))
                          : NULL;

  list_deinit(&result.cells);
  if (result.source_file) {
    ak_source_file_release(result.source_file);
  }

  return first;
}

static ak_buffer_t *clone_buffer(ak_buffer_t *buf) {
  if (!buf) {
    return NULL;
  }

  size_t size = ak_buffer_count(buf);
  ak_buffer_t *cloned = ak_buffer_new(size);
  if (!cloned) {
    return NULL;
  }

  uint8_t *src_data = ak_buffer_data(buf);
  ak_buffer_copy_to(cloned, src_data, size);

  return cloned;
}

akx_cell_t *akx_cell_clone(akx_cell_t *cell) {
  if (!cell) {
    return NULL;
  }

  akx_cell_t *cloned = create_cell(cell->type, cell->sourceloc);
  if (!cloned) {
    return NULL;
  }

  switch (cell->type) {
  case AKX_TYPE_SYMBOL:
    cloned->value.symbol = cell->value.symbol;
    break;

  case AKX_TYPE_INTEGER_LITERAL:
    cloned->value.integer_literal = cell->value.integer_literal;
    break;

  case AKX_TYPE_REAL_LITERAL:
    cloned->value.real_literal = cell->value.real_literal;
    break;

  case AKX_TYPE_STRING_LITERAL:
    cloned->value.string_literal = clone_buffer(cell->value.string_literal);
    if (!cloned->value.string_literal && cell->value.string_literal) {
      akx_cell_free(cloned);
      return NULL;
    }
    break;

  case AKX_TYPE_LIST:
  case AKX_TYPE_LIST_SQUARE:
  case AKX_TYPE_LIST_CURLY:
  case AKX_TYPE_LIST_TEMPLE:
    cloned->value.list_head = akx_cell_clone(cell->value.list_head);
    break;

  case AKX_TYPE_QUOTED:
    cloned->value.quoted_literal = clone_buffer(cell->value.quoted_literal);
    if (!cloned->value.quoted_literal && cell->value.quoted_literal) {
      akx_cell_free(cloned);
      return NULL;
    }
    break;
  }

  cloned->next = akx_cell_clone(cell->next);

  return cloned;
}
