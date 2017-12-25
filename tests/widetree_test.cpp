#include <gtest/gtest.h>
#include <widetree.h>
#include <test_utilities.h>


TEST(ComputeWideKey, DoesntCrash) {
  EXPECT_NO_THROW(ComputeWideKey(unit_cube(), {0.5, 0.5, 0.5}));
}

TEST(ComputeWideKey, WorksForOutOfBoundsCoords) {
  EXPECT_NO_THROW(ComputeWideKey(unit_cube(), {-0.5, 10.5, 0.5}));
}
