/*
 * rjn_time.cpp
 * Converted from rjn_time.f90
 * Copyright (c) 2026, arakawa@climtech.jp
 */

#include "rjn_time.h"
#include "rjn_utils.h"
#include "rjn_mpi_lib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* -----------------------------------------------------------------------
 * Private constants
 * ----------------------------------------------------------------------- */
#define BASIC_YEAR  1200   /* mod(BASIC_YEAR,400)==0 */

static const int MONTH_DATE[12]          = {31,28,31,30,31,30,31,31,30,31,30,31};
static const int MONTH_DATE_SUM[12]      = { 0,31,59,90,120,151,181,212,243,273,304,334};
static const int MONTH_DATE_LEAP[12]     = {31,29,31,30,31,30,31,31,30,31,30,31};
static const int MONTH_DATE_SUM_LEAP[12] = { 0,31,60,91,121,152,182,213,244,274,305,335};

/* -----------------------------------------------------------------------
 * Private state (module-level)
 * ----------------------------------------------------------------------- */
static int          s_time_unit = -1;
static model_time*  s_time      = NULL;  /* array[mdl], 0-based */
static int          s_num_mdl   = 0;

/* -----------------------------------------------------------------------
 * Private helper: leap year
 * ----------------------------------------------------------------------- */
static int is_leap_year(int year)
{
    if (year % 4 != 0) return 0;
    if (year % 400 == 0) return 1;
    if (year % 100 == 0) return 0;
    return 1;
}

/* -----------------------------------------------------------------------
 * set_time_unit / get_time_unit
 * ----------------------------------------------------------------------- */
void set_time_unit(int default_time_unit)
{
    s_time_unit = default_time_unit;
}

int get_time_unit(void)
{
    return s_time_unit;
}

/* -----------------------------------------------------------------------
 * set_time_data / get_time_data
 * ----------------------------------------------------------------------- */
void set_time_data(time_type* tm, int yyyy, int mo, int dd, int hh, int mm,
                   int64_t ss, int milli_sec, int micro_sec)
{
    tm->yyyy      = yyyy;
    tm->mo        = mo;
    tm->dd        = dd;
    tm->hh        = hh;
    tm->mm        = mm;
    tm->ss        = ss;
    tm->milli_sec = milli_sec;
    tm->micro_sec = micro_sec;
}

void get_time_data(const time_type* tm, int* yyyy, int* mo, int* dd, int* hh, int* mm,
                   int64_t* ss, int* milli_sec, int* micro_sec)
{
    *yyyy = tm->yyyy;
    *mo   = tm->mo;
    *dd   = tm->dd;
    *hh   = tm->hh;
    *mm   = tm->mm;
    *ss   = tm->ss;
    if (milli_sec) *milli_sec = tm->milli_sec;
    if (micro_sec) *micro_sec = tm->micro_sec;
}

/* -----------------------------------------------------------------------
 * Comparison functions
 * ----------------------------------------------------------------------- */
int EqualTime(const time_type* t1, const time_type* t2)
{
    if (t1->yyyy      != t2->yyyy)      return 0;
    if (t1->mo        != t2->mo)        return 0;
    if (t1->dd        != t2->dd)        return 0;
    if (t1->hh        != t2->hh)        return 0;
    if (t1->mm        != t2->mm)        return 0;
    if (t1->ss        != t2->ss)        return 0;
    if (t1->milli_sec != t2->milli_sec) return 0;
    if (t1->micro_sec != t2->micro_sec) return 0;
    return 1;
}

int NotEqualTime(const time_type* t1, const time_type* t2)
{
    return !EqualTime(t1, t2);
}

int GETime(const time_type* t1, const time_type* t2)
{
#define CMP_FIELD(f) if (t1->f != t2->f) return (t1->f > t2->f)
    CMP_FIELD(yyyy);
    CMP_FIELD(mo);
    CMP_FIELD(dd);
    CMP_FIELD(hh);
    CMP_FIELD(mm);
    CMP_FIELD(ss);
    CMP_FIELD(milli_sec);
    CMP_FIELD(micro_sec);
#undef CMP_FIELD
    return 1; /* equal → GE is true */
}

