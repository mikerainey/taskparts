#pragma once

#include "comparisonsort.hpp"
#ifndef PARLAY_SEQUENTIAL
#include <comparisonSort/quickSort/sort.h>
#else
#include <comparisonSort/quickSort/sort.h>
#endif

template <class T, class BinPred>
parlay::sequence<T> compSort2(parlay::sequence<T> &A, const BinPred& f) {
  compSort(A, f);
  return A;
}
