# Build parameters
# ================

include ../Makefile_benchmark
TASKPARTS_PATH=../../

PARLAYLIB?=-I$(PARLAYLIB_PATH)/include/
PARLAYLIB_EXAMPLES?=-I$(PARLAYLIB_PATH)/share/examples/
# -DPARLAY_USE_STD_ALLOC disables the pool allocator in parlaylib
PARLAYLIB_EXTRAS?=
PARLAYLIB_PREFIX?=$(PARLAYLIB) $(PARLAYLIB_EXAMPLES) $(PARLAYLIB_EXTRAS)

PBBSBENCH_PATH?=$(TASKPARTS_PATH)/../pbbsbench/
PBBSBENCH_PREFIX?=-I $(PBBSBENCH_PATH) -Wno-pragmas

CHUNKEDSEQ_PATH?=$(TASKPARTS_PATH)/../chunkedseq/
CHUNKEDSEQ_PREFIX?=-I $(CHUNKEDSEQ_PATH)/include

TASKPARTS_WORKSTEALING_DEQUE?=

INCLUDE_PREFIX_ALL=$(INCLUDE_PREFIX) $(PARLAYLIB_PREFIX) $(PBBSBENCH_PREFIX) $(CHUNKEDSEQ_PREFIX)

TASKPARTS_INCLUDE_PREFIX=$(INCLUDE_PREFIX_ALL) -DPARLAY_TASKPARTS $(TASKPARTS_WORKSTEALING_DEQUE)

# Binaries
# ========

# Default (parallel)
# -------

%.opt: %.cpp $(INCLUDE_FILES) install_folder
	$(CXX) $(OPT_PREFIX) $<  $(TASKPARTS_INCLUDE_PREFIX) -o $(INSTALL_PATH)/$@ $(LINKER_PREFIX)
%.sta: %.cpp $(INCLUDE_FILES) install_folder
	$(CXX) $(STA_PREFIX) $< $(TASKPARTS_INCLUDE_PREFIX) -o $(INSTALL_PATH)/$@ $(LINKER_PREFIX)
%.log: %.cpp $(INCLUDE_FILES) install_folder
	$(CXX) $(LOG_PREFIX) $< $(TASKPARTS_INCLUDE_PREFIX) -o $(INSTALL_PATH)/$@ $(LINKER_PREFIX)
%.dbg: %.cpp $(INCLUDE_FILES) install_folder
	$(CXX) $(DBG_PREFIX) $< $(TASKPARTS_INCLUDE_PREFIX) -o $(INSTALL_PATH)/$@ $(LINKER_PREFIX)

# Serial
# ------

INCLUDE_PREFIX_SERIAL=$(TASKPARTS_INCLUDE_PREFIX) -DTASKPARTS_SERIAL_ELISION

%.serial.opt: %.serial.cpp $(INCLUDE_FILES) install_folder
	$(CXX) $(OPT_PREFIX) $<  $(INCLUDE_PREFIX_SERIAL) -o $(INSTALL_PATH)/$@ $(LINKER_PREFIX)
%.serial.sta: %.serial.cpp $(INCLUDE_FILES) install_folder
	$(CXX) $(STA_PREFIX) $< $(INCLUDE_PREFIX_SERIAL) -o $(INSTALL_PATH)/$@ $(LINKER_PREFIX)
%.serial.log: %.serial.cpp $(INCLUDE_FILES) install_folder
	$(CXX) $(LOG_PREFIX) $< $(INCLUDE_PREFIX_SERIAL) -o $(INSTALL_PATH)/$@ $(LINKER_PREFIX)
%.serial.dbg: %.serial.cpp $(INCLUDE_FILES) install_folder
	$(CXX) $(DBG_PREFIX) $< $(INCLUDE_PREFIX_SERIAL) -o $(INSTALL_PATH)/$@ $(LINKER_PREFIX)

# Multiprogrammed
# ---------------

INCLUDE_PREFIX_MULTIPROGRAMMED=$(TASKPARTS_INCLUDE_PREFIX) -DTASKPARTS_MULTIPROGRAMMED

