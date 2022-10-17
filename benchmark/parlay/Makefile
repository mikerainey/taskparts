
# Taskparts parameters
# ====================

include ../Makefile_benchmark
TASKPARTS_PATH=../../

PARLAYLIB=-I$(PARLAYLIB_PATH)/include/ -DPARLAY_TASKPARTS
PARLAYLIB_EXAMPLES=-I$(PARLAYLIB_PATH)/share/examples/
INCLUDE_PREFIX := $(INCLUDE_PREFIX) $(PARLAYLIB) $(PARLAYLIB_EXAMPLES)

# Binaries
# ========

# Defaults
# --------

%.opt: %.cpp $(INCLUDE_FILES) install_folder
	$(CXX) $(OPT_PREFIX) $<  $(INCLUDE_PREFIX) -o $(INSTALL_PATH)/$@ $(LINKER_PREFIX)
%.sta: %.cpp $(INCLUDE_FILES) install_folder
	$(CXX) $(STA_PREFIX) $< $(INCLUDE_PREFIX) -o $(INSTALL_PATH)/$@ $(LINKER_PREFIX)
%.log: %.cpp $(INCLUDE_FILES) install_folder
	$(CXX) $(LOG_PREFIX) $< $(INCLUDE_PREFIX) -o $(INSTALL_PATH)/$@ $(LINKER_PREFIX)
%.dbg: %.cpp $(INCLUDE_FILES) install_folder
	$(CXX) $(DBG_PREFIX) $< $(INCLUDE_PREFIX) -o $(INSTALL_PATH)/$@ $(LINKER_PREFIX)

# Multiprogrammed
# ---------------

MULTIPROGRAMMED_FLGS=-DTASKPARTS_MULTIPROGRAMMED

%.multiprogrammed.opt: %.cpp $(INCLUDE_FILES) install_folder
	$(CXX) $(OPT_PREFIX) $<  $(INCLUDE_PREFIX) $(MULTIPROGRAMMED_FLGS) -o $(INSTALL_PATH)/$@ $(LINKER_PREFIX)
%.multiprogrammed.sta: %.cpp $(INCLUDE_FILES) install_folder
	$(CXX) $(STA_PREFIX) $< $(INCLUDE_PREFIX) $(MULTIPROGRAMMED_FLAGS) -o $(INSTALL_PATH)/$@ $(LINKER_PREFIX)
%.multiprogrammed.log: %.cpp $(INCLUDE_FILES) install_folder
	$(CXX) $(LOG_PREFIX) $< $(INCLUDE_PREFIX) $(MULTIPROGRAMMED_FLAGS) -o $(INSTALL_PATH)/$@ $(LINKER_PREFIX)
%.multiprogrammed.dbg: %.cpp $(INCLUDE_FILES) install_folder
	$(CXX) $(DBG_PREFIX) $< $(INCLUDE_PREFIX) $(MULTIPROGRAMMED_FLAGS) -o $(INSTALL_PATH)/$@ $(LINKER_PREFIX)

# Elastic work stealing
# =====================

# Surplus (using native semaphores)
# -------

ELASTIC_SURPLUS_FLAGS=-DTASKPARTS_ELASTIC_SURPLUS

%.surplus.elastic.opt: %.cpp $(INCLUDE_FILES) install_folder
	$(CXX) $(OPT_PREFIX) $(ELASTIC_SURPLUS_FLAGS) -o $(INSTALL_PATH)/$@ $< $(LINKER_PREFIX)
%.surplus.elastic.sta: %.cpp $(INCLUDE_FILES) install_folder
	$(CXX) $(STA_PREFIX) $(ELASTIC_SURPLUS_FLAGS) -o $(INSTALL_PATH)/$@ $< $(LINKER_PREFIX)
%.surplus.elastic.log: %.cpp $(INCLUDE_FILES) install_folder
	$(CXX) $(LOG_PREFIX) $(ELASTIC_SURPLUS_FLAGS) -o $(INSTALL_PATH)/$@ $< $(LINKER_PREFIX)
