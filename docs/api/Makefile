DOXYFILE = Doxyfile

.PHONY: all clean open

all:
	doxygen $(DOXYFILE)

open:
	xdg-open html/index.html

clean:
	rm -rf html latex xml
