#ifndef WIDE_TREE_H
#define WIDE_TREE_H

#include <spatialsorttree.h>
#include <vector>
#include <memory>


class WideNode;

class WideTree : public SpatialSortTree {
  public:
    WideTree(HOTBoundingBox bbox);
    WideTree(const WideTree&) = delete;
    WideTree& operator=(const WideTree&) = delete;
    WideTree(WideTree&&);
    WideTree& operator=(WideTree&& rhs);
    ~WideTree() override;

    void InsertItems(const HOTItem* begin, const HOTItem* end) override;

    bool VisitNearVertices(SpatialSortTree::VertexVisitor* visitor, HOTPoint position, double eps2) override;

    size_t Size() const;
    std::vector<HOTItem>::iterator begin() override;
    std::vector<HOTItem>::iterator end() override;

    void SetMaxNumLeafItems(int max_num_leaf_items);

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

template <typename T>
void ApplyPermutation(const int* perm, int n, const T* in, T* out) {
  for (int i = 0; i < n; ++i) {
    out[i] = in[perm[i]];
  }
}


#endif
