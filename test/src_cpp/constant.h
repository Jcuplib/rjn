#pragma once

#define COMP1_NAME   "COMP1"
#define COMP1_GRID   "COMP1_GRID"
#define COMP1_NX     10
#define COMP1_NY     1
#define COMP1_ORG    0.0
#define COMP1_STRIDE 1.0

#define COMP2_NAME   "COMP2"
#define COMP2_GRID   "COMP2_GRID"
#define COMP2_NX     10
#define COMP2_NY     1
#define COMP2_ORG    0.0
#define COMP2_STRIDE 1.0

#define COMP3_NAME   "COMP3"
#define COMP3_GRID   "COMP3_GRID"
#define COMP3_NX     10
#define COMP3_NY     1
#define COMP3_ORG    0.0
#define COMP3_STRIDE 1.0

#define NUM_OF_LAYER 10

#define COMP1_STEP    15
#define COMP1_DELTA_T 720
#define COMP2_STEP    15
#define COMP2_DELTA_T 720
#define COMP3_STEP    1
#define COMP3_DELTA_T 720

/* COMP1_INDEX: indices reversed (10..1) to verify remapping */
static const int COMP1_INDEX[COMP1_NX] = {10, 9, 8, 7, 6, 5, 4, 3, 2, 1};
static const int COMP2_INDEX[COMP2_NX] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
static const int COMP3_INDEX[COMP3_NX] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
