/*++
 * linux/drivers/video/wmt/lcd.c
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

#define LCD_C
#undef DEBUG
/* #define DEBUG */
/* #define DEBUG_DETAIL */
/*----------------------- DEPENDENCE -----------------------------------------*/
#include <linux/gpio.h>
#include "../lcd.h"
#include "../vout.h"

/*----------------------- PRIVATE MACRO --------------------------------------*/
/* #define  LCD_XXXX  xxxx    *//*Example*/

/*----------------------- PRIVATE CONSTANTS ----------------------------------*/
/* #define LCD_XXXX    1     *//*Example*/

/*----------------------- PRIVATE TYPE  --------------------------------------*/
/* typedef  xxxx lcd_xxx_t; *//*Example*/
struct lcd_device_t {
	lcd_parm_t* (*get_parm)(int arg);
};

/*----------EXPORTED PRIVATE VARIABLES are defined in lcd.h  -------------*/
/*----------------------- INTERNAL PRIVATE VARIABLES - -----------------------*/
/* int  lcd_xxx;        *//*Example*/
struct lcd_device_t lcd_device_array[LCD_PANEL_MAX];
int lcd_panel_on = 1;
int lcd_pwm_enable;
lcd_panel_t lcd_panel_id = LCD_PANEL_MAX;
int lcd_panel_bpp = 24;

vout_dev_t lcd_vout_dev_ops;

/*--------------------- INTERNAL PRIVATE FUNCTIONS ---------------------------*/
/* void lcd_xxx(void); *//*Example*/
lcd_panel_t lcd_lvds_id = LCD_PANEL_MAX;
int lcd_type;

/*----------------------- Function Body --------------------------------------*/
/*----------------------- Backlight --------------------------------------*/
void lcd_set_type(int type)
{
	lcd_type = type; /* 0-LCD, 1-LVDS */
}

int lcd_get_type(void)
{
	return lcd_type;
}

static struct {
	int gpio;
	int active;
} lcd_power;

/*----------------------- Function Body --------------------------------------*/
static int parse_uboot_param(void)
{
	char buf[64];
	size_t l = sizeof(buf);
	int ret;

	if (wmt_getsyspara("wmt.lcd.power", buf, &l)) {
		pr_err("please set wmt.lcd.power\n");
		return -EINVAL;
	}

	ret = sscanf(buf, "%d:%d", &lcd_power.gpio, &lcd_power.active);
	if (ret < 2) {
		pr_err("Batt: bat env param: %s\n", buf);
		return -EINVAL;
	}

	printk("lcd power: gpio-%d, active %d\n", lcd_power.gpio, lcd_power.active);

	ret = gpio_request(lcd_power.gpio, "lcd power");
	if (ret) {
		pr_err("request gpio %d failed\n", lcd_power.gpio);
		return ret;
	}

	return 0;
}

void lcd_power_on(bool on)
{
	if (lcd_power.gpio < 0)
		return;

	gpio_direction_output(lcd_power.gpio, on ? lcd_power.active : !lcd_power.active);
}

/*----------------------- Backlight --------------------------------------*/
void lcd_set_lvds_id(int id)
{
	lcd_lvds_id = id;
}

int lcd_get_lvds_id(void)
{
	return lcd_lvds_id;
}

void lcd_set_parm(int id, int bpp)
{
	lcd_panel_id = id;
	lcd_panel_bpp = bpp;
}

lcd_parm_t *lcd_get_parm(lcd_panel_t id, unsigned int arg)
{
	struct lcd_device_t *p;

	p = &lcd_device_array[id];
	if (p && p->get_parm)
		return p->get_parm(arg);
	return 0;
}

vout_dev_t *lcd_get_dev(void)
{
	if (lcd_panel_id >= LCD_PANEL_MAX)
		return 0;
	return &lcd_vout_dev_ops;
}

#ifdef CONFIG_KERNEL
static DEFINE_SEMAPHORE(lcd_sem);
#endif
void lcd_set_mutex(int lock)
{
#ifdef CONFIG_KERNEL
	if (lock)
		down(&lcd_sem);
	else
		up(&lcd_sem);
#endif
}


