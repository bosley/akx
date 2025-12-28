#include "akx.h"
#include "commands.h"
#include "help.h"
#include "repl.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>

volatile int keep_running = 1;
volatile int signal_count = 0;
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

APP_ON_SIGNAL(handle_sigabrt, SIGABRT) {
  (void)captured;
  (void)args;

  printf("\n[SIGABRT] Abort signal raised - terminating execution\n");
  fflush(stdout);
  _exit(134);
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
  AK24_REGISTER_SIGNAL_HANDLER(handle_sigabrt);

  AK24_LOG_TRACE("All signal handlers registered successfully");
  AK24_LOG_TRACE("AKX core initialized and ready");

  size_t argc = list_count(&ctx->args);

  if (argc == 1) {
    return akx_repl_start(g_core, g_runtime);
  }

  char **argv = AK24_ALLOC(sizeof(char *) * argc);
  if (!argv) {
    AK24_LOG_ERROR("Failed to allocate argv array");
    return 1;
  }

  list_iter_t iter = list_iter(&ctx->args);
  char **arg;
  size_t i = 0;
  while ((arg = list_next(&ctx->args, &iter))) {
    argv[i++] = *arg;
  }

  if (argc == 2) {
    if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
      akx_print_help();
      AK24_FREE(argv);
      return 0;
    }
    if (strcmp(argv[1], "nucleus") != 0) {
      int result = akx_interpret_file(argv[1], g_core, g_runtime);
      AK24_FREE(argv);
      return result;
    }
  }

  int result = akx_dispatch_command((int)argc, argv);
  AK24_FREE(argv);
  return result;

  return 0;
}

AK24_APPLICATION("akx", app_main, on_shutdown)
