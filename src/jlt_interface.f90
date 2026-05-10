!======================================================================
! jlt_interface.f90
! Fortran wrapper over rjn C++ coupler library.
! Provides full API compatibility with the original jlt_interface module
! so that existing Fortran callers need no source changes.
!
! Build notes:
!   - Link against the rjn library and jlt_c_helpers.o
!   - fnum() is a GNU/Intel Fortran intrinsic (Linux); it returns the
!     POSIX file-descriptor for an open Fortran I/O unit.
!   - User-supplied interpolation functions passed to
!     jlt_set_user_interpolation must carry the BIND(C) attribute.
!======================================================================
module jlt_interface
  use iso_c_binding, only : c_int, c_double, c_float, c_char, &
                             c_null_char, c_funptr, c_funloc
  implicit none
  private

  integer, parameter :: STR_SHORT = 64   ! mirrors rjn_constant.h
  integer, parameter :: STR_LONG  = 1024 ! mirrors rjn_constant.h

!=======================================================================
!  Public types
!=======================================================================
  public :: jlt_varp_type
  public :: jlt_varg_type

  type jlt_varp_type
    character(STR_SHORT) :: comp_name
    character(STR_SHORT) :: grid_name
    character(STR_SHORT) :: data_name
  end type jlt_varp_type

  type jlt_varg_type
    character(STR_SHORT) :: comp_name
    character(STR_SHORT) :: grid_name
    character(STR_SHORT) :: data_name
  end type jlt_varg_type

!=======================================================================
!  Abstract interface for user-supplied interpolation function
!  (must be BIND(C) when passed to jlt_set_user_interpolation)
!=======================================================================
  abstract interface
    subroutine interpolation_user_ifc( &
        send_data, send_rows, send_cols, &
        recv_data, recv_rows, recv_cols, &
        send_index, recv_index, coef,   &
        num_of_layer, num_of_conv, intpl_tag) bind(c)
      import c_double, c_int
      real(c_double), intent(in)    :: send_data(*)
      integer(c_int), value         :: send_rows, send_cols
      real(c_double), intent(inout) :: recv_data(*)
      integer(c_int), value         :: recv_rows, recv_cols
      integer(c_int), intent(in)    :: send_index(*), recv_index(*)
      real(c_double), intent(in)    :: coef(*)
      integer(c_int), value         :: num_of_layer, num_of_conv, intpl_tag
    end subroutine
  end interface

!=======================================================================
!  Generic interfaces
!=======================================================================
  interface jlt_put_data
    module procedure jlt_put_data_1d
    module procedure jlt_put_data_25d
    module procedure jlt_put_data_1d_ptr
    module procedure jlt_put_data_25d_ptr
  end interface jlt_put_data

  interface jlt_get_data
    module procedure jlt_get_data_1d
    module procedure jlt_get_data_25d
    module procedure jlt_get_data_1d_ptr
    module procedure jlt_get_data_25d_ptr
  end interface jlt_get_data

  interface jlt_bcast_local
    module procedure bcast_array_local_int
  end interface jlt_bcast_local

  interface jlt_send_array
    module procedure send_array_str
    module procedure send_array_int
    module procedure send_array_real
    module procedure send_array_dbl
  end interface jlt_send_array

  interface jlt_recv_array
    module procedure recv_array_str
    module procedure recv_array_int
    module procedure recv_array_real
    module procedure recv_array_dbl
  end interface jlt_recv_array

  interface jlt_is_my_component
    module procedure jlt_is_my_component_id_f
    module procedure jlt_is_my_component_name_f
  end interface jlt_is_my_component

!=======================================================================
!  Public declarations
!=======================================================================
  public :: jlt_set_world               ! subroutine (global_comm)
  public :: jlt_get_world               ! integer function ()
  public :: jlt_set_new_comp            ! subroutine (comp_name)
  public :: jlt_initialize              ! subroutine (comp_name, time_unit, log_level, log_stderr)
  public :: jlt_coupling_end            ! subroutine (time_array, is_call_mpi_finalize)
  public :: jlt_get_mpi_parameter       ! subroutine (comp_name, comm, group, size, rank)
  public :: jlt_get_myrank_global       ! integer function ()
  public :: jlt_get_num_of_component    ! integer function ()
  public :: jlt_get_component_name      ! character function (comp_id)
  public :: jlt_get_comp_num_from_name  ! integer function (comp_name)
  public :: jlt_is_my_component         ! logical function (comp_id | comp_name)
  public :: jlt_is_model_running        ! logical function (comp_name)
  public :: jlt_def_grid                ! subroutine (grid_index, comp_name, grid_name, num_of_vlayer)
  public :: jlt_end_grid_def            ! subroutine ()
  public :: jlt_def_varp                ! subroutine (varp_ptr, comp, data, grid, num_of_layer)
  public :: jlt_def_varg                ! subroutine (varg_ptr, comp, data, grid, ...)
  public :: jlt_end_var_def             ! subroutine ()
  public :: jlt_set_fill_value          ! subroutine (fill_value)  – no-op
  public :: jlt_get_fill_value          ! real(8) function ()      – returns -9999
  public :: jlt_set_mapping_table       ! subroutine (my, send_c, send_g, recv_c, recv_g, ...)
  public :: jlt_set_data                ! subroutine (send_c, send_g, send_d, recv_c, recv_g, recv_d, ...)
  public :: jlt_init_time               ! subroutine (time_array)
  public :: jlt_set_time                ! subroutine (comp_name, current_time, delta_t)
  public :: jlt_put_data                ! generic subroutine
  public :: jlt_get_data                ! generic subroutine
  public :: jlt_error                   ! subroutine (sub_name, err_str)
  public :: jlt_bcast_local             ! subroutine (my_comp, data)
  public :: jlt_send_array              ! generic subroutine
  public :: jlt_recv_array              ! generic subroutine
  public :: jlt_log                     ! subroutine (sub_name, log_str, log_level)
  public :: jlt_inc_calendar            ! subroutine (date, delta_t)
  public :: jlt_inc_time                ! subroutine (comp_name, time_array)
  public :: jlt_write_mapping_table     ! subroutine (fid)
  public :: jlt_read_mapping_table      ! subroutine (fid)
  public :: jlt_set_user_interpolation  ! subroutine (send_c, send_g, recv_c, recv_g, tag, func)

