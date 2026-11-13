# Builds four of TaskPaRTS's benchmark drivers (../../../benchmark/*.cpp),
# whose algorithms are ParlayLib's examples, against micro taskparts and the
# harness in benchmark.hpp. Give the ParlayLib source tree:
#
#   make PARLAYLIB=<parlaylib> KIND=stats
#
# KIND is serial (ParlayLib with no scheduler), or a level of instrumentation
# of micro taskparts: plain, stats, log or forks. The programs go to
# bin/KIND/.

CXX ?= c++
OPTFLAGS ?= -O3 -DNDEBUG -falign-functions=64
KIND ?= plain
PROGRAMS = mergesort samplesort quickhull BFS
DRIVERS = ../../../benchmark
MTP = ../microtaskparts

FLAGS_serial = -DPARLAY_SEQUENTIAL
MTP_FLAGS = -DPARLAY_TASKPARTS -I../parlay-adapter -I$(MTP)
FLAGS_plain = $(MTP_FLAGS)
FLAGS_stats = $(MTP_FLAGS) -DMTP_STATS
FLAGS_log = $(MTP_FLAGS) -DMTP_LOG
FLAGS_forks = $(MTP_FLAGS) -DMTP_COUNT_FORKS

CXXFLAGS = -std=c++17 -pthread $(OPTFLAGS) $(FLAGS_$(KIND)) \
  -I. -I$(PARLAYLIB)/include -I$(PARLAYLIB)/examples

all: $(addprefix bin/$(KIND)/,$(PROGRAMS))

# The drivers include "benchmark.hpp", which the compiler looks for beside
# the driver first. So each is compiled from a copy, beside nothing.
bin/$(KIND)/%: $(DRIVERS)/%.cpp benchmark.hpp $(MTP)/microtaskparts.hpp
	@test -n "$(PARLAYLIB)" || (echo "set PARLAYLIB" >&2; exit 1)
	@mkdir -p bin/$(KIND) build/$(KIND)
	cp $< build/$(KIND)/$*.cpp
	$(CXX) $(CXXFLAGS) build/$(KIND)/$*.cpp -o $@

clean:
	rm -rf bin build

.PHONY: all clean
