#copyright Carauleanu Valentin Gabriel 311CA

CC=gcc
CFLAGS=-Wall -Wextra -std=c99

build:
	$(CC) $(CFLAGS) vma.c lists.c aux_functions.c main.c -o vma
clean:
	rm -f vma

run_vma:
	./vma

.PHONY: pack clean
