#include <hashedoctree.h>
#include <cmath>
#include <cassert>
#include <iostream>
#include <algorithm>
#include <numeric>


// We use 32 bit keys. That is large enough for 2**10 buckets
// along each dimension.
static const int BITS_PER_DIM = 10;
static const HOTKey NUM_LEAF_BUCKETS = 1u << BITS_PER_DIM;

static HOTKey ComputeBucket(double min, double max, double pos, HOTKey num_buckets) {
  assert(max > min);
  double folded_pos = std::fmod(pos - min, max - min);
  if (folded_pos < 0) {
    folded_pos += max - min;
  }
  int bucket = num_buckets * folded_pos / (max - min);
  assert(bucket >= 0);
  assert((uint32_t)bucket < num_buckets);
  return bucket;
}

static double DistanceFromInterval(double a, double b, double x) {
  assert(b >= a);
  double dist = 0;
  dist = std::max(dist, std::max(0.0, a - x));
  dist = std::max(dist, std::max(0.0, x - b));
  return dist;
}

static double LInfinity(const HOTBoundingBox& bbox, const HOTPoint& point) {
  double dist = 0;
  dist = std::max(dist, DistanceFromInterval(bbox.min.x, bbox.max.x, point.x));
  dist = std::max(dist, DistanceFromInterval(bbox.min.y, bbox.max.y, point.y));
  dist = std::max(dist, DistanceFromInterval(bbox.min.z, bbox.max.z, point.z));
  return dist;
}

static double LInfinity(const HOTPoint& p0, const HOTPoint& p1) {
  double dist = 0;
  dist = std::max(dist, std::fabs(p0.x - p1.x));
  dist = std::max(dist, std::fabs(p0.y - p1.y));
  dist = std::max(dist, std::fabs(p0.z - p1.z));
  return dist;
}


HOTKey HOTComputeHash(HOTBoundingBox bbox, HOTPoint point) {
  HOTKey buckets[3] = {0};
  buckets[0] = ComputeBucket(bbox.min.x, bbox.max.x, point.x, NUM_LEAF_BUCKETS);
  buckets[1] = ComputeBucket(bbox.min.y, bbox.max.y, point.y, NUM_LEAF_BUCKETS);
  buckets[2] = ComputeBucket(bbox.min.z, bbox.max.z, point.z, NUM_LEAF_BUCKETS);
  HOTKey key = 0;
  for (int i = 0; i < 3; ++i) {
    for (int b = 0; b < BITS_PER_DIM; ++b) {
      uint32_t mask = 1u << b;
      key |= ((uint32_t)((buckets[i] & mask) == mask)) << (3 * b + i);
    }
  }
  return key;
}

static std::vector<HOTKey> HOTComputeItemKeys(HOTBoundingBox bbox,
    const HOTItem* begin, const HOTItem* end) {
  int n = std::distance(begin, end);
  std::vector<HOTKey> keys(n);
  for (int i = 0; i < n; ++i) {
    keys[i] = HOTComputeHash(bbox, begin[i].position);
  }
  return keys;
}

static std::vector<int> find_sort_permutation(const std::vector<HOTKey>& keys) {
  std::vector<int> p(keys.size());
  std::iota(p.begin(), p.end(), 0);
  std::sort(p.begin(), p.end(),
      [&](int i, int j) { return keys[i] < keys[j] ; });
  return p;
}

template <typename T>
static std::vector<T> permute(const std::vector<int>& permutation,
    const std::vector<T>& v) {
  assert(permutation.size() == v.size());
  std::vector<T> permuted_v(v.size());
  std::transform(permutation.begin(), permutation.end(), permuted_v.begin(),
      [&](int i) { return v[i]; });
  return permuted_v;
}


HOTBoundingBox ComputeChildBox(HOTBoundingBox bbox, int octant) {
  double lx = 0.5 * (bbox.max.x - bbox.min.x);
  double ly = 0.5 * (bbox.max.y - bbox.min.y);
  double lz = 0.5 * (bbox.max.z - bbox.min.z);
  int i = (octant & (1 << 0)) == (1 << 0);
  int j = (octant & (1 << 1)) == (1 << 1);
  int k = (octant & (1 << 2)) == (1 << 2);
  return HOTBoundingBox({
      {bbox.min.x + i * lx, bbox.min.y + j * ly, bbox.min.z + k * lz},
      {bbox.min.x + (i + 1) * lx, bbox.min.y + (j + 1) * ly, bbox.min.z + (k + 1) * lz}
      });
}

