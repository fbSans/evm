#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>

#include <inttypes.h>

#define SV_IMPLEMENTATION

#include "sv.h"
#include "evm.h"

#define EASM_COMMENT ";"

char *easm_instrunctions[] = {
    "push", "dup", "swap", "add", "sub","multu", "printu64", "halt", "jp", "jpc", "jc", "jcr", "eq" ,"gt", "ge", "lt", "le", "write64", "read64", "puts"
};

int is_easm_opcode(Sv name) 
{
    for(size_t i = 0; i < ARRAY_LEN(easm_instrunctions); ++i){
        if(sv_eq(name, sv_from_cstr(easm_instrunctions[i]))) return true;
    }

    return false;
}

typedef enum {
    EASM_TYPE_INST,
    EASM_TYPE_LABEL,
} Easm_TokenType;

typedef struct {
    Easm_TokenType type;
    Sv name;
    union
    {
        uint64_t data;
        int64_t offset;
        uint64_t address;
        Sv label;
    } as;
    const char *filepath;
    size_t row;
    size_t col;
} Easm_Token;


 typedef struct {
    Easm_Token *items;
    size_t size;
    size_t capacity;
} Easm_Tokens;

typedef struct {
    size_t *items;
    size_t size;
    size_t capacity;
} Indices;

typedef struct {
    Sv *items;
    size_t size;
    size_t capacity;
} Svs;

bool strtoi64(const char * ptr, int64_t *res)
{
    char *end;
    *res = strtoull(ptr, &end, 0);
    return end != ptr;
} 

bool strtou64(const char * ptr, uint64_t *res)
{
    char *end;
    *res = strtoull(ptr, &end, 0);
    return end != ptr;
} 

static void expect_comment_or_empty(Sv sv, const char *filepath, size_t row, size_t col){
    sv_trim_left(&sv);
    if(!sv_starts_with(sv, sv_from_cstr(EASM_COMMENT)) && sv.size > 0){
        fprintf(stderr, "%s:%zu:%zu Unexpected comment or empty line this location\n", filepath, row, col);
        exit(1);
    }
}

static void log_error_and_exit(const char *msg, const char *filepath, size_t row, size_t col){
    fprintf(stderr, "%s:%zu:%zu %s\n", filepath, row, col, msg);
    exit(1);
}
//TODO: Add a string builder for better error reports building
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

        Easm_Token token = {.filepath = filepath, .row = row, .col = line.data - line_start + 1};
        Sv opcode = sv_chop_left(&line);
        sv_trim_left(&line);
        
        if(is_easm_opcode(opcode)){
            token.type = EASM_TYPE_INST;
            token.name = opcode;
            //Instructions with opernads
            if(sv_eq(opcode, sv_from_cstr("push")) || sv_eq(opcode, sv_from_cstr("dup")) ||
            sv_eq(opcode, sv_from_cstr("jr")) ||
            sv_eq(opcode, sv_from_cstr("jrc"))){

                uint64_t num_operand;
                Sv operand = sv_chop_left(&line);
                expect_comment_or_empty(line, filepath, row, line.data - line_start);
                if(!strtou64(operand.data, &num_operand)){
                    log_error_and_exit("tokenizer: Expected a numeric operand", filepath, row, operand.data - line_start + 1);
                } 
                token.as.data = num_operand; 
            } else if ( sv_eq(opcode, sv_from_cstr("jp"))     ||
                        sv_eq(opcode, sv_from_cstr("jpc"))) {

                token.as.label = sv_chop_left(&line);
                expect_comment_or_empty(line, filepath, row, line.data - line_start);
            } 
        } else if (sv_ends_with(opcode, sv_from_cstr(":"))){
            if(opcode.size < 2) log_error_and_exit("tokeniner: Unexpected symbol", token.filepath, token.row, token.col);
            opcode.size--;
            token.name = opcode;
            token.type = EASM_TYPE_LABEL;
        } else {
            char message[1024] = {0};
            char *start = "tokeninzer: Unknown opcode: ";
            size_t start_size = strlen(start);
            memcpy(message, start, start_size);
            memcpy(message + start_size, opcode.data, opcode.size);
            log_error_and_exit(message, filepath, row, opcode.data - line_start + 1);
        }
        //Handling comments after instructions
        sv_trim_left(&line);
        expect_comment_or_empty(line, filepath, row, line.data - line_start + 1);
        da_append(tokens, token);
    }
}

