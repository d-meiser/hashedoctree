#ifndef SPATIAL_SORT_TREE_H
#define SPATIAL_SORT_TREE_H

#include <vector>


struct HOTPoint {
  double x;
  double y;
  double z;
};

struct HOTBoundingBox {
  HOTPoint min;
  HOTPoint max;
};

struct HOTItem {
  HOTPoint position;
  void* data;
};


class SpatialSortTree {
  public:
    virtual ~SpatialSortTree() = default;

    virtual void InsertItems(const HOTItem* begin, const HOTItem* end) = 0;

    class VertexVisitor {
      public:
        virtual ~VertexVisitor() = default;
        virtual bool Visit(HOTItem* item) = 0;
    };
    virtual bool VisitNearVertices(VertexVisitor* visitor, HOTPoint position, double eps) = 0;

    virtual std::vector<HOTItem>::iterator begin() = 0;
    virtual std::vector<HOTItem>::iterator end() = 0;
};

#endif
