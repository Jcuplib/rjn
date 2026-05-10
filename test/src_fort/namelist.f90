!=======+=========+=========+=========+=========+=========+=========+=========+
!> tcup_namelist defines configuration related subroutines
module namelist
  private

!--------------------------------   public  ----------------------------------!

  integer, parameter :: STR_SHORT = 64
  integer, parameter :: STR_LONG  = 1024
  
  integer, parameter, public :: ILS_LOG_LOUD     = 2
  integer, parameter, public :: ILS_LOG_WHISPER  = 1
  integer, parameter, public :: ILS_LOG_SILENT   = 0

  character(len=4), parameter, public :: JCUP_TYPE = "JCUP"
  character(len=3), parameter, public :: JMC_TYPE  = "JMC"
  character(len=3), parameter, public :: ALL_TYPE  = "ALL"

  public :: exchange_data_type
  public :: type_ici_configure

  public :: read_coupler_config               ! subroutine (config_file_name)
  public :: get_log_level                     ! integer function ()
  public :: get_cpl_log_level                 ! integer function ()
  public :: get_stop_step                     ! integer function ()
  public :: is_debug_mode                     ! logical function ()
  public :: is_check_grid                     ! logical function ()
  public :: is_restart                        ! logical function ()
  public :: get_restart_dir                   ! character(len=STR_LONG) function ()
  public :: out_restart                       ! logical function ()
  public :: reset_restart_flag                ! subroutine () 
  public :: out_memory_monitor                ! logical function ()
  public :: out_data_monitor                  ! logical function ()

  public :: read_namelist                     ! subroutine ()
  public :: get_config_mapping_tag            ! subroutine (send_comp_name, send_grid_name, 
                                              !             recv_comp_name, recv_grid_name, mapping_tag)
  public :: check_comp_grid_name              ! subroutine (comp_name, grid_name)
  public :: get_num_of_configuration          ! function ()
  public :: get_num_of_exchange               ! integer function (conf_num)
  public :: get_ed_ptr                        ! type(exchange_data_type), pointer function (conf_num, exchange_num)
  public :: get_put_var_name                  ! character(len=STR_SHORT) function (conf_num, exchange_num)
  public :: get_get_var_name                  ! character(len=STR_SHORT) function (conf_num, exchange_num)
  public :: get_put_comp_name                 ! character(len=STR_SHORT) function (conf_num)
  public :: get_put_grid_name                 ! character(len=STR_SHORT) fucntion (conf_num)
  public :: get_get_comp_name                 ! character(len=STR_SHORT) fucntion (conf_num)
  public :: get_get_grid_name                 ! character(len=STR_SHORT) fucntion (conf_num)
  public :: get_intpl_map_size                ! integer function (conf_num)
  public :: get_map_file_name                 ! character(len=STR_LONG) function (conf_num)
  public :: get_coef_file_name                ! character(len=STR_LONG) functino (conf_num)
  public :: get_endian_conv_flag              ! logical function (conf_num)
  public :: get_coupler_type                  ! character(len=STR_SHORT) function (conf_num)
  public :: get_num_of_put_data               ! integer function (comp_name)
  public :: get_ed_ptr_from_put_comp_and_num  ! type(exchange_data_type), pointer function (comp_name, data_num)

  public :: get_num_of_recv_target            ! integer function (comp_name) 
  public :: get_recv_target_comp_name         ! character(len=STR_SHORT) function (my_comp_name, recv_target_num)

  public :: get_num_of_get_data               ! integer function (my_comp_name, target_num)
  public :: get_ed_ptr_from_get_comp_and_num  ! type(exchange_data_type), pointer function (my_comp_name, target_num, 
!data_num)
  public :: set_disable_data_conf             ! subroutine (conf_num, exchange_num)
  public :: exchange_disable_data_conf        ! subroutine (my_comp, source_comp)
  public :: search_get_data_config            ! subroutine (get_comp_name, get_data_name, config_ptr, ed_ptr)
  
!--------------------------------  private  ----------------------------------!

  integer, parameter :: INTVL_SEC = 1
  integer, parameter :: INTVL_MIN = 2
  integer, parameter :: INTVL_HUR = 3
  integer, parameter :: INTVL_DAY = 4
  integer, parameter :: INTVL_MON = 5


  character(len=4)   :: my_coupler_type  = "ALL"   ! "JCUP" or "JMC" or "ALL"
  logical            :: is_my_coupler 

  real(kind=8), parameter :: DEFAULT_FILL_VALUE = -9999.d0

type coupler_conf_type
  integer                     :: log_level
  integer                     :: cpl_log_level ! log level of Jcup or JcupMC
  integer                     :: stop_step = 0
  logical                     :: debug_mode
  real(kind=8)                :: fill_value                ! default fill value
  logical                     :: grid_checker   = .false.
  logical                     :: restart_flag   = .false.
  character(len=STR_LONG)     :: restart_dir
  logical                     :: out_restart    = .false.
  logical                     :: memory_monitor = .false.
  logical                     :: data_monitor   = .false.
end type

type(coupler_conf_type), save :: coupler_conf

type exchange_data_type
  character(len=STR_SHORT) :: var_put
  character(len=STR_SHORT) :: var_get
  character(len=STR_SHORT) :: var_put_vec
  character(len=STR_SHORT) :: var_get_vec
  logical                     :: is_vec             ! vector data or not
  integer                     :: intvl
  integer                     :: lag
  integer                     :: num_of_layer = 1
  integer                     :: mapping_tag
  integer                     :: grid_intpl_tag
  integer                     :: time_intpl_tag
  integer                     :: exchange_tag
  logical                     :: is_ok = .true.
  real(kind=8)                :: fill_value
  character(len=3)            :: flag
  real(kind=8)                :: factor = 1.d0
  logical                     :: range_flag = .false. 
  real(kind=8)                :: data_range(2)
  real(kind=8)                :: amin
  real(kind=8)                :: amax
  type(type_ici_configure), pointer :: parent
  type(exchange_data_type), pointer :: next_ptr
end type
  
