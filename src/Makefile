TASKPARTS_ROOT_PATH?=../
TASKPARTS_INSTALL_PATH?=.
# binaries go here
TASKPARTS_BINARY_PATH?=bin
TASKPARTS_PLATFORM_FLAGS?=-DTASKPARTS_POSIX=1 -DTASKPARTS_X64=1
#TASKPARTS_PLATFORM_FLAGS?=-DTASKPARTS_DARWIN=1 -DTASKPARTS_ARM64=1 
TASKPARTS_OPTIONAL_FLAGS?=
TASKPARTS_CPP_FLAGS?=-std=c++17 -fno-stack-protector -fno-asynchronous-unwind-tables -I$(TASKPARTS_ROOT_PATH)/include/
TASKPARTS_FLAGS=$(TASKPARTS_CPP_FLAGS) $(TASKPARTS_PLATFORM_FLAGS) $(TASKPARTS_OPTIONAL_FLAGS)
TASKPARTS_OPT_FLAGS=-O3 -DNDEBUG -march=native
TASKPARTS_STA=-DTASKPARTS_STATS=1
TASKPARTS_LOG=-DTASKPARTS_LOGGING=1
TASKPARTS_STA_FLAGS=$(TASKPARTS_STA) $(TASKPARTS_OPT_FLAGS)
TASKPARTS_LOG_FLAGS=$(TASKPARTS_LOG) $(TASKPARTS_STA) $(TASKPARTS_OPT_FLAGS)
TASKPARTS_DBG_FLAGS=$(TASKPARTS_STA) $(TASKPARTS_LOG) $(VALGRIND_CFLAG) -O -g3
TASKPARTS_MSDBG=-DTASKPARTS_META_SCHEDULER_SERIAL_RANDOM=1
TASKPARTS_MSDBG_FLAGS=$(TASKPARTS_MSDBG) $(TASKPARTS_DBG_FLAGS)
TASKPARTS_LINKER_FLAGS?=

# Shared library
# ==============

libtaskparts.sta.o: taskparts.cpp install_folder
	$(CXX) $(TASKPARTS_FLAGS) $(TASKPARTS_STA_FLAGS) taskparts.cpp -c -o $(TASKPARTS_BINARY_PATH)/$@
libtaskparts.log.o: taskparts.cpp install_folder
	$(CXX) $(TASKPARTS_FLAGS) $(TASKPARTS_LOG_FLAGS) taskparts.cpp -c -o $(TASKPARTS_BINARY_PATH)/$@
libtaskparts.dbg.o: taskparts.cpp install_folder
	$(CXX) $(TASKPARTS_FLAGS) $(TASKPARTS_DBG_FLAGS) taskparts.cpp -c -o $(TASKPARTS_BINARY_PATH)/$@
libtaskparts.msdbg.o: taskparts.cpp install_folder
	$(CXX) $(TASKPARTS_FLAGS) $(TASKPARTS_MSDBG_FLAGS) taskparts.cpp -c -o $(TASKPARTS_BINARY_PATH)/$@

libtaskparts.sta.so: libtaskparts.sta.o
	$(CC) -shared -o $(TASKPARTS_BINARY_PATH)/libtaskparts.sta.so $(TASKPARTS_BINARY_PATH)/libtaskparts.sta.o
libtaskparts.log.so: libtaskparts.log.o
	$(CC) -shared -o $(TASKPARTS_BINARY_PATH)/libtaskparts.log.so $(TASKPARTS_BINARY_PATH)/libtaskparts.log.o
libtaskparts.dbg.so: libtaskparts.dbg.o
	$(CC) -shared -o $(TASKPARTS_BINARY_PATH)/libtaskparts.dbg.so $(TASKPARTS_BINARY_PATH)/libtaskparts.dbg.o
libtaskparts.msdbg.so: libtaskparts.msdbg.o
	$(CC) -shared -o $(TASKPARTS_BINARY_PATH)/libtaskparts.msdbg.so $(TASKPARTS_BINARY_PATH)/libtaskparts.msdbg.o

libtaskparts.sta.a: libtaskparts.sta.o
	ar rs $(TASKPARTS_BINARY_PATH)/libtaskparts.sta.a $(TASKPARTS_BINARY_PATH)/libtaskparts.sta.o
libtaskparts.log.a: libtaskparts.log.o
	ar rs  $(TASKPARTS_BINARY_PATH)/libtaskparts.log.a $(TASKPARTS_BINARY_PATH)/libtaskparts.log.o
