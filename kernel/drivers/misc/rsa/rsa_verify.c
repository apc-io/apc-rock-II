/*
 *  The RSA public-key cryptosystem
 *
 *  Copyright (C) 2006-2011, Brainspark B.V.
 *
 *  This file is part of PolarSSL (http://www.polarssl.org)
 *  Lead Maintainer: Paul Bakker <polarssl_maintainer at polarssl.org>
 *
 *  All rights reserved.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
/*
 *  RSA was designed by Ron Rivest, Adi Shamir and Len Adleman.
 *
 *  http://theory.lcs.mit.edu/~rivest/rsapaper.pdf
 *  http://www.cacr.math.uwaterloo.ca/hac/about/chap8.pdf
 */

#include "rsa.h"
#include "base64.h"
#include "pem.h"
#include "asn1.h"
#include "x509.h"
//#include <stdlib.h>
//#include <stdio.h>
#include <linux/vmalloc.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/ioport.h>
#include <linux/init.h>
#include <asm/io.h>
#include <linux/mtd/partitions.h>
#include <linux/platform_device.h>
#include <mach/hardware.h>
#include <linux/delay.h>



static const unsigned char base64_dec_map[128] =
{
		127, 127, 127, 127, 127, 127, 127, 127, 127, 127,
		127, 127, 127, 127, 127, 127, 127, 127, 127, 127,
		127, 127, 127, 127, 127, 127, 127, 127, 127, 127,
		127, 127, 127, 127, 127, 127, 127, 127, 127, 127,
		127, 127, 127,  62, 127, 127, 127,  63,  52,  53,
		 54,  55,  56,  57,  58,  59,  60,  61, 127, 127,
		127,  64, 127, 127, 127,   0,   1,   2,   3,   4,
			5,   6,   7,   8,   9,  10,  11,  12,  13,  14,
		 15,  16,  17,  18,  19,  20,  21,  22,  23,  24,
		 25, 127, 127, 127, 127, 127, 127,  26,  27,  28,
		 29,  30,  31,  32,  33,  34,  35,  36,  37,  38,
		 39,  40,  41,  42,  43,  44,  45,  46,  47,  48,
		 49,  50,  51, 127, 127, 127, 127, 127
};

/*
 * Decode a base64-formatted buffer
 */
int base64_decode( unsigned char *dst, size_t *dlen,
									 const unsigned char *src, size_t slen )
{
		size_t i, j, n;
		unsigned long x;
		unsigned char *p;

		for( i = j = n = 0; i < slen; i++ )
		{
				if( ( slen - i ) >= 2 &&
						src[i] == '\r' && src[i + 1] == '\n' )
						continue;

				if( src[i] == '\n' )
						continue;

				if( src[i] == '=' && ++j > 2 )
						return( POLARSSL_ERR_BASE64_INVALID_CHARACTER );

				if( src[i] > 127 || base64_dec_map[src[i]] == 127 )
						return( POLARSSL_ERR_BASE64_INVALID_CHARACTER );

				if( base64_dec_map[src[i]] < 64 && j != 0 )
						return( POLARSSL_ERR_BASE64_INVALID_CHARACTER );

				n++;
		}

		if( n == 0 )
				return( 0 );

		n = ((n * 6) + 7) >> 3;

		if( *dlen < n )
		{
				*dlen = n;
				return( POLARSSL_ERR_BASE64_BUFFER_TOO_SMALL );
		}

	 for( j = 3, n = x = 0, p = dst; i > 0; i--, src++ )
	 {
				if( *src == '\r' || *src == '\n' )
						continue;

				j -= ( base64_dec_map[*src] == 64 );
				x  = (x << 6) | ( base64_dec_map[*src] & 0x3F );

				if( ++n == 4 )
				{
						n = 0;
						if( j > 0 ) *p++ = (unsigned char)( x >> 16 );
						if( j > 1 ) *p++ = (unsigned char)( x >>  8 );
						if( j > 2 ) *p++ = (unsigned char)( x       );
				}
		}

		*dlen = p - dst;

		return( 0 );
}