%.surplus.elastic.dbg: %.cpp $(INCLUDE_FILES) install_folder
	$(CXX) $(DBG_PREFIX) $(ELASTIC_SURPLUS_FLAGS) -o $(INSTALL_PATH)/$@ $< $(LINKER_PREFIX)

# Surplus (using spinning semaphores)
# -------

SS_SURPLUS_FLGS=$(ELASTIC_SURPLUS_FLAGS) -DTASKPARTS_USE_SPINNING_SEMAPHORE

%.spin.surplus.elastic.opt: %.cpp $(INCLUDE_FILES) install_folder
	$(CXX) $(OPT_PREFIX) $(SS_SURPLUS_FLGS) -o $(INSTALL_PATH)/$@ $< $(LINKER_PREFIX)
%.spin.surplus.elastic.sta: %.cpp $(INCLUDE_FILES) install_folder
	$(CXX) $(STA_PREFIX) $(SS_SURPLUS_FLGS) -o $(INSTALL_PATH)/$@ $< $(LINKER_PREFIX)
%.spin.surplus.elastic.log: %.cpp $(INCLUDE_FILES) install_folder
	$(CXX) $(LOG_PREFIX) $(SS_SURPLUS_FLGS) -o $(INSTALL_PATH)/$@ $< $(LINKER_PREFIX)
%.spin.surplus.elastic.dbg: %.cpp $(INCLUDE_FILES) install_folder
	$(CXX) $(DBG_PREFIX) $(SS_SURPLUS_FLGS) -o $(INSTALL_PATH)/$@ $< $(LINKER_PREFIX)

# Tree (using native semaphores)
# -------

ELASTIC_TREE_FLAGS=-DTASKPARTS_ELASTIC_TREE

%.multilevel.surplus.elastic.opt: %.cpp $(INCLUDE_FILES) install_folder
	$(CXX) $(OPT_PREFIX) $(ELASTIC_TREE_FLAGS) -o $(INSTALL_PATH)/$@ $< $(LINKER_PREFIX)
%.multilevel.surplus.elastic.sta: %.cpp $(INCLUDE_FILES) install_folder
	$(CXX) $(STA_PREFIX) $(ELASTIC_TREE_FLAGS) -o $(INSTALL_PATH)/$@ $< $(LINKER_PREFIX)
%.multilevel.surplus.elastic.log: %.cpp $(INCLUDE_FILES) install_folder
	$(CXX) $(LOG_PREFIX) $(ELASTIC_TREE_FLAGS) -o $(INSTALL_PATH)/$@ $< $(LINKER_PREFIX)
%.multilevel.surplus.elastic.dbg: %.cpp $(INCLUDE_FILES) install_folder
	$(CXX) $(DBG_PREFIX) $(ELASTIC_TREE_FLAGS) -o $(INSTALL_PATH)/$@ $< $(LINKER_PREFIX)

# Tree (using spinning semaphores)
# -------

SS_TREE_FLGS=$(ELASTIC_TREE_FLAGS) -DTASKPARTS_USE_SPINNING_SEMAPHORE

%.spin.multilevel.surplus.elastic.opt: %.cpp $(INCLUDE_FILES) install_folder
	$(CXX) $(OPT_PREFIX) $(SS_TREE_FLGS) -o $(INSTALL_PATH)/$@ $< $(LINKER_PREFIX)
%.spin.multilevel.surplus.elastic.sta: %.cpp $(INCLUDE_FILES) install_folder
	$(CXX) $(STA_PREFIX) $(SS_TREE_FLGS) -o $(INSTALL_PATH)/$@ $< $(LINKER_PREFIX)
%.spin.multilevel.surplus.elastic.log: %.cpp $(INCLUDE_FILES) install_folder
	$(CXX) $(LOG_PREFIX) $(SS_TREE_FLGS) -o $(INSTALL_PATH)/$@ $< $(LINKER_PREFIX)
%.spin.multilevel.surplus.elastic.dbg: %.cpp $(INCLUDE_FILES) install_folder
	$(CXX) $(DBG_PREFIX) $(SS_TREE_FLGS) -o $(INSTALL_PATH)/$@ $< $(LINKER_PREFIX)

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
