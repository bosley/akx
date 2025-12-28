#include "nucleus_list.h"
#include <ak24/buffer.h>
#include <ak24/filepath.h>
#include <ak24/kernel.h>
#include <ak24/list.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef AK24_PLATFORM_WINDOWS
#include <windows.h>
#else
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#endif

static int ends_with(const char *str, const char *suffix) {
  if (!str || !suffix) {
    return 0;
  }
  size_t str_len = strlen(str);
  size_t suffix_len = strlen(suffix);
  if (suffix_len > str_len) {
    return 0;
  }
  return strcmp(str + str_len - suffix_len, suffix) == 0;
}

static int compare_strings(const void *a, const void *b) {
  const char *str_a = *(const char **)a;
  const char *str_b = *(const char **)b;
  return strcmp(str_a, str_b);
}

#ifdef AK24_PLATFORM_WINDOWS

static void scan_directory_recursive(const char *base_path,
                                     const char *relative_path,
                                     list_str_t *files) {
  char search_path[MAX_PATH];
  if (relative_path && relative_path[0]) {
    snprintf(search_path, MAX_PATH, "%s\\%s\\*", base_path, relative_path);
  } else {
    snprintf(search_path, MAX_PATH, "%s\\*", base_path);
  }

  WIN32_FIND_DATAA find_data;
  HANDLE hFind = FindFirstFileA(search_path, &find_data);

  if (hFind == INVALID_HANDLE_VALUE) {
    return;
  }

  do {
    if (strcmp(find_data.cFileName, ".") == 0 ||
        strcmp(find_data.cFileName, "..") == 0) {
      continue;
    }

    char rel_path[MAX_PATH];
    if (relative_path && relative_path[0]) {
      snprintf(rel_path, MAX_PATH, "%s\\%s", relative_path,
               find_data.cFileName);
    } else {
      snprintf(rel_path, MAX_PATH, "%s", find_data.cFileName);
    }

    if (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
      scan_directory_recursive(base_path, rel_path, files);
    } else {
      if (ends_with(find_data.cFileName, ".c")) {
        char *file_copy = AK24_ALLOC(strlen(rel_path) + 1);
        if (file_copy) {
          strcpy(file_copy, rel_path);
          list_push(files, file_copy);
        }
      }
    }
  } while (FindNextFileA(hFind, &find_data));

  FindClose(hFind);
}

#else

static void scan_directory_recursive(const char *base_path,
                                     const char *relative_path,
                                     list_str_t *files) {
  char full_path[1024];
  if (relative_path && relative_path[0]) {
    snprintf(full_path, sizeof(full_path), "%s/%s", base_path, relative_path);
  } else {
    snprintf(full_path, sizeof(full_path), "%s", base_path);
  }

  DIR *dir = opendir(full_path);
  if (!dir) {
    return;
  }

  struct dirent *entry;
  while ((entry = readdir(dir)) != NULL) {
    if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
      continue;
    }

    char rel_path[1024];
    if (relative_path && relative_path[0]) {
      snprintf(rel_path, sizeof(rel_path), "%s/%s", relative_path,
               entry->d_name);
    } else {
      snprintf(rel_path, sizeof(rel_path), "%s", entry->d_name);
    }

    char entry_full_path[1024];
    snprintf(entry_full_path, sizeof(entry_full_path), "%s/%s", full_path,
             entry->d_name);

    struct stat st;
    if (stat(entry_full_path, &st) == 0) {
      if (S_ISDIR(st.st_mode)) {
        scan_directory_recursive(base_path, rel_path, files);
      } else if (S_ISREG(st.st_mode)) {
        if (ends_with(entry->d_name, ".c")) {
          char *file_copy = AK24_ALLOC(strlen(rel_path) + 1);
          if (file_copy) {
            strcpy(file_copy, rel_path);
            list_push(files, file_copy);
          }
        }
      }
    }
  }

  closedir(dir);
}

#endif

int akx_nucleus_list(void) {
  ak_buffer_t *home = ak_filepath_home();
  if (!home) {
    printf("Error: Could not determine home directory\n");
    return 1;
  }

  ak_buffer_t *nucleus_path = ak_filepath_join(2, home->data, ".akx/nucleus");
  ak_buffer_free(home);

  if (!nucleus_path) {
    printf("Error: Could not construct nucleus path\n");
    return 1;
  }

  list_str_t files;
  list_init(&files);

  scan_directory_recursive((const char *)nucleus_path->data, "", &files);

  size_t count = list_count(&files);

  printf("\nAvailable Nuclei in %s (%zu files):\n\n",
         (const char *)nucleus_path->data, count);

  if (count == 0) {
    printf("  (no nucleus files found)\n\n");
  } else {
    char **files_array = AK24_ALLOC(sizeof(char *) * count);
    if (!files_array) {
      printf("Error: Failed to allocate files array\n");
      list_iter_t iter = list_iter(&files);
      char **file;
      while ((file = list_next(&files, &iter))) {
        AK24_FREE(*file);
      }
      list_deinit(&files);
      ak_buffer_free(nucleus_path);
      return 1;
    }

    list_iter_t iter = list_iter(&files);
    char **file;
    size_t i = 0;
    while ((file = list_next(&files, &iter))) {
      files_array[i++] = *file;
    }

    qsort(files_array, count, sizeof(char *), compare_strings);

    size_t max_len = 0;
    for (i = 0; i < count; i++) {
      size_t len = strlen(files_array[i]);
      if (len > max_len) {
        max_len = len;
      }
    }

    const size_t columns = 3;
    const size_t col_width = max_len + 2;

    for (i = 0; i < count; i++) {
      if (i % columns == 0) {
        printf("  ");
      }
      printf("%-*s", (int)col_width, files_array[i]);
      if ((i + 1) % columns == 0 || i == count - 1) {
        printf("\n");
      }
    }

    printf("\n");

    AK24_FREE(files_array);
  }

  list_iter_t iter = list_iter(&files);
  char **file;
  while ((file = list_next(&files, &iter))) {
    AK24_FREE(*file);
  }
  list_deinit(&files);

  ak_buffer_free(nucleus_path);

  return 0;
}
