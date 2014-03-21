/*++ 
Copyright (c) 2010 WonderMedia Technologies, Inc.

This program is free software: you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free Software 
Foundation, either version 2 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details. You
should have received a copy of the GNU General Public License along with this
program. If not, see http://www.gnu.org/licenses/>.

WonderMedia Technologies, Inc.
10F, 529, Chung-Cheng Road, Hsin-Tien, Taipei 231, R.O.C.
--*/
/*
 *  u-boot/include/linux/mtd/nand_ids.h
 *
 *  Copyright (c) 2000 David Woodhouse <dwmw2@mvhi.com>
 *                     Steven J. Hill <sjhill@cotw.com>
 *
 * $Id: nand_ids.h,v 1.1 2000/10/13 16:16:26 mdeans Exp $
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 *  Info:
 *   Contains standard defines and IDs for NAND flash devices
 *
 *  Changelog:
 *   01-31-2000 DMW     Created
 *   09-18-2000 SJH     Moved structure out of the Disk-On-Chip drivers
 *			so it can be used by other NAND flash device
 *			drivers. I also changed the copyright since none
 *			of the original contents of this file are specific
 *			to DoC devices. David can whack me with a baseball
 *			bat later if I did something naughty.
 *   10-11-2000 SJH     Added private NAND flash structure for driver
 *   2000-10-13 BE      Moved out of 'nand.h' - avoids duplication.
 */

