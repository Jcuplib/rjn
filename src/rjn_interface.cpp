/*
 * rjn_interface.cpp
 * Converted from rjn_interface.f90
 * Copyright (c) 2011, arakawa@rist.jp
 */

#include "rjn_interface.h"
#include "rjn_comp.h"
#include "rjn_data.h"
#include "rjn_exchange.h"
#include "rjn_exchange_class.h"
#include "rjn_grid.h"
#include "rjn_mpi_lib.h"
#include "rjn_remapping.h"
#include "rjn_time.h"
#include "rjn_utils.h"
#include "rjn_constant.h"
#include <mpi.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

int NUM_OF_EXCHANGE_DATA = 40; /* default; may be overridden before rjn_initialize */

/* -----------------------------------------------------------------------
 * Module-level private state
 * ----------------------------------------------------------------------- */
static char     s_my_comp_name[STR_SHORT] = {0};
static int      s_my_comp_id   = -1;
static int      s_my_comm      = -1;
static int      s_my_group     = -1;
static int      s_my_size      = -1;
static int      s_my_rank      = -1;
static int      s_max_num_of_exchange_data = -1;

static int64_t  s_current_sec    = 0;
static int64_t  s_next_sec       = 0;
static int32_t  s_current_delta_t = 0;

/* -----------------------------------------------------------------------
 * rjn_set_world / rjn_get_world  (re-exported from rjn_mpi_lib)
 * ----------------------------------------------------------------------- */
void rjn_set_world(int global_comm)  { jml_set_global_comm(global_comm); }
int  rjn_get_world(void)             { return jml_get_global_comm(); }

/* -----------------------------------------------------------------------
 * rjn_set_new_comp
 * ----------------------------------------------------------------------- */
void rjn_set_new_comp(const char* component_name)
{
    strncpy(s_my_comp_name, component_name, STR_SHORT - 1);
    s_my_comp_name[STR_SHORT - 1] = '\0';
    set_my_component(component_name);
}

/* -----------------------------------------------------------------------
 * rjn_initialize
 * ----------------------------------------------------------------------- */
void rjn_initialize(const char* model_name,
                    const char* default_time_unit,
                    int log_level,
                    int log_stderr)
{
    int num_of_comp, mdl;
    int opt_log_level, opt_log_stderr;
    int my_time_unit, tu_min, tu_max;

    if (s_my_comp_name[0] == '\0') {
        strncpy(s_my_comp_name, model_name, STR_SHORT - 1);
        rjn_set_new_comp(model_name);
    }

    init_model_process();

    /* Set time unit */
    if (default_time_unit != NULL) {
        if (strcmp(default_time_unit, "SEC") == 0)
            set_time_unit(TU_SEC);
        else if (strcmp(default_time_unit, "MIL") == 0)
            set_time_unit(TU_MIL);
        else if (strcmp(default_time_unit, "MCR") == 0)
            set_time_unit(TU_MCR);
        else {
            fprintf(stderr, "rjn_initialize, default_time_unit setting error: %s\n",
                    default_time_unit);
            jml_abort();
        }
    } else {
        set_time_unit(TU_SEC);
    }

    /* Verify all components use the same time unit */
    my_time_unit = get_time_unit();
    jml_AllreduceMin(my_time_unit, &tu_min);
    jml_AllreduceMax(my_time_unit, &tu_max);
    if (tu_min != tu_max) {
        fprintf(stderr, "rjn_initialize: time unit must be same in all components\n");
        jml_abort();
    }

    rjn_get_mpi_parameter(model_name, &s_my_comm, &s_my_group, &s_my_size, &s_my_rank);

    num_of_comp = get_num_of_total_component();
    s_max_num_of_exchange_data = NUM_OF_EXCHANGE_DATA;

    init_all_time(num_of_comp);
    for (mdl = 1; mdl <= num_of_comp; mdl++)
        init_each_time(mdl, 1);

    opt_log_level  = (log_level  >= 0) ? log_level  : 0;
    opt_log_stderr = (log_stderr >= 0) ? log_stderr : 0;

    set_log_level(opt_log_level, opt_log_stderr);
    init_log(model_name, NULL);

    s_my_comp_id = get_comp_id_from_name(model_name);
    init_grid(s_my_comp_id, s_my_rank, s_my_size);
    init_exchange();

    put_log("coupler initialization completed", 1);

    {
        int m;
        for (m = 1; m <= get_num_of_total_component(); m++) {
            if (is_my_component_id(m)) {
                char cname[STR_SHORT];
                char idstr[32];
                char msg[STR_MID];
                get_component_name(m, cname);
                snprintf(idstr, sizeof(idstr), "%d", m);
                snprintf(msg, STR_MID, "assigned component name : %s, comp_id = %s",
                         cname, idstr);
                put_log(msg, -1);
            }
        }
    }

    init_data(model_name);
    init_remapping(s_my_comm);
}

