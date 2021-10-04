#pragma once

#include <parlay/delayed_sequence.h>
#include <parlay/monoid.h>
#include <parlay/primitives.h>
#include <parlay/parallel.h>
#include <common/IO.h>
#include <common/sequenceIO.h>
#ifndef PARLAY_SEQUENTIAL
#include <suffixArray/bench/SA.h>
#include <suffixArray/parallelKS/SA.C>
#else
#include <suffixArray/bench/SA.h>
#include <suffixArray/parallelKS/SA.C>
// later: get serial algorithm to compile
/*
#include <suffixArray/bench/SA.h>
#include <suffixArray/serialDivsufsort/SA.C>
#include <suffixArray/serialDivsufsort/divsufsort.C>
#include <suffixArray/serialDivsufsort/trsort.C>
#include <suffixArray/serialDivsufsort/sssort.C>
*/
#endif
using uchar = unsigned char;
