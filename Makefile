CFLAGS= -Wall -Werror -Wswitch-enum -pedantic -std=c11 -ggdb 

all: evm

evm: evm.c
	$(CC) $(CFLAGS) -o evm evm.c