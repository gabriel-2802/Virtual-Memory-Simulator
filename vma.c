//copyright Carauleanu Valentin Gabriel 311CA
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "vma.h"
#include "lists.h"
#include "aux_functions.h"

arena_t *alloc_arena(const uint64_t size)
{
	arena_t *arena = malloc(sizeof(arena_t));
	DIE(!arena, "malloc failed\n");

	arena->alloc_list = dll_create(sizeof(block_t));
	arena->arena_size = size;
	return arena;
}

void dealloc_arena(arena_t *arena)
{
	node_t *block_node = arena->alloc_list->head;

	//parcurgem fiecare block, pentru a elibera lista de miniblock-uri
	for (unsigned int i = 0; i < arena->alloc_list->size; ++i) {
		block_t *block = (block_t *)block_node->data;
		list_t *mb_list = (list_t *)block->miniblock_list;

		//stergem toate bufferele din miniblock uri
		node_t *mb_node = mb_list->head;
		for (unsigned int j = 0; j < mb_list->size; ++j) {
			if (((miniblock_t *)mb_node->data)->rw_buffer)
				free(((miniblock_t *)mb_node->data)->rw_buffer);
			mb_node = mb_node->next;
		}

		//ulterior, stergem lista de miniblock uri
		dll_free(&mb_list);
		block_node = block_node->next;
	}

	dll_free(&arena->alloc_list);
	free(arena);
}

void alloc_block(arena_t *arena, const uint64_t address, const uint64_t size)
{
	if (!arena)
		return;

	if (address >= arena->arena_size) {
		printf("The allocated address is outside the size of arena\n");
		return;
	}

	if (address + size > arena->arena_size) {
		printf("The end address is past the size of the arena\n");
		return;
	}

	if (!valid_alloc_adddress(arena, address, size)) {
		printf("This zone was already allocated.\n");
		return;
	}

	miniblock_t miniblock = create_miniblock(address, size);

	list_t *block_list = arena->alloc_list;
	node_t *block_node = block_list->head;
	unsigned int i = 0;

	for (; i < block_list->size; ++i) {
		block_t *block = (block_t *)block_node->data;

		if (miniblock.start_address == block->start_address + block->size) {
			dll_add_nth_node((list_t *)block->miniblock_list,
							 ((list_t *)block->miniblock_list)->size,
							 &miniblock);
			block->size += miniblock.size;

			//testam posibila unificare a 2 block-uri
			merge_blocks(block_node, arena->alloc_list, i);
			return;
		}

		if (miniblock.start_address + miniblock.size == block->start_address) {
			dll_add_nth_node((list_t *)block->miniblock_list, 0,
							 &miniblock);
			block->start_address = miniblock.start_address;
			block->size += miniblock.size;
			return;
		}
		block_node = block_node->next;
	}

	/*daca am ajuns aici, miniblock ul nu este adiacent altor block uri
	prin urmare vom crea un block nou*/
	block_t new_block = create_block(miniblock);

	/*il adaugam in lista astfel incat lista de block-uri sa
	ramane sortata*/
	block_node = block_list->head;
	for (i = 0; i < block_list->size; ++i) {
		block_t *block = (block_t *)block_node->data;

		if (new_block.start_address < block->start_address)
			break;
		block_node = block_node->next;
	}

	dll_add_nth_node(block_list, i, &new_block);
}

void free_block(arena_t *arena, const uint64_t address)
{
	if (!arena)
		return;

	node_t *block_node = NULL;
	node_t *mb_node = NULL;
	int block_pos = -1, mb_pos = -1;

	if (!valid_mb_address(arena, &block_node, &mb_node, address,
						  &block_pos, &mb_pos)) {
		printf("Invalid address for free.\n");
		return;
	}

	block_t *block = (block_t *)block_node->data;
	list_t *mb_list = (list_t *)block->miniblock_list;
	miniblock_t *mb = (miniblock_t *)mb_node->data;

	block->size -= mb->size;
	if (mb_pos == FIRST)
		block->start_address =
		((miniblock_t *)mb_node->next->data)->start_address;

	if (mb->rw_buffer)
		free(mb->rw_buffer);
	dll_remove_nth_node(mb_list, mb_pos);
	free(mb_node->data);
	free(mb_node);

	if (!mb_list->size) {
		dll_remove_nth_node(arena->alloc_list, block_pos);
		free(block->miniblock_list);
		free(block_node->data);
		free(block_node);
		return;
	}

	split_blocks(arena, block_node, block_pos);
}