class HOTNode {
  public:
    HOTNode(HOTNodeKey key, HOTBoundingBox bbox, const HOTKey* key_begin, const
        HOTKey* key_end, HOTItem* items_begin) :
      key_(key), bbox_(bbox), children_{nullptr},
      key_begin_(key_begin), key_end_(key_end), items_begin_(items_begin)
    {
      // Maximum number of items in leaf nodes. This can be a configurable
      // parameter, but for now I just hardwire it.
      static const int MAX_NUM_ITEMS = 32;
      static const int MAX_LEVELS = BITS_PER_DIM;
      if (HOTNodeLevel(key_) < MAX_LEVELS && NumItems() > MAX_NUM_ITEMS) {
        // Build the octants.
        HOTNodeKey child_keys[8];
        HOTNodeComputeChildKeys(key_, child_keys);
        for (int octant = 0; octant < 8; ++octant) {
          const HOTKey* begin = std::lower_bound(
              key_begin_, key_end_, HOTNodeBegin(child_keys[octant]));
          const HOTKey* end = std::lower_bound(
              key_begin_, key_end_, HOTNodeEnd(child_keys[octant]));
          int num_child_items = std::distance(begin, end);
          if (num_child_items > 0) {
            children_[octant].reset(
                new HOTNode(child_keys[octant], 
                  ComputeChildBox(bbox_, octant),
                  begin, end, items_begin_ + std::distance(key_begin_, begin)));
          } else {
            children_[octant].reset(nullptr);
          }
        }
      }
    }

    bool VisitNearVertices(
        HOTTree::VertexVisitor* visitor, HOTPoint position, double eps) {
      if (LInfinity(bbox_, position) >= eps) return true;
      bool leaf = true;
      for (int i = 0; i < 8; ++i) {
        if (children_[i]) {
          leaf = false;
          if (!children_[i]->VisitNearVertices(visitor, position, eps)) {
            return false;
          }
        }
      }
      if (!leaf) return true;
      int n = std::distance(key_begin_, key_end_);
      for (int i = 0; i < n; ++i) {
        if (LInfinity(items_begin_[i].position, position) < eps) {
           bool cont = visitor->Visit(&items_begin_[i]);
           if (!cont) return false;
        }
      }
      return true;
    }

    size_t NumItems() const {
      return std::distance(key_begin_, key_end_);
    }

    int NumNodes() const {
      int num_nodes = 1;
      for (int i = 0; i < 8; ++i) {
        if (children_[i]) {
          num_nodes += children_[i]->NumNodes();
        }
      }
      return num_nodes;
    }

    int Depth() const {
      int depth = 1;
      for (int i = 0; i < 8; ++i) {
        if (children_[i]) {
          depth = std::max(depth, 1 + children_[i]->Depth());
        }
      }
      return depth;
    }

    void PrintNumItems(int indent) const {
      HOTNodePrint(key_);
      std::cout << " ";
      for (int i = 0; i < indent; ++i) {
        std::cout << ".";
      }
      std::cout << " ";
      std::cout << NumItems() << "\n";
      for (int i = 0; i < 8; ++i) {
        if (children_[i]) {
          children_[i]->PrintNumItems(indent + 1);
        }
      }
    }

    size_t Size() const {
      size_t size = sizeof(*this);
      for (int i = 0; i < 8; ++i) {
        if (children_[i]) {
          size += children_[i]->Size();
        }
      }
      return size;
    }

  private:
    HOTNodeKey key_;
    HOTBoundingBox bbox_;
    std::unique_ptr<HOTNode> children_[8];

    const HOTKey* key_begin_;
    const HOTKey* key_end_;
    HOTItem* items_begin_;

    bool IsLeaf() const {
      for (int i = 0; i < 8; ++i) {
        if (children_[i]) return false;
      }
      return true;
    }
};


