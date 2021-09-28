#include <taskparts/benchmark.hpp>
#include <parlay/delayed_sequence.h>
#include <parlay/monoid.h>
#include <parlay/primitives.h>
#include <parlay/parallel.h>

// ------------- Maximum Contiguous Subsequence Sum ------------------

// 10x improvement by using delayed sequence
template <class Seq>
typename Seq::value_type mcss(Seq const &A) {
  using T = typename Seq::value_type;
  auto f = [&] (auto a, auto b) {
    auto [aa, pa, sa, ta] = a;
    auto [ab, pb, sb, tb] = b;
    return std::make_tuple(
      std::max(aa,std::max(ab,sa+pb)),
      std::max(pa, ta+pb),
      std::max(sa + ab, sb),
      ta + tb
    );
  };
  auto S = parlay::delayed_seq<std::tuple<T,T,T,T>>(A.size(), [&] (size_t i) {
    return std::make_tuple(A[i],A[i],A[i],A[i]);
  });
  auto m = parlay::make_monoid(f, std::tuple<T,T,T,T>(0,0,0,0));
  auto r = parlay::reduce(S, m);
  return std::get<0>(r);
}

int main() {
  size_t n = taskparts::cmdline::parse_or_default_long("n", 100000000);
  std::vector<long long> a(n);
  parlay::parallel_for(0, n, [&](auto i) {
    a[i] = (i % 2 == 0 ? -1 : 1) * i;
  });
  parlay::benchmark_taskparts([&] (auto sched) {
    mcss(a);
  });
  return 0;
}
