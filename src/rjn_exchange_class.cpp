/*
 * rjn_exchange_class.cpp
 * Converted from rjn_exchange_class.f90
 * Copyright (c) 2026, arakawa@climtech.jp
 */

#include "rjn_exchange_class.h"
#include "rjn_mpi_lib.h"
#include "rjn_utils.h"
#include "rjn_grid.h"
#include "rjn_grid_class.h"
#include "rjn_comp.h"
#include "rjn_remapping.h"
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>
#include <math.h>

/* -----------------------------------------------------------------------
 * Forward declarations of private helpers
 * ----------------------------------------------------------------------- */
static void set_mapping_table_send_intpl(exchange_class* self,
    const char* send_comp_name, const char* send_grid_name,
    const char* recv_comp_name, const char* recv_grid_name,
    int map_tag,
    const int* send_grid, const int* recv_grid, const double* coef, int n);

static void set_mapping_table_recv_intpl(exchange_class* self,
    const char* send_comp_name, const char* send_grid_name,
    const char* recv_comp_name, const char* recv_grid_name,
    int map_tag,
    const int* send_grid, const int* recv_grid, const double* coef, int n);

static void make_grid_conv_table(const int* grid_index, int grid_n,
                                 const int* exchange_index, int ex_n,
                                 int** conv_table_out);
static void make_send_map_info(const int* send_grid_index, int n, send_map_info* sm);
static void make_recv_map_info(const int* recv_grid_index, int n, recv_map_info* rm);
static void send_send_grid_index(exchange_class* self, const int* grid_index);
static void recv_send_grid_index(exchange_class* self);
static void sort_conversion_table(int* send_index_table, int* send_conv_table,
                                  int* recv_index_table, int* recv_conv_table,
                                  double* coef, int n);
static void reorder_conversion_table(int* send_index_table, int* send_conv_table,
                                     int* recv_conv_table, double* coef, int n);
static void resort_conversion_table(int* send_index, int* send_conv, double* coef, int n);
static void make_parallel_interpolation_table(const int* send_1d, const int* recv_1d,
                                              const double* coef_1d, int n,
                                              int* conv_table_size_out,
                                              conv_table_2d** conv_table_out);
static void interpolate_data_serial(exchange_class* self,
    const double* send_data, int sd_rows, int sd_cols,
    double* recv_data, int rd_rows, int rd_cols,
    int num_of_layer, int intpl_tag);
static void interpolate_data_parallel(exchange_class* self,
    const double* send_data, int sd_rows, int sd_cols,
    double* recv_data, int rd_rows, int rd_cols,
    int num_of_layer, int intpl_tag);

/* -----------------------------------------------------------------------
 * Constructor
 * ----------------------------------------------------------------------- */
exchange_class exchange_class_init(const char* my_name, int intpl_flag, int intpl_mode)
{
    exchange_class ec;
    memset(&ec, 0, sizeof(ec));
    strncpy(ec.my_name, my_name, STR_SHORT - 1);
    ec.intpl_flag = intpl_flag;
    ec.intpl_mode = intpl_mode;
    return ec;
}

/* -----------------------------------------------------------------------
 * exchange_class_is_send_intpl
 * ----------------------------------------------------------------------- */
int exchange_class_is_send_intpl(const exchange_class* self)
{
    if (strcmp(self->my_name, self->send_comp_name) == 0)
        return self->intpl_flag;
    else
        return !self->intpl_flag;
}

int exchange_class_is_my_intpl(const exchange_class* self)
{
    return self->intpl_flag;
}

/* -----------------------------------------------------------------------
 * set_mapping_table  (dispatcher)
 * ----------------------------------------------------------------------- */
void exchange_class_set_mapping_table(exchange_class* self,
    const char* send_comp_name, const char* send_grid_name,
    const char* recv_comp_name, const char* recv_grid_name,
    int map_tag, int intpl_flag,
    const int* send_grid, const int* recv_grid, const double* coef, int n)
{
    strncpy(self->send_comp_name, send_comp_name, STR_SHORT - 1);
    strncpy(self->send_grid_name, send_grid_name, STR_SHORT - 1);
    strncpy(self->recv_comp_name, recv_comp_name, STR_SHORT - 1);
    strncpy(self->recv_grid_name, recv_grid_name, STR_SHORT - 1);
    self->map_tag      = map_tag;
    self->send_comp_id = get_comp_id_from_name(send_comp_name);
    self->recv_comp_id = get_comp_id_from_name(recv_comp_name);
    self->intpl_flag   = intpl_flag;

    if (exchange_class_is_send_intpl(self))
        set_mapping_table_send_intpl(self, send_comp_name, send_grid_name,
                                     recv_comp_name, recv_grid_name,
                                     map_tag, send_grid, recv_grid, coef, n);
    else
        set_mapping_table_recv_intpl(self, send_comp_name, send_grid_name,
                                     recv_comp_name, recv_grid_name,
                                     map_tag, send_grid, recv_grid, coef, n);
}

/* -----------------------------------------------------------------------
 * Private: make_grid_conv_table
 * For each exchange_index[i], find its position (1-based) in grid_index[].
 * ----------------------------------------------------------------------- */
static void make_grid_conv_table(const int* grid_index, int grid_n,
                                 const int* exchange_index, int ex_n,
                                 int** conv_table_out)
{
    int i, res;
    int* sorted_index;
    int* sorted_pos;
    int* conv_table;

    if (ex_n <= 0) return;

    sorted_index = (int*)malloc(grid_n * sizeof(int));
    sorted_pos   = (int*)malloc(grid_n * sizeof(int));
    conv_table   = (int*)calloc(ex_n, sizeof(int));

    for (i = 0; i < grid_n; i++) {
        sorted_index[i] = grid_index[i];
        sorted_pos[i]   = i + 1;  /* 1-based */
    }
    sort_int_1d(grid_n, sorted_index, sorted_pos);

    for (i = 0; i < ex_n; i++) {
        res = binary_search_int(sorted_index, grid_n, exchange_index[i]);
        if (res >= 0) conv_table[i] = sorted_pos[res];
    }

    free(sorted_index);
    free(sorted_pos);
    *conv_table_out = conv_table;
}