#ifndef __LINUX_MTD_NAND_IDS_H
#define __LINUX_MTD_NAND_IDS_H
#if 0
static struct nand_flash_dev nand_flash_ids[] = {
	{"Toshiba TC5816BDC",     NAND_MFR_TOSHIBA, 0x64, 21, 1, 2, 0x1000, 0},
	{"Toshiba TC5832DC",      NAND_MFR_TOSHIBA, 0x6b, 22, 0, 2, 0x2000, 0},
	{"Toshiba TH58V128DC",    NAND_MFR_TOSHIBA, 0x73, 24, 0, 2, 0x4000, 0},
	{"Toshiba TC58256FT/DC",  NAND_MFR_TOSHIBA, 0x75, 25, 0, 2, 0x4000, 0},
	{"Toshiba TH58512FT",     NAND_MFR_TOSHIBA, 0x76, 26, 0, 3, 0x4000, 0},
	{"Toshiba TC58V32DC",     NAND_MFR_TOSHIBA, 0xe5, 22, 0, 2, 0x2000, 0},
	{"Toshiba TC58V64AFT/DC", NAND_MFR_TOSHIBA, 0xe6, 23, 0, 2, 0x2000, 0},
	{"Toshiba TC58V16BDC",    NAND_MFR_TOSHIBA, 0xea, 21, 1, 2, 0x1000, 0},
	{"Toshiba TH58100FT",     NAND_MFR_TOSHIBA, 0x79, 27, 0, 3, 0x4000, 0},
	{"Samsung KM29N16000",    NAND_MFR_SAMSUNG, 0x64, 21, 1, 2, 0x1000, 0},
	{"Samsung unknown 4Mb",   NAND_MFR_SAMSUNG, 0x6b, 22, 0, 2, 0x2000, 0},
	{"Samsung KM29U128T",     NAND_MFR_SAMSUNG, 0x73, 24, 0, 2, 0x4000, 0},
	{"Samsung KM29U256T",     NAND_MFR_SAMSUNG, 0x75, 25, 0, 2, 0x4000, 0},
	{"Samsung unknown 64Mb",  NAND_MFR_SAMSUNG, 0x76, 26, 0, 3, 0x4000, 0},
	{"Samsung KM29W32000",    NAND_MFR_SAMSUNG, 0xe3, 22, 0, 2, 0x2000, 0},
	{"Samsung unknown 4Mb",   NAND_MFR_SAMSUNG, 0xe5, 22, 0, 2, 0x2000, 0},
	{"Samsung KM29U64000",    NAND_MFR_SAMSUNG, 0xe6, 23, 0, 2, 0x2000, 0},
	{"Samsung KM29W16000",    NAND_MFR_SAMSUNG, 0xea, 21, 1, 2, 0x1000, 0},
	{"Samsung K9F5616Q0C",    NAND_MFR_SAMSUNG, 0x45, 25, 0, 2, 0x4000, 1},
	{"Samsung K9K1216Q0C",    NAND_MFR_SAMSUNG, 0x46, 26, 0, 3, 0x4000, 1},
	{0,}
};
#endif
#define MLC 1
#define SLC 0
#define WD8 0
static struct WMT_nand_flash_dev WMT_nand_flash_ids[] = {
	//Hynix
	{0xADD314A5, 4096, 2048,  64, 0x40000, 5, 125, 127, 0, WD8, 1, 0, 1, MLC,  4, 0x140A0C46, 0x64780046, 0, 0, 0, 0x00000000, 0x00000000, "HY27UT088G2M-T(P)"},
	{0xADF1801D, 1024, 2048,  64, 0x20000, 4,   0,   1, 0, WD8, 4, 0, 1, SLC,  4, 0x140A0F64, 0x64780064, 0, 0, 0, 0x00000000, 0x00000000, "HY27UF081G2A"},
	{0xADF1001D, 1024, 2048,  64, 0x20000, 4,   0,   1, 0, WD8, 4, 0, 1, SLC,  4, 0x140A0C46, 0x64780046, 0, 0, 0, 0x00000000, 0x00000000, "H27U1G8F2BFR"},
	{0xADD59425, 4096, 4096, 218, 0x80000, 5, 125, 127, 0, WD8, 1, 0, 1, MLC, 12, 0x140A0F64, 0x64780064, 0, 0, 0, 0x00000000, 0x00000000, "HY27UAG8T2A"},
	{0xADD7949A, 2048, 8192, 448,0x200000, 5,   0, 255, 0, WD8, 1, 0, 1, MLC, 24, 0x140A0C64, 0x64500064, 0, 0, 0, 0x74420000, 0x00000000, "H27UBG8T2ATR"},
	{0xADD5949A, 1024, 8192, 448,0x200000, 5,   0, 255, 0, WD8, 1, 0, 1, MLC, 24, 0x140A0C64, 0x64640064, 0, 0, 0, 0x74420000, 0x00000000, "H27UAG8T2BTR-BC"},
	{0xADD794DA, 2048, 8192, 640,0x200000, 5,   0, 255, 0, WD8, 1, 0, 1, MLC, 24, 0x10080AC8, 0x645000C8, 0, 1, 1, 0x74C30000, 0x00000000, "H27UBG8T2BTR"},
	{0xADD79491, 2048, 8192, 640,0x200000, 5,   0, 255, 0, WD8, 1, 0, 1, MLC, 40, 0x100608C8, 0x647800C8, 0, 1, 1, 0x00000000, 0x00000000, "H27UBG8T2CTR-F20"},
	{0xADDE94DA, 4096, 8192, 640,0x200000, 5,   0, 255, 0, WD8, 1, 0, 1, MLC, 40, 0x100608C8, 0x647800C8, 1, 1, 1, 0x00000000, 0x00000000, "H27UCG8T2ATR-F20"},
	{0xADDE94EB, 2048,16384,1280,0x400000, 5,   0, 255, 0, WD8, 1, 0, 1, MLC, 40, 0x100608C8, 0x647800C8, 0, 1, 1, 0x74440000, 0x00000000, "H27UCG8T2BTR-F20"},
	//Samsung
	{0xECD314A5, 4096, 2048,  64, 0x40000, 5, 127,   0, 0, WD8, 1, 0, 1, MLC,  4, 0x140A0C64, 0x64400064, 0, 0, 0, 0x00000000, 0x00000000, "K9G8G08X0A"},
	{0xECD59429, 4096, 4096, 218, 0x80000, 5, 127,   0, 0, WD8, 1, 0, 1, MLC, 12, 0x140A0F64, 0x64400064, 0, 0, 0, 0x00000000, 0x00000000, "K9GAG08UXD"},
	{0xECF10095, 1024, 2048,  64, 0x20000, 4,   0,   1, 0, WD8, 4, 0, 1, SLC,  4, 0x140a1464, 0x64400064, 0, 0, 0, 0x00000000, 0x00000000, "K9F1G08U0B"},
	{0xECD514B6, 4096, 4096, 128, 0x80000, 5, 127,   0, 0, WD8, 1, 0, 1, MLC,  4, 0x140A0C64, 0x64400064, 0, 0, 0, 0x00000000, 0x00000000, "K9GAG08U0M"},
	{0xECD755B6, 8192, 4096, 128, 0x80000, 5, 127,   0, 0, WD8, 1, 0, 1, MLC,  4, 0x140A0C64, 0x64400064, 0, 0, 0, 0x00000000, 0x00000000, "K9LBG08U0M"},
	{0xECD58472, 2048, 8192, 436,0x100000, 5,   0, 127, 0, WD8, 1, 0, 1, MLC, 24, 0x190A0FFF, 0x6440012C, 0, 0, 0, 0x00000000, 0x00000000, "K9GAG08U0E"},
	{0xECD7947A, 4096, 8192, 448,0x100000, 5,   0, 127, 0, WD8, 1, 0, 1, MLC, 24, 0x190A0FFF, 0x6478012C, 0, 0, 1, 0x54430000, 0x00003FFF, "K9GBG08U0A"},
	{0xECD59476, 2048, 8192, 448,0x100000, 5,   0, 127, 0, WD8, 1, 0, 1, MLC, 24, 0x140A0CFF, 0x6478012C, 0, 0, 0, 0x00000000, 0x00000000, "K9GAG08U0F"},
	{0xECD7947E, 4096, 8192,1024,0x100000, 5,   0, 127, 0, WD8, 1, 0, 1, MLC, 40, 0x140B0BFF, 0x6478012C, 0, 1, 1, 0x64440000, 0x00000000, "K9GBG08U0B"},
	{0xECDED57A, 8192, 8192, 640,0x100000, 5,   0, 127, 0, WD8, 1, 0, 1, MLC, 24, 0x140A0CFF, 0x6478012C, 0, 0, 1, 0x58430000, 0x00000000, "K9LCG08U0A"},
	{0xECDED57E, 8192, 8192,1024,0x100000, 5,   0, 127, 0, WD8, 1, 0, 1, MLC, 40, 0x140B0CFF, 0x6478012C, 0, 1, 1, 0x68440000, 0x00000000, "K9LCG08U0B"},
	//Toshiba
	{0x98D594BA, 4096, 4096, 218, 0x80000, 5,   0, 127, 0, WD8, 1, 0, 1, MLC, 12, 0x190F0F70, 0x64b40070, 0, 0, 0, 0x00000000, 0x00000000, "TC58NVG4D1DTG0"},
	{0x98D19015, 1024, 2048,  64, 0x20000, 4,   0,   1, 0, WD8, 4, 0, 1, SLC,  4, 0x140A0C11, 0x64B40011, 0, 0, 0, 0x00000000, 0x00000000, "TC58NVG0S3ETA00"},
	{0x98D59432, 2048, 8192, 448,0x100000, 5,   0, 127, 0, WD8, 1, 0, 1, MLC, 24, 0x140A0C84, 0x64B40084, 0, 0, 0, 0x00000000, 0x00000000, "TC58NVG4D2FTA00"},
	{0x98D58432, 2048, 8192, 640,0x100000, 5,   0, 127, 0, WD8, 1, 0, 1, MLC, 40, 0x190A0FFF, 0x64B4012C, 0, 1, 1, 0x72560000, 0x00000000, "TC58NVG4D2HTA00"},
	{0x98DE8493, 2048,16384,1280,0x400000, 5,   0, 255, 0, WD8, 1, 0, 1, MLC, 40, 0x10070AFF, 0x64B4012C, 0, 1, 1, 0x72570000, 0x00000000, "TC58NVG6DCJTA00"},
	{0x98D79432, 4096, 8192, 448,0x100000, 5,   0, 127, 0, WD8, 1, 0, 1, MLC, 24, 0x140A0C78, 0x64FF0078, 0, 0, 0, 0x76550000, 0x00000000, "TC58NVG5D2FTAI0"},
	{0x98D79432, 4096, 8192, 640,0x100000, 5,   0, 127, 0, WD8, 1, 0, 1, MLC, 40, 0x10070AFF, 0x64B4012C, 0, 1, 1, 0x76560000, 0x00000000, "TC58NVG5D2HTA00"},
	{0x98D78493, 1024,16384,1280,0x400000, 5,   0, 255, 0, WD8, 1, 0, 1, MLC, 40, 0x10070AFF, 0x6478012C, 0, 1, 1, 0x72570000, 0x00000000, "TC58TEG5DCJTA00"},
	{0x98DE9493, 2048,16384,1280,0x400000, 5,   0, 255, 0, WD8, 1, 0, 1, MLC, 40, 0x14070A12, 0x6478012C, 1, 1, 1, 0x76500000, 0x00000000, "TC58TEG6DDKTA00"},
	//Miron
	{0x2C88044B, 4096, 8192, 448,0x200000, 5,   0, 255, 0, WD8, 1, 0, 1, MLC, 24, 0x321E32C8, 0x647800C8, 0, 0, 0, 0x00000000, 0x00000000, "MT29F64G08CBAAA"},
	{0x2C88044B, 4096, 8192, 448,0x200000, 5,   0, 255, 0, WD8, 1, 0, 1, MLC, 24, 0x321E32C8, 0x647800C8, 0, 0, 0, 0x00000000, 0x00000000, "MT29F128G08CFAAA"},
	{0x2C68044A, 4096, 4096, 224,0x100000, 5,   0,   1, 0, WD8, 1, 0, 1, MLC, 12, 0x321E32C8, 0x647800C8, 0, 0, 0, 0x00000000, 0x00000000, "MT29F32G08CBACA"},
	{0x2C64444B, 4096, 8192, 744,0x200000, 5,   0,   1, 0, WD8, 1, 0, 1, MLC, 40, 0x321E32C8, 0x647800C8, 0, 1, 0, 0xA9000000, 0x00000000, "MT29F64G08CBABA"},
	{0x2C44444B, 2048, 8192, 744,0x200000, 5,   0,   1, 0, WD8, 1, 0, 1, MLC, 40, 0x321E32C8, 0x647800C8, 0, 1, 0, 0xA9000000, 0x00000000, "MT29F32G08CBADA"},
	//Sandisk
	{0x45DE9493, 2048,16384,1280,0x400000, 5,   0,   1, 0, WD8, 1, 0, 1, MLC, 40, 0x10070AFF, 0x64B4012C, 1, 1, 1, 0x76570000, 0x00000000, "SDTNQGAMA-008G"},
	{0x45D78493, 1024,16384,1280,0x400000, 5,   0,   1, 0, WD8, 1, 0, 1, MLC, 40, 0x10070AFF, 0x64B4012C, 1, 1, 1, 0x72570000, 0x00000000, "SDTNQFAMA-004G"},
	//Intel
	{0x8988244B, 4096, 8192, 448,0x200000, 5,   0,   1, 0, WD8, 1, 0, 1, MLC, 24, 0x10070A46, 0x64400046, 0, 0, 0, 0xA9000000, 0x00000000, "JS29F64G08AAME1"},
	{0x8988244B, 4096, 8192, 744,0x200000, 5,   0,   1, 0, WD8, 1, 0, 1, MLC, 40, 0x10070A46, 0x64B40046, 0, 0, 0, 0xA9840000, 0x00000000, "JS29F64G08AAMF1"},
	//Mxic
	{0xC2F1801D, 1024, 2048,  64, 0x20000, 4,   0,   1, 0, WD8, 4, 0, 1, SLC,  4, 0x140A0F64, 0x64400064, 0, 0, 0, 0x00000000, 0x00000000, "MX30LF1G08AA"},
	//Mira
	{0x92F18095, 1024, 2048,  64, 0x20000, 4,   0,   1, 0, WD8, 4, 0, 1, SLC,  4, 0x140A0C64, 0x64400064, 0, 0, 0, 0x40000000, 0x00000000, "PSU1GA(3/4)0HT"},
	{0,}
	/*add new product item here.*/
};
/* Nand Product Table          We take HYNIX_NF_HY27UF081G2A for example.
struct WMT_nand_flash_dev{
 DWORD dwFlashID;            //composed by 4 bytes of ID. For example:0xADF1801D
 DWORD dwBlockCount;      //block count of one chip. For example: 1024
 DWORD dwPageSize;       //page size. For example:2048(other value can be 512 or 4096)
 DWORD dwSpareSize;       //spare area size. For example:16(almost all kinds of nand is 16)
 DWORD dwBlockSize;       //block size = dwPageSize * PageCntPerBlock. For example:131072
 DWORD dwAddressCycle;      //address cycle 4 or 5
 DWORD dwBI0Position;      //BI0 page postion in block
 DWORD dwBI1Position;      //BI1 page postion in block
 DWORD dwBIOffset;       //BI offset in page 0 or 2048
 DWORD dwDataWidth;      //data with X8 = 0 or X16 = 1
 DWORD dwPageProgramLimit;     //chip can program PAGE_PROGRAM_LIMIT times within the same page
 DWORD dwSeqRowReadSupport;    //whether support sequential row read, 1 = support, 0 = not support
 DWORD dwSeqPageProgram;     //chip need sequential page program in a block. 1 = need
 DWORD dwNandType;       //MLC = 1 or SLC = 0
 DWORD dwECCBitNum;      //ECC bit number needed
 DWORD dwRWTimming;     //NFC Read/Write Pulse width and Read/Write hold time. default =0x12121010
 DWORD dwTadl;      		//NFC write TADL timeing config
 DWORD dwDDR;      		  //NFC support toshia ddr(toggle) mode = 1 not support = 0
 DWORD dwRetry;    		  //NFC Read Retry support = 1 no
 DWORD dwRdmz; 			//NFC Randomizer support = 1,
 DWORD dwFlashID2;      //composed by additional 2 bytes of ID from original 4 bytes.
 char ProductName[MAX_PRODUCT_NAME_LENGTH]; //product name. for example "HYNIX_NF_HY27UF081G2A"
};
*/

int hynix_set_parameter(struct nand_chip *nand, int mode, int def_mode);
int hynix_get_parameter(struct nand_chip *nand, int mode);
int hynix_get_otp(struct nand_chip *nand);
int toshiba_get_parameter(struct nand_chip *nand, int mode);
int toshiba_set_parameter(struct nand_chip *nand, int mode, int def);
int samsung_get_parameter(struct nand_chip *nand, int mode);
int samsung_set_parameter(struct nand_chip *nand, int mode, int def_mode);
int sandisk_get_parameter(struct nand_chip *nand, int mode);
int sandisk_set_parameter(struct nand_chip *nand, int total_try_times, int def_value);
int micron_get_parameter(struct nand_chip *nand, int mode);
int micron_set_parameter(struct nand_chip *nand, int mode, int def_mode);
struct nand_read_retry_param chip_table[] = {
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

	{
		.nand_id = 0,
		.nand_id_5th = 0,
	}
};  
#endif /* __LINUX_MTD_NAND_IDS_H */
