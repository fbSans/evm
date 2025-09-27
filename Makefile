gFLAGS= -Wall -Werror -Wswitch-enum -pedantic -std=c11 -ggdb 

all: build/evm build/easm parse_bytes

build/evm: src/evm.c src/evm.h src/helpers.h
	$(CC) $(CFLAGS) -DEVM_DEBUG -DHELPERS_IMPLEMENTATION -o build/evm src/evm.c

build/easm: src/easm.c src/evm.c src/evm.h src/helpers.h
	$(CC) $(CFLAGS) -o build/easm src/easm.c src/evm.c

parse_bytes: parse_bytes.c src/helpers.h  src/evm.h
	$(CC) $(CFLAGS) -DHELPERS_IMPLEMENTATION -I ./src -o parse_bytes parse_bytes.c
