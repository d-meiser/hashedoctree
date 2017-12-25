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

void SortByKey(const uint8_t* keys, int n, int* perm) {
  int buckets[256] = {0};
  for (int i = 0; i < n; ++i) {
    ++buckets[keys[i]];
  }
  for (int i = 1; i < 256; ++i) {
    buckets[i] += buckets[i - 1];
  }
  for (int i = 0; i < n; ++i) {
    perm[--buckets[keys[i]]] = i;
  }
}
