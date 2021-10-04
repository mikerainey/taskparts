#pragma once

#include <parlay/delayed_sequence.h>
#include <parlay/monoid.h>
#include <parlay/primitives.h>
#include <parlay/parallel.h>
#include <testData/sequenceData/sequenceData.h>
#ifndef PARLAY_SEQUENTIAL
#include <comparisonSort/sampleSort/sort.h>
#else
#include <comparisonSort/serialSort/sort.h>
#endif

size_t dflt_n = 10000000;
