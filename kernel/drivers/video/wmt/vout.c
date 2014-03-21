/*++
 * linux/drivers/video/wmt/vout.c
 * WonderMedia video post processor (VPP) driver
 *
 * Copyright c 2012  WonderMedia  Technologies, Inc.
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

#define VOUT_C
#undef DEBUG
/* #define DEBUG */
/* #define DEBUG_DETAIL */
/*----------------------- DEPENDENCE -----------------------------------------*/
#include "vout.h"

/*----------------------- PRIVATE MACRO --------------------------------------*/
/* #define  VO_XXXX  xxxx    *//*Example*/

/*----------------------- PRIVATE CONSTANTS ----------------------------------*/
/* #define VO_XXXX    1     *//*Example*/

/*----------------------- PRIVATE TYPE  --------------------------------------*/
/* typedef  xxxx vo_xxx_t; *//*Example*/

/*----------EXPORTED PRIVATE VARIABLES are defined in vout.h  -------------*/
/*----------------------- INTERNAL PRIVATE VARIABLES - -----------------------*/
/* int  vo_xxx;        *//*Example*/
vout_t *vout_array[VPP_VOUT_NUM];
vout_inf_t *vout_inf_array[VOUT_INF_MODE_MAX];
vout_dev_t *vout_dev_list;

/*--------------------- INTERNAL PRIVATE FUNCTIONS ---------------------------*/
/* void vo_xxx(void); *//*Example*/

/*----------------------- Function Body --------------------------------------*/
/*----------------------- vout API --------------------------------------*/
void vout_print_entry(vout_t *vo)
{
	MSG(" ===== vout entry =====\n");
	MSG("0x%x\n", (int)vo);
	if (vo == 0)
		return;

	MSG("num %d,fix 0x%x,var 0x%x\n", vo->num, vo->fix_cap, vo->var_cap);
	MSG("inf 0x%x,dev 0x%x,ext_dev 0x%x\n", (int)vo->inf,
		(int)vo->dev, (int)vo->ext_dev);
	MSG("resx %d,resy %d,pixclk %d\n", vo->resx, vo->resy, vo->pixclk);
	MSG("status 0x%x,option %d,%d,%d\n", vo->status,
		vo->option[0], vo->option[1], vo->option[2]);

	if (vo->inf) {
		MSG(" ===== inf entry =====\n");
		MSG("mode %d, %s\n",
			vo->inf->mode, vout_inf_str[vo->inf->mode]);
	}

	if (vo->dev) {
		MSG(" ===== dev entry =====\n");
		MSG("name %s,inf %d,%s\n", vo->dev->name,
			vo->dev->mode, vout_inf_str[vo->dev->mode]);
		MSG("vout 0x%x,capability 0x%x\n",
			(int)vo->dev->vout, vo->dev->capability);
	}
}

void vout_register(int no, vout_t *vo)
{
	if (no >= VPP_VOUT_NUM)
		return;

	vo->num = no;
	vo->govr = (void *) p_govrh;
	vo->status = VPP_VOUT_STS_REGISTER + VPP_VOUT_STS_BLANK;
	vout_array[no] = vo;
}

vout_t *vout_get_entry(int no)
{
	if (no >= VPP_VOUT_NUM)
		return 0;
	return vout_array[no];
}

vout_info_t *vout_get_info_entry(int no)
{
	int i;

	if (no >= VPP_VOUT_NUM)
		return 0;

	for (i = 0; i < VPP_VOUT_NUM; i++) {
		if (vout_info[i].vo_mask & (0x1 << no))
			return &vout_info[i];
	}
	return 0;
}

void vout_change_status(vout_t *vo, int mask, int sts)
{
	DBG_DETAIL("(0x%x,%d)\n", mask, sts);
	if (sts)
		vo->status |= mask;
	else
		vo->status &= ~mask;

	switch (mask) {
	case VPP_VOUT_STS_PLUGIN:
		if (sts == 0) {
			vo->status &= ~(VPP_VOUT_STS_EDID +
				VPP_VOUT_STS_CONTENT_PROTECT);
			vo->edid_info.option = 0;
			vpp_netlink_notify_cp(0);
		}
		break;
	default:
		break;
	}
}

