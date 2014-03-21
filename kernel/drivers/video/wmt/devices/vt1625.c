/*++
 * linux/drivers/video/wmt/vt1625.c
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

#define VT1625_C
#define DEBUG
/* #define DEBUG_DETAIL */
/*----------------------- DEPENDENCE -----------------------------------------*/
#include "../vout.h"

/*----------------------- PRIVATE MACRO --------------------------------------*/
/* #define  VT1625_XXXX  xxxx    *//*Example*/
// #define CONFIG_VT1625_INTERRUPT
#define CONFIG_VT1625_POWER

/*----------------------- PRIVATE CONSTANTS ----------------------------------*/
/* #define VT1625_XXXX    1     *//*Example*/
#define VT1625_ADDR 0x40

enum {
	VT1625_INPUT_SELECTION = 0x00,
	VT1625_SYNC_SELECTION_0 = 0x01,
	VT1625_SYNC_SELECTION_1 = 0x02,
	VT1625_FILTER_SELECTION = 0x03,
	VT1625_OUTPUT_MODE = 0x04,
	VT1625_CLOCK_CONTROL = 0x05,

	/* start position & overflow */
	VT1625_OVERFLOW = 0x06,
	VT1625_START_ACTIVE_VIDEO = 0x07,
	VT1625_START_HORIZONTAL_POSITION = 0x08,
	VT1625_START_VERTICAL_POSITION = 0x09,
	
	/* amplitude factor */
	VT1625_CR_AMPLITUDE_FACTOR = 0x0A,
	VT1625_BLACK_LEVEL = 0x0B,
	VT1625_Y_AMPLITUDE_FACTOR = 0x0C,
	VT1625_CB_AMPLITUDE_FACTOR = 0x0D,

	VT1625_POWER_MANAGEMENT = 0x0E,
	VT1625_STATUS = 0x0F,

	/* Hue */
	VT1625_HUE_ADJUSTMENT = 0x10,
	VT1625_OVERFLOW_MISC = 0x11,

	/* PLL */
	VT1625_PLL_P2 = 0x12,
	VT1625_PLL_D = 0x13,
	VT1625_PLL_N = 0x14,
	VT1625_PLL_OVERFLOW = 0x15,

	/* Sub Carrier */
	VT1625_SUBCARRIER_VALUE_0 = 0x16,
	VT1625_SUBCARRIER_VALUE_1 = 0x17,
	VT1625_SUBCARRIER_VALUE_2 = 0x18,
	VT1625_SUBCARRIER_VALUE_3 = 0x19,

	VT1625_VERSION_ID = 0x1B,
	VT1625_DAC_OVERFLOW = 0x1C,
	
	/* test */
	VT1625_TEST_0 = 0x1D,
	VT1625_TEST_1 = 0x1E,

	VT1625_FILTER_SWITCH = 0x1F,
	VT1625_TV_SYNC_STEP = 0x20,
	VT1625_TV_BURST_ENVELOPE_STEP = 0x21,
	VT1625_TV_SUB_CARRIER_PHASE_ADJUST = 0x22,
	VT1625_TV_BLANK_LEVEL = 0x23,
	VT1625_TV_SIGNAL_OVERFLOW = 0x24,

	/* DAC & GPO */
	VT1625_DAC_SELECTION_0 = 0x4A,
	VT1625_DAC_SELECTION_1 = 0x4B,
	VT1625_GPO = 0x4C,

	VT1625_COLBAR_LUMA_DELAY = 0x4D,
	VT1625_UV_DELAY = 0x4E,
	VT1625_BURST_MAX_AMPLITUDE = 0x4F,

	/* Graphic timing */
	VT1625_GRAPHIC_H_TOTAL = 0x50,
	VT1625_GRAPHIC_H_ACTIVE = 0x51,
	VT1625_GRAPHIC_H_OVERFLOW = 0x52,
	VT1625_GRAPHIC_V_TOTAL = 0x53,
	VT1625_GRAPHIC_V_OVERFLOW = 0x54,

