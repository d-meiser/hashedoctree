#ifndef HASHED_OCTREE_H
#define HASHED_OCTREE_H

#include <cstdint>
#include <utility>
#include <vector>
#include <memory>
#include <limits>
#include <cassert>
#include <cmath>


struct HOTPoint {
  double x;
  double y;
  double z;
};

struct HOTBoundingBox {
  HOTPoint min;
  HOTPoint max;
};


typedef uint32_t HOTKey;

// Node keys are similar to the vertex keys.
typedef uint32_t HOTNodeKey;
HOTNodeKey HOTNodeRoot();
void HOTNodeComputeChildKeys(HOTNodeKey key, HOTNodeKey* child_keys);
bool HOTNodeValidKey(HOTNodeKey key);
int HOTNodeLevel(HOTNodeKey key);
HOTNodeKey HOTNodeParent(HOTNodeKey key);
HOTKey HOTNodeBegin(HOTNodeKey key);
HOTKey HOTNodeEnd(HOTNodeKey key);
void HOTNodePrint(HOTNodeKey key);

// A bunch of helpers
inline double DistanceFromInterval(double a, double b, double x);
inline double LInfinity(const HOTBoundingBox& bbox, const HOTPoint& point);
inline double LInfinity(const HOTPoint& p0, const HOTPoint& p1);
inline double DistanceFromEdgesOfInterval(double a, double b, double x);
inline double DistanceFromBoundary(const HOTBoundingBox& bbox, const HOTPoint& point);


// This should become an internal function down the road.
HOTKey HOTComputeHash(HOTBoundingBox bbox, HOTPoint point);

struct HOTItem {
  HOTPoint position;
  void* data;
};

class HOTNode;

class HOTTree {
  public:
    HOTTree(HOTBoundingBox bbox);
    HOTTree(HOTTree&&);
    HOTTree& operator=(HOTTree&& rhs);
    ~HOTTree();

    void InsertItems(const HOTItem* begin, const HOTItem* end);

    class VertexVisitor {
      public:
        virtual ~VertexVisitor() = default;
        virtual bool Visit(HOTItem* item) = 0;
    };
    bool VisitNearVertices(VertexVisitor* visitor, HOTPoint position, double eps2);

    std::vector<HOTItem>::iterator begin();
    std::vector<HOTItem>::iterator end();

    // Some diagnostics;
    int NumNodes() const;
    int Depth() const;
    void PrintNumItems() const;
    size_t Size() const;

  private:
    HOTBoundingBox bbox_;
    std::vector<HOTItem> items_;
    std::vector<HOTKey> keys_;
    std::unique_ptr<HOTNode> root_;

    void RebuildNodes();
};

double DistanceFromInterval(double a, double b, double x) {
  assert(b >= a);
  double dist = 0;
  dist = std::max(dist, std::max(0.0, a - x));
  dist = std::max(dist, std::max(0.0, x - b));
  return dist;
}

double LInfinity(const HOTBoundingBox& bbox, const HOTPoint& point) {
  double dist = 0;
  dist = std::max(dist, DistanceFromInterval(bbox.min.x, bbox.max.x, point.x));
  dist = std::max(dist, DistanceFromInterval(bbox.min.y, bbox.max.y, point.y));
  dist = std::max(dist, DistanceFromInterval(bbox.min.z, bbox.max.z, point.z));
  return dist;
}

double LInfinity(const HOTPoint& p0, const HOTPoint& p1) {
  double dist = 0;
  dist = std::max(dist, std::fabs(p0.x - p1.x));
  dist = std::max(dist, std::fabs(p0.y - p1.y));
  dist = std::max(dist, std::fabs(p0.z - p1.z));
  return dist;
}

double DistanceFromEdgesOfInterval(double a, double b, double x) {
  assert(b >= a);
  double dist = std::numeric_limits<double>::max();
  dist = std::min(dist, std::abs(a - x));
  dist = std::min(dist, std::abs(b - x));
  return dist;
}

double DistanceFromBoundary(const HOTBoundingBox& bbox, const HOTPoint& point) {
  double dist = std::numeric_limits<double>::max();
  dist = std::min(dist, DistanceFromEdgesOfInterval(bbox.min.x, bbox.max.x, point.x));
  dist = std::min(dist, DistanceFromEdgesOfInterval(bbox.min.y, bbox.max.y, point.y));
  dist = std::min(dist, DistanceFromEdgesOfInterval(bbox.min.z, bbox.max.z, point.z));
  return dist;
}

#endif
