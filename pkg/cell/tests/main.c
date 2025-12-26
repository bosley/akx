#include "tests.h"
#include <ak24/kernel.h>
#include <ak24/log.h>
#include <stdio.h>

int main(void) {
  ak_kernel_init("akx-cell-tests");
  ak_log_set_level(AK24_LOG_LEVEL_DEBUG);

  printf("=== AKX Cell Tests ===\n");
  run_all_tests();
  printf("======================\n");

  ak_kernel_deinit();
  return 0;
}
