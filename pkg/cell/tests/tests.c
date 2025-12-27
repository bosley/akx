#include "tests.h"
#include "akx_cell.h"
#include "testing.h"
#include <ak24/filepath.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define TEST_FILENAME_MAX_SIZE_MAX 256

static int count_cells(akx_cell_t *cell) {
  int count = 0;
  while (cell) {
    count++;
    cell = cell->next;
  }
  return count;
}

static void assert_cell_type(akx_cell_t *cell, akx_type_t expected) {
  ASSERT_NOT_NULL(cell);
  if (cell->type != expected) {
    printf("Expected cell type %d, got %d\n", expected, cell->type);
    exit(1);
  }
}

static void assert_symbol(akx_cell_t *cell, const char *expected) {
  assert_cell_type(cell, AKX_TYPE_SYMBOL);
  ASSERT_STREQ(cell->value.symbol, expected);
}

static void assert_integer(akx_cell_t *cell, int expected) {
  assert_cell_type(cell, AKX_TYPE_INTEGER_LITERAL);
  ASSERT_EQ(cell->value.integer_literal, expected);
}

static void assert_real(akx_cell_t *cell, double expected, double epsilon) {
  assert_cell_type(cell, AKX_TYPE_REAL_LITERAL);
  ASSERT_DOUBLE_EQ(cell->value.real_literal, expected, epsilon);
}

static void assert_string(akx_cell_t *cell, const char *expected) {
  assert_cell_type(cell, AKX_TYPE_STRING_LITERAL);
  ASSERT_NOT_NULL(cell->value.string_literal);
  const char *actual = (const char *)ak_buffer_data(cell->value.string_literal);
  ASSERT_STREQ(actual, expected);
}

static void assert_list_length(akx_cell_t *list_cell, int expected) {
  ASSERT_NOT_NULL(list_cell);
  int actual = count_cells(list_cell->value.list_head);
  if (actual != expected) {
    printf("Expected list length %d, got %d\n", expected, actual);
    exit(1);
  }
}

static void validate_list_chain(akx_cell_t *cell) {
  akx_cell_t *slow = cell;
  akx_cell_t *fast = cell;

  while (fast && fast->next) {
    slow = slow->next;
    fast = fast->next->next;

    if (slow == fast && fast != NULL) {
      printf("Cycle detected in list chain!\n");
      exit(1);
    }
  }
}

static akx_cell_t *parse_string_as_file(const char *content,
                                        const char *test_name) {
  ak_buffer_t *temp_dir = ak_filepath_temp();
  if (!temp_dir) {
    return NULL;
  }

  char filename[TEST_FILENAME_MAX_SIZE_MAX];
  snprintf(filename, sizeof(filename), "akx_test_%s_%d.akx", test_name,
           (int)getpid());

  ak_buffer_t *temp_path =
      ak_filepath_join(2, (const char *)ak_buffer_data(temp_dir), filename);
  ak_buffer_free(temp_dir);

  if (!temp_path) {
    return NULL;
  }

  const char *path_str = (const char *)ak_buffer_data(temp_path);

  FILE *f = fopen(path_str, "w");
  if (!f) {
    ak_buffer_free(temp_path);
    return NULL;
  }

  size_t written = fwrite(content, 1, strlen(content), f);
  fflush(f);
  fclose(f);

  if (written != strlen(content)) {
    remove(path_str);
    ak_buffer_free(temp_path);
    return NULL;
  }

  akx_parse_result_t result = akx_cell_parse_file(path_str);

  remove(path_str);
  ak_buffer_free(temp_path);

  akx_cell_t *head = NULL;
  akx_cell_t *tail = NULL;

  for (size_t i = 0; i < list_count(&result.cells); i++) {
    akx_cell_t *cell = *((akx_cell_t **)list_get(&result.cells, i));
    if (!head) {
      head = cell;
      tail = cell;
    } else {
      tail->next = cell;
      tail = cell;
    }
  }

  list_deinit(&result.cells);
  if (result.source_file) {
    ak_source_file_release(result.source_file);
  }

  return head;
}

static void test_integer_literals(void) {
  printf("  test_integer_literals...\n");

  akx_cell_t *cells = parse_string_as_file("(42)", "int_42");
  ASSERT_NOT_NULL(cells);
  assert_cell_type(cells, AKX_TYPE_LIST);
  assert_list_length(cells, 1);
  assert_integer(cells->value.list_head, 42);
  akx_cell_free(cells);

  cells = parse_string_as_file("(-10)", "int_neg10");
  ASSERT_NOT_NULL(cells);
  assert_integer(cells->value.list_head, -10);
  akx_cell_free(cells);

  cells = parse_string_as_file("(0)", "int_zero");
  ASSERT_NOT_NULL(cells);
  assert_integer(cells->value.list_head, 0);
  akx_cell_free(cells);
}

static void test_real_literals(void) {
  printf("  test_real_literals...\n");

  akx_cell_t *cells = parse_string_as_file("(3.14)", "real_pi");
  ASSERT_NOT_NULL(cells);
  assert_cell_type(cells, AKX_TYPE_LIST);
  assert_list_length(cells, 1);
  assert_real(cells->value.list_head, 3.14, 0.001);
  akx_cell_free(cells);

  cells = parse_string_as_file("(-2.5)", "real_neg");
  ASSERT_NOT_NULL(cells);
  assert_real(cells->value.list_head, -2.5, 0.001);
  akx_cell_free(cells);

  cells = parse_string_as_file("(0.0)", "real_zero");
  ASSERT_NOT_NULL(cells);
  assert_real(cells->value.list_head, 0.0, 0.001);
  akx_cell_free(cells);
}

static void test_string_literals(void) {
  printf("  test_string_literals...\n");

  akx_cell_t *cells = parse_string_as_file("(\"hello\")", "str_hello");
  ASSERT_NOT_NULL(cells);
  assert_cell_type(cells, AKX_TYPE_LIST);
  assert_list_length(cells, 1);
  assert_string(cells->value.list_head, "hello");
  akx_cell_free(cells);

  cells = parse_string_as_file("(\"with spaces\")", "str_spaces");
  ASSERT_NOT_NULL(cells);
  assert_string(cells->value.list_head, "with spaces");
  akx_cell_free(cells);

  cells = parse_string_as_file("(\"\")", "str_empty");
  ASSERT_NOT_NULL(cells);
  assert_string(cells->value.list_head, "");
  akx_cell_free(cells);
}

static void test_string_with_single_char(void) {
  printf("  test_string_with_single_char...\n");

  akx_cell_t *cells = parse_string_as_file("(\"a\")", "str_single");
  ASSERT_NOT_NULL(cells);
  assert_cell_type(cells, AKX_TYPE_LIST);
  assert_list_length(cells, 1);
  assert_string(cells->value.list_head, "a");
  akx_cell_free(cells);

  cells = parse_string_as_file("(\"x\")", "str_x");
  ASSERT_NOT_NULL(cells);
  assert_string(cells->value.list_head, "x");
  akx_cell_free(cells);

  cells = parse_string_as_file("(\"123\")", "str_num");
  ASSERT_NOT_NULL(cells);
  assert_string(cells->value.list_head, "123");
  akx_cell_free(cells);
}

