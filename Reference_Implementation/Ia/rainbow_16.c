
#include "rainbow_config.h"

#include "rainbow_16.h"

#include "blas.h"

#include "string.h"

#include "assert.h"

#include "prng_utils.h"


#ifdef _RAINBOW_16



#ifndef _DEBUG_RAINBOW_

static void rainbow_central_map( uint8_t * r , kptr_t k , const uint8_t * a );

static void rainbow_pubmap_seckey( uint8_t * z , const rainbow_key_kptr * sk , const uint8_t * w );

#endif


#ifndef _DEBUG_RAINBOW_
static
#endif
void rainbow_pubmap_seckey( uint8_t * z , const rainbow_key_kptr * sk , const uint8_t * w ) {

	uint8_t tt[_PUB_N_BYTE]  = {0};
	uint8_t tt2[_PUB_N_BYTE]  = {0};

	gf16mat_prod( tt , sk->mat_t , _PUB_N_BYTE , _PUB_N , w );
	gf256v_add( tt , sk->vec_t , _PUB_N_BYTE );

	rainbow_central_map( tt2 , sk->ckey , tt );

	gf16mat_prod( z , sk->mat_s , _PUB_M_BYTE , _PUB_M , tt2 );
	gf256v_add( z , sk->vec_s , _PUB_M_BYTE );

}



static inline
void rainbow_pubmap_wrapper( void * z, const void* pk_key, const void * w) {
	rainbow_pubmap_seckey( (uint8_t *)z , (const rainbow_key_kptr *)pk_key, (const uint8_t *)w );
}


void rainbow_genkey( kptr_t pk , kptr_t sk )
{

	rainbow_key_kptr _pk;
	rainbow_key_kptr* _sk = kptr_manifest(sk, 0);

	uint8_t seed[_HASH_LEN] = {0};
	_pk. ckey = kptr_wrap(seed, 0, sizeof(rainbow_ckey));
	_sk->ckey = kptr_wrap(seed, 0, sizeof(rainbow_ckey));

	gf16mat_rand_inv( _pk.mat_t , kptr_manifest(sk, RAINBOWKEY_MAT_T) , _PUB_N );
	gf16mat_rand_inv( _pk.mat_s , kptr_manifest(sk, RAINBOWKEY_MAT_S) , _PUB_M );

	gf256v_rand( _pk.vec_t , _PUB_N_BYTE );
	memcpy( kptr_manifest(sk, RAINBOWKEY_VEC_T) , _pk.vec_t , _PUB_N_BYTE );
	//gf16mat_prod( sk->vec_t , sk->mat_t , _PUB_N_BYTE , _PUB_N , pk->vec_t );

	gf256v_rand( _pk.vec_s , _PUB_M_BYTE );
	memcpy( kptr_manifest(sk, RAINBOWKEY_VEC_S) , _pk.vec_s , _PUB_M_BYTE );
	//gf16mat_prod( sk->vec_s , sk->mat_s , _PUB_M_BYTE , _PUB_M , pk->vec_s );

	mpkc_interpolate_gf16( kptr_manifest(pk, 0) , rainbow_pubmap_wrapper , (const void*) &_pk );

	kptr_deref_lhs(pk, _PUB_KEY_LEN-1, _SALT_BYTE);
	kptr_deref_lhs(sk, _SEC_KEY_LEN-1, _SALT_BYTE);
}





/////////////////////////////


static inline
void transpose_l1( uint8_t * r , const uint8_t * a )
{
	for(unsigned i=0;i<_O1;i++) {
		for(unsigned j=0;j<_O1;j++) {
			gf16v_set_ele( r+i*_O1_BYTE , j , gf16v_get_ele( a+j*_O1_BYTE , i ) );
		}
	}
}

static inline
void transpose_l2( uint8_t * r , const uint8_t * a )
{
	for(unsigned i=0;i<_O2;i++) {
		for(unsigned j=0;j<_O2;j++) {
			gf16v_set_ele( r+i*_O2_BYTE , j , gf16v_get_ele( a+j*_O2_BYTE , i ) );
		}
	}
}

static inline
void gen_l1_mat( uint8_t * mat , kptr_t l1_o , kptr_t l1_vo , const uint8_t * v ) {
	for(unsigned i=0;i<_O1;i++) {
		gf16mat_prod( mat + i*_O1_BYTE , kptr_reify(l1_vo, i*_V1*_O1_BYTE, _V1*_O1_BYTE) , _O1_BYTE , _V1 , v );
		gf256v_add( mat + i*_O1_BYTE , kptr_reify(l1_o, i*_O1_BYTE, _O1_BYTE) , _O1_BYTE );
	}
}

