/*
 *  drivers/mtd/nandids.c
 *
 *  Copyright (C) 2002 Thomas Gleixner (tglx@linutronix.de)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */
#include <linux/module.h>
#include <linux/mtd/nand.h>
/*
*	Chip ID list
*
*	Name. ID code, pagesize, chipsize in MegaByte, eraseblock size,
*	options
*
*	Pagesize; 0, 256, 512
*	0	get this information from the extended chip ID
+	256	256 Byte page size
*	512	512 Byte page size
*/
struct nand_flash_dev nand_flash_ids[] = {

#ifdef CONFIG_MTD_NAND_MUSEUM_IDS
	{"NAND 1MiB 5V 8-bit",		0x6e, 256, 1, 0x1000, 0},
	{"NAND 2MiB 5V 8-bit",		0x64, 256, 2, 0x1000, 0},
	{"NAND 4MiB 5V 8-bit",		0x6b, 512, 4, 0x2000, 0},
	{"NAND 1MiB 3,3V 8-bit",	0xe8, 256, 1, 0x1000, 0},
	{"NAND 1MiB 3,3V 8-bit",	0xec, 256, 1, 0x1000, 0},
	{"NAND 2MiB 3,3V 8-bit",	0xea, 256, 2, 0x1000, 0},
	{"NAND 4MiB 3,3V 8-bit",	0xd5, 512, 4, 0x2000, 0},
	{"NAND 4MiB 3,3V 8-bit",	0xe3, 512, 4, 0x2000, 0},
	{"NAND 4MiB 3,3V 8-bit",	0xe5, 512, 4, 0x2000, 0},
	{"NAND 8MiB 3,3V 8-bit",	0xd6, 512, 8, 0x2000, 0},

	{"NAND 8MiB 1,8V 8-bit",	0x39, 512, 8, 0x2000, 0},
	{"NAND 8MiB 3,3V 8-bit",	0xe6, 512, 8, 0x2000, 0},
	{"NAND 8MiB 1,8V 16-bit",	0x49, 512, 8, 0x2000, NAND_BUSWIDTH_16},
	{"NAND 8MiB 3,3V 16-bit",	0x59, 512, 8, 0x2000, NAND_BUSWIDTH_16},
#endif

	{"NAND 16MiB 1,8V 8-bit",	0x33, 512, 16, 0x4000, 0},
	{"NAND 16MiB 3,3V 8-bit",	0x73, 512, 16, 0x4000, 0},
	{"NAND 16MiB 1,8V 16-bit",	0x43, 512, 16, 0x4000, NAND_BUSWIDTH_16},
	{"NAND 16MiB 3,3V 16-bit",	0x53, 512, 16, 0x4000, NAND_BUSWIDTH_16},

	{"NAND 32MiB 1,8V 8-bit",	0x35, 512, 32, 0x4000, 0},
	{"NAND 32MiB 3,3V 8-bit",	0x75, 512, 32, 0x4000, 0},
	{"NAND 32MiB 1,8V 16-bit",	0x45, 512, 32, 0x4000, NAND_BUSWIDTH_16},
	{"NAND 32MiB 3,3V 16-bit",	0x55, 512, 32, 0x4000, NAND_BUSWIDTH_16},

	{"NAND 64MiB 1,8V 8-bit",	0x36, 512, 64, 0x4000, 0},
	{"NAND 64MiB 3,3V 8-bit",	0x76, 512, 64, 0x4000, 0},
	{"NAND 64MiB 1,8V 16-bit",	0x46, 512, 64, 0x4000, NAND_BUSWIDTH_16},
	{"NAND 64MiB 3,3V 16-bit",	0x56, 512, 64, 0x4000, NAND_BUSWIDTH_16},

	{"NAND 128MiB 1,8V 8-bit",	0x78, 512, 128, 0x4000, 0},
	{"NAND 128MiB 1,8V 8-bit",	0x39, 512, 128, 0x4000, 0},
	{"NAND 128MiB 3,3V 8-bit",	0x79, 512, 128, 0x4000, 0},
	{"NAND 128MiB 1,8V 16-bit",	0x72, 512, 128, 0x4000, NAND_BUSWIDTH_16},
	{"NAND 128MiB 1,8V 16-bit",	0x49, 512, 128, 0x4000, NAND_BUSWIDTH_16},
	{"NAND 128MiB 3,3V 16-bit",	0x74, 512, 128, 0x4000, NAND_BUSWIDTH_16},
	{"NAND 128MiB 3,3V 16-bit",	0x59, 512, 128, 0x4000, NAND_BUSWIDTH_16},

	{"NAND 256MiB 3,3V 8-bit",	0x71, 512, 256, 0x4000, 0},

	/*
	 * These are the new chips with large page size. The pagesize and the
	 * erasesize is determined from the extended id bytes
	 */
#define LP_OPTIONS (NAND_SAMSUNG_LP_OPTIONS | NAND_NO_READRDY | NAND_NO_AUTOINCR)
#define LP_OPTIONS16 (LP_OPTIONS | NAND_BUSWIDTH_16)

