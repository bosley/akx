#ifndef AKX_CORE_H
#define AKX_CORE_H

#include <ak24/kernel.h>

typedef struct akx_core_t akx_core_t;

akx_core_t *akx_core_init(void);

void akx_core_deinit(akx_core_t *core);

#endif
