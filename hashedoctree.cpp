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
  assert(bucket < num_buckets);
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

HOTTree::HOTTree(HOTBoundingBox bbox) : bbox_(bbox) {}

void HOTTree::InsertItems(const HOTItem* begin, const HOTItem* end) {
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


}
