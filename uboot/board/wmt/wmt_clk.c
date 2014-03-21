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


#include <common.h>
#include "include/wmt_clk.h"

#define PMC_BASE 0xD8130000
#define PMC_PLL 0xD8130200
#define PMC_CLK 0xD8130250
#define PLL_BUSY 0x7F0038
#define MAX_DF ((1<<7) - 1)
#define MAX_DR 1 //((1<<5) - 1)
#define MAX_DQ ((1<<2) - 1)
#define GET_DIVF(d) ((((d>>16)&MAX_DF)+1)*2)
#define GET_DIVR(d) (((d>>8)&MAX_DR)+1)
#define GET_DIVQ(d) (1<<(d&MAX_DQ))

#define SRC_FREQ25 25
#define SRC_FREQ24 24
#define SRC_FREQ SRC_FREQ24
/*#define debug_clk*/
static int dev_en_count[128] = {0};
struct pll_map pllmap[] = {//total 168
{126,		0x00290103},{129,		0x002A0103},{132,		0x002B0103},{135,		0x002C0103},{138,		0x002D0103},
{141,		0x002E0103},{144,		0x002F0103},{147,		0x00300103},{150,		0x00310103},{153,		0x00320103},
{156,		0x00330103},{159,		0x00340103},{162,		0x00350103},{165,		0x00360103},{168,		0x00370103},
{171,		0x00380103},{174,		0x00390103},{177,		0x003A0103},{180,		0x003B0103},{183,		0x003C0103},
{186,		0x003D0103},{189,		0x003E0103},{192,		0x003F0103},{195,		0x00400103},{198,		0x00410103},
{201,		0x00420103},{204,		0x00430103},{207,		0x00440103},{210,		0x00450103},{213,		0x00460103},
{216,		0x00470103},{219,		0x00480103},{222,		0x00490103},{225,		0x004A0103},{228,		0x004B0103},
{231,		0x004C0103},{234,		0x004D0103},{237,		0x004E0103},{240,		0x004F0103},{243,		0x00500103},
{246,		0x00510103},{249,		0x00520103},{252, 	0x00290102},{258, 	0x002A0102},{264, 	0x002B0102},
{270, 	0x002C0102},{276, 	0x002D0102},{282, 	0x002E0102},{288, 	0x002F0102},{294, 	0x00300102},
{300, 	0x00310102},{306, 	0x00320102},{312, 	0x00330102},{318, 	0x00340102},{324, 	0x00350102},
{330, 	0x00360102},{336, 	0x00370102},{342, 	0x00380102},{348, 	0x00390102},{354, 	0x003A0102},
{360, 	0x003B0102},{366, 	0x003C0102},{372, 	0x003D0102},{378, 	0x003E0102},{384, 	0x003F0102},
{390, 	0x00400102},{396, 	0x00410102},{402, 	0x00420102},{408, 	0x00430102},{414, 	0x00440102},
{420, 	0x00450102},{426, 	0x00460102},{432, 	0x00470102},{438, 	0x00480102},{444, 	0x00490102},
{450, 	0x004A0102},{456, 	0x004B0102},{462, 	0x004C0102},{468, 	0x004D0102},{474, 	0x004E0102},
{480, 	0x004F0102},{486, 	0x00500102},{492, 	0x00510102},{498, 	0x00520102},{504, 	0x00290101},
{516, 	0x002A0101},{528, 	0x002B0101},{540, 	0x002C0101},{552, 	0x002D0101},{564, 	0x002E0101},
{576, 	0x002F0101},{588, 	0x00300101},{600, 	0x00310101},{612, 	0x00320101},{624, 	0x00330101},
{636, 	0x00340101},{648, 	0x00350101},{660, 	0x00360101},{672, 	0x00370101},{684, 	0x00380101},
{696, 	0x00390101},{708, 	0x003A0101},{720, 	0x003B0101},{732, 	0x003C0101},{744, 	0x003D0101},
{756, 	0x003E0101},{768, 	0x003F0101},{780, 	0x00400101},{792, 	0x00410101},{804, 	0x00420101},
{816, 	0x00430101},{828, 	0x00440101},{840, 	0x00450101},{852, 	0x00460101},{864, 	0x00470101},
{876, 	0x00480101},{888, 	0x00490101},{900, 	0x004A0101},{912, 	0x004B0101},{924, 	0x004C0101},
{936, 	0x004D0101},{948, 	0x004E0101},{960, 	0x004F0101},{972, 	0x00500101},{984, 	0x00510101},
{996, 	0x00520101},{1008,	0x00290100},{1032,	0x002A0100},{1056,	0x002B0100},{1080,	0x002C0100},
{1104,	0x002D0100},{1128,	0x002E0100},{1152,	0x002F0100},{1176,	0x00300100},{1200,	0x00310100},
{1224,	0x00320100},{1248,	0x00330100},{1272,	0x00340100},{1296,	0x00350100},{1320,	0x00360100},
{1344,	0x00370100},{1368,	0x00380100},{1392,	0x00390100},{1416,	0x003A0100},{1440,	0x003B0100},
{1464,	0x003C0100},{1488,	0x003D0100},{1512,	0x003E0100},{1536,	0x003F0100},{1560,	0x00400100},
{1584,	0x00410100},{1608,	0x00420100},{1632,	0x00430100},{1656,	0x00440100},{1680,	0x00450100},
{1704,	0x00460100},{1728,	0x00470100},{1752,	0x00480100},{1776,	0x00490100},{1800,	0x004A0100},
{1824,	0x004B0100},{1848,	0x004C0100},{1872,	0x004D0100},{1896,	0x004E0100},{1920,	0x004F0100},
{1944,	0x00500100},{1968,	0x00510100},{1992,	0x00520100}//,2016,	0x00530100
};