int LTTime(const time_type* t1, const time_type* t2)
{
    return !GETime(t1, t2);
}

int LETime(const time_type* t1, const time_type* t2)
{
#define CMP_FIELD(f) if (t1->f != t2->f) return (t1->f < t2->f)
    CMP_FIELD(yyyy);
    CMP_FIELD(mo);
    CMP_FIELD(dd);
    CMP_FIELD(hh);
    CMP_FIELD(mm);
    CMP_FIELD(ss);
    CMP_FIELD(milli_sec);
    CMP_FIELD(micro_sec);
#undef CMP_FIELD
    return 1; /* equal → LE is true */
}

int GTTime(const time_type* t1, const time_type* t2)
{
    return !LETime(t1, t2);
}

/* -----------------------------------------------------------------------
 * Calendar arithmetic helpers
 * ----------------------------------------------------------------------- */
int64_t GetYearDate(int yyyy)
{
    int64_t dy = yyyy - BASIC_YEAR;
    if (yyyy < BASIC_YEAR) {
        char msg[STR_MID];
        snprintf(msg, STR_MID, "year %d should be >= BASIC_YEAR (%d)", yyyy, BASIC_YEAR);
        rjn_error("GetYearDate", msg);
    }
    return 365*dy + (dy+3)/4 - (dy-1)/100 + (dy-1)/400;
}

int64_t GetMonthDate(int yyyy, int mo)
{
    if (mo < 1 || mo > 12)
        rjn_error("GetMonthDate", "month should be 1 <= mo <= 12");

    if (is_leap_year(yyyy))
        return (int64_t)MONTH_DATE_SUM_LEAP[mo-1];
    else
        return (int64_t)MONTH_DATE_SUM[mo-1];
}

int64_t TimeToSecond(const time_type* t)
{
    int64_t d_sec = 60LL*60LL*24LL;
    return (GetYearDate(t->yyyy) + GetMonthDate(t->yyyy, t->mo) + t->dd - 1) * d_sec
           + t->hh * 3600LL + t->mm * 60LL + t->ss;
}

time_type SecondToTime(int64_t scnd)
{
    time_type dt;
    int64_t d_sec = 60LL*60LL*24LL;
    int64_t day, d_mod;
    int yyyy, mo, dd, hh, mm, ss;
    int64_t y_mod;

    memset(&dt, 0, sizeof(dt));

    day   = scnd / d_sec + 1;
    d_mod = scnd % d_sec;
    hh    = (int)(d_mod / 3600);
    mm    = (int)((d_mod - hh*3600) / 60);
    ss    = (int)(d_mod - hh*3600 - mm*60);

    yyyy = BASIC_YEAR + 1;
    while (day > GetYearDate(yyyy)) yyyy++;
    yyyy--;

    y_mod = day - GetYearDate(yyyy);

    for (mo = 1; mo <= 12; mo++) {
        if (y_mod <= GetMonthDate(yyyy, mo)) break;
    }
    mo--;

    if (mo > 1)
        dd = (int)(y_mod - GetMonthDate(yyyy, mo));
    else
        dd = (int)y_mod;

    dt.yyyy    = yyyy;
    dt.mo      = mo;
    dt.dd      = dd;
    dt.hh      = hh;
    dt.mm      = mm;
    dt.ss      = ss;
    dt.delta_t = 0;
    return dt;
}

void inc_calendar(time_type* now_time, int delta_t)
{
    int64_t time_sec;
    int now_milli_sec, now_micro_sec;
    time_type tmp;

    switch (s_time_unit) {
    case TU_SEC:
        time_sec = TimeToSecond(now_time) + delta_t;
        tmp = SecondToTime(time_sec);
        tmp.milli_sec = now_time->milli_sec;
        tmp.micro_sec = now_time->micro_sec;
        *now_time = tmp;
        break;
    case TU_MIL:
        time_sec = TimeToSecond(now_time);
        now_milli_sec = now_time->milli_sec;
        time_sec += (now_milli_sec + delta_t) / 1000;
        tmp = SecondToTime(time_sec);
        tmp.milli_sec = (now_milli_sec + delta_t) % 1000;
        tmp.micro_sec = now_time->micro_sec;
        *now_time = tmp;
        break;
    case TU_MCR:
        time_sec = TimeToSecond(now_time);
        now_milli_sec = now_time->milli_sec;
        now_micro_sec = now_time->micro_sec;
        time_sec += (now_milli_sec * 1000 + now_micro_sec + delta_t) / 1000000;
        tmp = SecondToTime(time_sec);
        tmp.micro_sec = (now_micro_sec + delta_t) % 1000;
        tmp.milli_sec = (now_milli_sec + (now_micro_sec + delta_t) / 1000) % 1000;
        *now_time = tmp;
        break;
    default:
        rjn_error("inc_calendar", "time_unit parameter error");
    }
}

