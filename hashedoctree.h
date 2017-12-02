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


class HOTTree {
 public:
  HOTTree(HOTBoundingBox bbox,
	  const HOTPoint* positions_begin,
	  const HOTPoint* positions_end,
	  const void* data_begin);

 private:
  HOTBoundingBox bbox_;
  const HOTPoint* positions_begin_;
  const HOTPoint* positions_end_;
  const void* data_begin_;
  std::vector<HOTKey> permutation_;
};

#endif
