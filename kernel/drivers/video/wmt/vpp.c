/*++
 * linux/drivers/video/wmt/vpp.c
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

#define VPP_C
#undef DEBUG
/* #define DEBUG */
/* #define DEBUG_DETAIL */

#include "vpp.h"

vpp_mod_base_t *vpp_mod_base_list[VPP_MOD_MAX];

unsigned int vpp_get_chipid(void)
{
	/* byte 3,2: chip id, byte 1:ver id, byte 0:sub id */
	/* ex: 0x34290101 (0x3429 A0), 0x34290102 (0x3429 A1) */
	return REG32_VAL(SYSTEM_CFG_CTRL_BASE_ADDR);
}

__inline__ void vpp_cache_sync(void)
{
	/* TODO */
}

void vpp_set_clock_enable(enum dev_id dev, int enable, int force)
{
#ifdef CONFIG_VPP_DISABLE_PM
	return;
#else
	int cnt;

	do {
		cnt = auto_pll_divisor(dev,
			(enable) ? CLK_ENABLE : CLK_DISABLE, 0, 0);
		if (enable) {
			if (cnt)
				break;
		} else {
			if (cnt == 0)
				break;
		}
	} while (force);
/*	MSG("%s(%d,%d,%d)\n", __FUNCTION__, dev, enable, cnt); */
#endif
}

/*----------------------- vpp module --------------------------------------*/
void vpp_mod_unregister(vpp_mod_t mod)
{
	vpp_mod_base_t *mod_p;

	if (mod >= VPP_MOD_MAX)
		return;

	mod_p = vpp_mod_base_list[mod];
	if (!mod_p)
		return;

	kfree(mod_p->fb_p);
	kfree(mod_p);
	vpp_mod_base_list[mod] = 0;
}

vpp_mod_base_t *vpp_mod_register(vpp_mod_t mod,
					int size, unsigned int flags)
{
	vpp_mod_base_t *mod_p;

	if (mod >= VPP_MOD_MAX)
		return 0;

	if (vpp_mod_base_list[mod])
		vpp_mod_unregister(mod);

	mod_p = kmalloc(size, GFP_KERNEL);
	if (!mod_p)
		return 0;

	vpp_mod_base_list[mod] = mod_p;
	memset(mod_p, 0, size);
	mod_p->mod = mod;

	if (flags & VPP_MOD_FLAG_FRAMEBUF) {
		mod_p->fb_p = kmalloc(sizeof(vpp_fb_base_t), GFP_KERNEL);
		if (!mod_p->fb_p)
			goto error;
		memset(mod_p->fb_p, 0, sizeof(vpp_fb_base_t));
	}
	DBG_DETAIL(" %d,0x%x,0x%x\n", mod, (int)mod_p, (int)mod_p->fb_p);
	return mod_p;
error:
	vpp_mod_unregister(mod);
	DPRINT("vpp mod register NG %d\n", mod);
	return 0;
}

vpp_mod_base_t *vpp_mod_get_base(vpp_mod_t mod)
{
	if (mod >= VPP_MOD_MAX)
		return 0;
	return vpp_mod_base_list[mod];
}

vpp_fb_base_t *vpp_mod_get_fb_base(vpp_mod_t mod)
{
	vpp_mod_base_t *mod_p;
	mod_p = vpp_mod_get_base(mod);
	if (mod_p)
		return mod_p->fb_p;
	return 0;
}

vdo_framebuf_t *vpp_mod_get_framebuf(vpp_mod_t mod)
{
	vpp_mod_base_t *mod_p;

	mod_p = vpp_mod_get_base(mod);
	if (mod_p && mod_p->fb_p)
		return &mod_p->fb_p->fb;
	return 0;
}

void vpp_mod_set_clock(vpp_mod_t mod, vpp_flag_t enable, int force)
{
	vpp_mod_base_t *base;
	enum dev_id pll_dev;
	int cur_sts;
	int ret;

#ifdef CONFIG_VPP_DISABLE_PM
	return;
#endif

	base = vpp_mod_get_base(mod);
	if (base == 0)
		return;

	pll_dev = (base->pm & 0xFF);
	if (pll_dev == 0)
		return;

	enable = (enable) ? VPP_FLAG_ENABLE : VPP_FLAG_DISABLE;
	if (force) {
		ret = auto_pll_divisor(pll_dev,
			(enable) ? CLK_ENABLE : CLK_DISABLE, 0, 0);
		DBG_DETAIL("[VPP] clk force(%s,%d),ret %d\n",
			vpp_mod_str[mod], enable, ret);
		return;
	}

	cur_sts = (base->pm & VPP_MOD_CLK_ON) ? 1 : 0;
	if (cur_sts != enable) {
		ret = auto_pll_divisor(pll_dev,
			(enable) ? CLK_ENABLE : CLK_DISABLE, 0, 0);
		base->pm = (enable) ? (base->pm | VPP_MOD_CLK_ON)
			: (base->pm & ~VPP_MOD_CLK_ON);
		DBG_MSG("[VPP] clk enable(%s,%d,cur %d),ret %d\n",
			vpp_mod_str[mod], enable, cur_sts, ret);
	}
}

vpp_display_format_t vpp_get_fb_field(vdo_framebuf_t *fb)
{
	if (fb->flag & VDO_FLAG_INTERLACE)
		return VPP_DISP_FMT_FIELD;
	return VPP_DISP_FMT_FRAME;
}

unsigned int vpp_get_base_clock(vpp_mod_t mod)
{
	unsigned int clock = 0;

	switch (mod) {
	default:
		clock = auto_pll_divisor(DEV_VPP, GET_FREQ, 0, 0);
		break;
	case VPP_MOD_GOVRH:
		clock = (p_govrh->vo_clock == 0) ?
			auto_pll_divisor(DEV_HDMILVDS, GET_FREQ, 0, 0) :
			p_govrh->vo_clock;
		break;
	case VPP_MOD_GOVRH2:
		clock = (p_govrh2->vo_clock == 0) ?
			auto_pll_divisor(DEV_DVO, GET_FREQ, 0, 0) :
			p_govrh2->vo_clock;
		break;
	}
	DBG_DETAIL("%d %d\n", mod, clock);
	return clock;
}

#if 1
void vpp_show_timing(char *str, struct fb_videomode *vmode,
					vpp_clock_t *clk)
{
	DPRINT("----- %s timing -----\n", str);
	if (vmode) {
		DPRINT("res(%d,%d),fps %d\n",
			vmode->xres, vmode->yres, vmode->refresh);
		DPRINT("pixclk %d(%d),hsync %d,vsync %d\n", vmode->pixclock,
			(int)(PICOS2KHZ(vmode->pixclock) * 1000),
			vmode->hsync_len, vmode->vsync_len);
		DPRINT("left %d,right %d,upper %d,lower %d\n",
			vmode->left_margin, vmode->right_margin,
			vmode->upper_margin, vmode->lower_margin);
		DPRINT("vmode 0x%x,sync 0x%x\n", vmode->vmode, vmode->sync);
	}

	if (clk) {
		DPRINT("H beg %d,end %d,total %d\n", clk->begin_pixel_of_active,
			clk->end_pixel_of_active, clk->total_pixel_of_line);
		DPRINT("V beg %d,end %d,total %d\n", clk->begin_line_of_active,
			clk->end_line_of_active, clk->total_line_of_frame);
		DPRINT("Hsync %d, Vsync %d\n", clk->hsync, clk->vsync);
		DPRINT("VBIE %d,PVBI %d\n", clk->line_number_between_VBIS_VBIE,
			clk->line_number_between_PVBI_VBIS);
	}
	DPRINT("-----------------------\n");
}