	/* 512 Megabit */
	{"NAND 64MiB 1,8V 8-bit",	0xA2, 0,  64, 0, LP_OPTIONS},
	{"NAND 64MiB 1,8V 8-bit",	0xA0, 0,  64, 0, LP_OPTIONS},
	{"NAND 64MiB 3,3V 8-bit",	0xF2, 0,  64, 0, LP_OPTIONS},
	{"NAND 64MiB 3,3V 8-bit",	0xD0, 0,  64, 0, LP_OPTIONS},
	{"NAND 64MiB 3,3V 8-bit",	0xF0, 0,  64, 0, LP_OPTIONS},
	{"NAND 64MiB 1,8V 16-bit",	0xB2, 0,  64, 0, LP_OPTIONS16},
	{"NAND 64MiB 1,8V 16-bit",	0xB0, 0,  64, 0, LP_OPTIONS16},
	{"NAND 64MiB 3,3V 16-bit",	0xC2, 0,  64, 0, LP_OPTIONS16},
	{"NAND 64MiB 3,3V 16-bit",	0xC0, 0,  64, 0, LP_OPTIONS16},

	/* 1 Gigabit */
	{"NAND 128MiB 1,8V 8-bit",	0xA1, 0, 128, 0, LP_OPTIONS},
	{"NAND 128MiB 3,3V 8-bit",	0xF1, 0, 128, 0, LP_OPTIONS},
	{"NAND 128MiB 3,3V 8-bit",	0xD1, 0, 128, 0, LP_OPTIONS},
	{"NAND 128MiB 1,8V 16-bit",	0xB1, 0, 128, 0, LP_OPTIONS16},
	{"NAND 128MiB 3,3V 16-bit",	0xC1, 0, 128, 0, LP_OPTIONS16},
	{"NAND 128MiB 1,8V 16-bit",     0xAD, 0, 128, 0, LP_OPTIONS16},

	/* 2 Gigabit */
	{"NAND 256MiB 1,8V 8-bit",	0xAA, 0, 256, 0, LP_OPTIONS},
	{"NAND 256MiB 3,3V 8-bit",	0xDA, 0, 256, 0, LP_OPTIONS},
	{"NAND 256MiB 1,8V 16-bit",	0xBA, 0, 256, 0, LP_OPTIONS16},
	{"NAND 256MiB 3,3V 16-bit",	0xCA, 0, 256, 0, LP_OPTIONS16},

	/* 4 Gigabit */
	{"NAND 512MiB 1,8V 8-bit",	0xAC, 0, 512, 0, LP_OPTIONS},
	{"NAND 512MiB 3,3V 8-bit",	0xDC, 0, 512, 0, LP_OPTIONS},
	{"NAND 512MiB 1,8V 16-bit",	0xBC, 0, 512, 0, LP_OPTIONS16},
	{"NAND 512MiB 3,3V 16-bit",	0xCC, 0, 512, 0, LP_OPTIONS16},

	/* 8 Gigabit */
	{"NAND 1GiB 1,8V 8-bit",	0xA3, 0, 1024, 0, LP_OPTIONS},
	{"NAND 1GiB 3,3V 8-bit",	0xD3, 0, 1024, 0, LP_OPTIONS},
	{"NAND 1GiB 1,8V 16-bit",	0xB3, 0, 1024, 0, LP_OPTIONS16},
	{"NAND 1GiB 3,3V 16-bit",	0xC3, 0, 1024, 0, LP_OPTIONS16},

	/* 16 Gigabit */
	{"NAND 2GiB 1,8V 8-bit",	0xA5, 0, 2048, 0, LP_OPTIONS},
	{"NAND 2GiB 3,3V 8-bit",	0xD5, 0, 2048, 0, LP_OPTIONS},
	{"NAND 2GiB 1,8V 16-bit",	0xB5, 0, 2048, 0, LP_OPTIONS16},
	{"NAND 2GiB 3,3V 16-bit",	0xC5, 0, 2048, 0, LP_OPTIONS16},

	/* 32 Gigabit */
	{"NAND 4GiB 1,8V 8-bit",	0xA7, 0, 4096, 0, LP_OPTIONS},
	{"NAND 4GiB 3,3V 8-bit",	0xD7, 0, 4096, 0, LP_OPTIONS},
	{"NAND 4GiB 1,8V 16-bit",	0xB7, 0, 4096, 0, LP_OPTIONS16},
	{"NAND 4GiB 3,3V 16-bit",	0xC7, 0, 4096, 0, LP_OPTIONS16},

	/* 64 Gigabit */
	{"NAND 8GiB 1,8V 8-bit",	0xAE, 0, 8192, 0, LP_OPTIONS},
	{"NAND 8GiB 3,3V 8-bit",	0xDE, 0, 8192, 0, LP_OPTIONS},
	{"NAND 8GiB 1,8V 16-bit",	0xBE, 0, 8192, 0, LP_OPTIONS16},
	{"NAND 8GiB 3,3V 16-bit",	0xCE, 0, 8192, 0, LP_OPTIONS16},

