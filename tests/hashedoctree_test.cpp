#include <gtest/gtest.h>
#include <hashedoctree.h>
#include <test_utilities.h>
#include <limits>

#include <hot_config.h>
#ifdef HOT_HAVE_TBB
#include <tbb/task_scheduler_init.h>
#endif


static double eps = std::numeric_limits<double>::epsilon();


TEST(ComputeHash, IsNullAtOrigin) {
  HOTBoundingBox bbox{{0, 0, 0}, {1, 1, 1}};
  HOTKey k = HOTComputeHash(bbox, {eps, eps, eps});
  EXPECT_EQ(0u, k);
}

TEST(ComputeHash, IsNotNullAwayFromOrigin) {
  HOTBoundingBox bbox{{0, 0, 0}, {1, 1, 1}};
  HOTKey k = HOTComputeHash(bbox, {0.5, 0.5, 0.5});
  EXPECT_NE(0u, k);
}

TEST(ComputeHash, SubstantiallyDifferentPointsYieldDifferentKeys) {
  HOTBoundingBox bbox{{0, 0, 0}, {1, 1, 1}};
  HOTKey k1 = HOTComputeHash(bbox, {0.5, 0.5, 0.5});
  HOTKey k2 = HOTComputeHash(bbox, {0.6, 0.6, 0.6});
  EXPECT_NE(k1, k2);
}

TEST(ComputeHash, CanComputeKeysOutsideOfBBox) {
  HOTBoundingBox bbox{{0, 0, 0}, {1, 1, 1}};
  EXPECT_GE(HOTComputeHash(bbox, {1.5, 1.5, 1.5}), 0u);
  EXPECT_GE(HOTComputeHash(bbox, {-1.5, -1.5, -1.5}), 0u);
  EXPECT_GE(HOTComputeHash(bbox, {-1.5, 1.5, -1.5}), 0u);
}


TEST(HOTTree, Ctor) {
  EXPECT_NO_THROW(HOTTree({{0, 0, 0}, {1, 1, 1}}));
}

TEST(HOTTree, InsertItemsDoesntThrow) {
  HOTTree tree({{0, 0, 0}, {1, 1, 1}});
  EXPECT_NO_THROW(tree.InsertItems(nullptr, nullptr));
}

TEST(HOTTree, EmptyTreeHasNonZeroSize) {
  HOTBoundingBox bbox({{0, 0, 0}, {1, 1, 1}});
  HOTTree tree(bbox);
  EXPECT_LT(0ull, tree.Size());
}

TEST(HOTTree, SizeGrowsWhenInsertingItems) {
  HOTBoundingBox bbox({{0, 0, 0}, {1, 1, 1}});
  HOTTree tree(bbox);
  size_t empty_size = tree.Size();
  int num_entities = 100;
  auto entities = BuildEntitiesAtRandomLocations(bbox, num_entities);
  auto items = BuildItems(&entities);
  tree.InsertItems(&items[0], &items[0] + num_entities);
  EXPECT_LT(empty_size, tree.Size());
}

TEST(HOTTree, OneItemYieldsOneNode) {
  HOTBoundingBox bbox({{0, 0, 0}, {1, 1, 1}});
  HOTTree tree(bbox);
  int num_entities = 1;
  auto entities = BuildEntitiesAtRandomLocations(bbox, num_entities);
  auto items = BuildItems(&entities);
  tree.InsertItems(&items[0], &items[0] + num_entities);
  EXPECT_EQ(1, tree.NumNodes());
  EXPECT_EQ(1, tree.Depth());
}

TEST(HOTTree, CanInsertAFewItems) {
  HOTBoundingBox bbox({{0, 0, 0}, {1, 1, 1}});
  HOTTree tree(bbox);
  int num_entities = 100;
  auto entities = BuildEntitiesAtRandomLocations(bbox, num_entities);
  auto items = BuildItems(&entities);
  tree.InsertItems(&items[0], &items[0] + num_entities);
  // With the default configuration these items don't fit into
  // a single node. So it's save to assert that we have more than
  // one node which then necessarily is in a tree with more than one level.
  EXPECT_LT(1, tree.NumNodes());
  EXPECT_LT(1, tree.Depth());
}



TEST(HOTTree, VertexInNeighbouringNodeIsVisited) {
  HOTTree tree = ConstructTreeWithRandomItems(unit_cube(), 100);
  double eps = 1.0e-10;

  std::vector<HOTItem> items(tree.begin(), tree.end());

  CountVisits counter(items[0].data);

  // Along x
  items[0].position = HOTPoint({0.5 - 0.5 * eps, 0.1, 0.1});
  items[1].position = HOTPoint({0.5 - 0.5 * eps, 0.1, 0.1});
  counter.Reset();
  HOTTree treex(unit_cube());
  treex.InsertItems(&items[0], &items[0] + items.size());
  treex.VisitNearVertices(&counter, items[0].position, eps);
  EXPECT_GT(counter.count_, 0);

  // Along y
  items[0].position = HOTPoint({0.1, 0.5 - 0.5 * eps, 0.1});
  items[1].position = HOTPoint({0.1, 0.5 + 0.49999 * eps, 0.1});
  counter.Reset();
  HOTTree treey(unit_cube());
  treey.InsertItems(&items[0], &items[0] + items.size());
  treey.VisitNearVertices(&counter, items[0].position, eps);
  EXPECT_GT(counter.count_, 0);

  // Along z
  items[0].position = HOTPoint({0.1, 0.1, 0.5 - 0.5 * eps});
  items[1].position = HOTPoint({0.1, 0.1, 0.5 + 0.49999 * eps});
  counter.Reset();
  HOTTree treez(unit_cube());
  treez.InsertItems(&items[0], &items[0] + items.size());
  treez.VisitNearVertices(&counter, items[0].position, eps);
  EXPECT_GT(counter.count_, 0);
}