/* -----------------------------------------------------------------------
 * rjn_coupling_end
 * ----------------------------------------------------------------------- */
void rjn_coupling_end(const int* time_array, int is_call_finalize)
{
    int i;
    int ngrid = get_num_of_my_grid();

    put_log("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! ", 1);
    put_log("!!!!!!!!!!!!!   COUPLER FINALIZE  START !!!!!!!!!!!! ", 1);
    put_log("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! ", 1);

    for (i = 1; i <= ngrid; i++) {
        char gname[STR_SHORT];
        get_grid_name(i, gname);
        delete_grid_rank_file(s_my_comp_name, gname);
    }

    jml_finalize(is_call_finalize);

    put_log("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! ", 1);
    put_log("!!!!!!!!!!!!!!!!  COUPLING COMPLETED !!!!!!!!!!!!!!! ", 1);
    put_log("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! ", 1);

    finalize_log();
}

/* -----------------------------------------------------------------------
 * rjn_get_mpi_parameter
 * ----------------------------------------------------------------------- */
void rjn_get_mpi_parameter(const char* comp_name,
                           int* my_comm, int* my_group,
                           int* my_size, int* my_rank)
{
    int comp_id;

    if (strcmp(comp_name, "GLOBAL") == 0) {
        *my_comm  = jml_GetCommGlobal();
        *my_group = 0;
        *my_size  = jml_GetCommSizeGlobal();
        *my_rank  = jml_GetMyrankGlobal();
        return;
    }

    comp_id = get_comp_id_from_name(comp_name);

    *my_comm  = is_my_component_id(comp_id) ?
                jml_GetComm(comp_id) : jml_GetCommNULL();
    *my_group = jml_GetMyGroup(comp_id);
    *my_size  = jml_GetCommSizeLocal(comp_id);
    *my_rank  = jml_GetMyrank(comp_id);
}

/* -----------------------------------------------------------------------
 * rjn_get_myrank_global
 * ----------------------------------------------------------------------- */
int rjn_get_myrank_global(void)
{
    return jml_GetMyrankGlobal();
}

/* -----------------------------------------------------------------------
 * Component queries
 * ----------------------------------------------------------------------- */
int rjn_get_num_of_component(void)               { return get_num_of_total_component(); }
void rjn_get_component_name(int comp_id, char* out) { get_component_name(comp_id, out); }
int rjn_get_comp_num_from_name(const char* n)    { return get_comp_id_from_name(n); }
int rjn_is_my_component_id(int comp_id)          { return is_my_component_id(comp_id); }
int rjn_is_my_component_name(const char* n)      { return is_my_component_name(n); }
int rjn_is_model_running(const char* n)          { return is_model_running(n); }

/* -----------------------------------------------------------------------
 * rjn_def_grid
 * ----------------------------------------------------------------------- */
