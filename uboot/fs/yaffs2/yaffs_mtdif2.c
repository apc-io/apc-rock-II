/*
 * YAFFS: Yet Another Flash File System. A NAND-flash specific file system.
 *
 * Copyright (C) 2002-2007 Aleph One Ltd.
 *   for Toby Churchill Ltd and Brightstar Engineering
 *
 * Created by Charles Manning <charles@aleph1.co.uk>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

/* mtd interface for YAFFS2 */

/* XXX U-BOOT XXX */
#include <common.h>
#include "asm/errno.h"

const char *yaffs_mtdif2_c_version =
    "$Id: yaffs_mtdif2.c,v 1.17 2007/02/14 01:09:06 wookey Exp $";

#include "yportenv.h"


#include "yaffs_mtdif2.h"

#include "linux/types.h"
#include "linux/time.h"

#include "yaffs_packedtags2.h"

extern int write_yaffs2_nand(unsigned int addr, const __u8 * data);
extern int write_oob_yaffs2_nand(unsigned int addr, const __u8 * data, const __u8 * oob, unsigned int ooblen);
extern int read_yaffs2_nand(unsigned int addr, const __u8 * data);
extern int read_oob_yaffs2_nand(unsigned int addr, const __u8 * data, const __u8 * oob, unsigned int ooblen);
extern int nand_block_isbad(unsigned int block);
extern int nand_block_markbad(unsigned int block);
extern void print_nand_buf(unsigned char * rvalue, int length);

int nandmtd2_WriteChunkWithTagsToNAND(yaffs_Device * dev, int chunkInNAND,
				      const __u8 * data,
				      const yaffs_ExtendedTags * tags)
{
	int retval = 0;

	unsigned int addr = chunkInNAND * dev->nDataBytesPerChunk;
	yaffs_PackedTags2 pt;
	T(YAFFS_TRACE_MTD,
	  (TSTR
	   ("nandmtd2_WriteChunkWithTagsToNAND chunk %d data %p tags %p"
	    TENDSTR), chunkInNAND, data, tags));

	if (tags)
		yaffs_PackTags2(&pt, tags);
	if (data) {
		retval = write_oob_yaffs2_nand(addr, data, (__u8 *) &pt, sizeof(pt));
	} 
	if (retval == 0)
		return YAFFS_OK;
	else
		return YAFFS_FAIL;
}

int nandmtd2_ReadChunkWithTagsFromNAND(yaffs_Device * dev, int chunkInNAND,
				       __u8 * data, yaffs_ExtendedTags * tags)
{
	int retval = 0;

	int addr = chunkInNAND * dev->nDataBytesPerChunk;

	yaffs_PackedTags2 pt;

	T(YAFFS_TRACE_MTD,
	  (TSTR
	   ("nandmtd2_ReadChunkWithTagsFromNAND chunk %d data %p tags %p"
	    TENDSTR), chunkInNAND, data, tags));

	if (data && !tags)
		retval = read_yaffs2_nand(addr, data);
	else if (tags) {
		retval = read_oob_yaffs2_nand(addr, data, dev->spareBuffer, sizeof(pt));
	}

	memcpy(&pt, dev->spareBuffer, sizeof(pt));

	if (tags)
		yaffs_UnpackTags2(tags, &pt);

	if(tags && retval == -EBADMSG && tags->eccResult == YAFFS_ECC_RESULT_NO_ERROR)
		tags->eccResult = YAFFS_ECC_RESULT_UNFIXED;

	if (retval == 0)
		return YAFFS_OK;
	else
		return YAFFS_FAIL;
}

int nandmtd2_MarkNANDBlockBad(struct yaffs_DeviceStruct *dev, int blockNo)
{
	int retval;
	T(YAFFS_TRACE_MTD,
	  (TSTR("nandmtd2_MarkNANDBlockBad %d" TENDSTR), blockNo));

	retval =
	    nand_block_markbad(blockNo);

	if (retval == 0)
		return YAFFS_OK;
	else
		return YAFFS_FAIL;

}

int nandmtd2_QueryNANDBlock(struct yaffs_DeviceStruct *dev, int blockNo,
			    yaffs_BlockState * state, int *sequenceNumber)
{
	int retval;

	T(YAFFS_TRACE_MTD,
	  (TSTR("nandmtd2_QueryNANDBlock %d" TENDSTR), blockNo));
	retval =
	    nand_block_isbad(blockNo);

	if (retval) {
		T(YAFFS_TRACE_MTD, (TSTR("block is bad"TENDSTR)));
		*state = YAFFS_BLOCK_STATE_DEAD;
		*sequenceNumber = 0;
	} else {
		yaffs_ExtendedTags t;
		nandmtd2_ReadChunkWithTagsFromNAND(dev,
						   blockNo *
						   dev->nChunksPerBlock, NULL,
						   &t);

		if (t.chunkUsed) {
			*sequenceNumber = t.sequenceNumber;
			*state = YAFFS_BLOCK_STATE_NEEDS_SCANNING;
		} else {
			*sequenceNumber = 0;
			*state = YAFFS_BLOCK_STATE_EMPTY;
		}
	}
#if 0
	T(YAFFS_TRACE_MTD,
	  (TSTR("block is bad seq %d state %d" TENDSTR), *sequenceNumber,
	   *state));
#endif
	if (retval == 0)
		return YAFFS_OK;
	else
		return YAFFS_FAIL;
}
int nandmtd_EraseBlockInNAND(yaffs_Device * dev, int blockNumber)
{
        int retval = erase_yaffs2_nand(blockNumber, 0);

        if (retval == 0)
                return YAFFS_OK;
        else
                return YAFFS_FAIL;
}

int nandmtd_InitialiseNAND(yaffs_Device * dev)
{
        return YAFFS_OK;
}
  
