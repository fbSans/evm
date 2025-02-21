CFLAGS= -Wall -Werror -Wswitch-enum -pedantic -std=c11 -ggdb 

all: build/evm build/easm

build/evm: src/evm.c
	$(CC) $(CFLAGS) -o build/evm src/evm.c

build/easm: src/sv.h src/easm.c
	$(CC) $(CFLAGS) -o build/easm src/easm.c