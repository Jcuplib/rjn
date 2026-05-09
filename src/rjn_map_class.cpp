/*
 * rjn_map_class.cpp
 * Converted from rjn_map_class.f90
 * Copyright (c) 2011, arakawa@rist.jp
 */

#include "rjn_map_class.h"
#include "rjn_utils.h"
#include <stdlib.h>
#include <string.h>

/* -----------------------------------------------------------------------
 * Constructor
 * ----------------------------------------------------------------------- */
map_class map_class_init(int comp_id)
{
    map_class m;
    m.comp_id               = comp_id;
    m.num_of_exchange_index = 0;
    m.send_recv_flag        = 1;   /* default: send map */
    m.my_grid               = NULL;
    m.exchange_index        = NULL;
    m.conv_table            = NULL;
    m.tmap_info             = NULL;
    return m;
}

/* -----------------------------------------------------------------------
 * set_map_grid
 * ----------------------------------------------------------------------- */
void map_class_set_map_grid(map_class* self, grid_class* my_grid)
{
    self->my_grid = my_grid;
}

/* -----------------------------------------------------------------------
 * Flag setters / getters
 * ----------------------------------------------------------------------- */
void map_class_set_send_flag(map_class* self) { self->send_recv_flag = 1; }
void map_class_set_recv_flag(map_class* self) { self->send_recv_flag = 0; }
int  map_class_is_send_map(const map_class* self) { return self->send_recv_flag; }
int  map_class_is_recv_map(const map_class* self) { return !self->send_recv_flag; }

/* -----------------------------------------------------------------------
 * Getters
 * ----------------------------------------------------------------------- */
int map_class_get_comp_id(const map_class* self) { return self->comp_id; }

int map_class_get_my_rank(const map_class* self)
{
    return grid_class_get_my_rank(self->my_grid);
}

int map_class_get_exchange_array_size(const map_class* self)
{
    return self->num_of_exchange_index;
}

/* -----------------------------------------------------------------------
 * set_exchange_index
 * Builds conv_table: for each exchange index, find its position in the
 * (sorted) grid_index array.  Uses a simple binary search.
 *
 * Fortran used 1-based indexing throughout; here sorted_pos stores the
 * 1-based position in the original grid_index array, consistent with
 * local_2_exchange / exchange_2_local which use conv_table as a 1-based
 * subscript into a caller-provided C array (adjusted by -1 at call site).
 * ----------------------------------------------------------------------- */
void map_class_set_exchange_index(map_class* self, int* index_ptr, int n)
{
    int  grid_size, i, res;
    int* grid_index;
    int* sorted_index;
    int* sorted_pos;

    self->num_of_exchange_index = n;
    self->exchange_index        = index_ptr;  /* borrowed */

    if (self->conv_table) free(self->conv_table);
    self->conv_table = (int*)malloc(n * sizeof(int));

    grid_index = grid_class_get_grid_index_ptr(self->my_grid);
    grid_size  = grid_class_get_grid_size(self->my_grid);

    sorted_index = (int*)malloc(grid_size * sizeof(int));
    sorted_pos   = (int*)malloc(grid_size * sizeof(int));

    for (i = 0; i < grid_size; i++) {
        sorted_index[i] = grid_index[i];
        sorted_pos[i]   = i + 1;   /* 1-based position (Fortran convention) */
    }

    sort_int_1d(grid_size, sorted_index, sorted_pos);

    for (i = 0; i < n; i++) {
        res = binary_search_int(sorted_index, grid_size, index_ptr[i]);
        if (res < 0)
            rjn_error("set_exchange_index", "exchange index not found in grid_index");
        self->conv_table[i] = sorted_pos[res];   /* 1-based */
    }

    free(sorted_index);
    free(sorted_pos);
}

/* -----------------------------------------------------------------------
 * set_target_map_info
 * target_rank is a borrowed pointer (size n), sorted/grouped by rank.
 * Counts distinct consecutive ranks and builds num_of_data / offset arrays.
 * pe_num indices are 0-based in C (Fortran used 1-based).
 * ----------------------------------------------------------------------- */
void map_class_set_target_map_info(map_class* self, int* target_rank, int n)
{
    int i, counter;
    target_map_info* ti;

    if (self->tmap_info) {
        if (self->tmap_info->num_of_data) free(self->tmap_info->num_of_data);
        if (self->tmap_info->offset)      free(self->tmap_info->offset);
        free(self->tmap_info);
    }

    ti = (target_map_info*)malloc(sizeof(target_map_info));
    ti->target_rank   = target_rank;   /* borrowed */
    ti->my_data_size  = n;

    /* Count distinct consecutive ranks */
    ti->num_of_pe = (n > 0) ? 1 : 0;
    for (i = 1; i < n; i++) {
        if (target_rank[i] != target_rank[i-1])
            ti->num_of_pe++;
    }

    ti->num_of_data = (int*)malloc(ti->num_of_pe * sizeof(int));
    ti->offset      = (int*)malloc(ti->num_of_pe * sizeof(int));

    /* Fill num_of_data */
    for (i = 0; i < ti->num_of_pe; i++) ti->num_of_data[i] = 1;
    counter = 0;
    for (i = 1; i < n; i++) {
        if (target_rank[i] == target_rank[i-1])
            ti->num_of_data[counter]++;
        else
            counter++;
    }

    /* Fill offset */
    if (ti->num_of_pe > 0) {
        ti->offset[0] = 0;
        for (i = 1; i < ti->num_of_pe; i++)
            ti->offset[i] = ti->offset[i-1] + ti->num_of_data[i-1];
    }

    self->tmap_info = ti;
}

/* -----------------------------------------------------------------------
 * local_2_exchange
 * Reorders local_data[] into exchange_data[] using conv_table.
 * conv_table stores 1-based indices into local_data; adjust to 0-based.
 * ----------------------------------------------------------------------- */
void map_class_local_2_exchange(const map_class* self,
                                const double* local_data, double* exchange_data)
{
    int i;
    for (i = 0; i < self->num_of_exchange_index; i++)
        exchange_data[i] = local_data[self->conv_table[i] - 1];
}

/* -----------------------------------------------------------------------
 * exchange_2_local
 * Scatters exchange_data[] back into local_data[] using conv_table.
 * ----------------------------------------------------------------------- */
void map_class_exchange_2_local(const map_class* self,
                                double* local_data, const double* exchange_data)
{
    int i;
    for (i = 0; i < self->num_of_exchange_index; i++)
        local_data[self->conv_table[i] - 1] = exchange_data[i];
}

/* -----------------------------------------------------------------------
 * Exchange info queries (pe_num is 0-based)
 * ----------------------------------------------------------------------- */
int map_class_get_num_of_target_pe(const map_class* self)
{
    return self->tmap_info->num_of_pe;
}

int map_class_get_target_rank(const map_class* self, int pe_num)
{
    return self->tmap_info->target_rank[pe_num];
}

int map_class_get_offset(const map_class* self, int pe_num)
{
    return self->tmap_info->offset[pe_num];
}

int map_class_get_num_of_data(const map_class* self, int pe_num)
{
    return self->tmap_info->num_of_data[pe_num];
}
