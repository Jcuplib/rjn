/*
 * comp1.cpp
 * C conversion of comp1.f90 - test component 1 (sender/receiver).
 */
#include "constant.h"
#include "common.h"
#include "stdlib.h"
#include "stdio.h"

int main(void) {

  printf("comp1 start\n") ; 
    
    init_common(COMP1_NAME, 1, COMP1_GRID, COMP1_NX, COMP1_NY, NUM_OF_LAYER);

    cal_and_def_grid(COMP1_INDEX, COMP1_NX);

    cal_and_set_map(COMP1_NAME, COMP1_GRID, COMP2_NAME, COMP2_GRID,
                    COMP1_ORG, COMP1_ORG + COMP1_NX * COMP1_STRIDE, COMP1_NX, COMP1_INDEX,
                    COMP2_ORG, COMP2_ORG + COMP2_NX * COMP2_STRIDE, COMP2_NX, COMP2_INDEX);

    cal_and_set_map(COMP2_NAME, COMP2_GRID, COMP1_NAME, COMP1_GRID,
                    COMP2_ORG, COMP2_ORG + COMP2_NX * COMP2_STRIDE, COMP2_NX, COMP2_INDEX,
                    COMP1_ORG, COMP1_ORG + COMP1_NX * COMP1_STRIDE, COMP1_NX, COMP1_INDEX);

    set_data();
    init_time(g_time_array);

    put_data_1d("varc1c2", 1, 0);

    for (int i = 1; i <= COMP1_STEP; i++) {
        set_time(COMP1_NAME, g_time_array, COMP1_DELTA_T);
        get_data_1d("varc2c1");
        put_data_1d("varc1c2", 1, i);
    }

    finalize_common();
    return 0;
}