void vpp_show_framebuf(char *str, vdo_framebuf_t *fb)
{
	if (fb == 0)
		return;
	DPRINT("----- %s framebuf -----\n", str);
	DPRINT("Y addr 0x%x, size %d\n", fb->y_addr, fb->y_size);
	DPRINT("C addr 0x%x, size %d\n", fb->c_addr, fb->c_size);
	DPRINT("W %d, H %d, FB W %d, H %d\n", fb->img_w, fb->img_h,
						fb->fb_w, fb->fb_h);
	DPRINT("bpp %d, color fmt %s\n", fb->bpp, vpp_colfmt_str[fb->col_fmt]);
	DPRINT("H crop %d, V crop %d, flag 0x%x\n",
		fb->h_crop, fb->v_crop, fb->flag);
	DPRINT("-----------------------\n");
}

void vpp_show_videomode(char *str, struct fb_videomode *v)
{
	if (v == 0)
		return;
	DPRINT("----- %s videomode -----\n", str);
	DPRINT("%dx%d@%d,%d\n", v->xres, v->yres, v->refresh, v->pixclock);
	DPRINT("h sync %d,bp %d,fp %d\n", v->hsync_len,
		v->left_margin, v->right_margin);
	DPRINT("v sync %d,bp %d,fp %d\n", v->vsync_len,
		v->upper_margin, v->lower_margin);
	DPRINT("sync 0x%x,vmode 0x%x,flag 0x%x\n", v->sync, v->vmode, v->flag);
	DPRINT("hsync %s,vsync %s\n",
		(v->sync & FB_SYNC_HOR_HIGH_ACT) ? "hi" : "lo",
		(v->sync & FB_SYNC_VERT_HIGH_ACT) ? "hi" : "lo");
	DPRINT("interlace %d,double %d\n",
		(v->vmode & FB_VMODE_INTERLACED) ? 1 : 0,
		(v->vmode & FB_VMODE_DOUBLE) ? 1 : 0);
	DPRINT("-----------------------\n");
}
#endif

vpp_csc_t vpp_check_csc_mode(vpp_csc_t mode, vdo_color_fmt src_fmt,
				vdo_color_fmt dst_fmt, unsigned int flags)
{
	if (mode >= VPP_CSC_MAX)
		return VPP_CSC_BYPASS;

	mode = (mode >= VPP_CSC_RGB2YUV_MIN) ?
		(mode - VPP_CSC_RGB2YUV_MIN) : mode;
	if (src_fmt >= VDO_COL_FMT_ARGB) {
		mode = VPP_CSC_RGB2YUV_MIN + mode;
		src_fmt = VDO_COL_FMT_ARGB;
	} else {
		src_fmt = VDO_COL_FMT_YUV444;
	}
	dst_fmt = (dst_fmt >= VDO_COL_FMT_ARGB) ?
		VDO_COL_FMT_ARGB : VDO_COL_FMT_YUV444;
	if (flags == 0)
		mode = (src_fmt != dst_fmt) ? mode : VPP_CSC_BYPASS;
	return mode;
}

int vpp_get_gcd(int A, int B)
{
	while (A != B) {
		if (A > B)
			A = A - B;
		else
			B = B - A;
	}
	return A;
}

int vpp_set_recursive_scale(vdo_framebuf_t *src_fb,
					vdo_framebuf_t *dst_fb)
{
#ifdef WMT_FTBLK_SCL
	int ret;

	ret = p_scl->scale(src_fb, dst_fb);
	return ret;
#else
	DBG_ERR("No scale\n");
	return 0;
#endif
}

void vpp_reg_dump(unsigned int addr, int size)
{
	int i;

	for (i = 0; i < size; i += 16) {
		DPRINT("0x%8x : 0x%08x 0x%08x 0x%08x 0x%08x\n",
			addr + i, vppif_reg32_in(addr + i),
			vppif_reg32_in(addr + i + 4),
			vppif_reg32_in(addr + i + 8),
			vppif_reg32_in(addr + i + 12));
	}
} /* End of vpp_reg_dump */

unsigned int vpp_convert_colfmt(int yuv2rgb, unsigned int data)
{
	unsigned int r, g, b;
	unsigned int y, u, v;
	unsigned int alpha;

	alpha = data & 0xff000000;
	if (yuv2rgb) {
		y = (data & 0xff0000) >> 16;
		u = (data & 0xff00) >> 8;
		v = (data & 0xff) >> 0;

		r = ((1000 * y) + 1402 * (v - 128)) / 1000;
		if (r > 0xFF)
			r = 0xFF;
		g = ((100000 * y) - (71414 * (v - 128))
			- (34414 * (u - 128))) / 100000;
		if (g > 0xFF)
			g = 0xFF;
		b = ((1000 * y) + (1772 * (u - 128))) / 1000;
		if (b > 0xFF)
			b = 0xFF;

		data = ((r << 16) + (g << 8) + b);
	} else {
		r = (data & 0xff0000) >> 16;
		g = (data & 0xff00) >> 8;
		b = (data & 0xff) >> 0;

		y = ((2990 * r) + (5870 * g) + (1440 * b)) / 10000;
		if (y > 0xFF)
			y = 0xFF;
		u = (1280000 - (1687 * r) - (3313 * g) + (5000 * b)) / 10000;
		if (u > 0xFF)
			u = 0xFF;
		v = (1280000 + (5000 * r) - (4187 * g) - (813 * b)) / 10000;
		if (v > 0xFF)
			v = 0xFF;

		data = ((y << 16) + (v << 8) + u);
	}
	data = data + alpha;
	return data;
}

unsigned int *vpp_backup_reg(unsigned int addr, unsigned int size)
{
	unsigned int *ptr;
	int i;

	size += 4;
	ptr = kmalloc(size, GFP_KERNEL);
	if (ptr == 0) {
		DPRINT("[VPP] *E* malloc backup fail\n");
		return 0;
	}

	for (i = 0; i < size; i += 4)
		ptr[i / 4] = REG32_VAL(addr + i);
	return ptr;
} /* End of vpp_backup_reg */

int vpp_restore_reg(unsigned int addr, unsigned int size,
					unsigned int *reg_ptr)
{
	int i;

	if (reg_ptr == NULL)
		return 0;

	size += 4;
	for (i = 0; i < size; i += 4)
		REG32_VAL(addr + i) = reg_ptr[i / 4];
	kfree(reg_ptr);
	reg_ptr = 0;
	return 0;
} /* End of vpp_restore_reg */