/* #define CONFIG_PANEL_SET_CLOCK */
void lcd_set_enable(int enable)
{
#ifdef CONFIG_PANEL_SET_CLOCK
	int hdmi_off;
#endif

	DBG_MSG("%d\n", enable);
	if (!p_lcd)
		return;

	lcd_set_mutex(1);
#ifdef CONFIG_PANEL_SET_CLOCK
	/* enable NA12 in Panel on */
	hdmi_off = (govrh_get_MIF_enable(p_govrh)) ? 0 : 1;
	if (hdmi_off && enable)
		vpp_set_clock_enable(DEV_NA12, enable, 1);
#endif

	if (enable) {
		if (p_lcd->initial)
			p_lcd->initial();
		else {
			lcd_enable_signal(1); /* singal enable */
			//REG32_VAL(GPIO_BASE_ADDR + 0x80) |= 0x801;
			//REG32_VAL(GPIO_BASE_ADDR + 0xC0) |= 0x801;
		}
	} else {
		if (p_lcd->uninitial)
			p_lcd->uninitial();
		else {
			lcd_enable_signal(0); /* singal disable */
			//REG32_VAL(GPIO_BASE_ADDR + 0xC0) &= ~0x801;
		}
	}

#ifdef CONFIG_PANEL_SET_CLOCK
	/* disable VPP & NA12 clock in Panel off */
	if (hdmi_off) {
		vpp_set_clock_enable(DEV_VPP, enable, 1);
		vpp_set_clock_enable(DEV_GOVRHD, enable, 1);
		vpp_set_clock_enable(DEV_HDMILVDS, enable, 1);
		if (enable == 0)
			vpp_set_clock_enable(DEV_NA12, enable, 1);
	}
#endif
	lcd_set_mutex(0);
}

void lcd_enable_signal(int enable)
{
	int hdmi_off;

	DBG_MSG("%d\n", enable);
	if (lcd_get_type()) { /* LVDS */
		/* TODO */
	} else { /* LCD */
		govrh_set_dvo_enable(p_govrh2, enable);
	}

	hdmi_off = (govrh_get_MIF_enable(p_govrh)) ? 0 : 1;
	vpp_set_clock_enable(DEV_DVO, (hdmi_off && !enable) ? 0 : 1, 1);
}

#ifdef __KERNEL__
/*----------------------- LCD --------------------------------------*/
static int __init lcd_arg_panel_id
(
	char *str			/*!<; // argument string */
)
{
	sscanf(str, "%d", (int *) &lcd_panel_id);
	if (lcd_panel_id >= LCD_PANEL_MAX)
		lcd_panel_id = LCD_PANEL_MAX;
	DBGMSG(KERN_INFO "set lcd panel id = %d\n", lcd_panel_id);
	return 1;
} /* End of lcd_arg_panel_id */

__setup("lcdid=", lcd_arg_panel_id);
#endif
int lcd_panel_register(int no, void (*get_parm)(int mode))
{
	struct lcd_device_t *p;

	if (no >= LCD_PANEL_MAX) {
		DBGMSG(KERN_ERR "*E* lcd device no max is %d !\n",
			LCD_PANEL_MAX);
		return -1;
	}

	p = &lcd_device_array[no];
	if (p->get_parm) {
		DBGMSG(KERN_ERR "*E* lcd device %d exist !\n", no);
		return -1;
	}
	p->get_parm = (void *) get_parm;
	return 0;
} /* End of lcd_device_register */

/*----------------------- vout device plugin --------------------------------------*/
void lcd_set_power_down(int enable)
{
	static int save_state = -1;

	if (save_state != enable) {
		/* lcd enable control by user */
		lcd_power_on(enable ? false : true);
		lcd_set_enable(enable ? false : true);
		save_state = enable;
	}
}

static void wmt_config_govrh_polar(vout_t *vo)
{
	/* wmt.display.polar [clock polar]:[hsync polart]:[vsync polar]*/
	char buf[64];
	size_t l = sizeof(buf);
	int clk_pol, hsync_pol, vsync_pol;

	if (wmt_getsyspara("wmt.display.polar", buf, &l)) {
		return;
	}

	sscanf(buf, "%d:%d:%d", &clk_pol, &hsync_pol, &vsync_pol);

	printk("govrh polar: clk-pol %d, hsync %d, vsync %d\n", clk_pol, hsync_pol, vsync_pol);
	govrh_set_dvo_clock_delay(vo->govr, clk_pol ? 0 : 1, 0);
	govrh_set_dvo_sync_polar(vo->govr, hsync_pol ? 0 : 1, vsync_pol ? 0 : 1);
}