void rjn_def_grid(const int* grid_index, int n,
                  const char* model_name, const char* grid_name,
                  int num_of_vgrid)
{
    int i;
    char msg[STR_MID];

    if (!is_my_component_name(model_name)) {
        snprintf(msg, STR_MID, "Component name : %s is not defined", model_name);
        rjn_error("rjn_def_grid", msg);
    }

    for (i = 0; i < n; i++) {
        if (grid_index[i] <= 0)
            rjn_error("rjn_def_grid", "grid index must be >= 1");
    }

    if (num_of_vgrid > 0 && num_of_vgrid > s_max_num_of_exchange_data)
        s_max_num_of_exchange_data = num_of_vgrid;

    def_grid(grid_index, n, model_name, grid_name);

    {
        int mn = grid_index[0], mx = grid_index[0];
        for (i = 1; i < n; i++) {
            if (grid_index[i] < mn) mn = grid_index[i];
            if (grid_index[i] > mx) mx = grid_index[i];
        }
        snprintf(msg, STR_MID,
                 "rjn_def_grid : component name : %s, grid name : %s, grid size : %d, min : %d, max : %d",
                 model_name, grid_name, n, mn, mx);
        put_log(msg, -1);
    }

    put_log("rjn_def_grid : make_grid_rank_file start", -1);
    make_grid_rank_file(model_name, grid_name, grid_index, n);
    put_log("rjn_def_grid : make_grid_rank_file finish", -1);
}

/* -----------------------------------------------------------------------
 * rjn_end_grid_def  — MPI barrier
 * ----------------------------------------------------------------------- */
void rjn_end_grid_def(void)
{
    MPI_Barrier(MPI_COMM_WORLD);
}

/* -----------------------------------------------------------------------
 * Variable pointer helpers
 * ----------------------------------------------------------------------- */
rjn_varp_type* rjn_def_varp(const char* comp_name,
                             const char* data_name,
                             const char* grid_name,
                             int num_of_layer)
{
    rjn_varp_type* p = (rjn_varp_type*)malloc(sizeof(rjn_varp_type));
    strncpy(p->comp_name, comp_name, STR_SHORT - 1);
    strncpy(p->grid_name, grid_name, STR_SHORT - 1);
    strncpy(p->data_name, data_name, STR_SHORT - 1);
    (void)num_of_layer;
    return p;
}

rjn_varg_type* rjn_def_varg(const char* comp_name,
                             const char* data_name,
                             const char* grid_name)
{
    rjn_varg_type* p = (rjn_varg_type*)malloc(sizeof(rjn_varg_type));
    strncpy(p->comp_name, comp_name, STR_SHORT - 1);
    strncpy(p->grid_name, grid_name, STR_SHORT - 1);
    strncpy(p->data_name, data_name, STR_SHORT - 1);
    return p;
}

void rjn_end_var_def(void) {}  /* no-op */

/* -----------------------------------------------------------------------
 * Fill-value stubs
 * ----------------------------------------------------------------------- */
void   rjn_set_fill_value(double fill_value) { (void)fill_value; }
double rjn_get_fill_value(void)              { return -9999.0; }

/* -----------------------------------------------------------------------
 * rjn_set_mapping_table
 * ----------------------------------------------------------------------- */
