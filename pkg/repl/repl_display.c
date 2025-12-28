#include "repl_display.h"
#include "akx_rt.h"
#include <ak24/buffer.h>
#include <stdio.h>

static void display_list(akx_cell_t *list, char open, char close);

static void display_cell_internal(akx_cell_t *cell) {
  if (!cell) {
    printf("nil");
    return;
  }

  switch (cell->type) {
  case AKX_TYPE_INTEGER_LITERAL:
    printf("%d", cell->value.integer_literal);
    break;

  case AKX_TYPE_REAL_LITERAL:
    printf("%f", cell->value.real_literal);
    break;

  case AKX_TYPE_STRING_LITERAL:
    if (cell->value.string_literal) {
      printf("\"%s\"",
             (const char *)ak_buffer_data(cell->value.string_literal));
    } else {
      printf("\"\"");
    }
    break;

  case AKX_TYPE_SYMBOL:
    printf("%s", cell->value.symbol ? cell->value.symbol : "nil");
    break;

  case AKX_TYPE_LIST:
    display_list(cell, '(', ')');
    break;

  case AKX_TYPE_LIST_SQUARE:
    display_list(cell, '[', ']');
    break;

  case AKX_TYPE_LIST_CURLY:
    display_list(cell, '{', '}');
    break;

  case AKX_TYPE_LIST_TEMPLE:
    display_list(cell, '<', '>');
    break;

  case AKX_TYPE_QUOTED:
    printf("'");
    if (cell->value.quoted_literal) {
      printf("%s", (const char *)ak_buffer_data(cell->value.quoted_literal));
    }
    break;

  case AKX_TYPE_LAMBDA:
    printf("<lambda>");
    break;

  default:
    printf("<unknown>");
    break;
  }
}

static void display_list(akx_cell_t *list, char open, char close) {
  printf("%c", open);

  akx_cell_t *current = list->value.list_head;
  int first = 1;

  while (current) {
    if (!first) {
      printf(" ");
    }
    first = 0;
    display_cell_internal(current);
    current = current->next;
  }

  printf("%c", close);
}

void repl_display_cell(akx_cell_t *cell) {
  display_cell_internal(cell);
  printf("\n");
}