/* -----------------------------------------------------------------------
 * Private: make_send_map_info
 * ----------------------------------------------------------------------- */
static void make_send_map_info(const int* send_grid_index, int n, send_map_info* sm)
{
    delete_same_index(send_grid_index, n, &sm->intpled_index, &sm->intpled_data_size);
}

/* -----------------------------------------------------------------------
 * Private: make_recv_map_info
 * ----------------------------------------------------------------------- */
static void make_recv_map_info(const int* recv_grid_index, int n, recv_map_info* rm)
{
    delete_same_index(recv_grid_index, n, &rm->intpled_index, &rm->intpled_data_size);
}

/* -----------------------------------------------------------------------
 * Private: send/recv grid index helpers
 * ----------------------------------------------------------------------- */
static void send_send_grid_index(exchange_class* self, const int* grid_index)
{
    int i;
    int* exchange_buffer;
    int ex_size = self->ex_map.exchange_data_size;

    exchange_buffer = (int*)malloc(ex_size * sizeof(int));
    for (i = 0; i < ex_size; i++)
        exchange_buffer[i] = grid_index[self->ex_map.conv_table[i] - 1];

    for (i = 0; i < self->ex_map.num_of_exchange_rank; i++) {
        int data_tag    = self->ex_map.exchange_rank[i];
        int num_of_data = self->ex_map.num_of_exchange[i];
        int offset      = self->ex_map.offset[i];
        jml_ISendModel2_int1d(self->send_comp_id, self->recv_comp_id,
                              exchange_buffer + offset, num_of_data,
                              self->ex_map.exchange_rank[i], data_tag);
    }
    jml_send_waitall();
    free(exchange_buffer);
}

static void recv_send_grid_index(exchange_class* self)
{
    int i;
    int* exchange_buffer;
    int ex_size = self->ex_map.exchange_data_size;

    exchange_buffer = (int*)calloc(ex_size, sizeof(int));

    for (i = 0; i < self->ex_map.num_of_exchange_rank; i++) {
        int data_tag    = jml_GetMyrank(self->recv_comp_id);
        int num_of_data = self->ex_map.num_of_exchange[i];
        int offset      = self->ex_map.offset[i];
        jml_IRecvModel2_int1d(self->recv_comp_id, self->send_comp_id,
                              exchange_buffer + offset, num_of_data,
                              self->ex_map.exchange_rank[i], data_tag);
    }
    jml_recv_waitall();

    make_conversion_table(self->send_grid_index, self->index_size,
                          exchange_buffer, ex_size,
                          &self->send_conv_table);
    free(exchange_buffer);
}

/* -----------------------------------------------------------------------
 * Private: sort_conversion_table
 * Sorts recv_conv_table ascending, reorders all other arrays in the same
 * permutation.
 * ----------------------------------------------------------------------- */
static void sort_conversion_table(int* send_index_table, int* send_conv_table,
                                  int* recv_index_table, int* recv_conv_table,
                                  double* coef, int n)
{
    int i;
    int* sorted_index = (int*)malloc(n * sizeof(int));
    int* sorted_pos   = (int*)malloc(n * sizeof(int));
    double* double_temp = (double*)malloc(n * sizeof(double));

    for (i = 0; i < n; i++) {
        sorted_index[i] = recv_conv_table[i];
        sorted_pos[i]   = i;
    }
    sort_int_1d(n, sorted_index, sorted_pos);

    for (i = 0; i < n; i++) recv_conv_table[i] = sorted_index[i];

    /* reorder send_conv_table */
    memcpy(sorted_index, send_conv_table, n * sizeof(int));
    for (i = 0; i < n; i++) send_conv_table[i] = sorted_index[sorted_pos[i]];

    /* reorder send_index_table */
    memcpy(sorted_index, send_index_table, n * sizeof(int));
    for (i = 0; i < n; i++) send_index_table[i] = sorted_index[sorted_pos[i]];

    /* reorder recv_index_table */
    memcpy(sorted_index, recv_index_table, n * sizeof(int));
    for (i = 0; i < n; i++) recv_index_table[i] = sorted_index[sorted_pos[i]];

    /* reorder coef */
    memcpy(double_temp, coef, n * sizeof(double));
    for (i = 0; i < n; i++) coef[i] = double_temp[sorted_pos[i]];

    free(sorted_index);
    free(sorted_pos);
    free(double_temp);
}

static void resort_conversion_table(int* send_index, int* send_conv, double* coef, int n)
{
    int i;
    int* sorted_index = (int*)malloc(n * sizeof(int));
    int* sorted_pos   = (int*)malloc(n * sizeof(int));
    double* double_temp = (double*)malloc(n * sizeof(double));

    for (i = 0; i < n; i++) {
        sorted_index[i] = send_index[i];
        sorted_pos[i]   = i;
    }
    sort_int_1d(n, sorted_index, sorted_pos);

    memcpy(sorted_index, send_conv, n * sizeof(int));
    for (i = 0; i < n; i++) send_conv[i] = sorted_index[sorted_pos[i]];

    memcpy(double_temp, coef, n * sizeof(double));
    for (i = 0; i < n; i++) coef[i] = double_temp[sorted_pos[i]];

    free(sorted_index);
    free(sorted_pos);
    free(double_temp);
}

static void reorder_conversion_table(int* send_index_table, int* send_conv_table,
                                     int* recv_conv_table, double* coef, int n)
{
    int i;
    int current_index  = recv_conv_table[0];
    int index_counter  = 0;

    for (i = 1; i < n; i++) {
        if (current_index != recv_conv_table[i]) {
            if (index_counter > 0) {
                int start = i - 1 - index_counter;
                int len   = index_counter + 1;
                resort_conversion_table(send_index_table + start,
                                        send_conv_table  + start,
                                        coef             + start, len);
            }
            current_index = recv_conv_table[i];
            index_counter = 0;
        } else {
            index_counter++;
        }
    }
    if (index_counter > 0) {
        int start = n - 1 - index_counter;
        int len   = index_counter + 1;
        resort_conversion_table(send_index_table + start,
                                send_conv_table  + start,
                                coef             + start, len);
    }
}