	/* TV timing */
	VT1625_TV_H_TOTAL = 0x55,
	VT1625_TV_H_ACTIVE = 0x56,
	VT1625_TV_H_SYNC_WIDTH = 0x57,
	VT1625_TV_H_OVERFLOW = 0x58,
	VT1625_TV_BURST_START = 0x59,
	VT1625_TV_BURST_END = 0x5A,
	VT1625_TV_VIDEO_START = 0x5B,
	VT1625_TV_VIDEO_END = 0x5C,
	VT1625_TV_VIDEO_OVERFLOW = 0x5D,

	/* scale factor */
	VT1625_V_SCALE_FACTOR = 0x5E,
	VT1625_H_SCALE_FACTOR = 0x5F,
	VT1625_SCALE_OVERFLOW = 0x60,
	VT1625_H_BLUR_SCALE_OVERFLOW = 0x61,
	VT1625_ADAPTIVE_DEFLICKER_THR = 0x62,
	VT1625_SCALE_H_TOTAL = 0x63,
	VT1625_SCALE_H_TOTAL_OVERFLOW = 0x64,

	/* Amplitude factor */
	VT1625_PY_AMP_FACTOR = 0x65,
	VT1625_PB_AMP_FACTOR = 0x66,
	VT1625_PR_AMP_FACTOR = 0x67,

	VT1625_POST_ADJUST = 0x68,
	VT1625_AUTO_CORRECT_SENSE = 0x69,

	/* WSS 0x6A - 0x73 */
	VT1625_INT_WSS_2 = 0x71,

	/* Close Caption 0x74 - 0x7A */

	/* Signature Value 0x7B - 0x82 */
} vt1625_reg_t;

/*----------------------- PRIVATE TYPE  --------------------------------------*/
/* typedef  xxxx vt1625_xxx_t; *//*Example*/

/*----------EXPORTED PRIVATE VARIABLES are defined in vt1625.h  -------------*/
/*----------------------- INTERNAL PRIVATE VARIABLES - -----------------------*/
/* int  vt1625_xxx;        *//*Example*/

const char vt1625_ntsc_param[] = {
	0x03, 0x00, 0x10, 0x10, 0x00, 0x00, 0x00, 0x88, /* 00 - 07 */
	0x09, 0x05, 0x6E, 0x15, 0x52, 0x4E, 0x37, 0xB7, /* 08 - 0f */
	0x08, 0x80, 0x04, 0x08, 0x08, 0x90, 0xD6, 0x7B, /* 10 - 17 */
	0xF0, 0x21, 0x02, 0x50, 0x43, 0x80, 0x00, 0x10, /* 18 - 1f */
	0x16, 0x08, 0xDC, 0x7D, 0x02, 0x56, 0x33, 0x8F, /* 20 - 27 */
	0x58, 0x00, 0x00, 0xA6, 0x29, 0xD4, 0x81, 0x00, /* 28 - 2f */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 30 - 37 */ 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 38 - 3f */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 40 - 47 */
	0x00, 0x00, 0xC5, 0x0F, 0x00, 0x01, 0x10, 0x44, /* 48 - 4f */
	0x59, 0xCF, 0x23, 0x0C, 0x02, 0x59, 0xCF, 0x7F, /* 50 - 57 */
	0x23, 0x94, 0xD6, 0xFE, 0x78, 0x06, 0x00, 0x00, /* 58 - 5f */
	0x80, 0x28, 0xFF, 0x59, 0x03, 0x55, 0x56, 0x56, /* 60 - 67 */
	0x00, 0x90, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, /* 68 - 6f */
	0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 70 - 77 */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 78 - 7f */
	0x00, 0x00, 0x00
};

