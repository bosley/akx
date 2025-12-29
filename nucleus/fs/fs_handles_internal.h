#ifndef FS_HANDLES_INTERNAL_H
#define FS_HANDLES_INTERNAL_H

#include <stdio.h>

typedef struct {
  FILE *fp;
  char *path;
  char *mode;
  long position;
} file_handle_t;

static int global_next_fid = 1;

static inline char *fid_to_key(int fid) {
  static char key[32];
  snprintf(key, sizeof(key), "__fs_handle_%d", fid);
  return key;
}

#endif
