CXX ?= c++
OPTFLAGS ?= -O2
CXXFLAGS = -std=c++17 -pthread -Wall $(OPTFLAGS)

PROGRAMS = fib migrate mixed seidel pipeline stress instr vertices
HEADERS = microtaskparts.hpp context.hpp

all: $(addprefix bin/,$(PROGRAMS))

bin/%: %.cpp $(HEADERS)
	@mkdir -p bin
	$(CXX) $(CXXFLAGS) $< -o $@

# Builds for testing. Each program has four more builds:
#   bin/NAME.perturb  workers pause at random where races are decided, and
#                     stacks and the ends of computations are checked (see
#                     microtaskparts.hpp)
#   bin/NAME.asan     the same, with the address sanitizer
#   bin/NAME.tsan     the same, with the thread sanitizer
#   bin/NAME.dbg      assertions on, no optimization
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

bin/%.dbg: %.cpp $(HEADERS)
	@mkdir -p bin
	$(CXX) -std=c++17 -pthread -Wall -O0 -g -DMTP_CHECK_STACKS -DMTP_CHECK_ORPHANS $< -o $@

# Builds with instrumentation (see microtaskparts.hpp):
#   bin/NAME.sta    totals of the time in each state, and counts of steals
#   bin/NAME.log    those, plus the log
#   bin/NAME.cnt    those of .sta, plus the counts of forks and of the
#                   operations of DAG calculus
#   bin/NAME.every  those of .sta, with every return to the loop recorded
bin/%.sta: %.cpp $(HEADERS)
	@mkdir -p bin
	$(CXX) $(CXXFLAGS) -DMTP_STATS $< -o $@

bin/%.log: %.cpp $(HEADERS)
	@mkdir -p bin
	$(CXX) $(CXXFLAGS) -DMTP_LOG $< -o $@

bin/%.cnt: %.cpp $(HEADERS)
	@mkdir -p bin
	$(CXX) $(CXXFLAGS) -DMTP_COUNT_FORKS -DMTP_COUNT_DAG $< -o $@

bin/%.every: %.cpp $(HEADERS)
	@mkdir -p bin
	$(CXX) $(CXXFLAGS) -DMTP_MARK_EVERY_SWITCH $< -o $@

# The builds for testing, with all of the instrumentation on.
INSTRFLAGS = -DMTP_LOG -DMTP_COUNT_FORKS -DMTP_COUNT_DAG

bin/%.iperturb: %.cpp $(HEADERS)
	@mkdir -p bin
	$(CXX) $(CXXFLAGS) $(TESTFLAGS) $(INSTRFLAGS) $< -o $@

bin/%.itsan: %.cpp $(HEADERS)
	@mkdir -p bin
	$(CXX) $(CXXFLAGS) $(TESTFLAGS) $(INSTRFLAGS) -fsanitize=thread $< -o $@

bin/%.ievery: %.cpp $(HEADERS)
	@mkdir -p bin
	$(CXX) $(CXXFLAGS) $(TESTFLAGS) $(INSTRFLAGS) -DMTP_MARK_EVERY_SWITCH $< -o $@

clean:
	rm -rf bin

.PHONY: all clean
