#ifndef AKX_REPL_INPUT_H
#define AKX_REPL_INPUT_H

typedef struct repl_input_ctx_t repl_input_ctx_t;

repl_input_ctx_t *repl_input_init(void);
void repl_input_deinit(repl_input_ctx_t *ctx);
char *repl_input_read_line(repl_input_ctx_t *ctx, const char *prompt);

#endif
