/*++ 
Copyright (c) 2010 WonderMedia Technologies, Inc.

This program is free software: you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free Software 
Foundation, either version 2 of the License, or (at your option) any later 
version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details. You
should have received a copy of the GNU General Public License along with this
program. If not, see http://www.gnu.org/licenses/>.

WonderMedia Technologies, Inc.
10F, 529, Chung-Cheng Road, Hsin-Tien, Taipei 231, R.O.C.
--*/
/*--
Revision History:
-------------------------------------------------------------------------------

--*/

#ifndef __CYPHER_H__
#define __CYPHER_H__

//#include "../../../board/wmt/include/global.h" 
#include "../../../common/wmt_display/hw/wmt_mmap.h"

#define IN
#define OUT
#define IO

/* Basic Define*/
#define REG32               *(volatile unsigned int *)
#define REG16               *(volatile unsigned short *)
#define REG8                *(volatile unsigned char *)
#define REG_GET32(addr)		( REG32(addr) )		/* Read 32 bits Register */
#define REG_GET16(addr)		( REG16(addr) )		/* Read 16 bits Register */
#define REG_GET8(addr)		( REG8(addr) )		/* Read  8 bits Register */
#define REG_SET32(addr, val)	( REG32(addr) = (val) )	/* Write 32 bits Register */
#define REG_SET16(addr, val)	( REG16(addr) = (val) )	/* Write 16 bits Register */
#define REG_SET8(addr, val)	( REG8(addr)  = (val) )	/* Write  8 bits Register */

/*
 *   Refer CYPHER Module ver. 0.01
 *
 */

#define CYPHER_REG_BASE				SECURITY_ENGINE_CFG_BASE_ADDR// 1KB  

//the CYPHER status 
#define CYPHER_OK	0
#define CYPHER_FAIL	1


/* 
 * Registers
 */
// MISC                                               
#define CYPHER_SRAM_WR_DATA_REG		CYPHER_REG_BASE + 0x0A8
#define CYPHER_SRAM_WR_ADDR_REG		CYPHER_REG_BASE + 0x0AC
#define CYPHER_SRAM_WR_ENABLE_REG		CYPHER_REG_BASE + 0x0B0
#define CYPHER_MODE_REG				CYPHER_REG_BASE + 0x0C0
#define CYPHER_SW_RESET_REG			CYPHER_REG_BASE + 0x0C4
#define CYPHER_CLK_ENABLE_REG			CYPHER_REG_BASE + 0x0C8
#define CYPHER_DMA_INT_ENABLE_REG		CYPHER_REG_BASE + 0x0CC
                                                      
// DMA                                                
#define CYPHER_DMA_START_REG			CYPHER_REG_BASE + 0x0D0
#define CYPHER_DMA_INT_STATUS_REG		CYPHER_REG_BASE + 0x0D4
#define CYPHER_DMA_SOUR_ADDR_REG		CYPHER_REG_BASE + 0x0E0
#define CYPHER_DMA_SOUR_RD_ADDR_REG	CYPHER_REG_BASE + 0x0E4
#define CYPHER_DMA_SOUR_RD_DWC_REG	CYPHER_REG_BASE + 0x0E8
#define CYPHER_DMA_DEST_ADDR_REG		CYPHER_REG_BASE + 0x0F0
#define CYPHER_DMA_DEST_RD_ADDR_REG	CYPHER_REG_BASE + 0x0F4
#define CYPHER_DMA_DEST_RD_DWC_REG	CYPHER_REG_BASE + 0x0F8


// SHA256
#define CYPHER_SHA256_START_REG		CYPHER_REG_BASE + 0x40
#define CYPHER_SHA256_DATA_LEN_REG		CYPHER_REG_BASE + 0x44


//---------------------------------------------------------------------------------------
// CYPHER define
//---------------------------------------------------------------------------------------
#define BUFFER_MAX 76802

#define PRD_BYTE_COUNT_MAX 65520		// have to meet 16 Bytes alignment
#define PRD_TABLE_BOUNDARY 8
#define PRD_BYTE_COUNT_BOUNDARY 16

#define DMA_BOUNDARY 16				// 16 bytes

#define DATA_MAX 307200


#define SHA256_TEXT_BOUNDARY 4
#define SHA256_OUTPUT_SIZE 32

// Encrypt/Decrypt mode
#define CYPHER_ENCRYPT 1
#define CYPHER_DECRYPT 0
#define CYPHER_ZERO 0x0L

// Cipher mode
#define CYPHER_SHA256_MODE 4
#define CYPHER_EXTERNAL_MODE 7

#define CYPHER_SHA256_CLK_ENABLE 0x2003

#define CYPHER_CLK_DISABLE 0

// Cipher DMA Interrupt
#define CYPHER_DMA_INT_ENABLE 1
#define CYPHER_DMA_INT_DISABLE 0

//---------------------------------------------------------------------------------------
// CYPHER Data structure
//---------------------------------------------------------------------------------------
typedef enum cypher_algo_e
{
	CYPHER_ALGO_SHA256 = 7 				// Secure Hash Algorithm 256
} cypher_algo_t;

#define CYPHER_ALGO_BASE_START   CYPHER_ALGO_SHA256
#define CYPHER_ALGO_BASE_END     CYPHER_ALGO_SHA256

// http://www-128.ibm.com/developerworks/tw/library/s-crypt02.html

#define SIZEOF(x) sizeof(x)
#define NO_OF( x )  ( SIZEOF( (x) ) / SIZEOF( (x)[0] ) )

typedef struct cypher_base_cfg_s
{
	cypher_algo_t algo_mode;
	int op_mode;
	u32 dec_enc;
	u32 input_addr;
	u32 output_addr;
	u32 text_length;
	u32 key_addr;
	u32 IV_addr;			// Initial Vector address for CBC, CTR and OFB mode of AES
	u32 INC;				// for CTR mode of AES
	u32 sha1_data_length;	// for SHA1 only
	u32 sha256_data_length;	// for SHA256 only
	int aes_hwkey;
} cypher_base_cfg_t;

//---------------------------------------------------------------------------------------
// Defined in the cypherif.c
//---------------------------------------------------------------------------------------
void cypher_initialization(void);


#endif // _CYPHER_H_


