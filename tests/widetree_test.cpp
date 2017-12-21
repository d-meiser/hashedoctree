#include <gtest/gtest.h>
#include <widetree.h>
#include <test_utilities.h>


TEST(ComputeWideKey, DoesntCrash) {
  uint8_t key;
  EXPECT_NO_THROW(key = ComputeWideKey(unit_cube(), {0.5, 0.5, 0.5}));
}

TEST(ComputeWideKey, WorksForOutOfBoundsCoords) {
  uint8_t key;
  EXPECT_NO_THROW(key = ComputeWideKey(unit_cube(), {-0.5, 10.5, 0.5}));
}
