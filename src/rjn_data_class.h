#pragma once
/*
 * rjn_data_class.h
 * Converted from rjn_data_class.f90
 * Copyright (c) 2026, arakawa@climtech.jp
 */

#include <stdint.h>
#include "rjn_constant.h"
#include "rjn_exchange_class.h"

/* -----------------------------------------------------------------------
 * data_class
 * ----------------------------------------------------------------------- */
class data_class {
public:
    char my_name[STR_SHORT];
    char send_comp_name[STR_SHORT];
    char send_grid_name[STR_SHORT];
    char send_data_name[STR_SHORT];
    char recv_comp_name[STR_SHORT];
    char recv_grid_name[STR_SHORT];
    char recv_data_name[STR_SHORT];

    exchange_class* my_exchange;  /* borrowed pointer */

    int     avr_flag;           /* 1 = average, 0 = snapshot */
    int     intvl;              /* exchange interval (seconds) */
    int     time_lag;           /* -1, 0, 1 */
    int     exchange_type;      /* CONCURRENT_SEND_RECV etc. */
    int     num_of_layer;       /* default 1 */
    double  offset;             /* default 0.0 */
    double  factor;             /* default 1.0 */
    int     grid_intpl_tag;     /* interpolation tag */
    int     exchange_tag;       /* MPI tag */

    int     exchange_data_size;    /* size of exchange data */
    int     exchange_buffer_size;  /* size of received data before interpolation */
    double  fill_value;

    double* data1d;        /* malloc'd [exchange_data_size] (num_of_layer==1) */
    double* data1d_tmp;    /* malloc'd [exchange_data_size] (averaging temp) */
    double* data2d;        /* malloc'd [exchange_data_size * num_of_layer] (row-major) */
    double* weight2d;      /* malloc'd [exchange_data_size * num_of_layer] */
    double* exchange_buffer; /* malloc'd [exchange_buffer_size * effective_layers] */

    int64_t put_sec;       /* time of last put, -1 = uninitialised */

    int          num_of_target;       /* number of exchange target processes */
    buffer_class* exchange_target;    /* malloc'd [num_of_target] */

    void set_my_exchange(const char* send_comp_name, const char* send_grid_name,
                         const char* recv_comp_name, const char* recv_grid_name,
                         int map_tag);
    exchange_class* get_my_exchange() const;

    void get_my_name(char* out) const;
    void get_send_comp_name(char* out) const;
    void get_send_grid_name(char* out) const;
    void get_send_data_name(char* out) const;
    void get_recv_comp_name(char* out) const;
    void get_recv_grid_name(char* out) const;
    void get_recv_data_name(char* out) const;

    int is_avr() const;
    int get_intvl() const;
    int get_time_lag() const;
    int get_exchange_type() const;
    int get_num_of_layer() const;
    int get_exchange_tag() const;
    int is_my_intpl() const;

    void put_data_1d(const double* data, int n_data,
                     int64_t next_sec, int32_t delta_t);
    void put_data_2d(const double* data, int n1, int n2,
                     int64_t next_sec, int32_t delta_t);
    void recv_data_1d(int64_t current_sec);
    void recv_data_2d(int64_t current_sec);
    void interpolate_data_1d();
    void interpolate_data_2d();
    void get_data_1d(double* data, int n_data,
                     int64_t current_sec, int* is_get_ok);
    void get_data_2d(double* data, int n1, int n2,
                     int64_t current_sec, int* is_get_ok);
};

/* -----------------------------------------------------------------------
 * Constructor
 * factor and offset are optional; pass 1.0 / 0.0 for defaults.
 * ----------------------------------------------------------------------- */
data_class data_class_init(
    const char* send_comp_name, const char* send_grid_name, const char* send_data_name,
    const char* recv_comp_name, const char* recv_grid_name, const char* recv_data_name,
    int avr_flag, int intvl, int time_lag, int exchange_type,
    int num_of_layer, int intpl_tag,
    double fill_value, int exchange_tag,
    double factor, double offset);

/* -----------------------------------------------------------------------
 * Methods
 * ----------------------------------------------------------------------- */

/* Link this data_class to the matching exchange_class; allocates buffers. */
void data_class_set_my_exchange(data_class* self,
    const char* send_comp_name, const char* send_grid_name,
    const char* recv_comp_name, const char* recv_grid_name,
    int map_tag);

exchange_class* data_class_get_my_exchange(const data_class* self);

void data_class_get_my_name(const data_class* self, char* out);
void data_class_get_send_comp_name(const data_class* self, char* out);
void data_class_get_send_grid_name(const data_class* self, char* out);
void data_class_get_send_data_name(const data_class* self, char* out);
void data_class_get_recv_comp_name(const data_class* self, char* out);
void data_class_get_recv_grid_name(const data_class* self, char* out);
void data_class_get_recv_data_name(const data_class* self, char* out);

