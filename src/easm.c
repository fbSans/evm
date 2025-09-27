#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>

#include <inttypes.h>

#define HELPERS_IMPLEMENTATION
#include "evm.h"
#define EASM_COMMENT ";"

char *easm_instrunctions[] = {
    "push", "dup", "swap", 
    "add", "sub","multu", 
    "printu64", "halt", 
    "jp", "jpc", "jc", 
    "jcr", "eq" ,"gt", 
    "ge", "lt", "le", 
    "write8", "write64", 
    "read8","read64", "puts",
    "call", "ret", "pushl",
    "push_heapb"
};

int is_easm_opcode(StringView name) 
{
    for(size_t i = 0; i < ARRAY_LEN(easm_instrunctions); ++i){
        if(sv_eq(name, sv_from_cstr(easm_instrunctions[i]))) return true;
    }

    return false;
}

typedef enum {
    EASM_TYPE_INST, 
    EASM_TYPE_LABEL,
    EASM_TYPE_MEM_LABEL,
    EASM_TYPE_BYTES, //this are too be placed in the data memory
} Easm_TokenType;

typedef struct {
    char *items;
    size_t count;
    size_t capacity;
} Bytes;

typedef struct {
    Easm_TokenType type;
    StringView name;
    union
    {
        uint64_t data;
        int64_t offset;
        uint64_t address;
        StringView label;
        Bytes bytes;
    } get;
    const char *filepath;
    size_t row;
    size_t col;
} Easm_Token;


 typedef struct {
    Easm_Token *items;
    size_t count;
    size_t capacity;
} Easm_Tokens;

typedef struct {
    size_t *items;
    size_t count;
    size_t capacity;
} Indices;


typedef struct {
    StringView *items;
    size_t count;
    size_t capacity;
} Svs;

// Todo: Integrate this two types bellow into the rest of the parser


typedef struct {
    bool ok;
    const char *message;
} Parse_Result;

//// Globals
Arena easm_arena = {0};


//// BYTES PARSING: over time will be moved into a separate unit
//This is the grammar
// bytes       ::= simple, | simple, bytes 
// simple      ::= 'char' |"char*" | num
// char        ::= '<ascii_literal>'
// num         ::= [:ascii-num:] | bin | hex
// hex         ::= 0x[0-9a-fA-F]{2}
// bin         ::= 0b[01]{8}


