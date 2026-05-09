/*
 * rjn_data_class.cpp
 * Converted from rjn_data_class.f90
 * Copyright (c) 2011, arakawa@rist.jp
 */

#include "rjn_data_class.h"
#include "rjn_exchange.h"
#include "rjn_utils.h"
#include "rjn_constant.h"
#include "rjn_mpi_lib.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* -----------------------------------------------------------------------
 * data_class_init (constructor)
 * ----------------------------------------------------------------------- */
data_class data_class_init(
    const char* send_comp_name, const char* send_grid_name, const char* send_data_name,
    const char* recv_comp_name, const char* recv_grid_name, const char* recv_data_name,
    int avr_flag, int intvl, int time_lag, int exchange_type,
    int num_of_layer, int intpl_tag,
    double fill_value, int exchange_tag,
    double factor, double offset)
{
    data_class d;
    memset(&d, 0, sizeof(d));

    strncpy(d.send_comp_name, send_comp_name, STR_SHORT - 1);
    strncpy(d.send_grid_name, send_grid_name, STR_SHORT - 1);
    strncpy(d.send_data_name, send_data_name, STR_SHORT - 1);
    strncpy(d.recv_comp_name, recv_comp_name, STR_SHORT - 1);
    strncpy(d.recv_grid_name, recv_grid_name, STR_SHORT - 1);
    strncpy(d.recv_data_name, recv_data_name, STR_SHORT - 1);

    d.avr_flag      = avr_flag;
    d.intvl         = intvl;
    d.time_lag      = time_lag;
    d.exchange_type = exchange_type;
    d.num_of_layer  = num_of_layer;
    d.grid_intpl_tag = intpl_tag;
    d.fill_value    = fill_value;
    d.exchange_tag  = exchange_tag;
    d.factor        = factor;
    d.offset        = offset;
    d.put_sec       = -1;

    d.my_exchange     = NULL;
    d.data1d          = NULL;
    d.data1d_tmp      = NULL;
    d.data2d          = NULL;
    d.weight2d        = NULL;
    d.exchange_buffer = NULL;
    d.exchange_target = NULL;

    return d;
}

/* -----------------------------------------------------------------------
 * data_class_set_my_exchange
 * ----------------------------------------------------------------------- */
void data_class_set_my_exchange(data_class* self,
    const char* send_comp_name, const char* send_grid_name,
    const char* recv_comp_name, const char* recv_grid_name,
    int map_tag)
{
    int i;
    char my_exchange_name[STR_SHORT];

    self->my_exchange = get_exchange_ptr_name(send_comp_name, send_grid_name,
                                              recv_comp_name, recv_grid_name, map_tag);

    exchange_class_get_my_name(self->my_exchange, my_exchange_name);
    if (strcmp(my_exchange_name, send_comp_name) == 0)
        strncpy(self->my_name, self->send_data_name, STR_SHORT - 1);
    else
        strncpy(self->my_name, self->recv_data_name, STR_SHORT - 1);

    /* Determine exchange_data_size and exchange_buffer_size */
    if (exchange_class_is_send_intpl(self->my_exchange)) {
        if (exchange_class_is_my_intpl(self->my_exchange))
            self->exchange_data_size = exchange_class_get_exchange_data_size(self->my_exchange);
        else
            self->exchange_data_size = exchange_class_get_exchange_buffer_size(self->my_exchange);
        self->exchange_buffer_size = exchange_class_get_exchange_buffer_size(self->my_exchange);
    } else {
        self->exchange_data_size   = exchange_class_get_exchange_data_size(self->my_exchange);
        self->exchange_buffer_size = exchange_class_get_exchange_buffer_size(self->my_exchange);
    }

    /* Allocate exchange_target array */
    self->num_of_target  = exchange_class_get_num_of_target_rank(self->my_exchange);
    self->exchange_target = (buffer_class*)malloc(self->num_of_target * sizeof(buffer_class));

    for (i = 0; i < self->num_of_target; i++) {
        int trank  = exchange_class_get_target_rank(self->my_exchange, i + 1);  /* 1-based */
        int tsize  = exchange_class_get_target_array_size(self->my_exchange, i + 1);
        self->exchange_target[i].target_rank  = trank;
        self->exchange_target[i].num_of_data  = tsize;
        self->exchange_target[i].num_of_layer = self->num_of_layer;
        self->exchange_target[i].buffer =
            (double*)malloc(tsize * self->num_of_layer * sizeof(double));
    }

    /* Allocate data buffers */
    if (self->num_of_layer == 1) {
        self->data1d = (double*)calloc(self->exchange_data_size, sizeof(double));
    } else {
        self->data2d = (double*)calloc(self->exchange_data_size * self->num_of_layer,
                                       sizeof(double));
    }

    if (self->avr_flag) {
        self->data1d_tmp = (double*)malloc(self->exchange_data_size * sizeof(double));
        self->weight2d   = (double*)calloc(self->exchange_data_size * self->num_of_layer,
                                           sizeof(double));
    } else {
        self->data1d_tmp = (double*)malloc(1 * sizeof(double));
        self->weight2d   = (double*)malloc(1 * sizeof(double));
    }

    {
        int eb_layers = (self->num_of_layer == 1) ? 1 : self->num_of_layer;
        self->exchange_buffer =
            (double*)calloc(self->exchange_buffer_size * eb_layers, sizeof(double));
    }
}

