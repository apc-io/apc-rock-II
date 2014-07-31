/*++
linux/arch/arm/mach-wmt/wmt_clk.c

Copyright (c) 2008  WonderMedia Technologies, Inc.

This program is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software Foundation,
either version 2 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License along with
this program.  If not, see <http://www.gnu.org/licenses/>.

WonderMedia Technologies, Inc.
10F, 529, Chung-Cheng Road, Hsin-Tien, Taipei 231, R.O.C.
--*/

#include <linux/mtd/mtd.h>
#include "wmt_clk.h"
#include <mach/hardware.h>

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/pm.h>
#include <linux/cpufreq.h>
#include <linux/ioport.h>
#include <linux/platform_device.h>

#include <asm/system.h>
#include <asm/pgtable.h>
#include <asm/mach/map.h>
#include <asm/irq.h>
#include <asm/sizes.h>
#include <linux/i2c.h>
#include <asm/div64.h>
#include <linux/mutex.h>
#include <linux/regulator/consumer.h>
#include <linux/interrupt.h>

#include "generic.h"

#define MMFREQ_VD	DEV_CNMVDU	// DEV_MSVD
int mmfreq_num = ~0;
int mmfreq_cur_num = ~0;
int mmfreq_type_mask;
int mmfreq_debug;
struct regulator *re;

