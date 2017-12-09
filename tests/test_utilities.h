#ifndef TEST_UTILITIES_H
#define TEST_UTILITIES_H

#include <hashedoctree.h>
#include <stdint.h>
#include <set>


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

// Record ids of visited of vertices.
class RecordIdsVisitor : public HOTTree::VertexVisitor {
  public:
    bool Visit(HOTItem* item) override {
      Entity *entity = static_cast<Entity*>(item->data);
      if (entity) {
        ids.insert(entity->id);
      }
      return true;
    }

    bool EntityVisited(int id) const {
      return ids.find(id) != ids.end();
    }

    std::set<int> ids;
};

std::vector<Entity> BuildEntitiesAtRandomLocations(HOTBoundingBox bbox, int n);
std::vector<HOTItem> BuildItems(std::vector<Entity>* entities);
HOTBoundingBox unit_cube();
HOTTree ConstructTreeWithRandomItems(HOTBoundingBox bbox, int n);
uint64_t rdtsc();

#endif