libtaskparts.dbg.a: libtaskparts.dbg.o
	ar rs $(TASKPARTS_BINARY_PATH)/libtaskparts.dbg.a $(TASKPARTS_BINARY_PATH)/libtaskparts.dbg.o
libtaskparts.msdbg.a: libtaskparts.msdbg.o
	ar rs $(TASKPARTS_BINARY_PATH)/libtaskparts.msdbg.a $(TASKPARTS_BINARY_PATH)/libtaskparts.msdbg.o

libraries: libtaskparts.sta.so libtaskparts.log.so libtaskparts.dbg.so libtaskparts.msdbg.so libtaskparts.sta.a libtaskparts.log.a libtaskparts.dbg.a libtaskparts.msdbg.a

# Local standalone binaries
# =========================

%.sta.o: %.cpp libtaskparts.sta.so install_folder
	$(CXX) $(TASKPARTS_FLAGS) $(TASKPARTS_STA_FLAGS) $< -c -o $(TASKPARTS_BINARY_PATH)/$@
%.sta: %.cpp %.sta.o libtaskparts.sta.so install_folder
	$(CXX) $(TASKPARTS_FLAGS) $(TASKPARTS_STA_FLAGS) $(TASKPARTS_BINARY_PATH)/$(basename $<).sta.o -o $(TASKPARTS_BINARY_PATH)/$@ -L$(TASKPARTS_BINARY_PATH) -ltaskparts.sta $(TASKPARTS_LINKER_FLAGS)
%.log.o: %.cpp libtaskparts.log.so install_folder
	$(CXX) $(TASKPARTS_FLAGS) $(TASKPARTS_LOG_FLAGS) $< -c -o $(TASKPARTS_BINARY_PATH)/$@
%.log: %.cpp %.log.o libtaskparts.log.so install_folder
	$(CXX) $(TASKPARTS_FLAGS) $(TASKPARTS_LOG_FLAGS) $(TASKPARTS_BINARY_PATH)/$(basename $<).log.o -o $(TASKPARTS_BINARY_PATH)/$@ -ltaskparts.log -L$(TASKPARTS_BINARY_PATH) $(TASKPARTS_LINKER_FLAGS)
%.dbg.o: %.cpp libtaskparts.dbg.so install_folder
	$(CXX) $(TASKPARTS_FLAGS) $(TASKPARTS_DBG_FLAGS) $< -c -o $(TASKPARTS_BINARY_PATH)/$@
%.dbg: %.cpp %.dbg.o libtaskparts.dbg.so install_folder
	$(CXX) $(TASKPARTS_FLAGS) $(TASKPARTS_DBG_FLAGS) $(TASKPARTS_BINARY_PATH)/$(basename $<).dbg.o -o $(TASKPARTS_BINARY_PATH)/$@ -ltaskparts.dbg -L$(TASKPARTS_BINARY_PATH) $(TASKPARTS_LINKER_FLAGS)
%.msdbg.o: %.cpp libtaskparts.msdbg.so install_folder
	$(CXX) $(TASKPARTS_FLAGS) $(TASKPARTS_MSDBG_FLAGS) $< -c -o $(TASKPARTS_BINARY_PATH)/$@
%.msdbg: %.cpp %.msdbg.o libtaskparts.msdbg.so install_folder
	$(CXX) $(TASKPARTS_FLAGS) $(TASKPARTS_MSDBG_FLAGS) $(TASKPARTS_BINARY_PATH)/$(basename $<).msdbg.o -o $(TASKPARTS_BINARY_PATH)/$@ -ltaskparts.msdbg -L$(TASKPARTS_BINARY_PATH) $(TASKPARTS_LINKER_FLAGS)

# Install
# =======

install:
	mkdir -p $(TASKPARTS_INSTALL_PATH)/lib
	cp bin/*.so $(TASKPARTS_INSTALL_PATH)/lib
	ln -s $(TASKPARTS_INSTALL_PATH)/lib/libtaskparts.sta.so $(TASKPARTS_INSTALL_PATH)/lib/libtaskparts.so
	cp bin/*.a $(TASKPARTS_INSTALL_PATH)/lib
	ln -s $(TASKPARTS_INSTALL_PATH)/lib/libtaskparts.sta.a $(TASKPARTS_INSTALL_PATH)/lib/libtaskparts.a
	cp -r ../include $(TASKPARTS_INSTALL_PATH)

.PHONY: install_folder
install_folder:
	mkdir -p $(TASKPARTS_BINARY_PATH)

clean:
	rm -rf bin
