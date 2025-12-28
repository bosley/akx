akx_cell_t *scanf_impl(akx_runtime_ctx_t *rt, akx_cell_t *args) {
  if (!args) {
    akx_rt_error(rt, "io/scanf requires at least a format string");
    return NULL;
  }

  akx_cell_t *format_cell = akx_rt_eval(rt, args);
  if (!format_cell ||
      !akx_rt_cell_is_type(format_cell, AKX_TYPE_STRING_LITERAL)) {
    akx_rt_error(rt, "io/scanf: first argument must be a string");
    if (format_cell)
      akx_rt_free_cell(rt, format_cell);
    return NULL;
  }

  const char *format = akx_rt_cell_as_string(format_cell);
  akx_cell_t *result_list = NULL;
  akx_cell_t *last_cell = NULL;

  for (const char *p = format; *p; p++) {
    if (*p == '%') {
      p++;
      if (*p == '%') {
        int ch = getchar();
        if (ch != '%') {
          akx_rt_error(rt, "io/scanf: expected '%' in input");
          akx_rt_free_cell(rt, format_cell);
          if (result_list)
            akx_rt_free_cell(rt, result_list);
          return NULL;
        }
        continue;
      }

      akx_cell_t *value = NULL;

      if (*p == 'd') {
        int val;
        if (scanf("%d", &val) != 1) {
          akx_rt_error(rt, "io/scanf: failed to read integer");
          akx_rt_free_cell(rt, format_cell);
          if (result_list)
            akx_rt_free_cell(rt, result_list);
          return NULL;
        }
        value = akx_rt_alloc_cell(rt, AKX_TYPE_INTEGER_LITERAL);
        akx_rt_set_int(rt, value, val);
      } else if (*p == 'f') {
        double val;
        if (scanf("%lf", &val) != 1) {
          akx_rt_error(rt, "io/scanf: failed to read real");
          akx_rt_free_cell(rt, format_cell);
          if (result_list)
            akx_rt_free_cell(rt, result_list);
          return NULL;
        }
        value = akx_rt_alloc_cell(rt, AKX_TYPE_REAL_LITERAL);
        akx_rt_set_real(rt, value, val);
      } else if (*p == 's') {
        char buf[1024];
        if (scanf("%1023s", buf) != 1) {
          akx_rt_error(rt, "io/scanf: failed to read string");
          akx_rt_free_cell(rt, format_cell);
          if (result_list)
            akx_rt_free_cell(rt, result_list);
          return NULL;
        }
        value = akx_rt_alloc_cell(rt, AKX_TYPE_STRING_LITERAL);
        akx_rt_set_string(rt, value, buf);
      } else if (*p == 'c') {
        int ch = getchar();
        if (ch == EOF) {
          akx_rt_error(rt, "io/scanf: failed to read character");
          akx_rt_free_cell(rt, format_cell);
          if (result_list)
            akx_rt_free_cell(rt, result_list);
          return NULL;
        }
        char buf[2] = {(char)ch, '\0'};
        value = akx_rt_alloc_cell(rt, AKX_TYPE_STRING_LITERAL);
        akx_rt_set_string(rt, value, buf);
      } else if (*p == 'l') {
        int ch;
        while ((ch = getchar()) != EOF &&
               (ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r')) {
        }
        if (ch != EOF) {
          ungetc(ch, stdin);
        }

        char buf[2048];
        if (!fgets(buf, sizeof(buf), stdin)) {
          akx_rt_error(rt, "io/scanf: failed to read line");
          akx_rt_free_cell(rt, format_cell);
          if (result_list)
            akx_rt_free_cell(rt, result_list);
          return NULL;
        }
        size_t len = strlen(buf);
        if (len > 0 && buf[len - 1] == '\n') {
          buf[len - 1] = '\0';
        }
        value = akx_rt_alloc_cell(rt, AKX_TYPE_STRING_LITERAL);
        akx_rt_set_string(rt, value, buf);
      } else {
        akx_rt_error_fmt(rt, "io/scanf: unknown format specifier '%%%c'", *p);
        akx_rt_free_cell(rt, format_cell);
        if (result_list)
          akx_rt_free_cell(rt, result_list);
        return NULL;
      }

      if (!result_list) {
        result_list = value;
        last_cell = value;
      } else {
        last_cell->next = value;
        last_cell = value;
      }
    } else if (*p == '\\') {
      p++;
      int expected = 0;
      if (*p == 'n')
        expected = '\n';
      else if (*p == 't')
        expected = '\t';
      else if (*p == '\\')
        expected = '\\';
      else if (*p == 'r')
        expected = '\r';
      else
        expected = *p;

      int ch = getchar();
      if (ch != expected) {
        akx_rt_error_fmt(rt, "io/scanf: expected character '\\%c' in input",
                         *p);
        akx_rt_free_cell(rt, format_cell);
        if (result_list)
          akx_rt_free_cell(rt, result_list);
        return NULL;
      }
    } else if (*p == ' ') {
      int ch;
      while ((ch = getchar()) != EOF &&
             (ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r')) {
      }
      if (ch != EOF) {
        ungetc(ch, stdin);
      }
    } else {
      int ch = getchar();
      if (ch != *p) {
        akx_rt_error_fmt(rt, "io/scanf: expected character '%c' in input", *p);
        akx_rt_free_cell(rt, format_cell);
        if (result_list)
          akx_rt_free_cell(rt, result_list);
        return NULL;
      }
    }
  }

  akx_rt_free_cell(rt, format_cell);

  if (!result_list) {
    result_list = akx_rt_alloc_cell(rt, AKX_TYPE_SYMBOL);
    akx_rt_set_symbol(rt, result_list, "nil");
  }

  return result_list;
}