int data_class_is_avr(const data_class* self);          /* 1/0 */
int data_class_get_intvl(const data_class* self);
int data_class_get_time_lag(const data_class* self);
int data_class_get_exchange_type(const data_class* self);
int data_class_get_num_of_layer(const data_class* self);
int data_class_get_exchange_tag(const data_class* self);
int data_class_is_my_intpl(const data_class* self);     /* 1/0 */

/* Send-side: put data into the exchange buffer and (conditionally) send.
 * data   : flat array [n_data] (1D) or [n1 * n2] (2D, row-major)
 * next_sec / delta_t: time-stepping control for averaging / interval logic
 */
void data_class_put_data_1d(data_class* self,
    const double* data, int n_data,
    int64_t next_sec, int32_t delta_t);

void data_class_put_data_2d(data_class* self,
    const double* data, int n1, int n2,
    int64_t next_sec, int32_t delta_t);

/* Receive-side: post non-blocking receive for 1D / 2D data */
void data_class_recv_data_1d(data_class* self, int64_t current_sec);
void data_class_recv_data_2d(data_class* self, int64_t current_sec);

/* Interpolate received exchange buffer into data1d / data2d */
void data_class_interpolate_data_1d(data_class* self);
void data_class_interpolate_data_2d(data_class* self);

/* Copy interpolated data into caller-supplied array.
 * data   : flat array [n_data] (1D) or [n1 * n2] (2D, row-major)
 * is_get_ok: set to 1 if data was available at current_sec, 0 otherwise
 */
void data_class_get_data_1d(data_class* self,
    double* data, int n_data,
    int64_t current_sec, int* is_get_ok);

void data_class_get_data_2d(data_class* self,
    double* data, int n1, int n2,
    int64_t current_sec, int* is_get_ok);

inline void data_class::set_my_exchange(
    const char* send_comp_name, const char* send_grid_name,
    const char* recv_comp_name, const char* recv_grid_name, int map_tag)
{
    data_class_set_my_exchange(this, send_comp_name, send_grid_name,
                               recv_comp_name, recv_grid_name, map_tag);
}

inline exchange_class* data_class::get_my_exchange() const
{ return data_class_get_my_exchange(this); }

inline void data_class::get_my_name(char* out) const { data_class_get_my_name(this, out); }
inline void data_class::get_send_comp_name(char* out) const { data_class_get_send_comp_name(this, out); }
inline void data_class::get_send_grid_name(char* out) const { data_class_get_send_grid_name(this, out); }
inline void data_class::get_send_data_name(char* out) const { data_class_get_send_data_name(this, out); }
inline void data_class::get_recv_comp_name(char* out) const { data_class_get_recv_comp_name(this, out); }
inline void data_class::get_recv_grid_name(char* out) const { data_class_get_recv_grid_name(this, out); }
inline void data_class::get_recv_data_name(char* out) const { data_class_get_recv_data_name(this, out); }

inline int data_class::is_avr() const { return data_class_is_avr(this); }
inline int data_class::get_intvl() const { return data_class_get_intvl(this); }
inline int data_class::get_time_lag() const { return data_class_get_time_lag(this); }
inline int data_class::get_exchange_type() const { return data_class_get_exchange_type(this); }
inline int data_class::get_num_of_layer() const { return data_class_get_num_of_layer(this); }
inline int data_class::get_exchange_tag() const { return data_class_get_exchange_tag(this); }
inline int data_class::is_my_intpl() const { return data_class_is_my_intpl(this); }

inline void data_class::put_data_1d(const double* data, int n_data,
    int64_t next_sec, int32_t delta_t)
{ data_class_put_data_1d(this, data, n_data, next_sec, delta_t); }
inline void data_class::put_data_2d(const double* data, int n1, int n2,
    int64_t next_sec, int32_t delta_t)
{ data_class_put_data_2d(this, data, n1, n2, next_sec, delta_t); }
inline void data_class::recv_data_1d(int64_t current_sec)
{ data_class_recv_data_1d(this, current_sec); }
inline void data_class::recv_data_2d(int64_t current_sec)
{ data_class_recv_data_2d(this, current_sec); }
inline void data_class::interpolate_data_1d()
{ data_class_interpolate_data_1d(this); }
inline void data_class::interpolate_data_2d()
{ data_class_interpolate_data_2d(this); }
inline void data_class::get_data_1d(double* data, int n_data,
    int64_t current_sec, int* is_get_ok)
{ data_class_get_data_1d(this, data, n_data, current_sec, is_get_ok); }
inline void data_class::get_data_2d(double* data, int n1, int n2,
    int64_t current_sec, int* is_get_ok)
{ data_class_get_data_2d(this, data, n1, n2, current_sec, is_get_ok); }
