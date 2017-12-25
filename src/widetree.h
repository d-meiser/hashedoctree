#ifndef WIDE_TREE_H
#define WIDE_TREE_H

#include <hashedoctree.h>

uint8_t ComputeWideKey(const HOTBoundingBox& bbox, HOTPoint location);
void ComputeManyWideKeys(const HOTBoundingBox& bbox, const double* locations,
    int n, int stride, uint8_t* keys);

void SortByKey(const uint8_t* keys, int n, int* perm);


#endif
