/*
 * rjn_remapping.cpp
 * Converted from rjn_remapping.f90
 * Copyright (c) 2026, arakawa@climtech.jp
 */

#include "rjn_remapping.h"
#include "rjn_utils.h"
#include "rjn_constant.h"
#include <mpi.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* -----------------------------------------------------------------------
 * Module-level private state
 * ----------------------------------------------------------------------- */
static MPI_Comm s_my_comm = MPI_COMM_NULL;
static int      s_my_size = 0;
static int      s_my_rank = 0;

/* -----------------------------------------------------------------------
 * init_remapping
 * ----------------------------------------------------------------------- */
void init_remapping(int local_comm)
{
    s_my_comm = MPI_Comm_f2c((MPI_Fint)local_comm);
    MPI_Comm_size(s_my_comm, &s_my_size);
    MPI_Comm_rank(s_my_comm, &s_my_rank);
}

/* -----------------------------------------------------------------------
 * make_grid_rank_file
 *
 * Each rank sorts its local grid_index array then sends it to rank 0.
 * Rank 0 receives all local sorted arrays and performs a merge sort
 * (selection from sorted streams) to produce global_index[] / global_rank[].
 * Rank 0 writes both to binary files; non-root ranks return early.
 * ----------------------------------------------------------------------- */

/* Helper structure used only by rank 0 during merge sort */
typedef struct {
    int  current_pos;   /* next unread position (0-based) */
    int  local_size;
    int* local_index;   /* malloc'd */
} local_grid_type;

void make_grid_rank_file(const char* my_comp, const char* my_grid,
                         const int* grid_index, int n)
{
    int local_size = n;
    int int_array[1];
    int* grid_size = NULL;
    int* sorted_index = NULL;
    char file_name[STR_LONG];
    FILE* fp;
    int ierr, i, j;

    /* Allocate grid_size on rank 0 only (others allocate 1-element dummy) */
    if (s_my_rank == 0)
        grid_size = (int*)malloc(s_my_size * sizeof(int));
    else
        grid_size = (int*)malloc(1 * sizeof(int));

    int_array[0] = local_size;
    MPI_Gather(int_array, 1, MPI_INT, grid_size, 1, MPI_INT, 0, s_my_comm);

    /* Sort local grid index */
    sorted_index = (int*)malloc(local_size * sizeof(int));
    memcpy(sorted_index, grid_index, local_size * sizeof(int));
    sort_int_1d(local_size, sorted_index, NULL);

    put_log("make_grid_rank_file : local sorting finish", -1);
    put_log("make_grid_rank_file : send/recv start", -1);

    if (s_my_rank == 0) {
        /* Gather all local sorted arrays */
        int global_size = 0;
        local_grid_type* local_grid = (local_grid_type*)malloc(s_my_size * sizeof(local_grid_type));

        for (i = 0; i < s_my_size; i++) {
            local_grid[i].current_pos = 0;   /* 0-based */
            local_grid[i].local_size  = grid_size[i];
            local_grid[i].local_index = (int*)malloc(grid_size[i] * sizeof(int));
            global_size += grid_size[i];
        }

        /* Rank 0's own data */
        memcpy(local_grid[0].local_index, sorted_index, local_size * sizeof(int));

        /* Receive from other ranks (0-based rank i -> Fortran i-1 maps to C i) */
        for (i = 1; i < s_my_size; i++) {
            MPI_Status ista;
            ierr = MPI_Recv(local_grid[i].local_index, grid_size[i], MPI_INT,
                            i, 0, s_my_comm, &ista);
            if (ierr != MPI_SUCCESS)
                rjn_error("make_grid_rank_file", "mpi_recv error");
        }

        put_log("make_grid_rank_file : send/recv finish", -1);
        put_log("make_grid_rank_file : global sorting start", -1);

        /* Merge sort: selection from sorted streams */
        int* global_index = (int*)malloc(global_size * sizeof(int));
        int* global_rank  = (int*)malloc(global_size * sizeof(int));

        for (i = 0; i < global_size; i++) {
            int current_index = 0x7fffffff;  /* huge */
            int current_rank  = -1;

            for (j = 0; j < s_my_size; j++) {
                if (local_grid[j].current_pos < local_grid[j].local_size) {
                    int val = local_grid[j].local_index[local_grid[j].current_pos];
                    if (val < current_index) {
                        current_index = val;
                        current_rank  = j;
                    }
                }
            }
            global_index[i] = current_index;
            global_rank[i]  = current_rank;   /* 0-based rank */
            local_grid[current_rank].current_pos++;
        }

        put_log("make_grid_rank_file : global sorting finish", -1);
        put_log("make_grid_rank_file : file output start", -1);

        /* Write GRID_INDEX file */
        snprintf(file_name, STR_LONG, "jlt.%s.%s.GRID_INDEX", my_comp, my_grid);
        fp = fopen(file_name, "wb");
        if (!fp) rjn_error("make_grid_rank_file", "cannot open GRID_INDEX file for write");
        fwrite(global_index, sizeof(int), global_size, fp);
        fclose(fp);

        /* Write GRID_RANK file */
        snprintf(file_name, STR_LONG, "jlt.%s.%s.GRID_RANK", my_comp, my_grid);
        fp = fopen(file_name, "wb");
        if (!fp) rjn_error("make_grid_rank_file", "cannot open GRID_RANK file for write");
        fwrite(global_rank, sizeof(int), global_size, fp);
        fclose(fp);

        put_log("make_grid_rank_file : file output finish", -1);

        for (i = 0; i < s_my_size; i++)
            free(local_grid[i].local_index);
        free(local_grid);
        free(global_index);
        free(global_rank);
    } else {
        /* Non-root: send sorted local index to rank 0 */
        ierr = MPI_Send(sorted_index, local_size, MPI_INT, 0, 0, s_my_comm);
        if (ierr != MPI_SUCCESS)
            rjn_error("make_grid_rank_file", "mpi_send error");

        put_log("make_grid_rank_file : send/recv finish", -1);
    }

    free(sorted_index);
    free(grid_size);
}

