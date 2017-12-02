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


TEST(HOTTree, InsertItems) {
  HOTTree tree({{0, 0, 0}, {1, 1, 1}});
  EXPECT_NO_THROW(tree.InsertItems(nullptr, nullptr));
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
