#include "akx.h"
#include "akx_core.h"
#include "akx_rt.h"
#include "akx_sv.h"
#include <ak24/application.h>
#include <stdio.h>
#include <unistd.h>

static volatile int keep_running = 1;
static volatile int signal_count = 0;
static akx_core_t *g_core = NULL;
static akx_runtime_ctx_t *g_runtime = NULL;

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
  AK24_LOG_TRACE("Shutting down AKX runtime (uptime: %ld seconds)",
                 (long)uptime);
  AK24_LOG_TRACE("Deinitializing AKX core");
  if (g_runtime) {
    akx_runtime_deinit(g_runtime);
    g_runtime = NULL;
  }

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
  printf("======================\n");
#endif
}

APP_MAIN(app_main) {
  ak_log_set_level(AK24_LOG_LEVEL_INFO);
  ak_log_set_color(true);
  ak_log_set_path_format(AK24_LOG_PATH_ABBREV);

  g_core = akx_core_init();
  if (!g_core) {
    AK24_LOG_ERROR("Failed to initialize AKX core");
    return 1;
  }

  g_runtime = akx_runtime_init();
  if (!g_runtime) {
    AK24_LOG_ERROR("Failed to initialize AKX runtime");
    akx_core_deinit(g_core);
    return 1;
  }

  AK24_REGISTER_SIGNAL_HANDLER(handle_sigint);
  AK24_REGISTER_SIGNAL_HANDLER(handle_sigterm);

  AK24_LOG_TRACE("All signal handlers registered successfully");
  AK24_LOG_TRACE("AKX core initialized and ready");

  if (list_count(&ctx->args) > 1) {
    list_iter_t iter = list_iter(&ctx->args);
    char **arg;
    int arg_index = 0;
    while ((arg = list_next(&ctx->args, &iter))) {
      if (arg_index == 0) {
        arg_index++;
        continue;
      }

      akx_parse_result_t result = akx_cell_parse_file(*arg);

      if (result.errors) {
        akx_parse_error_t *err = result.errors;
        while (err) {
          akx_sv_show_location(&err->location, AKX_ERROR_LEVEL_ERROR,
                               err->message);
          err = err->next;
        }
        akx_parse_result_free(&result);
        arg_index++;
        continue;
      }

      if (list_count(&result.cells) > 0) {
        if (akx_runtime_start(g_runtime, (akx_cell_list_t *)&result.cells) !=
            0) {
          akx_parse_error_t *err = akx_runtime_get_errors(g_runtime);
          while (err) {
            akx_sv_show_location(&err->location, AKX_ERROR_LEVEL_ERROR,
                                 err->message);
            err = err->next;
          }
        }
      }

      akx_parse_result_free(&result);
      arg_index++;
    }
  } else {
    printf("\nNo files provided. Usage: akx <file1> [file2] ...\n");
  }

  return 0;
}

AK24_APPLICATION("akx", app_main, on_shutdown)
