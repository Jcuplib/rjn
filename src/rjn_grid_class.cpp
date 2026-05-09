/*
 * rjn_grid_class.cpp
 * Converted from rjn_grid_class.f90
 * Copyright (c) 2011, arakawa@rist.jp
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
    if (self->grid_index) {
        free(self->grid_index);
        self->grid_index = NULL;
    }
    self->num_of_grid_index = 0;
}

/* -----------------------------------------------------------------------
 * def_grid: set grid name/comp name and copy index array
 * ----------------------------------------------------------------------- */
void grid_class_def_grid(grid_class* self,
                         const int* grid_index, int n,
                         const char* comp_name, const char* grid_name)
{
    strncpy(self->comp_name, comp_name, STR_SHORT - 1);
    self->comp_name[STR_SHORT - 1] = '\0';

    strncpy(self->grid_name, grid_name, STR_SHORT - 1);
    self->grid_name[STR_SHORT - 1] = '\0';

    self->num_of_grid_index = n;
    self->grid_index = (int*)malloc(n * sizeof(int));
    memcpy(self->grid_index, grid_index, n * sizeof(int));
}

/* -----------------------------------------------------------------------
 * Getters
 * ----------------------------------------------------------------------- */
void grid_class_get_grid_name(const grid_class* self, char* out)
{
    strncpy(out, self->grid_name, STR_SHORT - 1);
    out[STR_SHORT - 1] = '\0';
}

int grid_class_get_comp_id(const grid_class* self)
{
    return self->comp_id;
}

int grid_class_get_my_rank(const grid_class* self)
{
    return self->my_rank;
}

int grid_class_get_my_size(const grid_class* self)
{
    return self->my_size;
}

int grid_class_get_grid_size(const grid_class* self)
{
    return self->num_of_grid_index;
}

int* grid_class_get_grid_index_ptr(const grid_class* self)
{
    return self->grid_index;
}
