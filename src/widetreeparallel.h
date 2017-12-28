#ifndef WIDE_TREE_PARALLEL_H
#define WIDE_TREE_PARALLEL_H

#include <spatialsorttree.h>
#include <vector>
#include <memory>


class WideNode;

class WideTreeParallel : public SpatialSortTree {
  public:
    WideTreeParallel(HOTBoundingBox bbox);
    WideTreeParallel(const WideTreeParallel&) = delete;
    WideTreeParallel& operator=(const WideTreeParallel&) = delete;
    WideTreeParallel(WideTreeParallel&&);
    WideTreeParallel& operator=(WideTreeParallel&& rhs);
    ~WideTreeParallel();

    void InsertItems(const HOTItem* begin, const HOTItem* end);

    bool VisitNearVertices(SpatialSortTree::VertexVisitor* visitor, HOTPoint position, double eps2);

    size_t Size() const;
    std::vector<HOTItem>::iterator begin() override;
    std::vector<HOTItem>::iterator end() override;

    void SetMaxNumLeafItems(int maxnum_leaf_items);

  private:
    HOTBoundingBox bbox_;
    std::vector<HOTItem> items_;
    std::unique_ptr<WideNode> root_;
    int max_num_leaf_items_;
};

uint8_t ComputeWideKey(const HOTBoundingBox& bbox, HOTPoint location);
void ComputeManyWideKeys(const HOTBoundingBox& bbox, const double* locations,
    int n, int stride, uint8_t* keys);
void SortByKey(const uint8_t* keys, int n, int buckets[257], int* perm);

#endif
