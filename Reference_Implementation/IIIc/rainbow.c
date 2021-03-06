
#include "rainbow_config.h"

#include "rainbow.h"

#include "gf16.h"

#include "blas.h"

#include "stdint.h"

#include "stdlib.h"

#include "string.h"

//#define _DEBUG_MPKC_


#ifdef _RAINBOW_256


#ifndef _DEBUG_RAINBOW_
static unsigned rainbow_ivs_central_map( uint8_t * r , const rainbow_ckey * k , const uint8_t * a );
static void rainbow_central_map( uint8_t * r , const rainbow_ckey * k , const uint8_t * a );
#endif



#ifndef _DEBUG_RAINBOW_
static
#endif
void rainbow_pubmap_seckey( uint8_t * z , const rainbow_key * sk , const uint8_t * w ) {

	uint8_t tt[_PUB_N_BYTE]  = {0};
	uint8_t tt2[_PUB_N_BYTE]  = {0};

	gf256mat_prod( tt , sk->mat_t , _PUB_N_BYTE , _SEC_N , w );
	gf256v_add( tt , sk->vec_t , _PUB_N_BYTE );

	rainbow_central_map( tt2 , & sk->ckey , tt );

	gf256mat_prod( z , sk->mat_s , _PUB_M_BYTE , _PUB_M , tt2 );
	gf256v_add( z , sk->vec_s , _PUB_M_BYTE );

}


#ifndef _DEBUG_RAINBOW_
static
#endif
void rainbow_genkey_debug( rainbow_key * pk , rainbow_key * sk )
{
	gf256v_rand( (uint8_t *)&sk->ckey , sizeof(rainbow_ckey) );
	memcpy( (void *)&pk->ckey , (void *)&sk->ckey , sizeof(rainbow_ckey) );

	gf256mat_rand_inv( pk->mat_s , sk->mat_s , _PUB_M );
	gf256mat_rand_inv( pk->mat_t , sk->mat_t , _PUB_N );

	gf256v_rand( pk->vec_t , _PUB_N );
	memcpy( sk->vec_t , pk->vec_t , _PUB_N_BYTE );

	gf256v_rand( pk->vec_s , _PUB_M );
	memcpy( sk->vec_s , pk->vec_s , _PUB_M_BYTE );
}



#include "mpkc.h"

static inline
void rainbow_pubmap_wrapper( void * z, const void* pk_key, const void * w) {
	rainbow_pubmap_seckey( (uint8_t *)z , (const rainbow_key *)pk_key, (const uint8_t *)w );
}


void rainbow_genkey( uint8_t * pk , uint8_t * sk )
{

	rainbow_key _pk;
	rainbow_genkey_debug( &_pk , (rainbow_key *)sk );

	mpkc_interpolate_gf256( pk , rainbow_pubmap_wrapper , (const void*) &_pk );

	pk[_PUB_KEY_LEN-1] = _SALT_BYTE;
	sk[_SEC_KEY_LEN-1] = _SALT_BYTE;
}




unsigned rainbow_secmap( uint8_t * w , const rainbow_key * sk , const uint8_t * z )
{
	uint8_t _z[_PUB_N_BYTE] ;
	uint8_t y[_PUB_N_BYTE] ;
	uint8_t x[_PUB_N_BYTE] ;

	memcpy(_z,z,_PUB_M_BYTE);
	gf256v_add(_z,sk->vec_s,_PUB_M_BYTE);
	gf256mat_prod(y,sk->mat_s,_PUB_M_BYTE,_PUB_M,_z);

	unsigned succ = 0;
	unsigned time = 0;
	while( !succ ) {
		if( 256 == time ) break;

		gf256v_rand( x , _V1 );

		succ = rainbow_ivs_central_map( x , & sk->ckey , y );
		time ++;
	};

	gf256v_add(x,sk->vec_t,_PUB_N_BYTE);
	gf256mat_prod(w,sk->mat_t,_PUB_N_BYTE,_PUB_N,x);

	// return time;
	if( 256 <= time ) return -1;
	return 0;
}





/////////////////////////////


