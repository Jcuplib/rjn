/*
 * rjn_map_class.cpp
 * Converted from rjn_map_class.f90
 * Copyright (c) 2026, arakawa@climtech.jp
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
    self->set_map_grid(my_grid);
}

/* -----------------------------------------------------------------------
 * Flag setters / getters
 * ----------------------------------------------------------------------- */
void map_class::set_map_grid(grid_class* my_grid) { this->my_grid = my_grid; }
void map_class::set_send_flag() { this->send_recv_flag = 1; }
void map_class::set_recv_flag() { this->send_recv_flag = 0; }
int  map_class::is_send_map() const { return this->send_recv_flag; }
int  map_class::is_recv_map() const { return !this->send_recv_flag; }

void map_class_set_send_flag(map_class* self) { self->set_send_flag(); }
void map_class_set_recv_flag(map_class* self) { self->set_recv_flag(); }
int  map_class_is_send_map(const map_class* self) { return self->is_send_map(); }
int  map_class_is_recv_map(const map_class* self) { return self->is_recv_map(); }

/* -----------------------------------------------------------------------
 * Getters
 * ----------------------------------------------------------------------- */
int map_class::get_comp_id() const { return this->comp_id; }
int map_class_get_comp_id(const map_class* self) { return self->get_comp_id(); }

int map_class::get_my_rank() const
{
    return this->my_grid->get_my_rank();
}

int map_class_get_my_rank(const map_class* self)
{
    return self->get_my_rank();
}

int map_class::get_exchange_array_size() const
{
    return this->num_of_exchange_index;
}

int map_class_get_exchange_array_size(const map_class* self)
{
    return self->get_exchange_array_size();
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
void map_class::set_exchange_index(int* index_ptr, int n)
{
    int  grid_size, i, res;
    int* grid_index;
    int* sorted_index;
    int* sorted_pos;

    this->num_of_exchange_index = n;
    this->exchange_index        = index_ptr;  /* borrowed */

    if (this->conv_table) free(this->conv_table);
    this->conv_table = (int*)malloc(n * sizeof(int));

    grid_index = this->my_grid->get_grid_index_ptr();
    grid_size  = this->my_grid->get_grid_size();

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
        this->conv_table[i] = sorted_pos[res];   /* 1-based */
    }

    free(sorted_index);
    free(sorted_pos);
}

void map_class_set_exchange_index(map_class* self, int* index_ptr, int n)
{
    self->set_exchange_index(index_ptr, n);
}

/* -----------------------------------------------------------------------
 * set_target_map_info
 * target_rank is a borrowed pointer (size n), sorted/grouped by rank.
 * Counts distinct consecutive ranks and builds num_of_data / offset arrays.
 * pe_num indices are 0-based in C (Fortran used 1-based).
 * ----------------------------------------------------------------------- */
void map_class::set_target_map_info(int* target_rank, int n)
{
    int i, counter;
    target_map_info* ti;

    if (this->tmap_info) {
        if (this->tmap_info->num_of_data) free(this->tmap_info->num_of_data);
        if (this->tmap_info->offset)      free(this->tmap_info->offset);
        free(this->tmap_info);
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

    this->tmap_info = ti;
}

void map_class_set_target_map_info(map_class* self, int* target_rank, int n)
{
    self->set_target_map_info(target_rank, n);
}

/* -----------------------------------------------------------------------
 * local_2_exchange
 * Reorders local_data[] into exchange_data[] using conv_table.
 * conv_table stores 1-based indices into local_data; adjust to 0-based.
 * ----------------------------------------------------------------------- */
void map_class::local_2_exchange(const double* local_data, double* exchange_data) const
{
    int i;
    for (i = 0; i < this->num_of_exchange_index; i++)
        exchange_data[i] = local_data[this->conv_table[i] - 1];
}

void map_class_local_2_exchange(const map_class* self,
                                const double* local_data, double* exchange_data)
{
    self->local_2_exchange(local_data, exchange_data);
}

/* -----------------------------------------------------------------------
 * exchange_2_local
 * Scatters exchange_data[] back into local_data[] using conv_table.
 * ----------------------------------------------------------------------- */
void map_class::exchange_2_local(double* local_data, const double* exchange_data) const
{
    int i;
    for (i = 0; i < this->num_of_exchange_index; i++)
        local_data[this->conv_table[i] - 1] = exchange_data[i];
}

void map_class_exchange_2_local(const map_class* self,
                                double* local_data, const double* exchange_data)
{
    self->exchange_2_local(local_data, exchange_data);
}

/* -----------------------------------------------------------------------
 * Exchange info queries (pe_num is 0-based)
 * ----------------------------------------------------------------------- */
int map_class::get_num_of_target_pe() const
{
    return this->tmap_info->num_of_pe;
}

int map_class_get_num_of_target_pe(const map_class* self)
{
    return self->get_num_of_target_pe();
}

int map_class::get_target_rank(int pe_num) const
{
    return this->tmap_info->target_rank[pe_num];
}

int map_class_get_target_rank(const map_class* self, int pe_num)
{
    return self->get_target_rank(pe_num);
}

int map_class::get_offset(int pe_num) const
{
    return this->tmap_info->offset[pe_num];
}

int map_class_get_offset(const map_class* self, int pe_num)
{
    return self->get_offset(pe_num);
}

int map_class::get_num_of_data(int pe_num) const
{
    return this->tmap_info->num_of_data[pe_num];
}

int map_class_get_num_of_data(const map_class* self, int pe_num)
{
    return self->get_num_of_data(pe_num);
}
