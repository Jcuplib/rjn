#pragma once
/*
 * rjn_map_class.h
 * Converted from rjn_map_class.f90
 * Copyright (c) 2026, arakawa@climtech.jp
 */

#include "rjn_constant.h"
#include "rjn_grid_class.h"

/* -----------------------------------------------------------------------
 * target_map_info: exchange table for one target component
 * ----------------------------------------------------------------------- */
typedef struct {
    int  num_of_pe;             /* number of distinct target ranks */
    int* target_rank;           /* [num_of_pe] unique target ranks (borrowed ptr) */
    int* num_of_data;           /* [num_of_pe] number of data items for each rank */
    int* offset;                /* [num_of_pe] cumulative offset into exchange buffer */
    int  my_data_size;          /* total number of data items */
} target_map_info;

/* -----------------------------------------------------------------------
 * map_class
 * ----------------------------------------------------------------------- */
typedef struct {
    grid_class*     my_grid;              /* pointer to external grid_class (not owned) */
    int             comp_id;
    int             send_recv_flag;       /* 1 = send map, 0 = recv map */
    int             num_of_exchange_index;
    int*            exchange_index;       /* borrowed pointer (set_exchange_index) */
    int*            conv_table;           /* malloc'd [num_of_exchange_index] */
    target_map_info* tmap_info;           /* malloc'd (set_target_map_info) */
} map_class;

/* -----------------------------------------------------------------------
 * Constructor
 * ----------------------------------------------------------------------- */
map_class map_class_init(int comp_id);

/* -----------------------------------------------------------------------
 * Methods
 * ----------------------------------------------------------------------- */
void map_class_set_map_grid(map_class* self, grid_class* my_grid);
int  map_class_get_comp_id(const map_class* self);
int  map_class_get_my_rank(const map_class* self);

void map_class_set_send_flag(map_class* self);
void map_class_set_recv_flag(map_class* self);
int  map_class_is_send_map(const map_class* self);   /* 1/0 */
int  map_class_is_recv_map(const map_class* self);   /* 1/0 */

int  map_class_get_exchange_array_size(const map_class* self);

/* index_ptr is a borrowed pointer (the caller keeps ownership) */
void map_class_set_exchange_index(map_class* self, int* index_ptr, int n);

/* target_rank is a borrowed pointer */
void map_class_set_target_map_info(map_class* self, int* target_rank, int n);

/* Data conversion: local grid array ↔ exchange array */
void map_class_local_2_exchange(const map_class* self,
                                const double* local_data, double* exchange_data);
void map_class_exchange_2_local(const map_class* self,
                                double* local_data, const double* exchange_data);

/* Exchange info queries (pe_num is 0-based) */
int  map_class_get_num_of_target_pe(const map_class* self);
int  map_class_get_target_rank(const map_class* self, int pe_num);
int  map_class_get_offset(const map_class* self, int pe_num);
int  map_class_get_num_of_data(const map_class* self, int pe_num);
