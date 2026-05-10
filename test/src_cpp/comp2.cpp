/*
 * comp2.cpp
 * C conversion of comp2.f90 - test component 2 (sender/receiver).
 */
#include "constant.h"
#include "common.h"
#include "stdlib.h"
#include "stdio.h"

int main(void) {

  printf("comp2 start\n") ;
  
    init_common(COMP2_NAME, 2, COMP2_GRID, COMP2_NX, COMP2_NY, NUM_OF_LAYER);

    cal_and_def_grid(COMP2_INDEX, COMP2_NX);

    cal_and_set_map(COMP1_NAME, COMP1_GRID, COMP2_NAME, COMP2_GRID,
                    COMP1_ORG, COMP1_ORG + COMP1_NX * COMP1_STRIDE, COMP1_NX, COMP1_INDEX,
                    COMP2_ORG, COMP2_ORG + COMP2_NX * COMP2_STRIDE, COMP2_NX, COMP2_INDEX);

    cal_and_set_map(COMP2_NAME, COMP2_GRID, COMP1_NAME, COMP1_GRID,
                    COMP2_ORG, COMP2_ORG + COMP2_NX * COMP2_STRIDE, COMP2_NX, COMP2_INDEX,
                    COMP1_ORG, COMP1_ORG + COMP1_NX * COMP1_STRIDE, COMP1_NX, COMP1_INDEX);

    set_data();
    init_time(g_time_array);

    put_data_1d("varc2c1", 2, 0);

    for (int i = 1; i <= COMP2_STEP; i++) {
        set_time(COMP2_NAME, g_time_array, COMP2_DELTA_T);
        get_data_1d("varc1c2");
        put_data_1d("varc2c1", 2, i);
    }

    finalize_common();
    return 0;
}
