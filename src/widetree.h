#ifndef WIDE_TREE_H
#define WIDE_TREE_H

#include <hashedoctree.h>

uint8_t ComputeWideKey(HOTBoundingBox bbox, HOTPoint location);

void SortByKey(const uint8_t* keys, int n, int* perm);


#endif