int vout_query_inf_support(int no, vout_inf_mode_t mode)
{
	vout_t *vo;

	if (no >= VPP_VOUT_NUM)
		return 0;

	if (mode >= VOUT_INF_MODE_MAX)
		return 0;

	vo = vout_get_entry(no);
	return (vo->fix_cap & BIT(mode)) ? 1 : 0;
}

/*----------------------- vout interface API --------------------------------*/
int vout_inf_register(vout_inf_mode_t mode, vout_inf_t *inf)
{
	if (mode >= VOUT_INF_MODE_MAX) {
		DBG_ERR("vout interface mode invalid %d\n", mode);
		return -1;
	}

	if (vout_inf_array[mode])
		DBG_ERR("vout interface register again %d\n", mode);

	vout_inf_array[mode] = inf;
	return 0;
} /* End of vout_register */

vout_inf_t *vout_inf_get_entry(vout_inf_mode_t mode)
{
	if (mode >= VOUT_INF_MODE_MAX) {
		DBG_ERR("vout interface mode invalid %d\n", mode);
		return 0;
	}
	return vout_inf_array[mode];
}

/*----------------------- vout device API -----------------------------------*/
int vout_device_register(vout_dev_t *ops)
{
	vout_dev_t *list;

	if (vout_dev_list == 0) {
		vout_dev_list = ops;
		list = ops;
	} else {
		list = vout_dev_list;
		while (list->next != 0)
			list = list->next;
		list->next = ops;
	}
	ops->next = 0;
	return 0;
}

vout_dev_t *vout_get_device(vout_dev_t *ops)
{
	if (ops == 0)
		return vout_dev_list;
	return ops->next;
}

vout_t *vout_get_entry_adapter(vout_mode_t mode)
{
	int no;

	switch (mode) {
	case VOUT_SD_DIGITAL:
	case VOUT_DVI:
	case VOUT_LCD:
	case VOUT_DVO2HDMI:
	case VOUT_SD_ANALOG:
	case VOUT_VGA:
		no = VPP_VOUT_NUM_DVI;
		break;
	case VOUT_HDMI:
		no = VPP_VOUT_NUM_HDMI;
		break;
	case VOUT_LVDS:
		no = VPP_VOUT_NUM_LVDS;
		break;
	default:
		no = VPP_VOUT_NUM;
		break;
	}
	return vout_get_entry(no);
}

vout_inf_t *vout_get_inf_entry_adapter(vout_mode_t mode)
{
	int no;

	switch (mode) {
	case VOUT_SD_DIGITAL:
	case VOUT_SD_ANALOG:
	case VOUT_DVI:
	case VOUT_LCD:
	case VOUT_DVO2HDMI:
	case VOUT_VGA:
		no = VOUT_INF_DVI;
		break;
	case VOUT_HDMI:
		no = VOUT_INF_HDMI;
		break;
	case VOUT_LVDS:
		no = VOUT_INF_LVDS;
		break;
	default:
		no = VOUT_INF_MODE_MAX;
		return 0;
	}
	return vout_inf_get_entry(no);
}

