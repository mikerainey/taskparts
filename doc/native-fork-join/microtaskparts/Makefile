CXX ?= c++
OPTFLAGS ?= -O2
CXXFLAGS = -std=c++17 -pthread -Wall $(OPTFLAGS)

PROGRAMS = fib migrate

all: $(addprefix bin/,$(PROGRAMS))

bin/%: %.cpp microtaskparts.hpp context.hpp
	@mkdir -p bin
	$(CXX) $(CXXFLAGS) $< -o $@

clean:
	rm -rf bin

.PHONY: all clean
