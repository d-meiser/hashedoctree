#include <gtest/gtest.h>
#include <hashedoctree.h>
#include <limits>


static double eps = std::numeric_limits<double>::epsilon();


TEST(ComputeHash, IsNullAtOrigin) {
	HOTBoundingBox bbox{{0, 0, 0}, {1, 1, 1}};
	HOTKey k = HOTComputeHash(bbox, {eps, eps, eps});
	EXPECT_EQ(0, k);
}

TEST(ComputeHash, IsNotNullAwayFromOrigin) {
	HOTBoundingBox bbox{{0, 0, 0}, {1, 1, 1}};
	HOTKey k = HOTComputeHash(bbox, {0.5, 0.5, 0.5});
	EXPECT_NE(0, k);
}

TEST(ComputeHash, SubstantiallyDifferentPointsYieldDifferentKeys) {
	HOTBoundingBox bbox{{0, 0, 0}, {1, 1, 1}};
	HOTKey k1 = HOTComputeHash(bbox, {0.5, 0.5, 0.5});
	HOTKey k2 = HOTComputeHash(bbox, {0.6, 0.6, 0.6});
	EXPECT_NE(k1, k2);
}

TEST(ComputeHash, CanComputeKeysOutsideOfBBox) {
	HOTBoundingBox bbox{{0, 0, 0}, {1, 1, 1}};
	EXPECT_GE(HOTComputeHash(bbox, {1.5, 1.5, 1.5}), 0);
	EXPECT_GE(HOTComputeHash(bbox, {-1.5, -1.5, -1.5}), 0);
	EXPECT_GE(HOTComputeHash(bbox, {-1.5, 1.5, -1.5}), 0);
}

