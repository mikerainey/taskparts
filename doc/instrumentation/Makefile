PDFLATEX = pdflatex -file-line-error -halt-on-error

default: main.pdf

main.pdf: main.tex local.tex main.bib $(wildcard perf/*.tex) perf/times.bib \
  $(wildcard perf/bench/*.tex) perf/bench/bench.bib \
  plots.tex $(wildcard perf/curves/*.tex) perf/curves/curves.bib \
  $(wildcard perf/gamora/*.tex) $(wildcard perf/gamora/*/*.tex) perf/gamora/gamora.bib
	$(PDFLATEX) main
	bibtex main
	$(PDFLATEX) main
	$(PDFLATEX) main

clean:
	rm -f *.aux main.bbl main.blg *.log *.out main.pdf

.PHONY: default clean