HOTTree::HOTTree(HOTBoundingBox bbox) : bbox_(bbox) {}
HOTTree::HOTTree(HOTTree&&) = default;
HOTTree& HOTTree::operator=(HOTTree&& rhs) = default;
HOTTree::~HOTTree() {}

void HOTTree::InsertItems(const HOTItem* begin, const HOTItem* end) {
  if (begin == end) return;

  std::vector<HOTItem> new_items(begin, end);
  std::vector<HOTKey> new_keys = HOTComputeItemKeys(bbox_, begin, end);

  // We now bring items and keys into the order defined by the hash. We first
  // find the permutation needed for the reordering and then we apply the
  // permutation out-of-place to items and keys. Note that there is potential
  // for reasonably efficient parallelization here. We can statically partition
  // the keys, sort each partition in parallel, and then merge.
  std::vector<int> sort_permutation = find_sort_permutation(new_keys);
  new_keys = permute(sort_permutation, new_keys);
  new_items = permute(sort_permutation, new_items);

  // TODO: Merge the new keys and items with keys and items we already have.
  // For now we just clobber the existing keys and items. That resets the tree
  // with each InsertItems.
  keys_ = new_keys;
  items_ = new_items;

  RebuildNodes();
}

bool HOTTree::VisitNearVertices(
    VertexVisitor* visitor, HOTPoint position, double eps) {
  if (root_) {
    return root_->VisitNearVertices(visitor, position, eps);
  }
  return true;
}

int HOTTree::NumNodes() const {
  if (root_) {
    return root_->NumNodes();
  } else {
    return 0;
  }
}

int HOTTree::Depth() const {
  if (root_) {
    return root_->Depth();
  } else {
    return 0;
  }
}

void HOTTree::PrintNumItems() const {
  if (root_) root_->PrintNumItems(0);
}

void HOTTree::RebuildNodes() {
  if (keys_.size() == 0) {
    root_.reset(nullptr);
    return;
  }

  root_.reset(new HOTNode(
        1, bbox_, &keys_[0], &keys_[0] + keys_.size(), &items_[0]));
}

size_t HOTTree::Size() const {
  size_t size = sizeof(*this);
  size += items_.size() * sizeof(HOTItem);
  size += keys_.size() * sizeof(HOTKey);
  if (root_) {
    size += root_->Size();
  }
  return size;
}

std::vector<HOTItem>::iterator HOTTree::begin() {
  return items_.begin();
}

std::vector<HOTItem>::iterator HOTTree::end() {
  return items_.end();
}

void HOTNodeComputeChildKeys(HOTNodeKey key, HOTNodeKey* child_keys) {
  HOTNodeKey first_child = key << 3;
  for (int i = 0; i < 8; ++i) {
    child_keys[i] = first_child + i;
  }
}

HOTNodeKey HOTNodeRoot() {
  return 1u;
}

bool HOTNodeValidKey(HOTNodeKey key) {
  if (key & (1u << (BITS_PER_DIM * 3 + 1))) return false;
  HOTNodeKey m = 1u << (BITS_PER_DIM * 3);
  while (m > 0) {
    if (key & m) return true;
    for (int i = 0; i < 3; ++i, m >>= 1) {
      if (key & m) return false;
    }
  }
  return false;
}

int HOTNodeLevel(HOTNodeKey key) {
  int level = BITS_PER_DIM;
  while (level > 0) {
    if (key & (1u << (level * 3))) return level;
    --level;
  }
  return level;
}

HOTNodeKey HOTNodeParent(HOTNodeKey key) {
  return key >> 3;
}

HOTKey HOTNodeBegin(HOTNodeKey key) {
  int level = HOTNodeLevel(key);
  HOTKey begin = key ^ (1u << (3 * level));
  begin <<= 3 * (BITS_PER_DIM - level);
  return begin;
}

HOTKey HOTNodeEnd(HOTNodeKey key) {
  int level = HOTNodeLevel(key);
  HOTKey end = key ^ (1u << (3 * level));
  ++end;
  end <<= 3 * (BITS_PER_DIM - level);
  return end;
}

void HOTNodePrint(HOTNodeKey key) {
  for (int i = 31; i >= 0; --i) {
    if (key & (1u << i)) {
      std::cout << "1";
    } else {
      std::cout << "0";
    }
  }
}
