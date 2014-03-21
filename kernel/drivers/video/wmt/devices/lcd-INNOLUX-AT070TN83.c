/*++
 * linux/drivers/video/wmt/lcd-INNOLUX-AT070TN83.c
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

#define LCD_INNOLUX_AT070TN83_C
/* #define DEBUG */
/*----------------------- DEPENDENCE -----------------------------------------*/
#include "../lcd.h"

/*----------------------- PRIVATE MACRO --------------------------------------*/
/* #define  LCD_AT070TN83_XXXX  xxxx    *//*Example*/

/*----------------------- PRIVATE CONSTANTS ----------------------------------*/
/* #define LCD_AT070TN83_XXXX    1     *//*Example*/

/*----------------------- PRIVATE TYPE  --------------------------------------*/
/* typedef  xxxx lcd_xxx_t; *//*Example*/

/*----------EXPORTED PRIVATE VARIABLES are defined in lcd.h  -------------*/
static void lcd_at070tn83_power_on(void);
static void lcd_at070tn83_power_off(void);

/*----------------------- INTERNAL PRIVATE VARIABLES - -----------------------*/
/* int  lcd_xxx;        *//*Example*/
lcd_parm_t lcd_at070tn83_parm = {
	.bits_per_pixel = 18,
	.capability = LCD_CAP_CLK_HI,
	.vmode = {
	.name = "INNOLUX AT707TN83",
	.refresh = 60,
	.xres = 800,
	.yres = 480,
	.pixclock = KHZ2PICOS(33333),
	.left_margin = 45,
	.right_margin = 210,
	.upper_margin = 22,
	.lower_margin = 22,
	.hsync_len = 1,
	.vsync_len = 1,
	.sync = 0,
	.vmode = 0,
	.flag = 0,
	},
	.initial = lcd_at070tn83_power_on,
	.uninitial = lcd_at070tn83_power_off,
};

/*--------------------- INTERNAL PRIVATE FUNCTIONS ---------------------------*/
/* void lcd_xxx(void); *//*Example*/

/*----------------------- Function Body --------------------------------------*/
static void lcd_at070tn83_power_on(void)
{
/*	DPRINT("lcd_at070tn83_power_on\n"); */

	/* TODO */
#if (WMT_CUR_PID == WMT_PID_8425)
	vppif_reg32_write(GPIO_BASE_ADDR + 0x24, 0x7, 0, 0x7); /* gpio enable */
	vppif_reg32_write(GPIO_BASE_ADDR + 0x54, 0x7, 0, 0x7); /* output mode */
	vppif_reg32_write(GPIO_BASE_ADDR + 0x84, 0x7, 0, 0x0); /* output mode */
	vppif_reg32_write(GPIO_BASE_ADDR + 0x640, 0x707, 0, 0x707); /*pull en*/

	vppif_reg32_write(GPIO_BASE_ADDR + 0x84, 0x2, 1, 0x1); /* VGL lo */
	mdelay(8); /* delay 5ms */
	vppif_reg32_write(GPIO_BASE_ADDR + 0x84, 0x1, 0, 0x1); /* AVDD hi */
	mdelay(6); /* delay 5ms */
	vppif_reg32_write(GPIO_BASE_ADDR + 0x84, 0x4, 2, 0x1); /* VGH hi */
	mdelay(10); /* delay 10ms */
#endif
}

static void lcd_at070tn83_power_off(void)
{
/*	DPRINT("lcd_at070tn83_power_off\n"); */

	/* TODO */
#if (WMT_CUR_PID == WMT_PID_8425)
	mdelay(10); /* delay 10ms */
	vppif_reg32_write(GPIO_BASE_ADDR + 0x84, 0x4, 2, 0x0); /* VGH lo */
	mdelay(6); /* delay 5ms */
	vppif_reg32_write(GPIO_BASE_ADDR + 0x84, 0x1, 0, 0x0); /* AVDD lo */
	mdelay(8); /* delay 5ms */
	vppif_reg32_write(GPIO_BASE_ADDR + 0x84, 0x2, 1, 0x0); /* VGL lo */
#endif
}

lcd_parm_t *lcd_at070tn83_get_parm(int arg)
{
	return &lcd_at070tn83_parm;
}

int lcd_at070tn83_init(void)
{
	int ret;

	ret = lcd_panel_register(LCD_INNOLUX_AT070TN83,
		(void *) lcd_at070tn83_get_parm);
	return ret;
} /* End of lcd_oem_init */
module_init(lcd_at070tn83_init);

/*--------------------End of Function Body -----------------------------------*/
#undef LCD_INNOLUX_AT070TN83_C
