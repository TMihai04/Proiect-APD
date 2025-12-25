#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>

#include "life.h"


// Swaps 2 pointers
void swapp(void** a, void** b) {
    void* aux = *a;
    *a = *b;
    *b = aux;
}


void print_bin(uint8_t num) {
    for(int i = 7; i >= 0; i--) {
        printf("%d", num & (1 << i) ? 1 : 0);
    }
    fflush(stdout);
}


void print_binc(uint8_t num, char alive, char n_alive) {
    for(int i = 7; i >= 0; i--) {
        printf("%d", num & (1 << i) ? 1 : 0);
    }
    printf(" (%c)", IS_ALIVE(num) ? alive : n_alive);
    fflush(stdout);
}


void mprint_binc(
    uint8_t* mat, 
    int rows, 
    int cols, 
    char alive, 
    char n_alive
) {
    for(int i = 0; i < rows; i++) {
        for(int j = 0; j < cols; j++) {
            print_binc(mat[i * rows + j], alive, n_alive);
            printf(" ");
        }
        printf("\n");
    }
}


// NOTE: Do NOT forget to free the returned pointer
uint8_t* get_chunk(
    uint8_t* buffer, // full buffer of cells
    int rows, // buffer row count
    int columns, // columns row count
    int* start, // int[2] start corner (upper left) - inclusive
    int* end // int[2] end corner (lower right) - inclusive
) {
    int chunk_rows = end[1] - start[1] + 1;
    int chunk_cols = end[0] - start[0] + 1;

    uint8_t* chunk = calloc(chunk_rows * chunk_cols, sizeof(uint8_t));
    if(!chunk) {
        perror("Error allocating chunk array");
        exit(errno);
    }

    for(int i = start[1]; i <= end[1]; i++) {
        for(int j = start[0]; j <= end[0]; j++) {
            int buff_idx = i * columns + j;
            int chunk_idx = (i - start[1]) * chunk_cols + (j - start[0]);
            chunk[chunk_idx] = buffer[buff_idx];
        }
    }

    return chunk;
}


void place_chunk(
    uint8_t* buffer,
    int rows,
    int columns,
    uint8_t* chunk,
    int* start,
    int* end
) {
    // int chunk_rows = end[1] - start[1] + 1;
    int chunk_cols = end[0] - start[0] + 1;

    for(int i = start[1]; i <= end[1]; i++) {
        for(int j = start[0]; j <= end[0]; j++) {
            int buff_idx = i * columns + j;
            int chunk_idx = (i - start[1]) * chunk_cols + (j - start[0]);
            buffer[buff_idx] = chunk[chunk_idx];
        }
    }
}


// Loads a generation into an array (interpreted as a matrix, with a buffer of 0's of size 1), and passes the number of rows and columns as parameters (this does not take into account the 0 buffer)
uint8_t* fload_gen(
    char* in_file_name, // Input file's name
    int* rows, // Variable that will hold the amount of rows the matrix has
    int* columns // Variable that will hold the amount of collums the matrix has
) {
    // Open input file and read dimentions
    FILE* in_file = fopen(in_file_name, "r");
    if(!in_file) {
        perror("Error while opening input file");
        exit(errno);
    }

    fscanf(in_file, "%d%d", rows, columns);

    // Declare generation buffer
    int rows_real = *rows + 2;
    int cols_real = *columns + 2;
    uint8_t* buffer = calloc(rows_real * cols_real, sizeof(uint8_t));
    if(!buffer) {
        perror("Error while allocating initial buffer");
        exit(errno);
    }

    // Read rest of input file
    int rw = 1, cl = 1;
    while(rw <= *rows) {
        char line[IN_CHUNK];
        if(!fgets(line, IN_CHUNK, in_file)) {
            perror("Error at reading input line");
            exit(errno);
        }

        int line_len = strlen(line);
        if(line[line_len - 1] == '\n') {
            line[line_len - 1] = '\0';
        }

        line_len = strlen(line);
        // printf("line: %s\nlen:%d\n\n", line, line_len);
        for(int i = 0; i < line_len; i++) {
            int idx = rw * cols_real + cl;
            int idx_w = rw * cols_real + (cl - 1);
            int idx_n = (rw - 1) * cols_real + cl;

            switch(line[i]) {
                case 'X': 
                    buffer[idx] |= CELL_ALIVE | (IS_ALIVE(buffer[idx_w]) * CELL_WEST) | (IS_ALIVE(buffer[idx_n]) * CELL_NORTH);
                    buffer[idx_w] |= CELL_EAST;
                    buffer[idx_n] |= CELL_SOUTH; 
                    break;
                case '.':
                    buffer[idx] |= (IS_ALIVE(buffer[idx_w]) * CELL_WEST) | (IS_ALIVE(buffer[idx_n]) * CELL_NORTH);
                    break;
                default:
                    printf("Invalid character at input `%c`", line[i]);
                    fflush(stdout);
                    exit(-1);
            }

            cl++;
            if(cl > *columns) {
                cl = 1;
            }
        }

        if(line_len > 0) {
            rw++;
        }
    }

    fclose(in_file);

    return buffer;
}


