/*
 * rjn_utils.cpp
 * Converted from rjn_utils.f90
 * Copyright (c) 2011, arakawa@rist.jp
 */

#include "rjn_utils.h"
#include "rjn_mpi_lib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <math.h>

/* -----------------------------------------------------------------------
 * Private constants
 * ----------------------------------------------------------------------- */
#define TIME_STR_LEN  20
#define STD_ERR_FD    2
#define MIN_FID       10
#define MAX_FID       999

/* -----------------------------------------------------------------------
 * Private state (module-level variables)
 * ----------------------------------------------------------------------- */
static int  s_LogUnitID   = STD_ERR_FD;
static int  s_LogFileID   = 690;
static int  s_current_record = 0;
static int  s_my_rank_global = 0;
static int  s_log_level   = NO_OUTPUT_LOG;
static int  s_max_log_level = NO_OUTPUT_LOG;
static int  s_log_stderr  = NO_OUTPUT_STDERR;
static char s_timestr[TIME_STR_LEN];
static char s_model_name[STR_MID];
static char s_comment_char[3] = "!#";

static FILE* s_log_fp = NULL;  /* log file pointer */

/* -----------------------------------------------------------------------
 * Forward declarations of private helpers
 * ----------------------------------------------------------------------- */
static void set_date_time_string(void);
static void put_log_to_stderr(int log_l, const char* LogStr);
static void put_log_to_file(int log_l, const char* LogStr);
static void write_error_message(const char* ErrorStr);
static int  is_trim_char(char s, const char* trim_str);

/* quicksort helpers */
static void quicksort_int(int* a, int left, int right, int* b);
static void quicksort_str_impl(char* x, int str_len, int* idx, int left, int right);

/* -----------------------------------------------------------------------
 * Public: init_utils
 * ----------------------------------------------------------------------- */
void init_utils(void)
{
    /* nothing to do */
}

/* -----------------------------------------------------------------------
 * Public: set_fid
 * NOTE: In Fortran, set_fid checks whether a unit number is already opened
 * via INQUIRE. In C we cannot do that without tracking open FILE*s.
 * We simply scan from MAX(fid, MIN_FID) upward; the caller is responsible
 * for not reusing numbers.  (The original logic is preserved as a comment.)
 * ----------------------------------------------------------------------- */
void set_fid(int* fid)
{
    if (*fid < MIN_FID) *fid = MIN_FID;
    /* In the Fortran code the loop checks INQUIRE(unit=fid,OPENED=op).
     * Here we just ensure fid >= MIN_FID; full tracking would require a
     * companion table.  The file-open functions below use the assigned ID. */
}

/* -----------------------------------------------------------------------
 * Public: set_log_level
 * ----------------------------------------------------------------------- */
void set_log_level(int log_l, int log_std)
{
    int m = log_l > s_log_level ? log_l : s_log_level;
    s_max_log_level = m;
    s_log_level  = log_l;
    s_log_stderr = (log_std == OUTPUT_STDERR) ? OUTPUT_STDERR : NO_OUTPUT_STDERR;

    if (s_log_level != NO_OUTPUT_LOG &&
        s_log_level != STANDARD_LOG  &&
        s_log_level != DETAIL_LOG) {
        fprintf(stderr, "log_level setting error!!, check coupler_config\n");
        fprintf(stderr, "log level is set to STANDARD_LOG\n");
        s_log_level = STANDARD_LOG;
    }
}

/* -----------------------------------------------------------------------
 * Public: get_log_level
 * ----------------------------------------------------------------------- */
int get_log_level(void)
{
    return s_log_level;
}

/* -----------------------------------------------------------------------
 * Public: is_output_stderr
 * ----------------------------------------------------------------------- */
int is_output_stderr(void)
{
    return (s_log_stderr == OUTPUT_STDERR) ? 1 : 0;
}

/* -----------------------------------------------------------------------
 * Public: set_log_unit_id
 * ----------------------------------------------------------------------- */
void set_log_unit_id(int luid)
{
    s_LogUnitID = luid;
}

/* -----------------------------------------------------------------------
 * Private: set_date_time_string
 * ----------------------------------------------------------------------- */
static void set_date_time_string(void)
{
    time_t now = time(NULL);
    struct tm* t = localtime(&now);
    snprintf(s_timestr, TIME_STR_LEN, "%04d-%02d-%02d/%02d:%02d:%02d",
             t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
             t->tm_hour, t->tm_min, t->tm_sec);
}

