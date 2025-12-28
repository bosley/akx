#include "repl.h"
#include "akx.h"
#include "akx_sv.h"
#include "repl_buffer.h"
#include "repl_display.h"
#include "repl_history.h"
#include "repl_input.h"
#include <ak24/filepath.h>
#include <ak24/kernel.h>
#include <stdio.h>
#include <string.h>

int akx_repl_start(akx_core_t *core, akx_runtime_ctx_t *runtime) {
  if (!core || !runtime) {
    printf("Error: Invalid core or runtime\n");
    return 1;
  }

  printf("\nAKX REPL v0.1.0\n");
  printf("Type expressions or press Ctrl+D to exit\n\n");

  signal_count = 0;
  keep_running = 1;

  repl_input_ctx_t *input_ctx = repl_input_init();
  if (!input_ctx) {
    printf("Error: Failed to initialize input\n");
    return 1;
  }

  repl_buffer_t *buffer = repl_buffer_new();
  if (!buffer) {
    printf("Error: Failed to initialize buffer\n");
    repl_input_deinit(input_ctx);
    return 1;
  }

  repl_history_t *history = repl_history_new(1000);
  if (!history) {
    printf("Error: Failed to initialize history\n");
    repl_buffer_free(buffer);
    repl_input_deinit(input_ctx);
    return 1;
  }

  ak_buffer_t *home = ak_filepath_home();
  if (home) {
    ak_buffer_t *history_path =
        ak_filepath_join(2, (const char *)ak_buffer_data(home), ".akx/history");
    if (history_path) {
      repl_history_load(history, (const char *)ak_buffer_data(history_path));
      ak_buffer_free(history_path);
    }
    ak_buffer_free(home);
  }

  while (keep_running) {
    const char *prompt = repl_buffer_is_empty(buffer) ? "akx> " : "...> ";
    char *line = repl_input_read_line(input_ctx, prompt);

    if (!line || !keep_running) {
      if (line) {
        AK24_FREE(line);
      }
      break;
    }

    if (strlen(line) == 0) {
      AK24_FREE(line);
      if (repl_buffer_is_empty(buffer)) {
        continue;
      }
    } else {
      repl_buffer_append_line(buffer, line);
      AK24_FREE(line);
    }

    if (!repl_buffer_is_balanced(buffer)) {
      continue;
    }

    ak_buffer_t *code_buf = repl_buffer_get_content(buffer);
    if (!code_buf || ak_buffer_count(code_buf) == 0) {
      repl_buffer_clear(buffer);
      continue;
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

  home = ak_filepath_home();
  if (home) {
    ak_buffer_t *history_path =
        ak_filepath_join(2, (const char *)ak_buffer_data(home), ".akx/history");
    if (history_path) {
      repl_history_save(history, (const char *)ak_buffer_data(history_path));
      ak_buffer_free(history_path);
    }
    ak_buffer_free(home);
  }

  repl_history_free(history);
  repl_buffer_free(buffer);
  repl_input_deinit(input_ctx);

  printf("\nGoodbye!\n");
  return 0;
}
