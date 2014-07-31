/*++
 * linux/drivers/video/wmt/hw/wmt-govrh-reg.h
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

#ifndef WMT_GOVRH_REG_H
#define WMT_GOVRH_REG_H

/* feature */
#define WMT_FTBLK_GOVRH
#ifndef CONFIG_UBOOT
#define WMT_FTBLK_GOVRH_CURSOR
#endif
#define WMT_FTBLK_GOVRH2

#define GOVRH_FRAMEBUF_ALIGN	128

struct govrh_regs {
	/* base1 */
	unsigned int cur_addr; /* 0x00 */
	unsigned int cur_width;
	unsigned int cur_fb_width;
	unsigned int cur_vcrop;
	unsigned int cur_hcrop; /* 0x10 */
	union {
		unsigned int val;
		struct {
			unsigned int start : 11;
			unsigned int reserved : 5;
			unsigned int end : 11;
		} b;
	} cur_hcoord; /* 0x14 */

	union {
		unsigned int val;
		struct {
			unsigned int start : 11;
			unsigned int reserved : 5;
			unsigned int end : 11;
		} b;
	} cur_vcoord; /* 0x18 */

	union {
		unsigned int val;
		struct {
			unsigned int enable : 1;
			unsigned int reserved : 7;
			unsigned int out_field : 1;	/* 0:frame,1-field */
		} b;
	} cur_status; /* 0x1C */

	union {
		unsigned int val;
		struct {
			unsigned int colkey : 24;
			unsigned int enable : 1;
			unsigned int invert : 1;
			unsigned int reserved : 2;
			unsigned int alpha : 1;
		} b;
	} cur_color_key; /* 0x20 */

	unsigned int reserved[3];

	union {
		unsigned int val;
		struct {
			unsigned int rgb : 1;
			unsigned int yuv422 : 1;
		} b;
	} dvo_pix; /* 0x30 */

	union {
		unsigned int val;
		struct {
			unsigned int delay : 14;
			unsigned int inv : 1;
		} b;
	} dvo_dly_sel; /* 0x34 */

	union {
		unsigned int val;
		struct {
			unsigned int cur_enable : 1;
			unsigned int mem_enable : 1;
			unsigned int reserved : 7;
			unsigned int err_sts : 1;
			unsigned int reserved2 : 6;
			unsigned int cur_sts : 1;
			unsigned int mem_sts : 1;
		} b;
	} interrupt; /* 0x38 */

	unsigned int dvo_blank_data;
	unsigned int dirpath; /* 0x40 */
	union {
		unsigned int val;
		struct {
			unsigned int v : 8;
			unsigned int u : 8;
			unsigned int y : 8;
		} b;
	} saturation; /* 0x44 */

	union {
		unsigned int val;
		struct {
			unsigned int enable : 1; 
			unsigned int format : 1; /* 0:YCbCr, 1:RGB */
		} b;
	} saturation_enable; /* 0x48 */

	unsigned int reserved2[13];
	union {
		unsigned int val;
		struct {
			unsigned int enable : 1;
			unsigned int reserved : 7;
			unsigned int h264 : 1;
		} b;
	} mif; /* 0x80 */

	unsigned int colfmt; /* 0x84, 0:422,1:420 */
	unsigned int srcfmt; /* 0x88, 0:frame,1:field */
	unsigned int dstfmt; /* 0x8C, 0:frame,1:field */
	unsigned int ysa; /* 0x90 */
	unsigned int csa;
	unsigned int pixwid;
	unsigned int bufwid;
	unsigned int vcrop; /* 0xA0 */
	unsigned int hcrop;
	unsigned int fhi;
	unsigned int colfmt2; /* 0xAC, 1-444,other refer 0x84 */
	unsigned int ysa2;	/* 0xB0 */
	unsigned int csa2;
	union {
		unsigned int val;
		struct {
			unsigned int req_num : 8;	/* Y & RGB */
			unsigned int req_num_c : 8;	/* C */
			unsigned int frame_enable : 1;
		} b;
	} mif_frame_mode; /* 0xB8 */