/* -----------------------------------------------------------------------
 * Public: init_log
 * log_dir may be NULL (defaults to ".")
 * ----------------------------------------------------------------------- */
void init_log(const char* my_model_name, const char* log_dir)
{
    char file_name[STR_MID];
    char dir_name[STR_MID];
    char pe_num[6];

    strncpy(s_model_name, my_model_name, STR_MID - 1);
    s_model_name[STR_MID - 1] = '\0';

    s_my_rank_global = jml_GetMyrankGlobal();

    if (get_log_level() != NO_OUTPUT_LOG) {
        snprintf(pe_num, sizeof(pe_num), "%05d", jml_GetMyrankGlobal());

        snprintf(file_name, STR_MID, "jlt.%s.coupling.log.PE%s",
                 s_model_name, pe_num);

        strncpy(dir_name, (log_dir != NULL) ? log_dir : ".", STR_MID - 1);
        dir_name[STR_MID - 1] = '\0';

        char full_path[STR_MID * 2];
        snprintf(full_path, sizeof(full_path), "%s/%s", dir_name, file_name);

        open_log_file(full_path, &s_LogFileID);
    }
}

/* -----------------------------------------------------------------------
 * Public: open_log_file
 * ----------------------------------------------------------------------- */
void open_log_file(const char* file_name, int* log_file_unit)
{
    set_fid(log_file_unit);

    s_log_fp = fopen(file_name, "w");
    if (s_log_fp == NULL) {
        char msg[STR_MID * 2];
        snprintf(msg, sizeof(msg), "cannot create log file: %s", file_name);
        rjn_error("open_log_file", msg);
    }
}

/* -----------------------------------------------------------------------
 * Public: finalize_log
 * ----------------------------------------------------------------------- */
void finalize_log(void)
{
    if (s_max_log_level != NO_OUTPUT_LOG) {
        close_log_file(s_LogFileID);
    }
}

/* -----------------------------------------------------------------------
 * Public: close_log_file
 * ----------------------------------------------------------------------- */
void close_log_file(int log_file_unit)
{
    (void)log_file_unit;  /* unit number not used in C; we use s_log_fp */
    if (s_log_fp != NULL) {
        fclose(s_log_fp);
        s_log_fp = NULL;
    }
}

/* -----------------------------------------------------------------------
 * Private: put_log_to_stderr
 * ----------------------------------------------------------------------- */
static void put_log_to_stderr(int log_l, const char* LogStr)
{
    if (get_log_level() == DETAIL_LOG) {
        set_date_time_string();
        fprintf(stderr, "%s : %s :: \n", s_timestr, s_model_name);
        fprintf(stderr, "%s\n", LogStr);
    } else {
        if (log_l == 1) {
            set_date_time_string();
            fprintf(stderr, "%s : %s :: \n", s_timestr, s_model_name);
            fprintf(stderr, "%s\n", LogStr);
        }
    }
}

/* -----------------------------------------------------------------------
 * Private: put_log_to_file
 * ----------------------------------------------------------------------- */
static void put_log_to_file(int log_l, const char* LogStr)
{
    if (s_log_fp == NULL) return;

    set_date_time_string();

    if (get_log_level() == DETAIL_LOG) {
        fprintf(s_log_fp, "%s :: %s\n", s_timestr, LogStr);
    } else {
        if (log_l == 1) {
            fprintf(s_log_fp, "%s :: %s\n", s_timestr, LogStr);
        }
    }
    fflush(s_log_fp);
}

/* -----------------------------------------------------------------------
 * Public: put_log
 * Pass log_l=-1 if the optional log_l argument is absent (defaults to 2).
 * ----------------------------------------------------------------------- */
void put_log(const char* log_str, int log_l)
{
    int ll = (log_l < 0) ? 2 : log_l;

    if (is_output_stderr()) {
        put_log_to_stderr(ll, log_str);
    }

    if (get_log_level() != NO_OUTPUT_LOG) {
        put_log_to_file(ll, log_str);
    }
}

/* -----------------------------------------------------------------------
 * Private: write_error_message
 * ----------------------------------------------------------------------- */
static void write_error_message(const char* ErrorStr)
{
    put_log_to_file(1, ErrorStr);

    if (jml_isRoot()) {
        fprintf(stderr, "%s\n", ErrorStr);
    }
}

/* -----------------------------------------------------------------------
 * Public: rjn_error
 * ----------------------------------------------------------------------- */
