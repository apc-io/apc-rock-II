/*++
 * linux/drivers/video/wmt/vpp.h
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

#include "vpp-osif.h"
#include "./hw/wmt-vpp-hw.h"
#include "com-vpp.h"
#include "vout.h"

#ifndef VPP_H
#define VPP_H

/* #define CONFIG_VPP_SHENZHEN */ /* for ShenZhen code */

/* VPP feature config */
/* #define CONFIG_VPP_DEMO */ /* HDMI EDID, CP disable */
#define CONFIG_VPP_STREAM_CAPTURE /* stream capture current video display */
#define CONFIG_VPP_STREAM_BLOCK
#define CONFIG_VPP_STREAM_FIX_RESOLUTION
#define CONFIG_VPP_STREAM_ROTATE
/* #define CONFIG_VPP_DISABLE_PM */	/* disable power management */
#define CONFIG_VPP_VIRTUAL_DISPLAY	/* virtual fb dev */
#define CONFIG_VPP_NOTIFY
#define CONFIG_VPP_DYNAMIC_ALLOC  /* frame buffer dynamic allocate */

/* VPP constant define */
#define VPP_MB_ALLOC_NUM 3
#define VPP_STREAM_MB_ALLOC_NUM (VPP_MB_ALLOC_NUM*2)

typedef enum {
	VPP_INT_NULL = 0,
	VPP_INT_ALL = 0xffffffff,

	VPP_INT_GOVRH_PVBI = BIT0,
	VPP_INT_GOVRH_VBIS = BIT1,	/* write done */
	VPP_INT_GOVRH_VBIE = BIT2,

	VPP_INT_GOVW_PVBI = BIT3,
	VPP_INT_GOVW_VBIS = BIT4,
	VPP_INT_GOVW_VBIE = BIT5,

	VPP_INT_DISP_PVBI = BIT6,
	VPP_INT_DISP_VBIS = BIT7,
	VPP_INT_DISP_VBIE = BIT8,

	VPP_INT_LCD_EOF = BIT9,

	VPP_INT_SCL_PVBI = BIT12,
	VPP_INT_SCL_VBIS = BIT13,
	VPP_INT_SCL_VBIE = BIT14,

	VPP_INT_VPU_PVBI = BIT15,
	VPP_INT_VPU_VBIS = BIT16,
	VPP_INT_VPU_VBIE = BIT17,

	VPP_INT_GOVRH2_PVBI = BIT18,
	VPP_INT_GOVRH2_VBIS = BIT19,	/* write done */
	VPP_INT_GOVRH2_VBIE = BIT20,

	VPP_INT_MAX = BIT31,

} vpp_int_t;

typedef enum {
	/* SCL */
	VPP_INT_ERR_SCL_TG = BIT0,
	VPP_INT_ERR_SCLR1_MIF = BIT1,
	VPP_INT_ERR_SCLR2_MIF = BIT2,
	VPP_INT_ERR_SCLW_MIFRGB = BIT3,
	VPP_INT_ERR_SCLW_MIFY = BIT4,
	VPP_INT_ERR_SCLW_MIFC = BIT5,

	/* GOVRH2 */
	VPP_INT_ERR_GOVRH2_MIF = BIT19,

	/* GOVRH */
	VPP_INT_ERR_GOVRH_MIF = BIT20,

} vpp_int_err_t;

/* VPP FB capability flag */
#define VPP_FB_FLAG_COLFMT		0xFFFF
#define VPP_FB_FLAG_SCALE		BIT(16)
#define VPP_FB_FLAG_CSC			BIT(17)
#define VPP_FB_FLAG_MEDIA		BIT(18)
#define VPP_FB_FLAG_FIELD		BIT(19)

typedef struct {
	vdo_framebuf_t fb;
	vpp_csc_t csc_mode;
	int framerate;
	vpp_media_format_t media_fmt;
	int wait_ready;
	unsigned int capability;

	void (*set_framebuf)(vdo_framebuf_t *fb);
	void (*get_framebuf)(vdo_framebuf_t *fb);
	void (*set_addr)(unsigned int yaddr, unsigned int caddr);
	void (*get_addr)(unsigned int *yaddr, unsigned int *caddr);
	void (*set_csc)(vpp_csc_t mode);
	vdo_color_fmt (*get_color_fmt)(void);
	void (*set_color_fmt)(vdo_color_fmt colfmt);
	void (*fn_view)(int read, vdo_view_t *view);
} vpp_fb_base_t;