static inline
void gen_l2_mat( uint8_t * mat , kptr_t l2_o , kptr_t l2_vo , const uint8_t * v ) {
	for(unsigned i=0;i<_O2;i++) {
		gf16mat_prod( mat + i*_O2_BYTE , kptr_reify(l2_vo, i*_V2*_O2_BYTE, _V2*_O2_BYTE) , _O2_BYTE , _V2 , v );
		gf256v_add( mat + i*_O2_BYTE , kptr_reify(l2_o, i*_O2_BYTE, _O2_BYTE) , _O2_BYTE );
	}
}



#ifndef _DEBUG_RAINBOW_
static
#endif
void rainbow_central_map( uint8_t * r , kptr_t k , const uint8_t * a ) {
#ifdef _DEBUG_MPKC_
memcpy( r , a+_V1_BYTE , _PUB_M_BYTE );
return;
#endif
	uint8_t mat1[_O2*_O2] ;
	uint8_t temp[_O2_BYTE] ;

	gen_l1_mat( mat1 , kptr_slice(k, RAINBOWCKEY_L1_O) , kptr_slice(k, RAINBOWCKEY_L1_VO) , a );

	uint8_t mat2[_O2*_O2] ;
	transpose_l1( mat2 , mat1 );
	gf16mat_prod( r , mat2 , _O1_BYTE , _O1 , a+_V1_BYTE );

	mpkc_pub_map_gf16_n_m( temp , kptr_slice(k, RAINBOWCKEY_L1_VV) , a , _V1 , _O1 );
	gf256v_add( r , temp , _O1_BYTE );

	gen_l2_mat( mat1 , kptr_slice(k, RAINBOWCKEY_L2_O), kptr_slice(k, RAINBOWCKEY_L2_VO) , a );

	transpose_l2( mat2 , mat1 );
	gf16mat_prod( r+_O1_BYTE , mat2 , _O2_BYTE , _O2 , a+_V2_BYTE );

	mpkc_pub_map_gf16_n_m( temp , kptr_slice(k, RAINBOWCKEY_L2_VV) , a , _V2 , _O2 );
	gf256v_add( r+_O1_BYTE , temp , _O2_BYTE );

}



static inline
unsigned linear_solver_l1( uint8_t * r , const uint8_t * mat_32x32 , const uint8_t * cc )
{
	uint8_t mat[_O1*_O1] ;
	for(unsigned i=0;i<_O1;i++) {
		memcpy( mat + i*(_O1_BYTE+1) , mat_32x32 + i*(_O1_BYTE) , _O1 );
		mat[i*(_O1_BYTE+1)+_O1_BYTE] = gf16v_get_ele( cc , i );
	}
	unsigned r8 = gf16mat_gauss_elim( mat , _O1 , _O1+2 );
	for(unsigned i=0;i<_O1;i++) {
		gf16v_set_ele( r , i , mat[i*(_O1_BYTE+1)+_O1_BYTE] );
	}
	return r8;
}

static inline
unsigned linear_solver_l2( uint8_t * r , const uint8_t * mat_32x32 , const uint8_t * cc )
{
	uint8_t mat[_O2*_O2] ;
	for(unsigned i=0;i<_O2;i++) {
		memcpy( mat + i*(_O2_BYTE+1) , mat_32x32 + i*(_O2_BYTE) , _O2 );
		mat[i*(_O2_BYTE+1)+_O2_BYTE] = gf16v_get_ele( cc , i );
	}
	unsigned r8 = gf16mat_gauss_elim( mat , _O2 , _O2+2 );
	for(unsigned i=0;i<_O2;i++) {
		gf16v_set_ele( r , i , mat[i*(_O2_BYTE+1)+_O2_BYTE] );
	}
	return r8;
}


#include "hash_utils.h"