int pem_read_buffer( pem_context *ctx, const unsigned char *data, const unsigned char *pwd, size_t pwdlen, size_t *use_len )
{
		int ret;
		size_t len;
		unsigned char *buf;

		((void) pwd);
		((void) pwdlen);

		if( ctx == NULL )
				return( POLARSSL_ERR_PEM_INVALID_DATA );



		len = 0;
		ret = base64_decode( NULL, &len, data, strlen((char *)data) );

		if( ret == POLARSSL_ERR_BASE64_INVALID_CHARACTER )
				return( POLARSSL_ERR_PEM_INVALID_DATA + ret );

		if( ( buf = (unsigned char *) vmalloc( len ) ) == NULL )
				return( POLARSSL_ERR_PEM_MALLOC_FAILED );

		if( ( ret = base64_decode( buf, &len, data, strlen((char *)data) ) ) != 0 )
		{
				vfree( buf );
				return( POLARSSL_ERR_PEM_INVALID_DATA + ret );
		}


		ctx->buf = buf;
		ctx->buflen = len;
		//*use_len = s2 - data;

		return( 0 );
}

int asn1_get_len( unsigned char **p,
									const unsigned char *end,
									size_t *len )
{
		if( ( end - *p ) < 1 )
				return( POLARSSL_ERR_ASN1_OUT_OF_DATA );

		if( ( **p & 0x80 ) == 0 )
				*len = *(*p)++;
		else
		{
				switch( **p & 0x7F )
				{
				case 1:
						if( ( end - *p ) < 2 )
								return( POLARSSL_ERR_ASN1_OUT_OF_DATA );

						*len = (*p)[1];
						(*p) += 2;
						break;

				case 2:
						if( ( end - *p ) < 3 )
								return( POLARSSL_ERR_ASN1_OUT_OF_DATA );

						*len = ( (*p)[1] << 8 ) | (*p)[2];
						(*p) += 3;
						break;

				case 3:
						if( ( end - *p ) < 4 )
								return( POLARSSL_ERR_ASN1_OUT_OF_DATA );

						*len = ( (*p)[1] << 16 ) | ( (*p)[2] << 8 ) | (*p)[3];
						(*p) += 4;
						break;

				case 4:
						if( ( end - *p ) < 5 )
								return( POLARSSL_ERR_ASN1_OUT_OF_DATA );

						*len = ( (*p)[1] << 24 ) | ( (*p)[2] << 16 ) | ( (*p)[3] << 8 ) | (*p)[4];
						(*p) += 5;
						break;

				default:
						return( POLARSSL_ERR_ASN1_INVALID_LENGTH );
				}
		}

		if( *len > (size_t) ( end - *p ) )
				return( POLARSSL_ERR_ASN1_OUT_OF_DATA );

		return( 0 );
}

int asn1_get_tag( unsigned char **p,
									const unsigned char *end,
									size_t *len, int tag )
{
		if( ( end - *p ) < 1 )
				return( POLARSSL_ERR_ASN1_OUT_OF_DATA );

		if( **p != tag )
				return( POLARSSL_ERR_ASN1_UNEXPECTED_TAG );

		(*p)++;

		return( asn1_get_len( p, end, len ) );
}

int asn1_get_mpi( unsigned char **p,
									const unsigned char *end,
									mpi *X )
{
		int ret;
		size_t len;

		if( ( ret = asn1_get_tag( p, end, &len, ASN1_INTEGER ) ) != 0 )
				return( ret );

		ret = mpi_read_binary( X, *p, len );

		*p += len;

		return( ret );
}

static int x509_get_alg( unsigned char **p,
												 const unsigned char *end,
												 x509_buf *alg )
{
		int ret;
		size_t len;

		if( ( ret = asn1_get_tag( p, end, &len,
						ASN1_CONSTRUCTED | ASN1_SEQUENCE ) ) != 0 )
				return( POLARSSL_ERR_X509_CERT_INVALID_ALG + ret );

		end = *p + len;
		alg->tag = **p;

		if( ( ret = asn1_get_tag( p, end, &alg->len, ASN1_OID ) ) != 0 )
				return( POLARSSL_ERR_X509_CERT_INVALID_ALG + ret );

		alg->p = *p;
		*p += alg->len;

		if( *p == end )
				return( 0 );

		/*
		 * assume the algorithm parameters must be NULL
		 */
		if( ( ret = asn1_get_tag( p, end, &len, ASN1_NULL ) ) != 0 )
				return( POLARSSL_ERR_X509_CERT_INVALID_ALG + ret );

		if( *p != end )
				return( POLARSSL_ERR_X509_CERT_INVALID_ALG +
								POLARSSL_ERR_ASN1_LENGTH_MISMATCH );

		return( 0 );
}

