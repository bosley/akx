#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libtcc.h>

const char *get_abi_header() {
    return "#include <stddef.h>\n"
           "#include <stdint.h>\n"
           "#include <stdio.h>\n"
           "#include <stdlib.h>\n"
           "#include <string.h>\n"
           "\n"
           "typedef struct akx_runtime_ctx_t akx_runtime_ctx_t;\n"
           "typedef struct ak_context_t ak_context_t;\n"
           "typedef struct ak_lambda_t ak_lambda_t;\n"
           "typedef struct ak_buffer_t ak_buffer_t;\n"
           "typedef struct ak_source_range_t ak_source_range_t;\n"
           "\n"
           "typedef enum {\n"
           "  AKX_TYPE_SYMBOL = 0,\n"
           "  AKX_TYPE_STRING_LITERAL = 1,\n"
           "  AKX_TYPE_INTEGER_LITERAL = 2,\n"
           "  AKX_TYPE_REAL_LITERAL = 3,\n"
           "  AKX_TYPE_LIST = 4,\n"
           "  AKX_TYPE_LIST_SQUARE = 5,\n"
           "  AKX_TYPE_LIST_CURLY = 6,\n"
           "  AKX_TYPE_LIST_TEMPLE = 7,\n"
           "  AKX_TYPE_QUOTED = 8,\n"
           "  AKX_TYPE_LAMBDA = 9\n"
           "} akx_type_t;\n"
           "\n"
           "typedef struct akx_cell_t {\n"
           "  akx_type_t type;\n"
           "  struct akx_cell_t *next;\n"
           "  union {\n"
           "    const char *symbol;\n"
           "    int integer_literal;\n"
           "    double real_literal;\n"
           "    ak_buffer_t *string_literal;\n"
           "    struct akx_cell_t *list_head;\n"
           "    ak_buffer_t *quoted_literal;\n"
           "    ak_lambda_t *lambda;\n"
           "  } value;\n"
           "  ak_source_range_t *sourceloc;\n"
           "} akx_cell_t;\n";
}

int main() {
    TCCState *s = tcc_new();
    if (!s) {
        fprintf(stderr, "Could not create TCC state\n");
        return 1;
    }
    
    tcc_set_output_type(s, TCC_OUTPUT_MEMORY);
    
    FILE *f = fopen("nucleus/math/div.c", "r");
    if (!f) {
        fprintf(stderr, "Could not open div.c\n");
        tcc_delete(s);
        return 1;
    }
    
    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    char *div_source = malloc(fsize + 1);
    fread(div_source, 1, fsize, f);
    fclose(f);
    div_source[fsize] = 0;
    
    const char *abi = get_abi_header();
    size_t abi_len = strlen(abi);
    size_t total_len = abi_len + fsize + 1;
    char *combined = malloc(total_len);
    strcpy(combined, abi);
    strcat(combined, div_source);
    
    printf("Compiling div.c with ABI header...\n");
    if (tcc_compile_string(s, combined) == -1) {
        fprintf(stderr, "Compilation failed\n");
        free(div_source);
        free(combined);
        tcc_delete(s);
        return 1;
    }
    
    printf("Compilation succeeded!\n");
    free(div_source);
    free(combined);
    tcc_delete(s);
    return 0;
}