	/* 128 Gigabit */
	{"NAND 16GiB 1,8V 8-bit",	0x1A, 0, 16384, 0, LP_OPTIONS},
	{"NAND 16GiB 3,3V 8-bit",	0x3A, 0, 16384, 0, LP_OPTIONS},
	{"NAND 16GiB 1,8V 16-bit",	0x2A, 0, 16384, 0, LP_OPTIONS16},
	{"NAND 16GiB 3,3V 16-bit",	0x4A, 0, 16384, 0, LP_OPTIONS16},

	/* 256 Gigabit */
	{"NAND 32GiB 1,8V 8-bit",	0x1C, 0, 32768, 0, LP_OPTIONS},
	{"NAND 32GiB 3,3V 8-bit",	0x3C, 0, 32768, 0, LP_OPTIONS},
	{"NAND 32GiB 1,8V 16-bit",	0x2C, 0, 32768, 0, LP_OPTIONS16},
	{"NAND 32GiB 3,3V 16-bit",	0x4C, 0, 32768, 0, LP_OPTIONS16},

	/* 512 Gigabit */
	{"NAND 64GiB 1,8V 8-bit",	0x1E, 0, 65536, 0, LP_OPTIONS},
	{"NAND 64GiB 3,3V 8-bit",	0x3E, 0, 65536, 0, LP_OPTIONS},
	{"NAND 64GiB 1,8V 16-bit",	0x2E, 0, 65536, 0, LP_OPTIONS16},
	{"NAND 64GiB 3,3V 16-bit",	0x4E, 0, 65536, 0, LP_OPTIONS16},

	/*
	 * Renesas AND 1 Gigabit. Those chips do not support extended id and
	 * have a strange page/block layout !  The chosen minimum erasesize is
	 * 4 * 2 * 2048 = 16384 Byte, as those chips have an array of 4 page
	 * planes 1 block = 2 pages, but due to plane arrangement the blocks
	 * 0-3 consists of page 0 + 4,1 + 5, 2 + 6, 3 + 7 Anyway JFFS2 would
	 * increase the eraseblock size so we chose a combined one which can be
	 * erased in one go There are more speed improvements for reads and
	 * writes possible, but not implemented now
	 */
	{"AND 128MiB 3,3V 8-bit",	0x01, 2048, 128, 0x4000,
	 NAND_IS_AND | NAND_NO_AUTOINCR |NAND_NO_READRDY | NAND_4PAGE_ARRAY |
	 BBT_AUTO_REFRESH
	},

	{NULL,}
};

#define MLC 1
#define SLC 0
#define WD8 0
struct WMT_nand_flash_dev WMT_nand_flash_ids[] = {
	
	
	{0xADD314A5, 4096, 2048,  64, 0x40000, 5, 125, 127, 0, WD8, 1, 0, 1, MLC,  4, 0x140A0C12, 0x64780046, 0, 0, 0, 0x00000000, "HY27UT088G2M-T(P)", LP_OPTIONS},
	{0xADF1801D, 1024, 2048,  64, 0x20000, 4,   0,   1, 0, WD8, 4, 0, 1, SLC,  4, 0x140A0F12, 0x64780064, 0, 0, 0, 0x00000000, "HY27UF081G2A", LP_OPTIONS},
	{0xADF1001D, 1024, 2048,  64, 0x20000, 4,   0,   1, 0, WD8, 4, 0, 1, SLC,  4, 0x140A0C12, 0x64780046, 0, 0, 0, 0x00000000, "H27U1G8F2BFR", LP_OPTIONS},
	{0xADD59425, 4096, 4096, 218, 0x80000, 5, 125, 127, 0, WD8, 1, 0, 1, MLC, 12, 0x140A0F12, 0x64780064, 0, 0, 0, 0x00000000, "HY27UAG8T2A", LP_OPTIONS},
	{0xADD7949A, 2048, 8192, 448,0x200000, 5,   0, 255, 0, WD8, 1, 0, 1, MLC, 24, 0x140A0C12, 0x64500064, 0, 0, 0, 0x74420000, "H27UBG8T2ATR", LP_OPTIONS},
	{0xADD5949A, 1024, 8192, 448,0x200000, 5,   0, 255, 0, WD8, 1, 0, 1, MLC, 24, 0x140A0C12, 0x64640064, 0, 0, 0, 0x74420000, "H27UAG8T2BTR-BC", LP_OPTIONS},
	{0xADD794DA, 2048, 8192, 640,0x200000, 5,   0, 255, 0, WD8, 1, 0, 1, MLC, 24, 0x140A0F12, 0x645000C8, 0, 1, 1, 0x74C30000, "H27UBG8T2BTR", LP_OPTIONS},
	{0xADD79491, 2048, 8192, 640,0x200000, 5,   0, 255, 0, WD8, 1, 0, 1, MLC, 40, 0x140A0F12, 0x647800C8, 0, 1, 1, 0x00000000, "H27UBG8T2CTR-F20", LP_OPTIONS},
	{0xADDE94DA, 4096, 8192, 640,0x200000, 5,   0, 255, 0, WD8, 1, 0, 1, MLC, 40, 0x140A0F12, 0x647800C8, 1, 1, 1, 0x00000000, "H27UCG8T2ATR-F20", LP_OPTIONS},
	{0xADDE94EB, 2048,16384,1280,0x400000, 5,   0, 255, 0, WD8, 1, 0, 1, MLC, 40, 0x140A0F12, 0x647800C8, 0, 1, 1, 0x74440000, "H27UCG8T2BTR-F20", LP_OPTIONS},

