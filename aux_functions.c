//copyright Carauleanu Valentin Gabriel 311CA
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "vma.h"
#include "lists.h"
#define NMAX 500

void command(int *cmd, char *string)
{
	/*pentru un meniu mai eye-candy vom transforma comanda primita
	in valori numerice */

	if (!strcmp(string, "ALLOC_ARENA"))
		*cmd = 1;
	else if (!strcmp(string, "ALLOC_BLOCK"))
		*cmd = 2;
	else if (!strcmp(string, "FREE_BLOCK"))
		*cmd = 3;
	else if (!strcmp(string, "READ"))
		*cmd = 4;
	else if (!strcmp(string, "WRITE"))
		*cmd = 5;
	else if (!strcmp(string, "PMAP"))
		*cmd = 6;
	else if (!strcmp(string, "MPROTECT"))
		*cmd = 7;
	else if (!strcmp(string, "DEALLOC_ARENA"))
		*cmd = 8;
	else
		*cmd = -1;
}

char *read_cmd(void)
{
	// citim comanda primita
	char command[NMAX];
	scanf("%s", command);

	// eliminam \n de la finalul string ului citit
	int len = strlen(command);
	if (command[len - 1] == '\n') {
		command[len - 1] = '\0';
		len--;
	}
	// crestem len pentru a copia si \0
	len++;

	char *cmd = malloc(len * sizeof(char));
	if (!cmd)
		return NULL;

	strcpy(cmd, command);
	return cmd;
}

void alloc_arena_cmd(arena_t **arena)
{
	uint64_t size;
	scanf("%lu", &size);
	*arena = alloc_arena(size);
}

void alloc_block_cmd(arena_t *arena)
{
	size_t size;
	uint64_t address;

	scanf("%lu%lu", &address, &size);
	alloc_block(arena, address, size);
}

void free_block_cmd(arena_t *arena)
{
	uint64_t address;
	scanf("%lu", &address);
	free_block(arena, address);
}

void write_cmd(arena_t *arena)
{
	uint64_t address, size;
	char data[NMAX];
	scanf("%lu%lu", &address, &size);

	/*datorita formatului comenzilor, la citirea string-ului 'data',
	se citeste si primul 'space', din acest motiv vom citi size + 1
	elemente si vom trimite in functia write un pointer incepand de la
	elementul 1 al string-ului 'data'*/
	char ch = 'a';
	for (uint64_t i = 0; i < size + 1 && ch != '\0'; ++i) {
		ch = getchar();
		data[i] = ch;
	}

	write(arena, address, size, (int8_t *)(data + 1));
}

void read_data_cmd(arena_t *arena)
{
	uint64_t address, size;
	scanf("%lu%lu", &address, &size);

	read(arena, address, size);
}

void mprotect_cmd(arena_t *arena)
{
	uint64_t address;
	scanf("%lu", &address);
	char perm[NMAX];
	fgets(perm, NMAX - 1, stdin);
	/*pentru o prelucrare mai eficienta a datelor in
	functia mprotect vom elimina \n de la finalul string-ului*/
	int len = strlen(perm);
	if (perm[len - 1] == '\n')
		perm[len - 1] = '\0';

	mprotect(arena, address, (int8_t *)perm);
}

uint64_t get_free_memory(const arena_t *arena)
{
	uint64_t used_mem = 0;
	node_t *block_node = arena->alloc_list->head;

	/*parcurgem fiecare block pt a afla memoria totala folosita
	in arena*/
	for (unsigned int i = 0; i < arena->alloc_list->size; ++i) {
		used_mem += ((block_t *)block_node->data)->size;
		block_node = block_node->next;
	}

	return arena->arena_size - used_mem;
}

int valid_alloc_adddress(arena_t *arena, uint64_t address, uint64_t size)
{
	/*functia returneaza 0, pt o adresa de miniblock invalida si 1 in caz
	contrar*/
	list_t *block_list = arena->alloc_list;
	node_t *block_node = block_list->head;

	/*parcurgem lista de block uri si verificam daca adress + size
	se suprapune cu memoria ocupata de un block existent*/
	for (unsigned int i = 0; i < block_list->size; ++i) {
		block_t *block = (block_t *)block_node->data;

		if (address >= block->start_address && address <
			block->start_address + block->size)
			return 0;

		if (block->start_address >= address &&
			block->start_address < address + size)
			return 0;

		block_node = block_node->next;
	}

	return 1;
}

