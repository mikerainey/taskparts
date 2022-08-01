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

# Surplus
%.elastic_surplus.opt: %.cpp $(INCLUDE_FILES) install_folder
	$(CXX) $(OPT_PREFIX) -DTASKPARTS_ELASTIC_SURPLUS -o $(INSTALL_PATH)/$@ $< $(LINKER_PREFIX)
%.elastic_surplus.sta: %.cpp $(INCLUDE_FILES) install_folder
	$(CXX) $(STA_PREFIX) -DTASKPARTS_ELASTIC_SURPLUS -o $(INSTALL_PATH)/$@ $< $(LINKER_PREFIX)
%.elastic_surplus.log: %.cpp $(INCLUDE_FILES) install_folder
	$(CXX) $(LOG_PREFIX) -DTASKPARTS_ELASTIC_SURPLUS -o $(INSTALL_PATH)/$@ $< $(LINKER_PREFIX)
%.elastic_surplus.dbg: %.cpp $(INCLUDE_FILES) install_folder
	$(CXX) $(DBG_PREFIX) -DTASKPARTS_ELASTIC_SURPLUS -o $(INSTALL_PATH)/$@ $< $(LINKER_PREFIX)

clean:
	rm -rf bin
