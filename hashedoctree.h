#ifndef HASHED_OCTREE_H
#define HASHED_OCTREE_H

#include <cstdint>


struct HOTPoint
{
	double x;
	double y;
	double z;
};

struct HOTBoundingBox
{
	HOTPoint min;
	HOTPoint max;
};


typedef int32_t HOTKey;

HOTKey HOTComputeHash(HOTBoundingBox bbox, HOTPoint point);

#endif