void vpp_get_sys_parameter(void)
{
#ifndef CONFIG_VPOST
	char buf[40];
	int varlen = 40;
#else
	struct env_para_def param;
#endif

	/* vpp attribute by default */
	g_vpp.dbg_msg_level = 0;
	g_vpp.hdmi_audio_interface = VPP_HDMI_AUDIO_SPDIF;
	g_vpp.hdmi_cp_enable = 1;

#if 0
	if (wmt_getsyspara("wmt.display.direct_path", buf, &varlen) == 0) {
		sscanf(buf, "%d", &g_vpp.direct_path);
		DPRINT("[VPP] direct path %d\n", g_vpp.direct_path);
	}
#endif

#ifndef CONFIG_VPOST
	if (wmt_getsyspara("wmt.display.hdmi_audio_inf", buf, &varlen) == 0) {
		if (memcmp(buf, "i2s", 3) == 0)
			g_vpp.hdmi_audio_interface = VPP_HDMI_AUDIO_I2S;
		else if (memcmp(buf, "spdif", 5) == 0)
			g_vpp.hdmi_audio_interface = VPP_HDMI_AUDIO_SPDIF;
	}

	if (wmt_getsyspara("wmt.display.hdmi.vmode", buf, &varlen) == 0) {
		if (memcmp(buf, "720p", 4) == 0)
			g_vpp.hdmi_video_mode = 720;
		else if (memcmp(buf, "1080p", 5) == 0)
			g_vpp.hdmi_video_mode = 1080;
		else
			g_vpp.hdmi_video_mode = 0;
		DPRINT("[VPP] HDMI video mode %d\n", g_vpp.hdmi_video_mode);
	}

	g_vpp.mb_colfmt = VPP_UBOOT_COLFMT;
	/* [uboot parameter] fb param : no:xresx:yres:xoffset:yoffset */
	if (wmt_getsyspara("wmt.gralloc.param", buf, &varlen) == 0) {
		unsigned int parm[1];

		vpp_parse_param(buf, (unsigned int *)parm, 1, 0x1);
		if (parm[0] == 32)
			g_vpp.mb_colfmt = VDO_COL_FMT_ARGB;
		MSG("mb colfmt : %s,%s\n", buf,
			vpp_colfmt_str[g_vpp.mb_colfmt]);
	}
	p_govrh->fb_p->fb.col_fmt = g_vpp.mb_colfmt;
	p_govrh2->fb_p->fb.col_fmt = g_vpp.mb_colfmt;

	/* [uboot parameter] dual display : 0-single display, 1-dual display */
	g_vpp.dual_display = 1;
	if (wmt_getsyspara("wmt.display.dual", buf, &varlen) == 0) {
		unsigned int parm[1];

		MSG("display dual : %s\n", buf);
		vpp_parse_param(buf, (unsigned int *)parm, 1, 0);
		g_vpp.dual_display = parm[0];
	}

	if (g_vpp.dual_display == 0)
		g_vpp.alloc_framebuf = 0;

	if (wmt_getsyspara("wmt.display.hdmi", buf, &varlen) == 0) {
		unsigned int parm[1];

		MSG("hdmi sp mode : %s\n", buf);
		vpp_parse_param(buf, (unsigned int *)parm, 1, 0);
		g_vpp.hdmi_sp_mode = (parm[0]) ? 1 : 0;
	}

	if (wmt_getsyspara("wmt.hdmi.disable", buf, &varlen) == 0)
		g_vpp.hdmi_disable = 1;
#else
	if (env_read_para("wmt.display.hdmi", &param) == 0) {
		g_vpp.hdmi_sp_mode = strtoul(param.value, 0, 16);
		free(param.value);
	}
#endif
} /* End of vpp_get_sys_parameter */

void vpp_init(void)
{
	vpp_mod_base_t *mod_p;
	unsigned int mod_mask;
	int i;
	int no;

	vpp_get_sys_parameter();

	auto_pll_divisor(DEV_NA12, CLK_ENABLE, 0, 0);
	auto_pll_divisor(DEV_VPP, CLK_ENABLE, 0, 0);
	auto_pll_divisor(DEV_HDCE, CLK_ENABLE, 0, 0);
	auto_pll_divisor(DEV_HDMII2C, CLK_ENABLE, 0, 0);
	auto_pll_divisor(DEV_HDMI, CLK_ENABLE, 0, 0);
	auto_pll_divisor(DEV_GOVRHD, CLK_ENABLE, 0, 0);
	auto_pll_divisor(DEV_DVO, CLK_ENABLE, 0, 0);
	auto_pll_divisor(DEV_LVDS, CLK_ENABLE, 0, 0);
	auto_pll_divisor(DEV_HDMILVDS, CLK_ENABLE, 0, 0);
	auto_pll_divisor(DEV_SCL444U, CLK_ENABLE, 0, 0);
#ifdef CONFIG_KERNEL
	if (1) {
		if (govrh_get_MIF_enable(p_govrh))
			g_vpp.govrh_preinit = 1;
		if (govrh_get_MIF_enable(p_govrh2))
			g_vpp.govrh_preinit = 1;
		MSG("[VPP] govrh preinit %d\n", g_vpp.govrh_preinit);
	}
#endif

#ifndef CONFIG_VPP_DYNAMIC_ALLOC
	if (g_vpp.alloc_framebuf)
		g_vpp.alloc_framebuf(VPP_HD_MAX_RESX, VPP_HD_MAX_RESY);
#endif

	/* init video out module first */
	if (g_vpp.govrh_preinit == 0) {
		mod_mask = BIT(VPP_MOD_GOVRH2) | BIT(VPP_MOD_GOVRH)
			| BIT(VPP_MOD_DISP) | BIT(VPP_MOD_LCDC);
		for (i = 0; i < VPP_MOD_MAX; i++) {
			if (!(mod_mask & (0x01 << i)))
				continue;
			mod_p = vpp_mod_get_base(i);
			if (mod_p && mod_p->init)
				mod_p->init(mod_p);
		}
	}

#ifndef CONFIG_UBOOT
	/* init other module */
	mod_mask =  BIT(VPP_MOD_GOVW) | BIT(VPP_MOD_GOVM) | BIT(VPP_MOD_SCL)
		| BIT(VPP_MOD_SCLW) | BIT(VPP_MOD_VPU) | BIT(VPP_MOD_VPUW)
		| BIT(VPP_MOD_PIP) | BIT(VPP_MOD_VPPM);
	for (i = 0; i < VPP_MOD_MAX; i++) {
		if (!(mod_mask & (0x01 << i)))
			continue;
		mod_p = vpp_mod_get_base(i);
		if (mod_p && mod_p->init)
			mod_p->init(mod_p);
	}
#endif

#ifdef WMT_FTBLK_LVDS
	if (!g_vpp.govrh_preinit)
		lvds_init();
#endif
#ifdef WMT_FTBLK_VOUT_HDMI
	hdmi_init();
#endif

	vpp_set_clock_enable(DEV_SCL444U, 0, 1);

#ifndef CONFIG_VPOST
	/* init vout device & get default resolution */
	vout_init();
#endif

#ifdef CONFIG_KERNEL
	no = (g_vpp.virtual_display_mode == 1) ? 1 : 0;
	if (g_vpp.govrh_preinit) {
		struct fb_videomode vmode;
		vout_info_t *info;
		govrh_mod_t *govr;

		info = vout_get_info_entry(no);
		govr = vout_info_get_govr(no);
		govrh_get_videomode(govr, &vmode);
		govrh_get_framebuffer(govr, &info->fb);
		g_vpp.govrh_init_yres = vmode.yres;
		if ((info->resx != vmode.xres) || (info->resy != vmode.yres)) {
			g_vpp.govrh_preinit = 0;
			DPRINT("preinit not match (%dx%d) --> (%dx%d)\n",
				vmode.xres, vmode.yres, info->resx, info->resy);
			if (g_vpp.virtual_display || (g_vpp.dual_display == 0)) {
				if(!hdmi_get_plugin()) {
					vout_t *vo = vout_get_entry(VPP_VOUT_NUM_DVI);
					if(vo->dev && !strcmp(vo->dev->name, "CS8556") && vo->dev->init) {
						vo->dev->init(vo);
					}
				}
			}
		}
	}

	if (!g_vpp.govrh_preinit) {
		struct fb_videomode vmode;
		vout_info_t *info;

		info = vout_get_info_entry(no);
		memset(&vmode, 0, sizeof(struct fb_videomode));
		vmode.xres = info->resx;
		vmode.yres = info->resy;
		vmode.refresh = info->fps;
		if (vout_find_match_mode(no, &vmode, 1) == 0)
			vout_config(VPP_VOUT_ALL, info, &vmode);
	}
#endif
	vpp_set_clock_enable(DEV_HDMII2C, 0, 0);
	vpp_set_clock_enable(DEV_HDMI, 0, 0);
	vpp_set_clock_enable(DEV_HDCE, 0, 0);
	vpp_set_clock_enable(DEV_LVDS, 0, 0);
}

