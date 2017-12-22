#include <widetree.h>
#include <test_utilities.h>
#include <vector>
#include <iostream>
#include <numeric>


template <typename T>
void ApplyPermutation(const int* perm, int n, const T* in, T* out) {
  for (int i = 0; i < n; ++i) {
    out[perm[i]] = in[i];
  }
}


int main(int, char **) {
  int n = 1000000;
  uint64_t start, end;

  HOTBoundingBox bbox = unit_cube();
  std::vector<Entity> entities(BuildEntitiesAtRandomLocations(bbox, n));
  std::vector<HOTItem> items(BuildItems(&entities));
  std::vector<uint8_t> keys(n);

  start = rdtsc();
  for (int i = 0; i < n; ++i) {
    keys[i] = ComputeWideKey(bbox, items[i].position);
  }
  end = rdtsc();
  std::cout << "Generate keys: " << (end - start) / 1.0e6 << std::endl;

  std::vector<int> perm(n);
  start = rdtsc();
  SortByKey(&keys[0], n, &perm[0]);
  end = rdtsc();
  std::cout << "SortByKey: " << (end - start) / 1.0e6 << std::endl;

  std::vector<HOTItem> sorted_items(n);
  start = rdtsc();
  ApplyPermutation(&perm[0], n, &items[0], &sorted_items[0]);
  end = rdtsc();
  std::cout << "ApplyPermutation: " << (end - start) / 1.0e6 << std::endl;
}

