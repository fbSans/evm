#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <errno.h>

#include <inttypes.h>

#define SV_IMPLEMENTATION

#include "sv.h"
#include "evm.h"

#define EASM_COMMENT ";"

char *easm_instrunctions[] = {
    "push", "dup", "add", "halt",
};

int is_easm_opcode(Sv name) {
    for(size_t i = 0; i < ARRAY_LEN(easm_instrunctions); ++i){
        if(sv_eq(name, sv_from_cstr(easm_instrunctions[i]))) return true;
    }

    return false;
}

typedef enum {
    EASM_INST,
} Easm_TokenType;

typedef struct {
    Easm_TokenType type;
    Sv name;
    union
    {
        uint64_t data;
        int64_t offset;
        uint64_t address;
    } as;
    size_t row;
    size_t col;
} Easm_Token;


typedef struct {
    Easm_Token *items;
    size_t size;
    size_t capacity;
} Easm_Tokens;

bool strtoi64(const char * ptr, int64_t *res){
    char *end;
    *res = strtol(ptr, &end, 10);
    return end != ptr;
} 

bool strtou64(const char * ptr, uint64_t *res){
    char *end;
    *res = strtoul(ptr, &end, 10);
    return end != ptr;
} 

static void expect_comment_or_empty(Sv sv, const char *filepath, size_t row, size_t col){
    if(!sv_starts_with(sv, sv_from_cstr(EASM_COMMENT)) && sv.size > 0){
        fprintf(stderr, "%s:%zu:%zu Unexpected symbol", filepath, row, col);
        exit(1);
    }
}

static void log_error_and_exit(const char *msg, const char *filepath, size_t row, size_t col){
    fprintf(stderr, "%s:%zu:%zu %s", filepath, row, col, msg);
    exit(1);
}

void easm_tokenize(Sv src, Easm_Tokens *tokens, const char *filepath) 
{
    if(tokens == NULL) return;
    size_t row = 0;
    const char *line_start = NULL;
    while(src.size > 0) {
        Sv line = sv_next_line(&src);
        row++;
        line_start = line.data;

        //Handle empty lines and comments
        sv_trim_left(&line);
        if(line.size == 0 || sv_starts_with(line, sv_from_cstr(EASM_COMMENT))) continue;

        Easm_Token token = {.row = row, .col = line.data - line_start + 1};
        Sv opcode = sv_chop_left(&line);
        sv_trim_left(&line);
        
        if(is_easm_opcode(opcode)){
            printf("Opcode: "SV_FMT"    | Operand: "SV_FMT"\n", SV_ARG(opcode), SV_ARG(line));
            token.name = opcode;
    
            //Instructions with opernads
            if(sv_eq(opcode, sv_from_cstr("push")) || sv_eq(opcode, sv_from_cstr("dup"))){
                uint64_t num_operand;
                Sv operand = sv_chop_left(&line);
                expect_comment_or_empty(line, filepath, row, line.data - line_start);
                if(!strtou64(operand.data, &num_operand)){
                    log_error_and_exit("Expected a numeric operand", filepath, row, operand.data - line_start + 1);
                } 
                token.as.data = num_operand;   
            }
        } else {
            log_error_and_exit("Unknown opcode", filepath, row, opcode.data - line_start + 1);
        }
       
        //TODO: Handle jump instructions containg labels and numbers
        
        //Handling comments after instructions
        expect_comment_or_empty(line, filepath, row, line.data - line_start);
        da_append(tokens, token);
    }
}

//Tokens here must be all corresponding to instructions
void easm_parse(Easm_Tokens tokens, Evm_Insts program){
    while(tokens.size > 0){

    }
}

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
    Sv src = sv_from_cstr(content);

    Easm_Tokens tokens = {0};
    easm_tokenize(src, &tokens, filepath);
    return 0;
}