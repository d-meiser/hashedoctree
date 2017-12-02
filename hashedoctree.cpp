#include <hashedoctree.h>
#include <cmath>
#include <cassert>
#include <iostream>
#include <algorithm>
#include <numeric>


// We use 32 bit keys. That is large enough for 2**10 buckets
// along each dimension.
static const int BITS_PER_DIM = 10;
static const HOTKey NUM_LEAF_BUCKETS = 1 << BITS_PER_DIM;

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


HOTKey HOTComputeHash(HOTBoundingBox bbox, HOTPoint point) {
  int buckets[3] = {0};
  buckets[0] = ComputeBucket(bbox.min.x, bbox.max.x, point.x, NUM_LEAF_BUCKETS);
  buckets[1] = ComputeBucket(bbox.min.y, bbox.max.y, point.y, NUM_LEAF_BUCKETS);
  buckets[2] = ComputeBucket(bbox.min.z, bbox.max.z, point.z, NUM_LEAF_BUCKETS);
  HOTKey key = 0;
  for (int i = 0; i < 3; ++i) {
    for (int b = 0; b < BITS_PER_DIM; ++b) {
      key |= (buckets[i] & (1 << b)) << i;
    }
  }
  return key;
}

static std::vector<HOTKey> HOTComputeItemKeys(HOTBoundingBox bbox,
    const HOTItem* begin, const HOTItem* end) {
  std::vector<HOTKey> keys;
  std::transform(begin, end, std::back_inserter(keys),
      [bbox](const HOTItem& item) {
        return HOTComputeHash(bbox, item.position);
      });
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



class HOTNode {
  public:
    HOTNode(HOTNodeKey key, HOTBoundingBox bbox, const HOTKey* key_begin, const
        HOTKey* key_end, const HOTItem* items_begin) :
      key_(key), bbox_(bbox), leaf_(true), children_{nullptr},
      key_begin_(key_begin), key_end_(key_end), items_begin_(items_begin)
    {
      // Maximum number of items in leaf nodes. This can be a configurable
      // parameter, but for now I just hardwire it.
      static const int MAX_NUM_ITEMS = 8;
      if (NumItems() > MAX_NUM_ITEMS) {
        leaf_ = false;
        // Build the octants.
        HOTNodeKey child_keys[8];
        HOTNodeComputeChildKeys(key_, child_keys);
        for (int octant = 0; octant < 8; ++octant) {
        }
      }
    }

    size_t NumItems() const {
      return std::distance(key_begin_, key_end_);
    }

    HOTNodeKey key_;
    HOTBoundingBox bbox_;
    bool leaf_;
    HOTNode* children_[8];

    const HOTKey* key_begin_;
    const HOTKey* key_end_;
    const HOTItem* items_begin_;
};


HOTTree::HOTTree(HOTBoundingBox bbox) : bbox_(bbox) {}
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

  RebuildNodes();
}


void HOTTree::RebuildNodes() {
  if (keys_.size() == 0) {
    root_.reset(nullptr);
    return;
  }

  root_.reset(new HOTNode(
        1, bbox_, &keys_[0], &keys_[0] + keys_.size(), &items_[0]));
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