//total 43+88+88+172 = 391
struct pll_map pllmapAll[] = {
{1008,	0x00290100},{504, 	0x00290101},{252, 	0x00290102},{126,	0x00290103},
{1032,	0x002A0100},{516, 	0x002A0101},{258, 	0x002A0102},{129,	0x002A0103},
{1056,	0x002B0100},{528, 	0x002B0101},{264, 	0x002B0102},{132,	0x002B0103},
{1080,	0x002C0100},{540, 	0x002C0101},{270, 	0x002C0102},{135,	0x002C0103},
{1104,	0x002D0100},{552, 	0x002D0101},{276, 	0x002D0102},{138,	0x002D0103},
{1128,	0x002E0100},{564, 	0x002E0101},{282, 	0x002E0102},{141,	0x002E0103},
{1152,	0x002F0100},{576, 	0x002F0101},{288, 	0x002F0102},{144,	0x002F0103},
{1176,	0x00300100},{588, 	0x00300101},{294, 	0x00300102},{147,	0x00300103},
{1200,	0x00310100},{600, 	0x00310101},{300, 	0x00310102},{150,	0x00310103},
{1224,	0x00320100},{612, 	0x00320101},{306, 	0x00320102},{153,	0x00320103},
{1248,	0x00330100},{624, 	0x00330101},{312, 	0x00330102},{156,	0x00330103},
{1272,	0x00340100},{636, 	0x00340101},{318, 	0x00340102},{159,	0x00340103},
{1296,	0x00350100},{648, 	0x00350101},{324, 	0x00350102},{162,	0x00350103},
{1320,	0x00360100},{660, 	0x00360101},{330, 	0x00360102},{165,	0x00360103},
{1344,	0x00370100},{672, 	0x00370101},{336, 	0x00370102},{168,	0x00370103},
{1368,	0x00380100},{684, 	0x00380101},{342, 	0x00380102},{171,	0x00380103},
{1392,	0x00390100},{696, 	0x00390101},{348, 	0x00390102},{174,	0x00390103},
{1416,	0x003A0100},{708, 	0x003A0101},{354, 	0x003A0102},{177,	0x003A0103},
{1440,	0x003B0100},{720, 	0x003B0101},{360, 	0x003B0102},{180,	0x003B0103},
{1464,	0x003C0100},{732, 	0x003C0101},{366, 	0x003C0102},{183,	0x003C0103},
{1488,	0x003D0100},{744, 	0x003D0101},{372, 	0x003D0102},{186,	0x003D0103},
{1512,	0x003E0100},{756, 	0x003E0101},{378, 	0x003E0102},{189,	0x003E0103},
{1536,	0x003F0100},{768, 	0x003F0101},{384, 	0x003F0102},{192,	0x003F0103},
{1560,	0x00400100},{780, 	0x00400101},{390, 	0x00400102},{195,	0x00400103},
{1584,	0x00410100},{792, 	0x00410101},{396, 	0x00410102},{198,	0x00410103},
{1608,	0x00420100},{804, 	0x00420101},{402, 	0x00420102},{201,	0x00420103},
{1632,	0x00430100},{816, 	0x00430101},{408, 	0x00430102},{204,	0x00430103},
{1656,	0x00440100},{828, 	0x00440101},{414, 	0x00440102},{207,	0x00440103},
{1680,	0x00450100},{840, 	0x00450101},{420, 	0x00450102},{210,	0x00450103},
{1704,	0x00460100},{852, 	0x00460101},{426, 	0x00460102},{213,	0x00460103},
{1728,	0x00470100},{864, 	0x00470101},{432, 	0x00470102},{216,	0x00470103},
{1752,	0x00480100},{876, 	0x00480101},{438, 	0x00480102},{219,	0x00480103},
{1776,	0x00490100},{888, 	0x00490101},{444, 	0x00490102},{222,	0x00490103},
{1800,	0x004A0100},{900, 	0x004A0101},{450, 	0x004A0102},{225,	0x004A0103},
{1824,	0x004B0100},{912, 	0x004B0101},{456, 	0x004B0102},{228,	0x004B0103},
{1848,	0x004C0100},{924, 	0x004C0101},{462, 	0x004C0102},{231,	0x004C0103},
{1872,	0x004D0100},{936, 	0x004D0101},{468, 	0x004D0102},{234,	0x004D0103},
{1896,	0x004E0100},{948, 	0x004E0101},{474, 	0x004E0102},{237,	0x004E0103},
{1920,	0x004F0100},{960, 	0x004F0101},{480, 	0x004F0102},{240,	0x004F0103},
{1944,	0x00500100},{972, 	0x00500101},{486, 	0x00500102},{243,	0x00500103},
{1968,	0x00510100},{984, 	0x00510101},{492, 	0x00510102},{246,	0x00510103},
{1992,	0x00520100},{996, 	0x00520101},{498, 	0x00520102},{249,	0x00520103},
{2016,	0x00530100},{1008, 	0x00530101},{504, 	0x00530102},{252,	0x00530103},


{1008,	0x00140000},	{504, 	0x00140001},	{252, 	0x00140002},	{126,	0x00140003},
{1056,	0x00150000},	{528, 	0x00150001},	{264, 	0x00150002},	{132,	0x00150003},
{1104,	0x00160000},	{552, 	0x00160001},	{276, 	0x00160002},	{138,	0x00160003},
{1152,	0x00170000},	{576, 	0x00170001},	{288, 	0x00170002},	{144,	0x00170003},
{1200,	0x00180000},	{600, 	0x00180001},	{300, 	0x00180002},	{150,	0x00180003},
{1248,	0x00190000},	{624, 	0x00190001},	{312, 	0x00190002},	{156,	0x00190003},
{1296,	0x001A0000},	{648, 	0x001A0001},	{324, 	0x001A0002},	{162,	0x001A0003},
{1344,	0x001B0000},	{672, 	0x001B0001},	{336, 	0x001B0002},	{168,	0x001B0003},
{1392,	0x001C0000},	{696, 	0x001C0001},	{348, 	0x001C0002},	{174,	0x001C0003},
{1440,	0x001D0000},	{720, 	0x001D0001},	{360, 	0x001D0002},	{180,	0x001D0003},
{1488,	0x001E0000},	{744, 	0x001E0001},	{372, 	0x001E0002},	{186,	0x001E0003},
{1536,	0x001F0000},	{768, 	0x001F0001},	{384, 	0x001F0002},	{192,	0x001F0003},
{1584,	0x00200000},	{792, 	0x00200001},	{396, 	0x00200002},	{198,	0x00200003},
{1632,	0x00210000},	{816, 	0x00210001},	{408, 	0x00210002},	{204,	0x00210003},
{1680,	0x00220000},	{840, 	0x00220001},	{420, 	0x00220002},	{210,	0x00220003},
{1728,	0x00230000},	{864, 	0x00230001},	{432, 	0x00230002},	{216,	0x00230003},
{1776,	0x00240000},	{888, 	0x00240001},	{444, 	0x00240002},	{222,	0x00240003},
{1824,	0x00250000},	{912, 	0x00250001},	{456, 	0x00250002},	{228,	0x00250003},
{1872,	0x00260000},	{936, 	0x00260001},	{468, 	0x00260002},	{234,	0x00260003},
{1920,	0x00270000},	{960, 	0x00270001},	{480, 	0x00270002},	{240,	0x00270003},
{1968,	0x00280000},	{984, 	0x00280001},	{492, 	0x00280002},	{246,	0x00280003},
{2016,	0x00290000},	{1008, 	0x00290001},	{504, 	0x00290002},	{252,	0x00290003},

{144,	0x00020000},	
{192,	0x00030000},
{240,	0x00040000},
{288,	0x00050000},	{144, 	0x00050001},
{336,	0x00060000},	{168, 	0x00060001},
{384,	0x00070000},	{192, 	0x00070001},
{432,	0x00080000},	{216, 	0x00080001},
{480,	0x00090000},	{240, 	0x00090001},
{528,	0x000A0000},	{264, 	0x000A0001},	{132, 	0x000A0002},
{576,	0x000B0000},	{288, 	0x000B0001},	{144, 	0x000B0002},
{624,	0x000C0000},	{312, 	0x000C0001},	{156, 	0x000C0002},
{672,	0x000D0000},	{336, 	0x000D0001},	{168, 	0x000D0002},
{720,	0x000E0000},	{360, 	0x000E0001},	{180, 	0x000E0002},
{768,	0x000F0000},	{384, 	0x000F0001},	{192, 	0x000F0002},
{816,	0x00100000},	{408, 	0x00100001},	{204, 	0x00100002},
{864,	0x00110000},	{432, 	0x00110001},	{216, 	0x00110002},
{912,	0x00120000},	{456, 	0x00120001},	{228, 	0x00120002},
{960,	0x00130000},	{480, 	0x00130001},	{240, 	0x00130002},

{144,	0x00050100},
{168,	0x00060100},
{192,	0x00070100},
{216,	0x00080100},
{240,	0x00090100},
{264,	0x000A0100},	{132, 	0x000A0101},	
{288,	0x000B0100},	{144, 	0x000B0101},	
{312,	0x000C0100},	{156, 	0x000C0101},	
{336,	0x000D0100},	{168, 	0x000D0101},	
{360,	0x000E0100},	{180, 	0x000E0101},	
{384,	0x000F0100},	{192, 	0x000F0101},	
{408,	0x00100100},	{204, 	0x00100101},	
{432,	0x00110100},	{216, 	0x00110101},	
{456,	0x00120100},	{228, 	0x00120101},	
{480,	0x00130100},	{240, 	0x00130101},	
{504,	0x00140100},	{252, 	0x00140101},	{126, 	0x00140102},
{528,	0x00150100},	{264, 	0x00150101},	{132, 	0x00150102},
{552,	0x00160100},	{276, 	0x00160101},	{138, 	0x00160102},
{576,	0x00170100},	{288, 	0x00170101},	{144, 	0x00170102},
{600,	0x00180100},	{300, 	0x00180101},	{150, 	0x00180102},
{624,	0x00190100},	{312, 	0x00190101},	{156, 	0x00190102},
{648,	0x001A0100},	{324, 	0x001A0101},	{162, 	0x001A0102},
{672,	0x001B0100},	{336, 	0x001B0101},	{168, 	0x001B0102},
{696,	0x001C0100},	{348, 	0x001C0101},	{174, 	0x001C0102},
{720,	0x001D0100},	{360, 	0x001D0101},	{180, 	0x001D0102},
{744,	0x001E0100},	{372, 	0x001E0101},	{186, 	0x001E0102},
{768,	0x001F0100},	{384, 	0x001F0101},	{192, 	0x001F0102},
{792,	0x00200100},	{396, 	0x00200101},	{198, 	0x00200102},
{816,	0x00210100},	{408, 	0x00210101},	{204, 	0x00210102},
{840,	0x00220100},	{420, 	0x00220101},	{210, 	0x00220102},
{864,	0x00230100},	{432, 	0x00230101},	{216, 	0x00230102},
{888,	0x00240100},	{444, 	0x00240101},	{222, 	0x00240102},
{912,	0x00250100},	{456, 	0x00250101},	{228, 	0x00250102},
{936,	0x00260100},	{468, 	0x00260101},	{234, 	0x00260102},
{960,	0x00270100},	{480, 	0x00270101},	{240, 	0x00270102},
{984,	0x00280100},	{492, 	0x00280101},	{246, 	0x00280102}
};

