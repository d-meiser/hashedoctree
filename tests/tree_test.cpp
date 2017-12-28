#include <gtest/gtest.h>
#include <spatialsorttree.h>
#include <hashedoctree.h>
#include <widetree.h>
#include <test_utilities.h>

#include <hot_config.h>
#ifdef HOT_HAVE_TBB
#include <tbb/task_scheduler_init.h>
#include <hashedoctreeparallel.h>
#include <widetreeparallel.h>
#endif


struct SpatialSortTreeFixture : public ::testing::TestWithParam<SpatialSortTree*> {
  void TearDown() {
    delete GetParam();
  }
};

TEST_P(SpatialSortTreeFixture, VertexInNeighbouringNodeIsVisitedX) {
  std::vector<Entity> entities = BuildEntitiesAtRandomLocations(unit_cube(), 100);
  std::vector<HOTItem> items(BuildItems(&entities));
  double eps = 1.0e-10;

  CountVisits counter(items[0].data);

  items[0].position = HOTPoint({0.5 - 0.5 * eps, 0.1, 0.1});
  items[1].position = HOTPoint({0.5 - 0.5 * eps, 0.1, 0.1});
  counter.Reset();
  SpatialSortTree* tree = GetParam();
  tree->InsertItems(&items[0], &items[0] + items.size());
  tree->VisitNearVertices(&counter, items[0].position, eps);
  EXPECT_GT(counter.count_, 0);
}

TEST_P(SpatialSortTreeFixture, VertexInNeighbouringNodeIsVisitedY) {
  std::vector<Entity> entities = BuildEntitiesAtRandomLocations(unit_cube(), 100);
  std::vector<HOTItem> items(BuildItems(&entities));
  double eps = 1.0e-10;

  CountVisits counter(items[0].data);

  items[0].position = HOTPoint({0.1, 0.5 - 0.5 * eps, 0.1});
  items[1].position = HOTPoint({0.1, 0.5 - 0.5 * eps, 0.1});
  counter.Reset();
  SpatialSortTree* tree = GetParam();
  tree->InsertItems(&items[0], &items[0] + items.size());
  tree->VisitNearVertices(&counter, items[0].position, eps);
  EXPECT_GT(counter.count_, 0);
}


TEST_P(SpatialSortTreeFixture, VertexInNeighbouringNodeIsVisitedZ) {
  std::vector<Entity> entities = BuildEntitiesAtRandomLocations(unit_cube(), 100);
  std::vector<HOTItem> items(BuildItems(&entities));
  double eps = 1.0e-10;

  CountVisits counter(items[0].data);

  items[0].position = HOTPoint({0.1, 0.1, 0.5 - 0.5 * eps});
  items[1].position = HOTPoint({0.1, 0.1,0.5 - 0.5 * eps});
  counter.Reset();
  SpatialSortTree* tree = GetParam();
  tree->InsertItems(&items[0], &items[0] + items.size());
  tree->VisitNearVertices(&counter, items[0].position, eps);
  EXPECT_GT(counter.count_, 0);
}

TEST_P(SpatialSortTreeFixture, DuplicatesAreVisited) {
  std::vector<Entity> entities = BuildEntitiesAtRandomLocations(unit_cube(), 100);
  std::vector<HOTItem> items(BuildItems(&entities));
  items[4].position = items[0].position;
  items[11].position = items[0].position;
  items[13].position = items[3].position;
  RecordIdsVisitor visitor;
  SpatialSortTree* tree = GetParam();
  tree->InsertItems(&items[0], &items[0] + 100);
  tree->VisitNearVertices(&visitor, items[0].position, 1.0e-10);
  EXPECT_TRUE(visitor.EntityVisited(entities[0].id));
  EXPECT_TRUE(visitor.EntityVisited(entities[4].id));
  EXPECT_TRUE(visitor.EntityVisited(entities[11].id));
  EXPECT_FALSE(visitor.EntityVisited(entities[13].id));
}

TEST_P(SpatialSortTreeFixture, EightCornerVerticesAreAllVisited) {
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
  SpatialSortTree* tree = GetParam();
  RecordIdsVisitor visitor;
  tree->InsertItems(&items[0], &items[0] + 100);
  tree->VisitNearVertices(&visitor, items[0].position, eps);
  for (int i = 0; i < 8; ++i) {
    EXPECT_TRUE(visitor.EntityVisited(entities[i].id)) << i;
  }
}

TEST_P(SpatialSortTreeFixture, VisitEachVerticesNeighbours) {
  int n = 1000;
  std::vector<Entity> entities = BuildEntitiesAtRandomLocations(unit_cube(), n);
  std::vector<HOTItem> items(BuildItems(&entities));
  double eps = 1.0e-3;
  CountVisits counter(nullptr);
  SpatialSortTree* tree = GetParam();
  for (int i = 0; i < n; ++i) {
    counter.data_ = items[i].data;
    tree->VisitNearVertices(&counter, items[i].position, eps);
  }
  EXPECT_GE(counter.count_, 0);
}

std::vector<SpatialSortTree*> GetTrees() {
  std::vector<SpatialSortTree*> trees;
  trees.push_back(new HOTTree(unit_cube()));
  trees.push_back(new WideTree(unit_cube()));
  WideTree* anotherWideTree(new WideTree(unit_cube()));
  anotherWideTree->SetMaxNumLeafItems(5);
  trees.push_back(anotherWideTree);
#ifdef HOT_HAVE_TBB
  trees.push_back(new HOTTreeParallel(unit_cube()));
  trees.push_back(new WideTreeParallel(unit_cube()));
  WideTreeParallel* yetAnotherWideTree(new WideTreeParallel(unit_cube()));
  yetAnotherWideTree->SetMaxNumLeafItems(5);
  trees.push_back(yetAnotherWideTree);
#endif
  return trees;
}
INSTANTIATE_TEST_CASE_P(SomeTreeProperties,
    SpatialSortTreeFixture,
    ::testing::ValuesIn(GetTrees()));


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
