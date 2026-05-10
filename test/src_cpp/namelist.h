#pragma once
/*
 * namelist.h
 * C conversion of namelist.f90 (coupling.conf parser)
 * Only the subset of functions used by the test programs is implemented.
 */

#define NL_STR_SHORT 64
#define NL_STR_LONG  1024

typedef struct exchange_data_type {
    char   var_put[NL_STR_SHORT];
    char   var_get[NL_STR_SHORT];
    char   var_put_vec[NL_STR_SHORT];
    char   var_get_vec[NL_STR_SHORT];
    int    is_vec;
    int    intvl;
    int    lag;
    int    num_of_layer;
    int    mapping_tag;
    int    grid_intpl_tag;
    int    time_intpl_tag;
    int    exchange_tag;
    int    is_ok;
    double fill_value;
    char   flag[4];
    double factor;
    int    range_flag;
    double data_range[2];
    struct exchange_data_type* next_ptr;
} exchange_data_type;

typedef struct {
    char   put_comp_name[NL_STR_SHORT];
    char   put_grid_name[NL_STR_SHORT];
    char   get_comp_name[NL_STR_SHORT];
    char   get_grid_name[NL_STR_SHORT];
    int    intpl_map_size;
    char   map_file_name[NL_STR_LONG];
    char   coef_file_name[NL_STR_LONG];
    int    conv_endian;
    int    mapping_tag;
    char   coupler_type[NL_STR_SHORT];
    int    num_of_exchange;
    exchange_data_type* start_ed;
    exchange_data_type* ed;
} type_ici_configure;

void read_coupler_config(const char* conf_file_name);
void read_namelist(const char* file_name);

int                 get_num_of_configuration(void);
int                 get_num_of_exchange(int conf_num);  /* 1-based conf_num */
exchange_data_type* get_ed_ptr(int conf_num, int exchange_num); /* 1-based */
const char*         get_put_comp_name(int conf_num);
const char*         get_put_grid_name(int conf_num);
const char*         get_get_comp_name(int conf_num);
const char*         get_get_grid_name(int conf_num);