void rjn_set_mapping_table(const char* my_model_name,
                            const char* send_model_name, const char* send_grid_name,
                            const char* recv_model_name, const char* recv_grid_name,
                            int map_tag, int is_recv_intpl,
                            const char* intpl_mode,
                            const int* send_grid, const int* recv_grid,
                            const double* coef, int n)
{
    int is_my_intpl;
    int intpl_mode_int;
    int my_model_id, recv_model_id;
    int i;
    char msg[STR_MID];

    snprintf(msg, STR_MID, "set mapping table start : %s:%s, grid = %s:%s",
             send_model_name, recv_model_name, send_grid_name, recv_grid_name);
    put_log(msg, 1);

    my_model_id  = get_comp_id_from_name(my_model_name);
    recv_model_id = get_comp_id_from_name(recv_model_name);

    is_my_intpl = (my_model_id == recv_model_id) ? is_recv_intpl : !is_recv_intpl;

    if (strcmp(intpl_mode, "FAST") == 0 || strcmp(intpl_mode, "fast") == 0 ||
        strcmp(intpl_mode, "Fast") == 0)
        intpl_mode_int = INTPL_SERIAL_FAST;
    else if (strcmp(intpl_mode, "SAFE") == 0 || strcmp(intpl_mode, "safe") == 0 ||
             strcmp(intpl_mode, "Safe") == 0)
        intpl_mode_int = INTPL_SERIAL_SAFE;
    else if (strcmp(intpl_mode, "PARALLEL") == 0 || strcmp(intpl_mode, "parallel") == 0 ||
             strcmp(intpl_mode, "Parallel") == 0)
        intpl_mode_int = INTPL_PARALLEL;
    else if (strcmp(intpl_mode, "USER") == 0 || strcmp(intpl_mode, "user") == 0 ||
             strcmp(intpl_mode, "User") == 0)
        intpl_mode_int = INTPL_USER;
    else
        rjn_error("rjn_set_mapping_table", "intpl_mode must be FAST or SAFE or PARALLEL or USER");

    set_mapping_table(my_model_name, send_model_name, send_grid_name,
                      recv_model_name, recv_grid_name,
                      map_tag, is_my_intpl, intpl_mode_int,
                      send_grid, recv_grid, coef, n);

    snprintf(msg, STR_MID, "set mapping table end : %s:%s",
             send_model_name, recv_model_name);
    put_log(msg, 1);

    for (i = 1; i <= get_num_of_send_data(); i++) {
        if (is_my_send_data(i, send_model_name, send_grid_name,
                             recv_model_name, recv_grid_name))
            set_my_send_exchange(i, send_model_name, send_grid_name,
                                 recv_model_name, recv_grid_name, map_tag);
    }

    for (i = 1; i <= get_num_of_recv_data(); i++) {
        if (is_my_recv_data(i, send_model_name, send_grid_name,
                             recv_model_name, recv_grid_name))
            set_my_recv_exchange(i, send_model_name, send_grid_name,
                                 recv_model_name, recv_grid_name, map_tag);
    }
}

/* -----------------------------------------------------------------------
 * rjn_set_user_interpolation
 * ----------------------------------------------------------------------- */
void rjn_set_user_interpolation(const char* send_comp, const char* send_grid,
                                const char* recv_comp, const char* recv_grid,
                                int map_tag,
                                interpolation_user_ifc user_func)
{
    exchange_class* ep = get_exchange_ptr_name(send_comp, send_grid,
                                              recv_comp, recv_grid, map_tag);
    exchange_class_set_user_interpolation(ep, user_func);
}

/* -----------------------------------------------------------------------
 * rjn_write_mapping_table / rjn_read_mapping_table
 * ----------------------------------------------------------------------- */
void rjn_write_mapping_table(FILE* fp)
{
    write_exchange(fp);
}

void rjn_read_mapping_table(FILE* fp)
{
    int i, j;

    read_exchange(fp);

    for (i = 1; i <= get_num_of_exchange(); i++) {
        exchange_class* ep = get_exchange_ptr_num(i);
        char send_model_name[STR_SHORT], send_grid_name[STR_SHORT];
        char recv_model_name[STR_SHORT], recv_grid_name[STR_SHORT];
        int  map_tag;

        exchange_class_get_send_comp_name(ep, send_model_name);
        exchange_class_get_send_grid_name(ep, send_grid_name);
        exchange_class_get_recv_comp_name(ep, recv_model_name);
        exchange_class_get_recv_grid_name(ep, recv_grid_name);
        map_tag = exchange_class_get_map_tag(ep);

        for (j = 1; j <= get_num_of_send_data(); j++) {
            if (is_my_send_data(j, send_model_name, send_grid_name,
                                 recv_model_name, recv_grid_name))
                set_my_send_exchange(j, send_model_name, send_grid_name,
                                     recv_model_name, recv_grid_name, map_tag);
        }

        for (j = 1; j <= get_num_of_recv_data(); j++) {
            if (is_my_recv_data(j, send_model_name, send_grid_name,
                                 recv_model_name, recv_grid_name))
                set_my_recv_exchange(j, send_model_name, send_grid_name,
                                     recv_model_name, recv_grid_name, map_tag);
        }
    }
}

/* -----------------------------------------------------------------------
 * rjn_set_data (thin re-export)
 * ----------------------------------------------------------------------- */