%.multiprogrammed.opt: %.cpp $(INCLUDE_FILES) install_folder
	$(CXX) $(OPT_PREFIX) $<  $(TASKPARTS_INCLUDE_PREFIX) $(INCLUDE_PREFIX_MULTIPROGRAMMED) -o $(INSTALL_PATH)/$@ $(LINKER_PREFIX)
%.multiprogrammed.sta: %.cpp $(INCLUDE_FILES) install_folder
	$(CXX) $(STA_PREFIX) $< $(TASKPARTS_INCLUDE_PREFIX) $(INCLUDE_PREFIX_MULTIPROGRAMMED) -o $(INSTALL_PATH)/$@ $(LINKER_PREFIX)
%.multiprogrammed.log: %.cpp $(INCLUDE_FILES) install_folder
	$(CXX) $(LOG_PREFIX) $< $(TASKPARTS_INCLUDE_PREFIX) $(INCLUDE_PREFIX_MULTIPROGRAMMED) -o $(INSTALL_PATH)/$@ $(LINKER_PREFIX)
%.multiprogrammed.dbg: %.cpp $(INCLUDE_FILES) install_folder
	$(CXX) $(DBG_PREFIX) $< $(TASKPARTS_INCLUDE_PREFIX) $(INCLUDE_PREFIX_MULTIPROGRAMMED) -o $(INSTALL_PATH)/$@ $(LINKER_PREFIX)

# Elastic work stealing
# ---------------------

INCLUDE_PREFIX_ELASTIC=$(INCLUDE_PREFIX_ALL) -DTASKPARTS_USE_YWRA_DEQUE

### Surplus (using native semaphores)

INCLUDE_PREFIX_ELASTIC_SURPLUS=$(INCLUDE_PREFIX_ELASTIC) -DTASKPARTS_ELASTIC_SURPLUS

%.surplus2.elastic.opt: %.cpp $(INCLUDE_FILES) install_folder
	$(CXX) $(OPT_PREFIX) $(INCLUDE_PREFIX_ELASTIC_SURPLUS) -o $(INSTALL_PATH)/$@ $< $(LINKER_PREFIX)
%.surplus2.elastic.sta: %.cpp $(INCLUDE_FILES) install_folder
	$(CXX) $(STA_PREFIX) $(INCLUDE_PREFIX_ELASTIC_SURPLUS) -o $(INSTALL_PATH)/$@ $< $(LINKER_PREFIX)
%.surplus2.elastic.log: %.cpp $(INCLUDE_FILES) install_folder
	$(CXX) $(LOG_PREFIX) $(INCLUDE_PREFIX_ELASTIC_SURPLUS) -o $(INSTALL_PATH)/$@ $< $(LINKER_PREFIX)
%.surplus2.elastic.dbg: %.cpp $(INCLUDE_FILES) install_folder
	$(CXX) $(DBG_PREFIX) $(INCLUDE_PREFIX_ELASTIC_SURPLUS) -o $(INSTALL_PATH)/$@ $< $(LINKER_PREFIX)

### Surplus (using spinning semaphores)

INCLUDE_PREFIX_ELASTIC_SURPLUS_SPIN=$(INCLUDE_PREFIX_ELASTIC_SURPLUS) -DTASKPARTS_USE_SPINNING_SEMAPHORE

%.spin.surplus2.elastic.opt: %.cpp $(INCLUDE_FILES) install_folder
	$(CXX) $(OPT_PREFIX) $(INCLUDE_PREFIX_ELASTIC_SURPLUS_SPIN) -o $(INSTALL_PATH)/$@ $< $(LINKER_PREFIX)
%.spin.surplus2.elastic.sta: %.cpp $(INCLUDE_FILES) install_folder
	$(CXX) $(STA_PREFIX) $(INCLUDE_PREFIX_ELASTIC_SURPLUS_SPIN) -o $(INSTALL_PATH)/$@ $< $(LINKER_PREFIX)
%.spin.surplus2.elastic.log: %.cpp $(INCLUDE_FILES) install_folder
	$(CXX) $(LOG_PREFIX) $(INCLUDE_PREFIX_ELASTIC_SURPLUS_SPIN) -o $(INSTALL_PATH)/$@ $< $(LINKER_PREFIX)