void vpp_get_colfmt_bpp(vdo_color_fmt colfmt, int *y_bpp, int *c_bpp)
{
	switch (colfmt) {
	case VDO_COL_FMT_YUV420:
		*y_bpp = 8;
		*c_bpp = 4;
		break;
	case VDO_COL_FMT_YUV422H:
	case VDO_COL_FMT_YUV422V:
		*y_bpp = 8;
		*c_bpp = 8;
		break;
	case VDO_COL_FMT_RGB_565:
	case VDO_COL_FMT_RGB_1555:
	case VDO_COL_FMT_RGB_5551:
		*y_bpp = 16;
		*c_bpp = 0;
		break;
	case VDO_COL_FMT_YUV444:
		*y_bpp = 8;
		*c_bpp = 16;
		break;
	case VDO_COL_FMT_RGB_888:
	case VDO_COL_FMT_RGB_666:
		*y_bpp = 24;
		*c_bpp = 0;
		break;
	case VDO_COL_FMT_ARGB:
		*y_bpp = 32;
		*c_bpp = 0;
		break;
	default:
		break;
	}
}

int vpp_calc_refresh(int pixclk, int xres, int yres)
{
	int refresh = 60;
	int temp;

	temp = xres * yres;
	if (temp) {
		refresh = pixclk / temp;
		if (pixclk % temp)
			refresh += 1;
	}
	return refresh;
}

int vpp_calc_align(int value, int align)
{
	if (value % align) {
		value &= ~(align - 1);
		value += align;
	}
	return value;
}

int vpp_calc_fb_width(vdo_color_fmt colfmt, int width)
{
	int y_bpp, c_bpp;

	vpp_get_colfmt_bpp(colfmt, &y_bpp, &c_bpp);
	return vpp_calc_align(width, VPP_FB_WIDTH_ALIGN / (y_bpp / 8));
}

void vpp_set_NA12_hiprio(int type)
{
#if 0
	static int reg1, reg2;

	switch (type) {
	case 0: /* restore NA12 priority */
		vppif_reg32_out(MEMORY_CTRL_V4_CFG_BASE_ADDR + 0x8, reg1);
		vppif_reg32_out(MEMORY_CTRL_V4_CFG_BASE_ADDR + 0xC, reg2);
		break;
	case 1:	/* set NA12 to high priority */
		reg1 = vppif_reg32_in(MEMORY_CTRL_V4_CFG_BASE_ADDR + 0x8);
		reg2 = vppif_reg32_in(MEMORY_CTRL_V4_CFG_BASE_ADDR + 0xC);
		vppif_reg32_out(MEMORY_CTRL_V4_CFG_BASE_ADDR + 0x8, 0x600000);
		vppif_reg32_out(MEMORY_CTRL_V4_CFG_BASE_ADDR + 0xC, 0x0ff00000);
		break;
	case 2:
		reg1 = vppif_reg32_in(MEMORY_CTRL_V4_CFG_BASE_ADDR + 0x8);
		reg2 = vppif_reg32_in(MEMORY_CTRL_V4_CFG_BASE_ADDR + 0xC);
		vppif_reg32_out(MEMORY_CTRL_V4_CFG_BASE_ADDR + 0x8, 0x20003f);
		vppif_reg32_out(MEMORY_CTRL_V4_CFG_BASE_ADDR + 0xC, 0x00ffff00);
		break;
	default:
		break;
	}
#endif
}

#ifdef __KERNEL__
int vpp_set_audio(int format, int sample_rate, int channel)
{
	vout_audio_t info;

	DBG_MSG("set audio(fmt %d,rate %d,ch %d)\n",
		format, sample_rate, channel);
	info.fmt = format;
	info.sample_rate = sample_rate;
	info.channel = channel;
	return vout_set_audio(&info);
}

static DEFINE_SEMAPHORE(vpp_sem);
static DEFINE_SEMAPHORE(vpp_sem2);
void vpp_set_mutex(int idx, int lock)
{
	struct semaphore *sem;

	sem = ((g_vpp.dual_display == 0) || (idx == 0)) ? &vpp_sem : &vpp_sem2;
	if (lock)
		down(sem);
	else
		up(sem);
}

void vpp_free_framebuffer(void)
{
	if (g_vpp.mb[0] == 0)
		return;
	MSG("mb free 0x%x\n", g_vpp.mb[0]);
	mb_free(g_vpp.mb[0]);
	vpp_lock();
	g_vpp.mb[0] = 0;
	vpp_unlock();
}

