#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>

#include <inttypes.h>
#include "evm.h"

char *inst_to_str[EVM_INST_COUNT] = {
    [EVM_INST_PUSH]    = "EVM_INST_PUSH",
    [EVM_INST_DUP]     = "EVM_INST_DUP",
    [EVM_INST_SWAP]    = "EVM_INST_SWAP",
    [EVM_INST_ADD]    = "EVM_INST_ADD",
    [EVM_INST_SUB]    = "EVM_INST_SUB",
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
    [EVM_INST_JPC]      = "EVM_INST_JPC",
    [EVM_INST_JR]      = "EVM_INST_JR",
    [EVM_INST_JRC]     = "EVM_INST_JRC",
    [EVM_INST_HALT]    = "EVM_INST_HALT"
};

void dump_stack(const Stack *s)
{
    for(size_t i = 0; i < s->size; ++i){
        printf ("index: %zu value: %zu   ", i, s->items[i]);
    }
    printf("\n");
}

void stack_push(Stack *s, Data d)
{
    da_append(s, d);
}

Data stack_pop(Stack *s)
{
    assert(s->size > 0 && "stack_pop:  STACK UNDERFLOW");
    return s->items[--s->size];
}

Data stack_peek(Stack *s, size_t offset)
{
    assert(s->size > 0 && "stack_peek: STACK UNDERFLOW");
    assert(s->size - offset - 1 <= s->size && "stack_peek: STACK ACCESS OUT OF BOUNDS");
    return s->items[s->size - offset - 1];
}



void evm_init(Evm *evm, Evm_Insts program)
{
    memset(evm, 0, sizeof(*evm));
    evm->program = program;
    evm->memory_capacity = EVM_MEM_CAP;
    evm->memory = malloc(evm->memory_capacity);
    memset(evm->memory, 0, evm->memory_capacity);
}

/**Won't free the program, because it's from external source*/
void evm_free(Evm* evm)
{
    free(evm->memory);
    free(evm->stack.items);
    free(evm->call_stack.items);
}

Evm_Inst evm_next_inst(Evm *evm)
{
    assert(evm->ip < evm->program.size && "PROGRAM MEMORY ACCESS OUT OF BOUNDS");
    return evm->program.items[evm->ip++];
}

void evm_call(Evm *evm, Addr func_addr){
    stack_push(&evm->call_stack, evm->ip);
    evm->ip = func_addr;
}

void evm_ret(Evm *evm){
    evm->ip = (Addr) stack_pop(&evm->call_stack); 
}

void evm_push(Evm *evm, Data d)
{
    stack_push(&evm->stack, d);
}

Data evm_pop(Evm *evm)
{
    return stack_pop(&evm->stack);
}

Data evm_peek(Evm *evm, size_t offset)
{
    return stack_peek(&evm->stack, offset);
}

void evm_write8(Evm *evm, Addr dst, Data a)
{
    assert(dst < evm->memory_capacity && "DATA MEMEORY ACCESS OUT OF BOUNDS");
    uint8_t *dst8 = (uint8_t *)evm->memory + dst;
    *dst8 = a;
}

void evm_write64(Evm *evm, Addr dst, Data a)
{
    assert(dst < evm->memory_capacity && "DATA MEMEORY ACCESS OUT OF BOUNDS");
    evm->memory[dst] = a;
}

Data evm_read64(Evm *evm, Addr src)
{
    assert(src < evm->memory_capacity && "DATA MEMEORY ACCESS OUT OF BOUNDS");
    return evm->memory[src];
}

Data evm_read8(Evm *evm, Addr src)
{
    assert(src < evm->memory_capacity && "DATA MEMEORY ACCESS OUT OF BOUNDS");
    return *((uint8_t *)evm->memory + src);
}


void evm_run(Evm *evm){  
    while(true){
        Evm_Inst inst = evm_next_inst(evm);
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
            case EVM_INST_ADD:{
                Data a = evm_pop(evm);
                Data b = evm_pop(evm);  
                Data s = b + a;
                evm_push(evm, s);
            }
            break;
            case EVM_INST_SUB:{
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
            case EVM_INST_READ8: {
                Addr src = (Addr) evm_pop(evm);
                Data a = evm_read8(evm, src);
                evm_push(evm, a);
            } 
            break;
            case EVM_INST_READ64: {
                Addr src = (Addr) evm_pop(evm);
                Data a = evm_read64(evm, src);
                evm_push(evm, a);
            } 
            break;
            case EVM_INST_WRITE8: {
                Addr dst = (Addr) evm_pop(evm);
                Data a = evm_pop(evm);
                evm_write8(evm, dst, a);
            } 
            break;
            case EVM_INST_WRITE64:{
                Addr dst = (Addr) evm_pop(evm);
                Data a = evm_pop(evm);
                evm_write64(evm, dst, a);
            } 
            break;
            case EVM_INST_PRINTU: {
                Data a = evm_pop(evm);
                printf("%zu", a);
            }
            break;
            case EVM_INST_PUTS: {
                Addr ptr = (Addr) evm_pop(evm);
                Data size = evm_pop(evm);
                fwrite(&evm->memory[ptr], size, 1, stdout);
                fflush(stdout);
            }
            break;
            case EVM_INST_CALL: {
               Addr func_addr = (Addr) evm_pop(evm); 
               evm_call(evm, func_addr);
            }
            break;
            case EVM_INST_RET:{
               evm_ret(evm);
            }
            break;
            case EVM_INST_JP: {
                evm->ip = evm_pop(evm);
            }
            break;
            case EVM_INST_JPC: {
                Data cond = evm_pop(evm);
                Addr new_ip = (Addr) evm_pop(evm);
                if(cond){
                    evm->ip = new_ip;
                }
            }
            break;
            case EVM_INST_JR: {
                evm->ip += evm_pop(evm);
            }
            break;
            case EVM_INST_JRC: {
                Data cond = evm_pop(evm);
                long offset = evm_pop(evm);
                if(cond){
                    evm->ip += offset;
                }
            }
            break;
            case EVM_INST_HALT: 
                return;

            case EVM_INST_COUNT: 
            default:
                UNREACHABLE;
        }
        //printf("Ip: %zu Inst: %s Size: %zu\n", evm->ip, inst_to_str[inst], evm->stack.size);
    }
}


#ifdef EVM_DEBUG

static void testFib(void) 
{
    Evm evm = {0};
    Evm_Insts program = {0};

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
    da_append(&program, EVM_INST_ADD);
    
    //dup 0
    da_append(&program, EVM_INST_DUP);
    da_append(&program, 0);


    //push INT32_MAX
    da_append(&program, EVM_INST_PUSH);
    da_append(&program, INT32_MAX);

    //gt
    da_append(&program, EVM_INST_GT);

    //push 9
    da_append(&program, EVM_INST_PUSH);
    da_append(&program, 9);
     
    //swap
    da_append(&program, EVM_INST_SWAP);

    //jpc 
    da_append(&program, EVM_INST_JPC);

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
#endif //EVM_DEBUG