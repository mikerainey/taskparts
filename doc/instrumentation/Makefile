PDFLATEX = pdflatex -file-line-error -halt-on-error

default: main.pdf

main.pdf: main.tex local.tex main.bib $(wildcard perf/*.tex) perf/times.bib \
  $(wildcard perf/bench/*.tex) perf/bench/bench.bib \
  plots.tex $(wildcard perf/curves/*.tex) perf/curves/curves.bib \
  $(wildcard perf/gamora/*.tex) $(wildcard perf/gamora/*/*.tex) perf/gamora/gamora.bib \
  $(wildcard perf/suite/*.tex) perf/suite/suite.bib \
  perf/gamora/suite/suitegamora.bib \
  $(wildcard perf/icelake/suite/*.tex) perf/icelake/suite/suiteicelake.bib \
  $(wildcard perf/icelake/dense/*.tex) perf/icelake/dense/denseicelake.bib \
  $(wildcard perf/icelake/thp/*.tex) perf/icelake/thp/thpicelake.bib
	$(PDFLATEX) main
	bibtex main
	$(PDFLATEX) main
	$(PDFLATEX) main

clean:
	rm -f *.aux main.bbl main.blg *.log *.out main.pdf

.PHONY: default clean
