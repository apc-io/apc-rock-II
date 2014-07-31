/*++
 * linux/drivers/video/wmt/scl.h
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

#include "vpp.h"

#ifndef SCL_H
#define SCL_H

#define SCL_COMPLETE_INT VPP_INT_SCL_PVBI

typedef struct {
	VPP_MOD_BASE;

	int (*scale)(vdo_framebuf_t *src, vdo_framebuf_t *dst);
	int (*scale_finish)(void);

	vpp_scale_mode_t scale_mode;
	vpp_scale_mode_t scale_sync;
	int scale_complete;
	vpp_filter_mode_t filter_mode;

	vpp_dbg_timer_t overlap_timer;
	vpp_dbg_timer_t scale_timer;
} scl_mod_t;

typedef struct {
	VPP_MOD_BASE;
} sclw_mod_t;

#ifdef __cplusplus
extern "C" {
#endif

#ifdef SCL_C
#define EXTERN
#else
#define EXTERN   extern
#endif

EXTERN scl_mod_t *p_scl;
EXTERN sclw_mod_t *p_sclw;

#ifdef WMT_FTBLK_SCL

EXTERN void scl_reg_dump(void);
EXTERN void sclr_set_framebuffer(vdo_framebuf_t *inbuf);
EXTERN void sclw_set_framebuffer(vdo_framebuf_t *outbuf);

EXTERN void scl_set_enable(vpp_flag_t enable);
EXTERN void scl_set_reg_update(vpp_flag_t enable);
EXTERN void scl_set_reg_level(vpp_reglevel_t level);
EXTERN void scl_set_int_enable(vpp_flag_t enable, vpp_int_t int_bit);
EXTERN vpp_int_err_t scl_get_int_status(void);
EXTERN void scl_clean_int_status(vpp_int_err_t int_sts);
EXTERN void scl_set_csc_mode(vpp_csc_t mode);
EXTERN void scl_set_scale_enable(vpp_flag_t vscl_enable,
	vpp_flag_t hscl_enable);
EXTERN void scl_set_V_scale(int A, int B);
EXTERN void scl_set_H_scale(int A, int B);
EXTERN void scl_set_crop(int offset_x, int offset_y);
EXTERN void scl_set_tg_enable(vpp_flag_t enable);
EXTERN void scl_set_timing_master(vpp_mod_t mod_bit);
EXTERN vpp_mod_t scl_get_timing_master(void);
EXTERN void scl_set_drop_line(vpp_flag_t enable);
EXTERN void sclr_get_fb_info(U32 *width, U32 *act_width,
	U32 *x_offset, U32 *y_offset);
EXTERN void sclr_set_mif_enable(vpp_flag_t enable);
EXTERN void sclr_set_mif2_enable(vpp_flag_t enable);
EXTERN void sclr_set_colorbar(vpp_flag_t enable, int width, int inverse);
EXTERN void sclr_set_display_format(vpp_display_format_t source,
	vpp_display_format_t target);
EXTERN void sclr_set_field_mode(vpp_display_format_t fmt);
EXTERN void sclr_set_color_format(vdo_color_fmt format);
EXTERN vdo_color_fmt sclr_get_color_format(void);
EXTERN void sclr_set_media_format(vpp_media_format_t format);
EXTERN void sclr_set_fb_addr(U32 y_addr, U32 c_addr);
EXTERN void sclr_get_fb_addr(U32 *y_addr, U32 *c_addr);
EXTERN void sclr_set_width(U32 y_pixel, U32 y_buffer);
EXTERN void sclr_get_width(U32 *p_y_pixel, U32 *p_y_buffer);
EXTERN void sclr_set_crop(U32 h_crop, U32 v_crop);
EXTERN void sclr_set_threshold(U32 value);
EXTERN vdo_color_fmt scl_R2_get_color_format(void);
EXTERN void sclw_set_mif_enable(vpp_flag_t enable);
EXTERN void sclw_set_color_format(vdo_color_fmt format);
EXTERN vdo_color_fmt sclw_get_color_format(void);
EXTERN void sclw_set_field_mode(vpp_display_format_t fmt);
EXTERN void sclw_set_fb_addr(U32 y_addr, U32 c_addr);
EXTERN void sclw_get_fb_addr(U32 *y_addr, U32 *c_addr);
EXTERN void sclw_set_fb_width(U32 y_pixel, U32 y_buffer);
EXTERN void sclw_get_fb_width(U32 *width, U32 *buf_width);
EXTERN void scl_set_scale(unsigned int SRC_W, unsigned int SRC_H,
	unsigned int DST_W, unsigned int DST_H);
EXTERN int scl_mod_init(void);
EXTERN void scl_set_deblock_enable(int enable);
EXTERN void scl_set_field_filter_enable(int enable);
EXTERN void scl_set_frame_filter_enable(int enable);
EXTERN int scl_set_scale_overlap(vdo_framebuf_t *s,
	vdo_framebuf_t *in, vdo_framebuf_t *out);
EXTERN void scl_set_overlap(vpp_overlap_t *p);

#ifdef __cplusplus
}
#endif
#endif /* WMT_FTBLK_SCL */
#undef EXTERN
#endif /* SCL_H */
