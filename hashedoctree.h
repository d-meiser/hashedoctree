#ifndef HASHED_OCTREE_H
#define HASHED_OCTREE_H

#include <cstdint>
#include <utility>
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


typedef int32_t HOTKey;

// This should become an internal function down the road.
HOTKey HOTComputeHash(HOTBoundingBox bbox, HOTPoint point);

struct HOTItem {
  HOTPoint position;
  void* data;
};

class HOTTree {
  public:
    HOTTree(HOTBoundingBox bbox);

    void InsertItems(const HOTItem* begin, const HOTItem* end);

  private:
    HOTBoundingBox bbox_;
    std::vector<HOTItem> items_;
    std::vector<HOTKey> keys_;
};

#endif
