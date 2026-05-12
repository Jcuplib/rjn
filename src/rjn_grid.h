#pragma once
/*
 * rjn_grid.h
 * Converted from rjn_grid.f90
 * Copyright (c) 2026, arakawa@climtech.jp
 */

#include "rjn_constant.h"
#include "rjn_grid_class.h"

/* -----------------------------------------------------------------------
 * Public interface
 * ----------------------------------------------------------------------- */
void  init_grid(int comp_id, int rank, int size);
void  def_grid(const int* grid_index, int n, const char* comp_name, const char* grid_name);
void  end_grid_def(void);

int   get_num_of_my_grid(void);
void  get_grid_name(int grid_num, char* name_out); /* name_out: char[STR_SHORT], 1-based grid_num */
grid_class* get_grid_ptr(const char* grid_name);   /* returns pointer into internal array */