int lcd_set_mode(unsigned int *option)
{
	vout_t *vo;
	vout_inf_mode_t inf_mode;

	DBG_MSG("option %d,%d\n", option[0], option[1]);

	vo = lcd_vout_dev_ops.vout;
	inf_mode = vo->inf->mode;
	if (option) {
		unsigned int capability;

		if (lcd_panel_id == 0)
			p_lcd = lcd_get_oem_parm(vo->resx, vo->resy);
		else
			p_lcd = lcd_get_parm(lcd_panel_id, lcd_panel_bpp);

		if (!p_lcd) {
			DBG_ERR("lcd %d not support\n", lcd_panel_id);
			return -1;
		}
		DBG_MSG("[%s] %s (id %d,bpp %d)\n", vout_inf_str[inf_mode],
			p_lcd->vmode.name, lcd_panel_id, lcd_panel_bpp);
		capability = p_lcd->capability;
		switch (inf_mode) {
		case VOUT_INF_LVDS:
			lvds_set_sync_polar(
				(capability & LCD_CAP_HSYNC_HI) ? 0 : 1,
				(capability & LCD_CAP_VSYNC_HI) ? 0 : 1);
			lvds_set_rgb_type(lcd_panel_bpp);
			break;
		case VOUT_INF_DVI:
			govrh_set_dvo_clock_delay(vo->govr,
				(capability & LCD_CAP_CLK_HI) ? 0 : 1, 0);
			govrh_set_dvo_sync_polar(vo->govr,
				(capability & LCD_CAP_HSYNC_HI) ? 0 : 1,
				(capability & LCD_CAP_VSYNC_HI) ? 0 : 1);
			switch (lcd_panel_bpp) {
			case 15:
				govrh_IGS_set_mode(vo->govr, 0, 1, 1);
				break;
			case 16:
				govrh_IGS_set_mode(vo->govr, 0, 3, 1);
				break;
			case 18:
				govrh_IGS_set_mode(vo->govr, 0, 2, 1);
				break;
			case 24:
				govrh_IGS_set_mode(vo->govr, 0, 0, 0);
				break;
			default:
				break;
			}
			break;
		default:
			break;
		}
	} else
		p_lcd = 0;

	wmt_config_govrh_polar(vo);
	return 0;
}

int lcd_check_plugin(int hotplug)
{
	return (p_lcd) ? 1 : 0;
}

int lcd_get_edid(char *buf)
{
	return 1;
}

int lcd_config(vout_info_t *info)
{
	info->resx = p_lcd->vmode.xres;
	info->resy = p_lcd->vmode.yres;
	info->fps = p_lcd->vmode.refresh;
	return 0;
}

int lcd_init(vout_t *vo)
{
	DBG_MSG("%d\n", lcd_panel_id);

	/* vo_set_lcd_id(LCD_CHILIN_LW0700AT9003); */
	if (lcd_panel_id >= LCD_PANEL_MAX)
		return -1;

	if (lcd_panel_id == 0)
		p_lcd = lcd_get_oem_parm(vo->resx, vo->resy);
	else
		p_lcd = lcd_get_parm(lcd_panel_id, 24);

	if (p_lcd == 0)
		return -1;

	/* set default parameters */
	vo->resx = p_lcd->vmode.xres;
	vo->resy = p_lcd->vmode.yres;
	vo->pixclk = PICOS2KHZ(p_lcd->vmode.pixclock) * 1000;
	return 0;
}

vout_dev_t lcd_vout_dev_ops = {
	.name = "LCD",
	.mode = VOUT_INF_DVI,
	.capability = VOUT_DEV_CAP_FIX_RES + VOUT_DEV_CAP_FIX_PLUG,

	.init = lcd_init,
	.set_power_down = lcd_set_power_down,
	.set_mode = lcd_set_mode,
	.config = lcd_config,
	.check_plugin = lcd_check_plugin,
	.get_edid = lcd_get_edid,
};

int lcd_module_init(void)
{
	if (parse_uboot_param()) {
		lcd_power.gpio = -1;
	}

	vout_device_register(&lcd_vout_dev_ops);
	return 0;
} /* End of lcd_module_init */
module_init(lcd_module_init);
/*--------------------End of Function Body -----------------------------------*/
#undef LCD_C
