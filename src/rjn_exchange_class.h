#pragma once
/*
 * rjn_exchange_class.h
 * Converted from rjn_exchange_class.f90
 * Copyright (c) 2026, arakawa@climtech.jp
 */

#include <stdio.h>
#include <stdint.h>
#include "rjn_constant.h"

/**
 * @brief User-defined interpolation callback type.
 *
 * Register a callback of this type with rjn_set_user_interpolation() when the
 * corresponding mapping table uses the "USER" interpolation mode. The callback
 * receives flattened send and receive arrays, mapping indices, interpolation
 * coefficients, layer count, conversion-table size, and the user interpolation
 * tag from rjn_set_data().
 *
 * @param send_data Flattened source data.
 * @param send_rows First dimension length of @p send_data.
 * @param send_cols Second dimension length of @p send_data.
 * @param recv_data Flattened destination data, updated in place.
 * @param recv_rows First dimension length of @p recv_data.
 * @param recv_cols Second dimension length of @p recv_data.
 * @param send_index Source index table.
 * @param recv_index Destination index table.
 * @param coef Interpolation coefficient table.
 * @param num_of_layer Number of data layers.
 * @param num_of_conv Number of conversion-table entries.
 * @param intpl_tag User interpolation tag.
 */
typedef void (*interpolation_user_ifc)(
    const double* send_data, int send_rows, int send_cols,
    double* recv_data, int recv_rows, int recv_cols,
    const int* send_index, const int* recv_index,
    const double* coef,
    int num_of_layer, int num_of_conv, int intpl_tag);

#ifdef __cplusplus

/* -----------------------------------------------------------------------
 * buffer_class: MPI send/recv buffer for one target rank
 * ----------------------------------------------------------------------- */
class buffer_class {
public:
    int    target_rank;
    int    num_of_data;
    int    num_of_layer;
    double* buffer;       /* malloc'd [num_of_data * num_of_layer], column-major */
};

/* -----------------------------------------------------------------------
 * Internal map structures
 * ----------------------------------------------------------------------- */
class exchange_map_info {
public:
    int  num_of_exchange_rank;
    int* exchange_rank;        /* malloc'd [num_of_exchange_rank] */
    int* num_of_exchange;      /* malloc'd [num_of_exchange_rank] */
    int* offset;               /* malloc'd [num_of_exchange_rank] */
    int  exchange_data_size;
    int* exchange_index;       /* malloc'd [exchange_data_size] */
    int* conv_table;           /* malloc'd [exchange_data_size] */
};

class send_map_info {
public:
    int  intpled_data_size;
    int* intpled_index;        /* malloc'd */
    int* conv_table;           /* malloc'd */
};

class recv_map_info {
public:
    int  intpled_data_size;
    int* intpled_index;        /* malloc'd */
    int* conv_table;           /* malloc'd */
};

class conv_table_2d {
public:
    int    table_size;
    int*   send_conv_table;    /* malloc'd [table_size] */
    int*   recv_conv_table;    /* malloc'd [table_size] */
    double* coef;              /* malloc'd [table_size] */
};

/* -----------------------------------------------------------------------
 * exchange_class
 * ----------------------------------------------------------------------- */
class exchange_class {
public:
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

    void set_mapping_table(const char* send_comp_name, const char* send_grid_name,
                           const char* recv_comp_name, const char* recv_grid_name,
                           int map_tag, int intpl_flag,
                           const int* send_grid, const int* recv_grid,
                           const double* coef, int n);

    int  is_my_exchange(const char* send_comp_name, const char* send_grid_name,
                        const char* recv_comp_name, const char* recv_grid_name,
                        int map_tag) const;

    void get_my_name(char* out) const;
    void get_send_comp_name(char* out) const;
    void get_send_grid_name(char* out) const;
    void get_recv_comp_name(char* out) const;
    void get_recv_grid_name(char* out) const;
    int  get_map_tag() const;
    int  get_exchange_data_size() const;
    int  get_exchange_buffer_size() const;
    int  get_num_of_target_rank() const;
    int  get_target_rank(int target_rank_num) const;
    int  get_target_array_size(int target_rank_num) const;
    int  is_my_intpl() const;
    int  is_send_intpl() const;

    void local_2_exchange(const double* grid_data, double* exchange_data) const;
    void exchange_2_local(const double* exchange_data, double* grid_data) const;
    void target_2_exchange_buffer(const buffer_class* target_buffer, int n_buf,
                                  double* exchange_buffer, int exbuf_rows,
                                  int exbuf_cols) const;

    void send_data_1d(const double* data, int n_data,
                      double* exchange_buffer, int eb_n,
                      int intpl_tag, int exchange_tag);
    void recv_data_1d(double* exchange_buffer, int eb_n, int exchange_tag);
    void send_data_2d(const double* data, int n1, int n2,
                      buffer_class* exchange_buffer, int n_buf,
                      int num_of_layer, int intpl_tag, int exchange_tag);
    void recv_data_2d(buffer_class* exchange_target, int n_buf,
                      int num_of_layer, int exchange_tag);