int miniblock_nr(const arena_t *arena)
{
	int total = 0;
	node_t *block_node = arena->alloc_list->head;

	for (unsigned int i = 0; i < arena->alloc_list->size; ++i) {
		total = total +
		((list_t *)((block_t *)block_node->data)->miniblock_list)->size;
		block_node = block_node->next;
	}
	return total;
}

void merge_blocks(node_t *block_node, list_t *block_list, int pos)
{
	block_t *block = (block_t *)block_node->data,
			*next_block = (block_t *)block_node->next->data;

	/*daca adresa + size ale unui block coincide cu adresa de inceput a
	urmatorului block, cele 2 block uri trebuie unificate*/
	if (block->start_address + block->size == next_block->start_address) {
		list_t *removed_list = (list_t *)next_block->miniblock_list;

		/*vom elimina rand pe rand primul miniblock din block-ul 2 si
		il vom adauga in lista primului block */
		while (removed_list->size) {
			node_t *removed_mb_node =
				dll_remove_nth_node(removed_list, FIRST);

			dll_add_nth_node(block->miniblock_list,
							 ((list_t *)block->miniblock_list)->size,
							 removed_mb_node->data);

			free(removed_mb_node->data);
			free(removed_mb_node);
		}

		/*la final eliminam block ul 2, acesta avand lista de
		miniblock uri nula*/
		block->size += next_block->size;
		node_t *removed_block_node =
			dll_remove_nth_node(block_list, pos + 1);

		free(removed_list);
		free(removed_block_node->data);
		free(removed_block_node);
	}
}

miniblock_t create_miniblock(const uint64_t address, const uint64_t size)
{
	miniblock_t miniblock;
	miniblock.start_address = address;
	miniblock.size = size;

	// by default permisiunile sunt rw- a.k.a. 6
	miniblock.perm = 6;
	miniblock.rw_buffer = NULL;

	return miniblock;
}

block_t create_block(miniblock_t miniblock)
{
	block_t new_block;
	new_block.size = miniblock.size;
	new_block.start_address = miniblock.start_address;

	/*cream lista de miniblock uri si adaugam miniblock ul
	primit ca prim element*/
	new_block.miniblock_list = dll_create(sizeof(miniblock_t));
	dll_add_nth_node(new_block.miniblock_list, FIRST, &miniblock);

	return new_block;
}

int valid_mb_address(arena_t *arena, node_t **block_node, node_t **mb_node,
					 uint64_t address, int *block_pos, int *mb_pos)
{
	*block_node = arena->alloc_list->head;

	//parcurgem fiecare block
	for (unsigned int i = 0; i < arena->alloc_list->size; ++i) {
		block_t *block = (block_t *)(*block_node)->data;

		/*daca adresa primita este in interiorul unui block, vom parcurge
		lista de miniblock uri*/
		if (address >= block->start_address && address < block->start_address
			+ block->size) {
			list_t *mb_list = (list_t *)block->miniblock_list;
			(*mb_node) = mb_list->head;

			for (unsigned int j = 0; j < mb_list->size; ++j) {
				miniblock_t *mb = (miniblock_t *)(*mb_node)->data;

				/*daca adresa primita ca argument este egala cu adresa
				de start a unui mb, setam pozitiile block ului si
				mb-ului in block_pos, mb_pos si iesim din functie*/
				if (mb->start_address == address) {
					*block_pos = i;
					*mb_pos = j;
					return 1;
				}
				(*mb_node) = (*mb_node)->next;
			}
			/*daca am ajuns aici, adresa primita nu este adresa de inceput
			a unui miniblock, deci iesim din functie*/
			return 0;
		}
		(*block_node) = (*block_node)->next;
	}
	return 0;
}

