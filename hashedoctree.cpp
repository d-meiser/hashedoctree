#include <hashedoctree.h>
#include <cmath>
#include <cassert>
#include <iostream>


// We use 32 bit keys. That is large enough for 2**10 buckets
// along each dimension.
static const int BITS_PER_DIM = 10;
static const HOTKey NUM_LEAF_BUCKETS = 1 << BITS_PER_DIM;

static HOTKey ComputeBucket(double min, double max, double pos, HOTKey num_buckets) {
	assert(max > min);
	double folded_pos = std::fmod(pos - min, max - min);
	if (folded_pos < 0) {
		folded_pos += max - min;
	}
	int bucket = num_buckets * folded_pos / (max - min);
	assert(bucket >= 0);
	assert(bucket < num_buckets);
	return bucket;
}


HOTKey HOTComputeHash(HOTBoundingBox bbox, HOTPoint point)
{
	int buckets[3] = {0};
	buckets[0] = ComputeBucket(bbox.min.x, bbox.max.x, point.x, NUM_LEAF_BUCKETS);
	buckets[1] = ComputeBucket(bbox.min.y, bbox.max.y, point.y, NUM_LEAF_BUCKETS);
	buckets[2] = ComputeBucket(bbox.min.z, bbox.max.z, point.z, NUM_LEAF_BUCKETS);
	HOTKey key = 0;
	for (int i = 0; i < 3; ++i) {
		for (int b = 0; b < BITS_PER_DIM; ++b) {
			key |= (buckets[i] & (1 << b)) << i;
		}
	}
	return key;
}

HOTTree::HOTTree(
	HOTBoundingBox bbox,
	const HOTPoint* positions_begin,
	const HOTPoint* positions_end,
	const void* data_begin) :
	bbox_(bbox),
	positions_begin_(positions_begin),
	positions_end_(positions_end),
	data_begin_(data_begin) {}
