/*
 * kptr.c
 */

#include "kptr.h"

prng_t* prng_new(const uint8_t seed[_HASH_LEN])
{
	static prng_t _pool[10];
	static unsigned _count = 0;

	assert(_count < 10);
	memcpy(_pool[_count].state, seed, _HASH_LEN);
	_pool[_count].offset = 0;

	return &_pool[_count++];
}

uint8_t* kptr_reify(kptr_t ptr, unsigned offset, unsigned size)
{
	static uint8_t _ret[10000];

	assert(size < sizeof(_ret));
	assert(ptr.offset + offset + size <= ptr.size);
	if (ptr.cptr) {
		memcpy(_ret, kptr_manifest(ptr, offset), size);
	}
	else {
		assert(ptr.prng);
		prng_run(ptr.prng, _ret, size, ptr.offset + offset);
	}

	return _ret;
}

