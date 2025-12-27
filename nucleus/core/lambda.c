static void akx_lambda_context_free(void *ctx) {
  if (!ctx) {
    return;
  }
  akx_lambda_context_t *lambda_ctx = (akx_lambda_context_t *)ctx;
  if (lambda_ctx->param_names) {
    AK24_FREE(lambda_ctx->param_names);
  }
  if (lambda_ctx->body) {
    akx_cell_free(lambda_ctx->body);
  }
  AK24_FREE(lambda_ctx);
}

static void akx_lambda_invoke_impl(void *captured_ctx, void *invoke_args) {
  if (!captured_ctx) {
    return;
  }

  akx_lambda_context_t *lambda_ctx = (akx_lambda_context_t *)captured_ctx;
  akx_runtime_ctx_t *rt = lambda_ctx->rt;
  akx_cell_t *args = (akx_cell_t *)invoke_args;

  lambda_ctx->result = NULL;

  size_t arg_count = akx_rt_list_length(args);
  if (arg_count != lambda_ctx->param_count) {
    akx_rt_error_fmt(rt, "lambda: expected %zu arguments, got %zu",
                     lambda_ctx->param_count, arg_count);
    return;
  }

  for (size_t i = 0; i < lambda_ctx->param_count; i++) {
    akx_cell_t *arg = akx_rt_list_nth(args, i);
    akx_cell_t *evaled = akx_rt_eval(rt, arg);
    if (!evaled) {
      return;
    }
    akx_rt_scope_set(rt, lambda_ctx->param_names[i], evaled);
  }

  akx_cell_t *result = NULL;
  akx_cell_t *current = lambda_ctx->body;
  while (current) {
    if (result) {
      akx_cell_free(result);
    }
    result = akx_rt_eval(rt, current);
    if (!result) {
      return;
    }
    current = current->next;
  }

  lambda_ctx->result = result;
}

akx_cell_t *lambda_impl(akx_runtime_ctx_t *rt, akx_cell_t *args) {
  akx_cell_t *params_cell = akx_rt_list_nth(args, 0);
  if (!params_cell || params_cell->type != AKX_TYPE_LIST_SQUARE) {
    akx_rt_error(rt, "lambda: first argument must be a square-bracket list of "
                     "parameter names");
    return NULL;
  }

  akx_cell_t *params = params_cell->value.list_head;
  size_t param_count = akx_rt_list_length(params);

  const char **param_names = NULL;
  if (param_count > 0) {
    param_names = AK24_ALLOC(sizeof(char *) * param_count);
    if (!param_names) {
      akx_rt_error(rt, "lambda: failed to allocate parameter names");
      return NULL;
    }

    akx_cell_t *current = params;
    for (size_t i = 0; i < param_count; i++) {
      if (!current || current->type != AKX_TYPE_SYMBOL) {
        AK24_FREE(param_names);
        akx_rt_error(rt, "lambda: all parameters must be symbols");
        return NULL;
      }
      param_names[i] = akx_rt_cell_as_symbol(current);
      current = current->next;
    }
  }

  akx_cell_t *body = akx_rt_list_nth(args, 1);
  if (!body) {
    if (param_names) {
      AK24_FREE(param_names);
    }
    akx_rt_error(rt, "lambda: missing body");
    return NULL;
  }

  akx_cell_t *body_clone = NULL;
  akx_cell_t *body_tail = NULL;
  akx_cell_t *current = body;
  while (current) {
    akx_cell_t *cloned = akx_cell_clone(current);
    if (!cloned) {
      if (param_names) {
        AK24_FREE(param_names);
      }
      if (body_clone) {
        akx_cell_free(body_clone);
      }
      akx_rt_error(rt, "lambda: failed to clone body");
      return NULL;
    }
    cloned->next = NULL;
    if (!body_clone) {
      body_clone = cloned;
      body_tail = cloned;
    } else {
      body_tail->next = cloned;
      body_tail = cloned;
    }
    current = current->next;
  }

  akx_lambda_context_t *lambda_ctx = AK24_ALLOC(sizeof(akx_lambda_context_t));
  if (!lambda_ctx) {
    if (param_names) {
      AK24_FREE(param_names);
    }
    if (body_clone) {
      akx_cell_free(body_clone);
    }
    akx_rt_error(rt, "lambda: failed to allocate lambda context");
    return NULL;
  }

  lambda_ctx->rt = rt;
  lambda_ctx->param_names = param_names;
  lambda_ctx->param_count = param_count;
  lambda_ctx->body = body_clone;
  lambda_ctx->result = NULL;

  ak_lambda_t *lambda = ak_lambda_new(akx_lambda_invoke_impl, lambda_ctx,
                                      akx_lambda_context_free);
  if (!lambda) {
    akx_lambda_context_free(lambda_ctx);
    akx_rt_error(rt, "lambda: failed to create lambda");
    return NULL;
  }

  akx_cell_t *lambda_cell = akx_rt_alloc_cell(rt, AKX_TYPE_LAMBDA);
  if (!lambda_cell) {
    ak_lambda_free(lambda);
    akx_rt_error(rt, "lambda: failed to allocate lambda cell");
    return NULL;
  }

  akx_rt_set_lambda(rt, lambda_cell, lambda);

  return lambda_cell;
}
