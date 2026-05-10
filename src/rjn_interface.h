#pragma once
/*
 * rjn_interface.h
 * Converted from rjn_interface.f90
 * Copyright (c) 2011, arakawa@rist.jp
 */

#include <stdint.h>
#include <stdio.h>
#include "rjn_constant.h"
#include "rjn_exchange_class.h"

/* -----------------------------------------------------------------------
 * Descriptor types for send/recv variable pointers
 * ----------------------------------------------------------------------- */
typedef struct {
    char comp_name[STR_SHORT];
    char grid_name[STR_SHORT];
    char data_name[STR_SHORT];
} rjn_varp_type;

typedef struct {
    char comp_name[STR_SHORT];
    char grid_name[STR_SHORT];
    char data_name[STR_SHORT];
} rjn_varg_type;

#ifdef __cplusplus
extern "C" {
#endif

/* -----------------------------------------------------------------------
 * Lifecycle
 * ----------------------------------------------------------------------- */

/* Register this task's component name (must be called before rjn_initialize
 * if you want to use a name different from model_name). */
void rjn_set_new_comp(const char* component_name);

/* Initialise the coupler.
 * default_time_unit: "SEC" | "MIL" | "MCR" (NULL → "SEC")
 * log_level        : 0=NO_OUTPUT, 1=STANDARD, 2=DETAIL  (-1 → 0)
 * log_stderr       : 1 = mirror to stderr, 0 = no        (-1 → 0)
 */
void rjn_initialize(const char* model_name,
                    const char* default_time_unit,   /* NULL for SEC */
                    int log_level,                   /* -1 for default (0) */
                    int log_stderr);                 /* -1 for default (0) */

/* Finalise the coupler and (optionally) call MPI_Finalize.
 * time_array: unused (kept for API compatibility)
 * is_call_finalize: 1 = call MPI_Finalize, 0 = skip
 */
void rjn_coupling_end(const int* time_array, int is_call_finalize);

/* Query MPI parameters for a component.
 * Pass "GLOBAL" for comp_name to get global communicator info. */
void rjn_get_mpi_parameter(const char* comp_name,
                           int* my_comm, int* my_group,
                           int* my_size, int* my_rank);

/* Return global MPI rank of the calling process */
int rjn_get_myrank_global(void);

/* -----------------------------------------------------------------------
 * Component / model queries (thin wrappers over rjn_comp / rjn_mpi_lib)
 * ----------------------------------------------------------------------- */
int  rjn_get_num_of_component(void);
void rjn_get_component_name(int comp_id, char* out);
int  rjn_get_comp_num_from_name(const char* comp_name);
int  rjn_is_my_component_id(int comp_id);    /* renamed from rjn_is_my_component */
int  rjn_is_my_component_name(const char* comp_name);
int  rjn_is_model_running(const char* comp_name);

/* -----------------------------------------------------------------------
 * Grid definition
 * ----------------------------------------------------------------------- */

/* Define a grid on this component and write GRID_INDEX/RANK files.
 * num_of_vgrid is optional; pass 0 to ignore. */
void rjn_def_grid(const int* grid_index, int n,
                  const char* model_name, const char* grid_name,
                  int num_of_vgrid);

/* MPI barrier (replaces Fortran's end_grid_def barrier) */
void rjn_end_grid_def(void);

/* -----------------------------------------------------------------------
 * Variable pointer helpers (allocate + fill)
 * ----------------------------------------------------------------------- */
rjn_varp_type* rjn_def_varp(const char* comp_name,
                             const char* data_name,
                             const char* grid_name,
                             int num_of_layer);

rjn_varg_type* rjn_def_varg(const char* comp_name,
                             const char* data_name,
                             const char* grid_name);

void rjn_end_var_def(void);  /* no-op */

/* -----------------------------------------------------------------------
 * Fill-value helpers (stubs matching Fortran dummies)
 * ----------------------------------------------------------------------- */
void   rjn_set_fill_value(double fill_value);  /* no-op */
double rjn_get_fill_value(void);               /* returns -9999.0 */

/* -----------------------------------------------------------------------
 * Mapping table
 * ----------------------------------------------------------------------- */

/* Register the mapping table for one send→recv pair.
 * is_recv_intpl : 1 = interpolation done on receiver side
 * intpl_mode    : "FAST" | "SAFE" | "PARALLEL" | "USER"
 * send_grid / recv_grid / coef : arrays of length n
 */
void rjn_set_mapping_table(const char* my_model_name,
                            const char* send_model_name, const char* send_grid_name,
                            const char* recv_model_name, const char* recv_grid_name,
                            int map_tag, int is_recv_intpl,
                            const char* intpl_mode,
                            const int* send_grid, const int* recv_grid,
                            const double* coef, int n);

/* Register a user-defined interpolation function for a given exchange */
void rjn_set_user_interpolation(const char* send_comp, const char* send_grid,
                                const char* recv_comp, const char* recv_grid,
                                int map_tag,
                                interpolation_user_ifc user_func);

/* Persist / restore the full mapping table to/from a binary FILE* */
void rjn_write_mapping_table(FILE* fp);
void rjn_read_mapping_table(FILE* fp);

/* -----------------------------------------------------------------------
 * Data registration
 * (thin re-export of set_data from rjn_data)
 * ----------------------------------------------------------------------- */
void rjn_set_data(const char* send_comp, const char* send_grid, const char* send_data_name,
                  const char* recv_comp, const char* recv_grid, const char* recv_data_name,
                  int map_tag, int is_avr, int intvl, int time_lag, int num_of_layer,
                  int grid_intpl_tag, double fill_value, int exchange_tag);

/* -----------------------------------------------------------------------
 * Time management
 * ----------------------------------------------------------------------- */

/* Initialise coupler time; time_array[6] is unused (kept for compatibility) */
void rjn_init_time(const int* time_array);

/* Advance coupler time by delta_t seconds; posts non-blocking receives and
 * completes interpolation for all registered recv data entries.
 * current_time[6] is unused (kept for compatibility). */
void rjn_set_time(const char* comp_name,
                  const int* current_time, int delta_t);

/* -----------------------------------------------------------------------
 * Data send / receive
 * ----------------------------------------------------------------------- */

/* Send 1D data by name */
void rjn_put_data_1d(const char* data_name,
                     const double* data, int n_data);

/* Send 1D data by pointer */
void rjn_put_data_1d_ptr(const rjn_varp_type* varp_ptr,
                         const double* data, int n_data);

/* Receive 1D data by name; is_recv_ok set to 1 if data was available */
void rjn_get_data_1d(const char* data_name,
                     double* data, int n_data, int* is_recv_ok);

/* Receive 1D data by pointer */
void rjn_get_data_1d_ptr(const rjn_varg_type* varg_ptr,
                         double* data, int n_data, int* is_recv_ok);

/* Send 2D data by name; data is [n1 * n2] row-major */
void rjn_put_data_25d(const char* data_name,
                      const double* data, int n1, int n2);

/* Send 2D data by pointer */
void rjn_put_data_25d_ptr(const rjn_varp_type* varp_ptr,
                          const double* data, int n1, int n2);

/* Receive 2D data by name */
void rjn_get_data_25d(const char* data_name,
                      double* data, int n1, int n2, int* is_recv_ok);

/* Receive 2D data by pointer */
void rjn_get_data_25d_ptr(const rjn_varg_type* varg_ptr,
                           double* data, int n1, int n2, int* is_recv_ok);

/* -----------------------------------------------------------------------
 * Utilities
 * ----------------------------------------------------------------------- */

/* Abort with an error message */
void rjn_error_ifc(const char* sub_name, const char* error_str);

/* Log a message at the given log_level (pass -1 for default level 2) */
void rjn_log(const char* sub_name, const char* error_str, int log_level);

/* Increment a calendar time array by del_t seconds (in-place).
 * itime[6..] extends to [7] for milliseconds, [8] for microseconds
 * depending on the current time unit. */
void rjn_inc_calendar(int* itime, int itime_n, int del_t);

/* Retrieve current time for comp into itime (in-place) */
void rjn_inc_time(const char* component_name, int* itime, int itime_n);

/* Broadcast a local int array within a component's communicator */
void rjn_bcast_local(const char* my_comp_name, int* data, int n);

/* Send/recv an int array between component leaders */
void rjn_send_array_int(const char* my_comp_name, const char* recv_comp_name,
                        const int* array, int n);
void rjn_recv_array_int(const char* my_comp_name, const char* send_comp_name,
                        int* array, int n, int bcast_flag);

/* Send/recv a float array between component leaders */
void rjn_send_array_real(const char* my_comp_name, const char* recv_comp_name,
                         const float* array, int n);
void rjn_recv_array_real(const char* my_comp_name, const char* send_comp_name,
                         float* array, int n, int bcast_flag);

/* Send/recv a double array between component leaders */
void rjn_send_array_dbl(const char* my_comp_name, const char* recv_comp_name,
                        const double* array, int n);
void rjn_recv_array_dbl(const char* my_comp_name, const char* send_comp_name,
                        double* array, int n, int bcast_flag);

/* Send/recv a string (char array) between component leaders */
void rjn_send_array_str(const char* my_comp_name, const char* recv_comp_name,
                        const char* str);
void rjn_recv_array_str(const char* my_comp_name, const char* send_comp_name,
                        char* str, int bcast_flag);

/* Global communicator set/get (delegated to jml_set/get_global_comm) */
void rjn_set_world(int global_comm);
int  rjn_get_world(void);

#ifdef __cplusplus
}
#endif
