#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>

#include <inttypes.h>

#define DA_INIT_CAP (1024)
#define EVM_MEM_CAP (64 * 1024)

#define da_append(da, item) do {                                                      \
    if((da)->size >= (da)->capacity){                                                 \
        if((da)->capacity == 0) (da)->items = NULL;                                   \
        (da)->capacity = ((da)->capacity == 0) ? DA_INIT_CAP  : (da)->capacity * 2;   \
        (da)->items = realloc((da)->items, (da)->capacity * sizeof(*(da)->items));    \
        memset((da)->items + (da)->size, 0, (da)->capacity - (da)->size);             \
    }                                                                                 \
    (da)->items[(da)->size++] = item;\
} while (0)

#define UNIMPLEMENTED do {                                                              \
    fprintf(stderr, "%s:%d %s: not implemented yet!\n", __FILE__, __LINE__, __func__);  \
    exit(1);                                                                            \
}while(0)

#define UNREACHABLE do {                                                               \
    fprintf(stderr, "%s:%d %s: unreachable!\n",__FILE__, __LINE__,  __func__);         \
    exit(1);                                                                           \
}while(0)

#define TODO(msg) do {                                                                 \
    fprintf(stderr, "%s:%d %s: %s\n",__FILE__, __LINE__,  __func__, msg);              \
    exit(1);                                                                           \
}while(0)


typedef enum  {
    EVM_INST_PUSH = 0,
    EVM_INST_DUP,
    EVM_INST_SWAP,
    EVM_INST_ADDU,
    EVM_INST_SUBU,
    EVM_INST_MULTU,
    EVM_INST_GT,
    EVM_INST_LT,
    EVM_INST_EQ,
    EVM_INST_GE,
    EVM_INST_LE,
    EVM_INST_READ64,
    EVM_INST_WRITE64,
    EVM_INST_PRINTU,
    EVM_INST_PUTS,
    EVM_INST_JP,
    EVM_INST_JC,
    EVM_INST_JR,
    EVM_INST_JCR,
    EVM_INST_HALT,
    EVM_INST_COUNT
} EVM_INST;

char *inst_to_str[EVM_INST_COUNT] = {
    [EVM_INST_PUSH]    = "EVM_INST_PUSH",
    [EVM_INST_DUP]     = "EVM_INST_DUP",
    [EVM_INST_SWAP]    = "EVM_INST_SWAP",
    [EVM_INST_ADDU]    = "EVM_INST_ADDU",
    [EVM_INST_SUBU]    = "EVM_INST_SUBU",
    [EVM_INST_MULTU]   = "EVM_INST_MULTU",
    [EVM_INST_GT]      = "EVM_INST_GT",
    [EVM_INST_LT]      = "EVM_INST_LT",
    [EVM_INST_EQ]      = "EVM_INST_EQ",
    [EVM_INST_GE]      = "EVM_INST_GE",
    [EVM_INST_LE]      = "EVM_INST_LE",
    [EVM_INST_READ64]  = "EVM_INST_READ64",
    [EVM_INST_WRITE64] = "EVM_INST_WRITE64",
    [EVM_INST_PRINTU]  = "EVM_INST_PRINTU",
    [EVM_INST_JP]      = "EVM_INST_JP",
    [EVM_INST_JC]      = "EVM_INST_JC",
    [EVM_INST_JR]      = "EVM_INST_JR",
    [EVM_INST_JCR]     = "EVM_INST_JCR",
    [EVM_INST_HALT]    = "EVM_INST_HALT"
};

_Static_assert(EVM_INST_COUNT == 20, "Change in EVM_INST_COUNT");

typedef uint64_t Addr;
typedef uint64_t Data;

typedef struct {
    EVM_INST *items;
    size_t size;
    size_t capacity;
} Insts;

typedef struct {
    Data *items;
    size_t size;
    size_t capacity;
} Stack;

void stack_push(Stack *s, Data d){
    da_append(s, d);
}

