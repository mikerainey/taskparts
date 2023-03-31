# Taskparts parameters
# ====================

include ../Makefile_benchmark
CXX=clang++
CC=clang
TASKPARTS_PATH=../../
ROLLFORWARD_PATH?=../$(TASKPARTS_PATH)/rollforward/src/
TPALXX?=ROLLFORWARD_PATH=$(ROLLFORWARD_PATH) CXX=$(CXX) ./tpal++
HBTIMER_KMOD_INCLUDE_PREFIX?=
HBTIMER_KMOD_LINKER_FLAGS?=
TPAL_LINKER_PREFIX?=$(HBTIMER_KMOD_LINKER_FLAGS)
MATRIX_MARKET_PATH?=../$(TASKPARTS_PATH)/matrix-market/

# TPAL build flags
# ----------------

TPAL_INCLUDE_PREFIX?=-I $(ROLLFORWARD_PATH)/x64 -DTASKPARTS_TPALRTS $(HBTIMER_KMOD_INCLUDE_PREFIX) -I. -I$(MATRIX_MARKET_PATH)/include/mm
LINKER_PREFIX := $(TPAL_LINKER_PREFIX) $(LINKER_PREFIX)
TPALRTS_STA_PREFIX=$(STA_PREFIX) -fno-rtti -fno-pic -fno-pie -DTASKPARTS_USE_CHASELEV_DEQUE
TPALRTS_DBG_PREFIX=$(DBG_PREFIX) -fno-rtti -fno-pic -fno-pie

# Optimized compilation

%.heartbeat.s: %.heartbeat.cpp $(INCLUDE_FILES) install_folder
	$(CXX) $(TPALRTS_STA_PREFIX) -fno-verbose-asm -I$(ROLLFORWARD_PATH)/x64 $< -S -o $(INSTALL_PATH)/$@

%.heartbeat.rf.s: %.heartbeat.s $(INCLUDE_FILES) install_folder
	ROLLFORWARD_PATH=$(ROLLFORWARD_PATH) ./rollforward++ bin/$(basename $<).s bin/$(basename $<).rf.s

%.heartbeat.rf.o: %.heartbeat.rf.s $(INCLUDE_FILES) install_folder
	$(CC) -Wno-unused-command-line-argument -I$(ROLLFORWARD_PATH)/x64 bin/$< -c -o $(INSTALL_PATH)/$@

%.o: %.cpp %.heartbeat.rf.o $(INCLUDE_FILES) install_folder
	$(CXX) $(TPALRTS_STA_PREFIX) bin/$(basename $<).heartbeat.rf.o $< $(TPAL_INCLUDE_PREFIX) -o $(INSTALL_PATH)/$@ $(LINKER_PREFIX)

# Debugging compilation

%.dbg.heartbeat.s: %.heartbeat.cpp $(INCLUDE_FILES) install_folder
	$(CXX) $(TPALRTS_DBG_PREFIX) -fno-verbose-asm -I$(ROLLFORWARD_PATH)/x64 $< -S -o $(INSTALL_PATH)/$@

%.dbg.heartbeat.rf.s: %.dbg.heartbeat.s $(INCLUDE_FILES) install_folder
	ROLLFORWARD_PATH=$(ROLLFORWARD_PATH) ./rollforward++ bin/$(basename $<).s bin/$(basename $<).rf.s

%.dbg.heartbeat.rf.o: %.dbg.heartbeat.rf.s $(INCLUDE_FILES) install_folder
	$(CC) -Wno-unused-command-line-argument -I$(ROLLFORWARD_PATH)/x64 bin/$< -c -o $(INSTALL_PATH)/$@

%.dbg.o: %.cpp %.dbg.heartbeat.rf.o $(INCLUDE_FILES) install_folder
	$(CXX) $(TPALRTS_DBG_PREFIX) bin/$(basename $<).heartbeat.rf.o $< $(TPAL_INCLUDE_PREFIX) -o $(INSTALL_PATH)/$@ $(LINKER_PREFIX)


$(INSTALL_PATH)/rollforward.o: $(ROLLFORWARD_PATH)/x64/rollforward.c
	$(CXX) -fno-rtti -fno-pic -fno-pie $(ROLLFORWARD_PATH)/x64/rollforward.c -c -o $(INSTALL_PATH)/rollforward.o

# Binaries
# ========

%.sta: %.cpp %.heartbeat.rf.o $(INSTALL_PATH)/rollforward.o $(INCLUDE_FILES) install_folder
	$(CXX) $(TPALRTS_DBG_PREFIX) $(INSTALL_PATH)/rollforward.o bin/$(basename $<).heartbeat.rf.o $< $(TPAL_INCLUDE_PREFIX) -o $(INSTALL_PATH)/$@ $(LINKER_PREFIX)

%.dbg: %.cpp %.dbg.heartbeat.rf.o $(INSTALL_PATH)/rollforward.o $(INCLUDE_FILES) install_folder
	$(CXX) $(TPALRTS_DBG_PREFIX) $(INSTALL_PATH)/rollforward.o bin/$(basename $<).dbg.heartbeat.rf.o $< $(TPAL_INCLUDE_PREFIX) -o $(INSTALL_PATH)/$@ $(LINKER_PREFIX)

clean:
	rm -rf bin
