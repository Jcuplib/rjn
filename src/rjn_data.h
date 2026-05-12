#pragma once
/*
 * rjn_data.h
 * Converted from rjn_data.f90
 * Copyright (c) 2026, arakawa@climtech.jp
 */

#include <stdint.h>
#include "rjn_constant.h"
#include "rjn_data_class.h"

/* -----------------------------------------------------------------------
 * Module-level data registry management
 * ----------------------------------------------------------------------- */

/* Initialise the data registry; must be called before any other function.
 * my_comp_name: name of this component (used to determine send vs recv). */
void init_data(const char* my_comp_name);

/* Register a data exchange entry (automatically classified as send or recv).
 * is_avr   : 1 = average mode, 0 = snapshot
 * intvl    : exchange interval in seconds (<=0 = every step)
 * time_lag : 0 = IMMEDIATE, 1 = ADVANCE (sender) / BEHIND (receiver)
 * num_of_layer : 1 for 1D data, >1 for 2D layered data
 */
void set_data(const char* send_comp, const char* send_grid, const char* send_data_name,
              const char* recv_comp, const char* recv_grid, const char* recv_data_name,
              int map_tag, int is_avr, int intvl, int time_lag, int num_of_layer,
              int grid_intpl_tag, double fill_value, int exchange_tag);

/* Return the number of registered send / recv data entries */
int get_num_of_send_data(void);
int get_num_of_recv_data(void);

/* Return 1 if the data entry at data_num (1-based) matches the given names */
int is_my_send_data(int data_num,
                    const char* send_comp, const char* send_grid,
                    const char* recv_comp, const char* recv_grid);

int is_my_recv_data(int data_num,
                    const char* send_comp, const char* send_grid,
                    const char* recv_comp, const char* recv_grid);

/* Link the exchange object to the data entry (data_num is 1-based) */
void set_my_send_exchange(int data_num,
                          const char* send_comp, const char* send_grid,
                          const char* recv_comp, const char* recv_grid,
                          int map_tag);

void set_my_recv_exchange(int data_num,
                          const char* send_comp, const char* send_grid,
                          const char* recv_comp, const char* recv_grid,
                          int map_tag);

/* Return the my_name of a send/recv data entry (1-based).
 * out must point to a buffer of at least STR_SHORT bytes. */
void get_send_data_name(int data_num, char* out);
void get_recv_data_name(int data_num, char* out);

/* Return exchange interval (seconds) for a send/recv entry (1-based) */
int get_send_data_intvl(int data_num);
int get_recv_data_intvl(int data_num);

/* -----------------------------------------------------------------------
 * Send/recv operations (look up by data_name)
 * ----------------------------------------------------------------------- */

/* Send 1D data (num_of_layer must equal 1 for this entry) */
void put_data_1d(const char* data_name,
                 const double* data, int n_data,
                 int64_t next_sec, int32_t delta_t);

/* Send 2D data (num_of_layer > 1 for this entry);
 * data is [n1 * n2] row-major (n2 = num_of_layer) */
void put_data_2d(const char* data_name,
                 const double* data, int n1, int n2,
                 int64_t next_sec, int32_t delta_t);

/* Post non-blocking receive for named data entry */
void recv_my_data(const char* data_name, int64_t current_sec);

/* Complete interpolation of received data (no-op when time_lag == 0) */
void interpolate_recv_data(const char* data_name, int64_t current_sec);

/* Copy received/interpolated data into caller buffer (1D case) */
void get_data_1d(const char* data_name,
                 double* data, int n_data,
                 int64_t current_sec, int* is_get_ok);

/* Copy received/interpolated data into caller buffer (2D case) */
void get_data_2d(const char* data_name,
                 double* data, int n1, int n2,
                 int64_t current_sec, int* is_get_ok);
