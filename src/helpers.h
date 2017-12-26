#ifndef HELPERS_H
#define HELPERS_H

#include <cmath>
#include <cassert>
#include <limits>


inline double DistanceFromInterval(double a, double b, double x) {
  assert(b >= a);
  double dist = 0;
  dist = std::max(dist, std::max(0.0, a - x));
  dist = std::max(dist, std::max(0.0, x - b));
  return dist;
}

inline double LInfinity(const HOTBoundingBox& bbox, const HOTPoint& point) {
  double dist = 0;
  dist = std::max(dist, DistanceFromInterval(bbox.min.x, bbox.max.x, point.x));
  dist = std::max(dist, DistanceFromInterval(bbox.min.y, bbox.max.y, point.y));
  dist = std::max(dist, DistanceFromInterval(bbox.min.z, bbox.max.z, point.z));
  return dist;
}

inline double LInfinity(const HOTPoint& p0, const HOTPoint& p1) {
  double dist = 0;
  dist = std::max(dist, std::fabs(p0.x - p1.x));
  dist = std::max(dist, std::fabs(p0.y - p1.y));
  dist = std::max(dist, std::fabs(p0.z - p1.z));
  return dist;
}

inline double DistanceFromEdgesOfInterval(double a, double b, double x) {
  assert(b >= a);
  double dist = std::numeric_limits<double>::max();
  dist = std::min(dist, std::abs(a - x));
  dist = std::min(dist, std::abs(b - x));
  return dist;
}

inline double DistanceFromBoundary(const HOTBoundingBox& bbox, const HOTPoint& point) {
  double dist = std::numeric_limits<double>::max();
  dist = std::min(dist, DistanceFromEdgesOfInterval(bbox.min.x, bbox.max.x, point.x));
  dist = std::min(dist, DistanceFromEdgesOfInterval(bbox.min.y, bbox.max.y, point.y));
  dist = std::min(dist, DistanceFromEdgesOfInterval(bbox.min.z, bbox.max.z, point.z));
  return dist;
}

#endif