/* -----------------------------------------------------------------------
 * Private: make_parallel_interpolation_table
 * ----------------------------------------------------------------------- */
static void make_parallel_interpolation_table(const int* send_1d, const int* recv_1d,
                                              const double* coef_1d, int n,
                                              int* conv_table_size_out,
                                              conv_table_2d** conv_table_out)
{
    int i, j;
    int current_index, same_counter, same_num_counter, diff_num_counter;
    int** parallel_index;
    int*  index_pointer;
    conv_table_2d* ct;

    /* First pass: compute dimensions */
    current_index     = recv_1d[0];
    same_counter      = 1;
    same_num_counter  = 1;
    diff_num_counter  = 1;

    for (i = 1; i < n; i++) {
        if (current_index == recv_1d[i]) {
            same_counter++;
            if (same_counter > same_num_counter) same_num_counter = same_counter;
        } else {
            same_counter = 1;
            diff_num_counter++;
        }
        current_index = recv_1d[i];
    }

    parallel_index = (int**)calloc(diff_num_counter, sizeof(int*));
    for (i = 0; i < diff_num_counter; i++)
        parallel_index[i] = (int*)calloc(same_num_counter, sizeof(int));
    index_pointer = (int*)calloc(same_num_counter, sizeof(int));

    /* Fill with sentinel -255 */
    for (i = 0; i < diff_num_counter; i++)
        for (j = 0; j < same_num_counter; j++)
            parallel_index[i][j] = -255;

    /* Second pass: fill parallel_index */
    parallel_index[0][0] = 0;  /* 0-based */
    index_pointer[0]     = 1;
    current_index        = recv_1d[0];
    same_counter         = 1;
    diff_num_counter     = 0;

    for (i = 1; i < n; i++) {
        if (current_index == recv_1d[i]) {
            same_counter++;
        } else {
            same_counter = 1;
            diff_num_counter++;
        }
        index_pointer[same_counter - 1]++;
        parallel_index[index_pointer[same_counter - 1] - 1][same_counter - 1] = i;
        current_index = recv_1d[i];
    }

    *conv_table_size_out = same_num_counter;
    ct = (conv_table_2d*)calloc(same_num_counter, sizeof(conv_table_2d));

    for (i = 0; i < same_num_counter; i++) {
        int sz = index_pointer[i];
        ct[i].table_size      = sz;
        ct[i].send_conv_table = (int*)malloc(sz * sizeof(int));
        ct[i].recv_conv_table = (int*)malloc(sz * sizeof(int));
        ct[i].coef            = (double*)malloc(sz * sizeof(double));
        for (j = 0; j < sz; j++) {
            int idx = parallel_index[j][i];
            ct[i].send_conv_table[j] = send_1d[idx];
            ct[i].recv_conv_table[j] = recv_1d[idx];
            ct[i].coef[j]            = coef_1d[idx];
        }
    }

    *conv_table_out = ct;

    for (i = 0; i < diff_num_counter + 1; i++) free(parallel_index[i]);
    free(parallel_index);
    free(index_pointer);
}

/* -----------------------------------------------------------------------
 * Private: set_mapping_table_send_intpl
 * ----------------------------------------------------------------------- */
static void set_mapping_table_send_intpl(exchange_class* self,
    const char* send_comp_name, const char* send_grid_name,
    const char* recv_comp_name, const char* recv_grid_name,
    int map_tag,
    const int* send_grid, const int* recv_grid, const double* coef, int n)
{
    grid_class* my_grid;
    int is_send = (strcmp(self->my_name, send_comp_name) == 0);

    put_log("rjn_exchange_class : set_mapping_table_send_intpl start", -1);

    if (is_send) {
        my_grid = get_grid_ptr(send_grid_name);
        make_local_mapping_table_no_sort(
            send_grid, recv_grid, coef, n,
            grid_class_get_grid_index_ptr(my_grid), grid_class_get_grid_size(my_grid),
            &self->index_size,
            &self->send_grid_index, &self->recv_grid_index, &self->coef);
        if (self->index_size == 0) return;
        self->target_rank = (int*)malloc(self->index_size * sizeof(int));
        get_target_grid_rank(recv_comp_name, recv_grid_name,
                             self->recv_grid_index, self->index_size,
                             self->target_rank);
    } else {
        my_grid = get_grid_ptr(recv_grid_name);
        make_local_mapping_table_no_sort(
            recv_grid, send_grid, coef, n,
            grid_class_get_grid_index_ptr(my_grid), grid_class_get_grid_size(my_grid),
            &self->index_size,
            &self->recv_grid_index, &self->send_grid_index, &self->coef);
        if (self->index_size == 0) return;
        self->target_rank = (int*)malloc(self->index_size * sizeof(int));
        get_target_grid_rank(send_comp_name, send_grid_name,
                             self->send_grid_index, self->index_size,
                             self->target_rank);
    }

    reorder_index_by_target_rank(self->send_grid_index, self->recv_grid_index,
                                  self->coef, self->target_rank, self->index_size);

    make_exchange_table(self->target_rank, self->recv_grid_index, self->index_size,
                        &self->ex_map.num_of_exchange_rank,
                        &self->ex_map.exchange_rank,
                        &self->ex_map.num_of_exchange,
                        &self->ex_map.offset,
                        &self->ex_map.exchange_index,
                        &self->ex_map.exchange_data_size);

    if (is_send) {
        make_send_map_info(self->send_grid_index, self->index_size, &self->send_map);
        make_grid_conv_table(grid_class_get_grid_index_ptr(my_grid), grid_class_get_grid_size(my_grid),
                             self->send_map.intpled_index, self->send_map.intpled_data_size,
                             &self->ex_map.conv_table);
        make_grid_conv_table(grid_class_get_grid_index_ptr(my_grid), grid_class_get_grid_size(my_grid),
                             self->send_map.intpled_index, self->send_map.intpled_data_size,
                             &self->send_map.conv_table);
        make_conversion_table(self->send_grid_index, self->index_size,
                              self->send_map.intpled_index, self->send_map.intpled_data_size,
                              &self->send_conv_table);
        make_conversion_table(self->recv_grid_index, self->index_size,
                              self->ex_map.exchange_index, self->ex_map.exchange_data_size,
                              &self->recv_conv_table);

        if (self->intpl_mode == INTPL_SERIAL_SAFE || self->intpl_mode == INTPL_PARALLEL) {
            sort_conversion_table(self->send_grid_index, self->send_conv_table,
                                  self->recv_grid_index, self->recv_conv_table,
                                  self->coef, self->index_size);
            reorder_conversion_table(self->send_grid_index, self->send_conv_table,
                                     self->recv_conv_table, self->coef, self->index_size);
        }
        if (self->intpl_mode == INTPL_PARALLEL) {
            make_parallel_interpolation_table(self->send_conv_table, self->recv_conv_table,
                                              self->coef, self->index_size,
                                              &self->conv_table_size, &self->conv_table);
        }
    } else {
        make_recv_map_info(self->ex_map.exchange_index, self->ex_map.exchange_data_size, &self->my_map);
        make_grid_conv_table(self->my_map.intpled_index, self->my_map.intpled_data_size,
                             self->ex_map.exchange_index, self->ex_map.exchange_data_size,
                             &self->recv_conv_table);
        make_grid_conv_table(grid_class_get_grid_index_ptr(my_grid), grid_class_get_grid_size(my_grid),
                             self->my_map.intpled_index, self->my_map.intpled_data_size,
                             &self->my_map.conv_table);
    }

    put_log("rjn_exchange_class : set_mapping_table_send_intpl end", -1);
}

