TEX = Raijin

.PHONY: all clean distclean

all:
	latexmk -pdfdvi $(TEX).tex

clean:
	latexmk -c $(TEX).tex
	rm -f $(TEX).dvi

distclean:
	latexmk -C $(TEX).tex
	rm -f $(TEX).dvi