void inc_time(time_type* now_time, int delta_t)
{
    int64_t time_sec;
    int now_milli_sec, now_micro_sec;

    switch (s_time_unit) {
    case TU_SEC:
        now_time->ss += delta_t;
        break;
    case TU_MIL:
        time_sec = now_time->ss;
        now_milli_sec = now_time->milli_sec;
        time_sec += (now_milli_sec + delta_t) / 1000;
        now_time->ss        = time_sec;
        now_time->milli_sec = (now_milli_sec + delta_t) % 1000;
        break;
    case TU_MCR:
        time_sec = now_time->ss;
        now_milli_sec = now_time->milli_sec;
        now_micro_sec = now_time->micro_sec;
        time_sec += (now_milli_sec * 1000 + now_micro_sec + delta_t) / 1000000;
        now_time->ss        = time_sec;
        now_time->micro_sec = (now_micro_sec + delta_t) % 1000;
        now_time->milli_sec = (now_milli_sec + (now_micro_sec + delta_t) / 1000) % 1000;
        break;
    default:
        rjn_error("inc_time", "time_unit parameter error");
    }
}

void cal_time_diff(const time_type* time1, const time_type* time2,
                   int64_t* diff_sec, int* diff_mil, int* diff_mcr)
{
    int64_t sec1 = time1->ss;
    int64_t sec2 = time2->ss;
    int     mcr_sec1 = time1->milli_sec * 1000 + time1->micro_sec;
    int     mcr_sec2 = time2->milli_sec * 1000 + time2->micro_sec;
    int64_t sec_diff;
    int     mcr_sec_diff;

    sec_diff = sec1 - sec2;
    if (mcr_sec2 > mcr_sec1) sec_diff--;

    *diff_sec = sec_diff;

    mcr_sec_diff = mcr_sec1 - mcr_sec2;
    if (mcr_sec_diff < 0) mcr_sec_diff = 1000000 + mcr_sec_diff;

    *diff_mil = mcr_sec_diff / 1000;
    *diff_mcr = mcr_sec_diff % 1000;
}

/* -----------------------------------------------------------------------
 * Time string conversion
 * Fortran stores ss as a 14-digit integer (yyyymmddhhmm00-like packed int).
 * The original code writes/reads just the integer ss into a 14-char string.
 * ----------------------------------------------------------------------- */
void DateToTimeStr_date(char* time_str, int yyyy, int mo, int dd, int hh, int mm,
                        int64_t ss, int milli_sec, int micro_sec)
{
    snprintf(time_str, 15, "%014lld", (long long)ss);
    /* append milli and micro if non-zero – caller must provide at least 21 bytes */
    sprintf(time_str + 14, "%03d", milli_sec);
    sprintf(time_str + 17, "%03d", micro_sec);
    (void)yyyy; (void)mo; (void)dd; (void)hh; (void)mm;
}

void DateToTimeStr_type(char* time_str, const time_type* date)
{
    snprintf(time_str, 15, "%014lld", (long long)date->ss);

    switch (s_time_unit) {
    case TU_SEC:
        time_str[14] = '\0';
        break;
    case TU_MIL:
        snprintf(time_str + 14, 4, "%03d", date->milli_sec);
        time_str[17] = '\0';
        break;
    case TU_MCR:
        snprintf(time_str + 14, 4, "%03d", date->milli_sec);
        snprintf(time_str + 17, 4, "%03d", date->micro_sec);
        time_str[20] = '\0';
        break;
    default:
        rjn_error("DateToTimeStr_type", "time_unit parameter error");
    }
}

