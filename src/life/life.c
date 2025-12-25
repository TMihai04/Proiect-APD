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
    printf(" (%c)", IS_ALIVE(num) ? 'X' : '.');
    fflush(stdout);
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
        printf("line: %s\nlen:%d\n\n", line, line_len);
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