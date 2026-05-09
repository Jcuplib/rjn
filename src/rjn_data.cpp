/*
 * rjn_data.cpp
 * Converted from rjn_data.f90
 * Copyright (c) 2011, arakawa@rist.jp
 */

#include "rjn_data.h"
#include "rjn_exchange.h"
#include "rjn_utils.h"
#include "rjn_constant.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* -----------------------------------------------------------------------
 * Module-level private state
 * ----------------------------------------------------------------------- */
#define INITIAL_DATA_CAPACITY 4

static int         s_num_of_send_data = 0;
static int         s_send_capacity    = 0;
static data_class* s_send_data        = NULL;

static int         s_num_of_recv_data = 0;
static int         s_recv_capacity    = 0;
static data_class* s_recv_data        = NULL;

static char s_my_comp[STR_SHORT] = {0};

/* -----------------------------------------------------------------------
 * ensure_capacity
 * Doubles the array capacity when needed (pointer-based, not Fortran
 * move_alloc).
 * ----------------------------------------------------------------------- */
static void ensure_capacity(data_class** arr, int* capacity, int needed)
{
    if (needed < *capacity) return;

    int new_cap = (*capacity > 0) ? (*capacity * 2) : INITIAL_DATA_CAPACITY;
    while (new_cap <= needed) new_cap *= 2;

    data_class* tmp = (data_class*)realloc(*arr, new_cap * sizeof(data_class));
    if (!tmp) rjn_error("ensure_capacity", "realloc failed");
    *arr      = tmp;
    *capacity = new_cap;
}

/* -----------------------------------------------------------------------
 * init_data
 * ----------------------------------------------------------------------- */
void init_data(const char* my_comp_name)
{
    strncpy(s_my_comp, my_comp_name, STR_SHORT - 1);
    s_my_comp[STR_SHORT - 1] = '\0';

    s_num_of_send_data = 0;
    s_send_capacity    = INITIAL_DATA_CAPACITY;
    s_send_data        = (data_class*)malloc(s_send_capacity * sizeof(data_class));

    s_num_of_recv_data = 0;
    s_recv_capacity    = INITIAL_DATA_CAPACITY;
    s_recv_data        = (data_class*)malloc(s_recv_capacity * sizeof(data_class));
}

/* -----------------------------------------------------------------------
 * Internal helpers: set_send_data / set_recv_data
 * ----------------------------------------------------------------------- */
static void set_send_data(
    const char* send_comp, const char* send_grid, const char* send_data_name,
    const char* recv_comp, const char* recv_grid, const char* recv_data_name,
    int map_tag, int is_avr, int intvl, int time_lag, int exchange_type,
    int num_of_layer, int grid_intpl_tag, double fill_value, int exchange_tag)
{
    put_log("rjn_set_send_data", 2);
    put_log("  send_comp = ", 2); /* simplified; Fortran appended the string */

    ensure_capacity(&s_send_data, &s_send_capacity, s_num_of_send_data);

    s_send_data[s_num_of_send_data] = data_class_init(
        send_comp, send_grid, send_data_name,
        recv_comp, recv_grid, recv_data_name,
        is_avr, intvl, time_lag, exchange_type,
        num_of_layer, grid_intpl_tag,
        fill_value, exchange_tag,
        1.0, 0.0);

    if (is_exchange_assigned(send_comp, send_grid, recv_comp, recv_grid, map_tag)) {
        data_class_set_my_exchange(&s_send_data[s_num_of_send_data],
                                   send_comp, send_grid,
                                   recv_comp, recv_grid, map_tag);
    }

    s_num_of_send_data++;
}

static void set_recv_data(
    const char* send_comp, const char* send_grid, const char* send_data_name,
    const char* recv_comp, const char* recv_grid, const char* recv_data_name,
    int map_tag, int is_avr, int intvl, int time_lag, int exchange_type,
    int num_of_layer, int grid_intpl_tag, double fill_value, int exchange_tag)
{
    put_log("rjn_set_recv_data", 2);

    ensure_capacity(&s_recv_data, &s_recv_capacity, s_num_of_recv_data);

    s_recv_data[s_num_of_recv_data] = data_class_init(
        send_comp, send_grid, send_data_name,
        recv_comp, recv_grid, recv_data_name,
        is_avr, intvl, time_lag, exchange_type,
        num_of_layer, grid_intpl_tag,
        fill_value, exchange_tag,
        1.0, 0.0);

    if (is_exchange_assigned(send_comp, send_grid, recv_comp, recv_grid, map_tag)) {
        data_class_set_my_exchange(&s_recv_data[s_num_of_recv_data],
                                   send_comp, send_grid,
                                   recv_comp, recv_grid, map_tag);
    }

    s_num_of_recv_data++;
}

