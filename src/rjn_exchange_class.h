#pragma once
/*
 * rjn_exchange_class.h
 * Converted from rjn_exchange_class.f90
 * Copyright (c) 2011, arakawa@rist.jp
 */

#include <stdio.h>
#include <stdint.h>
#include "rjn_constant.h"

/* -----------------------------------------------------------------------
 * buffer_class: MPI send/recv buffer for one target rank
 * ----------------------------------------------------------------------- */
typedef struct {
    int    target_rank;
    int    num_of_data;
    int    num_of_layer;
    double* buffer;       /* malloc'd [num_of_data * num_of_layer], row-major */
} buffer_class;

/* -----------------------------------------------------------------------
 * Internal map structures
 * ----------------------------------------------------------------------- */
typedef struct {
    int  num_of_exchange_rank;
    int* exchange_rank;        /* malloc'd [num_of_exchange_rank] */
    int* num_of_exchange;      /* malloc'd [num_of_exchange_rank] */
    int* offset;               /* malloc'd [num_of_exchange_rank] */
    int  exchange_data_size;
    int* exchange_index;       /* malloc'd [exchange_data_size] */
    int* conv_table;           /* malloc'd [exchange_data_size] */
} exchange_map_info;

typedef struct {
    int  intpled_data_size;
    int* intpled_index;        /* malloc'd */
    int* conv_table;           /* malloc'd */
} send_map_info;

typedef struct {
    int  intpled_data_size;
    int* intpled_index;        /* malloc'd */
    int* conv_table;           /* malloc'd */
} recv_map_info;

typedef struct {
    int    table_size;
    int*   send_conv_table;    /* malloc'd [table_size] */
    int*   recv_conv_table;    /* malloc'd [table_size] */
    double* coef;              /* malloc'd [table_size] */
} conv_table_2d;

/* -----------------------------------------------------------------------
 * User-defined interpolation function pointer type
 * ----------------------------------------------------------------------- */
typedef void (*interpolation_user_ifc)(
    const double* send_data, int send_rows, int send_cols,
    double* recv_data, int recv_rows, int recv_cols,
    const int* send_index, const int* recv_index,
    const double* coef,
    int num_of_layer, int num_of_conv, int intpl_tag);

/* -----------------------------------------------------------------------
 * exchange_class
 * ----------------------------------------------------------------------- */
typedef struct {
    char my_name[STR_SHORT];
    char send_comp_name[STR_SHORT];
    char send_grid_name[STR_SHORT];
    char recv_comp_name[STR_SHORT];
    char recv_grid_name[STR_SHORT];
    int  map_tag;
    int  send_comp_id;
    int  recv_comp_id;

    /* remapping table */
    int     index_size;
    int*    send_grid_index;   /* malloc'd [index_size] */
    int*    recv_grid_index;   /* malloc'd [index_size] */
    double* coef;              /* malloc'd [index_size] */
    int*    target_rank;       /* malloc'd [index_size] */

    /* conversion tables */
    int*    send_conv_table;   /* malloc'd */
    int*    recv_conv_table;   /* malloc'd */

    /* 2D parallel interpolation table */
    int           conv_table_size;
    conv_table_2d* conv_table; /* malloc'd [conv_table_size] */

    exchange_map_info ex_map;
    send_map_info     send_map;
    recv_map_info     my_map;

    int  intpl_flag;                          /* 1/0 */
    int  intpl_mode;
    interpolation_user_ifc user_interpolation; /* NULL if not set */
} exchange_class;

/* -----------------------------------------------------------------------
 * Constructor
 * ----------------------------------------------------------------------- */
exchange_class exchange_class_init(const char* my_name, int intpl_flag, int intpl_mode);

/* -----------------------------------------------------------------------
 * Methods
 * ----------------------------------------------------------------------- */
void exchange_class_set_mapping_table(exchange_class* self,
    const char* send_comp_name, const char* send_grid_name,
    const char* recv_comp_name, const char* recv_grid_name,
    int map_tag, int intpl_flag,
    const int* send_grid, const int* recv_grid, const double* coef, int n);

int  exchange_class_is_my_exchange(const exchange_class* self,
    const char* send_comp_name, const char* send_grid_name,
    const char* recv_comp_name, const char* recv_grid_name, int map_tag); /* 1/0 */

void exchange_class_get_my_name(const exchange_class* self, char* out);
void exchange_class_get_send_comp_name(const exchange_class* self, char* out);
void exchange_class_get_send_grid_name(const exchange_class* self, char* out);
void exchange_class_get_recv_comp_name(const exchange_class* self, char* out);
void exchange_class_get_recv_grid_name(const exchange_class* self, char* out);
int  exchange_class_get_map_tag(const exchange_class* self);
int  exchange_class_get_exchange_data_size(const exchange_class* self);
int  exchange_class_get_exchange_buffer_size(const exchange_class* self);
int  exchange_class_get_num_of_target_rank(const exchange_class* self);
int  exchange_class_get_target_rank(const exchange_class* self, int target_rank_num); /* 1-based */
int  exchange_class_get_target_array_size(const exchange_class* self, int target_rank_num);
int  exchange_class_is_my_intpl(const exchange_class* self);  /* 1/0 */
int  exchange_class_is_send_intpl(const exchange_class* self); /* 1/0 */

void exchange_class_local_2_exchange(const exchange_class* self,
    const double* grid_data, double* exchange_data);
void exchange_class_exchange_2_local(const exchange_class* self,
    const double* exchange_data, double* grid_data);
void exchange_class_target_2_exchange_buffer(const exchange_class* self,
    const buffer_class* target_buffer, int n_buf,
    double* exchange_buffer, int exbuf_rows, int exbuf_cols);

/* exchange_buffer is [exchange_data_size][1] (2D flattened) */
void exchange_class_send_data_1d(exchange_class* self,
    const double* data, int n_data,
    double* exchange_buffer, int eb_n,
    int intpl_tag, int exchange_tag);

void exchange_class_recv_data_1d(exchange_class* self,
    double* exchange_buffer, int eb_n,
    int exchange_tag);

void exchange_class_send_data_2d(exchange_class* self,
    const double* data, int n1, int n2,
    buffer_class* exchange_buffer, int n_buf,
    int num_of_layer, int intpl_tag, int exchange_tag);

void exchange_class_recv_data_2d(exchange_class* self,
    buffer_class* exchange_target, int n_buf,
    int num_of_layer, int exchange_tag);

/* Accumulate exchange_buffer into recv_data (recv_data += contribution) */
void exchange_class_buffer_2_recv_data(const exchange_class* self,
    const double* exchange_buffer, int eb_rows, int eb_cols,
    double* recv_data, int rd_rows, int rd_cols);

void exchange_class_interpolate_data(exchange_class* self,
    const double* send_data, int sd_rows, int sd_cols,
    double* recv_data, int rd_rows, int rd_cols,
    int num_of_layer, int intpl_tag);

void exchange_class_set_user_interpolation(exchange_class* self,
    interpolation_user_ifc user_func);

void exchange_class_write(const exchange_class* self, FILE* fp);
void exchange_class_read(exchange_class* self, FILE* fp);
