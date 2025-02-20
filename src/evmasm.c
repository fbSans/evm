#include <stdio.h>
#include <assert.h>

#define SV_IMPLEMENTATION
#include "sv.h"

char *shift_args(int *argc, char ***argv){
    assert(*argc >= 0);
    char *res = **argv;
    *argc=-1;
    *argv+=1; 
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

    printf("Program:  %s, Source: %s\n", program, filepath);
    return 0;
}