Data stack_pop(Stack *s){
    assert(s->size > 0 && "stack_pop:  STACK UNDERFLOW");
    return s->items[--s->size];
    
}

Data stack_peek(Stack *s, size_t offset){
    assert(s->size > 0 && "stack_peek: STACK UNDERFLOW");
    assert(s->size - offset - 1 <= s->size && "stack_peek: STACK ACCESS OUT OF BOUNDS");
    return s->items[s->size - offset - 1];
}

typedef struct {
    Addr ip;
    Insts program;
    size_t program_size;
    Stack stack;
    Data *memory;
    size_t memory_capacity;
} Evm;

void evm_init(Evm *evm, Insts program){
    memset(evm, 0, sizeof(*evm));
    evm->program = program;
    evm->memory_capacity = EVM_MEM_CAP;
    evm->memory = malloc(evm->memory_capacity);
    memset(evm->memory, 0, evm->memory_capacity);
}

/**Won't free the program, because it's from external source*/
void evm_free(Evm* evm){
    free(evm->memory);
    free(evm->stack.items);
}

EVM_INST evm_next_inst(Evm *evm){
    assert(evm->ip < evm->program.size && "PROGRAM MEMORY ACCESS OUT OF BOUNDS");
    return evm->program.items[evm->ip++];
}


void evm_push(Evm *evm, Data d){
    stack_push(&evm->stack, d);
}

Data evm_pop(Evm *evm){
    return stack_pop(&evm->stack);
}

Data evm_peek(Evm *evm, size_t offset){
    return stack_peek(&evm->stack, offset);
}

void evm_write(Evm *evm, Addr dst, Data a){
    assert(dst < evm->memory_capacity && "DATA MEMEORY ACCESS OUT OF BOUNDS");
    evm->memory[dst] = a;
}

Data evm_read(Evm *evm, Addr src){
    assert(src < evm->memory_capacity && "DATA MEMEORY ACCESS OUT OF BOUNDS");
    return evm->memory[src];
}

void dump_stack(Stack *s){
    for(size_t i = 0; i < s->size; ++i){
        printf ("index: %zu value: %zu   ", i, s->items[i]);
    }
    printf("\n");
}

