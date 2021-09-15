#pragma once

#include <string>
#include <cstdint>
#include <sstream>

#include "machine.hpp"
#include "atomic.hpp"
#include "nativeforkjoin.hpp"

namespace taskparts {

using complexity_type = int64_t;

/*---------------------------------------------------------------------*/
/* The estimator data structure */
  
class estimator {
private:

  using state_type = union state_union {
    struct { float c; int nmax; } f;
    uint64_t u;
  };

  alignas(TASKPARTS_CACHE_LINE_SZB)
  std::atomic<uint64_t> s;

  alignas(TASKPARTS_CACHE_LINE_SZB)
  std::string name;

  static constexpr
  float alpha = 1.5;
    
public:

  estimator(std::string name) {
    state_type s0;
    s0.f = {.c = 0, .nmax = 0 };
    s.store(s0.u);
    std::stringstream stream;
    stream << name.substr(0, std::min(40, (int)name.length())) << this;
    this->name = stream.str();
  }

  auto get_name() -> const char* {
    return name.c_str();
  }  

  auto report(complexity_type n, uint64_t t) {
    auto kappa = get_kappa_cycles();
    bool done = false;
    do {
      state_type _ls;
      _ls.u = s.load();
      auto ls = _ls.f;
      auto c = ls.c;
      auto nmax = (complexity_type)ls.nmax;
      if ((t <= kappa) && (n > nmax)) {
	auto c2 = (float)t / (float)n;
	state_type ls2;
	ls2.f = {.c = c2, .nmax = (int)n};
	done = compare_exchange_with_backoff(s, _ls.u, ls2.u);
      } else {
	done = true;
      }
    } while (! done);
  }

  auto is_small(complexity_type n) -> bool {
    state_type _ls;
    _ls.u = s.load();
    auto ls = _ls.f;
    auto c = ls.c;
    auto nmax = ls.nmax;
    auto kappa = (float)get_kappa_cycles();
    return (n <= nmax)
      || (((float)n <= alpha * (float)nmax) && (((float)n * c) <= (alpha * kappa)));
  }
  
};

/*---------------------------------------------------------------------*/
/* Series-parallel guard */
  
namespace {
    
perworker::array<uint64_t> timer(0);

perworker::array<uint64_t> total(0);

perworker::array<bool> is_small(false);

auto total_now(uint64_t t) -> uint64_t {
  return total.mine() + (cycles::now() - t);
}

template <class Func>
auto measured_run(const Func& f) -> uint64_t {
  total.mine() = 0;
  timer.mine() = cycles::now();
  f();
  return total_now(timer.mine());
}
                    
template <
  class Complexity,
  class Par_body,
  class Seq_body
  >
auto _spguard(estimator& estim,
              const Complexity& complexity,
              const Par_body& par_body,
              const Seq_body& seq_body) {
  if (is_small.mine()) {
    seq_body();
    return;
  }
  complexity_type comp = (complexity_type)complexity();
  if (estim.is_small(comp)) {
    is_small.mine() = true;
    auto t = cycles::now();
    seq_body();
    auto elapsed = cycles::now() - t;
    estim.report(comp, elapsed);
    is_small.mine() = false;
  } else {
    auto t_before = total_now(timer.mine());
    auto t_body = measured_run(par_body);
    estim.report(comp, t_body);
    total.mine() = t_before + t_body;
    timer.mine() = cycles::now();
  }
}  

template <class Last>
auto type_name() -> std::string {
  return std::string(typeid(Last).name());
}

template <class First, class Second, class ... Types>
auto type_name() -> std::string {
  return type_name<First>() + "_" + type_name<Second, Types...>();
}

template <const char* ename, class ... Types>
class estim_wrp {
public:

  static
  estimator e;
  
};

template <const char* ename, class ... Types>
estimator estim_wrp<ename, Types ...>::e("" + std::string(ename) + "_" + type_name<Types ...>());

char dflt_ename[] = "_";

} // end namespace

template <
  const char* ename,
  class Complexity,
  class Par_body,
  class Seq_body
  >
auto spguard(const Complexity& complexity,
             const Par_body& par_body,
             const Seq_body& seq_body) {
  using wrapper = estim_wrp<ename, Complexity, Par_body, Seq_body>;
  _spguard(wrapper::e, complexity, par_body, seq_body);
}
  
template <
  class Complexity,
  class Par_body,
  class Seq_body
  >
auto spguard(const Complexity& complexity,
             const Par_body& par_body,
             const Seq_body& seq_body) {
  spguard<dflt_ename>(complexity, par_body, seq_body);
}

template <
  const char* ename,
  class Complexity,
  class Par_body
  >
auto spguard(const Complexity& complexity,
             const Par_body& par_body) {
  using wrapper = estim_wrp<ename, Complexity, Par_body, Par_body>;
  _spguard(wrapper::e, complexity, par_body, par_body);
}
  
template <
  class Complexity,
  class Par_body
  >
auto spguard(const Complexity& complexity,
             const Par_body& par_body) {
  spguard<dflt_ename>(complexity, par_body, par_body);
}
  
/*---------------------------------------------------------------------*/
/* Fork join */

template <typename F1, typename F2, typename Scheduler>
auto ogfork2join(const F1& f1, const F2& f2, Scheduler sched=Scheduler()) {
  if (is_small.mine()) {
    f1();
    f2();
    return;
  }
  auto t_before = total_now(timer.mine());
  uint64_t t_left, t_right;
  fork2join([&] {
    t_left = measured_run(f1);
  }, [&] {
    t_right = measured_run(f2);
  }, sched);
  total.mine() = t_before + t_left + t_right;
  timer.mine() = cycles::now();
}

} // end namespace