!=======================================================================
!  Private C-function interfaces (bind(c) wrappers for rjn_* functions)
!=======================================================================
  interface

    subroutine rjn_set_world_c(global_comm) bind(c, name="rjn_set_world")
      import c_int
      integer(c_int), value :: global_comm
    end subroutine

    function rjn_get_world_c() result(r) bind(c, name="rjn_get_world")
      import c_int
      integer(c_int) :: r
    end function

    subroutine rjn_set_new_comp_c(comp_name) bind(c, name="rjn_set_new_comp")
      import c_char
      character(kind=c_char), intent(in) :: comp_name(*)
    end subroutine

    subroutine rjn_initialize_c(model_name, time_unit, log_level, log_stderr) &
        bind(c, name="rjn_initialize")
      import c_char, c_int
      character(kind=c_char), intent(in) :: model_name(*), time_unit(*)
      integer(c_int), value :: log_level, log_stderr
    end subroutine

    subroutine rjn_coupling_end_c(time_array, is_call_finalize) &
        bind(c, name="rjn_coupling_end")
      import c_int
      integer(c_int), intent(in) :: time_array(*)
      integer(c_int), value :: is_call_finalize
    end subroutine

    subroutine rjn_get_mpi_parameter_c(comp_name, my_comm, my_group, my_size, my_rank) &
        bind(c, name="rjn_get_mpi_parameter")
      import c_char, c_int
      character(kind=c_char), intent(in) :: comp_name(*)
      integer(c_int), intent(out) :: my_comm, my_group, my_size, my_rank
    end subroutine

    function rjn_get_myrank_global_c() result(r) bind(c, name="rjn_get_myrank_global")
      import c_int
      integer(c_int) :: r
    end function

    function rjn_get_num_of_component_c() result(r) bind(c, name="rjn_get_num_of_component")
      import c_int
      integer(c_int) :: r
    end function

    subroutine rjn_get_component_name_c(comp_id, out) bind(c, name="rjn_get_component_name")
      import c_int, c_char
      integer(c_int), value :: comp_id
      character(kind=c_char), intent(out) :: out(*)
    end subroutine

    function rjn_get_comp_num_from_name_c(comp_name) result(r) &
        bind(c, name="rjn_get_comp_num_from_name")
      import c_char, c_int
      character(kind=c_char), intent(in) :: comp_name(*)
      integer(c_int) :: r
    end function

    function rjn_is_my_component_id_c(comp_id) result(r) &
        bind(c, name="rjn_is_my_component_id")
      import c_int
      integer(c_int), value :: comp_id
      integer(c_int) :: r
    end function

    function rjn_is_my_component_name_c(comp_name) result(r) &
        bind(c, name="rjn_is_my_component_name")
      import c_char, c_int
      character(kind=c_char), intent(in) :: comp_name(*)
      integer(c_int) :: r
    end function

    function rjn_is_model_running_c(comp_name) result(r) &
        bind(c, name="rjn_is_model_running")
      import c_char, c_int
      character(kind=c_char), intent(in) :: comp_name(*)
      integer(c_int) :: r
    end function

    subroutine rjn_def_grid_c(grid_index, n, model_name, grid_name, num_of_vgrid) &
        bind(c, name="rjn_def_grid")
      import c_int, c_char
      integer(c_int), intent(in) :: grid_index(*)
      integer(c_int), value :: n
      character(kind=c_char), intent(in) :: model_name(*), grid_name(*)
      integer(c_int), value :: num_of_vgrid
    end subroutine

    subroutine rjn_end_grid_def_c() bind(c, name="rjn_end_grid_def")
    end subroutine

    subroutine rjn_end_var_def_c() bind(c, name="rjn_end_var_def")
    end subroutine

    subroutine rjn_set_fill_value_c(fill_value) bind(c, name="rjn_set_fill_value")
      import c_double
      real(c_double), value :: fill_value
    end subroutine

    function rjn_get_fill_value_c() result(r) bind(c, name="rjn_get_fill_value")
      import c_double
      real(c_double) :: r
    end function

    subroutine rjn_set_mapping_table_c( &
        my_model_name, send_model_name, send_grid_name, &
        recv_model_name, recv_grid_name, &
        map_tag, is_recv_intpl, intpl_mode, &
        send_grid, recv_grid, coef, n) &
        bind(c, name="rjn_set_mapping_table")
      import c_char, c_int, c_double
      character(kind=c_char), intent(in) :: my_model_name(*)
      character(kind=c_char), intent(in) :: send_model_name(*), send_grid_name(*)
      character(kind=c_char), intent(in) :: recv_model_name(*), recv_grid_name(*)
      integer(c_int), value :: map_tag, is_recv_intpl
      character(kind=c_char), intent(in) :: intpl_mode(*)
      integer(c_int), intent(in) :: send_grid(*), recv_grid(*)
      real(c_double), intent(in) :: coef(*)
      integer(c_int), value :: n
    end subroutine

    subroutine rjn_set_data_c( &
        send_comp, send_grid, send_data_name, &
        recv_comp,  recv_grid,  recv_data_name, &
        map_tag, is_avr, intvl, time_lag, num_of_layer, &
        grid_intpl_tag, fill_value, exchange_tag) &
        bind(c, name="rjn_set_data")
      import c_char, c_int, c_double
      character(kind=c_char), intent(in) :: send_comp(*), send_grid(*), send_data_name(*)
      character(kind=c_char), intent(in) :: recv_comp(*),  recv_grid(*),  recv_data_name(*)
      integer(c_int), value :: map_tag, is_avr, intvl, time_lag, num_of_layer
      integer(c_int), value :: grid_intpl_tag
      real(c_double), value :: fill_value
      integer(c_int), value :: exchange_tag
    end subroutine

    subroutine rjn_init_time_c(time_array) bind(c, name="rjn_init_time")
      import c_int
      integer(c_int), intent(in) :: time_array(*)
    end subroutine

    subroutine rjn_set_time_c(comp_name, current_time, delta_t) &
        bind(c, name="rjn_set_time")
      import c_char, c_int
      character(kind=c_char), intent(in) :: comp_name(*)
      integer(c_int), intent(in) :: current_time(*)
      integer(c_int), value :: delta_t
    end subroutine

    subroutine rjn_put_data_1d_c(data_name, data, n_data) &
        bind(c, name="rjn_put_data_1d")
      import c_char, c_double, c_int
      character(kind=c_char), intent(in) :: data_name(*)
      real(c_double), intent(in) :: data(*)
      integer(c_int), value :: n_data
    end subroutine

    subroutine rjn_get_data_1d_c(data_name, data, n_data, is_recv_ok) &
        bind(c, name="rjn_get_data_1d")
      import c_char, c_double, c_int
      character(kind=c_char), intent(in) :: data_name(*)
      real(c_double), intent(inout) :: data(*)
      integer(c_int), value :: n_data
      integer(c_int), intent(out) :: is_recv_ok
    end subroutine

    subroutine rjn_put_data_25d_c(data_name, data, n1, n2) &
        bind(c, name="rjn_put_data_25d")
      import c_char, c_double, c_int
      character(kind=c_char), intent(in) :: data_name(*)
      real(c_double), intent(in) :: data(*)
      integer(c_int), value :: n1, n2
    end subroutine

    subroutine rjn_get_data_25d_c(data_name, data, n1, n2, is_recv_ok) &
        bind(c, name="rjn_get_data_25d")
      import c_char, c_double, c_int
      character(kind=c_char), intent(in) :: data_name(*)
      real(c_double), intent(inout) :: data(*)
      integer(c_int), value :: n1, n2
      integer(c_int), intent(out) :: is_recv_ok
    end subroutine

    subroutine rjn_error_ifc_c(sub_name, error_str) bind(c, name="rjn_error_ifc")
      import c_char
      character(kind=c_char), intent(in) :: sub_name(*), error_str(*)
    end subroutine

    subroutine rjn_log_c(sub_name, error_str, log_level) bind(c, name="rjn_log")
      import c_char, c_int
      character(kind=c_char), intent(in) :: sub_name(*), error_str(*)
      integer(c_int), value :: log_level
    end subroutine

    subroutine rjn_inc_calendar_c(itime, itime_n, del_t) bind(c, name="rjn_inc_calendar")
      import c_int
      integer(c_int), intent(inout) :: itime(*)
      integer(c_int), value :: itime_n, del_t
    end subroutine

    subroutine rjn_inc_time_c(comp_name, itime, itime_n) bind(c, name="rjn_inc_time")
      import c_char, c_int
      character(kind=c_char), intent(in) :: comp_name(*)
      integer(c_int), intent(inout) :: itime(*)
      integer(c_int), value :: itime_n
    end subroutine

    subroutine rjn_bcast_local_c(my_comp_name, data, n) bind(c, name="rjn_bcast_local")
      import c_char, c_int
      character(kind=c_char), intent(in) :: my_comp_name(*)
      integer(c_int), intent(inout) :: data(*)
      integer(c_int), value :: n
    end subroutine

    subroutine rjn_send_array_str_c(my_comp, recv_comp, str) &
        bind(c, name="rjn_send_array_str")
      import c_char
      character(kind=c_char), intent(in) :: my_comp(*), recv_comp(*), str(*)
    end subroutine

    subroutine rjn_recv_array_str_c(my_comp, send_comp, str, bcast_flag) &
        bind(c, name="rjn_recv_array_str")
      import c_char, c_int
      character(kind=c_char), intent(in)    :: my_comp(*), send_comp(*)
      character(kind=c_char), intent(inout) :: str(*)
      integer(c_int), value :: bcast_flag
    end subroutine

    subroutine rjn_send_array_int_c(my_comp, recv_comp, array, n) &
        bind(c, name="rjn_send_array_int")
      import c_char, c_int
      character(kind=c_char), intent(in) :: my_comp(*), recv_comp(*)
      integer(c_int), intent(in) :: array(*)
      integer(c_int), value :: n
    end subroutine

    subroutine rjn_recv_array_int_c(my_comp, send_comp, array, n, bcast_flag) &
        bind(c, name="rjn_recv_array_int")
      import c_char, c_int
      character(kind=c_char), intent(in) :: my_comp(*), send_comp(*)
      integer(c_int), intent(inout) :: array(*)
      integer(c_int), value :: n, bcast_flag
    end subroutine

    subroutine rjn_send_array_real_c(my_comp, recv_comp, array, n) &
        bind(c, name="rjn_send_array_real")
      import c_char, c_int, c_float
      character(kind=c_char), intent(in) :: my_comp(*), recv_comp(*)
      real(c_float), intent(in) :: array(*)
      integer(c_int), value :: n
    end subroutine

    subroutine rjn_recv_array_real_c(my_comp, send_comp, array, n, bcast_flag) &
        bind(c, name="rjn_recv_array_real")
      import c_char, c_int, c_float
      character(kind=c_char), intent(in) :: my_comp(*), send_comp(*)
      real(c_float), intent(inout) :: array(*)
      integer(c_int), value :: n, bcast_flag
    end subroutine

    subroutine rjn_send_array_dbl_c(my_comp, recv_comp, array, n) &
        bind(c, name="rjn_send_array_dbl")
      import c_char, c_int, c_double
      character(kind=c_char), intent(in) :: my_comp(*), recv_comp(*)
      real(c_double), intent(in) :: array(*)
      integer(c_int), value :: n
    end subroutine

    subroutine rjn_recv_array_dbl_c(my_comp, send_comp, array, n, bcast_flag) &
        bind(c, name="rjn_recv_array_dbl")
      import c_char, c_int, c_double
      character(kind=c_char), intent(in) :: my_comp(*), send_comp(*)
      real(c_double), intent(inout) :: array(*)
      integer(c_int), value :: n, bcast_flag
    end subroutine

    subroutine rjn_set_user_interpolation_c( &
        send_comp, send_grid, recv_comp, recv_grid, map_tag, user_func) &
        bind(c, name="rjn_set_user_interpolation")
      import c_char, c_int, c_funptr
      character(kind=c_char), intent(in) :: send_comp(*), send_grid(*)
      character(kind=c_char), intent(in) :: recv_comp(*), recv_grid(*)
      integer(c_int), value :: map_tag
      type(c_funptr), value :: user_func
    end subroutine

    ! Helpers in jlt_c_helpers.c
    subroutine jlt_write_mapping_table_name(fname) bind(c)
      import c_char
      character(kind=c_char), intent(in) :: fname(*)
    end subroutine

    subroutine jlt_read_mapping_table_name(fname) bind(c)
      import c_char
      character(kind=c_char), intent(in) :: fname(*)
    end subroutine

  end interface

