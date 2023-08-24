TASKPARTS_ROOT_PATH?=../
TASKPARTS_INSTALL_PATH?=.
TASKPARTS_SRC_PATH=$(TASKPARTS_ROOT_PATH)/src/
# binaries go here
TASKPARTS_BINARY_PATH?=bin
TASKPARTS_PLATFORM_FLAGS?=-DTASKPARTS_POSIX=1 -DTASKPARTS_X64=1
#TASKPARTS_PLATFORM_FLAGS?=-DTASKPARTS_DARWIN=1 -DTASKPARTS_ARM64=1 
TASKPARTS_OPTIONAL_FLAGS?=
TASKPARTS_CPP_FLAGS?=-std=c++17 -fno-stack-protector -fno-asynchronous-unwind-tables -I$(TASKPARTS_ROOT_PATH)/include/
TASKPARTS_FLAGS=$(TASKPARTS_CPP_FLAGS) $(TASKPARTS_PLATFORM_FLAGS) $(TASKPARTS_OPTIONAL_FLAGS) -DPARLAY_TASKPARTS
TASKPARTS_OPT_FLAGS=-O3 -march=native -DNDEBUG
TASKPARTS_STA=-DTASKPARTS_STATS=1
TASKPARTS_LOG=-DTASKPARTS_LOGGING=1
TASKPARTS_STA_FLAGS=$(TASKPARTS_STA) $(TASKPARTS_OPT_FLAGS)
TASKPARTS_LOG_FLAGS=$(TASKPARTS_LOG) $(TASKPARTS_OPT_FLAGS)
TASKPARTS_DBG_FLAGS=$(TASKPARTS_STA) $(TASKPARTS_LOG) $(VALGRIND_CFLAG) -O -g3 #-fsanitize=address
TASKPARTS_MSDBG=-DTASKPARTS_META_SCHEDULER_SERIAL_RANDOM=1
TASKPARTS_MSDBG_FLAGS=$(TASKPARTS_MSDBG) $(TASKPARTS_DBG_FLAGS)
TASKPARTS_OPTIONAL_LINKER_FLAGS?=
TASKPARTS_LINKER_FLAGS=-latomic $(TASKPARTS_OPTIONAL_LINKER_FLAGS) 

# Shared library
# ==============

libtaskparts.opt.o: $(TASKPARTS_SRC_PATH)/taskparts.cpp install_folder
	$(CXX) $(TASKPARTS_FLAGS) $(TASKPARTS_OPT_FLAGS) $(TASKPARTS_SRC_PATH)/taskparts.cpp -c -o $(TASKPARTS_BINARY_PATH)/$@
libtaskparts.sta.o: $(TASKPARTS_SRC_PATH)/taskparts.cpp install_folder
	$(CXX) $(TASKPARTS_FLAGS) $(TASKPARTS_STA_FLAGS) $(TASKPARTS_SRC_PATH)/taskparts.cpp -c -o $(TASKPARTS_BINARY_PATH)/$@
libtaskparts.log.o: $(TASKPARTS_SRC_PATH)/taskparts.cpp install_folder
	$(CXX) $(TASKPARTS_FLAGS) $(TASKPARTS_LOG_FLAGS) $(TASKPARTS_SRC_PATH)/taskparts.cpp -c -o $(TASKPARTS_BINARY_PATH)/$@
libtaskparts.dbg.o: $(TASKPARTS_SRC_PATH)/taskparts.cpp install_folder
	$(CXX) $(TASKPARTS_FLAGS) $(TASKPARTS_DBG_FLAGS) $(TASKPARTS_SRC_PATH)/taskparts.cpp -c -o $(TASKPARTS_BINARY_PATH)/$@
libtaskparts.msdbg.o: $(TASKPARTS_SRC_PATH)/taskparts.cpp install_folder
	$(CXX) $(TASKPARTS_FLAGS) $(TASKPARTS_MSDBG_FLAGS) $(TASKPARTS_SRC_PATH)/taskparts.cpp -c -o $(TASKPARTS_BINARY_PATH)/$@

libtaskparts.opt.so: libtaskparts.opt.o
	$(CC) -shared -o $(TASKPARTS_BINARY_PATH)/libtaskparts.opt.so $(TASKPARTS_BINARY_PATH)/libtaskparts.opt.o
libtaskparts.sta.so: libtaskparts.sta.o
	$(CC) -shared -o $(TASKPARTS_BINARY_PATH)/libtaskparts.sta.so $(TASKPARTS_BINARY_PATH)/libtaskparts.sta.o
libtaskparts.log.so: libtaskparts.log.o
	$(CC) -shared -o $(TASKPARTS_BINARY_PATH)/libtaskparts.log.so $(TASKPARTS_BINARY_PATH)/libtaskparts.log.o
