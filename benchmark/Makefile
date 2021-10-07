# binaries go here:
INSTALL_PATH=bin
# root folder of taskparts repository:
TASKPARTS_PATH=../
# Other parameters (all nil by default):
#   HWLOC_INCLUDE_PREFIX           include directives for hwloc (optionally empty)
#   HWLOC_LIBRARY_PREFIX           linker directives for hwloc (optionally empty)
#   TASKPARTS_ELASTIC_PREFIX       build configuration for elastic work stealing
#   PARLAYLIB_INCLUDE_PREFIX       include directives needed for parlay
#   PARLAYLIB_SERIAL_PREFIX       include directives needed for parlay
#   PBBSBENCH_INCLUDE_PREFIX       include directives needed for pbbs
#   MINI_UTS_PREFIX                include directives needed for mini uts benchmark
#   CHUNKEDSEQ_PREFIX              include directives needed for chunked sequence library

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
	$(MINI_UTS_PREFIX) \
	$(CHUNKEDSEQ_PREFIX)

LINKER_PREFIX=\
	-pthread \
	$(HWLOC_LIBRARY_PREFIX)

# WARNING: flag -fno-stack-protector needed for native fork join to compile safely 
COMMON_COMPILE_PREFIX=\
	$(INCLUDE_PREFIX) \
	-std=c++17 \
	-m64 \
	-fno-stack-protector \
	-DTASKPARTS_POSIX \
	-DTASKPARTS_X64 \
	$(TASKPARTS_ELASTIC_PREFIX)

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

# Binaries for serial elision
# ---------------------------

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

# Binaries for tpal, task parallel assembly language
# --------------------------------------------------

%.tpal.opt: %.tpal.cpp $(INCLUDE_FILES) install_folder
	$(CXX) $(OPT_PREFIX) -DTASKPARTS_TPALRTS $(basename $<)_manual.s -o $(INSTALL_PATH)/$@ $< $(LINKER_PREFIX)
%.tpal.sta: %.tpal.cpp $(INCLUDE_FILES) install_folder
	$(CXX) $(STA_PREFIX) -DTASKPARTS_TPALRTS $(basename $<)_manual.s -o $(INSTALL_PATH)/$@ $< $(LINKER_PREFIX)
%.tpal.log: %.tpal.cpp $(INCLUDE_FILES) install_folder
	$(CXX) $(LOG_PREFIX) -DTASKPARTS_TPALRTS $(basename $<)_manual.s -o $(INSTALL_PATH)/$@ $< $(LINKER_PREFIX)
%.tpal.dbg: %.tpal.cpp $(INCLUDE_FILES) install_folder
	$(CXX) $(DBG_PREFIX) -DTASKPARTS_TPALRTS $(basename $<)_manual.s -o $(INSTALL_PATH)/$@ $< $(LINKER_PREFIX)

# Binaries for elastic scheduling
# -------------------------------

%.elastic.opt: %.cpp $(INCLUDE_FILES) install_folder
	$(CXX) $(OPT_PREFIX) -DTASKPARTS_ELASTIC -o $(INSTALL_PATH)/$@ $< $(LINKER_PREFIX)
%.elastic.sta: %.cpp $(INCLUDE_FILES) install_folder
	$(CXX) $(STA_PREFIX) -DTASKPARTS_ELASTIC -o $(INSTALL_PATH)/$@ $< $(LINKER_PREFIX)
%.elastic.log: %.cpp $(INCLUDE_FILES) install_folder
	$(CXX) $(LOG_PREFIX) -DTASKPARTS_ELASTIC -o $(INSTALL_PATH)/$@ $< $(LINKER_PREFIX)
%.elastic.dbg: %.cpp $(INCLUDE_FILES) install_folder
	$(CXX) $(DBG_PREFIX) -DTASKPARTS_ELASTIC -o $(INSTALL_PATH)/$@ $< $(LINKER_PREFIX)

# Binaries for Cilk
# -----------------

CILK_COMMON_PREFIX=-fcilkplus -lcilkrts -DCILK -ldl -DPARLAY_CILK

%.cilk.opt: %.cilk.cpp $(INCLUDE_FILES) install_folder
	$(CXX) $(OPT_PREFIX) $(CILK_COMMON_PREFIX) -o $(INSTALL_PATH)/$@ $< $(LINKER_PREFIX)
# TODO: enable cilk w/ stats
%.cilk.sta: %.cilk.cpp $(INCLUDE_FILES) install_folder
	$(CXX) $(OPT_PREFIX) $(CILK_COMMON_PREFIX) -o $(INSTALL_PATH)/$@ $< $(LINKER_PREFIX)
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
	bfs \
	fib \
	integrate \
	mcss \
	primes \
	quickhull \
	removeduplicates \
	samplesort \
	suffixarray \
	wc

all_taskparts_opt: $(BENCHMARKS:%=%.opt)
all_taskparts_sta: $(BENCHMARKS:%=%.sta)
all_taskparts_log: $(BENCHMARKS:%=%.log)
all_taskparts_dbg: $(BENCHMARKS:%=%.dbg)

all_elastic_opt: $(BENCHMARKS:%=%.elastic.opt)
all_elastic_sta: $(BENCHMARKS:%=%.elastic.sta)
all_elastic_log: $(BENCHMARKS:%=%.elastic.log)
all_elastic_dbg: $(BENCHMARKS:%=%.elastic.dbg)

all_cilk_opt: $(BENCHMARKS:%=%.cilk.opt)
all_cilk_sta: $(BENCHMARKS:%=%.cilk.sta)

all_serial_opt: $(BENCHMARKS:%=%.serial.opt)
all_serial_sta: $(BENCHMARKS:%=%.serial.sta)

all_opt: all_taskparts_opt all_cilk_opt all_serial_opt
all_sta: all_taskparts_sta all_cilk_sta all_serial_sta

# e.g.,
# make PARLAYLIB_INCLUDE_PREFIX="-I /home/rainey/Work/parlaylib+taskparts/include/ -DPARLAY_TASKPARTS" PBBSBENCH_INCLUDE_PREFIX="-I /home/rainey/Work/pbbsbench/ -Wno-pragmas" -j all_elastic_sta all_serial_sta all_cilk_sta