!=======================================================================
contains
!=======================================================================

!=======+=========+=========+=========+=========+=========+=========+==

subroutine jlt_set_world(global_comm)
  integer, intent(in) :: global_comm
  call rjn_set_world_c(int(global_comm, c_int))
end subroutine jlt_set_world

!=======+=========+=========+=========+=========+=========+=========+==

integer function jlt_get_world()
  jlt_get_world = int(rjn_get_world_c())
end function jlt_get_world

!=======+=========+=========+=========+=========+=========+=========+==

subroutine jlt_set_new_comp(component_name)
  character(len=*), intent(in) :: component_name
  call rjn_set_new_comp_c(trim(component_name)//c_null_char)
end subroutine jlt_set_new_comp

!=======+=========+=========+=========+=========+=========+=========+==

subroutine jlt_initialize(model_name, default_time_unit, log_level, log_stderr)
  character(len=*),     intent(in)           :: model_name
  character(len=3),     intent(in), optional :: default_time_unit
  integer,              intent(in), optional :: log_level
  logical,              intent(in), optional :: log_stderr

  integer(c_int) :: i_log_level, i_log_stderr
  character(len=4) :: time_unit

  i_log_level  = 0_c_int
  i_log_stderr = 0_c_int
  if (present(log_level))  i_log_level  = int(log_level,  c_int)
  if (present(log_stderr)) then
    if (log_stderr) i_log_stderr = 1_c_int
  end if

  if (present(default_time_unit)) then
    time_unit = trim(default_time_unit)//c_null_char
  else
    time_unit = "SEC"//c_null_char
  end if

  call rjn_initialize_c(trim(model_name)//c_null_char, &
                         time_unit, i_log_level, i_log_stderr)
end subroutine jlt_initialize

!=======+=========+=========+=========+=========+=========+=========+==

subroutine jlt_coupling_end(time_array, isCallFinalize)
  integer, intent(in)           :: time_array(:)
  logical, intent(in), optional :: isCallFinalize

  integer(c_int) :: is_fin

  is_fin = 1_c_int
  if (present(isCallFinalize)) then
    if (.not.isCallFinalize) is_fin = 0_c_int
  end if

  call rjn_coupling_end_c(time_array, is_fin)
end subroutine jlt_coupling_end

!=======+=========+=========+=========+=========+=========+=========+==

subroutine jlt_get_mpi_parameter(comp_name, my_comm, my_group, my_size, my_rank)
  character(len=*), intent(in)  :: comp_name
  integer,          intent(out) :: my_comm, my_group, my_size, my_rank

  integer(c_int) :: c_comm, c_group, c_size, c_rank

  call rjn_get_mpi_parameter_c(trim(comp_name)//c_null_char, &
                                 c_comm, c_group, c_size, c_rank)
  my_comm  = int(c_comm)
  my_group = int(c_group)
  my_size  = int(c_size)
  my_rank  = int(c_rank)
end subroutine jlt_get_mpi_parameter

!=======+=========+=========+=========+=========+=========+=========+==

integer function jlt_get_myrank_global()
  jlt_get_myrank_global = int(rjn_get_myrank_global_c())
end function jlt_get_myrank_global

!=======+=========+=========+=========+=========+=========+=========+==

integer function jlt_get_num_of_component()
  jlt_get_num_of_component = int(rjn_get_num_of_component_c())
end function jlt_get_num_of_component

!=======+=========+=========+=========+=========+=========+=========+==

function jlt_get_component_name(comp_id) result(res)
  integer, intent(in)  :: comp_id
  character(len=STR_SHORT) :: res

  character(kind=c_char, len=1) :: c_buf(STR_SHORT+1)
  integer :: i

  call rjn_get_component_name_c(int(comp_id, c_int), c_buf)

  res = ''
  do i = 1, STR_SHORT
    if (c_buf(i) == c_null_char) exit
    res(i:i) = c_buf(i)
  end do
end function jlt_get_component_name

!=======+=========+=========+=========+=========+=========+=========+==

integer function jlt_get_comp_num_from_name(comp_name)
  character(len=*), intent(in) :: comp_name
  jlt_get_comp_num_from_name = &
      int(rjn_get_comp_num_from_name_c(trim(comp_name)//c_null_char))
end function jlt_get_comp_num_from_name

!=======+=========+=========+=========+=========+=========+=========+==

logical function jlt_is_my_component_id_f(comp_id)
  integer, intent(in) :: comp_id
  jlt_is_my_component_id_f = (rjn_is_my_component_id_c(int(comp_id, c_int)) /= 0)
end function jlt_is_my_component_id_f

logical function jlt_is_my_component_name_f(comp_name)
  character(len=*), intent(in) :: comp_name
  jlt_is_my_component_name_f = &
      (rjn_is_my_component_name_c(trim(comp_name)//c_null_char) /= 0)
end function jlt_is_my_component_name_f

!=======+=========+=========+=========+=========+=========+=========+==

logical function jlt_is_model_running(comp_name)
  character(len=*), intent(in) :: comp_name
  jlt_is_model_running = &
      (rjn_is_model_running_c(trim(comp_name)//c_null_char) /= 0)
end function jlt_is_model_running

!=======+=========+=========+=========+=========+=========+=========+==

subroutine jlt_def_grid(grid_index, model_name, grid_name, num_of_vgrid)
  integer,          intent(in)           :: grid_index(:)
  character(len=*), intent(in)           :: model_name
  character(len=*), intent(in)           :: grid_name
  integer,          intent(in), optional :: num_of_vgrid

  integer(c_int) :: nvg

  nvg = 0_c_int
  if (present(num_of_vgrid)) nvg = int(num_of_vgrid, c_int)

  call rjn_def_grid_c(grid_index, int(size(grid_index), c_int), &
                       trim(model_name)//c_null_char, &
                       trim(grid_name)//c_null_char,  &
                       nvg)
end subroutine jlt_def_grid

!=======+=========+=========+=========+=========+=========+=========+==

subroutine jlt_end_grid_def()
  call rjn_end_grid_def_c()
end subroutine jlt_end_grid_def

!=======+=========+=========+=========+=========+=========+=========+==

subroutine jlt_def_varp(varp_ptr, comp_name, data_name, grid_name, num_of_layer)
  type(jlt_varp_type), pointer :: varp_ptr
  character(len=*), intent(in) :: comp_name, data_name, grid_name
  integer,          intent(in) :: num_of_layer

  allocate(varp_ptr)
  varp_ptr%comp_name = trim(comp_name)
  varp_ptr%grid_name = trim(grid_name)
  varp_ptr%data_name = trim(data_name)
end subroutine jlt_def_varp

!=======+=========+=========+=========+=========+=========+=========+==

subroutine jlt_def_varg(varg_ptr, comp_name, data_name, grid_name, &
    num_of_data, send_model_name, send_data_name,                   &
    recv_mode, interval, time_lag, mapping_tag, exchange_tag, time_intpl_tag)
  type(jlt_varg_type), pointer         :: varg_ptr
  character(len=*), intent(in)         :: comp_name, data_name, grid_name
  integer,          intent(in), optional :: num_of_data
  character(len=*), intent(in)         :: send_model_name, send_data_name
  character(len=3), intent(in), optional :: recv_mode
  integer,          intent(in), optional :: interval, time_lag
  integer,          intent(in), optional :: mapping_tag, exchange_tag, time_intpl_tag

  allocate(varg_ptr)
  varg_ptr%comp_name = trim(comp_name)
  varg_ptr%grid_name = trim(grid_name)
  varg_ptr%data_name = trim(data_name)
end subroutine jlt_def_varg

!=======+=========+=========+=========+=========+=========+=========+==

subroutine jlt_end_var_def()
  call rjn_end_var_def_c()
end subroutine jlt_end_var_def

!=======+=========+=========+=========+=========+=========+=========+==

subroutine jlt_set_fill_value(fill_value)
  real(kind=8), intent(in) :: fill_value
  call rjn_set_fill_value_c(real(fill_value, c_double))
end subroutine jlt_set_fill_value

!=======+=========+=========+=========+=========+=========+=========+==

function jlt_get_fill_value() result(fill_value)
  real(kind=8) :: fill_value
  fill_value = real(rjn_get_fill_value_c(), kind=8)
end function jlt_get_fill_value

!=======+=========+=========+=========+=========+=========+=========+==

subroutine jlt_set_mapping_table(my_model_name,                   &
    send_model_name, send_grid_name, recv_model_name, recv_grid_name, &
    map_tag, is_recv_intpl, intpl_mode, send_grid, recv_grid, coef)
  character(len=*), intent(in) :: my_model_name
  character(len=*), intent(in) :: send_model_name, send_grid_name
  character(len=*), intent(in) :: recv_model_name, recv_grid_name
  integer,          intent(in) :: map_tag
  logical,          intent(in) :: is_recv_intpl
  character(len=*), intent(in) :: intpl_mode
  integer,          intent(in) :: send_grid(:), recv_grid(:)
  real(kind=8),     intent(in) :: coef(:)

  integer(c_int) :: i_recv_intpl

  i_recv_intpl = 0_c_int
  if (is_recv_intpl) i_recv_intpl = 1_c_int

  call rjn_set_mapping_table_c(                   &
      trim(my_model_name)//c_null_char,            &
      trim(send_model_name)//c_null_char,          &
      trim(send_grid_name)//c_null_char,           &
      trim(recv_model_name)//c_null_char,          &
      trim(recv_grid_name)//c_null_char,           &
      int(map_tag, c_int), i_recv_intpl,           &
      trim(intpl_mode)//c_null_char,               &
      send_grid, recv_grid, coef,                  &
      int(size(coef), c_int))
end subroutine jlt_set_mapping_table

!=======+=========+=========+=========+=========+=========+=========+==

subroutine jlt_set_data(send_comp, send_grid, send_data_name,  &
    recv_comp, recv_grid, recv_data_name,                       &
    map_tag, is_avr, intvl, time_lag, num_of_layer,            &
    grid_intpl_tag, fill_value, exchange_tag)
  character(len=*), intent(in) :: send_comp, send_grid, send_data_name
  character(len=*), intent(in) :: recv_comp, recv_grid, recv_data_name
  integer,          intent(in) :: map_tag
  logical,          intent(in) :: is_avr
  integer,          intent(in) :: intvl, time_lag, num_of_layer
  integer,          intent(in) :: grid_intpl_tag
  real(kind=8),     intent(in) :: fill_value
  integer,          intent(in) :: exchange_tag
  integer(c_int) :: i_is_avr

  i_is_avr = 0_c_int
  if (is_avr) i_is_avr = 1_c_int
  call rjn_set_data_c(                       &
      trim(send_comp)//c_null_char,           &
      trim(send_grid)//c_null_char,           &
      trim(send_data_name)//c_null_char,      &
      trim(recv_comp)//c_null_char,           &
      trim(recv_grid)//c_null_char,           &
      trim(recv_data_name)//c_null_char,      &
      int(map_tag,        c_int),             &
      i_is_avr,                               &
      int(intvl,          c_int),             &
      int(time_lag,       c_int),             &
      int(num_of_layer,   c_int),             &
      int(grid_intpl_tag, c_int),             &
      real(fill_value,    c_double),          &
      int(exchange_tag,   c_int))
end subroutine jlt_set_data

!=======+=========+=========+=========+=========+=========+=========+==

subroutine jlt_init_time(time_array)
  integer, intent(in) :: time_array(6)
  call rjn_init_time_c(time_array)
end subroutine jlt_init_time

!=======+=========+=========+=========+=========+=========+=========+==

subroutine jlt_set_time(comp_name, current_time, delta_t)
  character(len=*), intent(in) :: comp_name
  integer,          intent(in) :: current_time(6)
  integer,          intent(in) :: delta_t

  call rjn_set_time_c(trim(comp_name)//c_null_char, &
                       current_time, int(delta_t, c_int))
end subroutine jlt_set_time

!=======+=========+=========+=========+=========+=========+=========+==
!  jlt_put_data  (four specific procedures)
!=======+=========+=========+=========+=========+=========+=========+==

subroutine jlt_put_data_1d(data_name, data, data_vector)
  character(len=*),     intent(in)           :: data_name
  real(kind=8),         intent(in)           :: data(:)
  real(kind=8),         intent(in), optional :: data_vector(:)

  call rjn_put_data_1d_c(trim(data_name)//c_null_char, &
                           data, int(size(data), c_int))
end subroutine jlt_put_data_1d

subroutine jlt_put_data_1d_ptr(varp_ptr, data, data_vector)
  type(jlt_varp_type), pointer              :: varp_ptr
  real(kind=8),         intent(in)           :: data(:)
  real(kind=8),         intent(in), optional :: data_vector(:)

  call rjn_put_data_1d_c(trim(varp_ptr%data_name)//c_null_char, &
                           data, int(size(data), c_int))
end subroutine jlt_put_data_1d_ptr

subroutine jlt_put_data_25d(data_name, data, data_vector)
  character(len=*),     intent(in)           :: data_name
  real(kind=8),         intent(in)           :: data(:,:)
  real(kind=8),         intent(in), optional :: data_vector(:)

  call rjn_put_data_25d_c(trim(data_name)//c_null_char, &
                            data, int(size(data,1), c_int), int(size(data,2), c_int))
end subroutine jlt_put_data_25d

subroutine jlt_put_data_25d_ptr(varp_ptr, data, data_vector)
  type(jlt_varp_type), pointer              :: varp_ptr
  real(kind=8),         intent(in)           :: data(:,:)
  real(kind=8),         intent(in), optional :: data_vector(:)

  call rjn_put_data_25d_c(trim(varp_ptr%data_name)//c_null_char, &
                            data, int(size(data,1), c_int), int(size(data,2), c_int))
end subroutine jlt_put_data_25d_ptr

!=======+=========+=========+=========+=========+=========+=========+==
!  jlt_get_data  (four specific procedures)
!=======+=========+=========+=========+=========+=========+=========+==

subroutine jlt_get_data_1d(data_name, data, data_vector, is_recv_ok)
  character(len=*),     intent(in)              :: data_name
  real(kind=8),         intent(inout)           :: data(:)
  real(kind=8),         intent(inout), optional :: data_vector(:)
  logical,              intent(inout)           :: is_recv_ok

  integer(c_int) :: i_ok

  call rjn_get_data_1d_c(trim(data_name)//c_null_char, &
                           data, int(size(data), c_int), i_ok)
  is_recv_ok = (i_ok /= 0)
end subroutine jlt_get_data_1d

subroutine jlt_get_data_1d_ptr(varg_ptr, data, data_vector, is_recv_ok)
  type(jlt_varg_type), pointer               :: varg_ptr
  real(kind=8),         intent(inout)        :: data(:)
  real(kind=8),         intent(inout), optional :: data_vector(:)
  logical,              intent(inout)        :: is_recv_ok

  integer(c_int) :: i_ok

  call rjn_get_data_1d_c(trim(varg_ptr%data_name)//c_null_char, &
                           data, int(size(data), c_int), i_ok)
  is_recv_ok = (i_ok /= 0)
end subroutine jlt_get_data_1d_ptr

subroutine jlt_get_data_25d(data_name, data, data_vector, is_recv_ok)
  character(len=*),     intent(in)              :: data_name
  real(kind=8),         intent(inout)           :: data(:,:)
  real(kind=8),         intent(inout), optional :: data_vector(:)
  logical,              intent(inout)           :: is_recv_ok

  integer(c_int) :: i_ok

  call rjn_get_data_25d_c(trim(data_name)//c_null_char, &
                            data, int(size(data,1), c_int), int(size(data,2), c_int), i_ok)
  is_recv_ok = (i_ok /= 0)
end subroutine jlt_get_data_25d

subroutine jlt_get_data_25d_ptr(varg_ptr, data, data_vector, is_recv_ok)
  type(jlt_varg_type), pointer               :: varg_ptr
  real(kind=8),         intent(inout)        :: data(:,:)
  real(kind=8),         intent(inout), optional :: data_vector(:)
  logical,              intent(inout)        :: is_recv_ok

  integer(c_int) :: i_ok

  call rjn_get_data_25d_c(trim(varg_ptr%data_name)//c_null_char, &
                            data, int(size(data,1), c_int), int(size(data,2), c_int), i_ok)
  is_recv_ok = (i_ok /= 0)
end subroutine jlt_get_data_25d_ptr

!=======+=========+=========+=========+=========+=========+=========+==

subroutine jlt_error(sub_name, error_str)
  character(len=*), intent(in) :: sub_name, error_str
  call rjn_error_ifc_c(trim(sub_name)//c_null_char, trim(error_str)//c_null_char)
end subroutine jlt_error

!=======+=========+=========+=========+=========+=========+=========+==

subroutine jlt_log(sub_name, error_str, log_level)
  character(len=*), intent(in)           :: sub_name, error_str
  integer,          intent(in), optional :: log_level

  integer(c_int) :: ll

  ll = 2_c_int
  if (present(log_level)) ll = int(log_level, c_int)

  call rjn_log_c(trim(sub_name)//c_null_char, trim(error_str)//c_null_char, ll)
end subroutine jlt_log

!=======+=========+=========+=========+=========+=========+=========+==

subroutine bcast_array_local_int(my_comp_name, data)
  character(len=*), intent(in)    :: my_comp_name
  integer,          intent(inout) :: data(:)

  call rjn_bcast_local_c(trim(my_comp_name)//c_null_char, &
                           data, int(size(data), c_int))
end subroutine bcast_array_local_int

!=======+=========+=========+=========+=========+=========+=========+==
!  jlt_send_array / jlt_recv_array
!=======+=========+=========+=========+=========+=========+=========+==

subroutine send_array_str(my_comp_name, recv_comp_name, array)
  character(len=*), intent(in) :: my_comp_name, recv_comp_name, array
  call rjn_send_array_str_c(trim(my_comp_name)//c_null_char,  &
                              trim(recv_comp_name)//c_null_char, &
                              trim(array)//c_null_char)
end subroutine send_array_str

subroutine recv_array_str(my_comp_name, send_comp_name, array, bcast_flag)
  character(len=*), intent(in)    :: my_comp_name, send_comp_name
  character(len=*), intent(inout) :: array
  logical,          intent(in), optional :: bcast_flag

  character(kind=c_char, len=1) :: c_buf(STR_LONG+1)
  integer(c_int) :: ibcast
  integer :: i

  ibcast = 1_c_int
  if (present(bcast_flag)) then
    if (.not.bcast_flag) ibcast = 0_c_int
  end if

  call rjn_recv_array_str_c(trim(my_comp_name)//c_null_char,  &
                              trim(send_comp_name)//c_null_char, &
                              c_buf, ibcast)
  array = ''
  do i = 1, min(len(array), STR_LONG)
    if (c_buf(i) == c_null_char) exit
    array(i:i) = c_buf(i)
  end do
end subroutine recv_array_str

subroutine send_array_int(my_comp_name, recv_comp_name, array)
  character(len=*), intent(in) :: my_comp_name, recv_comp_name
  integer,          intent(in) :: array(:)
  call rjn_send_array_int_c(trim(my_comp_name)//c_null_char,  &
                              trim(recv_comp_name)//c_null_char, &
                              array, int(size(array), c_int))
end subroutine send_array_int

subroutine recv_array_int(my_comp_name, send_comp_name, array, bcast_flag)
  character(len=*), intent(in)    :: my_comp_name, send_comp_name
  integer,          intent(inout) :: array(:)
  logical,          intent(in), optional :: bcast_flag

  integer(c_int) :: ibcast

  ibcast = 1_c_int
  if (present(bcast_flag)) then
    if (.not.bcast_flag) ibcast = 0_c_int
  end if

  call rjn_recv_array_int_c(trim(my_comp_name)//c_null_char,  &
                              trim(send_comp_name)//c_null_char, &
                              array, int(size(array), c_int), ibcast)
end subroutine recv_array_int

subroutine send_array_real(my_comp_name, recv_comp_name, array)
  character(len=*), intent(in) :: my_comp_name, recv_comp_name
  real(kind=4),     intent(in) :: array(:)
  call rjn_send_array_real_c(trim(my_comp_name)//c_null_char,  &
                               trim(recv_comp_name)//c_null_char, &
                               array, int(size(array), c_int))
end subroutine send_array_real

subroutine recv_array_real(my_comp_name, send_comp_name, array, bcast_flag)
  character(len=*), intent(in)    :: my_comp_name, send_comp_name
  real(kind=4),     intent(inout) :: array(:)
  logical,          intent(in), optional :: bcast_flag

  integer(c_int) :: ibcast

  ibcast = 1_c_int
  if (present(bcast_flag)) then
    if (.not.bcast_flag) ibcast = 0_c_int
  end if

  call rjn_recv_array_real_c(trim(my_comp_name)//c_null_char,  &
                               trim(send_comp_name)//c_null_char, &
                               array, int(size(array), c_int), ibcast)
end subroutine recv_array_real

subroutine send_array_dbl(my_comp_name, recv_comp_name, array)
  character(len=*), intent(in) :: my_comp_name, recv_comp_name
  real(kind=8),     intent(in) :: array(:)
  call rjn_send_array_dbl_c(trim(my_comp_name)//c_null_char,  &
                              trim(recv_comp_name)//c_null_char, &
                              array, int(size(array), c_int))
end subroutine send_array_dbl

subroutine recv_array_dbl(my_comp_name, send_comp_name, array, bcast_flag)
  character(len=*), intent(in)    :: my_comp_name, send_comp_name
  real(kind=8),     intent(inout) :: array(:)
  logical,          intent(in), optional :: bcast_flag

  integer(c_int) :: ibcast

  ibcast = 1_c_int
  if (present(bcast_flag)) then
    if (.not.bcast_flag) ibcast = 0_c_int
  end if

  call rjn_recv_array_dbl_c(trim(my_comp_name)//c_null_char,  &
                              trim(send_comp_name)//c_null_char, &
                              array, int(size(array), c_int), ibcast)
end subroutine recv_array_dbl

!=======+=========+=========+=========+=========+=========+=========+==

subroutine jlt_inc_calendar(itime, del_t)
  integer, intent(inout) :: itime(:)
  integer, intent(in)    :: del_t
  call rjn_inc_calendar_c(itime, int(size(itime), c_int), int(del_t, c_int))
end subroutine jlt_inc_calendar

!=======+=========+=========+=========+=========+=========+=========+==

subroutine jlt_inc_time(component_name, itime)
  character(len=*), intent(in)    :: component_name
  integer,          intent(inout) :: itime(:)
  call rjn_inc_time_c(trim(component_name)//c_null_char, &
                       itime, int(size(itime), c_int))
end subroutine jlt_inc_time

!=======+=========+=========+=========+=========+=========+=========+==
!  jlt_write_mapping_table / jlt_read_mapping_table
!
!  The C backend uses raw binary I/O (fwrite/fread) rather than Fortran
!  record-format I/O.  The Fortran unit should be opened with
!  form='unformatted', access='stream' for portable binary exchange.
!
!  fnum() is a GNU/Intel Fortran intrinsic returning the POSIX fd for an
!  open unit; it is available on all supported Linux toolchains.
!=======+=========+=========+=========+=========+=========+=========+==

subroutine jlt_write_mapping_table(fid)
  integer, intent(in) :: fid
  character(len=STR_LONG) :: fname
  flush(fid)
  inquire(unit=fid, name=fname)
  call jlt_write_mapping_table_name(trim(fname)//c_null_char)
end subroutine jlt_write_mapping_table

subroutine jlt_read_mapping_table(fid)
  integer, intent(in) :: fid
  character(len=STR_LONG) :: fname
  inquire(unit=fid, name=fname)
  call jlt_read_mapping_table_name(trim(fname)//c_null_char)
end subroutine jlt_read_mapping_table

!=======+=========+=========+=========+=========+=========+=========+==
!  jlt_set_user_interpolation
!
!  The user-supplied procedure must carry BIND(C) and match the
!  interpolation_user_ifc abstract interface declared in this module.
!=======+=========+=========+=========+=========+=========+=========+==

subroutine jlt_set_user_interpolation(send_comp, send_grid, &
    recv_comp, recv_grid, map_tag, user_func)
  character(len=*), intent(in) :: send_comp, send_grid
  character(len=*), intent(in) :: recv_comp, recv_grid
  integer,          intent(in) :: map_tag
  procedure(interpolation_user_ifc) :: user_func

  call rjn_set_user_interpolation_c(     &
      trim(send_comp)//c_null_char,       &
      trim(send_grid)//c_null_char,       &
      trim(recv_comp)//c_null_char,       &
      trim(recv_grid)//c_null_char,       &
      int(map_tag, c_int),                &
      c_funloc(user_func))
end subroutine jlt_set_user_interpolation

!=======+=========+=========+=========+=========+=========+=========+==

end module jlt_interface
