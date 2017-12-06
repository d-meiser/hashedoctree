#ifndef TEST_UTILITIES_H
#define TEST_UTILITIES_H

#include <hashedoctree.h>


struct Entity {
  HOTPoint position;
  int id;
};

std::vector<Entity> BuildEntitiesAtRandomLocations(HOTBoundingBox bbox, int n);
std::vector<HOTItem> BuildItems(std::vector<Entity>* entities);
HOTBoundingBox unit_cube();

#endif