void TimeStrToDate_date(const char* time_str, int* yyyy, int* mo, int* dd,
                        int* hh, int* mm, int64_t* ss, int* milli_sec, int* micro_sec)
{
    char buf[15];
    int slen = (int)strlen(time_str);

    *milli_sec = 0;
    *micro_sec = 0;
    *yyyy = *mo = *dd = *hh = *mm = 0;

    if (slen == 14 || slen == 17 || slen == 20) {
        strncpy(buf, time_str, 14); buf[14] = '\0';
        sscanf(buf, "%lld", (long long*)ss);
        if (slen >= 17) sscanf(time_str + 14, "%3d", milli_sec);
        if (slen >= 20) sscanf(time_str + 17, "%3d", micro_sec);
    } else {
        rjn_error("TimeStrToDate_date", "time_str length error");
    }
}

void TimeStrToDate_type(const char* time_str, time_type* date)
{
    int slen = (int)strlen(time_str);
    char buf[15];

    date->milli_sec = 0;
    date->micro_sec = 0;

    if (slen == 14 || slen == 17 || slen == 20) {
        strncpy(buf, time_str, 14); buf[14] = '\0';
        sscanf(buf, "%lld", (long long*)&date->ss);
        if (slen >= 17) sscanf(time_str + 14, "%3d", &date->milli_sec);
        if (slen >= 20) sscanf(time_str + 17, "%3d", &date->micro_sec);
    } else {
        rjn_error("TimeStrToDate_type", "time_str length error");
    }
}

/* -----------------------------------------------------------------------
 * Model time lifecycle
 * mdl and dmn are 1-based in the Fortran original; converted to 0-based.
 * ----------------------------------------------------------------------- */
void init_all_time(int mdl)
{
    if (BASIC_YEAR % 400 != 0)
        rjn_error("init_all_time", "mod(BASIC_YEAR,400) != 0");

    s_time = (model_time*)malloc(mdl * sizeof(model_time));
    if (!s_time) rjn_error("init_all_time", "data allocation error");

    s_num_mdl = mdl;
    {
        int i;
        for (i = 0; i < mdl; i++) {
            s_time[i].tm   = NULL;
            s_time[i].n_tm = 0;
        }
    }
}

void init_each_time(int mdl, int dmn)
{
    int m = mdl - 1;  /* 0-based */

    s_time[m].tm = (model_time_type*)malloc(dmn * sizeof(model_time_type));
    if (!s_time[m].tm) rjn_error("init_each_time", "data allocation error");
    s_time[m].n_tm = dmn;
    memset(s_time[m].tm, 0, dmn * sizeof(model_time_type));
}

void destruct_all_time(void)
{
    int i;
    for (i = 0; i < s_num_mdl; i++) {
        if (s_time[i].tm) { free(s_time[i].tm); s_time[i].tm = NULL; }
    }
    if (s_time) { free(s_time); s_time = NULL; }
    s_num_mdl = 0;
}

/* -----------------------------------------------------------------------
 * Start time
 * ----------------------------------------------------------------------- */
void set_start_time_str(int mdl, int dmn, const char* time_str)
{
    time_type* t = &s_time[mdl-1].tm[dmn-1].start_time;
    TimeStrToDate_type(time_str, t);
}

void set_start_time_date(int mdl, int dmn, int yyyy, int mo, int dd, int hh, int mm,
                         int64_t ss, int milli_sec, int micro_sec)
{
    set_time_data(&s_time[mdl-1].tm[dmn-1].start_time,
                  yyyy, mo, dd, hh, mm, ss, milli_sec, micro_sec);
}

void get_start_time_str(int mdl, int dmn, char* time_str)
{
    DateToTimeStr_type(time_str, &s_time[mdl-1].tm[dmn-1].start_time);
}

void get_start_time_date(int mdl, int dmn, int* yyyy, int* mo, int* dd,
                         int* hh, int* mm, int64_t* ss, int* milli_sec, int* micro_sec)
{
    get_time_data(&s_time[mdl-1].tm[dmn-1].start_time,
                  yyyy, mo, dd, hh, mm, ss, milli_sec, micro_sec);
}

