/*++
 * linux/drivers/video/wmt/govrh.c
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

#define GOVRH_C
#undef DEBUG
/* #define DEBUG */
/* #define DEBUG_DETAIL */

#include "govrh.h"

#ifdef WMT_FTBLK_GOVRH
void govrh_reg_dump(vpp_mod_base_t *base)
{
	govrh_mod_t *p_govr = (govrh_mod_t *) base;
	volatile struct govrh_regs *regs =
		(volatile struct govrh_regs *) base->mmio;
	int igs_mode[4] = {888, 555, 666, 565};

	DPRINT("========== GOVRH register dump ==========\n");
	vpp_reg_dump((unsigned int)base->mmio, 512);
	DPRINT("=========================================\n");
	DPRINT("MIF enable %d\n", regs->mif.b.enable);
	DPRINT("color bar enable %d,mode %d,inv %d\n",
		regs->cb_enable.b.enable, regs->cb_enable.b.mode,
		regs->cb_enable.b.inversion);
	DPRINT("---------- frame buffer ----------\n");
	DPRINT("colr format %s\n",
		vpp_colfmt_str[govrh_get_color_format(p_govr)]);
	DPRINT("width active %d,fb %d\n", regs->pixwid, regs->bufwid);
	DPRINT("Y addr 0x%x, C addr 0x%x\n", regs->ysa, regs->csa);
	DPRINT("Y addr2 0x%x, C addr2 0x%x\n", regs->ysa2, regs->csa2);
	DPRINT("H crop %d, V crop %d\n", regs->hcrop, regs->vcrop);
	DPRINT("source format %s,H264 %d\n",
		(regs->srcfmt) ? "field" : "frame", regs->h264_input_en);
	DPRINT("H scaleup %d,dirpath %d\n", regs->hscale_up, regs->dirpath);
	DPRINT("---------- GOVRH TG1 ----------\n");
	DPRINT("TG enable %d, Twin mode %d\n",
		regs->tg_enable.b.enable, regs->tg_enable.b.mode);
	DPRINT("DVO clk %d,%d,Read cyc %d\n",
		vpp_get_base_clock(p_govr->mod),
		auto_pll_divisor((p_govr->mod == VPP_MOD_GOVRH) ?
			DEV_HDMILVDS : DEV_DVO, GET_FREQ, 0, 0),
		regs->read_cyc);
	DPRINT("H total %d, Sync %d, beg %d, end %d\n",
		regs->h_allpxl, regs->hdmi_hsynw,
		regs->actpx_bg, regs->actpx_end);
	DPRINT("V total %d, Sync %d, beg %d, end %d\n",
		regs->v_allln, regs->hdmi_vbisw,
		regs->actln_bg, regs->actln_end);
	DPRINT("VBIE %d,PVBI %d\n", regs->vbie_line, regs->pvbi_line);
	DPRINT("---------- GOVRH TG2 ----------\n");
	DPRINT("H total %d, Sync %d, beg %d, end %d\n",
		regs->h_allpxl2, regs->hdmi_hsynw2,
		regs->actpx_bg2, regs->actpx_end2);
	DPRINT("V total %d, Sync %d, beg %d, end %d\n",
		regs->v_allln2, regs->hdmi_vbisw2,
		regs->actln_bg2, regs->actln_end2);
	DPRINT("VBIE %d,PVBI %d\n", regs->vbie_line2, regs->pvbi_line2);
	DPRINT("---------- DVO ----------------\n");
	DPRINT("DVO enable %d,data width %d bits\n",
		regs->dvo_set.b.enable, (regs->dvo_set.b.outwidth) ? 12 : 24);
	DPRINT("DVO color format %s\n",
		vpp_colfmt_str[govrh_get_dvo_color_format(p_govr)]);
	DPRINT("Polar H %s,V %s\n",
		(regs->dvo_set.b.hsync_polar) ? "Low" : "High",
		(regs->dvo_set.b.vsync_polar) ? "Low" : "High");
	DPRINT("DVO RGB mode %d,%s\n", igs_mode[regs->igs_mode.b.mode],
		(regs->igs_mode.b.ldi) ? "msb" : "lsb");
	DPRINT("RGB swap %d\n", regs->dvo_set.b.rgb_swap);
	DPRINT("---------- CSC ----------------\n");
	DPRINT("CSC mode %s\n",
		(regs->csc_mode.b.mode) ? "YUV2RGB" : "RGB2YUV");
	DPRINT("CSC enable DVO %d,DISP %d,LVDS %d,HDMI %d\n",
		regs->yuv2rgb.b.dvo, regs->yuv2rgb.b.disp,
		regs->yuv2rgb.b.lvds, regs->yuv2rgb.b.hdmi);
	DPRINT("---------- Misc ----------------\n");
	DPRINT("Contrast 0x%x,Brightness 0x%x\n",
		regs->contrast.val, regs->brightness);
	DPRINT("LVDS RGB mode %d,%s\n", igs_mode[regs->igs_mode2.b.mode],
		(regs->igs_mode2.b.ldi) ? "msb" : "lsb");
	DPRINT("HDMI 3D mode %d,blank 0x%x\n",
		regs->hdmi_3d.b.mode, regs->hdmi_3d.b.blank_value);
#ifdef WMT_FTBLK_GOVRH_CURSOR
	DPRINT("---------- CURSOR -------------\n");
	DPRINT("enable %d,field %d\n",
		regs->cur_status.b.enable, regs->cur_status.b.out_field);
	DPRINT("width %d,fb width %d\n", regs->cur_width, regs->cur_fb_width);
	DPRINT("crop(%d,%d)\n", regs->cur_hcrop, regs->cur_vcrop);
	DPRINT("coord H(%d,%d),V(%d,%d)\n",
		regs->cur_hcoord.b.start, regs->cur_hcoord.b.end,
		regs->cur_vcoord.b.start, regs->cur_vcoord.b.end);
	DPRINT("color key enable 0x%x,color key 0x%x,alpha enable %d\n",
		regs->cur_color_key.b.enable, regs->cur_color_key.b.colkey,
		regs->cur_color_key.b.alpha);
#endif
}


void govrh_set_tg_enable(govrh_mod_t *base, vpp_flag_t enable)
{
	volatile struct govrh_regs *regs =
		(volatile struct govrh_regs *) base->mmio;

	DBG_DETAIL("(govr %d,%d)\n", (base->mod == VPP_MOD_GOVRH) ?
				1 : 2, enable);

	switch (enable) {
	case VPP_FLAG_ENABLE:
		regs->tg_enable.b.enable = enable;
		break;
	case VPP_FLAG_DISABLE:
		if (regs->tg_enable.b.enable)
			regs->tg_enable.b.enable = enable;
		break;
	default:
		break;
	}
	return;
}

unsigned int govrh_set_clock(govrh_mod_t *base, unsigned int pixel_clock)
{
	int pmc_clk = 0;

	DBG_MSG("(govr %d,%d)\n", (base->mod == VPP_MOD_GOVRH) ?
		1 : 2, pixel_clock);

	base->vo_clock = pixel_clock;
	if (base->mod == VPP_MOD_GOVRH) {
		pmc_clk = auto_pll_divisor(DEV_HDMILVDS, SET_PLLDIV,
			0, pixel_clock);
		DBG_MSG("set %d,get %d\n", pixel_clock,
			auto_pll_divisor(DEV_HDMILVDS, GET_FREQ, 0, 0));
		g_vpp.hdmi_pixel_clock = pmc_clk;
	} else
		pmc_clk = auto_pll_divisor(DEV_DVO, SET_PLLDIV, 0, pixel_clock);
	return 0;
}

void govrh_set_tg1(govrh_mod_t *base, vpp_clock_t *timing)
{
	volatile struct govrh_regs *regs =
		(volatile struct govrh_regs *) base->mmio;

	DBG_DETAIL("(govr %d)\n", (base->mod == VPP_MOD_GOVRH) ? 1 : 2);

	regs->tg_enable.b.mode = 0;
	regs->dstfmt = 0;
	regs->read_cyc = timing->read_cycle;

	regs->actpx_bg = timing->begin_pixel_of_active;
	regs->actpx_end = timing->end_pixel_of_active;
	regs->h_allpxl = timing->total_pixel_of_line;

	regs->actln_bg = timing->begin_line_of_active + 1;
	regs->actln_end = timing->end_line_of_active + 1;
	regs->v_allln = timing->total_line_of_frame;

	regs->vbie_line = timing->line_number_between_VBIS_VBIE;
	/* pre vbi should more 6 to avoid garbage line */
	regs->pvbi_line = (timing->line_number_between_PVBI_VBIS < 6) ?
				6 : timing->line_number_between_PVBI_VBIS;

	regs->hdmi_hsynw = timing->hsync;
	regs->hdmi_vbisw = timing->vsync;

#ifdef DEBUG_DETAIL
	vpp_show_timing("govrh tg1", 0, timing);
#endif
}

int govrh_get_tg_mode(govrh_mod_t *base)
{
	volatile struct govrh_regs *regs =
		(volatile struct govrh_regs *) base->mmio;

	DBG_DETAIL("(govr %d)\n", (base->mod == VPP_MOD_GOVRH) ? 1 : 2);

	return regs->tg_enable.b.mode;
}

