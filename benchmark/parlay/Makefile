
# Taskparts parameters
# ====================

include ../Makefile_benchmark
TASKPARTS_PATH=../../

PARLAYLIB?=-I$(PARLAYLIB_PATH)/include/ -DPARLAY_TASKPARTS
PARLAYLIB_EXAMPLES?=-I$(PARLAYLIB_PATH)/share/examples/
# -DPARLAY_USE_STD_ALLOC disables the pool allocator in parlaylib
PARLAYLIB_EXTRAS?=
PARLAYLIB_PREFIX=$(PARLAYLIB) $(PARLAYLIB_EXAMPLES) $(PARLAYLIB_EXTRAS)

TASKPARTS_WORKSTEALING_DEQUE?=

# Binaries
# ========

INCLUDE_PREFIX_ALL=$(INCLUDE_PREFIX) $(PARLAYLIB_PREFIX)

INCLUDE_PREFIX_NONELASTIC=$(INCLUDE_PREFIX_ALL) $(TASKPARTS_WORKSTEALING_DEQUE)

# Default
# -------

%.opt: %.cpp $(INCLUDE_FILES) install_folder
	$(CXX) $(OPT_PREFIX) $<  $(INCLUDE_PREFIX_NONELASTIC) -o $(INSTALL_PATH)/$@ $(LINKER_PREFIX)
%.sta: %.cpp $(INCLUDE_FILES) install_folder
	$(CXX) $(STA_PREFIX) $< $(INCLUDE_PREFIX_NONELASTIC) -o $(INSTALL_PATH)/$@ $(LINKER_PREFIX)
%.log: %.cpp $(INCLUDE_FILES) install_folder
	$(CXX) $(LOG_PREFIX) $< $(INCLUDE_PREFIX_NONELASTIC) -o $(INSTALL_PATH)/$@ $(LINKER_PREFIX)
%.dbg: %.cpp $(INCLUDE_FILES) install_folder
	$(CXX) $(DBG_PREFIX) $< $(INCLUDE_PREFIX_NONELASTIC) -o $(INSTALL_PATH)/$@ $(LINKER_PREFIX)

# Multiprogrammed
# ---------------

INCLUDE_PREFIX_MULTIPROGRAMMED=$(INCLUDE_PREFIX_NONELASTIC) -DTASKPARTS_MULTIPROGRAMMED

%.multiprogrammed.opt: %.cpp $(INCLUDE_FILES) install_folder
	$(CXX) $(OPT_PREFIX) $<  $(INCLUDE_PREFIX_NONELASTIC) $(INCLUDE_PREFIX_MULTIPROGRAMMED) -o $(INSTALL_PATH)/$@ $(LINKER_PREFIX)
%.multiprogrammed.sta: %.cpp $(INCLUDE_FILES) install_folder
	$(CXX) $(STA_PREFIX) $< $(INCLUDE_PREFIX_NONELASTIC) $(INCLUDE_PREFIX_MULTIPROGRAMMED) -o $(INSTALL_PATH)/$@ $(LINKER_PREFIX)
%.multiprogrammed.log: %.cpp $(INCLUDE_FILES) install_folder
	$(CXX) $(LOG_PREFIX) $< $(INCLUDE_PREFIX_NONELASTIC) $(INCLUDE_PREFIX_MULTIPROGRAMMED) -o $(INSTALL_PATH)/$@ $(LINKER_PREFIX)
%.multiprogrammed.dbg: %.cpp $(INCLUDE_FILES) install_folder
	$(CXX) $(DBG_PREFIX) $< $(INCLUDE_PREFIX_NONELASTIC) $(INCLUDE_PREFIX_MULTIPROGRAMMED) -o $(INSTALL_PATH)/$@ $(LINKER_PREFIX)

# Elastic work stealing
# =====================

INCLUDE_PREFIX_ELASTIC=$(INCLUDE_PREFIX_ALL) -DTASKPARTS_USE_YWRA_DEQUE

# Surplus (using native semaphores)
# -------

INCLUDE_PREFIX_ELASTIC_SURPLUS=$(INCLUDE_PREFIX_ELASTIC) -DTASKPARTS_ELASTIC_SURPLUS

%.surplus2.elastic.opt: %.cpp $(INCLUDE_FILES) install_folder
	$(CXX) $(OPT_PREFIX) $(INCLUDE_PREFIX_ELASTIC_SURPLUS) -o $(INSTALL_PATH)/$@ $< $(LINKER_PREFIX)
%.surplus2.elastic.sta: %.cpp $(INCLUDE_FILES) install_folder
	$(CXX) $(STA_PREFIX) $(INCLUDE_PREFIX_ELASTIC_SURPLUS) -o $(INSTALL_PATH)/$@ $< $(LINKER_PREFIX)
%.surplus2.elastic.log: %.cpp $(INCLUDE_FILES) install_folder
	$(CXX) $(LOG_PREFIX) $(INCLUDE_PREFIX_ELASTIC_SURPLUS) -o $(INSTALL_PATH)/$@ $< $(LINKER_PREFIX)
