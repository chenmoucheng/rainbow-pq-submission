/*
 * kptr.h
 */

#ifndef _KPTR_H_
#define _KPTR_H_

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "hash_utils.h"

typedef struct {
	uint8_t state[_HASH_LEN];
	unsigned offset;
	unsigned size;
} prng_t;

prng_t* prng_new(const uint8_t seed[_HASH_LEN], unsigned size);

static inline
void prng_run(prng_t* prng, uint8_t* output, unsigned size, unsigned offset)
{
	assert(offset == prng->offset);
	memcpy(prng->state, &prng->offset, sizeof(prng->offset));
	prng->offset += size;
	prng->offset %= prng->size;
	sha2_chain_msg(output, size, prng->state, sizeof(prng->state));
}

typedef struct {
	uint8_t* cptr;
	unsigned offset;
	unsigned size;
	uint8_t seed[_HASH_LEN];
	prng_t* prng;
} kptr_t;

static inline
kptr_t kptr_init(uint8_t* cptr, unsigned size, const uint8_t seed[_HASH_LEN])
{
	kptr_t ret = { .cptr = cptr, .offset = 0, .size = size, .seed = {0}, .prng = 0 };
	if (ret.cptr == 0) {
		memcpy(ret.seed, seed, sizeof(ret.seed));
		ret.prng = prng_new(ret.seed, ret.size);
	}

	return ret;
}

static inline
void kptr_reinit(kptr_t* ptr)
{
	if (ptr->cptr == 0) {
		ptr->prng = prng_new(ptr->seed, ptr->size);
	}
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

uint8_t* kptr_reify(kptr_t ptr, unsigned offset, unsigned size);

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

