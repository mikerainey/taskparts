PDFLATEX = pdflatex -file-line-error -halt-on-error

VIEWS = perf/fib/efib perf/parlay/etime perf/parlay/euser perf/parlay/esys \
	perf/gamora/fib/efibgamora perf/gamora/parlay/etimegamora \
	perf/gamora/parlay/eusergamora perf/gamora/parlay/esysgamora \
	perf/icelake/fib2/kfibicelake \
	$(addprefix perf/icelake/thp/,$(addsuffix icelake,htime huser hsys)) \
	perf/icelake/stacks/ktimeicelake \
	$(addprefix perf/icelake/patience2/,$(addsuffix icelake,qtime quser qsys qidle qsleeps)) \
	perf/icelake/stacks/kusericelake perf/icelake/stacks/ksysicelake \
	$(addprefix perf/gamora/suite/,$(addsuffix gamora,stime suser ssys \
	  fwork fsched fidle fasleep fsteals fsleeps fforkwakes fthiefwakes)) \
	$(addprefix perf/icelake/suite/,$(addsuffix icelake,stime suser ssys \
	  fwork fsched fidle fasleep fsteals fsleeps fforkwakes fthiefwakes))
BIBS = perf/fib/efib.bib perf/parlay/eparlay.bib \
	perf/gamora/fib/efibgamora.bib perf/gamora/parlay/eparlaygamora.bib \
	perf/icelake/fib2/efib2icelake.bib perf/icelake/patience2/epatience2icelake.bib \
	perf/icelake/stacks/estacksicelake.bib perf/gamora/suite/esuitegamora.bib \
	perf/icelake/suite/esuiteicelake.bib perf/icelake/thp/ethpicelake.bib \
	../native-fork-join/perf/icelake/percore/suitepercore.bib

default: main.pdf

main.pdf: main.tex local.tex plots.tex main.bib $(addsuffix .tex,$(VIEWS)) $(BIBS)
	$(PDFLATEX) main
	bibtex main
	$(PDFLATEX) main
	$(PDFLATEX) main

clean:
	rm -f *.aux main.bbl main.blg *.log *.out main.pdf

.PHONY: default clean
