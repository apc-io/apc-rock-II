/*++
 * common/wmt_display/uboot-vpp.c
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

#include <config.h>
#include <common.h>
#include <command.h>
#include <version.h>
#include <stdarg.h>
#include <linux/types.h>
#include <devices.h>
#include <linux/stddef.h>
#include <asm/byteorder.h>

/* #define DEBUG */
#include "vpp.h"
#include "vout.h"
#include "minivgui.h"

#define WMT_DISPLAY

#include "../../board/wmt/include/wmt_clk.h"

#include "hw_devices.h"
#include "wmt_display.h"
#include "vout.h"

struct wmt_display_param_t g_display_param;
struct wmt_display_param_t g_display_param2;
struct wmt_pwm_setting_t g_pwm_setting;
struct wmt_backlight_param_t g_backlight_param;
vpp_timing_t g_display_tmr;
struct gpio_operation_t g_lcd_pw_pin;
int g_display_vaild;
unsigned int g_fb_phy;
unsigned int g_img_phy;
int g_logo_x;
int g_logo_y;
logo_scale_t g_logo_scale = LOGO_NOT_SCALE;

char *video_str[] = {
	"SDA", "SDD" , "LCD", "DVI" , "HDMI", "DVO2HDMI", "LVDS", "VGA"
};

extern int vo_dvi_initial(void);
extern int vo_hdmi_initial(void);
extern int vo_lvds_initial(void);
extern int vo_virtual_initial(void);
extern int vt1632_module_init(void);
extern int sil902x_module_init(void);
extern void vpp_init(void);
extern vout_dev_t *lcd_get_dev(void);
extern void vpp_config(vout_info_t *info);

static unsigned int alignmentMemory(unsigned int memtotal)
{
	unsigned int memmax = 2;

	while (memtotal > memmax) {
		memmax *= 2;
	}
	return memmax;
}

static unsigned int wmt_get_fb(unsigned int memtotal, unsigned int mbsize)
{
	unsigned int addr;

	addr = alignmentMemory(memtotal);
	addr <<= 20;
	return addr;
}

static int wmt_init_check(void)
{
	char *tmp;

	check_tf_boot();
	check_display_direction(); //added by howayhuo

	g_fb_phy = 0;
	if ((tmp = getenv (ENV_DIRECTFB)) != NULL)
		g_fb_phy = (unsigned int)simple_strtoul(tmp, NULL, 16);

	if (g_fb_phy == 0) {
		unsigned int memtotal;
		unsigned int mbsize;

		/* we must check "memtotal" because we get framebuf from it */
		if ((tmp = getenv (ENV_MEMTOTAL)) != NULL) {
			memtotal = (unsigned int)simple_strtoul(tmp, NULL, 10);
		} else {
			printf("err:need %s to cal fb addr\n", ENV_MEMTOTAL);
			return -1;
		}

		if ((tmp = getenv ("mbsize")) != NULL) {
			mbsize = (unsigned int)simple_strtoul (tmp, NULL, 10);
		} else {
			printf("err:need %s to cal fb addr\n", "mbsize");
				return -1;
		}
		g_fb_phy = wmt_get_fb(memtotal, mbsize);
	}

	if ((tmp = getenv (ENV_IMGADDR)) != NULL)
		g_img_phy = (unsigned int)simple_strtoul (tmp, NULL, 16);
	else {
		printf("%s is not found,(display show) invaild\n", ENV_IMGADDR);
		g_img_phy = 0xFFFF;
	}
	return 0;
}

#if 0
int uboot_vpp_alloc_framebuffer(unsigned int resx, unsigned int resy)
{
	return 0;
}
#endif

void vpp_mod_init(void)
{
#ifdef WMT_FTBLK_GOVRH
	govrh_mod_init();
#endif
#ifdef WMT_FTBLK_SCL
	scl_mod_init();
#endif
	/* register vout interface */
	vo_dvi_initial();
	vo_hdmi_initial();
	vo_lvds_initial();
}

void vpp_device_init(void)
{
	auto_pll_divisor(DEV_PWM,CLK_ENABLE, 0, 0);
	lcd_module_init();
	lcd_oem_init();
	lcd_lw700at9003_init();
	lcd_at070tn83_init();
	lcd_a080sn01_init();
	lcd_ek08009_init();
	lcd_HSD101PFW2_init();
	lcd_LVDS_1024x600_init();
	lcd_ssd2828_init();

	vt1632_module_init();
	sil902x_module_init();
	cs8556_module_init();
	vt1625_module_init();
}

