#ifndef WIDE_TREE_H
#define WIDE_TREE_H

#include <hashedoctree.h>

uint8_t ComputeWideKey(HOTBoundingBox bbox, HOTPoint location);

void SortByKey(
    const uint8_t* key_begin, const uint8_t* key_end,
    const int* perm_in,
    int* perm_out);


#endif