static void test_symbols(void) {
  printf("  test_symbols...\n");

  akx_cell_t *cells = parse_string_as_file("foo", "sym_foo");
  ASSERT_NOT_NULL(cells);
  assert_cell_type(cells, AKX_TYPE_LIST);
  assert_list_length(cells, 1);
  assert_symbol(cells->value.list_head, "foo");
  akx_cell_free(cells);

  cells = parse_string_as_file("bar-baz", "sym_dash");
  ASSERT_NOT_NULL(cells);
  assert_symbol(cells->value.list_head, "bar-baz");
  akx_cell_free(cells);

  cells = parse_string_as_file("+", "sym_plus");
  ASSERT_NOT_NULL(cells);
  assert_symbol(cells->value.list_head, "+");
  akx_cell_free(cells);
}

static void test_simple_virtual_list(void) {
  printf("  test_simple_virtual_list...\n");

  akx_cell_t *cells = parse_string_as_file("put \"hello\"", "vlist_simple");
  ASSERT_NOT_NULL(cells);
  assert_cell_type(cells, AKX_TYPE_LIST);
  assert_list_length(cells, 2);

  akx_cell_t *first = cells->value.list_head;
  assert_symbol(first, "put");

  akx_cell_t *second = first->next;
  assert_string(second, "hello");

  akx_cell_free(cells);
}

static void test_multi_arg_virtual_list(void) {
  printf("  test_multi_arg_virtual_list...\n");

  akx_cell_t *cells = parse_string_as_file("add 1 2 3", "vlist_multi");
  ASSERT_NOT_NULL(cells);
  assert_cell_type(cells, AKX_TYPE_LIST);
  assert_list_length(cells, 4);

  akx_cell_t *cmd = cells->value.list_head;
  assert_symbol(cmd, "add");

  assert_integer(cmd->next, 1);
  assert_integer(cmd->next->next, 2);
  assert_integer(cmd->next->next->next, 3);

  akx_cell_free(cells);
}

static void test_virtual_list_newline_termination(void) {
  printf("  test_virtual_list_newline_termination...\n");

  akx_cell_t *cells = parse_string_as_file("add 1 2\nsub 3 4", "vlist_newline");
  ASSERT_NOT_NULL(cells);

  ASSERT_EQ(count_cells(cells), 2);

  assert_cell_type(cells, AKX_TYPE_LIST);
  assert_list_length(cells, 3);
  assert_symbol(cells->value.list_head, "add");

  akx_cell_t *second_expr = cells->next;
  assert_cell_type(second_expr, AKX_TYPE_LIST);
  assert_list_length(second_expr, 3);
  assert_symbol(second_expr->value.list_head, "sub");

  akx_cell_free(cells);
}

static void test_paren_list(void) {
  printf("  test_paren_list...\n");

  akx_cell_t *cells = parse_string_as_file("(a b c)", "paren_list");
  ASSERT_NOT_NULL(cells);
  assert_cell_type(cells, AKX_TYPE_LIST);
  assert_list_length(cells, 3);

  akx_cell_t *first = cells->value.list_head;
  assert_symbol(first, "a");
  assert_symbol(first->next, "b");
  assert_symbol(first->next->next, "c");

  akx_cell_free(cells);
}

static void test_square_list(void) {
  printf("  test_square_list...\n");

  akx_cell_t *cells = parse_string_as_file("[1 2 3]", "square_list");
  ASSERT_NOT_NULL(cells);
  assert_cell_type(cells, AKX_TYPE_LIST_SQUARE);
  assert_list_length(cells, 3);

  akx_cell_t *first = cells->value.list_head;
  assert_integer(first, 1);
  assert_integer(first->next, 2);
  assert_integer(first->next->next, 3);

  akx_cell_free(cells);
}

static void test_curly_list(void) {
  printf("  test_curly_list...\n");

  akx_cell_t *cells = parse_string_as_file("{x y z}", "curly_list");
  ASSERT_NOT_NULL(cells);
  assert_cell_type(cells, AKX_TYPE_LIST_CURLY);
  assert_list_length(cells, 3);

  akx_cell_t *first = cells->value.list_head;
  assert_symbol(first, "x");
  assert_symbol(first->next, "y");
  assert_symbol(first->next->next, "z");

  akx_cell_free(cells);
}

static void test_temple_list(void) {
  printf("  test_temple_list...\n");

  akx_cell_t *cells = parse_string_as_file("<foo bar>", "temple_list");
  ASSERT_NOT_NULL(cells);
  assert_cell_type(cells, AKX_TYPE_LIST_TEMPLE);
  assert_list_length(cells, 2);

  akx_cell_t *first = cells->value.list_head;
  assert_symbol(first, "foo");
  assert_symbol(first->next, "bar");

  akx_cell_free(cells);
}

static void test_empty_lists(void) {
  printf("  test_empty_lists...\n");

  akx_cell_t *cells = parse_string_as_file("()", "empty_paren");
  ASSERT_NOT_NULL(cells);
  assert_cell_type(cells, AKX_TYPE_LIST);
  assert_list_length(cells, 0);
  akx_cell_free(cells);

  cells = parse_string_as_file("[]", "empty_square");
  ASSERT_NOT_NULL(cells);
  assert_cell_type(cells, AKX_TYPE_LIST_SQUARE);
  assert_list_length(cells, 0);
  akx_cell_free(cells);

  cells = parse_string_as_file("{}", "empty_curly");
  ASSERT_NOT_NULL(cells);
  assert_cell_type(cells, AKX_TYPE_LIST_CURLY);
  assert_list_length(cells, 0);
  akx_cell_free(cells);

  cells = parse_string_as_file("<>", "empty_temple");
  ASSERT_NOT_NULL(cells);
  assert_cell_type(cells, AKX_TYPE_LIST_TEMPLE);
  assert_list_length(cells, 0);
  akx_cell_free(cells);
}

static void test_nested_delimiters(void) {
  printf("  test_nested_delimiters...\n");

  akx_cell_t *cells =
      parse_string_as_file("(outer [inner {deep <temple>}])", "nested");
  ASSERT_NOT_NULL(cells);
  assert_cell_type(cells, AKX_TYPE_LIST);
  assert_list_length(cells, 2);

  akx_cell_t *outer_sym = cells->value.list_head;
  assert_symbol(outer_sym, "outer");

  akx_cell_t *square = outer_sym->next;
  assert_cell_type(square, AKX_TYPE_LIST_SQUARE);
  assert_list_length(square, 2);

  akx_cell_t *inner_sym = square->value.list_head;
  assert_symbol(inner_sym, "inner");

  akx_cell_t *curly = inner_sym->next;
  assert_cell_type(curly, AKX_TYPE_LIST_CURLY);
  assert_list_length(curly, 2);

  akx_cell_t *deep_sym = curly->value.list_head;
  assert_symbol(deep_sym, "deep");

  akx_cell_t *temple = deep_sym->next;
  assert_cell_type(temple, AKX_TYPE_LIST_TEMPLE);
  assert_list_length(temple, 1);
  assert_symbol(temple->value.list_head, "temple");

  akx_cell_free(cells);
}

