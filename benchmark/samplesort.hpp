#pragma once

#include "comparisonsort.hpp"
#ifndef PARLAY_SEQUENTIAL
#include <comparisonSort/sampleSort/sort.h>
#else
#include <comparisonSort/serialSort/sort.h>
#endif

template <class T, class BinPred>
parlay::sequence<T> compSort2(parlay::sequence<T> &A, const BinPred& f) {
  return compSort(A, f);
}
