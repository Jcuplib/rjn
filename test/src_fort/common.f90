module common
  use mpi
  use constant, only : STR_SHORT, STR_LONG
  implicit none
  private
!--------------------------------   public  ----------------------------------!

  integer, public :: time_array(6) = (/2000,1,1,0,0,0/)

  public :: init_common
  public :: cal_and_def_grid
  public :: cal_and_set_map
  public :: set_data
  public :: init_time
  public :: set_time
  public :: put_data_1d
  public :: get_data_1d
  public :: put_data_2d
  public :: get_data_2d
  public :: finalize_common

  
!--------------------------------   private  ---------------------------------!

  integer, parameter :: MAX_GRID = 8
  
  character(len=STR_SHORT) :: my_name
  character(len=STR_SHORT) :: my_grid
  character(len=STR_SHORT) :: target_name
  character(len=STR_SHORT) :: target_grid

  integer :: my_comp_id
  integer :: my_nx
  integer :: my_ny
  integer :: num_of_layer
  integer :: num_of_global_grid  ! global grid size
  
  integer :: my_comm
  integer :: my_group
  integer :: my_size
  integer :: my_rank
  integer :: my_pos_x, my_pos_y
  integer :: is, ie, js, je
  integer :: num_of_local_grid   ! local grid size
  
  integer :: my_rank_global

  integer, pointer :: grid_index(:)

contains

!=======+=========+=========+=========+=========+=========+=========+=========+

subroutine init_common(comp_name, comp_id, grid_name, nx, ny, nz)
  !use h3ou_api, only : h3ou_init, h3ou_get_mpi_parameter
  use namelist, only : read_coupler_config, read_namelist
  use jlt_interface, only : jlt_set_new_comp, jlt_initialize, jlt_get_mpi_parameter
  !use palmtime, only : palm_TimeInit
  implicit none
  character(len=*), intent(IN) :: comp_name
  integer, intent(IN)          :: comp_id
  character(len=*), intent(IN) :: grid_name
  integer, intent(IN)          :: NX, NY, NZ

  integer, parameter :: MAX_GRID = 8
  integer :: ierror
  integer :: m, n  
  
  my_name = trim(comp_name)
  my_comp_id = comp_id
  my_grid = trim(grid_name)
  my_nx   = NX
  my_ny   = NY
  num_of_layer = NZ
  
  call read_coupler_config("coupling.conf")
  call read_namelist("coupling.conf")
  !call jlt_set_new_comp(my_name)
  call jlt_initialize(my_name, "SEC", log_level = 2)

  call jlt_get_mpi_parameter(trim(my_name), my_comm, my_group, my_size, my_rank)
  !write(0, *) trim(my_name), my_size, my_rank

  call mpi_comm_rank(MPI_COMM_WORLD, my_rank_global, ierror)

  m = my_size
  n = 1
  my_pos_x = mod(my_rank, m) + 1
  my_pos_y = my_rank/m + 1

  call cal_start_end(my_nx, m, my_pos_x, is, ie)
  call cal_start_end(my_ny, n, my_pos_y, js, je)

  num_of_local_grid = (ie - is + 1) * (je - js + 1)
  !call palm_TimeInit(trim(comp_name), my_comm)
  
end subroutine init_common
  
!=======+=========+=========+=========+=========+=========+=========+=========+

subroutine cal_mn(my_size, m, n)
  implicit none
  integer, intent(IN)  :: my_size
  integer, intent(OUT) :: m, n

  m = int(sqrt(dble(my_size)))
  do
     n = my_size/m
     if (mod(my_size, m) == 0) exit
     m = m + 1
  end do
  
end subroutine cal_mn

!=======+=========+=========+=========+=========+=========+=========+=========+

subroutine cal_start_end(num_grid, num_div, my_pos, is, ie)
  implicit none
  integer, intent(IN)  :: num_grid
  integer, intent(IN)  :: num_div
  integer, intent(IN)  :: my_pos
  integer, intent(OUT) :: is, ie
  integer :: div_num

  div_num = num_grid/num_div

  is = div_num * (my_pos - 1) + min(my_pos - 1, mod(num_grid, num_div))
  ie = is + div_num - 1
  if (my_pos <= mod(num_grid, num_div)) ie = ie + 1

  is = is + 1
  ie = ie + 1
  
end subroutine cal_start_end

