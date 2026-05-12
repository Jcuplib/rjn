/*
 * common.cpp
 * C conversion of common.f90 - shared test utilities wrapping rjn_interface.
 */
#include "common.h"
#include "namelist.h"
#include "rjn_interface.h"
#include "rjn_mpi_lib.h"
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

int g_time_array[6] = {2000, 1, 1, 0, 0, 0};

/* ---- module-level state ---- */
static char my_name[NL_STR_SHORT];
static char my_grid[NL_STR_SHORT];
static int  my_comp_id;
static int  my_nx, my_ny;
static int  g_num_of_layer;

static int  my_comm, my_group, my_size, my_rank;
static int  my_pos_x, my_pos_y;
static int  is_x, ie_x, js_y, je_y;
static int  num_of_local_grid;
static int  my_rank_global;
static int* grid_index;   /* malloc'd, size = num_of_local_grid */

/* ---- domain decomposition ---- */

/* Distribute num_grid points over num_div processes; my_pos is 1-based rank+1.
   Returns 1-based [is, ie]. */
static void cal_start_end(int num_grid, int num_div, int my_pos,
                           int* is_out, int* ie_out) {
    int div_num = num_grid / num_div;
    int rem     = num_grid % num_div;
    int offset  = div_num * (my_pos - 1) + (my_pos - 1 < rem ? my_pos - 1 : rem);
    *is_out = offset + 1;                           /* 1-based */
    *ie_out = offset + div_num + (my_pos <= rem ? 1 : 0);
}

/* ---- public functions ---- */

void init_common(const char* comp_name, int comp_id, const char* grid_name,
                 int nx, int ny, int nz) {
    strncpy(my_name, comp_name, NL_STR_SHORT - 1);
    my_comp_id      = comp_id;
    strncpy(my_grid, grid_name, NL_STR_SHORT - 1);
    my_nx           = nx;
    my_ny           = ny;
    g_num_of_layer  = nz;

    
    read_coupler_config("coupling.conf");
    read_namelist("coupling.conf");


    rjn_initialize(my_name, "SEC", 2, 0);

    rjn_get_mpi_parameter(my_name, &my_comm, &my_group, &my_size, &my_rank);

    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank_global);

    my_pos_x = (my_rank % my_size) + 1;
    my_pos_y = my_rank / my_size + 1;

    cal_start_end(my_nx, my_size, my_pos_x, &is_x, &ie_x);
    cal_start_end(my_ny, 1,       my_pos_y, &js_y, &je_y);

    num_of_local_grid = (ie_x - is_x + 1) * (je_y - js_y + 1);
}

void cal_and_def_grid(const int* global_index, int /*n_global*/) {
    grid_index = (int*)malloc(num_of_local_grid * sizeof(int));

    int counter = 0;
    for (int j = js_y; j <= je_y; j++) {
        for (int i = is_x; i <= ie_x; i++) {
            grid_index[counter++] = global_index[i - 1]; /* 1-based → 0-based */
        }
    }

    rjn_def_grid(grid_index, num_of_local_grid, my_name, my_grid, g_num_of_layer);

    {
        char fname[64];
        snprintf(fname, sizeof(fname), "fort.%d", 300 + my_rank_global);
        FILE* fp = fopen(fname, "a");
        if (fp) {
            fprintf(fp, " cal_and_def_grid, %d %d %d %d %d",
                    is_x, ie_x, js_y, je_y, num_of_local_grid);
            for (int k = 0; k < num_of_local_grid; k++)
                fprintf(fp, " %d", grid_index[k]);
            fprintf(fp, "\n");
            fclose(fp);
        }
    }

    rjn_end_grid_def();
}

/* ---- mapping table helpers ---- */

static void cal_index_coef(double r_point, double s_start, double s_stride,
                            int* idx1, int* idx2, double* coef1, double* coef2) {
    *idx1  = (int)((r_point - s_start) / s_stride);
    *idx2  = *idx1 + 1;
    *coef1 = (s_stride * *idx2 - r_point) / s_stride;
    *coef2 = (r_point  - s_stride * *idx1) / s_stride;
}

/* Write .send.map / .recv.map / .coef.map for the recv component (rank 0 only).
   recv_comp_name is used as the file prefix. */
