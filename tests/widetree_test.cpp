#include <gtest/gtest.h>
#include <widetree.h>
#include <test_utilities.h>


TEST(ComputeWideKey, DoesntCrash) {
  EXPECT_NO_THROW(ComputeWideKey(unit_cube(), {0.5, 0.5, 0.5}));
}

TEST(ComputeWideKey, WorksForOutOfBoundsCoords) {
  EXPECT_NO_THROW(ComputeWideKey(unit_cube(), {-0.5, 10.5, 0.5}));
}

TEST(WideTree, Ctor) {
  WideTree tree(unit_cube());
}

TEST(WideTree, InsertItemsDoesntThrow) {
  WideTree tree(unit_cube());
  EXPECT_NO_THROW(tree.InsertItems(nullptr, nullptr));
}

TEST(WideTree, SizeGrowsWhenInsertingItems) {
  HOTBoundingBox bbox({{0, 0, 0}, {1, 1, 1}});
  WideTree tree(bbox);
  size_t empty_size = tree.Size();
  int num_entities = 100;
  auto entities = BuildEntitiesAtRandomLocations(bbox, num_entities);
  auto items = BuildItems(&entities);
  tree.InsertItems(&items[0], &items[0] + num_entities);
  EXPECT_LT(empty_size, tree.Size());
}

static WideTree ConstructWideTreeWithRandomItems(HOTBoundingBox bbox, int n) {
  assert(n > 0);
  WideTree tree(bbox);
  auto entities = BuildEntitiesAtRandomLocations(bbox, n);
  auto items = BuildItems(&entities);
  tree.InsertItems(&items[0], &items[0] + n);
  return std::move(tree);
}

TEST(WideTree, VertexInNeighbouringNodeIsVisited) {
  WideTree tree = ConstructWideTreeWithRandomItems(unit_cube(), 100);
  double eps = 1.0e-10;

  std::vector<HOTItem> items(tree.begin(), tree.end());

  CountVisits counter(items[0].data);

  // Along x
  items[0].position = HOTPoint({0.5 - 0.5 * eps, 0.1, 0.1});
  items[1].position = HOTPoint({0.5 - 0.5 * eps, 0.1, 0.1});
  counter.Reset();
  WideTree treex(unit_cube());
  treex.InsertItems(&items[0], &items[0] + items.size());
  treex.VisitNearVertices(&counter, items[0].position, eps);
  EXPECT_GT(counter.count_, 0);

  // Along y
  items[0].position = HOTPoint({0.1, 0.5 - 0.5 * eps, 0.1});
  items[1].position = HOTPoint({0.1, 0.5 + 0.49999 * eps, 0.1});
  counter.Reset();
  WideTree treey(unit_cube());
  treey.InsertItems(&items[0], &items[0] + items.size());
  treey.VisitNearVertices(&counter, items[0].position, eps);
  EXPECT_GT(counter.count_, 0);

  // Along z
  items[0].position = HOTPoint({0.1, 0.1, 0.5 - 0.5 * eps});
  items[1].position = HOTPoint({0.1, 0.1, 0.5 + 0.49999 * eps});
  counter.Reset();
  WideTree treez(unit_cube());
  treez.InsertItems(&items[0], &items[0] + items.size());
  treez.VisitNearVertices(&counter, items[0].position, eps);
  EXPECT_GT(counter.count_, 0);
}