/* -----------------------------------------------------------------------
 * Private: set_mapping_table_recv_intpl
 * ----------------------------------------------------------------------- */
static void set_mapping_table_recv_intpl(exchange_class* self,
    const char* send_comp_name, const char* send_grid_name,
    const char* recv_comp_name, const char* recv_grid_name,
    int map_tag,
    const int* send_grid, const int* recv_grid, const double* coef, int n)
{
    grid_class* my_grid;
    int is_send = (strcmp(self->my_name, send_comp_name) == 0);

    put_log("rjn_exchange_class : set_mapping_table_recv_intpl start", -1);

    if (is_send) {
        my_grid = get_grid_ptr(send_grid_name);
        make_local_mapping_table_no_sort(
            send_grid, recv_grid, coef, n,
            grid_class_get_grid_index_ptr(my_grid), grid_class_get_grid_size(my_grid),
            &self->index_size,
            &self->send_grid_index, &self->recv_grid_index, &self->coef);
        if (self->index_size == 0) return;
        self->target_rank = (int*)malloc(self->index_size * sizeof(int));
        get_target_grid_rank(recv_comp_name, recv_grid_name,
                             self->recv_grid_index, self->index_size, self->target_rank);
    } else {
        my_grid = get_grid_ptr(recv_grid_name);
        make_local_mapping_table_no_sort(
            recv_grid, send_grid, coef, n,
            grid_class_get_grid_index_ptr(my_grid), grid_class_get_grid_size(my_grid),
            &self->index_size,
            &self->recv_grid_index, &self->send_grid_index, &self->coef);
        if (self->index_size == 0) return;
        self->target_rank = (int*)malloc(self->index_size * sizeof(int));
        get_target_grid_rank(send_comp_name, send_grid_name,
                             self->send_grid_index, self->index_size, self->target_rank);
    }

    reorder_index_by_target_rank(self->send_grid_index, self->recv_grid_index,
                                  self->coef, self->target_rank, self->index_size);

    make_exchange_table(self->target_rank, self->send_grid_index, self->index_size,
                        &self->ex_map.num_of_exchange_rank,
                        &self->ex_map.exchange_rank,
                        &self->ex_map.num_of_exchange,
                        &self->ex_map.offset,
                        &self->ex_map.exchange_index,
                        &self->ex_map.exchange_data_size);

    if (is_send) {
        make_grid_conv_table(grid_class_get_grid_index_ptr(my_grid), grid_class_get_grid_size(my_grid),
                             self->ex_map.exchange_index, self->ex_map.exchange_data_size,
                             &self->ex_map.conv_table);
        send_send_grid_index(self, grid_class_get_grid_index_ptr(my_grid));
    } else {
        make_recv_map_info(self->recv_grid_index, self->index_size, &self->my_map);
        make_grid_conv_table(grid_class_get_grid_index_ptr(my_grid), grid_class_get_grid_size(my_grid),
                             self->my_map.intpled_index, self->my_map.intpled_data_size,
                             &self->my_map.conv_table);
        recv_send_grid_index(self);
        make_conversion_table(self->recv_grid_index, self->index_size,
                              self->my_map.intpled_index, self->my_map.intpled_data_size,
                              &self->recv_conv_table);

        if (self->intpl_mode == INTPL_SERIAL_SAFE || self->intpl_mode == INTPL_PARALLEL) {
            sort_conversion_table(self->send_grid_index, self->send_conv_table,
                                  self->recv_grid_index, self->recv_conv_table,
                                  self->coef, self->index_size);
            reorder_conversion_table(self->send_grid_index, self->send_conv_table,
                                     self->recv_conv_table, self->coef, self->index_size);
        }
        if (self->intpl_mode == INTPL_PARALLEL) {
            make_parallel_interpolation_table(self->send_conv_table, self->recv_conv_table,
                                              self->coef, self->index_size,
                                              &self->conv_table_size, &self->conv_table);
        }
    }

    put_log("rjn_exchange_class : set_mapping_table_recv_intpl end", -1);
}

/* -----------------------------------------------------------------------
 * Getter functions
 * ----------------------------------------------------------------------- */
int exchange_class_is_my_exchange(const exchange_class* self,
    const char* send_comp_name, const char* send_grid_name,
    const char* recv_comp_name, const char* recv_grid_name, int map_tag)
{
    if (strcmp(send_comp_name, self->send_comp_name) != 0) return 0;
    if (strcmp(send_grid_name, self->send_grid_name) != 0) return 0;
    if (strcmp(recv_comp_name, self->recv_comp_name) != 0) return 0;
    if (strcmp(recv_grid_name, self->recv_grid_name) != 0) return 0;
    if (map_tag != self->map_tag)                          return 0;
    return 1;
}