void govrh_set_tg2(govrh_mod_t *base, vpp_clock_t *timing)
{
	volatile struct govrh_regs *regs =
		(volatile struct govrh_regs *) base->mmio;

	DBG_DETAIL("(govr %d)\n", (base->mod == VPP_MOD_GOVRH) ? 1 : 2);

	regs->tg_enable.b.mode = 1;
	regs->dstfmt = 1;

	regs->actpx_bg2 = timing->begin_pixel_of_active;
	regs->actpx_end2 = timing->end_pixel_of_active;
	regs->h_allpxl2 = timing->total_pixel_of_line;

	regs->actln_bg2 = timing->begin_line_of_active + 1;
	regs->actln_end2 = timing->end_line_of_active + 1;
	regs->v_allln2 = timing->total_line_of_frame;

	regs->vbie_line2 = timing->line_number_between_VBIS_VBIE;
	regs->pvbi_line2 = (timing->line_number_between_PVBI_VBIS < 6) ?
		6 : timing->line_number_between_PVBI_VBIS;

	regs->hdmi_hsynw2 = timing->hsync;
	regs->hdmi_vbisw2 = timing->vsync;

#ifdef DEBUG_DETAIL
	vpp_show_timing("govrh tg2", 0, timing);
#endif
}

void govrh_get_tg(govrh_mod_t *base, vpp_clock_t *tmr)
{
	volatile struct govrh_regs *regs =
		(volatile struct govrh_regs *) base->mmio;

	DBG_DETAIL("(govr %d)\n", (base->mod == VPP_MOD_GOVRH) ? 1 : 2);

	tmr->read_cycle = regs->read_cyc;
	tmr->total_pixel_of_line = regs->h_allpxl;
	tmr->begin_pixel_of_active = regs->actpx_bg;
	tmr->end_pixel_of_active = regs->actpx_end;

	tmr->total_line_of_frame = regs->v_allln;
	tmr->begin_line_of_active = (regs->actln_bg - 1);
	tmr->end_line_of_active = (regs->actln_end - 1);

	tmr->line_number_between_VBIS_VBIE = regs->vbie_line;
	tmr->line_number_between_PVBI_VBIS = regs->pvbi_line;

	tmr->hsync = regs->hdmi_hsynw;
	tmr->vsync = regs->hdmi_vbisw;

	if (regs->tg_enable.b.mode) {
		tmr->vsync = (tmr->vsync - 1) * 2;
		tmr->begin_line_of_active = (tmr->begin_line_of_active - 1) * 2;
		tmr->end_line_of_active = (tmr->end_line_of_active - 1) * 2;
		tmr->total_line_of_frame = (tmr->total_line_of_frame - 1) * 2;
	}
}

int govrh_get_hscale_up(govrh_mod_t *base)
{
	volatile struct govrh_regs *regs =
		(volatile struct govrh_regs *) base->mmio;
	int reg;

	reg = regs->hscale_up & 0x1;
	DBG_DETAIL("(govr %d,reg %d)\n", (base->mod == VPP_MOD_GOVRH) ?
		1 : 2, reg);
	return reg;
}

void govrh_set_direct_path(govrh_mod_t *base, int enable)
{
	volatile struct govrh_regs *regs =
		(volatile struct govrh_regs *) base->mmio;

	DBG_DETAIL("(govr %d,%d)\n", (base->mod == VPP_MOD_GOVRH) ?
		1 : 2, enable);

	regs->dirpath = enable;
}

vpp_int_err_t govrh_get_int_status(govrh_mod_t *base)
{
	volatile struct govrh_regs *regs =
		(volatile struct govrh_regs *) base->mmio;
	vpp_int_err_t int_sts;

	DBG_DETAIL("(govr %d)\n", (base->mod == VPP_MOD_GOVRH) ? 1 : 2);

	int_sts = 0;
	if (regs->interrupt.b.err_sts)
		int_sts |= VPP_INT_ERR_GOVRH_MIF;
	return int_sts;
}

void govrh_clean_int_status(govrh_mod_t *base, vpp_int_err_t int_sts)
{
	volatile struct govrh_regs *regs =
		(volatile struct govrh_regs *) base->mmio;
	unsigned int val;

	DBG_DETAIL("(govr %d,%d)\n", (base->mod == VPP_MOD_GOVRH) ?
		1 : 2, int_sts);

	if (int_sts & VPP_INT_ERR_GOVRH_MIF) {
		val = regs->interrupt.val;
		val = (val & 0xff) + 0x20000;
		regs->interrupt.val = val;
	}
}

void govrh_set_int_enable(govrh_mod_t *base,
			vpp_flag_t enable, vpp_int_err_t int_bit)
{
	volatile struct govrh_regs *regs =
		(volatile struct govrh_regs *) base->mmio;

	DBG_DETAIL("(govr %d,%d)\n", (base->mod == VPP_MOD_GOVRH) ?
				1 : 2, enable);

	/* clean status first before enable/disable interrupt */
	govrh_clean_int_status(base, int_bit);

	if (int_bit & VPP_INT_ERR_GOVRH_MIF)
		regs->interrupt.b.mem_enable = enable;
}

void govrh_set_dvo_enable(govrh_mod_t *base, vpp_flag_t enable)
{
	volatile struct govrh_regs *regs =
		(volatile struct govrh_regs *) base->mmio;
/*	DPRINT("[GOVRH] %s(%d)\n",__FUNCTION__,enable); */

	DBG_DETAIL("(govr %d,%d)\n",
		(base->mod == VPP_MOD_GOVRH) ? 1 : 2, enable);

	regs->dvo_set.b.enable = enable;
#if 1
	if (enable) {
		/* GPIO to function pin */
		vppif_reg32_out(GPIO_BASE_ADDR + 0x44, 0x0);
		/* GPIO pull disable */
		vppif_reg32_out(GPIO_BASE_ADDR + 0x484, 0x0);
		/* 1:pullup,0:pulldn */
		vppif_reg32_out(GPIO_BASE_ADDR + 0x4C4, 0x0);
		return;
	}
	/* disable dvo */
	vppif_reg32_out(GPIO_BASE_ADDR + 0x44, 0xFFFFFFFF); /* Enable GPIO */
	vppif_reg32_out(GPIO_BASE_ADDR + 0x84, 0x0);  /* GPIO output enable */
	vppif_reg32_out(GPIO_BASE_ADDR + 0x484, 0x0); /* GPIO pull disable */
	vppif_reg32_out(GPIO_BASE_ADDR + 0x4C4, 0x0); /* 1:pullup,0:pulldn */
#endif
}

void govrh_set_dvo_sync_polar(govrh_mod_t *base,
					vpp_flag_t hsync, vpp_flag_t vsync)
{
	volatile struct govrh_regs *regs =
		(volatile struct govrh_regs *) base->mmio;

	DBG_DETAIL("(govr %d,%d,%d)\n",
		(base->mod == VPP_MOD_GOVRH) ? 1 : 2, hsync, vsync);

	regs->dvo_set.b.hsync_polar = hsync;
	regs->dvo_set.b.vsync_polar = vsync;
}

vdo_color_fmt govrh_get_dvo_color_format(govrh_mod_t *base)
{
	volatile struct govrh_regs *regs =
		(volatile struct govrh_regs *) base->mmio;

	DBG_DETAIL("(govr %d)\n", (base->mod == VPP_MOD_GOVRH) ? 1 : 2);

	if (regs->dvo_pix.b.rgb)
		return VDO_COL_FMT_ARGB;
	if (regs->dvo_pix.b.yuv422)
		return VDO_COL_FMT_YUV422H;
	return VDO_COL_FMT_YUV444;
}

void govrh_set_dvo_color_format(govrh_mod_t *base, vdo_color_fmt fmt)
{
	volatile struct govrh_regs *regs =
		(volatile struct govrh_regs *) base->mmio;

	DBG_DETAIL("(govr %d,%s)\n",
		(base->mod == VPP_MOD_GOVRH) ? 1 : 2, vpp_colfmt_str[fmt]);

	switch (fmt) {
	case VDO_COL_FMT_ARGB:
		regs->dvo_pix.b.rgb = 1;
		regs->dvo_pix.b.yuv422 = 0;
		break;
	case VDO_COL_FMT_YUV422H:
		regs->dvo_pix.b.rgb = 0;
		regs->dvo_pix.b.yuv422 = 1;
		break;
	case VDO_COL_FMT_YUV444:
	default:
		regs->dvo_pix.b.rgb = 0;
		regs->dvo_pix.b.yuv422 = 0;
		break;
	}
}

void govrh_set_dvo_outdatw(govrh_mod_t *base, vpp_datawidht_t width)
{
	volatile struct govrh_regs *regs =
		(volatile struct govrh_regs *) base->mmio;
	unsigned int clk_delay;

	DBG_DETAIL("(govr %d,%d)\n",
		(base->mod == VPP_MOD_GOVRH) ? 1 : 2, width);

	regs->dvo_set.b.outwidth = (width == VPP_DATAWIDHT_12) ? 1 : 0;

	clk_delay = (width == VPP_DATAWIDHT_24) ?
		VPP_GOVR_DVO_DELAY_24 : VPP_GOVR_DVO_DELAY_12;
	govrh_set_dvo_clock_delay(base, ((clk_delay & BIT14) != 0x0),
		clk_delay & 0x3FFF);
}

void govrh_set_dvo_clock_delay(govrh_mod_t *base,
					int inverse, int delay)
{
	volatile struct govrh_regs *regs =
		(volatile struct govrh_regs *) base->mmio;

	DBG_DETAIL("(govr %d,%d,%d)\n", (base->mod == VPP_MOD_GOVRH) ?
		1 : 2, inverse, delay);

#ifdef CONFIG_HW_DVO_DELAY
	regs = p_govrh->mmio;
#endif

	regs->dvo_dly_sel.b.inv = inverse;
	regs->dvo_dly_sel.b.delay = delay;
}

void govrh_set_colorbar(govrh_mod_t *base,
				vpp_flag_t enable, int mode, int inv)
{
	volatile struct govrh_regs *regs =
		(volatile struct govrh_regs *) base->mmio;

	DBG_DETAIL("(govr %d,%d)\n",
		(base->mod == VPP_MOD_GOVRH) ? 1 : 2, enable);

	regs->cb_enable.b.enable = enable;
	regs->cb_enable.b.mode = mode;
	regs->cb_enable.b.inversion = inv;
}

