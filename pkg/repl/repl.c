#include "repl.h"
#include "akx_cell.h"
#include "akx_sv.h"
#include "repl_buffer.h"
#include "repl_display.h"
#include <ak24/filepath.h>
#include <ak24/kernel.h>
#include <linenoise.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int akx_repl_start(akx_core_t *core, akx_runtime_ctx_t *runtime,
                   akx_repl_signal_ctx_t *signal_ctx) {
  if (!core || !runtime) {
    printf("Error: Invalid core or runtime\n");
    return 1;
  }

  printf("\nAKX REPL v0.1.0\n");
  printf("Type expressions or press Ctrl+D to exit\n\n");

  volatile int local_keep_running = 1;
  volatile int local_signal_count = 0;

  volatile int *keep_running = &local_keep_running;
  volatile int *signal_count = &local_signal_count;

  if (signal_ctx) {
    keep_running = signal_ctx->keep_running;
    signal_count = signal_ctx->signal_count;
  }

  if (signal_count) {
    *signal_count = 0;
  }
  if (keep_running) {
    *keep_running = 1;
  }

  repl_buffer_t *buffer = repl_buffer_new();
  if (!buffer) {
    printf("Error: Failed to initialize buffer\n");
    return 1;
  }

  linenoiseHistorySetMaxLen(1000);

  ak_buffer_t *akx_home_buf = NULL;
  const char *akx_home = getenv("AKX_HOME");
  if (!akx_home) {
    const char *home = getenv("HOME");
    if (home) {
      akx_home_buf = ak_filepath_join(2, home, ".akx");
      if (akx_home_buf) {
        akx_home = (const char *)ak_buffer_data(akx_home_buf);
      }
    }
  }

  if (akx_home) {
    ak_buffer_t *history_path = ak_filepath_join(2, akx_home, "repl_history");
    if (history_path) {
      linenoiseHistoryLoad((const char *)ak_buffer_data(history_path));
      ak_buffer_free(history_path);
    }
  }

  if (akx_home_buf) {
    ak_buffer_free(akx_home_buf);
  }

  while (*keep_running) {
    const char *prompt = repl_buffer_is_empty(buffer) ? "akx> " : "...> ";
    char *line = linenoise(prompt);

    if (!line || !*keep_running) {
      if (line) {
        linenoiseFree(line);
      }
      break;
    }

    if (strlen(line) == 0) {
      if (repl_buffer_is_empty(buffer)) {
        linenoiseFree(line);
        continue;
      }
    } else {
      repl_buffer_append_line(buffer, line);
    }

    linenoiseFree(line);

    if (!repl_buffer_is_balanced(buffer)) {
      continue;
    }

    ak_buffer_t *code_buf = repl_buffer_get_content(buffer);
    if (!code_buf || ak_buffer_count(code_buf) == 0) {
      repl_buffer_clear(buffer);
      continue;
    }

    const char *code_str = (const char *)ak_buffer_data(code_buf);

    char *history_entry = AK24_ALLOC(strlen(code_str) + 1);
    if (history_entry) {
      strcpy(history_entry, code_str);
      for (char *p = history_entry; *p; p++) {
        if (*p == '\n')
          *p = ' ';
      }
      linenoiseHistoryAdd(history_entry);
      AK24_FREE(history_entry);
    }

    akx_parse_result_t result = akx_cell_parse_buffer(code_buf, "<repl>");

    if (result.errors) {
      akx_parse_error_t *err = result.errors;
      while (err) {
        akx_sv_show_location(&err->location, AKX_ERROR_LEVEL_ERROR,
                             err->message);
        err = err->next;
      }
    } else if (list_count(&result.cells) > 0) {
      list_iter_t iter = list_iter(&result.cells);
      akx_cell_t **cell_ptr;
      while ((cell_ptr = list_next(&result.cells, &iter))) {
        akx_cell_t *evaled = akx_rt_eval(runtime, *cell_ptr);
        if (!evaled) {
          akx_parse_error_t *err = akx_runtime_get_errors(runtime);
          while (err) {
            akx_sv_show_location(&err->location, AKX_ERROR_LEVEL_ERROR,
                                 err->message);
            err = err->next;
          }
        } else {
          repl_display_cell(evaled);
          akx_cell_free(evaled);
        }
      }
    }

    akx_parse_result_free(&result);
    repl_buffer_clear(buffer);
  }

  akx_home_buf = NULL;
  akx_home = getenv("AKX_HOME");
  if (!akx_home) {
    const char *home = getenv("HOME");
    if (home) {
      akx_home_buf = ak_filepath_join(2, home, ".akx");
      if (akx_home_buf) {
        akx_home = (const char *)ak_buffer_data(akx_home_buf);
      }
    }
  }

  if (akx_home) {
    ak_buffer_t *history_path = ak_filepath_join(2, akx_home, "repl_history");
    if (history_path) {
      linenoiseHistorySave((const char *)ak_buffer_data(history_path));
      ak_buffer_free(history_path);
    }
  }

  if (akx_home_buf) {
    ak_buffer_free(akx_home_buf);
  }

  repl_buffer_free(buffer);

  printf("\nGoodbye!\n");
  return 0;
}