void exchange_class_get_my_name(const exchange_class* self, char* out)
{ strncpy(out, self->my_name, STR_SHORT - 1); out[STR_SHORT-1]='\0'; }
void exchange_class_get_send_comp_name(const exchange_class* self, char* out)
{ strncpy(out, self->send_comp_name, STR_SHORT - 1); out[STR_SHORT-1]='\0'; }
void exchange_class_get_send_grid_name(const exchange_class* self, char* out)
{ strncpy(out, self->send_grid_name, STR_SHORT - 1); out[STR_SHORT-1]='\0'; }
void exchange_class_get_recv_comp_name(const exchange_class* self, char* out)
{ strncpy(out, self->recv_comp_name, STR_SHORT - 1); out[STR_SHORT-1]='\0'; }
void exchange_class_get_recv_grid_name(const exchange_class* self, char* out)
{ strncpy(out, self->recv_grid_name, STR_SHORT - 1); out[STR_SHORT-1]='\0'; }
int exchange_class_get_map_tag(const exchange_class* self) { return self->map_tag; }
int exchange_class_get_num_of_target_rank(const exchange_class* self)
{ return self->ex_map.num_of_exchange_rank; }
int exchange_class_get_target_rank(const exchange_class* self, int target_rank_num)
{ return self->ex_map.exchange_rank[target_rank_num - 1]; }
int exchange_class_get_target_array_size(const exchange_class* self, int target_rank_num)
{ return self->ex_map.num_of_exchange[target_rank_num - 1]; }

int exchange_class_get_exchange_data_size(const exchange_class* self)
{
    if (self->intpl_flag) {
        if (exchange_class_is_send_intpl(self))
            return self->send_map.intpled_data_size;
        else
            return self->my_map.intpled_data_size;
    }
    return self->ex_map.exchange_data_size;
}

int exchange_class_get_exchange_buffer_size(const exchange_class* self)
{
    return self->ex_map.exchange_data_size;
}

/* -----------------------------------------------------------------------
 * Data movement
 * ----------------------------------------------------------------------- */
void exchange_class_local_2_exchange(const exchange_class* self,
    const double* grid_data, double* exchange_data)
{
    int i;
    if (exchange_class_is_send_intpl(self)) {
        int sz = exchange_class_get_exchange_data_size(self);
        for (i = 0; i < sz; i++)
            exchange_data[i] = grid_data[self->send_map.conv_table[i] - 1];
    } else {
        int sz = self->ex_map.exchange_data_size;
        for (i = 0; i < sz; i++)
            exchange_data[i] = grid_data[self->ex_map.conv_table[i] - 1];
    }
}

void exchange_class_exchange_2_local(const exchange_class* self,
    const double* exchange_data, double* grid_data)
{
    int i;
    for (i = 0; i < self->my_map.intpled_data_size; i++)
        grid_data[self->my_map.conv_table[i] - 1] = exchange_data[i];
}

void exchange_class_target_2_exchange_buffer(const exchange_class* self,
    const buffer_class* target_buffer, int n_buf,
    double* exchange_buffer, int exbuf_rows, int exbuf_cols)
{
    int i, r, c;
    int num_of_layer, num_of_data, offset;

    if (n_buf == 0) return;
    num_of_layer = target_buffer[0].num_of_layer;

    for (i = 0; i < self->ex_map.num_of_exchange_rank; i++) {
        num_of_data = self->ex_map.num_of_exchange[i];
        offset      = self->ex_map.offset[i];
        for (r = 0; r < num_of_data; r++)
            for (c = 0; c < num_of_layer; c++)
                exchange_buffer[(offset + r) + c * exbuf_rows] =
                    target_buffer[i].buffer[r + c * target_buffer[i].num_of_data];
    }
}

/* -----------------------------------------------------------------------
 * send_data_1d / recv_data_1d
 * ----------------------------------------------------------------------- */
void exchange_class_send_data_1d(exchange_class* self,
    const double* data, int n_data,
    double* exchange_buffer, int eb_n,
    int intpl_tag, int exchange_tag)
{
    int i;
    char log_str[STR_MID];

    snprintf(log_str, STR_MID, "  [send_data_1d] send data START, exchange_tag = %d", exchange_tag);
    put_log(log_str, -1);

    if (self->intpl_flag) {
        /* reshape data to 2D column, then interpolate */
        double* sd2 = (double*)malloc(n_data * sizeof(double));
        double* eb2 = (double*)malloc(eb_n  * sizeof(double));
        memcpy(sd2, data, n_data * sizeof(double));
        exchange_class_interpolate_data(self, sd2, n_data, 1, eb2, eb_n, 1, 1, intpl_tag);
        memcpy(exchange_buffer, eb2, eb_n * sizeof(double));
        free(sd2);
        free(eb2);
    } else {
        for (i = 0; i < self->ex_map.exchange_data_size; i++)
            exchange_buffer[i] = data[i];
    }

    for (i = 0; i < self->ex_map.num_of_exchange_rank; i++) {
        int target_rank = self->ex_map.exchange_rank[i];
        int num_of_data = self->ex_map.num_of_exchange[i];
        int offset      = self->ex_map.offset[i];
        jml_ISendModel2_double1d(self->send_comp_id, self->recv_comp_id,
                                  exchange_buffer + offset, num_of_data,
                                  target_rank, exchange_tag);
    }

    snprintf(log_str, STR_MID, "  [send_data_1d] send data END, exchange_tag = %d", exchange_tag);
    put_log(log_str, -1);
}