/* -----------------------------------------------------------------------
 * Simple getters
 * ----------------------------------------------------------------------- */
exchange_class* data_class_get_my_exchange(const data_class* self)
{
    return self->my_exchange;
}

void data_class_get_my_name(const data_class* self, char* out)
{
    strncpy(out, self->my_name, STR_SHORT - 1); out[STR_SHORT-1] = '\0';
}
void data_class_get_send_comp_name(const data_class* self, char* out)
{
    strncpy(out, self->send_comp_name, STR_SHORT - 1); out[STR_SHORT-1] = '\0';
}
void data_class_get_send_grid_name(const data_class* self, char* out)
{
    strncpy(out, self->send_grid_name, STR_SHORT - 1); out[STR_SHORT-1] = '\0';
}
void data_class_get_send_data_name(const data_class* self, char* out)
{
    strncpy(out, self->send_data_name, STR_SHORT - 1); out[STR_SHORT-1] = '\0';
}
void data_class_get_recv_comp_name(const data_class* self, char* out)
{
    strncpy(out, self->recv_comp_name, STR_SHORT - 1); out[STR_SHORT-1] = '\0';
}
void data_class_get_recv_grid_name(const data_class* self, char* out)
{
    strncpy(out, self->recv_grid_name, STR_SHORT - 1); out[STR_SHORT-1] = '\0';
}
void data_class_get_recv_data_name(const data_class* self, char* out)
{
    strncpy(out, self->recv_data_name, STR_SHORT - 1); out[STR_SHORT-1] = '\0';
}

int data_class_is_avr(const data_class* self)          { return self->avr_flag; }
int data_class_get_intvl(const data_class* self)        { return self->intvl; }
int data_class_get_time_lag(const data_class* self)     { return self->time_lag; }
int data_class_get_exchange_type(const data_class* self){ return self->exchange_type; }
int data_class_get_num_of_layer(const data_class* self) { return self->num_of_layer; }
int data_class_get_exchange_tag(const data_class* self) { return self->exchange_tag; }

int data_class_is_my_intpl(const data_class* self)
{
    return exchange_class_is_my_intpl(self->my_exchange);
}

/* -----------------------------------------------------------------------
 * data_class_put_data_1d
 * ----------------------------------------------------------------------- */
