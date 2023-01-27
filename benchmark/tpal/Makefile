# Taskparts parameters
# ====================

include ../Makefile_benchmark
CXX?=g++
TASKPARTS_PATH=../../
ROLLFORWARD_PATH?=../$(TASKPARTS_PATH)/rollforward/src/
TPALXX?=ROLLFORWARD_PATH=$(ROLLFORWARD_PATH) CXX=$(CXX) ./tpal++

# TPAL build flags
# ----------------

TPAL_INCLUDE_PREFIX?=-I $(ROLLFORWARD_PATH)/x64 -DTASKPARTS_TPALRTS
LINKER_PREFIX := $(TPAL_LINKER_PREFIX) $(LINKER_PREFIX)

%.heartbeat.s: %.heartbeat.cpp $(INCLUDE_FILES) install_folder
	$(CXX) $(OPT_PREFIX) -I$(ROLLFORWARD_PATH)/x64 $< -S -o $(INSTALL_PATH)/$@

%.heartbeat.rf.s: %.heartbeat.s $(INCLUDE_FILES) install_folder
	./rollforward++ bin/$(basename $<).s bin/$(basename $<).rf.s

%.heartbeat.rf.o: %.heartbeat.rf.s $(INCLUDE_FILES) install_folder
	$(CXX) -I$(ROLLFORWARD_PATH)/x64 bin/$< -c -o $(INSTALL_PATH)/$@

%.o: %.cpp %.heartbeat.rf.o $(INCLUDE_FILES) install_folder
	$(CXX) $(STA_PREFIX) bin/$(basename $<).heartbeat.rf.o $< $(TPAL_INCLUDE_PREFIX) -o $(INSTALL_PATH)/$@ $(LINKER_PREFIX)

# Binaries
# ========

%.opt: %.cpp $(INCLUDE_FILES) install_folder
	$(TPALXX) $(CXX) $(OPT_PREFIX) $<  $(TPAL_INCLUDE_PREFIX) -o $(INSTALL_PATH)/$@ $(LINKER_PREFIX)
%.sta: %.cpp %.heartbeat.rf.o $(INCLUDE_FILES) install_folder
	$(CXX) $(STA_PREFIX) $(ROLLFORWARD_PATH)/x64/rollforward.c bin/$(basename $<).heartbeat.rf.o $< $(TPAL_INCLUDE_PREFIX) -o $(INSTALL_PATH)/$@ $(LINKER_PREFIX)
%.log: %.cpp $(INCLUDE_FILES) install_folder
	$(TPALXX) $(CXX) $(LOG_PREFIX) $< $(TPAL_INCLUDE_PREFIX) -o $(INSTALL_PATH)/$@ $(LINKER_PREFIX)
%.dbg: %.cpp $(INCLUDE_FILES) install_folder
	$(TPALXX) $(CXX) $(DBG_PREFIX) $< $(TPAL_INCLUDE_PREFIX) -o $(INSTALL_PATH)/$@ $(LINKER_PREFIX)

clean:
	rm -rf bin
