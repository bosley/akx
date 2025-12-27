#ifndef AKX_CELL_H
#define AKX_CELL_H

#include <ak24/buffer.h>
#include <ak24/intern.h>
#include <ak24/kernel.h>
#include <ak24/lambda.h>
#include <ak24/list.h>
#include <ak24/scanner.h>
#include <ak24/sourceloc.h>

typedef enum {
  AKX_TYPE_SYMBOL,
  AKX_TYPE_STRING_LITERAL,
  AKX_TYPE_INTEGER_LITERAL,
  AKX_TYPE_REAL_LITERAL,
  AKX_TYPE_LIST,
  AKX_TYPE_LIST_SQUARE,
  AKX_TYPE_LIST_CURLY,
  AKX_TYPE_LIST_TEMPLE,
  AKX_TYPE_QUOTED,
  AKX_TYPE_LAMBDA,
} akx_type_t;

typedef struct akx_cell_t akx_cell_t;

typedef struct akx_parse_error_t {
  ak_source_loc_t location;
  char message[256];
  struct akx_parse_error_t *next;
} akx_parse_error_t;

typedef struct {
  list_t(akx_cell_t *) cells;
  akx_parse_error_t *errors;
  ak_source_file_t *source_file;
} akx_parse_result_t;

struct akx_cell_t {
  akx_type_t type;
  akx_cell_t *next;

  union {
    const char *symbol;
    int integer_literal;
    double real_literal;
    ak_buffer_t *string_literal;
    akx_cell_t *list_head;
    ak_buffer_t *quoted_literal;
    ak_lambda_t *lambda;
  } value;

  ak_source_range_t *sourceloc;
};

akx_parse_result_t akx_cell_parse_file(const char *path);

akx_parse_result_t akx_cell_parse_buffer(ak_buffer_t *buf,
                                         const char *filename);

void akx_cell_free(akx_cell_t *cell);

void akx_parse_result_free(akx_parse_result_t *result);

akx_cell_t *akx_cell_unwrap_quoted(akx_cell_t *quoted_cell);

akx_cell_t *akx_cell_clone(akx_cell_t *cell);

#endif