static void test_virtual_list_with_explicit(void) {
  printf("  test_virtual_list_with_explicit...\n");

  akx_cell_t *cells =
      parse_string_as_file("put (strcat \"a\" \"b\") \"c\"", "vlist_explicit");
  ASSERT_NOT_NULL(cells);
  assert_cell_type(cells, AKX_TYPE_LIST);
  assert_list_length(cells, 3);

  akx_cell_t *put = cells->value.list_head;
  assert_symbol(put, "put");

  akx_cell_t *strcat_list = put->next;
  assert_cell_type(strcat_list, AKX_TYPE_LIST);
  assert_list_length(strcat_list, 3);
  assert_symbol(strcat_list->value.list_head, "strcat");

  akx_cell_t *c_str = put->next->next;
  assert_string(c_str, "c");

  akx_cell_free(cells);
}

static void test_quoted_symbol(void) {
  printf("  test_quoted_symbol...\n");

  akx_cell_t *cells = parse_string_as_file("'x", "quote_sym");
  ASSERT_NOT_NULL(cells);
  assert_cell_type(cells, AKX_TYPE_QUOTED);
  ASSERT_NOT_NULL(cells->value.quoted_literal);

  const char *quoted =
      (const char *)ak_buffer_data(cells->value.quoted_literal);
  ASSERT_STREQ(quoted, "x");

  akx_cell_free(cells);
}

static void test_quoted_list(void) {
  printf("  test_quoted_list...\n");

  akx_cell_t *cells = parse_string_as_file("'(a b c)", "quote_list");
  ASSERT_NOT_NULL(cells);
  assert_cell_type(cells, AKX_TYPE_QUOTED);
  ASSERT_NOT_NULL(cells->value.quoted_literal);

  const char *quoted =
      (const char *)ak_buffer_data(cells->value.quoted_literal);
  ASSERT_STREQ(quoted, "(a b c)");

  akx_cell_free(cells);
}

static void test_quoted_virtual_list(void) {
  printf("  test_quoted_virtual_list...\n");

  akx_cell_t *cells =
      parse_string_as_file("'put \"hello\" 1 2 3", "quote_vlist");
  ASSERT_NOT_NULL(cells);
  assert_cell_type(cells, AKX_TYPE_QUOTED);
  ASSERT_NOT_NULL(cells->value.quoted_literal);

  akx_cell_free(cells);
}

static void test_unwrap_quoted(void) {
  printf("  test_unwrap_quoted...\n");

  akx_cell_t *cells = parse_string_as_file("'(a b c)", "unwrap_test");
  ASSERT_NOT_NULL(cells);
  assert_cell_type(cells, AKX_TYPE_QUOTED);

  akx_cell_t *unwrapped = akx_cell_unwrap_quoted(cells);
  ASSERT_NOT_NULL(unwrapped);
  assert_cell_type(unwrapped, AKX_TYPE_LIST);
  assert_list_length(unwrapped, 3);
  assert_symbol(unwrapped->value.list_head, "a");

  akx_cell_free(unwrapped);
  akx_cell_free(cells);
}

static void test_multiple_expressions(void) {
  printf("  test_multiple_expressions...\n");

  akx_cell_t *cells =
      parse_string_as_file("add 1 2\n(mul 3 4)\nput \"hello\"", "multi_expr");
  ASSERT_NOT_NULL(cells);

  ASSERT_EQ(count_cells(cells), 3);

  assert_cell_type(cells, AKX_TYPE_LIST);
  assert_symbol(cells->value.list_head, "add");

  akx_cell_t *second = cells->next;
  assert_cell_type(second, AKX_TYPE_LIST);
  assert_symbol(second->value.list_head, "mul");

  akx_cell_t *third = second->next;
  assert_cell_type(third, AKX_TYPE_LIST);
  assert_symbol(third->value.list_head, "put");
  assert_string(third->value.list_head->next, "hello");

  akx_cell_free(cells);
}

static void test_deeply_nested_parens(void) {
  printf("  test_deeply_nested_parens...\n");

  akx_cell_t *cells =
      parse_string_as_file("(a (b (c (d (e)))))", "deep_parens");
  ASSERT_NOT_NULL(cells);
  assert_cell_type(cells, AKX_TYPE_LIST);
  assert_list_length(cells, 2);

  akx_cell_t *level1 = cells->value.list_head;
  assert_symbol(level1, "a");

  akx_cell_t *level2 = level1->next;
  assert_cell_type(level2, AKX_TYPE_LIST);
  assert_list_length(level2, 2);
  assert_symbol(level2->value.list_head, "b");

  akx_cell_t *level3 = level2->value.list_head->next;
  assert_cell_type(level3, AKX_TYPE_LIST);
  assert_list_length(level3, 2);
  assert_symbol(level3->value.list_head, "c");

  akx_cell_t *level4 = level3->value.list_head->next;
  assert_cell_type(level4, AKX_TYPE_LIST);
  assert_list_length(level4, 2);
  assert_symbol(level4->value.list_head, "d");

  akx_cell_t *level5 = level4->value.list_head->next;
  assert_cell_type(level5, AKX_TYPE_LIST);
  assert_list_length(level5, 1);
  assert_symbol(level5->value.list_head, "e");

  akx_cell_free(cells);
}

static void test_deeply_nested_mixed(void) {
  printf("  test_deeply_nested_mixed...\n");

  akx_cell_t *cells = parse_string_as_file("(a [b {c <d (e)>}])", "deep_mixed");
  ASSERT_NOT_NULL(cells);
  assert_cell_type(cells, AKX_TYPE_LIST);
  assert_list_length(cells, 2);

  akx_cell_t *a = cells->value.list_head;
  assert_symbol(a, "a");

  akx_cell_t *square = a->next;
  assert_cell_type(square, AKX_TYPE_LIST_SQUARE);
  assert_list_length(square, 2);
  assert_symbol(square->value.list_head, "b");

  akx_cell_t *curly = square->value.list_head->next;
  assert_cell_type(curly, AKX_TYPE_LIST_CURLY);
  assert_list_length(curly, 2);
  assert_symbol(curly->value.list_head, "c");

  akx_cell_t *temple = curly->value.list_head->next;
  assert_cell_type(temple, AKX_TYPE_LIST_TEMPLE);
  assert_list_length(temple, 2);
  assert_symbol(temple->value.list_head, "d");

  akx_cell_t *inner_paren = temple->value.list_head->next;
  assert_cell_type(inner_paren, AKX_TYPE_LIST);
  assert_list_length(inner_paren, 1);
  assert_symbol(inner_paren->value.list_head, "e");

  akx_cell_free(cells);
}

static void test_nested_with_data_at_each_level(void) {
  printf("  test_nested_with_data_at_each_level...\n");

  akx_cell_t *cells = parse_string_as_file("(1 (2 (3 (4 5))))", "data_levels");
  ASSERT_NOT_NULL(cells);
  assert_cell_type(cells, AKX_TYPE_LIST);
  assert_list_length(cells, 2);

  assert_integer(cells->value.list_head, 1);

  akx_cell_t *level2 = cells->value.list_head->next;
  assert_cell_type(level2, AKX_TYPE_LIST);
  assert_list_length(level2, 2);
  assert_integer(level2->value.list_head, 2);

  akx_cell_t *level3 = level2->value.list_head->next;
  assert_cell_type(level3, AKX_TYPE_LIST);
  assert_list_length(level3, 2);
  assert_integer(level3->value.list_head, 3);

  akx_cell_t *level4 = level3->value.list_head->next;
  assert_cell_type(level4, AKX_TYPE_LIST);
  assert_list_length(level4, 2);
  assert_integer(level4->value.list_head, 4);
  assert_integer(level4->value.list_head->next, 5);

  akx_cell_free(cells);
}

