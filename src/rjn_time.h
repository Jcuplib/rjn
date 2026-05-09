#pragma once
/*
 * rjn_time.h
 * Converted from rjn_time.f90
 * Copyright (c) 2011, arakawa@rist.jp
 */

#include <stdio.h>
#include <stdint.h>
#include "rjn_constant.h"

/* -----------------------------------------------------------------------
 * Time unit constants
 * ----------------------------------------------------------------------- */
#define TU_SEC  0  /* delta_t in seconds      */
#define TU_MIL  1  /* delta_t in milliseconds */
#define TU_MCR  2  /* delta_t in microseconds */

/* -----------------------------------------------------------------------
 * time_type: basic date/time record
 * ----------------------------------------------------------------------- */
typedef struct {
    int     yyyy;
    int     mo;
    int     dd;
    int     hh;
    int     mm;
    int64_t ss;
    int     milli_sec;
    int     micro_sec;
    int     delta_t;
} time_type;

/* -----------------------------------------------------------------------
 * model_time_type: per-domain time record
 * ----------------------------------------------------------------------- */
typedef struct {
    time_type start_time;
    time_type end_time;
    time_type current_time;
    time_type before_time;
} model_time_type;

/* -----------------------------------------------------------------------
 * model_time: per-component array of model_time_type
 * ----------------------------------------------------------------------- */
typedef struct {
    model_time_type* tm;   /* malloc'd array[num_domains] */
    int              n_tm; /* number of domains           */
} model_time;

/* -----------------------------------------------------------------------
 * Time unit control
 * ----------------------------------------------------------------------- */
void set_time_unit(int default_time_unit);
int  get_time_unit(void);

/* -----------------------------------------------------------------------
 * time_type field access
 * ----------------------------------------------------------------------- */
void set_time_data(time_type* tm, int yyyy, int mo, int dd, int hh, int mm,
                   int64_t ss, int milli_sec, int micro_sec);
void get_time_data(const time_type* tm, int* yyyy, int* mo, int* dd, int* hh, int* mm,
                   int64_t* ss, int* milli_sec, int* micro_sec);

/* -----------------------------------------------------------------------
 * Comparison functions (replace Fortran operator overloads)
 * ----------------------------------------------------------------------- */
int EqualTime(const time_type* t1, const time_type* t2);    /* 1/0 */
int NotEqualTime(const time_type* t1, const time_type* t2); /* 1/0 */
int GETime(const time_type* t1, const time_type* t2);       /* 1/0 */
int GTTime(const time_type* t1, const time_type* t2);       /* 1/0 */
int LETime(const time_type* t1, const time_type* t2);       /* 1/0 */
int LTTime(const time_type* t1, const time_type* t2);       /* 1/0 */

/* -----------------------------------------------------------------------
 * Calendar arithmetic
 * ----------------------------------------------------------------------- */
int64_t TimeToSecond(const time_type* t);
int64_t GetYearDate(int yyyy);
int64_t GetMonthDate(int yyyy, int mo);
time_type SecondToTime(int64_t scnd);

void inc_calendar(time_type* now_time, int delta_t);
void inc_time(time_type* now_time, int delta_t);
void cal_time_diff(const time_type* time1, const time_type* time2,
                   int64_t* diff_sec, int* diff_mil, int* diff_mcr);

/* -----------------------------------------------------------------------
 * Time string conversion
 * DateToTimeStr / TimeStrToDate with separate variants for by-value and
 * by-type forms.  The Fortran "overloaded" interface is split into named
 * functions.
 * ----------------------------------------------------------------------- */
/* time_str: char[21] minimum (14 + 3 + 3 + NUL)  */
void DateToTimeStr_date(char* time_str, int yyyy, int mo, int dd, int hh, int mm,
                        int64_t ss, int milli_sec, int micro_sec);
void DateToTimeStr_type(char* time_str, const time_type* date);

void TimeStrToDate_date(const char* time_str, int* yyyy, int* mo, int* dd, int* hh, int* mm,
                        int64_t* ss, int* milli_sec, int* micro_sec);
void TimeStrToDate_type(const char* time_str, time_type* date);