!=======+=========+=========+=========+=========+=========+=========+=========+

subroutine cal_and_def_grid(global_index)
  use jlt_interface, only: jlt_def_grid, jlt_end_grid_def
  implicit none
  integer, intent(IN) :: global_index(:)
  integer :: i, j, counter

  allocate(grid_index(num_of_local_grid))

  counter = 0
  do j = js, je
     do i = is, ie
        counter = counter +1
        grid_index(counter) = global_index(i)
     end do
  end do
  
  call jlt_def_grid(grid_index, trim(my_name), trim(my_grid), num_of_layer)

  write(300 + my_rank_global, *) "cal_and_def_grid, ", is, ie, js, je, num_of_local_grid, grid_index

  call jlt_end_grid_def()
  

end subroutine cal_and_def_grid

!=======+=========+=========+=========+=========+=========+=========+=========+

subroutine cal_and_set_map(send_comp, send_grid, recv_comp, recv_grid, &
                           send_xs, send_xe, send_nx, send_grid_index, &
                           recv_xs, recv_xe, recv_nx, recv_grid_index)
  use jlt_interface, only : jlt_send_array, jlt_recv_array, jlt_set_mapping_table
  implicit none
  character(len=*), intent(IN) :: send_comp
  character(len=*), intent(IN) :: send_grid
  character(len=*), intent(IN) :: recv_comp
  character(len=*), intent(IN) :: recv_grid
  real(kind=8), intent(IN) :: send_xs, send_xe
  integer     , intent(IN) :: send_nx
  integer,      intent(IN) :: send_grid_index(:)
  real(kind=8), intent(IN) :: recv_xs, recv_xe
  integer     , intent(IN) :: recv_nx
  integer,      intent(IN) :: recv_grid_index(:)
  integer      :: table_size
  integer, allocatable :: send_index(:)
  integer, allocatable :: recv_index(:)
  real(kind=8), allocatable :: coef(:)
  integer :: int_array(1)
  integer :: fid1, fid2, fid3
  integer :: i

  table_size = 1

  if (trim(my_name) == trim(recv_comp)) then
     if (my_rank == 0) then
        call make_mapping_file(send_xs, send_xe, send_nx, send_grid_index, &
                               recv_xs, recv_xe, recv_nx, recv_grid_index, table_size)
     end if
  end if     

  if (trim(my_name) == trim(recv_comp)) then
     int_array(1) = table_size
     call jlt_send_array(trim(my_name), trim(send_comp), int_array)
     call MPI_Bcast(int_array, 1, MPI_INTEGER, 0, my_comm, i)
     table_size = int_array(1)
  else
     call jlt_recv_array(trim(my_name), trim(recv_comp), int_array)
     table_size = int_array(1)
  end if

  !if (my_rank == 0) then
    fid1 = 80
    fid2 = 81
    fid3 = 82
    
    allocate(send_index(table_size))
    allocate(recv_index(table_size))
    allocate(coef(table_size))
  
    open(fid1, file=trim(recv_comp)//".send.map", form="formatted", action="read", status="old")
    open(fid2, file=trim(recv_comp)//".recv.map", form="formatted", action="read", status="old")
    open(fid3, file=trim(recv_comp)//".coef.map", form="formatted", action="read", status="old")

    do i = 1, table_size
       read(fid1, *) send_index(i)
       read(fid2, *) recv_index(i)
       read(fid3, *) coef(i)
    end do

    close(fid1)
    close(fid2)
    close(fid3)
  !else
  !  allocate(send_index(1))
  !  allocate(recv_index(1))
  !  allocate(coef(1))
  !end if

  call jlt_set_mapping_table(my_name, send_comp, send_grid, recv_comp, recv_grid, 1, .false., "SAFE", send_index, recv_index, coef)
  
  write(300 + my_rank_global, *) "cal_and_set_map, ", send_grid, recv_grid, send_index, recv_index

  deallocate(send_index)
  deallocate(recv_index)
  deallocate(coef)
  
end subroutine cal_and_set_map

!=======+=========+=========+=========+=========+=========+=========+=========+

subroutine make_mapping_file(send_xs, send_xe, send_nx, send_index, &
                             recv_xs, recv_xe, recv_nx, recv_index, &
                             table_size)
  implicit none
  real(kind=8), intent(IN)  :: send_xs, send_xe
  integer     , intent(IN)  :: send_nx
  integer     , intent(IN)  :: send_index(:)
  real(kind=8), intent(IN)  :: recv_xs, recv_xe
  integer     , intent(IN)  :: recv_nx
  integer     , intent(IN)  :: recv_index(:)
  integer     , intent(OUT) :: table_size
  real(kind=8) :: rx
  real(kind=8) :: sx_stride
  real(kind=8) :: rx_stride
  real(kind=8) :: coefx1, coefx2
  integer      :: idx1, idx2
  integer      :: si
  integer      :: ri
  integer      :: fid1, fid2, fid3
  integer      :: sidx1, sidx2
  integer      :: ridx
  real(kind=8) :: coef1, coef2

  fid1 = 80
  fid2 = 81
  fid3 = 82

  open(fid1, file=trim(my_name)//".send.map", status="replace")
  open(fid2, file=trim(my_name)//".recv.map", status="replace")
  open(fid3, file=trim(my_name)//".coef.map", status="replace")
  
  sx_stride = (send_xe - send_xs)/send_nx
  rx_stride = (recv_xe - recv_xs)/recv_nx

  !write(0, *) "make_mapping_file ", send_xs, sx_stride, recv_xs, rx_stride
  
  ridx = 0
  table_size = 0
     do ri = 1, recv_nx
       rx = rx_stride * (ri - 1) + recv_xs
       call cal_index_coef(rx, send_xs, sx_stride, idx1, idx2, coefx1, coefx2)
       !write(0, *) "cal_index_coef ", rx, send_xs, sx_stride, idx1, idx2, coefx1, coefx2
       
       ridx  = ridx + 1
       sidx1 = idx1 + 1
       sidx2 = idx2 + 1
       coef1 = coefx1
       coef2 = coefx2
       if (coef1 > 0.d0) then
          if (sidx1 > 0) then
          if  ((sidx1 <= size(send_index)).and.(ridx <= size(recv_index))) then
            !write(0, *) send_index(sidx1), recv_index(ridx), coef1
            write(fid1, *) send_index(sidx1)
            write(fid2, *) recv_index(ridx)
            write(fid3, *) coef1
            table_size = table_size + 1
         end if
         end if
         
       end if
       if (coef2 > 0.d0) then
          if (sidx2 > 0) then
          if  ((sidx2 <= size(send_index)).and.(ridx <= size(recv_index))) then
            !write(0, *) send_index(sidx2), recv_index(ridx), coef2
            write(fid1, *) send_index(sidx2)
            write(fid2, *) recv_index(ridx)
            write(fid3, *) coef2
            table_size = table_size + 1
         end if
         end if
       end if
  end do

  close(fid1)
  close(fid2)
  close(fid3)

end subroutine make_mapping_file

!=======+=========+=========+=========+=========+=========+=========+=========+

subroutine cal_index_coef(r_point, s_start, s_stride, idx1, idx2, coef1, coef2)
  implicit none
  real(kind=8), intent(IN)  :: r_point
  real(kind=8), intent(IN)  :: s_start
  real(kind=8), intent(IN)  :: s_stride
  integer     , intent(OUT) :: idx1, idx2
  real(kind=8), intent(OUT) :: coef1, coef2

  idx1 = int((r_point-s_start)/s_stride)
  idx2 = idx1 + 1

  coef1 = (s_stride * idx2 - r_point)/s_stride
  coef2 = (r_point - s_stride * idx1)/s_stride
  
end subroutine cal_index_coef

!=======+=========+=========+=========+=========+=========+=========+=========+

subroutine cal_and_set_map_org(my_comp_name, &
                           send_comp_name, send_grid_name, send_grid_size, &
                           recv_comp_name, recv_grid_name, recv_grid_size)
  use jlt_interface, only : jlt_set_mapping_table
  implicit none
  character(len=*), intent(IN) :: my_comp_name
  character(len=*), intent(IN) :: send_comp_name, send_grid_name
  integer, intent(IN)          :: send_grid_size
  character(len=*), intent(IN) :: recv_comp_name, recv_grid_name
  integer, intent(IN)          :: recv_grid_size
  integer                      :: send_map_size, recv_map_size
  integer, allocatable         :: send_data_index(:)
  integer, allocatable         :: recv_data_index(:)
  real(kind=8), allocatable    :: coef(:)
  integer :: send_recv_ratio   
  integer :: counter
  integer :: i, j


  if (mod(max(send_grid_size, recv_grid_size), min(send_grid_size, recv_grid_size)) /= 0) then
     write(0, *) "send grid size and recv grid size setting invalid, ", send_grid_size, recv_grid_size
     stop
  end if

  send_recv_ratio = max(send_grid_size, recv_grid_size)/min(send_grid_size, recv_grid_size)
  
  send_map_size = max(send_grid_size, recv_grid_size)
  recv_map_size = max(send_grid_size, recv_grid_size)
  
  if (send_map_size /= recv_map_size) then
     write(0, *) "send and recv map size invalid"
     stop
  end if
  

  if (my_rank == 0) then

    allocate(send_data_index(send_map_size))
    allocate(recv_data_index(recv_map_size))
    allocate(coef(send_map_size))

    if (send_grid_size > recv_grid_size) then
       counter = 0
       do j = 1, recv_grid_size
          do i = 1, send_recv_ratio
             counter = counter + 1
             recv_data_index(counter) = j
             send_data_index(counter) = counter
             coef(counter) = 1.d0/dble(send_recv_ratio)
          end do
       end do
       
    else
      counter = 0
       do j = 1, send_grid_size
          do i = 1, send_recv_ratio
             counter = counter + 1
             recv_data_index(counter) = counter
             send_data_index(counter) = j
             coef(counter) = 1.d0
          end do
       end do
    end if
    
  else
    allocate(send_data_index(1))
    allocate(recv_data_index(1))
    allocate(coef(1))
  end if    


  call jlt_set_mapping_table(my_comp_name, send_comp_name, send_grid_name, &
       recv_comp_name, recv_grid_name, &
       1, .false., "SAFE", send_data_index, recv_data_index, coef)
 
  call mpi_finalize(i)
  stop
  
end subroutine cal_and_set_map_org

!=======+=========+=========+=========+=========+=========+=========+=========+

subroutine set_data()
  use namelist, only : exchange_data_type, get_num_of_configuration, &
                       get_put_comp_name, get_put_grid_name, &
                       get_get_comp_name, get_get_grid_name, &
                       get_num_of_exchange, get_ed_ptr
  use jlt_interface, only : jlt_set_data
  implicit none
  type(exchange_data_type), pointer :: ed_ptr
  character(len=64) :: put_comp_name, put_grid_name, get_comp_name, get_grid_name
  integer :: i, j

  do i = 1, get_num_of_configuration()
     if (trim(get_put_comp_name(i)) == trim(my_name)) then
        put_comp_name = get_put_comp_name(i)
        put_grid_name = get_put_grid_name(i)
        get_comp_name = get_get_comp_name(i)
        get_grid_name = get_get_grid_name(i)
        do j = 1, get_num_of_exchange(i)
           ed_ptr => get_ed_ptr(i, j)
           call jlt_set_data(trim(put_comp_name), trim(put_grid_name), trim(ed_ptr%var_put), &
                                  trim(get_comp_name), trim(get_grid_name), trim(ed_ptr%var_get), &
                                  1, (trim(ed_ptr%flag) == "AVR"), ed_ptr%intvl, ed_ptr%lag, ed_ptr%num_of_layer, &
                                  ed_ptr%grid_intpl_tag, ed_ptr%fill_value, ed_ptr%mapping_tag)
        end do
     end if
     if (trim(get_get_comp_name(i)) == trim(my_name)) then
        put_comp_name = get_put_comp_name(i)
        put_grid_name = get_put_grid_name(i)
        get_comp_name = get_get_comp_name(i)
        get_grid_name = get_get_grid_name(i)
        do j = 1, get_num_of_exchange(i)
           ed_ptr => get_ed_ptr(i, j)
           call jlt_set_data(trim(put_comp_name), trim(put_grid_name), trim(ed_ptr%var_put), &
                                  trim(get_comp_name), trim(get_grid_name), trim(ed_ptr%var_get), &
                                  1, (trim(ed_ptr%flag) == "AVR"), ed_ptr%intvl, ed_ptr%lag, ed_ptr%num_of_layer, &
                                  ed_ptr%grid_intpl_tag, ed_ptr%fill_value, ed_ptr%mapping_tag)
        end do
     end if
     
  end do
  
end subroutine set_data

!=======+=========+=========+=========+=========+=========+=========+=========+

subroutine init_time(time_array)
  use jlt_interface, only : jlt_init_time
  implicit none
  integer, intent(IN) :: time_array(6)
  integer :: ierror

  call jlt_init_time(time_array)

end subroutine init_time

!=======+=========+=========+=========+=========+=========+=========+=========+

subroutine set_time(comp_name, time_array, delta_t)
  use jlt_interface, only : jlt_set_time
  !use palmtime, only : palm_TimeStart, palm_TimeEnd
  implicit none
  character(len=*), intent(IN) :: comp_name
  integer, intent(IN) :: time_array(6)
  integer(kind=4), intent(IN) :: delta_t
  integer :: i
  
  !call palm_TimeStart("h3ou_set_time")
  call jlt_set_time(comp_name, time_array, delta_t)
  !call palm_TimeEnd("h3ou_set_time")
  
end subroutine set_time

!=======+=========+=========+=========+=========+=========+=========+=========+

subroutine put_data_1d(data_name, data_id, step)
  use jlt_interface, only : jlt_put_data
  implicit none
  character(len=*), intent(IN) :: data_name
  integer, intent(IN) :: data_id, step
  real(kind=8), allocatable :: data(:)
  integer :: i
  
  allocate(data(num_of_local_grid))
  do i = 1, num_of_local_grid
     data(i) = step*10000+data_id*100 + grid_index(i) ! is + i - 1
  end do

  call jlt_put_data(data_name, data)

  deallocate(data)
  
end subroutine put_data_1d

!=======+=========+=========+=========+=========+=========+=========+=========+

subroutine put_data_2d(data_name, data_id, step)
  use jlt_interface, only : jlt_put_data
  implicit none
  character(len=*), intent(IN) :: data_name
  integer, intent(IN) :: data_id, step
  real(kind=8), allocatable :: data(:,:)
  integer :: i, k
  
  allocate(data(num_of_local_grid, NUM_OF_LAYER))
  do k = 1, NUM_OF_LAYER
    do i = 1, num_of_local_grid
       data(i,k) = k*1000000.d0 + step*10000.d0 + data_id*100 + grid_index(i) !num_of_local_grid*my_rank + i
    end do
  end do
 
  call jlt_put_data(data_name, data)

  deallocate(data)
  
end subroutine put_data_2d

  
!=======+=========+=========+=========+=========+=========+=========+=========+

subroutine get_data_1d(data_name)
  use jlt_interface, only : jlt_get_data
  implicit none
  character(len=*), intent(IN) :: data_name
  real(kind=8), allocatable :: data(:)
  logical :: is_get_ok
  integer :: i

  allocate(data(num_of_local_grid))

  data(:) = -1.d0

  call jlt_get_data(data_name, data, is_recv_ok = is_get_ok)

  write(300 + my_comp_id*100 + my_rank, *) int(data)
  
  deallocate(data)

end subroutine get_data_1d

  
!=======+=========+=========+=========+=========+=========+=========+=========+

subroutine get_data_2d(data_name)
  use jlt_interface, only : jlt_get_data
  implicit none
  character(len=*), intent(IN) :: data_name
  real(kind=8), allocatable :: data(:,:)
  logical :: is_get_ok
  integer :: k

  allocate(data(num_of_local_grid, NUM_OF_LAYER))

  data(:,:) = -1.d0

  call jlt_get_data(data_name, data, is_recv_ok = is_get_ok)

  write(300 + my_comp_id*100 + my_rank, *) int(data)

  !if (trim(my_name) == "PADA") then
  !  do k = 1, NUM_OF_LAYER
  !    write(100 + my_comp_id*200 + my_rank_global, *) int(data(1:10,k))
  !  end do
  !end if
 
  deallocate(data)

end subroutine get_data_2d

  
!=======+=========+=========+=========+=========+=========+=========+=========+

subroutine finalize_common()
  use jlt_interface, only : jlt_coupling_end
  !use palmtime, only : palm_TimeFinalize
  implicit none
  integer :: time_array(6)
  integer :: i


  !call palm_TimeFinalize()
  !call mpi_finalize(i)

  call jlt_coupling_end(time_array, isCallFinalize = .true.)
    

end subroutine finalize_common

!=======+=========+=========+=========+=========+=========+=========+=========+

end module common