static int x509_get_pubkey( unsigned char **p,
														const unsigned char *end,
														x509_buf *pk_alg_oid,
														mpi *N, mpi *E )
{
		int ret, can_handle;
		size_t len;
		unsigned char *end2;

		if( ( ret = x509_get_alg( p, end, pk_alg_oid ) ) != 0 )
				return( ret );

		/*
		 * only RSA public keys handled at this time
		 */
		can_handle = 0;

		if( pk_alg_oid->len == 9 &&
				memcmp( pk_alg_oid->p, OID_PKCS1_RSA, 9 ) == 0 )
				can_handle = 1;

		if( pk_alg_oid->len == 9 &&
				memcmp( pk_alg_oid->p, OID_PKCS1, 8 ) == 0 )
		{
				if( pk_alg_oid->p[8] >= 2 && pk_alg_oid->p[8] <= 5 )
						can_handle = 1;

				if ( pk_alg_oid->p[8] >= 11 && pk_alg_oid->p[8] <= 14 )
						can_handle = 1;
		}

		if( pk_alg_oid->len == 5 &&
				memcmp( pk_alg_oid->p, OID_RSA_SHA_OBS, 5 ) == 0 )
				can_handle = 1;

		if( can_handle == 0 )
				return( POLARSSL_ERR_X509_UNKNOWN_PK_ALG );

		if( ( ret = asn1_get_tag( p, end, &len, ASN1_BIT_STRING ) ) != 0 )
				return( POLARSSL_ERR_X509_CERT_INVALID_PUBKEY + ret );

		if( ( end - *p ) < 1 )
				return( POLARSSL_ERR_X509_CERT_INVALID_PUBKEY +
								POLARSSL_ERR_ASN1_OUT_OF_DATA );

		end2 = *p + len;

		if( *(*p)++ != 0 )
				return( POLARSSL_ERR_X509_CERT_INVALID_PUBKEY );

		/*
		 *  RSAPublicKey ::= SEQUENCE {
		 *      modulus           INTEGER,  -- n
		 *      publicExponent    INTEGER   -- e
		 *  }
		 */
		if( ( ret = asn1_get_tag( p, end2, &len,
						ASN1_CONSTRUCTED | ASN1_SEQUENCE ) ) != 0 )
				return( POLARSSL_ERR_X509_CERT_INVALID_PUBKEY + ret );

		if( *p + len != end2 )
				return( POLARSSL_ERR_X509_CERT_INVALID_PUBKEY +
								POLARSSL_ERR_ASN1_LENGTH_MISMATCH );

		if( ( ret = asn1_get_mpi( p, end2, N ) ) != 0 ||
				( ret = asn1_get_mpi( p, end2, E ) ) != 0 )
				return( POLARSSL_ERR_X509_CERT_INVALID_PUBKEY + ret );

		if( *p != end )
				return( POLARSSL_ERR_X509_CERT_INVALID_PUBKEY +
								POLARSSL_ERR_ASN1_LENGTH_MISMATCH );

		return( 0 );
}


void pem_free( pem_context *ctx )
{
	if( ctx->buf )
		vfree( ctx->buf );

	if( ctx->info )
		vfree( ctx->info );

	memset(ctx, 0, sizeof(pem_context));
}