	unsigned int reserved3[10];
	union {
		unsigned int val;
		struct {
			unsigned int update : 1;
			unsigned int reserved : 7;
			unsigned int level : 1;	/* 0:level 1, 1:level2 */
		} b;
	} sts; /* 0xE4 */

	union {
		unsigned int val;
		struct {
			unsigned int fixed : 1;	/* 0-top, 1-bottom */
			unsigned int enable : 1;
		} b;
	} swfld; /* 0xE8 */

	unsigned int reserved4[5];
	/* base2 */
	union {
		unsigned int val;
		struct {
			unsigned int enable : 1;
			unsigned int reserved : 7;
			unsigned int mode : 1; /* 0-frame,1-field */
		} b;
	} tg_enable; /* 0x100 */

	unsigned int read_cyc;
	unsigned int h_allpxl;
	unsigned int v_allln;
	unsigned int actln_bg; /* 0x110 */
	unsigned int actln_end;
	unsigned int actpx_bg;
	unsigned int actpx_end;
	unsigned int vbie_line; /* 0x120 */
	unsigned int pvbi_line;
	unsigned int hdmi_vbisw;
	unsigned int hdmi_hsynw;
	union {
		unsigned int val;
		struct {
			unsigned int offset : 12;
			unsigned int reserved : 4;
			unsigned int field_invert : 1;
		} b;
	} vsync_offset; /* 0x130 */

	unsigned int field_status; /* 0x134, 1-BOTTOM,0-TOP */
	unsigned int reserved5[1]; /* 0x138 */
	union {
		unsigned int val;
		struct {
			unsigned int mode : 3;	/* 011-frame packing progressive format,111-frame packing interlace format */
			unsigned int inv_filed_polar : 1;
			unsigned int blank_value : 16;
			unsigned int reserved : 11;
			unsigned int addr_sel : 1;	/* in frame packing interlace mode */
		} b;
	} hdmi_3d; /* 0x13C */

	unsigned int reserved5_2[2];
	union {
		unsigned int val;
		struct {
			unsigned int outwidth : 1;	/* 0-24bit,1-12bit */
			unsigned int hsync_polar : 1;	/* 0-active high,1-active low */
			unsigned int enable : 1;
			unsigned int vsync_polar : 1;	/* 0-active high,1-active low */
			unsigned int reserved : 4;
			unsigned int rgb_swap : 2;	/* 0-RGB[7:0],1-RGB[0:7],2-BGR[7:0],3-BGR[0:7] */
			unsigned int reserved2 : 6;
			unsigned int blk_dis : 1;	/* 0-Blank Data,1-Embeded sync CCIR656 */
		} b;
	} dvo_set; /* 0x148 */

	unsigned int reserved6;
	union {
		unsigned int val;
		struct {
			unsigned int enable : 1;
			unsigned int reserved1 : 7;
			unsigned int mode : 1;
			unsigned int reserved2 : 7;
			unsigned int inversion : 1;
		} b;
	} cb_enable; /* 0x150 */

	unsigned int reserved7;
	unsigned int h_allpxl2;
	unsigned int v_allln2;
	unsigned int actln_bg2;	/* 0x160 */
	unsigned int actln_end2;
	unsigned int actpx_bg2;
	unsigned int actpx_end2;
	unsigned int vbie_line2; /* 0x170 */
	unsigned int pvbi_line2;
	unsigned int hdmi_vbisw2;
	unsigned int hdmi_hsynw2;
	union {
		unsigned int val;
		struct {
			unsigned int outwidth : 1;	/* 0-24bit,1-12bit */
			unsigned int hsync_polar : 1;	/* 0-active high,1-active low */
			unsigned int enable : 1;
			unsigned int vsync_polar : 1;	/* 0-active high,1-active low */
		} b;
	} lvds_ctrl; /* 0x180 */

