#ifndef _LIFE
#define _LIFE

#include <stdint.h>

#define IN_CHUNK 1024
#define OUT_DIR "outputs"

/* Constants */
#define CELL_ALIVE  0b00000001
#define CELL_WEST   0b00000010
#define CELL_NORTH  0b00000100
#define CELL_EAST   0b00001000
#define CELL_SOUTH  0b00010000

#define PARALLEL_1D_TAG 69
#define PARALLEL_2D_TAG 420
#define WAIT_TAG        80085
#define DONE_TAG        1337

#define HEADER_TAG      0
#define DATA_TAG        1

#define MINIMUM_1D      2
#define MINIMUM_2D      4

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

#define MAX(a, b) ((a > b) ? a : b)
#define MIN(a, b) ((a < b) ? a : b)

/* Debug */
extern int _ldebug;

/* Types */
// This struct does not need to be opaque
typedef struct _area_t {
    int from[2];
    int to[2];
} area_t;

/* Utils */
// void swap(void*, void*);
void swapp(void** a, void** b);
int mequal(uint8_t* m1, uint8_t* m2, int rows, int cols);

void print_area(area_t area);
void print_areas(area_t* areas, int len);

void print_bin(uint8_t num);
void print_binc(uint8_t num, char alive, char n_alive);
void mprint_binc(uint8_t* mat, int rows, int cols, char alive, char n_alive);

uint8_t* get_chunk(uint8_t* buffer, int rows, int columns, int* start, int* end);
void place_chunk(uint8_t* buffer, int rows, int columns, uint8_t* chunk, int* start, int* end);

void validate_path(char* path);
char* get_output_path(char* in_name, char* type);

/* I/O */
uint8_t* fload_gen(char* in_file_name, int* rows, int* columns);
void fwrite_gen(char* out_file_name, uint8_t* cells, int rows, int cols, float t_elapsed);

/* Work */
// In place solver, using the per cell data and the two above macros
/*
    Args:
        uint8_t*: Matrix array chunk
        int: rows of given matrix chunk
        int: columns of given matrix chunk
*/
void solver(uint8_t* cells, int rows, int cols);
void usolver(uint8_t* cells, int rows, int cols);
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
void updater(uint8_t* cells, int rows, int cols);
void next_gen(uint8_t* buffer, int buff_rows, int buff_cols);

area_t* create_jobs_1d(int rows, int columns, int workers, int* job_cnt);
area_t* create_jobs_2d(int rows, int columns, int workers, int* job_cnt, int* workers_x);

void worker_parallel_1d(int rank, int nworkers);
void worker_parallel_2d(int rank, int nworkers, int workers_x);

#endif