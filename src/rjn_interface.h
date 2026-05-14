#pragma once
/**
 * @file rjn_interface.h
 * @brief Public C API for the Raijin coupler.
 *
 * The routines in this header are the user-facing entry points for
 * initializing Raijin, defining grids and exchange metadata, advancing
 * coupling time, sending and receiving data, and finalizing the coupler.
 * String arguments are null-terminated C strings.
 */

#include <stdint.h>
#include <stdio.h>
#include "rjn_constant.h"
#include "rjn_exchange_class.h"

/**
 * @brief Descriptor for data sent from a component.
 *
 * Objects of this type are created by rjn_def_varp() and can be used by the
 * pointer variants of rjn_put_data_*().
 */
typedef struct {
    char comp_name[STR_SHORT]; /**< Component name. */
    char grid_name[STR_SHORT]; /**< Grid name. */
    char data_name[STR_SHORT]; /**< Send data name. */
} rjn_varp_type;

/**
 * @brief Descriptor for data received by a component.
 *
 * Objects of this type are created by rjn_def_varg() and can be used by the
 * pointer variants of rjn_get_data_*().
 */
typedef struct {
    char comp_name[STR_SHORT]; /**< Component name. */
    char grid_name[STR_SHORT]; /**< Grid name. */
    char data_name[STR_SHORT]; /**< Receive data name. */
} rjn_varg_type;