/* -----------------------------------------------------------------------
 * Model time lifecycle
 * mdl and dmn are 1-based (internally converted to 0-based)
 * ----------------------------------------------------------------------- */
void init_all_time(int mdl);
void init_each_time(int mdl, int dmn);
void destruct_all_time(void);

/* -----------------------------------------------------------------------
 * Start time
 * ----------------------------------------------------------------------- */
void set_start_time_str(int mdl, int dmn, const char* time_str);
void set_start_time_date(int mdl, int dmn, int yyyy, int mo, int dd, int hh, int mm,
                         int64_t ss, int milli_sec, int micro_sec);
void get_start_time_str(int mdl, int dmn, char* time_str);
void get_start_time_date(int mdl, int dmn, int* yyyy, int* mo, int* dd, int* hh, int* mm,
                         int64_t* ss, int* milli_sec, int* micro_sec);
void get_start_time_type(int mdl, int dmn, time_type* data_time);

/* -----------------------------------------------------------------------
 * End time
 * ----------------------------------------------------------------------- */
void set_end_time_str(int mdl, int dmn, const char* time_str);
void set_end_time_date(int mdl, int dmn, int yyyy, int mo, int dd, int hh, int mm,
                       int64_t ss, int milli_sec, int micro_sec);
void get_end_time_str(int mdl, int dmn, char* time_str);
void get_end_time_date(int mdl, int dmn, int* yyyy, int* mo, int* dd, int* hh, int* mm,
                       int64_t* ss, int* milli_sec, int* micro_sec);

/* -----------------------------------------------------------------------
 * Current time
 * ----------------------------------------------------------------------- */
void set_current_time_str(int mdl, int dmn, const char* time_str, int delta_t);  /* delta_t=-1 → absent */
void set_current_time_date(int mdl, int dmn, int yyyy, int mo, int dd, int hh, int mm,
                           int64_t ss, int milli_sec, int micro_sec, int delta_t);
void set_current_time_type(int mdl, int dmn, const time_type* data_time, int delta_t);

void get_current_time_str(int mdl, int dmn, char* time_str, int* delta_t);  /* delta_t may be NULL */
void get_current_time_date(int mdl, int dmn, int* yyyy, int* mo, int* dd, int* hh, int* mm,
                           int64_t* ss, int* milli_sec, int* micro_sec, int* delta_t);
void get_current_time_type(int mdl, int dmn, time_type* data_time, int* delta_t);

/* -----------------------------------------------------------------------
 * Before time
 * ----------------------------------------------------------------------- */
void get_before_time_str(int mdl, int dmn, char* time_str, int* delta_t);
void get_before_time_date(int mdl, int dmn, int* yyyy, int* mo, int* dd, int* hh, int* mm,
                          int64_t* ss, int* milli_sec, int* micro_sec, int* delta_t);
void get_before_time_type(int mdl, int dmn, time_type* data_time, int* delta_t);

/* -----------------------------------------------------------------------
 * Delta-t query
 * ----------------------------------------------------------------------- */
void get_delta_t(int mdl, int dmn, int* delta_t);

/* -----------------------------------------------------------------------
 * Exchange step queries
 * ----------------------------------------------------------------------- */
int is_before_exchange_step(int mdl, int dmn, int interval);        /* 1/0 */
int is_exchange_step(int mdl, int dmn, int interval);               /* 1/0 */
int is_exchange_step_from_c_time(int my_mdl, int my_dmn, int interval,
                                 const time_type* current_time);    /* 1/0 */
int is_next_exchange_step(int mdl, int dmn, int interval);          /* 1/0 */
void cal_next_exchange_time(int mdl, int dmn, int interval, time_type* next_time);

/* -----------------------------------------------------------------------
 * Persistence
 * ----------------------------------------------------------------------- */
void write_time(int comp_id, FILE* fp);
void read_time(int comp_id, FILE* fp);
void dump_time(FILE* fp);
void restore_time(FILE* fp);

/* -----------------------------------------------------------------------
 * Buffer helpers (used by read_time/write_time)
 * buf is int64_t[36]; is is 0-based offset
 * ----------------------------------------------------------------------- */
void set_buffer(const time_type* t, int64_t* buf, int is);
void get_buffer(time_type* t, const int64_t* buf, int is);