	{0xECD314A5, 4096, 2048,  64, 0x40000, 5, 127,   0, 0, WD8, 1, 0, 1, MLC,  4, 0x140A0C12, 0x64400064, 0, 0, 0, 0x00000000, "K9G8G08X0A", LP_OPTIONS},
	{0xECD59429, 4096, 4096, 218, 0x80000, 5, 127,   0, 0, WD8, 1, 0, 1, MLC, 12, 0x140A0F12, 0x64400064, 0, 0, 0, 0x00000000, "K9GAG08UXD", LP_OPTIONS},
	{0xECF10095, 1024, 2048,  64, 0x20000, 4,   0,   1, 0, WD8, 4, 0, 1, SLC,  4, 0x140a1412, 0x64400064, 0, 0, 0, 0x00000000, "K9F1G08U0B", LP_OPTIONS},
	{0xECD514B6, 4096, 4096, 128, 0x80000, 5, 127,   0, 0, WD8, 1, 0, 1, MLC,  4, 0x140A0C12, 0x64400064, 0, 0, 0, 0x00000000, "K9GAG08U0M", LP_OPTIONS},
	{0xECD755B6, 8192, 4096, 128, 0x80000, 5, 127,   0, 0, WD8, 1, 0, 1, MLC,  4, 0x140A0C12, 0x64400064, 0, 0, 0, 0x00000000, "K9LBG08U0M", LP_OPTIONS},
	{0xECD58472, 2048, 8192, 436,0x100000, 5,   0, 127, 0, WD8, 1, 0, 1, MLC, 24, 0x190A0F12, 0x6440012C, 0, 0, 0, 0x00000000, "K9GAG08U0E", LP_OPTIONS},
	{0xECD7947A, 4096, 8192, 448,0x100000, 5,   0, 127, 0, WD8, 1, 0, 1, MLC, 24, 0x190A0F12, 0x6478012C, 0, 0, 1, 0x54430000, "K9GBG08U0A", LP_OPTIONS},
	{0xECD59476, 2048, 8192, 448,0x100000, 5,   0, 127, 0, WD8, 1, 0, 1, MLC, 24, 0x140A0C12, 0x6478012C, 0, 0, 0, 0x00000000, "K9GAG08U0F", LP_OPTIONS},
	{0xECD7947E, 4096, 8192,1024,0x100000, 5,   0, 127, 0, WD8, 1, 0, 1, MLC, 40, 0x140B0B12, 0x6478012C, 0, 1, 1, 0x64440000, "K9GBG08U0B", LP_OPTIONS},
	{0xECDED57A, 8192, 8192, 640,0x100000, 5,   0, 127, 0, WD8, 1, 0, 1, MLC, 24, 0x140A0C12, 0x6478012C, 0, 0, 1, 0x58430000, "K9LCG08U0A", LP_OPTIONS},
	{0xECDED57E, 8192, 8192,1024,0x100000, 5,   0, 127, 0, WD8, 1, 0, 1, MLC, 40, 0x140B0C12, 0x6478012C, 0, 1, 1, 0x68440000, "K9LCG08U0B", LP_OPTIONS},

	{0x98D594BA, 4096, 4096, 218, 0x80000, 5,   0, 127, 0, WD8, 1, 0, 1, MLC, 12, 0x190F0F12, 0x64b40070, 0, 0, 0, 0x00000000, "TC58NVG4D1DTG0", LP_OPTIONS},
	{0x98D19015, 1024, 2048,  64, 0x20000, 4,   0,   1, 0, WD8, 4, 0, 1, SLC,  4, 0x140A0C12, 0x64B40011, 0, 0, 0, 0x00000000, "TC58NVG0S3ETA00", LP_OPTIONS},
	{0x98D59432, 2048, 8192, 448,0x100000, 5,   0, 127, 0, WD8, 1, 0, 1, MLC, 24, 0x140A0C12, 0x64B40084, 0, 0, 0, 0x00000000, "TC58NVG4D2FTA00", LP_OPTIONS},
	{0x98D58432, 2048, 8192, 640,0x100000, 5,   0, 127, 0, WD8, 1, 0, 1, MLC, 40, 0x190A0F12, 0x64B4012C, 0, 1, 1, 0x72560000, "TC58NVG4D2HTA00", LP_OPTIONS},
	{0x98DE8493, 2048,16384,1280,0x400000, 5,   0, 255, 0, WD8, 1, 0, 1, MLC, 40, 0x14070A12, 0x64B4012C, 0, 1, 1, 0x72570000, "TC58NVG6DCJTA00", LP_OPTIONS},
	{0x98D79432, 4096, 8192, 448,0x100000, 5,   0, 127, 0, WD8, 1, 0, 1, MLC, 24, 0x140A0C12, 0x64FF0078, 0, 0, 0, 0x76550000, "TC58NVG5D2FTAI0", LP_OPTIONS},
	{0x98D79432, 4096, 8192, 640,0x100000, 5,   0, 127, 0, WD8, 1, 0, 1, MLC, 40, 0x14070A12, 0x64B4012C, 0, 1, 1, 0x76560000, "TC58NVG5D2HTA00", LP_OPTIONS},
	{0x98D78493, 1024,16384,1280,0x400000, 5,   0, 255, 0, WD8, 1, 0, 1, MLC, 40, 0x14070A12, 0x6478012C, 1, 1, 1, 0x72570000, "TC58TEG5DCJTA00", LP_OPTIONS},
	{0x98DE9493, 2048,16384,1280,0x400000, 5,   0, 255, 0, WD8, 1, 0, 1, MLC, 40, 0x14070A12, 0x6478012C, 1, 1, 1, 0x76500000, "TC58TEG6DDKTA00", LP_OPTIONS},

