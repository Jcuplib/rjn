TOPDIR = ..
include $(TOPDIR)/Mkinclude

.SUFFIXES: .f90 .o

OBJS    = constant.o namelist.o common.o

TARGET1 = comp1
TARGET2 = comp2

all: $(TARGET1) $(TARGET2)

$(TARGET1): $(OBJS) comp1.o
	$(FC) $(LDFLAGS) -o $@ $^ $(RJNLIB)

$(TARGET2): $(OBJS) comp2.o
	$(FC) $(LDFLAGS) -o $@ $^ $(RJNLIB)

clean:
	-$(RM) -f *.mod *.o $(TARGET1) $(TARGET2)

.f90.o:
	$(FC) $(FFLAGS) -I$(MODDIR) -c $<
