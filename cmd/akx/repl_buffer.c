#include "repl_buffer.h"
#include <ak24/kernel.h>
#include <string.h>

repl_buffer_t *repl_buffer_new(void) {
  repl_buffer_t *buf = AK24_ALLOC(sizeof(repl_buffer_t));
  if (!buf) {
    return NULL;
  }

  buf->content = ak_buffer_new(256);
  if (!buf->content) {
    AK24_FREE(buf);
    return NULL;
  }

  buf->line_count = 0;
  buf->paren_depth = 0;
  buf->bracket_depth = 0;
  buf->brace_depth = 0;
  buf->temple_depth = 0;

  return buf;
}

void repl_buffer_free(repl_buffer_t *buf) {
  if (!buf) {
    return;
  }

  if (buf->content) {
    ak_buffer_free(buf->content);
  }

  AK24_FREE(buf);
}

void repl_buffer_append_line(repl_buffer_t *buf, const char *line) {
  if (!buf || !line) {
    return;
  }

  size_t len = strlen(line);
  if (len > 0) {
    ak_buffer_copy_to(buf->content, (uint8_t *)line, len);
  }
  ak_buffer_copy_to(buf->content, (uint8_t *)"\n", 1);
  buf->line_count++;
}

void repl_buffer_clear(repl_buffer_t *buf) {
  if (!buf) {
    return;
  }

  ak_buffer_clear(buf->content);
  buf->line_count = 0;
  buf->paren_depth = 0;
  buf->bracket_depth = 0;
  buf->brace_depth = 0;
  buf->temple_depth = 0;
}

bool repl_buffer_is_balanced(repl_buffer_t *buf) {
  if (!buf || !buf->content) {
    return true;
  }

  int paren = 0, bracket = 0, brace = 0, temple = 0;
  bool in_string = false;
  bool escape_next = false;

  uint8_t *data = ak_buffer_data(buf->content);
  size_t len = ak_buffer_count(buf->content);

  for (size_t i = 0; i < len; i++) {
    uint8_t c = data[i];

    if (escape_next) {
      escape_next = false;
      continue;
    }

    if (c == '\\' && in_string) {
      escape_next = true;
      continue;
    }

    if (c == '"') {
      in_string = !in_string;
      continue;
    }

    if (in_string) {
      continue;
    }

    switch (c) {
    case '(':
      paren++;
      break;
    case ')':
      paren--;
      break;
    case '[':
      bracket++;
      break;
    case ']':
      bracket--;
      break;
    case '{':
      brace++;
      break;
    case '}':
      brace--;
      break;
    case '<':
      temple++;
      break;
    case '>':
      temple--;
      break;
    }
  }

  buf->paren_depth = paren;
  buf->bracket_depth = bracket;
  buf->brace_depth = brace;
  buf->temple_depth = temple;

  return (paren == 0 && bracket == 0 && brace == 0 && temple == 0);
}

bool repl_buffer_is_empty(repl_buffer_t *buf) {
  if (!buf || !buf->content) {
    return true;
  }
  return ak_buffer_count(buf->content) == 0;
}

ak_buffer_t *repl_buffer_get_content(repl_buffer_t *buf) {
  if (!buf) {
    return NULL;
  }
  return buf->content;
}