libtaskparts.dbg.so: libtaskparts.dbg.o
	$(CC) -shared -o $(TASKPARTS_BINARY_PATH)/libtaskparts.dbg.so $(TASKPARTS_BINARY_PATH)/libtaskparts.dbg.o
libtaskparts.msdbg.so: libtaskparts.msdbg.o
	$(CC) -shared -o $(TASKPARTS_BINARY_PATH)/libtaskparts.msdbg.so $(TASKPARTS_BINARY_PATH)/libtaskparts.msdbg.o

libtaskparts.opt.a: libtaskparts.opt.o
	ar rs $(TASKPARTS_BINARY_PATH)/libtaskparts.opt.a $(TASKPARTS_BINARY_PATH)/libtaskparts.opt.o
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

%.opt.o: %.cpp libtaskparts.opt.so install_folder
	$(CXX) $(TASKPARTS_FLAGS) $(TASKPARTS_OPT_FLAGS) $< -c -o $(TASKPARTS_BINARY_PATH)/$@
%.opt: %.cpp %.opt.o libtaskparts.opt.so install_folder
	$(CXX) $(TASKPARTS_FLAGS) $(TASKPARTS_OPT_FLAGS) $(TASKPARTS_BINARY_PATH)/$(basename $<).opt.o -o $(TASKPARTS_BINARY_PATH)/$@ -L$(TASKPARTS_BINARY_PATH) -ltaskparts.opt $(TASKPARTS_LINKER_FLAGS)
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

# Serial
# ------

SERIAL_CFLAGS=-DPARLAY_SEQUENTIAL $(TASKPARTS_OPTIONAL_FLAGS)

%.serial_opt: %.cpp $(INCLUDE_FILES) install_folder
	$(CXX) $(SERIAL_CFLAGS) $(TASKPARTS_OPT_FLAGS) -o $(TASKPARTS_BINARY_PATH)/$@ $< 
%.serial_dbg: %.cpp $(INCLUDE_FILES) install_folder
	$(CXX) $(SERIAL_CFLAGS) $(TASKPARTS_DBG_FLAGS) -o $(TASKPARTS_BINARY_PATH)/$@ $< 

# Homegrown (parlay)
# ---------

HOMEGROWN_CFLAGS=-std=c++17 -DPARLAY_HOMEGROWN $(TASKPARTS_OPTIONAL_FLAGS)

%.homegrown_opt: %.cpp $(INCLUDE_FILES) install_folder
	$(CXX) $(HOMEGROWN_CFLAGS) $(TASKPARTS_OPT_FLAGS) -o $(TASKPARTS_BINARY_PATH)/$@ $< 
%.homegrown_dbg: %.cpp $(INCLUDE_FILES) install_folder
	$(CXX) $(HOMEGROWN_CFLAGS) $(TASKPARTS_DBG_FLAGS) -o $(TASKPARTS_BINARY_PATH)/$@ $< 

# OpenCilk
# --------

OPENCILK_CXX?=clang++
OPENCILK_COMMON_PREFIX=-DPARLAY_OPENCILK $(TASKPARTS_FLAGS) $(TASKPARTS_OPT_FLAGS) -fopencilk

%.opencilk.sta: %.opencilk.cpp $(INCLUDE_FILES) install_folder
	$(OPENCILK_CXX) -o $(TASKPARTS_BINARY_PATH)/$@ $< $(OPENCILK_COMMON_PREFIX) -L$(TASKPARTS_BINARY_PATH)


# Install
# =======

install: libtaskparts.sta.o libtaskparts.sta.a
	mkdir -p $(TASKPARTS_INSTALL_PATH)/lib
	cp bin/*.so $(TASKPARTS_INSTALL_PATH)/lib
	ln -s $(TASKPARTS_INSTALL_PATH)/lib/libtaskparts.sta.so $(TASKPARTS_INSTALL_PATH)/lib/libtaskparts.so
	cp bin/*.a $(TASKPARTS_INSTALL_PATH)/lib
	ln -s $(TASKPARTS_INSTALL_PATH)/lib/libtaskparts.sta.a $(TASKPARTS_INSTALL_PATH)/lib/libtaskparts.a
	cp -r ../include $(TASKPARTS_INSTALL_PATH)
	mkdir -p $(TASKPARTS_INSTALL_PATH)/benchmarks
	cp -r bin/*.sta $(TASKPARTS_INSTALL_PATH)/benchmarks

.PHONY: install_folder
install_folder:
	mkdir -p $(TASKPARTS_BINARY_PATH)

clean:
	rm -rf bin