#ifndef ARRAYSIZE
#define ARRAYSIZE(a)           (sizeof(a) / sizeof(a[0]))
#endif

static void check_PLL_DIV_busy(void)
{
	while ((*(volatile unsigned int *)(PMC_BASE+0x18))&PLL_BUSY)
		;
}
#ifdef debug_clk
static void print_refer_count(void)
{
	int i;
	for (i = 0; i < 4; i++) {
		printf("clk cnt %d ~ %d: %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d\n",i*16,(i*16)+15,
		dev_en_count[i*16],dev_en_count[i*16+1],dev_en_count[i*16+2],dev_en_count[i*16+3],dev_en_count[i*16+4],dev_en_count[i*16+5],dev_en_count[i*16+6],dev_en_count[i*16+7],
		dev_en_count[i*16+8],dev_en_count[i*16+9],dev_en_count[i*16+10],dev_en_count[i*16+11],dev_en_count[i*16+12],dev_en_count[i*16+13],dev_en_count[i*16+14],dev_en_count[i*16+15]);
	}
}
#endif
static int disable_dev_clk(enum dev_id dev)
{
	int en_count;

	if (dev >= 128) {
		printf("device dev_id > 128\n");
		return -1;
	}

	#ifdef debug_clk
	print_refer_count();
	#endif

	en_count = dev_en_count[dev];
	if (en_count <= 1) {
		dev_en_count[dev] = en_count = 0;
		*(volatile unsigned int *)(PMC_CLK + 4*(dev/32))
		&= ~(1 << (dev - 32*(dev/32)));
	} else if (en_count > 1) {
		dev_en_count[dev] = (--en_count);
	}

	#ifdef debug_clk
	print_refer_count();
	#endif

	return en_count;
}

static int enable_dev_clk(enum dev_id dev)
{
	int en_count, tmp;

	if (dev > 128) {
		printf("device dev_id > 128\n");
		return -1;
	}

	#ifdef debug_clk
	printf("device dev_id = %d\n",dev);
	print_refer_count();
	#endif

	en_count = dev_en_count[dev];
	tmp = *(volatile unsigned int *)(PMC_CLK + 4*(dev/32));
	if (en_count <= 0 || !(tmp&(1 << (dev - 32*(dev/32))))) {
		dev_en_count[dev] = en_count = 1;
		*(volatile unsigned int *)(PMC_CLK + 4*(dev/32))
		|= 1 << (dev - 32*(dev/32));
	} else
		dev_en_count[dev] = (++en_count);

	#ifdef debug_clk
	print_refer_count();
	#endif

	return en_count;
}
/*
* PLLA return 0, PLLB return 1,
* PLLC return 2, PLLD return 3,
* PLLE return 4, PLLF return 5,
* PLLG return 6,
* device not found return -1.
* device has no divisor but has clock enable return -2.
*/
static int calc_pll_num(enum dev_id dev, int *div_offset)
{
	switch (dev) {
		case DEV_UART0:
		case DEV_UART1:
		case DEV_UART2:
		case DEV_UART3:		/*case DEV_UART4:		case DEV_UART5:*/
		case DEV_AHBB:
		case DEV_CIR:
		case DEV_SYS:		/* no use */
		case DEV_RTC:
		case DEV_UHDC:
		case DEV_PERM:	/* no use */
		case DEV_PDMA:	/* no use */
		case DEV_VID:
		*div_offset = 0;
		return -2;
		case DEV_SAE:
		*div_offset = 0x348;
		return 1;
		case DEV_C24MOUT:
		*div_offset = 0x3E4;
		return 1;
		case DEV_I2C0:
		*div_offset = 0x3A0;
		return 1;
		case DEV_I2C1:
		*div_offset = 0x3A4;
		return 1;
		case DEV_I2C2:
		*div_offset = 0x3A8;
		return 1;
		case DEV_I2C3:
		*div_offset = 0x3AC;
		return 1;
		case DEV_I2C4:
		*div_offset = 0x39C;
		return 1;
		case DEV_PWM:
		*div_offset = 0x350;
		return 1;
		case DEV_SPI0:
		*div_offset = 0x340;
		return 1;
		case DEV_SPI1:
		*div_offset = 0x344;
		return 1;
		case DEV_HDMILVDS:
		case DEV_SDTV:
		case DEV_HDMI:
		*div_offset = 0x36C;
		return 2;
		case DEV_DVO:
		case DEV_LVDS:
		*div_offset = 0x370;
		return 6;
		case DEV_CSI0:
		*div_offset = 0x380;
		return 1;
		case DEV_CSI1:
		*div_offset = 0x384;
		return 1;
		case DEV_MALI:
		*div_offset = 0x388;
		return 1;
		case DEV_DDRMC:
		*div_offset = 0x310;
		return 3;
		case DEV_WMTNA:
		*div_offset = 0x358;
		return 3;
		case DEV_CNMNA:
		*div_offset = 0x360;
		return 3;
		case DEV_WMTVDU:
		case DEV_JENC:
		case DEV_HDCE:
		case DEV_H264:
		case DEV_JDEC:
		case DEV_MSVD:
		case DEV_AMP:
		case DEV_VPU:
		case DEV_MBOX:
		*div_offset = 0x368;
		return 3;
		case DEV_CNMVDU:
		case DEV_VP8DEC:
		*div_offset = 0x38C;
		return 3;
		case DEV_NA12:
		case DEV_VPP:
		case DEV_GE:
		case DEV_DISP:
		case DEV_GOVRHD:
		case DEV_SCL444U:
		case DEV_GOVW:
		*div_offset = 0x35C;
		return 1;
		case DEV_PAXI:
		case DEV_DMA:
		*div_offset = 0x354;
		return 1;
		case DEV_L2C:
		*div_offset = 0x30C;
		return 0;
		case DEV_L2CAXI:
		*div_offset = 0x3B0;
		return 0;
		case DEV_AT:
		*div_offset = 0x3B4;
		return 0;
		case DEV_PERI:
		*div_offset = 0x3B8;
		return 0;
		case DEV_TRACE:
		*div_offset = 0x3BC;
		return 0;
		case DEV_L2CPAXI:
		*div_offset = 0x3C0;
		return 0;
		case DEV_DBG:
		*div_offset = 0x3D0;
		return 0;
		case DEV_NAND:
		*div_offset = 0x318;
		return 1;
		/*case DEV_NOR:
		*div_offset = 0x31C;
		return 1;*/
		case DEV_SDMMC0:
		*div_offset = 0x330;
		return 1;
		case DEV_SDMMC1:
		*div_offset = 0x334;
		return 1;
		case DEV_SDMMC2:
		*div_offset = 0x338;
		return 1;
		/*case DEV_SDMMC3:
		*div_offset = 0x33C;
		return 1;*/
		case DEV_ADC:
		*div_offset = 0x394;
		return 1;
		case DEV_HDMII2C: /* div 1~63 */
		*div_offset = 0x390;
		return 4;
		case DEV_SF:
		*div_offset = 0x314;
		return 1;
		case DEV_PCM0:
		*div_offset = 0x324;
		return 1;
		case DEV_PCM1:
		*div_offset = 0x328;
		return 1;
		case DEV_I2S:
		case DEV_ARF:
		case DEV_ARFP:
		*div_offset = 0x374;
		return 4;
		case DEV_ARM:
		*div_offset = 0x300;
			return 0;
		case DEV_AHB:
		*div_offset = 0x304;
			return 1;
		case DEV_APB:
		case DEV_SCC:
		case DEV_GPIO:
		*div_offset = 0x320;
			return 1;
		/*case DEV_ETHPHY:
		*div_offset = 25;
			return -2;*/
		default:
		return -1;
	}
}