static void test_nested_integer_preservation(void) {
  printf("  test_nested_integer_preservation...\n");

  akx_cell_t *cells =
      parse_string_as_file("(outer (mid (inner 42)))", "int_preserve");
  ASSERT_NOT_NULL(cells);
  assert_cell_type(cells, AKX_TYPE_LIST);

  akx_cell_t *outer = cells->value.list_head;
  assert_symbol(outer, "outer");

  akx_cell_t *mid_list = outer->next;
  assert_cell_type(mid_list, AKX_TYPE_LIST);
  assert_symbol(mid_list->value.list_head, "mid");

  akx_cell_t *inner_list = mid_list->value.list_head->next;
  assert_cell_type(inner_list, AKX_TYPE_LIST);
  assert_symbol(inner_list->value.list_head, "inner");
  assert_integer(inner_list->value.list_head->next, 42);

  akx_cell_free(cells);
}

static void test_nested_string_preservation(void) {
  printf("  test_nested_string_preservation...\n");

  akx_cell_t *cells =
      parse_string_as_file("(a (b (c \"deep string\")))", "str_preserve");
  ASSERT_NOT_NULL(cells);
  assert_cell_type(cells, AKX_TYPE_LIST);

  akx_cell_t *level1 = cells->value.list_head;
  assert_symbol(level1, "a");

  akx_cell_t *level2 = level1->next;
  assert_cell_type(level2, AKX_TYPE_LIST);
  assert_symbol(level2->value.list_head, "b");

  akx_cell_t *level3 = level2->value.list_head->next;
  assert_cell_type(level3, AKX_TYPE_LIST);
  assert_symbol(level3->value.list_head, "c");
  assert_string(level3->value.list_head->next, "deep string");

  akx_cell_free(cells);
}

static void test_nested_real_precision(void) {
  printf("  test_nested_real_precision...\n");

  akx_cell_t *cells =
      parse_string_as_file("(calc [math {value 3.14159}])", "real_precision");
  ASSERT_NOT_NULL(cells);
  assert_cell_type(cells, AKX_TYPE_LIST);

  assert_symbol(cells->value.list_head, "calc");

  akx_cell_t *square = cells->value.list_head->next;
  assert_cell_type(square, AKX_TYPE_LIST_SQUARE);
  assert_symbol(square->value.list_head, "math");

  akx_cell_t *curly = square->value.list_head->next;
  assert_cell_type(curly, AKX_TYPE_LIST_CURLY);
  assert_symbol(curly->value.list_head, "value");
  assert_real(curly->value.list_head->next, 3.14159, 0.00001);

  akx_cell_free(cells);
}

static void test_mixed_types_nested(void) {
  printf("  test_mixed_types_nested...\n");

  akx_cell_t *cells =
      parse_string_as_file("(func [1 \"str\" 3.14 sym])", "mixed_types");
  ASSERT_NOT_NULL(cells);
  assert_cell_type(cells, AKX_TYPE_LIST);
  assert_list_length(cells, 2);

  assert_symbol(cells->value.list_head, "func");

  akx_cell_t *square = cells->value.list_head->next;
  assert_cell_type(square, AKX_TYPE_LIST_SQUARE);
  assert_list_length(square, 4);

  akx_cell_t *elem = square->value.list_head;
  assert_integer(elem, 1);

  elem = elem->next;
  assert_string(elem, "str");

  elem = elem->next;
  assert_real(elem, 3.14, 0.001);

  elem = elem->next;
  assert_symbol(elem, "sym");

  akx_cell_free(cells);
}

static void test_virtual_with_deeply_nested_explicit(void) {
  printf("  test_virtual_with_deeply_nested_explicit...\n");

  akx_cell_t *cells =
      parse_string_as_file("cmd (a (b (c))) arg", "virt_deep_expl");
  ASSERT_NOT_NULL(cells);
  assert_cell_type(cells, AKX_TYPE_LIST);
  assert_list_length(cells, 3);

  assert_symbol(cells->value.list_head, "cmd");

  akx_cell_t *nested = cells->value.list_head->next;
  assert_cell_type(nested, AKX_TYPE_LIST);
  assert_symbol(nested->value.list_head, "a");

  akx_cell_t *level2 = nested->value.list_head->next;
  assert_cell_type(level2, AKX_TYPE_LIST);
  assert_symbol(level2->value.list_head, "b");

  akx_cell_t *level3 = level2->value.list_head->next;
  assert_cell_type(level3, AKX_TYPE_LIST);
  assert_symbol(level3->value.list_head, "c");

  akx_cell_t *arg = nested->next;
  assert_symbol(arg, "arg");

  akx_cell_free(cells);
}

static void test_nested_virtual_in_explicit(void) {
  printf("  test_nested_virtual_in_explicit...\n");

  akx_cell_t *cells =
      parse_string_as_file("(outer inner 1 2 another 3 4)", "virt_in_expl");
  ASSERT_NOT_NULL(cells);
  assert_cell_type(cells, AKX_TYPE_LIST);
  assert_list_length(cells, 7);

  akx_cell_t *elem = cells->value.list_head;
  assert_symbol(elem, "outer");

  elem = elem->next;
  assert_symbol(elem, "inner");

  elem = elem->next;
  assert_integer(elem, 1);

  elem = elem->next;
  assert_integer(elem, 2);

  elem = elem->next;
  assert_symbol(elem, "another");

  elem = elem->next;
  assert_integer(elem, 3);

  elem = elem->next;
  assert_integer(elem, 4);

  akx_cell_free(cells);
}

static void test_multiple_nested_in_virtual(void) {
  printf("  test_multiple_nested_in_virtual...\n");

  akx_cell_t *cells =
      parse_string_as_file("cmd [a] {b} (c)", "multi_nest_virt");
  ASSERT_NOT_NULL(cells);
  assert_cell_type(cells, AKX_TYPE_LIST);
  assert_list_length(cells, 4);

  akx_cell_t *cmd = cells->value.list_head;
  assert_symbol(cmd, "cmd");

  akx_cell_t *square = cmd->next;
  assert_cell_type(square, AKX_TYPE_LIST_SQUARE);
  assert_list_length(square, 1);
  assert_symbol(square->value.list_head, "a");

  akx_cell_t *curly = square->next;
  assert_cell_type(curly, AKX_TYPE_LIST_CURLY);
  assert_list_length(curly, 1);
  assert_symbol(curly->value.list_head, "b");

  akx_cell_t *paren = curly->next;
  assert_cell_type(paren, AKX_TYPE_LIST);
  assert_list_length(paren, 1);
  assert_symbol(paren->value.list_head, "c");

  akx_cell_free(cells);
}

