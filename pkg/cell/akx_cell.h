#ifndef AKX_CELL_H
#define AKX_CELL_H

#include <ak24/buffer.h>
#include <ak24/intern.h>
#include <ak24/kernel.h>
#include <ak24/list.h>
#include <ak24/scanner.h>
#include <ak24/sourceloc.h>

typedef enum {
  AKX_TYPE_SYMBOL,
  AKX_TYPE_STRING_LITERAL,
  AKX_TYPE_CHAR_LITERAL,
  AKX_TYPE_INTEGER_LITERAL,
  AKX_TYPE_REAL_LITERAL,
  AKX_TYPE_LIST,
  AKX_TYPE_LIST_SQUARE,
  AKX_TYPE_LIST_CURLY,
  AKX_TYPE_LIST_TEMPLE,
  AKX_TYPE_QUOTED,
} akx_type_t;

typedef struct akx_cell_t akx_cell_t;

struct akx_cell_t {
  akx_type_t type;
  akx_cell_t *next;

  union {
    const char *symbol;
    char char_literal;
    int integer_literal;
    double real_literal;
    ak_buffer_t *string_literal;
    akx_cell_t *list_head;
    ak_buffer_t *quoted_literal;
  } value;

  ak_source_range_t *sourceloc;
};

akx_cell_t *akx_cell_parse_file(const char *path);

akx_cell_t *akx_cell_parse_buffer(ak_buffer_t *buf, const char *filename);

void akx_cell_free(akx_cell_t *cell);

akx_cell_t *akx_cell_unwrap_quoted(akx_cell_t *quoted_cell);

#endif
