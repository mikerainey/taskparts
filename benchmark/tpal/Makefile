# Taskparts parameters
# ====================

include ../Makefile_benchmark
TASKPARTS_PATH=../../
TPALXX?=./tpal++
ROLLFORWARD_PATH?=../$(TASKPARTS_PATH)/rollforward/src/

# TPAL build flags
# ----------------

TPAL_INCLUDE_PREFIX?=-I $(ROLLFORWARD_PATH)/x64 -DTASKPARTS_TPALRTS
LINKER_PREFIX := $(TPAL_LINKER_PREFIX) $(LINKER_PREFIX)

# Binaries
# ========

%.opt: %.cpp $(INCLUDE_FILES) install_folder
	$(TPALXX) $(CXX) $(OPT_PREFIX) $<  $(TPAL_INCLUDE_PREFIX) -o $(INSTALL_PATH)/$@ $(LINKER_PREFIX)
%.sta: %.cpp $(INCLUDE_FILES) install_folder
	$(TPALXX) $(CXX) $(STA_PREFIX) $< $(TPAL_INCLUDE_PREFIX) -o $(INSTALL_PATH)/$@ $(LINKER_PREFIX)
%.log: %.cpp $(INCLUDE_FILES) install_folder
	$(TPALXX) $(CXX) $(LOG_PREFIX) $< $(TPAL_INCLUDE_PREFIX) -o $(INSTALL_PATH)/$@ $(LINKER_PREFIX)
%.dbg: %.cpp $(INCLUDE_FILES) install_folder
	$(TPALXX) $(CXX) $(DBG_PREFIX) $< $(TPAL_INCLUDE_PREFIX) -o $(INSTALL_PATH)/$@ $(LINKER_PREFIX)

clean:
	rm -rf bin
