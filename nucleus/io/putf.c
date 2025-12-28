akx_cell_t *putf_impl(akx_runtime_ctx_t *rt, akx_cell_t *args) {
  if (!args) {
    akx_rt_error(rt, "io/putf requires at least a format string");
    return NULL;
  }

  akx_cell_t *format_cell = akx_rt_eval(rt, args);
  if (!format_cell ||
      !akx_rt_cell_is_type(format_cell, AKX_TYPE_STRING_LITERAL)) {
    akx_rt_error(rt, "io/putf: first argument must be a string");
    if (format_cell)
      akx_rt_free_cell(rt, format_cell);
    return NULL;
  }

  const char *format = akx_rt_cell_as_string(format_cell);
  akx_cell_t *current_arg = akx_rt_cell_next(args);
  int char_count = 0;

  for (const char *p = format; *p; p++) {
    if (*p == '\\') {
      p++;
      if (*p == 'n') {
        putchar('\n');
        char_count++;
      } else if (*p == 't') {
        putchar('\t');
        char_count++;
      } else if (*p == '\\') {
        putchar('\\');
        char_count++;
      } else if (*p == '"') {
        putchar('"');
        char_count++;
      } else if (*p == 'r') {
        putchar('\r');
        char_count++;
      } else {
        putchar(*p);
        char_count++;
      }
    } else if (*p == '%') {
      p++;
      if (*p == '%') {
        putchar('%');
        char_count++;
        continue;
      }

      if (!current_arg) {
        akx_rt_error(rt, "io/putf: not enough arguments for format string");
        akx_rt_free_cell(rt, format_cell);
        return NULL;
      }

      akx_cell_t *arg = akx_rt_eval(rt, current_arg);
      if (!arg) {
        akx_rt_free_cell(rt, format_cell);
        return NULL;
      }

      akx_type_t arg_type = akx_rt_cell_get_type(arg);

      if (*p == 'd') {
        if (arg_type == AKX_TYPE_INTEGER_LITERAL) {
          int val = akx_rt_cell_as_int(arg);
          char buf[32];
          int len = snprintf(buf, sizeof(buf), "%d", val);
          printf("%s", buf);
          char_count += len;
        } else {
          akx_rt_error(rt, "io/putf: %d expects integer");
          akx_rt_free_cell(rt, arg);
          akx_rt_free_cell(rt, format_cell);
          return NULL;
        }
      } else if (*p == 'f') {
        if (arg_type == AKX_TYPE_REAL_LITERAL) {
          double val = akx_rt_cell_as_real(arg);
          char buf[64];
          int len = snprintf(buf, sizeof(buf), "%f", val);
          printf("%s", buf);
          char_count += len;
        } else {
          akx_rt_error(rt, "io/putf: %f expects real");
          akx_rt_free_cell(rt, arg);
          akx_rt_free_cell(rt, format_cell);
          return NULL;
        }
      } else if (*p == 's') {
        if (arg_type == AKX_TYPE_STRING_LITERAL) {
          const char *str = akx_rt_cell_as_string(arg);
          printf("%s", str);
          char_count += strlen(str);
        } else {
          akx_rt_error(rt, "io/putf: %s expects string");
          akx_rt_free_cell(rt, arg);
          akx_rt_free_cell(rt, format_cell);
          return NULL;
        }
      } else if (*p == 'y') {
        if (arg_type == AKX_TYPE_SYMBOL) {
          const char *sym = akx_rt_cell_as_symbol(arg);
          printf("%s", sym);
          char_count += strlen(sym);
        } else {
          akx_rt_error(rt, "io/putf: %y expects symbol");
          akx_rt_free_cell(rt, arg);
          akx_rt_free_cell(rt, format_cell);
          return NULL;
        }
      } else if (*p == 'l') {
        if (arg_type == AKX_TYPE_LAMBDA) {
          printf("<lambda>");
          char_count += 8;
        } else {
          akx_rt_error(rt, "io/putf: %l expects lambda");
          akx_rt_free_cell(rt, arg);
          akx_rt_free_cell(rt, format_cell);
          return NULL;
        }
      } else if (*p == 'q') {
        if (arg_type == AKX_TYPE_QUOTED) {
          printf("'");
          char_count += 1;
        } else {
          akx_rt_error(rt, "io/putf: %q expects quoted");
          akx_rt_free_cell(rt, arg);
          akx_rt_free_cell(rt, format_cell);
          return NULL;
        }
      } else if (*p == 'L') {
        if (arg_type == AKX_TYPE_LIST || arg_type == AKX_TYPE_LIST_SQUARE ||
            arg_type == AKX_TYPE_LIST_CURLY ||
            arg_type == AKX_TYPE_LIST_TEMPLE) {
          printf("<list>");
          char_count += 6;
        } else {
          akx_rt_error(rt, "io/putf: %L expects list");
          akx_rt_free_cell(rt, arg);
          akx_rt_free_cell(rt, format_cell);
          return NULL;
        }
      } else if (*p == 't') {
        const char *type_name = "unknown";
        switch (arg_type) {
        case AKX_TYPE_SYMBOL:
          type_name = "symbol";
          break;
        case AKX_TYPE_STRING_LITERAL:
          type_name = "string";
          break;
        case AKX_TYPE_INTEGER_LITERAL:
          type_name = "integer";
          break;
        case AKX_TYPE_REAL_LITERAL:
          type_name = "real";
          break;
        case AKX_TYPE_LIST:
          type_name = "list";
          break;
        case AKX_TYPE_LIST_SQUARE:
          type_name = "list-square";
          break;
        case AKX_TYPE_LIST_CURLY:
          type_name = "list-curly";
          break;
        case AKX_TYPE_LIST_TEMPLE:
          type_name = "list-temple";
          break;
        case AKX_TYPE_QUOTED:
          type_name = "quoted";
          break;
        case AKX_TYPE_LAMBDA:
          type_name = "lambda";
          break;
        }
        printf("%s", type_name);
        char_count += strlen(type_name);
      } else if (*p == 'v') {
        switch (arg_type) {
        case AKX_TYPE_SYMBOL: {
          const char *sym = akx_rt_cell_as_symbol(arg);
          printf("%s", sym);
          char_count += strlen(sym);
          break;
        }
        case AKX_TYPE_STRING_LITERAL: {
          const char *str = akx_rt_cell_as_string(arg);
          printf("%s", str);
          char_count += strlen(str);
          break;
        }
        case AKX_TYPE_INTEGER_LITERAL: {
          int val = akx_rt_cell_as_int(arg);
          char buf[32];
          int len = snprintf(buf, sizeof(buf), "%d", val);
          printf("%s", buf);
          char_count += len;
          break;
        }
        case AKX_TYPE_REAL_LITERAL: {
          double val = akx_rt_cell_as_real(arg);
          char buf[64];
          int len = snprintf(buf, sizeof(buf), "%f", val);
          printf("%s", buf);
          char_count += len;
          break;
        }
        case AKX_TYPE_LAMBDA: {
          printf("<lambda>");
          char_count += 8;
          break;
        }
        case AKX_TYPE_LIST:
        case AKX_TYPE_LIST_SQUARE:
        case AKX_TYPE_LIST_CURLY:
        case AKX_TYPE_LIST_TEMPLE: {
          printf("<list>");
          char_count += 6;
          break;
        }
        case AKX_TYPE_QUOTED: {
          printf("'");
          char_count += 1;
          break;
        }
        }
      } else {
        akx_rt_error_fmt(rt, "io/putf: unknown format specifier '%%%c'", *p);
        akx_rt_free_cell(rt, arg);
        akx_rt_free_cell(rt, format_cell);
        return NULL;
      }

      if (arg_type != AKX_TYPE_LAMBDA) {
        akx_rt_free_cell(rt, arg);
      }
      current_arg = akx_rt_cell_next(current_arg);
    } else {
      putchar(*p);
      char_count++;
    }
  }

  akx_rt_free_cell(rt, format_cell);

  akx_cell_t *result = akx_rt_alloc_cell(rt, AKX_TYPE_INTEGER_LITERAL);
  akx_rt_set_int(rt, result, char_count);
  return result;
}
