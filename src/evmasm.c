#include <stdio.h>
#include <assert.h>
#include <errno.h>

#define SV_IMPLEMENTATION
#include "sv.h"

// Helpers
char *shift_args(int *argc, char ***argv){
    assert(*argc >= 0);
    char *res = **argv;
    *argc-=1;
    *argv+=1; 
    return res;
}


/**allocates memory and returns the content of the file, returning the pointer to the memory*/
char *slurp_file(const char *filepath){
    FILE *f = fopen(filepath, "r");

    if(f == NULL){
        fprintf(stderr, "Could not open file %s: %s\n", filepath, strerror(errno));
        exit(1);
    }

    fseek(f, 0, SEEK_END);
    long n = ftell(f);

    if(n <= 0){
        fprintf(stderr, "Could not tell file size on %s: %s\n", filepath, strerror(errno));
        exit(1); 
    }

    fseek(f, 0, SEEK_SET);
    
    char *res = malloc(n);
    size_t m = fread(res, 1, n, f);
    while(m < (size_t) n){
        m += fread(res + m, n, 1, f);
    }
    return res;
}




int main(int argc, char **argv){
    const char *program = shift_args(&argc, &argv);
    if(argc < 1){
        fprintf(stderr, "Usage:\n");
        fprintf(stderr, "    %s <file>\n", program);
        exit(1);
    }
    
    const char *filepath = shift_args(&argc, &argv);
    char *content = slurp_file(filepath);
    (void) content;
    printf("%s\n", content);
    return 0;
}