/* -----------------------------------------------------------------------
 * delete_grid_rank_file
 * Root only; silently ignores errors (matches Fortran iostat= pattern).
 * ----------------------------------------------------------------------- */
void delete_grid_rank_file(const char* my_comp, const char* my_grid)
{
    char file_name[STR_LONG];

    if (s_my_rank != 0) return;

    put_log("delete_grid_rank_file : file delete start", -1);

    snprintf(file_name, STR_LONG, "jlt.%s.%s.GRID_INDEX", my_comp, my_grid);
    remove(file_name);   /* ignore return value, like Fortran iostat */

    snprintf(file_name, STR_LONG, "jlt.%s.%s.GRID_RANK", my_comp, my_grid);
    remove(file_name);

    put_log("delete_grid_rank_file : file delete finish", -1);
}

/* -----------------------------------------------------------------------
 * get_target_grid_rank
 * Reads GRID_INDEX/GRID_RANK files in chunks of 20 integers.
 * target_index[] is first sorted (with companion sorted_pos) so that
 * matches can be found with a single forward pass through the file.
 * ----------------------------------------------------------------------- */
void get_target_grid_rank(const char* target_comp, const char* target_grid,
                          const int* target_index, int n,
                          int* target_rank)
{
    const int chunk_size = 20;
    char file_name[STR_LONG];
    FILE* fp1 = NULL;
    FILE* fp2 = NULL;
    long file_bytes;
    int file_size, num_of_read, mod_size;
    int* index_chunk = NULL;
    int* rank_chunk  = NULL;
    int* sorted_index = NULL;
    int* sorted_pos   = NULL;
    int current_pos;  /* 0-based position in sorted_index */
    int i, j;

    if (n == 0) return;

    /* Open GRID_INDEX file */
    snprintf(file_name, STR_LONG, "jlt.%s.%s.GRID_INDEX", target_comp, target_grid);
    fp1 = fopen(file_name, "rb");
    if (!fp1) {
        char msg[STR_MID];
        snprintf(msg, STR_MID, "file open error, file name = %s", file_name);
        rjn_error("get_target_grid_rank", msg);
        return;
    }

    /* Open GRID_RANK file */
    snprintf(file_name, STR_LONG, "jlt.%s.%s.GRID_RANK", target_comp, target_grid);
    fp2 = fopen(file_name, "rb");
    if (!fp2) {
        char msg[STR_MID];
        snprintf(msg, STR_MID, "file open error, file name = %s", file_name);
        fclose(fp1);
        rjn_error("get_target_grid_rank", msg);
        return;
    }

    /* Determine file size in int elements */
    fseek(fp1, 0, SEEK_END);
    file_bytes = ftell(fp1);
    rewind(fp1);
    file_size    = (int)(file_bytes / sizeof(int));
    num_of_read  = file_size / chunk_size;
    mod_size     = file_size - chunk_size * num_of_read;

    /* Allocate chunk buffers */
    index_chunk = (int*)malloc(chunk_size * sizeof(int));
    rank_chunk  = (int*)malloc(chunk_size * sizeof(int));

    /* Sort target_index with companion position array (0-based) */
    sorted_index = (int*)malloc(n * sizeof(int));
    sorted_pos   = (int*)malloc(n * sizeof(int));
    for (i = 0; i < n; i++) {
        sorted_index[i] = target_index[i];
        sorted_pos[i]   = i;   /* 0-based */
    }
    sort_int_1d(n, sorted_index, sorted_pos);

    current_pos = 0;   /* index into sorted_index[], 0-based */

    /* Process full chunks */
    for (i = 0; i < num_of_read; i++) {
        fread(index_chunk, sizeof(int), chunk_size, fp1);
        fread(rank_chunk,  sizeof(int), chunk_size, fp2);

        /* Skip chunk if last element is less than current target */
        if (index_chunk[chunk_size - 1] < sorted_index[current_pos])
            continue;

        for (j = 0; j < chunk_size; j++) {
            if (sorted_index[current_pos] == index_chunk[j]) {
                target_rank[sorted_pos[current_pos]] = rank_chunk[j];
                current_pos++;
                if (current_pos >= n) goto done;

                /* Handle duplicate target indices */
                while (sorted_index[current_pos] == sorted_index[current_pos - 1]) {
                    target_rank[sorted_pos[current_pos]] = rank_chunk[j];
                    current_pos++;
                    if (current_pos >= n) goto done;
                }
                if (current_pos >= n) goto done;
            }
        }
    }

    /* Process remainder chunk */
    if (mod_size > 0) {
        fread(index_chunk, sizeof(int), mod_size, fp1);
        fread(rank_chunk,  sizeof(int), mod_size, fp2);

        for (j = 0; j < mod_size; j++) {
            if (sorted_index[current_pos] == index_chunk[j]) {
                target_rank[sorted_pos[current_pos]] = rank_chunk[j];
                current_pos++;
                if (current_pos >= n) goto done;

                while (sorted_index[current_pos] == sorted_index[current_pos - 1]) {
                    target_rank[sorted_pos[current_pos]] = rank_chunk[j];
                    current_pos++;
                    if (current_pos >= n) goto done;
                }
                if (current_pos >= n) goto done;
            }
        }
    }

done:
    fclose(fp1);
    fclose(fp2);
    free(index_chunk);
    free(rank_chunk);
    free(sorted_index);
    free(sorted_pos);
}

