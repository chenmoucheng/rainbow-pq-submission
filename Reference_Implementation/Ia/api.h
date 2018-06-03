//
//  api.h
//
//  Created by Bassham, Lawrence E (Fed) on 9/6/17.
//  Copyright © 2017 Bassham, Lawrence E (Fed). All rights reserved.
//


//   This is a sample 'api.h' for use 'sign.c'

#ifndef api_h
#define api_h


#include "rainbow_config.h"
#include "rainbow_16.h"


#include "kptr.h"


//  Set these three values apropriately for your algorithm
#define CRYPTO_SECRETKEYBYTES sizeof(rainbow_key_kptr)
#define CRYPTO_PUBLICKEYBYTES _PUB_KEY_LEN
#define CRYPTO_BYTES _SIGNATURE_BYTE

// Change the algorithm name
#define CRYPTO_ALGNAME _S_NAME

int
crypto_sign_keypair(kptr_t pk, kptr_t sk);

int
crypto_sign(unsigned char *sm, unsigned long long *smlen,
            const unsigned char *m, unsigned long long mlen,
            kptr_t sk);

int
crypto_sign_open(unsigned char *m, unsigned long long *mlen,
                 const unsigned char *sm, unsigned long long smlen,
                 kptr_t pk);

#endif /* api_h */
