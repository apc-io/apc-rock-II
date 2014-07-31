/*++
 * linux/drivers/video/wmt/lcd.h
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

#ifndef LCD_H
/* To assert that only one occurrence is included */
#define LCD_H
/*-------------------- MODULE DEPENDENCY -------------------------------------*/
#include "vpp.h"

/*	following is the C++ header	*/
#ifdef	__cplusplus
extern	"C" {
#endif

/*-------------------- EXPORTED PRIVATE CONSTANTS ----------------------------*/
/* #define  LCD_XXXX  1    *//*Example*/

/*-------------------- EXPORTED PRIVATE TYPES---------------------------------*/
/* typedef  void  lcd_xxx_t;  *//*Example*/
typedef enum {
	LCD_WMT_OEM,
	LCD_CHILIN_LW0700AT9003,
	LCD_INNOLUX_AT070TN83,
	LCD_AUO_A080SN01,
	LCD_EKING_EK08009,
	LCD_HANNSTAR_HSD101PFW2,
	LCD_LVDS_1024x600,
	LCD_B079XAN01,
	LCD_BP080WX7,
	LCD_PANEL_MAX
} lcd_panel_t;

#define LCD_CAP_CLK_HI		BIT(0)
#define LCD_CAP_HSYNC_HI	BIT(1)
#define LCD_CAP_VSYNC_HI	BIT(2)
#define LCD_CAP_DE_LO		BIT(3)
typedef struct {
	int bits_per_pixel;
	unsigned int capability;
	struct fb_videomode vmode;

	void (*initial)(void);
	void (*uninitial)(void);
} lcd_parm_t;

/*-------------------- EXPORTED PRIVATE VARIABLES ---------------------------*/
#ifdef LCD_C /* allocate memory for variables only in vout.c */
#define EXTERN
#else
#define EXTERN   extern
#endif /* ifdef LCD_C */

/* EXTERN int      lcd_xxx; *//*Example*/

EXTERN lcd_parm_t *p_lcd;
#undef EXTERN

/*--------------------- EXPORTED PRIVATE MACROS ------------------------------*/
/* #define LCD_XXX_YYY   xxxx *//*Example*/
/*--------------------- EXPORTED PRIVATE FUNCTIONS  --------------------------*/
/* extern void  lcd_xxx(void); *//*Example*/

int lcd_panel_register(int no, void (*get_parm)(int mode));
lcd_parm_t *lcd_get_parm(lcd_panel_t id, unsigned int arg);
void lcd_set_parm(int id, int bpp);
lcd_parm_t *lcd_get_oem_parm(int resx, int resy);
void lcd_set_lvds_id(int id);
int lcd_get_lvds_id(void);
void lcd_set_type(int type);
int lcd_get_type(void);
void lcd_set_enable(int enable);
void lcd_enable_signal(int enable);

/* LCD back light */
#ifdef	__cplusplus
}
#endif
#endif /* ifndef LCD_H */