int vout_info_add_entry(vout_t *vo)
{
	vout_info_t *p;
	int i = 0;

	if (vo == 0) /* invalid */
		return 0;

	if (vo->inf) {
		switch (vo->inf->mode) {
		case VOUT_INF_DVI:
		case VOUT_INF_LVDS:
			vo->govr = p_govrh2;
			break;
		default:
			vo->govr = p_govrh;
			break;
		}
	}

	if (g_vpp.virtual_display) {
		if (vo->inf)
			i = 1;
	} else {
		if (vo->inf == 0) /* invalid */
			return 0;

		if (g_vpp.dual_display) {
			for (i = 0; i < VPP_VOUT_INFO_NUM; i++) {
				p = &vout_info[i];
				if (p->vo_mask == 0) /* no use */
					break;

				if (p->vo_mask & (0x1 << vo->num)) /* exist */
					return i;

				if (vo->govr && (p->govr_mod ==
					((vpp_mod_base_t *)(vo->govr))->mod))
					break;
			}
		}
	}

	if (i >= VPP_VOUT_INFO_NUM) {
		DBG_ERR("full\n");
		return 0;
	}
	p = &vout_info[i];
	p->num = i;
	if (p->vo_mask == 0) {
		p->resx = vo->resx;
		p->resy = vo->resy;
		p->resx_virtual = vpp_calc_align(p->resx, 4);
		p->resy_virtual = p->resy;
		p->fps = (int) vo->pixclk;
		p->govr_mod = (vo->govr) ?
			((vpp_mod_base_t *)vo->govr)->mod : VPP_MOD_MAX;
		p->govr = vo->govr;
	}	
	p->vo_mask |= (0x1 << vo->num);
	DBG_MSG("info %d,vo mask 0x%x,%s,%dx%d@%d\n", i, p->vo_mask,
		vpp_mod_str[p->govr_mod], p->resx, p->resy, p->fps);
	return i;
}

vout_info_t *vout_info_get_entry(int no)
{
	if (g_vpp.dual_display == 0)
		return &vout_info[0];

	if ((no >= VPP_VOUT_INFO_NUM) || (vout_info[no].vo_mask == 0))
		no = 0;

	return &vout_info[no];
}

void vout_info_set_fixed_timing(int no, struct fb_videomode *vmode)
{
	vout_info_t *info;

	DBG_MSG("(%d)\n", no);

	info = vout_info_get_entry(no);
	info->fixed_vmode = vmode;
}

govrh_mod_t *vout_info_get_govr(int no)
{
	vout_info_t *info;
	govrh_mod_t *govr;

	info = vout_info_get_entry(no);
	govr = info->govr;
	if (g_vpp.virtual_display || (g_vpp.dual_display == 0)) {
		govr = (hdmi_get_plugin()) ? p_govrh : p_govrh2;
	}
	return govr;
}

int vout_check_ratio_16_9(unsigned int resx, unsigned int resy)
{
	int val;

	val = (resx * 10) / resy;
	return (val >= 15) ? 1 : 0;
}

struct fb_videomode *vout_get_video_mode(int vout_num,
	struct fb_videomode *vmode, int option)
{
	int i;
	struct fb_videomode *p, *best = NULL;
	unsigned int diff = -1, diff_refresh = -1;
	int d;
	int resx, resy, fps;
	unsigned int pixel_clock, diff_pixel_clock = -1;
	vout_t *vo = 0;
	char *edid = 0;

	resx = vmode->xres;
	resy = vmode->yres;
	fps = vmode->refresh;
	pixel_clock = vmode->pixclock;
	DBG_MSG("%d,%dx%d@%d,%d,0x%x\n", vout_num, resx, resy, fps,
		(int) PICOS2KHZ(vmode->pixclock) * 1000, option);

	/* EDID detail timing */	
	if (option & VOUT_MODE_OPTION_EDID) {
		unsigned int opt;
		struct fb_videomode *edid_vmode;

		vo = vout_get_entry(vout_num);
		if (vo == 0)
			return 0;

		edid = vout_get_edid(vout_num);
		if (edid_parse(edid, &vo->edid_info)) {
			opt = fps | ((option &
				VOUT_MODE_OPTION_INTERLACE) ?
				EDID_TMR_INTERLACE : 0);
			if (edid_find_support(&vo->edid_info,
				resx, resy, opt, &edid_vmode)) {
				if (edid_vmode) {
					DBG_MSG("EDID detail timing\n");
					return edid_vmode;
				}
			}
		}
	}

