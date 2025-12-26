#ifndef AKX_SV_H
#define AKX_SV_H

#include <ak24/sourceloc.h>

typedef enum {
  AKX_ERROR_LEVEL_ERROR,
  AKX_ERROR_LEVEL_WARNING,
  AKX_ERROR_LEVEL_INFO,
} akx_error_level_t;

void akx_sv_show_error(ak_source_range_t *range, akx_error_level_t level,
                       const char *message);

void akx_sv_show_location(ak_source_loc_t *loc, akx_error_level_t level,
                          const char *message);

#endif