type type_ici_configure
  character(len=STR_SHORT) :: put_comp_name 
  character(len=STR_SHORT) :: put_grid_name
  character(len=STR_SHORT) :: get_comp_name 
  character(len=STR_SHORT) :: get_grid_name
  integer                  :: intpl_map_size        ! size of mapping table file
  character(len=STR_LONG)  :: map_file_name         ! file name of mapping table
  character(len=STR_LONG)  :: coef_file_name        ! file name of interpolation coefficient
  logical                  :: conv_endian           ! flag of endian conversion    
  integer :: mapping_tag
  character(len=STR_SHORT) :: coupler_type          ! coupler type = "JCUP" or "JMC", default : "JCUP"
  integer :: num_of_exchange
  type(exchange_data_type), pointer :: start_ed
  type(exchange_data_type), pointer :: ed ! exchange data
end type

integer :: num_of_config 

type(type_ici_configure), pointer :: config(:)

integer, parameter :: MAX_DISABLE_DATA = 100
integer :: disable_data_num(MAX_DISABLE_DATA, 2)  ! 1 = config_num, 2 = exchange_num
integer, save :: num_of_disable_data = 0

contains

!=======+=========+=========+=========+=========+=========+=========+=========+
!> read configuration file
!! \protected
subroutine read_coupler_config(conf_file_name)
  implicit none
  character(len=*), intent(IN) :: conf_file_name
  character(len=STR_SHORT)     :: log_level      = "SILENT"
  character(len=STR_SHORT)     :: cpl_log_level  = "SILENT"
  integer                      :: stop_step      = 0
  real(kind=8)                 :: fill_value     = DEFAULT_FILL_VALUE
  logical                      :: debug_mode     = .false.
  logical                      :: grid_checker   = .false.
  logical                      :: restart_flag   = .false.
  logical                      :: out_restart    = .false.
  logical                      :: memory_monitor = .false.
  logical                      :: data_monitor   = .false.
  character(len=STR_LONG)   :: restart_dir = ""
  namelist / coupler_config / log_level, cpl_log_level, stop_step, debug_mode, fill_value, grid_checker, &
                              restart_flag, out_restart, restart_dir, memory_monitor, data_monitor
  integer :: nfl = 565
  integer :: istat

  call ils_set_fid(nfl)

  open(nfl, file=trim(conf_file_name), action='read', iostat = istat)

  if (istat /= 0) then
     call ils_error("read_coupler_config", "File Open error ! file name = "//trim(conf_file_name))
  end if

  read(nfl, nml=coupler_config, iostat = istat)

  close(nfl)

  if (istat /= 0) goto 100

  select case(trim(log_level))
  case ("SILENT")
    coupler_conf%log_level = ILS_LOG_SILENT
  case("WHISPER")
    coupler_conf%log_level = ILS_LOG_WHISPER
  case("LOUD")
    coupler_conf%log_level = ILS_LOG_LOUD
  case default
    write(0,*) "read_coupler_config, log_level error : "//trim(log_level)
    call ils_error("read_coupler_config", "log_level error")
  end select

  select case(trim(cpl_log_level))
  case ("SILENT")
    coupler_conf%cpl_log_level = ILS_LOG_SILENT
  case("WHISPER")
    coupler_conf%cpl_log_level = ILS_LOG_WHISPER
  case("LOUD")
    coupler_conf%cpl_log_level = ILS_LOG_LOUD
  case default
    write(0,*) "read_coupler_config, log_level error : "//trim(log_level)
    call ils_error("read_coupler_config", "log_level error")
  end select

  coupler_conf%stop_step    = stop_step
  coupler_conf%debug_mode   = debug_mode
  coupler_conf%fill_value   = fill_value
  coupler_conf%grid_checker = grid_checker
  coupler_conf%restart_flag = restart_flag
  coupler_conf%out_restart  = out_restart
  if (trim(restart_dir) == "") then
    coupler_conf%restart_dir  = "./" ! current directory
  else
    coupler_conf%restart_dir  = trim(restart_dir)//"/"
  end if
  coupler_conf%memory_monitor = memory_monitor
  coupler_conf%data_monitor   = data_monitor
  return

100 continue
 
  coupler_conf%log_level =  2 ! silent mode
  coupler_conf%debug_mode = .false.

  return

end subroutine read_coupler_config

!=======+=========+=========+=========+=========+=========+=========+=========+
!> get log level
!! \protected
integer function get_log_level()
  implicit none

  get_log_level = coupler_conf%log_level

end function get_log_level

!=======+=========+=========+=========+=========+=========+=========+=========+
!> get log level
!! \protected
integer function get_cpl_log_level()
  implicit none

  get_cpl_log_level = coupler_conf%cpl_log_level

end function get_cpl_log_level

!=======+=========+=========+=========+=========+=========+=========+=========+
!> get log level
!! \protected
integer function get_stop_step()
  implicit none

  get_stop_step = coupler_conf%stop_step

end function get_stop_step

!=======+=========+=========+=========+=========+=========+=========+=========+
!> return debug mode or not
!! \protected
logical function is_debug_mode()
  implicit none

  is_debug_mode = coupler_conf%debug_mode

end function is_debug_mode

!=======+=========+=========+=========+=========+=========+=========+=========+
!> return grid checker flag
!! \protected
logical function is_check_grid()
  implicit none

  is_check_grid = coupler_conf%grid_checker

end function is_check_grid

!=======+=========+=========+=========+=========+=========+=========+=========+
!> return restart flag
!! \protected
logical function is_restart()
  implicit none

  is_restart = coupler_conf%restart_flag

end function is_restart

!=======+=========+=========+=========+=========+=========+=========+=========+
!> return restart flag
!! \protected
character(len=STR_LONG) function get_restart_dir()
  implicit none

  get_restart_dir =  coupler_conf%restart_dir

end function get_restart_dir

!=======+=========+=========+=========+=========+=========+=========+=========+
!> return restart flag
!! \protected
logical function out_restart()
  implicit none

  out_restart = coupler_conf%out_restart

end function out_restart

!=======+=========+=========+=========+=========+=========+=========+=========+

logical function out_memory_monitor()
  implicit none

  out_memory_monitor = coupler_conf%memory_monitor

end function out_memory_monitor

!=======+=========+=========+=========+=========+=========+=========+=========+

logical function out_data_monitor()
  implicit none

  out_data_monitor = coupler_conf%data_monitor

end function out_data_monitor

!=======+=========+=========+=========+=========+=========+=========+=========+
!> set restart flag => .false.
!! \protected
subroutine reset_restart_flag()
  implicit none

  coupler_conf%restart_flag = .false.

end subroutine reset_restart_flag

!=======+=========+=========+=========+=========+=========+=========+=========+
!> read namelist file
!! \protected
subroutine read_namelist(file_name)
  implicit none
    !! Variables for namelist
    character(len=*), intent(IN) :: file_name
    character(len=STR_SHORT)     :: comp_put, comp_get    !! Component name for put and get
    character(len=STR_SHORT)     :: grid_put, grid_get    !! Grid name for put and get
    integer                      :: intpl_map_size
    character(len=STR_LONG)      :: map_file_name
    character(len=STR_LONG)      :: coef_file_name
    logical                      :: endian_conv 
    integer                      :: mapping_tag
    integer                      :: grid_intpl_tag, time_intpl_tag
    character(len=STR_SHORT)     :: var_put    , var_get     !! Var name for put and get (Scalar)
    character(len=STR_SHORT)     :: var_put_vec, var_get_vec !! Var name for put and get (Vector)
    integer :: intvl                                         !! Interval of exchange
    integer :: lag                                           !! Lag of exchange, -1, 0 or 1
                                                             !!  See pages about time lag in 'SCUP.ppt'
    character(len=3) :: flag                                 !! 'AVR' or 'SNP'
    integer :: layer                                         !! number of vertical layer
    real(kind=8) :: factor
    real(kind=8) :: data_range(2)
    real(kind=8) :: fill_value
    integer :: is_ok
    namelist/nam_ici/ comp_put, comp_get, grid_put, grid_get, &
                      intpl_map_size, map_file_name, coef_file_name, endian_conv, mapping_tag, &
                      var_put, var_get, var_put_vec, var_get_vec, grid_intpl_tag, &
                      time_intpl_tag, intvl, lag, flag, layer, factor, data_range, fill_value, is_ok
    
    character(len=256) :: log_str
    integer :: ios
    integer :: nfl = 565
    integer :: istat

    ! set coupler type
    my_coupler_type = ALL_TYPE

    call ils_set_fid(nfl)

    open(nfl, file=trim(file_name), action='read', iostat = istat)
    
   if (istat /= 0) then
      call ils_error("read_namelist","File Open error ! file name = "//trim(file_name))
   end if
  

    num_of_config = 0

    !write(log_str,*) "ici_namelist__read_namelist (ici_namelst.f90): open, unit=",nfl,", file=" &
    ! &         //trim(file_name)

    !call ils_put_log(log_str)

    call count_num_of_config()
    call read_configuration()

    !!!!!call write_configuration()

    return

contains

  subroutine count_num_of_config()
    implicit none
    integer :: i

    rewind(nfl)

    do
      comp_put = ' '
      comp_get = ' '
      grid_put = ' '
      grid_get = ' '
      intpl_map_size = 0
      map_file_name = ' '
      coef_file_name = ' '
      endian_conv = .false.
      mapping_tag = 1
      var_put = ' '
      var_get = ' '
      var_put_vec = ' '
      var_get_vec = ' '
      grid_intpl_tag = 0
      time_intpl_tag = 0
      intvl = -999
      lag = -1
      flag = ' '
      layer = 1
      factor = 1.d0
      data_range(1) = -huge(0.d0)
      data_range(2) = huge(0.d0)
      fill_value = coupler_conf%fill_value 
      is_ok = 1

      read(nfl,nam_ici,iostat=ios, err=900)

      if ( ios /= 0 ) then
        exit
      end if

      if ( comp_put /= ' ' .or. comp_get /= ' ' ) then
        
        if ( (comp_put /= ' ' .and. comp_get == ' ') .or. (comp_put == ' ' .and. comp_get /= ' ') ) then
          write(0, *) "Error: ici_namelist__count_num_of_config (ici_namelist.f90): "
          write(0, *) "       comp_put='"//trim(comp_put)//"' and comp_get='"//trim(comp_get)//"' is wrong"
          call ils_error("count_num_of_config", "component setting error")
        end if

        num_of_config = num_of_config + 1

      end if

    end do

    allocate(config(num_of_config))

    do i = 1, num_of_config
      config(i)%num_of_exchange = 0
      config(i)%ed => null()
    end do

    return
    
  900 continue

    call read_error()
    
  end subroutine count_num_of_config
 

  subroutine read_configuration
    implicit none
    type (type_ici_configure), pointer :: current_config
    character(len=128) :: log_str
    integer :: config_counter
    
    rewind(nfl)

    config_counter = 0

    do
      comp_put = ' '
      comp_get = ' '
      grid_put = ' '
      grid_get = ' '
      intpl_map_size = 0
      map_file_name = ' '
      coef_file_name = ' '
      endian_conv = .false.
      mapping_tag = 1
      var_put = ' '
      var_get = ' '
      var_put_vec = ' '
      var_get_vec = ' '
      grid_intpl_tag = 0
      time_intpl_tag = 0
      intvl = -999
      lag = -1
      flag = ' '
      layer = 1
      factor = 1.d0
      data_range(1) = -huge(0.d0)
      data_range(2) = huge(0.d0)
      fill_value = coupler_conf%fill_value 
      is_ok = 1

      read(nfl,nam_ici,iostat=ios,err=900)
      if ( ios /= 0 ) then
        exit
      end if

      if ( comp_put /= ' ' .or. comp_get /= ' ' ) then
        
        if ( (comp_put /= ' ' .and. comp_get == ' ') .or. (comp_put == ' ' .and. comp_get /= ' ') ) then
          write(0, *) "Error: ici_namelist__read_configuration (ici_namelist.f90): "
          write(0, *) "       comp_put='"//trim(comp_put)//"' and comp_get='"//trim(comp_get)//"' is wrong"
          call ils_error("read_configuration", "component setting error")
        end if

        config_counter = config_counter + 1
        is_my_coupler = .true.

        config(config_counter)%put_comp_name   = comp_put
        config(config_counter)%put_grid_name   = grid_put
        config(config_counter)%get_comp_name   = comp_get
        config(config_counter)%get_grid_name   = grid_get
        config(config_counter)%intpl_map_size  = intpl_map_size
        config(config_counter)%map_file_name   = trim(map_file_name)
        config(config_counter)%coef_file_name  = trim(coef_file_name)
        config(config_counter)%conv_endian     = endian_conv
        call cal_mapping_tag(config_counter, comp_put, comp_get, mapping_tag)
        config(config_counter)%mapping_tag     = mapping_tag
        current_config => config(config_counter)

        call ils_put_log(' ')
        call ils_put_log('-')
        call ils_put_log('ici_read_namelist (ici_namelist.f90): Read '//trim(file_name))
        call ils_put_log("     comp_put ,  comp_get     = '"//trim(comp_put) //"', '"//trim(comp_get) //"'")
        call ils_put_log("     grid_put ,  grid_get     = '"//trim(grid_put) //"', '"//trim(grid_get) //"'")
        write(log_str, *) "    intpl_map_size = ", intpl_map_size
        call ils_put_log(trim(log_str))
        call ils_put_log("     map_file_name  = "//trim(map_file_name))
        call ils_put_log("     coef_file_name = "//trim(coef_file_name))
        
        write(log_str, *) "    mapping tag = ",mapping_tag
        call ils_put_log(trim(log_str))
                 
      else if ( var_put /= ' ' .or. var_get /= ' ' .or. var_put_vec /= ' ' .or. var_get_vec /= ' ') then
        !
        !! Register trnsfmrp and trnsfmrg in varp and varg respectively
        !
        if ( var_put == ' ' .and. var_put_vec == ' ' ) then
          write(0,*) ' '
          write(0,*) "Error: ici_namelist__set_trnsfmr (ici_namelist.F90):"
          write(0,*) "       var_put=' ' and var_put_vec=' ' is wrong"
          call ils_error("read_configuration","variable setting error")
        end if
        if ( var_put /= ' ' .and. var_put_vec /= ' ' ) then
          write(0,*) ' '
          write(0,*) "Error: ici_namelist__set_trnsfmr (ici_namelist.F90): "
          write(0,*) "       var_put='"//trim(var_put)//"' and var_put_vec='"  &
           &                              //trim(var_put_vec)//"' is wrong"
          call ils_error("read_configuration","variable setting error")
        end if
        if ( var_put /= ' ' .and. var_get_vec /= ' ' ) then
          write(0,*) ' '
          write(0,*) "Error: ici_namelist__set_trnsfmr (ici_namelist.F90): "
          write(0,*) "       var_put='"//trim(var_put)//"' and var_get_vec='"  &
           &                              //trim(var_get_vec)//"' is wrong"
          call ils_error("read_configuration","variable setting error")
        end if
        if ( var_put_vec /= ' ' .and. var_get /= ' ' ) then
          write(0,*) ' '
          write(0,*) "Error: ici_namelist__set_trnsfmr (ici_namelist.F90): "
          write(0,*) "       var_put_vec='"//trim(var_put_vec)//"' and var_get='" &
           &                              //trim(var_get)//"' is wrong"
          call ils_error("read_configuration","variable setting error")
        end if
        if ( flag /= 'AVR' .and. flag /= 'SNP' ) then
          write(0,*) ' '
          write(0,*) "Error: ici_namelist__set_trnsfmr (ici_namelist.F90):"
          write(0,*) "       flag should be 'AVR' or 'SNP'"
          call ils_error("read_configuration","flag setting error")
        end if
        if ( lag /= -1 .and. lag /= 0 .and. lag /= 1 ) then
          write(0,*) ' '
          write(0,*) "Error: ici_namelist__set_trnsfmr (ici_namelist.F90):"
          write(0,*) "       lag should be -1 or 0 or 1"
          call ils_error("read_configuration","lag setting error")
        end if
        if ( lag == 0 .and. flag == 'AVR' ) then
          write(0,*) ' '
          write(0,*) "Error: ici_namelist__set_trnsfmr (ici_namelist.F90):"
          write(0,*) "       flag should be 'SNP' when lag == 0"
          call ils_error("read_configuration","lag setting error")
        end if


        if ( var_get == ' ' .and. var_get_vec == ' ' ) then
          write(0,*) ' '
          write(0,*) "Error: ici_namelist__set_trnsfmr (ici_namelst.F90): " &
           &                                //"var_get=' ' and var_get_vec=' '"
          call ils_error("read_configuration","variable setting error")
        end if

        if (is_my_coupler) then
          current_config%num_of_exchange = current_config%num_of_exchange + 1
        else
          cycle
        end if

        if (current_config%num_of_exchange == 1) then
          allocate(current_config%ed)
          current_config%start_ed => current_config%ed
        else
          allocate(current_config%ed%next_ptr)
          current_config%ed => current_config%ed%next_ptr
        end if

        current_config%ed%next_ptr => null()
        current_config%ed%var_put = var_put
        current_config%ed%var_get = var_get
        current_config%ed%var_put_vec = var_put_vec
        current_config%ed%var_get_vec = var_get_vec
        current_config%ed%is_vec = (var_get == " ")
        current_config%ed%grid_intpl_tag = grid_intpl_tag
        current_config%ed%time_intpl_tag = time_intpl_tag
        current_config%ed%intvl    = intvl
        current_config%ed%lag      = lag
        current_config%ed%flag     = flag
        current_config%ed%num_of_layer = layer
        current_config%ed%factor   = factor
        if (data_range(2) == huge(0.d0)) then
          current_config%ed%range_flag = .false.
        else
          current_config%ed%range_flag = .true.
        end if
        current_config%ed%data_range = data_range
        current_config%ed%fill_value = fill_value
        current_config%ed%is_ok = (is_ok == 1)
        current_config%ed%mapping_tag = current_config%mapping_tag
        current_config%ed%exchange_tag = current_config%num_of_exchange
        current_config%ed%parent => current_config

        if (intvl == 0) then
          current_config%ed%lag = 0
        end if

        !call ils_put_log(' ')
        !call ils_put_log('ici_read_namelist (ici_namelist.f90): Read '//trim(file_name))
        if ( var_put /= ' ' ) then
         !call ils_put_log('       var_put    , var_get     = '//var_put//"    , "//var_get)
        else
         !call ils_put_log('       var_put_vec, var_get_vec = '//var_put_vec//", "// var_get_vec)
        end if
        write(log_str,*) '       intvl = ',intvl
        !call ils_put_log(log_str)
        write(log_str,*) '       lag   = ',lag
        !call ils_put_log(log_str)
        write(log_str,*) '       flag  = ',flag
        !call ils_put_log(log_str)
        if (current_config%ed%range_flag) then
          write(log_str,*) '       range  = ',data_range(1), data_range(2)
          !call ils_put_log(log_str)
        end if    
      end if

    end do

    !call ils_put_log(' ')
    close(nfl)

    return


    
  900 continue
    
    call read_error()
    
  end subroutine read_configuration

  subroutine write_configuration()
    !use ils_utils, only : ils_put_log
    implicit none
    type(exchange_data_type), pointer :: ed_ptr
    character(len=256) :: log_str
    integer :: i, j

    write(log_str,*) ' '
    !call ils_put_log(log_str)
    write(log_str,*) 'ici_read_namelist (ici_namelist.f90): Read '//trim(file_name)
    !call ils_put_log(log_str)
    write(log_str,'(A,I5)') "  number of config : ", num_of_config
    !call ils_put_log(log_str)

    do i = 1, num_of_config
      !call ils_put_log(' ')
      !call ils_put_log("    put component name : "//trim(config(i)%put_comp_name))
      !call ils_put_log("    put grid name      : "//trim(config(i)%put_grid_name))
      !call ils_put_log("    get component name : "//trim(config(i)%get_comp_name))
      !call ils_put_log("    get grid name      : "//trim(config(i)%get_grid_name))
      write(log_str, '(A,I5)') "    mapping tag        : ", config(i)%mapping_tag
      !call ils_put_log(log_str)
      write(log_str, '(A,I5)') "    number of exchange data : ", config(i)%num_of_exchange
      !call ils_put_log(log_str)

      ed_ptr => config(i)%start_ed

      do j = 1, config(i)%num_of_exchange

        !call ils_put_log("      var_put     : "//trim(ed_ptr%var_put))
        !call ils_put_log("      var_get     : "//trim(ed_ptr%var_get))
        !call ils_put_log("      var_put_vec : "//trim(ed_ptr%var_put_vec))
        !call ils_put_log("      var_get_vec : "//trim(ed_ptr%var_get_vec))
        write(log_str, '(A,I5)') "       intvl : ", ed_ptr%intvl
        !call ils_put_log(log_str)
        write(log_str, '(A,I5)') "       lag   : ", ed_ptr%lag
        !call ils_put_log(log_str)
        write(log_str, '(A,A)')  "       flag  : ", ed_ptr%flag
        !call ils_put_log(log_str)

        ed_ptr => ed_ptr%next_ptr
      end do

    end do

    
  end subroutine write_configuration
 
  subroutine read_error()
    !use ils_utils, only : ils_error
    implicit none

    !call ils_error("read_namelist", 'ici_namelist__read_namelist (ici_namelist.f90): '//trim(file_name)//' read error')

  end subroutine read_error

end subroutine read_namelist

!=======+=========+=========+=========+=========+=========+=========+=========+
!> calculate mapping tag
!! \private
subroutine cal_mapping_tag(current_num, put_comp, get_comp, mapping_tag)
  implicit none
  integer, intent(IN) :: current_num ! current config number
  character(len=*), intent(IN) :: put_comp, get_comp ! component name
  integer, intent(OUT) :: mapping_tag
  integer :: i

  mapping_tag = 1

  do i = 1, current_num-1
      if ((trim(config(i)%put_comp_name) == trim(put_comp)).and. &
          (trim(config(i)%get_comp_name) == trim(get_comp))) then
        mapping_tag = mapping_tag + 1
      end if
  end do

end subroutine cal_mapping_tag

!=======+=========+=========+=========+=========+=========+=========+=========+
!> calculate mapping tag
!! \private
subroutine get_config_mapping_tag(put_comp, put_grid, get_comp, get_grid, &
                                  map_file_name, coef_file_name, mapping_tag)
  !use ils_utils, only : ils_error
  implicit none
  character(len=*), intent(IN) :: put_comp, put_grid, get_comp, get_grid ! component name
  character(len=*), intent(IN) :: map_file_name, coef_file_name
  integer, intent(OUT) :: mapping_tag
  integer :: i

  do i = 1, size(config)
      if ((trim(config(i)%put_comp_name) == trim(put_comp)).and. &
          (trim(config(i)%get_comp_name) == trim(get_comp))) then
         if ((trim(config(i)%put_grid_name) == trim(put_grid)).and. &
             (trim(config(i)%get_grid_name) == trim(get_grid))) then
            if ((trim(config(i)%map_file_name) == trim(map_file_name)).and. &
                (trim(config(i)%coef_file_name) == trim(coef_file_name))) then
               mapping_tag = config(i)%mapping_tag
               return

            end if
         end if
      end if
  end do

  !call ils_error("get_config_mapping_tag", &
  !!                "no such comp_name, grid_name, "//trim(put_comp)//","//trim(put_grid)// &
  !                ","//trim(get_comp)//","//trim(get_grid))

end subroutine get_config_mapping_tag

!=======+=========+=========+=========+=========+=========+=========+=========+
!> write configuration
!! \private
subroutine write_configure()

end subroutine write_configure

!=======+=========+=========+=========+=========+=========+=========+=========+
!> check componant name and grid name
!! \protected
subroutine check_comp_grid_name(comp_name, grid_name)
  !use ils_utils, only : ils_error
  implicit none
  character(len=*), intent(IN) :: comp_name, grid_name
  integer :: i

  do i = 1, num_of_config
    if ((trim(config(i)%put_comp_name)==trim(comp_name)).and.((trim(config(i)%put_grid_name)==trim(grid_name)))) return
    if ((trim(config(i)%get_comp_name)==trim(comp_name)).and.((trim(config(i)%get_grid_name)==trim(grid_name)))) return
  end do

  !call ils_error("check_comp_grid_name", "Component name : "//trim(comp_name)//", grid name : "//trim(grid_name)//&
  !                " is not defined in config file")

end subroutine check_comp_grid_name

!=======+=========+=========+=========+=========+=========+=========+=========+
!> get the number of configuration
!! \protected
integer function get_num_of_configuration()
  implicit none

  get_num_of_configuration = num_of_config

end function get_num_of_configuration

!=======+=========+=========+=========+=========+=========+=========+=========+
!> get number of exchange 
integer function get_num_of_exchange(conf_num)
  implicit none
  integer, intent(IN) :: conf_num

  get_num_of_exchange = config(conf_num)%num_of_exchange

end function get_num_of_exchange

!=======+=========+=========+=========+=========+=========+=========+=========+
!> get exchange data pointer
function get_ed_ptr(conf_num, exchange_num) result(data_conf_ptr)
  implicit none
  integer, intent(IN) :: conf_num
  integer, intent(IN) :: exchange_num
  type(exchange_data_type), pointer :: data_conf_ptr
  integer :: counter

  counter = 0
  data_conf_ptr => null()

  data_conf_ptr => config(conf_num)%start_ed
  do while (associated(data_conf_ptr))
    counter = counter + 1
    if (counter == exchange_num) then
       return
    end if
    data_conf_ptr => data_conf_ptr%next_ptr
  end do
  
end function get_ed_ptr

!=======+=========+=========+=========+=========+=========+=========+=========+
!> get put var name
function get_put_var_name(conf_num, exchange_num) result(var_name)
  implicit none
  integer, intent(IN) :: conf_num
  integer, intent(IN) :: exchange_num
  type(exchange_data_type), pointer :: data_conf_ptr
  character(len=STR_SHORT) :: var_name
  integer :: counter

  counter = 0
  data_conf_ptr => null()
  var_name = ""

  data_conf_ptr => config(conf_num)%start_ed
  do while (associated(data_conf_ptr))
    counter = counter + 1
    if (counter == exchange_num) then
       if (trim(data_conf_ptr%var_put_vec) == "") then
         var_name = trim(data_conf_ptr%var_put)
       else
         var_name = trim(data_conf_ptr%var_put_vec)
       end if
       return
    end if
    data_conf_ptr => data_conf_ptr%next_ptr
  end do
  
end function get_put_var_name

!=======+=========+=========+=========+=========+=========+=========+=========+
!> get put var name
function get_get_var_name(conf_num, exchange_num) result(var_name)
  implicit none
  integer, intent(IN) :: conf_num
  integer, intent(IN) :: exchange_num
  type(exchange_data_type), pointer :: data_conf_ptr
  character(len=STR_SHORT) :: var_name
  integer :: counter

  counter = 0
  data_conf_ptr => null()
  var_name = ""

  data_conf_ptr => config(conf_num)%start_ed
  do while (associated(data_conf_ptr))
    counter = counter + 1
    if (counter == exchange_num) then
       if (trim(data_conf_ptr%var_get_vec) == "") then
         var_name = trim(data_conf_ptr%var_get)
       else
         var_name = trim(data_conf_ptr%var_get_vec)
       end if
       return
    end if
    data_conf_ptr => data_conf_ptr%next_ptr
  end do
  
end function get_get_var_name

!=======+=========+=========+=========+=========+=========+=========+=========+
!> get put component name
!! \protected
character(len=STR_SHORT) function get_put_comp_name(conf_num)
  implicit none
  integer, intent(IN) :: conf_num

  get_put_comp_name = config(conf_num)%put_comp_name

end function get_put_comp_name

!=======+=========+=========+=========+=========+=========+=========+=========+
!> get put grid name
!! \protected
character(len=STR_SHORT) function get_put_grid_name(conf_num)
  implicit none
  integer, intent(IN) :: conf_num

  get_put_grid_name = config(conf_num)%put_grid_name

end function get_put_grid_name

!=======+=========+=========+=========+=========+=========+=========+=========+
!> get get component name
!! \protected
character(len=STR_SHORT) function get_get_comp_name(conf_num)
  implicit none
  integer, intent(IN) :: conf_num

  get_get_comp_name = config(conf_num)%get_comp_name

end function get_get_comp_name

!=======+=========+=========+=========+=========+=========+=========+=========+
!> get get grid name
!! \protected
character(len=STR_SHORT) function get_get_grid_name(conf_num)
  implicit none
  integer, intent(IN) :: conf_num

  get_get_grid_name = config(conf_num)%get_grid_name

end function get_get_grid_name


!=======+=========+=========+=========+=========+=========+=========+=========+
!> get interpolation map size
!! \protected

integer function get_intpl_map_size(conf_num)
  implicit none
  integer, intent(IN) :: conf_num

  get_intpl_map_size = config(conf_num)%intpl_map_size

end function get_intpl_map_size


!=======+=========+=========+=========+=========+=========+=========+=========+
!> get map file name
!! \protected
character(len=STR_LONG) function get_map_file_name(conf_num)
  implicit none
  integer, intent(IN) :: conf_num

  get_map_file_name = config(conf_num)%map_file_name

end function get_map_file_name

!=======+=========+=========+=========+=========+=========+=========+=========+
!> get map file name
!! \protected
character(len=STR_LONG) function get_coef_file_name(conf_num)
  implicit none
  integer, intent(IN) :: conf_num

  get_coef_file_name = config(conf_num)%coef_file_name

end function get_coef_file_name

!=======+=========+=========+=========+=========+=========+=========+=========+
!> get map file name
!! \protected
logical function get_endian_conv_flag(conf_num)
  implicit none
  integer, intent(IN) :: conf_num

  get_endian_conv_flag = config(conf_num)%conv_endian

end function get_endian_conv_flag

!=======+=========+=========+=========+=========+=========+=========+=========+
!> get coupler type config
character(len=STR_SHORT) function get_coupler_type(conf_num)
  implicit none
  integer, intent(IN) :: conf_num

  get_coupler_type = config(conf_num)%coupler_type

end function get_coupler_type


!=======+=========+=========+=========+=========+=========+=========+=========+
!> get the number of put data
!! \protected
integer function get_num_of_put_data(comp_name)
  implicit none
  character(len=*), intent(IN) :: comp_name
  integer :: counter
  integer :: i

  counter = 0

  do i = 1, num_of_config
    if (trim(comp_name) == trim(config(i)%put_comp_name)) then
       counter = counter + config(i)%num_of_exchange
    end if
  end do

  get_num_of_put_data = counter

end function get_num_of_put_data

!=======+=========+=========+=========+=========+=========+=========+=========+
!> get exchange data pointer
!! \protected
function get_ed_ptr_from_put_comp_and_num(comp_name, data_num) result(data_conf_ptr)
  !use ils_utils, only : ils_error
  implicit none
  character(len=*), intent(IN) :: comp_name
  integer, intent(IN) :: data_num
  type(exchange_data_type), pointer :: data_conf_ptr
  integer :: counter
  integer :: i

  counter = 0
  data_conf_ptr => null()

  do i = 1, num_of_config
    if (trim(config(i)%put_comp_name) == trim(comp_name)) then
       data_conf_ptr => config(i)%start_ed
       do while (associated(data_conf_ptr))
         counter = counter + 1
         if (counter == data_num) return
         data_conf_ptr => data_conf_ptr%next_ptr
       end do
    end if
  end do

  !call ils_error("get_ed_ptr_from_comp_name_and_num", "comp name : "//trim(comp_name))
  
end function get_ed_ptr_from_put_comp_and_num

!=======+=========+=========+=========+=========+=========+=========+=========+
!> get the number of recv target
!! \protected
integer function get_num_of_recv_target(comp_name)
  implicit none
  character(len=*), intent(IN) :: comp_name
  integer :: counter 
  integer :: i

  counter = 0
  do i = 1, num_of_config
    if (trim(config(i)%get_comp_name) == trim(comp_name)) counter = counter + 1
  end do

  get_num_of_recv_target = counter

end function get_num_of_recv_target

!=======+=========+=========+=========+=========+=========+=========+=========+
!> get the component name of recv target
!! \protected
character(len=STR_SHORT) function get_recv_target_comp_name(comp_name, target_num)
  !use ils_utils, only : ils_error
  implicit none
  character(len=*), intent(IN) :: comp_name
  integer, intent(IN) :: target_num
  integer :: counter
  integer :: i

  counter = 0
  do i = 1, num_of_config
    if (trim(config(i)%get_comp_name) == trim(comp_name)) then
      counter = counter + 1
      if (counter == target_num) then
        get_recv_target_comp_name = config(i)%put_comp_name
        return
      end if
    end if
  end do

  !call ils_error("get_recv_target_name", "no such component, comp name : "//trim(comp_name))

end function get_recv_target_comp_name

!=======+=========+=========+=========+=========+=========+=========+=========+
!> get the number of get data
!! \protected
integer function get_num_of_get_data(comp_name, target_num)
  !use ils_utils, only : ils_error
  implicit none
  character(len=*), intent(IN) :: comp_name ! recv comp_name
  integer, intent(IN) :: target_num
  integer :: i, counter

  counter = 0
  do i = 1, num_of_config
    if (trim(config(i)%get_comp_name) == trim(comp_name)) then
      counter = counter + 1
      if (counter == target_num) then
         get_num_of_get_data = config(i)%num_of_exchange
         return
      end if
    end if
  end do

  call ils_error("get_num_of_get_data", "no such component, comp name : "//trim(comp_name))

end function get_num_of_get_data

!=======+=========+=========+=========+=========+=========+=========+=========+
!> get exchange data pointer
!! \protected
function get_ed_ptr_from_get_comp_and_num(my_comp_name, target_num, data_num) result(data_conf_ptr)
  implicit none
  character(len=*), intent(IN) :: my_comp_name
  integer, intent(IN) :: target_num
  integer, intent(IN) :: data_num
  type(exchange_data_type), pointer :: data_conf_ptr
  integer :: target_counter, data_counter
  integer :: i

  target_counter = 0
  data_counter = 0
  data_conf_ptr => null()

  do i = 1, num_of_config
    if (trim(config(i)%get_comp_name) == trim(my_comp_name)) then
       target_counter = target_counter + 1
       if (target_counter == target_num) then
         data_conf_ptr => config(i)%start_ed
         do while (associated(data_conf_ptr))
           data_counter = data_counter + 1
           if (data_counter == data_num) return
           data_conf_ptr => data_conf_ptr%next_ptr
         end do
      end if
    end if
  end do

  call ils_error("get_ed_ptr_from_get_comp_and_num", "comp name : "//trim(my_comp_name))
  
end function get_ed_ptr_from_get_comp_and_num

!=======+=========+=========+=========+=========+=========+=========+=========+
!> get data configuration pointer
!! \protected
function get_data_conf_ptr_from_put_data_name(config, data_name) result(data_conf_ptr)
  implicit none
  type(type_ici_configure), intent(IN) :: config
  character(len=*), intent(IN) :: data_name
  type(exchange_data_type), pointer :: data_conf_ptr
  
  data_conf_ptr => config%start_ed

  do while(associated(data_conf_ptr))
    if (trim(data_conf_ptr%var_put) == trim(data_name)) return
    if (trim(data_conf_ptr%var_put_vec) == trim(data_name)) return
    data_conf_ptr => data_conf_ptr%next_ptr
  end do

end function get_data_conf_ptr_from_put_data_name

!=======+=========+=========+=========+=========+=========+=========+=========+
!> get data configuration pointer
!! \protected
function get_data_conf_ptr_from_get_data_name(config, data_name) result(data_conf_ptr)
  implicit none
  type(type_ici_configure), intent(IN) :: config
  character(len=*), intent(IN) :: data_name
  type(exchange_data_type), pointer :: data_conf_ptr
  
  data_conf_ptr => config%start_ed

  do while(associated(data_conf_ptr))
    if (trim(data_conf_ptr%var_get) == trim(data_name)) return
    if (trim(data_conf_ptr%var_get_vec) == trim(data_name)) return
    data_conf_ptr => data_conf_ptr%next_ptr
  end do

end function get_data_conf_ptr_from_get_data_name

!=======+=========+=========+=========+=========+=========+=========+=========+
!> get component configuration pointer
!! \protected
function get_config_ptr_from_put_comp_name(comp_name) result(config_ptr)
  implicit none
  character(len=*), intent(IN) :: comp_name
  type(type_ici_configure), pointer :: config_ptr
  integer :: i

  do i = 1, num_of_config
    if (trim(config(i)%put_comp_name) == trim(comp_name)) then
      config_ptr => config(i)
      return
    end if
  end do

  config_ptr => null()

end function get_config_ptr_from_put_comp_name

!=======+=========+=========+=========+=========+=========+=========+=========+
!> get component configuration pointer
!! \protected
function get_config_ptr_from_get_comp_name(comp_name) result(config_ptr)
  implicit none
  character(len=*), intent(IN) :: comp_name
  type(type_ici_configure), pointer :: config_ptr
  integer :: i

  do i = 1, num_of_config
    if (trim(config(i)%get_comp_name) == trim(comp_name)) then
      config_ptr => config(i)
      return
    end if
  end do

  config_ptr => null()

end function get_config_ptr_from_get_comp_name

!=======+=========+=========+=========+=========+=========+=========+=========+
!> get component configuration pointer
!! \protected
function get_config_ptr_from_put_data_name(data_name) result(config_ptr)
  implicit none
  character(len=*), intent(IN) :: data_name
  type(type_ici_configure), pointer :: config_ptr
  integer :: i

  do i = 1, num_of_config
    if (associated(get_data_conf_ptr_from_put_data_name(config(i), data_name))) then
      config_ptr => config(i)
      return
    end if
  end do

  config_ptr => null()

end function get_config_ptr_from_put_data_name

!=======+=========+=========+=========+=========+=========+=========+=========+
!> get component configuration pointer
!! \protected
function get_config_ptr_from_get_data_name(data_name) result(config_ptr)
  implicit none
  character(len=*), intent(IN) :: data_name
  type(type_ici_configure), pointer :: config_ptr
  integer :: i

  do i = 1, num_of_config
    if (associated(get_data_conf_ptr_from_get_data_name(config(i), data_name))) then
      config_ptr => config(i)
      return
    end if
  end do

  call ils_error("get_config_ptr_from_get_data_name", "no such data name : "//trim(data_name))

end function get_config_ptr_from_get_data_name

!=======+=========+=========+=========+=========+=========+=========+=========+
!> get exchange data configuration pointer
!! \protected
function get_ed_ptr_from_get_data_name(config_ptr, data_name) result (ed_ptr)
  implicit none
  type(type_ici_configure), pointer :: config_ptr
  character(len=*), intent(IN) :: data_name
  type(exchange_data_type), pointer :: ed_ptr
  
  ed_ptr => config_ptr%start_ed

  do while(associated(ed_ptr))
    if (trim(data_name) == trim(ed_ptr%var_get)) return
    if (trim(data_name) == trim(ed_ptr%var_get_vec)) return
    ed_ptr => ed_ptr%next_ptr    
  end do

  call ils_error("get_ed_ptr_from_get_data_name", "no such data name : "//trim(data_name))
  
end function get_ed_ptr_from_get_data_name

!=======+=========+=========+=========+=========+=========+=========+=========+
!> search get data configuration
!! \protected
subroutine search_get_data_config(get_comp_name, get_data_name, config_ptr, ed_ptr)
  !use ils_utils, only : ils_error
  implicit none
  character(len=*), intent(IN) :: get_comp_name
  character(len=*), intent(IN) :: get_data_name
  type(type_ici_configure), pointer :: config_ptr
  type(exchange_data_type), pointer :: ed_ptr
  integer :: i

  do i = 1, num_of_config
    if (trim(config(i)%get_comp_name) == trim(get_comp_name)) then
      config_ptr => config(i)
      ed_ptr => config_ptr%start_ed

      do while(associated(ed_ptr))
        if (trim(get_data_name) == trim(ed_ptr%var_get)) return
        if (trim(get_data_name) == trim(ed_ptr%var_get_vec)) return
        ed_ptr => ed_ptr%next_ptr    
      end do
     
    end if
  end do

  !call ils_error("search_get_data_config", "no such name : "//trim(get_comp_name)//","//trim(get_data_name))

end subroutine search_get_data_config

!=======+=========+=========+=========+=========+=========+=========+=========+
!>

subroutine ils_set_fid(fid)
  implicit none
  integer, intent(INOUT) :: fid
  logical :: op
  integer, parameter :: MIN_FID = 10
  integer, parameter :: MAX_FID = 999
  
  fid = max(fid, MIN_FID)

  do 
    if (fid > MAX_FID) then
      write(0, *) "[ils_set_fid], fid exceeded MAX_FID"
      stop
    end if
    inquire(unit = fid, OPENED = op)
    if (op) then
       fid = fid + 1
    else
      return
    end if
  end do

end subroutine ils_set_fid

!=======+=========+=========+=========+=========+=========+=========+=========+

subroutine ils_error(sub_name, log_str)
  implicit none
  character(len=*), intent(IN) :: sub_name
  character(len=*), intent(IN) :: log_str

  write(0, *) trim(sub_name)//","//trim(log_str)
  stop 1111
  
end subroutine ils_error

!=======+=========+=========+=========+=========+=========+=========+=========+

subroutine ils_put_log(log_str)
  implicit none
  character(len=*), intent(IN) :: log_str

  !write(0, *) trim(log_str)
  
end subroutine ils_put_log

!=======+=========+=========+=========+=========+=========+=========+=========+

end module namelist
