TOPDIR = ..
include $(TOPDIR)/Mkinclude

TESTSRCDIR = $(TOPDIR)/src

all: setup librjn tests

setup:
	test -d $(LIBDIR) || mkdir -p $(LIBDIR)
	test -d $(MODDIR) || mkdir -p $(MODDIR)

librjn: setup
	$(MAKE) -C $(RJNSRCDIR) TOPDIR=$(abspath $(TOPDIR)) all

tests: librjn
	$(MAKE) -C $(TESTSRCDIR)
	cp $(TESTSRCDIR)/comp1_cpp .
	cp $(TESTSRCDIR)/comp2_cpp .

clean:
	$(MAKE) -C $(TESTSRCDIR) clean
	-$(RM) -f comp1_cpp comp2_cpp

distclean: clean
	-$(MAKE) -C $(RJNSRCDIR) TOPDIR=$(abspath $(TOPDIR)) clean
	$(RM) -rf $(LIBDIR) $(MODDIR)
