TOPDIR = ..
include $(TOPDIR)/Mkinclude

INCFLAGS = -I$(RJNSRCDIR)

COMMON_OBJS = namelist.o common.o

TARGET1 = comp1_cpp
TARGET2 = comp2_cpp

all: $(TARGET1) $(TARGET2)

$(TARGET1): $(COMMON_OBJS) comp1.o
	$(CXX) $(CXXFLAGS) -o $@ $^ $(RJNLIB) -lmpi

$(TARGET2): $(COMMON_OBJS) comp2.o
	$(CXX) $(CXXFLAGS) -o $@ $^ $(RJNLIB) -lmpi

clean:
	-$(RM) -f *.o $(TARGET1) $(TARGET2)

.SUFFIXES:
.SUFFIXES: .o .cpp

.cpp.o:
	$(CXX) $(CXXFLAGS) $(INCFLAGS) -c $<