    void buffer_2_recv_data(const double* exchange_buffer, int eb_rows, int eb_cols,
                            double* recv_data, int rd_rows, int rd_cols) const;
    void interpolate_data(const double* send_data, int sd_rows, int sd_cols,
                          double* recv_data, int rd_rows, int rd_cols,
                          int num_of_layer, int intpl_tag);
    void set_user_interpolation(interpolation_user_ifc user_func);
    void write(FILE* fp) const;
    void read(FILE* fp);
};

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

inline void exchange_class::set_mapping_table(
    const char* send_comp_name, const char* send_grid_name,
    const char* recv_comp_name, const char* recv_grid_name,
    int map_tag, int intpl_flag,
    const int* send_grid, const int* recv_grid, const double* coef, int n)
{
    exchange_class_set_mapping_table(this, send_comp_name, send_grid_name,
        recv_comp_name, recv_grid_name, map_tag, intpl_flag, send_grid,
        recv_grid, coef, n);
}

inline int exchange_class::is_my_exchange(
    const char* send_comp_name, const char* send_grid_name,
    const char* recv_comp_name, const char* recv_grid_name, int map_tag) const
{
    return exchange_class_is_my_exchange(this, send_comp_name, send_grid_name,
        recv_comp_name, recv_grid_name, map_tag);
}

inline void exchange_class::get_my_name(char* out) const { exchange_class_get_my_name(this, out); }
inline void exchange_class::get_send_comp_name(char* out) const { exchange_class_get_send_comp_name(this, out); }
inline void exchange_class::get_send_grid_name(char* out) const { exchange_class_get_send_grid_name(this, out); }
inline void exchange_class::get_recv_comp_name(char* out) const { exchange_class_get_recv_comp_name(this, out); }
inline void exchange_class::get_recv_grid_name(char* out) const { exchange_class_get_recv_grid_name(this, out); }
inline int exchange_class::get_map_tag() const { return exchange_class_get_map_tag(this); }
inline int exchange_class::get_exchange_data_size() const { return exchange_class_get_exchange_data_size(this); }
inline int exchange_class::get_exchange_buffer_size() const { return exchange_class_get_exchange_buffer_size(this); }
inline int exchange_class::get_num_of_target_rank() const { return exchange_class_get_num_of_target_rank(this); }
inline int exchange_class::get_target_rank(int target_rank_num) const { return exchange_class_get_target_rank(this, target_rank_num); }
inline int exchange_class::get_target_array_size(int target_rank_num) const { return exchange_class_get_target_array_size(this, target_rank_num); }
inline int exchange_class::is_my_intpl() const { return exchange_class_is_my_intpl(this); }
inline int exchange_class::is_send_intpl() const { return exchange_class_is_send_intpl(this); }

inline void exchange_class::local_2_exchange(const double* grid_data, double* exchange_data) const
{ exchange_class_local_2_exchange(this, grid_data, exchange_data); }
inline void exchange_class::exchange_2_local(const double* exchange_data, double* grid_data) const
{ exchange_class_exchange_2_local(this, exchange_data, grid_data); }
inline void exchange_class::target_2_exchange_buffer(const buffer_class* target_buffer, int n_buf,
    double* exchange_buffer, int exbuf_rows, int exbuf_cols) const
{ exchange_class_target_2_exchange_buffer(this, target_buffer, n_buf, exchange_buffer, exbuf_rows, exbuf_cols); }

inline void exchange_class::send_data_1d(const double* data, int n_data,
    double* exchange_buffer, int eb_n, int intpl_tag, int exchange_tag)
{ exchange_class_send_data_1d(this, data, n_data, exchange_buffer, eb_n, intpl_tag, exchange_tag); }
inline void exchange_class::recv_data_1d(double* exchange_buffer, int eb_n, int exchange_tag)
{ exchange_class_recv_data_1d(this, exchange_buffer, eb_n, exchange_tag); }
inline void exchange_class::send_data_2d(const double* data, int n1, int n2,
    buffer_class* exchange_buffer, int n_buf, int num_of_layer, int intpl_tag, int exchange_tag)
{ exchange_class_send_data_2d(this, data, n1, n2, exchange_buffer, n_buf, num_of_layer, intpl_tag, exchange_tag); }
inline void exchange_class::recv_data_2d(buffer_class* exchange_target, int n_buf,
    int num_of_layer, int exchange_tag)
{ exchange_class_recv_data_2d(this, exchange_target, n_buf, num_of_layer, exchange_tag); }

inline void exchange_class::buffer_2_recv_data(const double* exchange_buffer, int eb_rows, int eb_cols,
    double* recv_data, int rd_rows, int rd_cols) const
{ exchange_class_buffer_2_recv_data(this, exchange_buffer, eb_rows, eb_cols, recv_data, rd_rows, rd_cols); }
inline void exchange_class::interpolate_data(const double* send_data, int sd_rows, int sd_cols,
    double* recv_data, int rd_rows, int rd_cols, int num_of_layer, int intpl_tag)
{ exchange_class_interpolate_data(this, send_data, sd_rows, sd_cols, recv_data, rd_rows, rd_cols, num_of_layer, intpl_tag); }
inline void exchange_class::set_user_interpolation(interpolation_user_ifc user_func)
{ exchange_class_set_user_interpolation(this, user_func); }
inline void exchange_class::write(FILE* fp) const { exchange_class_write(this, fp); }
inline void exchange_class::read(FILE* fp) { exchange_class_read(this, fp); }

#endif /* __cplusplus */