void data_class_put_data_1d(data_class* self,
    const double* data, int n_data,
    int64_t next_sec, int32_t delta_t)
{
    char log_str[STR_MID];
    int i;

    if (self->time_lag == 0) {
        snprintf(log_str, STR_MID, "  [put_data_1d ] put and send data START , data_name = %s",
                 self->my_name);
        put_log(log_str, -1);

        exchange_class_local_2_exchange(self->my_exchange, data, self->data1d);
        exchange_class_send_data_1d(self->my_exchange,
            self->data1d, self->exchange_data_size,
            self->exchange_buffer, self->exchange_buffer_size,
            self->grid_intpl_tag, self->exchange_tag);
        jml_send_waitall();

        snprintf(log_str, STR_MID, "  [put_data_1d ] put and send data END , data_name = %s",
                 self->my_name);
        put_log(log_str, -1);
        return;
    }

    if (self->put_sec == next_sec) {
        char emsg[STR_MID];
        snprintf(emsg, STR_MID, "data [%s] double put error", self->my_name);
        rjn_error("put_data_1d", emsg);
    } else {
        self->put_sec = next_sec;
    }

    snprintf(log_str, STR_MID, "  [put_data_1d ] put data START , data_name = %s", self->my_name);
    put_log(log_str, -1);

    if (self->intvl <= 0) {
        exchange_class_local_2_exchange(self->my_exchange, data, self->data1d);
        exchange_class_send_data_1d(self->my_exchange,
            self->data1d, self->exchange_data_size,
            self->exchange_buffer, self->exchange_buffer_size,
            self->grid_intpl_tag, self->exchange_tag);
    } else {
        if (self->avr_flag) {
            double weight = (delta_t == 0) ? 1.0 : (double)delta_t;

            exchange_class_local_2_exchange(self->my_exchange, data, self->data1d_tmp);

            if (delta_t == 0) {
                for (i = 0; i < self->exchange_data_size; i++) self->data1d[i] = 0.0;
                for (i = 0; i < self->exchange_data_size; i++) self->weight2d[i] = 0.0;
            }

            for (i = 0; i < self->exchange_data_size; i++) {
                if (self->data1d_tmp[i] != self->fill_value) {
                    self->data1d[i]   += self->data1d_tmp[i] * weight;
                    self->weight2d[i] += weight;
                }
            }

            if (delta_t == 0 && self->exchange_type == ADVANCE_SEND_RECV) {
                snprintf(log_str, STR_MID, "  [put_data_1d ] put data SKIP , data_name = %s",
                         self->my_name);
                put_log(log_str, -1);
                return;
            }

            if (next_sec % (int64_t)self->intvl == 0) {
                for (i = 0; i < self->exchange_data_size; i++) {
                    if (self->weight2d[i] != 0.0)
                        self->data1d[i] /= self->weight2d[i];
                }
                exchange_class_send_data_1d(self->my_exchange,
                    self->data1d, self->exchange_data_size,
                    self->exchange_buffer, self->exchange_buffer_size,
                    self->grid_intpl_tag, self->exchange_tag);
                for (i = 0; i < self->exchange_data_size; i++) self->data1d[i]   = 0.0;
                for (i = 0; i < self->exchange_data_size; i++) self->weight2d[i] = 0.0;
            }
        } else {
            /* snapshot */
            if (delta_t == 0 && self->exchange_type == ADVANCE_SEND_RECV) {
                snprintf(log_str, STR_MID, "  [put_data_1d ] put data SKIP , data_name = %s",
                         self->my_name);
                put_log(log_str, -1);
                return;
            }

            if (next_sec % (int64_t)self->intvl == 0) {
                exchange_class_local_2_exchange(self->my_exchange, data, self->data1d);
                exchange_class_send_data_1d(self->my_exchange,
                    self->data1d, self->exchange_data_size,
                    self->exchange_buffer, self->exchange_buffer_size,
                    self->grid_intpl_tag, self->exchange_tag);
            }
        }
    }

    snprintf(log_str, STR_MID, "  [put_data_1d ] put data END , data_name = %s", self->my_name);
    put_log(log_str, -1);
}

/* -----------------------------------------------------------------------
 * data_class_put_data_2d
 * data is [n1 * n2] row-major; first dimension = exchange_data_size,
 * second = num_of_layer.
 * ----------------------------------------------------------------------- */
