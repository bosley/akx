#ifndef AKX_REPL_BUFFER_H
#define AKX_REPL_BUFFER_H

#include <ak24/buffer.h>
#include <stdbool.h>
#include <stddef.h>

typedef struct {
  ak_buffer_t *content;
  size_t line_count;
  int paren_depth;
  int bracket_depth;
  int brace_depth;
  int temple_depth;
} repl_buffer_t;

repl_buffer_t *repl_buffer_new(void);
void repl_buffer_free(repl_buffer_t *buf);
void repl_buffer_append_line(repl_buffer_t *buf, const char *line);
void repl_buffer_clear(repl_buffer_t *buf);
bool repl_buffer_is_balanced(repl_buffer_t *buf);
bool repl_buffer_is_empty(repl_buffer_t *buf);
ak_buffer_t *repl_buffer_get_content(repl_buffer_t *buf);

#endif