	{0x2C88044B, 4096, 8192, 448,0x200000, 5,   0, 255, 0, WD8, 1, 0, 1, MLC, 24, 0x140B1210/*0x321E32C8*/, 0x647800C8, 0, 0, 0, 0x00000000, "MT29F64G08CBAAA", LP_OPTIONS},
	{0x2C88044B, 4096, 8192, 448,0x200000, 5,   0, 255, 0, WD8, 1, 0, 1, MLC, 24, 0x140B1210/*0x321E32C8*/, 0x647800C8, 0, 0, 0, 0x00000000, "MT29F128G08CFAAA", LP_OPTIONS},
	{0x2C68044A, 4096, 4096, 224,0x100000, 5,   0,   1, 0, WD8, 1, 0, 1, MLC, 12, 0x140B1210/*0x321E32C8*/, 0x647800C8, 0, 0, 0, 0xA9000000, "MT29F32G08CBACA", LP_OPTIONS},
	{0x2C64444B, 4096, 8192, 744,0x200000, 5,   0,   1, 0, WD8, 1, 0, 1, MLC, 40, 0x140B1210/*0x321E32C8*/, 0x647800C8, 0, 1, 0, 0xA9000000, "MT29F64G08CBABA", LP_OPTIONS},
	{0x2C44444B, 2048, 8192, 744,0x200000, 5,   0,   1, 0, WD8, 1, 0, 1, MLC, 40, 0x140B1210/*0x321E32C8*/, 0x647800C8, 0, 1, 0, 0xA9000000, "MT29F32G08CBADA", LP_OPTIONS},

	{0x45DE9493, 2048,16384,1280,0x400000, 5,   0,   1, 0, WD8, 1, 0, 1, MLC, 40, 0x14070A12, 0x64B40140, 1, 1, 1, 0x76570000, "SDTNQGAMA-008G", LP_OPTIONS},
	{0x45D78493, 1024,16384,1280,0x400000, 5,   0,   1, 0, WD8, 1, 0, 1, MLC, 40, 0x14070A12, 0x64B40140, 1, 1, 1, 0x72570000, "SDTNQFAMA-004G", LP_OPTIONS},

	{0x8988244B, 4096, 8192, 448,0x200000, 5,   0,   1, 0, WD8, 1, 0, 1, MLC, 24, 0x10070A12, 0x64400046, 0, 0, 0, 0xA9000000, "JS29F64G08AAME1", LP_OPTIONS},
	{0x8988244B, 4096, 8192, 744,0x200000, 5,   0,   1, 0, WD8, 1, 0, 1, MLC, 40, 0x10070A12, 0x64B40046, 0, 0, 0, 0xA9840000, "JS29F64G08AAMF1", LP_OPTIONS},


	{0xC2F1801D, 1024, 2048,  64, 0x20000, 4,   0,   1, 0, WD8, 4, 0, 1, SLC,  4, 0x140A0F12, 0x64400064, 0, 0, 0, 0x00000000, "MX30LF1G08AA", LP_OPTIONS},

	{0x92F18095, 1024, 2048,  64, 0x20000, 4,   0,   1, 0, WD8, 4, 0, 1, SLC,  4, 0x140A0C12, 0x64400064, 0, 0, 0, 0x40000000, "PSU1GA(3/4)0HT", LP_OPTIONS},
	{0,}
	/*add new product item here.*/
};

