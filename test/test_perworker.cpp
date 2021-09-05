#include "taskparts/perworker.hpp"
#include <stdio.h>
#include <assert.h>

namespace taskparts {

perworker::array<int> pwints;

auto test1() -> bool {
  for (int i = 0; i < pwints.size(); i++) {
    pwints[i] = 123;
  }
  return true;
}

} // end namespace

int main() {
  assert(taskparts::test1());
  return 0;
}