int vpp_alloc_framebuffer(unsigned int resx, unsigned int resy)
{
	unsigned int y_size;
	unsigned int fb_size;
	unsigned int colfmt;
	int y_bpp, c_bpp;
	int i;

#ifdef CONFIG_VPP_DYNAMIC_ALLOC
	if (g_vpp.mb[0]) {
		vpp_free_framebuffer();
	}
#endif

	if ((resx == 0) && (resy == 0)) {
		return -1;
	}

	/* alloc govw & govrh frame buffer */
	if (g_vpp.mb[0] == 0) {
		unsigned int mb_resx, mb_resy;
		int fb_num;
		unsigned int phy_base;

#ifdef CONFIG_VPP_DYNAMIC_ALLOC
		mb_resx = resx;
		mb_resy = resy;
		colfmt = g_vpp.mb_colfmt;
		vpp_get_colfmt_bpp(colfmt, &y_bpp, &c_bpp);
		fb_num = VPP_MB_ALLOC_NUM;
#else
		char buf[100];
		int varlen = 100;

		if (wmt_getsyspara("wmt.display.mb", (unsigned char *)buf,
			&varlen) == 0) {
			unsigned int parm[10];

			vpp_parse_param(buf, (unsigned int *)parm, 4, 0);
			MSG("boot parm mb (%d,%d),bpp %d,fb %d\n",
					parm[0], parm[1], parm[2], parm[3]);
			mb_resx = parm[0];
			mb_resy = parm[1];
			y_bpp = parm[2] * 8;
			c_bpp = 0;
			fb_num = parm[3];
		} else {
			mb_resx = VPP_HD_MAX_RESX;
			mb_resy = VPP_HD_MAX_RESY;
			colfmt = g_vpp.mb_colfmt;
			vpp_get_colfmt_bpp(colfmt, &y_bpp, &c_bpp);
			fb_num = VPP_MB_ALLOC_NUM;
		}
#endif
		mb_resx = vpp_calc_align(mb_resx,
				VPP_FB_ADDR_ALIGN / (y_bpp / 8));
		y_size = mb_resx * mb_resy * y_bpp / 8;
		fb_size = mb_resx * mb_resy * (y_bpp + c_bpp) / 8;
		g_vpp.mb_fb_size = fb_size;
		g_vpp.mb_y_size = y_size;
		phy_base = mb_alloc(fb_size * fb_num);
		if (phy_base) {
			MSG("mb alloc 0x%x,%d\n", phy_base, fb_size * fb_num);
			for (i = 0; i < fb_num; i++) {
				g_vpp.mb[i] = (unsigned int)(phy_base +
					(fb_size * i));
				MSG("mb 0x%x,fb %d,y %d\n", g_vpp.mb[i],
					fb_size, y_size);
			}
		} else {
			DBG_ERR("alloc fail\n");
			return -1;
		}
		if (!g_vpp.govrh_preinit) { /* keep uboot logo */
			memset(mb_phys_to_virt(phy_base), 0, fb_size);
			MSG("mb clean 0x%x %d\n", phy_base, fb_size);
		}
	}

	vpp_lock();
#ifdef CONFIG_VPP_STREAM_ROTATE
	{
		unsigned int resx_mb;

		g_vpp.stream_mb_cnt = VPP_MB_ALLOC_NUM;
		colfmt = VDO_COL_FMT_YUV422H;
		vpp_get_colfmt_bpp(colfmt, &y_bpp, &c_bpp);
		resx_mb = vpp_calc_fb_width(colfmt, resx);
		y_size = resx_mb * resy * y_bpp / 8;
		fb_size = vpp_calc_align(resx_mb, 64) * resy *
			(y_bpp + c_bpp) / 8;
		for (i = 0; i < g_vpp.stream_mb_cnt; i++) {
			g_vpp.stream_mb[i] = g_vpp.mb[0] + fb_size * i;
		}
	}
#else
	/* assign mb to stream mb */
	{
	int index = 0, offset = 0;
	unsigned int size = g_vpp.mb_fb_size;
	unsigned int resx_fb;

	colfmt = VPP_UBOOT_COLFMT;
	vpp_get_colfmt_bpp(colfmt, &y_bpp, &c_bpp);
	resx_fb = vpp_calc_fb_width(colfmt, resx);
	y_size = resx_fb * resy * y_bpp / 8;
	fb_size = vpp_calc_align(resx_fb, 64) * resy * (y_bpp + c_bpp) / 8;
	g_vpp.stream_mb_y_size = y_size;
	g_vpp.stream_mb_cnt = VPP_STREAM_MB_ALLOC_NUM;
	for (i = 0; i < VPP_STREAM_MB_ALLOC_NUM; i++) {
		if (size < fb_size) {
			index++;
			if (index >= VPP_MB_ALLOC_NUM) {
				index = 0;
				g_vpp.stream_mb_cnt = i;
				break;
			}
			offset = 0;
			size = g_vpp.mb_fb_size;
		}
		g_vpp.stream_mb[i] = g_vpp.mb[index] + offset;
		size -= fb_size;
		offset += fb_size;
		DBG_DETAIL("stream mb %d 0x%x\n", i, g_vpp.stream_mb[i]);
	}
	}
#endif
	vpp_unlock();
	return 0;
} /* End of vpp_alloc_framebuffer */

/*----------------------- vpp mb for stream ---------------------------------*/
#ifdef CONFIG_VPP_STREAM_CAPTURE
#ifdef CONFIG_VPP_STREAM_BLOCK
DECLARE_WAIT_QUEUE_HEAD(vpp_mb_event);
#endif

unsigned int vpp_mb_get_mask(unsigned int phy)
{
	int i;
	unsigned int mask;

	for (i = 0; i < g_vpp.stream_mb_cnt; i++) {
		if (g_vpp.stream_mb[i] == phy)
			break;
	}
	if (i >= g_vpp.stream_mb_cnt)
		return 0;
	mask = 0x1 << i;
	return mask;
}

int vpp_mb_get(unsigned int phy)
{
	unsigned int mask;
	int i, cnt;

#ifdef CONFIG_VPP_STREAM_BLOCK
	vpp_unlock();
	i = wait_event_interruptible(vpp_mb_event,
		(g_vpp.stream_mb_sync_flag != 1));
	vpp_lock();
	if (i)
		return -1;
#else /* non-block */
	if (g_vpp.stream_mb_sync_flag) { /* not new mb updated */
		vpp_dbg_show(VPP_DBGLVL_STREAM, 0,
			"*W* mb_get addr not update");
		return -1;
	}
#endif
	g_vpp.stream_mb_sync_flag = 1;
	for (i = 0, cnt = 0; i < g_vpp.stream_mb_cnt; i++) {
		if (g_vpp.stream_mb_lock & (0x1 << i))
			cnt++;
	}

#if 0
	if (cnt >= (g_vpp.stream_mb_cnt - 2)) {
		vpp_dbg_show(VPP_DBGLVL_STREAM, 0, "*W* mb_get addr not free");
		return -1;
	}
#endif

	mask = vpp_mb_get_mask(phy);
	if (mask == 0) {
		vpp_dbg_show(VPP_DBGLVL_STREAM, 0, "*W* mb_get invalid addr");
		return -1;
	}
	if (g_vpp.stream_mb_lock & mask) {
		vpp_dbg_show(VPP_DBGLVL_STREAM, 0, "*W* mb_get lock addr");
		return -1;
	}
	g_vpp.stream_mb_lock |= mask;
	if (vpp_check_dbg_level(VPP_DBGLVL_STREAM)) {
		char buf[50];

		sprintf(buf, "stream mb get 0x%x,mask 0x%x(0x%x)",
			phy, mask, g_vpp.stream_mb_lock);
		vpp_dbg_show(VPP_DBGLVL_STREAM, 1, buf);
	}
	return 0;
}

int vpp_mb_put(unsigned int phy)
{
	unsigned int mask;

	if (phy == 0) {
		g_vpp.stream_mb_lock = 0;
		g_vpp.stream_mb_index = 0;
		return 0;
	}

	mask = vpp_mb_get_mask(phy);
	if (mask == 0) {
		DPRINT("[VPP] *W* mb_put addr 0x%x\n", phy);
		return 1;
	}
	if (!(g_vpp.stream_mb_lock & mask))
		DPRINT("[VPP] *W* mb_put nonlock addr 0x%x\n", phy);
	g_vpp.stream_mb_lock &= ~mask;
	if (vpp_check_dbg_level(VPP_DBGLVL_STREAM)) {
		char buf[50];

		sprintf(buf, "stream mb put 0x%x,mask 0x%x(0x%x)",
			phy, mask, g_vpp.stream_mb_lock);
		vpp_dbg_show(VPP_DBGLVL_STREAM, 2, buf);
	}
	return 0;
}

