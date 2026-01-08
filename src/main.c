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


int rank = -1;
int comm_size = -1, worker_cnt = -1;

int rows = -1, columns = -1;
int rows_real = -1, cols_real = -1;
uint8_t* serial_buffer = NULL;
uint8_t* parallel_1d_buffer = NULL;
uint8_t* parallel_2d_buffer = NULL;

int init_from[2], init_to[2];
float tstart = -1, tend = -1, telapsed = 1;
char* output_path = NULL;

int job_1d_cnt = -1;
area_t* jobs_1d = NULL;


void usage(char* prg) {
    printf("Usage:mpiexec -n <prc_cnt> %s <file_in> <num_gens>\n", prg);
    fflush(stdout);
}


int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    MPI_Comm_size(MPI_COMM_WORLD, &comm_size);
    worker_cnt = comm_size - 1;

    // Main Process
    if(rank == 0) {
        if(argc != 3) {
            usage(argv[0]);
            MPI_Abort(MPI_COMM_WORLD, 0);
        }

        if(comm_size < 2) {
            printf("At least 2 processes needed. Got %d instead\n", comm_size);
            fflush(stdout);
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

        // argv[1]: Input file name
        serial_buffer = fload_gen(argv[1], &rows, &columns);

        rows_real = rows + 2;
        cols_real = columns + 2;

        init_from[0] = 0; init_from[1] = 0;
        init_to[0] = cols_real - 1; init_to[1] = rows_real - 1;

        parallel_1d_buffer = get_chunk(serial_buffer, rows_real, cols_real, init_from, init_to);
        parallel_2d_buffer = get_chunk(serial_buffer, rows_real, cols_real, init_from, init_to);

        #ifdef DEBUG
        printf("Initial generation serial:\n\n");
        mprint_binc(serial_buffer, rows_real, cols_real, 'X', '.');
        printf("\n---\t---\t---\n\n");

        printf("Initial generation parallel 1d:\n\n");
        mprint_binc(serial_buffer, rows_real, cols_real, 'X', '.');
        printf("\n---\t---\t---\n\n");

        printf("Initial generation parallel 2d:\n\n");
        mprint_binc(serial_buffer, rows_real, cols_real, 'X', '.');
        printf("\n---\t---\t---\n\n");
        #endif

        // --- Serial Version ---
        printf("\n\n-------\tSERIAL VERSION\t-------\n\n");

        printf("Initial generation:\n\n");
        mprint_binc(serial_buffer, rows_real, cols_real, 'X', '.');
        printf("\n---\t---\t---\n\n");

        tstart = MPI_Wtime();
        for(int gen = 0; gen < generations; gen++) {
            next_gen(serial_buffer, rows_real, cols_real);

            #ifdef DEBUG
            printf("Generation %d:\n\n", gen + 1);
            mprint_binc(serial_buffer, rows_real, cols_real, 'X', '.');
            printf("\n---\t---\t---\n\n");
            #endif
        }
        tend = MPI_Wtime();

        telapsed = tend - tstart;
        printf("End result:\n");
        printf("* Time elapsed: %f [s]\n\n", telapsed);
        mprint_binc(serial_buffer, rows_real, cols_real, 'X', '.');
        printf("\n---\t---\t---\n\n");

        output_path = get_output_path(argv[1], "serial");
        fwrite_gen(output_path, serial_buffer, rows_real, cols_real, telapsed);
        free(output_path);

        // -- Parallel version 1 - 1D data decomposition --
        printf("\n\n-------\tPARALLEL VERSION - 1D\t-------\n\n");
        jobs_1d = create_jobs_1d(rows, columns, worker_cnt, &job_1d_cnt);
        print_areas(jobs_1d, job_1d_cnt);
        free(jobs_1d);

        for(int i = 0; i < worker_cnt; i++) {
            MPI_Send(NULL, 0, MPI_INT, i + 1, PARALLEL_1D_TAG, MPI_COMM_WORLD);
        }


        // -- Parallel version 1 - 2D data decomposition --
        printf("\n\n-------\tPARALLEL VERSION - 2D\t-------\n\n");

        for(int i = 0; i < worker_cnt; i++) {
            MPI_Send(NULL, 0, MPI_INT, i + 1, PARALLEL_2D_TAG, MPI_COMM_WORLD);
        }


        // -- Clean-up the workspace --
        for(int i = 0; i < worker_cnt; i++) {
            MPI_Send(NULL, 0, MPI_INT, i + 1, DONE_TAG, MPI_COMM_WORLD);
        }


        free(serial_buffer);
        free(parallel_1d_buffer);
        free(parallel_2d_buffer);
    }
    // Worker processes
    else {
        MPI_Status mode_status;

        while(true) {
            MPI_Recv(NULL, 0, MPI_INT, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &mode_status);

            switch(mode_status.MPI_TAG) {
                case PARALLEL_1D_TAG:
                    printf("Parallel mode 1d - [%d]\n", rank);
                    fflush(stdout);
                    break;
                case PARALLEL_2D_TAG:
                    printf("Parallel mode 2d - [%d]\n", rank);
                    fflush(stdout);
                    break;
                case DONE_TAG:
                    printf("DONE! - [%d]\n", rank);
                    fflush(stdout);
                    goto done;
            }
        }

        done:
            printf("Exited while loop - [%d]\n", rank);
            fflush(stdout);
        // -- Parallel version 1 - 1D data decomposition --


        // -- Parallel version 1 - 2D data decomposition --
    }

    MPI_Finalize();

    return 0;
}