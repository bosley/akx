#include "akx_sv.h"
#include <ak24/kernel.h>
#include <stdio.h>
#include <string.h>

static const char *level_color(akx_error_level_t level) {
  switch (level) {
  case AKX_ERROR_LEVEL_ERROR:
    return "\033[1;31m";
  case AKX_ERROR_LEVEL_WARNING:
    return "\033[1;33m";
  case AKX_ERROR_LEVEL_INFO:
    return "\033[1;36m";
  default:
    return "\033[0m";
  }
}

static const char *level_name(akx_error_level_t level) {
  switch (level) {
  case AKX_ERROR_LEVEL_ERROR:
    return "error";
  case AKX_ERROR_LEVEL_WARNING:
    return "warning";
  case AKX_ERROR_LEVEL_INFO:
    return "info";
  default:
    return "note";
  }
}

void akx_sv_show_error(ak_source_range_t *range, akx_error_level_t level,
                       const char *message) {
  if (!range || !range->start.file) {
    printf("%s%s:\033[0m %s\n", level_color(level), level_name(level), message);
    return;
  }

  ak_source_file_t *file = range->start.file;

  char location_buf[512];
  ak_source_range_format(*range, location_buf, sizeof(location_buf));

  printf("\n");
  printf("%s%s:\033[0m %s\n", level_color(level), level_name(level), message);
  printf("   \033[1;37m-->\033[0m %s\n", location_buf);
  printf("\033[1;34m    |\033[0m\n");

  uint32_t start_line = range->start.line;
  uint32_t end_line = range->end.line;

  if (start_line == end_line) {
    const char *line_text = ak_source_get_line(file, start_line);
    size_t line_len = ak_source_get_line_len(file, start_line);

    if (line_text) {
      printf("\033[1;34m%3u |\033[0m ", start_line);
      for (size_t i = 0; i < line_len; i++) {
        char c = line_text[i];
        if (c == '\t') {
          printf("    ");
        } else {
          printf("%c", c);
        }
      }
      printf("\n");

      printf("\033[1;34m    |\033[0m ");

      uint32_t current_col = 1;
      for (size_t i = 0; i < line_len && current_col < range->start.column;
           i++) {
        if (line_text[i] == '\t') {
          printf("    ");
          current_col += 4;
        } else {
          printf(" ");
          current_col++;
        }
      }

      printf("%s", level_color(level));

      uint32_t highlight_len = range->end.column - range->start.column;
      if (highlight_len == 0) {
        highlight_len = 1;
      }

      for (uint32_t i = 0; i < highlight_len; i++) {
        printf("^");
      }

      printf("\033[0m\n");
    }
  } else {
    for (uint32_t line_num = start_line;
         line_num <= end_line && line_num <= start_line + 3; line_num++) {
      const char *line_text = ak_source_get_line(file, line_num);
      size_t line_len = ak_source_get_line_len(file, line_num);

      if (line_text) {
        printf("\033[1;34m%3u |\033[0m ", line_num);
        for (size_t i = 0; i < line_len; i++) {
          char c = line_text[i];
          if (c == '\t') {
            printf("    ");
          } else {
            printf("%c", c);
          }
        }
        printf("\n");
      }
    }

    if (end_line > start_line + 3) {
      printf("\033[1;34m    |\033[0m ... (%u more lines)\n",
             end_line - start_line - 3);
    }
  }

  printf("\033[1;34m    |\033[0m\n");
  printf("\n");
}

void akx_sv_show_location(ak_source_loc_t *loc, akx_error_level_t level,
                          const char *message) {
  if (!loc || !loc->file) {
    printf("%s%s:\033[0m %s\n", level_color(level), level_name(level), message);
    return;
  }

  ak_source_range_t range;
  range.start = *loc;
  range.end = *loc;
  range.end.column = loc->column + 1;
  range.end.offset = loc->offset + 1;

  akx_sv_show_error(&range, level, message);
}
