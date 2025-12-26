#include "akx_cell.h"
#include "akx_sv.h"
#include <string.h>

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
    } else if (cell->type == AKX_TYPE_LIST && cell->value.list_head) {
      akx_cell_free(cell->value.list_head);
    }

    if (cell->sourceloc) {
      AK24_FREE(cell->sourceloc);
    }

    AK24_FREE(cell);
    cell = next;
  }
}

static akx_cell_t *parse_argument(ak_scanner_t *scanner,
                                  ak_source_file_t *source_file);
static akx_cell_t *parse_explicit_list(ak_scanner_t *scanner,
                                       ak_source_file_t *source_file);
static akx_cell_t *parse_virtual_list(ak_scanner_t *scanner,
                                      ak_source_file_t *source_file);

static akx_cell_t *parse_static_type(ak_scanner_t *scanner,
                                     ak_source_file_t *source_file) {
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
                                        ak_source_file_t *source_file) {
  size_t start_pos = scanner->position;

  uint8_t escape = '\\';
  ak_scanner_find_group_result_t result =
      ak_scanner_find_group(scanner, '"', '"', &escape, false);

  if (!result.success) {
    ak_source_loc_t error_loc =
        ak_source_loc_from_offset(source_file, start_pos);
    akx_sv_show_location(&error_loc, AKX_ERROR_LEVEL_ERROR,
                         "unterminated string literal");
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

static akx_cell_t *parse_char_literal(ak_scanner_t *scanner,
                                      ak_source_file_t *source_file) {
  size_t start_pos = scanner->position;

  uint8_t escape = '\\';
  ak_scanner_find_group_result_t result =
      ak_scanner_find_group(scanner, '\'', '\'', &escape, false);

  if (!result.success) {
    ak_source_loc_t error_loc =
        ak_source_loc_from_offset(source_file, start_pos);
    akx_sv_show_location(&error_loc, AKX_ERROR_LEVEL_ERROR,
                         "unterminated character literal");
    return NULL;
  }

  ak_source_loc_t start_loc = ak_source_loc_from_offset(source_file, start_pos);
  ak_source_loc_t end_loc = ak_source_loc_from_offset(
      source_file, result.index_of_closing_symbol + 1);
  ak_source_range_t range = ak_source_range_new(start_loc, end_loc);

  akx_cell_t *cell = create_cell(AKX_TYPE_CHAR_LITERAL, &range);
  if (!cell) {
    return NULL;
  }

  size_t content_start = result.index_of_start_symbol + 1;
  size_t content_len = result.index_of_closing_symbol - content_start;

  if (content_len == 0) {
    cell->value.char_literal = '\0';
  } else if (content_len == 1) {
    uint8_t *buf_data = ak_buffer_data(scanner->buffer);
    cell->value.char_literal = buf_data[content_start];
  } else if (content_len == 2) {
    uint8_t *buf_data = ak_buffer_data(scanner->buffer);
    if (buf_data[content_start] == '\\') {
      switch (buf_data[content_start + 1]) {
      case 'n':
        cell->value.char_literal = '\n';
        break;
      case 't':
        cell->value.char_literal = '\t';
        break;
      case 'r':
        cell->value.char_literal = '\r';
        break;
      case '\\':
        cell->value.char_literal = '\\';
        break;
      case '\'':
        cell->value.char_literal = '\'';
        break;
      default:
        cell->value.char_literal = buf_data[content_start + 1];
        break;
      }
    } else {
      cell->value.char_literal = buf_data[content_start];
    }
  } else {
    uint8_t *buf_data = ak_buffer_data(scanner->buffer);
    cell->value.char_literal = buf_data[content_start];
  }

  scanner->position = result.index_of_closing_symbol + 1;

  return cell;
}

static akx_cell_t *parse_explicit_list(ak_scanner_t *scanner,
                                       ak_source_file_t *source_file) {
  size_t start_pos = scanner->position;

  ak_scanner_find_group_result_t result =
      ak_scanner_find_group(scanner, '(', ')', NULL, true);

  if (!result.success) {
    ak_source_loc_t error_loc =
        ak_source_loc_from_offset(source_file, start_pos);
    akx_sv_show_location(&error_loc, AKX_ERROR_LEVEL_ERROR,
                         "unterminated list - missing closing ')'");
    return NULL;
  }

  ak_source_loc_t start_loc = ak_source_loc_from_offset(source_file, start_pos);
  ak_source_loc_t end_loc = ak_source_loc_from_offset(
      source_file, result.index_of_closing_symbol + 1);
  ak_source_range_t range = ak_source_range_new(start_loc, end_loc);

  akx_cell_t *list_cell = create_cell(AKX_TYPE_LIST, &range);
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

          akx_cell_t *arg = parse_argument(sub_scanner, source_file);
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

static int is_newline(uint8_t c) { return c == '\n' || c == '\r'; }

static akx_cell_t *parse_virtual_list(ak_scanner_t *scanner,
                                      ak_source_file_t *source_file) {
  size_t start_pos = scanner->position;

  akx_cell_t *first_symbol = parse_static_type(scanner, source_file);
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

    akx_cell_t *arg = parse_argument(scanner, source_file);
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

static akx_cell_t *parse_argument(ak_scanner_t *scanner,
                                  ak_source_file_t *source_file) {
  if (!ak_scanner_skip_whitespace_and_comments(scanner)) {
    return NULL;
  }

  if (scanner->position >= ak_buffer_count(scanner->buffer)) {
    return NULL;
  }

  uint8_t *buf_data = ak_buffer_data(scanner->buffer);
  uint8_t current = buf_data[scanner->position];

  if (current == '(') {
    return parse_explicit_list(scanner, source_file);
  } else if (current == '"') {
    return parse_string_literal(scanner, source_file);
  } else if (current == '\'') {
    return parse_char_literal(scanner, source_file);
  } else {
    return parse_static_type(scanner, source_file);
  }
}

akx_cell_t *akx_cell_parse_buffer(ak_buffer_t *buf, const char *filename) {
  if (!buf || !filename) {
    return NULL;
  }

  ak_source_file_t *source_file = ak_source_file_new(
      filename, (const char *)ak_buffer_data(buf), ak_buffer_count(buf));
  if (!source_file) {
    AK24_LOG_ERROR("Failed to create source file for %s", filename);
    return NULL;
  }

  ak_scanner_t *scanner = ak_scanner_new(buf, 0);
  if (!scanner) {
    AK24_LOG_ERROR("Failed to create scanner");
    ak_source_file_release(source_file);
    return NULL;
  }

  akx_cell_t *head = NULL;
  akx_cell_t *tail = NULL;

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

    if (current == '(') {
      expr = parse_explicit_list(scanner, source_file);
    } else {
      expr = parse_virtual_list(scanner, source_file);
    }

    if (!expr) {
      break;
    }

    if (!head) {
      head = expr;
      tail = expr;
    } else {
      tail->next = expr;
      tail = expr;
    }

    while (tail->next) {
      tail = tail->next;
    }
  }

  ak_scanner_free(scanner);
  ak_source_file_release(source_file);

  return head;
}

akx_cell_t *akx_cell_parse_file(const char *path) {
  if (!path) {
    return NULL;
  }

  ak_buffer_t *buf = ak_buffer_from_file(path);
  if (!buf) {
    AK24_LOG_ERROR("Failed to read file: %s", path);
    return NULL;
  }

  akx_cell_t *result = akx_cell_parse_buffer(buf, path);
  ak_buffer_free(buf);

  return result;
}