#ifdef __cplusplus
extern "C" {
#endif

/** @name Lifecycle */
/** @{ */

/**
 * @brief Set the MPI communicator used as Raijin's global communicator.
 *
 * Call this only when Raijin should couple a subset of MPI processes. The
 * communicator must be created by the application before this call.
 *
 * @param global_comm Fortran MPI communicator handle.
 */
void rjn_set_world(int global_comm);

/**
 * @brief Return the MPI communicator currently used as Raijin's global communicator.
 *
 * @return Fortran MPI communicator handle.
 */
int  rjn_get_world(void);

/**
 * @brief Register an additional component name on this task.
 *
 * This is used when one executable contains multiple components. If the task
 * has only one component, rjn_initialize() registers its model name
 * automatically.
 *
 * @param component_name Component name to register.
 */
void rjn_set_new_comp(const char* component_name);

/**
 * @brief Initialize the coupler.
 *
 * This routine initializes component metadata, MPI communicators, time
 * management, logging, grid storage, exchange storage, and remapping support.
 * It must be called once before grid and data definitions.
 *
 * @param model_name Name of the component initialized by this call.
 * @param default_time_unit Time unit: "SEC", "MIL", or "MCR". Pass NULL for "SEC".
 * @param log_level Log output level: 0 for none, 1 for standard, 2 for detail.
 *                  Pass -1 for the default level 0.
 * @param log_stderr Nonzero to mirror log output to stderr. Pass -1 for the
 *                   default value 0.
 */
void rjn_initialize(const char* model_name,
                    const char* default_time_unit,
                    int log_level,
                    int log_stderr);

/**
 * @brief Finalize coupling and optionally finalize MPI.
 *
 * The current implementation keeps @p time_array for API compatibility and
 * removes temporary grid-rank files for grids owned by this component.
 *
 * @param time_array End time array, kept for API compatibility.
 * @param is_call_finalize Nonzero to call MPI_Finalize() through Raijin.
 */
void rjn_coupling_end(const int* time_array, int is_call_finalize);

/**
 * @brief Query MPI information for a component or the global communicator.
 *
 * Use "GLOBAL" as @p comp_name to obtain global communicator information.
 *
 * @param comp_name Component name, or "GLOBAL".
 * @param my_comm Output Fortran MPI communicator handle.
 * @param my_group Output group ID.
 * @param my_size Output number of processes in the communicator.
 * @param my_rank Output local rank number in the communicator.
 */
void rjn_get_mpi_parameter(const char* comp_name,
                           int* my_comm, int* my_group,
                           int* my_size, int* my_rank);

/**
 * @brief Return the global MPI rank of the calling process.
 *
 * @return Global MPI rank.
 */
int rjn_get_myrank_global(void);

/** @} */

/** @name Component Queries */
/** @{ */

/**
 * @brief Return the total number of coupled components.
 *
 * @return Number of registered components.
 */
int  rjn_get_num_of_component(void);

/**
 * @brief Return a component name from its component ID.
 *
 * @param comp_id Component ID.
 * @param out Output buffer. The caller must provide at least STR_SHORT bytes.
 */
void rjn_get_component_name(int comp_id, char* out);

/**
 * @brief Return a component ID from its component name.
 *
 * @param comp_name Component name.
 * @return Component ID.
 */
int  rjn_get_comp_num_from_name(const char* comp_name);

/**
 * @brief Test whether the calling process belongs to a component ID.
 *
 * @param comp_id Component ID.
 * @return 1 if the process belongs to the component, otherwise 0.
 */
int  rjn_is_my_component_id(int comp_id);

/**
 * @brief Test whether the calling process belongs to a component name.
 *
 * @param comp_name Component name.
 * @return 1 if the process belongs to the component, otherwise 0.
 */
int  rjn_is_my_component_name(const char* comp_name);

/**
 * @brief Test whether a component is running in the current MPI job.
 *
 * @param comp_name Component name.
 * @return 1 if the component is running, otherwise 0.
 */
int  rjn_is_model_running(const char* comp_name);

/** @} */

/** @name Grid and Variable Definitions */
/** @{ */

/**
 * @brief Define a grid owned by a component.
 *
 * The grid index array contains local grid point indices for this MPI rank.
 * Indices must be positive. This routine also writes grid-rank metadata used
 * when building mapping tables.
 *
 * @param grid_index Local grid point indices.
 * @param n Number of entries in @p grid_index.
 * @param model_name Component name.
 * @param grid_name Grid name.
 * @param num_of_vgrid Number of vertical layers. Pass 0 if unused.
 */
void rjn_def_grid(const int* grid_index, int n,
                  const char* model_name, const char* grid_name,
                  int num_of_vgrid);

/**
 * @brief Declare the end of grid definition.
 *
 * This routine performs a barrier on the global communicator.
 */
void rjn_end_grid_def(void);

/**
 * @brief Create a send variable descriptor.
 *
 * The returned pointer is allocated with malloc() and is owned by the caller.
 *
 * @param comp_name Component name.
 * @param data_name Send data name.
 * @param grid_name Grid name.
 * @param num_of_layer Number of data layers, kept for API compatibility.
 * @return Newly allocated send variable descriptor.
 */
rjn_varp_type* rjn_def_varp(const char* comp_name,
                            const char* data_name,
                            const char* grid_name,
                            int num_of_layer);

/**
 * @brief Create a receive variable descriptor.
 *
 * The returned pointer is allocated with malloc() and is owned by the caller.
 *
 * @param comp_name Component name.
 * @param data_name Receive data name.
 * @param grid_name Grid name.
 * @return Newly allocated receive variable descriptor.
 */
rjn_varg_type* rjn_def_varg(const char* comp_name,
                            const char* data_name,
                            const char* grid_name);

/**
 * @brief Declare the end of variable definition.
 *
 * This routine is currently a no-op kept for API compatibility.
 */
void rjn_end_var_def(void);

/**
 * @brief Set the fill value used by compatibility callers.
 *
 * This routine is currently a no-op.
 *
 * @param fill_value Fill value.
 */
void   rjn_set_fill_value(double fill_value);

/**
 * @brief Return the compatibility fill value.
 *
 * @return The fill value -9999.0.
 */
double rjn_get_fill_value(void);

/** @} */

/** @name Mapping and Exchange Metadata */
/** @{ */

/**
 * @brief Register a mapping table for a send-grid to receive-grid pair.
 *
 * @param my_model_name Local component name.
 * @param send_model_name Sending component name.
 * @param send_grid_name Sending grid name.
 * @param recv_model_name Receiving component name.
 * @param recv_grid_name Receiving grid name.
 * @param map_tag Mapping table identifier.
 * @param is_recv_intpl Nonzero when interpolation is performed on the receiver side.
 * @param intpl_mode Interpolation mode: "FAST", "SAFE", "PARALLEL", or "USER".
 * @param send_grid Sending grid indices. Length is @p n.
 * @param recv_grid Receiving grid indices. Length is @p n.
 * @param coef Interpolation coefficients. Length is @p n.
 * @param n Number of mapping entries.
 */
void rjn_set_mapping_table(const char* my_model_name,
                           const char* send_model_name, const char* send_grid_name,
                           const char* recv_model_name, const char* recv_grid_name,
                           int map_tag, int is_recv_intpl,
                           const char* intpl_mode,
                           const int* send_grid, const int* recv_grid,
                           const double* coef, int n);

/**
 * @brief Register a user-defined interpolation function for one mapping table.
 *
 * This is used with mapping tables whose interpolation mode is "USER".
 *
 * @param send_comp Sending component name.
 * @param send_grid Sending grid name.
 * @param recv_comp Receiving component name.
 * @param recv_grid Receiving grid name.
 * @param map_tag Mapping table identifier.
 * @param user_func User-defined interpolation function.
 */
void rjn_set_user_interpolation(const char* send_comp, const char* send_grid,
                                const char* recv_comp, const char* recv_grid,
                                int map_tag,
                                interpolation_user_ifc user_func);

/**
 * @brief Write all registered mapping tables to a binary stream.
 *
 * @param fp Writable binary stream.
 */
void rjn_write_mapping_table(FILE* fp);

/**
 * @brief Read mapping tables from a binary stream.
 *
 * @param fp Readable binary stream.
 */
void rjn_read_mapping_table(FILE* fp);

/**
 * @brief Register data exchange metadata for one send/receive data pair.
 *
 * @param send_comp Sending component name.
 * @param send_grid Sending grid name.
 * @param send_data_name Sending data name.
 * @param recv_comp Receiving component name.
 * @param recv_grid Receiving grid name.
 * @param recv_data_name Receiving data name.
 * @param map_tag Mapping table identifier.
 * @param is_avr Nonzero when the sending data are time averaged.
 * @param intvl Exchange interval in seconds.
 * @param time_lag Time lag in seconds.
 * @param num_of_layer Number of vertical layers.
 * @param grid_intpl_tag Data identification tag passed to interpolation.
 * @param fill_value Fill value for missing data.
 * @param exchange_tag MPI exchange tag.
 */
void rjn_set_data(const char* send_comp, const char* send_grid, const char* send_data_name,
                  const char* recv_comp, const char* recv_grid, const char* recv_data_name,
                  int map_tag, int is_avr, int intvl, int time_lag, int num_of_layer,
                  int grid_intpl_tag, double fill_value, int exchange_tag);

/** @} */

/** @name Time Management */
/** @{ */

/**
 * @brief Initialize coupler time.
 *
 * @param time_array Initial time array in year, month, day, hour, minute,
 *                   second order. Kept for API compatibility.
 */
void rjn_init_time(const int* time_array);

/**
 * @brief Advance coupler time and process scheduled data exchanges.
 *
 * This routine posts receives, waits for pending sends and receives, and
 * completes interpolation for registered receive data entries.
 *
 * @param comp_name Component name.
 * @param current_time Current time array, kept for API compatibility.
 * @param delta_t Time-step length in seconds.
 */
void rjn_set_time(const char* comp_name,
                  const int* current_time, int delta_t);

/** @} */

/** @name Data Send and Receive */
/** @{ */

/**
 * @brief Send one-dimensional data by data name.
 *
 * @param data_name Send data name.
 * @param data Send buffer.
 * @param n_data Number of elements in @p data.
 */
void rjn_put_data_1d(const char* data_name,
                     const double* data, int n_data);

/**
 * @brief Send one-dimensional data by send variable descriptor.
 *
 * @param varp_ptr Send variable descriptor.
 * @param data Send buffer.
 * @param n_data Number of elements in @p data.
 */
void rjn_put_data_1d_ptr(const rjn_varp_type* varp_ptr,
                         const double* data, int n_data);

/**
 * @brief Receive one-dimensional data by data name.
 *
 * @param data_name Receive data name.
 * @param data Receive buffer.
 * @param n_data Number of elements in @p data.
 * @param is_recv_ok Output flag set to 1 when new data were available.
 */
void rjn_get_data_1d(const char* data_name,
                     double* data, int n_data, int* is_recv_ok);

/**
 * @brief Receive one-dimensional data by receive variable descriptor.
 *
 * @param varg_ptr Receive variable descriptor.
 * @param data Receive buffer.
 * @param n_data Number of elements in @p data.
 * @param is_recv_ok Output flag set to 1 when new data were available.
 */
void rjn_get_data_1d_ptr(const rjn_varg_type* varg_ptr,
                         double* data, int n_data, int* is_recv_ok);

/**
 * @brief Send data with an explicit layer dimension by data name.
 *
 * The data layout is Fortran-compatible: element (i,k) is stored as
 * data[i + k * n1] for zero-based C indices.
 *
 * @param data_name Send data name.
 * @param data Send buffer of size @p n1 * @p n2.
 * @param n1 First dimension length.
 * @param n2 Second dimension length.
 */
void rjn_put_data_25d(const char* data_name,
                      const double* data, int n1, int n2);

/**
 * @brief Send data with an explicit layer dimension by send descriptor.
 *
 * @param varp_ptr Send variable descriptor.
 * @param data Send buffer of size @p n1 * @p n2.
 * @param n1 First dimension length.
 * @param n2 Second dimension length.
 */
void rjn_put_data_25d_ptr(const rjn_varp_type* varp_ptr,
                          const double* data, int n1, int n2);

/**
 * @brief Receive data with an explicit layer dimension by data name.
 *
 * @param data_name Receive data name.
 * @param data Receive buffer of size @p n1 * @p n2.
 * @param n1 First dimension length.
 * @param n2 Second dimension length.
 * @param is_recv_ok Output flag set to 1 when new data were available.
 */
void rjn_get_data_25d(const char* data_name,
                      double* data, int n1, int n2, int* is_recv_ok);

/**
 * @brief Receive data with an explicit layer dimension by receive descriptor.
 *
 * @param varg_ptr Receive variable descriptor.
 * @param data Receive buffer of size @p n1 * @p n2.
 * @param n1 First dimension length.
 * @param n2 Second dimension length.
 * @param is_recv_ok Output flag set to 1 when new data were available.
 */
void rjn_get_data_25d_ptr(const rjn_varg_type* varg_ptr,
                          double* data, int n1, int n2, int* is_recv_ok);

/** @} */

/** @name Utilities */
/** @{ */

/**
 * @brief Output an error message and terminate through Raijin error handling.
 *
 * @param sub_name Routine name reported in the error message.
 * @param error_str Error message.
 */
void rjn_error_ifc(const char* sub_name, const char* error_str);

/**
 * @brief Write a log message.
 *
 * @param sub_name Routine name reported in the log message.
 * @param error_str Log message.
 * @param log_level Log output level. Pass -1 for the default level 2.
 */
void rjn_log(const char* sub_name, const char* error_str, int log_level);

/**
 * @brief Increment a calendar time array by a number of seconds.
 *
 * @param itime Time array. The first six entries are year, month, day, hour,
 *              minute, and second. Millisecond and microsecond fields are used
 *              when the active time unit requires them and @p itime_n is large
 *              enough.
 * @param itime_n Number of entries available in @p itime.
 * @param del_t Seconds to add.
 */
void rjn_inc_calendar(int* itime, int itime_n, int del_t);

/**
 * @brief Copy the current Raijin time for a component into a calendar array.
 *
 * @param component_name Component name.
 * @param itime Output time array.
 * @param itime_n Number of entries available in @p itime.
 */
void rjn_inc_time(const char* component_name, int* itime, int itime_n);

/**
 * @brief Broadcast an integer array within a component communicator.
 *
 * @param my_comp_name Component name.
 * @param data Integer array to broadcast in place.
 * @param n Number of elements in @p data.
 */
void rjn_bcast_local(const char* my_comp_name, int* data, int n);

/**
 * @brief Send an integer array from one component leader to another.
 *
 * @param my_comp_name Sending component name.
 * @param recv_comp_name Receiving component name.
 * @param array Integer array to send.
 * @param n Number of elements in @p array.
 */
void rjn_send_array_int(const char* my_comp_name, const char* recv_comp_name,
                        const int* array, int n);

/**
 * @brief Receive an integer array from another component leader.
 *
 * @param my_comp_name Receiving component name.
 * @param send_comp_name Sending component name.
 * @param array Receive buffer.
 * @param n Number of elements in @p array.
 * @param bcast_flag Nonzero to broadcast received data within the local component.
 */
void rjn_recv_array_int(const char* my_comp_name, const char* send_comp_name,
                        int* array, int n, int bcast_flag);

/**
 * @brief Send a single-precision real array from one component leader to another.
 *
 * @param my_comp_name Sending component name.
 * @param recv_comp_name Receiving component name.
 * @param array Array to send.
 * @param n Number of elements in @p array.
 */
void rjn_send_array_real(const char* my_comp_name, const char* recv_comp_name,
                         const float* array, int n);

/**
 * @brief Receive a single-precision real array from another component leader.
 *
 * @param my_comp_name Receiving component name.
 * @param send_comp_name Sending component name.
 * @param array Receive buffer.
 * @param n Number of elements in @p array.
 * @param bcast_flag Nonzero to broadcast received data within the local component.
 */
void rjn_recv_array_real(const char* my_comp_name, const char* send_comp_name,
                         float* array, int n, int bcast_flag);

/**
 * @brief Send a double-precision real array from one component leader to another.
 *
 * @param my_comp_name Sending component name.
 * @param recv_comp_name Receiving component name.
 * @param array Array to send.
 * @param n Number of elements in @p array.
 */
void rjn_send_array_dbl(const char* my_comp_name, const char* recv_comp_name,
                        const double* array, int n);

/**
 * @brief Receive a double-precision real array from another component leader.
 *
 * @param my_comp_name Receiving component name.
 * @param send_comp_name Sending component name.
 * @param array Receive buffer.
 * @param n Number of elements in @p array.
 * @param bcast_flag Nonzero to broadcast received data within the local component.
 */
void rjn_recv_array_dbl(const char* my_comp_name, const char* send_comp_name,
                        double* array, int n, int bcast_flag);

/**
 * @brief Send a string from one component leader to another.
 *
 * @param my_comp_name Sending component name.
 * @param recv_comp_name Receiving component name.
 * @param str Null-terminated string to send.
 */
void rjn_send_array_str(const char* my_comp_name, const char* recv_comp_name,
                        const char* str);

/**
 * @brief Receive a string from another component leader.
 *
 * @param my_comp_name Receiving component name.
 * @param send_comp_name Sending component name.
 * @param str Receive buffer. The caller must provide at least STR_LONG bytes.
 * @param bcast_flag Nonzero to broadcast received data within the local component.
 */
void rjn_recv_array_str(const char* my_comp_name, const char* send_comp_name,
                        char* str, int bcast_flag);

/** @} */

#ifdef __cplusplus
}
#endif