//Tokens here must be all corresponding to instructions
void easm_parse(Easm_Tokens tokens, Evm_Insts *program)
{
    Easm_Tokens labels = {0};
    Indices unresolved = {0};
    Easm_Tokens names = {0};
    
    for(size_t i = 0; i < tokens.size ; ++i){
        //printf(SV_FMT"\n", SV_ARG(tokens.items[i].name));
        Easm_Token token = tokens.items[i];
        switch(token.type){
            case EASM_TYPE_INST:{
                if(sv_eq(token.name, sv_from_cstr("push"))){
                    da_append(program, EVM_INST_PUSH);
                    da_append(program, token.as.data);
                } else if(sv_eq(token.name, sv_from_cstr("dup"))) {
                    da_append(program, EVM_INST_DUP);
                    da_append(program, token.as.data);
                } else if(sv_eq(token.name, sv_from_cstr("swap"))) {
                    da_append(program, EVM_INST_SWAP);
                } else if(sv_eq(token.name, sv_from_cstr("add"))) {
                    da_append(program, EVM_INST_ADD);
                } else if(sv_eq(token.name, sv_from_cstr("sub"))) {
                    da_append(program, EVM_INST_SUB);
                } else if(sv_eq(token.name, sv_from_cstr("multu"))) {
                    da_append(program, EVM_INST_MULTU);
                } else if(sv_eq(token.name, sv_from_cstr("eq"))) {
                    da_append(program, EVM_INST_EQ);
                }  else if(sv_eq(token.name, sv_from_cstr("gt"))) {
                    da_append(program, EVM_INST_GT);
                }  else if(sv_eq(token.name, sv_from_cstr("ge"))) {
                    da_append(program, EVM_INST_GE);
                } else if(sv_eq(token.name, sv_from_cstr("lt"))) {
                    da_append(program, EVM_INST_LT);
                }  else if(sv_eq(token.name, sv_from_cstr("le"))) {
                    da_append(program, EVM_INST_LE);
                } else if(sv_eq(token.name, sv_from_cstr("printu64"))) {
                    da_append(program, EVM_INST_PRINTU);
                } else if ( sv_eq(token.name, sv_from_cstr("jp"))){
                    da_append(&names, token);
                    da_append(&unresolved, program->size + 1);
                    da_append(program, EVM_INST_PUSH);
                    da_append(program, UINT32_MAX); //placeholder (check it later)
                    da_append(program, EVM_INST_JP);
                }  else if ( sv_eq(token.name, sv_from_cstr("jpc"))){
                    da_append(&names, token);
                    da_append(&unresolved, program->size + 1);
                    da_append(program, EVM_INST_PUSH);
                    da_append(program, UINT32_MAX); //placeholder (check it later)
                    da_append(program, EVM_INST_SWAP);
                    da_append(program, EVM_INST_JPC);
                } else if ( sv_eq(token.name, sv_from_cstr("jr"))){
                    UNIMPLEMENTED;
                } else if ( sv_eq(token.name, sv_from_cstr("jrc"))){
                    UNIMPLEMENTED;
                } else if(sv_eq(token.name, sv_from_cstr("puts"))) {
                    da_append(program, EVM_INST_PUTS);
                } else if(sv_eq(token.name, sv_from_cstr("write64"))) {
                    da_append(program, EVM_INST_WRITE64);
                } else if(sv_eq(token.name, sv_from_cstr("read64"))) {
                    da_append(program, EVM_INST_READ64);
                }else if(sv_eq(token.name, sv_from_cstr("halt"))) {
                    da_append(program, EVM_INST_HALT);
                } else {
                    char message[1024] = {0};
                    char *start = "parser: Unknown opcode: ";
                    size_t start_size = strlen(start);
                    memcpy(message, start, start_size);
                    memcpy(message + start_size, token.name.data, token.name.size);
                    log_error_and_exit(message, token.filepath, token.row, token.col);
                }
            } 
            break;
            case EASM_TYPE_LABEL: {
                token.as.address = program->size;
                da_append(&labels, token);
                //printf("%zu\n", token.as.address);
            }
            break;
            default:{
                UNREACHABLE; 
            }
        }
    }

    //Second pass
    for(size_t i = 0; i < unresolved.size; ++i){
        size_t inst_idx = unresolved.items[i];
        Data *replacee = &program->items[inst_idx];
        Easm_Token token = names.items[i]; // for name and localtion

        assert(*replacee == UINT32_MAX); 
        bool found = false;
        for(size_t j = 0; j < labels.size; ++j){
            found = true;
            Easm_Token label = labels.items[j];
            assert(label.type == EASM_TYPE_LABEL); 
            if(sv_eq(token.as.label, label.name)){
                *replacee = label.as.address;
            }
        }
        if(!found) {
            char message[] = "parser: Undefined label";
            log_error_and_exit(message, token.filepath, token.row, token.col);
        }
    }

    free(labels.items);
    free(unresolved.items);
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
Sv slurp_file(const char *filepath)
{
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
    char *data = malloc(n + 1);
    data[n] = '\0';
    assert(data != NULL);

    size_t m = fread(data, 1, n, f);
    while(m < (size_t) n){
        m += fread(data + m, n - m, 1, f);
    }

    
    if(f) fclose(f);
    return sv_from_parts(data, n);
}

int main(int argc, char **argv)
{
    const char *program = shift_args(&argc, &argv);
    if(argc < 1){
        fprintf(stderr, "Usage:\n");
        fprintf(stderr, "    %s <file>\n", program);
        exit(1);
    }
    
    const char *filepath = shift_args(&argc, &argv);
    Sv src = slurp_file(filepath);
    
    Easm_Tokens easm_tokens = {0};
    Evm_Insts evm_program = {0};
    easm_tokenize(src, &easm_tokens, filepath);
    easm_parse(easm_tokens, &evm_program);

    Evm evm = {0};
    evm_init(&evm, evm_program);
    evm_run(&evm);
    evm_free(&evm);
    free(evm_program.items);
    free(easm_tokens.items);
    free((char *) src.data);

   return 0;
}