const char vt1625_pal_param[] = {
	0x03, 0x00, 0x00, 0x10, 0x03, 0x00, 0x00, 0x8f, /* 00 - 07 */
	0x00, 0x01, 0x7A, 0x00, 0x55, 0x58, 0x37, 0xB7, /* 08 - 0f */
	0x08, 0x80, 0x04, 0x08, 0x08, 0x90, 0xCB, 0x8A, /* 10 - 17 */
	0x09, 0x2A, 0x06, 0x50, 0x41, 0x80, 0x00, 0x00, /* 18 - 1f */
	0x17, 0x0C, 0x4E, 0x85, 0x02, 0x5F, 0x34, 0x8C, /* 20 - 27 */
	0x4F, 0x5E, 0x15, 0xA2, 0x22, 0x80, 0xD3, 0x10, /* 28 - 2f */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 30 - 37 */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 38 - 3f */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 40 - 47 */
	0x00, 0x00, 0xC5, 0x0F, 0x00, 0x01, 0x10, 0x4C, /* 48 - 4f */
	0x5f, 0xCF, 0x23, 0x70, 0x02, 0x5F, 0xD0, 0x7F, /* 50 - 57 */
	0x23, 0x92, 0xCE, 0xDF, 0x86, 0x06, 0x00, 0x00, /* 58 - 5f */
	0x80, 0x20, 0xFF, 0x5F, 0x03, 0x5f, 0x00, 0x00, /* 60 - 67 */
	0x00, 0x90, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, /* 68 - 6f */
	0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 70 - 77 */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 78 - 7f */
	0x00, 0x00, 0x00
};

int vt1625_vga_mode;
int vt1625_tv_mode;

/*--------------------- INTERNAL PRIVATE FUNCTIONS ---------------------------*/
/* void vt1625_xxx(void); *//*Example*/

/*----------------------- Function Body --------------------------------------*/

/*the define and struct i2c_msg were declared int linux/i2c.h*/
void vt1625_reg_dump(void)
{
	int i;
	char buf[256];

	vpp_i2c_read(VPP_DVI_I2C_ID, VT1625_ADDR, VT1625_INPUT_SELECTION,
		buf, 128);
	for (i = 0; i < 128; i += 8) {
		MSG("%d : 0x%02x 0x%02x 0x%02x 0x%02x",
			i, buf[i], buf[i + 1], buf[i + 2], buf[i + 3]);
		MSG(" 0x%02x 0x%02x 0x%02x 0x%02x\n",
			buf[i + 4], buf[i + 5], buf[i + 6], buf[i + 7]);
	}
}

void vt1625_set_tv_mode(int ntsc)
{
	char *p;
	char buf[10];

	vpp_i2c_read(VPP_DVI_I2C_ID, VT1625_ADDR,
			VT1625_INPUT_SELECTION, buf, 5);
	DBG_MSG("ntsc %d, 0x%x, 0x%x\n", ntsc, buf[0], buf[4]);
	vt1625_tv_mode = (ntsc) ? 1 : 2;
	if (buf[0] && (vt1625_vga_mode != -1)) {
		if (ntsc && !(buf[4] & BIT0))
			return;
		if (!ntsc && (buf[4] & BIT0))
			return;
	}

	DBG_MSG("tv %s,vga %d\n", (ntsc) ? "NTSC" : "PAL", vt1625_vga_mode);

	p = (char *)((ntsc) ?  vt1625_ntsc_param : vt1625_pal_param);
	vpp_i2c_write(VPP_DVI_I2C_ID, VT1625_ADDR, VT1625_INPUT_SELECTION,
				&p[VT1625_INPUT_SELECTION], 0x71);
	if (vt1625_vga_mode == -1) {
		mdelay(10);
		vpp_i2c_read(VPP_DVI_I2C_ID, VT1625_ADDR,
			VT1625_STATUS, buf, 1);
		vt1625_vga_mode = (buf[0] & 0x7) ? 0 : 1;
		DBG_MSG("get vga mode %d, 0x%x\n", vt1625_vga_mode, buf[0]);
	}
	
	if (vt1625_vga_mode) {
		vpp_i2c_read(VPP_DVI_I2C_ID, VT1625_ADDR,
			VT1625_SYNC_SELECTION_1, buf, 1);
		buf[0] |= 0xA0;
		vpp_i2c_write(VPP_DVI_I2C_ID, VT1625_ADDR,
			VT1625_SYNC_SELECTION_1, buf, 1);
		
		vpp_i2c_read(VPP_DVI_I2C_ID, VT1625_ADDR,
			VT1625_DAC_OVERFLOW, buf, 1);
		buf[0] |= 0x20;
		vpp_i2c_write(VPP_DVI_I2C_ID, VT1625_ADDR,
			VT1625_DAC_OVERFLOW, buf, 1);

		vpp_i2c_read(VPP_DVI_I2C_ID, VT1625_ADDR,
			VT1625_TEST_1, buf, 1);
		buf[0] |= 0x40;
		vpp_i2c_write(VPP_DVI_I2C_ID, VT1625_ADDR,
			VT1625_TEST_1, buf, 1);
	} else {
#ifdef CONFIG_VT1625_INTERRUPT
		/* interrupt (VGA no work) */
		if (!g_vpp.virtual_display) {
			vpp_i2c_read(VPP_DVI_I2C_ID, VT1625_ADDR,
				VT1625_INT_WSS_2, buf, 1);
			buf[0] |= 0xA0; /* enable sense interrupt */
			vpp_i2c_write(VPP_DVI_I2C_ID, VT1625_ADDR,
				VT1625_INT_WSS_2, buf, 1);
		}
#endif
	}
#ifdef CONFIG_VT1625_POWER
	buf[0] = (vt1625_vga_mode) ? 0x38 : 0x37;
	vpp_i2c_write(VPP_DVI_I2C_ID, VT1625_ADDR,
		VT1625_POWER_MANAGEMENT, buf, 1);
#endif
//	vt1625_reg_dump();
}

