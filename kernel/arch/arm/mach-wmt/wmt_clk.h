/*++
linux/arch/arm/mach-wmt/wmt_clk.h

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
#ifndef _WMT_CLK_H_
#define _WMT_CLK_H_

/*
 *   WMT_CLK register struct
 *
 */

enum dev_id {
 DEV_SDMMC0 = 0, /* PMC lower register offset PMC OFFSET + 0x250*/
 DEV_SDMMC1 = 1,
 DEV_SDMMC2 = 2,
 /*DEV_SDMMC3 = 3,
 DEV_ETHMAC = 4,*/
 DEV_C24MOUT = 6,/*DEV_ETHPHY = 6,*/
 DEV_RTC = 7,
 DEV_I2C0 = 8,
 DEV_I2C1 = 9,
 DEV_I2C2 = 10,
 DEV_GPIO = 11,
 DEV_I2C3 = 12,
 /*DEV_KEYPAD = 14,*/
 DEV_CNMVDU = 15,/*DEV_EBM = 15,*/
 DEV_PAXI = 16,
 DEV_PWM = 17,
 DEV_ADC = 18,
 DEV_I2C4 = 19,
 DEV_SCC = 21,
 DEV_SYS = 22,
 DEV_AMP = 24,
 DEV_MALI = 26,
 DEV_PCM0 = 27,
 DEV_PCM1 = 28,
 DEV_PERM = 30,
 DEV_MBOX = 31,
 

 DEV_DDRMC = 32, /* PMC upper register offset 0xd8130254 */
 DEV_ARF   = 32+3,
 DEV_ARFP  = 32+4,
 DEV_DMA   = 32+5,
 DEV_PDMA  = 32+6,
 /*DEV_VDMA  = 32+7,*/
 DEV_UHDC  = 32+9,
 DEV_AHBB  = 32+13,
 DEV_NAND  = 32+16,
 /*DEV_NOR   = 32+17,*/
 DEV_SPI0  = 32+19,
 DEV_SPI1  = 32+20,
 DEV_SF    = 32+23,
 DEV_UART0 = 32+24,
 DEV_UART1 = 32+25,
 DEV_UART2 = 32+26,
 DEV_UART3 = 32+27,
 DEV_CSI0 = 32+28,/*DEV_UART4 = 32+28,*/
 DEV_CSI1 = 32+29,/*DEV_UART5 = 32+29,*/

 DEV_WMTNA   = 64,/*DEV_NA0   = 64,*/ /* PMC upper register offset 0xd8130258 */
 /*DEV_NA0REF= 64+1,*/
 DEV_CNMNA = 64+2,
 DEV_JDEC  = 64+3,
 DEV_MSVD  = 64+4,
 DEV_VP8DEC= 64+5,
 DEV_SAE   = 64+6,
 DEV_HDCE  = 64+7,
 DEV_H264  = 64+8,
 DEV_JENC  = 64+9,
 DEV_LVDS  = 64+14,
 DEV_CIR   = 64+15,
 DEV_NA12  = 64+16,
 DEV_VPU   = 64+17,
 DEV_VPP   = 64+18,
 DEV_VID   = 64+19,
 DEV_WMTVDU  = 64+20,/*DEV_VDU   = 64+20,*/
 DEV_SCL444U = 64+21,
 DEV_HDMII2C = 64+22,
 DEV_HDMI  = 64+23,
 DEV_GOVW  = 64+24,
 DEV_GOVRHD= 64+25,
 DEV_GE    = 64+26,
 DEV_DISP  = 64+27,
 DEV_DVO   = 64+29,
 DEV_HDMILVDS = 64+30,
 DEV_SDTV  = 64+31,
 
 DEV_I2S   = 96+2, /* PMC upper register offset 0xd8130258 */
 /*DEV_ROT,			
 DEV_XD,*/


 DEV_ARM, /* number >= 128 has no clk_en to enable clk */
 DEV_AHB,
 DEV_APB,
 DEV_L2C,
 DEV_L2CAXI,
 DEV_L2CPAXI,
 DEV_AT,
 DEV_PERI,
 DEV_TRACE,
 DEV_DBG,
};

enum clk_cmd {
CLK_DISABLE = 0,
GET_FREQ,
CLK_ENABLE,
SET_DIV,
SET_PLL,
SET_PLLDIV,
GET_CPUTIMER
};

enum power_cmd {
DEV_PWRON = 0,
DEV_PWROFF,
DEV_PWRSTS,
};

struct plla_param {
unsigned int plla_clk;
unsigned int arm_div;
unsigned int l2c_div;
unsigned int l2c_tag_div;
unsigned int l2c_data_div;
unsigned int axi_div;
unsigned int tb_index;
};

struct pll_map {
unsigned int freq;
unsigned int pll;
};

enum wmt_mmfreq_type {
	WMT_MMFREQ_HDMI_PLUG = 0x01,
	WMT_MMFREQ_MIRACAST = 0x02,
	WMT_MMFREQ_MULTI_VD = 0x04
};

extern int auto_pll_divisor(enum dev_id dev, enum clk_cmd cmd, int unit, int freq);
extern int manu_pll_divisor(enum dev_id dev, int DIVF, int DIVR, int DIVQ, int dev_div);
extern int set_plla_divisor(struct plla_param *plla_env);
extern int wmt_power_dev(enum dev_id dev, enum power_cmd cmd);
extern void wmt_resume_mmfreq(void);
extern void wmt_suspend_mmfreq(void);
extern void wmt_set_mmfreq(int num);
extern void wmt_enable_mmfreq(enum wmt_mmfreq_type type, int enable);
#endif /* __WMT_CLK_H__*/