	/* video mode table */
	for (i = 0; ; i++) {
		p = (struct fb_videomode *) &vpp_videomode[i];
		if (p->pixclock == 0)
			break;

		if (option & VOUT_MODE_OPTION_LESS) {
			if ((p->xres > resx) || (p->yres > resy))
				continue;
		}
		if (option & VOUT_MODE_OPTION_GREATER) {
			if ((p->xres < resx) || (p->yres < resy))
				continue;
		}
		if (option & VOUT_MODE_OPTION_INTERLACE) {
			if (!(p->vmode & FB_VMODE_INTERLACED))
				continue;
		}
		if (option & VOUT_MODE_OPTION_PROGRESS) {
			if ((p->vmode & FB_VMODE_INTERLACED))
				continue;
		}
		if ((option & VOUT_MODE_OPTION_EDID) &&
			(edid_parse(edid, &vo->edid_info))) {
			unsigned int opt;
			struct fb_videomode *edid_vmode;

			opt = p->refresh | ((option &
				VOUT_MODE_OPTION_INTERLACE) ?
				EDID_TMR_INTERLACE : 0);
			if (edid_find_support(&vo->edid_info,
				p->xres, p->yres, opt, &edid_vmode)) {
				if (edid_vmode)
					p = edid_vmode;
			} else {
				continue;
			}
		}
		d = abs(p->xres - resx) + abs(p->yres - resy);
		d = abs(d);
		if (diff > d) {
			diff = d;
			diff_refresh = abs(p->refresh - fps);
			diff_pixel_clock = abs(p->pixclock - pixel_clock);
			best = p;
		} else if (diff == d) {
			d = abs(p->refresh - fps);
			if (diff_refresh > d) {
				diff_refresh = d;
				diff_pixel_clock =
					abs(p->pixclock - pixel_clock);
				best = p;
			} else if (diff_refresh == d) {
				d = abs(p->pixclock - pixel_clock);
				if (diff_pixel_clock > d) {
					diff_pixel_clock = d;
					best = p;
				}
			}
		}
	}
	if (best)
		DBG_MSG("%dx%d@%d\n", best->xres, best->yres, best->refresh);
	return best;
}

#ifndef CONFIG_VPOST
int vout_find_match_mode(int fbnum,
				struct fb_videomode *vmode, int match)
{
	vout_info_t *info;
	struct fb_videomode *p;
	int no = VPP_VOUT_NUM;
	unsigned int option;
	int i;
	unsigned int vo_mask;

	DBG_DETAIL("(%d)\n", fbnum);

	info = vout_info_get_entry(fbnum);
	if (vmode->refresh == 59)
		vmode->refresh = 60;

	/* fixed timing */
	if (info->fixed_vmode) {
		if (info->fixed_vmode->xres != vmode->xres)
			return -1;
		if (info->fixed_vmode->yres != vmode->yres)
			return -1;
		if (info->fixed_vmode->refresh != vmode->refresh)
			return -1;
		p = info->fixed_vmode;
		goto label_find_match;
	}

	vo_mask = vout_get_mask(info);
	if (vo_mask == 0 && vmode->pixclock)
		return 0;

	/* find plugin or first vout */
	for (i = 0; i < VPP_VOUT_NUM; i++) {
		if ((vo_mask & (0x1 << i)) == 0)
			continue;

		if (no == VPP_VOUT_NUM)
			no = i;		/* get first vo */

		if (vout_chkplug(i)) {
			no = i;
			break;
		}
	}

	/* resolution match and interlace match */
	option = VOUT_MODE_OPTION_GREATER + VOUT_MODE_OPTION_LESS +
			VOUT_MODE_OPTION_EDID;
	option |= (vmode->vmode & FB_VMODE_INTERLACED) ?
		VOUT_MODE_OPTION_INTERLACE : VOUT_MODE_OPTION_PROGRESS;
	p = vout_get_video_mode(no, vmode, option);
	if (p)
		goto label_find_match;

