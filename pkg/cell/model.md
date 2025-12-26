# AKX Cell Memory Model

## Structure

### Top-Level Expressions
`akx_parse_result_t.cells` is a `list_t(akx_cell_t *)` containing independent top-level expressions.

### Cell Linking
- **`next`**: Links sibling cells within a list (horizontal)
- **`list_head`**: Points to first child of a LIST-type cell (vertical)

Example: `(a b (c d))`
```
LIST
  list_head → a
              next → b
                     next → LIST
                            list_head → c
                                        next → d
```

## Parse Result

```c
typedef struct {
  list_t(akx_cell_t *) cells;      // Top-level expressions
  akx_parse_error_t *errors;        // Parse errors (linked list)
  ak_source_file_t *source_file;    // Source file (kept alive)
} akx_parse_result_t;
```

Caller owns the result. Free with `akx_parse_result_free()`.

## Errors

Errors collected during parsing, not printed by library.
Each error has `ak_source_loc_t` and message.
Deduplicated by position.

## Cloning

`akx_cell_clone()` deep copies a cell and all children/siblings.
Cloned cells are independent - modifying one doesn't affect the other.
Source locations are copied by value.

