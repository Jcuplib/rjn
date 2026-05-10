TOPDIR = ..
include $(TOPDIR)/Mkinclude

TESTSRCDIR = $(TOPDIR)/src_fort

all: setup librjn tests

setup:
	test -d $(LIBDIR) || mkdir -p $(LIBDIR)
	test -d $(MODDIR) || mkdir -p $(MODDIR)

librjn: setup
	$(MAKE) -C $(RJNSRCDIR) TOPDIR=$(abspath $(TOPDIR)) all

tests: librjn
	$(MAKE) -C $(TESTSRCDIR)
	cp $(TESTSRCDIR)/comp1 .
	cp $(TESTSRCDIR)/comp2 .

clean:
	$(MAKE) -C $(TESTSRCDIR) clean
	-$(RM) -f comp1 comp2

distclean: clean
	$(MAKE) -C $(RJNSRCDIR) TOPDIR=$(abspath $(TOPDIR)) clean
	$(RM) -rf $(LIBDIR) $(MODDIR)
