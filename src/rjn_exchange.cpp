/*
 * rjn_exchange.cpp
 * Converted from rjn_exchange.f90
 * Copyright (c) 2026, arakawa@climtech.jp
 */

#include "rjn_exchange.h"
#include "rjn_utils.h"
#include <string.h>
#include <stdio.h>

/* -----------------------------------------------------------------------
 * Module-level private state
 * ----------------------------------------------------------------------- */
#define MAX_EXCHANGE 8

static int            s_num_of_exchange = 0;
static exchange_class s_exchange[MAX_EXCHANGE];

/* -----------------------------------------------------------------------
 * init_exchange
 * ----------------------------------------------------------------------- */
void init_exchange(void)
{
    s_num_of_exchange = 0;
    /* Nothing else needed: exchange objects are value-initialised on first use */
}

/* -----------------------------------------------------------------------
 * set_mapping_table
 * ----------------------------------------------------------------------- */
void set_mapping_table(const char* my_name,
                       const char* send_comp_name, const char* send_grid_name,
                       const char* recv_comp_name, const char* recv_grid_name,
                       int map_tag, int intpl_flag, int intpl_mode,
                       const int* send_grid, const int* recv_grid,
                       const double* coef, int n)
{
    s_num_of_exchange++;

    if (s_num_of_exchange > MAX_EXCHANGE)
        rjn_error("set_mapping_table",
                  "num_of_exchange exceeded MAX_EXCHANGE. Please increase MAX_EXCHANGE");

    s_exchange[s_num_of_exchange - 1] =
        exchange_class_init(my_name, intpl_flag, intpl_mode);

    exchange_class_set_mapping_table(
        &s_exchange[s_num_of_exchange - 1],
        send_comp_name, send_grid_name,
        recv_comp_name, recv_grid_name,
        map_tag, intpl_flag,
        send_grid, recv_grid, coef, n);
}

/* -----------------------------------------------------------------------
 * get_num_of_exchange
 * ----------------------------------------------------------------------- */
int get_num_of_exchange(void)
{
    return s_num_of_exchange;
}

/* -----------------------------------------------------------------------
 * is_exchange_assigned
 * ----------------------------------------------------------------------- */
int is_exchange_assigned(const char* send_comp_name, const char* send_grid_name,
                         const char* recv_comp_name, const char* recv_grid_name,
                         int map_tag)
{
    int i;
    for (i = 0; i < s_num_of_exchange; i++) {
        if (exchange_class_is_my_exchange(&s_exchange[i],
                                          send_comp_name, send_grid_name,
                                          recv_comp_name, recv_grid_name, map_tag))
            return 1;
    }
    return 0;
}

/* -----------------------------------------------------------------------
 * get_exchange_ptr_name
 * ----------------------------------------------------------------------- */
exchange_class* get_exchange_ptr_name(const char* send_comp_name, const char* send_grid_name,
                                      const char* recv_comp_name, const char* recv_grid_name,
                                      int map_tag)
{
    int i;
    char msg[256];

    for (i = 0; i < s_num_of_exchange; i++) {
        if (exchange_class_is_my_exchange(&s_exchange[i],
                                          send_comp_name, send_grid_name,
                                          recv_comp_name, recv_grid_name, map_tag))
            return &s_exchange[i];
    }

    snprintf(msg, sizeof(msg),
             "no such name: %s:%s:%s:%s",
             send_comp_name, send_grid_name, recv_comp_name, recv_grid_name);
    rjn_error("[rjn_exchange:get_exchange_ptr]", msg);
    return NULL;
}

/* -----------------------------------------------------------------------
 * get_exchange_ptr_num (exchange_num is 1-based)
 * ----------------------------------------------------------------------- */
exchange_class* get_exchange_ptr_num(int exchange_num)
{
    if (exchange_num > s_num_of_exchange)
        rjn_error("[rjn_exchange:get_exchange_ptr]",
                  "exchange num exceeded the defined exchange pattern");

    return &s_exchange[exchange_num - 1];
}

/* -----------------------------------------------------------------------
 * write_exchange
 * ----------------------------------------------------------------------- */
void write_exchange(FILE* fp)
{
    int i;
    fwrite(&s_num_of_exchange, sizeof(int), 1, fp);
    for (i = 0; i < s_num_of_exchange; i++)
        exchange_class_write(&s_exchange[i], fp);
}

/* -----------------------------------------------------------------------
 * read_exchange
 * ----------------------------------------------------------------------- */
void read_exchange(FILE* fp)
{
    int i;
    fread(&s_num_of_exchange, sizeof(int), 1, fp);
    for (i = 0; i < s_num_of_exchange; i++)
        exchange_class_read(&s_exchange[i], fp);
}
