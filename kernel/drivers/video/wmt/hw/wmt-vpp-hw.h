/*++
 * linux/drivers/video/wmt/hw/wmt-vpp-hw.h
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

#ifndef WMT_VPP_HW_H
#define WMT_VPP_HW_H

/*-------------------- EXPORTED PRIVATE CONSTANTS ----------------------------*/
/*
* Product ID / Project ID
* 84xx series: 8420/3300, 8430/3357, 8435/3437
* 85xx series: 8500/3400, 8510/3426, 8520/3429
*/
/* 84xx series, (1-100) with VDU & DSP */
#define VIA_PID_8420	10	/* 3300 */
#define VIA_PID_8430	12	/* 3357 */
#define WMT_PID_8435	14	/* 3437 */
#define WMT_PID_8440	16	/* 3451 */
#define WMT_PID_8425	18	/* 3429 */
#define WMT_PID_8710	20	/* 3445 */
#define WMT_PID_8950	22	/* 3481 */
#define WMT_PID_8980	24	/* 3498 */

/* 85xx series, (101-200) */
#define VIA_PID_8500	110	/* 3400 */
#define WMT_PID_8505	111
#define WMT_PID_8510	112	/* 3426* */

#define WMT_PID_8950_A	1

/* current pid */
#define WMT_CUR_PID	WMT_PID_8980
#define WMT_SUB_PID	0

/* #define WMT_SUB_PID	WMT_PID_8505 */
#ifndef WMT_SUB_PID
	#define WMT_SUB_PID	0
#endif

/* VPP interrupt map to irq */
#define VPP_IRQ_SCL_FINISH	IRQ_VPP_IRQ0
#define VPP_IRQ_SCL		IRQ_VPP_IRQ1
#define VPP_IRQ_SCL444_TG	IRQ_VPP_IRQ2
#define VPP_IRQ_VPPM		IRQ_VPP_IRQ3
#define VPP_IRQ_GOVW_TG		IRQ_VPP_IRQ4
#define VPP_IRQ_GOVW		IRQ_VPP_IRQ5
#define VPP_IRQ_GOVM		IRQ_VPP_IRQ6
#define VPP_IRQ_GE		IRQ_VPP_IRQ7
#define VPP_IRQ_GOVRH_TG	IRQ_VPP_IRQ8	/* PVBI or VBIS or VBIE */
#define VPP_IRQ_DVO		IRQ_VPP_IRQ9
#define VPP_IRQ_VID		IRQ_VPP_IRQ10
#define VPP_IRQ_GOVR		IRQ_VPP_IRQ11	/* underrun & mif */
#define VPP_IRQ_GOVRSD_TG	IRQ_VPP_IRQ12
#define VPP_IRQ_VPU		IRQ_VPP_IRQ13
#define VPP_IRQ_VPU_TG		IRQ_VPP_IRQ14
#define VPP_IRQ_HDMI_CP		IRQ_VPP_IRQ15
#define VPP_IRQ_HDMI_HPDH	IRQ_VPP_IRQ16
#define VPP_IRQ_HDMI_HPDL	IRQ_VPP_IRQ17
#define VPP_IRQ_GOVR_0		IRQ_VPP_IRQ18
#define VPP_IRQ_GOVR_2		IRQ_VPP_IRQ19
#define VPP_IRQ_CEC		IRQ_VPP_IRQ20
#define VPP_IRQ_GOVR2_0		IRQ_VPP_IRQ21
#define VPP_IRQ_GOVR2		IRQ_VPP_IRQ22
#define VPP_IRQ_GOVR2_2		IRQ_VPP_IRQ23
#define VPP_IRQ_DVO2		IRQ_VPP_IRQ24
#define VPP_IRQ_GOVR2_TG	IRQ_VPP_IRQ25

/* DVI I2C */
#define VPP_DVI_I2C_BIT		0x80 /* use sw id that can vary */
#define VPP_DVI_I2C_SW_BIT 	0x10 /* hw or sw i2c */
#define VPP_DVI_I2C_ID_MASK 	0x1F
#define VPP_DVI_I2C_ID 		(VPP_DVI_I2C_BIT + 0x1)
#define VPP_DVI_EDID_ID 	(VPP_DVI_I2C_SW_BIT + 0x1) /* DVO EDID use sw i2c bus 1 */

/* vout */
#define VPP_VOUT_INFO_NUM 	2 /* linux fb or govr number */

#define VPP_VOUT_NUM 		2
#define VPP_VOUT_ALL 		0xFFFFFFFF
#define VPP_VOUT_NUM_HDMI 	0
#define VPP_VOUT_NUM_LVDS 	1
#define VPP_VOUT_NUM_DVI 	1

#define WMT_FTBLK_VOUT_DVI
#define WMT_FTBLK_VOUT_HDMI
#define WMT_FTBLK_VOUT_LVDS

/* hw parameter */
#define VPP_VOINT_NO 		0  /* DVO external board interrupt use GPIOxx */
#define VPP_UBOOT_COLFMT 	VDO_COL_FMT_RGB_565
#define VPP_FB_ADDR_ALIGN 	64
#define VPP_FB_WIDTH_ALIGN 	64   /* hw should 4 byte align,android
	framework 8 byte align modify by aksenxu VPU need 64bytes alignment
	you need modify FramebufferNativeWindow::FramebufferNativeWindow
	in android framework together */
#define VPP_GOVR_DVO_DELAY_24 	0x4036
#define VPP_GOVR_DVO_DELAY_12 	0x120

/*-------------------- DEPENDENCY -------------------------------------*/
#ifdef __KERNEL__
#ifndef CONFIG_WMT_HDMI
#undef WMT_FTBLK_VOUT_HDMI
#endif
#endif

#include "wmt-vpp-reg.h"
#include "wmt-govrh-reg.h"
#include "wmt-lvds-reg.h"
#ifdef WMT_FTBLK_VOUT_HDMI
#include "wmt-hdmi-reg.h"
#endif
#ifndef CFG_LOADER
#include "wmt-scl-reg.h"
#include "wmt-cec-reg.h"
#endif
#endif /* WMT_VPP_HW_H */
