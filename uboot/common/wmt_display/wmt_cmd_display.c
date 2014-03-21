/*++
Copyright (c) 2012 WonderMedia Technologies, Inc.

This program is free software: you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free Software
Foundation, either version 2 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details. You
should have received a copy of the GNU General Public License along with this
program. If not, see http://www.gnu.org/licenses/>.

WonderMedia Technologies, Inc.
4F, 531, Chung-Cheng Road, Hsin-Tien, Taipei 231, R.O.C.
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

#include "vpp.h"
#include "wmt_display.h"
#include "minivgui.h"
#include "hw/wmt-pwm.h"
#include "../../board/wmt/include/wmt_iomux.h"

extern int text_x, text_y;
extern void lcd_blt_enable(int no,int enable);
extern void * arm_memset(void * s,int c,size_t count);

int display_init(int on, int force)
{
	if ((g_display_vaild&DISPLAY_ENABLE) == DISPLAY_ENABLE) {
		if (force) { // force re-initial
			if (g_display_param.vout == VPP_VOUT_LCD)
				lcd_blt_enable(g_pwm_setting.pwm_no, 0);
//	govrh_set_dvo_enable(VPP_FLAG_DISABLE);
//	govrh_set_tg_enable(VPP_FLAG_DISABLE);
			g_display_vaild = 0;
		}
	} else
		force = 1;

	if (force) {
		if (wmt_graphic_init()) {
			printf("wmt_graphic_init failed\n");
			return -1;
		} else
			printf("wmt_graphic_init ok\n");
	}
	if ((g_display_param.vout == VPP_VOUT_LCD) && (on != 0)) {
		printf("before lcd_blt_enable !!!!!!!!!\n");
		lcd_blt_enable(g_pwm_setting.pwm_no, 1);
		printf("lcd_blt_enable !!!!!!!!!\n");
	}
	text_x = 30;
	text_y = 30 - CHAR_HEIGHT;
	return 0;
}

static void framebuffer_clean(void)
{
	int no;
	mv_surface *s;

	for(no = 0; no < 2; no++) {
		s = mv_getSurface(no);
		if(s->startAddr)
			arm_memset(s->startAddr, 0, s->height * s->lineBytes);
	}
}

int display_clear(void)
{
	if (g_display_vaild |= DISPLAY_ENABLE) {
		framebuffer_clean();
		return 0;
	} else
		return display_init(1, 0);
}

int display_show(int format)
{
	if (g_img_phy == 0xFFFF) {
		printf("%s is not found , command (display show) is invaild\n", ENV_IMGADDR);
		return -1;
	}
	if (display_init(0, 0))
		return -1;

	printf("Loading BMP .....\n");

	g_logo_scale = (g_vpp.virtual_display) ? LOGO_MODE_MAX : LOGO_NOT_SCALE;

	if(g_logo_scale == LOGO_NOT_SCALE) {
		if (mv_loadBmp(0, (unsigned char *)g_img_phy)) {
			printf("failed\r\n");
			return -1;
		}

		if(g_vpp.virtual_display) {
			if (mv_loadBmp(1, (unsigned char *)g_img_phy)) {
				printf("failed\r\n");
				return -1;
			}
		}
	}
	else {
		vout_info_t *info;
		mv_surface *s;
		int i;

		for (i = 0; i < 2; i++) {
			info = &vout_info[i];
			s = mv_getSurface(i);
			if (info->govr && s->startAddr) {
				vdo_framebuf_t src_fb, *dst_fb;
				unsigned int logo_addr = (unsigned int)s->startAddr;
				int line_bytes = s->lineBytes;

				s->startAddr = (char *)(logo_addr - 1920 * 1080 * 4);
				s->lineBytes = 1920 * 4;

				if (mv_loadBmp(i, (unsigned char *)g_img_phy)) {
					printf("failed\r\n");
					return -1;
				}
				memset(&src_fb, 0, sizeof(vdo_framebuf_t));
				src_fb.col_fmt = (s->bits_per_pixel == 16) ?
				                 VDO_COL_FMT_RGB_565 : VDO_COL_FMT_ARGB;
				src_fb.img_w = s->img_width;
				src_fb.img_h = s->img_height;
				src_fb.fb_w = s->lineBytes / (s->bits_per_pixel / 8);
				src_fb.fb_h = s->height;
				src_fb.y_addr = (unsigned int)s->img_startAddr;
				dst_fb = &info->govr->fb_p->fb;
				p_scl->scale_sync = 1;
				vpp_set_recursive_scale(&src_fb, dst_fb);

				//recover startAddr and lineBytes
				s->startAddr = (char *)logo_addr;
				s->lineBytes = line_bytes;
			}
		}
	}

	printf("Load Ok\n");
	if (g_display_param.vout == VPP_VOUT_LCD)
		lcd_blt_enable(g_pwm_setting.pwm_no, 1);

	return 0;
}

int display_animation(unsigned int img_phy_addr)
{
	g_logo_x = -1;
	g_logo_y = -1;
	if (display_init(0, 0))
		return -1;
	if (mv_loadBmp(0, (unsigned char *)img_phy_addr)) {
		printf("failed\r\n");
		return -1;
	}

	if(g_vpp.virtual_display) {
		if (mv_loadBmp(1, (unsigned char *)img_phy_addr)) {
			printf("failed\r\n");
			return -1;
		}
	}
	if (g_display_param.vout == VPP_VOUT_LCD)
		lcd_blt_enable(g_pwm_setting.pwm_no, 1);
	return 0;
}

static int display_set_pwm(char *val, int on)
{

	if (val == NULL) {
		printf("error : can not get pwm parameter\n");
		return -1;
	}

	lcd_blt_enable(g_pwm_setting.pwm_no, 0);

	parse_pwm_params(NULL, val);

	if ((g_display_vaild&PWMDEFMASK) == PWMDEFTP) {
		lcd_blt_set_pwm(g_pwm_setting.pwm_no, g_pwm_setting.duty, g_pwm_setting.period);
	} else {
		// fan : may need to check PWM power ..
		pwm_set_period(g_pwm_setting.pwm_no, g_pwm_setting.period-1);
		pwm_set_duty(g_pwm_setting.pwm_no, g_pwm_setting.duty-1);
		pwm_set_control(g_pwm_setting.pwm_no, (g_pwm_setting.duty-1)? 0x35:0x8);
		pwm_set_gpio(g_pwm_setting.pwm_no, g_pwm_setting.duty-1);
		pwm_set_scalar(g_pwm_setting.pwm_no, g_pwm_setting.scalar-1);
	}

	lcd_blt_enable(g_pwm_setting.pwm_no, on);

	return 0;
}

static int wmt_display_main (cmd_tbl_t * cmdtp, int flag, int argc, char *argv[])
{
	int tmp;

	if (argc < 2) {
		printf ("Usage:\n%s\n", cmdtp->usage);
		return 0;
	}

	if (strncmp(argv[1], "init", 2) == 0) {
		if (argc == 3) {
			if (strcmp(argv[2], "force") == 0) {
				display_init(1, 1);
				return 0;
			}
		}
		display_init(1, 0);
		return 0;
	}

	if (strncmp(argv[1], "clear", 2) == 0) {
		display_clear();
		return 0;
	}

	if (strncmp(argv[1], "show", 2) == 0) {
		if (4 == argc) {
			g_logo_x = simple_strtoul(argv[2], NULL, 10);
			g_logo_y = simple_strtoul(argv[3], NULL, 10);
			printf("(%d, %d) ", g_logo_x, g_logo_y);
		} else {
			g_logo_x = -1;
			g_logo_y = -1;
		}
		display_show(0);
		return 0;
	}

	if (strncmp(argv[1], "setpwm", 2) == 0) {
		if (argc == 3) {
			if (argv[2] != NULL) {
				display_set_pwm(argv[2], 1);
				return 0;
			}
		} else if (argc == 4) {
			tmp = simple_strtoul(argv[2], NULL, 10);
			if (argv[3] != NULL) {
				display_set_pwm(argv[3], tmp);
				return 0;
			}
		}
	}

	if (!strncmp(argv[1], "backlight", 2)) {
		if (!strncmp(argv[2], "on", 2))
			lcd_backlight_power_on(1);
		else
			lcd_backlight_power_on(0);
		return 0;
	}

	printf ("Usage:\n%s\n", cmdtp->usage);
	return 0;
}

U_BOOT_CMD(
	display,	5,	1,	wmt_display_main,
	"show    - \n",
	NULL
);

static int do_avdetect (cmd_tbl_t * cmd, int flag, int argc, char *argv[])
{
	int ret;
	struct gpio_param_t avdetect_gpio;

	ret = parse_gpio_param("wmt.io.avdetect", &avdetect_gpio);
	if(ret) {
		DPRINT("please set wmt.io.avdetect\n");
		return -1;
	}

	if(avdetect_gpio.gpiono > 19) {
		DPRINT("invalid avdetect gpio no: %d\n", avdetect_gpio.gpiono);
		return -1;
	}

	gpio_direction_input(avdetect_gpio.gpiono);
	gpio_setpull(avdetect_gpio.gpiono, GPIO_PULL_UP);
	gpio_request(avdetect_gpio.gpiono);
	mdelay(10);

	if (avdetect_gpio.act) {
		if(gpio_get_value(avdetect_gpio.gpiono))
			DPRINT("av plug in\n");
		else
			DPRINT("av plug out\n");
	}
	else {
		if(gpio_get_value(avdetect_gpio.gpiono))
			DPRINT("av plug out\n");
		else
			DPRINT("av plug in\n");
	}

	return 0;
}

U_BOOT_CMD(
	avdetect,	1,	0,	do_avdetect,
	"avdetect - print AV in/out status\n",
	""
);

