#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>
#include <sys/stat.h>
#include <math.h>

#include "life.h"


// Swaps 2 pointers
void swapp(void** a, void** b) {
    void* aux = *a;
    *a = *b;
    *b = aux;
}


// Compares each element of `m1` with its corresponding element from `m2`. If all element pairs are equal, returns 1, otherwise 0. This method does not account for any padding, so use it accordingly
int mequal(uint8_t* m1, uint8_t* m2, int rows, int cols) {
    for(int i = 0; i < rows; i++) {
        for(int j = 0; j < cols; j++) {
            int idx = i * rows + j;
            
            if(m1[idx] != m2[idx]) return 0;
        }
    }
    return 1;
}


/* Prints the 2 points that define the area */
void print_area(area_t area) {
    fprintf(stdout, "(%d, %d), (%d, %d)\n", area.from[0], area.from[1], area.to[0], area.to[1]);
}


void print_areas(area_t* areas, int len) {
    for(int i = 0; i < len; i++) {
        print_area(areas[i]);
    }
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


// Validates the path given up to the filename. Relative paths to the executable's location ONLY
void validate_path(char* path) {
    char delims[] = "/\\";
    char* token = strtok(path, delims);
    fflush(stdout);
    int path_len = 1;
    char* rebuilt_path = calloc(1, sizeof(char));
    if(!rebuilt_path) {
        perror("Error allocating space for path");
        exit(errno);
    }
    rebuilt_path[0] = '\0';

    while(token) {
        // printf("%s -> %s\n", rebuilt_path, token);
        fflush(stdout);
        if(strchr(token, '.') && strlen(token) > 1) {
            break;
        }

        path_len += strlen(token) + 1;
        rebuilt_path = realloc(rebuilt_path, path_len);
        if(!rebuilt_path) {
            perror("Error allocating space for path");
            exit(errno);
        }

        strcat(rebuilt_path, token);
        #ifdef _WIN32 // Check if code is run on Windows
        strcat(rebuilt_path, "\\");
        #else // Presume if not Windows, then Linux
        strcat(rebuilt_path, "/");
        #endif

        struct stat sb;
        if(stat(rebuilt_path, &sb) != 0 || !S_ISDIR(sb.st_mode)) {
            int mkdir_err = 0;
            #ifdef _WIN32
            mkdir_err = mkdir(rebuilt_path);
            #else
            mkdir_err = mkdir(rebuilt_path, 0700);
            #endif
            if(mkdir_err != 0) {
                perror("Error creating specified output path");
                exit(errno);
            }
        }

        token = strtok(NULL, delims);
    }

    free(rebuilt_path);
}


// NOTE: Do NOT forget to free the returned pointer
char* get_output_path(char* in_name, char* type) {
    char* win_sep = "\\";
    char* unix_sep = "/";

    // printf("start\n");
    // fflush(stdout);

    char* where = strrchr(in_name, '/');
    if(!where) {
        where = strrchr(in_name, '\\'); 
    }

    // printf("%s\n", where);
    // fflush(stdout);

    char in_name_no_prefix[strlen(in_name)];
    strcpy(in_name_no_prefix, where + 1);

    // printf("%s\n", in_name_no_prefix);
    // fflush(stdout);

    where = strrchr(in_name_no_prefix, '.');
    int how_much = where - in_name_no_prefix;

    char name_no_suffix[strlen(in_name)];
    strncpy(name_no_suffix, in_name_no_prefix, how_much);

    // printf("%s\n", name_no_suffix);
    // fflush(stdout);

    char name_type[strlen(name_no_suffix) + 1 + strlen(type) + 1];
    name_type[0] = '\0';
    strcat(name_type, name_no_suffix);
    strcat(name_type, "_");
    strcat(name_type, type);

    // len(".") + len(<sep>) + len(OUT_DIR) + len(<sep>) + len(name_no_suffix) + len(<sep>) + len(in_name) + len(suffix) + len("\0")
    int path_len = 1 + 1 + strlen(OUT_DIR) + 1 + strlen(name_no_suffix) + 1 + strlen(name_type) + strlen(where) + 1;
    char* ret = calloc(path_len, sizeof(char));
    if(!ret) {
        perror("Error getting path allocation");
        exit(errno);
    }
    ret[0] = '\0';

    strcat(ret, ".");

    #ifdef _WIN32
    strcat(ret, win_sep);
    #else
    strcat(ret, unix_sep);
    #endif

    strcat(ret, OUT_DIR);

    #ifdef _WIN32
    strcat(ret, win_sep);
    #else
    strcat(ret, unix_sep);
    #endif

    strcat(ret, name_no_suffix);

    #ifdef _WIN32
    strcat(ret, win_sep);
    #else
    strcat(ret, unix_sep);
    #endif

    strcat(ret, name_type);
    strcat(ret, where);

    return ret;
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


// Method that writes the given cell buffer to a given file. Ouput will look similar to the input. This method writes the whole given buffer, so padding should be removed before if unwanted.
void fwrite_gen(
    char* out_file_name,
    uint8_t* cells,
    int rows,
    int cols,
    float t_elapsed // Optional parameter. If <0, will be ignored
) {
    char file_path[strlen(out_file_name) + 1];
    strcpy(file_path, out_file_name);
    validate_path(file_path);

    FILE* out_file = fopen(out_file_name, "w");
    if(!out_file) {
        perror("Error opening output file");
        exit(errno);
    }

    // Prints time if given
    if(t_elapsed >= 0) {
        fprintf(out_file, "%f\n", t_elapsed);
    }

    for(int i = 0; i < rows; i++) {
        for(int j = 0; j < cols; j++) {
            int idx = i * cols + j;

            fprintf(out_file, "%c", IS_ALIVE(cells[idx]) ? 'X' : '.');
        }
        fprintf(out_file, "\n");
    }

    fclose(out_file);
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
void next_gen(uint8_t* buffer, int buff_rows, int buff_cols) {
    int from[] = {0, 0};
    int to[] = {buff_cols - 1, buff_rows - 1};
    // Gets work area = cells + padding
    uint8_t* work_area = get_chunk(buffer, buff_rows, buff_cols, from, to);

    // Solver works on the whole buffer
    solver(work_area, buff_rows, buff_cols);
    // Updates just the cells, since padding doesn't matter
    updater(work_area, buff_rows, buff_cols);

    from[0] = 1; from[1] = 1;
    to[0] = buff_cols - 2; to[1] = buff_rows - 2;
    // Gets updated cells
    uint8_t* truth = get_chunk(work_area, buff_rows, buff_cols, from, to);

    // Replaces the cells in the buffer
    place_chunk(buffer, buff_rows, buff_cols, truth, from, to);

    // Clean-up
    free(work_area);
    free(truth);
}


/*
    Creates the job buffer for the 1D parallel version.
    Last block might not have the same size as the rest.
    Work areas are 1 indexed, as to take into account the padding from the big.
*/
area_t* create_jobs_1d(int rows, int columns, int workers, int* job_cnt) {
    int nblocks = MIN(workers, rows);
    int block_height = rows / nblocks;

    area_t* blocks = calloc(nblocks, sizeof(area_t));
    if(!blocks) {
        perror("Error allocating memory for jobs list (1D)");
        exit(errno);
    }

    int i = 0;
    for(i = 0; i < nblocks - 1; i++) {
        blocks[i] = (area_t) {from: {1, i * block_height + 1}, to: {columns, (i + 1) * block_height}};
    }
    blocks[i] = (area_t) {from: {1, i * block_height + 1}, to: {columns, rows}};

    *job_cnt = nblocks;
    return blocks;
}


area_t* create_jobs_2d(int rows, int columns, int workers, int* job_cnt) {
    return NULL;
}

