## VMS - Copyright Carauleanu Valentin Gabriel 311CA

## Introduction

The programm has three static libraries:

1. `vma.h` - This contains the main functions of the program.
2. `aux_functions.h` - This includes functions to receive and read keyboard commands, and other auxiliary functions related to `vma.h`.
3. `lists.h` - This has functions that allow the implementation of circular doubly-linked lists.

## Memory Allocation Simulator

The memory allocation simulator is implemented based on the concept of lists in lists:

- `arena` stores a pointer to a circular doubly-linked list, with each node containing a pointer to a `block_t` structure.
- Each `block` contains a pointer to another circular doubly-linked list of `miniblock_t` structures, along with `size` and `start_address`.

## Functions Description

### Arena Allocation:
The `alloc_arena` function sets up an `arena` with a specified length and an empty circular doubly-linked list of `block_t` elements.

### Arena Deallocation:
The `dealloc_arena` function goes through each block and each miniblock within, removing potential buffers and freeing up the lists of miniblocks and blocks. The arena is then released.

### Block Allocation: 
This function validates the memory address and creates a miniblock if it can fit within the arena and does not overlap an existing block. It either merges the new miniblock with an existing block, if possible, or creates a new block for it.

### Buffer Allocation:
The buffer memory is only allocated at the moment of a write command, not during miniblock creation.

### Block Release:
This function validates the memory address (it should be the start address of a miniblock) and removes the respective miniblock from its parent block's list. The block size is reduced by the miniblock size, and the block's start address is updated if needed. If the block's miniblock list is empty, the block is deleted.

### Read and Write:
These functions validate the given address (it should be included in a miniblock), then check if all memory areas within the specified range have the necessary permissions for read/write operations. These functions start reading/writing from the given address until they reach the end of the miniblock list or until the specified size is reached.

### Mprotect:
This function validates the given address and processes the permissions command to get the numerical representation of the permissions. The miniblock's permissions are then updated.

## Development Experience

The most challenging aspect of creating `my_vma` was understanding the project description. The concept of VMA was new and the transition from VMA to the VMA simulator to be implemented was difficult. Functions like `merge_blocks` and `split_blocks` were challenging but rewarding. Implementing the read and write functions was interesting since the read/write operation could continue from one miniblock to another. Here's hoping that the next project will be less abstract.
