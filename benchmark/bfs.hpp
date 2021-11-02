#pragma once

#include "graph.hpp"

#ifndef PARLAY_SEQUENTIAL
#include <breadthFirstSearch/simpleBFS/BFS.C>
#else
#include <breadthFirstSearch/serialBFS/BFS.C>
#endif

