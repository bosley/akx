#ifndef AKX_REPL_HISTORY_H
#define AKX_REPL_HISTORY_H

#include <ak24/list.h>
#include <stddef.h>

typedef struct {
  list_str_t entries;
  size_t max_entries;
  size_t current_index;
} repl_history_t;

repl_history_t *repl_history_new(size_t max_entries);
void repl_history_free(repl_history_t *hist);
void repl_history_add(repl_history_t *hist, const char *entry);
const char *repl_history_prev(repl_history_t *hist);
const char *repl_history_next(repl_history_t *hist);
void repl_history_save(repl_history_t *hist, const char *filepath);
void repl_history_load(repl_history_t *hist, const char *filepath);

#endif