/* -----------------------------------------------------------------------
 * make_local_mapping_table_no_sort
 * Sorts only grid_index; iterates global_index linearly using binary search.
 * ----------------------------------------------------------------------- */
void make_local_mapping_table_no_sort(
    const int* global_index, const int* global_target, const double* global_coef,
    int n_global,
    const int* grid_index, int n_grid,
    int* local_size_out,
    int** local_index_out, int** local_target_out, double** local_coef_out)
{
    int* sorted_index = NULL;
    int  counter, i, res;

    sorted_index = (int*)malloc(n_grid * sizeof(int));
    memcpy(sorted_index, grid_index, n_grid * sizeof(int));
    sort_int_1d(n_grid, sorted_index, NULL);

    /* Count matches */
    counter = 0;
    for (i = 0; i < n_global; i++) {
        res = binary_search_int(sorted_index, n_grid, global_index[i]);
        if (res >= 0) counter++;
    }

    *local_size_out   = counter;
    *local_index_out  = (int*)malloc(counter * sizeof(int));
    *local_target_out = (int*)malloc(counter * sizeof(int));
    *local_coef_out   = (double*)malloc(counter * sizeof(double));

    /* Fill output arrays */
    counter = 0;
    for (i = 0; i < n_global; i++) {
        res = binary_search_int(sorted_index, n_grid, global_index[i]);
        if (res >= 0) {
            (*local_index_out)[counter]  = global_index[i];
            (*local_target_out)[counter] = global_target[i];
            (*local_coef_out)[counter]   = global_coef[i];
            counter++;
        }
    }

    free(sorted_index);
}