static void test_quoted_nested_list(void) {
  printf("  test_quoted_nested_list...\n");

  akx_cell_t *cells = parse_string_as_file("'(a (b (c)))", "quote_nested");
  ASSERT_NOT_NULL(cells);
  assert_cell_type(cells, AKX_TYPE_QUOTED);
  ASSERT_NOT_NULL(cells->value.quoted_literal);

  const char *quoted =
      (const char *)ak_buffer_data(cells->value.quoted_literal);
  ASSERT_STREQ(quoted, "(a (b (c)))");

  akx_cell_free(cells);
}

static void test_unwrap_quoted_nested(void) {
  printf("  test_unwrap_quoted_nested...\n");

  akx_cell_t *cells =
      parse_string_as_file("'(outer [inner {deep}])", "unwrap_nested");
  ASSERT_NOT_NULL(cells);
  assert_cell_type(cells, AKX_TYPE_QUOTED);

  akx_cell_t *unwrapped = akx_cell_unwrap_quoted(cells);
  ASSERT_NOT_NULL(unwrapped);
  assert_cell_type(unwrapped, AKX_TYPE_LIST);
  assert_list_length(unwrapped, 2);

  assert_symbol(unwrapped->value.list_head, "outer");

  akx_cell_t *square = unwrapped->value.list_head->next;
  assert_cell_type(square, AKX_TYPE_LIST_SQUARE);
  assert_list_length(square, 2);
  assert_symbol(square->value.list_head, "inner");

  akx_cell_t *curly = square->value.list_head->next;
  assert_cell_type(curly, AKX_TYPE_LIST_CURLY);
  assert_list_length(curly, 1);
  assert_symbol(curly->value.list_head, "deep");

  akx_cell_free(unwrapped);
  akx_cell_free(cells);
}

static void test_quoted_mixed_delimiters(void) {
  printf("  test_quoted_mixed_delimiters...\n");

  akx_cell_t *cells = parse_string_as_file("'[a {b <c (d)>}]", "quote_mixed");
  ASSERT_NOT_NULL(cells);
  assert_cell_type(cells, AKX_TYPE_QUOTED);
  ASSERT_NOT_NULL(cells->value.quoted_literal);

  const char *quoted =
      (const char *)ak_buffer_data(cells->value.quoted_literal);
  ASSERT_STREQ(quoted, "[a {b <c (d)>}]");

  akx_cell_t *unwrapped = akx_cell_unwrap_quoted(cells);
  ASSERT_NOT_NULL(unwrapped);
  assert_cell_type(unwrapped, AKX_TYPE_LIST_SQUARE);

  akx_cell_free(unwrapped);
  akx_cell_free(cells);
}

static void test_multiple_quote_levels(void) {
  printf("  test_multiple_quote_levels...\n");

  akx_cell_t *cells = parse_string_as_file("'''a", "triple_quote");
  ASSERT_NOT_NULL(cells);
  assert_cell_type(cells, AKX_TYPE_QUOTED);
  ASSERT_NOT_NULL(cells->value.quoted_literal);

  const char *quoted =
      (const char *)ak_buffer_data(cells->value.quoted_literal);
  ASSERT_STREQ(quoted, "''a");

  akx_cell_t *unwrapped1 = akx_cell_unwrap_quoted(cells);
  ASSERT_NOT_NULL(unwrapped1);
  assert_cell_type(unwrapped1, AKX_TYPE_QUOTED);

  const char *quoted2 =
      (const char *)ak_buffer_data(unwrapped1->value.quoted_literal);
  ASSERT_STREQ(quoted2, "'a");

  akx_cell_t *unwrapped2 = akx_cell_unwrap_quoted(unwrapped1);
  ASSERT_NOT_NULL(unwrapped2);
  assert_cell_type(unwrapped2, AKX_TYPE_QUOTED);

  const char *quoted3 =
      (const char *)ak_buffer_data(unwrapped2->value.quoted_literal);
  ASSERT_STREQ(quoted3, "a");

  akx_cell_t *unwrapped3 = akx_cell_unwrap_quoted(unwrapped2);
  ASSERT_NOT_NULL(unwrapped3);
  assert_cell_type(unwrapped3, AKX_TYPE_LIST);
  assert_symbol(unwrapped3->value.list_head, "a");

  akx_cell_free(unwrapped3);
  akx_cell_free(unwrapped2);
  akx_cell_free(unwrapped1);
  akx_cell_free(cells);
}

static void test_quote_in_explicit_list(void) {
  printf("  test_quote_in_explicit_list...\n");

  akx_cell_t *cells = parse_string_as_file("('a)", "quote_in_paren");
  ASSERT_NOT_NULL(cells);
  assert_cell_type(cells, AKX_TYPE_LIST);
  assert_list_length(cells, 1);

  akx_cell_t *quoted = cells->value.list_head;
  assert_cell_type(quoted, AKX_TYPE_QUOTED);
  ASSERT_NOT_NULL(quoted->value.quoted_literal);

  const char *quoted_str =
      (const char *)ak_buffer_data(quoted->value.quoted_literal);
  ASSERT_STREQ(quoted_str, "a");

  akx_cell_free(cells);
}

static void test_multiple_quotes_in_list(void) {
  printf("  test_multiple_quotes_in_list...\n");

  akx_cell_t *cells = parse_string_as_file("'(a 'b 'c)", "multi_quote_list");
  ASSERT_NOT_NULL(cells);
  assert_cell_type(cells, AKX_TYPE_QUOTED);

  akx_cell_t *unwrapped = akx_cell_unwrap_quoted(cells);
  ASSERT_NOT_NULL(unwrapped);
  assert_cell_type(unwrapped, AKX_TYPE_LIST);
  assert_list_length(unwrapped, 3);

  akx_cell_t *first = unwrapped->value.list_head;
  assert_symbol(first, "a");

  akx_cell_t *second = first->next;
  assert_cell_type(second, AKX_TYPE_QUOTED);
  const char *quoted_b =
      (const char *)ak_buffer_data(second->value.quoted_literal);
  ASSERT_STREQ(quoted_b, "b");

  akx_cell_t *third = second->next;
  assert_cell_type(third, AKX_TYPE_QUOTED);
  const char *quoted_c =
      (const char *)ak_buffer_data(third->value.quoted_literal);
  ASSERT_STREQ(quoted_c, "c");

  akx_cell_free(unwrapped);
  akx_cell_free(cells);
}