void govrh_set_contrast(govrh_mod_t *base, unsigned int value)
{
	volatile struct govrh_regs *regs =
		(volatile struct govrh_regs *) base->mmio;

	DBG_DETAIL("(govr %d,0x%x)\n",
		(base->mod == VPP_MOD_GOVRH) ? 1 : 2, value);

	regs->contrast.val = value;
}

unsigned int govrh_get_contrast(govrh_mod_t *base)
{
	volatile struct govrh_regs *regs =
		(volatile struct govrh_regs *) base->mmio;

	DBG_DETAIL("(govr %d)\n", (base->mod == VPP_MOD_GOVRH) ? 1 : 2);

	return regs->contrast.val;
}

void govrh_set_brightness(govrh_mod_t *base, unsigned int value)
{
	volatile struct govrh_regs *regs =
		(volatile struct govrh_regs *) base->mmio;

	DBG_DETAIL("(govr %d,0x%x)\n",
		(base->mod == VPP_MOD_GOVRH) ? 1 : 2, value);

	regs->brightness = value;
}

unsigned int govrh_get_brightness(govrh_mod_t *base)
{
	volatile struct govrh_regs *regs =
		(volatile struct govrh_regs *) base->mmio;

	DBG_DETAIL("(govr %d)\n", (base->mod == VPP_MOD_GOVRH) ? 1 : 2);

	return regs->brightness;
}

void govrh_set_saturation(govrh_mod_t *base, unsigned int value)
{
	volatile struct govrh_regs *regs =
		(volatile struct govrh_regs *) base->mmio;

	DBG_DETAIL("(govr %d,0x%x)\n",
		(base->mod == VPP_MOD_GOVRH) ? 1 : 2, value);

	regs->saturation.val = value;
	regs->saturation_enable.b.enable = 1;
}

unsigned int govrh_get_saturation(govrh_mod_t *base)
{
	volatile struct govrh_regs *regs =
		(volatile struct govrh_regs *) base->mmio;

	DBG_DETAIL("(govr %d)\n", (base->mod == VPP_MOD_GOVRH) ? 1 : 2);

	return regs->saturation.val;
}

void govrh_set_MIF_enable(govrh_mod_t *base, vpp_flag_t enable)
{
	volatile struct govrh_regs *regs =
		(volatile struct govrh_regs *) base->mmio;

	DBG_DETAIL("(govr %d,%d)\n",
		(base->mod == VPP_MOD_GOVRH) ? 1 : 2, enable);

	regs->mif.b.enable = enable;
}

int govrh_get_MIF_enable(govrh_mod_t *base)
{
	volatile struct govrh_regs *regs =
		(volatile struct govrh_regs *) base->mmio;

	DBG_DETAIL("(govr %d)\n", (base->mod == VPP_MOD_GOVRH) ? 1 : 2);

	return regs->mif.b.enable;
}

void govrh_set_frame_mode(govrh_mod_t *base,
				unsigned int width, vdo_color_fmt colfmt)
{
	volatile struct govrh_regs *regs =
		(volatile struct govrh_regs *) base->mmio;
	int y_byte, c_byte;
	int enable;

	enable = 1;
	vpp_get_colfmt_bpp(colfmt, &y_byte, &c_byte);
	y_byte = width * y_byte / 8;
	if (y_byte % 8)
		enable = 0;
	if (c_byte) {
		c_byte = width * c_byte / 8;
		if (c_byte % 8)
			enable = 0;
	}
	if (regs->bufwid % 128)
		enable = 0;
	if ((width == 720) && (colfmt == VDO_COL_FMT_RGB_565))
		enable = 0;
	regs->mif_frame_mode.b.frame_enable = enable;
	regs->mif_frame_mode.b.req_num =
		(y_byte / 128) + ((y_byte % 128) ? 1 : 0);
	regs->mif_frame_mode.b.req_num_c =
		(c_byte) ? ((c_byte / 128) + ((c_byte % 128) ? 1 : 0)) : 0;
}

void govrh_set_color_format(govrh_mod_t *base, vdo_color_fmt format)
{
	volatile struct govrh_regs *regs =
		(volatile struct govrh_regs *) base->mmio;

	DBG_DETAIL("(govr %d,%s)\n",
		(base->mod == VPP_MOD_GOVRH) ? 1 : 2, vpp_colfmt_str[format]);

	if (format < VDO_COL_FMT_ARGB)
		regs->yuv2rgb.b.rgb_mode = 0;

	switch (format) {
	case VDO_COL_FMT_YUV420:
		regs->colfmt = 1;
		regs->colfmt2 = 0;
		break;
	case VDO_COL_FMT_YUV422H:
		regs->colfmt = 0;
		regs->colfmt2 = 0;
		break;
	case VDO_COL_FMT_YUV444:
		regs->colfmt = 0;
		regs->colfmt2 = 1;
		break;
	case VDO_COL_FMT_ARGB:
		regs->yuv2rgb.b.rgb_mode = 1;
		break;
	case VDO_COL_FMT_RGB_1555:
		regs->yuv2rgb.b.rgb_mode = 2;
		break;
	case VDO_COL_FMT_RGB_565:
		regs->yuv2rgb.b.rgb_mode = 3;
		break;
	default:
		DBGMSG("*E* check the parameter\n");
		return;
	}
#ifdef WMT_FTBLK_GOVRH_CURSOR
	govrh_CUR_set_colfmt((govrh_cursor_mod_t *)p_cursor, format);
#endif
	govrh_set_frame_mode(base, regs->pixwid, format);
	regs->saturation_enable.b.format = (regs->yuv2rgb.b.rgb_mode) ? 1 : 0;
}

vdo_color_fmt govrh_get_color_format(govrh_mod_t *base)
{
	volatile struct govrh_regs *regs =
		(volatile struct govrh_regs *) base->mmio;

	DBG_DETAIL("(govr %d)\n", (base->mod == VPP_MOD_GOVRH) ? 1 : 2);

	switch (regs->yuv2rgb.b.rgb_mode) {
	case 1:
		return VDO_COL_FMT_ARGB;
	case 2:
		return VDO_COL_FMT_RGB_1555;
	case 3:
		return VDO_COL_FMT_RGB_565;
	default:
		break;
	}
	if (regs->colfmt2)
		return VDO_COL_FMT_YUV444;
	if (regs->colfmt)
		return VDO_COL_FMT_YUV420;
	return VDO_COL_FMT_YUV422H;
}

void govrh_set_source_format(govrh_mod_t *base,
					vpp_display_format_t format)
{
	volatile struct govrh_regs *regs =
		(volatile struct govrh_regs *) base->mmio;

	DBG_DETAIL("(govr %d,%d)\n",
		(base->mod == VPP_MOD_GOVRH) ? 1 : 2, format);

	regs->srcfmt = (format == VPP_DISP_FMT_FIELD) ? 1 : 0;
}

void govrh_set_output_format(govrh_mod_t *base,
					vpp_display_format_t field)
{
	volatile struct govrh_regs *regs =
		(volatile struct govrh_regs *) base->mmio;

	DBG_DETAIL("(govr %d,%d)\n",
		(base->mod == VPP_MOD_GOVRH) ? 1 : 2, field);

	regs->dstfmt = (field == VPP_DISP_FMT_FIELD) ? 1 : 0;
}

void govrh_set_fb_addr(govrh_mod_t *base,
			unsigned int y_addr, unsigned int c_addr)
{
	volatile struct govrh_regs *regs =
		(volatile struct govrh_regs *) base->mmio;

	/* DBG_DETAIL("(govr %d,0x%x,0x%x)\n",
		(base->mod == VPP_MOD_GOVRH)? 1:2,y_addr,c_addr); */
#ifdef DEBUG
	if ((y_addr % GOVRH_FRAMEBUF_ALIGN) || (c_addr % GOVRH_FRAMEBUF_ALIGN))
		DBG_ERR("addr should align %d(0x%x,0x%x)\n",
		GOVRH_FRAMEBUF_ALIGN, y_addr, c_addr);
#endif
	regs->ysa = y_addr;
	regs->csa = c_addr;
}

void govrh_get_fb_addr(govrh_mod_t *base,
				unsigned int *y_addr, unsigned int *c_addr)
{
	volatile struct govrh_regs *regs =
		(volatile struct govrh_regs *) base->mmio;

	*y_addr = regs->ysa;
	*c_addr = regs->csa;

	DBG_DETAIL("(govr %d,0x%x,0x%x)\n",
		(base->mod == VPP_MOD_GOVRH) ? 1 : 2, *y_addr, *c_addr);
}

void govrh_set_fb2_addr(govrh_mod_t *base,
			unsigned int y_addr, unsigned int c_addr)
{
	volatile struct govrh_regs *regs =
		(volatile struct govrh_regs *) base->mmio;

	DBG_DETAIL("(govr %d,0x%x,0x%x)\n",
		(base->mod == VPP_MOD_GOVRH) ? 1 : 2, y_addr, c_addr);
#ifdef DEBUG
	if ((y_addr % GOVRH_FRAMEBUF_ALIGN) || (c_addr % GOVRH_FRAMEBUF_ALIGN))
		DBG_ERR("addr should align %d(0x%x,0x%x)\n",
				GOVRH_FRAMEBUF_ALIGN, y_addr, c_addr);
#endif
	regs->ysa2 = y_addr;
	regs->csa2 = c_addr;
}

void govrh_get_fb2_addr(govrh_mod_t *base,
			unsigned int *y_addr, unsigned int *c_addr)
{
	volatile struct govrh_regs *regs =
			(volatile struct govrh_regs *) base->mmio;

	*y_addr = regs->ysa2;
	*c_addr = regs->csa2;

	DBG_DETAIL("(govr %d,0x%x,0x%x)\n",
		(base->mod == VPP_MOD_GOVRH) ? 1 : 2, *y_addr, *c_addr);
}

