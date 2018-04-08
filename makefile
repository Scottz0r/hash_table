CC=gcc
CFLAGS=-I .

all: hash_table.c tests.c
	$(CC) -o hash_table_tests tests.c hash_table.c $(CFLAGS)
