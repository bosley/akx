#include <stddef.h>
#include <string.h>

akx_cell_t *form_define(akx_runtime_ctx_t *rt, akx_cell_t *args) {
  if (akx_rt_list_length(args) < 2) {
    akx_rt_error(rt, "form-define requires at least 2 arguments (:name spec)");
    return NULL;
  }

  akx_cell_t *name_cell = akx_rt_list_nth(args, 0);
  const char *form_name = NULL;

  if (name_cell->type == AKX_TYPE_SYMBOL) {
    const char *sym = akx_rt_cell_as_symbol(name_cell);
    if (sym && sym[0] == ':') {
      form_name = sym + 1;
    } else {
      akx_rt_error(
          rt, "form-define: name must be a keyword (symbol starting with ':')");
      return NULL;
    }
  } else {
    akx_rt_error(rt, "form-define: name must be a keyword");
    return NULL;
  }

  akx_cell_t *spec_cell = akx_rt_list_nth(args, 1);

  ak_form_t *form = NULL;

  if (spec_cell->type == AKX_TYPE_LIST) {
    akx_cell_t *head = spec_cell->value.list_head;
    if (!head || head->type != AKX_TYPE_SYMBOL) {
      akx_rt_error(rt,
                   "form-define: list form must start with modifier symbol");
      return NULL;
    }

    const char *modifier = akx_rt_cell_as_symbol(head);
    akx_cell_t *inner_spec = head->next;

    if (!inner_spec) {
      akx_rt_error(rt, "form-define: modifier requires inner type");
      return NULL;
    }

    ak_form_t *inner_form = NULL;
    if (inner_spec->type == AKX_TYPE_SYMBOL) {
      const char *inner_name = akx_rt_cell_as_symbol(inner_spec);
      if (inner_name && inner_name[0] == ':') {
        inner_name = inner_name + 1;
      }
      inner_form = akx_rt_lookup_form(rt, inner_name);
    }

    if (!inner_form) {
      akx_rt_error(rt, "form-define: inner form not found");
      return NULL;
    }

    if (strcmp(modifier, "optional") == 0) {
      form = ak_form_new_optional(inner_form);
    } else if (strcmp(modifier, "repeatable") == 0) {
      form = ak_form_new_repeatable(inner_form);
    } else if (strcmp(modifier, "list") == 0) {
      form = ak_form_new_list(inner_form);
    } else {
      akx_rt_error_fmt(rt, "form-define: unknown modifier '%s'", modifier);
      return NULL;
    }
  } else if (spec_cell->type == AKX_TYPE_SYMBOL) {
    const char *type_name = akx_rt_cell_as_symbol(spec_cell);
    if (type_name && type_name[0] == ':') {
      type_name = type_name + 1;
    }

    if (strcmp(type_name, "int") == 0) {
      form = ak_form_new_primitive(AK_FORM_PRIMITIVE_I64);
    } else if (strcmp(type_name, "real") == 0) {
      form = ak_form_new_primitive(AK_FORM_PRIMITIVE_F64);
    } else if (strcmp(type_name, "byte") == 0) {
      form = ak_form_new_primitive(AK_FORM_PRIMITIVE_U8);
    } else if (strcmp(type_name, "string") == 0) {
      form = ak_form_new_list(ak_form_new_primitive(AK_FORM_PRIMITIVE_CHAR));
    } else {
      ak_form_t *existing = akx_rt_lookup_form(rt, type_name);
      if (existing) {
        form = ak_form_new_named(form_name, existing);
      }
    }
  } else if (spec_cell->type == AKX_TYPE_LIST_CURLY) {
    list_void_t parts;
    list_init(&parts);

    akx_cell_t *current = spec_cell->value.list_head;
    while (current) {
      if (current->type == AKX_TYPE_SYMBOL) {
        const char *part_name = akx_rt_cell_as_symbol(current);
        if (part_name && part_name[0] == ':') {
          part_name = part_name + 1;
        }
        ak_form_t *part_form = akx_rt_lookup_form(rt, part_name);
        if (part_form) {
          list_push(&parts, part_form);
        }
      }
      current = current->next;
    }

    form = ak_form_new_compound(&parts);
    list_deinit(&parts);
  } else if (spec_cell->type == AKX_TYPE_LIST_SQUARE) {
    list_void_t parts;
    list_str_t field_names;
    list_init(&parts);
    list_init(&field_names);

    akx_cell_t *current = spec_cell->value.list_head;
    while (current && current->next) {
      if (current->type == AKX_TYPE_SYMBOL) {
        const char *field_name = akx_rt_cell_as_symbol(current);
        if (field_name && field_name[0] == ':') {
          field_name = field_name + 1;
        }
        list_push(&field_names, (void *)field_name);

        current = current->next;
        if (current && current->type == AKX_TYPE_SYMBOL) {
          const char *type_name = akx_rt_cell_as_symbol(current);
          if (type_name && type_name[0] == ':') {
            type_name = type_name + 1;
          }
          ak_form_t *field_form = akx_rt_lookup_form(rt, type_name);
          if (field_form) {
            list_push(&parts, field_form);
          }
        }
      }
      current = current->next;
    }

    if (list_count(&parts) > 0) {
      ak_form_t *pattern = ak_form_new_compound(&parts);
      form = ak_form_new_struct(pattern, &field_names);
    }

    list_deinit(&parts);
    list_deinit(&field_names);
  }

  if (!form) {
    akx_rt_error(rt, "failed to create form");
    return NULL;
  }

  if (akx_rt_register_form(rt, form_name, form) != 0) {
    ak_form_free(form);
    akx_rt_error(rt, "failed to register form");
    return NULL;
  }

  akx_cell_t *result = akx_rt_alloc_cell(rt, AKX_TYPE_SYMBOL);
  akx_rt_set_symbol(rt, result, form_name);
  return result;
}
