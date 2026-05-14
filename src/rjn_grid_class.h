#pragma once
/*
 * rjn_grid_class.h
 * Converted from rjn_grid_class.f90
 * Copyright (c) 2026, arakawa@climtech.jp
 */

#include "rjn_constant.h"

/* -----------------------------------------------------------------------
 * grid_class
 * ----------------------------------------------------------------------- */
class grid_class {
public:
    int  comp_id;
    int  my_rank;
    int  my_size;
    char comp_name[STR_SHORT];
    char grid_name[STR_SHORT];
    int  num_of_grid_index;
    int* grid_index;   /* malloc'd array[num_of_grid_index] */

    void def_grid(const int* grid_index, int n,
                  const char* comp_name, const char* grid_name);
    void get_grid_name(char* out) const;
    int  get_comp_id() const;
    int  get_my_rank() const;
    int  get_my_size() const;
    int  get_grid_size() const;
    int* get_grid_index_ptr() const;
    void free();
};

/* -----------------------------------------------------------------------
 * Constructor / destructor
 * ----------------------------------------------------------------------- */
grid_class grid_class_init(int comp_id, int my_rank, int my_size);
void       grid_class_free(grid_class* self);

/* -----------------------------------------------------------------------
 * Methods (free functions using self pointer)
 * ----------------------------------------------------------------------- */
void  grid_class_def_grid(grid_class* self,
                          const int* grid_index, int n,
                          const char* comp_name, const char* grid_name);

void  grid_class_get_grid_name(const grid_class* self, char* out); /* char[STR_SHORT] */
int   grid_class_get_comp_id(const grid_class* self);
int   grid_class_get_my_rank(const grid_class* self);
int   grid_class_get_my_size(const grid_class* self);
int   grid_class_get_grid_size(const grid_class* self);
int*  grid_class_get_grid_index_ptr(const grid_class* self); /* returns raw pointer */
