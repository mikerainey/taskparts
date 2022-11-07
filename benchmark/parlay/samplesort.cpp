#include "comparisonsort.hpp"

int main() {
  parlay::benchmark_taskparts([&] (auto sched) { // benchmark
    benchmark();
  }, [&] (auto sched) { // setup
    gen_input();
  }, [&] (auto sched) { // teardown
#ifndef NDEBUG
    auto first_ten = result.head(10);
    std::cout << "first 10 elements: " << parlay::to_chars(first_ten) << std::endl;
#endif
  }, [&] (auto sched) { // reset
    std::copy(data.begin(), data.end(), result.begin());
    // the line below currently causes livelock b/c it spawns; the blame goes to the reset_fiber
    //result = data;
  });
  return 0;
}
