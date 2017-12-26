#include <widetreeparallel.h>
#include <widetree.h>
#include <helpers.h>
#include <cassert>
#include <cmath>


class WideNode {
  static const int MAX_NUM_LEAF_VERTICES = 256;

  public:
    WideNode(HOTBoundingBox bbox) : bbox_(bbox) {}

    void InsertItemsInPlace(HOTItem* begin, HOTItem* end) {
      int n = std::distance(begin, end);
      if (n == 0) return;
      std::vector<HOTItem> temp(n);
      InsertItems(begin, end, &temp[0]);
      std::copy(temp.begin(), temp.end(), begin);
    }

    void InsertItems(const HOTItem* begin, const HOTItem* end, HOTItem* sorted_items) {
      items_begin_ = sorted_items;
      items_end_ = sorted_items + std::distance(begin, end);
      int n = std::distance(begin, end);
      if (n < MAX_NUM_LEAF_VERTICES) {
        std::copy(begin, end, sorted_items);
        return;
      }
      std::vector<uint8_t> keys(n);
      ComputeManyWideKeys(bbox_, &begin->position.x, n, 4, &keys[0]);
      std::vector<int> perm(n);
      SortByKey(&keys[0], n, buckets_, &perm[0]);
      std::vector<HOTItem> temp(n);
      ApplyPermutation(&perm[0], n, begin, sorted_items);
      double dx = (bbox_.max.x - bbox_.min.x) / 4;
      double dy = (bbox_.max.y - bbox_.min.y) / 4;
      double dz = (bbox_.max.z - bbox_.min.z) / 4;
      for (int i = 0; i < 256; ++i) {
        if (!children_[i]) {
          int a = (i >> 5) & 0x7;
          int b = (i >> 2) & 0x7;
          int c = (i >> 0) & 0x3;
          HOTBoundingBox child_box{
              {bbox_.min.x + a * dx, bbox_.min.y + b * dy, bbox_.min.z + c * dz},
              {bbox_.min.x + (a + 1) * dx, bbox_.min.y + (b + 1) * dy, bbox_.min.z + (c + 1) * dz}};
          children_[i].reset(new WideNode(child_box));
        }
        children_[i]->InsertItems(
            begin + buckets_[i], begin + buckets_[i + 1], items_begin_ + buckets_[i]);
      }
    }

    bool VisitNearVertices(SpatialSortTree::VertexVisitor* visitor,
        HOTPoint visitor_position, double eps2) {
      uint8_t key = ComputeWideKey(bbox_, visitor_position);
      WideNode* selected_child = children_[key].get();
      if (selected_child &&
          DistanceFromBoundary(selected_child->bbox_, visitor_position)) {
        return selected_child->VisitNearVertices(visitor, visitor_position, eps2);
      }
      bool leaf = true;
      for (int i = 0; i < 256; ++i) {
        if (children_[i]) {
          leaf = false;
          if (LInfinity(children_[i]->bbox_, visitor_position) < eps2) {
            if (!children_[i]->VisitNearVertices(
                  visitor, visitor_position, eps2)) {
              return false;
            }
          }
        }
      }
      if (!leaf) return true;
      int n = std::distance(items_begin_, items_end_);
      for (int i = 0; i < n; ++i) {
        if (LInfinity(items_begin_[i].position, visitor_position) < eps2) {
           bool cont = visitor->Visit(&items_begin_[i]);
           if (!cont) return false;
        }
      }
      return true;
    }

  private:
    HOTBoundingBox bbox_;
    std::unique_ptr<WideNode> children_[256];
    HOTItem* items_begin_;
    HOTItem* items_end_;
    int buckets_[257];
};

WideTreeParallel::WideTreeParallel(HOTBoundingBox bbox) : bbox_(bbox) {}
WideTreeParallel::WideTreeParallel(WideTreeParallel&&) = default;
WideTreeParallel& WideTreeParallel::operator=(WideTreeParallel&&) = default;
WideTreeParallel::~WideTreeParallel() = default;

void WideTreeParallel::InsertItems(const HOTItem* begin, const HOTItem* end) {
  int n = std::distance(begin, end);
  if (n == 0) return;
  items_.resize(n);
  if (!root_) {
    root_.reset(new WideNode(bbox_));
  }
  root_->InsertItems(begin, end, &items_[0]);
}

size_t WideTreeParallel::Size() const {
  return items_.size();
}

std::vector<HOTItem>::iterator WideTreeParallel::begin() {
  return items_.begin();
}

std::vector<HOTItem>::iterator WideTreeParallel::end() {
  return items_.end();
}

bool WideTreeParallel::VisitNearVertices(SpatialSortTree::VertexVisitor* visitor,
    HOTPoint position, double eps2) {
  if (root_) {
    return root_->VisitNearVertices(visitor, position, eps2);
  }
  return true;
}
