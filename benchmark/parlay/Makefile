
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

%.elastic_surplus.opt: %.cpp $(INCLUDE_FILES) install_folder
	$(CXX) $(OPT_PREFIX) $(ELASTIC_SURPLUS_FLAGS) -o $(INSTALL_PATH)/$@ $< $(LINKER_PREFIX)
%.elastic_surplus.sta: %.cpp $(INCLUDE_FILES) install_folder
	$(CXX) $(STA_PREFIX) $(ELASTIC_SURPLUS_FLAGS) -o $(INSTALL_PATH)/$@ $< $(LINKER_PREFIX)
%.elastic_surplus.log: %.cpp $(INCLUDE_FILES) install_folder
	$(CXX) $(LOG_PREFIX) $(ELASTIC_SURPLUS_FLAGS) -o $(INSTALL_PATH)/$@ $< $(LINKER_PREFIX)
%.elastic_surplus.dbg: %.cpp $(INCLUDE_FILES) install_folder
	$(CXX) $(DBG_PREFIX) $(ELASTIC_SURPLUS_FLAGS) -o $(INSTALL_PATH)/$@ $< $(LINKER_PREFIX)

# Surplus (using spinning semaphores)
# -------

SS_SURPLUS_FLGS=$(ELASTIC_SURPLUS_FLAGS) -DTASKPARTS_USE_SPINNING_SEMAPHORE

%.elastic_surplus_spin.opt: %.cpp $(INCLUDE_FILES) install_folder
	$(CXX) $(OPT_PREFIX) $(SS_SURPLUS_FLGS) -o $(INSTALL_PATH)/$@ $< $(LINKER_PREFIX)
%.elastic_surplus_spin.sta: %.cpp $(INCLUDE_FILES) install_folder
	$(CXX) $(STA_PREFIX) $(SS_SURPLUS_FLGS) -o $(INSTALL_PATH)/$@ $< $(LINKER_PREFIX)
%.elastic_surplus_spin.log: %.cpp $(INCLUDE_FILES) install_folder
	$(CXX) $(LOG_PREFIX) $(SS_SURPLUS_FLGS) -o $(INSTALL_PATH)/$@ $< $(LINKER_PREFIX)
%.elastic_surplus_spin.dbg: %.cpp $(INCLUDE_FILES) install_folder
	$(CXX) $(DBG_PREFIX) $(SS_SURPLUS_FLGS) -o $(INSTALL_PATH)/$@ $< $(LINKER_PREFIX)

# Tree (using native semaphores)
# -------

ELASTIC_TREE_FLAGS=-DTASKPARTS_ELASTIC_TREE

%.elastic_tree.opt: %.cpp $(INCLUDE_FILES) install_folder
	$(CXX) $(OPT_PREFIX) $(ELASTIC_TREE_FLAGS) -o $(INSTALL_PATH)/$@ $< $(LINKER_PREFIX)
%.elastic_tree.sta: %.cpp $(INCLUDE_FILES) install_folder
	$(CXX) $(STA_PREFIX) $(ELASTIC_TREE_FLAGS) -o $(INSTALL_PATH)/$@ $< $(LINKER_PREFIX)
%.elastic_tree.log: %.cpp $(INCLUDE_FILES) install_folder
	$(CXX) $(LOG_PREFIX) $(ELASTIC_TREE_FLAGS) -o $(INSTALL_PATH)/$@ $< $(LINKER_PREFIX)
%.elastic_tree.dbg: %.cpp $(INCLUDE_FILES) install_folder
	$(CXX) $(DBG_PREFIX) $(ELASTIC_TREE_FLAGS) -o $(INSTALL_PATH)/$@ $< $(LINKER_PREFIX)

# Tree (using spinning semaphores)
# -------

SS_TREE_FLGS=$(ELASTIC_TREE_FLAGS) -DTASKPARTS_USE_SPINNING_SEMAPHORE

%.elastic_tree_spin.opt: %.cpp $(INCLUDE_FILES) install_folder
	$(CXX) $(OPT_PREFIX) $(SS_TREE_FLGS) -o $(INSTALL_PATH)/$@ $< $(LINKER_PREFIX)
%.elastic_tree_spin.sta: %.cpp $(INCLUDE_FILES) install_folder
	$(CXX) $(STA_PREFIX) $(SS_TREE_FLGS) -o $(INSTALL_PATH)/$@ $< $(LINKER_PREFIX)
%.elastic_tree_spin.log: %.cpp $(INCLUDE_FILES) install_folder
	$(CXX) $(LOG_PREFIX) $(SS_TREE_FLGS) -o $(INSTALL_PATH)/$@ $< $(LINKER_PREFIX)
%.elastic_tree_spin.dbg: %.cpp $(INCLUDE_FILES) install_folder
	$(CXX) $(DBG_PREFIX) $(SS_TREE_FLGS) -o $(INSTALL_PATH)/$@ $< $(LINKER_PREFIX)

# Cleanup
# =======

clean:
	rm -rf bin
