#include <hashedoctree.h>
#include <test_utilities.h>
#include <string>
#include <iostream>
#include <tbb/parallel_for.h>


struct Configuration {
  int num_vertices;
  int num_iter;
};

struct TimingResults {
  double ConstructTreeWithRandomItems;
  double VertexDedup1;
  double BuildTreeFromOrderedItems;
  double VertexDedup2;
  double ParallelVertexDedup;
};

Configuration parse_command_line(int argn, char **argv);
HOTTree BuildTreeFromOrderedItems(HOTBoundingBox bbox, const HOTItem* begin, const HOTItem* end);
void VertexDedup(HOTTree* tree);
void ParallelVertexDedup(HOTTree* tree);


int main(int argn, char **argv) {
  Configuration conf = parse_command_line(argn, argv);

  TimingResults results = {};

  std::cout.precision(5);
  std::cout << std::scientific;

  std::cout << "{\n";
  std::cout << "  \"num_vertices\": " << conf.num_vertices << ",\n";
  std::cout << "  \"num_iter\": " << conf.num_iter << ",\n";
  for (int i = 0; i < conf.num_iter; ++i) {

    std::cout << "  \"iteration " << i << "\": {\n";

    std::cout << "    \"timings\": {\n";

    uint64_t start, end;
    start = rdtsc();
    HOTTree tree = ConstructTreeWithRandomItems(unit_cube(), conf.num_vertices);
    end = rdtsc();
    std::cout << "      \"ConstructTreeWithRandomItems\": " << (end - start) / 1.0e6 << ",\n";
    results.ConstructTreeWithRandomItems += (end - start) / 1.0e6;

    start = rdtsc();
    VertexDedup(&tree);
    end = rdtsc();
    std::cout << "      \"VertexDedup1\":                 " << (end - start) / 1.0e6 << ",\n";
    results.VertexDedup1 += (end - start) / 1.0e6;

    start = rdtsc();
    HOTTree tree2 = BuildTreeFromOrderedItems(unit_cube(), &*tree.begin(), &*tree.end());
    end = rdtsc();
    std::cout << "      \"BuildTreeFromOrderedItems\":    " << (end - start) / 1.0e6 << ",\n";
    results.BuildTreeFromOrderedItems += (end - start) / 1.0e6;

    start = rdtsc();
    VertexDedup(&tree2);
    end = rdtsc();
    std::cout << "      \"VertexDedup2\":                 " << (end - start) / 1.0e6 << "\n";
    results.VertexDedup2 += (end - start) / 1.0e6;

    start = rdtsc();
    ParallelVertexDedup(&tree2);
    end = rdtsc();
    std::cout << "      \"ParallelVertexDedup\":          " << (end - start) / 1.0e6 << "\n";
    results.ParallelVertexDedup += (end - start) / 1.0e6;

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
}

HOTTree BuildTreeFromOrderedItems(HOTBoundingBox bbox, const HOTItem* begin, const HOTItem* end) {
  HOTTree tree(bbox);
  tree.InsertItems(begin, end);
  return std::move(tree);
}

void VertexDedup(HOTTree* tree) {
  double eps = 1.0e-3;
  CountVisits counter(nullptr);
  auto item = tree->begin();
  int n = std::distance(tree->begin(), tree->end());
  for (int i = 0; i < n; ++i) {
    counter.data_ = item[i].data;
    tree->VisitNearVertices(&counter, item[i].position, eps);
  }
}

void ParallelVertexDedup(HOTTree* tree) {
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


static int find_string(std::string s, int argn, char **argv) {
  int i = 1;
  for (; i != argn; ++i) {
    if (s == argv[i]) break;
  }
  return i;
}

static const std::string usage("Usage: vertex_dedup_test [--num_vertices num_vertices] [--num_iter num_iter]");

Configuration parse_command_line(int argn, char **argv) {
  Configuration conf;
  conf.num_vertices = 100;
  conf.num_iter = 10;

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

  return conf;
}