void split_blocks(arena_t *arena, node_t *block_node, int block_pos)
{
	block_t *block = (block_t *)block_node->data;
	list_t *mb_list = (list_t *)block->miniblock_list;
	node_t *mb_node = mb_list->head;
	block_t new_block;

	/*parurgem block-ul primit ca arugemnt pentru a verifica, daca
	2 miniblock uri adiacente din lista nu sunt adiacente din punct de
	vedere al memoriei*/
	for (unsigned int i = 0; i < mb_list->size - 1; ++i) {
		if (((miniblock_t *)mb_node->data)->start_address +
			((miniblock_t *)mb_node->data)->size !=
			((miniblock_t *)mb_node->next->data)->start_address) {
			/*ajungand aici, block ul primit trebuie spart in 2, deci
			cream noul block si lista sa de miniblock-uri (inital cu 0 el)*/
			new_block.start_address =
				((miniblock_t *)mb_node->next->data)->start_address;
			new_block.miniblock_list = dll_create(sizeof(miniblock_t));
			new_block.size = 0;

			/*vom elimina toate miniblock urile din block ul inital
			de dupa  pozitia i si le adaugam la finalul listei de mb
			din block ul nou creat*/
			unsigned int size = mb_list->size;
			for (unsigned int j = i + 1; j < size; ++j) {
				node_t *removed = dll_remove_nth_node(mb_list, i + 1);

				new_block.size += ((miniblock_t *)removed->data)->size;
				block->size -= ((miniblock_t *)removed->data)->size;

				dll_add_nth_node(new_block.miniblock_list,
								 ((list_t *)new_block.miniblock_list)->size,
								 (miniblock_t *)removed->data);

				free(removed->data);
				free(removed);
			}
			dll_add_nth_node(arena->alloc_list, block_pos + 1, &new_block);
			return;
		}
		mb_node = mb_node->next;
	}
}

int read_permissions(uint8_t perm)
{
	if (perm >= 4 && perm <= 7)
		return 1;

	return 0;
}

int write_permissions(uint8_t perm)
{
	if (perm == 2 || perm == 7 || perm == 3 || perm == 6)
		return 1;

	return 0;
}

int valid_perm(list_t *mb_list, node_t *mb_node,
			   const uint64_t size, const uint64_t address, perm_t permission)
{
	size_t length = 0;
	node_t *copy = mb_node;

	/*parcurgem mb urile incepand cu miniblock ul primit, verificand
	daca acesta are permisiunile necesare. Parcurgerea este finalizata cand
	am ajuns la finalul listei de mb -uri sau cand lungimea buffer - elor
	parcurse este egala cu size*/
	do {
		miniblock_t *mb = (miniblock_t *)copy->data;

		/*primul mb nu este parcurs neaparat de la inceput, deci nu vom
		adauga in length inteaga sa lungime*/
		!length ? (length += (mb->start_address + mb->size - address)) :
				(length += mb->size);

		if (!permission(mb->perm))
			return 0;

		copy = copy->next;
	} while (copy != mb_list->head && length < size);

	return 1;
}

block_t *find_block(arena_t *arena, uint64_t address)
{
	node_t *block_node = arena->alloc_list->head;

	/*parcurgem lista de block uri pentru a verifica daca
	adresa data este in intervalul de memorie al unui block*/
	for (unsigned int i = 0; i < arena->alloc_list->size; ++i) {
		block_t *block = (block_t *)block_node->data;

		if (address >= block->start_address && address <
			block->start_address + block->size)
			return block;

		block_node = block_node->next;
	}
	return NULL;
}

node_t *find_miniblock_node(block_t *block, uint64_t address)
{
	node_t *mb_node = ((list_t *)block->miniblock_list)->head;

	/*parcurgem lista de miniblock uri pentru a verifica daca
	adresa data este in intervalul de memorie al unui miniblock*/
	for (unsigned int i = 0; i < ((list_t *)block->miniblock_list)->size;
		++i) {
		if (address >= ((miniblock_t *)mb_node->data)->start_address &&
			address < ((miniblock_t *)mb_node->data)->start_address +
			((miniblock_t *)mb_node->data)->size)
			return mb_node;

		mb_node = mb_node->next;
	}

	return NULL;
}

void print_perms(miniblock_t *mb)
{
	int read = 0, write = 0, exec = 0, perm = (int)mb->perm;
	int n = 4;

	/*prelucram permisiunile numerice ale mb-ului primit pentru a
	le printa sub forma de string, la final*/
	while (perm) {
		if (perm - n >= 0) {
			perm -= n;
			switch (n) {
			case 4:
				read = 1;
				break;
			case 2:
				write = 1;
				break;
			case 1:
				exec = 1;
				break;
			}
		}
		n /= 2;
	}

	!read ? printf("-") : printf("R");
	!write ? printf("-") : printf("W");
	!exec ? printf("-\n") : printf("X\n");
}
