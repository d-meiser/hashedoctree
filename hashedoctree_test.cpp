#include <gtest/gtest.h>
#include <hashedoctree.h>
#include <limits>
#include <random>


static double eps = std::numeric_limits<double>::epsilon();
static std::random_device rd;
static std::mt19937 gen(rd());


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

struct Entity {
  HOTPoint position;
  int id;
};

static std::vector<Entity> BuildEntitiesAtRandomLocations(
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

static std::vector<HOTItem> BuildItems(std::vector<Entity>* entities) {
  std::vector<HOTItem> items;
  int n = entities->size();
  items.reserve(n);
  for (int i = 0; i < n; ++i) {
    items.push_back(HOTItem{(*entities)[i].position, &(*entities)[i]});
  }
  return items;
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

static HOTTree ConstructTreeWithRandomItems(HOTBoundingBox bbox, int n) {
  assert(n > 0);
  HOTTree tree(bbox);
  auto entities = BuildEntitiesAtRandomLocations(bbox, n);
  auto items = BuildItems(&entities);
  tree.InsertItems(&items[0], &items[0] + n);
  return std::move(tree);
}

static const HOTBoundingBox unit_cube({{0, 0, 0}, {1, 1, 1}});

class CountVisits : public HOTTree::VertexVisitor {
  public:
    CountVisits() : count_{0} {}
    ~CountVisits() override {}
    bool Visit(HOTItem*) override {
      ++count_;
      return true;
    }
    void Reset() {
      count_ = 0;
    }
    int count_;
};

TEST(HOTTree, VertexVisitsSelf) {
  HOTTree tree = ConstructTreeWithRandomItems(unit_cube, 20);
  HOTItem first_item = *tree.begin();
  CountVisits counter;
  tree.VisitNearVertices(&counter, first_item.position, 1.0e-16);
  EXPECT_GT(counter.count_, 0);
}

TEST(HOTTree, VertexInNeighbouringNodeIsVisited) {
  HOTTree tree = ConstructTreeWithRandomItems(unit_cube, 100);
  double eps = 1.0e-10;
  CountVisits counter;

  std::vector<HOTItem> items(tree.begin(), tree.end());

  // Along x
  items[0].position = HOTPoint({0.5 - 0.5 * eps, 0.1, 0.1});
  items[1].position = HOTPoint({0.5 - 0.5 * eps, 0.1, 0.1});
  counter.Reset();
  HOTTree treex(unit_cube);
  treex.InsertItems(&items[0], &items[0] + items.size());
  treex.VisitNearVertices(&counter, items[0].position, eps);
  EXPECT_GT(counter.count_, 1);

  // Along y
  items[0].position = HOTPoint({0.1, 0.5 - 0.5 * eps, 0.1});
  items[1].position = HOTPoint({0.1, 0.5 + 0.49999 * eps, 0.1});
  counter.Reset();
  HOTTree treey(unit_cube);
  treey.InsertItems(&items[0], &items[0] + items.size());
  treey.VisitNearVertices(&counter, items[0].position, eps);
  EXPECT_GT(counter.count_, 1);

  // Along z
  items[0].position = HOTPoint({0.1, 0.1, 0.5 - 0.5 * eps});
  items[1].position = HOTPoint({0.1, 0.1, 0.5 + 0.49999 * eps});
  counter.Reset();
  HOTTree treez(unit_cube);
  treez.InsertItems(&items[0], &items[0] + items.size());
  treez.VisitNearVertices(&counter, items[0].position, eps);
  EXPECT_GT(counter.count_, 1);
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
