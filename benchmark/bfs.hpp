#pragma once

#include <parlay/delayed_sequence.h>
#include <parlay/monoid.h>
#include <parlay/primitives.h>
#include <parlay/parallel.h>

#include <common/graph.h>
#include <common/IO.h>
#include <common/graphIO.h>
#include <common/sequenceIO.h>
#ifndef PARLAY_SEQUENTIAL
#include <breadthFirstSearch/simpleBFS/BFS.C>
#else
#include <breadthFirstSearch/serialBFS/BFS.C>
#endif
using namespace std;
using namespace benchIO;

