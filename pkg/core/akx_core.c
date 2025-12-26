#include "akx_core.h"

struct akx_core_t {
  int initialized;
  ak_cjit_unit_t *cjit;
};

akx_core_t *akx_core_init(void) {
  akx_core_t *core = AK24_ALLOC(sizeof(akx_core_t));
  if (!core) {
    return NULL;
  }

  core->initialized = 1;
  core->cjit = NULL;

  if (!ak_cjit_available()) {
    AK24_LOG_ERROR("CJIT not available");
    AK24_FREE(core);
    return NULL;
  }

  core->cjit = ak_cjit_unit_new(NULL, NULL, NULL);
  if (!core->cjit) {
    AK24_LOG_ERROR("Failed to create CJIT unit");
    AK24_FREE(core);
    return NULL;
  }

  const char *test_code = "int add(int a, int b) { return a + b; }";

  if (ak_cjit_add_source(core->cjit, test_code, "test.c") != AK_CJIT_OK) {
    AK24_LOG_ERROR("Failed to add source to CJIT");
    ak_cjit_unit_free(core->cjit);
    AK24_FREE(core);
    return NULL;
  }

  if (ak_cjit_relocate(core->cjit) != AK_CJIT_OK) {
    AK24_LOG_ERROR("Failed to relocate CJIT unit");
    ak_cjit_unit_free(core->cjit);
    AK24_FREE(core);
    return NULL;
  }

  typedef int (*add_fn)(int, int);
  add_fn add = (add_fn)ak_cjit_get_symbol(core->cjit, "add");
  if (!add) {
    AK24_LOG_ERROR("Failed to get symbol from CJIT");
    ak_cjit_unit_free(core->cjit);
    AK24_FREE(core);
    return NULL;
  }

  int result = add(5, 3);
  if (result != 8) {
    AK24_LOG_ERROR("CJIT function returned wrong result: %d (expected 8)",
                   result);
    ak_cjit_unit_free(core->cjit);
    AK24_FREE(core);
    return NULL;
  }

  AK24_LOG_TRACE("AKX core initialized (CJIT test: 5 + 3 = %d, backend: %s)",
                result, ak_cjit_backend_name());

  return core;
}

void akx_core_deinit(akx_core_t *core) {
  if (!core) {
    return;
  }

  if (core->cjit) {
    ak_cjit_unit_free(core->cjit);
  }

  AK24_LOG_TRACE("AKX core deinitialized");

  AK24_FREE(core);
}