/* -----------------------------------------------------------------------
 * make_local_mapping_table
 * Sorts global_index (with companion sorted_pos) first, then uses a
 * two-pointer merge scan against sorted grid_index.
 * ----------------------------------------------------------------------- */
void make_local_mapping_table(
    const int* global_index, const int* global_target, const double* global_coef,
    int n_global,
    const int* grid_index, int n_grid,
    int* local_size_out,
    int** local_index_out, int** local_target_out, double** local_coef_out)
{
    int* sorted_index      = NULL;   /* sorted copy of global_index */
    int* sorted_pos        = NULL;   /* companion: original 0-based position */
    int* sorted_grid_index = NULL;   /* sorted copy of grid_index */
    int  counter, current_pos, i;

    sorted_index = (int*)malloc(n_global * sizeof(int));
    sorted_pos   = (int*)malloc(n_global * sizeof(int));
    for (i = 0; i < n_global; i++) {
        sorted_index[i] = global_index[i];
        sorted_pos[i]   = i;
    }
    sort_int_1d(n_global, sorted_index, sorted_pos);

    sorted_grid_index = (int*)malloc(n_grid * sizeof(int));
    memcpy(sorted_grid_index, grid_index, n_grid * sizeof(int));
    sort_int_1d(n_grid, sorted_grid_index, NULL);

    /* Count matches via two-pointer scan */
    counter     = 0;
    current_pos = 0;
    for (i = 0; i < n_grid; i++) {
        if (current_pos >= n_global) break;
        while (current_pos < n_global &&
               sorted_index[current_pos] <= sorted_grid_index[i]) {
            if (sorted_index[current_pos] == sorted_grid_index[i])
                counter++;
            current_pos++;
        }
    }

    *local_size_out   = counter;

    if (counter == 0) {
        *local_index_out  = (int*)malloc(0);
        *local_target_out = (int*)malloc(0);
        *local_coef_out   = (double*)malloc(0);
        free(sorted_index);
        free(sorted_pos);
        free(sorted_grid_index);
        return;
    }

    *local_index_out  = (int*)malloc(counter * sizeof(int));
    *local_target_out = (int*)malloc(counter * sizeof(int));
    *local_coef_out   = (double*)malloc(counter * sizeof(double));

    /* Fill output via second two-pointer scan */
    counter     = 0;
    current_pos = 0;
    for (i = 0; i < n_grid; i++) {
        if (current_pos >= n_global) break;
        while (current_pos < n_global &&
               sorted_index[current_pos] <= sorted_grid_index[i]) {
            if (sorted_index[current_pos] == sorted_grid_index[i]) {
                int orig = sorted_pos[current_pos];
                (*local_index_out)[counter]  = sorted_index[current_pos];
                (*local_target_out)[counter] = global_target[orig];
                (*local_coef_out)[counter]   = global_coef[orig];
                counter++;
            }
            current_pos++;
        }
    }

    free(sorted_index);
    free(sorted_pos);
    free(sorted_grid_index);
}

