#include <hashedoctree.h>
#include <test_utilities.h>
#include <string>
#include <iostream>


struct Configuration {
  int n;
};

Configuration parse_command_line(int argn, char **argv);
HOTTree BuildTreeFromOrderedItems(HOTBoundingBox bbox, const HOTItem* begin, const HOTItem* end);
void VertexDedup(HOTTree* tree);


int main(int argn, char **argv) {
  Configuration conf = parse_command_line(argn, argv);

  HOTTree tree = ConstructTreeWithRandomItems(unit_cube(), conf.n);
  VertexDedup(&tree);

  HOTTree tree2 = BuildTreeFromOrderedItems(unit_cube(), &*tree.begin(), &*tree.end());
  VertexDedup(&tree2);
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


static int find_string(std::string s, int argn, char **argv) {
  int i = 1;
  for (; i != argn; ++i) {
    if (s == argv[i]) break;
  }
  return i;
}

static const std::string usage("Usage: vertex_dedup_test --num_vertices n");

Configuration parse_command_line(int argn, char **argv) {
  Configuration conf;
  conf.n = 100;

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
    conf.n = std::stoi(std::string(argv[i + 1]));
  }

  return conf;
}