static void test_deeply_nested_quotes(void) {
  printf("  test_deeply_nested_quotes...\n");

  akx_cell_t *cells = parse_string_as_file("'''(''moot)", "deep_quote_nest");
  ASSERT_NOT_NULL(cells);
  assert_cell_type(cells, AKX_TYPE_QUOTED);

  akx_cell_t *unwrapped1 = akx_cell_unwrap_quoted(cells);
  ASSERT_NOT_NULL(unwrapped1);
  assert_cell_type(unwrapped1, AKX_TYPE_QUOTED);

  akx_cell_t *unwrapped2 = akx_cell_unwrap_quoted(unwrapped1);
  ASSERT_NOT_NULL(unwrapped2);
  assert_cell_type(unwrapped2, AKX_TYPE_QUOTED);

  akx_cell_t *unwrapped3 = akx_cell_unwrap_quoted(unwrapped2);
  ASSERT_NOT_NULL(unwrapped3);
  assert_cell_type(unwrapped3, AKX_TYPE_LIST);
  assert_list_length(unwrapped3, 1);

  akx_cell_t *inner_quoted = unwrapped3->value.list_head;
  assert_cell_type(inner_quoted, AKX_TYPE_QUOTED);

  akx_cell_t *unwrapped4 = akx_cell_unwrap_quoted(inner_quoted);
  ASSERT_NOT_NULL(unwrapped4);
  assert_cell_type(unwrapped4, AKX_TYPE_QUOTED);

  akx_cell_t *unwrapped5 = akx_cell_unwrap_quoted(unwrapped4);
  ASSERT_NOT_NULL(unwrapped5);
  assert_cell_type(unwrapped5, AKX_TYPE_LIST);
  assert_symbol(unwrapped5->value.list_head, "moot");

  akx_cell_free(unwrapped5);
  akx_cell_free(unwrapped4);
  akx_cell_free(unwrapped3);
  akx_cell_free(unwrapped2);
  akx_cell_free(unwrapped1);
  akx_cell_free(cells);
}

static void test_adjacent_empty_lists(void) {
  printf("  test_adjacent_empty_lists...\n");

  akx_cell_t *cells = parse_string_as_file("()[]{}<>", "adjacent_empty");
  ASSERT_NOT_NULL(cells);

  ASSERT_EQ(count_cells(cells), 4);

  assert_cell_type(cells, AKX_TYPE_LIST);
  assert_list_length(cells, 0);

  akx_cell_t *second = cells->next;
  assert_cell_type(second, AKX_TYPE_LIST_SQUARE);
  assert_list_length(second, 0);

  akx_cell_t *third = second->next;
  assert_cell_type(third, AKX_TYPE_LIST_CURLY);
  assert_list_length(third, 0);

  akx_cell_t *fourth = third->next;
  assert_cell_type(fourth, AKX_TYPE_LIST_TEMPLE);
  assert_list_length(fourth, 0);

  akx_cell_free(cells);
}

static void test_adjacent_nested_lists(void) {
  printf("  test_adjacent_nested_lists...\n");

  akx_cell_t *cells = parse_string_as_file("(a)(b)(c)", "adjacent_nested");
  ASSERT_NOT_NULL(cells);

  ASSERT_EQ(count_cells(cells), 3);

  assert_cell_type(cells, AKX_TYPE_LIST);
  assert_list_length(cells, 1);
  assert_symbol(cells->value.list_head, "a");

  akx_cell_t *second = cells->next;
  assert_cell_type(second, AKX_TYPE_LIST);
  assert_list_length(second, 1);
  assert_symbol(second->value.list_head, "b");

  akx_cell_t *third = second->next;
  assert_cell_type(third, AKX_TYPE_LIST);
  assert_list_length(third, 1);
  assert_symbol(third->value.list_head, "c");

  akx_cell_free(cells);
}

static void test_deeply_nested_empty(void) {
  printf("  test_deeply_nested_empty...\n");

  akx_cell_t *cells = parse_string_as_file("(((())))", "deep_empty");
  ASSERT_NOT_NULL(cells);
  assert_cell_type(cells, AKX_TYPE_LIST);
  assert_list_length(cells, 1);

  akx_cell_t *level2 = cells->value.list_head;
  assert_cell_type(level2, AKX_TYPE_LIST);
  assert_list_length(level2, 1);

  akx_cell_t *level3 = level2->value.list_head;
  assert_cell_type(level3, AKX_TYPE_LIST);
  assert_list_length(level3, 1);

  akx_cell_t *level4 = level3->value.list_head;
  assert_cell_type(level4, AKX_TYPE_LIST);
  assert_list_length(level4, 0);

  akx_cell_free(cells);
}

static void test_whitespace_in_deep_nesting(void) {
  printf("  test_whitespace_in_deep_nesting...\n");

  akx_cell_t *cells = parse_string_as_file(
      "(\n  a\n  [\n    b\n    {\n      c\n    }\n  ]\n)", "ws_deep");
  ASSERT_NOT_NULL(cells);
  assert_cell_type(cells, AKX_TYPE_LIST);
  assert_list_length(cells, 2);

  assert_symbol(cells->value.list_head, "a");

  akx_cell_t *square = cells->value.list_head->next;
  assert_cell_type(square, AKX_TYPE_LIST_SQUARE);
  assert_list_length(square, 2);
  assert_symbol(square->value.list_head, "b");

  akx_cell_t *curly = square->value.list_head->next;
  assert_cell_type(curly, AKX_TYPE_LIST_CURLY);
  assert_list_length(curly, 1);
  assert_symbol(curly->value.list_head, "c");

  akx_cell_free(cells);
}

static void test_symbol_identity_nested(void) {
  printf("  test_symbol_identity_nested...\n");

  akx_cell_t *cells = parse_string_as_file("(foo (foo (foo)))", "sym_identity");
  ASSERT_NOT_NULL(cells);

  akx_cell_t *sym1 = cells->value.list_head;
  assert_symbol(sym1, "foo");

  akx_cell_t *level2 = sym1->next;
  akx_cell_t *sym2 = level2->value.list_head;
  assert_symbol(sym2, "foo");

  akx_cell_t *level3 = sym2->next;
  akx_cell_t *sym3 = level3->value.list_head;
  assert_symbol(sym3, "foo");

  ASSERT_TRUE(sym1->value.symbol == sym2->value.symbol);
  ASSERT_TRUE(sym2->value.symbol == sym3->value.symbol);

  akx_cell_free(cells);
}

static void test_list_chain_integrity(void) {
  printf("  test_list_chain_integrity...\n");

  akx_cell_t *cells =
      parse_string_as_file("(a b c (d e f) g h)", "chain_integrity");
  ASSERT_NOT_NULL(cells);

  validate_list_chain(cells);
  validate_list_chain(cells->value.list_head);

  akx_cell_t *nested = cells->value.list_head->next->next->next;
  assert_cell_type(nested, AKX_TYPE_LIST);
  validate_list_chain(nested->value.list_head);

  akx_cell_free(cells);
}

static void test_sourceloc_nested(void) {
  printf("  test_sourceloc_nested...\n");

  akx_cell_t *cells = parse_string_as_file("(outer (inner))", "sourceloc_nest");
  ASSERT_NOT_NULL(cells);

  ASSERT_NOT_NULL(cells->sourceloc);

  akx_cell_t *outer_sym = cells->value.list_head;
  ASSERT_NOT_NULL(outer_sym->sourceloc);

  akx_cell_t *inner_list = outer_sym->next;
  ASSERT_NOT_NULL(inner_list->sourceloc);

  akx_cell_t *inner_sym = inner_list->value.list_head;
  ASSERT_NOT_NULL(inner_sym->sourceloc);

  akx_cell_free(cells);
}

