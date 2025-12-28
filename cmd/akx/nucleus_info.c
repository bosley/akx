#include "nucleus_info.h"
#include "akx_core.h"
#include "akx_rt.h"
#include <ak24/kernel.h>
#include <ak24/list.h>
#include <ak24/map.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static int compare_strings(const void *a, const void *b) {
  const char *str_a = *(const char **)a;
  const char *str_b = *(const char **)b;
  return strcmp(str_a, str_b);
}

int akx_nucleus_info(void) {
  akx_core_t *core = akx_core_init();
  if (!core) {
    printf("Error: Failed to initialize AKX core\n");
    return 1;
  }

  akx_runtime_ctx_t *runtime = akx_runtime_init();
  if (!runtime) {
    printf("Error: Failed to initialize AKX runtime\n");
    akx_core_deinit(core);
    return 1;
  }

  map_void_t *builtins = akx_rt_get_builtins(runtime);
  if (!builtins) {
    printf("Error: Failed to get builtins map\n");
    akx_runtime_deinit(runtime);
    akx_core_deinit(core);
    return 1;
  }

  list_str_t names;
  list_init(&names);

  map_iter_t iter = map_iter(builtins);
  char **key_ptr;
  while ((key_ptr = map_next_generic(builtins, &iter))) {
    const char *name = *key_ptr;
    list_push(&names, (char *)name);
  }

  size_t count = list_count(&names);

  char **names_array = AK24_ALLOC(sizeof(char *) * count);
  if (!names_array) {
    printf("Error: Failed to allocate names array\n");
    list_deinit(&names);
    akx_runtime_deinit(runtime);
    akx_core_deinit(core);
    return 1;
  }

  list_iter_t list_iter_obj = list_iter(&names);
  char **name_ptr;
  size_t i = 0;
  while ((name_ptr = list_next(&names, &list_iter_obj))) {
    names_array[i++] = *name_ptr;
  }

  qsort(names_array, count, sizeof(char *), compare_strings);

  size_t max_len = 0;
  for (i = 0; i < count; i++) {
    size_t len = strlen(names_array[i]);
    if (len > max_len) {
      max_len = len;
    }
  }

  const size_t columns = 4;
  const size_t col_width = max_len + 2;

  printf("\nCompiled-in Builtins (%zu):\n\n", count);

  for (i = 0; i < count; i++) {
    if (i % columns == 0) {
      printf("  ");
    }
    printf("%-*s", (int)col_width, names_array[i]);
    if ((i + 1) % columns == 0 || i == count - 1) {
      printf("\n");
    }
  }

  printf("\n");

  AK24_FREE(names_array);
  list_deinit(&names);
  akx_runtime_deinit(runtime);
  akx_core_deinit(core);

  return 0;
}
