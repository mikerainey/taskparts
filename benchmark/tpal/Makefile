# Taskparts parameters
# ====================

include ../Makefile_benchmark
CXX?=g++
CC?=gcc
TASKPARTS_PATH=../../
ROLLFORWARD_PATH?=../$(TASKPARTS_PATH)/rollforward/src/
TPALXX?=ROLLFORWARD_PATH=$(ROLLFORWARD_PATH) CXX=$(CXX) ./tpal++
HBTIMER_KMOD_INCLUDE_PREFIX?=
HBTIMER_KMOD_LINKER_FLAGS?=
TPAL_LINKER_PREFIX?=$(HBTIMER_KMOD_LINKER_FLAGS)

# TPAL build flags
# ----------------

TPAL_INCLUDE_PREFIX?=-I $(ROLLFORWARD_PATH)/x64 -DTASKPARTS_TPALRTS $(HBTIMER_KMOD_INCLUDE_PREFIX)
LINKER_PREFIX := $(TPAL_LINKER_PREFIX) $(LINKER_PREFIX)
TPALRTS_STA_PREFIX=$(STA_PREFIX) -fno-rtti -fno-pic -fno-pie

%.heartbeat.s: %.heartbeat.cpp $(INCLUDE_FILES) install_folder
	$(CXX) $(TPALRTS_STA_PREFIX) -fno-verbose-asm -I$(ROLLFORWARD_PATH)/x64 $< -S -o $(INSTALL_PATH)/$@

%.heartbeat.rf.s: %.heartbeat.s $(INCLUDE_FILES) install_folder
	./rollforward++ bin/$(basename $<).s bin/$(basename $<).rf.s

%.heartbeat.rf.o: %.heartbeat.rf.s $(INCLUDE_FILES) install_folder
	$(CC) -Wno-unused-command-line-argument -I$(ROLLFORWARD_PATH)/x64 bin/$< -c -o $(INSTALL_PATH)/$@

$(INSTALL_PATH)/rollforward.o: $(ROLLFORWARD_PATH)/x64/rollforward.c
	$(CXX) -fno-rtti -fno-pic -fno-pie $(ROLLFORWARD_PATH)/x64/rollforward.c -c -o $(INSTALL_PATH)/rollforward.o

%.o: %.cpp %.heartbeat.rf.o $(INCLUDE_FILES) install_folder
	$(CXX) $(TPALRTS_STA_PREFIX) bin/$(basename $<).heartbeat.rf.o $< $(TPAL_INCLUDE_PREFIX) -o $(INSTALL_PATH)/$@ $(LINKER_PREFIX)

# Binaries
# ========

%.sta: %.cpp %.heartbeat.rf.o $(INSTALL_PATH)/rollforward.o $(INCLUDE_FILES) install_folder
	$(CXX) $(TPALRTS_STA_PREFIX) $(INSTALL_PATH)/rollforward.o bin/$(basename $<).heartbeat.rf.o $< $(TPAL_INCLUDE_PREFIX) -o $(INSTALL_PATH)/$@ $(LINKER_PREFIX)

###################################################################################################################
# %.opt: %.cpp $(INCLUDE_FILES) install_folder									  #
# 	$(TPALXX) $(CXX) $(OPT_PREFIX) $<  $(TPAL_INCLUDE_PREFIX) -o $(INSTALL_PATH)/$@ $(LINKER_PREFIX)	  #
# %.log: %.cpp $(INCLUDE_FILES) install_folder									  #
# 	$(TPALXX) $(CXX) $(LOG_PREFIX) $< $(TPAL_INCLUDE_PREFIX) -o $(INSTALL_PATH)/$@ $(LINKER_PREFIX)		  #
# %.dbg: %.cpp $(INCLUDE_FILES) install_folder									  #
# 	$(TPALXX) $(CXX) $(DBG_PREFIX) $< $(TPAL_INCLUDE_PREFIX) -o $(INSTALL_PATH)/$@ $(LINKER_PREFIX)		  #
###################################################################################################################

clean:
	rm -rf bin