/* if the clk of the dev is not enable, then enable it or fail */
int check_clk_enabled(enum dev_id dev, enum clk_cmd cmd) {
	unsigned int pmc_clk_en;
	if (dev < 128) {
		pmc_clk_en = *(volatile unsigned int *)(PMC_CLK + 4*(dev/32));
		if (!(pmc_clk_en & (1 << (dev - 32*(dev/32))))) {
			/*enable_dev_clk(dev);*/
			printf("dev[%d] clock disabled\n", dev);
			return -1;
		}
	}
	return 0;
}
/*
* get only PLL frequency not including divisor of each device
* PLL_NO : PLL number(0~6) means PLLA ~ PLLG
* return : return the frequence in MHz.
*/
int get_pll_freq(int PLL_NO)
{
	int freq;
	unsigned int tmp;

	if (PLL_NO < 0 || PLL_NO > 6) {
		printf("PLL NO(%d) is out of range\n", PLL_NO);
		return -1;
	}

	tmp = *(volatile unsigned int *)(PMC_PLL + 4*PLL_NO);
	freq = ((SRC_FREQ)*GET_DIVF(tmp)) / (GET_DIVR(tmp)*GET_DIVQ(tmp));

	return freq;
}

static int get_freq(enum dev_id dev, int *divisor) {

	unsigned int tmp, freq, div = 0, base = 1000000;
	int PLL_NO, j = 0, div_addr_offs;

	PLL_NO = calc_pll_num(dev, &div_addr_offs);
	if (PLL_NO == -1) {
		printf("device not found");
		return -1;
	}

	if (PLL_NO == -2)
		return div_addr_offs*1000000;

	*(volatile unsigned char *)(PMC_BASE + div_addr_offs + 2) = (dev == DEV_SDTV) ? 1 : 0;

	check_PLL_DIV_busy();

	div = *divisor = *(volatile unsigned char *)(PMC_BASE + div_addr_offs);
	//printf("div_addr_offs=0x%x PLL_NO=%d \n", div_addr_offs, PLL_NO);
	tmp = *(volatile unsigned int *)(PMC_PLL + 4*PLL_NO);
	
	if ((dev != DEV_SDMMC0) && (dev != DEV_SDMMC1) && (dev != DEV_SDMMC2)) {
		div = div&0x1F;
	} else {
		if (div & (1<<5))
			j = 1;
		div &= 0x1F;
		div = div*(j?64:1)*2;
	}

	freq = (SRC_FREQ*GET_DIVF(tmp))*(base/(GET_DIVR(tmp)*GET_DIVQ(tmp)*div));

	return freq;
}
static int get_freq_t(enum dev_id dev, int *divisor) {

	unsigned int div = 0, freq = 0, base = 1000000, tmp;
	int PLL_NO, i, j = 0, div_addr_offs;
	#ifdef DEBUG_CLK
	unsigned int t1 = 0, t2 = 0;
	#endif

	PLL_NO = calc_pll_num(dev, &div_addr_offs);
	if (PLL_NO == -1) {
		printf("device not found");
		return -1;
	}

	if (PLL_NO == -2)
		return div_addr_offs*1000000;

	*(volatile unsigned char *)(PMC_BASE + div_addr_offs + 2) = (dev == DEV_SDTV) ? 1 : 0;

	check_PLL_DIV_busy();

	div = *divisor = *(volatile unsigned char *)(PMC_BASE + div_addr_offs);

	if ((dev != DEV_SDMMC0) && (dev != DEV_SDMMC1) && (dev != DEV_SDMMC2)) {
		div = div&0x1F;
	} else {
		if (div & (1<<5))
			j = 1;
		div &= 0x1F;
		div = div*(j?64:1)*2;
	}

	#ifdef DEBUG_CLK
	printf("div_addr_offs=0x%x PLL_NO=%d PMC_PLL=0x%x\n", div_addr_offs, PLL_NO, PMC_PLL);
	t1 = wmt_read_oscr();
	#endif
	tmp = *(volatile unsigned int *)(PMC_PLL + 4*PLL_NO);
	for (i = 0; i < ARRAYSIZE(pllmapAll); i++) {
		if (pllmapAll[i].pll == tmp) {
			freq = pllmapAll[i].freq;
			#ifdef DEBUG_CLK
			printf("********dev%d---PLL_NO=%X---get freq=%d set0x%x\n",	dev, PLL_NO+10, freq, pllmapAll[i].pll);
			#endif
			break;
		}
	}
	if (i >= ARRAYSIZE(pllmapAll) || freq == 0) {
		printf("gfreq : dev%d********************pll not match table**************\n", dev);
		/*freq = (SRC_FREQ*GET_DIVF(tmp))*(base/(GET_DIVR(tmp)*GET_DIVQ(tmp)*div));*/
		freq = ((SRC_FREQ/GET_DIVQ(tmp)) * (GET_DIVF(tmp)/GET_DIVR(tmp)) * base) / div;
	} else {
		freq = (freq * 15625)<<6;
		freq = freq/div;
	}

	#ifdef DEBUG_CLK
	t2 = wmt_read_oscr() - t1;
	printf("************delay_time=%d\n", t2);
	printf("get_freq cmd: freq=%d \n", freq);
	#endif

	return freq;
}