void rjn_error(const char* routine_name, const char* message)
{
    char message_str[STR_MID];

    snprintf(message_str, STR_MID,
             "!!! error !!! [RANK=%d][%s] : %s, program terminated",
             s_my_rank_global, routine_name, message);

    fprintf(stderr, "%s\n", message_str);

    write_error_message(message_str);
    close_log_file(s_LogFileID);
    jml_abort();
    exit(1);
}

/* -----------------------------------------------------------------------
 * Public: check_argument
 * ----------------------------------------------------------------------- */
void check_argument(const char* routine_name, int my_value, int l_value, int u_value)
{
    if (l_value <= my_value && my_value <= u_value) return;
    rjn_error(routine_name, "argument error");
}

/* -----------------------------------------------------------------------
 * Public: IntegerToStr
 * ----------------------------------------------------------------------- */
void IntegerToStr(int idata, char* buf, int buf_len)
{
    snprintf(buf, buf_len, "%d", idata);
}

/* -----------------------------------------------------------------------
 * Public: LongIntToStr
 * ----------------------------------------------------------------------- */
void LongIntToStr(int64_t idata, char* buf, int buf_len)
{
    snprintf(buf, buf_len, "%lld", (long long)idata);
}

/* -----------------------------------------------------------------------
 * Public: StrToInt
 * ----------------------------------------------------------------------- */
int StrToInt(const char* istr)
{
    int val;
    if (sscanf(istr, "%d", &val) != 1) {
        char msg[STR_MID];
        snprintf(msg, STR_MID, "String argument \"%s\" format error", istr);
        rjn_error("StrToInt", msg);
    }
    return val;
}

/* -----------------------------------------------------------------------
 * Public: IDate2CDate
 * cdate must point to at least 15 bytes (14 chars + NUL)
 * ----------------------------------------------------------------------- */
void IDate2CDate(int yyyy, int mo, int dd, int hh, int mm, int ss, char* cdate)
{
    snprintf(cdate, 15, "%04d%02d%02d%02d%02d%02d", yyyy, mo, dd, hh, mm, ss);
}

/* -----------------------------------------------------------------------
 * Public: cdate_2_idate
 * ----------------------------------------------------------------------- */
void cdate_2_idate(const char* cdate, int* yyyy, int* mo, int* dd,
                   int* hh, int* mm, int* ss)
{
    char tmp[5];

    /* yyyy */
    strncpy(tmp, cdate + 0, 4); tmp[4] = '\0';
    if (sscanf(tmp, "%d", yyyy) != 1) goto err;

    /* mo */
    strncpy(tmp, cdate + 4, 2); tmp[2] = '\0';
    if (sscanf(tmp, "%d", mo) != 1) goto err;

    /* dd */
    strncpy(tmp, cdate + 6, 2); tmp[2] = '\0';
    if (sscanf(tmp, "%d", dd) != 1) goto err;

    /* hh */
    strncpy(tmp, cdate + 8, 2); tmp[2] = '\0';
    if (sscanf(tmp, "%d", hh) != 1) goto err;

    /* mm */
    strncpy(tmp, cdate + 10, 2); tmp[2] = '\0';
    if (sscanf(tmp, "%d", mm) != 1) goto err;

    /* ss */
    strncpy(tmp, cdate + 12, 2); tmp[2] = '\0';
    if (sscanf(tmp, "%d", ss) != 1) goto err;

    return;

err:
    {
        char msg[STR_MID];
        snprintf(msg, STR_MID, "String argument \"%s\" format error", cdate);
        rjn_error("cdate_2_idate", msg);
    }
}

/* -----------------------------------------------------------------------
 * Quicksort for integer array (with optional companion array)
 * ----------------------------------------------------------------------- */
static void quicksort_int(int* a, int left, int right, int* b)
{
    int i, j, pivot, tmp;

    if (left >= right) return;

    i = left;
    j = right;
    pivot = a[(left + right) / 2];

    while (1) {
        while (a[i] < pivot) i++;
        while (a[j] > pivot) j--;
        if (i <= j) {
            tmp = a[i]; a[i] = a[j]; a[j] = tmp;
            if (b != NULL) { tmp = b[i]; b[i] = b[j]; b[j] = tmp; }
            i++;
            j--;
        }
        if (i > j) break;
    }

    if (left < j)  quicksort_int(a, left, j, b);
    if (i < right) quicksort_int(a, i, right, b);
}

/* -----------------------------------------------------------------------
 * Public: sort_int_1d
 * data2 may be NULL (companion array to reorder alongside data).
 * ----------------------------------------------------------------------- */