void rjn_set_data(const char* send_comp, const char* send_grid, const char* send_data_name,
                  const char* recv_comp, const char* recv_grid, const char* recv_data_name,
                  int map_tag, int is_avr, int intvl, int time_lag, int num_of_layer,
                  int grid_intpl_tag, double fill_value, int exchange_tag)
{
    set_data(send_comp, send_grid, send_data_name,
             recv_comp, recv_grid, recv_data_name,
             map_tag, is_avr, intvl, time_lag, num_of_layer,
             grid_intpl_tag, fill_value, exchange_tag);
}

/* -----------------------------------------------------------------------
 * rjn_init_time
 * ----------------------------------------------------------------------- */
void rjn_init_time(const int* time_array)
{
    (void)time_array;
    s_current_sec    = 0;
    s_next_sec       = 0;
    s_current_delta_t = 0;
}

/* -----------------------------------------------------------------------
 * rjn_set_time
 * ----------------------------------------------------------------------- */
void rjn_set_time(const char* comp_name,
                  const int* current_time, int delta_t)
{
    int i;
    int num_recv;
    char log_str[STR_MID];
    char dname[STR_SHORT];

    (void)comp_name;
    (void)current_time;

    s_current_sec     = s_next_sec;
    s_next_sec        = s_current_sec + (int64_t)delta_t;
    s_current_delta_t = (int32_t)delta_t;

    snprintf(log_str, STR_MID,
             "  [rjn_set_time] set time START, current_sec = %ld, next_sec = %ld, delta_t = %d",
             (long)s_current_sec, (long)s_next_sec, delta_t);
    put_log(log_str, -1);

    num_recv = get_num_of_recv_data();
    for (i = 1; i <= num_recv; i++) {
        get_recv_data_name(i, dname);
        recv_my_data(dname, s_current_sec);
    }

    jml_send_waitall();
    jml_recv_waitall();

    for (i = 1; i <= num_recv; i++) {
        get_recv_data_name(i, dname);
        interpolate_recv_data(dname, s_current_sec);
    }

    put_log("  [rjn_set_time] set time END", -1);
}

/* -----------------------------------------------------------------------
 * rjn_put_data_1d
 * ----------------------------------------------------------------------- */
void rjn_put_data_1d(const char* data_name, const double* data, int n_data)
{
    char log_str[STR_MID];
    snprintf(log_str, STR_MID,
             "  [rjn_put_data_1d ] put data START , data_name = %s", data_name);
    put_log(log_str, -1);

    put_data_1d(data_name, data, n_data, s_next_sec, s_current_delta_t);

    snprintf(log_str, STR_MID,
             "  [rjn_put_data_1d ] put data END , data_name = %s", data_name);
    put_log(log_str, -1);
}

void rjn_put_data_1d_ptr(const rjn_varp_type* varp_ptr,
                         const double* data, int n_data)
{
    put_data_1d(varp_ptr->data_name, data, n_data, s_next_sec, s_current_delta_t);
}

/* -----------------------------------------------------------------------
 * rjn_get_data_1d
 * ----------------------------------------------------------------------- */
void rjn_get_data_1d(const char* data_name,
                     double* data, int n_data, int* is_recv_ok)
{
    char log_str[STR_MID];
    snprintf(log_str, STR_MID,
             "  [rjn_get_data_1d ] get data START , data_name = %s", data_name);
    put_log(log_str, -1);

    get_data_1d(data_name, data, n_data, s_current_sec, is_recv_ok);

    snprintf(log_str, STR_MID,
             "  [rjn_get_data_1d ] get data END , data_name = %s", data_name);
    put_log(log_str, -1);
}

void rjn_get_data_1d_ptr(const rjn_varg_type* varg_ptr,
                         double* data, int n_data, int* is_recv_ok)
{
    get_data_1d(varg_ptr->data_name, data, n_data, s_current_sec, is_recv_ok);
}

/* -----------------------------------------------------------------------
 * rjn_put_data_25d
 * ----------------------------------------------------------------------- */