static int set_divisor(enum dev_id dev, int unit, int freq, int *divisor) {

	unsigned int tmp, PLL, div = 0;
	int PLL_NO, i, j = 0, div_addr_offs, SD_MMC = 1, ret;

	if ((dev != DEV_SDMMC0) && (dev != DEV_SDMMC1) && (dev != DEV_SDMMC2))
		SD_MMC = 0;
	PLL_NO = calc_pll_num(dev, &div_addr_offs);
	if (PLL_NO == -1) {
		printf("device not found");
		return -1;
	}

	if (PLL_NO == -2) {
		printf("device has no divisor");
		return -1;
	}

	*(volatile unsigned char *)(PMC_BASE + div_addr_offs + 2) = (dev == DEV_SDTV) ? 1 : 0;

	check_PLL_DIV_busy();

	tmp = *(volatile unsigned int *)(PMC_PLL + 4*PLL_NO);
	PLL = (SRC_FREQ*GET_DIVF(tmp))/(GET_DIVR(tmp)*GET_DIVQ(tmp));

	ret = check_clk_enabled(dev, SET_DIV);
	if (ret)
		return -1;

	PLL *= 1000000;
	if (unit == 1)
		freq *= 1000;
	else if (unit == 2)
		freq *= 1000000;

	if (SD_MMC == 0) {
		for (i = 1; i < 33; i++) {
			if ((i > 1 && (i%2)) && (PLL_NO == 2))
				continue;
			if ((PLL/i) <= ((unsigned int)freq)) {
				*divisor = div = i;
				break;
			}
		}
	} else {
		if ((PLL/64) >= ((unsigned int)freq))
				j = 1;
		for (i = 1; i < 33; i++)
			if ((PLL/(i*(j?64:1)*2)) <= ((unsigned int)freq)) {
				*divisor = div = i;
				break;
			}
	}
	if (div != 0 && div < 33) {
		check_PLL_DIV_busy();
		if (dev == DEV_WMTNA)
			*(volatile unsigned int *)(PMC_BASE + div_addr_offs) =	(0x200 | ((div == 32) ? 0 : div));
		else
			*(volatile unsigned int *)(PMC_BASE + div_addr_offs)
			=	((dev == DEV_SDTV)?0x10000:0) + (j?32:0) + ((div == 32) ? 0 : div);
		check_PLL_DIV_busy();
		if (SD_MMC == 1)
			return (PLL/(div*(j?64:1)*2));
		else
			return (PLL/(div*(j?64:1)));
	}
	printf("no suitable divisor");
	return -1;
}
static int set_divisor_t(enum dev_id dev, int unit, int freq, int *divisor) {

	unsigned int div = 0, tmp;
	int ret, PLL_NO, i, j = 1, div_addr_offs, SD_MMC = 1;
	unsigned int PLL = 0, freq_target = freq;
	if ((dev != DEV_SDMMC0) && (dev != DEV_SDMMC1) && (dev != DEV_SDMMC2))
		SD_MMC = 0;

	PLL_NO = calc_pll_num(dev, &div_addr_offs);
	if (PLL_NO == -1) {
		printf("device not found");
		return -1;
	} else if (PLL_NO == -2) {
		printf("device has no divisor");
		return -1;
	}

	*(volatile unsigned char *)(PMC_BASE + div_addr_offs + 2) = (dev == DEV_SDTV) ? 1 : 0;

	check_PLL_DIV_busy();
	tmp = *(volatile unsigned int *)(PMC_PLL + 4*PLL_NO);

	ret = check_clk_enabled(dev, SET_DIV);
	if (ret)
		return -1;

	if (unit == 1)
		freq_target *= 1000;
	else if (unit == 2)
		freq_target *= 1000000;

	for (i = 0; i < ARRAYSIZE(pllmapAll); i++) {
		if (pllmapAll[i].pll == tmp) {
			PLL = pllmapAll[i].freq;
			break;
		}
	}
	if (i >= ARRAYSIZE(pllmapAll) || PLL == 0) {
		printf("set div : dev%d********************pll not match table**************\n", dev);
		PLL = (SRC_FREQ/GET_DIVQ(tmp)) * (GET_DIVF(tmp)/GET_DIVR(tmp));
	}
	PLL = (PLL * 15625)<<6;

	if (SD_MMC == 0) {
		for (i = 1; i < 64; i++) {
			if ((i > 1 && (i%2)) && (PLL_NO == 2))
				continue;
				freq = PLL/i;
			if (freq <= freq_target) {
				*divisor = div = i;
				break;
			}
		}
	} else {
		if ((PLL>>6) >= freq_target)
			j = 1<<6;
		for (i = 1; i < 64; i++) {
			freq = PLL/(i*j);
			if (freq <= freq_target) {
				*divisor = div = i;
				break;
			}
		}
	}
	if (div > 32)
		div = 32;
	if (div != 0 && div < 33) {
		check_PLL_DIV_busy();
		if (dev == DEV_WMTNA)
			*(volatile unsigned int *)(PMC_BASE + div_addr_offs) =	(0x200 | ((div == 32) ? 0 : div));
		else
			*(volatile unsigned int *)(PMC_BASE + div_addr_offs)
			=	((dev == DEV_SDTV)?0x10000:0) + (j==64?32:0) + ((div == 32) ? 0 : div);
		#ifdef DEBUG_CLK
		printf("SETDIV********dev%d PLL_%X PLL=0x%x div%d cal_freq=%d target_freq%d\n",
		dev, PLL_NO+10, PLL, div, freq, freq_target);
		#endif
		check_PLL_DIV_busy();
		return freq;
	}
	printf("no suitable divisor dev0x%x div=%d treq%d\n", dev, div, freq_target);
	return -1;
}

static int set_pll_speed(enum dev_id dev, int unit, int freq, int *divisor) {

	unsigned int PLL, DIVF=1, DIVR=1, DIVQ=1;
	unsigned int last_freq, div=1, base, base_f=1/*, filt_range, filter*/;
	unsigned long minor, min_minor = 0xFF000000;
	int PLL_NO, div_addr_offs, DF, DR, VD, DQ;

	PLL_NO = calc_pll_num(dev, &div_addr_offs);
	if (PLL_NO == -1) {
		printf("device not belong to PLL A B C D E F G");
		return -1;
	}

	if (PLL_NO == -2) {
		printf("device not found");
		return -1;
	}

	*(volatile unsigned char *)(PMC_BASE + div_addr_offs + 2) = (dev == DEV_SDTV) ? 1 : 0;

	check_PLL_DIV_busy();
	VD = *divisor = *(volatile unsigned char *)(PMC_BASE + div_addr_offs);
	base = 1000000;
	if (unit == 1)
		base_f = 1000;
	else if (unit == 2)
		base_f = 1000000;
	else if (unit != 0) {
		printf("unit is out of range");
		return -1;
	}

	for (DR = MAX_DR; DR >= 0; DR--) {
		for (DQ = MAX_DQ; DQ >= 0; DQ--) {
			for (DF = 0; DF <= MAX_DF; DF++) {
				if (SRC_FREQ/(DR+1) < 10)
					break;
				if ((1000/SRC_FREQ) > ((2*(DF+1))/(DR+1)))
					continue;
				if ((2000/SRC_FREQ) < ((2*(DF+1))/(DR+1)))
					break;
				if (((SRC_FREQ * 2 * (DF+1)) * (base/(VD * (DR+1)*(1<<DQ)))) == (freq * base_f)) {
					DIVF = DF;
					DIVR = DR;
					DIVQ = DQ;
					div = VD;
					/*printf("find the equal value\n");*/
					goto find;
				} else if (((SRC_FREQ * 2 * (DF+1)) * (base/(VD * (DR+1)*(1<<DQ)))) < (freq * base_f)) {
					minor = (freq * base_f) - ((SRC_FREQ * 2 * (DF+1)) * (base/(VD * (DR+1)*(1<<DQ))));
					//printf("minor=0x%x, min_minor=0x%x", minor, min_minor);
					if (minor < min_minor) {
						DIVF = DF;
						DIVR = DR;
						DIVQ = DQ;
						div = VD;
						min_minor = minor;
					}
				} else if (((SRC_FREQ * 2 * (DF+1)) * (base/(VD * (DR+1)*(1<<DQ)))) > (freq * base_f)) {
					if (PLL_NO == 2) {
						minor = ((SRC_FREQ * 2 * (DF+1)) * (base/(VD * (DR+1)*(1<<DQ)))) - (freq * base_f);
						if (minor < min_minor) {
							DIVF = DF;
							DIVR = DR;
							DIVQ = DQ;
							div = VD;
							min_minor = minor;
						}
					}
					break;
				}
			}//DF
		}//DQ
	}//DR
/*minimun:*/
find:

	/*filter = SRC_FREQ/(DIVR+1);
	filt_range = check_filter(filter);*/
	last_freq = (SRC_FREQ * 2 * (DIVF+1)) * (base / ((DIVR+1)*(1<<DIVQ)*(*divisor)));
	/*printf("DIVF%d, DIVR%d, DIVQ%d, divisor%d freq=%dHz \n",
	DIVF, DIVR, DIVQ, *divisor, last_freq);*/
	PLL = (DIVF<<16) + (DIVR<<8) + DIVQ/* + (filt_range<<24)*/;

	/* if the clk of the device is not enable, then enable it */
	/*if (dev < 128) {
		pmc_clk_en = *(volatile unsigned int *)(PMC_CLK + 4*(dev/32));
		if (!(pmc_clk_en & (1 << (dev - 32*(dev/32)))))
			enable_dev_clk(dev);
	}*/

	check_PLL_DIV_busy();
	/*printf("PLL0x%x, pll addr =0x%x\n", PLL, PMC_PLL + 4*PLL_NO);*/
	*(volatile unsigned int *)(PMC_PLL + 4*PLL_NO) = PLL;
	check_PLL_DIV_busy();
	return last_freq;

	printf("no suitable pll");
	return -1;
}

