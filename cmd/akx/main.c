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

  printf("Arguments (%u):\n", list_count(&ctx->args));
  list_iter_t iter = list_iter(&ctx->args);
  char **arg;
  while ((arg = list_next(&ctx->args, &iter))) {
    printf("  %s\n", *arg);
  }
  printf("\n");

  AK24_REGISTER_SIGNAL_HANDLER(handle_sigint);
  AK24_REGISTER_SIGNAL_HANDLER(handle_sigterm);

  AK24_LOG_INFO("All signal handlers registered successfully");
  AK24_LOG_INFO("AKX core initialized and ready");

  printf("\nRunning main loop... (Press Ctrl+C to exit)\n\n");

  int iteration = 0;
  while (keep_running) {
    iteration++;
    printf("\r[%d] Running... (signals received: %d)", iteration, signal_count);
    fflush(stdout);
    sleep(1);
  }

  printf("\n\nMain loop exited.\n");
  return 0;
}

AK24_APPLICATION("akx", app_main, on_shutdown)