%.surplus2.elastic.dbg: %.cpp $(INCLUDE_FILES) install_folder
	$(CXX) $(DBG_PREFIX) $(INCLUDE_PREFIX_ELASTIC_SURPLUS) -o $(INSTALL_PATH)/$@ $< $(LINKER_PREFIX)

# Surplus (using spinning semaphores)
# -------

INCLUDE_PREFIX_ELASTIC_SURPLUS_SPIN=$(INCLUDE_PREFIX_ELASTIC_SURPLUS) -DTASKPARTS_USE_SPINNING_SEMAPHORE

%.spin.surplus2.elastic.opt: %.cpp $(INCLUDE_FILES) install_folder
	$(CXX) $(OPT_PREFIX) $(INCLUDE_PREFIX_ELASTIC_SURPLUS_SPIN) -o $(INSTALL_PATH)/$@ $< $(LINKER_PREFIX)
%.spin.surplus2.elastic.sta: %.cpp $(INCLUDE_FILES) install_folder
	$(CXX) $(STA_PREFIX) $(INCLUDE_PREFIX_ELASTIC_SURPLUS_SPIN) -o $(INSTALL_PATH)/$@ $< $(LINKER_PREFIX)
%.spin.surplus2.elastic.log: %.cpp $(INCLUDE_FILES) install_folder
	$(CXX) $(LOG_PREFIX) $(INCLUDE_PREFIX_ELASTIC_SURPLUS_SPIN) -o $(INSTALL_PATH)/$@ $< $(LINKER_PREFIX)
%.spin.surplus2.elastic.dbg: %.cpp $(INCLUDE_FILES) install_folder
	$(CXX) $(DBG_PREFIX) $(INCLUDE_PREFIX_ELASTIC_SURPLUS_SPIN) -o $(INSTALL_PATH)/$@ $< $(LINKER_PREFIX)

# Tree (using native semaphores)
# -------

INCLUDE_PREFIX_ELASTIC_TREE=$(INCLUDE_PREFIX_ELASTIC) -DTASKPARTS_ELASTIC_TREE

%.surplus.elastic.opt: %.cpp $(INCLUDE_FILES) install_folder
	$(CXX) $(OPT_PREFIX) $(INCLUDE_PREFIX_ELASTIC_TREE) -o $(INSTALL_PATH)/$@ $< $(LINKER_PREFIX)
%.surplus.elastic.sta: %.cpp $(INCLUDE_FILES) install_folder
	$(CXX) $(STA_PREFIX) $(INCLUDE_PREFIX_ELASTIC_TREE) -o $(INSTALL_PATH)/$@ $< $(LINKER_PREFIX)
%.surplus.elastic.log: %.cpp $(INCLUDE_FILES) install_folder
	$(CXX) $(LOG_PREFIX) $(INCLUDE_PREFIX_ELASTIC_TREE) -o $(INSTALL_PATH)/$@ $< $(LINKER_PREFIX)
%.surplus.elastic.dbg: %.cpp $(INCLUDE_FILES) install_folder
	$(CXX) $(DBG_PREFIX) $(INCLUDE_PREFIX_ELASTIC_TREE) -o $(INSTALL_PATH)/$@ $< $(LINKER_PREFIX)

# Tree (using spinning semaphores)
# -------

INCLUDE_PREFIX_ELASTIC_TREE_SPIN=$(INCLUDE_PREFIX_ELASTIC_TREE) -DTASKPARTS_USE_SPINNING_SEMAPHORE

%.spin.surplus.elastic.opt: %.cpp $(INCLUDE_FILES) install_folder
	$(CXX) $(OPT_PREFIX) $(INCLUDE_PREFIX_ELASTIC_TREE_SPIN) -o $(INSTALL_PATH)/$@ $< $(LINKER_PREFIX)
%.spin.surplus.elastic.sta: %.cpp $(INCLUDE_FILES) install_folder
	$(CXX) $(STA_PREFIX) $(INCLUDE_PREFIX_ELASTIC_TREE_SPIN) -o $(INSTALL_PATH)/$@ $< $(LINKER_PREFIX)
%.spin.surplus.elastic.log: %.cpp $(INCLUDE_FILES) install_folder
	$(CXX) $(LOG_PREFIX) $(INCLUDE_PREFIX_ELASTIC_TREE_SPIN) -o $(INSTALL_PATH)/$@ $< $(LINKER_PREFIX)
%.spin.surplus.elastic.dbg: %.cpp $(INCLUDE_FILES) install_folder
	$(CXX) $(DBG_PREFIX) $(INCLUDE_PREFIX_ELASTIC_TREE_SPIN) -o $(INSTALL_PATH)/$@ $< $(LINKER_PREFIX)

# Benchmarks
# ==========

BENCHMARKS=$(basename $(wildcard *.cpp))

BENCHMARK_STA=$(addsuffix .sta,$(BENCHMARKS))

foo:
	echo $(BENCHMARKS)
	echo $(TASKPARTS_BINARY_EXTS)

# Cleanup
# =======

clean:
	rm -rf bin