#define VPP_MOD_BASE \
	vpp_mod_t mod; /* module id*/\
	void *mmio;	/* regs base address */\
	unsigned int int_catch; /* interrupt catch */\
	vpp_fb_base_t *fb_p; /* framebuf base pointer */\
	unsigned int pm; /* power dev id,bit31-0:power off */\
	unsigned int *reg_bk; /* register backup pointer */\
	void  (*init)(void *base); /* module initial */\
	void (*dump_reg)(void); /* dump hardware register */\
	void (*set_enable)(vpp_flag_t enable); /* module enable/disable */\
	void (*set_colorbar)(vpp_flag_t enable, int mode, int inv); \
		/* hw colorbar enable/disable & mode */\
	void (*set_tg)(vpp_clock_t *tmr, unsigned int pixel_clock); /*set tg*/\
	void (*get_tg)(vpp_clock_t *tmr); /* get timing */\
	unsigned int (*get_sts)(void); /* get interrupt or error status */\
	void (*clr_sts)(unsigned int sts); /* clear interrupt or err status */\
	void (*suspend)(int sts); /* module suspend */\
	void (*resume)(int sts) /* module resume */
/* End of vpp_mod_base_t */

typedef struct {
	VPP_MOD_BASE;
} vpp_mod_base_t;

#define VPP_MOD_FLAG_FRAMEBUF	BIT(0)
#define VPP_MOD_CLK_ON		BIT(31)

typedef enum {
	VPP_SCALE_MODE_REC_TABLE, /* old design but 1/32 limit */
	VPP_SCALE_MODE_RECURSIVE, /*no rec table,not smooth than bilinear mode*/
	VPP_SCALE_MODE_BILINEAR,/*more smooth but less than 1/2 will drop line*/
	VPP_SCALE_MODE_ADAPTIVE,/* scl dn 1-1/2 bilinear mode, other rec mode */
	VPP_SCALE_MODE_MAX
} vpp_scale_mode_t;

typedef enum {
	VPP_HDMI_AUDIO_I2S,
	VPP_HDMI_AUDIO_SPDIF,
	VPP_HDMI_AUDIO_MAX
} vpp_hdmi_audio_inf_t;

typedef enum {
	VPP_FILTER_SCALE,
	VPP_FILTER_DEBLOCK,
	VPP_FILTER_FIELD_DEFLICKER,
	VPP_FILTER_FRAME_DEFLICKER,
	VPP_FILTER_MODE_MAX
} vpp_filter_mode_t;

#define VPP_DBG_PERIOD_NUM	10
typedef struct {
	int index;
	int period_us[VPP_DBG_PERIOD_NUM];
	struct timeval pre_tv;
} vpp_dbg_period_t;

typedef struct {
	struct timeval pre_tv;
	unsigned int threshold;
	unsigned int reset;
	unsigned int cnt;
	unsigned int sum;
	unsigned int min;
	unsigned int max;
} vpp_dbg_timer_t;

#ifdef __KERNEL__
#define VPP_PROC_NUM		10
typedef struct {
	int (*func)(void *arg); /* function pointer */
	void *arg; /* function argument */
	struct list_head list;
	vpp_int_t type; /* interrupt type */
	struct semaphore sem; /* wait sem */
	int wait_ms; /* wait complete timout (ms) */
	int work_cnt; /* work counter if 0 then forever */
} vpp_proc_t;

typedef struct {
	struct list_head list;
	struct tasklet_struct tasklet;
	int ref;
} vpp_irqproc_t;
#endif

#ifndef CFG_LOADER
#include "vppm.h"
#endif
#include "lcd.h"

#ifndef CFG_LOADER
/* #ifdef WMT_FTBLK_SCL */
#include "scl.h"
/* #endif */
#endif
/*
#ifdef WMT_FTBLK_GE
#include "ge.h"
#endif
*/
#ifdef WMT_FTBLK_GOVRH
#include "govrh.h"
#endif
#ifdef WMT_FTBLK_LVDS
#include "lvds.h"
#endif
/* #ifdef WMT_FTBLK_HDMI */
#include "hdmi.h"
#ifndef CFG_LOADER
#include "cec.h"
#endif
/* #endif */
#ifdef CONFIG_WMT_EDID
#include "edid.h"
#endif

typedef enum {
	VPP_DBGLVL_DISABLE = 0x0,
	VPP_DBGLVL_SCALE = 1,
	VPP_DBGLVL_DISPFB = 2,
	VPP_DBGLVL_INT = 3,
	VPP_DBGLVL_FPS = 4,
	VPP_DBGLVL_IOCTL = 5,
	VPP_DBGLVL_DIAG = 6,
	VPP_DBGLVL_STREAM = 7,
	VPP_DBGLVL_ALL = 0xFF,
} vpp_dbg_level_t;