void evm_run(Evm *evm){  
    while(true){
        EVM_INST inst = evm_next_inst(evm);
       
        switch(inst){
            case EVM_INST_PUSH: {
                Data a = evm_next_inst(evm); 
                evm_push(evm, a);
            }
            break;
            case EVM_INST_DUP: { 
                Data offset = evm_next_inst(evm);
                Data a = evm_peek(evm, offset);
                evm_push(evm, a);
            }
            break;
            case EVM_INST_SWAP: { 
                Data a = evm_pop(evm);
                Data b = evm_pop(evm);
                evm_push(evm, a);
                evm_push(evm, b);
            }
            break;
            case EVM_INST_ADDU:{
                Data a = evm_pop(evm);
                Data b = evm_pop(evm);  
                Data s = b + a;
                evm_push(evm, s);
            }
            break;
            case EVM_INST_SUBU:{
                Data a = evm_pop(evm);
                Data b = evm_pop(evm);  
                Data s = b - a;
                evm_push(evm, s);
            } 
            break;
            case EVM_INST_MULTU: {
                Data a = evm_pop(evm);
                Data b = evm_pop(evm);  
                Data s = b * a;
                evm_push(evm, s);
            } 
            break;
            case EVM_INST_GT: {
                Data a = evm_pop(evm);
                Data b = evm_pop(evm);
                evm_push(evm, a > b);
            }
            break;
            case EVM_INST_LT: {
                Data a = evm_pop(evm);
                Data b = evm_pop(evm);
                evm_push(evm, a < b);
            }
            break;
            case EVM_INST_EQ: {
                Data a = evm_pop(evm);
                Data b = evm_pop(evm);
                evm_push(evm, a == b);
            }
            break;
            case EVM_INST_GE: {
                Data a = evm_pop(evm);
                Data b = evm_pop(evm);
                evm_push(evm, a >= b);
            }
            break;
            case EVM_INST_LE: {
                Data a = evm_pop(evm);
                Data b = evm_pop(evm);
                evm_push(evm, a <= b);
            }
            break;
            case EVM_INST_READ64: {
                Addr src = (Addr) evm_pop(evm);
                Data a = evm_read(evm, src);
                evm_push(evm, a);
            } 
            break;
            case EVM_INST_WRITE64:{
                Addr dst = evm_pop(evm);
                Data a = evm_pop(evm);
                evm_write(evm, dst, a);
            } 
            break;
            case EVM_INST_PRINTU: {
                Data a = evm_pop(evm);
                printf("%zu", a);
            }
            break;
            case EVM_INST_PUTS: {
                Data ptr = evm_pop(evm);
                Data size = evm_pop(evm);
                fwrite(&evm->memory[ptr], size, 1, stdout);
            }
            break;
            case EVM_INST_JP: {
                evm->ip = evm_pop(evm);
            }
            case EVM_INST_JC: {
                Data cond = evm_pop(evm);
                Data new_ip = evm_pop(evm);
                if(cond){
                    evm->ip = new_ip;
                }
            }
            break;
            case EVM_INST_JR: {
                evm->ip += evm_pop(evm);
            }
            break;
            case EVM_INST_JCR: {
                Data cond = evm_pop(evm);
                long offset = evm_pop(evm);
                if(cond){
                    evm->ip += offset;
                }
            }
            case EVM_INST_HALT: 
                return;

            case EVM_INST_COUNT: 
            default:
                UNREACHABLE;
        }
        // printf("Ip: %zu Inst: %s Size: %zu\n", evm->ip, inst_to_str[inst], evm->stack.size);
        //dump_stack(&evm->stack);
    }
}

void testFib(void) 
{
    Evm evm = {0};
    Insts program = {0};

    //push 'newline'
    da_append(&program, EVM_INST_PUSH);
    da_append(&program, (Data) (0x0a0a0a0au));

    //push 0 
    da_append(&program, EVM_INST_PUSH);
    da_append(&program, 0);
    
    //write64
    da_append(&program, EVM_INST_WRITE64);

    //push 0 
    da_append(&program, EVM_INST_PUSH);
    da_append(&program, 0);

    //push 1
    da_append(&program, EVM_INST_PUSH);
    da_append(&program, 1);
    
    //dup 1
    da_append(&program, EVM_INST_DUP);
    da_append(&program, 1);
    
    //print
    da_append(&program, EVM_INST_PRINTU);

    
    //push 1
    da_append(&program, EVM_INST_PUSH);
    da_append(&program, 1);

    //push 0 
    da_append(&program, EVM_INST_PUSH);
    da_append(&program, 0);
    
    //puts
    da_append(&program, EVM_INST_PUTS);
    
    //swap
    da_append(&program, EVM_INST_SWAP);

    //dup 1
    da_append(&program, EVM_INST_DUP);
    da_append(&program, 1);
    
    //add
    da_append(&program, EVM_INST_ADDU);
    
    //dup 0
    da_append(&program, EVM_INST_DUP);
    da_append(&program, 0);


    //push 1000
    da_append(&program, EVM_INST_PUSH);
    da_append(&program, INT32_MAX);

    //gt
    da_append(&program, EVM_INST_GT);

    //push 0
    da_append(&program, EVM_INST_PUSH);
    da_append(&program, 9);
     
    //swap
    da_append(&program, EVM_INST_SWAP);

    //jc 
    da_append(&program, EVM_INST_JC);

    //halt
    da_append(&program, EVM_INST_HALT); 
    
    evm_init(&evm, program);
    evm_run(&evm);
    evm_free(&evm);
    free(program.items);
}

int main(void) 
{
    testFib();
    return 0;
}