#include "help.h"
#include <stdio.h>

void akx_print_help(void) {
  printf("\n");
  printf("AKX - A self-extending Lisp runtime\n");
  printf("\n");
  printf("USAGE:\n");
  printf("  akx                     Start REPL mode\n");
  printf("  akx <file.akx>          Execute an AKX file\n");
  printf("  akx nucleus info        List compiled-in builtins\n");
  printf("  akx nucleus list        List available nuclei in ~/.akx/nucleus\n");
  printf("  akx -h, --help          Show this help message\n");
  printf("\n");
  printf("EXAMPLES:\n");
  printf("  akx script.akx          Run script.akx\n");
  printf("  akx nucleus info        Show all built-in functions\n");
  printf("  akx nucleus list        Show loadable nucleus files\n");
  printf("\n");
  printf("For more information, see the documentation.\n");
  printf("\n");
}
