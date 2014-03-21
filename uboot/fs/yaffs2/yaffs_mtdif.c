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

/* XXX U-BOOT XXX */
#include <common.h>

const char *yaffs_mtdif_c_version =
    "$Id: yaffs_mtdif.c,v 1.19 2007/02/14 01:09:06 wookey Exp $";

#include "yportenv.h"


#include "yaffs_mtdif.h"

#include "linux/types.h"
#include "linux/time.h"
#include "linux/mtd/nand.h"


extern struct nand_chip nand_dev_desc[];

int erase_yaffs2_nand(unsigned int block, int nanddev);

static inline void translate_spare2oob(const yaffs_Spare *spare, __u8 *oob)
{
	oob[0] = spare->tagByte0;
	oob[1] = spare->tagByte1;
	oob[2] = spare->tagByte2;
	oob[3] = spare->tagByte3;
	oob[4] = spare->tagByte4;
	oob[5] = spare->tagByte5 & 0x3f;
	oob[5] |= spare->blockStatus == 'Y' ? 0: 0x80;
	oob[5] |= spare->pageStatus == 0 ? 0: 0x40;
	oob[6] = spare->tagByte6;
	oob[7] = spare->tagByte7;
}

static inline void translate_oob2spare(yaffs_Spare *spare, __u8 *oob)
{
	struct yaffs_NANDSpare *nspare = (struct yaffs_NANDSpare *)spare;
	spare->tagByte0 = oob[0];
	spare->tagByte1 = oob[1];
	spare->tagByte2 = oob[2];
	spare->tagByte3 = oob[3];
	spare->tagByte4 = oob[4];
	spare->tagByte5 = oob[5] == 0xff ? 0xff : oob[5] & 0x3f;
	spare->blockStatus = oob[5] & 0x80 ? 0xff : 'Y';
	spare->pageStatus = oob[5] & 0x40 ? 0xff : 0;
	spare->ecc1[0] = spare->ecc1[1] = spare->ecc1[2] = 0xff;
	spare->tagByte6 = oob[6];
	spare->tagByte7 = oob[7];
	spare->ecc2[0] = spare->ecc2[1] = spare->ecc2[2] = 0xff;

	nspare->eccres1 = nspare->eccres2 = 0; /* FIXME */
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
