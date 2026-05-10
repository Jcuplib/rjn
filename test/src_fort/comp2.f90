program comp2
  use mpi
  use constant
  use common
  implicit none
  integer :: int_array(1)
  integer :: ierror
  integer :: i
  integer :: my_rank

  call init_common(COMP2_NAME, 2, COMP2_GRID, COMP2_NX, COMP2_NY, NUM_OF_LAYER)

  call cal_and_def_grid(COMP2_INDEX)

  call cal_and_set_map(COMP1_NAME, COMP1_GRID, COMP2_NAME, COMP2_GRID, &
                       COMP1_ORG, COMP1_ORG+COMP1_NX*COMP1_STRIDE, COMP1_NX, COMP1_INDEX, &
                       COMP2_ORG,   COMP2_ORG+COMP2_NX*COMP2_STRIDE,     COMP2_NX,   COMP2_INDEX)

  call cal_and_set_map(COMP2_NAME, COMP2_GRID, COMP1_NAME, COMP1_GRID, &
                       COMP2_ORG,   COMP2_ORG+COMP2_NX*COMP2_STRIDE,     COMP2_NX  , COMP2_INDEX, &
                       COMP1_ORG, COMP1_ORG+COMP1_NX*COMP1_STRIDE, COMP1_NX, COMP1_INDEX)


  call set_data()
  
  call init_time(time_array)

  i = 0
  
  call put_data_1d("varc2c1", 2, i)
  !call put_data_1d("varc2c3", 2, i)

  do i = 1, COMP2_STEP
    call set_time(COMP2_NAME, time_array, COMP2_DELTA_T)
  
    call get_data_1d("varc1c2")
    !call get_data_1d("varc3c2")

    call put_data_1d("varc2c1", 2, i)
    !call put_data_1d("varc2c3", 2, i)
 
  end do

  call finalize_common()
 
end program comp2
