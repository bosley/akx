#include <limits.h>
#include <stdlib.h>
#include <unistd.h>

akx_cell_t *os_cwd_impl(akx_runtime_ctx_t *rt, akx_cell_t *args) {
  (void)args;

  char cwd[PATH_MAX];
  if (getcwd(cwd, sizeof(cwd)) == NULL) {
    akx_rt_error(rt, "os/cwd: failed to get current working directory");
    return NULL;
  }

  akx_cell_t *result = akx_rt_alloc_cell(rt, AKX_TYPE_STRING_LITERAL);
  akx_rt_set_string(rt, result, cwd);
  return result;
}
