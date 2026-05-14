/*
 * rjn_grid.cpp
 * Converted from rjn_grid.f90
 * Copyright (c) 2026, arakawa@climtech.jp
 */

#include "rjn_grid.h"
#include "rjn_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* -----------------------------------------------------------------------
 * Private state (module-level)
 * ----------------------------------------------------------------------- */
#define GRID_MAX  8   /* same as MAX_GRID in rjn_constant.h */

static int        s_num_of_my_grid = 0;
static grid_class s_my_grid[GRID_MAX];
static int        s_my_comp_id     = 0;
static int        s_my_rank        = 0;
static int        s_my_size        = 0;

/* -----------------------------------------------------------------------
 * Public: init_grid
 * ----------------------------------------------------------------------- */
void init_grid(int comp_id, int rank, int size)
{
    s_my_comp_id     = comp_id;
    s_my_rank        = rank;
    s_my_size        = size;
    s_num_of_my_grid = 0;
}

/* -----------------------------------------------------------------------
 * Public: def_grid
 * ----------------------------------------------------------------------- */
void def_grid(const int* grid_index, int n, const char* comp_name, const char* grid_name)
{
    if (s_num_of_my_grid >= GRID_MAX)
        rjn_error("def_grid", "too many grids");

    s_my_grid[s_num_of_my_grid] = grid_class_init(s_my_comp_id, s_my_rank, s_my_size);
    s_my_grid[s_num_of_my_grid].def_grid(grid_index, n, comp_name, grid_name);
    s_num_of_my_grid++;
}

/* -----------------------------------------------------------------------
 * Public: end_grid_def  (no-op in original)
 * ----------------------------------------------------------------------- */
void end_grid_def(void)
{
    /* nothing to do */
}

/* -----------------------------------------------------------------------
 * Public: get_num_of_my_grid
 * ----------------------------------------------------------------------- */
int get_num_of_my_grid(void)
{
    return s_num_of_my_grid;
}

/* -----------------------------------------------------------------------
 * Public: get_grid_name  (grid_num is 1-based)
 * ----------------------------------------------------------------------- */
void get_grid_name(int grid_num, char* name_out)
{
    s_my_grid[grid_num - 1].get_grid_name(name_out);
}

/* -----------------------------------------------------------------------
 * Public: get_grid_ptr
 * Returns a pointer into the static array; NULL if not found.
 * ----------------------------------------------------------------------- */
grid_class* get_grid_ptr(const char* grid_name)
{
    int i;
    char gname[STR_SHORT];

    for (i = 0; i < s_num_of_my_grid; i++) {
        s_my_grid[i].get_grid_name(gname);
        if (strcmp(gname, grid_name) == 0)
            return &s_my_grid[i];
    }

    {
        char msg[STR_MID];
        snprintf(msg, STR_MID, "[get_grid_ptr]:no such grid name, grid_name = %s", grid_name);
        rjn_error("get_grid_ptr", msg);
    }
    return NULL;
}