void sort_int_1d(int num_of_data, int* data, int* data2)
{
    int i, maxv;

    if (num_of_data <= 1) return;

    maxv = data[0];
    for (i = 1; i < num_of_data; i++)
        if (data[i] > maxv) maxv = data[i];
    if (maxv <= 0) return;

    quicksort_int(data, 0, num_of_data - 1, data2);
}

/* -----------------------------------------------------------------------
 * Quicksort for string array (ascending, each string has fixed stride str_len)
 * idx[] carries companion permutation indices (1-based, as Fortran order)
 * ----------------------------------------------------------------------- */
static void quicksort_str_impl(char* x, int str_len, int* idx, int left, int right)
{
    int i = left, j = right;
    int mid = (left + right) / 2;
    char* pivot = x + mid * str_len;

    while (1) {
        while (strncmp(x + i * str_len, pivot, str_len) < 0) i++;
        while (strncmp(x + j * str_len, pivot, str_len) > 0) j--;
        if (i <= j) {
            /* swap strings */
            char* si = x + i * str_len;
            char* sj = x + j * str_len;
            char tmp_s[STR_LONG];
            int  tmp_i;
            memcpy(tmp_s, si, str_len);
            memcpy(si, sj, str_len);
            memcpy(sj, tmp_s, str_len);
            if (idx != NULL) {
                tmp_i = idx[i]; idx[i] = idx[j]; idx[j] = tmp_i;
            }
            i++;
            j--;
        }
        if (i > j) break;
    }

    if (left < j)  quicksort_str_impl(x, str_len, idx, left, j);
    if (i < right) quicksort_str_impl(x, str_len, idx, i, right);
}

/* -----------------------------------------------------------------------
 * Public: sort_str_1d
 * a   : flat char array, n strings each of str_len bytes (not NUL-terminated).
 * order: optional output (0-based original indices); pass NULL to skip.
 * ----------------------------------------------------------------------- */
void sort_str_1d(int n, char* a, int str_len, int* order)
{
    int i;
    int* idx = NULL;

    if (n <= 1) return;

    idx = (int*)malloc(n * sizeof(int));
    for (i = 0; i < n; i++) idx[i] = i;

    quicksort_str_impl(a, str_len, idx, 0, n - 1);

    if (order != NULL) {
        for (i = 0; i < n; i++) order[i] = idx[i];
    }
    free(idx);
}

/* -----------------------------------------------------------------------
 * Public: binary_search_int
 * data is 0-based array of n elements, sorted ascending.
 * Returns 0-based index of key, or -1 if not found.
 * ----------------------------------------------------------------------- */
int binary_search_int(const int* data, int n, int key)
{
    int low = 0, high = n - 1, middle;

    while (low <= high) {
        middle = (low + high) / 2;
        if (key == data[middle])      return middle;
        else if (key < data[middle])  high = middle - 1;
        else                          low  = middle + 1;
    }
    return -1;
}

/* -----------------------------------------------------------------------
 * Public: binary_search_str
 * names: flat char array, n strings each of str_len bytes.
 * Returns 0-based index of target, or -1 if not found.
 * ----------------------------------------------------------------------- */
int binary_search_str(const char* names, int n, int str_len, const char* target)
{
    int left = 0, right = n - 1, mid, cmp;

    while (left <= right) {
        mid = (left + right) / 2;
        cmp = strncmp(target, names + mid * str_len, str_len);
        if (cmp == 0)      return mid;
        else if (cmp < 0)  right = mid - 1;
        else               left  = mid + 1;
    }
    return -1;
}

/* -----------------------------------------------------------------------
 * Public: split_string
 * Splits source_str at first '=' into splited_str1 and splited_str2.
 * (The reg_chr argument was ignored in the Fortran implementation; the
 *  split always happens at '='.)
 * ----------------------------------------------------------------------- */
