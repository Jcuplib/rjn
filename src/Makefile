TOPDIR = ../test
include ${TOPDIR}/Mkinclude

RJNOBJS = \
	rjn_mpi_lib.o \
	rjn_utils.o \
	rjn_time.o \
	rjn_comp.o \
	rjn_grid_class.o \
	rjn_grid.o \
	rjn_remapping.o \
	rjn_exchange_class.o \
	rjn_exchange.o \
	rjn_data_class.o \
	rjn_data.o \
	rjn_interface.o \
	jlt_c_helpers.o \
	jlt_interface.o

all: $(MODDIR) $(RJNLIB)
	@echo "Complete making rjn (C++ + Fortran interface)."

$(MODDIR):
	mkdir -p $(MODDIR)

$(RJNLIB): $(RJNOBJS)
	$(AR) $(ARFLAGS) $@ $^
	$(RANLIB) $@

clean:
	-$(RM) -f *.o *.mod

.SUFFIXES:
.SUFFIXES: .o .cpp .c .f90

.cpp.o:
	$(CXX) $(CXXFLAGS) -I. -c $<

.c.o:
	$(CC) $(CFLAGS) -I. -c $<

.f90.o:
	$(FC) $(FFLAGS) -I. $(MODFLAG) -c $<

# jlt_interface.f90 has no dependencies on other .mod files in this dir
jlt_interface.o: jlt_interface.f90
	$(FC) $(FFLAGS) -I. $(MODFLAG) -c $<
