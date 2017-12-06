#ifndef TEST_UTILITIES_H
#define TEST_UTILITIES_H

#include <hashedoctree.h>


struct Entity {
  HOTPoint position;
  int id;
};

// Count visits of vertices excluding self.
class CountVisits : public HOTTree::VertexVisitor {
  public:
    CountVisits(void* data) : count_{0}, data_{data} {}
    ~CountVisits() override {}
    bool Visit(HOTItem* item) override {
      if (item->data != data_) {
        ++count_;
      }
      return true;
    }
    void Reset() {
      count_ = 0;
    }

    int count_;
    void* data_;
};

std::vector<Entity> BuildEntitiesAtRandomLocations(HOTBoundingBox bbox, int n);
std::vector<HOTItem> BuildItems(std::vector<Entity>* entities);
HOTBoundingBox unit_cube();
HOTTree ConstructTreeWithRandomItems(HOTBoundingBox bbox, int n);

#endif
