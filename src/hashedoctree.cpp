#include <hashedoctree.h>
#include <helpers.h>
#include <cmath>
#include <cassert>
#include <iostream>
#include <algorithm>
#include <numeric>
#include <tbb/parallel_sort.h>


static void HOTNodeComputePartitionPointers(
    const HOTKey* key_begin, const HOTKey* key_end, const HOTNodeKey* child_keys,
    const HOTKey** partition_ptrs);

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


static uint32_t Part1By2_32(uint32_t a) {
  a &= 0x000003ff;                  // a = ---- ---- ---- ---- ---- --98 7654 3210
  a = (a ^ (a << 16)) & 0xff0000ff; // a = ---- --98 ---- ---- ---- ---- 7654 3210
  a = (a ^ (a <<  8)) & 0x0300f00f; // a = ---- --98 ---- ---- 7654 ---- ---- 3210
  a = (a ^ (a <<  4)) & 0x030c30c3; // a = ---- --98 ---- 76-- --54 ---- 32-- --10
  a = (a ^ (a <<  2)) & 0x09249249; // a = ---- 9--8 --7- -6-- 5--4 --3- -2-- 1--0
  return a;
}

static uint32_t MortonEncode_32(uint32_t a, uint32_t b, uint32_t c) {
  return Part1By2_32(a) + (Part1By2_32(b) << 1) + (Part1By2_32(c) << 2);
}

#if defined(USE_64BIT_KEYS)
// Define these only for 64 bit keys to avoid unused static function warning.
static uint64_t Part1By2_64(uint32_t x) {
  uint64_t a = x & 0x1fffff; 
  a = (a | a << 32) & 0x1f00000000ffff;
  a = (a | a << 16) & 0x1f0000ff0000ff;
  a = (a | a << 8) & 0x100f00f00f00f00f;
  a = (a | a << 4) & 0x10c30c30c30c30c3;
  a = (a | a << 2) & 0x1249249249249249;
  return a;
}

static uint64_t MortonEncode_64(uint32_t a, uint32_t b, uint32_t c) {
  return Part1By2_64(a) + (Part1By2_64(b) << 1) + (Part1By2_64(c) << 2);
}
#endif

HOTKey HOTComputeHash(HOTBoundingBox bbox, HOTPoint point) {
  HOTKey a, b, c;
  a = ComputeBucket(bbox.min.x, bbox.max.x, point.x, NUM_LEAF_BUCKETS);
  b = ComputeBucket(bbox.min.y, bbox.max.y, point.y, NUM_LEAF_BUCKETS);
  c = ComputeBucket(bbox.min.z, bbox.max.z, point.z, NUM_LEAF_BUCKETS);
  return MortonEncode_32(a, b, c);
}

static std::vector<HOTKey> HOTComputeItemKeys(HOTBoundingBox bbox,
    const HOTItem* begin, const HOTItem* end) {
  int n = std::distance(begin, end);
  std::vector<HOTKey> keys(n);
  tbb::parallel_for(tbb::blocked_range<int>(0, n, 1<<10),
      [&](const tbb::blocked_range<int>& range) {
          for (int i = range.begin(); i != range.end(); ++i) {
            keys[i] = HOTComputeHash(bbox, begin[i].position);
          }
        },
      tbb::static_partitioner());
  return keys;
}

static std::vector<int> find_sort_permutation(const std::vector<HOTKey>& keys) {
  std::vector<int> p(keys.size());
  std::iota(p.begin(), p.end(), 0);
  tbb::parallel_sort(p.begin(), p.end(),
      [&](int i, int j) { return keys[i] < keys[j] ; });
  return p;
}

