gFLAGS= -Wall -Werror -Wswitch-enum -pedantic -std=c11 -ggdb 

all: build/evm build/easm 

build/evm: src/evm.c src/evm.h src/helpers.h
	$(CC) $(CFLAGS) -DEVM_DEBUG -DHELPERS_IMPLEMENTATION -o build/evm src/evm.c

build/easm: src/easm.c src/evm.c src/evm.h src/helpers.h
	$(CC) $(CFLAGS) -o build/easm src/easm.c src/evm.c