int vpp_mb_irqproc_sync(int arg)
{
	if (!g_vpp.stream_enable)
		return 0;

	g_vpp.stream_sync_cnt++;
	if ((g_vpp.stream_sync_cnt % 2) == 0) {
		g_vpp.stream_mb_sync_flag = 0;
#ifdef CONFIG_VPP_STREAM_BLOCK
		wake_up_interruptible(&vpp_mb_event);
#endif
	}
	return 0;
}

void vpp_mb_scale_bitblit(vdo_framebuf_t *fb)
{
	int index = g_vpp.stream_mb_index;
	vdo_framebuf_t src, dst;

	if (p_scl->scale_complete == 0)
		return;

#ifdef CONFIG_VPP_STREAM_ROTATE
	index = g_vpp.stream_mb_index + 1;
	index = (index >= g_vpp.stream_mb_cnt) ? 0 : index;
#else
	do {
		index++;
		if (index >= g_vpp.stream_mb_cnt)
			index = 0;

		if (g_vpp.stream_mb_lock & (0x1 << index))
			continue;
		break;
	} while (1);
#endif

	g_vpp.stream_mb_index = index;
	p_scl->scale_sync = 1;
	src = *fb;
#ifdef CONFIG_VPP_STREAM_FIX_RESOLUTION
	dst = g_vpp.stream_fb;
#else
	dst = *fb;
	dst.col_fmt = VDO_COL_FMT_YUV422H;
	dst.fb_w = vpp_calc_align(dst.fb_w, 64);
#endif
	dst.y_addr = g_vpp.stream_mb[index];
	dst.c_addr = dst.y_addr + (dst.fb_w * dst.img_h);
	vpp_set_recursive_scale(&src, &dst);
}
#endif

/*----------------------- irq proc --------------------------------------*/
vpp_irqproc_t *vpp_irqproc_array[32];
struct list_head vpp_irqproc_free_list;
vpp_proc_t vpp_proc_array[VPP_PROC_NUM];
static void vpp_irqproc_do_tasklet(unsigned long data);

void vpp_irqproc_init(void)
{
	int i;

	INIT_LIST_HEAD(&vpp_irqproc_free_list);

	for (i = 0; i < VPP_PROC_NUM; i++)
		list_add_tail(&vpp_proc_array[i].list, &vpp_irqproc_free_list);
}

vpp_irqproc_t *vpp_irqproc_get_entry(vpp_int_t vpp_int)
{
	int no;

	if (vpp_int == 0)
		return 0;

	for (no = 0; no < 32; no++) {
		if (vpp_int & (0x1 << no))
			break;
	}

	if (vpp_irqproc_array[no] == 0) { /* will create in first use */
		vpp_irqproc_t *irqproc;

		irqproc = kmalloc(sizeof(vpp_irqproc_t), GFP_KERNEL);
		vpp_irqproc_array[no] = irqproc;
		INIT_LIST_HEAD(&irqproc->list);
		tasklet_init(&irqproc->tasklet,
			vpp_irqproc_do_tasklet, vpp_int);
		irqproc->ref = 0;
	}
	return vpp_irqproc_array[no];
} /* End of vpp_irqproc_get_entry */

void vpp_irqproc_set_ref(vpp_irqproc_t *irqproc,
				vpp_int_t type, int enable)
{
	if (enable) {
		irqproc->ref++;
		if (vppm_get_int_enable(type) == 0)
			vppm_set_int_enable(1, type);
	} else {
		irqproc->ref--;
		if (irqproc->ref == 0)
			vppm_set_int_enable(0, type);
	}
}

static void vpp_irqproc_do_tasklet
(
	unsigned long data		/*!<; // tasklet input data */
)
{
	vpp_irqproc_t *irqproc;

	vpp_lock();
	irqproc = vpp_irqproc_get_entry(data);
	if (irqproc) {
		struct list_head *cur;
		struct list_head *next;
		vpp_proc_t *entry;

		next = (&irqproc->list)->next;
		while (next != &irqproc->list) {
			cur = next;
			next = cur->next;
			entry = list_entry(cur, vpp_proc_t, list);
			if (entry->func) {
				if (entry->func(entry->arg))
					continue;
			}

			if (entry->work_cnt == 0)
				continue;

			entry->work_cnt--;
			if (entry->work_cnt == 0) {
				if (entry->wait_ms == 0)
					vpp_irqproc_set_ref(irqproc, data, 0);
				else
					up(&entry->sem);
				list_del_init(cur);
				list_add_tail(&entry->list,
					&vpp_irqproc_free_list);
			}
		}
	}
	vpp_unlock();
} /* End of vpp_irqproc_do_tasklet */

int vpp_irqproc_work(
	vpp_int_t type,             /* interrupt type */
	int (*func)(void *argc),    /* proc function pointer */
	void *arg,                  /* proc argument */
	int wait_ms,                /* wait complete timeout (ms) */
	int work_cnt                /* 0 - forever */
)
{
	int ret;
	vpp_proc_t *entry;
	struct list_head *ptr;
	vpp_irqproc_t *irqproc;

	/* DPRINT("[VPP] vpp_irqproc_work(type 0x%x,wait %d)\n",type,wait); */

	if ((vpp_irqproc_free_list.next == 0) ||
		list_empty(&vpp_irqproc_free_list)) {
		if (func)
			func(arg);
		return 0;
	}

	ret = 0;
	vpp_lock();

	ptr = vpp_irqproc_free_list.next;
	entry = list_entry(ptr, vpp_proc_t, list);
	list_del_init(ptr);
	entry->func = func;
	entry->arg = arg;
	entry->type = type;
	entry->wait_ms = wait_ms;
	entry->work_cnt = work_cnt;
	sema_init(&entry->sem, 1);
	down(&entry->sem);

	irqproc = vpp_irqproc_get_entry(type);
	if (irqproc) {
		list_add_tail(&entry->list, &irqproc->list);
	} else {
		irqproc = vpp_irqproc_array[31];
		list_add_tail(&entry->list, &irqproc->list);
	}
	vpp_irqproc_set_ref(irqproc, type, 1);
	vpp_unlock();

	if (wait_ms) {
		unsigned int tmr_cnt;

		tmr_cnt = (wait_ms * HZ) / 1000;
		ret = down_timeout(&entry->sem, tmr_cnt);
		if (ret) {
			DPRINT("*W* vpp_irqproc_work timeout(type 0x%x,%d)\n",
				type, wait_ms);
			vpp_lock();
			list_del_init(ptr);
			list_add_tail(ptr, &vpp_irqproc_free_list);
			vpp_unlock();
			if (func)
				func(arg);
		}
	}

	if ((work_cnt == 0) || (wait_ms == 0)) {
		/* don't clear ref, forever will do in delete,
		no wait will do in proc complete */
	} else {
		vpp_lock();
		vpp_irqproc_set_ref(irqproc, type, 0);
		vpp_unlock();
	}
	return ret;
} /* End of vpp_irqproc_work */

void vpp_irqproc_del_work(
	vpp_int_t type,         /* interrupt type */
	int (*func)(void *argc)	/* proc function pointer */
)
{
	vpp_irqproc_t *irqproc;

	vpp_lock();
	irqproc = vpp_irqproc_get_entry(type);
	if (irqproc) {
		struct list_head *cur;
		struct list_head *next;
		vpp_proc_t *entry;

		next = (&irqproc->list)->next;
		while (next != &irqproc->list) {
			cur = next;
			next = cur->next;
			entry = list_entry(cur, vpp_proc_t, list);
			if (entry->func == func) {
				vpp_irqproc_set_ref(irqproc, type, 0);
				list_del_init(cur);
				list_add_tail(&entry->list,
					&vpp_irqproc_free_list);
			}
		}
	}
	vpp_unlock();
}