void exchange_class_recv_data_1d(exchange_class* self,
    double* exchange_buffer, int eb_n, int exchange_tag)
{
    int i;
    char log_str[STR_MID];

    snprintf(log_str, STR_MID, "  [recv_data_1d] recv data START, exchange_tag = %d", exchange_tag);
    put_log(log_str, -1);

    for (i = 0; i < self->ex_map.num_of_exchange_rank; i++) {
        int target_rank = self->ex_map.exchange_rank[i];
        int num_of_data = self->ex_map.num_of_exchange[i];
        int offset      = self->ex_map.offset[i];
        jml_IRecvModel2_double1d(self->recv_comp_id, self->send_comp_id,
                                  exchange_buffer + offset, num_of_data,
                                  target_rank, exchange_tag);
    }

    snprintf(log_str, STR_MID, "  [recv_data_1d] recv data END, exchange_tag = %d", exchange_tag);
    put_log(log_str, -1);
}

/* -----------------------------------------------------------------------
 * send_data_2d / recv_data_2d
 * ----------------------------------------------------------------------- */
void exchange_class_send_data_2d(exchange_class* self,
    const double* data, int n1, int n2,
    buffer_class* exchange_buffer, int n_buf,
    int num_of_layer, int intpl_tag, int exchange_tag)
{
    int i;
    double* intpl_buffer = NULL;
    int intpl_size = 0;

    if (self->intpl_flag) {
        for (i = 0; i < self->ex_map.num_of_exchange_rank; i++)
            intpl_size += self->ex_map.num_of_exchange[i];
        intpl_buffer = (double*)malloc(intpl_size * num_of_layer * sizeof(double));
        exchange_class_interpolate_data(self, data, n1, n2,
                                         intpl_buffer, intpl_size, num_of_layer,
                                         num_of_layer, intpl_tag);
    }

    for (i = 0; i < self->ex_map.num_of_exchange_rank; i++) {
        int target_rank = self->ex_map.exchange_rank[i];
        int num_of_data = self->ex_map.num_of_exchange[i];
        int offset      = self->ex_map.offset[i];
        int r, c;

        if (self->intpl_flag) {
            for (r = 0; r < num_of_data; r++)
                for (c = 0; c < num_of_layer; c++)
                    exchange_buffer[i].buffer[r + c * num_of_data] =
                        intpl_buffer[(offset + r) + c * intpl_size];
        } else {
            for (r = 0; r < num_of_data; r++)
                for (c = 0; c < num_of_layer; c++)
                    exchange_buffer[i].buffer[r + c * num_of_data] =
                        data[(offset + r) + c * n1];
        }

        jml_ISendModel3_double2d(self->send_comp_id, self->recv_comp_id,
                                  exchange_buffer[i].buffer, num_of_data, num_of_layer,
                                  target_rank, exchange_tag);
    }

    if (intpl_buffer) free(intpl_buffer);
}

void exchange_class_recv_data_2d(exchange_class* self,
    buffer_class* exchange_target, int n_buf,
    int num_of_layer, int exchange_tag)
{
    int i;

    for (i = 0; i < self->ex_map.num_of_exchange_rank; i++) {
        int target_rank = self->ex_map.exchange_rank[i];
        int num_of_data = self->ex_map.num_of_exchange[i];
        jml_IRecvModel3_double2d(self->recv_comp_id, self->send_comp_id,
                                  exchange_target[i].buffer, num_of_data, num_of_layer,
                                  target_rank, exchange_tag);
    }
}

/* -----------------------------------------------------------------------
 * buffer_2_recv_data: accumulate exchange_buffer into recv_data
 * ----------------------------------------------------------------------- */
void exchange_class_buffer_2_recv_data(const exchange_class* self,
    const double* exchange_buffer, int eb_rows, int eb_cols,
    double* recv_data, int rd_rows, int rd_cols)
{
    int i, k;
    for (k = 0; k < rd_cols; k++) {
        for (i = 0; i < eb_rows; i++) {
            int recv_index = self->recv_conv_table[i] - 1;  /* 0-based */
            recv_data[recv_index + k * rd_rows] +=
                exchange_buffer[i + k * eb_rows];
        }
    }
}

/* -----------------------------------------------------------------------
 * interpolate_data  (dispatcher)
 * ----------------------------------------------------------------------- */
void exchange_class_interpolate_data(exchange_class* self,
    const double* send_data, int sd_rows, int sd_cols,
    double* recv_data, int rd_rows, int rd_cols,
    int num_of_layer, int intpl_tag)
{
    switch (self->intpl_mode) {
    case INTPL_SERIAL_FAST:
    case INTPL_SERIAL_SAFE:
        interpolate_data_serial(self, send_data, sd_rows, sd_cols,
                                recv_data, rd_rows, rd_cols,
                                num_of_layer, intpl_tag);
        break;
    case INTPL_PARALLEL:
        interpolate_data_parallel(self, send_data, sd_rows, sd_cols,
                                  recv_data, rd_rows, rd_cols,
                                  num_of_layer, intpl_tag);
        break;
    case INTPL_USER:
        if (self->user_interpolation) {
            self->user_interpolation(send_data, sd_rows, sd_cols,
                                     recv_data, rd_rows, rd_cols,
                                     self->send_conv_table, self->recv_conv_table,
                                     self->coef,
                                     num_of_layer,
                                     (self->recv_conv_table ? self->index_size : 0),
                                     intpl_tag);
        } else {
            rjn_error("interpolate_data",
                "intpl_mode is set to USER, but interpolation subroutine is not defined");
        }
        break;
    default:
        interpolate_data_serial(self, send_data, sd_rows, sd_cols,
                                recv_data, rd_rows, rd_cols,
                                num_of_layer, intpl_tag);
        break;
    }

}

