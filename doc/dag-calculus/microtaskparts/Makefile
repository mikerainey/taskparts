CXX ?= c++
OPTFLAGS ?= -O2
CXXFLAGS = -std=c++17 -pthread -Wall $(OPTFLAGS)

PROGRAMS = fib migrate mixed seidel pipeline stress
HEADERS = microtaskparts.hpp context.hpp

all: $(addprefix bin/,$(PROGRAMS))

bin/%: %.cpp $(HEADERS)
	@mkdir -p bin
	$(CXX) $(CXXFLAGS) $< -o $@

# Builds for testing. Each program has four more builds:
#   bin/NAME.perturb  workers pause at random where races are decided, and
#                     stacks are checked (see microtaskparts.hpp)
#   bin/NAME.asan     the same, with the address sanitizer
#   bin/NAME.tsan     the same, with the thread sanitizer
#   bin/NAME.dbg      assertions on, no optimization
TESTFLAGS = -DMTP_PERTURB -DMTP_CHECK_STACKS -g

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
	$(CXX) -std=c++17 -pthread -Wall -O0 -g -DMTP_CHECK_STACKS $< -o $@

clean:
	rm -rf bin

.PHONY: all clean