void govrh_set_fb_width(govrh_mod_t *base, unsigned int width)
{
	volatile struct govrh_regs *regs =
		(volatile struct govrh_regs *) base->mmio;

	DBG_DETAIL("(govr %d,%d)\n",
		(base->mod == VPP_MOD_GOVRH) ? 1 : 2, width);

	regs->bufwid = width;
}

#define GOVRH_CURSOR_RIGHT_POS		6
void govrh_set_fb_info(govrh_mod_t *base, unsigned int width,
	unsigned int act_width, unsigned int x_offset, unsigned int y_offset)
{
	volatile struct govrh_regs *regs =
		(volatile struct govrh_regs *) base->mmio;

	DBG_DETAIL("(govr %d,fb_w %d,img_w %d,x %d,y %d)\n",
		(base->mod == VPP_MOD_GOVRH) ? 1 : 2,
		width, act_width, x_offset, y_offset);

	regs->pixwid = act_width;
	regs->bufwid = width;
	regs->hcrop = x_offset;
	regs->vcrop = y_offset;
	govrh_set_frame_mode(base, act_width, govrh_get_color_format(base));

#ifdef WMT_FTBLK_GOVRH_CURSOR
	/* cursor postion over when change resolution */
	if ((regs->actpx_end - regs->actpx_bg) != regs->pixwid) {
		if (regs->cur_hcoord.b.end >=
			(regs->pixwid - GOVRH_CURSOR_RIGHT_POS)) {
			int pos;

			pos = regs->pixwid - GOVRH_CURSOR_RIGHT_POS;
			regs->cur_hcoord.b.end = pos;
			pos -= regs->cur_width;
			regs->cur_hcoord.b.start = (pos+1);

			p_cursor->posx = pos;
		}
	}
#endif
}

void govrh_get_fb_info(govrh_mod_t *base, unsigned int *width,
	unsigned int *act_width, unsigned int *x_offset, unsigned int *y_offset)
{
	volatile struct govrh_regs *regs =
		(volatile struct govrh_regs *) base->mmio;

	DBG_DETAIL("(govr %d)\n", (base->mod == VPP_MOD_GOVRH) ? 1 : 2);

	*act_width = regs->pixwid;
	*width = regs->bufwid;
	*x_offset = regs->hcrop;
	*y_offset = regs->vcrop;
}

void govrh_set_fifo_index(govrh_mod_t *base, unsigned int index)
{
	volatile struct govrh_regs *regs =
		(volatile struct govrh_regs *) base->mmio;

	DBG_DETAIL("(govr %d,%d)\n",
		(base->mod == VPP_MOD_GOVRH) ? 1 : 2, index);

	regs->fhi = index;
}

void govrh_set_reg_level(govrh_mod_t *base, vpp_reglevel_t level)
{
	volatile struct govrh_regs *regs =
		(volatile struct govrh_regs *) base->mmio;

	DBG_DETAIL("(govr %d,%d)\n",
		(base->mod == VPP_MOD_GOVRH) ? 1 : 2, level);

	regs->sts.b.level = (level == VPP_REG_LEVEL_1) ? 0 : 1;
}

void govrh_set_reg_update(govrh_mod_t *base, vpp_flag_t enable)
{
	volatile struct govrh_regs *regs =
		(volatile struct govrh_regs *) base->mmio;

	DBG_DETAIL("(govr %d,%d)\n",
		(base->mod == VPP_MOD_GOVRH) ? 1 : 2, enable);

	regs->sts.b.update = enable;
}

void govrh_set_tg(govrh_mod_t *base,
			vpp_clock_t *tmr, unsigned int pixel_clock)
{
	volatile struct govrh_regs *regs =
		(volatile struct govrh_regs *) base->mmio;
	int tg_enable;

	DBG_DETAIL("(govr %d,%d)\n",
		(base->mod == VPP_MOD_GOVRH) ? 1 : 2, pixel_clock);

	tg_enable = regs->tg_enable.b.enable;
	regs->tg_enable.b.enable = 0;

	tmr->read_cycle = govrh_set_clock(base, pixel_clock);
	govrh_set_tg1(base, tmr);

	regs->tg_enable.b.enable = tg_enable;
}

void govrh_set_csc_mode(govrh_mod_t *base, vpp_csc_t mode)
{
	volatile struct govrh_regs *regs =
		(volatile struct govrh_regs *) base->mmio;
	vdo_color_fmt src_fmt, dst_fmt;
	unsigned int enable;
	unsigned int csc_parm5;

	DBG_DETAIL("(govr %d,%d)\n",
		(base->mod == VPP_MOD_GOVRH) ? 1 : 2, mode);

	enable = 0;
	src_fmt = (govrh_get_color_format(base) < VDO_COL_FMT_ARGB) ?
		VDO_COL_FMT_YUV444 : VDO_COL_FMT_ARGB;
	dst_fmt = (govrh_get_dvo_color_format(base) < VDO_COL_FMT_ARGB) ?
		VDO_COL_FMT_YUV444 : VDO_COL_FMT_ARGB;
	if (src_fmt == VDO_COL_FMT_ARGB)
		enable |= (dst_fmt != VDO_COL_FMT_ARGB) ? BIT0 : 0x00; /* DVO */
	else
		enable |= (dst_fmt == VDO_COL_FMT_ARGB) ? BIT0 : 0x00; /* DVO */
#ifdef WMT_FTBLK_LVDS
	vpp_set_clock_enable(DEV_LVDS, 1, 0);
	if (src_fmt == VDO_COL_FMT_ARGB) {
		if (lvds_get_colfmt() < VDO_COL_FMT_ARGB)
			enable |= BIT3;
	} else {
		if (lvds_get_colfmt() >= VDO_COL_FMT_ARGB)
			enable |= BIT3;
	}
	vpp_set_clock_enable(DEV_LVDS, 0, 0);
#endif
#ifdef WMT_FTBLK_HDMI
	vpp_set_clock_enable(DEV_HDMI, 1, 0);
	if (src_fmt == VDO_COL_FMT_ARGB) {
		if (hdmi_get_output_colfmt() < VDO_COL_FMT_ARGB)
			enable |= BIT4;
	} else {
		if (hdmi_get_output_colfmt() >= VDO_COL_FMT_ARGB)
			enable |= BIT4;
	}
	vpp_set_clock_enable(DEV_HDMI, 0, 0);
#endif
	mode = vpp_check_csc_mode(mode, src_fmt, dst_fmt, enable);
	if (mode >= VPP_CSC_MAX) {
		enable = 0;
		/* for internal color bar (YUV base) */
		mode = VPP_CSC_YUV2RGB_JFIF_0_255;
	}
	/* HDMI YUV mode too light 0xff00ff -> 0x01010001 */
	csc_parm5 = (mode == VPP_CSC_RGB2YUV_JFIF_0_255) ?
			0x01010001 : vpp_csc_parm[mode][5];

	regs->dmacsc_coef0 = vpp_csc_parm[mode][0]; /* C1,C2 */
	regs->dmacsc_coef1 = vpp_csc_parm[mode][1]; /* C3,C4 */
	regs->dmacsc_coef2 = vpp_csc_parm[mode][2]; /* C5,C6 */
	regs->dmacsc_coef3 = vpp_csc_parm[mode][3]; /* C7,C8 */
	regs->dmacsc_coef4 = vpp_csc_parm[mode][4] & 0xFFFF; /* C9 */
	regs->dmacsc_coef5 = ((vpp_csc_parm[mode][4] & 0xFFFF0000) >> 16) +
				((csc_parm5 & 0xFFFF) << 16); /* I,J */
	regs->dmacsc_coef6 = (csc_parm5 & 0xFFFF0000) >> 16;    /* K */
	regs->csc_mode.val = (vpp_csc_parm[mode][6] & BIT0) +
			((vpp_csc_parm[mode][6] & BIT8) ? BIT1 : 0x0);

	/* enable bit0:DVO, bit1:VGA, bit2:DISP, bit3:LVDS, bit4:HDMI */
	regs->yuv2rgb.val = (regs->yuv2rgb.val & ~0xFF) +
			((mode >= VPP_CSC_RGB2YUV_SDTV_0_255) ? 0x1C : 0x18);
	regs->yuv2rgb.b.dvo = (enable & BIT0) ? 1 : 0;
#ifdef WMT_FTBLK_LVDS
	regs->yuv2rgb.b.lvds = (enable & BIT3) ? 1 : 0;
#endif
#ifdef WMT_FTBLK_HDMI
	regs->yuv2rgb.b.hdmi = (enable & BIT4) ? 1 : 0;
#endif
}

void govrh_set_media_format(govrh_mod_t *base, vpp_flag_t h264)
{
	volatile struct govrh_regs *regs =
		(volatile struct govrh_regs *) base->mmio;

	DBG_DETAIL("(govr %d,%d)\n",
		(base->mod == VPP_MOD_GOVRH) ? 1 : 2, h264);

	regs->mif.b.h264 = h264;
}

void govrh_HDMI_set_blank_value(govrh_mod_t *base, unsigned int val)
{
	volatile struct govrh_regs *regs =
		(volatile struct govrh_regs *) base->mmio;

	DBG_DETAIL("(govr %d,%d)\n", (base->mod == VPP_MOD_GOVRH) ? 1 : 2, val);

	regs->hdmi_3d.b.blank_value = val;
}

void govrh_HDMI_set_3D_mode(govrh_mod_t *base, int mode)
{
	volatile struct govrh_regs *regs =
		(volatile struct govrh_regs *) base->mmio;

	DBG_DETAIL("(govr %d,%d)\n",
		(base->mod == VPP_MOD_GOVRH) ? 1 : 2, mode);

	/* 0-disable,3-frame pack progress(use fb1&2),7-frame pack interlace */
	regs->hdmi_3d.b.mode = mode;
}

