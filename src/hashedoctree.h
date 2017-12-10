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

#endif
