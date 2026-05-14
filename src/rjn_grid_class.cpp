/*
 * rjn_grid_class.cpp
 * Converted from rjn_grid_class.f90
 * Copyright (c) 2026, arakawa@climtech.jp
 */

#include "rjn_grid_class.h"
#include <stdlib.h>
#include <string.h>

/* -----------------------------------------------------------------------
 * Constructor
 * ----------------------------------------------------------------------- */
grid_class grid_class_init(int comp_id, int my_rank, int my_size)
{
    grid_class g;
    g.comp_id           = comp_id;
    g.my_rank           = my_rank;
    g.my_size           = my_size;
    g.num_of_grid_index = 0;
    g.grid_index        = NULL;
    g.comp_name[0]      = '\0';
    g.grid_name[0]      = '\0';
    return g;
}

/* -----------------------------------------------------------------------
 * Destructor
 * ----------------------------------------------------------------------- */
void grid_class_free(grid_class* self)
{
    self->free();
}

/* -----------------------------------------------------------------------
 * def_grid: set grid name/comp name and copy index array
 * ----------------------------------------------------------------------- */
void grid_class::def_grid(const int* grid_index, int n,
                          const char* comp_name, const char* grid_name)
{
    strncpy(this->comp_name, comp_name, STR_SHORT - 1);
    this->comp_name[STR_SHORT - 1] = '\0';

    strncpy(this->grid_name, grid_name, STR_SHORT - 1);
    this->grid_name[STR_SHORT - 1] = '\0';

    this->num_of_grid_index = n;
    this->grid_index = (int*)malloc(n * sizeof(int));
    memcpy(this->grid_index, grid_index, n * sizeof(int));
}

void grid_class_def_grid(grid_class* self,
                         const int* grid_index, int n,
                         const char* comp_name, const char* grid_name)
{
    self->def_grid(grid_index, n, comp_name, grid_name);
}

/* -----------------------------------------------------------------------
 * Getters
 * ----------------------------------------------------------------------- */
void grid_class::get_grid_name(char* out) const
{
    strncpy(out, this->grid_name, STR_SHORT - 1);
    out[STR_SHORT - 1] = '\0';
}

void grid_class_get_grid_name(const grid_class* self, char* out)
{
    self->get_grid_name(out);
}

int grid_class::get_comp_id() const
{
    return this->comp_id;
}

int grid_class_get_comp_id(const grid_class* self)
{
    return self->get_comp_id();
}

int grid_class::get_my_rank() const
{
    return this->my_rank;
}

int grid_class_get_my_rank(const grid_class* self)
{
    return self->get_my_rank();
}

int grid_class::get_my_size() const
{
    return this->my_size;
}

int grid_class_get_my_size(const grid_class* self)
{
    return self->get_my_size();
}

int grid_class::get_grid_size() const
{
    return this->num_of_grid_index;
}

int grid_class_get_grid_size(const grid_class* self)
{
    return self->get_grid_size();
}

int* grid_class::get_grid_index_ptr() const
{
    return this->grid_index;
}

int* grid_class_get_grid_index_ptr(const grid_class* self)
{
    return self->get_grid_index_ptr();
}

void grid_class::free()
{
    if (this->grid_index) {
        ::free(this->grid_index);
        this->grid_index = NULL;
    }
    this->num_of_grid_index = 0;
}
