/*
 * (C) Copyright 2011
 * Texas Instruments, <www.ti.com>
 * Author: Vikram Pandita <vikram.pandita@ti.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <config.h>
#include <common.h>
#include <sparse.h>
#include <mmc.h>
#include <fastboot.h>

//#define SDEBUG   

#define SPARSE_HEADER_MAJOR_VER 1

#define SZ_128M                       0x08000000
#define SZ_512M                       0x20000000
#define SZ_640M                       0x28000000
#define SZ_768M                       0x30000000
#define SZ_896M                       0x38000000
extern  struct cmd_fastboot_interface priv;
int _unsparse(unsigned char init, unsigned char *source_input, u32 sector_input, u32 section_size,
	      unsigned mmcc, int (*WRITE)(unsigned mmcc, unsigned long sector, unsigned long len, unsigned char *src))			   
{
	static sparse_header_t *header = (void*)0;
	static u32 i = 0, outlen = 0, len_mmc = 0, sector = 0;
	static unsigned char *source;
	static u32 left;
   	static unsigned char *BOUNDARY = (void*)SZ_640M ;  /*512M+128M*/
	//printf("_unsparse: write to mmc slot[%d] @ %x ptn address, header->total_chunks=%d\n", mmcc, sector, header->total_chunks);
	if (init == 2) {
		BOUNDARY = (void*)0xffffffff; 		
	}
	printf("init:%d, source:0x%08X, sector:%d, section_size:%d, BOUNDARY:0x%x\n",
		init, source_input, sector_input, section_size, BOUNDARY);

	if (init >= 1) {
		i = 0, outlen = 0, len_mmc = 0;
		source = source_input; /* Only keep the 1st time source_input */		
		sector = sector_input;
		header = (void*) source;
		printf("total_blks:%d, blk_sz:%d(%d sectors)\n", 
			header->total_blks, 
			header->blk_sz, 
			(header->total_blks * (header->blk_sz / 512)));
		if ((header->total_blks * (header->blk_sz / 512)) > section_size ) {
			printf("sparse: section size %d sectors limit: exceeded\n",
					section_size);
			return 1;
		}
	
		if (header->magic != SPARSE_HEADER_MAGIC) {
			printf("sparse: bad magic\n");
			return 1;
		}
	
		if ((header->major_version != SPARSE_HEADER_MAJOR_VER) ||
		    (header->file_hdr_sz != sizeof(sparse_header_t)) ||
		    (header->chunk_hdr_sz != sizeof(chunk_header_t))) {
			printf("sparse: incompatible format\n");
			return 1;
		}
		/**
		 * backup the sparse header in 126M
		 */
		memcpy((void*)(SZ_128M-0x200000), header, sizeof(sparse_header_t));
		header = (void*) (SZ_128M-0x200000);
		/* todo: ensure image will fit */
	
		/* Skip the header now */
		source += header->file_hdr_sz;		
	}
	
	for (; i < header->total_chunks; i++) {
		unsigned int len = 0;
		int r;
		
		chunk_header_t *chunk = (void*) source;
		/* move to next chunk */
		source += sizeof(chunk_header_t);		
		
		if (chunk->chunk_type == CHUNK_TYPE_RAW) {
			/**
			 * +----------+-------------+
			 * |          |chunk_header | source
			 * +----------+-------------+
			 *
			 * +------------+-----------+
			 * |            |chunk_header | source
			 * +------------+-----------+			 
			 */
			 /*init source address eq or ge boundary*/
			if (source >=  BOUNDARY) {
				left = (unsigned int)(BOUNDARY -(unsigned char *) chunk);
				memcpy((void *)(SZ_128M - left), (void *)chunk, left);
				source = (unsigned char *)(SZ_128M - left);
				printf(" Case 1 chunk header source >=  BOUNDARY left :0x%x ,source:0x%x\n", left ,source);
				return 0;
			}
			/**
			 * +--+---------------------+
			 * |  |chunk_header | source|
			 * +--+---------------------+
			 */
			 /*chunk end eq boundary*/
			if ((source + (chunk->chunk_sz * header->blk_sz)) == BOUNDARY) {
				/////////////////////////////////////////////////
				len = chunk->chunk_sz * header->blk_sz;

				if (chunk->total_sz != (len + sizeof(chunk_header_t))) {
					printf(" 1 _unsparse: bad chunk size for chunk 0x%x, type Raw\n", i);
					return 1;
				}
				
				outlen += len/512;
				if (outlen > section_size) {
					printf("_unsparse: section size %d sector limit: exceeded\n", section_size);
					return 1;
				}
#ifdef SDEBUG
				printf("source eq boundary _unsparse: RAW chunk->chunk_sz=%x header->blk_sz=0x%x: write(sector=0x%x,len=0x%x),outlen=0x%x KB\n",
			    	chunk->chunk_sz, header->blk_sz, sector, len, outlen/2);
#endif
				len_mmc = len /512;
				
				if (len % 512)
					len_mmc++;  

				r = WRITE(mmcc, sector, len_mmc, source);
				if (r < 0) {
					printf("_unsparse: mmc write failed\n");
					return 1;
				}
			
				sector += len_mmc; 
				source +=  len;
				source = (unsigned char *)SZ_128M;
				i++;
				printf(" Case 2 data end = BOUNDARY left :0x%x ,source:0x%x\n", left ,source);
				return 0;	//success for next image
			}			 
			/**
			 *
			 * +--------+---------------+
			 * |        |chunk_header | source|
			 * +--------+---------------+			 
			 *			 
			 */
			 /*chunk data gt boundary */
			if (source + (chunk->chunk_sz * header->blk_sz) > BOUNDARY) {
				/////////////////////////////////////////////////
				len = chunk->chunk_sz * header->blk_sz;

				if (chunk->total_sz != (len + sizeof(chunk_header_t))) {
					printf(" 2 _unsparse: bad chunk size for chunk 0x%x, type Raw,chunk->total_sz :0x%x ,len :0x%x\n", i ,chunk->total_sz, len);
					return 1;
				}
				
				len = (unsigned int)((BOUNDARY - source) / header->blk_sz) * header->blk_sz;
				
				left = (unsigned int)(BOUNDARY - source) % header->blk_sz;

				outlen += len/512;
				if (outlen > section_size) {
					printf("_unsparse: section size %d MB limit: exceeded\n", section_size /(1024*1024));
					return 1;
				}
#ifdef SDEBUG
				printf("source over boundary _unsparse: RAW chunk->chunk_sz=0x%x header->blk_sz=0x%x: write(sector=0x%x,len=0x%x),outlen=0x%x KB ,i =0x%x\n",
			    	chunk->chunk_sz, header->blk_sz, sector, len, outlen/1024 ,i );
#endif
				len_mmc = len /512;
				
				r = WRITE(mmcc, sector, len_mmc, source);
				if (r < 0) {
					printf("_unsparse: mmc write failed\n");
					return 1;
				}
			
				sector += len_mmc; 
				chunk->chunk_sz -= (len / header->blk_sz);
				chunk->total_sz  -= len;
				memcpy((void *)(SZ_128M - sizeof(chunk_header_t) - left), (void *)chunk, sizeof(chunk_header_t));
				memcpy((void *)(SZ_128M - left), (void *)(source + len), left);
				source =(unsigned char *)( SZ_128M - sizeof(chunk_header_t) - left);
				chunk = (void *)source;
				printf(" Case 3 data >= BOUNDARY left :0x%x ,source:0x%x\n", left ,source);
				return 0;	//success for next image
			}
		}
	
		if (chunk->chunk_type == CHUNK_TYPE_DONT_CARE) {
			/**
			 * +----------+-------------+
			 * |          |chunk_header | source
			 * +----------+-------------+
			 *
			 * +------------+-----------+
			 * |            |chunk_header | source
			 * +------------+-----------+			 
			 */			
			if (source >= BOUNDARY) {
				left = (unsigned int)(BOUNDARY - (unsigned char *)chunk);
				memcpy((void *)(SZ_128M - left), (void *)chunk, left);
				source =(unsigned char *)( SZ_128M - left);
				printf("Case 4 x type source >= BOUNDARY left :0x%x ,source:0x%x\n", left ,source);				
				return 0;
			}
		}

		switch (chunk->chunk_type) {
		case CHUNK_TYPE_RAW:
			len = chunk->chunk_sz * header->blk_sz;

			if (chunk->total_sz != (len + sizeof(chunk_header_t))) {
				printf(" 3  _unsparse: bad RAW chunk size for chunk 0x%x, chunk->total_sz :0x%x ,len :0x%x\n", i, chunk->total_sz, len );
				return 1;
			}

			outlen += len/512;
			if (outlen > section_size) {
				printf("_unsparse: section size %d sector limit: exceeded\n", section_size);
				return 1;
			}
#ifdef SDEBUG
			printf("_unsparse: RAW chunk->chunk_sz=%x header->blk_sz=0x%x: write(sector=0x%x,len=0x%x),outlen=0x%x KB\n",
			       chunk->chunk_sz, header->blk_sz, sector, len, outlen/2);
#endif
			len_mmc = len /512;
			if (len % 512)
				len_mmc++;  

			r = WRITE(mmcc, sector, len_mmc, source);
			if (r < 0) {
				printf("_unsparse: mmc write failed\n");
				return 1;
			}
			
			sector += len_mmc; 
			source +=  len;
			break;

		case CHUNK_TYPE_DONT_CARE:
			if (chunk->total_sz != sizeof(chunk_header_t)) {
				printf(" _unsparse: bogus DONT CARE chunk\n");
				return 1;
			}
			len = chunk->chunk_sz * header->blk_sz;
#ifdef SDEBUG
			printf("_unsparse: DONT_CARE blk=%d bsz=%d: skip(sector=%d,len=%d)\n",
			       chunk->chunk_sz, header->blk_sz, sector, len);
#endif

			outlen += len/512;
			if (outlen > section_size) {
				printf("_unsparse: section size %d sector limit: exceeded\n", section_size);
				return 1;
			}
			sector += (len / 512);
			break;

		default:
			printf("_unsparse: unknown chunk ID %04x\n", chunk->chunk_type);
			return 1;
		}
	}

	return 0;
}

u8 do_unsparse(unsigned char init, unsigned char *source, u32 sector, u32 section_size, char *slot_no)
{
	unsigned mmcc = simple_strtoul(slot_no, NULL, 16);
	//printf("%s, slot_no:%x, mmcc:%x,sector:0x%x,section_size:0x%x \n",__FUNCTION__, slot_no, mmcc, sector, section_size);
		if (_unsparse(init, source, sector, section_size, mmcc, mmc_fb_write))
			return 1;
	return 0;
}
			