static void test_clone_simple_types(void) {
  printf("  test_clone_simple_types...\n");

  akx_cell_t *cells = parse_string_as_file("(42)", "clone_int");
  ASSERT_NOT_NULL(cells);
  akx_cell_t *cloned = akx_cell_clone(cells);
  ASSERT_NOT_NULL(cloned);
  assert_cell_type(cloned, AKX_TYPE_LIST);
  assert_integer(cloned->value.list_head, 42);
  akx_cell_free(cells);
  akx_cell_free(cloned);

  cells = parse_string_as_file("(3.14)", "clone_real");
  ASSERT_NOT_NULL(cells);
  cloned = akx_cell_clone(cells);
  ASSERT_NOT_NULL(cloned);
  assert_real(cloned->value.list_head, 3.14, 0.001);
  akx_cell_free(cells);
  akx_cell_free(cloned);

  cells = parse_string_as_file("(\"hello\")", "clone_str");
  ASSERT_NOT_NULL(cells);
  cloned = akx_cell_clone(cells);
  ASSERT_NOT_NULL(cloned);
  assert_string(cloned->value.list_head, "hello");
  akx_cell_free(cells);
  akx_cell_free(cloned);

  cells = parse_string_as_file("foo", "clone_sym");
  ASSERT_NOT_NULL(cells);
  cloned = akx_cell_clone(cells);
  ASSERT_NOT_NULL(cloned);
  assert_symbol(cloned->value.list_head, "foo");
  akx_cell_free(cells);
  akx_cell_free(cloned);
}

static void test_clone_empty_lists(void) {
  printf("  test_clone_empty_lists...\n");

  akx_cell_t *cells = parse_string_as_file("()", "clone_empty_paren");
  ASSERT_NOT_NULL(cells);
  akx_cell_t *cloned = akx_cell_clone(cells);
  ASSERT_NOT_NULL(cloned);
  assert_cell_type(cloned, AKX_TYPE_LIST);
  assert_list_length(cloned, 0);
  akx_cell_free(cells);
  akx_cell_free(cloned);

  cells = parse_string_as_file("[]", "clone_empty_square");
  ASSERT_NOT_NULL(cells);
  cloned = akx_cell_clone(cells);
  ASSERT_NOT_NULL(cloned);
  assert_cell_type(cloned, AKX_TYPE_LIST_SQUARE);
  assert_list_length(cloned, 0);
  akx_cell_free(cells);
  akx_cell_free(cloned);

  cells = parse_string_as_file("{}", "clone_empty_curly");
  ASSERT_NOT_NULL(cells);
  cloned = akx_cell_clone(cells);
  ASSERT_NOT_NULL(cloned);
  assert_cell_type(cloned, AKX_TYPE_LIST_CURLY);
  assert_list_length(cloned, 0);
  akx_cell_free(cells);
  akx_cell_free(cloned);

  cells = parse_string_as_file("<>", "clone_empty_temple");
  ASSERT_NOT_NULL(cells);
  cloned = akx_cell_clone(cells);
  ASSERT_NOT_NULL(cloned);
  assert_cell_type(cloned, AKX_TYPE_LIST_TEMPLE);
  assert_list_length(cloned, 0);
  akx_cell_free(cells);
  akx_cell_free(cloned);
}

static void test_clone_flat_list(void) {
  printf("  test_clone_flat_list...\n");

  akx_cell_t *cells = parse_string_as_file("(a b c)", "clone_flat");
  ASSERT_NOT_NULL(cells);
  akx_cell_t *cloned = akx_cell_clone(cells);
  ASSERT_NOT_NULL(cloned);
  assert_cell_type(cloned, AKX_TYPE_LIST);
  assert_list_length(cloned, 3);

  akx_cell_t *elem = cloned->value.list_head;
  assert_symbol(elem, "a");
  assert_symbol(elem->next, "b");
  assert_symbol(elem->next->next, "c");

  akx_cell_free(cells);
  akx_cell_free(cloned);
}

static void test_clone_nested_lists(void) {
  printf("  test_clone_nested_lists...\n");

  akx_cell_t *cells =
      parse_string_as_file("(a (b (c (d (e)))))", "clone_nested");
  ASSERT_NOT_NULL(cells);
  akx_cell_t *cloned = akx_cell_clone(cells);
  ASSERT_NOT_NULL(cloned);
  assert_cell_type(cloned, AKX_TYPE_LIST);
  assert_list_length(cloned, 2);

  akx_cell_t *level1 = cloned->value.list_head;
  assert_symbol(level1, "a");

  akx_cell_t *level2 = level1->next;
  assert_cell_type(level2, AKX_TYPE_LIST);
  assert_list_length(level2, 2);
  assert_symbol(level2->value.list_head, "b");

  akx_cell_t *level3 = level2->value.list_head->next;
  assert_cell_type(level3, AKX_TYPE_LIST);
  assert_list_length(level3, 2);
  assert_symbol(level3->value.list_head, "c");

  akx_cell_t *level4 = level3->value.list_head->next;
  assert_cell_type(level4, AKX_TYPE_LIST);
  assert_list_length(level4, 2);
  assert_symbol(level4->value.list_head, "d");

  akx_cell_t *level5 = level4->value.list_head->next;
  assert_cell_type(level5, AKX_TYPE_LIST);
  assert_list_length(level5, 1);
  assert_symbol(level5->value.list_head, "e");

  akx_cell_free(cells);
  akx_cell_free(cloned);
}

static void test_clone_mixed_delimiters(void) {
  printf("  test_clone_mixed_delimiters...\n");

  akx_cell_t *cells =
      parse_string_as_file("(outer [inner {deep <temple>}])", "clone_mixed");
  ASSERT_NOT_NULL(cells);
  akx_cell_t *cloned = akx_cell_clone(cells);
  ASSERT_NOT_NULL(cloned);
  assert_cell_type(cloned, AKX_TYPE_LIST);
  assert_list_length(cloned, 2);

  akx_cell_t *outer_sym = cloned->value.list_head;
  assert_symbol(outer_sym, "outer");

  akx_cell_t *square = outer_sym->next;
  assert_cell_type(square, AKX_TYPE_LIST_SQUARE);
  assert_list_length(square, 2);

  akx_cell_t *inner_sym = square->value.list_head;
  assert_symbol(inner_sym, "inner");

  akx_cell_t *curly = inner_sym->next;
  assert_cell_type(curly, AKX_TYPE_LIST_CURLY);
  assert_list_length(curly, 2);

  akx_cell_t *deep_sym = curly->value.list_head;
  assert_symbol(deep_sym, "deep");

  akx_cell_t *temple = deep_sym->next;
  assert_cell_type(temple, AKX_TYPE_LIST_TEMPLE);
  assert_list_length(temple, 1);
  assert_symbol(temple->value.list_head, "temple");

  akx_cell_free(cells);
  akx_cell_free(cloned);
}

static void test_clone_quoted_expressions(void) {
  printf("  test_clone_quoted_expressions...\n");

  akx_cell_t *cells = parse_string_as_file("'x", "clone_quote_sym");
  ASSERT_NOT_NULL(cells);
  akx_cell_t *cloned = akx_cell_clone(cells);
  ASSERT_NOT_NULL(cloned);
  assert_cell_type(cloned, AKX_TYPE_QUOTED);
  ASSERT_NOT_NULL(cloned->value.quoted_literal);
  const char *quoted =
      (const char *)ak_buffer_data(cloned->value.quoted_literal);
  ASSERT_STREQ(quoted, "x");
  akx_cell_free(cells);
  akx_cell_free(cloned);

  cells = parse_string_as_file("'(a b c)", "clone_quote_list");
  ASSERT_NOT_NULL(cells);
  cloned = akx_cell_clone(cells);
  ASSERT_NOT_NULL(cloned);
  assert_cell_type(cloned, AKX_TYPE_QUOTED);
  ASSERT_NOT_NULL(cloned->value.quoted_literal);
  quoted = (const char *)ak_buffer_data(cloned->value.quoted_literal);
  ASSERT_STREQ(quoted, "(a b c)");
  akx_cell_free(cells);
  akx_cell_free(cloned);
}

