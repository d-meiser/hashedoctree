#ifndef HASHED_OCTREE_H
#define HASHED_OCTREE_H

#include <spatialsorttree.h>

#include <cstdint>
#include <utility>
#include <vector>
#include <memory>
#include <limits>
#include <cmath>


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
HOTBoundingBox ComputeChildBox(HOTBoundingBox bbox, int octant);


// This should become an internal function down the road.
HOTKey HOTComputeHash(HOTBoundingBox bbox, HOTPoint point);

class HOTNode;

class HOTTree : public SpatialSortTree {
  public:
    HOTTree(HOTBoundingBox bbox);
    HOTTree(HOTTree&&);
    HOTTree& operator=(HOTTree&& rhs);
    ~HOTTree() override;

    void InsertItems(const HOTItem* begin, const HOTItem* end) override;

    bool VisitNearVertices(VertexVisitor* visitor, HOTPoint position, double eps2) override;

    std::vector<HOTItem>::iterator begin() override;
    std::vector<HOTItem>::iterator end() override;

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


#endif
