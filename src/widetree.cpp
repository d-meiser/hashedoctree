#include <widetree.h>
#include <cassert>
#include <cmath>


static uint8_t ComputeBucket(double min, double max, double pos, int num_buckets) {
  assert(max > min);
  double folded_pos = std::fmod(pos - min, max - min);
  if (folded_pos < 0) {
    folded_pos += max - min;
  }
  uint8_t bucket = num_buckets * folded_pos / (max - min);
  assert(bucket < num_buckets);
  return bucket;
}

uint8_t ComputeWideKey(const HOTBoundingBox& bbox, HOTPoint point) {
  uint8_t a, b, c;
  a = ComputeBucket(bbox.min.x, bbox.max.x, point.x, 8);
  assert(a < 8);
  b = ComputeBucket(bbox.min.y, bbox.max.y, point.y, 8);
  assert(b < 8);
  c = ComputeBucket(bbox.min.z, bbox.max.z, point.z, 4);
  assert(c < 4);
  return (a << 5) + (b << 2) + (c << 0);
}

void ComputeManyWideKeys(const HOTBoundingBox& bbox, const double* locations,
    int n, int stride, uint8_t* keys) {
  double dx = (bbox.max.x - bbox.min.x) / 8;
  double dy = (bbox.max.y - bbox.min.y) / 8;
  double dz = (bbox.max.z - bbox.min.z) / 4;
  for (int i = 0; i < n; ++i) {
    uint8_t a, b, c;
    const double *l = locations + i * stride;
    a = (l[0] - bbox.min.x) / dx;
    b = (l[1] - bbox.min.y) / dy;
    c = (l[2] - bbox.min.z) / dz;
    keys[i] = (a << 5) + (b << 2) + (c << 0);
  }
}

void SortByKey(const uint8_t* keys, int n, int buckets[257], int* perm) {
  int my_buckets[256] = {0};
  for (int i = 0; i < n; ++i) {
    ++my_buckets[keys[i]];
  }
  for (int i = 1; i < 256; ++i) {
    my_buckets[i] += my_buckets[i - 1];
  }
  buckets[0] = 0;
  std::copy(my_buckets, my_buckets + 256, buckets + 1);
  for (int i = 0; i < n; ++i) {
    perm[--my_buckets[keys[i]]] = i;
  }
}

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

    bool VisitNearVertices(HOTTree::VertexVisitor* visitor,
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

WideTree::WideTree(HOTBoundingBox bbox) : bbox_(bbox) {}
WideTree::WideTree(WideTree&&) = default;
WideTree& WideTree::operator=(WideTree&&) = default;
WideTree::~WideTree() = default;

void WideTree::InsertItems(const HOTItem* begin, const HOTItem* end) {
  int n = std::distance(begin, end);
  if (n == 0) return;
  items_.resize(n);
  if (!root_) {
    root_.reset(new WideNode(bbox_));
  }
  root_->InsertItems(begin, end, &items_[0]);
}

size_t WideTree::Size() const {
  return items_.size();
}

std::vector<HOTItem>::iterator WideTree::begin() {
  return items_.begin();
}

std::vector<HOTItem>::iterator WideTree::end() {
  return items_.end();
}

bool WideTree::VisitNearVertices(HOTTree::VertexVisitor* visitor,
    HOTPoint position, double eps2) {
  if (root_) {
    return root_->VisitNearVertices(visitor, position, eps2);
  }
  return true;
}
