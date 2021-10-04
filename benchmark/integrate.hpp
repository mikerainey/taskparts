#pragma once

#include <parlay/delayed_sequence.h>
#include <parlay/monoid.h>
#include <parlay/primitives.h>
#include <parlay/parallel.h>

// ---------------------------- Integration ----------------------------

template <typename F>
double integrate(size_t num_samples, double start, double end, F f) {
   double delta = (end-start)/num_samples;
   auto samples = parlay::delayed_seq<double>(num_samples, [&] (size_t i) {
        return f(start + delta/2 + i * delta); });
   return delta * parlay::reduce(samples, parlay::addm<double>());

}
