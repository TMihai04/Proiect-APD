#ifndef _LIFE
#define _LIFE

#include <stdint.h>

#define IN_CHUNK 1024

/* Constants */
#define CELL_ALIVE  0b00000001
#define CELL_WEST   0b00000010
#define CELL_NORTH  0b00000100
#define CELL_EAST   0b00001000
#define CELL_SOUTH  0b00010000

/* Macros */
// 0x01 if active / has neighbour, 0x00 otherwise
#define IS_ALIVE(cell) (cell & CELL_ALIVE)
#define HAS_WEST(cell) ((cell & CELL_WEST) >> 1)
#define HAS_NORTH(cell) ((cell & CELL_NORTH) >> 2)
#define HAS_EAST(cell) ((cell & CELL_EAST) >> 3)
#define HAS_SOUTH(cell) ((cell & CELL_SOUTH) >> 4)

// Used a karnough map
// Minterms: 7,11,13,14,15,19,21,22,23,25,26,27,28,29
#define MAKE_ALIVE(cell) ((~HAS_SOUTH(cell) & HAS_EAST(cell) & HAS_NORTH(cell) & HAS_WEST(cell)) | (HAS_SOUTH(cell) & ~HAS_EAST(cell) & HAS_NORTH(cell) & HAS_WEST(cell)) | (HAS_SOUTH(cell) & HAS_EAST(cell) & ~HAS_NORTH(cell) & HAS_WEST(cell)) | (HAS_SOUTH(cell) & HAS_EAST(cell) & HAS_NORTH(cell) & ~HAS_WEST(cell)) | (~HAS_SOUTH(cell) & HAS_NORTH(cell) & HAS_WEST(cell) & IS_ALIVE(cell)) | (~HAS_SOUTH(cell) & HAS_EAST(cell) & HAS_WEST(cell) & IS_ALIVE(cell)) | (~HAS_SOUTH(cell) & HAS_EAST(cell) & HAS_NORTH(cell) & IS_ALIVE(cell)) | (HAS_SOUTH(cell) & ~HAS_EAST(cell) & HAS_WEST(cell) & IS_ALIVE(cell)) | (HAS_SOUTH(cell) & ~HAS_EAST(cell) & HAS_NORTH(cell) & IS_ALIVE(cell)) | (HAS_SOUTH(cell) & HAS_EAST(cell) & ~HAS_NORTH(cell) & IS_ALIVE(cell)))

// Same rules
// Is just the negation of the above logical expression
#define MAKE_DEAD(cell) ( (HAS_SOUTH(cell) | HAS_EAST(cell) | HAS_NORTH(cell)) & (HAS_SOUTH(cell) | HAS_EAST(cell) | HAS_WEST(cell)) & (HAS_SOUTH(cell) | HAS_EAST(cell) | IS_ALIVE(cell)) & (HAS_SOUTH(cell) | HAS_NORTH(cell) | HAS_WEST(cell)) & (HAS_SOUTH(cell) | HAS_NORTH(cell) | IS_ALIVE(cell)) & (HAS_SOUTH(cell) | HAS_WEST(cell) | IS_ALIVE(cell)) & (HAS_EAST(cell) | HAS_NORTH(cell) | HAS_WEST(cell)) & (HAS_EAST(cell) | HAS_NORTH(cell) | IS_ALIVE(cell)) & (HAS_EAST(cell) | HAS_WEST(cell) | IS_ALIVE(cell)) & (HAS_NORTH(cell) | HAS_WEST(cell) | IS_ALIVE(cell)) & (~HAS_SOUTH(cell) | ~HAS_EAST(cell) | ~HAS_NORTH(cell) | ~HAS_WEST(cell)))

/* Utils */
// void swap(void*, void*);
void swapp(void**, void**);

void print_bin(uint8_t);
void print_binc(uint8_t, char, char);
void mprint_binc(uint8_t*, int, int, char, char);

uint8_t* get_chunk(uint8_t*, int, int, int*, int*);
void place_chunk(uint8_t*, int, int, uint8_t*, int*, int*);

void validate_path(char*);

/* I/O */
uint8_t* fload_gen(char*, int*, int*);
void fwrite_gen(char*, uint8_t*, int, int, float);

/* Work */
// In place solver, using the per cell data and the two above macros
/*
    Args:
        uint8_t*: Matrix array chunk
        int: rows of given matrix chunk
        int: columns of given matrix chunk
*/
void solver(uint8_t*, int, int);
void usolver(uint8_t*, int, int);
// Updater that works similarly to the updates done in the `fload_gen(...)` method (life.c: 74)
/*
    Args:
        uint8_t*: Matrix array chunk
        int: rows of given matrix chunk
        int: columns of given matrix chunk
        **NOTE:**:
            - If the starting position is (1, 1) this is the target cell, and it will work with cells (0, 1) and (1, 0).
            - So for a call `updater(arr, (x0, y0), (x1, y1))` the arr will have to be a box with the boundries `rect(x0 - 1, y0 - 1, x1, y1)`
*/
void updater(uint8_t*, int, int);
void next_gen(uint8_t*, int, int);

#endif