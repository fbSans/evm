CFLAGS= -Wall -Werror -Wswitch-enum -pedantic -std=c11 -ggdb 

all: build/evm build/evmasm

build/evm: src/evm.c
	$(CC) $(CFLAGS) -o build/evm src/evm.c

build/evmasm: src/sv.h src/evmasm.c
	$(CC) $(CFLAGS) -o build/evmasm src/evmasm.c