#define PMC_BASE PM_CTRL_BASE_ADDR
#define PMC_PLL (PM_CTRL_BASE_ADDR + 0x200)
#define PMC_CLK (PM_CTRL_BASE_ADDR + 0x250)
#define WM8980_SCC_BASE 0xD8110110
#define CPINFO (WM8980_SCC_BASE + WMT_MMAP_OFFSET)

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
extern int wmt_getsyspara(char *varname, char *varval, int *varlen);
#define DEBUG_PLLA
/*
#define DEBUG
*/
#ifdef DEBUG
static int dbg_mask = 1;
module_param(dbg_mask, int, S_IRUGO | S_IWUSR);
#define fq_dbg(fmt, args...) \
	do {\
		if (dbg_mask) \
			printk(KERN_ERR "[%s]_%d_%d: " fmt, __func__ , __LINE__, smp_processor_id(), ## args);\
	} while (0)
#define fq_trace() \
	do {\
		if (dbg_mask) \
			printk(KERN_ERR "trace in %s %d\n", __func__, __LINE__);\
	} while (0)
#else
#define fq_dbg(fmt, args...)
#define fq_trace()
#endif
extern int wmt_getsyspara(char *varname, char *varval, int *varlen);
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
#define MAXPLL 3515625*(1<<8)
unsigned int tblpll = MAXPLL/1000000;
unsigned int fr_rdn = 0;
static DEFINE_SPINLOCK(wmt_clk_irqlock);
unsigned long wmt_clk_irqlock_flags;

void wmt_mc5_autotune(void)//gri
{
	unsigned int reg_val, reg_tmp;
	
	MC_CLOCK_CTRL0_VAL = 0x0;
	wmb();
	MC_CLOCK_CTRL0_VAL = 0x80000000;
	while(MC_CLOCK_CTRL0_VAL != 0xC0000000);
	if ((MC_CONF_VAL & 0x3) == 0x2) {
		//ddr3
		reg_val = (MC_CLOCK_CTRL1_VAL & 0xffff8080);
		reg_tmp = (0x7f0000 & reg_val);
		reg_tmp |= ((0x7f0000 & reg_val) >> 8);
		reg_tmp |= ((0x7f0000 & reg_val) >> 16);
		reg_val |= reg_tmp;
		MC_CLOCK_CTRL1_VAL = reg_val;
	} else {
		//lpddr2
		reg_val = (MC_CLOCK_CTRL1_VAL & 0xffff0000);
		reg_tmp = (0x7f000000 & reg_val);
		reg_tmp |= ((0x7f000000 & reg_val) >> 16);
		reg_tmp |= ((0x7f000000 & reg_val) >> 24);
		reg_val |= reg_tmp;
		MC_CLOCK_CTRL1_VAL = reg_val;	
	}
}

void wmt_clk_mutex_lock(int lock)
{
	if (lock) {
		spin_lock_irqsave(&wmt_clk_irqlock, wmt_clk_irqlock_flags);
	}	else {
		spin_unlock_irqrestore(&wmt_clk_irqlock, wmt_clk_irqlock_flags);
	}
}

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
		printk("clk cnt %d ~ %d: %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d\n",i*16,(i*16)+15,
		dev_en_count[i*16],dev_en_count[i*16+1],dev_en_count[i*16+2],dev_en_count[i*16+3],dev_en_count[i*16+4],dev_en_count[i*16+5],dev_en_count[i*16+6],dev_en_count[i*16+7],
		dev_en_count[i*16+8],dev_en_count[i*16+9],dev_en_count[i*16+10],dev_en_count[i*16+11],dev_en_count[i*16+12],dev_en_count[i*16+13],dev_en_count[i*16+14],dev_en_count[i*16+15]);
	}
}
#endif
#if 0
static int disable_dev_clk(enum dev_id dev)
{
	int en_count;

	if (dev >= 128) {
		printk(KERN_INFO"device dev_id = %d > 128\n",dev);
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
		*(volatile unsigned char *)(PMC_BASE + 0x604) = 0;
		while((REG32_VAL((PMC_BASE + 0x604)) & 0xf5) != 0x00){};
		*(volatile unsigned char *)(PMC_BASE + 0x62C) = 0;
		while((REG32_VAL((PMC_BASE + 0x62C)) & 0xf5) != 0x00){};
		/*printk(KERN_INFO" really disable clock\n");*/
	} else if (en_count > 1) {
		dev_en_count[dev] = (--en_count);
	}

	#ifdef debug_clk
	print_refer_count();
	#endif

	return en_count;
}
#endif
/*
* ON : clock enable : 1, clock disable : 0
*
*/
static int enable_dev_clk(enum dev_id dev, int ON)
{
	int en_count, tmp;

	if (dev > 128) {
		printk(KERN_INFO"device dev_id = %d > 128\n",dev);
		return -1;
	}

	#ifdef debug_clk
	printk(KERN_INFO"device dev_id = %d\n",dev);
	print_refer_count();
	#endif

	//mutex_lock(&clk_cnt_lock);
	wmt_clk_mutex_lock(1);
	en_count = dev_en_count[dev];
	
	if (ON) {
		tmp = *(volatile unsigned int *)(PMC_CLK + 4*(dev/32));
		if (en_count <= 0 || !(tmp&(1 << (dev - 32*(dev/32))))) {
			dev_en_count[dev] = en_count = 1;
			*(volatile unsigned int *)(PMC_CLK + 4*(dev/32))
			|= 1 << (dev - 32*(dev/32));
			/*if (dev == DEV_MSVD) {
				*(volatile unsigned char *)(PMC_BASE + 0x604) = 1;
				while((REG32_VAL((PMC_BASE + 0x604)) & 0xf5) != 0xf1){};
				*(volatile unsigned char *)(PMC_BASE + 0x62C) = 1;
				while((REG32_VAL((PMC_BASE + 0x62C)) & 0xf5) != 0xf1){};
			}*/
			/*printk(KERN_INFO" really enable clock\n");*/
		} else
			dev_en_count[dev] = (++en_count);
	} else {
		if (en_count <= 1) {
			dev_en_count[dev] = en_count = 0;
			*(volatile unsigned int *)(PMC_CLK + 4*(dev/32))
			&= ~(1 << (dev - 32*(dev/32)));
			/**(volatile unsigned char *)(PMC_BASE + 0x604) = 0;
			while((REG32_VAL((PMC_BASE + 0x604)) & 0xf5) != 0x00){};
			*(volatile unsigned char *)(PMC_BASE + 0x62C) = 0;
			while((REG32_VAL((PMC_BASE + 0x62C)) & 0xf5) != 0x00){};*/
			/*printk(KERN_INFO" really disable clock\n");*/
		} else if (en_count > 1) {
			dev_en_count[dev] = (--en_count);
		}
	}
	//mutex_unlock(&clk_cnt_lock);
	wmt_clk_mutex_lock(0);

	#ifdef debug_clk
	print_refer_count();
	#endif

	/* mmfreq */
	if ((dev == MMFREQ_VD) && (mmfreq_num != ~0)) {
		int cnt;
		wmt_clk_mutex_lock(1);
		cnt = dev_en_count[MMFREQ_VD];
		wmt_clk_mutex_lock(0);
		if (cnt == 0)
			wmt_set_mmfreq(mmfreq_num);
	}

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
			if (dev == DEV_ARM)
				return 0;
			/*enable_dev_clk(dev);*/
			printk(KERN_INFO"dev[%d] clock disabled\n", dev);
			return -1;
		}
	}
	return 0;
}
#if 0
static int get_freq(enum dev_id dev, int *divisor) {

	unsigned int div = 0, freq;
	int PLL_NO, j = 0, div_addr_offs;
	unsigned long long freq_llu, base = 1000000, tmp, mod;

	PLL_NO = calc_pll_num(dev, &div_addr_offs);
	if (PLL_NO == -1) {
		printk(KERN_INFO"device not found");
		return -1;
	}

	if (PLL_NO == -2)
		return div_addr_offs*1000000;

	*(volatile unsigned char *)(PMC_BASE + div_addr_offs + 2) = (dev == DEV_SDTV) ? 1 : 0;

	check_PLL_DIV_busy();

	div = *divisor = *(volatile unsigned char *)(PMC_BASE + div_addr_offs);
	//printk(KERN_INFO"div_addr_offs=0x%x PLL_NO=%d PMC_PLL=0x%x\n", div_addr_offs, PLL_NO, PMC_PLL);
	tmp = *(volatile unsigned int *)(PMC_PLL + 4*PLL_NO);
	
	if ((dev != DEV_SDMMC0) && (dev != DEV_SDMMC1) && (dev != DEV_SDMMC2)) {
		div = div&0x1F;
	} else {
		if (div & (1<<5))
			j = 1;
		div &= 0x1F;
		div = div*(j?64:1)*2;
	}

	freq_llu = (SRC_FREQ*GET_DIVF(tmp)) * base;
	//printk(KERN_INFO" freq_llu =%llu\n", freq_llu);
	tmp = GET_DIVR(tmp) * GET_DIVQ(tmp) * div;
	mod = do_div(freq_llu, tmp);
	freq = (unsigned int)freq_llu;
	//printk("get_freq cmd: freq=%d freq(unsigned long long)=%llu div=%llu mod=%llu\n", freq, freq_llu, tmp, mod);

	return freq;
}
#endif
extern unsigned int wmt_read_oscr(void);
static int get_freq_t(enum dev_id dev, int *divisor) {

	unsigned int div = 0, freq = 0, div_ahb = 1;
	int PLL_NO, i, j = 0, div_addr_offs;
	unsigned long long freq_llu = 0, base = 1000000, tmp, mod;
	#ifdef DEBUG_CLK
	unsigned int t1 = 0, t2 = 0;
	#endif

	if (dev == DEV_APB) {
		PLL_NO = calc_pll_num(DEV_AHB, &div_addr_offs);
		if (PLL_NO < 0) {
			printk(KERN_INFO"device not found");
			return -1;
		}
		div_ahb = *(volatile unsigned char *)(PMC_BASE + div_addr_offs);
	}

	PLL_NO = calc_pll_num(dev, &div_addr_offs);
	if (PLL_NO == -1) {
		printk(KERN_INFO"device not found");
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
	printk(KERN_INFO"div_addr_offs=0x%x PLL_NO=%d PMC_PLL=0x%x\n", div_addr_offs, PLL_NO, PMC_PLL);
	t1 = wmt_read_oscr();
	#endif
	//mutex_lock(&clk_cnt_lock);
	tmp = *(volatile unsigned int *)(PMC_PLL + 4*PLL_NO);
	//mutex_unlock(&clk_cnt_lock);
	for (i = 0; i < ARRAYSIZE(pllmapAll); i++) {
		if (pllmapAll[i].pll == tmp) {
			freq = pllmapAll[i].freq;
			#ifdef DEBUG_CLK
			printk("********dev%d---PLL_NO=%X---get freq=%d set0x%x\n",	dev, PLL_NO+10, freq, pllmapAll[i].pll);
			#endif
			break;
		}
	}
	if (i >= ARRAYSIZE(pllmapAll) || freq == 0) {
		printk(KERN_INFO"gfreq : dev%d********************pll not match table**************\n", dev);
		freq_llu = (SRC_FREQ*GET_DIVF(tmp)) * base;
		//printk(KERN_INFO" freq_llu =%llu\n", freq_llu);
		tmp = GET_DIVR(tmp) * GET_DIVQ(tmp) * div * div_ahb;
		mod = do_div(freq_llu, tmp);
		freq = (unsigned int)freq_llu;
	} else {
		freq = (freq * 15625)<<6;
		freq = freq/(div*div_ahb);
	}

	#ifdef DEBUG_CLK
	t2 = wmt_read_oscr() - t1;
	printk("************delay_time=%d\n", t2);
	printk("get_freq cmd: freq=%d freq(unsigned long long)=%llu div=%llu mod=%llu\n",
	freq, freq_llu, tmp, mod);
	#endif

	#ifdef DEBUG_PLLA
	if (dev == DEV_ARM && fr_rdn > (tblpll*1000000))
		return fr_rdn;
	#endif

	return freq;
}
#if 0
static int set_divisor(enum dev_id dev, int unit, int freq, int *divisor) {

	unsigned int div = 0, tmp;
	int PLL_NO, i, j = 0, div_addr_offs, SD_MMC = 1, ret;
	unsigned long long base = 1000000, PLL, PLL_tmp, mod, div_llu, freq_target = freq, mini,tmp1;
	if ((dev != DEV_SDMMC0) && (dev != DEV_SDMMC1) && (dev != DEV_SDMMC2))
		SD_MMC = 0;

	PLL_NO = calc_pll_num(dev, &div_addr_offs);
	if (PLL_NO == -1) {
		printk(KERN_INFO"device not found");
		return -1;
	}

	if (PLL_NO == -2) {
		printk(KERN_INFO"device has no divisor");
		return -1;
	}
	
	*(volatile unsigned char *)(PMC_BASE + div_addr_offs + 2) = (dev == DEV_SDTV) ? 1 : 0;

	check_PLL_DIV_busy();

	tmp = *(volatile unsigned int *)(PMC_PLL + 4*PLL_NO);
	PLL = SRC_FREQ*GET_DIVF(tmp)*base;
	//printk(KERN_INFO" PLL =%llu, dev_id=%d, PLL_NO=%d, PMC_PLL=0x%x, tmp = 0x%x\n", PLL, dev, PLL_NO,PMC_PLL,(unsigned int)&tmp);
	div_llu = GET_DIVR(tmp)*GET_DIVQ(tmp);

	ret = check_clk_enabled(dev, SET_DIV);
	if (ret)
		return -1;

	if (unit == 1)
		freq_target *= 1000;
	else if (unit == 2)
		freq_target *= 1000000;

	if (SD_MMC == 0) {
		for (i = 1; i < 33; i++) {
			if ((i > 1 && (i%2)) && (PLL_NO == 2))
				continue;
				PLL_tmp = PLL;
				mod = do_div(PLL_tmp, div_llu*i);
			if (PLL_tmp <= freq_target) {
				*divisor = div = i;
				break;
			}
		}
	} else {
		mini = freq_target;
		tmp1 = PLL;
		do_div(tmp1, div_llu);
		do_div(tmp1, 64);
		if ((tmp1) >= freq_target)
				j = 1;
		for (i = 1; i < 33; i++) {
			PLL_tmp = PLL;
			mod = do_div(PLL_tmp, div_llu*(i*(j?64:1)*2));
		if (PLL_tmp <= freq_target) {
			*divisor = div = i;
				break;
			}
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
		freq = (unsigned int)PLL_tmp;
		/*printk("set_divisor: freq=%d freq(unsigned long long)=%llu div=%llu mod=%llu divisor%d pmc_pll=0x%x\n",
		freq, PLL_tmp, div_llu, mod, *divisor, tmp);*/
		return freq;
	}
	printk(KERN_INFO"no suitable divisor");
	return -1;
}
#endif
static int set_divisor_t(enum dev_id dev, int unit, int freq, int *divisor) {

	unsigned int div = 0, tmp;
	int /*ret,*/ PLL_NO, i, j = 1, div_addr_offs, SD_MMC = 1;
	unsigned int PLL = 0, freq_target = freq;
	if ((dev != DEV_SDMMC0) && (dev != DEV_SDMMC1) && (dev != DEV_SDMMC2))
		SD_MMC = 0;

	PLL_NO = calc_pll_num(dev, &div_addr_offs);
	if (PLL_NO == -1) {
		printk(KERN_INFO"device not found");
		return -1;
	} else if (PLL_NO == -2) {
		printk(KERN_INFO"device has no divisor");
		return -1;
	}

	*(volatile unsigned char *)(PMC_BASE + div_addr_offs + 2) = (dev == DEV_SDTV) ? 1 : 0;

	check_PLL_DIV_busy();
	tmp = *(volatile unsigned int *)(PMC_PLL + 4*PLL_NO);

	/*ret = check_clk_enabled(dev, SET_DIV);
	if (ret)
		return -1;*/

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
		printk(KERN_INFO"set div : dev%d********************pll not match table**************\n", dev);
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
		//mutex_lock(&clk_cnt_lock);
		check_PLL_DIV_busy();
		if (dev == DEV_WMTNA)
			*(volatile unsigned int *)(PMC_BASE + div_addr_offs) =	(0x200 | ((div == 32) ? 0 : div));
		else
			*(volatile unsigned int *)(PMC_BASE + div_addr_offs)
			=	((dev == DEV_SDTV)?0x10000:0) + (j==64?32:0) + ((div == 32) ? 0 : div);
		#ifdef DEBUG_CLK
		printk("SETDIV********dev%d PLL_%X PLL=0x%x div%d cal_freq=%d target_freq%d\n",
		dev, PLL_NO+10, PLL, div, freq, freq_target);
		#endif
		check_PLL_DIV_busy();
		//mutex_unlock(&clk_cnt_lock);
		return freq;
	}
	printk(KERN_INFO"no suitable divisor\n");
	return -1;
}
#if 0
static int set_pll_speed(enum dev_id dev, int unit, int freq, int *divisor, int wait) {

	unsigned int PLL, DIVF=1, DIVR=1, DIVQ=1;
	unsigned int last_freq, div=1;
	unsigned long long minor, min_minor = 0xFF000000;
	int PLL_NO, div_addr_offs, DF, DR, VD, DQ;
	unsigned long long base = 1000000, base_f = 1, PLL_llu, div_llu, freq_llu, mod;
	PLL_NO = calc_pll_num(dev, &div_addr_offs);
	if (PLL_NO == -1) {
		printk(KERN_INFO"device not belong to PLL A B C D E");
		return -1;
	}

	if (PLL_NO == -2) {
		printk(KERN_INFO"device has no divisor");
		return -1;
	}

	*(volatile unsigned char *)(PMC_BASE + div_addr_offs + 2) = (dev == DEV_SDTV) ? 1 : 0;

	check_PLL_DIV_busy();
	VD = *divisor = *(volatile unsigned char *)(PMC_BASE + div_addr_offs);

	if (unit == 1)
		base_f = 1000;
	else if (unit == 2)
		base_f = 1000000;
	else if (unit != 0) {
		printk(KERN_INFO"unit is out of range");
		return -1;
	}
	freq_llu = freq * base_f;
	for (DR = MAX_DR; DR >= 0; DR--) {
		for (DQ = MAX_DQ; DQ >= 0; DQ--) {
			for (DF = 0; DF <= MAX_DF; DF++) {
				if (SRC_FREQ/(DR+1) < 10)
					break;
				if ((1000/SRC_FREQ) > ((2*(DF+1))/(DR+1)))
					continue;
				if ((2000/SRC_FREQ) < ((2*(DF+1))/(DR+1)))
					break;
				PLL_llu = (unsigned long long)(SRC_FREQ * 2 * (DF+1) * base);
				div_llu = (unsigned long long)(VD * (DR+1)*(1<<DQ));
				mod = do_div(PLL_llu, div_llu);
				if (PLL_llu == freq_llu) {
					DIVF = DF;
					DIVR = DR;
					DIVQ = DQ;
					div = VD;
					/*printk(KERN_INFO"find the equal value");*/
					goto find;
				} else if (PLL_llu < freq_llu) {
					minor = freq_llu - PLL_llu;
					//printk(KERN_INFO"minor=0x%x, min_minor=0x%x", minor, min_minor);
					if (minor < min_minor) {
						DIVF = DF;
						DIVR = DR;
						DIVQ = DQ;
						div = VD;
						min_minor = minor;
					}
				} else if (PLL_llu > freq_llu) {
					if (PLL_NO == 2) {
						minor = PLL_llu - freq_llu;
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
	//printk(KERN_INFO"minimum minor=0x%x, unit=%d \n", (unsigned int)min_minor, unit);
find:

	PLL_llu = (unsigned long long)(SRC_FREQ * 2 * (DIVF+1) * base);
	div_llu = (unsigned long long)((DIVR+1)*(1<<DIVQ)*(*divisor));
	mod = do_div(PLL_llu, div_llu);
	last_freq = (unsigned int)PLL_llu;
	/*printk("set_pll cmd: freq=%d freq(unsigned long long)=%llu div=%llu mod=%llu\n", last_freq, PLL_llu, div_llu, mod);
	printk(KERN_INFO"DIVF%d, DIVR%d, DIVQ%d, divisor%d freq=%dHz \n",
	DIVF, DIVR, DIVQ, *divisor, last_freq);*/
	PLL = (DIVF<<16) + (DIVR<<8) + DIVQ;

	/* if the clk of the device is not enable, then enable it */
	/*if (dev < 128) {
		pmc_clk_en = *(volatile unsigned int *)(PMC_CLK + 4*(dev/32));
		if (!(pmc_clk_en & (1 << (dev - 32*(dev/32)))))
			enable_dev_clk(dev);
	}*/

	check_PLL_DIV_busy();
	/*printk(KERN_INFO"PLL0x%x, pll addr =0x%x\n", PLL, PMC_PLL + 4*PLL_NO);*/
	*(volatile unsigned int *)(PMC_PLL + 4*PLL_NO) = PLL;
	if (wait)
		check_PLL_DIV_busy();
	return last_freq;

	printk(KERN_INFO"no suitable pll");
	return -1;
}
#endif
static int set_pll_speed_t(enum dev_id dev, int unit, int freq, int *divisor, int wait) {

	unsigned int PLL, VD;
	unsigned int minor, min_minor = 0x7F000000, set_freq = 0;
	int PLL_NO, div_addr_offs, i;
	unsigned int tmp_freq, tmp_PLL = 0;
	PLL_NO = calc_pll_num(dev, &div_addr_offs);
	if (PLL_NO == -1) {
		printk(KERN_INFO"device not belong to PLL A B C D E");
		return -1;
	}

	if (PLL_NO == -2) {
		printk(KERN_INFO"device has no divisor");
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
		printk(KERN_INFO"unit is out of range");
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
		printk(KERN_INFO"minimum minor=0x%x, unit=%d \n", (unsigned int)min_minor, unit);
	printk("SETPLL********dev%d PLL_%X PLL=0x%x div%d cal_freq=%d target_freq%d\n",
	dev, PLL_NO+10, tmp_PLL, div, set_freq, freq);
	#endif
	PLL = tmp_PLL;

	//mutex_lock(&clk_cnt_lock);
	check_PLL_DIV_busy();
	*(volatile unsigned int *)(PMC_PLL + 4*PLL_NO) = PLL;
	if (wait)
		check_PLL_DIV_busy();
	//mutex_unlock(&clk_cnt_lock);
	
	return set_freq;
}
#if 0
static int set_pll_divisor(enum dev_id dev, int unit, int freq, int *divisor) {

	unsigned int PLL, DIVF=1, DIVR=1, DIVQ=1, pmc_clk_en, old_divisor;
	unsigned int last_freq, div=1;
	unsigned long long minor, min_minor = 0xFF000000;
	int PLL_NO, div_addr_offs, DF, DR, VD, DQ;
	unsigned long long base = 1000000, base_f = 1, PLL_llu, div_llu, freq_llu, mod;
	PLL_NO = calc_pll_num(dev, &div_addr_offs);
	if (PLL_NO == -1) {
		printk(KERN_INFO"device not belong to PLL A B C D E");
		return -1;
	}

	if (PLL_NO == -2) {
		printk(KERN_INFO"device has no divisor");
		return -1;
	}

	*(volatile unsigned char *)(PMC_BASE + div_addr_offs + 2) = (dev == DEV_SDTV) ? 1 : 0;

	check_PLL_DIV_busy();

	old_divisor = *(volatile unsigned char *)(PMC_BASE + div_addr_offs);

	if (unit == 1)
		base_f = 1000;
	else if (unit == 2)
		base_f = 1000000;
	else if (unit != 0) {
		printk(KERN_INFO"unit is out of range");
		return -1;
	}
//printk(KERN_INFO"target freq=%d unit=%d PLL_NO=%d\n", freq, unit, PLL_NO);
	freq_llu = freq * base_f;
	//tmp = div * freq;
	for (DR = MAX_DR; DR >= 0; DR--) {
		for (DQ = MAX_DQ; DQ >= 0; DQ--) {
			for (VD = 32; VD >= 1; VD--) {
				if ((VD > 1 && (VD%2)) && ((PLL_NO == 2) || (PLL_NO == 6)))
					continue;
				for (DF = 0; DF <= MAX_DF; DF++) {
					if (SRC_FREQ/(DR+1) < 10)
						break;
					if ((1000/SRC_FREQ) > ((2*(DF+1))/(DR+1)))
						continue;
					if ((2000/SRC_FREQ) < ((2*(DF+1))/(DR+1)))
						break;
					PLL_llu = (unsigned long long)(SRC_FREQ * 2 * (DF+1) * base);
					div_llu = (unsigned long long)(VD * (DR+1)*(1<<DQ));
					mod = do_div(PLL_llu, div_llu);
					if (PLL_llu == freq_llu) {
						DIVF = DF;
						DIVR = DR;
						DIVQ = DQ;
						div = VD;
						/*printk(KERN_INFO"find the equal value");*/
						goto find;
					} else if ((PLL_llu < freq_llu) /*&& (dev != DEV_I2S)*/) {
						minor = freq_llu - PLL_llu;
						//printk(KERN_INFO"minor=0x%x, min_minor=0x%x", minor, min_minor);
						if (minor < min_minor) {
							DIVF = DF;
							DIVR = DR;
							DIVQ = DQ;
							div = VD;
							min_minor = minor;
							/*printk(KERN_INFO"< target :DIVF%d, DIVR%d, DIVQ%d, divisor%d min_minor=%lluHz \n",
							DIVF, DIVR, DIVQ, div, min_minor);*/
						}
					} else if (PLL_llu > freq_llu) {
						if ((PLL_NO == 2) || (PLL_NO == 6)) {
							minor = PLL_llu - freq_llu;
							if (minor < min_minor) {
								DIVF = DF;
								DIVR = DR;
								DIVQ = DQ;
								div = VD;
								min_minor = minor;
								/*printk(KERN_INFO" > target DIVF%d, DIVR%d, DIVQ%d, divisor%d min_minor=%lluHz \n",
								DIVF, DIVR, DIVQ, div, min_minor);*/
							}
						}
						break;
					}
				}//DF
			}//VD
		}//DQ
	}//DR
/*minimun:*/
	//printk(KERN_INFO"minimum minor=%llu, unit=%d \n", min_minor, unit);
find:

	*divisor = div;

	PLL_llu = (unsigned long long)(SRC_FREQ * 2 * (DIVF+1) * base);
	div_llu = (unsigned long long)((DIVR+1)*(1<<DIVQ)*(*divisor));
	mod = do_div(PLL_llu, div_llu);
	last_freq = (unsigned int)PLL_llu;
	/*printk("set_pll_divisor cmd: freq=%d freq(unsigned long long)=%llu div=%llu mod=%llu\n", last_freq, PLL_llu, div_llu, mod);
	printk(KERN_INFO"DIVF%d, DIVR%d, DIVQ%d, divisor%d freq=%dHz \n",
	DIVF, DIVR, DIVQ, *divisor, last_freq);*/
	PLL = (DIVF<<16) + (DIVR<<8) + DIVQ;

	/* if the clk of the device is not enable, then enable it */
	if (dev < 128) {
		pmc_clk_en = *(volatile unsigned int *)(PMC_CLK + 4*(dev/32));
		if (!(pmc_clk_en & (1 << (dev - 32*(dev/32)))) && dev != DEV_ARM) {
			/*enable_dev_clk(dev);*/
			printk(KERN_INFO"device clock is disabled");
			return -1;
		}
	}

	check_PLL_DIV_busy();
	if (old_divisor < *divisor) {
		if (dev == DEV_WMTNA)
			*(volatile unsigned int *)(PMC_BASE + div_addr_offs) =	(0x200 | ((div == 32) ? 0 : div));
		else
			*(volatile unsigned int *)(PMC_BASE + div_addr_offs)
			= ((dev == DEV_SDTV)?0x10000:0) +/*(j?32:0) + */((div == 32) ? 0 : div)/* + (div&1) ? (1<<8): 0*/;
		check_PLL_DIV_busy();
	}
	//printk(KERN_INFO"PLL0x%x, pll addr =0x%x\n", PLL, PMC_PLL + 4*PLL_NO);
	*(volatile unsigned int *)(PMC_PLL + 4*PLL_NO) = PLL;
	check_PLL_DIV_busy();
	/*printk(KERN_INFO"DIVF%d, DIVR%d, DIVQ%d, div%d div_addr_offs=0x%x\n",
	DIVF, DIVR, DIVQ, div, div_addr_offs);*/

	if (old_divisor > *divisor) {
		if (dev == DEV_WMTNA)
			*(volatile unsigned int *)(PMC_BASE + div_addr_offs) =	(0x200 | ((div == 32) ? 0 : div));
		else
			*(volatile unsigned int *)(PMC_BASE + div_addr_offs)
			= ((dev == DEV_SDTV)?0x10000:0) +/*(j?32:0) + */((div == 32) ? 0 : div)/* + (div&1) ? (1<<8): 0*/;
		check_PLL_DIV_busy();
	}


	/*ddv = *(volatile unsigned int *)(PMC_BASE + div_addr_offs);
	check_PLL_DIV_busy();
	pllx = *(volatile unsigned int *)(PMC_PLL + 4*PLL_NO);
	printk(KERN_INFO"read divisor=%d, pll=0x%x from register\n", 0x1F&ddv, pllx);*/
	return last_freq;

	/*printk(KERN_INFO"no suitable divisor");
	return -1;*/
}
#endif

static int set_pll_divisor_t(enum dev_id dev, int unit, int freq, int *divisor) {

	unsigned int PLL, old_divisor, div=1, set_freq = 0;
	unsigned int minor, min_minor = 0x7F000000, tmp_freq = 0, tmp_PLL = 0, tfreq = (unsigned int)freq;
	int i, PLL_NO, div_addr_offs, VD/*, ret*/;

	PLL_NO = calc_pll_num(dev, &div_addr_offs);
	if (PLL_NO == -1) {
		printk(KERN_INFO"device not belong to PLL A B C D E");
		return -1;
	} else if (PLL_NO == -2) {
		printk(KERN_INFO"device has no divisor");
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
		printk(KERN_INFO"unit is out of range");
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
		printk(KERN_INFO"minimum minor=0x%x, unit=%d \n", (unsigned int)min_minor, unit);
	printk("SETPLLDIV********dev%d PLL_%X PLL=0x%x div%d cal_freq=%d target_freq%d\n",
	dev, PLL_NO+10, tmp_PLL, div, set_freq, tfreq);
	#endif

	*divisor = div;
	PLL = tmp_PLL;

	/*ret = check_clk_enabled(dev, SET_PLLDIV);
	if (ret)
		return -1;*/

	//mutex_lock(&clk_cnt_lock);
	wmt_clk_mutex_lock(1);
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
	wmt_clk_mutex_lock(0);
	//mutex_unlock(&clk_cnt_lock);

	return set_freq;
}

static int get_freq_timer(enum dev_id dev, int *divisor) {

	unsigned int div = 0, freq = 0, div_ahb = 1;
	int PLL_NO, i, j = 0, div_addr_offs;
	unsigned long long freq_llu = 0, base = 1000000, tmp, mod;
	#ifdef DEBUG_CLK
	unsigned int t1 = 0, t2 = 0;
	#endif

	if (dev == DEV_APB) {
		PLL_NO = calc_pll_num(DEV_AHB, &div_addr_offs);
		if (PLL_NO < 0) {
			printk(KERN_INFO"device not found");
			return -1;
		}
		div_ahb = *(volatile unsigned char *)(PMC_BASE + div_addr_offs);
	}

	PLL_NO = calc_pll_num(dev, &div_addr_offs);
	if (PLL_NO == -1) {
		printk(KERN_INFO"device not found");
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
	printk(KERN_INFO"div_addr_offs=0x%x PLL_NO=%d PMC_PLL=0x%x\n", div_addr_offs, PLL_NO, PMC_PLL);
	t1 = wmt_read_oscr();
	#endif
	//mutex_lock(&clk_cnt_lock);
	tmp = *(volatile unsigned int *)(PMC_PLL + 4*PLL_NO);
	//mutex_unlock(&clk_cnt_lock);
	for (i = 0; i < ARRAYSIZE(pllmapAll); i++) {
		if (pllmapAll[i].pll == tmp) {
			freq = pllmapAll[i].freq;
			#ifdef DEBUG_CLK
			printk("********dev%d---PLL_NO=%X---get freq=%d set0x%x\n",	dev, PLL_NO+10, freq, pllmapAll[i].pll);
			#endif
			break;
		}
	}
	if (i >= ARRAYSIZE(pllmapAll) || freq == 0) {
		printk(KERN_INFO"gfreq : dev%d********************pll not match table**************\n", dev);
		freq_llu = (SRC_FREQ*GET_DIVF(tmp)) * base;
		//printk(KERN_INFO" freq_llu =%llu\n", freq_llu);
		tmp = GET_DIVR(tmp) * GET_DIVQ(tmp) * div * div_ahb;
		mod = do_div(freq_llu, tmp);
		freq = (unsigned int)freq_llu;
	} else {
		freq = (freq * 15625)<<6;
		freq = freq/(div*div_ahb);
	}

	#ifdef DEBUG_CLK
	t2 = wmt_read_oscr() - t1;
	printk("************delay_time=%d\n", t2);
	printk("get_freq cmd: freq=%d freq(unsigned long long)=%llu div=%llu mod=%llu\n",
	freq, freq_llu, tmp, mod);
	#endif

	return freq;
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
	/*unsigned int t1 = 0, t2 = 0, t3;*/
	/*if (mutexInit == 23456789) {
		mutexInit = 0;
		mutex_init(&clk_cnt_lock);
	}*/

	switch (cmd) {
		case CLK_DISABLE:
			if (dev < 128) {
				//en_count = disable_dev_clk(dev);
				en_count = enable_dev_clk(dev, 0);
				return en_count;
			} else {
				printk(KERN_INFO"device has not clock enable register");
				return -1;
			}
		case CLK_ENABLE:
			if (dev < 128) {
				en_count = enable_dev_clk(dev, 1);
				return en_count;
			} else {
				printk(KERN_INFO"device has not clock enable register");
				return -1;
			}
		case GET_FREQ:
			#ifdef PMC_TABLE
			/*t1 = wmt_read_oscr();*/
			last_freq = get_freq_t(dev, &divisor);
			/*t2 = wmt_read_oscr() - t1;
			t1 = wmt_read_oscr();*/
			#else
			last_freq = get_freq(dev, &divisor);
			#endif
			/*t3 = wmt_read_oscr() - t1;
			printk("*******************GF t3=%d tt2=%d min=%d\n", t3, t2, last_freq - last_freq1);*/
			return last_freq;
		case SET_DIV:
			divisor = 0;
			#ifdef PMC_TABLE
			/*t1 = wmt_read_oscr();*/
			last_freq = set_divisor_t(dev, unit, freq, &divisor);
			/*t2 = wmt_read_oscr() - t1;
			t1 = wmt_read_oscr();*/
			#else
			last_freq = set_divisor(dev, unit, freq, &divisor);
			#endif
			/*t3 = wmt_read_oscr() - t1;
			printk("*******************SD t3=%d tt2=%d min=%d\n", t3, t2, last_freq - last_freq1);*/
			return last_freq;
		case SET_PLL:
			divisor = 0;
			#ifdef PMC_TABLE
			/*t1 = wmt_read_oscr();*/
			last_freq = set_pll_speed_t(dev, unit, freq, &divisor, 1);
			/*t2 = wmt_read_oscr() - t1;
			t1 = wmt_read_oscr();*/
			#else
			last_freq = set_pll_speed(dev, unit, freq, &divisor, 1);
			#endif
			/*t3 = wmt_read_oscr() - t1;
			printk("*******************SP t3=%d tt2=%d min=%d\n", t3, t2, last_freq - last_freq1);*/
			return last_freq;
		case SET_PLLDIV:
			divisor = 0;
			#ifdef PMC_TABLE
			/*t1 = wmt_read_oscr();*/
			last_freq = set_pll_divisor_t(dev, unit, freq, &divisor);
			/*t2 = wmt_read_oscr() - t1;
			t1 = wmt_read_oscr();*/
			#else
			last_freq = set_pll_divisor(dev, unit, freq, &divisor);
			#endif
			/*t3 = wmt_read_oscr() - t1;
			printk("******************SPD t3=%d tt2=%d min=%d\n", t3, t2, last_freq - last_freq1);*/
			return last_freq;
		case GET_CPUTIMER:
			last_freq = get_freq_timer(dev, &divisor);
			return last_freq;
		default:
		printk(KERN_INFO"clock cmd unknow");
		/*last_freq = get_freq(dev, &divisor);
		last_freq = set_divisor(dev, unit, freq, &divisor);
		last_freq = set_pll_speed(dev, unit, freq, &divisor, 1);
		last_freq = set_pll_divisor(dev, unit, freq, &divisor);*/
		return -1;
	}
}
EXPORT_SYMBOL(auto_pll_divisor);

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
	
	if ((dev == DEV_SDMMC0)||(dev == DEV_SDMMC1)||(dev == DEV_SDMMC2))
		SD_MMC = 1;
	
	if (DIVF < 0 || DIVF > MAX_DF){
		printk(KERN_INFO"DIVF is out of range 0 ~ %d", MAX_DF);
		return -1;
	}
	if (DIVR < 0 || DIVR > MAX_DR){
		printk(KERN_INFO"DIVR is out of range 0 ~ %d", MAX_DR);
		return -1;
	}
	if (DIVQ < 0 || DIVQ > MAX_DQ){
		printk(KERN_INFO"DIVQ is out of range 0 ~ %d", MAX_DQ);
		return -1;
	}
	if ((1000/SRC_FREQ) > ((2*(DIVF+1))/(DIVR+1))) {
		printk(KERN_INFO"((2*(DIVF+1))/(DIVR+1)) should great than (1000/SRC_FREQ)");
		return -1;
	}
	if ((2000/SRC_FREQ) < ((2*(DIVF+1))/(DIVR+1))) {
		printk(KERN_INFO"((2*(DIVF+1))/(DIVR+1)) should less than (2000/SRC_FREQ)");
		return -1;
	}

	if (dev_div > ((SD_MMC == 1)?63:31)){
		printk(KERN_INFO"divisor is out of range 0 ~ 31");
		return -1;
	}

	PLL_NO = calc_pll_num(dev, &div_addr_offs);
	if (PLL_NO == -1) {
		printk(KERN_INFO"device not found");
		return -1;
	}
	old_divisor = *(volatile unsigned char *)(PMC_BASE + div_addr_offs);

	if (SD_MMC == 1 && (dev_div&32))
		j = 1; /* sdmmc has a another divider = 64 */	
	div = dev_div&0x1F;	
	freq = (1000 * SRC_FREQ * 2 * (DIVF+1))/((DIVR+1)*(1<<DIVQ)*div*(j?128:1));
	freq *= 1000;
	//printk(KERN_INFO"DIVF%d, DIVR%d, DIVQ%d, dev_div%d, freq=%dkHz\n", DIVF, DIVR, DIVQ, dev_div, freq);

	PLL = (DIVF<<16) + (DIVR<<8) + DIVQ;

	/* if the clk of the device is not enable, then enable it */
	if (dev < 128) {
		pmc_clk_en = *(volatile unsigned int *)(PMC_CLK + 4*(dev/32));
		if (!(pmc_clk_en & (1 << (dev - 32*(dev/32)))))
			enable_dev_clk(dev, 1);
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
	//printk(KERN_INFO"set divisor =0x%x, divider address=0x%x\n", PLL, (PMC_BASE + div_addr_offs));
	return freq;
}
EXPORT_SYMBOL(manu_pll_divisor);


unsigned int ftb[20] = {0xffffffff};
int set_plla_divisor(struct plla_param *plla_env)
{
	int divisor = 0, ret = 0, i;
	unsigned int plla_clk, arm_div, l2c_div, l2c_tag, l2c_data, axi_div;
	unsigned int cpu_clk, index, /*old_arm_div,*/ nand_clk, tfreq, tb_index;
	unsigned int drv_en, tbl_num, info, tmp;
	int varlen = 512;
	unsigned char buf[512] = {0};
	char *ptr = NULL;
	
	plla_clk = plla_env->plla_clk;
	arm_div = plla_env->arm_div;
	l2c_div = plla_env->l2c_div;
	l2c_tag = plla_env->l2c_tag_div;
	l2c_data = plla_env->l2c_data_div;
	axi_div = plla_env->axi_div;
	tb_index = plla_env->tb_index;
	
	cpu_clk = get_freq_t(DEV_ARM, &divisor);
	cpu_clk >>= 20;
	//cpu_clk /= 1000000;
	tfreq = plla_clk/arm_div;

	info = *(volatile unsigned int *)(CPINFO);
	if (ftb[0] == 0xffffffff) {
		if ((info&0x8) == 0)
			ret = wmt_getsyspara("wmt.cfadj.param", buf, &varlen);
		else
			ret = wmt_getsyspara("wmt.cflpadj.param", buf, &varlen);
		if (ret) {
			printk(KERN_INFO "Can not find uboot env wmt.cfadj.param\n");
			ret = -ENODATA;
			ftb[0] = 0;
			goto no_adj;
		}
		sscanf(buf, "%x:%d:", &drv_en, &tbl_num);
		if ((drv_en&1) == 0 || tbl_num == 0) {
			ret = -ENODATA;
			ftb[0] = 0;
			goto no_adj;
		}

		ptr = buf;
		if (tbl_num > 20)
			tbl_num = 20;
		fq_dbg("dvfs_adj_table:");
		for (i = 0; i < tbl_num; i++) {
			strsep(&ptr, "[");
			sscanf(ptr, "%d][", &tmp);
			ftb[i] = tmp;
			fq_dbg("ftb[%d]=%d \n", i, ftb[i]);
		}
		fq_dbg("\n");
	}
	#ifdef DEBUG_PLLA
	if (ftb[0] != 0)
		tblpll = ftb[tb_index];
	else
		ret = 1;
	//printk("tblpll = %d\n", tblpll);
	#endif
no_adj:
	//printk("ret = %d\n", ret);
	if (!ret) {
		if (tfreq != tblpll) {
			fr_rdn = (tfreq*15625)<<6;
			tfreq = tblpll;
			plla_clk = tfreq;
			arm_div = 1;
		} else
			fr_rdn = 0;
	} else {
		#ifdef DEBUG_PLLA_FX
		if (tfreq > (MAXPLL/1000000)) {
			fr_rdn = tfreq*1000000;
			tfreq = (MAXPLL/1000000);
			plla_clk = tfreq;
			arm_div = 1;
		} else
		#endif
			fr_rdn = 0;
	}

	//printk("tfreq = %d\n", tfreq);
	#if 0
	if (cpu_clk > tfreq) {
		//change cpu from faster to slower
		old_arm_div = *(volatile unsigned int *)(PMC_BASE + 0x300);
		index = set_pll_speed_t(DEV_ARM, 1, (plla_clk*1000)/old_arm_div, &divisor, 1);
		if (index < 0)
			return -1;

		if (*(volatile unsigned char *)(PMC_BASE + 0x300) != arm_div) {
			*(volatile unsigned int *)(PMC_BASE + 0x300) = arm_div;
			check_PLL_DIV_busy();
		}
		if (*(volatile unsigned int *)(PMC_BASE + 0x30C) != l2c_div) {
			*(volatile unsigned int *)(PMC_BASE + 0x30C) = l2c_div;
			check_PLL_DIV_busy();
		}
		if (*(volatile unsigned int *)(PMC_BASE + 0x3F0) != l2c_tag) {
			*(volatile unsigned int *)(PMC_BASE + 0x3F0) = l2c_tag;
			check_PLL_DIV_busy();
		}
		if (*(volatile unsigned int *)(PMC_BASE + 0x3F4) != l2c_data) {
			*(volatile unsigned int *)(PMC_BASE + 0x3F4) = l2c_data;
			check_PLL_DIV_busy();
		}
		if (*(volatile unsigned int *)(PMC_BASE + 0x3B0) != axi_div) {
			*(volatile unsigned int *)(PMC_BASE + 0x3B0) = axi_div;
			check_PLL_DIV_busy();
		}
	} else 
	#endif
	{
		//change cpu from slower to faster
		nand_clk = get_freq_t(DEV_NAND, &divisor);
		//if ((nand_clk*divisor) > plla_clk)
			*(volatile unsigned int *)(PMC_BASE + 0x300) = 2;
		/*else
			*(volatile unsigned int *)(PMC_BASE + 0x300) = arm_div;*/
		check_PLL_DIV_busy();
		if (*(volatile unsigned int *)(PMC_BASE + 0x30C) != l2c_div) {
			*(volatile unsigned int *)(PMC_BASE + 0x30C) = l2c_div;
			check_PLL_DIV_busy();
		}
		if (*(volatile unsigned int *)(PMC_BASE + 0x3F0) != l2c_tag) {
			*(volatile unsigned int *)(PMC_BASE + 0x3F0) = l2c_tag;
			check_PLL_DIV_busy();
		}
		if (*(volatile unsigned int *)(PMC_BASE + 0x3F4) != l2c_data) {
			*(volatile unsigned int *)(PMC_BASE + 0x3F4) = l2c_data;
			check_PLL_DIV_busy();
		}
		if (*(volatile unsigned int *)(PMC_BASE + 0x3B0) != axi_div) {
			*(volatile unsigned int *)(PMC_BASE + 0x3B0) = axi_div;
			check_PLL_DIV_busy();
		}
		//if ((nand_clk*divisor) > plla_clk) {
			index = set_pll_speed_t(DEV_ARM, 1, (plla_clk*1000)/2, &divisor, 1);
			if (index < 0)
				return -1;
			index = index*2;
			*(volatile unsigned int *)(PMC_BASE + 0x300) = arm_div;
		/*} else {
			index = set_pll_speed_t(DEV_ARM, 1, (plla_clk*1000)/arm_div, &divisor, 0);
			if (index < 0)
				return -1;
		}*/
	}

	return index;
}
EXPORT_SYMBOL(set_plla_divisor);

int wm_pmc_set(enum dev_id dev, enum power_cmd cmd)
{
	int retval = -1;
	unsigned int temp = 0, base = 0, mali_val = 0;
	unsigned int mali_off, mali_l2c_off = 0x00130620/*, mali_gp_off = 0x00130624*/;
	mali_off = mali_l2c_off;

	base = 0xfe000000;
	temp = REG32_VAL(base + 0x00110110);
	temp = ((temp >> 10)&0xC0)|(temp&0x3F);

	if ((cmd == DEV_PWRON)) {
		if ((temp & 0xC0) == 0x80) {//8700/8720 can't be power on
			retval = -1;
		} else {
			while ((REG32_VAL(base + mali_off)&0xF0)!= 0
			&& (REG32_VAL(base + mali_off)&0xF0)!= 0xF0)
				;

			mali_val = REG32_VAL(base + mali_off);
			if (!((mali_val&0xF0)==0xF0))
				REG32_VAL(base + mali_off) |= 1;

			while ((REG32_VAL(base + mali_off)&0xF0)!= 0
			&& (REG32_VAL(base + mali_off)&0xF0)!= 0xF0)
				;
			retval = 0;
		}
	} else if (cmd == DEV_PWROFF) {
		if ((temp & 0xC3) == 0xC0) {//8710B0 can't power off after power on
			retval = -1;
		} else {
			while ((REG32_VAL(base + mali_off)&0xF0)!= 0
			&& (REG32_VAL(base + mali_off)&0xF0)!= 0xF0)
				;
			mali_val = REG32_VAL(base + mali_off);
			if ((mali_val&0xF0))
				REG32_VAL(base + mali_off) &= ~1;
			while ((REG32_VAL(base + mali_off)&0xF0)!= 0
			&& (REG32_VAL(base + mali_off)&0xF0)!= 0xF0)
				;
			retval = 0;
		}
	} else if (cmd == DEV_PWRSTS) {
		while ((REG32_VAL(base + mali_off)&0xF0)!= 0
		&& (REG32_VAL(base + mali_off)&0xF0)!= 0xF0)
			;
		temp = REG32_VAL(base + mali_off);
		if (temp & 0xF0)
			retval = 1;
		else
			retval = 0;
	}
	return retval;
}



#define LOADER_ADDR												0xffff0000
#define HIBERNATION_ENTER_EXIT_CODE_BASE_ADDR	0xFFFFFFC0
#define DO_POWER_SET							      (HIBERNATION_ENTER_EXIT_CODE_BASE_ADDR + 0x2C)
#define CP15_C1_XPbit 23
/*
 * Function:
 * int wmt_power_dev(enum dev_id dev, enum power_cmd cmd)
 * This function is used to control/get WMT SoC specific device power state.
 * 
 * Parameter:
 *  The available dev_id is DEV_MALI cmd used to control/get device power state. 
 *  The available power_cmd are
 *  - DEV_PWRON
 *  - DEV_PWROFF
 *  - DEV_PWRSTS
 * 
 * Return:
 *  - As power_cmd are DEV_PWRON, DEV_PWROFF 0 indicates success. 
 *    Negative value indicates failure as error code.
 * 
 *  - As power_cmd is DEV_PWRSTS
 * 	 0 indicates the current device is power off.
 * 	 1 indicates the current device is power on.
 * 	 Negative value indicates failure as error code.
 */
extern int spi_read_status(int chip);
int wmt_power_dev(enum dev_id dev, enum power_cmd cmd)
{
#if 0
	static unsigned int base = 0;
	unsigned int exec_at = (unsigned int)-1, temp = 0, bootdev;
	int (*theKernel_power)(int from, enum dev_id dev, enum power_cmd cmd);
	int en_count, retval = 0, rc = 0;
	unsigned long flags;
	bootdev = GPIO_STRAP_STATUS_VAL;

	/*printk(KERN_INFO"entry dev_id=%d cmd=%d, boot_strap=%x,\n",dev, cmd, GPIO_STRAP_STATUS_VAL);*/
	/*enble SF clock*/
	if((bootdev & 0x4040) == 0)
		en_count = enable_dev_clk(DEV_SF); //SF boot
	else if((bootdev & 0x4040) == 4) 
		en_count = enable_dev_clk(DEV_NAND);//NAND boot
	else
		en_count = enable_dev_clk(DEV_SDMMC0);//MMC boot
	udelay(1);
#ifdef CONFIG_MTD_WMT_SF	
	rc = spi_read_status(0);
	if (rc)
		printk("wr c0 wait status ret=%d\n", rc);
#endif	
	/*rc = spi_read_status(1);
	if (rc)
		printk("wr c1 wait status ret=%d\n", rc);*/
	
	rc = 0;
	/*jump to loader api to do something*/
	if (base == 0)
		base = (unsigned int)ioremap/*_nocache*/(LOADER_ADDR, 0x10000);
	exec_at = base + (DO_POWER_SET - LOADER_ADDR);
	theKernel_power = (int (*)(int from, enum dev_id dev, enum power_cmd cmd))exec_at;
	/*temp = *(volatile unsigned int *)(0xFE110110);
	printk(KERN_INFO"entry exec_at=0x%x chip_id=0x%x, clock enable=0x%x\n", exec_at,
	((temp >> 10)&0xC0)|(temp&0x3F), *(volatile unsigned int *)(0xFE130250));*/
	/*backup flags and disable irq*/
	local_irq_save(flags);
	/*enable subpage AP bits*/
	asm volatile ("mrc p15, 0, %0, c1, c0, 0" : "=r" (temp));
	//if (!(temp & (1<<CP15_C1_XPbit)))
//		printk(KERN_INFO"1 xp(23) bits =0x%x cp15c0=0x%x\n", temp&(1<<CP15_C1_XPbit), temp);
	if (temp & (1<<CP15_C1_XPbit)) {
		rc = 1;
		temp &= ~(1<<CP15_C1_XPbit);
	  /*printk(KERN_INFO"2 xp(23) bits =0x%x \n", temp&(1<<CP15_C1_XPbit));*/
		asm volatile ("mcr p15, 0, %0, c1, c0, 0" : : "r" (temp));
	}
	//retval = theKernel_power(4, DEV_MALI, cmd);
	retval = wm_pmc_set(DEV_MALI, cmd);

	if (rc == 1) {
		/*disable subpage AP bits*/
		asm volatile ("mrc p15, 0, %0, c1, c0, 0" : "=r" (temp));
		/*printk(KERN_INFO"3 xp(23) bits =0x%x \n", temp&(1<<CP15_C1_XPbit));*/
		temp |= (1<<CP15_C1_XPbit);
		/*printk(KERN_INFO"4 xp(23) bits =0x%x \n", temp&(1<<CP15_C1_XPbit));*/
		asm volatile ("mcr p15, 0, %0, c1, c0, 0" : : "r" (temp));
	}
	/*restore irq flags*/
	local_irq_restore(flags);

	/*iounmap((void *)base);*/
	/*printk(KERN_INFO"entry base=0x%x exec_at=0x%x clock enable =0x%x\n",
	base, exec_at, *(volatile unsigned int *)(0xFE130250));*/

	/*disable SF clock*/
	if((bootdev & 0x4040) == 0)
		en_count = disable_dev_clk(DEV_SF); //SF boot
	else if((bootdev & 0x4040) == 4) 
		en_count = disable_dev_clk(DEV_NAND);//NAND boot
	else
		en_count = disable_dev_clk(DEV_SDMMC0);//MMC boot

	/*printk(KERN_INFO"exit!!ret = (%d)\n",retval);*/
#endif
	return 0;//retval;
}
EXPORT_SYMBOL(wmt_power_dev);

/*struct pll_map pllmapAll[] = {
{126,		0x00140003}, {126,	0x00290103}, {126, 	0x00140102},
{129,		0x002A0103},
{132,		0x00150003}, {132,	0x002B0103}, {132, 	0x00150102}, {132, 	0x000A0101}, {132, 	0x000A0002},
{135,		0x002C0103},
{138,		0x00160003}, {138,	0x002D0103}, {138, 	0x00160102},
{141,		0x002E0103},
{144,		0x00170003}, {144,	0x002F0103}, {144,	0x00020000}, {144, 	0x00170102}, {144, 	0x000B0101}, {144,	0x00050100}, {144, 	0x000B0002}, {144, 	0x00050001},
{147,		0x00300103},
{150,		0x00180003}, {150,	0x00310103}, {150, 	0x00180102},
{153,		0x00320103},
{156,		0x00190003}, {156,	0x00330103}, {156, 	0x000C0101}, {156, 	0x000C0002},
{159,		0x00340103},
{162,		0x001A0003}, {162,	0x00350103}, {162, 	0x001A0102},
{165,		0x00360103}, {156, 	0x00190102},
{168,		0x001B0003}, {168,	0x00370103}, {168, 	0x001B0102}, {168, 	0x000D0101}, {168,	0x00060100}, {168, 	0x000D0002}, {168, 	0x00060001},
{171,		0x00380103},
{174,		0x001C0003}, {174,	0x00390103}, {174, 	0x001C0102},
{177,		0x003A0103},
{180,		0x001D0003}, {180,	0x003B0103}, {180, 	0x001D0102}, {180, 	0x000E0101}, {180, 	0x000E0002},
{183,		0x003C0103},
{186,		0x001E0003}, {186, 	0x001E0102}, {186, 	0x003D0103},
{189,		0x003E0103},
{192,		0x001F0003}, {192,	0x00030000}, {192, 	0x003F0103}, {192, 	0x001F0102}, {192, 	0x000F0101}, {192,	0x00070100}, {192, 	0x000F0002}, {192, 	0x00070001},
{195,		0x00400103},
{198,		0x00200003}, {198, 	0x00200102},
{198,		0x00410103},
{201,		0x00420103},
{204,		0x00210003}, {204,	0x00430103}, {204, 	0x00210102}, {204, 	0x00100101}, {204, 	0x00100002},
{207,		0x00440103},
{210,		0x00220003}, {210,	0x00450103}, {210, 	0x00220102},
{213,		0x00460103},
{216,		0x00230003}, {216,	0x00470103}, {216, 	0x00230102}, {216, 	0x00110101}, {216, 	0x00080001}, {216,	0x00080100}, {216, 	0x00110002},
{219,		0x00480103},
{222,		0x00240003}, {222, 	0x00240102},
{222,		0x00490103},
{225,		0x004A0103},
{228,		0x004B0103}, {228, 	0x00250102}, {228, 	0x00120101}, {228,	0x00250003}, {228, 	0x00120002},
{231,		0x004C0103},
{234,		0x00260003}, {234, 	0x00260102},
{234,		0x004D0103},
{237,		0x004E0103},
{240,		0x00270003}, {240, 	0x00270102}, {240,	0x004F0103}, {240,	0x00040000}, {240, 	0x00130101}, {240, 	0x00090001}, {240,	0x00090100}, {240, 	0x00130002},
{243,		0x00500103},
{246,		0x00280003}, {246, 	0x00280102},
{246,		0x00510103},
{249,		0x00520103},
{252,		0x00290003}, {252,	0x00530103}, {252, 	0x00290102}, {252, 	0x00140002}, {252, 	0x00140101},
{258, 	0x002A0102},
{264, 	0x00150002}, {264, 	0x002B0102}, {264, 	0x00150101}, {264, 	0x000A0001}, {264,	0x000A0100},
{270, 	0x002C0102},
{276, 	0x00160002}, {276, 	0x002D0102}, {276, 	0x00160101},
{282, 	0x002E0102},
{288, 	0x00170002}, {288, 	0x002F0102}, {288,	0x00050000}, {288, 	0x000B0001}, {288, 	0x00170101}, {288,	0x000B0100},
{294, 	0x00300102},
{300, 	0x00180002}, {300, 	0x00310102}, {300, 	0x00180101},
{306, 	0x00320102},
{312, 	0x00190002}, {312, 	0x00330102}, {312, 	0x000C0001}, {312, 	0x00190101}, {312,	0x000C0100},
{318, 	0x00340102},
{324, 	0x001A0002}, {324, 	0x00350102}, {324, 	0x001A0101},
{330, 	0x00360102},
{336, 	0x001B0002}, {336, 	0x00370102}, {336,	0x00060000}, {336, 	0x000D0001}, {336, 0x001B0101}, {336,	0x000D0100},
{342, 	0x00380102},
{348, 	0x001C0002}, {348, 	0x00390102}, {384,	0x00070000}, {348, 	0x001C0101}, {384,	0x000F0100}, {384, 	0x000F0001},
{354, 	0x003A0102},
{360, 	0x001D0002}, {360, 	0x003B0102}, {360, 	0x000E0001}, {360, 	0x001D0101}, {360,	0x000E0100},
{366, 	0x003C0102},
{372, 	0x001E0002}, {372, 	0x003D0102}, {372, 	0x001E0101},
{378, 	0x003E0102},
{384, 	0x001F0002}, {384, 	0x003F0102}, {384, 	0x001F0101},
{390, 	0x00400102},
{396, 	0x00200002}, {396, 	0x00410102}, {396, 	0x00200101},
{402, 	0x00420102},
{408, 	0x00210002}, {408, 	0x00430102}, {408, 	0x00210101}, {408,	0x00100100}, {408, 0x00100001},
{414, 	0x00440102},
{420, 	0x00220002}, {420, 	0x00450102}, {420, 	0x00220101},
{426, 	0x00460102},
{432, 	0x00230002}, {432, 	0x00470102}, {432, 	0x00230101}, {432,	0x00080000}, {432, 0x00110001}, {432, 0x00110100},
{438, 	0x00480102}, {480, 	0x00130001}, {480,	0x00130100}, {480,	0x00090000},
{444, 	0x00240002}, {444, 	0x00490102}, {444, 	0x00240101},
{450, 	0x004A0102},
{456, 	0x00250002}, {456, 	0x004B0102}, {456, 	0x00250101}, {456,	0x00120100}, {456, 0x00120001},
{462, 	0x004C0102},
{468, 	0x00260002}, {468, 	0x004D0102}, {468, 	0x00260101},
{474, 	0x004E0102},
{480, 	0x00270002}, {480, 	0x004F0102}, {480, 	0x00270101},
{486, 	0x00500102},
{492, 	0x00280002}, {492, 	0x00510102}, {492, 	0x00280101},
{498, 	0x00520102},
{504, 	0x00140001}, {504, 	0x00290002}, {504,	0x00530102}, {504, 	0x00290101}, {504, 0x00140100},
{516, 	0x002A0101},
{528, 	0x00150001}, {528, 	0x002B0101}, {528,	0x00150100}, {528,	0x000A0000},
{540, 	0x002C0101},
{552, 	0x00160001}, {552, 	0x002D0101}, {552,	0x00160100},
{564, 	0x002E0101},
{576, 	0x00170001}, {576, 	0x002F0101}, {576,	0x00170100}, {576,	0x000B0000},
{588, 	0x00300101},
{600, 	0x00180001}, {600, 	0x00310101}, {600,	0x00180100},
{612, 	0x00320101},
{624, 	0x00190001}, {624, 	0x00330101}, {624,	0x000C0000}, {624,	0x00190100},
{636, 	0x00340101},
{648, 	0x001A0001}, {648, 	0x00350101}, {648,	0x001A0100},
{660, 	0x00360101},
{672, 	0x001B0001}, {672, 	0x00370101}, {672,	0x000D0000}, {672,	0x001B0100},
{684, 	0x00380101},
{696, 	0x001C0001}, {696, 	0x00390101}, {696,	0x001C0100},
{708, 	0x003A0101},
{720, 	0x001D0001}, {720, 	0x003B0101}, {720,	0x000E0000}, {720,	0x001D0100},
{732, 	0x003C0101},
{744, 	0x001E0001}, {744, 	0x003D0101}, {744,	0x001E0100},
{756, 	0x003E0101},
{768, 	0x001F0001}, {768, 	0x003F0101}, {768,	0x000F0000}, {768,	0x001F0100},
{780, 	0x00400101},
{792, 	0x00200001}, {792, 	0x00410101}, {792,	0x00200100},
{804, 	0x00420101},
{816, 	0x00210001}, {816, 	0x00430101}, {816,	0x00100000}, {816,	0x00210100},
{828, 	0x00440101},
{840, 	0x00220001}, {840, 	0x00450101}, {840,	0x00220100},
{852, 	0x00460101},
{864, 	0x00230001}, {864, 	0x00470101}, {864,	0x00230100}, {864,	0x00110000},
{876, 	0x00480101},
{888, 	0x00240001}, {888, 	0x00490101}, {888,	0x00240100},
{900, 	0x004A0101},
{912, 	0x00250001}, {912, 	0x004B0101}, {912,	0x00120000}, {912,	0x00250100},
{924, 	0x004C0101},
{936, 	0x004D0101}, {936,	0x00260100},
{948, 	0x004E0101},
{960, 	0x00270001}, {960, 	0x004F0101}, {960,	0x00270100}, {960,	0x00130000},
{972, 	0x00500101},
{984, 	0x00280001}, {984, 	0x00510101}, {984,	0x00280100},
{936, 	0x00260001}, {996, 	0x00520101},
{1008,	0x00290001}, {1008,	0x00530101}, {1008,	0x00290100}, {1008,	0x00140000},
{1032,	0x002A0100},
{1056,	0x00150000}, {1056,	0x002B0100},
{1080,	0x002C0100},
{1104,	0x00160000}, {1104,	0x002D0100},
{1128,	0x002E0100},
{1152,	0x00170000}, {1152,	0x002F0100},
{1176,	0x00300100},
{1200,	0x00180000}, {1200,	0x00310100},
{1224,	0x00320100},
{1248,	0x00190000}, {1248,	0x00330100},
{1272,	0x00340100},
{1296,	0x001A0000}, {1296,	0x00350100},
{1320,	0x00360100},
{1344,	0x001B0000}, {1344,	0x00370100},
{1368,	0x00380100},
{1392,	0x001C0000}, {1392,	0x00390100},
{1416,	0x003A0100},
{1440,	0x001D0000}, {1440,	0x003B0100},
{1464,	0x003C0100},
{1488,	0x001E0000}, {1488,	0x003D0100},
{1512,	0x003E0100},
{1536,	0x001F0000}, {1536,	0x003F0100},
{1560,	0x00400100},
{1584,	0x00200000}, {1584,	0x00410100},
{1608,	0x00420100},
{1632,	0x00210000}, {1632,	0x00430100},
{1656,	0x00440100},
{1680,	0x00220000}, {1680,	0x00450100},
{1704,	0x00460100},
{1728,	0x00230000}, {1728,	0x00470100},
{1752,	0x00480100},
{1776,	0x00240000}, {1776,	0x00490100},
{1800,	0x004A0100},
{1824,	0x004B0100}, {1824,	0x00250000},
{1848,	0x004C0100},
{1872,	0x00260000}, {1872,	0x004D0100},
{1896,	0x004E0100},
{1920,	0x00270000}, {1920,	0x004F0100},
{1944,	0x00500100},
{1968,	0x00280000}, {1968,	0x00510100},
{1992,	0x00520100},
{2016,	0x00290000}, {2016,	0x00530100}
};*/
void wmt_set_DIV(enum dev_id dev,int div)
{
	int PLL_NO, div_addr_offs;

	PLL_NO = calc_pll_num(dev, &div_addr_offs);
	check_PLL_DIV_busy();
	if (dev == DEV_WMTNA)
		*(volatile unsigned int *)(PMC_BASE + div_addr_offs) =	(0x200 | ((div == 32) ? 0 : div));
	else
		*(volatile unsigned int *)(PMC_BASE + div_addr_offs)
		=	((dev == DEV_SDTV)?0x10000:0) + ((div == 32) ? 0 : div);
	check_PLL_DIV_busy();
}

extern int vpp_parse_param(char *buf, unsigned int *param,
					int cnt, unsigned int hex_mask);

#define WMT_VDD_CONFIG
struct workqueue_struct *mmfreq_workqueue = 0;
struct work_struct mmfreq_work;
static int div_table[3][7];
static int table_num;
unsigned int mc5_08_default;
unsigned int mc5_18_default;
unsigned int mc5_20_default;

void wmt_resume_mmfreq(void)
{
	int current_voltage = 0;

	if (re) {
		current_voltage = regulator_get_voltage(re);
		regulator_set_voltage(re, current_voltage, current_voltage);
	}
	mmfreq_cur_num = ~0;
}

void wmt_suspend_mmfreq(void)
{
	if (!mmfreq_workqueue)
		return;

	flush_workqueue(mmfreq_workqueue);
}

void wmt_do_mmfreq(struct work_struct *ptr)
{
	static int init;

	int num = mmfreq_num;
	int dev_cnt;

	wmt_clk_mutex_lock(1);
	dev_cnt = dev_en_count[MMFREQ_VD];
	wmt_clk_mutex_lock(0);


//	printk("%s %d --> %d,%d\n", __FUNCTION__, mmfreq_cur_num, num,dev_cnt);

	if (init == 0) {
#ifdef WMT_VDD_CONFIG
		re = regulator_get(NULL, "wmt_vdd");
#endif
		init = 1;
	}

	if (dev_cnt)
		return;

	if (num > table_num)
		return;

	if (num == mmfreq_cur_num)
		return;

	if (mmfreq_debug) {
#ifdef WMT_VDD_CONFIG
		printk("mmfreq %d --> %d,re 0x%x\n",mmfreq_cur_num,num,(int)re);
#endif
		printk("vol %d,mali %d,vpp %d,vdu %d,vduna %d,cmn %d,cmnna %d\n",
			div_table[num][0], div_table[num][1],
			div_table[num][2], div_table[num][3],
			div_table[num][4], div_table[num][5],
			div_table[num][6]);
	}
#ifdef WMT_VDD_CONFIG
	if (mmfreq_cur_num < num) { /* lo to hi */
		regulator_set_voltage(re, div_table[num][0]*1000, div_table[num][0]*1000);
	}
#endif
	wmt_set_DIV(DEV_MALI,div_table[num][1]);
	wmt_set_DIV(DEV_VPP,div_table[num][2]);
	wmt_set_DIV(DEV_WMTVDU,div_table[num][3]);	
	wmt_set_DIV(DEV_WMTNA,div_table[num][4]);
	wmt_set_DIV(DEV_CNMVDU,div_table[num][5]);
	wmt_set_DIV(DEV_CNMNA,div_table[num][6]);
#ifdef WMT_VDD_CONFIG
	if (mmfreq_cur_num > num) { /* hi to lo */
		regulator_set_voltage(re, div_table[num][0]*1000, div_table[num][0]*1000);
	}
#endif
	mmfreq_cur_num = num;
	printk("vol %d,mali %d,vpp %d,vdu %d,vduna %d,cmn %d,cmnna %d,end wmt_do_mmfreq\n",
		div_table[num][0], div_table[num][1],
		div_table[num][2], div_table[num][3],
		div_table[num][4], div_table[num][5],
		div_table[num][6]);
}

void wmt_set_mmfreq(int num)
{
	(*(volatile unsigned int *)(MEMORY_CTRL_V4_CFG_BASE_ADDR + 0x08)) =
		(num) ? 0x00f80000 : mc5_08_default;
	(*(volatile unsigned int *)(MEMORY_CTRL_V4_CFG_BASE_ADDR + 0x18)) =
		(num) ? 0x00fa0000 : mc5_18_default;
	(*(volatile unsigned int *)(MEMORY_CTRL_V4_CFG_BASE_ADDR + 0x20)) =
		(num) ? 0x00020505 : mc5_20_default;

	if (!mmfreq_workqueue)
		return;
	
//	printk("%s %d --> %d\n", __FUNCTION__, mmfreq_cur_num, num);
	mmfreq_num = num;
	if (mmfreq_cur_num == num)
		return;

	flush_workqueue(mmfreq_workqueue);
	queue_work(mmfreq_workqueue, &mmfreq_work);
}

void wmt_enable_mmfreq(enum wmt_mmfreq_type type, int enable)
{	
	wmt_clk_mutex_lock(1);
	mmfreq_type_mask = (enable) ?
		(mmfreq_type_mask | type) : (mmfreq_type_mask & ~type);
	wmt_clk_mutex_lock(0);
	wmt_set_mmfreq((mmfreq_type_mask) ? 1 : 0);
}

int wmt_mmfreq_init(void)
{
	char buf[100];
	int varlen = 100;
	int parm[21];
	int i;
	unsigned int info;

	mc5_08_default =
	(*(volatile unsigned int *)(MEMORY_CTRL_V4_CFG_BASE_ADDR + 0x08));
	mc5_18_default =
	(*(volatile unsigned int *)(MEMORY_CTRL_V4_CFG_BASE_ADDR + 0x18));
	mc5_20_default =
	(*(volatile unsigned int *)(MEMORY_CTRL_V4_CFG_BASE_ADDR + 0x20));

	info = *(volatile unsigned int *)(CPINFO);
	if (info & 0x8)
		i = wmt_getsyspara("wmt.mmfreqlp.param", buf, &varlen);
	else
		i = wmt_getsyspara("wmt.mmfreq.param", buf, &varlen);

	if (i)
		return 0;

	table_num = vpp_parse_param(buf, (unsigned int *)parm, 21, 0x1);
	if (parm[0] & 0x1) {
		mmfreq_debug = (parm[0] & 0x10) ? 1 : 0;
#if 0
		for (i = 0; i < table_num; i++ ) {
			printk("%d:%d,",i,parm[i]);
		}
		printk("\n");
#endif
		table_num = (table_num - 3 + 1) / 9;
		printk("mmfreq.parm en 0x%x,sr %d,num %d,%d\n",parm[0],parm[1],parm[2],table_num);
		for (i=0; i<table_num; i++) {
			div_table[i][0] = parm[9 * i + 4];
			div_table[i][1] = parm[9 * i + 5];
			div_table[i][2] = parm[9 * i + 6];
			div_table[i][3] = parm[9 * i + 7];
			div_table[i][4] = parm[9 * i + 8];
			div_table[i][5] = parm[9 * i + 9];
			div_table[i][6] = parm[9 * i + 10];
			printk("vol %d,mali %d,vpp %d,vdu %d,vduna %d,cmn %d,cmnna %d\n",
				div_table[i][0], div_table[i][1],
				div_table[i][2], div_table[i][3],
				div_table[i][4], div_table[i][5],
				div_table[i][6]);
		}
	}
	else {
		table_num = 0;
		printk("mmfreq disable\n");
	}

	if (table_num >= 2) {
		mmfreq_workqueue = create_singlethread_workqueue("mmfreq_wq");
		if (!mmfreq_workqueue)
			return -1;
		INIT_WORK(&mmfreq_work,wmt_do_mmfreq);
	}
	return 0;
}
module_init(wmt_mmfreq_init);

