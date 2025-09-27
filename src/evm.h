#ifndef EVM_H_
#define EVM_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>

#include <inttypes.h>

#define HELPERS_REMOVE_PREFIX
#include "helpers.h"

#define EVM_MEM_CAP (64 * 1024)

typedef enum {
    EVM_INST_PUSH = 0,
    EVM_INST_PUSH_HEAP_B,
    EVM_INST_DUP,
    EVM_INST_SWAP,
    EVM_INST_ADD,
    EVM_INST_SUB,
    EVM_INST_MULTU,
    EVM_INST_GT,
    EVM_INST_LT,
    EVM_INST_EQ,
    EVM_INST_GE,
    EVM_INST_LE,
    EVM_INST_READ8,
    EVM_INST_READ64,
    EVM_INST_WRITE8,
    EVM_INST_WRITE64,
    EVM_INST_PRINTU,
    EVM_INST_PUTS,
    EVM_INST_CALL,
    EVM_INST_RET,
    EVM_INST_JP,
    EVM_INST_JPC,
    EVM_INST_JR,
    EVM_INST_JRC,
    EVM_INST_HALT,
    EVM_INST_COUNT
} Evm_Opcode;

static_assert(EVM_INST_COUNT == 25, "Change in EVM_INST_COUNT");


typedef uint64_t Addr;
typedef uint64_t Data;
typedef uint64_t Evm_Inst;

typedef struct {
    Evm_Inst *items; //in order to accept 64bit immedia values
    size_t count;
    size_t capacity;
} Evm_Insts;

typedef struct {
    Data *items;
    size_t count;
    size_t capacity;
} Stack;

typedef struct {
    Addr heap_base; /*addresses from the user will be offsets from this address in memeory*/
    Addr ip;
    Evm_Insts program;
    size_t program_size;
    Stack stack;
    Data *memory;
    size_t memory_capacity;
    Stack call_stack;
} Evm;

void evm_init(Evm *evm, Evm_Insts program, const char *initial_data, size_t initial_data_size);
void evm_run(Evm *evm);
void evm_free(Evm* evm);


#endif //EVM_H_