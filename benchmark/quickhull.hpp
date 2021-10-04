#pragma once

#include <math.h>
#include <parlay/delayed_sequence.h>
#include <parlay/monoid.h>
#include <parlay/primitives.h>
#include <parlay/parallel.h>
#include <common/IO.h>
#include <common/geometry.h>
#include <common/geometryIO.h>
#include <testData/geometryData/geometryData.h>
#ifndef PARLAY_SEQUENTIAL
#include <convexHull/quickHull/hull.C>
#else
#include <convexHull/serialHull/hull.C>
#endif
using namespace benchIO;
using namespace dataGen;
using namespace std;

using coord = double;

template <class coord>
point2d<coord> randKuzmin(size_t i) {
  vector2d<coord> v = vector2d<coord>(randOnUnitSphere2d<coord>(i));
  size_t j = dataGen::hash<size_t>(i);
  double s = dataGen::hash<double>(j);
  double r = sqrt(1.0/((1.0-s)*(1.0-s))-1.0);
  return point2d<coord>(v*r);
}

template <class coord>
point3d<coord> randPlummer(size_t i) {
  vector3d<coord> v = vector3d<coord>(randOnUnitSphere3d<coord>(i));
  size_t j = dataGen::hash<size_t>(i);
  double s = pow(dataGen::hash<double>(j),2.0/3.0);
  double r = sqrt(s/(1-s));
  return point3d<coord>(v*r);
}

size_t dflt_n = 10000000;