static void interpolate_data_serial(exchange_class* self,
    const double* send_data, int sd_rows, int sd_cols,
    double* recv_data, int rd_rows, int rd_cols,
    int num_of_layer, int intpl_tag)
{
    int i, k;
    double missing_value = 0.0;
    int use_missing = (intpl_tag > 9800);

    if (!self->recv_conv_table) return;

    /* Zero out recv_data */
    for (i = 0; i < rd_rows * rd_cols; i++) recv_data[i] = 0.0;

    if (use_missing) {
        double* weight_data = (double*)calloc(rd_rows, sizeof(double));
        int*    check_data  = (int*)calloc(rd_rows, sizeof(int));
        if (intpl_tag > 9900)      missing_value = -999.0;
        else if (intpl_tag > 9800) missing_value = -1.0;

        for (k = 0; k < num_of_layer; k++) {
            memset(weight_data, 0, rd_rows * sizeof(double));
            memset(check_data,  0, rd_rows * sizeof(int));
            for (i = 0; i < self->index_size; i++) {
                int sg = self->send_conv_table[i] - 1;
                int rg = self->recv_conv_table[i] - 1;
                double sv = send_data[sg + k * sd_rows];
                if (sv == missing_value) {
                    check_data[rg] = 1;
                } else {
                    recv_data[rg + k * rd_rows] += sv * self->coef[i];
                    weight_data[rg] += self->coef[i];
                }
            }
            for (i = 0; i < rd_rows; i++) {
                if (weight_data[i] > 0.0)
                    recv_data[i + k * rd_rows] /= weight_data[i];
                else if (check_data[i])
                    recv_data[i + k * rd_rows] = missing_value;
            }
        }
        free(weight_data);
        free(check_data);
    } else {
        for (k = 0; k < num_of_layer; k++) {
            for (i = 0; i < self->index_size; i++) {
                int sg = self->send_conv_table[i] - 1;
                int rg = self->recv_conv_table[i] - 1;
                recv_data[rg + k * rd_rows] += send_data[sg + k * sd_rows] * self->coef[i];
            }
        }
    }
}

static void interpolate_data_parallel(exchange_class* self,
    const double* send_data, int sd_rows, int sd_cols,
    double* recv_data, int rd_rows, int rd_cols,
    int num_of_layer, int intpl_tag)
{
    int i, j, k;
    double missing_value = 0.0;
    int use_missing = (intpl_tag > 9800);

    if (!self->conv_table) return;

    for (i = 0; i < rd_rows * rd_cols; i++) recv_data[i] = 0.0;

    if (use_missing) {
        double* weight_data = (double*)calloc(rd_rows, sizeof(double));
        int*    check_data  = (int*)calloc(rd_rows, sizeof(int));
        if (intpl_tag > 9900)      missing_value = -999.0;
        else if (intpl_tag > 9800) missing_value = -1.0;

        for (k = 0; k < num_of_layer; k++) {
            memset(weight_data, 0, rd_rows * sizeof(double));
            memset(check_data,  0, rd_rows * sizeof(int));
            for (j = 0; j < self->conv_table_size; j++) {
                conv_table_2d* ct = &self->conv_table[j];
                for (i = 0; i < ct->table_size; i++) {
                    int sg = ct->send_conv_table[i] - 1;
                    int rg = ct->recv_conv_table[i] - 1;
                    double sv = send_data[sg + k * sd_rows];
                    if (sv == missing_value) {
                        check_data[rg] = 1;
                    } else {
                        recv_data[rg + k * rd_rows] += sv * ct->coef[i];
                        weight_data[rg] += ct->coef[i];
                    }
                }
            }
            for (i = 0; i < rd_rows; i++) {
                if (weight_data[i] > 0.0)
                    recv_data[i + k * rd_rows] /= weight_data[i];
                else if (check_data[i])
                    recv_data[i + k * rd_rows] = missing_value;
            }
        }
        free(weight_data);
        free(check_data);
    } else {
        for (k = 0; k < num_of_layer; k++) {
            for (j = 0; j < self->conv_table_size; j++) {
                conv_table_2d* ct = &self->conv_table[j];
                for (i = 0; i < ct->table_size; i++) {
                    int sg = ct->send_conv_table[i] - 1;
                    int rg = ct->recv_conv_table[i] - 1;
                    recv_data[rg + k * rd_rows] += send_data[sg + k * sd_rows] * ct->coef[i];
                }
            }
        }
    }
}

void exchange_class_set_user_interpolation(exchange_class* self,
    interpolation_user_ifc user_func)
{
    self->user_interpolation = user_func;
}

/* -----------------------------------------------------------------------
 * Persistence: write/read (binary)
 * ----------------------------------------------------------------------- */
static void write_int_array(FILE* fp, const int* a, int n) { fwrite(a, sizeof(int), n, fp); }
static void write_double_array(FILE* fp, const double* a, int n) { fwrite(a, sizeof(double), n, fp); }
static void read_int_array(FILE* fp, int** a, int n) {
    *a = (int*)malloc(n * sizeof(int));
    fread(*a, sizeof(int), n, fp);
}
static void read_double_array(FILE* fp, double** a, int n) {
    *a = (double*)malloc(n * sizeof(double));
    fread(*a, sizeof(double), n, fp);
}

