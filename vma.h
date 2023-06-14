//copyright Carauleanu Valentin Gabriel 311CA
#pragma once
#include <inttypes.h>
#include <stddef.h>
#include "lists.h"

typedef struct dll_list_t list_t;

typedef struct dll_node_t node_t;

/*perm_t este un pointer la o functia ce verifica
permiunile primite */
typedef int (*perm_t)(uint8_t);

typedef struct {
	uint64_t start_address;
	size_t size;
	void *miniblock_list;
} block_t;

typedef struct {
	uint64_t start_address;
	size_t size;
	uint8_t perm;
	void *rw_buffer;
} miniblock_t;

typedef struct {
	uint64_t arena_size;
	list_t *alloc_list;
} arena_t;

arena_t *alloc_arena(const uint64_t size);

void dealloc_arena(arena_t *arena);

void alloc_block(arena_t *arena, const uint64_t address, const uint64_t size);

void free_block(arena_t *arena, const uint64_t address);

void read(arena_t *arena, uint64_t address, uint64_t size);

void write(arena_t *arena, const uint64_t address,  const uint64_t size,
		   int8_t *data);

void pmap(const arena_t *arena);

void mprotect(arena_t *arena, uint64_t address, int8_t *permission);
