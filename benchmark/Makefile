# Taskparts-specific parameters
#   binaries go here:
INSTALL_PATH?=bin
#   root folder of taskparts repository:
TASKPARTS_PATH?=../
TASKPARTS_PLATFORM?=-DTASKPARTS_POSIX -DTASKPARTS_X64

# Other parameters (all nil by default):
#   HWLOC_INCLUDE_PREFIX           include directives for hwloc (optionally empty)
#   HWLOC_LIBRARY_PREFIX           linker directives for hwloc (optionally empty)
#   PARLAYLIB_INCLUDE_PREFIX       include directives needed for parlay
#   PARLAYLIB_SERIAL_PREFIX        include directives needed for parlay
#   PBBSBENCH_INCLUDE_PREFIX       include directives needed for pbbs
#   MINI_UTS_INCLUDE_PREFIX        include directives needed for mini uts benchmark
#   MINI_UTS_LINKER_FLAGS          linker flags needed for mini uts benchmark
#   CHUNKEDSEQ_PREFIX              include directives needed for chunked sequence library
# 

# External parameters
PARLAYLIB_INCLUDE_PREFIX?=-I $(TASKPARTS_PATH)/../parlaylib/include/ -DPARLAY_TASKPARTS
PBBSBENCH_INCLUDE_PREFIX?=-I $(TASKPARTS_PATH)/../pbbsbench/ -Wno-pragmas
MINI_UTS_INCLUDE_PREFIX?=-I $(TASKPARTS_PATH)/../mini-uts/
MINI_UTS_LINKER_FLAGS?=-lm
CHUNKEDSEQ_PREFIX?=-I $(TASKPARTS_PATH)/../chunkedseq/include
CILKRTS_STATS_PREFIX?=

.PHONY: install_folder
install_folder:
	mkdir -p $(INSTALL_PATH)

TASKPARTS_INCLUDE_PATH=$(TASKPARTS_PATH)/include/
TASKPARTS_POSIX_INCLUDE_PATH=$(TASKPARTS_INCLUDE_PATH)/taskparts/posix
TASKPARTS_X64_INCLUDE_PATH=$(TASKPARTS_INCLUDE_PATH)/taskparts/x64
TASKPARTS_BENCHMARK_PATH=.
TASKPARTS_EXAMPLE_PATH=../example

