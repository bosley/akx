#include "fs_module.h"
#include <stdlib.h>
#include <string.h>

static fs_module_data_t *global_fs_data = NULL;

akx_cell_t *fs_lifecycle_dummy(akx_runtime_ctx_t *rt, akx_cell_t *args) {
  akx_cell_t *result = akx_rt_alloc_cell(rt, AKX_TYPE_SYMBOL);
  akx_rt_set_symbol(rt, result, "nil");
  return result;
}

void fs_ensure_init(akx_runtime_ctx_t *rt) {
  if (global_fs_data) {
    return;
  }

  global_fs_data = AK24_ALLOC(sizeof(fs_module_data_t));
  if (!global_fs_data) {
    return;
  }

  global_fs_data->next_fid = 1;
  map_init(&global_fs_data->file_handles);
}

file_handle_t *fs_get_handle(akx_runtime_ctx_t *rt, int fid) {
  if (!global_fs_data) {
    return NULL;
  }

  void **handle_ptr = map_get_generic(&global_fs_data->file_handles, &fid);
  if (!handle_ptr || !*handle_ptr) {
    return NULL;
  }

  return (file_handle_t *)*handle_ptr;
}

int fs_add_handle(akx_runtime_ctx_t *rt, file_handle_t *handle) {
  if (!global_fs_data) {
    return -1;
  }

  int fid = global_fs_data->next_fid++;
  map_set_generic(&global_fs_data->file_handles, &fid, handle);
  return fid;
}

void fs_remove_handle(akx_runtime_ctx_t *rt, int fid) {
  if (!global_fs_data) {
    return;
  }

  void **handle_ptr = map_get_generic(&global_fs_data->file_handles, &fid);
  if (handle_ptr && *handle_ptr) {
    file_handle_t *handle = (file_handle_t *)*handle_ptr;
    if (handle->fp) {
      fclose(handle->fp);
    }
    if (handle->path) {
      AK24_FREE(handle->path);
    }
    if (handle->mode) {
      AK24_FREE(handle->mode);
    }
    AK24_FREE(handle);
  }

  map_remove(&global_fs_data->file_handles, &fid);
}
