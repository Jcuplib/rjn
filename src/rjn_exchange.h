#pragma once
/*
 * rjn_exchange.h
 * Converted from rjn_exchange.f90
 * Copyright (c) 2026, arakawa@climtech.jp
 */

#include "rjn_exchange_class.h"
#include <stdio.h>

/* -----------------------------------------------------------------------
 * Module-level exchange registry management
 * ----------------------------------------------------------------------- */

/* Initialise the exchange registry (must be called once before use) */
void init_exchange(void);

/* Register a new exchange pattern.
 * intpl_flag: 1 = interpolation enabled, 0 = direct copy
 * intpl_mode: interpolation mode (passed through to exchange_class_init)
 * send_grid / recv_grid / coef: arrays of length n
 */
void set_mapping_table(const char* my_name,
                       const char* send_comp_name, const char* send_grid_name,
                       const char* recv_comp_name, const char* recv_grid_name,
                       int map_tag, int intpl_flag, int intpl_mode,
                       const int* send_grid, const int* recv_grid,
                       const double* coef, int n);

/* Return the number of registered exchange patterns */
int get_num_of_exchange(void);

/* Return 1 if an exchange matching the given names and tag is registered */
int is_exchange_assigned(const char* send_comp_name, const char* send_grid_name,
                         const char* recv_comp_name, const char* recv_grid_name,
                         int map_tag);

/* Return a pointer to the exchange_class matching the given names/tag.
 * Calls rjn_error if not found. */
exchange_class* get_exchange_ptr_name(const char* send_comp_name, const char* send_grid_name,
                                      const char* recv_comp_name, const char* recv_grid_name,
                                      int map_tag);

/* Return a pointer to the exchange_class by 1-based index.
 * Calls rjn_error if exchange_num > num_of_exchange. */
exchange_class* get_exchange_ptr_num(int exchange_num);

/* Persist / restore the exchange registry to/from a binary FILE* */
void write_exchange(FILE* fp);
void read_exchange(FILE* fp);