/* -----------------------------------------------------------------------
 * reorder_index_by_target_rank
 * Sorts target_rank[] (in-place) and applies the same permutation to
 * send_index[], recv_index[], coef[].
 * ----------------------------------------------------------------------- */
void reorder_index_by_target_rank(int* send_index, int* recv_index,
                                  double* coef, int* target_rank, int n)
{
    int*    sorted_pos = NULL;
    int*    int_temp   = NULL;
    double* real_temp  = NULL;
    int i;

    if (n <= 0) return;

    sorted_pos = (int*)malloc(n * sizeof(int));
    int_temp   = (int*)malloc(n * sizeof(int));
    real_temp  = (double*)malloc(n * sizeof(double));

    for (i = 0; i < n; i++)
        sorted_pos[i] = i;

    sort_int_1d(n, target_rank, sorted_pos);

    /* Reorder send_index */
    memcpy(int_temp, send_index, n * sizeof(int));
    for (i = 0; i < n; i++)
        send_index[i] = int_temp[sorted_pos[i]];

    /* Reorder recv_index */
    memcpy(int_temp, recv_index, n * sizeof(int));
    for (i = 0; i < n; i++)
        recv_index[i] = int_temp[sorted_pos[i]];

    /* Reorder coef */
    memcpy(real_temp, coef, n * sizeof(double));
    for (i = 0; i < n; i++)
        coef[i] = real_temp[sorted_pos[i]];

    free(sorted_pos);
    free(int_temp);
    free(real_temp);
}

/* -----------------------------------------------------------------------
 * delete_same_index
 * Returns a sorted array of unique values from index_in[].
 * ----------------------------------------------------------------------- */
void delete_same_index(const int* index_in, int n,
                       int** index_out, int* n_out)
{
    int* sorted = NULL;
    int  counter, i;

    if (n <= 0) {
        *index_out = (int*)malloc(0);
        *n_out = 0;
        return;
    }

    sorted = (int*)malloc(n * sizeof(int));
    memcpy(sorted, index_in, n * sizeof(int));
    sort_int_1d(n, sorted, NULL);

    /* Count unique values */
    counter = 1;
    for (i = 1; i < n; i++)
        if (sorted[i] != sorted[i-1]) counter++;

    *n_out     = counter;
    *index_out = (int*)malloc(counter * sizeof(int));

    /* Fill unique values */
    (*index_out)[0] = sorted[0];
    counter = 1;
    for (i = 1; i < n; i++) {
        if (sorted[i] != sorted[i-1]) {
            (*index_out)[counter] = sorted[i];
            counter++;
        }
    }

    free(sorted);
}

/* -----------------------------------------------------------------------
 * make_exchange_table
 * ----------------------------------------------------------------------- */