int govrh_is_top_field(govrh_mod_t *base)
{
	volatile struct govrh_regs *regs =
		(volatile struct govrh_regs *) base->mmio;

	DBG_DETAIL("(govr %d)\n", (base->mod == VPP_MOD_GOVRH) ? 1 : 2);

	return (regs->field_status) ? 1 : 0;
}

/*----------------------- GOVRH IGS --------------------------------------*/
void govrh_IGS_set_mode(govrh_mod_t *base,
				int no, int mode_18bit, int msb)
{
	volatile struct govrh_regs *regs =
		(volatile struct govrh_regs *) base->mmio;

	DBG_DETAIL("(govr %d,%d)\n", (base->mod == VPP_MOD_GOVRH) ? 1 : 2, no);

	if (no == 0) { /* DVO */
		/* 0-24bit,10-18bit,01-555,11-565 */
		regs->igs_mode.b.mode = mode_18bit;
		regs->igs_mode.b.ldi = msb; /* 0-lsb,1-msb */
	} else { /* LVDS */
		/* 0-24bit,10-18bit,01-555,11-565 */
		regs->igs_mode2.b.mode = mode_18bit;
		regs->igs_mode2.b.ldi = msb; /* 0-lsb,1-msb */
	}
}

void govrh_IGS_set_RGB_swap(govrh_mod_t *base, int mode)
{
	volatile struct govrh_regs *regs =
		(volatile struct govrh_regs *) base->mmio;

	DBG_DETAIL("(govr %d,%d)\n",
		(base->mod == VPP_MOD_GOVRH) ? 1 : 2, mode);

	/* 0-RGB [7-0], 1-RGB [0-7], 2-BGR [7-0], 3-BGR [0-7] */
	regs->dvo_set.b.rgb_swap = mode;
}

/*----------------------- GOVRH CURSOR --------------------------------------*/
#ifdef WMT_FTBLK_GOVRH_CURSOR
#define CONFIG_GOVR2_CURSOR
void *govrh_CUR_get_mmio(govrh_cursor_mod_t *base)
{
#ifdef CONFIG_GOVR2_CURSOR
	base = (govrh_cursor_mod_t *)
		((govrh_get_MIF_enable(p_govrh)) ? p_govrh : p_govrh2);
#endif
	return base->mmio;
}

void govrh_CUR_set_enable(govrh_cursor_mod_t *base, vpp_flag_t enable)
{
	volatile struct govrh_regs *regs =
		(volatile struct govrh_regs *) govrh_CUR_get_mmio(base);

	DBG_DETAIL("(0x%x,%d)\n", base->mmio, enable);

	regs->cur_status.b.enable = enable;
}

void govrh_CUR_set_fb_addr(govrh_cursor_mod_t *base,
				unsigned int y_addr, unsigned int c_addr)
{
	volatile struct govrh_regs *regs =
		(volatile struct govrh_regs *) govrh_CUR_get_mmio(base);

	DBG_DETAIL("(0x%x,0x%x,0x%x)\n", base->mmio, y_addr, c_addr);

	regs->cur_addr = y_addr;
}

void govrh_CUR_get_fb_addr(govrh_cursor_mod_t *base,
			unsigned int *y_addr, unsigned int *c_addr)
{
	volatile struct govrh_regs *regs =
		(volatile struct govrh_regs *) govrh_CUR_get_mmio(base);

	*y_addr = regs->cur_addr;
	*c_addr = 0;

	DBG_DETAIL("(0x%x,0x%x,0x%x)\n", base->mmio, *y_addr, *c_addr);
}

void govrh_CUR_set_coordinate(govrh_cursor_mod_t *base,
	unsigned int x1, unsigned int y1, unsigned int x2, unsigned int y2)
{
	volatile struct govrh_regs *regs =
		(volatile struct govrh_regs *) govrh_CUR_get_mmio(base);

	DBG_DETAIL("(0x%x)\n", base->mmio);

	regs->cur_hcoord.b.start = x1;
	regs->cur_hcoord.b.end = x2;
	regs->cur_vcoord.b.start = y1;
	regs->cur_vcoord.b.end = y2;
}

void govrh_CUR_set_position(govrh_cursor_mod_t *base,
					unsigned int x, unsigned int y)
{
	volatile struct govrh_regs *regs =
		(volatile struct govrh_regs *) govrh_CUR_get_mmio(base);
	unsigned int w, h;

	DBG_DETAIL("(0x%x,%d,%d)\n", base->mmio, x, y);

/*	w = regs->cur_hcoord.b.end - regs->cur_hcoord.b.start; */
	w = regs->cur_width - 1;
	h = regs->cur_vcoord.b.end - regs->cur_vcoord.b.start;
	govrh_CUR_set_coordinate(base, x, y, x+w, y+h);
}

void govrh_CUR_set_color_key_mode(govrh_cursor_mod_t *base,
					int enable, int alpha, int mode)
{
	/*
		if( colkey_en )
			colkey_hit = (mode)? (data == colkey):(data != colkey);

		if( (alpha_en) or (colkey_hit) or (alpha value) )
			show pixel
	*/
	volatile struct govrh_regs *regs =
		(volatile struct govrh_regs *) govrh_CUR_get_mmio(base);

	DBG_DETAIL("(0x%x,%d,%d,%d)\n", base->mmio, enable, alpha, mode);

	regs->cur_color_key.b.alpha = alpha;
	regs->cur_color_key.b.enable = enable;
	regs->cur_color_key.b.invert = mode;
}

void govrh_CUR_set_color_key(govrh_cursor_mod_t *base,
				int enable, int alpha, unsigned int colkey)
{
	volatile struct govrh_regs *regs =
		(volatile struct govrh_regs *) govrh_CUR_get_mmio(base);

	DBG_DETAIL("(0x%x,0x%x)\n", base->mmio, colkey);

	regs->cur_color_key.b.colkey = colkey;
	regs->cur_color_key.b.alpha = 0;
	regs->cur_color_key.b.enable = 0;

#ifdef __KERNEL__
	if (!(alpha) && enable) {
		int i, j;
		unsigned int *ptr1, *ptr2;
		int yuv2rgb;

		ptr1 = (unsigned int *)mb_phys_to_virt(regs->cur_addr);
		for (i = 0; i < base->fb_p->fb.fb_h; i++) {
			for (j = 0; j < base->fb_p->fb.fb_w; j++) {
				if ((*ptr1 & 0xFFFFFF) == (colkey & 0xFFFFFF))
					*ptr1 &= 0xFFFFFF;
				else
					*ptr1 |= 0xFF000000;
				ptr1++;
			}
		}

		yuv2rgb = (base->colfmt < VDO_COL_FMT_ARGB) ? 1 : 0;
		ptr1 = (unsigned int *) mb_phys_to_virt(base->cursor_addr1);
		ptr2 = (unsigned int *) mb_phys_to_virt(base->cursor_addr2);
		for (i = 0; i < base->fb_p->fb.fb_h; i++) {
			for (j = 0; j < base->fb_p->fb.fb_w; j++) {
				*ptr2 = vpp_convert_colfmt(yuv2rgb, *ptr1);
				ptr1++;
				ptr2++;
			}
		}
	}
#endif
}

void govrh_CUR_set_colfmt(govrh_cursor_mod_t *base,
					vdo_color_fmt colfmt)
{
#ifdef __KERNEL__
	volatile struct govrh_regs *regs =
		(volatile struct govrh_regs *) govrh_CUR_get_mmio(base);
#endif

	DBG_DETAIL("(0x%x,%s)\n", base->mmio, vpp_colfmt_str[colfmt]);

	colfmt = (colfmt < VDO_COL_FMT_ARGB) ?
			VDO_COL_FMT_YUV444 : VDO_COL_FMT_ARGB;
	if (base->fb_p->fb.col_fmt == colfmt)
		return;

	base->fb_p->fb.col_fmt = colfmt;
#ifdef __KERNEL__
	regs->cur_addr = (base->colfmt == colfmt) ?
			base->cursor_addr1 : base->cursor_addr2;
#endif
	DBG_DETAIL("(0x%x)\n", regs->cur_addr);
}

int govrh_irqproc_set_position(void *arg)
{
	govrh_cursor_mod_t *base = (govrh_cursor_mod_t *) arg;
	volatile struct govrh_regs *regs =
		(volatile struct govrh_regs *) govrh_CUR_get_mmio(base);

	int begin;
	int pos_x, pos_y;
	int crop_x, crop_y;
	int width, height;
	int govrh_w, govrh_h;

	govrh_w = regs->actpx_end - regs->actpx_bg;
	govrh_h = regs->actln_end - regs->actln_bg;
	if (regs->tg_enable.b.mode)
		govrh_h *= 2;

	/* cursor H */
	width = base->fb_p->fb.img_w;
	begin = base->posx - base->hotspot_x;
	if (begin < 0) {	/* over left edge */
		pos_x = 0;
		crop_x = 0 - begin;
		width = width - crop_x;
	} else {
		pos_x = begin;
		crop_x = 0;
	}
	if ((begin + base->fb_p->fb.img_w) > govrh_w) /* over right edge */
		width = govrh_w - begin;

/*	DPRINT("[CURSOR] H pos %d,hotspot %d,posx %d,crop %d,width %d\n",
		p_cursor->posx,p_cursor->hotspot_x,pos_x,crop_x,width); */

	/* cursor V */
	height = base->fb_p->fb.img_h;
	begin = base->posy - base->hotspot_y;
	if (begin < 0) { /* over top edge */
		pos_y = 0;
		crop_y = 0 - begin;
		height = height - crop_y;
	} else {
		pos_y = begin;
		crop_y = 0;
	}
	if ((begin + base->fb_p->fb.img_h) > govrh_h) /* over right edge */
		height = govrh_h - begin;

	if (regs->tg_enable.b.mode) {
		regs->cur_fb_width = (base->fb_p->fb.fb_w*2);
		pos_y /= 2;
		crop_y /= 2;
		height /= 2;
	} else
		regs->cur_fb_width = base->fb_p->fb.fb_w;

/*	DPRINT("[CURSOR] V pos %d,hotspot %d,posx %d,crop %d,height %d\n",
		p_cursor->posy,p_cursor->hotspot_y,pos_y,crop_y,height); */

	regs->cur_width = width;
	regs->cur_vcrop = crop_y;
	regs->cur_hcrop = crop_x;
	govrh_CUR_set_coordinate(base, pos_x, pos_y,
		pos_x + width - 1, pos_y + height - 1);
	return 0;
}

