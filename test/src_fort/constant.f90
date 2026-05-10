module constant
  implicit none
  private
!--------------------------------   public  ----------------------------------!
  integer, parameter, public :: STR_SHORT = 64
  integer, parameter, public :: STR_LONG  = 256

  character(len=STR_SHORT), parameter, public :: COMP1_NAME   = "COMP1"
  character(len=STR_SHORT), parameter, public :: COMP1_GRID   = "COMP1_GRID"
  integer                 , parameter, public :: COMP1_NX = 10
  integer                 , parameter, public :: COMP1_NY = 1
  integer                            , public :: COMP1_SIZE = COMP1_NX * COMP1_NY  
  real(kind=8)            , parameter, public :: COMP1_ORG = 0.d0
  real(kind=8)            , parameter, public :: COMP1_STRIDE = 1.d0
  !integer                            , public :: COMP1_INDEX(COMP1_NX) = (/1,2,3,4,5,6,7,8,9,10/)
  integer                            , public :: COMP1_INDEX(COMP1_NX) = (/10, 9, 8, 7, 6, 5, 4, 3, 2, 1/)
  character(len=STR_SHORT), parameter, public :: COMP2_NAME   = "COMP2"
  character(len=STR_SHORT), parameter, public :: COMP2_GRID   = "COMP2_GRID"
  integer                 , parameter, public :: COMP2_NX = 10
  integer                 , parameter, public :: COMP2_NY = 1
  integer                            , public :: COMP2_SIZE = COMP2_NX * COMP2_NY  
  real(kind=8)            , parameter, public :: COMP2_ORG = 0.d0
  real(kind=8)            , parameter, public :: COMP2_STRIDE = 1.d0
  integer                            , public :: COMP2_INDEX(COMP2_NX) = (/1,2,3,4,5,6,7,8,9,10/)
  character(len=STR_SHORT), parameter, public :: COMP3_NAME   = "COMP3"
  character(len=STR_SHORT), parameter, public :: COMP3_GRID   = "COMP3_GRID"
  integer                 , parameter, public :: COMP3_NX = 10
  integer                 , parameter, public :: COMP3_NY = 1
  integer                            , public :: COMP3_SIZE = COMP3_NX * COMP3_NY  
  real(kind=8)            , parameter, public :: COMP3_ORG = 0.d0
  real(kind=8)            , parameter, public :: COMP3_STRIDE = 1.d0
  integer                            , public :: COMP3_INDEX(COMP2_NX) = (/1,2,3,4,5,6,7,8,9,10/)
  integer                 , parameter, public :: NUM_OF_LAYER = 10
  
  integer, parameter, public :: COMP1_STEP    = 15
  integer, parameter, public :: COMP1_DELTA_T = 720
  integer, parameter, public :: COMP2_STEP      = 15
  integer, parameter, public :: COMP2_DELTA_T   = 720
  integer, parameter, public :: COMP3_STEP      = 1
  integer, parameter, public :: COMP3_DELTA_T   = 720

!--------------------------------   private  ---------------------------------!


end module constant