static int set_pll_speed_t(enum dev_id dev, int unit, int freq, int *divisor, int wait) {

	unsigned int PLL, VD;
	unsigned int minor, min_minor = 0x7F000000, set_freq = 0;
	int PLL_NO, div_addr_offs, i;
	unsigned int tmp_freq, tmp_PLL = 0;
	PLL_NO = calc_pll_num(dev, &div_addr_offs);
	if (PLL_NO == -1) {
		printf("device not belong to PLL A B C D E");
		return -1;
	}

	if (PLL_NO == -2) {
		printf("device has no divisor");
		return -1;
	}

	*(volatile unsigned char *)(PMC_BASE + div_addr_offs + 2) = (dev == DEV_SDTV) ? 1 : 0;

	check_PLL_DIV_busy();
	VD = *divisor = *(volatile unsigned char *)(PMC_BASE + div_addr_offs);

	if (unit == 1)
		freq *= 1000;
	else if (unit == 2)
		freq *= 1000000;
	else if (unit != 0) {
		printf("unit is out of range");
		return -1;
	}
	
	for (i = 0; i < ARRAYSIZE(pllmap); i++) {
		if (pllmap[i].freq > 1150 && PLL_NO == 4)
			continue;
		if (pllmap[i].freq > 1250 && ((PLL_NO == 2) || (PLL_NO == 6)))
			continue;
		tmp_freq = ((pllmap[i].freq*15625)<<6) / VD;
		if (tmp_freq == freq) {
			tmp_PLL = pllmap[i].pll;
			set_freq = tmp_freq;
			min_minor = 0;
			goto find;
		} else if (tmp_freq < freq) {
			minor = freq - tmp_freq;
			if (minor < min_minor) {
				tmp_PLL = pllmap[i].pll;
				set_freq = tmp_freq;
				min_minor = minor;
			}
		} else if (tmp_freq > freq) {
			if (PLL_NO == 2) {
				minor = tmp_freq - freq;
				if (minor < min_minor) {
					tmp_PLL = pllmap[i].pll;
					set_freq = tmp_freq;
					min_minor = minor;
				}
			}
			break;
		}
	}
find:
	#ifdef DEBUG_CLK
	if (min_minor)
		printf("minimum minor=0x%x, unit=%d \n", (unsigned int)min_minor, unit);
	printf("SETPLL********dev%d PLL_%X PLL=0x%x cal_freq=%d target_freq%d\n",
	dev, PLL_NO+10, tmp_PLL, set_freq, freq);
	#endif
	PLL = tmp_PLL;
	
	check_PLL_DIV_busy();
	*(volatile unsigned int *)(PMC_PLL + 4*PLL_NO) = PLL;
	if (wait)
		check_PLL_DIV_busy();
	return set_freq;
}

