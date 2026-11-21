PDFLATEX = pdflatex -file-line-error -halt-on-error

default: main.pdf

main.pdf: main.tex local.tex main.bib perf/times.tex perf/times.bib \
		perf/parlay/parlay.tex perf/parlay/parlay.bib \
		perf/gamora/timesgamora.tex perf/gamora/timesgamora.bib \
		perf/gamora/parlay/parlaygamora.tex perf/gamora/parlay/parlaygamora.bib \
		perf/suite/suite.tex perf/suite/suite.bib \
		perf/gamora/suite/suitegamora.tex perf/gamora/suite/suitegamora.bib \
		perf/suite/karatsuba/suitek.tex perf/suite/karatsuba/suitek.bib \
		perf/gamora/suite/karatsuba/suitekgamora.tex perf/gamora/suite/karatsuba/suitekgamora.bib
	$(PDFLATEX) main
	bibtex main
	$(PDFLATEX) main
	$(PDFLATEX) main

clean:
	rm -f *.aux main.bbl main.blg *.log *.out main.pdf

.PHONY: default clean