int x509parse_public_key( rsa_context *rsa, const unsigned char *key, size_t keylen )
{
		int ret;
		size_t len;
		unsigned char *p, *end;
		x509_buf alg_oid;
		pem_context pem;

		memset( &pem, 0, sizeof( pem_context ) );
		ret = pem_read_buffer( &pem,
						key, NULL, 0, &len );

		if( ret == 0 )
		{
				/*
				 * Was PEM encoded
				 */
				keylen = pem.buflen;

		}
		else if( ret != POLARSSL_ERR_PEM_NO_HEADER_PRESENT )
		{
				pem_free( &pem );
				return( ret );
		}

		p = ( ret == 0 ) ? pem.buf : (unsigned char *) key;
		end = p + keylen;

		/*
		 *  PublicKeyInfo ::= SEQUENCE {
		 *    algorithm       AlgorithmIdentifier,
		 *    PublicKey       BIT STRING
		 *  }
		 *
		 *  AlgorithmIdentifier ::= SEQUENCE {
		 *    algorithm       OBJECT IDENTIFIER,
		 *    parameters      ANY DEFINED BY algorithm OPTIONAL
		 *  }
		 *
		 *  RSAPublicKey ::= SEQUENCE {
		 *      modulus           INTEGER,  -- n
		 *      publicExponent    INTEGER   -- e
		 *  }
		 */

		if( ( ret = asn1_get_tag( &p, end, &len,
										ASN1_CONSTRUCTED | ASN1_SEQUENCE ) ) != 0 )
		{
				pem_free( &pem );
				rsa_free( rsa );
				return( POLARSSL_ERR_X509_CERT_INVALID_FORMAT + ret );
		}

		if( ( ret = x509_get_pubkey( &p, end, &alg_oid, &rsa->N, &rsa->E ) ) != 0 )
		{
				pem_free( &pem );
				rsa_free( rsa );
				return( POLARSSL_ERR_X509_KEY_INVALID_FORMAT + ret );
		}

#if 0
		if( ( ret = rsa_check_pubkey( rsa ) ) != 0 )
		{
#if defined(POLARSSL_PEM_C)
				pem_free( &pem );
#endif
				rsa_free( rsa );
				return( ret );
		}
#endif
		rsa->len = mpi_size( &rsa->N );



		pem_free( &pem );

		return( 0 );
}
/*
 * Initialize an RSA context
 */
void rsa_init( rsa_context *ctx, int padding, int hash_id)
{
	memset( ctx, 0, sizeof(rsa_context));

	ctx->padding = padding;
	ctx->hash_id = hash_id;
}


/*
 * Do an RSA public key operation
 */

int rsa_public( rsa_context *ctx,	const unsigned char *input,	unsigned char *output)
{
	int ret;
	size_t olen;
	mpi T;

	mpi_init( &T );

	MPI_CHK( mpi_read_binary( &T, input, ctx->len ) );

	if( mpi_cmp_mpi( &T, &ctx->N ) >= 0 )	{
		mpi_free( &T );
		return( POLARSSL_ERR_RSA_BAD_INPUT_DATA );
	}

	olen = ctx->len;
	MPI_CHK( mpi_exp_mod( &T, &T, &ctx->E, &ctx->N, &ctx->RN ) );
	MPI_CHK( mpi_write_binary( &T, output, olen ) );

cleanup:

	mpi_free( &T );

	if( ret != 0 )
			return( POLARSSL_ERR_RSA_PUBLIC_FAILED + ret );

	return( 0 );
}



/*
 * Free the components of an RSA key
 */
void rsa_free( rsa_context *ctx )
{
	mpi_free( &ctx->RQ ); mpi_free( &ctx->RP ); mpi_free( &ctx->RN );
	mpi_free( &ctx->QP ); mpi_free( &ctx->DQ ); mpi_free( &ctx->DP );
	mpi_free( &ctx->Q  ); mpi_free( &ctx->P  ); mpi_free( &ctx->D );
	mpi_free( &ctx->E  ); mpi_free( &ctx->N  );
}

int rsa_check(unsigned int pub_key_addr, unsigned int pub_key_size, unsigned int sig_addr, unsigned int sig_size, u8 *out_buf)
{

	size_t len;
	rsa_context rsa;
	unsigned char key[1024];
	unsigned char signature[1024];
	unsigned char verified[1024];
	int i;
	unsigned char *tmp;
	int ret, siglen;

	rsa_init(&rsa, RSA_PKCS_V15, 0);

	memcpy((void *)key, (void *)pub_key_addr, pub_key_size);
	key[pub_key_size] = '\0';
	len = pub_key_size;

	x509parse_public_key(&rsa, key, len);

	tmp = (unsigned char *) rsa.N.p;

	memcpy(signature, (void *)sig_addr, sig_size);
	signature[sig_size]='\0';

	ret = rsa_public(&rsa, signature, verified);

	if (ret){
		printk("Signature verify failed!!!\n");
		return -2;
	 }


	if ((verified[0] != 0) && (verified[1] != RSA_SIGN)) {
		printk("Signature verify failed!!!\n");
		return -1;
	}

	tmp = &verified[2];
	while (*tmp == 0xff) // skip padding
		tmp++;
	tmp++; // skip a terminator

	siglen = rsa.len -( tmp - verified);

	for (i = 0; i < siglen; i++) {
		out_buf[i] = tmp[i];
	}

	rsa_free(&rsa);
	return 0;
}
EXPORT_SYMBOL(rsa_check);