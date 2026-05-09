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
	rjn_interface.o

all: $(RJNLIB)
	@echo "Complete making rjn (C++)."

$(RJNLIB): $(RJNOBJS)
	$(AR) $(ARFLAGS) $@ $^
	$(RANLIB) $@

clean:
	-$(RM) -f *.o

.SUFFIXES:
.SUFFIXES: .o .cpp

.cpp.o:
	$(CXX) $(CXXFLAGS) -I. -c $<