	/* resolution match but interlace not match */
	option = VOUT_MODE_OPTION_GREATER + VOUT_MODE_OPTION_LESS +
			VOUT_MODE_OPTION_EDID;
	option |= (vmode->vmode & FB_VMODE_INTERLACED) ?
		VOUT_MODE_OPTION_PROGRESS : VOUT_MODE_OPTION_INTERLACE;
	p = vout_get_video_mode(no, vmode, option);
	if (p)
		goto label_find_match;

/*	if( !match ){ */
		/* resolution less best mode */
		option = VOUT_MODE_OPTION_LESS + VOUT_MODE_OPTION_EDID;
		p = vout_get_video_mode(no, vmode, option);
		if (p)
			goto label_find_match;
/*	} */
	DBG_ERR("no support mode\n");
	return -1;
label_find_match:
	*vmode = *p;
#ifdef CONFIG_UBOOT
	info->p_vmode = p;
#endif
	return 0;
}
#endif

int vout_find_edid_support_mode(
	edid_info_t *info,
	unsigned int *resx,
	unsigned int *resy,
	unsigned int *fps,
	int r_16_9
)
{
	/* check the EDID to find one that not large and same ratio mode*/
#ifdef CONFIG_WMT_EDID
	int i, cnt;
	struct fb_videomode *p;
	unsigned int w, h, f, option;

    if ((*resx == 720) && (*resy == 480) && (*fps == 50))
        *fps = 60;

	for (i = 0, cnt = 0; ; i++) {
		if (vpp_videomode[i].pixclock == 0)
			break;
		cnt++;
	}

	for (i = cnt - 1; i >= 0; i--) {
		p = (struct fb_videomode *) &vpp_videomode[i];
		h = p->yres;
		if (h > *resy)
			continue;

		w = p->xres;
		if (w > *resx)
			continue;

		f = p->refresh;
		if (f > *fps)
			continue;

		if (r_16_9 != vout_check_ratio_16_9(w, h))
			continue;

		option = f & EDID_TMR_FREQ;
		option |= (p->vmode & FB_VMODE_INTERLACED) ?
			EDID_TMR_INTERLACE : 0;

		if (edid_find_support(info, w, h, option, &p)) {
			*resx = w;
			*resy = h;
			*fps = f;
			DBG_MSG("(%dx%d@%d)\n", w, h, f);
			return 1;
		}
	}
#endif
	return 0;
}

/*----------------------- vout control API ----------------------------------*/
void vout_set_framebuffer(unsigned int mask, vdo_framebuf_t *fb)
{
	int i;
	vout_t *vo;

	if (mask == 0)
		return;

	for (i = 0; i < VPP_VOUT_NUM; i++) {
		if ((mask & (0x1 << i)) == 0)
			continue;
		vo = vout_get_entry(i);
		if (vo && (vo->govr))
			vo->govr->fb_p->set_framebuf(fb);
	}
}

void vout_set_tg_enable(unsigned int mask, int enable)
{
	int i;
	vout_t *vo;

	if (mask == 0)
		return;

	for (i = 0; i < VPP_VOUT_NUM; i++) {
		if ((mask & (0x1 << i)) == 0)
			continue;
		vo = vout_get_entry(i);
		if (vo && (vo->govr))
			govrh_set_tg_enable(vo->govr, enable);
	}
}

int vout_set_blank(unsigned int mask, vout_blank_t arg)
{
	int i;
	vout_t *vo;

	DBG_DETAIL("(0x%x,%d)\n", mask, arg);

	if (mask == 0)
		return 0;

	for (i = 0; i < VPP_VOUT_NUM; i++) {
		if ((mask & (0x1 << i)) == 0)
			continue;

		vo = vout_get_entry(i);
		if (vo && (vo->inf)) {
			vout_change_status(vo, VPP_VOUT_STS_BLANK, arg);
			vo->inf->blank(vo, arg);
			if (vo->dev && vo->dev->set_power_down)
				vo->dev->set_power_down(
					(arg == VOUT_BLANK_POWERDOWN) ? 1 : 0);
		}
	}

	if (1) {
		int govr_enable_mask, govr_mask;
		govrh_mod_t *govr;

		govr_enable_mask = 0;
		for (i = 0; i < VPP_VOUT_NUM; i++) {
			vo = vout_get_entry(i);
			if (vo && (vo->govr)) {
				govr_mask = (vo->govr->mod == VPP_MOD_GOVRH) ?
						0x1 : 0x2;
				govr_enable_mask |= (vo->status &
					VPP_VOUT_STS_BLANK) ? 0 : govr_mask;
			}
		}
		for (i = 0; i < VPP_VOUT_INFO_NUM; i++) {
			govr = (i == 0) ? p_govrh : p_govrh2;
			govr_mask = 0x1 << i;
			govrh_set_MIF_enable(govr,
				(govr_enable_mask & govr_mask) ? 1 : 0);
		}
	}
	return 0;
}