/*----------------------- Linux Netlink --------------------------------------*/
#ifdef CONFIG_VPP_NOTIFY
#define VPP_NETLINK_PROC_MAX	2

struct vpp_netlink_proc_t {
	__u32 pid;
	rwlock_t lock;
};

struct switch_dev vpp_sdev = {
	.name = "hdmi",
};

static struct switch_dev vpp_sdev_hdcp = {
	.name = "hdcp",
};

#if 0
static struct switch_dev vpp_sdev_audio = {
	.name = "hdmi_audio",
};
#endif

struct vpp_netlink_proc_t vpp_netlink_proc[VPP_NETLINK_PROC_MAX];
static struct sock *vpp_nlfd;
static DEFINE_SEMAPHORE(vpp_netlink_receive_sem);

struct vpp_netlink_proc_t *vpp_netlink_get_proc(int no)
{
	if (no == 0)
		return 0;
	if (no > VPP_NETLINK_PROC_MAX)
		return 0;
	return &vpp_netlink_proc[no - 1];
}

static void vpp_netlink_receive(struct sk_buff *skb)
{
	struct nlmsghdr *nlh = NULL;
	struct vpp_netlink_proc_t *proc;

	if (down_trylock(&vpp_netlink_receive_sem))
		return;

	if (skb->len >= sizeof(struct nlmsghdr)) {
		nlh = nlmsg_hdr(skb);
		if ((nlh->nlmsg_len >= sizeof(struct nlmsghdr))
			&& (skb->len >= nlh->nlmsg_len)) {
			proc = vpp_netlink_get_proc(nlh->nlmsg_type);
			if (proc) {
				write_lock_bh(&proc->lock);
				proc->pid = nlh->nlmsg_pid;
				write_unlock_bh(&proc->lock);
				DPRINT("[VPP] rx user pid 0x%x\n", proc->pid);
			}
		}
	}
	up(&vpp_netlink_receive_sem);
	wmt_enable_mmfreq(WMT_MMFREQ_HDMI_PLUG, hdmi_get_plugin());
}

void vpp_netlink_init(void)
{
	vpp_netlink_proc[0].pid = 0;
	vpp_netlink_proc[1].pid = 0;
	rwlock_init(&(vpp_netlink_proc[0].lock));
	rwlock_init(&(vpp_netlink_proc[1].lock));
	vpp_nlfd = netlink_kernel_create(&init_net, NETLINK_CEC_TEST, 0,
		vpp_netlink_receive, NULL, THIS_MODULE);
	if (!vpp_nlfd)
		DPRINT(KERN_ERR "can not create a netlink socket\n");
}

static ssize_t attr_show_parsed_edid(struct device *dev,
			       struct device_attribute *attr, char *buf)
{
	ssize_t len = 0;
	int i, j;
	unsigned char audio_format, sample_freq, bitrate;
	int sample_freq_num, bitrate_num;

#ifdef DEBUG
	DPRINT("------- EDID Parsed ------\n");
	if(strlen(edid_parsed.tv_name.vendor_name) != 0)
		DPRINT("Vendor Name: %s\n", edid_parsed.tv_name.vendor_name);

	if(strlen(edid_parsed.tv_name.monitor_name) != 0)
		DPRINT("Monitor Name: %s\n", edid_parsed.tv_name.monitor_name);

	for(i = 0; i < AUD_SAD_NUM; i++) {
		if(edid_parsed.sad[i].flag == 0) {
			if(i == 0)
				printk("No SAD Data\n");
			break;
		}
		DPRINT("SAD %d: 0x%02X 0x%02X 0x%02X\n", i,
			edid_parsed.sad[i].sad_byte[0], edid_parsed.sad[i].sad_byte[1],
			edid_parsed.sad[i].sad_byte[2]);
	}
	DPRINT("--------------------------\n");
#endif
	/* print Vendor Name */
	if(strlen(edid_parsed.tv_name.vendor_name) != 0) {
		len += sprintf(buf + len, "%-16s", "Vendor Name");
		len += sprintf(buf + len, ": %s\n", edid_parsed.tv_name.vendor_name);
	}

	/* print Monitor Name */
	if(strlen(edid_parsed.tv_name.monitor_name) != 0) {
		len += sprintf(buf + len, "%-16s", "Monitor Name");
		len += sprintf(buf + len, ": %s\n", edid_parsed.tv_name.monitor_name);
	}

	for(i = 0; i < AUD_SAD_NUM; i++) {
		if(edid_parsed.sad[i].flag == 0)
			break;
		/*
		SAD Byte 1 (format and number of channels):
		   bit 7: Reserved (0)
		   bit 6..3: Audio format code
		     1 = Linear Pulse Code Modulation (LPCM)
		     2 = AC-3
		     3 = MPEG1 (Layers 1 and 2)
		     4 = MP3
		     5 = MPEG2
		     6 = AAC
		     7 = DTS
		     8 = ATRAC
		     0, 15: Reserved
		     9 = One-bit audio aka SACD
		    10 = DD+
		    11 = DTS-HD
		    12 = MLP/Dolby TrueHD
		    13 = DST Audio
		    14 = Microsoft WMA Pro
		   bit 2..0: number of channels minus 1  (i.e. 000 = 1 channel; 001 = 2 channels; 111 =
			     8 channels)
		*/
		audio_format = (edid_parsed.sad[i].sad_byte[0] & 0x78) >> 3;
		if(audio_format == 0 || audio_format == 15)
			continue;

		/* print header */
		len += sprintf(buf + len, "%-16s", "Audio Format");
		len += sprintf(buf + len, ": ");

		switch(audio_format) {
			case 1:
				len += sprintf(buf + len, "pcm");
			break;
			case 2:
				len += sprintf(buf + len, "ac3");
			break;
			case 3:
				len += sprintf(buf + len, "mpeg1");
			break;
			case 4:
				len += sprintf(buf + len, "mp3");
			break;
			case 5:
				len += sprintf(buf + len, "mpeg2");
			break;
			case 6:
				len += sprintf(buf + len, "aac");
			break;
			case 7:
				len += sprintf(buf + len, "dts");
			break;
			case 8:
				len += sprintf(buf + len, "atrac");
			break;
			case 9:
				len += sprintf(buf + len, "one_bit_audio");
			break;
			case 10:
				len += sprintf(buf + len, "eac3");
			break;
			case 11:
				len += sprintf(buf + len, "dts-hd");
			break;
			case 12:
				len += sprintf(buf + len, "mlp");
			break;
			case 13:
				len += sprintf(buf + len, "dst");
			break;
			case 14:
				len += sprintf(buf + len, "wmapro");
			break;
			default:
			break;
		}

		/* separator */
		len += sprintf(buf + len, ",");

		/* number of channels */
		len += sprintf(buf + len, "%d", (edid_parsed.sad[i].sad_byte[0] & 0x7) + 1);

		/* separator */
		len += sprintf(buf + len, ",");

		/*
		SAD Byte 2 (sampling frequencies supported):
		   bit 7: Reserved (0)
		   bit 6: 192kHz
		   bit 5: 176kHz
		   bit 4: 96kHz
		   bit 3: 88kHz
		   bit 2: 48kHz
		   bit 1: 44kHz
		   bit 0: 32kHz
		*/
		sample_freq = edid_parsed.sad[i].sad_byte[1];
		sample_freq_num = 0;
		for(j = 0; j < 7; j++) {
			if(sample_freq & (1 << j)) {
				if(sample_freq_num != 0)
					len += sprintf(buf + len, "|"); /* separator */
				switch(j) {
					case 0:
						len += sprintf(buf + len, "32KHz");
					break;
					case 1:
						len += sprintf(buf + len, "44KHz");
					break;
					case 2:
						len += sprintf(buf + len, "48KHz");
					break;
					case 3:
						len += sprintf(buf + len, "88KHz");
					break;
					case 4:
						len += sprintf(buf + len, "96KHz");
					break;
					case 5:
						len += sprintf(buf + len, "176KHz");
					break;
					case 6:
						len += sprintf(buf + len, "192KHz");
					break;
					default:
					break;
				}
				sample_freq_num++;
			}
		}

		if(sample_freq_num == 0)
			len += sprintf(buf +len, "0");

		/* separator */
		len += sprintf(buf + len, ",");

		/*
		SAD Byte 3 (bitrate):
	 	 For LPCM, bits 7:3 are reserved and the remaining bits define bit depth
	   	bit 2: 24 bit
	  	bit 1: 20 bit
	   	bit 0: 16 bit
		For all other sound formats, bits 7..0 designate the maximum supported bitrate divided by
		8 kbit/s.
		*/
		bitrate = edid_parsed.sad[i].sad_byte[2];
		bitrate_num = 0;
		if(audio_format == 1) { /* for LPCM */
			for(j = 0; j < 3; j++) {
				if(bitrate & (1 << j)) {
					if(bitrate_num != 0)
						len += sprintf(buf + len, "|"); /* separator */
					switch(j) {
						case 0:
							len += sprintf(buf + len, "16bit");
						break;
						case 1:
							len += sprintf(buf + len, "20bit");
						break;
						case 2:
							len += sprintf(buf + len, "24bit");
						break;
						default:
						break;
					}
					bitrate_num++;
				}
			}
		} else if(audio_format >= 2 && audio_format <= 8) /* From AC3 to ATRAC */
			len += sprintf(buf + len, "%dkbps", bitrate * 8);
		else /* From One-bit-audio to WMA Pro*/
			len += sprintf(buf + len, "%d", bitrate);

		len += sprintf(buf + len, "\n");
	}

