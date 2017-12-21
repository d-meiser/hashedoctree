#include <widetree.h>
#include <cassert>
#include <cmath>


static int ComputeBucket(double min, double max, double pos, int num_buckets) {
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

uint8_t ComputeWideKey(HOTBoundingBox bbox, HOTPoint point) {
  int a, b, c;
  a = ComputeBucket(bbox.min.x, bbox.max.x, point.x, 8);
  assert(a >= 0);
  assert(a < 8);
  b = ComputeBucket(bbox.min.y, bbox.max.y, point.y, 8);
  assert(b >= 0);
  assert(b < 8);
  c = ComputeBucket(bbox.min.z, bbox.max.z, point.z, 4);
  assert(c >= 0);
  assert(c < 4);
  return (a << 5) + (b << 2) + (c << 0);
}

void SortByKey(
    const uint8_t* key_begin, const uint8_t* key_end,
    const int* perm_in,
    int* perm_out) {
  int buckets[256] = {0};
  for (const uint8_t* k = key_begin; k != key_end; ++k) {
    ++buckets[*k];
  }
  for (int i = 1; i < 256; ++i) {
    buckets[i] += buckets[i - 1];
  }
  for (const uint8_t* k = key_begin; k != key_end; ++k, ++perm_in) {
    perm_out[--buckets[*k]] = *perm_in;
  }
}