void data_class_put_data_2d(data_class* self,
    const double* data, int n1, int n2,
    int64_t next_sec, int32_t delta_t)
{
    char log_str[STR_MID];
    int i, k;
    int num_of_layer = self->num_of_layer;

    if (self->time_lag == 0) {
        snprintf(log_str, STR_MID, "  [put_data_2d ] put and send data START , data_name = %s",
                 self->my_name);
        put_log(log_str, -1);

        for (k = 0; k < num_of_layer; k++) {
            exchange_class_local_2_exchange(self->my_exchange,
                data + k * n1,
                self->data2d + k * self->exchange_data_size);
        }
        exchange_class_send_data_2d(self->my_exchange,
            self->data2d, self->exchange_data_size, num_of_layer,
            self->exchange_target, self->num_of_target,
            num_of_layer, self->grid_intpl_tag, self->exchange_tag);
        jml_send_waitall();

        snprintf(log_str, STR_MID, "  [put_data_2d ] put and send data END , data_name = %s",
                 self->my_name);
        put_log(log_str, -1);
        return;
    }

    if (self->put_sec == next_sec) {
        char emsg[STR_MID];
        snprintf(emsg, STR_MID, "data [%s] double put error", self->my_name);
        rjn_error("put_data_2d", emsg);
    } else {
        self->put_sec = next_sec;
    }

    snprintf(log_str, STR_MID, "  [put_data_2d ] put data START , data_name = %s", self->my_name);
    put_log(log_str, -1);

    if (self->intvl <= 0) {
        for (k = 0; k < num_of_layer; k++) {
            exchange_class_local_2_exchange(self->my_exchange,
                data + k * n1,
                self->data2d + k * self->exchange_data_size);
        }
        exchange_class_send_data_2d(self->my_exchange,
            self->data2d, self->exchange_data_size, num_of_layer,
            self->exchange_target, self->num_of_target,
            num_of_layer, self->grid_intpl_tag, self->exchange_tag);
    } else {
        if (self->avr_flag) {
            double weight = (delta_t == 0) ? 1.0 : (double)delta_t;

            if (delta_t == 0) {
                int sz2d = self->exchange_data_size * num_of_layer;
                for (i = 0; i < sz2d; i++) { self->data2d[i] = 0.0; self->weight2d[i] = 0.0; }
            }

            for (k = 0; k < num_of_layer; k++) {
                exchange_class_local_2_exchange(self->my_exchange,
                    data + k * n1, self->data1d_tmp);
                for (i = 0; i < self->exchange_data_size; i++) {
                    if (self->data1d_tmp[i] != self->fill_value) {
                        self->data2d[k * self->exchange_data_size + i]   += self->data1d_tmp[i] * weight;
                        self->weight2d[k * self->exchange_data_size + i] += weight;
                    }
                }
            }

            if (delta_t == 0 && self->exchange_type == ADVANCE_SEND_RECV) {
                snprintf(log_str, STR_MID, "  [put_data_2d ] put data SKIP , data_name = %s",
                         self->my_name);
                put_log(log_str, -1);
                return;
            }

            if (next_sec % (int64_t)self->intvl == 0) {
                int sz2d = self->exchange_data_size * num_of_layer;
                for (i = 0; i < sz2d; i++) {
                    if (self->weight2d[i] != 0.0)
                        self->data2d[i] /= self->weight2d[i];
                }
                exchange_class_send_data_2d(self->my_exchange,
                    self->data2d, self->exchange_data_size, num_of_layer,
                    self->exchange_target, self->num_of_target,
                    num_of_layer, self->grid_intpl_tag, self->exchange_tag);
                for (i = 0; i < sz2d; i++) { self->data2d[i] = 0.0; self->weight2d[i] = 0.0; }
            }
        } else {
            /* snapshot */
            if (delta_t == 0 && self->exchange_type == ADVANCE_SEND_RECV) {
                snprintf(log_str, STR_MID, "  [put_data_2d ] put data SKIP , data_name = %s",
                         self->my_name);
                put_log(log_str, -1);
                return;
            }

            if (next_sec % (int64_t)self->intvl == 0) {
                for (k = 0; k < num_of_layer; k++) {
                    exchange_class_local_2_exchange(self->my_exchange,
                        data + k * n1,
                        self->data2d + k * self->exchange_data_size);
                }
                exchange_class_send_data_2d(self->my_exchange,
                    self->data2d, self->exchange_data_size, num_of_layer,
                    self->exchange_target, self->num_of_target,
                    num_of_layer, self->grid_intpl_tag, self->exchange_tag);
            }
        }
    }

    snprintf(log_str, STR_MID, "  [put_data_2d ] put data END , data_name = %s", self->my_name);
    put_log(log_str, -1);
}

