# Taskparts parameters
# ====================

include ../Makefile_benchmark
TASKPARTS_PATH=../../
TPALXX?=./tpal++

# Heartbeat kernel module
# -----------------------

HBTIMER_KMOD_INCLUDE_PREFIX?=
HBTIMER_KMOD_LINKER_FLAGS?=
LINKER_PREFIX := $(HBTIMER_KMOD_LINKER_FLAGS) $(LINKER_PREFIX)

# TPAL build flags
# ----------------

TPAL_INCLUDE_PREFIX?=$(HBTIMER_KMOD_INCLUDE_PREFIX) -DTASKPARTS_TPALRTS
TPAL_LINKER_PREFIX?=$(HBTIMER_KMOD_LINKER_FLAGS)
LINKER_PREFIX := $(TPAL_LINKER_PREFIX) $(LINKER_PREFIX)

# Binaries
# ========

# later, to use kernel module use the flag:
# -DTASKPARTS_TPALRTS_HBTIMER_KMOD_HEARTBEAT

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
