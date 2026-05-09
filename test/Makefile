TOPDIR = .
include Mkinclude

all: setup librjn tests

setup:
	test -d $(BINDIR) || mkdir -p $(BINDIR)
	test -d $(LIBDIR) || mkdir -p $(LIBDIR)
	test -d $(MODDIR) || mkdir -p $(MODDIR)

librjn: setup
	$(MAKE) -C $(RJNSRCDIR) TOPDIR=$(abspath $(TOPDIR)) all

tests: librjn
	$(MAKE) -C src

allclean: clean
	$(RM) -rf $(BINDIR)
	$(RM) -rf $(LIBDIR)
	$(RM) -rf $(MODDIR)

clean:
	$(MAKE) -C src clean
	-$(MAKE) -C $(RJNSRCDIR) TOPDIR=$(abspath $(TOPDIR)) clean