/* -----------------------------------------------------------------------
 * data_class_recv_data_1d
 * ----------------------------------------------------------------------- */
void data_class_recv_data_1d(data_class* self, int64_t current_sec)
{
    char log_str[STR_MID];

    if (self->time_lag == 0) return;

    if (current_sec % (int64_t)self->intvl == 0) {
        snprintf(log_str, STR_MID,
                 "  [recv_data_1d] recv data START, data_name = %s, exchange_tag = %d",
                 self->my_name, self->exchange_tag);
        put_log(log_str, -1);

        exchange_class_recv_data_1d(self->my_exchange,
            self->exchange_buffer, self->exchange_buffer_size,
            self->exchange_tag);

        put_log("  [recv_data_1d] recv data END", -1);
    }
}

/* -----------------------------------------------------------------------
 * data_class_recv_data_2d
 * ----------------------------------------------------------------------- */
void data_class_recv_data_2d(data_class* self, int64_t current_sec)
{
    char log_str[STR_MID];

    if (self->time_lag == 0) return;

    if (current_sec % (int64_t)self->intvl == 0) {
        snprintf(log_str, STR_MID,
                 "  [recv_data_2d] recv data, data_name = %s, exchange_tag = %d",
                 self->my_name, self->exchange_tag);
        put_log(log_str, -1);

        exchange_class_recv_data_2d(self->my_exchange,
            self->exchange_target, self->num_of_target,
            self->num_of_layer, self->exchange_tag);

        put_log("  [recv_data_2d] recv data END", -1);
    }
}

/* -----------------------------------------------------------------------
 * data_class_interpolate_data_1d
 * ----------------------------------------------------------------------- */
void data_class_interpolate_data_1d(data_class* self)
{
    double* intpl_data;
    int n = self->exchange_data_size;   /* same as size(self%data1d) */
    int i;

    intpl_data = (double*)calloc(n, sizeof(double));  /* 1-layer flat [n*1] */

    if (exchange_class_is_my_intpl(self->my_exchange)) {
        /* recv-side interpolation */
        exchange_class_interpolate_data(self->my_exchange,
            self->exchange_buffer, self->exchange_buffer_size, 1,
            intpl_data, n, 1,
            1, self->grid_intpl_tag);
        for (i = 0; i < n; i++)
            self->data1d[i] = intpl_data[i];
    } else {
        /* send-side interpolation: just gather from exchange_buffer */
        exchange_class_buffer_2_recv_data(self->my_exchange,
            self->exchange_buffer, self->exchange_buffer_size, 1,
            intpl_data, n, 1);
        for (i = 0; i < n; i++)
            self->data1d[i] = intpl_data[i];
    }

    free(intpl_data);
}

/* -----------------------------------------------------------------------
 * data_class_interpolate_data_2d
 * ----------------------------------------------------------------------- */
