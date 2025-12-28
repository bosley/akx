#ifndef AKX_FS_MODULE_H
#define AKX_FS_MODULE_H

#include "akx_cell.h"
#include "akx_rt.h"
#include <ak24/kernel.h>
#include <stdio.h>

#define FS_READ_BUFFER_SIZE 4096

typedef struct {
  FILE *fp;
  char *path;
  char *mode;
  long position;
} file_handle_t;

typedef struct {
  int next_fid;
  map_int_t file_handles;
} fs_module_data_t;

void fs_ensure_init(akx_runtime_ctx_t *rt);

file_handle_t *fs_get_handle(akx_runtime_ctx_t *rt, int fid);
int fs_add_handle(akx_runtime_ctx_t *rt, file_handle_t *handle);
void fs_remove_handle(akx_runtime_ctx_t *rt, int fid);

#endif
