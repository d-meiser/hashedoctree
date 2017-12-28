#include <spatialsorttree.h>
#include <hashedoctree.h>
#include <widetree.h>
#include <test_utilities.h>
#include <string>
#include <iostream>
#include <cassert>
#include <hot_config.h>
#ifdef HOT_HAVE_TBB
#include <tbb/parallel_for.h>
#include <tbb/task_scheduler_init.h>
#include <hashedoctreeparallel.h>
#include <widetreeparallel.h>
#endif


struct Configuration {
  int num_vertices;
  int num_iter;
  int num_threads;
  const char* tree_type;
};

struct TimingResults {
  double ConstructTreeWithRandomItems;
  double VertexDedup1;
  double BuildTreeFromOrderedItems;
  double VertexDedup2;
  double ParallelVertexDedup;
};

Configuration parse_command_line(int argn, char **argv);
std::unique_ptr<SpatialSortTree> BuildTreeWithRandomItems(HOTBoundingBox bbox, int n, const char* type);
std::unique_ptr<SpatialSortTree> BuildTreeFromOrderedItems(
    HOTBoundingBox bbox, const HOTItem* begin, const HOTItem* end, const char* type);
void VertexDedup(SpatialSortTree* tree);
#ifdef HOT_HAVE_TBB
void ParallelVertexDedup(SpatialSortTree* tree);
#endif
std::unique_ptr<SpatialSortTree> TreeFromType(const HOTBoundingBox& bbox, const char* type);


int main(int argn, char **argv) {
  Configuration conf = parse_command_line(argn, argv);

#ifdef HOT_HAVE_TBB
  tbb::task_scheduler_init scheduler(conf.num_threads);
#endif

  TimingResults results = {0, 0, 0, 0, 0};

  std::cout.precision(5);
  std::cout << std::scientific;

  std::cout << "{\n";
  std::cout << "  \"num_vertices\": " << conf.num_vertices << ",\n";
  std::cout << "  \"num_iter\": " << conf.num_iter << ",\n";
  std::cout << "  \"num_threads\": " << conf.num_threads << ",\n";
  std::cout << "  \"tree_type\": \"" << conf.tree_type << "\",\n";
  for (int i = 0; i < conf.num_iter; ++i) {

    std::cout << "  \"iteration " << i << "\": {\n";

    std::cout << "    \"timings\": {\n";

    uint64_t start, end;
    start = rdtsc();
    std::unique_ptr<SpatialSortTree> tree =
        BuildTreeWithRandomItems(unit_cube(), conf.num_vertices, conf.tree_type);
    end = rdtsc();
    std::cout << "      \"ConstructTreeWithRandomItems\": " << (end - start) / 1.0e6 << ",\n";
    results.ConstructTreeWithRandomItems += (end - start) / 1.0e6;

    start = rdtsc();
    VertexDedup(tree.get());
    end = rdtsc();
    std::cout << "      \"VertexDedup1\":                 " << (end - start) / 1.0e6 << ",\n";
    results.VertexDedup1 += (end - start) / 1.0e6;

    start = rdtsc();
    std::unique_ptr<SpatialSortTree> tree2 =
        BuildTreeFromOrderedItems(unit_cube(), &*tree->begin(), &*tree->end(),
        conf.tree_type);
    end = rdtsc();
    std::cout << "      \"BuildTreeFromOrderedItems\":    " << (end - start) / 1.0e6 << ",\n";
    results.BuildTreeFromOrderedItems += (end - start) / 1.0e6;

    start = rdtsc();
    VertexDedup(tree2.get());
    end = rdtsc();
    std::cout << "      \"VertexDedup2\":                 " << (end - start) / 1.0e6 << "\n";
    results.VertexDedup2 += (end - start) / 1.0e6;

#if HOT_HAVE_TBB
    start = rdtsc();
    ParallelVertexDedup(tree2.get());
    end = rdtsc();
    std::cout << "      \"ParallelVertexDedup\":          " << (end - start) / 1.0e6 << "\n";
    results.ParallelVertexDedup += (end - start) / 1.0e6;
#endif

    std::cout << "    }\n  }," << std::endl;
  }

  std::cout << "  \"totals\": {\n";
  std::cout << "    \"ConstructTreeWithRandomItems\":   " << results.ConstructTreeWithRandomItems << ",\n";
  std::cout << "    \"VertexDedup1\":                   " << results.VertexDedup1 << ",\n";
  std::cout << "    \"BuildTreeFromOrderedItems\":      " << results.BuildTreeFromOrderedItems << ",\n";
  std::cout << "    \"VertexDedup2\":                   " << results.VertexDedup2 << "\n";
  std::cout << "    \"ParallelVertexDedup\":            " << results.ParallelVertexDedup << "\n";
  std::cout << "  },\n";

  std::cout << "  \"averages\": {\n";
  std::cout << "    \"ConstructTreeWithRandomItems\":   " << results.ConstructTreeWithRandomItems / conf.num_iter << ",\n";
  std::cout << "    \"VertexDedup1\":                   " << results.VertexDedup1 / conf.num_iter << ",\n";
  std::cout << "    \"BuildTreeFromOrderedItems\":      " << results.BuildTreeFromOrderedItems / conf.num_iter << ",\n";
  std::cout << "    \"VertexDedup2\":                   " << results.VertexDedup2 / conf.num_iter << "\n";
  std::cout << "    \"ParallelVertexDedup\":            " << results.ParallelVertexDedup / conf.num_iter << "\n";
  std::cout << "  }\n";
  std::cout << "}\n";

#ifdef HOT_HAVE_TBB
  scheduler.terminate();
#endif
}

