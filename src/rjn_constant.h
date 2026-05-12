#pragma once
/*
 * rjn_constant.h
 * Converted from rjn_constant.f90
 * Copyright (c) 2026, arakawa@climtech.jp
 */

#include <stdint.h>

/* String length constants */
#define STR_SHORT  64
#define STR_MID    256
#define STR_LONG   1024

/* Fill value */
static const double DEFAULT_FILL_VALUE = -999.0;

/* Exchange data */
extern int NUM_OF_EXCHANGE_DATA; /* = 40, mutable */

/* String constants */
#define NO_NAME "NO_NAME_ASSIGNED"
#define CONF_FILE "coupling.conf"

/* Sentinel values */
#define NO_DATA    (-999999)
#define NO_GRID    (-999999)
#define NO_MODEL   (-999)

/* Size limits */
#define MAX_MODEL   8
#define MAX_DOMAIN  5
#define MAX_GRID    8
#define NUM_OF_EXCHANGE_GRID  3
#define FINE_NUM    99

/* Send/recv mode */
#define CONCURRENT_SEND_RECV   0
#define ADVANCE_SEND_RECV     (-1)
#define BEHIND_SEND_RECV       1
#define IMMEDIATE_SEND_RECV    2
#define ASSYNC_SEND_RECV     (-99)
#define NO_SEND_RECV       99999999

/* Component relation */
#define COMP_PARALLEL  1
#define COMP_SERIAL    2
#define COMP_SUPERSET  3
#define COMP_SUBSET    4
#define COMP_OVERLAP   5

/* MPI tags */
#define RJN_ANY_TAG   99
#define RJN_SOME_TAG  98

/* Send/recv status */
#define SEND_RECV_OK  1
#define SEND_RECV_NG  0
#define SEND_SKIP    (-1)
#define RECV_SKIP    (-1)

#define DATA_EXCHANGE_ERROR  SEND_RECV_NG
#define DATA_EXCHANGE_OK     SEND_RECV_OK

/* Date/time */
#define DATE_LEN  14

/* Exchange/data constants */
#define NUM_OF_EXCHANGE_CODE  10
#define GRID_SET_END  (-999)
#define INFO_FLAG     3
#define SEND_FLAG     0
#define RECV_FLAG     1
#define FINISH_FLAG  (-1)
#define REGRID_FLAG  (-2)
#define ABORT_FLAG   9999

/* Communicator types */
#define COUPLER_MODEL_COMM  1
#define INTRA_COUPLER_COMM  2

/* Boundary/exchange types */
#define TOP_BOUNDARY      1
#define BOTTOM_BOUNDARY   2
#define EXCHANGE_2D       1
#define EXCHANGE_3D       2
#define EXCHANGE_PARTICLE 3
#define EXCHANGE_DIRECT   4

/* Data types */
#define INT_DATA    1
#define REAL_DATA   2
#define DOUBLE_DATA 3

#define N_DATA_GRID  13

#define DATA_1D   1
#define DATA_2D   2
#define DATA_25D  25
#define DATA_3D   3
#define DATA_P    4
#define END_FLAG  (-999)

#define DEBUG_FILE_ID  800

/* Interpolation mode (20250609) */
#define INTPL_SERIAL_FAST  1
#define INTPL_SERIAL_SAFE  2
#define INTPL_PARALLEL     3
#define INTPL_USER         4
