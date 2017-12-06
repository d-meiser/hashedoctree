#include <test_utilities.h>
#include <random>
#include <cassert>

namespace {
std::random_device rd;
std::mt19937 gen(rd());
}


std::vector<Entity> BuildEntitiesAtRandomLocations(
    HOTBoundingBox bbox, int n) {
  std::vector<Entity> entities;
  entities.reserve(n);
  std::uniform_real_distribution<> dist_x(bbox.min.x, bbox.max.x);
  std::uniform_real_distribution<> dist_y(bbox.min.y, bbox.max.y);
  std::uniform_real_distribution<> dist_z(bbox.min.z, bbox.max.z);
  for (int i = 0; i < n; ++i) {
    entities.emplace_back(Entity{{dist_x(gen), dist_y(gen), dist_z(gen)}, i});
  }
  return entities;
}

std::vector<HOTItem> BuildItems(std::vector<Entity>* entities) {
  std::vector<HOTItem> items;
  int n = entities->size();
  items.reserve(n);
  for (int i = 0; i < n; ++i) {
    items.push_back(HOTItem{(*entities)[i].position, &(*entities)[i]});
  }
  return items;
}

HOTBoundingBox unit_cube() {
  return HOTBoundingBox({{0, 0, 0}, {1, 1, 1}});
}

HOTTree ConstructTreeWithRandomItems(HOTBoundingBox bbox, int n) {
  assert(n > 0);
  HOTTree tree(bbox);
  auto entities = BuildEntitiesAtRandomLocations(bbox, n);
  auto items = BuildItems(&entities);
  tree.InsertItems(&items[0], &items[0] + n);
  return std::move(tree);
}