/// algorithm 7
int rainbow_sign( uint8_t * signature , kptr_t sk , const uint8_t * _digest )
{
	kptr_t k = *(kptr_t*)kptr_manifest(sk, RAINBOWKEY_CKEY);
//// line 1 - 5
	uint8_t mat_l1[_O1*_O1] ;
	uint8_t mat_l2[_O2*_O2] ;
	uint8_t temp_o1[_O1_BYTE]  = {0};
	uint8_t temp_o2[_O2_BYTE] ;
	uint8_t vinegar[_V1_BYTE] ;
	unsigned l1_succ = 0;
	unsigned time = 0;
	while( !l1_succ ) {
		if( 512 == time ) break;
		gf256v_rand( vinegar , _V1_BYTE );
		gen_l1_mat( mat_l1 , kptr_slice(k, RAINBOWCKEY_L1_O) , kptr_slice(k, RAINBOWCKEY_L1_VO) , vinegar );

		l1_succ = linear_solver_l1( temp_o1 , mat_l1 , temp_o1 );
		time ++;
	}
	uint8_t temp_vv1[_O1_BYTE] ;
	mpkc_pub_map_gf16_n_m( temp_vv1 , kptr_slice(k, RAINBOWCKEY_L1_VV) , vinegar , _V1 , _O1 );

	//// line 7 - 14
	uint8_t _z[_PUB_M_BYTE] ;
	uint8_t y[_PUB_M_BYTE] ;
	uint8_t x[_PUB_N_BYTE] ;
	uint8_t w[_PUB_N_BYTE] ;
	uint8_t digest_salt[_HASH_LEN + _SALT_BYTE] = {0};
	uint8_t * salt = digest_salt + _HASH_LEN;
	memcpy( digest_salt , _digest , _HASH_LEN );

	memcpy( x , vinegar , _V1_BYTE );
	unsigned succ = 0;
	while( !succ ) {
		if( 512 == time ) break;

		gf256v_rand( salt , _SALT_BYTE );  /// line 8
		sha2_chain_msg( _z , _PUB_M_BYTE , digest_salt , _HASH_LEN+_SALT_BYTE ); /// line 9

		gf256v_add(_z,kptr_reify(sk,RAINBOWKEY_VEC_S,sizeof(((rainbow_key*)0)->vec_s)),_PUB_M_BYTE);
		gf16mat_prod(y,kptr_reify(sk,RAINBOWKEY_MAT_S,sizeof(((rainbow_key*)0)->mat_s)),_PUB_M_BYTE,_PUB_M,_z); /// line 10

		memcpy( temp_o1 , temp_vv1 , _O1_BYTE );
		gf256v_add( temp_o1 , y , _O1_BYTE );
		linear_solver_l1( x + _V1_BYTE , mat_l1 , temp_o1 );

		gen_l2_mat( mat_l2 , kptr_slice(k, RAINBOWCKEY_L2_O) , kptr_slice(k, RAINBOWCKEY_L2_VO) , x );
		mpkc_pub_map_gf16_n_m( temp_o2 , kptr_slice(k, RAINBOWCKEY_L2_VV) , x , _V2 , _O2 );
		gf256v_add( temp_o2 , y+_O1_BYTE , _O2_BYTE );
		succ = linear_solver_l2( x + _V2_BYTE , mat_l2 , temp_o2 );  /// line 13

		time ++;
	};
	gf256v_add(x,kptr_reify(sk,RAINBOWKEY_VEC_T,sizeof(((rainbow_key*)0)->vec_t)),_PUB_N_BYTE);
	gf16mat_prod(w,kptr_reify(sk,RAINBOWKEY_MAT_T,sizeof(((rainbow_key*)0)->mat_t)),_PUB_N_BYTE,_PUB_N,x);

	memset( signature , 0 , _SIGNATURE_BYTE );
        // return time;
	if( 256 <= time ) return -1;
	gf256v_add( signature , w , _PUB_N_BYTE );
	gf256v_add( signature + _PUB_N_BYTE , salt , _SALT_BYTE );
	return 0;
}

/// algorithm 8
int rainbow_verify( const uint8_t * digest , const uint8_t * signature , kptr_t pk )
{
	unsigned char digest_ck[_PUB_M_BYTE];
	rainbow_pubmap( digest_ck , pk , signature );

	unsigned char correct[_PUB_M_BYTE];
	unsigned char digest_salt[_HASH_LEN + _SALT_BYTE];
	memcpy( digest_salt , digest , _HASH_LEN );
	memcpy( digest_salt+_HASH_LEN , signature+_PUB_N_BYTE , _SALT_BYTE );
	sha2_chain_msg( correct , _PUB_M_BYTE , digest_salt , _HASH_LEN+_SALT_BYTE );

	unsigned char cc = 0;
	for(unsigned i=0;i<_PUB_M_BYTE;i++) {
		cc |= (digest_ck[i]^correct[i]);
	}
	return (0==cc)? 0: -1;
}



#endif  /// _RAINBOW_16