/* -----------------------------------------------------------------------
 * set_data
 * ----------------------------------------------------------------------- */
void set_data(const char* send_comp, const char* send_grid, const char* send_data_name,
              const char* recv_comp, const char* recv_grid, const char* recv_data_name,
              int map_tag, int is_avr, int intvl, int time_lag, int num_of_layer,
              int grid_intpl_tag, double fill_value, int exchange_tag)
{
    int exchange_type = CONCURRENT_SEND_RECV;

    if (time_lag == 0)
        exchange_type = IMMEDIATE_SEND_RECV;

    if (strcmp(s_my_comp, send_comp) == 0) {
        if (time_lag == 1)
            exchange_type = ADVANCE_SEND_RECV;
        set_send_data(send_comp, send_grid, send_data_name,
                      recv_comp, recv_grid, recv_data_name,
                      map_tag, is_avr, intvl, time_lag, exchange_type,
                      num_of_layer, grid_intpl_tag, fill_value, exchange_tag);
    } else {
        if (time_lag == 1)
            exchange_type = BEHIND_SEND_RECV;
        set_recv_data(send_comp, send_grid, send_data_name,
                      recv_comp, recv_grid, recv_data_name,
                      map_tag, is_avr, intvl, time_lag, exchange_type,
                      num_of_layer, grid_intpl_tag, fill_value, exchange_tag);
    }
}

/* -----------------------------------------------------------------------
 * get_num_of_send_data / get_num_of_recv_data
 * ----------------------------------------------------------------------- */
int get_num_of_send_data(void) { return s_num_of_send_data; }
int get_num_of_recv_data(void) { return s_num_of_recv_data; }

/* -----------------------------------------------------------------------
 * is_my_send_data / is_my_recv_data  (data_num is 1-based)
 * ----------------------------------------------------------------------- */
int is_my_send_data(int data_num,
                    const char* send_comp, const char* send_grid,
                    const char* recv_comp, const char* recv_grid)
{
    data_class* d = &s_send_data[data_num - 1];
    char sc[STR_SHORT], sg[STR_SHORT], rc[STR_SHORT], rg[STR_SHORT];
    data_class_get_send_comp_name(d, sc);
    data_class_get_send_grid_name(d, sg);
    data_class_get_recv_comp_name(d, rc);
    data_class_get_recv_grid_name(d, rg);
    return (strcmp(sc, send_comp) == 0 && strcmp(sg, send_grid) == 0 &&
            strcmp(rc, recv_comp) == 0 && strcmp(rg, recv_grid) == 0) ? 1 : 0;
}

int is_my_recv_data(int data_num,
                    const char* send_comp, const char* send_grid,
                    const char* recv_comp, const char* recv_grid)
{
    data_class* d = &s_recv_data[data_num - 1];
    char sc[STR_SHORT], sg[STR_SHORT], rc[STR_SHORT], rg[STR_SHORT];
    data_class_get_send_comp_name(d, sc);
    data_class_get_send_grid_name(d, sg);
    data_class_get_recv_comp_name(d, rc);
    data_class_get_recv_grid_name(d, rg);
    return (strcmp(sc, send_comp) == 0 && strcmp(sg, send_grid) == 0 &&
            strcmp(rc, recv_comp) == 0 && strcmp(rg, recv_grid) == 0) ? 1 : 0;
}

/* -----------------------------------------------------------------------
 * set_my_send_exchange / set_my_recv_exchange  (data_num is 1-based)
 * ----------------------------------------------------------------------- */
void set_my_send_exchange(int data_num,
                          const char* send_comp, const char* send_grid,
                          const char* recv_comp, const char* recv_grid,
                          int map_tag)
{
    data_class_set_my_exchange(&s_send_data[data_num - 1],
                               send_comp, send_grid,
                               recv_comp, recv_grid, map_tag);
}

void set_my_recv_exchange(int data_num,
                          const char* send_comp, const char* send_grid,
                          const char* recv_comp, const char* recv_grid,
                          int map_tag)
{
    data_class_set_my_exchange(&s_recv_data[data_num - 1],
                               send_comp, send_grid,
                               recv_comp, recv_grid, map_tag);
}

/* -----------------------------------------------------------------------
 * get_send_data_name / get_recv_data_name  (data_num is 1-based)
 * ----------------------------------------------------------------------- */
void get_send_data_name(int data_num, char* out)
{
    data_class_get_my_name(&s_send_data[data_num - 1], out);
}

void get_recv_data_name(int data_num, char* out)
{
    data_class_get_my_name(&s_recv_data[data_num - 1], out);
}

/* -----------------------------------------------------------------------
 * get_send_data_intvl / get_recv_data_intvl  (data_num is 1-based)
 * ----------------------------------------------------------------------- */