void rjn_put_data_25d(const char* data_name,
                      const double* data, int n1, int n2)
{
    char log_str[STR_MID];
    snprintf(log_str, STR_MID,
             "  [rjn_put_data_25d ] put data START , data_name = %s", data_name);
    put_log(log_str, -1);

    put_data_2d(data_name, data, n1, n2, s_next_sec, s_current_delta_t);

    snprintf(log_str, STR_MID,
             "  [rjn_put_data_25d ] put data END , data_name = %s", data_name);
    put_log(log_str, -1);
}

void rjn_put_data_25d_ptr(const rjn_varp_type* varp_ptr,
                          const double* data, int n1, int n2)
{
    put_data_2d(varp_ptr->data_name, data, n1, n2, s_next_sec, s_current_delta_t);
}

/* -----------------------------------------------------------------------
 * rjn_get_data_25d
 * ----------------------------------------------------------------------- */
void rjn_get_data_25d(const char* data_name,
                      double* data, int n1, int n2, int* is_recv_ok)
{
    char log_str[STR_MID];
    snprintf(log_str, STR_MID,
             "  [rjn_get_data_25d ] get data START , data_name = %s", data_name);
    put_log(log_str, -1);

    get_data_2d(data_name, data, n1, n2, s_current_sec, is_recv_ok);

    snprintf(log_str, STR_MID,
             "  [rjn_get_data_25d ] get data END , data_name = %s", data_name);
    put_log(log_str, -1);
}

void rjn_get_data_25d_ptr(const rjn_varg_type* varg_ptr,
                           double* data, int n1, int n2, int* is_recv_ok)
{
    get_data_2d(varg_ptr->data_name, data, n1, n2, s_current_sec, is_recv_ok);
}

/* -----------------------------------------------------------------------
 * rjn_error_ifc
 * ----------------------------------------------------------------------- */
void rjn_error_ifc(const char* sub_name, const char* error_str)
{
    rjn_error(sub_name, error_str);
}

/* -----------------------------------------------------------------------
 * rjn_log
 * ----------------------------------------------------------------------- */
void rjn_log(const char* sub_name, const char* error_str, int log_level)
{
    int ll = (log_level >= 0) ? log_level : 2;
    char msg[STR_MID];
    snprintf(msg, STR_MID, "Sub[%s] : %s", sub_name, error_str);
    put_log(msg, ll);
}

/* -----------------------------------------------------------------------
 * rjn_inc_calendar
 * ----------------------------------------------------------------------- */
void rjn_inc_calendar(int* itime, int itime_n, int del_t)
{
    time_type t;
    int tu;

    t.yyyy = itime[0];
    t.mo   = itime[1];
    t.dd   = itime[2];
    t.hh   = itime[3];
    t.mm   = itime[4];
    t.ss   = itime[5];
    t.milli_sec = 0;
    t.micro_sec = 0;

    tu = get_time_unit();
    if (tu == TU_MIL && itime_n >= 7) t.milli_sec = itime[6];
    if (tu == TU_MCR && itime_n >= 8) { t.milli_sec = itime[6]; t.micro_sec = itime[7]; }

    inc_calendar(&t, del_t);

    itime[0] = t.yyyy;
    itime[1] = t.mo;
    itime[2] = t.dd;
    itime[3] = t.hh;
    itime[4] = t.mm;
    itime[5] = t.ss;

    if (tu == TU_MIL && itime_n >= 7) itime[6] = t.milli_sec;
    if (tu == TU_MCR && itime_n >= 8) { itime[6] = t.milli_sec; itime[7] = t.micro_sec; }
}

/* -----------------------------------------------------------------------
 * rjn_inc_time
 * ----------------------------------------------------------------------- */
void rjn_inc_time(const char* component_name, int* itime, int itime_n)
{
    int comp_id = get_comp_id_from_name(component_name);
    time_type t;
    int tu;

    get_current_time_type(comp_id, 1, &t, NULL);

    itime[0] = t.yyyy;
    itime[1] = t.mo;
    itime[2] = t.dd;
    itime[3] = t.hh;
    itime[4] = t.mm;
    itime[5] = t.ss;

    tu = get_time_unit();
    if (tu == TU_MIL && itime_n >= 7) itime[6] = t.milli_sec;
    if (tu == TU_MCR && itime_n >= 8) { itime[6] = t.milli_sec; itime[7] = t.micro_sec; }
}

