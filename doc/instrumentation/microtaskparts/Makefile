CXX ?= c++
# Functions are aligned to a cache line so that the builds can be compared:
# adding code anywhere moves the functions that follow it, and where fib
# happens to land changes its running time by a few percent.
OPTFLAGS ?= -O2 -falign-functions=64
CXXFLAGS = -std=c++17 -pthread -Wall $(OPTFLAGS)

PROGRAMS = fib migrate

all: $(addprefix bin/,$(PROGRAMS))

# Each program builds plain, as bin/NAME, and with each kind of
# instrumentation, as bin/NAME.KIND.
KINDS = stats log forks atomic packed
FLAGS_stats = -DMTP_STATS
FLAGS_log = -DMTP_LOG
FLAGS_forks = -DMTP_COUNT_FORKS
# fib only: forks counted the wrong way (see fib.cpp)
FLAGS_atomic = -DCOUNT_FORKS_ATOMIC
FLAGS_packed = -DCOUNT_FORKS_PACKED

instrumented: $(foreach p,$(PROGRAMS),$(addprefix bin/$(p).,stats log forks)) \
  bin/fib.atomic bin/fib.packed

bin/%: %.cpp microtaskparts.hpp context.hpp
	@mkdir -p bin
	$(CXX) $(CXXFLAGS) $< -o $@

define KIND_RULE
bin/%.$(1): %.cpp microtaskparts.hpp context.hpp
	@mkdir -p bin
	$$(CXX) $$(CXXFLAGS) $$(FLAGS_$(1)) $$< -o $$@
endef
$(foreach k,$(KINDS),$(eval $(call KIND_RULE,$(k))))

clean:
	rm -rf bin

.PHONY: all instrumented clean