	union {
		unsigned int val;
		struct {
			unsigned int pix : 2;	/* 0-YUV444,1-RGB,2-YUV422,3-RGB */
		} b;
	} lvds_ctrl2; /* 0x184 */

	unsigned int reserved_dac[12];

	union {
		unsigned int val;
		struct {
			unsigned int praf : 8;
			unsigned int pbaf : 8;
			unsigned int yaf : 8;
		} b;
	} contrast; /* 0x1B8 */

	unsigned int brightness;
	unsigned int dmacsc_coef0; /* 0x1C0 */
	unsigned int dmacsc_coef1;
	unsigned int dmacsc_coef2;
	unsigned int dmacsc_coef3;
	unsigned int dmacsc_coef4; /* 0x1D0 */
	unsigned int reserved8;
	unsigned int dmacsc_coef5;
	unsigned int dmacsc_coef6;
	union {
		unsigned int val;
		struct {
			unsigned int mode : 1;	/* 1: YUV2RGB, 0: RGB2YUV */
			unsigned int clamp : 1;	/* 0:Y,1:Y-16 */
		} b;
	} csc_mode; /* 0x1E0 */

	union {
		unsigned int val;
		struct {
			unsigned int dvo : 1;
			unsigned int vga : 1;
			unsigned int reserved1 : 1;
			unsigned int dac_clkinv : 1;
			unsigned int blank_zero : 1;
			unsigned int disp : 1;
			unsigned int lvds : 1;
			unsigned int hdmi : 1;
			unsigned int rgb_mode : 2; /* 0-YUV, 1-RGB24, 2-1555, 3-565 */
		} b;
	} yuv2rgb; /* 0x1E4 */

	unsigned int h264_input_en;	/* 0x1E8 */
	unsigned int reserved9;
	unsigned int lvds_clkinv; 	/* 0x1F0 */
	unsigned int hscale_up;		/* 0x1F4 */
	union {
		unsigned int val;
		struct {
			unsigned int mode : 3;	/* 0:888,1:555,2:666,3:565,4:original */
			unsigned int reserved : 5;
			unsigned int ldi : 1;	/* 0:shift right,1:shift left */
		} b;
	} igs_mode; /* 0x1F8 */

	union {
		unsigned int val;
		struct {
			unsigned int mode : 3;	/* 0:888,1:555,2:666,3:565,4:original */
			unsigned int reserved : 5;
			unsigned int ldi : 1;	/* 0:shift right,1:shift left */
		} b;
	} igs_mode2; /* 0x1FC */
};

/* GOVRH */
#define REG_GOVRH_BASE1_BEGIN		(GOVRH_BASE1_ADDR+0x00)
#define REG_GOVRH_YSA			(GOVRH_BASE1_ADDR+0x90)
#define REG_GOVRH_CSA			(GOVRH_BASE1_ADDR+0x94)
#define REG_GOVRH_BASE1_END		(GOVRH_BASE1_ADDR+0xe8)
#define REG_GOVRH_BASE2_BEGIN		(GOVRH_BASE2_ADDR+0x00)
#define REG_GOVRH_BASE2_END		(GOVRH_BASE2_ADDR+0xFC)

/* GOVRH2 */
#define REG_GOVRH2_BASE1_BEGIN		(GOVRH2_BASE1_ADDR+0x00)
#define REG_GOVRH2_YSA			(GOVRH2_BASE1_ADDR+0x90)
#define REG_GOVRH2_CSA			(GOVRH2_BASE1_ADDR+0x94)
#define REG_GOVRH2_BASE1_END		(GOVRH2_BASE1_ADDR+0xe8)
#define REG_GOVRH2_BASE2_BEGIN		(GOVRH2_BASE2_ADDR+0x00)
#define REG_GOVRH2_BASE2_END		(GOVRH2_BASE2_ADDR+0xFC)

#endif /* WMT_GOVRH_REG_H */