TEST(HOTTree, DuplicatesAreVisited) {
  std::vector<Entity> entities = BuildEntitiesAtRandomLocations(unit_cube(), 100);
  std::vector<HOTItem> items(BuildItems(&entities));
  items[4].position = items[0].position;
  items[11].position = items[0].position;
  items[13].position = items[3].position;
  HOTTree tree(unit_cube());
  RecordIdsVisitor visitor;
  tree.InsertItems(&items[0], &items[0] + 100);
  tree.VisitNearVertices(&visitor, items[0].position, 1.0e-10);
  EXPECT_TRUE(visitor.EntityVisited(entities[0].id));
  EXPECT_TRUE(visitor.EntityVisited(entities[4].id));
  EXPECT_TRUE(visitor.EntityVisited(entities[11].id));
  EXPECT_FALSE(visitor.EntityVisited(entities[13].id));
}

TEST(HOTTree, EightCornerVerticesAreAllVisited) {
  double eps = 1.0e-10;
  std::vector<Entity> entities = BuildEntitiesAtRandomLocations(unit_cube(), 100);
  std::vector<HOTItem> items(BuildItems(&entities));
  int m = 0;
  for (int i = 0; i < 2; ++i) {
    for (int j = 0; j < 2; ++j) {
      for (int k = 0; k < 2; ++k) {
        items[m].position = HOTPoint({
          0.5 + 0.1 * (i - 0.5) * eps,
          0.5 + 0.1 * (j - 0.5) * eps,
          0.5 + 0.1 * (j - 0.5) * eps});
        ++m;
      }
    }
  }
  HOTTree tree(unit_cube());
  RecordIdsVisitor visitor;
  tree.InsertItems(&items[0], &items[0] + 100);
  tree.VisitNearVertices(&visitor, items[0].position, eps);
  for (int i = 0; i < 8; ++i) {
    EXPECT_TRUE(visitor.EntityVisited(entities[i].id)) << i;
  }
}

TEST(HOTTree, VisitEachVerticesNeighbours) {
  int n = 1000;
  HOTTree tree = ConstructTreeWithRandomItems(unit_cube(), n);
  double eps = 1.0e-3;
  CountVisits counter(nullptr);
  auto item = tree.begin();
  for (int i = 0; i < n; ++i) {
    counter.data_ = item[i].data;
    tree.VisitNearVertices(&counter, item[i].position, eps);
  }
  EXPECT_GE(counter.count_, 0);
}


TEST(HOTNodeKey, ZeroIsNotValidNode) {
  EXPECT_FALSE(HOTNodeValidKey(0u));
}

TEST(HOTNodeKey, RootNodeIsValid) {
  EXPECT_TRUE(HOTNodeValidKey(HOTNodeRoot()));
}

TEST(HOTNodeKey, RootNodeIsLevelZero) {
  EXPECT_EQ(0, HOTNodeLevel(HOTNodeRoot()));
}

TEST(HOTNodeKey, ChildrenOfRootAreAtLevelOne) {
  HOTNodeKey child_keys[8];
  HOTNodeComputeChildKeys(HOTNodeRoot(), child_keys);
  EXPECT_EQ(1, HOTNodeLevel(child_keys[0]));
}

TEST(HOTNodeKey, ParentIsAtLowerLevel) {
  HOTNodeKey key = 64u;
  ASSERT_TRUE(HOTNodeValidKey(key));
  EXPECT_LT(HOTNodeLevel(HOTNodeParent(key)), HOTNodeLevel(key));
}

TEST(HOTNodeKey, BeginOfRootIsZero) {
  EXPECT_EQ(0u, HOTNodeBegin(HOTNodeRoot()));
}

TEST(HOTNodeKey, EndOfRootIsTwoToTheThirty) {
  EXPECT_EQ(1u << (3 * 10), HOTNodeEnd(HOTNodeRoot()));
}

TEST(HOTNodeKey, BeginSpotChecks) {
  EXPECT_EQ(1u << (3 * 9), HOTNodeBegin(9u));
  EXPECT_EQ(2u << (3 * 9), HOTNodeBegin(10u));
  EXPECT_EQ(3u << (3 * 9), HOTNodeBegin(11u));
  EXPECT_EQ(1u << (3 * 8), HOTNodeBegin(65u));
  EXPECT_EQ(10u << (3 * 8), HOTNodeBegin(74u));
}


int main(int argn, char **argv) {
  ::testing::InitGoogleTest(&argn, argv);
#ifdef HOT_HAVE_TBB
  tbb::task_scheduler_init scheduler(2);
#endif
  int result = RUN_ALL_TESTS();
#ifdef HOT_HAVE_TBB
  scheduler.terminate();
#endif
  return result;
}