void get_start_time_type(int mdl, int dmn, time_type* data_time)
{
    *data_time = s_time[mdl-1].tm[dmn-1].start_time;
}

/* -----------------------------------------------------------------------
 * End time
 * ----------------------------------------------------------------------- */
void set_end_time_str(int mdl, int dmn, const char* time_str)
{
    TimeStrToDate_type(time_str, &s_time[mdl-1].tm[dmn-1].end_time);
}

void set_end_time_date(int mdl, int dmn, int yyyy, int mo, int dd, int hh, int mm,
                       int64_t ss, int milli_sec, int micro_sec)
{
    set_time_data(&s_time[mdl-1].tm[dmn-1].end_time,
                  yyyy, mo, dd, hh, mm, ss, milli_sec, micro_sec);
}

void get_end_time_str(int mdl, int dmn, char* time_str)
{
    DateToTimeStr_type(time_str, &s_time[mdl-1].tm[dmn-1].end_time);
}

void get_end_time_date(int mdl, int dmn, int* yyyy, int* mo, int* dd,
                       int* hh, int* mm, int64_t* ss, int* milli_sec, int* micro_sec)
{
    get_time_data(&s_time[mdl-1].tm[dmn-1].end_time,
                  yyyy, mo, dd, hh, mm, ss, milli_sec, micro_sec);
}

/* -----------------------------------------------------------------------
 * Current time
 * delta_t = -1 means "not present" (Fortran optional argument absent)
 * ----------------------------------------------------------------------- */
void set_current_time_str(int mdl, int dmn, const char* time_str, int delta_t)
{
    model_time_type* mt = &s_time[mdl-1].tm[dmn-1];
    mt->before_time = mt->current_time;
    TimeStrToDate_type(time_str, &mt->current_time);
    if (delta_t >= 0) mt->current_time.delta_t = delta_t;
}

void set_current_time_date(int mdl, int dmn, int yyyy, int mo, int dd, int hh, int mm,
                           int64_t ss, int milli_sec, int micro_sec, int delta_t)
{
    model_time_type* mt = &s_time[mdl-1].tm[dmn-1];
    mt->before_time = mt->current_time;
    set_time_data(&mt->current_time, yyyy, mo, dd, hh, mm, ss, milli_sec, micro_sec);
    if (delta_t >= 0) mt->current_time.delta_t = delta_t;
}

void set_current_time_type(int mdl, int dmn, const time_type* data_time, int delta_t)
{
    model_time_type* mt = &s_time[mdl-1].tm[dmn-1];
    mt->before_time  = mt->current_time;
    mt->current_time = *data_time;
    if (delta_t >= 0) mt->current_time.delta_t = delta_t;
}

void get_current_time_str(int mdl, int dmn, char* time_str, int* delta_t)
{
    model_time_type* mt = &s_time[mdl-1].tm[dmn-1];
    DateToTimeStr_type(time_str, &mt->current_time);
    if (delta_t) *delta_t = mt->current_time.delta_t;
}

void get_current_time_date(int mdl, int dmn, int* yyyy, int* mo, int* dd,
                           int* hh, int* mm, int64_t* ss, int* milli_sec,
                           int* micro_sec, int* delta_t)
{
    model_time_type* mt = &s_time[mdl-1].tm[dmn-1];
    get_time_data(&mt->current_time, yyyy, mo, dd, hh, mm, ss, milli_sec, micro_sec);
    if (delta_t) *delta_t = mt->current_time.delta_t;
}

void get_current_time_type(int mdl, int dmn, time_type* data_time, int* delta_t)
{
    model_time_type* mt = &s_time[mdl-1].tm[dmn-1];
    *data_time = mt->current_time;
    if (delta_t) *delta_t = mt->current_time.delta_t;
}

/* -----------------------------------------------------------------------
 * Before time
 * ----------------------------------------------------------------------- */
void get_before_time_str(int mdl, int dmn, char* time_str, int* delta_t)
{
    model_time_type* mt = &s_time[mdl-1].tm[dmn-1];
    DateToTimeStr_type(time_str, &mt->before_time);
    if (delta_t) *delta_t = mt->before_time.delta_t;
}

