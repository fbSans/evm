#ifndef EVM_H_
#define EVM_H_

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
    fprintf(stderr, "todo: %s:%d %s: `%s`\n",__FILE__, __LINE__,  __func__, msg);              \
    exit(1);                                                                           \
}while(0)

#define ARRAY_LEN(a) sizeof((a))/sizeof((a)[0])


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
} Evm_Inst;

static_assert(EVM_INST_COUNT == 20, "Change in EVM_INST_COUNT");


typedef uint64_t Addr;
typedef uint64_t Data;

typedef struct {
    Evm_Inst *items;
    size_t size;
    size_t capacity;
} Evm_Insts;

typedef struct {
    Data *items;
    size_t size;
    size_t capacity;
} Stack;

typedef struct {
    Addr ip;
    Evm_Insts program;
    size_t program_size;
    Stack stack;
    Data *memory;
    size_t memory_capacity;
} Evm;

void evm_init(Evm *evm, Evm_Insts program);
void evm_run(Evm *evm);
void evm_free(Evm* evm);


#endif //EVM_H_