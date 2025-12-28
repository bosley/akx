#include "commands.h"
#include "nucleus_info.h"
#include "nucleus_list.h"
#include <stdio.h>
#include <string.h>

int akx_dispatch_command(int argc, char **argv) {
  if (argc < 2) {
    printf("Usage: akx <command> [args...]\n");
    return 1;
  }

  const char *command = argv[1];

  if (strcmp(command, "nucleus") == 0) {
    if (argc < 3) {
      printf("Usage: akx nucleus <info|list>\n");
      return 1;
    }

    const char *subcommand = argv[2];

    if (strcmp(subcommand, "info") == 0) {
      return akx_nucleus_info();
    } else if (strcmp(subcommand, "list") == 0) {
      return akx_nucleus_list();
    } else {
      printf("Unknown nucleus subcommand: %s\n", subcommand);
      printf("Available subcommands: info, list\n");
      return 1;
    }
  } else {
    printf("Unknown command: %s\n", command);
    printf("Available commands: nucleus\n");
    return 1;
  }
}