void get_before_time_date(int mdl, int dmn, int* yyyy, int* mo, int* dd,
                          int* hh, int* mm, int64_t* ss, int* milli_sec,
                          int* micro_sec, int* delta_t)
{
    model_time_type* mt = &s_time[mdl-1].tm[dmn-1];
    get_time_data(&mt->before_time, yyyy, mo, dd, hh, mm, ss, milli_sec, micro_sec);
    if (delta_t) *delta_t = mt->before_time.delta_t;
}

void get_before_time_type(int mdl, int dmn, time_type* data_time, int* delta_t)
{
    model_time_type* mt = &s_time[mdl-1].tm[dmn-1];
    *data_time = mt->before_time;
    if (delta_t) *delta_t = mt->before_time.delta_t;
}

/* -----------------------------------------------------------------------
 * get_delta_t
 * ----------------------------------------------------------------------- */
void get_delta_t(int mdl, int dmn, int* delta_t)
{
    *delta_t = s_time[mdl-1].tm[dmn-1].current_time.delta_t;
}

/* -----------------------------------------------------------------------
 * Exchange step queries
 * ----------------------------------------------------------------------- */
int is_before_exchange_step(int mdl, int dmn, int interval)
{
    model_time_type* mt = &s_time[mdl-1].tm[dmn-1];
    int64_t diff_sec;
    int diff_mil, diff_mcr;
    int64_t time_diff;

    cal_time_diff(&mt->before_time, &mt->start_time, &diff_sec, &diff_mil, &diff_mcr);

    switch (s_time_unit) {
    case TU_SEC:
        return (diff_sec % (int64_t)interval == 0) ? 1 : 0;
    case TU_MIL:
        time_diff = diff_sec * 1000 + diff_mil;
        return (time_diff % (int64_t)interval == 0) ? 1 : 0;
    case TU_MCR:
        time_diff = diff_sec * 1000000LL + diff_mil * 1000 + diff_mcr;
        return (time_diff % (int64_t)interval == 0) ? 1 : 0;
    default:
        rjn_error("is_before_exchange_step", "time_unit parameter error");
        return 0;
    }
}

int is_exchange_step(int mdl, int dmn, int interval)
{
    model_time_type* mt = &s_time[mdl-1].tm[dmn-1];
    int64_t diff_sec;
    int diff_mil, diff_mcr;
    int64_t time_diff;

    cal_time_diff(&mt->current_time, &mt->start_time, &diff_sec, &diff_mil, &diff_mcr);

    switch (s_time_unit) {
    case TU_SEC:
        return (diff_sec % (int64_t)interval == 0) ? 1 : 0;
    case TU_MIL:
        time_diff = diff_sec * 1000 + diff_mil;
        return (time_diff % (int64_t)interval == 0) ? 1 : 0;
    case TU_MCR:
        time_diff = diff_sec * 1000000LL + diff_mil * 1000 + diff_mcr;
        return (time_diff % (int64_t)interval == 0) ? 1 : 0;
    default:
        rjn_error("is_exchange_step", "time_unit parameter error");
        return 0;
    }
}

int is_exchange_step_from_c_time(int my_mdl, int my_dmn, int interval,
                                  const time_type* current_time)
{
    int64_t diff_sec;
    int diff_mil, diff_mcr;
    int64_t time_diff;

    cal_time_diff(current_time, &s_time[my_mdl-1].tm[my_dmn-1].start_time,
                  &diff_sec, &diff_mil, &diff_mcr);

    switch (s_time_unit) {
    case TU_SEC:
        return (diff_sec % (int64_t)interval == 0) ? 1 : 0;
    case TU_MIL:
        time_diff = diff_sec * 1000 + diff_mil;
        return (time_diff % (int64_t)interval == 0) ? 1 : 0;
    case TU_MCR:
        time_diff = diff_sec * 1000000LL + diff_mil * 1000 + diff_mcr;
        return (time_diff % (int64_t)interval == 0) ? 1 : 0;
    default:
        rjn_error("is_exchange_step_from_c_time", "time_unit parameter error");
        return 0;
    }
}