std::unique_ptr<SpatialSortTree> BuildTreeWithRandomItems(HOTBoundingBox bbox, int n, const char* type) {
  assert(n > 0);
  std::unique_ptr<SpatialSortTree> tree = TreeFromType(bbox, type);
  auto entities = BuildEntitiesAtRandomLocations(bbox, n);
  auto items = BuildItems(&entities);
  tree->InsertItems(&items[0], &items[0] + n);
  return tree;
}

std::unique_ptr<SpatialSortTree> BuildTreeFromOrderedItems(HOTBoundingBox bbox,
    const HOTItem* begin, const HOTItem* end, const char* type) {
  std::unique_ptr<SpatialSortTree> tree = TreeFromType(bbox, type);
  tree->InsertItems(begin, end);
  return tree;
}

void VertexDedup(SpatialSortTree* tree) {
  double eps = 1.0e-3;
  CountVisits counter(nullptr);
  auto item = tree->begin();
  int n = std::distance(tree->begin(), tree->end());
  for (int i = 0; i < n; ++i) {
    counter.data_ = item[i].data;
    tree->VisitNearVertices(&counter, item[i].position, eps);
  }
}

#ifdef HOT_HAVE_TBB
void ParallelVertexDedup(SpatialSortTree* tree) {
  double eps = 1.0e-3;
  auto item = tree->begin();
  int n = std::distance(tree->begin(), tree->end());
  tbb::parallel_for (tbb::blocked_range<int>(0, n, 1 << 10),
    [&](const tbb::blocked_range<int>& range) {
      CountVisits counter(nullptr);
      for (int i = range.begin(); i != range.end(); ++i) {
        counter.data_ = item[i].data;
        tree->VisitNearVertices(&counter, item[i].position, eps);
      }
    });
}
#endif

static int find_string(std::string s, int argn, char **argv) {
  int i = 1;
  for (; i != argn; ++i) {
    if (s == argv[i]) break;
  }
  return i;
}

static const std::string usage(
    "Usage: vertex_dedup_test "
    "[--num_vertices num_vertices] "
    "[--num_iter num_iter] "
    "[--num_threads num_threads] "
    "[--tree_type tree_type"
    "\n\n"
    "Available tree_types:\n"
    "  HashedOctree\n"
    "  WideTree\n"
#ifdef HOT_HAVE_TBB
    "  HashedOctreeParallel\n"
    "  WideTreeParallel\n"
#endif
    );

Configuration parse_command_line(int argn, char **argv) {
  Configuration conf;
  conf.num_vertices = 100;
  conf.num_iter = 10;
  conf.num_threads = 1;
  conf.tree_type = "HashedOctree";

  int i;
  i = find_string("--help", argn, argv);
  if (i != argn) {
    std::cout << usage << std::endl;
    exit(0);
  }

  i = find_string("--num_vertices", argn, argv);
  if (i != argn) {
    if (i == argn - 1) {
      std::cout << "Error: Number of vertices parameter missing." << std::endl;
      std::cout << usage << std::endl;
      exit(1);
    }
    conf.num_vertices = std::stoi(std::string(argv[i + 1]));
  }

  i = find_string("--num_iter", argn, argv);
  if (i != argn) {
    if (i == argn - 1) {
      std::cout << "Error: Number of iterations parameter missing." << std::endl;
      std::cout << usage << std::endl;
      exit(1);
    }
    conf.num_iter = std::stoi(std::string(argv[i + 1]));
  }

  i = find_string("--num_threads", argn, argv);
  if (i != argn) {
    if (i == argn - 1) {
      std::cout << "Error: Number of threads parameter missing." << std::endl;
      std::cout << usage << std::endl;
      exit(1);
    }
    conf.num_threads = std::stoi(std::string(argv[i + 1]));
  }

  i = find_string("--tree_type", argn, argv);
  if (i != argn) {
    if (i == argn - 1) {
      std::cout << "Error: tree type parameter missing." << std::endl;
      std::cout << usage << std::endl;
      exit(1);
    }
    conf.tree_type = argv[i + 1];
  }

  return conf;
}

std::unique_ptr<SpatialSortTree> TreeFromType(const HOTBoundingBox& bbox,
    const char* type) {
  if (std::string("HashedOctree") == type) {
    return std::unique_ptr<SpatialSortTree>(new HOTTree(bbox));
  } else if (std::string("WideTree") == type) {
    return std::unique_ptr<SpatialSortTree>(new WideTree(bbox));
#ifdef HOT_HAVE_TBB
  } else if (std::string("HashedOctreeParallel") == type) {
    return std::unique_ptr<SpatialSortTree>(new HOTTreeParallel(bbox));
  } else if (std::string("WideTreeParallel") == type) {
    return std::unique_ptr<SpatialSortTree>(new WideTreeParallel(bbox));
#endif
  }
  return nullptr;
}