int vt1625_check_plugin(int hotplug)
{
	char buf[1];
	int plugin;

	if (g_vpp.virtual_display)
		return 1;

	vpp_i2c_read(VPP_DVI_I2C_ID, VT1625_ADDR, VT1625_STATUS, buf, 1);
	plugin = ((buf[0] & 0x3F) == 0x3F) ? 0 : 1;
	DBG_MSG("[VT1625] DAC A %d, B %d, C %d, D %d, E %d, F %d\n",
		(buf[0] & 0x20) ? 1 : 0, (buf[0] & 0x10) ? 1 : 0,
		(buf[0] & 0x08) ? 1 : 0, (buf[0] & 0x04) ? 1 : 0,
		(buf[0] & 0x02) ? 1 : 0, (buf[0] & 0x01) ? 1 : 0);
	return plugin;
}

int vt1625_init(struct vout_s *vo)
{
	char buf[5];

	DBG_MSG("\n");
	// vt1625_vga_mode = -1;
	vt1625_vga_mode = 0;
	if (vt1625_tv_mode) { /* resume reinit */
		MSG("[VT1625] DVI reinit\n");
		vt1625_set_tv_mode((vt1625_tv_mode == 1) ? 1 : 0);
		return 0;
	}
	
	vpp_i2c_read(VPP_DVI_I2C_ID, VT1625_ADDR, VT1625_VERSION_ID, buf, 1);
	if (buf[0] != 0x50) /* check version id */
		return -1;
	vo->option[0] = VDO_COL_FMT_ARGB;
	vo->option[1] = VPP_DATAWIDHT_12;
	
	vpp_i2c_read(VPP_DVI_I2C_ID, VT1625_ADDR,
			VT1625_INPUT_SELECTION, buf, 5);
	if (buf[0]) {
		vt1625_tv_mode = (buf[4]) ? 2 : 1;
	}
	MSG("[VT1625] DVI ext device\n");
	return 0;
}

int vt1625_set_mode(unsigned int *option)
{
	DBG_MSG("\n");
#ifdef CONFIG_VT1625_INTERRUPT
	vout_set_int_type(1);
#endif
	return 0;
}