void make_exchange_table(
    const int* target_rank, const int* local_table, int n,
    int* num_of_rank_out,
    int** exchange_rank_out, int** num_of_index_out,
    int** offset_out, int** exchange_index_out,
    int* exchange_data_size_out)
{
    int  num_of_rank;
    int* exchange_rank    = NULL;
    int* num_of_all_index = NULL;  /* raw counts per rank group */
    int* num_of_index     = NULL;  /* unique-index counts per rank */
    int* offset           = NULL;
    int* temp_index       = NULL;
    int* same_rank_index  = NULL;
    int* diff_index       = NULL;
    int  diff_n;
    int  counter, is;
    int  i, j;

    if (n <= 0) {
        *num_of_rank_out     = 0;
        *exchange_rank_out   = NULL;
        *num_of_index_out    = NULL;
        *offset_out          = NULL;
        *exchange_index_out  = NULL;
        *exchange_data_size_out = 0;
        return;
    }

    /* Count distinct consecutive rank groups */
    num_of_rank = 1;
    for (i = 1; i < n; i++)
        if (target_rank[i] != target_rank[i-1]) num_of_rank++;

    exchange_rank    = (int*)malloc(num_of_rank * sizeof(int));
    num_of_all_index = (int*)malloc(num_of_rank * sizeof(int));
    num_of_index     = (int*)malloc(num_of_rank * sizeof(int));
    offset           = (int*)malloc(num_of_rank * sizeof(int));
    temp_index       = (int*)malloc(n * sizeof(int));

    /* Fill exchange_rank and num_of_all_index */
    {
        int gr = 0;
        counter = 1;
        exchange_rank[0] = target_rank[0];
        for (i = 1; i < n; i++) {
            if (target_rank[i] != target_rank[i-1]) {
                num_of_all_index[gr] = counter;
                gr++;
                exchange_rank[gr] = target_rank[i];
                counter = 1;
            } else {
                counter++;
            }
        }
        num_of_all_index[gr] = counter;
    }

    /* For each rank group, delete duplicate indices */
    counter = 0;  /* position in local_table */
    is      = 0;  /* write position in temp_index */
    for (i = 0; i < num_of_rank; i++) {
        int cnt = num_of_all_index[i];
        same_rank_index = (int*)malloc(cnt * sizeof(int));
        for (j = 0; j < cnt; j++)
            same_rank_index[j] = local_table[counter + j];
        counter += cnt;

        delete_same_index(same_rank_index, cnt, &diff_index, &diff_n);
        num_of_index[i] = diff_n;
        for (j = 0; j < diff_n; j++)
            temp_index[is + j] = diff_index[j];
        is += diff_n;

        free(diff_index);
        free(same_rank_index);
    }

    /* Build offset array */
    offset[0] = 0;
    for (i = 1; i < num_of_rank; i++)
        offset[i] = offset[i-1] + num_of_index[i-1];

    /* Total unique exchange data size */
    {
        int total = 0;
        for (i = 0; i < num_of_rank; i++)
            total += num_of_index[i];
        *exchange_data_size_out = total;

        *exchange_index_out = (int*)malloc(total * sizeof(int));
        memcpy(*exchange_index_out, temp_index, total * sizeof(int));
    }

    free(temp_index);
    free(num_of_all_index);

    *num_of_rank_out   = num_of_rank;
    *exchange_rank_out = exchange_rank;
    *num_of_index_out  = num_of_index;
    *offset_out        = offset;
}

/* -----------------------------------------------------------------------
 * make_conversion_table
 * Stores 1-based positions (Fortran convention) in conv_table.
 * conv_table[i] = 1-based index of remapping_index[i] in grid_index[].
 * ----------------------------------------------------------------------- */
void make_conversion_table(const int* remapping_index, int n_remap,
                           const int* grid_index, int n_grid,
                           int** conv_table_out)
{
    int* sorted_index = NULL;
    int* sorted_pos   = NULL;
    int* conv_table   = NULL;
    int  i, res;

    sorted_index = (int*)malloc(n_grid * sizeof(int));
    sorted_pos   = (int*)malloc(n_grid * sizeof(int));

    for (i = 0; i < n_grid; i++) {
        sorted_index[i] = grid_index[i];
        sorted_pos[i]   = i + 1;   /* 1-based position (Fortran convention) */
    }
    sort_int_1d(n_grid, sorted_index, sorted_pos);

    conv_table = (int*)malloc(n_remap * sizeof(int));

    for (i = 0; i < n_remap; i++) {
        res = binary_search_int(sorted_index, n_grid, remapping_index[i]);
        if (res < 0)
            rjn_error("make_conversion_table", "remapping index not found in grid_index");
        conv_table[i] = sorted_pos[res];   /* 1-based */
    }

    free(sorted_index);
    free(sorted_pos);

    *conv_table_out = conv_table;
}