static void make_mapping_file(const char* recv_comp_name,
                               double send_xs, double send_xe, int send_nx,
                               const int* send_index,
                               double recv_xs, double recv_xe, int recv_nx,
                               const int* recv_index,
                               int* table_size_out) {
    char fnames[3][NL_STR_LONG];
    snprintf(fnames[0], NL_STR_LONG, "%s.send.map", recv_comp_name);
    snprintf(fnames[1], NL_STR_LONG, "%s.recv.map", recv_comp_name);
    snprintf(fnames[2], NL_STR_LONG, "%s.coef.map", recv_comp_name);

    FILE* fp[3];
    for (int k = 0; k < 3; k++) {
        fp[k] = fopen(fnames[k], "w");
        if (!fp[k]) {
            fprintf(stderr, "make_mapping_file: cannot open %s\n", fnames[k]);
            *table_size_out = 0;
            return;
        }
    }

    double sx_stride = (send_xe - send_xs) / send_nx;
    double rx_stride = (recv_xe - recv_xs) / recv_nx;
    int table_size = 0;

    for (int ri = 1; ri <= recv_nx; ri++) {
        double rx = rx_stride * (ri - 1) + recv_xs;
        int idx1, idx2;
        double c1, c2;
        cal_index_coef(rx, send_xs, sx_stride, &idx1, &idx2, &c1, &c2);

        int ridx  = ri;
        int sidx1 = idx1 + 1; /* 1-based */
        int sidx2 = idx2 + 1;

        if (c1 > 0.0 && sidx1 > 0 && sidx1 <= send_nx && ridx <= recv_nx) {
            fprintf(fp[0], " %d\n", send_index[sidx1 - 1]);
            fprintf(fp[1], " %d\n", recv_index[ridx  - 1]);
            fprintf(fp[2], " %21.14E\n", c1);
            table_size++;
        }
        if (c2 > 0.0 && sidx2 > 0 && sidx2 <= send_nx && ridx <= recv_nx) {
            fprintf(fp[0], " %d\n", send_index[sidx2 - 1]);
            fprintf(fp[1], " %d\n", recv_index[ridx  - 1]);
            fprintf(fp[2], " %21.14E\n", c2);
            table_size++;
        }
    }

    for (int k = 0; k < 3; k++) fclose(fp[k]);
    *table_size_out = table_size;
}

void cal_and_set_map(const char* send_comp, const char* send_grid,
                     const char* recv_comp, const char* recv_grid,
                     double send_xs, double send_xe, int send_nx,
                     const int* send_grid_index,
                     double recv_xs, double recv_xe, int recv_nx,
                     const int* recv_grid_index) {
    int table_size = 1;
    int int_array[1];

    /* recv_comp rank 0 computes and writes the mapping file */
    if (strcmp(my_name, recv_comp) == 0) {
        if (my_rank == 0) {
            make_mapping_file(my_name,
                              send_xs, send_xe, send_nx, send_grid_index,
                              recv_xs, recv_xe, recv_nx, recv_grid_index,
                              &table_size);
        }
        /* broadcast table_size to other ranks in this component */
        MPI_Comm local_comm = MPI_Comm_f2c((MPI_Fint)my_comm);
        MPI_Bcast(&table_size, 1, MPI_INT, 0, local_comm);

        /* send table_size to send_comp */
        int_array[0] = table_size;
        rjn_send_array_int(my_name, send_comp, int_array, 1);
    } else {
        /* send_comp receives table_size from recv_comp */
        rjn_recv_array_int(my_name, recv_comp, int_array, 1, 1 /*bcast*/);
        table_size = int_array[0];
    }

    /* All processes read the mapping files */
    int* send_idx = (int*)   malloc(table_size * sizeof(int));
    int* recv_idx = (int*)   malloc(table_size * sizeof(int));
    double* coef  = (double*)malloc(table_size * sizeof(double));

    char fn_send[NL_STR_LONG], fn_recv[NL_STR_LONG], fn_coef[NL_STR_LONG];
    snprintf(fn_send, NL_STR_LONG, "%s.send.map", recv_comp);
    snprintf(fn_recv, NL_STR_LONG, "%s.recv.map", recv_comp);
    snprintf(fn_coef, NL_STR_LONG, "%s.coef.map", recv_comp);

    FILE* f1 = fopen(fn_send, "r");
    FILE* f2 = fopen(fn_recv, "r");
    FILE* f3 = fopen(fn_coef, "r");
    if (!f1 || !f2 || !f3) {
        fprintf(stderr, "cal_and_set_map: cannot open mapping files for %s\n", recv_comp);
    } else {
        for (int i = 0; i < table_size; i++) {
            fscanf(f1, "%d", &send_idx[i]);
            fscanf(f2, "%d", &recv_idx[i]);
            fscanf(f3, "%lf", &coef[i]);
        }
    }
    if (f1) fclose(f1);
    if (f2) fclose(f2);
    if (f3) fclose(f3);

    rjn_set_mapping_table(my_name, send_comp, send_grid, recv_comp, recv_grid,
                          1, 0 /*is_recv_intpl=false*/, "SAFE",
                          send_idx, recv_idx, coef, table_size);

    {
        char fname[64];
        snprintf(fname, sizeof(fname), "fort.%d", 300 + my_rank_global);
        FILE* fp = fopen(fname, "a");
        if (fp) {
            fprintf(fp, " cal_and_set_map, %s %s", send_grid, recv_grid);
            for (int i = 0; i < table_size; i++)
                fprintf(fp, " %d", send_idx[i]);
            for (int i = 0; i < table_size; i++)
                fprintf(fp, " %d", recv_idx[i]);
            fprintf(fp, "\n");
            fclose(fp);
        }
    }

    free(send_idx);
    free(recv_idx);
    free(coef);
}