void govrh_CUR_irqproc(int arg)
{
	if (p_cursor->enable) {
		if (p_cursor->chg_flag) {
			p_cursor->hide_cnt = 0;
			govrh_CUR_set_enable(p_cursor, 1);
		} else {
			p_cursor->hide_cnt++;
			if (p_cursor->hide_cnt >
			(GOVRH_CURSOR_HIDE_TIME * p_govrh->fb_p->framerate))
				govrh_CUR_set_enable(p_cursor, 0);
		}
	}

	if (p_cursor->chg_flag) {
		govrh_irqproc_set_position(p_cursor);
		p_cursor->chg_flag = 0;
	}
}

void govrh_CUR_proc_view(govrh_cursor_mod_t *base,
					int read, vdo_view_t *view)
{
	vdo_framebuf_t *fb;

	DBG_DETAIL("(0x%x)\n", base->mmio);

	fb = &base->fb_p->fb;
	if (read) {
		view->resx_src = fb->fb_w;
		view->resy_src = fb->fb_h;
		view->resx_virtual = fb->img_w;
		view->resy_virtual = fb->img_h;
		view->resx_visual = fb->img_w;
		view->resy_visual = fb->img_h;
		view->posx = base->posx;
		view->posy = base->posy;
		view->offsetx = fb->h_crop;
		view->offsety = fb->v_crop;
	} else {
		base->posx = view->posx;
		base->posy = view->posy;
		base->chg_flag = 1;
	}
}

void govrh_CUR_set_framebuffer(govrh_cursor_mod_t *base,
					vdo_framebuf_t *fb)
{
	volatile struct govrh_regs *regs =
		(volatile struct govrh_regs *) govrh_CUR_get_mmio(base);

	DBG_DETAIL("(0x%x)\n", base->mmio);

	regs->cur_addr = fb->y_addr;
	regs->cur_width = fb->img_w;
	if (regs->tg_enable.b.mode)
		regs->cur_fb_width = (fb->fb_w*2);
	else
		regs->cur_fb_width = fb->fb_w;
	regs->cur_vcrop = fb->v_crop;
	regs->cur_hcrop = fb->h_crop;
	regs->cur_status.b.out_field = vpp_get_fb_field(fb);
	govrh_irqproc_set_position(base);
	base->colfmt = fb->col_fmt;
	base->cursor_addr1 = fb->y_addr;
#ifdef __KERNEL__
	if (base->cursor_addr2 == 0)
		base->cursor_addr2 = mb_alloc(64*64*4);
#endif
	vpp_cache_sync();
}

void govrh_CUR_init(void *base)
{
#ifdef CONFIG_GOVR2_CURSOR
	volatile struct govrh_regs *regs;

	regs = (volatile struct govrh_regs *) p_govrh->mmio;
	regs->cur_color_key.b.invert = 1;
	regs = (volatile struct govrh_regs *) p_govrh2->mmio;
	regs->cur_color_key.b.invert = 1;
#else
	govrh_cursor_mod_t *mod_p;
	volatile struct govrh_regs *regs;

	mod_p = (govrh_cursor_mod_t *) base;
	regs = (volatile struct govrh_regs *) mod_p->mmio;

	regs->cur_color_key.b.invert = 1;
#endif
}
#endif /* WMT_FTBLK_GOVRH_CURSOR */

/*----------------------- GOVRH API --------------------------------------*/
void govrh_vmode_to_timing(vpp_mod_t mod,
				struct fb_videomode *vmode, vpp_clock_t *t)
{
	unsigned int pixel_clock;
	int temp;

	t->begin_pixel_of_active = vmode->hsync_len + vmode->left_margin;
	t->end_pixel_of_active = t->begin_pixel_of_active + vmode->xres;
	t->total_pixel_of_line = t->end_pixel_of_active + vmode->right_margin;
	t->begin_line_of_active = vmode->vsync_len + vmode->upper_margin;
	t->end_line_of_active = t->begin_line_of_active + vmode->yres;
	t->total_line_of_frame = t->end_line_of_active + vmode->lower_margin;
	t->line_number_between_VBIS_VBIE = vmode->vsync_len + 1;
	temp = t->total_line_of_frame - t->end_line_of_active;
	t->line_number_between_PVBI_VBIS = (temp > 2) ? (temp-1) : 1;
	t->hsync = vmode->hsync_len;
	t->vsync = vmode->vsync_len;

	pixel_clock = PICOS2KHZ(vmode->pixclock) * 1000;
	t->read_cycle = vpp_get_base_clock(mod) / pixel_clock;
}

void govrh_set_videomode(govrh_mod_t *base,
					struct fb_videomode *p_vmode)
{
	volatile struct govrh_regs *regs =
		(volatile struct govrh_regs *) base->mmio;
	struct fb_videomode vmode;
	vpp_clock_t t1;
	int tg_enable;

	DBG_MSG("(govr %d)\n", (base->mod == VPP_MOD_GOVRH) ? 1 : 2);

	tg_enable = regs->tg_enable.b.enable;
	regs->tg_enable.b.enable = 0;

	vmode = *p_vmode;
	if (vmode.vmode & FB_VMODE_DOUBLE) {
		vmode.xres *= 2;
		DBGMSG("[GOVRH] H scale up\n");
	}

	if (vmode.vmode & FB_VMODE_INTERLACED) {
		vmode.vsync_len /= 2;
		vmode.upper_margin /= 2;
		vmode.yres /= 2;
		vmode.lower_margin /= 2;
	}

	govrh_vmode_to_timing(base->mod, &vmode, &t1);
	t1.read_cycle = govrh_set_clock(base, PICOS2KHZ(vmode.pixclock)*1000);
	if (vmode.vmode & FB_VMODE_INTERLACED) {
		t1.total_line_of_frame += 1;
		t1.begin_line_of_active += 1;
		t1.end_line_of_active += 1;
		t1.vsync += 1;
	}
	govrh_set_tg1(base, &t1);

	if (vmode.vmode & FB_VMODE_INTERLACED) {
		vpp_clock_t t2;

		vmode.upper_margin += 1;
		/* 480i/576i repeat 2 pixel */
		if (vmode.vmode & FB_VMODE_DOUBLE)
			vmode.xres *= 2;
		govrh_vmode_to_timing(base->mod, &vmode, &t2);
		t2.total_line_of_frame -= 1;
		t2.vsync += 1;
		govrh_set_tg2(base, &t2);
	}
	govrh_set_output_format(base,
		(vmode.vmode & FB_VMODE_INTERLACED) ? 1 : 0);
	regs->hscale_up = (vmode.vmode & FB_VMODE_DOUBLE) ? 1 : 0;
	regs->vsync_offset.val = (vmode.vmode & FB_VMODE_INTERLACED) ?
		((regs->h_allpxl / 2) | BIT16) : 0;

	regs->tg_enable.b.enable = tg_enable;
}

void govrh_get_videomode(govrh_mod_t *base, struct fb_videomode *vmode)
{
	volatile struct govrh_regs *regs
		= (volatile struct govrh_regs *) base->mmio;
	vpp_clock_t t;
	int htotal, vtotal;
	unsigned int pixel_clock;

	vmode->flag = FB_MODE_IS_FROM_VAR;
	vmode->name = 0;

	govrh_get_tg(base, &t);
	pixel_clock = vpp_get_base_clock(base->mod);
	vmode->pixclock = KHZ2PICOS(pixel_clock/1000);

	vmode->hsync_len = t.hsync;
	vmode->left_margin = t.begin_pixel_of_active - t.hsync;
	vmode->xres = t.end_pixel_of_active - t.begin_pixel_of_active;
	vmode->right_margin = t.total_pixel_of_line - t.end_pixel_of_active;

	vmode->vsync_len = t.vsync;
	vmode->upper_margin = t.begin_line_of_active - t.vsync;
	vmode->yres = t.end_line_of_active - t.begin_line_of_active;
	vmode->lower_margin = t.total_line_of_frame - t.end_line_of_active;

	htotal = vmode->xres + vmode->right_margin
		+ vmode->hsync_len + vmode->left_margin;
	vtotal = vmode->yres + vmode->lower_margin
		+ vmode->vsync_len + vmode->upper_margin;
	vmode->vmode = (regs->tg_enable.b.mode) ? FB_VMODE_INTERLACED : 0;
	if (vmode->vmode & FB_VMODE_INTERLACED)
		vtotal /= 2;
	if (vmode->vmode & FB_VMODE_DOUBLE)
		vtotal *= 2;

	if (htotal && vtotal)
		vmode->refresh = pixel_clock/(htotal * vtotal);
	else
		vmode->refresh = 60;

	{ /* get sync polar */
		int hsync_hi, vsync_hi;
		vout_inf_mode_t interface;

		switch (base->mod) {
		case VPP_MOD_GOVRH:
			interface = VOUT_INF_HDMI;
			break;
		case VPP_MOD_GOVRH2:
			interface = VOUT_INF_DVI;
			if (lcd_get_type())
				interface = VOUT_INF_LVDS;
				break;
		default:
			interface = VOUT_INF_DVI;
			break;
		}

		switch (interface) {
		case VOUT_INF_DVI:
			hsync_hi = (regs->dvo_set.b.hsync_polar) ? 0 : 1;
			vsync_hi = (regs->dvo_set.b.vsync_polar) ? 0 : 1;
			break;
		case VOUT_INF_HDMI:
			vo_hdmi_set_clock(1);
			hdmi_get_sync_polar(&hsync_hi, &vsync_hi);
			vo_hdmi_set_clock(0);
			break;
		case VOUT_INF_LVDS:
			lvds_get_sync_polar(&hsync_hi, &vsync_hi);
			break;
		default:
			hsync_hi = 0;
			vsync_hi = 0;
			break;
		}
		vmode->sync = (hsync_hi) ? FB_SYNC_HOR_HIGH_ACT : 0;
		vmode->sync |= (vsync_hi) ? FB_SYNC_VERT_HIGH_ACT : 0;
	}
}