template <typename T>
static std::vector<T> permute(const std::vector<int>& permutation,
    const std::vector<T>& v) {
  assert(permutation.size() == v.size());
  std::vector<T> permuted_v(v.size());
  tbb::parallel_for(tbb::blocked_range<int>(0, v.size(), 1<<10),
      [&](const tbb::blocked_range<int>& range) {
          for (int i = range.begin(); i != range.end(); ++i) {
            permuted_v[i] = v[permutation[i]];
          }
        },
      tbb::static_partitioner());
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

// Convert a triple of binary digits into an integer.
// i, j, and k should be either 0 or 1.
static int from_binary_digits(int i, int j, int k) {
  return (i << 2) + (j << 1) + (k << 0);
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
        const HOTKey* partition_ptrs[9];
        HOTNodeComputePartitionPointers(key_begin, key_end, child_keys, partition_ptrs);
        for (int octant = 0; octant < 8; ++octant) {
          const HOTKey* begin = partition_ptrs[octant];
          const HOTKey* end = partition_ptrs[octant + 1];
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
        HOTTree::VertexVisitor* visitor,
        HOTKey visitor_key,
        HOTPoint visitor_position,
        double eps) {
      int my_level = HOTNodeLevel(key_);
      int visitor_octant = (visitor_key >> (3 * (BITS_PER_DIM - (my_level + 1)))) & 0x07u;
      HOTNode* selected_child = children_[visitor_octant].get();
      if (selected_child &&
          DistanceFromBoundary(selected_child->bbox_, visitor_position) > eps) {
        // Most common case: We need to recurse and the item is not near the
        // surface of the child node.
        return selected_child->VisitNearVertices(
            visitor, visitor_key, visitor_position, eps);
      }
      // Otherwise this is either a leaf node or we are near the boundary.
      bool leaf = true;
      for (int i = 0; i < 8; ++i) {
        if (children_[i]) {
          leaf = false;
          if (LInfinity(children_[i]->bbox_, visitor_position) < eps) {
            if (!children_[i]->VisitNearVertices(
                  visitor, visitor_key, visitor_position, eps)) {
              return false;
            }
          }
        }
      }
      if (!leaf) return true;
      int n = std::distance(key_begin_, key_end_);
      for (int i = 0; i < n; ++i) {
        if (LInfinity(items_begin_[i].position, visitor_position) < eps) {
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
    HOTKey visitor_key = HOTComputeHash(bbox_, position);
    if (LInfinity(bbox_, position) < eps) {
      return root_->VisitNearVertices(visitor, visitor_key, position, eps);
    }
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

void HOTNodeComputePartitionPointers(
    const HOTKey* key_begin, const HOTKey* key_end, const HOTNodeKey* child_keys,
    const HOTKey** partition_ptrs) {
  partition_ptrs[0] = key_begin;
  partition_ptrs[8] = key_end;
  int partition_index;
  partition_index = from_binary_digits(1, 0, 0);
  partition_ptrs[partition_index] =
    std::lower_bound(partition_ptrs[0], partition_ptrs[8],
        HOTNodeBegin(child_keys[partition_index]));
  partition_index = from_binary_digits(0, 1, 0);
  partition_ptrs[partition_index] =
    std::lower_bound(partition_ptrs[0], partition_ptrs[4],
        HOTNodeBegin(child_keys[partition_index]));
  partition_index = from_binary_digits(1, 1, 0);
  partition_ptrs[partition_index] =
    std::lower_bound(partition_ptrs[4], partition_ptrs[8],
        HOTNodeBegin(child_keys[partition_index]));
  partition_index = from_binary_digits(0, 0, 1);
  partition_ptrs[partition_index] =
    std::lower_bound(partition_ptrs[0], partition_ptrs[2],
        HOTNodeBegin(child_keys[partition_index]));
  partition_index = from_binary_digits(0, 1, 1);
  partition_ptrs[partition_index] =
    std::lower_bound(partition_ptrs[2], partition_ptrs[4],
        HOTNodeBegin(child_keys[partition_index]));
  partition_index = from_binary_digits(1, 0, 1);
  partition_ptrs[partition_index] =
    std::lower_bound(partition_ptrs[4], partition_ptrs[6],
        HOTNodeBegin(child_keys[partition_index]));
  partition_index = from_binary_digits(1, 1, 1);
  partition_ptrs[partition_index] =
    std::lower_bound(partition_ptrs[6], partition_ptrs[8],
        HOTNodeBegin(child_keys[partition_index]));
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