void write(arena_t *arena, const uint64_t address,  const uint64_t size,
		   int8_t *data)
{
	block_t *block = find_block(arena, address);
	if (!block) {
		printf("Invalid address for write.\n");
		return;
	}

	node_t *mb_node = find_miniblock_node(block, address);
	if (!mb_node) {
		printf("Invalid address for write.\n");
		return;
	}

	if (!valid_perm(block->miniblock_list, mb_node, size,
					address, write_permissions)) {
		printf("Invalid permissions for write.\n");
		return;
	}

	list_t *mb_list = (list_t *)block->miniblock_list;
	unsigned int length = 0; //lungimea pe care am scris-o

	do {
		miniblock_t *mb = (miniblock_t *)mb_node->data;
		mb->rw_buffer = malloc(mb->size);
		DIE(!mb->rw_buffer, "malloc failed\n");

		if (length)
			for (unsigned int i = 0; i < mb->size && length < size; ++i) {
				memcpy(mb->rw_buffer + i, data + length, 1);
				length++;
			}
		else
			for (unsigned int i = (address - mb->start_address); i <
				mb->size && length < size; ++i) {
				memcpy(mb->rw_buffer + i, data + length, 1);
				length++;
			}

		mb_node = mb_node->next;
	} while (mb_node != mb_list->head && length < size);

	int remaining_chars = size - length;

	if (remaining_chars) {
		printf("Warning: size was bigger than the block size. ");
		printf("Writing %d characters.\n", length);
	}
}

void read(arena_t *arena, uint64_t address, uint64_t size)
{
	block_t *block = find_block(arena, address);
	if (!block) {
		printf("Invalid address for read.\n");
		return;
	}

	node_t *mb_node = find_miniblock_node(block, address);
	if (!mb_node) {
		printf("Invalid address for read.\n");
		return;
	}

	if (!valid_perm(block->miniblock_list, mb_node, size,
					address, read_permissions)) {
		printf("Invalid permissions for read.\n");
		return;
	}

	list_t *mb_list = (list_t *)block->miniblock_list;
	unsigned int length = 0; //lungimea pe care am citit-o

	char string[NMAX];
	do {
		miniblock_t *mb = (miniblock_t *)mb_node->data;
		if (length)
			for (unsigned int i = 0; i < mb->size && length < size; ++i) {
				string[length] = *(char *)(mb->rw_buffer + i);
				length++;
			}
		else
			for (unsigned int i = (address - mb->start_address); i <
				mb->size && length < size; ++i) {
				string[length] = *(char *)(mb->rw_buffer + i);
				length++;
			}

		mb_node = mb_node->next;
	} while (mb_node != mb_list->head && length < size);
	string[length] = '\0';
	int remaining_chars = size - length;

	if (remaining_chars) {
		printf("Warning: size was bigger than the block size. ");
		printf("Reading %d characters.\n", length);
	}
	printf("%s\n", string);
}

void pmap(const arena_t *arena)
{
	printf("Total memory: 0x%lX bytes\n", arena->arena_size);
	printf("Free memory: 0x%lX bytes\n", get_free_memory(arena));
	printf("Number of allocated blocks: %d\n", arena->alloc_list->size);
	printf("Number of allocated miniblocks: %d\n", miniblock_nr(arena));
	if (!arena->alloc_list->size)
		return;

	//parcurgem fiecare block si miniblock
	node_t *block_node = arena->alloc_list->head;
	for (int i = 0; i < (int)arena->alloc_list->size; ++i) {
		block_t *block = (block_t *)block_node->data;
		printf("\nBlock %d begin\n", i + 1);
		printf("Zone: 0x%lX - 0x%lX\n", block->start_address,
			   (block->start_address + block->size));

		node_t *mb_node = ((list_t *)block->miniblock_list)->head;

		for (int j = 0; j < (int)((list_t *)block->miniblock_list)->size;
			 ++j) {
			miniblock_t *mb = (miniblock_t *)mb_node->data;
			printf("Miniblock %d:		0x%lX		-		0x%lX		| ",
				   j + 1, mb->start_address, (mb->start_address + mb->size));
			print_perms(mb);
			mb_node = mb_node->next;
		}
		printf("Block %d end\n", i + 1);
		block_node = block_node->next;
	}
}

void mprotect(arena_t *arena, uint64_t address, int8_t *permission)
{
	block_t *block = find_block(arena, address);
	if (!block) {
		printf("Invalid address for mprotect.\n");
		return;
	}

	node_t *mb_node = find_miniblock_node(block, address);
	if (!mb_node) {
		printf("Invalid address for mprotect.\n");
		return;
	}

	if (((miniblock_t *)mb_node->data)->start_address != address) {
		printf("Invalid address for mprotect.\n");
		return;
	}

	uint8_t perm = 0;
	char *token = strtok((char *)permission, " |");
	while (token) {
		if (!strcmp(token, "PROT_NONE"))
			break;
		else if (!strcmp(token, "PROT_READ"))
			perm += 4;
		else if (!strcmp(token, "PROT_WRITE"))
			perm += 2;
		else if (!strcmp(token, "PROT_EXEC"))
			perm += 1;

		token = strtok(NULL, " |");
	}
	((miniblock_t *)mb_node->data)->perm = perm;
}
