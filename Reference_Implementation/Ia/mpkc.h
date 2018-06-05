
#ifndef _MPKC_H_
#define _MPKC_H_



#include "blas.h"

#include "string.h"

#include "rainbow_config.h"

#include "kptr.h"

#ifndef TERMS_QUAD_POLY
#define TERMS_QUAD_POLY(N) (((N)*(N+1)/2)+N)
#endif


#ifdef  __cplusplus
extern  "C" {
#endif



#define IDX_XSQ(i,n_var) (((2*(n_var)+1-i)*(i)/2)+n_var)

/// xi <= xj
#define IDX_QTERMS_REVLEX(xi,xj) ((xj)*(xj+1)/2 + (xi))



static inline
void mpkc_pub_map_gf16( uint8_t * z , kptr_t pk_mat , const uint8_t * w )
{
	uint8_t r[_PUB_M_BYTE]  = {0};
	uint8_t tmp[_PUB_M_BYTE] ;
	const unsigned n_var = _PUB_N;
	uint8_t x[_PUB_N]  = {0};
	for(unsigned i=0;i<n_var;i++) x[i] = gf16v_get_ele(w,i);

	unsigned linear_offset = 0;
	for(unsigned i=0;i<n_var;i++) {
		gf16v_madd( r , kptr_reify(pk_mat, linear_offset, _PUB_M_BYTE) , x[i] , _PUB_M_BYTE );
		linear_offset += _PUB_M_BYTE;
	}

	unsigned quad_offset = (_PUB_M_BYTE)*(_PUB_N);
	assert(linear_offset == quad_offset);
	for(unsigned i=0;i<n_var;i++) {
		memset( tmp , 0 , _PUB_M_BYTE );
		for(unsigned j=0;j<=i;j++) {
			gf16v_madd( tmp , kptr_reify(pk_mat, quad_offset, _PUB_M_BYTE) , x[j] , _PUB_M_BYTE );
			quad_offset += _PUB_M_BYTE;
		}
		gf16v_madd( r , tmp , x[i] , _PUB_M_BYTE );
	}
	gf256v_add( r , kptr_reify(pk_mat, quad_offset, _PUB_M_BYTE) , _PUB_M_BYTE );
	memcpy( z , r , _PUB_M_BYTE );
}



static inline
void mpkc_pub_map_gf16_n_m( uint8_t * z , kptr_t pk_mat , const uint8_t * w , unsigned n, unsigned m )
{
	assert( n <= 256 );
	assert( m <= 256 );
	uint8_t tmp[128] ;
	unsigned m_byte = (m+1)/2;
	uint8_t *r = z;
	//memset(r,0,m_byte);
	unsigned offset = 0;

	gf16mat_prod( r , kptr_reify(pk_mat, offset, n*m_byte) , m_byte , n , w );
	offset += n*m_byte;

	uint8_t _x[256] ;
	gf16v_split( _x , w , n );

	for(unsigned i=0;i<n;i++) {
		memset( tmp , 0 , m_byte );
		for(unsigned j=0;j<=i;j++) {
			gf16v_madd( tmp , kptr_reify(pk_mat, offset, m_byte) , _x[j] , m_byte );
			offset += m_byte;
		}
		gf16v_madd( r , tmp , _x[i] , m_byte );
	}
	gf256v_add( r , kptr_reify(pk_mat, offset, m_byte) , m_byte );
}



static inline
void mpkc_interpolate_gf16( kptr_t poly , void (*quad_poly)(void *,void *,const void *) , void * key )
{
	uint8_t tmp[_PUB_N_BYTE] = {0};
	uint8_t tmp_r0[_PUB_M_BYTE] = {0};
	uint8_t tmp_r1[_PUB_M_BYTE] = {0};
	uint8_t tmp_r2[_PUB_M_BYTE] = {0};
	const unsigned n_var = _PUB_N;

	uint8_t const_terms[_PUB_M_BYTE];
	gf256v_set_zero(tmp,_PUB_N_BYTE);
	quad_poly( const_terms , key , tmp );
	kptr_reify_lhs( poly , (TERMS_QUAD_POLY(_PUB_N)-1)*(_PUB_M_BYTE) , const_terms , _PUB_M_BYTE );

	for(unsigned i=0;i<n_var;i++) {
		gf256v_set_zero(tmp,_PUB_N_BYTE);
		gf16v_set_ele(tmp,i,1);
		quad_poly( tmp_r0 , key , tmp ); /// v + v^2
		gf256v_add( tmp_r0 , const_terms , _PUB_M_BYTE );

		memcpy( tmp_r2 , tmp_r0 , _PUB_M_BYTE );
		gf16v_mul_scalar( tmp_r0 , gf16_mul(2,2) , _PUB_M_BYTE ); /// 3v + 3v^2
		gf16v_set_ele(tmp,i,2);
		quad_poly( tmp_r1 , key , tmp ); /// 2v + 3v^2
		gf256v_add( tmp_r1 , const_terms , _PUB_M_BYTE );

		gf256v_add( tmp_r0 , tmp_r1 , _PUB_M_BYTE );     /// v
		gf256v_add( tmp_r2 , tmp_r0 , _PUB_M_BYTE);   /// v^2
		kptr_reify_lhs( poly , _PUB_M_BYTE*i , tmp_r0 , _PUB_M_BYTE );
		kptr_reify_lhs( poly , _PUB_M_BYTE*(n_var+IDX_QTERMS_REVLEX(i,i)) , tmp_r2 , _PUB_M_BYTE );
	}

	for(unsigned i=0;i<n_var;i++) {
		unsigned base_idx = n_var+IDX_QTERMS_REVLEX(0,i);
		memcpy( tmp_r1 , kptr_reify(poly , _PUB_M_BYTE*i , _PUB_M_BYTE ) , _PUB_M_BYTE );
		memcpy( tmp_r2 , kptr_reify(poly , _PUB_M_BYTE*(n_var+IDX_QTERMS_REVLEX(i,i)) , _PUB_M_BYTE ) , _PUB_M_BYTE );

		for(unsigned j=0;j<i;j++) {
			gf256v_set_zero(tmp,_PUB_N_BYTE);
			gf16v_set_ele(tmp,i,1);
			gf16v_set_ele(tmp,j,1);

			quad_poly( tmp_r0 , key , tmp ); /// v1 + v1^2 + v2 + v2^2 + v1v2
			gf256v_add( tmp_r0 , const_terms , _PUB_M_BYTE );

			gf256v_add( tmp_r0 , tmp_r1 , _PUB_M_BYTE );
			gf256v_add( tmp_r0 , tmp_r2 , _PUB_M_BYTE );
			gf256v_add( tmp_r0 , kptr_reify(poly,_PUB_M_BYTE*j,_PUB_M_BYTE) , _PUB_M_BYTE );
			gf256v_add( tmp_r0 , kptr_reify(poly,_PUB_M_BYTE*(n_var+IDX_QTERMS_REVLEX(j,j)),_PUB_M_BYTE) , _PUB_M_BYTE );

			kptr_reify_lhs( poly , _PUB_M_BYTE*(base_idx+j), tmp_r0 , _PUB_M_BYTE );
		}
	}
}




#ifdef  __cplusplus
}
#endif


#endif