int vout_set_mode(int no, vout_inf_mode_t mode)
{
	vout_t *vo;

	DBG_DETAIL("(%d,%d)\n", no, mode);

	if (vout_query_inf_support(no, mode) == 0) {
		DBG_ERR("not support this interface(%d,%d)\n", no, mode);
		return -1;
	}

	vo = vout_get_entry(no);
	if (vo->inf) {
		if (vo->inf->mode == mode)
			return 0;
		vo->inf->uninit(vo, 0);
		vout_change_status(vo, VPP_VOUT_STS_ACTIVE, 0);
		if (vo->dev)
			vo->dev->set_power_down(1);
	}

	vo->inf = vout_inf_get_entry(mode);
	vo->inf->init(vo, 0);
	vout_change_status(vo, VPP_VOUT_STS_ACTIVE, 1);
	return 0;
}

int vout_config(unsigned int mask,
		vout_info_t *info, struct fb_videomode *vmode)
{
	vout_t *vo;
	unsigned int govr_mask = 0;
	int i;

	DBG_DETAIL("(0x%x)\n", mask);

	if (mask == 0)
		return 0;

	/* option for interface & device config */
    info->resx = vmode->xres;
    info->resy = vmode->yres;
    info->fps = (vmode->refresh == 59) ? 60 : vmode->refresh;
	info->option = (vmode->vmode & FB_VMODE_INTERLACED) ?
					VPP_OPT_INTERLACE : 0;
	info->option |= (vmode->sync & FB_SYNC_HOR_HIGH_ACT) ?
					VPP_DVO_SYNC_POLAR_HI : 0;
	info->option |= (vmode->sync & FB_SYNC_VERT_HIGH_ACT) ?
					VPP_DVO_VSYNC_POLAR_HI : 0;
	for (i = 0; i < VPP_VOUT_NUM; i++) {
		if ((mask & (0x1 << i)) == 0)
			continue;

		vo = vout_get_entry(i);
		if (vo == 0)
			continue;

		if (vo->govr == 0)
			continue;

		if ((govr_mask & (0x1 << vo->govr->mod)) == 0) {
			govrh_set_videomode(vo->govr, vmode);
			govr_mask |= (0x1 << vo->govr->mod);
		}

		if (vo->inf) {
			vo->inf->config(vo, (int)info);
			if (vo->dev)
				vo->dev->config(info);
		}
	}
	return 0;
}

int vout_chkplug(int no)
{
	vout_t *vo;
	vout_inf_t *inf;
	int ret = 0;

	DBG_DETAIL("(%d)\n", no);

	vo = vout_get_entry(no);
	if (vo == 0)
		return 0;

	if (vo->inf == 0)
		return 0;

	inf = vout_inf_get_entry(vo->inf->mode);
	if (inf == 0)
		return 0;

	if (vo->dev && vo->dev->check_plugin)
		ret = vo->dev->check_plugin(0);
	else
		ret = inf->chkplug(vo, 0);
	vout_change_status(vo, VPP_VOUT_STS_PLUGIN, ret);
	return ret;
}