struct nand_read_retry_param chip_table[] = {
#ifdef CONFIG_MTD_NAND_WMT
	//Hynix
	{
		.magic = "readretry", 
		.nand_id = 0xADD794DA,
		.nand_id_5th = 0x74C30000,
		.eslc_reg_num = 5, 
		.eslc_offset = {0xa0, 0xa1, 0xb0, 0xb1, 0xc9},
		.eslc_set_value = {0x26, 0x26, 0x26, 0x26, 0x1}, 
		.retry_reg_num = 4,
		.retry_offset = {0xa7, 0xad, 0xae, 0xaf},
		.retry_value = {0, 0x6,0xa, 0x6, 0x0, 0x3, 0x7, 0x8, 0, 0x6, 0xd, 0xf, 0x0, 0x9, 0x14, 0x17, 0x0, 0x0, 0x1a, 0x1e, 0x0, 0x0, 0x20, 0x25}, 
		.total_try_times = 6, 
		.cur_try_times = -1,
		.set_parameter = hynix_set_parameter, 
		.get_parameter = hynix_get_parameter,
		.get_otp_table = NULL, 
		.retry = 0
	},
	{
		.magic = "readretry", 
		.nand_id = 0xADDE94DA, 
		.nand_id_5th = 0,
		.eslc_reg_num = 4, 
		.eslc_offset = {0xb0, 0xb1, 0xa0, 0xa1}, 
		.eslc_set_value = {0xa, 0xa, 0xa, 0xa}, 
		.retry_reg_num = 8, 
		.retry_offset = {0xcc, 0xbf, 0xaa, 0xab, 0xcd, 0xad, 0xae, 0xaf}, 
		.otp_len = 2,
		.otp_offset = {0xff, 0xcc},
		.otp_data = {0x40, 0x4d},
		.total_try_times = 7, 
		.cur_try_times = -1, 
		.set_parameter = hynix_set_parameter, 
		.get_parameter = hynix_get_parameter, 
		.get_otp_table = hynix_get_otp, 
		.retry = 0
	},
	{
		.magic = "readretry", 
		.nand_id = 0xADDE94EB, 
		.nand_id_5th = 0x74440000,
		.eslc_reg_num = 4, 
		.eslc_offset = {0xa0, 0xa1, 0xa7, 0xa8}, 
		.eslc_set_value = {0xa, 0xa, 0xa, 0xa}, 
		.retry_reg_num = 8, 
		.retry_offset = {0xb0, 0xb1, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7}, 
		.otp_len = 2,
		.otp_offset = {0xae, 0xb0},
		.otp_data = {0x00, 0x4d},
		.total_try_times = 7, 
		.cur_try_times = -1, 
		.set_parameter = hynix_set_parameter, 
		.get_parameter = hynix_get_parameter, 
		.get_otp_table = hynix_get_otp, 
		.retry = 0
	},
	{
		.magic = "readretry", 
		.nand_id = 0xADD79491, 
		.nand_id_5th = 0x0,
		.eslc_reg_num = 4, 
		.eslc_offset = {0xa0, 0xa1, 0xa7, 0xa8}, 
		.eslc_set_value = {0xa, 0xa, 0xa, 0xa}, 
		.retry_reg_num = 8, 
		.retry_offset = {0xb0, 0xb1, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7}, 
		.otp_len = 2,
		.otp_offset = {0xae, 0xb0},
		.otp_data = {0x00, 0x4d},
		.total_try_times = 7, 
		.cur_try_times = -1, 
		.set_parameter = hynix_set_parameter, 
		.get_parameter = hynix_get_parameter, 
		.get_otp_table = hynix_get_otp, 
		.retry = 0
	},
	//Toshiba
	{
		.magic = "readretry", 
		.nand_id = 0x98D58432, 
		.nand_id_5th = 0x72560000,
		.retry_reg_num = 4, 
		.retry_offset = {4, 5, 6, 7},
		.retry_value = {0, 0, 0, 0, 4, 4, 4, 4, 0x7c, 0x7c, 0x7c, 0x7c, 0x78, 0x78, 0x78, 0x78, 0x74, 0x74, 0x74, 0x74, 0x8, 0x8, 0x8, 0x8},
		.total_try_times = 6,
		.cur_try_times = 0,
		.set_parameter = toshiba_set_parameter,
		.get_parameter = toshiba_get_parameter,
		.retry = 0,
	},
	{
		.magic = "readretry", 
		.nand_id = 0x98DE8493, 
		.nand_id_5th = 0x72570000,
		.retry_reg_num = 4, 
		.retry_offset = {4, 5, 6, 7},
		.retry_value = {0, 0, 0, 0, 4, 4, 4, 4, 0x7c, 0x7c, 0x7c, 0x7c, 0x78, 0x78, 0x78, 0x78, 0x74, 0x74, 0x74, 0x74, 0x8, 0x8, 0x8, 0x8},
		.total_try_times = 6,
		.cur_try_times = 0,
		.set_parameter = toshiba_set_parameter,
		.get_parameter = toshiba_get_parameter,
		.retry = 0,
	},
	{
		.magic = "readretry", 
		.nand_id = 0x98DE9482, 
		.nand_id_5th = 0x72570000,
		.retry_reg_num = 4, 
		.retry_offset = {4, 5, 6, 7},
		.retry_value = {0, 0, 0, 0, 4, 4, 4, 4, 0x7c, 0x7c, 0x7c, 0x7c, 0x78, 0x78, 0x78, 0x78, 0x74, 0x74, 0x74, 0x74, 0x8, 0x8, 0x8, 0x8}, 
		.total_try_times = 6, 
		.cur_try_times = 0, 
		.set_parameter = toshiba_set_parameter, 
		.get_parameter = toshiba_get_parameter,
		.retry = 0,
	},
	{
		.magic = "readretry", 
		.nand_id = 0x98D79432, 
		.nand_id_5th = 0x76560000,
		.retry_reg_num = 4, 
		.retry_offset = {4, 5, 6, 7},
		.retry_value = {0, 0, 0, 0, 4, 4, 4, 4, 0x7c, 0x7c, 0x7c, 0x7c, 0x78, 0x78, 0x78, 0x78, 0x74, 0x74, 0x74, 0x74, 0x8, 0x8, 0x8, 0x8},
		.total_try_times = 6,
		.cur_try_times = 0,
		.set_parameter = toshiba_set_parameter,
		.get_parameter = toshiba_get_parameter,
		.retry = 0,
	},
	{
		.magic = "readretry", 
		.nand_id = 0x98D78493, 
		.nand_id_5th = 0x72570000,
		.retry_reg_num = 4, 
		.retry_offset = {4, 5, 6, 7},
		.retry_value = {0, 0, 0, 0, 4, 4, 4, 4, 0x7c, 0x7c, 0x7c, 0x7c, 0x78, 0x78, 0x78, 0x78, 0x74, 0x74, 0x74, 0x74, 0x8, 0x8, 0x8, 0x8},
		.total_try_times = 6,
		.cur_try_times = 0,
		.set_parameter = toshiba_set_parameter,
		.get_parameter = toshiba_get_parameter,
		.retry = 0,
	},

