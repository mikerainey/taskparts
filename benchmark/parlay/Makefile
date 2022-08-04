
# Taskparts parameters
# ====================

include ../Makefile_benchmark
TASKPARTS_PATH=../../

PARLAYLIB=-I$(PARLAYLIB_PATH)/include/ -DPARLAY_TASKPARTS
PARLAYLIB_EXAMPLES=-I$(PARLAYLIB_PATH)/share/examples/
INCLUDE_PREFIX := $(INCLUDE_PREFIX) $(PARLAYLIB) $(PARLAYLIB_EXAMPLES)

# Binaries
# ========

%.opt: %.cpp $(INCLUDE_FILES) install_folder
	$(CXX) $(OPT_PREFIX) $<  $(INCLUDE_PREFIX) -o $(INSTALL_PATH)/$@ $(LINKER_PREFIX)
%.sta: %.cpp $(INCLUDE_FILES) install_folder
	$(CXX) $(STA_PREFIX) $< $(INCLUDE_PREFIX) -o $(INSTALL_PATH)/$@ $(LINKER_PREFIX)
%.log: %.cpp $(INCLUDE_FILES) install_folder
	$(CXX) $(LOG_PREFIX) $< $(INCLUDE_PREFIX) -o $(INSTALL_PATH)/$@ $(LINKER_PREFIX)
%.dbg: %.cpp $(INCLUDE_FILES) install_folder
	$(CXX) $(DBG_PREFIX) $< $(INCLUDE_PREFIX) -o $(INSTALL_PATH)/$@ $(LINKER_PREFIX)

# Elastic work stealing
# =====================

# Surplus (using native semaphores)
# -------

%.elastic_surplus.opt: %.cpp $(INCLUDE_FILES) install_folder
	$(CXX) $(OPT_PREFIX) -DTASKPARTS_ELASTIC_SURPLUS -o $(INSTALL_PATH)/$@ $< $(LINKER_PREFIX)
%.elastic_surplus.sta: %.cpp $(INCLUDE_FILES) install_folder
	$(CXX) $(STA_PREFIX) -DTASKPARTS_ELASTIC_SURPLUS -o $(INSTALL_PATH)/$@ $< $(LINKER_PREFIX)
%.elastic_surplus.log: %.cpp $(INCLUDE_FILES) install_folder
	$(CXX) $(LOG_PREFIX) -DTASKPARTS_ELASTIC_SURPLUS -o $(INSTALL_PATH)/$@ $< $(LINKER_PREFIX)
%.elastic_surplus.dbg: %.cpp $(INCLUDE_FILES) install_folder
	$(CXX) $(DBG_PREFIX) -DTASKPARTS_ELASTIC_SURPLUS -o $(INSTALL_PATH)/$@ $< $(LINKER_PREFIX)

# Surplus (using spinning semaphores)
# -------

SS_SURPLUS_FLGS=-DTASKPARTS_ELASTIC_SURPLUS -DTASKPARTS_USE_SPINNING_SEMAPHORE
%.elastic_surplus_spin.opt: %.cpp $(INCLUDE_FILES) install_folder
	$(CXX) $(OPT_PREFIX) $(SS_SURPLUS_FLGS) -o $(INSTALL_PATH)/$@ $< $(LINKER_PREFIX)
%.elastic_surplus_spin.sta: %.cpp $(INCLUDE_FILES) install_folder
	$(CXX) $(STA_PREFIX) $(SS_SURPLUS_FLGS) -o $(INSTALL_PATH)/$@ $< $(LINKER_PREFIX)
%.elastic_surplus_spin.log: %.cpp $(INCLUDE_FILES) install_folder
	$(CXX) $(LOG_PREFIX) $(SS_SURPLUS_FLGS) -o $(INSTALL_PATH)/$@ $< $(LINKER_PREFIX)
%.elastic_surplus_spin.dbg: %.cpp $(INCLUDE_FILES) install_folder
	$(CXX) $(DBG_PREFIX) $(SS_SURPLUS_FLGS) -o $(INSTALL_PATH)/$@ $< $(LINKER_PREFIX)

clean:
	rm -rf bin
