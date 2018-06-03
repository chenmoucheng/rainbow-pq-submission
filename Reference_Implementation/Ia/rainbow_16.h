
#ifndef _RAINBOW_H_
#define _RAINBOW_H_

#include "rainbow_config.h"

#include "blas.h"

#include "kptr.h"

//#define _DEBUG_MPKC_


#ifdef  __cplusplus
extern  "C" {
#endif



struct _rainbow_ckey {
	uint8_t l1_o[_O1*_O1_BYTE];
	uint8_t l1_vo[_O1][_V1*_O1_BYTE];
	uint8_t l1_vv[TERMS_QUAD_POLY(_V1)*_O1_BYTE];

	uint8_t l2_o[_O2*_O2_BYTE];
	uint8_t l2_vo[_O2][_V2*_O2_BYTE];
	uint8_t l2_vv[TERMS_QUAD_POLY(_V2)*_O2_BYTE];
};

typedef struct _rainbow_ckey rainbow_ckey;

#define RAINBOWCKEY_L1_O  0
#define RAINBOWCKEY_L1_VO (RAINBOWCKEY_L1_O  + _O1*_O1_BYTE)
#define RAINBOWCKEY_L1_VV (RAINBOWCKEY_L1_VO + _O1*_V1*_O1_BYTE)
#define RAINBOWCKEY_L2_O  (RAINBOWCKEY_L1_VV + TERMS_QUAD_POLY(_V1)*_O1_BYTE)
#define RAINBOWCKEY_L2_VO (RAINBOWCKEY_L2_O  + _O2*_O2_BYTE)
#define RAINBOWCKEY_L2_VV (RAINBOWCKEY_L2_VO + _O2*_V2*_O2_BYTE)

struct _rainbow_key {
	uint8_t mat_t[_PUB_N * _PUB_N_BYTE];
	uint8_t vec_t[_PUB_N_BYTE];
	uint8_t mat_s[_PUB_M * _PUB_M_BYTE];
	uint8_t vec_s[_PUB_M_BYTE];

	rainbow_ckey ckey;
};

typedef struct _rainbow_key rainbow_key;

#define RAINBOWKEY_MAT_T 0
#define RAINBOWKEY_VEC_T (RAINBOWKEY_MAT_T + _PUB_N * _PUB_N_BYTE)
#define RAINBOWKEY_MAT_S (RAINBOWKEY_VEC_T + _PUB_N_BYTE)
#define RAINBOWKEY_VEC_S (RAINBOWKEY_MAT_S + _PUB_M * _PUB_M_BYTE)
#define RAINBOWKEY_CKEY  (RAINBOWKEY_VEC_S + _PUB_M_BYTE)

struct _rainbow_key_kptr {
	uint8_t mat_t[_PUB_N * _PUB_N_BYTE];
	uint8_t vec_t[_PUB_N_BYTE];
	uint8_t mat_s[_PUB_M * _PUB_M_BYTE];
	uint8_t vec_s[_PUB_M_BYTE];

	kptr_t ckey;
};

typedef struct _rainbow_key_kptr rainbow_key_kptr;


/// length for secret key ( extra 1 for length of salt)
#define _SEC_KEY_LEN (sizeof(rainbow_key) + 1)



/// algorithm 6
void rainbow_genkey( kptr_t pk , kptr_t sk );


#include "mpkc.h"

#define rainbow_pubmap mpkc_pub_map_gf16


//#define _DEBUG_MPKC_

#define _DEBUG_RAINBOW_

#ifdef _DEBUG_RAINBOW_

/// algorithm 1
void rainbow_central_map( uint8_t * r , kptr_t k , const uint8_t * a );

void rainbow_pubmap_seckey( uint8_t * z , const rainbow_key_kptr * sk , const uint8_t * w );

#endif



/// algorithm 7
int rainbow_sign( uint8_t * signature , kptr_t sk , const uint8_t * digest );

/// algorithm 8
int rainbow_verify( const uint8_t * digest , const uint8_t * signature , kptr_t pk );



#ifdef  __cplusplus
}
#endif


#endif