INCLUDE_FILES=\
	$(wildcard $(TASKPARTS_INCLUDE_PATH)/taskparts/*.hpp) \
	$(wildcard $(TASKPARTS_POSIX_INCLUDE_PATH)/*.hpp) \
	$(wildcard $(TASKPARTS_X64_INCLUDE_PATH)/*.hpp) \
	$(wildcard $(TASKPARTS_BENCHMARK_PATH)/*.hpp) \
	$(wildcard $(TASKPARTS_EXAMPLE_PATH)/*.hpp)

INCLUDE_PREFIX=\
	-I $(TASKPARTS_INCLUDE_PATH) \
	$(HWLOC_INCLUDE_PREFIX) \
	$(PARLAYLIB_INCLUDE_PREFIX) \
	$(PBBSBENCH_INCLUDE_PREFIX) \
	$(MINI_UTS_INCLUDE_PREFIX) \
	$(CHUNKEDSEQ_PREFIX)

ifeq (, $(shell which jemalloc-config))
JEMALLOC =
else
JEMALLOCLD = $(shell jemalloc-config --libdir)
JEMALLOC = -L$(JEMALLOCLD) -ljemalloc
endif

# jemalloc needs to go last
LINKER_PREFIX=\
	-pthread \
	$(HWLOC_LIBRARY_PREFIX) \
	$(MINI_UTS_LINKER_FLAGS) \
	$(JEMALLOC)

# WARNING: flag -fno-stack-protector needed for native fork join to
# compile binaries that do not segfault
COMMON_COMPILE_PREFIX=\
	$(INCLUDE_PREFIX) \
	-std=c++17 \
	-fno-stack-protector \
	$(TASKPARTS_PLATFORM)

OPT_COMPILE_PREFIX=\
	$(COMMON_COMPILE_PREFIX) \
	-O3 \
	-march=native \
	-fno-asynchronous-unwind-tables \
	-fomit-frame-pointer \
	-DNDEBUG

OPT_PREFIX=\
	$(OPT_COMPILE_PREFIX)
STA_PREFIX=\
	$(OPT_COMPILE_PREFIX) \
	-DTASKPARTS_STATS
LOG_PREFIX=\
	$(STA_PREFIX) \
	-DTASKPARTS_LOG
DBG_PREFIX=\
	$(COMMON_COMPILE_PREFIX) \
	-Og \
	-g3 \
	-DTASKPARTS_STATS \
	-DTASKPARTS_LOG

# Binaries for native fork join
# -----------------------------

%.opt: %.cpp $(INCLUDE_FILES) install_folder
	$(CXX) $(OPT_PREFIX) -o $(INSTALL_PATH)/$@ $< $(LINKER_PREFIX)
%.sta: %.cpp $(INCLUDE_FILES) install_folder
	$(CXX) $(STA_PREFIX) -o $(INSTALL_PATH)/$@ $< $(LINKER_PREFIX)
%.log: %.cpp $(INCLUDE_FILES) install_folder
	$(CXX) $(LOG_PREFIX) -o $(INSTALL_PATH)/$@ $< $(LINKER_PREFIX)
%.dbg: %.cpp $(INCLUDE_FILES) install_folder
	$(CXX) $(DBG_PREFIX) -o $(INSTALL_PATH)/$@ $< $(LINKER_PREFIX)

# Binaries for serial elision
# ---------------------------

%.elision.opt: %.cpp $(INCLUDE_FILES) install_folder
	$(CXX) $(OPT_PREFIX) -DTASKPARTS_SERIAL_ELISION -o $(INSTALL_PATH)/$@ $< $(LINKER_PREFIX)
%.elision.sta: %.cpp $(INCLUDE_FILES) install_folder
	$(CXX) $(STA_PREFIX) -DTASKPARTS_SERIAL_ELISION -o $(INSTALL_PATH)/$@ $< $(LINKER_PREFIX)
%.elision.log: %.cpp $(INCLUDE_FILES) install_folder
	$(CXX) $(LOG_PREFIX) -DTASKPARTS_SERIAL_ELISION -o $(INSTALL_PATH)/$@ $< $(LINKER_PREFIX)
%.elision.dbg: %.cpp $(INCLUDE_FILES) install_folder
	$(CXX) $(DBG_PREFIX) -DTASKPARTS_SERIAL_ELISION -o $(INSTALL_PATH)/$@ $< $(LINKER_PREFIX)

# Binaries for serial-by-construction programs
# --------------------------------------------

PARLAYLIB_SERIAL_PREFIX=-DPARLAY_SEQUENTIAL -DPARLAY_USE_STD_ALLOC
SERIAL_PREFIX=-DTASKPARTS_SERIAL -DTASKPARTS_SERIAL_ELISION $(PARLAYLIB_SERIAL_PREFIX)

%.serial.opt: %.serial.cpp $(INCLUDE_FILES) install_folder
	$(CXX) $(OPT_PREFIX) $(SERIAL_PREFIX) -o $(INSTALL_PATH)/$@ $< $(LINKER_PREFIX)
%.serial.sta: %.serial.cpp $(INCLUDE_FILES) install_folder
	$(CXX) $(STA_PREFIX) $(SERIAL_PREFIX) -o $(INSTALL_PATH)/$@ $< $(LINKER_PREFIX)
%.serial.log: %.serial.cpp $(INCLUDE_FILES) install_folder
	$(CXX) $(LOG_PREFIX) $(SERIAL_PREFIX) -o $(INSTALL_PATH)/$@ $< $(LINKER_PREFIX)
%.serial.dbg: %.serial.cpp $(INCLUDE_FILES) install_folder
	$(CXX) $(DBG_PREFIX) $(SERIAL_PREFIX) -o $(INSTALL_PATH)/$@ $< $(LINKER_PREFIX)

# Binaries for TPAL, Task Parallel Assembly Language
# --------------------------------------------------

%.tpal.opt: %.tpal.cpp $(INCLUDE_FILES) install_folder
	$(CXX) $(OPT_PREFIX) -DTASKPARTS_TPALRTS $(basename $<)_manual.s -o $(INSTALL_PATH)/$@ $< $(LINKER_PREFIX)
%.tpal.sta: %.tpal.cpp $(INCLUDE_FILES) install_folder
	$(CXX) $(STA_PREFIX) -DTASKPARTS_TPALRTS $(basename $<)_manual.s -o $(INSTALL_PATH)/$@ $< $(LINKER_PREFIX)
%.tpal.log: %.tpal.cpp $(INCLUDE_FILES) install_folder
	$(CXX) $(LOG_PREFIX) -DTASKPARTS_TPALRTS $(basename $<)_manual.s -o $(INSTALL_PATH)/$@ $< $(LINKER_PREFIX)
%.tpal.dbg: %.tpal.cpp $(INCLUDE_FILES) install_folder
	$(CXX) $(DBG_PREFIX) -DTASKPARTS_TPALRTS $(basename $<)_manual.s -o $(INSTALL_PATH)/$@ $< $(LINKER_PREFIX)

# For hand compiling TPAL programs from assembly output
%.tpal_orig.s: %.tpal_orig.cpp $(INCLUDE_FILES) install_folder
	$(CXX) $(OPT_PREFIX) -fno-verbose-asm -mavx2 -mfma -DTASKPARTS_TPALRTS -S  $<

# Binaries for elastic scheduling
# -------------------------------

# Flat
%.elastic_flat.opt: %.cpp $(INCLUDE_FILES) install_folder
	$(CXX) $(OPT_PREFIX) -DTASKPARTS_ELASTIC_FLAT -o $(INSTALL_PATH)/$@ $< $(LINKER_PREFIX)
%.elastic_flat.sta: %.cpp $(INCLUDE_FILES) install_folder
	$(CXX) $(STA_PREFIX) -DTASKPARTS_ELASTIC_FLAT -o $(INSTALL_PATH)/$@ $< $(LINKER_PREFIX)
%.elastic_flat.log: %.cpp $(INCLUDE_FILES) install_folder
	$(CXX) $(LOG_PREFIX) -DTASKPARTS_ELASTIC_FLAT -o $(INSTALL_PATH)/$@ $< $(LINKER_PREFIX)
%.elastic_flat.dbg: %.cpp $(INCLUDE_FILES) install_folder
	$(CXX) $(DBG_PREFIX) -DTASKPARTS_ELASTIC_FLAT -o $(INSTALL_PATH)/$@ $< $(LINKER_PREFIX)

%.elastic_flat_tpal.opt: %.tpal.cpp $(INCLUDE_FILES) install_folder
	$(CXX) $(OPT_PREFIX) -DTASKPARTS_ELASTIC_FLAT -DTASKPARTS_TPALRTS $(basename $<)_manual.s -o $(INSTALL_PATH)/$@ $< $(LINKER_PREFIX)
%.elastic_flat_tpal.sta: %.tpal.cpp $(INCLUDE_FILES) install_folder
	$(CXX) $(STA_PREFIX) -DTASKPARTS_ELASTIC_FLAT -DTASKPARTS_TPALRTS $(basename $<)_manual.s -o $(INSTALL_PATH)/$@ $< $(LINKER_PREFIX)
%.elastic_flat_tpal.log: %.tpal.cpp $(INCLUDE_FILES) install_folder
	$(CXX) $(LOG_PREFIX) -DTASKPARTS_ELASTIC_FLAT -DTASKPARTS_TPALRTS $(basename $<)_manual.s -o $(INSTALL_PATH)/$@ $< $(LINKER_PREFIX)
%.elastic_flat_tpal.dbg: %.tpal.cpp $(INCLUDE_FILES) install_folder
	$(CXX) $(DBG_PREFIX) -DTASKPARTS_ELASTIC_FLAT -DTASKPARTS_TPALRTS $(basename $<)_manual.s -o $(INSTALL_PATH)/$@ $< $(LINKER_PREFIX)

# Flat (spin version)
%.elastic_flat_spin.opt: %.cpp $(INCLUDE_FILES) install_folder
	$(CXX) $(OPT_PREFIX) -DTASKPARTS_ELASTIC_FLAT -DTASKPARTS_USE_SPINNING_SEMAPHORE -o $(INSTALL_PATH)/$@ $< $(LINKER_PREFIX)
%.elastic_flat_spin.sta: %.cpp $(INCLUDE_FILES) install_folder
	$(CXX) $(STA_PREFIX) -DTASKPARTS_ELASTIC_FLAT -DTASKPARTS_USE_SPINNING_SEMAPHORE -o $(INSTALL_PATH)/$@ $< $(LINKER_PREFIX)
%.elastic_flat_spin.log: %.cpp $(INCLUDE_FILES) install_folder
	$(CXX) $(LOG_PREFIX) -DTASKPARTS_ELASTIC_FLAT -DTASKPARTS_USE_SPINNING_SEMAPHORE -o $(INSTALL_PATH)/$@ $< $(LINKER_PREFIX)
%.elastic_flat_spin.dbg: %.cpp $(INCLUDE_FILES) install_folder
	$(CXX) $(DBG_PREFIX) -DTASKPARTS_ELASTIC_FLAT -DTASKPARTS_USE_SPINNING_SEMAPHORE -o $(INSTALL_PATH)/$@ $< $(LINKER_PREFIX)

%.elastic_flat_spin_tpal.opt: %.tpal.cpp $(INCLUDE_FILES) install_folder
	$(CXX) $(OPT_PREFIX) -DTASKPARTS_ELASTIC_FLAT -DTASKPARTS_USE_SPINNING_SEMAPHORE -DTASKPARTS_TPALRTS $(basename $<)_manual.s -o $(INSTALL_PATH)/$@ $< $(LINKER_PREFIX)
%.elastic_flat_spin_tpal.sta: %.tpal.cpp $(INCLUDE_FILES) install_folder
	$(CXX) $(STA_PREFIX) -DTASKPARTS_ELASTIC_FLAT -DTASKPARTS_USE_SPINNING_SEMAPHORE -DTASKPARTS_TPALRTS $(basename $<)_manual.s -o $(INSTALL_PATH)/$@ $< $(LINKER_PREFIX)
%.elastic_flat_spin_tpal.log: %.tpal.cpp $(INCLUDE_FILES) install_folder
	$(CXX) $(LOG_PREFIX) -DTASKPARTS_ELASTIC_FLAT -DTASKPARTS_USE_SPINNING_SEMAPHORE -DTASKPARTS_TPALRTS $(basename $<)_manual.s -o $(INSTALL_PATH)/$@ $< $(LINKER_PREFIX)
%.elastic_flat_spin_tpal.dbg: %.tpal.cpp $(INCLUDE_FILES) install_folder
	$(CXX) $(DBG_PREFIX) -DTASKPARTS_ELASTIC_FLAT -DTASKPARTS_USE_SPINNING_SEMAPHORE -DTASKPARTS_TPALRTS $(basename $<)_manual.s -o $(INSTALL_PATH)/$@ $< $(LINKER_PREFIX)

# Multiprogrammed
%.multiprogrammed.opt: %.cpp $(INCLUDE_FILES) install_folder
	$(CXX) $(OPT_PREFIX) -DTASKPARTS_MULTIPROGRAMMED -o $(INSTALL_PATH)/$@ $< $(LINKER_PREFIX)
%.multiprogrammed.sta: %.cpp $(INCLUDE_FILES) install_folder
	$(CXX) $(STA_PREFIX) -DTASKPARTS_MULTIPROGRAMMED -o $(INSTALL_PATH)/$@ $< $(LINKER_PREFIX)
%.multiprogrammed.log: %.cpp $(INCLUDE_FILES) install_folder
	$(CXX) $(LOG_PREFIX) -DTASKPARTS_MULTIPROGRAMMED -o $(INSTALL_PATH)/$@ $< $(LINKER_PREFIX)
%.multiprogrammed.dbg: %.cpp $(INCLUDE_FILES) install_folder
	$(CXX) $(DBG_PREFIX) -DTASKPARTS_MULTIPROGRAMMED -o $(INSTALL_PATH)/$@ $< $(LINKER_PREFIX)

%.multiprogrammed_tpal.opt: %.tpal.cpp $(INCLUDE_FILES) install_folder
	$(CXX) $(OPT_PREFIX) -DTASKPARTS_MULTIPROGRAMMED -DTASKPARTS_TPALRTS $(basename $<)_manual.s -o $(INSTALL_PATH)/$@ $< $(LINKER_PREFIX)
%.multiprogrammed_tpal.sta: %.tpal.cpp $(INCLUDE_FILES) install_folder
	$(CXX) $(STA_PREFIX) -DTASKPARTS_MULTIPROGRAMMED -DTASKPARTS_TPALRTS $(basename $<)_manual.s -o $(INSTALL_PATH)/$@ $< $(LINKER_PREFIX)
%.multiprogrammed_tpal.log: %.tpal.cpp $(INCLUDE_FILES) install_folder
	$(CXX) $(LOG_PREFIX) -DTASKPARTS_MULTIPROGRAMMED -DTASKPARTS_TPALRTS $(basename $<)_manual.s -o $(INSTALL_PATH)/$@ $< $(LINKER_PREFIX)
%.multiprogrammed_tpal.dbg: %.tpal.cpp $(INCLUDE_FILES) install_folder
	$(CXX) $(DBG_PREFIX) -DTASKPARTS_MULTIPROGRAMMED -DTASKPARTS_TPALRTS $(basename $<)_manual.s -o $(INSTALL_PATH)/$@ $< $(LINKER_PREFIX)

# Binaries for Cilk
# -----------------

CILK_COMMON_PREFIX=-fcilkplus -lcilkrts -DCILK -ldl -DPARLAY_CILK

%.cilk.opt: %.cilk.cpp $(INCLUDE_FILES) install_folder
	$(CXX) $(OPT_PREFIX) $(CILK_COMMON_PREFIX) -o $(INSTALL_PATH)/$@ $< $(LINKER_PREFIX)
# TODO: enable cilk w/ stats
%.cilk.sta: %.cilk.cpp $(INCLUDE_FILES) install_folder
	$(CXX) $(OPT_PREFIX) -o $(INSTALL_PATH)/$@ $< $(CILKRTS_STATS_PREFIX) $(CILK_COMMON_PREFIX) $(LINKER_PREFIX)
%.cilk.dbg: %.cilk.cpp $(INCLUDE_FILES) install_folder
	$(CXX) $(DBG_PREFIX) $(CILK_COMMON_PREFIX) -o $(INSTALL_PATH)/$@ $< $(LINKER_PREFIX)

# Assembly code
# -------------

%.s: %.cpp $(INCLUDE_FILES) install_folder
	$(CXX) $(OPT_PREFIX) -fno-verbose-asm -S $<


clean:
	rm -rf bin gen_rollforward

# Benchmarks
# ----------

BENCHMARKS=\
	samplesort \
	quicksort \
	quickhull \
	removeduplicates \
	suffixarray \
	histogram \
	bfs \
	delaunay \
	index \
	classify \
	raycast \
	mcss \
	wc \
	integrate \
	bignumadd \
	grep

#	nbody \
#	mis \
#	nearest \

TPAL_BENCHMARKS=\
	pdfs

	# sum_tree \
	# spmv \
	# srad

all_elastic_flat_opt: $(BENCHMARKS:%=%.elastic_flat.opt)
all_elastic_flat_sta: $(BENCHMARKS:%=%.elastic_flat.sta)
all_elastic_flat_log: $(BENCHMARKS:%=%.elastic_flat.log)
all_elastic_flat_dbg: $(BENCHMARKS:%=%.elastic_flat.dbg)

all_elastic_flat_spin_opt: $(BENCHMARKS:%=%.elastic_flat_spin.opt)
all_elastic_flat_spin_sta: $(BENCHMARKS:%=%.elastic_flat_spin.sta)
all_elastic_flat_spin_log: $(BENCHMARKS:%=%.elastic_flat_spin.log)
all_elastic_flat_spin_dbg: $(BENCHMARKS:%=%.elastic_flat_spin.dbg)

all_cilk_opt: $(BENCHMARKS:%=%.cilk.opt)
all_cilk_sta: $(BENCHMARKS:%=%.cilk.sta)

all_serial_opt: $(BENCHMARKS:%=%.serial.opt)
all_serial_sta: $(BENCHMARKS:%=%.serial.sta)

all_opt: all_taskparts_opt all_cilk_opt all_serial_opt
all_sta: all_taskparts_sta all_cilk_sta all_serial_sta

all_tpal_serial: $(TPAL_BENCHMARKS:%=%.serial.opt) $(TPAL_BENCHMARKS:%=%.serial.sta)
all_tpal_cilk: $(TPAL_BENCHMARKS:%=%.cilk.opt) $(TPAL_BENCHMARKS:%=%.cilk.sta)

all_tpal_opt: $(TPAL_BENCHMARKS:%=%.tpal.opt)
all_tpal_sta: $(TPAL_BENCHMARKS:%=%.tpal.sta)
all_tpal_log: $(TPAL_BENCHMARKS:%=%.tpal.log)
all_tpal_dbg: $(TPAL_BENCHMARKS:%=%.tpal.dbg)

all_elastic_flat_tpal_opt: $(TPAL_BENCHMARKS:%=%.elastic_flat_tpal.opt)
all_elastic_flat_tpal_sta: $(TPAL_BENCHMARKS:%=%.elastic_flat_tpal.sta)
all_elastic_flat_tpal_log: $(TPAL_BENCHMARKS:%=%.elastic_flat_tpal.log)
all_elastic_flat_tpal_dbg: $(TPAL_BENCHMARKS:%=%.elastic_flat_tpal.dbg)

all_elastic_flat_spin_tpal_opt: $(TPAL_BENCHMARKS:%=%.elastic_flat_spin_tpal.opt)
all_elastic_flat_spin_tpal_sta: $(TPAL_BENCHMARKS:%=%.elastic_flat_spin_tpal.sta)
all_elastic_flat_spin_tpal_log: $(TPAL_BENCHMARKS:%=%.elastic_flat_spin_tpal.log)
all_elastic_flat_spin_tpal_dbg: $(TPAL_BENCHMARKS:%=%.elastic_flat_spin_tpal.dbg)

%.taskparts.opt: %.opt
	ln -f -s $(abspath $(INSTALL_PATH)/$*.opt) $(INSTALL_PATH)/$*.taskparts.opt
%.taskparts.sta: %.sta
	ln -f -s $(abspath $(INSTALL_PATH)/$*.sta) $(INSTALL_PATH)/$*.taskparts.sta
%.taskparts.log: %.log
	ln -f -s $(abspath $(INSTALL_PATH)/$*.log) $(INSTALL_PATH)/$*.taskparts.log
%.taskparts.dbg: %.dbg
	ln -f -s $(abspath $(INSTALL_PATH)/$*.dbg) $(INSTALL_PATH)/$*.taskparts.dbg

all_taskparts_opt: $(BENCHMARKS:%=%.taskparts.opt)
all_taskparts_sta: $(BENCHMARKS:%=%.taskparts.sta)
all_taskparts_log: $(BENCHMARKS:%=%.taskparts.log)
all_taskparts_dbg: $(BENCHMARKS:%=%.taskparts.dbg)

%.taskparts_tpal.opt: %.tpal.opt
	ln -f -s $(abspath $(INSTALL_PATH)/$*.tpal.opt) $(INSTALL_PATH)/$*.taskparts_tpal.opt
%.taskparts_tpal.sta: %.tpal.sta
	ln -f -s $(abspath $(INSTALL_PATH)/$*.tpal.sta) $(INSTALL_PATH)/$*.taskparts_tpal.sta
%.taskparts_tpal.log: %.tpal.log
	ln -f -s $(abspath $(INSTALL_PATH)/$*.tpal.log) $(INSTALL_PATH)/$*.taskparts_tpal.log
%.taskparts_tpal.dbg: %.tpal.dbg
	ln -f -s $(abspath $(INSTALL_PATH)/$*.tpal.dbg) $(INSTALL_PATH)/$*.taskparts_tpal.dbg

all_taskparts_tpal_opt: $(TPAL_BENCHMARKS:%=%.taskparts_tpal.opt)
all_taskparts_tpal_sta: $(TPAL_BENCHMARKS:%=%.taskparts_tpal.sta)
all_taskparts_tpal_log: $(TPAL_BENCHMARKS:%=%.taskparts_tpal.log)
all_taskparts_tpal_dbg: $(TPAL_BENCHMARKS:%=%.taskparts_tpal.dbg)

all_multiprogrammed_opt: $(BENCHMARKS:%=%.multiprogrammed.opt)
all_multiprogrammed_sta: $(BENCHMARKS:%=%.multiprogrammed.sta)
all_multiprogrammed_log: $(BENCHMARKS:%=%.multiprogrammed.log)
all_multiprogrammed_dbg: $(BENCHMARKS:%=%.multiprogrammed.dbg)

all_elastic_tpal: all_taskparts_tpal_sta all_elastic_flat_tpal_sta all_elastic_flat_spin_tpal_sta all_tpal_serial #all_tpal_cilk

all_elastic: all_taskparts_sta all_elastic_flat_spin_sta all_elastic_flat_sta all_elastic_tpal all_cilk_sta all_serial_sta all_multiprogrammed_sta
