#ifndef AKX_H
#define AKX_H

#include "akx_cell.h"
#include "akx_core.h"
#include "akx_rt.h"
#include <ak24/application.h>

int akx_interpret_file(const char *filename, akx_core_t *core,
                       akx_runtime_ctx_t *runtime);

#endif