/* -----------------------------------------------------------------------
 * rjn_bcast_local
 * ----------------------------------------------------------------------- */
void rjn_bcast_local(const char* my_comp_name, int* data, int n)
{
    int my_model = get_comp_id_from_name(my_comp_name);
    jml_BcastLocal_int(my_model, data, 1, n, 0);
}

/* -----------------------------------------------------------------------
 * rjn_send_array / rjn_recv_array variants
 * ----------------------------------------------------------------------- */
void rjn_send_array_str(const char* my_comp_name, const char* recv_comp_name,
                        const char* str)
{
    int my_model   = get_comp_id_from_name(my_comp_name);
    int recv_model = get_comp_id_from_name(recv_comp_name);
    if (jml_isLocalLeader(my_model))
        jml_SendLeader_str(str, (int)strlen(str) + 1, recv_model);
}

void rjn_recv_array_str(const char* my_comp_name, const char* send_comp_name,
                        char* str, int bcast_flag)
{
    int my_model   = get_comp_id_from_name(my_comp_name);
    int send_model = get_comp_id_from_name(send_comp_name);
    if (jml_isLocalLeader(my_model))
        jml_RecvLeader_str(str, STR_LONG, send_model);
    if (bcast_flag)
        jml_BcastLocal_str(my_model, str, STR_LONG, 0);
}

void rjn_send_array_int(const char* my_comp_name, const char* recv_comp_name,
                        const int* array, int n)
{
    int my_model   = get_comp_id_from_name(my_comp_name);
    int recv_model = get_comp_id_from_name(recv_comp_name);
    if (jml_isLocalLeader(my_model))
        jml_SendLeader_int1d(array, 1, n, recv_model);
}

void rjn_recv_array_int(const char* my_comp_name, const char* send_comp_name,
                        int* array, int n, int bcast_flag)
{
    int my_model   = get_comp_id_from_name(my_comp_name);
    int send_model = get_comp_id_from_name(send_comp_name);
    if (jml_isLocalLeader(my_model))
        jml_RecvLeader_int1d(array, 1, n, send_model);
    if (bcast_flag)
        jml_BcastLocal_int(my_model, array, 1, n, 0);
}

void rjn_send_array_real(const char* my_comp_name, const char* recv_comp_name,
                         const float* array, int n)
{
    int my_model   = get_comp_id_from_name(my_comp_name);
    int recv_model = get_comp_id_from_name(recv_comp_name);
    if (jml_isLocalLeader(my_model))
        jml_SendLeader_float1d(array, 1, n, recv_model - 1, RJN_ANY_TAG);
}

void rjn_recv_array_real(const char* my_comp_name, const char* send_comp_name,
                         float* array, int n, int bcast_flag)
{
    int my_model   = get_comp_id_from_name(my_comp_name);
    int send_model = get_comp_id_from_name(send_comp_name);
    if (jml_isLocalLeader(my_model))
        jml_RecvLeader_float1d(array, 1, n, send_model - 1, RJN_ANY_TAG);
    if (bcast_flag)
        jml_BcastLocal_float(my_model, array, 1, n, 0);
}

void rjn_send_array_dbl(const char* my_comp_name, const char* recv_comp_name,
                        const double* array, int n)
{
    int my_model   = get_comp_id_from_name(my_comp_name);
    int recv_model = get_comp_id_from_name(recv_comp_name);
    if (jml_isLocalLeader(my_model))
        jml_SendLeader_double1d(array, 1, n, recv_model - 1, RJN_ANY_TAG);
}

void rjn_recv_array_dbl(const char* my_comp_name, const char* send_comp_name,
                        double* array, int n, int bcast_flag)
{
    int my_model   = get_comp_id_from_name(my_comp_name);
    int send_model = get_comp_id_from_name(send_comp_name);
    if (jml_isLocalLeader(my_model))
        jml_RecvLeader_double1d(array, 1, n, send_model - 1, RJN_ANY_TAG);
    if (bcast_flag)
        jml_BcastLocal_double(my_model, array, 1, n, 0);
}
