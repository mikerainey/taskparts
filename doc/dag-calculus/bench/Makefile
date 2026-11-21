CXX ?= c++
OPTFLAGS ?= -O2
CXXFLAGS = -std=c++17 -pthread -Wall $(OPTFLAGS)

PROGRAMS = pipefib ferret x264 heat life lcs cholesky preorder
MTP = ../microtaskparts
HEADERS = $(MTP)/microtaskparts.hpp $(MTP)/context.hpp harness.hpp pipe.hpp \
          stencil2d.hpp

all: $(addprefix bin/,$(PROGRAMS))

bin/%: %.cpp $(HEADERS)
	@mkdir -p bin
	$(CXX) $(CXXFLAGS) $< -o $@

# Builds for testing, as in $(MTP)/Makefile:
#   bin/NAME.perturb  workers pause at random where races are decided, and
#                     stacks and the ends of computations are checked
#   bin/NAME.asan     the same, with the address sanitizer
#   bin/NAME.tsan     the same, with the thread sanitizer
TESTFLAGS = -DMTP_PERTURB -DMTP_CHECK_STACKS -DMTP_CHECK_ORPHANS -g

bin/%.perturb: %.cpp $(HEADERS)
	@mkdir -p bin
	$(CXX) $(CXXFLAGS) $(TESTFLAGS) $< -o $@

bin/%.asan: %.cpp $(HEADERS)
	@mkdir -p bin
	$(CXX) $(CXXFLAGS) $(TESTFLAGS) -fsanitize=address -fno-omit-frame-pointer $< -o $@

bin/%.tsan: %.cpp $(HEADERS)
	@mkdir -p bin
	$(CXX) $(CXXFLAGS) $(TESTFLAGS) -fsanitize=thread $< -o $@

# Builds with instrumentation:
#   bin/NAME.sta    totals of the time in each state, and counts of steals
#   bin/NAME.cnt    those, plus the counts of forks and of the operations of
#                   DAG calculus
bin/%.sta: %.cpp $(HEADERS)
	@mkdir -p bin
	$(CXX) $(CXXFLAGS) -DMTP_STATS $< -o $@

bin/%.cnt: %.cpp $(HEADERS)
	@mkdir -p bin
	$(CXX) $(CXXFLAGS) -DMTP_COUNT_FORKS -DMTP_COUNT_DAG $< -o $@

clean:
	rm -rf bin

.PHONY: all clean
