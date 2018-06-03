/*
 * kptr.h
 */

#ifndef _KPTR_H_
#define _KPTR_H_

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "hash_utils.h"

typedef struct {
	union {
		uint8_t seed[_HASH_LEN];
		unsigned salt;
	};
	uint8_t* cptr;
	unsigned offset;
	unsigned size;
} kptr_t;

static inline
kptr_t kptr_wrap(const uint8_t seed[_HASH_LEN], uint8_t* cptr, unsigned size)
{
	kptr_t ret = { .cptr = cptr, .offset = 0, .size = size };
	memcpy(ret.seed, seed, sizeof(ret.seed));

	return ret;
}

static inline
kptr_t kptr_slice(kptr_t ptr, unsigned offset)
{
	ptr.offset += offset;
	assert(ptr.offset < ptr.size);

	return ptr;
}

/*
 * kptr_manifest:
 *     intended for internal use by kptr_reify;
 *     external use for I/O & debug purposes only!
 */
static inline
uint8_t* kptr_manifest(kptr_t ptr, unsigned offset)
{
	assert(ptr.cptr);
	return ptr.cptr + ptr.offset + offset;
}

static inline
uint8_t* kptr_reify(kptr_t ptr, unsigned offset, unsigned size)
{
	static uint8_t ret[1000000];
	assert(size < sizeof(ret));
	assert(ptr.offset + offset < ptr.size);
	if (ptr.cptr) {
		memcpy(ret, kptr_manifest(ptr, offset), size);
	}
	else {
		ptr.salt = ptr.offset + offset;
		sha2_chain_msg(ret, size, ptr.seed, sizeof(ptr.seed));
	}

	return ret;
}

static inline
uint8_t kptr_deref_rhs(kptr_t ptr, unsigned offset)
{
	assert(ptr.cptr);
	assert(ptr.offset + offset < ptr.size);
	return ptr.cptr[ptr.offset + offset];
}

static inline
void kptr_deref_lhs(kptr_t ptr, unsigned offset, uint8_t val)
{
	assert(ptr.cptr);
	assert(ptr.offset + offset < ptr.size);
	ptr.cptr[ptr.offset + offset] = val;
}

#endif /* _KPTR_H_ */