int is_next_exchange_step(int mdl, int dmn, int interval)
{
    model_time_type* mt = &s_time[mdl-1].tm[dmn-1];
    int64_t diff_sec;
    int diff_mil, diff_mcr;
    int64_t time_diff;

    cal_time_diff(&mt->current_time, &mt->start_time, &diff_sec, &diff_mil, &diff_mcr);

    switch (s_time_unit) {
    case TU_SEC:
        time_diff = diff_sec + mt->current_time.delta_t;
        return (time_diff % (int64_t)interval == 0) ? 1 : 0;
    case TU_MIL:
        time_diff = diff_sec * 1000 + diff_mil + mt->current_time.delta_t;
        return (time_diff % (int64_t)interval == 0) ? 1 : 0;
    case TU_MCR:
        time_diff = diff_sec * 1000000LL + diff_mil * 1000 + diff_mcr + mt->current_time.delta_t;
        return (time_diff % (int64_t)interval == 0) ? 1 : 0;
    default:
        rjn_error("is_next_exchange_step", "time_unit parameter error");
        return 0;
    }
}

void cal_next_exchange_time(int mdl, int dmn, int interval, time_type* next_time)
{
    model_time_type* mt = &s_time[mdl-1].tm[dmn-1];
    int64_t diff_sec;
    int diff_mil, diff_mcr;
    int64_t time_diff;
    int num_of_interval;

    cal_time_diff(&mt->before_time, &mt->start_time, &diff_sec, &diff_mil, &diff_mcr);

    switch (s_time_unit) {
    case TU_SEC:
        time_diff = diff_sec;
        num_of_interval = (int)(time_diff / interval);
        *next_time = mt->start_time;
        inc_time(next_time, (num_of_interval + 1) * interval);
        break;
    case TU_MIL:
        time_diff = diff_sec * 1000 + diff_mil;
        num_of_interval = (int)(time_diff / interval);
        *next_time = mt->start_time;
        inc_time(next_time, (num_of_interval + 1) * interval);
        break;
    case TU_MCR:
        time_diff = diff_sec * 1000000LL + diff_mil * 1000 + diff_mcr;
        num_of_interval = (int)(time_diff / interval);
        *next_time = mt->start_time;
        inc_time(next_time, (num_of_interval + 1) * interval);
        break;
    default:
        rjn_error("cal_next_exchange_time", "time_unit parameter error");
    }
}

/* -----------------------------------------------------------------------
 * Persistence (text mode for write_time/read_time, binary for dump/restore)
 * The Fortran write(fid,*)/read(fid,*) are list-directed I/O on all fields
 * of the time_type record.  We serialize each field on a single line.
 * ----------------------------------------------------------------------- */

/* helper: write a single time_type as 9 integers to text file */
static void write_time_type_text(FILE* fp, const time_type* t)
{
    fprintf(fp, "%d %d %d %d %d %lld %d %d %d\n",
            t->yyyy, t->mo, t->dd, t->hh, t->mm,
            (long long)t->ss, t->milli_sec, t->micro_sec, t->delta_t);
}

/* helper: read a single time_type from text file */
static void read_time_type_text(FILE* fp, time_type* t)
{
    fscanf(fp, "%d %d %d %d %d %lld %d %d %d",
           &t->yyyy, &t->mo, &t->dd, &t->hh, &t->mm,
           (long long*)&t->ss, &t->milli_sec, &t->micro_sec, &t->delta_t);
}

void write_time(int comp_id, FILE* fp)
{
    int j, n;
    if (!jml_isLocalLeader(comp_id)) return;

    n = s_time[comp_id-1].n_tm;
    for (j = 0; j < n; j++) {
        model_time_type* mt = &s_time[comp_id-1].tm[j];
        write_time_type_text(fp, &mt->start_time);
        write_time_type_text(fp, &mt->end_time);
        write_time_type_text(fp, &mt->current_time);
        write_time_type_text(fp, &mt->before_time);
    }
}

