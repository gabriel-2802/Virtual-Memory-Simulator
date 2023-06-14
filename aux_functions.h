//copyright Carauleanu Valentin Gabriel 311CA
#pragma once
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include <stddef.h>
#include "lists.h"
#include "vma.h"
#define NMAX 100

/*functia transforma comanda primita intr-0 valoare numerica,
ce urmeaza sa fie folosita intr-o instructiune switch */
void command(int *cmd, char *string);

//functia citeste comanda primita
char *read_cmd(void);

//functia prelucreaza comanda primitas si  apeleaza alloc_arena
void alloc_arena_cmd(arena_t **arena);

//functia prelucreaza comanda primita si apeleaza alloc_block
void alloc_block_cmd(arena_t *arena);

//functia prelucreaza comanda si apeleaza functia free_block
void free_block_cmd(arena_t *arena);

//functia prelucreaza comanda si apeleaza functia write_block
void write_cmd(arena_t *arena);

//functia prelucreaza comanda si apeleaza functia read
void read_data_cmd(arena_t *arena);

//functia prelucreaza comanda si apeleaza functia mprotect
void mprotect_cmd(arena_t *arena);

//functia calculeaza memoria libera din arena
uint64_t get_free_memory(const arena_t *arena);

//functia testeaza daca adresa primita este valid
int valid_alloc_adddress(arena_t *arena, uint64_t address, uint64_t size);

//functia returneaza nr de miniblock-uri din arena
int miniblock_nr(const arena_t *arena);

//functia uneste 2 block uri adiacente
void merge_blocks(dll_node_t *block_node, list_t *block_list, int pos);

//functia creaza un miniblock cu datele primite
miniblock_t create_miniblock(const uint64_t address, const uint64_t size);

//functia creaza un block cu miniblock-ul primit
block_t create_block(miniblock_t miniblock);

/*functia parcurge arena, verificand daca adresa primita este valida,
iar in caz afirmativ seteaza mb_node, block_node, block_pos si mb_pos in
pozitia gasita*/
int valid_mb_address(arena_t *arena, dll_node_t **block_node,
					 dll_node_t **mb_node, uint64_t address,
					 int *block_pos, int *mb_pos);

/*functia desparte un block in doua, daca un miniblock din intriorul sau
a fost eliberat*/
void split_blocks(arena_t *arena, dll_node_t *block_node, int block_pos);

//functia verifica daca permisiunile primite sunt de READ
int read_permissions(uint8_t perm);

//functia verifica daca permisiunile primite sunt de WRITE
int write_permissions(uint8_t perm);

/*functia primeste ca parametru o functie de verificare a permisiunilor
si verifica daca toate miniblock urile incepand de la adresa address pana la
address + size au permisiunile valide*/
int valid_perm(list_t *mb_list, dll_node_t *mb_node, const uint64_t size,
			   const uint64_t address, perm_t permission);

/*functia returneaza block -ul care curpinde adresa data si NULL in
caz contrar*/
block_t *find_block(arena_t *arena, uint64_t address);

dll_node_t *find_miniblock_node(block_t *block, uint64_t address);

/*functia transforma permisunile numerica ale unui minublock in
string-uri si le printeaza*/
void print_perms(miniblock_t *mb);