static inline
void transpose_l1( uint8_t * r , const uint8_t * a )
{
	for(unsigned i=0;i<_O1;i++) {
		for(unsigned j=0;j<_O1;j++) {
			r[i*_O1+j] = a[j*_O1+i];
		}
	}
}

static inline
void transpose_l2( uint8_t * r , const uint8_t * a )
{
	for(unsigned i=0;i<_O2;i++) {
		for(unsigned j=0;j<_O2;j++) {
			r[i*_O2+j] = a[j*_O2+i];
		}
	}
}

static inline
void gen_l1_mat( uint8_t * mat , const rainbow_ckey * k , const uint8_t * v ) {
	for(unsigned i=0;i<_O1;i++) {
		gf256mat_prod( mat + i*_O1 , k->l1_vo[i] , _O1 , _V1 , v );
		gf256v_add( mat + i*_O1 , k->l1_o + i*_O1 , _O1 );
	}
}

static inline
void gen_l2_mat( uint8_t * mat , const rainbow_ckey * k , const uint8_t * v ) {
	for(unsigned i=0;i<_O2;i++) {
		gf256mat_prod( mat + i*_O2 , k->l2_vo[i] , _O2 , _V2 , v );
		gf256v_add( mat + i*_O2 , k->l2_o + i*_O2 , _O2 );
	}
}



#ifndef _DEBUG_RAINBOW_
static
#endif
void rainbow_central_map( uint8_t * r , const rainbow_ckey * k , const uint8_t * a ) {
#ifdef _DEBUG_MPKC_
memcpy( r , a+_V1 , _PUB_M );
return;
#endif
	/// warning: asming _O2 > _O1
	uint8_t mat1[_O2*_O2] ;
	uint8_t mat2[_O2*_O2] ;
	uint8_t temp[_O2] ;

	gen_l1_mat( mat1 , k , a );
	transpose_l1( mat2 , mat1 );
	gf256mat_prod( r , mat2 , _O1 , _O1 , a+_V1 );
	//memset(temp,0,20);
	mpkc_pub_map_gf256_n_m( temp , k->l1_vv , a , _V1 , _O1 );
	gf256v_add( r , temp , _O1 );

	gen_l2_mat( mat1 , k , a );
	transpose_l2( mat2 , mat1 );
	gf256mat_prod( r+_O1 , mat2 , _O2 , _O2 , a+_V2 );
	mpkc_pub_map_gf256_n_m( temp , k->l2_vv , a , _V2 , _O2 );
	gf256v_add( r+_O1 , temp , _O2 );

}


#define _O1_W (_O1+1)
#define _O2_W (_O2+1)


static inline
unsigned linear_solver_l1( uint8_t * r , const uint8_t * mat_inp , const uint8_t * cc )
{
	uint8_t mat[_O1_W*_O1] ;
	for(unsigned i=0;i<_O1;i++) {
		memcpy( mat+i*_O1_W , mat_inp+i*_O1 , _O1 );
		mat[i*_O1_W + _O1] = cc[i];
	}

	unsigned rv = gf256mat_gauss_elim( mat , _O1 , _O1_W );
	gf256mat_submat( r , 1 , _O1 , mat , _O1_W , _O1 );
	return rv;
}


static inline
unsigned linear_solver_l2( uint8_t * r , const uint8_t * mat_inp , const uint8_t * cc ) {
	uint8_t mat[_O2_W*_O2] ;
	for(unsigned i=0;i<_O2;i++) {
		memcpy( mat+i*_O2_W , mat_inp+i*_O2 , _O2 );
		mat[i*_O2_W + _O2] = cc[i];
	}

	unsigned rv = gf256mat_gauss_elim( mat , _O2 , _O2_W );
	gf256mat_submat( r , 1 , _O2 , mat , _O2_W , _O2 );
	return rv;

}