static void test_clone_virtual_list(void) {
  printf("  test_clone_virtual_list...\n");

  akx_cell_t *cells =
      parse_string_as_file("put \"hello\" 1 2 3", "clone_vlist");
  ASSERT_NOT_NULL(cells);
  akx_cell_t *cloned = akx_cell_clone(cells);
  ASSERT_NOT_NULL(cloned);
  assert_cell_type(cloned, AKX_TYPE_LIST);
  assert_list_length(cloned, 5);

  akx_cell_t *elem = cloned->value.list_head;
  assert_symbol(elem, "put");
  elem = elem->next;
  assert_string(elem, "hello");
  elem = elem->next;
  assert_integer(elem, 1);
  elem = elem->next;
  assert_integer(elem, 2);
  elem = elem->next;
  assert_integer(elem, 3);

  akx_cell_free(cells);
  akx_cell_free(cloned);
}

static void test_clone_multiple_expressions(void) {
  printf("  test_clone_multiple_expressions...\n");

  akx_cell_t *cells =
      parse_string_as_file("add 1 2\n(mul 3 4)\nput \"hello\"", "clone_multi");
  ASSERT_NOT_NULL(cells);
  akx_cell_t *cloned = akx_cell_clone(cells);
  ASSERT_NOT_NULL(cloned);

  ASSERT_EQ(count_cells(cloned), 3);

  assert_cell_type(cloned, AKX_TYPE_LIST);
  assert_symbol(cloned->value.list_head, "add");

  akx_cell_t *second = cloned->next;
  assert_cell_type(second, AKX_TYPE_LIST);
  assert_symbol(second->value.list_head, "mul");

  akx_cell_t *third = second->next;
  assert_cell_type(third, AKX_TYPE_LIST);
  assert_symbol(third->value.list_head, "put");

  akx_cell_free(cells);
  akx_cell_free(cloned);
}

static void test_clone_independence(void) {
  printf("  test_clone_independence...\n");

  akx_cell_t *cells = parse_string_as_file("(1 2 3)", "clone_indep");
  ASSERT_NOT_NULL(cells);
  akx_cell_t *cloned = akx_cell_clone(cells);
  ASSERT_NOT_NULL(cloned);

  akx_cell_t *orig_first = cells->value.list_head;
  akx_cell_t *clone_first = cloned->value.list_head;

  ASSERT_EQ(orig_first->value.integer_literal, 1);
  ASSERT_EQ(clone_first->value.integer_literal, 1);

  clone_first->value.integer_literal = 999;

  ASSERT_EQ(orig_first->value.integer_literal, 1);
  ASSERT_EQ(clone_first->value.integer_literal, 999);

  akx_cell_free(cells);
  akx_cell_free(cloned);
}

static void test_clone_sourceloc_preservation(void) {
  printf("  test_clone_sourceloc_preservation...\n");

  akx_cell_t *cells =
      parse_string_as_file("(outer (inner))", "clone_sourceloc");
  ASSERT_NOT_NULL(cells);
  akx_cell_t *cloned = akx_cell_clone(cells);
  ASSERT_NOT_NULL(cloned);

  ASSERT_NOT_NULL(cells->sourceloc);
  ASSERT_NOT_NULL(cloned->sourceloc);

  akx_cell_t *orig_outer = cells->value.list_head;
  akx_cell_t *clone_outer = cloned->value.list_head;
  ASSERT_NOT_NULL(orig_outer->sourceloc);
  ASSERT_NOT_NULL(clone_outer->sourceloc);

  akx_cell_t *orig_inner = orig_outer->next;
  akx_cell_t *clone_inner = clone_outer->next;
  ASSERT_NOT_NULL(orig_inner->sourceloc);
  ASSERT_NOT_NULL(clone_inner->sourceloc);

  akx_cell_free(cells);
  akx_cell_free(cloned);
}

static void test_clone_null_cell(void) {
  printf("  test_clone_null_cell...\n");

  akx_cell_t *cloned = akx_cell_clone(NULL);
  ASSERT_TRUE(cloned == NULL);
}

void run_all_tests(void) {
  printf("Running akx_cell tests...\n");

  printf("\n=== Basic Literals ===\n");
  test_integer_literals();
  test_real_literals();
  test_string_literals();
  test_string_with_single_char();
  test_symbols();

  printf("\n=== Virtual Lists ===\n");
  test_simple_virtual_list();
  test_multi_arg_virtual_list();
  test_virtual_list_newline_termination();

  printf("\n=== Explicit Lists ===\n");
  test_paren_list();
  test_square_list();
  test_curly_list();
  test_temple_list();
  test_empty_lists();

  printf("\n=== Basic Nesting ===\n");
  test_nested_delimiters();
  test_virtual_list_with_explicit();

  printf("\n=== Quoted Expressions ===\n");
  test_quoted_symbol();
  test_quoted_list();
  test_quoted_virtual_list();
  test_unwrap_quoted();

  printf("\n=== Multiple Expressions ===\n");
  test_multiple_expressions();

  printf("\n=== Deep Nesting (3-5 levels) ===\n");
  test_deeply_nested_parens();
  test_deeply_nested_mixed();
  test_nested_with_data_at_each_level();

  printf("\n=== Data Validation in Nested Contexts ===\n");
  test_nested_integer_preservation();
  test_nested_string_preservation();
  test_nested_real_precision();
  test_mixed_types_nested();

  printf("\n=== Virtual + Explicit Combinations ===\n");
  test_virtual_with_deeply_nested_explicit();
  test_nested_virtual_in_explicit();
  test_multiple_nested_in_virtual();

  printf("\n=== Quoted Nested Structures ===\n");
  test_quoted_nested_list();
  test_unwrap_quoted_nested();
  test_quoted_mixed_delimiters();
  test_multiple_quote_levels();
  test_quote_in_explicit_list();
  test_multiple_quotes_in_list();
  test_deeply_nested_quotes();

  printf("\n=== Edge Cases ===\n");
  test_adjacent_empty_lists();
  test_adjacent_nested_lists();
  test_deeply_nested_empty();
  test_whitespace_in_deep_nesting();

  printf("\n=== Data Integrity Checks ===\n");
  test_symbol_identity_nested();
  test_list_chain_integrity();
  test_sourceloc_nested();

  printf("\n=== Clone Tests ===\n");
  test_clone_simple_types();
  test_clone_empty_lists();
  test_clone_flat_list();
  test_clone_nested_lists();
  test_clone_mixed_delimiters();
  test_clone_quoted_expressions();
  test_clone_virtual_list();
  test_clone_multiple_expressions();
  test_clone_independence();
  test_clone_sourceloc_preservation();
  test_clone_null_cell();

  printf("\nAll tests passed!\n");
}
