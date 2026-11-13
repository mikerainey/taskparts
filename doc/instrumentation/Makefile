PDFLATEX = pdflatex -file-line-error -halt-on-error

default: main.pdf

main.pdf: main.tex local.tex main.bib $(wildcard perf/*.tex) perf/times.bib \
  $(wildcard perf/bench/*.tex) perf/bench/bench.bib perf/relaunch/sched.tex \
  plots.tex $(wildcard perf/curves/*.tex) perf/curves/curves.bib
	$(PDFLATEX) main
	bibtex main
	$(PDFLATEX) main
	$(PDFLATEX) main

clean:
	rm -f *.aux main.bbl main.blg *.log *.out main.pdf

.PHONY: default clean