void split_string(const char* source_str, char reg_chr,
                  char* splited_str1, int s1_len,
                  char* splited_str2, int s2_len)
{
    int i, n, str_counter;
    int is_substring = 0;
    (void)reg_chr;   /* unused, as in the Fortran original */

    memset(splited_str1, 0, s1_len);
    memset(splited_str2, 0, s2_len);

    n = (int)strlen(source_str);
    str_counter = 0;

    for (i = 0; i < n; i++) {
        if (source_str[i] == '=') {
            is_substring = 1;
            str_counter = 0;
            continue;
        }
        if (is_substring) {
            if (str_counter < s2_len - 1)
                splited_str2[str_counter] = source_str[i];
        } else {
            if (str_counter < s1_len - 1)
                splited_str1[str_counter] = source_str[i];
        }
        str_counter++;
    }

    /* trim leading/trailing whitespace (adjustl + trim) */
    {
        char tmp[STR_MID];
        int j, k, len;

        /* trim splited_str1 */
        strncpy(tmp, splited_str1, s1_len - 1);
        tmp[s1_len - 1] = '\0';
        j = 0;
        while (tmp[j] == ' ') j++;
        len = (int)strlen(tmp + j);
        while (len > 0 && tmp[j + len - 1] == ' ') len--;
        memset(splited_str1, 0, s1_len);
        strncpy(splited_str1, tmp + j, len);

        /* trim splited_str2 */
        strncpy(tmp, splited_str2, s2_len - 1);
        tmp[s2_len - 1] = '\0';
        j = 0;
        while (tmp[j] == ' ') j++;
        len = (int)strlen(tmp + j);
        while (len > 0 && tmp[j + len - 1] == ' ') len--;
        memset(splited_str2, 0, s2_len);
        strncpy(splited_str2, tmp + j, len);
    }
}

/* -----------------------------------------------------------------------
 * Private: is_trim_char
 * ----------------------------------------------------------------------- */
static int is_trim_char(char s, const char* trim_str)
{
    int i, n = (int)strlen(trim_str);
    for (i = 0; i < n; i++)
        if (s == trim_str[i]) return 1;
    return 0;
}

/* -----------------------------------------------------------------------
 * Public: TrimString
 * Strips leading and trailing characters that appear in trim_str.
 * ----------------------------------------------------------------------- */
void TrimString(const char* source_str, const char* trim_str,
                char* result, int result_len)
{
    int n = (int)strlen(source_str);
    int start = 0, end = n - 1;
    int j, k, len;
    char tmp[STR_MID];

    while (start <= end && is_trim_char(source_str[start], trim_str)) start++;
    while (end >= start && is_trim_char(source_str[end],   trim_str)) end--;

    /* copy and strip spaces */
    len = end - start + 1;
    if (len < 0) len = 0;
    if (len >= (int)sizeof(tmp)) len = (int)sizeof(tmp) - 1;
    strncpy(tmp, source_str + start, len);
    tmp[len] = '\0';

    /* additional trim of whitespace (adjustl + trim) */
    j = 0;
    while (tmp[j] == ' ') j++;
    len = (int)strlen(tmp + j);
    while (len > 0 && tmp[j + len - 1] == ' ') len--;

    memset(result, 0, result_len);
    if (len > result_len - 1) len = result_len - 1;
    strncpy(result, tmp + j, len);
}

/* -----------------------------------------------------------------------
 * Public: startsWith
 * ----------------------------------------------------------------------- */
int startsWith(const char* source_str, const char* prefix)
{
    int plen = (int)strlen(prefix);
    return (strncmp(source_str, prefix, plen) == 0) ? 1 : 0;
}

/* -----------------------------------------------------------------------
 * Public: lw2up  (lower-case → upper-case)
 * ----------------------------------------------------------------------- */
void lw2up(const char* buf, char* result, int result_len)
{
    int i, n = (int)strlen(buf);
    if (n > result_len - 1) n = result_len - 1;
    for (i = 0; i < n; i++)
        result[i] = (char)toupper((unsigned char)buf[i]);
    result[n] = '\0';
}

/* -----------------------------------------------------------------------
 * Public: up2lw  (upper-case → lower-case)
 * ----------------------------------------------------------------------- */
void up2lw(const char* buf, char* result, int result_len)
{
    int i, n = (int)strlen(buf);
    if (n > result_len - 1) n = result_len - 1;
    for (i = 0; i < n; i++)
        result[i] = (char)tolower((unsigned char)buf[i]);
    result[n] = '\0';
}

/* -----------------------------------------------------------------------
 * Public: is_comment_line
 * ----------------------------------------------------------------------- */
int is_comment_line(const char* data_str)
{
    int i, n = (int)strlen(s_comment_char);
    for (i = 0; i < n; i++) {
        if (data_str[0] == s_comment_char[i]) return 1;
    }
    return 0;
}

/* -----------------------------------------------------------------------
 * Public: cut_comment
 * Copies source_str to result, stopping at any character in comment_char.
 * ----------------------------------------------------------------------- */
void cut_comment(const char* source_str, char* result, int result_len)
{
    int i, n = (int)strlen(source_str);
    memset(result, 0, result_len);
    for (i = 0; i < n && i < result_len - 1; i++) {
        if (is_trim_char(source_str[i], s_comment_char)) break;
        result[i] = source_str[i];
    }
}
