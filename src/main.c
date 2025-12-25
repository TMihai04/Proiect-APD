#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "mpi.h"

#include "life/life.h"

/*
    Compile:
    gcc -Wall -g src/main.c src/life/life.h src/life/life.c -I "c:\Program Files (x86)\Microsoft SDKs\MPI\Include" -L "c:\Program Files (x86)\Microsoft SDKs\MPI\Lib\x64" -lmsmpi -o life_mpi.exe
*/

void usage(char* prg) {
    printf("Usage:mpiexec -n <prc_cnt> %s <file_in> <num_gens>\n", prg);
    fflush(stdout);
}


int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    int rank = -1;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    int comm_size = -1;
    MPI_Comm_size(MPI_COMM_WORLD, &comm_size);

    if(rank == 0) {
        if(argc != 3) {
            usage(argv[0]);
            MPI_Abort(MPI_COMM_WORLD, 0);
        }

        // argv[2]: Number of generations
        int generations = -1;
        char* endptr = NULL;

        generations = strtol(argv[2], &endptr, 10);
        if(strlen(endptr) > 0) {
            printf("`<num_gens>` should be an integer");
            fflush(stdout);
            MPI_Abort(MPI_COMM_WORLD, -1);
        }

        // --- Serial Version ---

        // argv[1]: Input file name
        int rows = -1, columns = -1;
        uint8_t* serial_buffer = fload_gen(argv[1], &rows, &columns);

        int rows_real = rows + 2;
        int cols_real = columns + 2;

        mprint_binc(serial_buffer, rows_real, cols_real, 'X', '.');
        printf("\n---\t---\t---\n\n");

        int st[2] = {1, 1};
        int ed[2] = {10, 10};
        uint8_t* chunk = get_chunk(serial_buffer, rows_real, cols_real, st, ed);

        mprint_binc(chunk, rows, columns, 'X', '.');
        printf("\n---\t---\t---\n\n");

        solver(chunk, rows, columns);
        mprint_binc(chunk, rows, columns, 'X', '.');
        printf("\n---\t---\t---\n\n");

        updater(chunk, rows, columns);
        mprint_binc(chunk, rows, columns, 'X', '.');
        printf("\n---\t---\t---\n\n");

        place_chunk(serial_buffer, rows_real, cols_real, chunk, st, ed);
        mprint_binc(serial_buffer, rows_real, cols_real, 'X', '.');
        printf("\n---\t---\t---\n\n");

        free(chunk);
        free(serial_buffer);
    }

    MPI_Finalize();

    return 0;
}