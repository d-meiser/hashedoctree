#include <widetree.h>
#include <test_utilities.h>
#include <vector>
#include <iostream>
#include <numeric>


int main(int, char **) {
  int n = 100000;
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
  std::iota(perm.begin(), perm.end(), 0);

}