int vout_inf_chkplug(int no, vout_inf_mode_t mode)
{
	vout_t *vo;
	vout_inf_t *inf;
	int plugin = 0;

	DBG_MSG("(%d,%d)\n", no, mode);
	if (vout_query_inf_support(no, mode) == 0)
		return 0;

	vo = vout_get_entry(no);
	inf = vout_inf_get_entry(mode);
	if (inf) {
		if (inf->chkplug)
			plugin = inf->chkplug(vo, 0);
	}
	return plugin;
}

char *vout_get_edid(int no)
{
	vout_t *vo;
	int ret;

	DBG_DETAIL("(%d)\n", no);

	if (edid_disable)
		return 0;
	vo = vout_get_entry(no);
	if (vo == 0)
		return 0;

	if (vo->status & VPP_VOUT_STS_EDID) {
		DBG_MSG("edid exist\n");
		return vo->edid;
	}

	vout_change_status(vo, VPP_VOUT_STS_EDID, 0);
#ifdef CONFIG_VOUT_EDID_ALLOC
	if (vo->edid == 0) {
		vo->edid = kmalloc(128 * EDID_BLOCK_MAX, GFP_KERNEL);
		if (!vo->edid) {
			DBG_ERR("edid buf alloc\n");
			return 0;
		}
	}
#endif

	ret = 1;
	if (vo->dev && vo->dev->get_edid) {
		ret = vo->dev->get_edid(vo->edid);
	} else {
		if (vo->inf->get_edid)
			ret = vo->inf->get_edid(vo, (int)vo->edid);
	}

	if (ret == 0) {
		DBG_DETAIL("edid read\n");
		vout_change_status(vo, VPP_VOUT_STS_EDID, 1);
		return vo->edid;
	} else {
		DBG_MSG("read edid fail\n");
	}

#ifdef CONFIG_VOUT_EDID_ALLOC
	kfree(vo->edid);
	vo->edid = 0;
#endif
	return 0;
}

int vout_get_edid_option(int no)
{
	vout_t *vo;

	DBG_DETAIL("(%d)\n", no);

	vo = vout_get_entry(no);
	if (vo == 0)
		return 0;

	if (vo->edid_info.option)
		return vo->edid_info.option;

	if (vout_get_edid(no) == 0) {
		if (no == VPP_VOUT_NUM_HDMI) { /* HDMI wo EDID still can work */
			vo->edid_info.option = (EDID_OPT_HDMI + EDID_OPT_AUDIO);
			return vo->edid_info.option;
		}
		return 0;
	}

	edid_parse(vo->edid, &vo->edid_info);
	return vo->edid_info.option;
}

unsigned int vout_get_mask(vout_info_t *vo_info)
{
	if (g_vpp.dual_display == 0)
		return VPP_VOUT_ALL;

	if (g_vpp.virtual_display) {
		if (vo_info->num == 0)
			return 0;
		return VPP_VOUT_ALL;
	}
	return vo_info->vo_mask;
}

int vout_check_plugin(int clr_sts)
{
	vout_t *vo;
	int i;
	int plugin = 0;

	for (i = 0; i <= VPP_VOUT_NUM; i++) {
		vo = vout_get_entry(i);
		if (vo == 0)
			continue;
		if (vo->inf == 0)
			continue;

		if (vo->dev) {
			if (!(vo->dev->capability & VOUT_DEV_CAP_FIX_PLUG)) {
				if (vout_chkplug(i)) {
					plugin = 1;
					if (clr_sts)
						vout_change_status(vo,
							VPP_VOUT_STS_PLUGIN, 0);
					DBG_MSG("[VPP] ext dev plugin\n");
				}
			}
		} else {
			if (!(vo->inf->capability & VOUT_INF_CAP_FIX_PLUG)) {
				if (vout_chkplug(i)) {
					plugin = 1;
					if (clr_sts)
						vout_change_status(vo,
							VPP_VOUT_STS_PLUGIN, 0);
					DBG_MSG("[VPP] inf dev plugin\n");
				}
			}
		}
	}
	return plugin;
}
/*--------------------End of Function Body -----------------------------------*/
#undef VOUT_C
