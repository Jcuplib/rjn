#pragma once
/*
 * rjn_utils.h
 * Converted from rjn_utils.f90
 * Copyright (c) 2011, arakawa@rist.jp
 */

#include <stdint.h>
#include "rjn_constant.h"

/* Log level constants */
#define NO_OUTPUT_LOG  0
#define STANDARD_LOG   1
#define DETAIL_LOG     2

/* Stderr output constants */
#define NO_OUTPUT_STDERR  0
#define OUTPUT_STDERR     1

/* Init */
void init_utils(void);

/* File ID management */
void set_fid(int* fid);

/* Log level control */
void set_log_level(int log_l, int log_std);
int  get_log_level(void);
int  is_output_stderr(void);   /* returns 1/0 */

/* Log unit */
void set_log_unit_id(int luid);

/* Log lifecycle */
void init_log(const char* my_model_name, const char* log_dir); /* log_dir may be NULL */
void open_log_file(const char* file_name, int* log_file_unit);
void finalize_log(void);
void close_log_file(int log_file_unit);

/* Log output / error */
void put_log(const char* log_str, int log_l); /* pass -1 for log_l if not given */
void rjn_error(const char* routine_name, const char* message);
void check_argument(const char* routine_name, int my_value, int l_value, int u_value);

/* Integer <-> string conversion */
void IntegerToStr(int idata, char* buf, int buf_len);
void LongIntToStr(int64_t idata, char* buf, int buf_len);
int  StrToInt(const char* istr);

/* Date string utilities */
void IDate2CDate(int yyyy, int mo, int dd, int hh, int mm, int ss, char* cdate); /* cdate must be char[15] */
void cdate_2_idate(const char* cdate, int* yyyy, int* mo, int* dd, int* hh, int* mm, int* ss);

/* Sorting */
void sort_int_1d(int num_of_data, int* data, int* data2); /* data2 may be NULL */
void sort_str_1d(int n, char* a, int str_len, int* order); /* order may be NULL */

/* Binary search */
int binary_search_int(const int* data, int n, int key);  /* returns 0-based index or -1 */
int binary_search_str(const char* names, int n, int str_len, const char* target); /* returns 0-based index or -1 */

/* String utilities */
void split_string(const char* source_str, char reg_chr,
                  char* splited_str1, int s1_len,
                  char* splited_str2, int s2_len);
void TrimString(const char* source_str, const char* trim_str, char* result, int result_len);
int  startsWith(const char* source_str, const char* prefix); /* returns 1/0 */
void lw2up(const char* buf, char* result, int result_len);
void up2lw(const char* buf, char* result, int result_len);
int  is_comment_line(const char* data_str);                  /* returns 1/0 */
void cut_comment(const char* source_str, char* result, int result_len);