typedef struct {
	/* internal parameter */
	int govrh_preinit;
	int (*alloc_framebuf)(unsigned int resx, unsigned int resy);
	int dual_display; /* use 2 govr */
	int virtual_display;
	int fb0_bitblit;
	int fb_manual; /* not check var & internel timing */
	int fb_recheck; /* recheck for plug but no change res */
	int govrh_init_yres;
	int virtual_display_mode;

	/* hdmi */
	int hdmi_video_mode; /* 0-auto,720,1080 */
	vpp_hdmi_audio_inf_t hdmi_audio_interface; /* 0-I2S, 1-SPDIF */
	int hdmi_cp_enable; /* 0-off, 1-on */
	unsigned int hdmi_ctrl;
	unsigned int hdmi_audio_pb1;
	unsigned int hdmi_audio_pb4;
	unsigned int hdmi_i2c_freq;
	unsigned int hdmi_i2c_udelay;
	int hdmi_init;
	unsigned int hdmi_bksv[2];
	char *hdmi_cp_p;
	int hdmi_3d_type;
	unsigned int hdmi_pixel_clock;
	int hdmi_certify_flag;
	int hdmi_sp_mode;
	int hdmi_disable;

	/* alloc frame buffer */
	unsigned int mb[VPP_MB_ALLOC_NUM];
	unsigned int mb_y_size;
	unsigned int mb_fb_size;
	int mb_colfmt;

	/* debug */
	int dbg_msg_level; /* debug message level */
	int dbg_wait;
	int dbg_flag;

#if 0
	/* HDMI DDC debug */
	int dbg_hdmi_ddc_ctrl_err;
	int dbg_hdmi_ddc_read_err;
	int dbg_hdmi_ddc_crc_err;
#endif

#ifdef CONFIG_VPP_STREAM_CAPTURE
	/* stream capture current video display */
	int stream_enable;
	unsigned int stream_mb_lock;
	int stream_mb_sync_flag;
	int stream_mb_index;
	unsigned int stream_mb[VPP_STREAM_MB_ALLOC_NUM];
	unsigned int stream_mb_y_size;
	int stream_mb_cnt;
	unsigned int stream_sync_cnt;
#ifdef CONFIG_VPP_STREAM_FIX_RESOLUTION
	vdo_framebuf_t stream_fb;
#endif
#endif
} vpp_info_t;