static int set_pll_divisor(enum dev_id dev, int unit, int freq, int *divisor) {

	unsigned int PLL, DIVF=1, DIVR=1, DIVQ=1, pmc_clk_en, old_divisor;
	unsigned int last_freq, div=1, base, base_f=1/*, filt_range, filter*/;
	unsigned long minor, min_minor = 0xFF000000;
	int PLL_NO, div_addr_offs, DF, DR, VD, DQ;

	PLL_NO = calc_pll_num(dev, &div_addr_offs);
	if (PLL_NO == -1) {
		printf("device not belong to PLL A B C D E");
		return -1;
	}

	if (PLL_NO == -2) {
		printf("device has no divisor");
		return -1;
	}

	*(volatile unsigned char *)(PMC_BASE + div_addr_offs + 2) = (dev == DEV_SDTV) ? 1 : 0;

	check_PLL_DIV_busy();

	old_divisor = *(volatile unsigned char *)(PMC_BASE + div_addr_offs);

	base = 1000000;
	if (unit == 1)
		base_f = 1000;
	else if (unit == 2)
		base_f = 1000000;
	else if (unit != 0) {
		printf("unit is out of range");
		return -1;
	}

	//tmp = div * freq;
	for (DR = MAX_DR; DR >= 0; DR--) {
		for (DQ = MAX_DQ; DQ >= 0; DQ--) {
			for (VD = 32; VD >= 1; VD--) {
				if ((VD > 1 && (VD%2)) && (PLL_NO == 2))
					continue;
				for (DF = 0; DF <= MAX_DF; DF++) {
					if (SRC_FREQ/(DR+1) < 10)
						break;
					if ((1000/SRC_FREQ) > ((2*(DF+1))/(DR+1)))
						continue;
					if ((2000/SRC_FREQ) < ((2*(DF+1))/(DR+1)))
						break;
					if (((SRC_FREQ * 2 * (DF+1)) * (base/(VD * (DR+1)*(1<<DQ)))) == (freq * base_f)) {
						DIVF = DF;
						DIVR = DR;
						DIVQ = DQ;
						div = VD;
						/*printf("find the equal value\n");*/
						goto find;
					} else if (((SRC_FREQ * 2 * (DF+1)) * (base/(VD * (DR+1)*(1<<DQ)))) < (freq * base_f)) {
						minor = (freq * base_f) - ((SRC_FREQ * 2 * (DF+1)) * (base/(VD * (DR+1)*(1<<DQ))));
						/*if (unit == 2)
							minor = ((SRC_FREQ * DF) - (VD * (DR*(1<<DQ)))) * base;
						else if (unit == 1)
							minor = (1000 * SRC_FREQ * DF) - (freq * VD * (DR*(1<<DQ)));
						else 
							minor = (1000 * SRC_FREQ * DF) - ((freq * VD * (DR*(1<<DQ)))/1000);*/
						//printf("minor=0x%x, min_minor=0x%x", minor, min_minor);
						if (minor < min_minor) {
							DIVF = DF;
							DIVR = DR;
							DIVQ = DQ;
							div = VD;
							min_minor = minor;
								/*if (min_minor < 1000000 && unit == 2)
									goto minimun;
								else if (min_minor < 1000 && unit == 1)
								 goto minimun;
								else if (min_minor < 20 && unit == 0)
								 goto minimun;*/
						}
					} else if (((SRC_FREQ * 2 * (DF+1)) * (base/(VD * (DR+1)*(1<<DQ)))) > (freq * base_f)) {
						if (PLL_NO == 2) {
							minor = ((SRC_FREQ * 2 * (DF+1)) * (base/(VD * (DR+1)*(1<<DQ)))) - (freq * base_f);
							if (minor < min_minor) {
								DIVF = DF;
								DIVR = DR;
								DIVQ = DQ;
								div = VD;
								min_minor = minor;
							}
						}
						break;
					}
				}//DF
			}//VD
		}//DQ
	}//DR
/*minimun:*/
	//printf("minimum minor=0x%x, unit=%d \n", (unsigned int)min_minor, unit);
find:

	/*filter = SRC_FREQ/(DIVR+1);
	filt_range = check_filter(filter);*/
	*divisor = div;
	last_freq = (SRC_FREQ * 2 * (DIVF+1)) * (base / ((DIVR+1)*(1<<DIVQ)*(*divisor)));
	/*printf("DIVF%d, DIVR%d, DIVQ%d, divisor%d freq=%dHz \n",
	DIVF, DIVR, DIVQ, *divisor, last_freq);*/
	PLL = (DIVF<<16) + (DIVR<<8) + DIVQ/* + (filt_range<<24)*/;


	/* if the clk of the device is not enable, then enable it */
	if (dev < 128) {
		pmc_clk_en = *(volatile unsigned int *)(PMC_CLK + 4*(dev/32));
		if (!(pmc_clk_en & (1 << (dev - 32*(dev/32))))) {
			/*enable_dev_clk(dev);*/
			printf("device clock is disabled");
			return -1;
		}
	}

	check_PLL_DIV_busy();
	if (old_divisor < *divisor) {
		*(volatile unsigned int *)(PMC_BASE + div_addr_offs)
		= ((dev == DEV_SDTV)?0x10000:0) +/*(j?32:0) + */((div == 32) ? 0 : div)/* + (div&1) ? (1<<8): 0*/;
		check_PLL_DIV_busy();
	}
	//printf("PLL0x%x, pll addr =0x%x\n", PLL, PMC_PLL + 4*PLL_NO);
	*(volatile unsigned int *)(PMC_PLL + 4*PLL_NO) = PLL;
	check_PLL_DIV_busy();
	/*printf("DIVF%d, DIVR%d, DIVQ%d, div%d div_addr_offs=0x%x\n",
	DIVF, DIVR, DIVQ, div, div_addr_offs);*/

	if (old_divisor > *divisor) {
		*(volatile unsigned int *)(PMC_BASE + div_addr_offs)
		= ((dev == DEV_SDTV)?0x10000:0) +/*(j?32:0) + */((div == 32) ? 0 : div)/* + (div&1) ? (1<<8): 0*/;
		check_PLL_DIV_busy();
	}

	return last_freq;

	printf("no suitable divisor");
	return -1;
}


static int set_pll_divisor_t(enum dev_id dev, int unit, int freq, int *divisor) {

	unsigned int PLL, old_divisor, div=1, set_freq = 0;
	unsigned int minor, min_minor = 0x7F000000, tmp_freq = 0, tmp_PLL = 0, tfreq = (unsigned int)freq;
	int i, PLL_NO, div_addr_offs, VD, ret;

	PLL_NO = calc_pll_num(dev, &div_addr_offs);
	if (PLL_NO == -1) {
		printf("device not belong to PLL A B C D E");
		return -1;
	} else if (PLL_NO == -2) {
		printf("device has no divisor");
		return -1;
	}

	*(volatile unsigned char *)(PMC_BASE + div_addr_offs + 2) = (dev == DEV_SDTV) ? 1 : 0;

	check_PLL_DIV_busy();
	old_divisor = *(volatile unsigned char *)(PMC_BASE + div_addr_offs);

	if (unit == 1)
		tfreq *= 1000;
	else if (unit == 2)
		tfreq *= 1000000;
	else if (unit != 0) {
		printf("unit is out of range");
		return -1;
	}

	for (i = 0; i < ARRAYSIZE(pllmap); i++) {
		if (pllmap[i].freq > 1150 && PLL_NO == 4)
			continue;
		if (pllmap[i].freq > 1250 && ((PLL_NO == 2) || (PLL_NO == 6)))
			continue;
		for (VD = 32; VD >= 1; VD--) {
			if ((VD > 1 && (VD == 3)) && ((PLL_NO == 2) || (PLL_NO == 6)))
				continue;
			tmp_freq = ((pllmap[i].freq*15625)<<6) / VD;
			if (tmp_freq == tfreq) {
				tmp_PLL = pllmap[i].pll;
				set_freq = tmp_freq;
				div = VD;
				min_minor = 0;
				goto find;
			} else if (tmp_freq < tfreq) {
				minor = tfreq - tmp_freq;
				if (minor < min_minor) {
					tmp_PLL = pllmap[i].pll;
					set_freq = tmp_freq;
					div = VD;
					min_minor = minor;
				}
			} else if (tmp_freq > tfreq) {
				if ((PLL_NO == 2) || (PLL_NO == 6)) {
					minor = tmp_freq - tfreq;
					if (minor < min_minor) {
						tmp_PLL = pllmap[i].pll;
						set_freq = tmp_freq;
						div = VD;
						min_minor = minor;
					}
				}
				break;
			}
		}
	}
find:
	#ifdef DEBUG_CLK
	if (min_minor)
		printf("minimum minor=0x%x, unit=%d \n", (unsigned int)min_minor, unit);
	printf("SETPLLDIV********dev%d PLL_%X PLL=0x%x div%d cal_freq=%d target_freq%d\n",
	dev, PLL_NO+10, tmp_PLL, div, set_freq, tfreq);
	#endif

	*divisor = div;
	PLL = tmp_PLL;

	ret = check_clk_enabled(dev, SET_PLLDIV);
	if (ret)
		return -1;

	check_PLL_DIV_busy();
	if (old_divisor < *divisor) {
		if (dev == DEV_WMTNA)
			*(volatile unsigned int *)(PMC_BASE + div_addr_offs) =	(0x200 | ((div == 32) ? 0 : div));
		else
			*(volatile unsigned int *)(PMC_BASE + div_addr_offs)
			= ((dev == DEV_SDTV)?0x10000:0) +/*(j?32:0) + */((div == 32) ? 0 : div)/* + (div&1) ? (1<<8): 0*/;
		check_PLL_DIV_busy();
	}
	*(volatile unsigned int *)(PMC_PLL + 4*PLL_NO) = PLL;
	check_PLL_DIV_busy();

	if (old_divisor > *divisor) {
		if (dev == DEV_WMTNA)
			*(volatile unsigned int *)(PMC_BASE + div_addr_offs) =	(0x200 | ((div == 32) ? 0 : div));
		else
			*(volatile unsigned int *)(PMC_BASE + div_addr_offs)
			= ((dev == DEV_SDTV)?0x10000:0) +/*(j?32:0) + */((div == 32) ? 0 : div)/* + (div&1) ? (1<<8): 0*/;
		check_PLL_DIV_busy();
	}

	return set_freq;
}


