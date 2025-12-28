#include "akx.h"
#include "akx_sv.h"
#include <ak24/list.h>
#include <stdio.h>

int akx_interpret_file(const char *filename, akx_core_t *core,
                       akx_runtime_ctx_t *runtime) {
  (void)core;

  akx_parse_result_t result = akx_cell_parse_file(filename);

  if (result.errors) {
    akx_parse_error_t *err = result.errors;
    while (err) {
      akx_sv_show_location(&err->location, AKX_ERROR_LEVEL_ERROR, err->message);
      err = err->next;
    }
    akx_parse_result_free(&result);
    return 1;
  }

  if (list_count(&result.cells) > 0) {
    if (akx_runtime_start(runtime, (akx_cell_list_t *)&result.cells) != 0) {
      akx_parse_error_t *err = akx_runtime_get_errors(runtime);
      while (err) {
        akx_sv_show_location(&err->location, AKX_ERROR_LEVEL_ERROR,
                             err->message);
        err = err->next;
      }
      akx_parse_result_free(&result);
      return 1;
    }
  }

  akx_parse_result_free(&result);
  return 0;
}