void govrh_set_framebuffer(govrh_mod_t *base, vdo_framebuf_t *fb)
{
	DBG_DETAIL("(govr %d)\n", (base->mod == VPP_MOD_GOVRH) ? 1 : 2);

	govrh_set_fb_addr(base, fb->y_addr, fb->c_addr);
	govrh_set_color_format(base, fb->col_fmt);
	govrh_set_fb_info(base, fb->fb_w, fb->img_w, fb->h_crop, fb->v_crop);
	govrh_set_source_format(base, vpp_get_fb_field(fb));
	govrh_set_csc_mode(base, base->fb_p->csc_mode);
}

void govrh_get_framebuffer(govrh_mod_t *base, vdo_framebuf_t *fb)
{
	volatile struct govrh_regs *regs
		= (volatile struct govrh_regs *) base->mmio;
	vpp_clock_t clock;
	int y_bpp, c_bpp;

	DBG_DETAIL("(govr %d)\n", (base->mod == VPP_MOD_GOVRH) ? 1 : 2);

	govrh_get_fb_addr(base, &fb->y_addr, &fb->c_addr);
	fb->col_fmt = govrh_get_color_format(base);
	govrh_get_fb_info(base, &fb->fb_w,
		&fb->img_w, &fb->h_crop, &fb->v_crop);
	govrh_get_tg(base, &clock);
	fb->img_h = clock.end_line_of_active - clock.begin_line_of_active;
	fb->fb_h = fb->img_h;
	fb->flag = (regs->srcfmt) ? VDO_FLAG_INTERLACE : 0;
	vpp_get_colfmt_bpp(fb->col_fmt, &y_bpp, &c_bpp);
	fb->bpp = y_bpp + c_bpp;
	fb->y_size = fb->fb_w * fb->fb_h * y_bpp / 8;
	fb->c_size = fb->fb_w * fb->fb_h * c_bpp / 8;
}

/*----------------------- GOVRH MODULE API -----------------------------*/
int govrh_suspend_yaddr;
void govrh_suspend(govrh_mod_t *base, int sts)
{
#ifdef CONFIG_PM
	volatile struct govrh_regs *regs
			= (volatile struct govrh_regs *) base->mmio;

	switch (sts) {
	case 0:	/* disable module */
		base->pm_enable = govrh_get_MIF_enable(base);
		govrh_set_MIF_enable(base, 0);
		break;
	case 1: /* disable tg */
		base->pm_tg = regs->tg_enable.b.enable;
		govrh_set_tg_enable(base, 0);
		break;
	case 2:	/* backup register */
		base->reg_bk = vpp_backup_reg((unsigned int)base->mmio,
			(REG_GOVRH_BASE2_END-REG_GOVRH_BASE1_BEGIN));
		if (base->vo_clock == 0)
			base->vo_clock = vpp_get_base_clock(base->mod);
		govrh_suspend_yaddr = 0;
		break;
	default:
		break;
	}
#endif
}

void govrh_resume(govrh_mod_t *base, int sts)
{
#ifdef CONFIG_PM
/*	volatile struct govrh_regs *regs
			= (volatile struct govrh_regs *) base->mmio; */

	switch (sts) {
	case 0:	/* restore register */
		govrh_set_clock(base, base->vo_clock);
		vpp_restore_reg((unsigned int)base->mmio,
			(REG_GOVRH_BASE2_END-REG_GOVRH_BASE1_BEGIN),
			base->reg_bk);
		if (base == p_govrh && govrh_suspend_yaddr)
			govrh_set_fb_addr(p_govrh, govrh_suspend_yaddr, 0);
		base->reg_bk = 0;
		break;
	case 1:	/* enable module */
		govrh_set_MIF_enable(base, base->pm_enable);
		break;
	case 2: /* enable tg */
		govrh_set_tg_enable(base, base->pm_tg);
		break;
	default:
		break;
	}
#endif
}

void govrh_init(void *base)
{
	govrh_mod_t *mod_p;
	volatile struct govrh_regs *regs;

	mod_p = (govrh_mod_t *) base;
	regs = (volatile struct govrh_regs *) mod_p->mmio;

	govrh_set_reg_level(base, VPP_REG_LEVEL_1);
	govrh_set_colorbar(base, VPP_FLAG_DISABLE, 1, 0);
	govrh_set_tg_enable(base, VPP_FLAG_ENABLE);
	govrh_set_dvo_color_format(base, VDO_COL_FMT_ARGB);
	govrh_set_dvo_enable(base, VPP_FLAG_DISABLE);

	govrh_set_framebuffer(base, &mod_p->fb_p->fb);
	govrh_set_fifo_index(base, 0xf);
	govrh_set_csc_mode(base, mod_p->fb_p->csc_mode);

	govrh_set_reg_update(base, VPP_FLAG_ENABLE);
	govrh_set_tg_enable(base, VPP_FLAG_ENABLE);
/*	govrh_set_MIF_enable(base, VPP_FLAG_ENABLE); */
	govrh_set_int_enable(base, VPP_FLAG_ENABLE, mod_p->int_catch);
}

void govrh_mod_dump_reg(void)
{
	govrh_reg_dump((void *)p_govrh);
}

void govrh_mod_set_enable(vpp_flag_t enable)
{
	govrh_set_MIF_enable(p_govrh, enable);
}

void govrh_mod_set_colorbar(vpp_flag_t enable, int mode, int inv)
{
	govrh_set_colorbar(p_govrh, enable, mode, inv);
}

void govrh_mod_set_tg(vpp_clock_t *tmr, unsigned int pixel_clock)
{
	govrh_set_tg(p_govrh, tmr, pixel_clock);
}

void govrh_mod_get_tg(vpp_clock_t *tmr)
{
	govrh_get_tg(p_govrh, tmr);
}

unsigned int govrh_mod_get_sts(void)
{
	return govrh_get_int_status(p_govrh);
}

void govrh_mod_clr_sts(unsigned int sts)
{
	govrh_clean_int_status(p_govrh, sts);
}

void govrh_mod_suspend(int sts)
{
	govrh_suspend(p_govrh, sts);
}

void govrh_mod_resume(int sts)
{
	govrh_resume(p_govrh, sts);
}

void govrh_mod_set_framebuf(vdo_framebuf_t *fb)
{
	govrh_set_framebuffer(p_govrh, fb);
}
void govrh_mod_set_addr(unsigned int yaddr, unsigned int caddr)
{
	govrh_set_fb_addr(p_govrh, yaddr, caddr);
}
void govrh_mod_get_addr(unsigned int *yaddr, unsigned int *caddr)
{
	govrh_get_fb_addr(p_govrh, yaddr, caddr);
}
void govrh_mod_set_csc(vpp_csc_t mode)
{
	govrh_set_csc_mode(p_govrh, mode);
}
vdo_color_fmt govrh_mod_get_color_fmt(void)
{
	return govrh_get_color_format(p_govrh);
}
void govrh_mod_set_color_fmt(vdo_color_fmt colfmt)
{
	govrh_set_color_format(p_govrh, colfmt);
}

#ifdef WMT_FTBLK_GOVRH_CURSOR
void cursor_mod_set_enable(vpp_flag_t enable)
{
	govrh_CUR_set_enable(p_cursor, enable);
}

void cursor_mod_set_framebuf(vdo_framebuf_t *fb)
{
	govrh_CUR_set_framebuffer(p_cursor, fb);
}

void cursor_mod_set_addr(unsigned int yaddr, unsigned int caddr)
{
	govrh_CUR_set_fb_addr(p_cursor, yaddr, caddr);
}

void cursor_mod_get_addr(unsigned int *yaddr, unsigned int *caddr)
{
	govrh_CUR_get_fb_addr(p_cursor, yaddr, caddr);
}

void cursor_mod_fn_view(int read, vdo_view_t *view)
{
	govrh_CUR_proc_view(p_cursor, read, view);
}
#endif

#ifdef WMT_FTBLK_GOVRH2
void govrh2_mod_dump_reg(void)
{
	govrh_reg_dump((void *)p_govrh2);
}

void govrh2_mod_set_enable(vpp_flag_t enable)
{
	govrh_set_MIF_enable(p_govrh2, enable);
}

void govrh2_mod_set_colorbar(vpp_flag_t enable, int mode, int inv)
{
	govrh_set_colorbar(p_govrh2, enable, mode, inv);
}

void govrh2_mod_set_tg(vpp_clock_t *tmr, unsigned int pixel_clock)
{
	govrh_set_tg(p_govrh2, tmr, pixel_clock);
}

void govrh2_mod_get_tg(vpp_clock_t *tmr)
{
	govrh_get_tg(p_govrh2, tmr);
}

unsigned int govrh2_mod_get_sts(void)
{
	return govrh_get_int_status(p_govrh2);
}

void govrh2_mod_clr_sts(unsigned int sts)
{
	govrh_clean_int_status(p_govrh2, sts);
}

void govrh2_mod_suspend(int sts)
{
	govrh_suspend(p_govrh2, sts);
}

