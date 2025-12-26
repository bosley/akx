#include "akx.h"
#include "akx_core.h"
#include <ak24/application.h>
#include <stdio.h>
#include <unistd.h>

static volatile int keep_running = 1;
static volatile int signal_count = 0;
static akx_core_t *g_core = NULL;

APP_ON_SIGNAL(handle_sigint, SIGINT) {
  (void)captured;
  int signum = *(int *)args;
  (void)signum;

  signal_count++;
  printf("\n[SIGINT] Caught Ctrl+C (signal count: %d)\n", signal_count);
  printf("[SIGINT] Press Ctrl+C again to exit\n");

  if (signal_count >= 2) {
    printf("[SIGINT] Exiting after multiple interrupts...\n");
    keep_running = 0;
  }
}

APP_ON_SIGNAL(handle_sigterm, SIGTERM) {
  (void)captured;
  (void)args;

  printf("\n[SIGTERM] Received termination signal\n");
  printf("[SIGTERM] Shutting down gracefully...\n");
  keep_running = 0;
}

APP_ON_SHUTDOWN(on_shutdown) {
  time_t uptime = time(NULL) - ctx->shutdown_info->start_time;
  printf("\n=== Shutting Down ===\n");
  printf("Uptime: %ld seconds\n", uptime);
  printf("Total signals received: %d\n", signal_count);

  if (g_core) {
    akx_core_deinit(g_core);
    g_core = NULL;
  }

#if AK24_BUILD_DEBUG_MEMORY
  printf("\n=== Memory Statistics ===\n");
  printf("Total allocations: %zu\n",
         ctx->shutdown_info->memory_stats.total_allocations);
  printf("Total frees: %zu\n", ctx->shutdown_info->memory_stats.total_frees);
  printf("Current bytes: %zu\n",
         ctx->shutdown_info->memory_stats.current_bytes);
  printf("Peak bytes: %zu\n", ctx->shutdown_info->memory_stats.peak_bytes);
#endif

  printf("======================\n");
}

static void print_cell(akx_cell_t *cell, int indent) {
  if (!cell) {
    return;
  }

  for (int i = 0; i < indent; i++) {
    printf("  ");
  }

  switch (cell->type) {
  case AKX_TYPE_SYMBOL:
    printf("SYMBOL: %s\n", cell->value.symbol);
    break;
  case AKX_TYPE_STRING_LITERAL:
    printf("STRING: \"%.*s\"\n",
           (int)ak_buffer_count(cell->value.string_literal),
           (char *)ak_buffer_data(cell->value.string_literal));
    break;
  case AKX_TYPE_CHAR_LITERAL:
    printf("CHAR: '%c'\n", cell->value.char_literal);
    break;
  case AKX_TYPE_INTEGER_LITERAL:
    printf("INT: %d\n", cell->value.integer_literal);
    break;
  case AKX_TYPE_REAL_LITERAL:
    printf("REAL: %f\n", cell->value.real_literal);
    break;
  case AKX_TYPE_LIST:
    printf("LIST:\n");
    print_cell(cell->value.list_head, indent + 1);
    break;
  }

  if (cell->next) {
    print_cell(cell->next, indent);
  }
}

APP_MAIN(app_main) {
  ak_log_set_level(AK24_LOG_LEVEL_INFO);
  ak_log_set_color(true);
  ak_log_set_path_format(AK24_LOG_PATH_ABBREV);

  printf("=== AKX - AK24 S-Expression Tokenizer ===\n\n");
  printf("Process ID: %d\n\n", getpid());

  g_core = akx_core_init();
  if (!g_core) {
    AK24_LOG_ERROR("Failed to initialize AKX core");
    return 1;
  }

  AK24_REGISTER_SIGNAL_HANDLER(handle_sigint);
  AK24_REGISTER_SIGNAL_HANDLER(handle_sigterm);

  AK24_LOG_INFO("All signal handlers registered successfully");
  AK24_LOG_INFO("AKX core initialized and ready");

  if (list_count(&ctx->args) > 1) {
    list_iter_t iter = list_iter(&ctx->args);
    char **arg;
    int arg_index = 0;
    while ((arg = list_next(&ctx->args, &iter))) {
      if (arg_index == 0) {
        arg_index++;
        continue;
      }

      printf("\n=== Parsing file: %s ===\n", *arg);

      akx_cell_t *cells = akx_cell_parse_file(*arg);
      if (cells) {
        printf("\n=== Parsed cells ===\n");
        print_cell(cells, 0);
        akx_cell_free(cells);
      } else {
        AK24_LOG_ERROR("Failed to parse file: %s", *arg);
      }
      arg_index++;
    }
  } else {
    printf("\nNo files provided. Usage: akx <file1> [file2] ...\n");
  }

  return 0;
}

AK24_APPLICATION("akx", app_main, on_shutdown)
