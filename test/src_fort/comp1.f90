program comp1
  use mpi
  use constant
  use common
  implicit none
  character(len=64) :: my_name = "COMP1"
  character(len=64) :: my_grid = "COMP1_GRID"
  integer :: int_array(1)
  integer :: ierror
  integer :: i
  integer :: my_rank
  
  
  call init_common(COMP1_NAME, 1, COMP1_GRID, COMP1_NX, COMP1_NY, NUM_OF_LAYER)
  
  call cal_and_def_grid(COMP1_INDEX)

  call cal_and_set_map(COMP1_NAME, COMP1_GRID, COMP2_NAME, COMP2_GRID, &
                       COMP1_ORG, COMP1_ORG+COMP1_NX*COMP1_STRIDE, COMP1_NX, COMP1_INDEX, &
                       COMP2_ORG,   COMP2_ORG+COMP2_NX*COMP2_STRIDE,     COMP2_NX,   COMP2_INDEX)

  call cal_and_set_map(COMP2_NAME, COMP2_GRID, COMP1_NAME, COMP1_GRID, &
                       COMP2_ORG,   COMP2_ORG+COMP2_NX*COMP2_STRIDE,     COMP2_NX  , COMP2_INDEX,  &
                       COMP1_ORG, COMP1_ORG+COMP1_NX*COMP1_STRIDE, COMP1_NX, COMP1_INDEX)
  

  call set_data()
  
  call init_time(time_array)

  i = 0
  
  call put_data_1d("varc1c2", 1, i)
  !call put_data_2d("varc1c3", 1, i)

  do i = 1, COMP1_STEP
    call set_time(COMP1_NAME, time_array, COMP1_DELTA_T)
  
    call get_data_1d("varc2c1")
    !call get_data_2d("varc3c1")
    call put_data_1d("varc1c2", 1, i)
    !call put_data_2d("varc1c3", 1, i)
  end do

  call finalize_common()
  
end program comp1
