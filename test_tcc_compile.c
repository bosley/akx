#include <stdio.h>
#include <stdlib.h>
#include <libtcc.h>

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
    
    char *source = malloc(fsize + 1);
    fread(source, 1, fsize, f);
    fclose(f);
    source[fsize] = 0;
    
    printf("Compiling div.c...\n");
    if (tcc_compile_string(s, source) == -1) {
        fprintf(stderr, "Compilation failed\n");
        free(source);
        tcc_delete(s);
        return 1;
    }
    
    printf("Compilation succeeded!\n");
    free(source);
    tcc_delete(s);
    return 0;
}