void data_class_interpolate_data_2d(data_class* self)
{
    int num_of_layer = self->num_of_layer;
    int eb_size      = self->exchange_buffer_size;
    int ed_size      = self->exchange_data_size;
    int i;

    if (exchange_class_is_my_intpl(self->my_exchange)) {
        exchange_class_target_2_exchange_buffer(self->my_exchange,
            self->exchange_target, self->num_of_target,
            self->exchange_buffer, eb_size, num_of_layer);
        exchange_class_interpolate_data(self->my_exchange,
            self->exchange_buffer, eb_size, num_of_layer,
            self->data2d, ed_size, num_of_layer,
            num_of_layer, self->grid_intpl_tag);
    } else {
        exchange_class_target_2_exchange_buffer(self->my_exchange,
            self->exchange_target, self->num_of_target,
            self->exchange_buffer, eb_size, num_of_layer);
        for (i = 0; i < ed_size * num_of_layer; i++)
            self->data2d[i] = 0.0;
        exchange_class_buffer_2_recv_data(self->my_exchange,
            self->exchange_buffer, eb_size, num_of_layer,
            self->data2d, ed_size, num_of_layer);
    }
}

/* -----------------------------------------------------------------------
 * data_class_get_data_1d
 * ----------------------------------------------------------------------- */
void data_class_get_data_1d(data_class* self,
    double* data, int n_data,
    int64_t current_sec, int* is_get_ok)
{
    char log_str[STR_MID];
    int i;

    if (self->time_lag == 0) {
        snprintf(log_str, STR_MID,
                 "  [get_data_1d] recv and get data START, data_name = %s, exchange_tag = %d",
                 self->my_name, self->exchange_tag);
        put_log(log_str, -1);

        exchange_class_recv_data_1d(self->my_exchange,
            self->exchange_buffer, self->exchange_buffer_size,
            self->exchange_tag);
        jml_recv_waitall();

        data_class_interpolate_data_1d(self);

        for (i = 0; i < n_data; i++) data[i] = self->fill_value;
        exchange_class_exchange_2_local(self->my_exchange, self->data1d, data);
        *is_get_ok = 1;

        put_log("  [get_data_1d] recv and get data END", -1);
        return;
    }

    if (current_sec % (int64_t)self->intvl == 0) {
        for (i = 0; i < n_data; i++) data[i] = self->fill_value;
        exchange_class_exchange_2_local(self->my_exchange, self->data1d, data);
        *is_get_ok = 1;
    } else {
        put_log("  [get_data_1d ] get data SKIP", -1);
        *is_get_ok = 0;
    }
}

/* -----------------------------------------------------------------------
 * data_class_get_data_2d
 * data is [n1 * n2] row-major; first dimension = local grid size,
 * second = num_of_layer.
 * ----------------------------------------------------------------------- */
void data_class_get_data_2d(data_class* self,
    double* data, int n1, int n2,
    int64_t current_sec, int* is_get_ok)
{
    char log_str[STR_MID];
    int i, k;
    int num_of_layer = self->num_of_layer;

    if (self->time_lag == 0) {
        snprintf(log_str, STR_MID,
                 "  [get_data_2d] recv and get data START, data_name = %s, exchange_tag = %d",
                 self->my_name, self->exchange_tag);
        put_log(log_str, -1);

        exchange_class_recv_data_2d(self->my_exchange,
            self->exchange_target, self->num_of_target,
            num_of_layer, self->exchange_tag);
        jml_recv_waitall();

        data_class_interpolate_data_2d(self);

        /* Set fill value for all layers */
        for (i = 0; i < n1 * num_of_layer; i++) data[i] = self->fill_value;

        for (k = 0; k < num_of_layer; k++) {
            exchange_class_exchange_2_local(self->my_exchange,
                self->data2d + k * self->exchange_data_size,
                data + k * n1);
        }
        *is_get_ok = 1;

        put_log("  [get_data_2d] recv and get data END", -1);
        return;
    }

    if (current_sec % (int64_t)self->intvl == 0) {
        for (i = 0; i < n1 * num_of_layer; i++) data[i] = self->fill_value;
        for (k = 0; k < num_of_layer; k++) {
            exchange_class_exchange_2_local(self->my_exchange,
                self->data2d + k * self->exchange_data_size,
                data + k * n1);
        }
        *is_get_ok = 1;
    } else {
        put_log("  [get_data_2d ] get data SKIP", -1);
        *is_get_ok = 0;
    }
}