	if(len == 0)
		len += sprintf(buf + len, "\n");

	return len;
}

static DEVICE_ATTR(edid_parsed, 0444, attr_show_parsed_edid, NULL);

void vpp_switch_state_init(void)
{
	/* /sys/class/switch/hdmi/state */
	switch_dev_register(&vpp_sdev);
	switch_set_state(&vpp_sdev, hdmi_get_plugin() ? 1 : 0);
	switch_dev_register(&vpp_sdev_hdcp);
	switch_set_state(&vpp_sdev_hdcp, 0);
#if 0
	switch_dev_register(&vpp_sdev_audio);
#endif
	device_create_file(vpp_sdev.dev, &dev_attr_edid_parsed);
}

void vpp_netlink_notify(int no, int cmd, int arg)
{
	int ret;
	int size;
	unsigned char *old_tail;
	struct sk_buff *skb;
	struct nlmsghdr *nlh;
	struct vpp_netlink_proc_t *proc;

	proc = vpp_netlink_get_proc(no);
	if (!proc)
		return;

	MSG("[VPP] netlink notify %d,cmd %d,0x%x\n", no, cmd, arg);

	switch (cmd) {
	case DEVICE_RX_DATA:
		size = NLMSG_SPACE(sizeof(struct wmt_cec_msg));
		break;
	case DEVICE_PLUG_IN:
	case DEVICE_PLUG_OUT:
	case DEVICE_STREAM:
		size = NLMSG_SPACE(sizeof(struct wmt_cec_msg));
		break;
	default:
		return;
	}

	skb = alloc_skb(size, GFP_ATOMIC);
	if (skb == NULL)
		return;
	old_tail = skb->tail;
	nlh = NLMSG_PUT(skb, 0, 0, 0, size-sizeof(*nlh));
	nlh->nlmsg_len = skb->tail - old_tail;

	switch (cmd) {
	case DEVICE_RX_DATA:
		nlh->nlmsg_type = DEVICE_RX_DATA;
		memcpy(NLMSG_DATA(nlh), (struct wmt_cec_msg *)arg,
			sizeof(struct wmt_cec_msg));
		break;
	case DEVICE_PLUG_IN:
	case DEVICE_PLUG_OUT:
		{
			static int cnt;
			struct wmt_cec_msg *msg;

			msg = (struct wmt_cec_msg *)NLMSG_DATA(nlh);
			msg->msgdata[0] = cnt;
			cnt++;
		}
		if (arg) {
			nlh->nlmsg_type = DEVICE_PLUG_IN;
			nlh->nlmsg_flags = edid_get_hdmi_phy_addr();
		} else {
			nlh->nlmsg_type = DEVICE_PLUG_OUT;
		}
		size = NLMSG_SPACE(sizeof(0));
		break;
	case DEVICE_STREAM:
		nlh->nlmsg_type = cmd;
		nlh->nlmsg_flags = arg;
		break;
	default:
		return;
	}

	NETLINK_CB(skb).pid = 0;
	NETLINK_CB(skb).dst_group = 0;

	if (proc->pid != 0) {
		ret = netlink_unicast(vpp_nlfd, skb, proc->pid, MSG_DONTWAIT);
		return;
	}
nlmsg_failure: /* NLMSG_PUT go to */
	if (skb != NULL)
		kfree_skb(skb);
	return;
}

void vpp_netlink_notify_plug(int vo_num, int plugin)
{
	/* set unplug flag for check_var */
	if (plugin == 0) {
		int mask = 0;

		if (g_vpp.virtual_display)
			mask = ~1;
		else if (vo_num == VPP_VOUT_ALL)
			mask = ~0;
		else {
			vout_info_t *vo_info;
			vo_info = vout_get_info_entry(vo_num);
			if (vo_info)
				mask = 0x1 << (vo_info->num);
		}
		g_vpp.fb_recheck |= mask;
	}

	if ((vpp_netlink_proc[0].pid == 0) && (vpp_netlink_proc[1].pid == 0))
		return;

	/* if hdmi unplug, clear edid_parsed */
	if(hdmi_get_plugin() == 0)
		memset(&edid_parsed, 0, sizeof(edid_parsed_t));

	vpp_netlink_notify(USER_PID, DEVICE_PLUG_IN, plugin);
	vpp_netlink_notify(WP_PID, DEVICE_PLUG_IN, plugin);

	/* hdmi plugin/unplug */
	plugin = hdmi_get_plugin();
	switch_set_state(&vpp_sdev, plugin ? 1 : 0);
	wmt_enable_mmfreq(WMT_MMFREQ_HDMI_PLUG, plugin);
}

void vpp_netlink_notify_cp(int enable)
{
	switch_set_state(&vpp_sdev_hdcp, enable);
}
#endif /* CONFIG_VPP_NOTIFY */
#endif /* __KERNEL__ */
