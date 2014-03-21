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
#define DEBUG
/* #define DEBUG_DETAIL */
/*----------------------- DEPENDENCE -----------------------------------------*/
#include "../lcd.h"
#include "../vout.h"
#include "../../../board/wmt/include/wmt_iomux.h"

/*----------------------- PRIVATE MACRO --------------------------------------*/
/* #define  LCD_XXXX  xxxx    *//*Example*/

/*----------------------- PRIVATE CONSTANTS ----------------------------------*/
/* #define LCD_XXXX    1     *//*Example*/

/*----------------------- PRIVATE TYPE  --------------------------------------*/
/* typedef  xxxx lcd_xxx_t; *//*Example*/
typedef struct {
	lcd_parm_t* (*get_parm)(int arg);
} lcd_device_t;

/*----------EXPORTED PRIVATE VARIABLES are defined in lcd.h  -------------*/
/*----------------------- INTERNAL PRIVATE VARIABLES - -----------------------*/
/* int  lcd_xxx;        *//*Example*/
lcd_device_t lcd_device_array[LCD_PANEL_MAX];
int lcd_panel_on = 1;
int lcd_pwm_enable;
lcd_panel_t lcd_panel_id = LCD_PANEL_MAX;
int lcd_panel_bpp = 24;

extern vout_dev_t lcd_vout_dev_ops;

/*--------------------- INTERNAL PRIVATE FUNCTIONS ---------------------------*/
/* void lcd_xxx(void); *//*Example*/
lcd_panel_t lcd_lvds_id = LCD_PANEL_MAX;

/*----------------------- Function Body --------------------------------------*/
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
	lcd_device_t *p;

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

void lcd_set_enable(int enable)
{
	DBG_MSG("%d\n",enable);
	if (!p_lcd)
		return;

	if (enable) {
		if (p_lcd->initial)
			p_lcd->initial();
		else {
			lcd_enable_signal(1); /* singal enable */
//			REG32_VAL(GPIO_BASE_ADDR + 0x80) |= 0x801;
//			REG32_VAL(GPIO_BASE_ADDR + 0xC0) |= 0x801;
		}
	} else {
		if (p_lcd->uninitial)
			p_lcd->uninitial();
		else {
			lcd_enable_signal(0); /* singal disable */
//			REG32_VAL(GPIO_BASE_ADDR + 0xC0) &= ~0x801;
		}
	}

	/* disable VPP clock in Panel off */
	if (govrh_get_MIF_enable(p_govrh) == 0) { /* HDMI */
		vpp_set_clock_enable(DEV_VPP, enable, 1);
	}
	
}

void lcd_enable_signal(int enable)
{
	DBG_MSG("%d\n",enable);	
	if (lvds_get_enable()) { /* LVDS */
		/* TODO */
	} else { /* LCD */
		govrh_set_dvo_enable(p_govrh2, enable);
	}
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
int lcd_panel_register
(
	int no,						/*!<; //[IN] device number */
	void (*get_parm)(int mode)	/*!<; //[IN] get info function pointer */
)
{
	lcd_device_t *p;

	if (no >= LCD_PANEL_MAX) {
		DBGMSG(KERN_ERR "*E* lcd device no max is %d !\n", LCD_PANEL_MAX);
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

struct { int gpio; int active; } lcd_power;

static int parse_env_lcd_power(void)
{
	static char env[] = "wmt.lcd.power";
	char *s;
	char *endp;

	s = getenv(env);
	if (!s) {
		printf("please set %s\n", env);
		return -1;
	}

	lcd_power.gpio = simple_strtoul(s, &endp, 0);
	if (*endp == '\0')
		return -1;

	s = endp + 1;
	lcd_power.active = simple_strtoul(s, &endp, 0);

	//printf("lcd power gpio %d, active %d\n", lcd_power.gpio, lcd_power.active);
	return 0;
}

/*----------------------- vout device plugin --------------------------------------*/
void lcd_set_power_down(int enable)
{
	/* lcd enable control by user */

	lcd_set_enable(1);

	if (parse_env_lcd_power())
		return;

	if (lcd_power.gpio < 0)
		return;

	gpio_request(lcd_power.gpio);
	gpio_direction_output(lcd_power.gpio,
			      enable ? !lcd_power.active : lcd_power.active);
}

void lcd_backlight_power_on(int on)
{
	if (parse_backlight_params())
		return;

	if (g_backlight_param.gpio < 0)
		return;

	gpio_request(g_backlight_param.gpio);
	gpio_direction_output(g_backlight_param.gpio,
			      on ? g_backlight_param.active : !g_backlight_param.active);
}

/* wmt_config_govrh_polar
 *  config govrh polar by uboot env "wmt.display.polar"
 */
static void wmt_config_govrh_polar(vout_t *vo)
{
	/* wmt.display.polar [clock polar]:[hsync polart]:[vsync polar]*/
	enum {
		idx_clk,
		idx_hsync,
		idx_vsync,
		idx_max
	};

	char *p;
	long ps[idx_max];
	char * endp;
	int i = 0;

	p = getenv("wmt.display.polar");
	if (!p)
		return;

	while (i < idx_max) {
		ps[i++] = simple_strtoul(p, &endp, 16);
		if (*endp == '\0')
			break;
		p = endp + 1;

		if (*p == '\0')
			break;
	}

	printf("govrh polar: clk-pol %d, hsync %d, vsync %d\n", ps[idx_clk], ps[idx_hsync], ps[idx_vsync]);
	govrh_set_dvo_clock_delay(vo->govr, (ps[idx_clk]) ? 0 : 1, 0);
	govrh_set_dvo_sync_polar(vo->govr, (ps[idx_hsync]) ? 0 : 1, (ps[idx_vsync]) ? 0 : 1);
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
		DBG_MSG("[%s] %s (id %d,bpp %d)\n", vout_inf_str[inf_mode], p_lcd->vmode.name, lcd_panel_id, lcd_panel_bpp);
		capability = p_lcd->capability;
		switch (inf_mode) {
		case VOUT_INF_LVDS:
			lvds_set_sync_polar((capability & LCD_CAP_HSYNC_HI) ? 0 : 1,
				(capability & LCD_CAP_VSYNC_HI) ? 0 : 1);
			lvds_set_rgb_type(lcd_panel_bpp);
			break;
		case VOUT_INF_DVI:
			govrh_set_dvo_clock_delay(vo->govr, (capability & LCD_CAP_CLK_HI) ? 0 : 1, 0);
			govrh_set_dvo_sync_polar(vo->govr, (capability & LCD_CAP_HSYNC_HI) ? 0 : 1,
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

	/* add for configing clk/vsync/hsynv polar by uboot env */
	wmt_config_govrh_polar(vo);

	return 0;
}

int lcd_check_plugin(int hotplug)
{
	return (p_lcd)? 1 : 0;
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
	vout_device_register(&lcd_vout_dev_ops);
	return 0;
} /* End of lcd_module_init */
module_init(lcd_module_init);
/*--------------------End of Function Body -----------------------------------*/
#undef LCD_C