bool ishex(char c)
{
    return isdigit(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

bool isbin(char c){
    return '0' <= c || c <= '1';
}


bool isHexStart(StringView input)
{
    return sv_starts_with(input, sv_from_cstr("0x")) || sv_starts_with(input, sv_from_cstr("0X"));
}

bool isBinStart(StringView input)
{
    return sv_starts_with(input, sv_from_cstr("0b")) || sv_starts_with(input, sv_from_cstr("0B"));
}

bool is_double_quote(char c)
{
    return c == '"';
}

bool strtoi64(const char * ptr, int64_t *res)
{
    char *end;
    *res = strtoull(ptr, &end, 0);
    return end != ptr;
} 

bool strtou64(const char *ptr, uint64_t *res)
{
    char *end;
    uint64_t start = 0;
    start = strtoull(ptr, &end, 0);
    if(res) *res = start;
    return end != ptr;
} 

char hex_value(char c){
    assert(ishex(c) && "Expected hexadecimal input");
    if(isdigit(c)) return c - '0';
    if(c <='f') return c - 'a';
    return c - 'A';
}

Parse_Result parseHex(StringView *input, Bytes *res)
{
    sv_take(input, 2); // remove 0x or 0X
    StringView candidate = sv_clone(*input);
    candidate = sv_take_while(&candidate, ishex);
    if(candidate.count < 1) {
        fprintf(stderr, "");
        return (Parse_Result) {
            .ok = false,
            .message = "Invalid hex spec",
        };
    }

    StringView hexchars = sv_take(&candidate, 2);
    sv_take(input, hexchars.count);

    
    if(hexchars.count == 1){
        const char first = *sv_take(&hexchars, 1).data;
        da_append(res, hex_value(first));
        return (Parse_Result){
            .ok = true,
        };
    }
         
    char val = 16 * hex_value(hexchars.data[0]) + hex_value(hexchars.data[1]);
    da_append(res, val);
    return (Parse_Result) {
        .ok = true,
        .message = "",
    };
}

bool expect_remove_char(StringView *sv, char c)
{
    if(sv->count == 0) return false;
    sv_trim_left(sv);
    if (sv->data[0] == c) {
        sv_take(sv, 1);
        return true;
    }
    return false;
}

char next_char(const char *input, bool *res)
{
    *res = true;
    if(input[0] == '\\'){
        input++;
        switch(input[0]){
            case 't':{
                return '\t';
            }
            case 'n':{
                return '\n';
            }
            case 'r':{
                return '\n';
            }
            case '0':{
                return '\0';
            }
            case 'f':{
                return '\f';
            }
            case 'a':{
                return '\a';
            }
            case '"':
                return '"';
            case '\\':
                return '\\';
            default:
                *res =false;
        }   
    }
    return *input;
    *res = true;
}

Parse_Result bytes_from_dq_string(StringView *input, Bytes *res){
    Arena_Mark mark = arena_Mark(&easm_arena);
    Parse_Result ret = {.ok = true, .message = ""};
    while(input->count > 0 && input->data[0] != '"'){
        char first = input->data[0];
        if(first == '\\' && input->count < 2) {
            ret.ok = false;
            ret.message = "Malformed escaping in string";
            RETURN_DEFER(ret, ret);
        }

        int skip = first == '\\' ? 2 : 1;
        
        char n = next_char(input->data, &ret.ok);
        if(!ret.ok){
            ret.ok = false;
            ret.message = arena_sprintf(&easm_arena, "unsupported escape character `\\%s`", *input->data);
            RETURN_DEFER(ret, ret);
        }

        da_append(res, n);
        sv_take(input, skip);

    }

defer:
    arena_restore(&easm_arena, mark);
    return ret;
}



Parse_Result parse_bytes(StringView *input, Bytes *res)
{
    Arena_Mark mark = arena_Mark(&easm_arena);
    Parse_Result ret = {.ok = true, .message = ""};

    sv_trim_left(input);
    size_t old_input_count = input->count;
    while(input->count > 0) {
        sv_trim_left(input);
        if(sv_starts_with(*input, sv_from_cstr("'"))){
            /*char*/

            if(input->count < 3 ||input->data[1] == '\\' && input->count < 4){
                ret.ok = false;
                ret.message = "Incomplete char spec";
                RETURN_DEFER(ret, ret);
            }

            sv_take(input, 1);

            if(input->data[0] == '\\'){
                const char* char_array = sv_take(input, 2).data;
                char n = next_char(char_array, &ret.ok);
                da_append(res, n);
                if(!ret.ok){
                    ret.message = arena_sprintf(&easm_arena, "unsupported escape character `\\%s`", *input->data);
                    RETURN_DEFER(ret, ret);
                }
            } else {
                da_append(res, input->data[0]);
                sv_take(input, 1);
            }
            
            if(!expect_remove_char(input, '\'')){
                ret.ok = false;
                ret.message = "invalid char literal, no closing `'`\n";
                RETURN_DEFER(ret, ret);
            }

        } else if(sv_starts_with(*input, sv_from_cstr("\""))){
            /* "str" */
            sv_take(input, 1);
            bytes_from_dq_string(input, res);

            if(!expect_remove_char(input, '"')){
                ret.ok = false;
                ret.message = "Unclosed string literal";
                RETURN_DEFER(ret, ret);
            }

        } else if (isHexStart(*input)){
            if(!(ret = parseHex(input, res)).ok) {
                RETURN_DEFER(ret, ret);
            }
        } else if(isBinStart(*input)){
            /*bin*/
            TODO("parse bin");
        } else if(*input->data >= '0' && *input->data <= '9'){
            TODO("parse decimal");
        }

        bool sep = expect_remove_char(input, ',');
        if(sep) continue;

        bool term = expect_remove_char(input, ';');
        if (term) break;

        /// Shoudnt be here
        ret.ok = false;
        ret.message = "Invalid bytes spec, expected `,` to separate byte items and `;` to terminate it.\n";
        RETURN_DEFER(ret, ret);
    }

    if(old_input_count <= input->count) {
        ret.ok = false;
        ret.message = "No content for string literal";
        RETURN_DEFER(ret, ret);
    }

defer:
    arena_restore(&easm_arena, mark);
    return ret;
}





static void expect_comment_or_empty(StringView sv, const char *filepath, size_t row, size_t col){
    sv_trim_left(&sv);
    if(!sv_starts_with(sv, sv_from_cstr(EASM_COMMENT)) && sv.count > 0){
        fprintf(stderr, "%s:%zu:%zu Expected comment or empty line this location\n", filepath, row, col);
        arena_free(&easm_arena);
        exit(1);
    }
}

static void log_error_and_exit(const char *msg, const char *filepath, size_t row, size_t col){
    fprintf(stderr, "%s:%zu:%zu %s\n", filepath, row, col, msg);
    arena_free(&easm_arena);
    exit(1);
}

//TODO: Add a string builder for better error reports building
void easm_tokenize(StringView src, Easm_Tokens *tokens, const char *filepath) 
{
    Arena_Mark mark = arena_Mark(&easm_arena);
    if(tokens == NULL) return;
    size_t row = 0;
    const char *line_start = NULL;
    while(src.count > 0) {
        StringView  snapshot = src;
        StringView line = sv_next_line(&src);
        row++;
        line_start = line.data;

        //Handle empty lines and comments
        sv_trim_left(&line);
        if(line.count == 0 || sv_starts_with(line, sv_from_cstr(EASM_COMMENT))) continue;

        Easm_Token token = {.filepath = filepath, .row = row, .col = line.data - line_start + 1};
        StringView opcode = sv_chop_left(&line);
        sv_trim_left(&line);
        
        if(is_easm_opcode(opcode)){
            token.type = EASM_TYPE_INST;
            token.name = opcode;
            //Instructions with opernads

            if(sv_eq(opcode, sv_from_cstr("push")) || sv_eq(opcode, sv_from_cstr("dup")) ||
            sv_eq(opcode, sv_from_cstr("jr")) ||
            sv_eq(opcode, sv_from_cstr("jrc"))){

                uint64_t num_operand;
                StringView operand = sv_chop_left(&line);
                expect_comment_or_empty(line, filepath, row, line.data - line_start);
                if(!strtou64(operand.data, &num_operand)){
                    log_error_and_exit("tokenizer: Expected a numeric operand", filepath, row, operand.data - line_start + 1);
                } 
                token.get.data = num_operand; 
            } else if ( sv_eq(opcode, sv_from_cstr("pushl"))||
                        sv_eq(opcode, sv_from_cstr("jp"))   ||
                        sv_eq(opcode, sv_from_cstr("jpc"))  ||
                        sv_eq(opcode, sv_from_cstr("call"))) {
                
                if(line.count < 1) {
                    log_error_and_exit(arena_sprintf(&easm_arena, "tokenizer: no operand specified for `"SV_FMT"`.", SV_ARG(opcode)), filepath, row, line.data - line_start + 1);
                }    
                token.get.label = sv_chop_left(&line);
                expect_comment_or_empty(line, filepath, row, line.data - line_start);
            } 
        } else if (sv_ends_with(opcode, sv_from_cstr(":"))){
            if(opcode.count < 2) log_error_and_exit("tokeniner: Unexpected empty label", token.filepath, token.row, token.col);
            opcode.count--;
            token.name = opcode;
            if(sv_starts_with(opcode, sv_from_cstr("."))){
                token.type = EASM_TYPE_MEM_LABEL;
            } else {
                token.type = EASM_TYPE_LABEL;
            }
        } else if(sv_starts_with(opcode, sv_from_cstr("db"))){
            //restore the whole content until after db and offer the stream to build a string
            // The line orientation wont fail, it will continue from where we stopped
            token.name = opcode;
            token.type = EASM_TYPE_BYTES;

            sv_take(&opcode, 3);
            src = sv_clone(snapshot);
            sv_trim_left(&src);
            sv_take(&src, 2); // remove db
            Parse_Result res = parse_bytes(&src, &token.get.bytes);
            line.count = 0;
            if(!res.ok){
                log_error_and_exit(arena_sprintf(&easm_arena, "tokenizer: %s", res.message), token.filepath, token.row, token.col);
            }
        }else {
            char message[1024] = {0};
            char *start = "tokeninzer: Unknown opcode: ";
            size_t start_size = strlen(start);
            memcpy(message, start, start_size);
            memcpy(message + start_size, opcode.data, opcode.count);
            log_error_and_exit(message, filepath, row, opcode.data - line_start + 1);
        }
        //Handling comments after instructions
        sv_trim_left(&line);
        expect_comment_or_empty(line, filepath, row, line.data - line_start + 1);
        da_append(tokens, token);
    }
    arena_restore(&easm_arena, mark);
}

//Tokens here must be all corresponding to instructions
// No support for string literals in instructions or as instructions
void easm_generate(Easm_Tokens tokens, Evm_Insts *program, Bytes *memory)
{
    Easm_Tokens labels = {0};
    Indices unresolved = {0};
    Easm_Tokens names = {0};
    
    
    for(size_t i = 0; i < tokens.count ; ++i){
        //printf(SV_FMT"\n", SV_ARG(tokens.items[i].name));
        Easm_Token token = tokens.items[i];
        switch(token.type){
            case EASM_TYPE_INST:{
                if(sv_eq(token.name, sv_from_cstr("push"))){
                    da_append(program, EVM_INST_PUSH);
                    da_append(program, token.get.data);
                } else if (sv_eq(token.name, sv_from_cstr("push_heapb"))){
                     da_append(program, EVM_INST_PUSH_HEAP_B);
                } else if (sv_eq(token.name, sv_from_cstr("pushl"))){
                    da_append(&names, token);
                    da_append(&unresolved, program->count + 1);
                    da_append(program, EVM_INST_PUSH);
                    da_append(program, UINT32_MAX); //placeholder (check it later)       
                } else if(sv_eq(token.name, sv_from_cstr("dup"))) {
                    da_append(program, EVM_INST_DUP);
                    da_append(program, token.get.data);
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
                } else if(sv_eq(token.name, sv_from_cstr("ret"))) {
                    da_append(program, EVM_INST_RET);
                } else if ( sv_eq(token.name, sv_from_cstr("call"))){
                    da_append(&names, token);
                    da_append(&unresolved, program->count + 1);
                    da_append(program, EVM_INST_PUSH);
                    da_append(program, UINT32_MAX); //placeholder (check it later)
                    da_append(program, EVM_INST_CALL);
                } else if ( sv_eq(token.name, sv_from_cstr("jp"))){
                    da_append(&names, token);
                    da_append(&unresolved, program->count + 1);
                    da_append(program, EVM_INST_PUSH);
                    da_append(program, UINT32_MAX); //placeholder (check it later)
                    da_append(program, EVM_INST_JP);
                } else if ( sv_eq(token.name, sv_from_cstr("jpc"))){
                    da_append(&names, token);
                    da_append(&unresolved, program->count + 1);
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
                } else if(sv_eq(token.name, sv_from_cstr("write8"))) {
                    da_append(program, EVM_INST_WRITE8);
                }else if(sv_eq(token.name, sv_from_cstr("write64"))) {
                    da_append(program, EVM_INST_WRITE64);
                }  else if(sv_eq(token.name, sv_from_cstr("read8"))) {
                    da_append(program, EVM_INST_READ8);
                } else if(sv_eq(token.name, sv_from_cstr("read64"))) {
                    da_append(program, EVM_INST_READ64);
                } else if(sv_eq(token.name, sv_from_cstr("halt"))) {
                    da_append(program, EVM_INST_HALT);
                } else {
                    printf(SV_FMT", %zu\n", SV_ARG(token.name), token.name.count);
                    char message[1024] = {0};
                    char *start = "generator: Unknown opcode: ";
                    size_t start_size = strlen(start);
                    memcpy(message, start, start_size);
                    memcpy(message + start_size, token.name.data, token.name.count);
                    log_error_and_exit(message, token.filepath, token.row, token.col);
                }
            } 
            break;
            case EASM_TYPE_LABEL: {
                token.get.address = program->count;
                da_append(&labels, token);
                //printf("%zu\n", token.get.address);
            }
            break;
            case EASM_TYPE_MEM_LABEL: {
                token.get.address = memory->count;
                da_append(&labels, token);
                //printf("%zu\n", token.get.address);
            }
            break;
            case EASM_TYPE_BYTES: {
                da_append_array(memory, (const char *)(&token.get.bytes.count), sizeof(size_t)); //accomdating the hole lenght in memory
                da_append_array(memory, token.get.bytes.items, token.get.bytes.count);
            }
            break;
            default:{
                UNREACHABLE; 
            }
        }
    }

    //Second pass
    for(size_t i = 0; i < unresolved.count; ++i){
        size_t replacement_idx = unresolved.items[i];
        Easm_Token token = names.items[i]; // for name and localtion
        
        assert(program->items[replacement_idx] == UINT32_MAX); 
        bool found = false;
        for(size_t j = 0; j < labels.count; ++j){
            Easm_Token label = labels.items[j];
            assert(label.type == EASM_TYPE_LABEL || label.type == EASM_TYPE_MEM_LABEL); 
            if(sv_eq(token.get.label, label.name)){
                found = true;
                program->items[replacement_idx] = label.get.address;
                break;
            }
        }
        if(!found) {
            char message[] = "generator: Undefined label";
            log_error_and_exit(message, token.filepath, token.row, token.col);
        } 
    }

    free(labels.items);
    free(unresolved.items);
    free(names.items);
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
    StringBuilder sb;
    read_file_into_sb(&sb, filepath);
    StringView src = sv_from_parts(sb.items, sb.count);
   

    Easm_Tokens easm_tokens = {0};
    Evm_Insts evm_program = {0};
    Bytes byte_memory = {0};
   
    
    easm_tokenize(src, &easm_tokens, filepath);
    easm_generate(easm_tokens, &evm_program, &byte_memory);

    
    Evm evm = {0};
    evm_init(&evm, evm_program, byte_memory.items, byte_memory.count);
    evm_run(&evm);
    evm_free(&evm);
    free(evm_program.items);
    free(easm_tokens.items);
    free(sb.items);
    return 0;
}