%.spin.surplus2.elastic.dbg: %.cpp $(INCLUDE_FILES) install_folder
	$(CXX) $(DBG_PREFIX) $(INCLUDE_PREFIX_ELASTIC_SURPLUS_SPIN) -o $(INSTALL_PATH)/$@ $< $(LINKER_PREFIX)

### Tree (using native semaphores)

INCLUDE_PREFIX_ELASTIC_TREE=$(INCLUDE_PREFIX_ELASTIC) -DTASKPARTS_ELASTIC_TREE

%.surplus.elastic.opt: %.cpp $(INCLUDE_FILES) install_folder
	$(CXX) $(OPT_PREFIX) $(INCLUDE_PREFIX_ELASTIC_TREE) -o $(INSTALL_PATH)/$@ $< $(LINKER_PREFIX)
%.surplus.elastic.sta: %.cpp $(INCLUDE_FILES) install_folder
	$(CXX) $(STA_PREFIX) $(INCLUDE_PREFIX_ELASTIC_TREE) -o $(INSTALL_PATH)/$@ $< $(LINKER_PREFIX)
%.surplus.elastic.log: %.cpp $(INCLUDE_FILES) install_folder
	$(CXX) $(LOG_PREFIX) $(INCLUDE_PREFIX_ELASTIC_TREE) -o $(INSTALL_PATH)/$@ $< $(LINKER_PREFIX)
%.surplus.elastic.dbg: %.cpp $(INCLUDE_FILES) install_folder
	$(CXX) $(DBG_PREFIX) $(INCLUDE_PREFIX_ELASTIC_TREE) -o $(INSTALL_PATH)/$@ $< $(LINKER_PREFIX)

### Tree (using spinning semaphores)

INCLUDE_PREFIX_ELASTIC_TREE_SPIN=$(INCLUDE_PREFIX_ELASTIC_TREE) -DTASKPARTS_USE_SPINNING_SEMAPHORE

%.spin.surplus.elastic.opt: %.cpp $(INCLUDE_FILES) install_folder
	$(CXX) $(OPT_PREFIX) $(INCLUDE_PREFIX_ELASTIC_TREE_SPIN) -o $(INSTALL_PATH)/$@ $< $(LINKER_PREFIX)
%.spin.surplus.elastic.sta: %.cpp $(INCLUDE_FILES) install_folder
	$(CXX) $(STA_PREFIX) $(INCLUDE_PREFIX_ELASTIC_TREE_SPIN) -o $(INSTALL_PATH)/$@ $< $(LINKER_PREFIX)
%.spin.surplus.elastic.log: %.cpp $(INCLUDE_FILES) install_folder
	$(CXX) $(LOG_PREFIX) $(INCLUDE_PREFIX_ELASTIC_TREE_SPIN) -o $(INSTALL_PATH)/$@ $< $(LINKER_PREFIX)
%.spin.surplus.elastic.dbg: %.cpp $(INCLUDE_FILES) install_folder
	$(CXX) $(DBG_PREFIX) $(INCLUDE_PREFIX_ELASTIC_TREE_SPIN) -o $(INSTALL_PATH)/$@ $< $(LINKER_PREFIX)

# Cilk Plus
# ---------

CILK_COMMON_PREFIX=$(INCLUDE_PREFIX_ALL) -fcilkplus -lcilkrts -DCILK -ldl -DPARLAY_CILKPLUS

%.cilk.opt: %.cilk.cpp $(INCLUDE_FILES) install_folder
	$(CXX) $(OPT_COMPILE_PREFIX) $(CILK_COMMON_PREFIX) -o $(INSTALL_PATH)/$@ $< $(LINKER_PREFIX)
%.cilk.sta: %.cilk.cpp $(INCLUDE_FILES) install_folder
	$(CXX) $(OPT_COMPILE_PREFIX) -o $(INSTALL_PATH)/$@ $< $(CILKRTS_STATS_PREFIX) $(CILK_COMMON_PREFIX) $(LINKER_PREFIX)
%.cilk.dbg: %.cilk.cpp $(INCLUDE_FILES) install_folder
	$(CXX) $(DBG_PREFIX) $(CILK_COMMON_PREFIX) -o $(INSTALL_PATH)/$@ $< $(LINKER_PREFIX)

# Cleanup
# =======

clean:
	rm -rf bin

clean_all: clean
	rm -f log*.json
	rm -f vgcore*