int wmt_graphic_init(void)
{
	vout_info_t *info;
	int i;
	int y_bpp, c_bpp;
	vout_t *vo;

	if (wmt_init_check())
		return -1;

	vpp_mod_init();
	vpp_device_init();

	g_vpp.govrh_preinit = 0;
	g_vpp.alloc_framebuf = 0; /* uboot_vpp_alloc_framebuffer; */
	vpp_init();

	for (i = 0; i < VPP_VOUT_INFO_NUM; i++) {
		info = &vout_info[i];
		if (info->vo_mask) {
			vdo_framebuf_t *fb;
			unsigned int pixel_size, line_size;
            		struct fb_videomode vmode;
			mv_surface s;

			if (info->govr == 0)
				continue;
			/* find match video mode */
			vmode.xres = info->resx;
			vmode.yres = info->resy;
			vmode.refresh = info->fps;
			vmode.vmode = (info->option & VPP_OPT_INTERLACE) ?
				FB_VMODE_INTERLACED : 0;
			vout_find_match_mode(i, &vmode, 1);

			info->resx = vmode.xres;
			info->resy = vmode.yres;

			/* get framebuffer parameter */
			fb = &info->govr->fb_p->fb;
			fb->img_w = info->resx;
			fb->img_h = info->resy;
			fb->fb_w = vpp_calc_fb_width(fb->col_fmt, info->resx);
			fb->fb_h = info->resy;
			printf("[UBOOT] fb%d,img(%d,%d),fb(%d,%d)\n", i,
				fb->img_w, fb->img_h, fb->fb_w, fb->fb_h);

			info->resx_virtual = fb->fb_w;
			info->resy_virtual = fb->fb_h;
			vpp_get_colfmt_bpp(fb->col_fmt, &y_bpp, &c_bpp);
			pixel_size = y_bpp / 8;
			line_size = fb->fb_w * pixel_size;
			g_fb_phy -= (fb->fb_h * line_size);
			fb->y_addr = g_fb_phy;
			fb->c_addr = 0;

			/* vout config framebuffer & timing */
			vout_set_framebuffer(info->vo_mask,
					&info->govr->fb_p->fb);
			vout_config(info->vo_mask, info, info->p_vmode);

			/* init surface */
			s.width = fb->img_w;
			s.height = fb->img_h;
			s.startAddr = (char *) fb->y_addr;
			s.bits_per_pixel = pixel_size * 8;
			s.lineBytes = line_size;
			printf("fb %d, g_fb_phy = 0x%x, %s\n", i, g_fb_phy, vpp_mod_str[info->govr_mod]);
			mv_initSurface(i,&s);

			/* clear frame buffer */
			memset((void *)fb->y_addr, 0, fb->fb_h * line_size);
		}

		if (g_vpp.dual_display == 0) {
			break;
		}
	}

	/* enable vout */
	if (g_vpp.virtual_display) {
		extern int hdmi_cur_plugin;
		int plugin;

		/*use plug status in config to avoid reconfig TV reset signal*/
		plugin = hdmi_cur_plugin;
		/* plugin = vout_chkplug(VPP_VOUT_NUM_HDMI); */
		vout_change_status(vout_get_entry(VPP_VOUT_NUM_HDMI),
			VPP_VOUT_STS_BLANK, (plugin) ? 0 : 1);
		vout_change_status(vout_get_entry(VPP_VOUT_NUM_DVI),
			VPP_VOUT_STS_BLANK, (plugin) ? 1 : 0);
	}

	for (i = 0; i < VPP_VOUT_NUM; i++) {
		if ((vo = vout_get_entry(i))) {
			vout_set_blank((0x1 << i),
				(vo->status & VPP_VOUT_STS_BLANK) ? 1 : 0);
		}
	}

	/* set pwm for LCD backlight */
	g_display_param.vout = VPP_VOUT_DVI;
	if ( lcd_get_dev() || (lcd_get_lvds_id() != LCD_PANEL_MAX))
		g_display_param.vout = VPP_VOUT_LCD;

	if (g_display_param.vout == VPP_VOUT_LCD) {
		parse_pwm_params(ENV_DISPLAY_PWM, NULL);
		//parse_gpio_operation(ENV_LCD_POWER);
		if ((g_display_vaild&PWMDEFMASK) == PWMDEFTP)
			lcd_blt_set_pwm(g_pwm_setting.pwm_no,
					g_pwm_setting.duty, g_pwm_setting.period);
		else {
			/* fan : may need to check PWM power .. */
			pwm_set_period(g_pwm_setting.pwm_no,
				g_pwm_setting.period - 1);
			pwm_set_duty(g_pwm_setting.pwm_no,
				g_pwm_setting.duty - 1);
			pwm_set_control(g_pwm_setting.pwm_no,
				(g_pwm_setting.duty - 1) ? 0x34 : 0x8);
			pwm_set_gpio(g_pwm_setting.pwm_no,
				g_pwm_setting.duty - 1);
			pwm_set_scalar(g_pwm_setting.pwm_no,
				g_pwm_setting.scalar - 1);
		}
		//lcd_set_enable(1);
	}
#if 0
	p_govrh->dump_reg();
	p_govrh2->dump_reg();
//	hdmi_reg_dump();
	lvds_reg_dump();
#endif
	g_display_vaild |= DISPLAY_ENABLE;
	return 0;
}
#undef WMT_DISPLAY

