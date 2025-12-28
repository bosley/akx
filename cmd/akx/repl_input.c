#include "repl_input.h"
#include <ak24/kernel.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef AK24_PLATFORM_POSIX
#include <termios.h>
#include <unistd.h>
#endif

#ifdef AK24_PLATFORM_WINDOWS
#include <conio.h>
#include <windows.h>
#endif

struct repl_input_ctx_t {
#ifdef AK24_PLATFORM_POSIX
  struct termios orig_termios;
  int tty_fd;
#endif
#ifdef AK24_PLATFORM_WINDOWS
  HANDLE hStdin;
  DWORD orig_mode;
#endif
  int initialized;
};

repl_input_ctx_t *repl_input_init(void) {
  repl_input_ctx_t *ctx = AK24_ALLOC(sizeof(repl_input_ctx_t));
  if (!ctx) {
    return NULL;
  }

  memset(ctx, 0, sizeof(repl_input_ctx_t));

#ifdef AK24_PLATFORM_POSIX
  ctx->tty_fd = STDIN_FILENO;
  if (isatty(ctx->tty_fd)) {
    ctx->initialized = 1;
  }
#endif

#ifdef AK24_PLATFORM_WINDOWS
  ctx->hStdin = GetStdHandle(STD_INPUT_HANDLE);
  if (ctx->hStdin != INVALID_HANDLE_VALUE) {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    ctx->initialized = 1;
  }
#endif

  return ctx;
}

void repl_input_deinit(repl_input_ctx_t *ctx) {
  if (!ctx) {
    return;
  }
  AK24_FREE(ctx);
}

char *repl_input_read_line(repl_input_ctx_t *ctx, const char *prompt) {
  if (!ctx) {
    return NULL;
  }

  if (prompt) {
    printf("%s", prompt);
    fflush(stdout);
  }

  char *line = NULL;
  size_t len = 0;
  ssize_t read;

#ifdef AK24_PLATFORM_POSIX
  read = getline(&line, &len, stdin);
  if (read == -1) {
    if (line) {
      free(line);
    }
    return NULL;
  }

  if (read > 0 && line[read - 1] == '\n') {
    line[read - 1] = '\0';
    read--;
  }

  char *result = AK24_ALLOC(read + 1);
  if (!result) {
    free(line);
    return NULL;
  }
  memcpy(result, line, read);
  result[read] = '\0';
  free(line);
  return result;
#endif

#ifdef AK24_PLATFORM_WINDOWS
  wchar_t wbuf[4096];
  DWORD chars_read;

  if (!ReadConsoleW(ctx->hStdin, wbuf, 4096, &chars_read, NULL)) {
    return NULL;
  }

  if (chars_read > 0 && wbuf[chars_read - 1] == L'\n') {
    chars_read--;
  }
  if (chars_read > 0 && wbuf[chars_read - 1] == L'\r') {
    chars_read--;
  }
  wbuf[chars_read] = L'\0';

  int utf8_len =
      WideCharToMultiByte(CP_UTF8, 0, wbuf, chars_read, NULL, 0, NULL, NULL);
  if (utf8_len <= 0) {
    return NULL;
  }

  char *result = AK24_ALLOC(utf8_len + 1);
  if (!result) {
    return NULL;
  }

  WideCharToMultiByte(CP_UTF8, 0, wbuf, chars_read, result, utf8_len, NULL,
                      NULL);
  result[utf8_len] = '\0';
  return result;
#endif

#if !defined(AK24_PLATFORM_POSIX) && !defined(AK24_PLATFORM_WINDOWS)
  size_t capacity = 256;
  line = AK24_ALLOC(capacity);
  if (!line) {
    return NULL;
  }

  if (!fgets(line, capacity, stdin)) {
    AK24_FREE(line);
    return NULL;
  }

  len = strlen(line);
  if (len > 0 && line[len - 1] == '\n') {
    line[len - 1] = '\0';
  }

  return line;
#endif
}
