#include "repl_history.h"
#include <ak24/kernel.h>
#include <stdio.h>
#include <string.h>

repl_history_t *repl_history_new(size_t max_entries) {
  repl_history_t *hist = AK24_ALLOC(sizeof(repl_history_t));
  if (!hist) {
    return NULL;
  }

  list_init(&hist->entries);
  hist->max_entries = max_entries;
  hist->current_index = 0;

  return hist;
}

void repl_history_free(repl_history_t *hist) {
  if (!hist) {
    return;
  }

  list_iter_t iter = list_iter(&hist->entries);
  char **entry;
  while ((entry = list_next(&hist->entries, &iter))) {
    AK24_FREE(*entry);
  }
  list_deinit(&hist->entries);

  AK24_FREE(hist);
}

void repl_history_add(repl_history_t *hist, const char *entry) {
  if (!hist || !entry || strlen(entry) == 0) {
    return;
  }

  char *entry_copy = AK24_ALLOC(strlen(entry) + 1);
  if (!entry_copy) {
    return;
  }
  strcpy(entry_copy, entry);

  if (list_count(&hist->entries) >= hist->max_entries) {
    char **oldest = list_get(&hist->entries, 0);
    if (oldest) {
      AK24_FREE(*oldest);
    }
    list_remove(&hist->entries, 0);
  }

  list_push(&hist->entries, entry_copy);
  hist->current_index = list_count(&hist->entries);
}

const char *repl_history_prev(repl_history_t *hist) {
  if (!hist || list_count(&hist->entries) == 0) {
    return NULL;
  }

  if (hist->current_index > 0) {
    hist->current_index--;
  }

  char **entry = list_get(&hist->entries, hist->current_index);
  return entry ? *entry : NULL;
}

const char *repl_history_next(repl_history_t *hist) {
  if (!hist || list_count(&hist->entries) == 0) {
    return NULL;
  }

  if (hist->current_index < list_count(&hist->entries) - 1) {
    hist->current_index++;
    char **entry = list_get(&hist->entries, hist->current_index);
    return entry ? *entry : NULL;
  }

  hist->current_index = list_count(&hist->entries);
  return NULL;
}

void repl_history_save(repl_history_t *hist, const char *filepath) {
  if (!hist || !filepath) {
    return;
  }

  FILE *f = fopen(filepath, "w");
  if (!f) {
    return;
  }

  list_iter_t iter = list_iter(&hist->entries);
  char **entry;
  while ((entry = list_next(&hist->entries, &iter))) {
    fprintf(f, "%s\n", *entry);
  }

  fclose(f);
}

void repl_history_load(repl_history_t *hist, const char *filepath) {
  if (!hist || !filepath) {
    return;
  }

  FILE *f = fopen(filepath, "r");
  if (!f) {
    return;
  }

  char line[4096];
  while (fgets(line, sizeof(line), f)) {
    size_t len = strlen(line);
    if (len > 0 && line[len - 1] == '\n') {
      line[len - 1] = '\0';
    }
    if (len > 0) {
      repl_history_add(hist, line);
    }
  }

  fclose(f);
  hist->current_index = list_count(&hist->entries);
}
