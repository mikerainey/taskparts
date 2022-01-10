#pragma once

#include "comparisonsort.hpp"
#ifndef PARLAY_SEQUENTIAL
#include <benchmarks/comparisonSort/quickSort/sort.h>
#else
#include <benchmarks/comparisonSort/quickSort/sort.h>
#endif

template <class T, class BinPred>
parlay::sequence<T> compSort2(parlay::sequence<T> &A, const BinPred& f) {
  compSort(A, f);
  return A;
}