#ifndef _DEBUG_RAINBOW_
static
#endif
unsigned rainbow_ivs_central_map( uint8_t * r , const rainbow_ckey * k , const uint8_t * a ) {
#ifdef _DEBUG_MPKC_
memcpy( r+_V1 , a , _PUB_M );
return;
#endif
	uint8_t mat1[_O1*_O1] ;
	uint8_t mat2[_O2*_O2] ;
	uint8_t temp[_O2] ;
	mpkc_pub_map_gf256_n_m( temp , k->l1_vv , r , _V1 , _O1 );
	gf256v_add( temp  , a , _O1 );
	gen_l1_mat( mat1 , k , r );
	unsigned r1 = linear_solver_l1( r+_V1 , mat1 , temp );

	gen_l2_mat( mat2 , k , r );
	mpkc_pub_map_gf256_n_m( temp , k->l2_vv , r , _V2 , _O2 );
	gf256v_add( temp  , a+_O1 , _O2 );
	unsigned r2 = linear_solver_l2( r+_V2 , mat2 , temp );

	return r1&r2;
}



#include "hash_utils.h"


/// algorithm 7
int rainbow_sign( uint8_t * signature , const uint8_t * _sk , const uint8_t * _digest )
{
	const rainbow_key * sk = (const rainbow_key *)_sk;
	const rainbow_ckey * k = &( sk->ckey);
//// line 1 - 5
	uint8_t mat_l1[_O1*_O1] ;
	uint8_t mat_l2[_O2*_O2] ;
	uint8_t temp_o1[_O1]  = {0};
	uint8_t temp_o2[_O2] ;
	uint8_t vinegar[_V1] ;
	unsigned l1_succ = 0;
	unsigned time = 0;
	while( !l1_succ ) {
		if( 512 == time ) break;
		gf256v_rand( vinegar , _V1 );
		gen_l1_mat( mat_l1 , k , vinegar );

		l1_succ = linear_solver_l1( temp_o1 , mat_l1 , temp_o1 );
		time ++;
	}
	uint8_t temp_vv1[_O1] ;
	mpkc_pub_map_gf256_n_m( temp_vv1 , k->l1_vv , vinegar , _V1 , _O1 );

//// line 7 - 14
	uint8_t _z[_PUB_N_BYTE] ;
	uint8_t y[_PUB_N_BYTE] ;
	uint8_t x[_PUB_N_BYTE] ;
	uint8_t w[_PUB_N_BYTE] ;

	uint8_t digest_salt[_HASH_LEN + _SALT_BYTE] = {0};
	uint8_t * salt = digest_salt + _HASH_LEN;
	memcpy( digest_salt , _digest , _HASH_LEN );

	memcpy( x , vinegar , _V1 );

	unsigned succ = 0;
	while( !succ ) {
		if( 512 == time ) break;

		gf256v_rand( salt , _SALT_BYTE );  /// line 8
		sha2_chain_msg( _z , _PUB_M_BYTE , digest_salt , _HASH_LEN+_SALT_BYTE ); /// line 9

		gf256v_add(_z,sk->vec_s,_PUB_M_BYTE);
		gf256mat_prod(y,sk->mat_s,_PUB_M_BYTE,_PUB_M,_z); /// line 10

		memcpy( temp_o1 , temp_vv1 , _O1 );
		gf256v_add( temp_o1 , y , _O1 );
		linear_solver_l1( x + _V1 , mat_l1 , temp_o1 );

		gen_l2_mat( mat_l2 , k , x );
		mpkc_pub_map_gf256_n_m( temp_o2 , k->l2_vv , x , _V2 , _O2 );
		gf256v_add( temp_o2 , y+_O1 , _O2 );
		succ = linear_solver_l2( x + _V2 , mat_l2 , temp_o2 );  /// line 13

		time ++;
	};

	gf256v_add(x,sk->vec_t,_PUB_N_BYTE);
	gf256mat_prod(w,sk->mat_t,_PUB_N_BYTE,_PUB_N,x);

	memset( signature , 0 , _SIGNATURE_BYTE );
	// return time;
	if( 256 <= time ) return -1;
	gf256v_add( signature , w , _PUB_N_BYTE );
	gf256v_add( signature + _PUB_N_BYTE , salt , _SALT_BYTE );
	return 0;
}

/// algorithm 8
int rainbow_verify( const uint8_t * digest , const uint8_t * signature , const uint8_t * pk )
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






#endif  /// _RAINBOW_256_