/*
* cmd : CLK_DISABLE  : disable clock,
*       CLK_ENABLE   : enable clock,
*       GET_FREQ     : get device frequency, it doesn't enable or disable clock,
*       SET_DIV      : set clock by setting divisor only(clock must be enabled by CLK_ENABLE command),
*       SET_PLL      : set clock by setting PLL only, no matter clock is enabled or not,
*                      this cmd can be used before CLK_ENABLE cmd to avoid a extreme speed
*                      to be enabled when clock enable.
*       SET_PLLDIV   : set clock by setting PLL and divisor(clock must be enabled by CLK_ENABLE command).
* dev : Target device ID to be set the clock.
* unit : the unit of parameter "freq", 0 = Hz, 1 = KHz, 2 = MHz.
* freq : frequency of the target to be set when cmd is "SET_XXX".
*
* return : return value is different depend on cmd type,
*				CLK_DISABLE  : return internal count which means how many drivers still enable this clock,
*				               retrun -1 if this device has no clock enable register.
*				               
*				CLK_ENABLE   : return internal count which means how many drivers enable this clock,
*				               retrun -1 if this device has no clock enable register.
*
*				GET_FREQ     : return device frequency in Hz when clock is enabled,
*				               return -1 when clock is disabled.
*
*       SET_DIV      : return the finally calculated frequency when clock is enabled,
*				               return -1 when clock is disabled.
*
*       SET_PLL      : return the finally calculated frequency no matter clock is enabled or not.
*
*       SET_PLLDIV   : return the finally calculated frequency when clock is enabled,
*				               return -1 when clock is disabled.
* Caution :
* 1. The final clock freq maybe an approximative value,
*    equal to or less than the setting freq.
* 2. SET_DIV and SET_PLLDIV commands which would set divisor register must use CLK_ENABLE command
*    first to enable device clock.
* 3. Due to default frequency may be extremely fast when clock is enabled. use SET_PLL command can
*    set the frequency into a reasonable value, but don't need to enable clock first.
*/
#define PMC_TABLE 1
int auto_pll_divisor(enum dev_id dev, enum clk_cmd cmd, int unit, int freq)
{
	int last_freq, divisor, en_count;

	switch (cmd) {
		case CLK_DISABLE:
			if (dev < 128) {
				en_count = disable_dev_clk(dev);
				return en_count;
			} else {
				printf("device has not clock enable register");
				return -1;
			}
		case CLK_ENABLE:
			if (dev < 128) {
				en_count = enable_dev_clk(dev);
				return en_count;
			} else {
				printf("device has not clock enable register");
				return -1;
			}
		case GET_FREQ:
			#ifdef PMC_TABLE
			last_freq = get_freq_t(dev, &divisor);
			#else
			last_freq = get_freq(dev, &divisor);
			#endif
			return last_freq;
		case SET_DIV:
			divisor = 0;
			#ifdef PMC_TABLE
			last_freq = set_divisor_t(dev, unit, freq, &divisor);
			#else
			last_freq = set_divisor(dev, unit, freq, &divisor);
			#endif
			return last_freq;
		case SET_PLL:
			divisor = 0;
			#ifdef PMC_TABLE
			last_freq = set_pll_speed_t(dev, unit, freq, &divisor, 1);
			#else
			last_freq = set_pll_speed(dev, unit, freq, &divisor);
			#endif
			return last_freq;
		case SET_PLLDIV:
			divisor = 0;
			#ifdef PMC_TABLE
			last_freq = set_pll_divisor_t(dev, unit, freq, &divisor);
			#else
			last_freq = set_pll_divisor(dev, unit, freq, &divisor);
			#endif
			return last_freq;
		default:
		printf("clock cmd unknow");
		return -1;
	}
}


/**
* freq = src_freq * 2 * (DIVF+1)/((DIVR+1)*(2^DIVQ))
*
* dev : Target device ID to be set the clock.
* DIVF : Feedback divider value.
* DIVR : Reference divider value.
* DIVQ : Output divider value.
* dev_div : Divisor belongs to each device, 0 means not changed.
* return : The final clock freq, in Hz, will be returned when success,
*            -1 means fail (waiting busy timeout).
*
* caution :
* 1. src_freq/(DIVR+1) should great than or equal to 10,
*/
int manu_pll_divisor(enum dev_id dev, int DIVF, int DIVR, int DIVQ, int dev_div)
{

	unsigned int PLL, freq, pmc_clk_en, old_divisor, div;
	int PLL_NO, div_addr_offs, j = 0, SD_MMC = 0;
	
	if (DIVF < 0 || DIVF > MAX_DF){
		printf("DIVF is out of range 0 ~ 127");
		return -1;
	}
	if (DIVR < 0 || DIVR > MAX_DR){
		printf("DIVR is out of range 0 ~ 1");
		return -1;
	}
	if (DIVQ < 0 || DIVQ > MAX_DQ){
		printf("DIVQ is out of range 0 ~ 3");
		return -1;
	}
	if ((800/SRC_FREQ) > ((2*(DIVF+1))/(DIVR+1))) {
		printf("((2(DIVF+1))/(DIVR+1)) should great than (800/SRC_FREQ)");
		return -1;
	}
	if ((1600/SRC_FREQ) < ((2*(DIVF+1))/(DIVR+1))) {
		printf("((2*(DIVF+1))/(DIVR+1)) should less than (1600/SRC_FREQ)");
		return -1;
	}

	if (dev_div > ((SD_MMC == 1)?63:31)){
		printf("divisor is out of range 0 ~ 31");
		return -1;
	}

	PLL_NO = calc_pll_num(dev, &div_addr_offs);
	if (PLL_NO == -1) {
		printf("device not found");
		return -1;
	}
	old_divisor = *(volatile unsigned char *)(PMC_BASE + div_addr_offs);

	if (SD_MMC == 1 && (dev_div&32))
		j = 1; /* sdmmc has a another divider = 64 */	
	div = dev_div&0x1F;	
	freq = (1000 * SRC_FREQ * 2 * (DIVF+1))/((DIVR+1)*(1<<DIVQ)*div*(j?128:1));
	freq *= 1000;
	//printf("DIVF%d, DIVR%d, DIVQ%d, dev_div%d, freq=%dkHz\n", DIVF, DIVR, DIVQ, dev_div, freq);

	PLL = (DIVF<<16) + (DIVR<<8) + DIVQ;

	/* if the clk of the device is not enable, then enable it */
	if (dev < 128) {
		pmc_clk_en = *(volatile unsigned int *)(PMC_CLK + 4*(dev/32));
		if (!(pmc_clk_en & (1 << (dev - 32*(dev/32)))))
			enable_dev_clk(dev);
	}

	check_PLL_DIV_busy();
	if (old_divisor < dev_div) {
		*(volatile unsigned int *)(PMC_BASE + div_addr_offs)
		= (j?32:0) + ((div == 32) ? 0 : div)/* + (div&1) ? (1<<8): 0*/;
		check_PLL_DIV_busy();
	}
	*(volatile unsigned int *)(PMC_PLL + 4*PLL_NO) = PLL;
	check_PLL_DIV_busy();
	if (old_divisor > dev_div) {
		*(volatile unsigned int *)(PMC_BASE + div_addr_offs)
		= (j?32:0) + ((div == 32) ? 0 : div)/* + (div&1) ? (1<<8): 0*/;
		check_PLL_DIV_busy();
	}


	//PLL = (j?32:0) + ((div == 32) ? 0 : div) /*+ (div&1) ? (1<<8): 0*/;
	//printf("set divisor =0x%x, divider address=0x%x\n", PLL, (PMC_BASE + div_addr_offs));
	return freq;
}