void vt1625_set_power_down(int enable)
{
	vout_t *vo;
	char buf[1];
	char cur[1];

	/*
	bit 0-2 : DAC D/E/F - VGA
	bit 3 : DAC C - CVBS
	bit 3-5 : DAC A/B/C - YPbPr
	bit 6 : PLL
	bit 7 : IO pad
	*/
	vo = vout_get_entry(VPP_VOUT_NUM_DVI);
	if (vo->status & (VPP_VOUT_STS_BLANK + VPP_VOUT_STS_POWERDN)) {
		enable = 1;
	}

	if (vt1625_tv_mode == 0) /* power down for not support resolution */
		buf[0] = 0xFF;
	else if (vt1625_vga_mode == -1)
		buf[0] = (enable) ? 0xFF : 0x37;
	else
		buf[0] = (enable) ? 0xFF : ((vt1625_vga_mode) ? 0x38 : 0x37);

	vpp_i2c_read(VPP_DVI_I2C_ID, VT1625_ADDR,
		VT1625_POWER_MANAGEMENT, cur, 1);

	if (cur[0] == buf[0])
		return;

	DBG_MSG("enable %d,cur 0x%x,new 0x%x\n", enable, cur[0], buf[0]);
#if 1
	if (enable == 0) {
		cur[0] &= ~0x40; /* turn on PLL */
		vpp_i2c_write(VPP_DVI_I2C_ID, VT1625_ADDR,
			VT1625_POWER_MANAGEMENT, cur, 1);
		mdelay(3);
		
 		cur[0] &= ~0x80; /* turn on IO pad */
		vpp_i2c_write(VPP_DVI_I2C_ID, VT1625_ADDR,
			VT1625_POWER_MANAGEMENT, cur, 1);
		mdelay(3);
	}
#endif
#ifdef CONFIG_VT1625_POWER
	vpp_i2c_write(VPP_DVI_I2C_ID, VT1625_ADDR,
		VT1625_POWER_MANAGEMENT, buf, 1);
#endif
}

int vt1625_config(vout_info_t *info)
{
	int ntsc = -1;

	DBG_MSG("%d,%d\n", info->resx, info->resy);
	if (info->resx == 720) {
		switch (info->resy) {
		case 480:
			ntsc = 1;
			break;
		case 576:
			ntsc = 0;
			break;
		default:
			break;
		}
	}

	if (ntsc != -1)
		vt1625_set_tv_mode(ntsc);
	else
		vt1625_tv_mode = 0;
	DBG_MSG("end\n");
	return 0;
}

int vt1625_get_edid(char *buf)
{
	return 0;
}

int vt1625_interrupt(void)
{
	char buf[1];

	vppif_reg32_write(GPIO_BASE_ADDR + 0x4c0, 0x1 << VPP_VOINT_NO,
			VPP_VOINT_NO, 0x0); /* GPIO pull-up */
	/* interrupt */
	vpp_i2c_read(VPP_DVI_I2C_ID, VT1625_ADDR, VT1625_INT_WSS_2, buf, 1);
	DBG_MSG("0x%x\n", buf[0]);
	buf[0] &= ~0x40; /* clear interrupt */
	vpp_i2c_write(VPP_DVI_I2C_ID, VT1625_ADDR, VT1625_INT_WSS_2, buf, 1);
	return vt1625_check_plugin(1);
}
/*----------------------- vout device plugin ---------------------------------*/
vout_dev_t vt1625_vout_dev_ops = {
	.name = "VT1625",
	.mode = VOUT_INF_DVI,

	.init = vt1625_init,
	.set_power_down = vt1625_set_power_down,
	.set_mode = vt1625_set_mode,
	.config = vt1625_config,
	.check_plugin = vt1625_check_plugin,
	.get_edid = vt1625_get_edid,
#ifdef CONFIG_VT1625_INTERRUPT
	.interrupt = vt1625_interrupt,
#endif
};

int vt1625_module_init(void)
{
	vout_device_register(&vt1625_vout_dev_ops);
	return 0;
}
module_init(vt1625_module_init);
/*--------------------End of Function Body -----------------------------------*/
#undef VT1625_C