void govrh2_mod_resume(int sts)
{
	govrh_resume(p_govrh2, sts);
}

void govrh2_mod_set_framebuf(vdo_framebuf_t *fb)
{
	govrh_set_framebuffer(p_govrh2, fb);
}
void govrh2_mod_set_addr(unsigned int yaddr, unsigned int caddr)
{
	govrh_set_fb_addr(p_govrh2, yaddr, caddr);
}
void govrh2_mod_get_addr(unsigned int *yaddr, unsigned int *caddr)
{
	govrh_get_fb_addr(p_govrh2, yaddr, caddr);
}
void govrh2_mod_set_csc(vpp_csc_t mode)
{
	govrh_set_csc_mode(p_govrh2, mode);
}
vdo_color_fmt govrh2_mod_get_color_fmt(void)
{
	return govrh_get_color_format(p_govrh2);
}
void govrh2_mod_set_color_fmt(vdo_color_fmt colfmt)
{
	govrh_set_color_format(p_govrh2, colfmt);
}

#endif
int govrh_mod_init(void)
{
	govrh_mod_t *mod_p;
	vpp_fb_base_t *mod_fb_p;
	vdo_framebuf_t *fb_p;

	mod_p = (govrh_mod_t *) vpp_mod_register(VPP_MOD_GOVRH,
				sizeof(govrh_mod_t), VPP_MOD_FLAG_FRAMEBUF);
	if (!mod_p) {
		DPRINT("*E* GOVRH module register fail\n");
		return -1;
	}

	/* module member variable */
	mod_p->mmio = (void *)REG_GOVRH_BASE1_BEGIN;
	mod_p->int_catch = VPP_INT_NULL; /* VPP_INT_ERR_GOVRH_MIF; */

	/* module member function */
	mod_p->init = govrh_init;
	mod_p->dump_reg = govrh_mod_dump_reg;
	mod_p->set_enable = govrh_mod_set_enable;
	mod_p->set_colorbar = govrh_mod_set_colorbar;
	mod_p->set_tg = govrh_mod_set_tg;
	mod_p->get_tg = govrh_mod_get_tg;
	mod_p->get_sts = govrh_mod_get_sts;
	mod_p->clr_sts = govrh_mod_clr_sts;
	mod_p->suspend = govrh_mod_suspend;
	mod_p->resume = govrh_mod_resume;

	/* module frame buffer */
	mod_fb_p = mod_p->fb_p;
	mod_fb_p->csc_mode = VPP_CSC_RGB2YUV_SDTV_0_255;
	mod_fb_p->framerate = VPP_HD_DISP_FPS;
	mod_fb_p->media_fmt = VPP_MEDIA_FMT_MPEG;
	mod_fb_p->wait_ready = 0;
	mod_fb_p->capability = BIT(VDO_COL_FMT_YUV420)
			| BIT(VDO_COL_FMT_YUV422H)
			| BIT(VDO_COL_FMT_YUV444) | BIT(VDO_COL_FMT_ARGB)
			| BIT(VDO_COL_FMT_RGB_1555) | BIT(VDO_COL_FMT_RGB_565)
			| VPP_FB_FLAG_CSC | VPP_FB_FLAG_MEDIA
			| VPP_FB_FLAG_FIELD;

	/* module frame buffer member function */
	mod_fb_p->set_framebuf = govrh_mod_set_framebuf;
	mod_fb_p->set_addr = govrh_mod_set_addr;
	mod_fb_p->get_addr = govrh_mod_get_addr;
	mod_fb_p->set_csc = govrh_mod_set_csc;
	mod_fb_p->get_color_fmt = govrh_mod_get_color_fmt;
	mod_fb_p->set_color_fmt = govrh_mod_set_color_fmt;

	fb_p = &mod_p->fb_p->fb;
	fb_p->y_addr = 0;
	fb_p->c_addr = 0;
	fb_p->col_fmt = VPP_UBOOT_COLFMT;
	fb_p->img_w = VPP_HD_DISP_RESX;
	fb_p->img_h = VPP_HD_DISP_RESY;
	fb_p->fb_w = VPP_HD_MAX_RESX;
	fb_p->fb_h = VPP_HD_MAX_RESY;
	fb_p->h_crop = 0;
	fb_p->v_crop = 0;
	fb_p->flag = 0;

	p_govrh = mod_p;

#ifdef WMT_FTBLK_GOVRH_CURSOR
	mod_p = (govrh_mod_t *) vpp_mod_register(VPP_MOD_CURSOR,
			sizeof(govrh_cursor_mod_t), VPP_MOD_FLAG_FRAMEBUF);
	if (!mod_p) {
		DPRINT("*E* CURSOR module register fail\n");
		return -1;
	}

	/* module member function */
	mod_p->init = govrh_CUR_init;
	mod_p->set_enable = cursor_mod_set_enable;
	mod_p->mmio = (void *)REG_GOVRH_BASE1_BEGIN;

	/* module frame buffer */
	mod_fb_p = mod_p->fb_p;
	mod_fb_p->csc_mode = VPP_CSC_RGB2YUV_SDTV_0_255;
	mod_fb_p->framerate = VPP_HD_DISP_FPS;
	mod_fb_p->media_fmt = VPP_MEDIA_FMT_MPEG;
	mod_fb_p->wait_ready = 0;
	mod_fb_p->capability = BIT(VDO_COL_FMT_YUV420)
			| BIT(VDO_COL_FMT_YUV422H)
			| BIT(VDO_COL_FMT_YUV444) | BIT(VDO_COL_FMT_ARGB)
			| VPP_FB_FLAG_CSC | VPP_FB_FLAG_MEDIA
			| VPP_FB_FLAG_FIELD;

	/* module frame buffer member function */
	mod_fb_p->set_framebuf = cursor_mod_set_framebuf;
	mod_fb_p->set_addr = cursor_mod_set_addr;
	mod_fb_p->get_addr = cursor_mod_get_addr;
	mod_fb_p->fn_view = cursor_mod_fn_view;

	fb_p = &mod_p->fb_p->fb;
	fb_p->y_addr = 0;
	fb_p->c_addr = 0;
	fb_p->col_fmt = VDO_COL_FMT_YUV444;
	fb_p->img_w = VPP_HD_DISP_RESX;
	fb_p->img_h = VPP_HD_DISP_RESY;
	fb_p->fb_w = VPP_HD_MAX_RESX;
	fb_p->fb_h = VPP_HD_MAX_RESY;
	fb_p->h_crop = 0;
	fb_p->v_crop = 0;
	fb_p->flag = 0;

	p_cursor = (govrh_cursor_mod_t *) mod_p;
#endif

#ifdef WMT_FTBLK_GOVRH2
	mod_p = (govrh_mod_t *) vpp_mod_register(VPP_MOD_GOVRH2,
				sizeof(govrh_mod_t), VPP_MOD_FLAG_FRAMEBUF);
	if (!mod_p) {
		DPRINT("*E* GOVRH module register fail\n");
		return -1;
	}

	/* module member variable */
	mod_p->mmio = (void *)REG_GOVRH2_BASE1_BEGIN;
	mod_p->int_catch = VPP_INT_NULL; /* VPP_INT_ERR_GOVRH_MIF; */

	/* module member function */
	mod_p->init = govrh_init;
	mod_p->dump_reg = govrh2_mod_dump_reg;
	mod_p->set_enable = govrh2_mod_set_enable;
	mod_p->set_colorbar = govrh2_mod_set_colorbar;
	mod_p->set_tg = govrh2_mod_set_tg;
	mod_p->get_tg = govrh2_mod_get_tg;
	mod_p->get_sts = govrh2_mod_get_sts;
	mod_p->clr_sts = govrh2_mod_clr_sts;
	mod_p->suspend = govrh2_mod_suspend;
	mod_p->resume = govrh2_mod_resume;

	/* module frame buffer */
	mod_fb_p = mod_p->fb_p;
	mod_fb_p->csc_mode = VPP_CSC_RGB2YUV_SDTV_0_255;
	mod_fb_p->framerate = VPP_HD_DISP_FPS;
	mod_fb_p->media_fmt = VPP_MEDIA_FMT_MPEG;
	mod_fb_p->wait_ready = 0;
	mod_fb_p->capability = BIT(VDO_COL_FMT_YUV420)
		| BIT(VDO_COL_FMT_YUV422H)
		| BIT(VDO_COL_FMT_YUV444) | BIT(VDO_COL_FMT_ARGB)
		| BIT(VDO_COL_FMT_RGB_1555) | BIT(VDO_COL_FMT_RGB_565)
		| VPP_FB_FLAG_CSC | VPP_FB_FLAG_MEDIA | VPP_FB_FLAG_FIELD;

	/* module frame buffer member function */
	mod_fb_p->set_framebuf = govrh2_mod_set_framebuf;
	mod_fb_p->set_addr = govrh2_mod_set_addr;
	mod_fb_p->get_addr = govrh2_mod_get_addr;
	mod_fb_p->set_csc = govrh2_mod_set_csc;
	mod_fb_p->get_color_fmt = govrh2_mod_get_color_fmt;
	mod_fb_p->set_color_fmt = govrh2_mod_set_color_fmt;

	fb_p = &mod_p->fb_p->fb;
	fb_p->y_addr = 0;
	fb_p->c_addr = 0;
	fb_p->col_fmt = VPP_UBOOT_COLFMT;
	fb_p->img_w = VPP_HD_DISP_RESX;
	fb_p->img_h = VPP_HD_DISP_RESY;
	fb_p->fb_w = VPP_HD_MAX_RESX;
	fb_p->fb_h = VPP_HD_MAX_RESY;
	fb_p->h_crop = 0;
	fb_p->v_crop = 0;
	fb_p->flag = 0;

	p_govrh2 = mod_p;
#endif
	return 0;
}
module_init(govrh_mod_init);
#endif /* WMT_FTBLK_GOVRH */
