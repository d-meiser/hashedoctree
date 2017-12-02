#ifndef HASHED_OCTREE_H
#define HASHED_OCTREE_H

#include <cstdint>
#include <utility>
#include <vector>
#include <memory>


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

// Node keys are similar to the 
typedef uint32_t HOTNodeKey;
HOTNodeKey HOTNodeRoot();
void HOTNodeComputeChildKeys(HOTNodeKey key, HOTNodeKey* child_keys);
bool HOTNodeValidKey(HOTNodeKey key);
int HOTNodeLevel(HOTNodeKey key);
HOTNodeKey HOTNodeParent(HOTNodeKey key);
HOTKey HOTNodeBegin(HOTNodeKey key);
HOTKey HOTNodeEnd(HOTNodeKey key);

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
    ~HOTTree();

    void InsertItems(const HOTItem* begin, const HOTItem* end);

  private:
    HOTBoundingBox bbox_;
    std::vector<HOTItem> items_;
    std::vector<HOTKey> keys_;
    std::unique_ptr<HOTNode> root_;

    void RebuildNodes();
};

#endif
