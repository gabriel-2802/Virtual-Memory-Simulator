//copyright Carauleanu Valentin Gabriel 311CA
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "vma.h"
#include "lists.h"
#include "aux_functions.h"

int main(void)
{
	char *cmd_string = NULL;
	int cmd = 0, done = 0;
	arena_t *arena = NULL;

	while (1) {
		cmd_string = read_cmd();
		if (!cmd_string) {
			printf("Invalid command\n");
			continue;
		}
		command(&cmd, cmd_string);

		switch (cmd) {
		case 1:
			alloc_arena_cmd(&arena);
			break;
		case 2:
			alloc_block_cmd(arena);
			break;
		case 3:
			free_block_cmd(arena);
			break;
		case 4:
			read_data_cmd(arena);
			break;
		case 5:
			write_cmd(arena);
			break;
		case 6:
			pmap(arena);
			break;
		case 7:
			mprotect_cmd(arena);
			break;
		case 8:
			dealloc_arena(arena);
			done = 1;
			break;
		default:
			printf("Invalid command. Please try again.\n");
			break;
		}

		free(cmd_string);
		if (done)
			break;
	}

	return 0;
}