#ifdef __cplusplus
extern "C" {
#endif

#ifdef VPP_C
#define EXTERN

const unsigned int vpp_csc_parm[VPP_CSC_MAX][7] = {
	/* C1,C2        C3,C4       C5,C6       C7,C8       C9,I
	J,K      b0:YC2RGB,b8:clamp */
	{0x000004a8, 0x04a80662, 0x1cbf1e70, 0x081204a8, 0x00010000,
	0x00010001, 0x00000101}, /* YUV2RGB_SDTV_0_255 */
	{0x00000400, 0x0400057c, 0x1d351ea8, 0x06ee0400, 0x00010000,
	0x00010001, 0x00000001}, /* YUV2RGB_SDTV_16_235 */
	{0x000004a8, 0x04a8072c, 0x1ddd1f26, 0x087604a8, 0x00010000,
	0x00010001, 0x00000101}, /* YUV2RGB_HDTV_0_255 */
	{0x00000400, 0x04000629, 0x1e2a1f45, 0x07440400, 0x00010000,
	0x00010001, 0x00000001}, /* YUV2RGB_HDTV_16_235 */
	{0x00000400, 0x0400059c, 0x1d251ea0, 0x07170400, 0x00010000,
	0x00010001, 0x00000001}, /* YUV2RGB_JFIF_0_255 */
	{0x00000400, 0x0400057c, 0x1d351ea8, 0x06ee0400, 0x00010000,
	0x00010001, 0x00000001}, /* YUV2RGB_SMPTE170M */
	{0x00000400, 0x0400064d, 0x1e001f19, 0x074f0400, 0x00010000,
	0x00010001, 0x00000001}, /* YUV2RGB_SMPTE240M */
	{0x02040107, 0x1f680064, 0x01c21ed6, 0x1e8701c2, 0x00211fb7,
	0x01010101, 0x00000000}, /* RGB2YUV_SDTV_0_255 */
	{0x02590132, 0x1f500075, 0x020b1ea5, 0x1e4a020b, 0x00011fab,
	0x01010101, 0x00000000}, /* RGB2YUV_SDTV_16_235 */
	{0x027500bb, 0x1f99003f, 0x01c21ea6, 0x1e6701c2, 0x00211fd7,
	0x01010101, 0x00000000}, /* RGB2YUV_HDTV_0_255 */
	{0x02dc00da, 0x1f88004a, 0x020b1e6d, 0x1e25020b, 0x00011fd0,
	0x01010101, 0x00000000}, /* RGB2YUV_HDTV_16_235 */
	{0x02590132, 0x1f530075, 0x02001ead, 0x1e530200, 0x00011fad,
	0x00ff00ff, 0x00000000}, /* RGB2YUV_JFIF_0_255 */
	{0x02590132, 0x1f500075, 0x020b1ea5, 0x1e4a020b, 0x00011fab,
	0x01010101, 0x00000000}, /* RGB2YUV_SMPTE170M */
	{0x02ce00d9, 0x1f890059, 0x02001e77, 0x1e380200, 0x00011fc8,
	0x01010101, 0x00000000}, /* RGB2YUV_SMPTE240M */
};

const struct fb_videomode vpp_videomode[] = {
	/* 640x480@60 DMT/CEA861 */
	{ NULL, 60, 640, 480, KHZ2PICOS(25175), 48, 16, 33, 10, 96, 2,
	  0, FB_VMODE_NONINTERLACED, 0},
#if 0
	/* 640x480@60 CVT */
	{ NULL, 60, 640, 480, KHZ2PICOS(23750), 80, 16, 13, 3, 64, 4,
	  FB_SYNC_VERT_HIGH_ACT, FB_VMODE_NONINTERLACED, 0},
#endif
	/* 640x480@75 DMT */
	{ NULL, 75, 640, 480, KHZ2PICOS(31500), 120, 16, 16, 1, 64, 3,
	  0, FB_VMODE_NONINTERLACED, FB_MODE_IS_VESA},
#if 0
	/* 640x480@75 CVT */
	{ NULL, 75, 640, 480, KHZ2PICOS(30750), 88, 24, 17, 3, 64, 4,
	  FB_SYNC_VERT_HIGH_ACT, FB_VMODE_NONINTERLACED, 0},
#endif
	/* 720x480@60 CEA861 */
	{ NULL, 60, 720, 480, KHZ2PICOS(27027), 60, 16, 30, 9, 62, 6,
	  0, FB_VMODE_NONINTERLACED, 0},
	/* 720x480i@60 CEA861 */
	{ NULL, 60, 720, 480, KHZ2PICOS(27000), 114, 38, 30, 8, 124, 6,
	  0, FB_VMODE_INTERLACED + FB_VMODE_DOUBLE, 0},
	/* 720x576@50 CEA861 */
	{ NULL, 50, 720, 576, KHZ2PICOS(27000), 68, 12, 39, 5, 64, 5,
	  0, FB_VMODE_NONINTERLACED, 0},
	/* 720x576i@50 CEA861 */
	{ NULL, 50, 720, 576, KHZ2PICOS(27000), 138, 24, 38, 4, 126, 6,
	  0, FB_VMODE_INTERLACED + FB_VMODE_DOUBLE, 0},
	/* 800x480@60 CVT */
	{ NULL, 60, 800, 480, KHZ2PICOS(29500), 96, 24, 10, 3, 72, 7,
	  FB_SYNC_VERT_HIGH_ACT, FB_VMODE_NONINTERLACED, 0},
	/* 800x480@75 CVT */
	{ NULL, 75, 800, 480, KHZ2PICOS(38500), 112, 32, 14, 3, 80, 7,
	  FB_SYNC_VERT_HIGH_ACT, FB_VMODE_NONINTERLACED, 0},
	/* 800x600@60 DMT */
	{ NULL, 60, 800, 600, KHZ2PICOS(40000), 88, 40, 23, 1, 128, 4,
	  FB_SYNC_VERT_HIGH_ACT + FB_SYNC_HOR_HIGH_ACT,
	  FB_VMODE_NONINTERLACED, FB_MODE_IS_VESA},
#if 0
	/* 800x600@60 CVT */
	{ NULL, 60, 800, 600, KHZ2PICOS(38250), 112, 32, 17, 3, 80, 4,
	  FB_SYNC_VERT_HIGH_ACT, FB_VMODE_NONINTERLACED, 0},
#endif
	/* 800x600@75 DMT */
	{ NULL, 75, 800, 600, KHZ2PICOS(49500), 160, 16, 21, 1, 80, 3,
	  FB_SYNC_VERT_HIGH_ACT + FB_SYNC_HOR_HIGH_ACT,
	  FB_VMODE_NONINTERLACED, FB_MODE_IS_VESA},
#if 0
	/* 800x600@75 CVT */
	{ NULL, 75, 800, 600, KHZ2PICOS(49000), 120, 40, 22, 3, 80, 4,
	  FB_SYNC_VERT_HIGH_ACT, FB_VMODE_NONINTERLACED, 0},
#endif
	/* 848x480@60 DMT */
	{ NULL, 60, 848, 480, KHZ2PICOS(33750), 112, 16, 23, 6, 112, 8,
	  FB_SYNC_VERT_HIGH_ACT + FB_SYNC_HOR_HIGH_ACT,
	  FB_VMODE_NONINTERLACED, FB_MODE_IS_VESA},
	/* 1024x600@60 DMT */
	{ NULL, 60, 1024, 600, KHZ2PICOS(49000), 144, 40, 11, 3, 104, 10,
	  FB_SYNC_VERT_HIGH_ACT, FB_VMODE_NONINTERLACED, FB_MODE_IS_VESA},
	/* 1024x768@60 DMT */
	{ NULL, 60, 1024, 768, KHZ2PICOS(65000), 160, 24, 29, 3, 136, 6,
	  0, FB_VMODE_NONINTERLACED, FB_MODE_IS_VESA},
	/* 1024x768@60 CVT */
	{ NULL, 60, 1024, 768, KHZ2PICOS(63500), 152, 48, 23, 3, 104, 4,
	  FB_SYNC_VERT_HIGH_ACT, FB_VMODE_NONINTERLACED, 0},
	/* 1024x768@75 DMT */
	{ NULL, 75, 1024, 768, KHZ2PICOS(78750), 176, 16, 28, 1, 96, 3,
	  FB_SYNC_VERT_HIGH_ACT + FB_SYNC_HOR_HIGH_ACT,
	  FB_VMODE_NONINTERLACED, FB_MODE_IS_VESA},
#if 0
	/* 1024x768@75 CVT */
	{ NULL, 75, 1024, 768, KHZ2PICOS(82000), 168, 64, 30, 3, 104, 4,
	  FB_SYNC_VERT_HIGH_ACT, FB_VMODE_NONINTERLACED, 0},
#endif
	/* 1152x864@60 CVT */
	{ NULL, 60, 1152, 864, KHZ2PICOS(81750), 184, 64, 26, 3, 120, 4,
	  FB_SYNC_VERT_HIGH_ACT, FB_VMODE_NONINTERLACED, 0},
	/* 1152x864@75 DMT */
	{ NULL, 75, 1152, 864, KHZ2PICOS(108000), 256, 64, 32, 1, 128, 3,
	  FB_SYNC_VERT_HIGH_ACT + FB_SYNC_HOR_HIGH_ACT,
	  FB_VMODE_NONINTERLACED, FB_MODE_IS_VESA},
	/* 1152x864@75 CVT */
	{ NULL, 75, 1152, 864, KHZ2PICOS(104000), 192, 72, 34, 3, 120, 4,
	  FB_SYNC_VERT_HIGH_ACT, FB_VMODE_NONINTERLACED, 0},
	/* 1280x720@50 CEA861,HDMI_1280x720p50_16x9 */
	{ NULL, 50, 1280, 720, KHZ2PICOS(74250), 220, 440, 20, 5, 40, 5,
	  FB_SYNC_VERT_HIGH_ACT + FB_SYNC_HOR_HIGH_ACT,
	  FB_VMODE_NONINTERLACED, 0},
	/* 1280x720@60 CEA861,HDMI_1280x720p60_16x9 */
	{ NULL, 60, 1280, 720, KHZ2PICOS(74250), 220, 110, 20, 5, 40, 5,
	  FB_SYNC_VERT_HIGH_ACT + FB_SYNC_HOR_HIGH_ACT,
	  FB_VMODE_NONINTERLACED, 0},
#if 0
	/* 1280x720@60 CVT */
	{ NULL, 60, 1280, 720, KHZ2PICOS(74500), 192, 64, 20, 3, 128, 5,
	  FB_SYNC_VERT_HIGH_ACT, FB_VMODE_NONINTERLACED, 0},
#endif
	/* 1280x720@75 CVT */
	{ NULL, 75, 1280, 720, KHZ2PICOS(95750), 208, 80, 27, 3, 128, 5,
	  FB_SYNC_VERT_HIGH_ACT, FB_VMODE_NONINTERLACED, 0},
	/* 1280x768@60 DMT/CVT */
	{ NULL, 60, 1280, 768, KHZ2PICOS(79500), 192, 64, 20, 3, 128, 7,
	  FB_SYNC_VERT_HIGH_ACT, FB_VMODE_NONINTERLACED, FB_MODE_IS_VESA},
	/* 1280x768@75 DMT/CVT */
	{ NULL, 75, 1280, 768, KHZ2PICOS(102250), 208, 80, 27, 3, 128, 7,
	  FB_SYNC_VERT_HIGH_ACT, FB_VMODE_NONINTERLACED, FB_MODE_IS_VESA},
	/* 1280x800@60 DMT/CVT */
	{ NULL, 60, 1280, 800, KHZ2PICOS(83500), 200, 72, 22, 3, 128, 6,
	  FB_SYNC_VERT_HIGH_ACT, FB_VMODE_NONINTERLACED, FB_MODE_IS_VESA},
	/* 1280x800@75 DMT/CVT */
	{ NULL, 75, 1280, 800, KHZ2PICOS(106500), 208, 80, 29, 3, 128, 6,
	  FB_SYNC_VERT_HIGH_ACT, FB_VMODE_NONINTERLACED, FB_MODE_IS_VESA},
	/* 1280x960@60 DMT */
	{ NULL, 60, 1280, 960, KHZ2PICOS(108000), 312, 96, 36, 1, 112, 3,
	  FB_SYNC_VERT_HIGH_ACT + FB_SYNC_HOR_HIGH_ACT,
	  FB_VMODE_NONINTERLACED, FB_MODE_IS_VESA},
#if 0
	/* 1280x960@60 CVT */
	{ NULL, 60, 1280, 960, KHZ2PICOS(101250), 208, 80, 29, 3, 128, 4,
	  FB_SYNC_VERT_HIGH_ACT, FB_VMODE_NONINTERLACED, 0},
#endif
	/* 1280x960@75 CVT */
	{ NULL, 75, 1280, 960, KHZ2PICOS(130000), 224, 88, 38, 3, 136, 4,
	  FB_SYNC_VERT_HIGH_ACT, FB_VMODE_NONINTERLACED, 0},
	/* 1280x1024@60 DMT */
	{ NULL, 60, 1280, 1024, KHZ2PICOS(108000), 248, 48, 38, 1, 112, 3,
	  FB_SYNC_VERT_HIGH_ACT + FB_SYNC_HOR_HIGH_ACT,
	  FB_VMODE_NONINTERLACED, FB_MODE_IS_VESA},
#if 0
	/* 1280x1024@60 CVT */
	{ NULL, 60, 1280, 1024, KHZ2PICOS(109000), 216, 80, 29, 3, 136, 7,
	  FB_SYNC_VERT_HIGH_ACT, FB_VMODE_NONINTERLACED, 0},
#endif
	/* 1280x1024@75 DMT */
	{ NULL, 75, 1280, 1024, KHZ2PICOS(135000), 248, 16, 38, 1, 144, 3,
	  FB_SYNC_VERT_HIGH_ACT + FB_SYNC_HOR_HIGH_ACT,
	  FB_VMODE_NONINTERLACED, FB_MODE_IS_VESA},
#if 0
	/* 1280x1024@75 CVT */
	{ NULL, 75, 1280, 1024, KHZ2PICOS(138750), 224, 88, 38, 3, 136, 7,
	  FB_SYNC_VERT_HIGH_ACT, FB_VMODE_NONINTERLACED, 0},
#endif
	/* 1360x768@60 */
	{ NULL, 60, 1360, 768, KHZ2PICOS(85500), 256, 64, 18, 3, 112, 6,
	  FB_SYNC_VERT_HIGH_ACT + FB_SYNC_HOR_HIGH_ACT,
	  FB_VMODE_NONINTERLACED, 0},
	/* 1366x768@60 */
	{ NULL, 60, 1366, 768, KHZ2PICOS(85500), 213, 70, 24, 3, 143, 3,
	  FB_SYNC_VERT_HIGH_ACT + FB_SYNC_HOR_HIGH_ACT,
	  FB_VMODE_NONINTERLACED, 0},
	/* 1400x1050@60 DMT/CVT */
	{ NULL, 60, 1400, 1050, KHZ2PICOS(121750), 232, 88, 32, 3, 144, 4,
	  FB_SYNC_VERT_HIGH_ACT, FB_VMODE_NONINTERLACED, FB_MODE_IS_VESA},
#if 0
	/* 1400x1050@60+R DMT/CVT */
	{ NULL, 60, 1400, 1050, KHZ2PICOS(101000), 80, 48, 23, 3, 32, 4,
	  FB_SYNC_VERT_HIGH_ACT, FB_VMODE_NONINTERLACED, FB_MODE_IS_VESA},
#endif
	/* 1440x480p@60 CEA861 */
	{ NULL, 60, 1400, 480, KHZ2PICOS(54054), 120, 32, 30, 9, 124, 6,
	  0, FB_VMODE_NONINTERLACED, 0},
	/* 1440x900@60 DMT/CVT */
	{ NULL, 60, 1440, 900, KHZ2PICOS(106500), 232, 80, 25, 3, 152, 6,
	  FB_SYNC_VERT_HIGH_ACT, FB_VMODE_NONINTERLACED, FB_MODE_IS_VESA},
	/* 1440x900@75 DMT/CVT */
	{ NULL, 75, 1440, 900, KHZ2PICOS(136750), 248, 96, 33, 3, 152, 6,
	  FB_SYNC_VERT_HIGH_ACT, FB_VMODE_NONINTERLACED, FB_MODE_IS_VESA},
	/* 1600x1200@60 DMT/CVT */
	{ NULL, 60, 1600, 1200, KHZ2PICOS(162000), 304, 64, 46, 1, 192, 3,
	  FB_SYNC_VERT_HIGH_ACT, FB_VMODE_NONINTERLACED, FB_MODE_IS_VESA},
	/* 1680x1050@60 DMT/CVT */
	{ NULL, 60, 1680, 1050, KHZ2PICOS(146250), 280, 104, 30, 3, 176, 6,
	  FB_SYNC_VERT_HIGH_ACT, FB_VMODE_NONINTERLACED, FB_MODE_IS_VESA},
	/* 1920x1080@25,HDMI_1920x1080p25_16x9 */
	{ NULL, 25, 1920, 1080, KHZ2PICOS(74250), 148, 528, 36, 4, 44, 5,
	  FB_SYNC_VERT_HIGH_ACT + FB_SYNC_HOR_HIGH_ACT,
	  FB_VMODE_NONINTERLACED, 0},
	/* 1920x1080@30,HDMI_1920x1080p30_16x9 */
	{ NULL, 30, 1920, 1080, KHZ2PICOS(74250), 148, 88, 36, 4, 44, 5,
	  FB_SYNC_VERT_HIGH_ACT + FB_SYNC_HOR_HIGH_ACT,
	  FB_VMODE_NONINTERLACED, 0},
	/* 1920x1080@50,HDMI_1920x1080p50_16x9 */
	{ NULL, 50, 1920, 1080, KHZ2PICOS(148500), 148, 528, 36, 4, 44, 5,
	  FB_SYNC_VERT_HIGH_ACT + FB_SYNC_HOR_HIGH_ACT,
	  FB_VMODE_NONINTERLACED, 0},
	/* 1920x1080i@50 */
	{ NULL, 50, 1920, 1080, KHZ2PICOS(74250), 148, 528, 30, 4, 44, 10,
	  FB_SYNC_VERT_HIGH_ACT + FB_SYNC_HOR_HIGH_ACT,
	  FB_VMODE_INTERLACED, 0},
	/* 1920x1080@60 */
	{ NULL, 60, 1920, 1080, KHZ2PICOS(148500), 148, 88, 36, 4, 44, 5,
	  FB_SYNC_VERT_HIGH_ACT + FB_SYNC_HOR_HIGH_ACT,
	  FB_VMODE_NONINTERLACED, 0},
	/* 1920x1080i@60 */
	{ NULL, 60, 1920, 1080, KHZ2PICOS(74250), 148, 88, 30, 4, 44, 10,
	  FB_SYNC_VERT_HIGH_ACT + FB_SYNC_HOR_HIGH_ACT,
	  FB_VMODE_INTERLACED, 0},
	/* 1920x1200@60+R DMT/CVT */
	{ NULL, 60, 1920, 1200, KHZ2PICOS(154000), 80, 48, 26, 3, 32, 6,
	  FB_SYNC_HOR_HIGH_ACT, FB_VMODE_NONINTERLACED, FB_MODE_IS_VESA},
	/* 1920x1200@60 DMT/CVT */
	{ NULL, 60, 1920, 1200, KHZ2PICOS(193250), 336, 136, 36, 3, 200, 6,
	  FB_SYNC_VERT_HIGH_ACT, FB_VMODE_NONINTERLACED, FB_MODE_IS_VESA},
	{ NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	  0, 0, 0 },
};

char *vpp_colfmt_str[] = {"YUV420", "YUV422H", "YUV422V", "YUV444", "YUV411",
			"GRAY", "ARGB", "AUTO", "RGB888", "RGB666", "RGB565",
			"RGB1555", "RGB5551"};
const char *vpp_mod_str[] = {"GOVRH2", "GOVRH", "DISP", "GOVW", "GOVM", "SCL",
	"SCLW", "VPU", "VPUW", "PIP", "VPPM", "LCDC", "CUR", "MAX"};

#else
#define EXTERN extern

extern const unsigned int vpp_csc_parm[VPP_CSC_MAX][7];
extern char *vpp_colfmt_str[];
extern const struct fb_videomode vpp_videomode[];
extern char *vpp_mod_str[];
#endif

EXTERN vpp_info_t g_vpp;

static __inline__ int vpp_get_hdmi_spdif(void)
{
	return (g_vpp.hdmi_audio_interface == VPP_HDMI_AUDIO_SPDIF) ? 1 : 0;
}

/* Internal functions */
EXTERN int get_key(void);
EXTERN U8 vppif_reg8_in(U32 offset);
EXTERN U8 vppif_reg8_out(U32 offset, U8 val);
EXTERN U16 vppif_reg16_in(U32 offset);
EXTERN U16 vppif_reg16_out(U32 offset, U16 val);
EXTERN U32 vppif_reg32_in(U32 offset);
EXTERN U32 vppif_reg32_out(U32 offset, U32 val);
EXTERN U32 vppif_reg32_write(U32 offset, U32 mask, U32 shift, U32 val);
EXTERN U32 vppif_reg32_read(U32 offset, U32 mask, U32 shift);
EXTERN U32 vppif_reg32_mask(U32 offset, U32 mask, U32 shift);
EXTERN unsigned int vpp_get_chipid(void);

/* Export functions */
EXTERN void vpp_mod_unregister(vpp_mod_t mod);
EXTERN vpp_mod_base_t *vpp_mod_register(vpp_mod_t mod, int size,
	unsigned int flags);
EXTERN vpp_mod_base_t *vpp_mod_get_base(vpp_mod_t mod);
EXTERN vpp_fb_base_t *vpp_mod_get_fb_base(vpp_mod_t mod);
EXTERN vdo_framebuf_t *vpp_mod_get_framebuf(vpp_mod_t mod);
EXTERN void vpp_mod_init(void);
EXTERN void vpp_mod_set_clock(vpp_mod_t mod,
	vpp_flag_t enable, int force);

EXTERN unsigned int vpp_get_base_clock(vpp_mod_t mod);
EXTERN void vpp_set_video_scale(vdo_view_t *vw);
EXTERN int vpp_set_recursive_scale(vdo_framebuf_t *src_fb,
	vdo_framebuf_t *dst_fb);
EXTERN vpp_display_format_t vpp_get_fb_field(vdo_framebuf_t *fb);
EXTERN void vpp_wait_vsync(int no, int cnt);
EXTERN int vpp_get_gcd(int A, int B);
EXTERN vpp_csc_t vpp_check_csc_mode(vpp_csc_t mode,
	vdo_color_fmt src_fmt, vdo_color_fmt dst_fmt, unsigned int flags);
EXTERN __inline__ void vpp_cache_sync(void);
EXTERN int vpp_calc_refresh(int pixclk, int xres, int yres);
EXTERN int vpp_calc_align(int value, int align);
EXTERN int vpp_calc_fb_width(vdo_color_fmt colfmt, int width);
EXTERN void vpp_get_colfmt_bpp(vdo_color_fmt colfmt,
	int *y_bpp, int *c_bpp);
EXTERN int vpp_irqproc_work(vpp_int_t type, int (*func)(void *argc),
	void *arg, int wait_ms, int work_cnt);
EXTERN int vpp_check_dbg_level(vpp_dbg_level_t level);

#ifdef __KERNEL__
EXTERN void vpp_dbg_show(int level, int tmr, char *str);
EXTERN void vpp_dbg_wake_up(void);
EXTERN int vpp_dbg_get_period_usec(vpp_dbg_period_t *p, int cmd);
EXTERN void vpp_dbg_timer(vpp_dbg_timer_t *p, char *str, int cmd);
EXTERN void vpp_dbg_back_trace(void);
EXTERN void vpp_dbg_show_val1(int level, int tmr, char *str, int val);
EXTERN void vpp_dbg_wait(char *str);
EXTERN void vpp_irqproc_init(void);
EXTERN void vpp_irqproc_del_work(vpp_int_t type,
	int (*func)(void *argc));
EXTERN vpp_irqproc_t *vpp_irqproc_get_entry(vpp_int_t vpp_int);

/* dev-vpp.c */
EXTERN void vpp_get_info(int fbn, struct fb_var_screeninfo *var);
EXTERN int vpp_set_par(struct fb_info *info);
EXTERN unsigned int *vpp_backup_reg(unsigned int addr, unsigned int size);
EXTERN int vpp_restore_reg(unsigned int addr,
	unsigned int size, unsigned int *reg_ptr);
EXTERN void vpp_backup_reg2(unsigned int addr,
	unsigned int size, unsigned int *ptr);
EXTERN void vpp_restore_reg2(unsigned int addr,
	unsigned int size, unsigned int *reg_ptr);
EXTERN int vpp_pan_display(struct fb_var_screeninfo *var,
			struct fb_info *info);
EXTERN void vpp_netlink_init(void);
EXTERN void vpp_netlink_notify(int no, int cmd, int arg);
EXTERN void vpp_netlink_notify_plug(int vout_num, int plugin);
EXTERN void vpp_netlink_notify_cp(int enable);
EXTERN void vpp_switch_state_init(void);
EXTERN int vpp_set_blank(struct fb_info *info, int blank);
#endif

EXTERN void vpp_reg_dump(unsigned int addr, int size);
EXTERN unsigned int vpp_convert_colfmt(int yuv2rgb, unsigned int data);
EXTERN void vpp_init(void);

/* EXTERN void vpp_set_power_mgr(int arg); */
EXTERN void vpp_show_timing(char *str,
	struct fb_videomode *vmode, vpp_clock_t *clk);
EXTERN void vpp_show_framebuf(char *str, vdo_framebuf_t *fb);
EXTERN void vpp_show_videomode(char *str, struct fb_videomode *v);
EXTERN void vpp_set_mutex(int idx, int lock);
EXTERN void vpp_set_NA12_hiprio(int type);
EXTERN void vpp_free_framebuffer(void);
EXTERN int vpp_alloc_framebuffer(unsigned int resx, unsigned int resy);

EXTERN int vpp_mb_get(unsigned int phy);
EXTERN int vpp_mb_put(unsigned int phy);
EXTERN int vpp_mb_irqproc_sync(int arg);
EXTERN void vpp_mb_scale_bitblit(vdo_framebuf_t *fb);

#undef EXTERN

#ifdef __cplusplus
}
#endif
#endif /* VPP_H */