// Solver for the next generation. In place modifications
void solver(uint8_t* cells, int rows, int cols) {
    for(int i = 0; i < rows; i++) {
        for(int j = 0; j < cols; j++) {
            int idx = i * cols + j;
            cells[idx] = (cells[idx] & 0xfe) | MAKE_ALIVE(cells[idx]);
        }
    }
}


// Exact same functionality as the solver, except it calls the updater at the end
void usolver(uint8_t* cells, int rows, int cols) {
    solver(cells, rows, cols);
    updater(cells, rows, cols);
}


void updater(uint8_t* cells, int rows, int cols) {
    // row = 0 and col = 0 is the "bandaid" from the other operation, so we start from row = 1, col = 1
    for(int i = 1; i < rows; i++) {
        for(int j = 1; j < cols; j++) {
            int idx = i * cols + j;
            int idx_n = (i - 1) * cols + j;
            int idx_w = i * cols + (j - 1);

            cells[idx] = IS_ALIVE(cells[idx]) | (CELL_NORTH * IS_ALIVE(cells[idx_n])) | (CELL_WEST * IS_ALIVE(cells[idx_w]));
            cells[idx_n] = (cells[idx_n] & ~CELL_SOUTH) |  (CELL_SOUTH * IS_ALIVE(cells[idx]));
            cells[idx_w] = (cells[idx_w] & ~CELL_EAST) | (CELL_EAST * IS_ALIVE(cells[idx]));
        }
    }
}


// Senquential solver for a next generation. Needs the whole buffer. In-place operation
void next_gen_classic(uint8_t* buffer, int buff_rows, int buff_cols) {
    int wz_rows = buff_rows - 2;
    int wz_cols = buff_cols - 2;

    int from[2] = {1, 1};
    int to[2] = {wz_cols, wz_rows};
    uint8_t* work_zone = get_chunk(buffer, buff_rows, buff_cols, from, to);

    usolver(work_zone, wz_rows, wz_cols);
    place_chunk(buffer, buff_rows, buff_cols, work_zone, from, to);

    free(work_zone);

    // vertical
    from[0] = 0; // x0
    from[1] = 0; // y0
    to[0] = 2; // x1
    to[1] = wz_rows + 1; // y1
    uint8_t* patch = get_chunk(buffer, buff_rows, buff_cols, from, to);
    // printf("Patch vertical:\n");
    // mprint_binc(patch, wz_rows + 1, 2, 'X', '.');
    // printf("\n---\t---\t---\n\n");

    updater(patch, wz_rows + 2, 3);
    from[0] = 1; from[1] = 1;
    to[0] = 1; to[1] = wz_rows;
    uint8_t* patch_chunk = get_chunk(patch, wz_rows + 2, 3, from, to);
    place_chunk(buffer, buff_rows, buff_cols, patch_chunk, from, to);

    free(patch);
    free(patch_chunk);

    // mprint_binc(patch, wz_rows + 1, 2, 'X', '.');
    // printf("\n---\t---\t---\n\n");
    
    // horizontal
    from[0] = 0; // x0
    from[1] = 0; // y0
    to[0] = wz_cols + 1; // x1
    to[1] = 2; // y1
    patch = get_chunk(buffer, buff_rows, buff_cols, from, to);

    updater(patch, 3, wz_cols + 2);
    from[0] = 1; from[1] = 1;
    to[0] = wz_cols; to[1] = 1;
    patch_chunk = get_chunk(patch, 3, wz_cols + 2, from, to); 
    place_chunk(buffer, buff_rows, buff_cols, patch_chunk, from, to);

    free(patch);
    free(patch_chunk);
}