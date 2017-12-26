#include <widetree.h>
#include <test_utilities.h>
#include <vector>
#include <iostream>
#include <numeric>


int main(int, char **) {
  int n = 1000000;
  int buckets[257];
  uint64_t start, end;

  HOTBoundingBox bbox = unit_cube();
  std::vector<Entity> entities(BuildEntitiesAtRandomLocations(bbox, n));
  std::vector<HOTItem> items(BuildItems(&entities));
  std::vector<uint8_t> keys(n);

  start = rdtsc();
  ComputeManyWideKeys(bbox, &items[0].position.x, n,  sizeof(items[0]) / sizeof(double), &keys[0]);
  end = rdtsc();
  std::cout << "Generate keys: " << (end - start) / 1.0e6 << std::endl;

  std::vector<int> perm(n);
  start = rdtsc();
  SortByKey(&keys[0], n, buckets, &perm[0]);
  end = rdtsc();
  std::cout << "SortByKey: " << (end - start) / 1.0e6 << std::endl;

  std::vector<HOTItem> sorted_items(n);
  start = rdtsc();
  ApplyPermutation(&perm[0], n, &items[0], &sorted_items[0]);
  end = rdtsc();
  std::cout << "ApplyPermutation: " << (end - start) / 1.0e6 << std::endl;

  ComputeManyWideKeys(bbox, &sorted_items[0].position.x, n,  sizeof(sorted_items[0]) / sizeof(double), &keys[0]);
  start = rdtsc();
  SortByKey(&keys[0], n, buckets, &perm[0]);
  end = rdtsc();
  std::cout << "SortByKey (pre sorted): " << (end - start) / 1.0e6 << std::endl;

  start = rdtsc();
  // Recycle items
  ApplyPermutation(&perm[0], n, &sorted_items[0], &items[0]);
  end = rdtsc();
  std::cout << "ApplyPermutation (pre sorted): " << (end - start) / 1.0e6 << std::endl;
}