	//samsung
	{
		.magic = "readretry", 
		.nand_id = 0xECD7947E, 
		.nand_id_5th = 0x64440000,
		.retry_reg_num = 4,
		.retry_offset = {0xA7, 0xA4, 0xA5, 0xA6},
		.retry_def_value = {0, 0, 0, 0},
		.retry_value = {5, 0xA, 0, 0, 0x28, 0, 0xEC, 0xD8, 0xED, 0xF5, 0xED, 0xE6, 0xA, 0xF, 5, 0,
										0xF, 0xA, 0xFB, 0xEC, 0xE8, 0xEF, 0xE8, 0xDC, 0xF1, 0xFB, 0xFE, 0xF0, 0xA, 0x0, 0xFB, 0xEC,
										0xD0, 0xE2, 0xD0, 0xC2, 0x14, 0xF, 0xFB, 0xEC, 0xE8, 0xFB, 0xE8, 0xDC, 0x1E, 0x14, 0xFB, 0xEC,
										0xFB, 0xFF, 0xFB, 0xF8, 0x7, 0xC, 0x2, 0},
		.total_try_times = 14,
		.cur_try_times = 0,
		.set_parameter = samsung_set_parameter,
		.get_parameter = samsung_get_parameter,
		.retry = 0,
	},
	{
		.magic = "readretry", 
		.nand_id = 0xECDED57E, 
		.nand_id_5th = 0x68440000,
		.retry_reg_num = 4,
		.retry_offset = {0xA7, 0xA4, 0xA5, 0xA6},
		.retry_def_value = {0, 0, 0, 0},
		.retry_value = {5, 0xA, 0, 0, 0x28, 0, 0xEC, 0xD8, 0xED, 0xF5, 0xED, 0xE6, 0xA, 0xF, 5, 0,
										0xF, 0xA, 0xFB, 0xEC, 0xE8, 0xEF, 0xE8, 0xDC, 0xF1, 0xFB, 0xFE, 0xF0, 0xA, 0x0, 0xFB, 0xEC,
										0xD0, 0xE2, 0xD0, 0xC2, 0x14, 0xF, 0xFB, 0xEC, 0xE8, 0xFB, 0xE8, 0xDC, 0x1E, 0x14, 0xFB, 0xEC,
										0xFB, 0xFF, 0xFB, 0xF8, 0x7, 0xC, 0x2, 0},
		.total_try_times = 14,
		.cur_try_times = 0,
		.set_parameter = samsung_set_parameter,
		.get_parameter = samsung_get_parameter,
		.retry = 0,
	},
	//Sandisk
	{
		.magic = "readretry", 
		.nand_id = 0x45DE9493, 
		.nand_id_5th = 0x76570000,
		.retry_reg_num = 3, 
		.retry_offset = {4, 5, 7},
		.retry_def_value = {0, 0, 0, 0xFF, 0xFF},
		.retry_value = {0xF0, 0, 0xF0, 0xE0, 0, 0xE0, 0xD0, 0, 0xD0, 0x10, 0, 0x10, 0x20, 0, 0x20, 0x30, 0, 0x30,
										0xC0, 0, 0xD0, 0x00, 0, 0x10, 0x00, 0, 0x20, 0x10, 0, 0x20, 0xB0, 0, 0xD0, 0xA0, 0, 0xD0,
										0x90, 0, 0xD0, 0xB0, 0, 0xC0, 0xA0, 0, 0xC0, 0x90, 0, 0xC0,//lower page retry parameter
										0x00, 0xF0, 0, 0x0F, 0xE0, 0, 0x0F, 0xD0, 0, 0x0E, 0xE0, 0, 0x0E, 0xD0, 0, 0x0D, 0xF0, 0,
										0x0D, 0xE0, 0, 0x0D, 0xD0, 0, 0x01, 0x10, 0, 0x02, 0x20, 0, 0x02, 0x10, 0, 0x03, 0x20, 0,
										0x0F, 0x00, 0, 0x0E, 0xF0, 0, 0x0D, 0xC0, 0, 0x0F, 0xF0, 0, 0x01, 0x00, 0, 0x02, 0x00, 0,
										0x0D, 0xB0, 0, 0x0C, 0xA0, 0},//upper page retry parameter
		.otp_len = 9,
		.otp_offset = {0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xA, 0xB, 0xC},
		.otp_data = {0, 0, 0, 0, 0, 0, 0, 0, 0},
		.total_try_times = 0x1410,//bit15~8 for upper page, bit7~0 for lower page
		.cur_try_times = -1,
		.set_parameter = sandisk_set_parameter,
		.get_parameter = sandisk_get_parameter,
		.retry = 0,
	},
	{
		.magic = "readretry",
		.nand_id = 0x45D78493,
		.nand_id_5th = 0x72570000,
		.retry_reg_num = 3,
		.retry_offset = {4, 5, 7},
		.retry_def_value = {0, 0, 0, 0xFF, 0xFF},
		.retry_value = {0xF0, 0, 0xF0, 0xE0, 0, 0xE0, 0xD0, 0, 0xD0, 0x10, 0, 0x10, 0x20, 0, 0x20, 0x30, 0, 0x30,
										0xC0, 0, 0xD0, 0x00, 0, 0x10, 0x00, 0, 0x20, 0x10, 0, 0x20, 0xB0, 0, 0xD0, 0xA0, 0, 0xD0,
										0x90, 0, 0xD0, 0xB0, 0, 0xC0, 0xA0, 0, 0xC0, 0x90, 0, 0xC0,//lower page retry parameter
										0x00, 0xF0, 0, 0x0F, 0xE0, 0, 0x0F, 0xD0, 0, 0x0E, 0xE0, 0, 0x0E, 0xD0, 0, 0x0D, 0xF0, 0,
										0x0D, 0xE0, 0, 0x0D, 0xD0, 0, 0x01, 0x10, 0, 0x02, 0x20, 0, 0x02, 0x10, 0, 0x03, 0x20, 0,
										0x0F, 0x00, 0, 0x0E, 0xF0, 0, 0x0D, 0xC0, 0, 0x0F, 0xF0, 0, 0x01, 0x00, 0, 0x02, 0x00, 0,
										0x0D, 0xB0, 0, 0x0C, 0xA0, 0},//upper page retry parameter
		.otp_len = 9,
		.otp_offset = {0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xA, 0xB, 0xC},
		.otp_data = {0, 0, 0, 0, 0, 0, 0, 0, 0},
		.total_try_times = 0x1410,//bit15~8 for upper page, bit7~0 for lower page
		.cur_try_times = -1,
		.set_parameter = sandisk_set_parameter,
		.get_parameter = sandisk_get_parameter,
		.retry = 0,
	},
	