void set_data(void) {
    int n = get_num_of_configuration();
    for (int i = 1; i <= n; i++) {
        const char* put_comp = get_put_comp_name(i);
        const char* put_grid = get_put_grid_name(i);
        const char* get_comp = get_get_comp_name(i);
        const char* get_grid = get_get_grid_name(i);

        if (strcmp(put_comp, my_name) != 0 && strcmp(get_comp, my_name) != 0)
            continue;

        int m = get_num_of_exchange(i);
        for (int j = 1; j <= m; j++) {
            exchange_data_type* ed = get_ed_ptr(i, j);
            rjn_set_data(put_comp, put_grid, ed->var_put,
                         get_comp, get_grid, ed->var_get,
                         1,                             /* map_tag */
                         strcmp(ed->flag, "AVR") == 0,  /* is_avr */
                         ed->intvl, ed->lag, ed->num_of_layer,
                         ed->grid_intpl_tag, ed->fill_value, ed->mapping_tag);
        }
    }
}

void init_time(const int* time_array) {
    rjn_init_time(time_array);
}

void set_time(const char* comp_name, const int* time_array, int delta_t) {
    rjn_set_time(comp_name, time_array, delta_t);
}

void put_data_1d(const char* data_name, int data_id, int step) {
    double* data = (double*)malloc(num_of_local_grid * sizeof(double));
    for (int i = 0; i < num_of_local_grid; i++)
        data[i] = step * 10000 + data_id * 100 + grid_index[i];
    rjn_put_data_1d(data_name, data, num_of_local_grid);
    free(data);
}

void get_data_1d(const char* data_name) {
    double* data = (double*)malloc(num_of_local_grid * sizeof(double));
    for (int i = 0; i < num_of_local_grid; i++) data[i] = -1.0;

    int is_recv_ok = 0;
    rjn_get_data_1d(data_name, data, num_of_local_grid, &is_recv_ok);

    {
        char fname[64];
        snprintf(fname, sizeof(fname), "fort.%d", 300 + my_comp_id * 100 + my_rank);
        FILE* fp = fopen(fname, "a");
        if (fp) {
            for (int i = 0; i < num_of_local_grid; i++)
                fprintf(fp, " %d", (int)data[i]);
            fprintf(fp, "\n");
            fclose(fp);
        }
    }
    free(data);
}

void put_data_25d(const char* data_name, int data_id, int step) {
    int n = num_of_local_grid * g_num_of_layer;
    double* data = (double*)malloc(n * sizeof(double));
    for (int k = 0; k < g_num_of_layer; k++) {
        for (int i = 0; i < num_of_local_grid; i++) {
            data[i + k * num_of_local_grid] =
                (k + 1) * 1000000.0 + step * 10000.0 + data_id * 100 + grid_index[i];
        }
    }
    rjn_put_data_25d(data_name, data, num_of_local_grid, g_num_of_layer);
    free(data);
}

void get_data_25d(const char* data_name) {
    int n = num_of_local_grid * g_num_of_layer;
    double* data = (double*)malloc(n * sizeof(double));
    for (int i = 0; i < n; i++) data[i] = -1.0;

    int is_recv_ok = 0;
    rjn_get_data_25d(data_name, data, num_of_local_grid, g_num_of_layer, &is_recv_ok);

    {
        char fname[64];
        snprintf(fname, sizeof(fname), "fort.%d", 300 + my_comp_id * 100 + my_rank);
        FILE* fp = fopen(fname, "a");
        if (fp) {
            for (int i = 0; i < n; i++) fprintf(fp, " %d", (int)data[i]);
            fprintf(fp, "\n");
            fclose(fp);
        }
    }
    free(data);
}

void finalize_common(void) {
    int time_array[6] = {0};
    rjn_coupling_end(time_array, 1 /*call MPI_Finalize*/);
    free(grid_index);
    grid_index = NULL;
}
