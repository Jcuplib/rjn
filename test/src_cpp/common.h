#pragma once
/*
 * common.h
 * C conversion of common.f90 - shared test utilities.
 */

extern int g_time_array[6]; /* {2000, 1, 1, 0, 0, 0} */

void init_common(const char* comp_name, int comp_id, const char* grid_name,
                 int nx, int ny, int nz);

/* n_global: size of global_index array */
void cal_and_def_grid(const int* global_index, int n_global);

void cal_and_set_map(const char* send_comp, const char* send_grid,
                     const char* recv_comp, const char* recv_grid,
                     double send_xs, double send_xe, int send_nx,
                     const int* send_grid_index,
                     double recv_xs, double recv_xe, int recv_nx,
                     const int* recv_grid_index);

void set_data(void);
void init_time(const int* time_array);
void set_time(const char* comp_name, const int* time_array, int delta_t);
void put_data_1d(const char* data_name, int data_id, int step);
void get_data_1d(const char* data_name);
void put_data_25d(const char* data_name, int data_id, int step);
void get_data_25d(const char* data_name);
void finalize_common(void);