void exchange_class_write(const exchange_class* self, FILE* fp)
{
    int i, table_size;

    fwrite(self->my_name,       STR_SHORT, 1, fp);
    fwrite(self->send_comp_name,STR_SHORT, 1, fp);
    fwrite(self->send_grid_name,STR_SHORT, 1, fp);
    fwrite(self->recv_comp_name,STR_SHORT, 1, fp);
    fwrite(self->recv_grid_name,STR_SHORT, 1, fp);
    fwrite(&self->map_tag,      sizeof(int), 1, fp);
    fwrite(&self->send_comp_id, sizeof(int), 1, fp);
    fwrite(&self->recv_comp_id, sizeof(int), 1, fp);
    fwrite(&self->intpl_flag,   sizeof(int), 1, fp);
    fwrite(&self->intpl_mode,   sizeof(int), 1, fp);

    fwrite(&self->index_size, sizeof(int), 1, fp);
    if (self->index_size > 0) {
        write_int_array(fp, self->send_grid_index, self->index_size);
        write_int_array(fp, self->recv_grid_index, self->index_size);
        write_double_array(fp, self->coef, self->index_size);
        write_int_array(fp, self->target_rank, self->index_size);
    }

    table_size = self->send_conv_table ? self->index_size : 0;
    fwrite(&table_size, sizeof(int), 1, fp);
    if (table_size > 0) write_int_array(fp, self->send_conv_table, table_size);

    table_size = self->recv_conv_table ? self->index_size : 0;
    fwrite(&table_size, sizeof(int), 1, fp);
    if (table_size > 0) write_int_array(fp, self->recv_conv_table, table_size);

    fwrite(&self->conv_table_size, sizeof(int), 1, fp);
    for (i = 0; i < self->conv_table_size; i++) {
        int ts = self->conv_table[i].table_size;
        fwrite(&ts, sizeof(int), 1, fp);
        if (ts > 0) {
            write_int_array(fp, self->conv_table[i].send_conv_table, ts);
            write_int_array(fp, self->conv_table[i].recv_conv_table, ts);
            write_double_array(fp, self->conv_table[i].coef, ts);
        }
    }

    fwrite(&self->ex_map.num_of_exchange_rank, sizeof(int), 1, fp);
    if (self->ex_map.num_of_exchange_rank > 0) {
        write_int_array(fp, self->ex_map.exchange_rank, self->ex_map.num_of_exchange_rank);
        write_int_array(fp, self->ex_map.num_of_exchange, self->ex_map.num_of_exchange_rank);
        write_int_array(fp, self->ex_map.offset, self->ex_map.num_of_exchange_rank);
    }

    fwrite(&self->ex_map.exchange_data_size, sizeof(int), 1, fp);

    table_size = self->ex_map.exchange_index ? self->ex_map.exchange_data_size : 0;
    fwrite(&table_size, sizeof(int), 1, fp);
    if (table_size > 0) write_int_array(fp, self->ex_map.exchange_index, table_size);

    table_size = self->ex_map.conv_table ? self->ex_map.exchange_data_size : 0;
    fwrite(&table_size, sizeof(int), 1, fp);
    if (table_size > 0) write_int_array(fp, self->ex_map.conv_table, table_size);

    fwrite(&self->send_map.intpled_data_size, sizeof(int), 1, fp);
    if (self->send_map.intpled_data_size > 0) {
        write_int_array(fp, self->send_map.intpled_index, self->send_map.intpled_data_size);
        write_int_array(fp, self->send_map.conv_table, self->send_map.intpled_data_size);
    }

    fwrite(&self->my_map.intpled_data_size, sizeof(int), 1, fp);
    if (self->my_map.intpled_data_size > 0) {
        write_int_array(fp, self->my_map.intpled_index, self->my_map.intpled_data_size);
        write_int_array(fp, self->my_map.conv_table, self->my_map.intpled_data_size);
    }
}

void exchange_class_read(exchange_class* self, FILE* fp)
{
    int i, conv_table_size;

    fread(self->my_name,       STR_SHORT, 1, fp);
    fread(self->send_comp_name,STR_SHORT, 1, fp);
    fread(self->send_grid_name,STR_SHORT, 1, fp);
    fread(self->recv_comp_name,STR_SHORT, 1, fp);
    fread(self->recv_grid_name,STR_SHORT, 1, fp);
    fread(&self->map_tag,      sizeof(int), 1, fp);
    fread(&self->send_comp_id, sizeof(int), 1, fp);
    fread(&self->recv_comp_id, sizeof(int), 1, fp);
    fread(&self->intpl_flag,   sizeof(int), 1, fp);
    fread(&self->intpl_mode,   sizeof(int), 1, fp);

    fread(&self->index_size, sizeof(int), 1, fp);
    if (self->index_size > 0) {
        read_int_array(fp, &self->send_grid_index, self->index_size);
        read_int_array(fp, &self->recv_grid_index, self->index_size);
        read_double_array(fp, &self->coef, self->index_size);
        read_int_array(fp, &self->target_rank, self->index_size);
    }

    fread(&conv_table_size, sizeof(int), 1, fp);
    if (conv_table_size > 0) read_int_array(fp, &self->send_conv_table, conv_table_size);

    fread(&conv_table_size, sizeof(int), 1, fp);
    if (conv_table_size > 0) read_int_array(fp, &self->recv_conv_table, conv_table_size);

    fread(&self->conv_table_size, sizeof(int), 1, fp);
    if (self->conv_table_size > 0) {
        self->conv_table = (conv_table_2d*)calloc(self->conv_table_size, sizeof(conv_table_2d));
        for (i = 0; i < self->conv_table_size; i++) {
            int ts;
            fread(&ts, sizeof(int), 1, fp);
            self->conv_table[i].table_size = ts;
            if (ts > 0) {
                read_int_array(fp, &self->conv_table[i].send_conv_table, ts);
                read_int_array(fp, &self->conv_table[i].recv_conv_table, ts);
                read_double_array(fp, &self->conv_table[i].coef, ts);
            }
        }
    }

    fread(&self->ex_map.num_of_exchange_rank, sizeof(int), 1, fp);
    if (self->ex_map.num_of_exchange_rank > 0) {
        read_int_array(fp, &self->ex_map.exchange_rank, self->ex_map.num_of_exchange_rank);
        read_int_array(fp, &self->ex_map.num_of_exchange, self->ex_map.num_of_exchange_rank);
        read_int_array(fp, &self->ex_map.offset, self->ex_map.num_of_exchange_rank);
    }

    fread(&self->ex_map.exchange_data_size, sizeof(int), 1, fp);

    fread(&conv_table_size, sizeof(int), 1, fp);
    if (conv_table_size > 0) read_int_array(fp, &self->ex_map.exchange_index, conv_table_size);

    fread(&conv_table_size, sizeof(int), 1, fp);
    if (conv_table_size > 0) read_int_array(fp, &self->ex_map.conv_table, conv_table_size);

    fread(&self->send_map.intpled_data_size, sizeof(int), 1, fp);
    if (self->send_map.intpled_data_size > 0) {
        read_int_array(fp, &self->send_map.intpled_index, self->send_map.intpled_data_size);
        read_int_array(fp, &self->send_map.conv_table, self->send_map.intpled_data_size);
    }

    fread(&self->my_map.intpled_data_size, sizeof(int), 1, fp);
    if (self->my_map.intpled_data_size > 0) {
        read_int_array(fp, &self->my_map.intpled_index, self->my_map.intpled_data_size);
        read_int_array(fp, &self->my_map.conv_table, self->my_map.intpled_data_size);
    }
}
