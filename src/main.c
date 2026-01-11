#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <mpi.h>

#include "life/life.h"

// #define DEBUG

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
int job_2d_cnt = -1, job_2d_width = -1;
area_t* jobs_1d = NULL;
area_t* jobs_2d = NULL;


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

        // if(comm_size < 1) {
        //     printf("At least one processes needed. Got %d instead\n", comm_size);
        //     fflush(stdout);
        //     MPI_Abort(MPI_COMM_WORLD, 0);
        // }

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

        #ifdef DEBUG
        printf("Initial generation:\n\n");
        mprint_binc(serial_buffer, rows_real, cols_real, 'X', '.');
        printf("\n---\t---\t---\n\n");
        #endif

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
        #ifdef DEBUG
        mprint_binc(serial_buffer, rows_real, cols_real, 'X', '.');
        #endif
        printf("\n---\t---\t---\n\n");

        output_path = get_output_path(argv[1], "serial");
        fwrite_gen(output_path, serial_buffer, rows_real, cols_real, telapsed);
        free(output_path);

        // -- Parallel version 1 - 1D data decomposition --
        // Execution check
        if(worker_cnt == 0) {
            printf("At least 1 worker needed for parallel versions\n"); fflush(stdout);
            goto final_mpi;
        }
        if(worker_cnt < MINIMUM_1D) {
            printf("At least %d workers needed for parallel version 1D. Got %d\n", MINIMUM_1D, worker_cnt); fflush(stdout);
            goto done_msg;
        }

        printf("\n\n-------\tPARALLEL VERSION - 1D\t-------\n\n");
        fflush(stdout);
        jobs_1d = create_jobs_1d(rows, columns, worker_cnt, &job_1d_cnt);
        // print_areas(jobs_1d, job_1d_cnt);

        #ifdef DEBUG
        printf("Initial generation:\n\n");
        mprint_binc(parallel_1d_buffer, rows_real, cols_real, 'X', '.');
        printf("\n---\t---\t---\n\n");
        #endif

        tstart = MPI_Wtime();
        for(int gen = 0; gen < generations; gen++) {
            // Notifies workers of work mode
            for(int i = 0; i < job_1d_cnt; i++) {
                MPI_Send(&job_1d_cnt, 1, MPI_INT, i + 1, PARALLEL_1D_TAG, MPI_COMM_WORLD);
            }
            for(int i = job_1d_cnt; i < worker_cnt; i++) {
                MPI_Send(&job_1d_cnt, 1, MPI_INT, i + 1, WAIT_TAG, MPI_COMM_WORLD);
            }

            // Send data to workers
            for(int i = 0; i < job_1d_cnt; i++) {
                int worker_id = i + 1;

                int job_rows = jobs_1d[i].to[1] - jobs_1d[i].from[1] + 1;
                MPI_Send(&job_rows, 1, MPI_INT, worker_id, HEADER_TAG, MPI_COMM_WORLD);

                if(_ldebug) {
                    printf("[master]: Sent rows to worker [%d]: %d\n", worker_id, job_rows);
                }

                int job_cols = columns;
                MPI_Send(&job_cols, 1, MPI_INT, worker_id, HEADER_TAG, MPI_COMM_WORLD);

                if(_ldebug) {
                    printf("[master]: Sent cols to worker [%d]: %d\n", worker_id, job_cols);
                }

                uint8_t* job_data = get_chunk(parallel_1d_buffer, rows_real, cols_real, jobs_1d[i].from, jobs_1d[i].to);
                MPI_Send(job_data, job_rows * job_cols, MPI_UINT8_T, worker_id, HEADER_TAG, MPI_COMM_WORLD);

                if(_ldebug) {
                    printf("[master]: Sent data to worker [%d]: %d (len)\n", worker_id, job_rows * job_cols);
                }

                free(job_data);
            }

            MPI_Barrier(MPI_COMM_WORLD); // Wait for solver

            MPI_Barrier(MPI_COMM_WORLD); // Wait for updater

            for(int i = 0; i < job_1d_cnt; i++) {
                int worker_id = i + 1;

                int job_rows = jobs_1d[i].to[1] - jobs_1d[i].from[1] + 1;
                int job_cols = columns;
                uint8_t* job_data = calloc(job_rows * job_cols, sizeof(uint8_t));
                if(!job_data) {
                    perror("Failed to allocate memory for job chunks (1D)");
                    exit(errno);
                }

                MPI_Recv(job_data, job_rows * job_cols, MPI_UINT8_T, worker_id, DATA_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

                if(_ldebug) {
                    printf("[master]: Received data from worker [%d]: %d (len)\n", worker_id, job_rows * job_cols);
                }

                place_chunk(parallel_1d_buffer, rows_real, cols_real, job_data, jobs_1d[i].from, jobs_1d[i].to);

                free(job_data);
            }

            #ifdef DEBUG
            printf("Generation %d:\n\n", gen + 1);
            mprint_binc(parallel_1d_buffer, rows_real, cols_real, 'X', '.');
            printf("\n---\t---\t---\n\n");
            #endif
        }
        tend = MPI_Wtime();

        telapsed = tend - tstart;
        printf("End result:\n");
        printf("* Time elapsed: %f [s]\n\n", telapsed);
        #ifdef DEBUG
        mprint_binc(parallel_1d_buffer, rows_real, cols_real, 'X', '.');
        #endif
        printf("\n---\t---\t---\n\n");

        free(jobs_1d);

        int is_1d_correct = mequal(serial_buffer, parallel_1d_buffer, rows_real, cols_real);

        printf("Is the 1D parallel version equal to the serial version?\n");
        if(is_1d_correct) {
            printf("\tYES\n\t\t:)\n");
        }
        else {
            printf("\tNO\n\t\t:(\n");
        }
        printf("\n---\t---\t---\n\n");
        fflush(stdout);

        output_path = get_output_path(argv[1], "parallel1d");
        fwrite_gen(output_path, parallel_1d_buffer, rows_real, cols_real, telapsed);
        free(output_path);


        // -- Parallel version 2 - 2D data decomposition --
        // Execution check
        if(worker_cnt < MINIMUM_2D) {
            printf("At least %d workers needed for parallel version 2D. Got %d\n", MINIMUM_2D, worker_cnt); fflush(stdout);
            goto done_msg;
        }

        printf("\n\n-------\tPARALLEL VERSION - 2D\t-------\n\n");
        fflush(stdout);
        jobs_2d = create_jobs_2d(rows, columns, worker_cnt, &job_2d_cnt, &job_2d_width);
        // print_areas(jobs_2d, job_2d_cnt);

        #ifdef DEBUG
        printf("Initial generation:\n\n");
        mprint_binc(parallel_2d_buffer, rows_real, cols_real, 'X', '.');
        printf("\n---\t---\t---\n\n");
        #endif

        tstart = MPI_Wtime();
        for(int gen = 0; gen < generations; gen++) {
            // Notifies workers of work mode
            for(int i = 0; i < job_2d_cnt; i++) {
                MPI_Send(&job_2d_cnt, 1, MPI_INT, i + 1, PARALLEL_2D_TAG, MPI_COMM_WORLD);

                // Send additional data
                MPI_Send(&job_2d_width, 1, MPI_INT, i + 1, HEADER_TAG, MPI_COMM_WORLD);
            }
            for(int i = job_2d_cnt; i < worker_cnt; i++) {
                MPI_Send(&job_2d_cnt, 1, MPI_INT, i + 1, WAIT_TAG, MPI_COMM_WORLD);
            }

            // Send data to workers
            for(int i = 0; i < job_2d_cnt; i++) {
                int worker_id = i + 1;

                int job_rows = jobs_2d[i].to[1] - jobs_2d[i].from[1] + 1;
                MPI_Send(&job_rows, 1, MPI_INT, worker_id, HEADER_TAG, MPI_COMM_WORLD);

                if(_ldebug) {
                    printf("[master]: Sent rows to worker [%d]: %d\n", worker_id, job_rows);
                }

                int job_cols = jobs_2d[i].to[0] - jobs_2d[i].from[0] + 1;
                MPI_Send(&job_cols, 1, MPI_INT, worker_id, HEADER_TAG, MPI_COMM_WORLD);

                if(_ldebug) {
                    printf("[master]: Sent cols to worker [%d]: %d\n", worker_id, job_cols);
                }

                uint8_t* job_data = get_chunk(parallel_2d_buffer, rows_real, cols_real, jobs_2d[i].from, jobs_2d[i].to);
                MPI_Send(job_data, job_rows * job_cols, MPI_UINT8_T, worker_id, HEADER_TAG, MPI_COMM_WORLD);

                if(_ldebug) {
                    printf("[master]: Sent data to worker [%d]: %d (len)\n", worker_id, job_rows * job_cols);
                }

                free(job_data);
            }

            MPI_Barrier(MPI_COMM_WORLD); // Wait for solver

            MPI_Barrier(MPI_COMM_WORLD); // Wait for updater

            for(int i = 0; i < job_2d_cnt; i++) {
                int worker_id = i + 1;
                int job_rows = jobs_2d[i].to[1] - jobs_2d[i].from[1] + 1;
                int job_cols = jobs_2d[i].to[0] - jobs_2d[i].from[0] + 1;
                uint8_t* job_data = calloc(job_rows * job_cols, sizeof(uint8_t));
                if(!job_data) {
                    perror("Failed to allocate memory for job chunks (2D)");
                    exit(errno);
                }

                MPI_Recv(job_data, job_rows * job_cols, MPI_UINT8_T, worker_id, DATA_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

                if(_ldebug) {
                    printf("[master]: Received data from worker [%d]: %d (len)\n", worker_id, job_rows * job_cols);
                }

                place_chunk(parallel_2d_buffer, rows_real, cols_real, job_data, jobs_2d[i].from, jobs_2d[i].to);

                free(job_data);
            }

            #ifdef DEBUG
            printf("Generation %d:\n\n", gen + 1);
            mprint_binc(parallel_2d_buffer, rows_real, cols_real, 'X', '.');
            printf("\n---\t---\t---\n\n");
            #endif
        }
        tend = MPI_Wtime();

        telapsed = tend - tstart;
        printf("End result:\n");
        printf("* Time elapsed: %f [s]\n\n", telapsed);
        #ifdef DEBUG
        mprint_binc(parallel_2d_buffer, rows_real, cols_real, 'X', '.');
        #endif
        printf("\n---\t---\t---\n\n");

        free(jobs_2d);

        int is_2d_correct = mequal(serial_buffer, parallel_2d_buffer, rows_real, cols_real);

        printf("Is the 2D parallel version equal to the serial version?\n");
        if(is_2d_correct) {
            printf("\tYES\n\t\t:)\n");
        }
        else {
            printf("\tNO\n\t\t:(\n");
        }
        printf("\n---\t---\t---\n\n");
        fflush(stdout);

        output_path = get_output_path(argv[1], "parallel2d");
        fwrite_gen(output_path, parallel_2d_buffer, rows_real, cols_real, telapsed);
        free(output_path);


        // -- Clean-up the workspace --
        done_msg:
        for(int i = 0; i < worker_cnt; i++) {
            MPI_Send(&worker_cnt, 1, MPI_INT, i + 1, DONE_TAG, MPI_COMM_WORLD);
        }


        free(serial_buffer);
        free(parallel_1d_buffer);
        free(parallel_2d_buffer);
    }
    // Worker processes
    else {
        MPI_Status mode_status;
        int nworkers = -1, workers_x = -1;

        while(true) {
            MPI_Recv(&nworkers, 1, MPI_INT, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &mode_status);

            switch(mode_status.MPI_TAG) {
                case PARALLEL_1D_TAG:
                    if(_ldebug) {
                        printf("Parallel mode 1d - [%d]\n", rank);
                        fflush(stdout);
                    }
                    
                    worker_parallel_1d(rank, nworkers);
                    
                    break;
                case PARALLEL_2D_TAG:
                    if(_ldebug) {
                        printf("Parallel mode 2d - [%d]\n", rank);
                        fflush(stdout);
                    }

                    MPI_Recv(&workers_x, 1, MPI_INT, 0, HEADER_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                    worker_parallel_2d(rank, nworkers, workers_x);

                    break;
                case WAIT_TAG:
                    if(_ldebug) {
                        printf("Waiting for work - [%d]\n", rank);
                        fflush(stdout);
                    }

                    MPI_Barrier(MPI_COMM_WORLD);
                    MPI_Barrier(MPI_COMM_WORLD);

                    break;
                case DONE_TAG:
                    if(_ldebug) {
                        printf("DONE! - [%d]\n", rank);
                        fflush(stdout);
                    }
                    goto done;
            }
        }

        done:
        if(_ldebug) {
            printf("[%d]: Exited execution loop\n", rank);
            fflush(stdout);
        }
    }

    final_mpi:
    MPI_Finalize();

    return 0;
}