void read_time(int comp_id, FILE* fp)
{
    int j, n;
    /* 9 fields × 4 time records = 36 int64_t values */
    int64_t time_buffer[36];

    n = s_time[comp_id-1].n_tm;

    if (jml_isLocalLeader(comp_id)) {
        for (j = 0; j < n; j++) {
            model_time_type* mt = &s_time[comp_id-1].tm[j];
            read_time_type_text(fp, &mt->start_time);
            read_time_type_text(fp, &mt->end_time);
            read_time_type_text(fp, &mt->current_time);
            read_time_type_text(fp, &mt->before_time);

            set_buffer(&mt->start_time,   time_buffer, 0);
            set_buffer(&mt->end_time,     time_buffer, 9);
            set_buffer(&mt->current_time, time_buffer, 18);
            set_buffer(&mt->before_time,  time_buffer, 27);

            /* Broadcast 36 int64_t values to all local PEs */
            jml_BcastLocal_long(comp_id, time_buffer, 1, 36, 0);
        }
    } else {
        for (j = 0; j < n; j++) {
            model_time_type* mt = &s_time[comp_id-1].tm[j];
            jml_BcastLocal_long(comp_id, time_buffer, 1, 36, 0);

            get_buffer(&mt->start_time,   time_buffer, 0);
            get_buffer(&mt->end_time,     time_buffer, 9);
            get_buffer(&mt->current_time, time_buffer, 18);
            get_buffer(&mt->before_time,  time_buffer, 27);
        }
    }
}

void dump_time(FILE* fp)
{
    int i, j;
    for (i = 0; i < s_num_mdl; i++) {
        int n = s_time[i].n_tm;
        for (j = 0; j < n; j++) {
            model_time_type* mt = &s_time[i].tm[j];
            fwrite(&mt->start_time,   sizeof(time_type), 1, fp);
            fwrite(&mt->end_time,     sizeof(time_type), 1, fp);
            fwrite(&mt->current_time, sizeof(time_type), 1, fp);
            fwrite(&mt->before_time,  sizeof(time_type), 1, fp);
        }
    }
}

void restore_time(FILE* fp)
{
    int i, j;
    char log_str[STR_MID];

    put_log("------------------------------   restore time     ----------------------------------", -1);

    for (i = 0; i < s_num_mdl; i++) {
        int n = s_time[i].n_tm;
        for (j = 0; j < n; j++) {
            model_time_type* mt = &s_time[i].tm[j];
            fread(&mt->start_time,   sizeof(time_type), 1, fp);
            fread(&mt->end_time,     sizeof(time_type), 1, fp);
            fread(&mt->current_time, sizeof(time_type), 1, fp);
            fread(&mt->before_time,  sizeof(time_type), 1, fp);

            snprintf(log_str, STR_MID, "   start   time = %lld", (long long)mt->start_time.ss);
            put_log(log_str, -1);
            snprintf(log_str, STR_MID, "   end     time = %lld", (long long)mt->end_time.ss);
            put_log(log_str, -1);
            snprintf(log_str, STR_MID, "   current time = %lld", (long long)mt->current_time.ss);
            put_log(log_str, -1);
            snprintf(log_str, STR_MID, "   before  time = %lld", (long long)mt->before_time.ss);
            put_log(log_str, -1);
        }
    }
}

/* -----------------------------------------------------------------------
 * Buffer helpers
 * is is 0-based (Fortran used 1-based; callers above already subtract 1)
 * ----------------------------------------------------------------------- */
void set_buffer(const time_type* t, int64_t* buf, int is)
{
    buf[is+0] = t->yyyy;
    buf[is+1] = t->mo;
    buf[is+2] = t->dd;
    buf[is+3] = t->hh;
    buf[is+4] = t->mm;
    buf[is+5] = t->ss;
    buf[is+6] = t->milli_sec;
    buf[is+7] = t->micro_sec;
    buf[is+8] = t->delta_t;
}

void get_buffer(time_type* t, const int64_t* buf, int is)
{
    t->yyyy      = (int)buf[is+0];
    t->mo        = (int)buf[is+1];
    t->dd        = (int)buf[is+2];
    t->hh        = (int)buf[is+3];
    t->mm        = (int)buf[is+4];
    t->ss        =      buf[is+5];
    t->milli_sec = (int)buf[is+6];
    t->micro_sec = (int)buf[is+7];
    t->delta_t   = (int)buf[is+8];
}
