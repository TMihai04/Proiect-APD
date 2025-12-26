#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "mpi.h"

#include "life/life.h"

#define DEBUG

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

        printf("Initial generation:\n\n");
        mprint_binc(serial_buffer, rows_real, cols_real, 'X', '.');
        printf("\n---\t---\t---\n\n");

        float tstart_serial = MPI_Wtime();
        for(int gen = 0; gen < generations; gen++) {
            next_gen(serial_buffer, rows_real, cols_real);

            #ifdef DEBUG
            printf("Generation %d:\n\n", gen + 1);
            mprint_binc(serial_buffer, rows_real, cols_real, 'X', '.');
            printf("\n---\t---\t---\n\n");
            #endif
        }
        float tend_serial = MPI_Wtime();

        float telapsed_serial = tend_serial - tstart_serial;
        printf("End result:\n");
        printf("* Time elapsed: %f [s]\n\n", telapsed_serial);
        mprint_binc(serial_buffer, rows_real, cols_real, 'X', '.');
        printf("\n---\t---\t---\n\n");

        fwrite_gen(".\\outputs\\bacteria10\\bacteria10_serial.txt", serial_buffer, rows_real, cols_real, telapsed_serial);

        free(serial_buffer);
    }

    MPI_Finalize();

    return 0;
}