	//Micron
	{
		.magic = "readretry", 
		.nand_id = 0x2C64444B, 
		.nand_id_5th = 0xA9000000,
		.retry_reg_num = 1, 
		.retry_offset = {0x89},
		.retry_def_value = {0},
		.retry_value = {1, 2, 3, 4, 5, 6, 7},
		.total_try_times = 7,
		.cur_try_times = 0,
		.set_parameter = micron_set_parameter,
		.get_parameter = micron_get_parameter,
		.retry = 0,
	},
	{
		.magic = "readretry", 
		.nand_id = 0x2C44444B, 
		.nand_id_5th = 0xA9000000,
		.retry_reg_num = 1, 
		.retry_offset = {0x89},
		.retry_def_value = {0},
		.retry_value = {1, 2, 3, 4, 5, 6, 7},
		.total_try_times = 7,
		.cur_try_times = 0,
		.set_parameter = micron_set_parameter,
		.get_parameter = micron_get_parameter,
		.retry = 0,
	},
#endif
	{
		.nand_id = 0,
		.nand_id_5th = 0,
	}
};



/*
*	Manufacturer ID list
*/
struct nand_manufacturers nand_manuf_ids[] = {
	{NAND_MFR_TOSHIBA, "Toshiba"},
	{NAND_MFR_SAMSUNG, "Samsung"},
	{NAND_MFR_FUJITSU, "Fujitsu"},
	{NAND_MFR_NATIONAL, "National"},
	{NAND_MFR_RENESAS, "Renesas"},
	{NAND_MFR_STMICRO, "ST Micro"},
	{NAND_MFR_HYNIX, "Hynix"},
	{NAND_MFR_MICRON, "Micron"},
	{NAND_MFR_SANDISK, "Sandisk"},
	{NAND_MFR_AMD, "AMD"},
	{NAND_MFR_INTEL, "Intel"},
	{NAND_MFR_MACRONIX, "Macronix"},
	{NAND_MFR_MXIC, "Mxic"},
	{NAND_MFR_MIRA, "Mira"},
	{0x0, "Unknown"}
};

EXPORT_SYMBOL(nand_manuf_ids);
EXPORT_SYMBOL(nand_flash_ids);
EXPORT_SYMBOL(WMT_nand_flash_ids);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Thomas Gleixner <tglx@linutronix.de>");
MODULE_DESCRIPTION("Nand device & manufacturer IDs");