int get_send_data_intvl(int data_num)
{
    return data_class_get_intvl(&s_send_data[data_num - 1]);
}

int get_recv_data_intvl(int data_num)
{
    return data_class_get_intvl(&s_recv_data[data_num - 1]);
}

/* -----------------------------------------------------------------------
 * Internal helpers: get_send_data_ptr / get_recv_data_ptr
 * ----------------------------------------------------------------------- */
static data_class* get_send_data_ptr(const char* data_name)
{
    int i;
    char name[STR_SHORT];
    for (i = 0; i < s_num_of_send_data; i++) {
        data_class_get_my_name(&s_send_data[i], name);
        if (strcmp(name, data_name) == 0)
            return &s_send_data[i];
    }
    {
        char msg[256];
        snprintf(msg, sizeof(msg), "no such data, data name = %s", data_name);
        rjn_error("[rjn_data:get_send_data_ptr]", msg);
    }
    return NULL;
}

static data_class* get_recv_data_ptr(const char* data_name)
{
    int i;
    char name[STR_SHORT];
    for (i = 0; i < s_num_of_recv_data; i++) {
        data_class_get_my_name(&s_recv_data[i], name);
        if (strcmp(name, data_name) == 0)
            return &s_recv_data[i];
    }
    {
        char msg[256];
        snprintf(msg, sizeof(msg), "no such data, data name = %s", data_name);
        rjn_error("[rjn_data:get_recv_data_ptr]", msg);
    }
    return NULL;
}

/* -----------------------------------------------------------------------
 * put_data_1d
 * ----------------------------------------------------------------------- */
void put_data_1d(const char* data_name,
                 const double* data, int n_data,
                 int64_t next_sec, int32_t delta_t)
{
    data_class* dp = get_send_data_ptr(data_name);
    if (data_class_get_num_of_layer(dp) > 1)
        rjn_error("[rjn_data:put_data_1d]", "data dimension mismatch");
    data_class_put_data_1d(dp, data, n_data, next_sec, delta_t);
}

/* -----------------------------------------------------------------------
 * put_data_2d
 * ----------------------------------------------------------------------- */
void put_data_2d(const char* data_name,
                 const double* data, int n1, int n2,
                 int64_t next_sec, int32_t delta_t)
{
    data_class* dp = get_send_data_ptr(data_name);
    if (data_class_get_num_of_layer(dp) <= 1)
        rjn_error("[rjn_data:put_data_2d]", "data dimension mismatch");
    data_class_put_data_2d(dp, data, n1, n2, next_sec, delta_t);
}

/* -----------------------------------------------------------------------
 * recv_my_data
 * ----------------------------------------------------------------------- */
void recv_my_data(const char* data_name, int64_t current_sec)
{
    data_class* dp = get_recv_data_ptr(data_name);
    if (data_class_get_num_of_layer(dp) == 1)
        data_class_recv_data_1d(dp, current_sec);
    else
        data_class_recv_data_2d(dp, current_sec);
}

/* -----------------------------------------------------------------------
 * interpolate_recv_data
 * ----------------------------------------------------------------------- */
void interpolate_recv_data(const char* data_name, int64_t current_sec)
{
    data_class* dp = get_recv_data_ptr(data_name);
    int intvl;

    if (data_class_get_time_lag(dp) == 0) return;

    intvl = data_class_get_intvl(dp);
    if (current_sec % (int64_t)intvl == 0) {
        if (data_class_get_num_of_layer(dp) == 1)
            data_class_interpolate_data_1d(dp);
        else
            data_class_interpolate_data_2d(dp);
    }
}

/* -----------------------------------------------------------------------
 * get_data_1d
 * ----------------------------------------------------------------------- */
void get_data_1d(const char* data_name,
                 double* data, int n_data,
                 int64_t current_sec, int* is_get_ok)
{
    data_class* dp = get_recv_data_ptr(data_name);
    if (data_class_get_num_of_layer(dp) > 1)
        rjn_error("[rjn_data:get_data_1d]", "data dimension mismatch");
    data_class_get_data_1d(dp, data, n_data, current_sec, is_get_ok);
}

/* -----------------------------------------------------------------------
 * get_data_2d
 * ----------------------------------------------------------------------- */
void get_data_2d(const char* data_name,
                 double* data, int n1, int n2,
                 int64_t current_sec, int* is_get_ok)
{
    data_class* dp = get_recv_data_ptr(data_name);
    if (data_class_get_num_of_layer(dp) <= 1)
        rjn_error("[rjn_data:get_data_2d]", "data dimension mismatch");
    data_class_get_data_2d(dp, data, n1, n2, current_sec, is_get_ok);
}
