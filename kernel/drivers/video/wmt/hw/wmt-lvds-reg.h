/*++
 * linux/drivers/video/wmt/hw/wmt-lvds-reg.h
 * WonderMedia video post processor (VPP) driver
 *
 * Copyright c 2013  WonderMedia  Technologies, Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * WonderMedia Technologies, Inc.
 * 4F, 533, Chung-Cheng Road, Hsin-Tien, Taipei 231, R.O.C
--*/

#ifndef WMT_LVDS_REG_H
#define WMT_LVDS_REG_H

#define WMT_FTBLK_LVDS

#define REG_LVDS_BEGIN		(LVDS_BASE_ADDR + 0x00)
#define REG_LVDS_STATUS		(LVDS_BASE_ADDR + 0x00)
#define REG_LVDS_TEST		(LVDS_BASE_ADDR + 0x04)
#define REG_LVDS_LEVEL		(LVDS_BASE_ADDR + 0x08)
#define REG_LVDS_IGS		(LVDS_BASE_ADDR + 0x0C)
#define REG_LVDS_SET		(LVDS_BASE_ADDR + 0x10)
#define REG_LVDS_SET2		(LVDS_BASE_ADDR + 0x14)
#define REG_LVDS_DETECT		(LVDS_BASE_ADDR + 0x18)
#define REG_LVDS_TEST2		(LVDS_BASE_ADDR + 0x1C)
#define REG_LVDS_END		(LVDS_BASE_ADDR + 0x1C)

/* REG_LVDS_STATUS,0x00 */
#define LVDS_TEST		REG_LVDS_STATUS, 0xF00, 8
#define LVDS_DUAL_CHANNEL	REG_LVDS_STATUS, BIT4, 4
#define LVDS_INV_CLK		REG_LVDS_STATUS, BIT0, 0

/* REG_LVDS_TEST,0x04 */
#define LVDS_PLL_R_F		REG_LVDS_TEST, BIT18, 18
#define LVDS_PLL_CPSET		REG_LVDS_TEST, 0x30000, 16
#define LVDS_PLLCK_DLY		REG_LVDS_TEST, 0x7000, 12
#define LVDS_TRE_EN		REG_LVDS_TEST, 0x600, 9
#define LVDS_PD			REG_LVDS_TEST, BIT8, 8
#define LVDS_VBG_SEL		REG_LVDS_TEST, 0xC, 2
#define LVDS_DRV_PDMODE		REG_LVDS_TEST, BIT0, 0

/* REG_LVDS_LEVEL,0x08 */
#define LVDS_REG_LEVEL		REG_LVDS_LEVEL, BIT8, 8
#define LVDS_REG_UPDATE		REG_LVDS_LEVEL, BIT0, 0

/* REG_LVDS_IGS,0x0C */
#define LVDS_LDI_SHIFT_LEFT	REG_LVDS_IGS, BIT8, 8 /* 0-shift right,1-shift left */
#define LVDS_IGS_BPP_TYPE	REG_LVDS_IGS, 0x7, 0 /* 0-888,1-555,2-666,3-565 */

/* REG_LVDS_SET,0x10 */
#define LVDS_VSYNC_POLAR_LO	REG_LVDS_SET, BIT3, 3 /* 0-active high,1-active low */
#define LVDS_DVO_ENABLE		REG_LVDS_SET, BIT2, 2
#define LVDS_HSYNC_POLAR_LO	REG_LVDS_SET, BIT1, 1 /* 0-active high,1-active low */
#define LVDS_OUT_DATA_12	REG_LVDS_SET, BIT0, 0 /* 0-24bit,1-12bit */

/* REG_LVDS_SET2,0x14 */
#define LVDS_COLFMT_YUV422	REG_LVDS_SET2, BIT1, 1 /* 0-RGB or YUV444,1-YUV422 */
#define LVDS_COLFMT_RGB		REG_LVDS_SET2, BIT0, 0

/* REG_LVDS_DETECT,0x18 */
#define LVDS_RSEN		REG_LVDS_DETECT, BIT8, 8
#define LVDS_PLL_READY		REG_LVDS_DETECT, BIT0, 0

/* REG_LVDS_TEST2,0x1C */
#define LVDS_PLL_TSYNC		REG_LVDS_TEST2, BIT0, 0
#define LVDS_TP2S_TYPE		REG_LVDS_TEST2, BIT1, 1
#define LVDS_DIV_SEL		REG_LVDS_TEST2, 0xC, 2
#define LVDS_PD_V2I		REG_LVDS_TEST2, BIT4, 4
#define LVDS_VCO_SX		REG_LVDS_TEST2, BIT5, 5
#define LVDS_VCO_MODE		REG_LVDS_TEST2, BIT6, 6
#define LVDS_VSREF_SEL		REG_LVDS_TEST2, 0x300, 8
#define LVDS_MODE		REG_LVDS_TEST2, BIT10, 10
#define LVDS_PD_L2HA		REG_LVDS_TEST2, BIT11, 11
#define LVDS_PD_L2HB		REG_LVDS_TEST2, BIT12, 12
#define LVDS_L2HA_HSEN		REG_LVDS_TEST2, BIT13, 13
#define LVDS_RESA_EN		REG_LVDS_TEST2, BIT14, 14
#define LVDS_RESA_S		REG_LVDS_TEST2, BIT15, 15
#define LVDS_PLL_LPFS		REG_LVDS_TEST2, 0x30000, 16

#endif /* WMT_LVDS_REG_H */
