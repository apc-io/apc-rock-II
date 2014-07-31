/******************************************************************************
 *
 * Copyright(c) 2013 Realtek Corporation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110, USA
 *
 *
 ******************************************************************************/
/*
 * Description:
 *	This file can be applied to following platforms:
 *	wonderMedia wm8880 platform
 */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/delay.h> // for msleep
#include <mach/hardware.h>//for REG32_VAL
#include <linux/gpio.h>
#include <mach/wmt_iomux.h>
#include <mach/wmt_misc.h> //for wifi_power_ctrl

#ifdef CONFIG_MMC

#endif // CONFIG_MMC
extern void wmt_detect_sdio2(void);
extern void force_remove_sdio2(void);
/*
 * Return:
 *	0:	power on successfully
 *	others: power on failed
 */
int platform_wifi_power_on(void)
{
	int ret = 0;

#ifdef CONFIG_MMC
{			
		printk("platform_wifi_power_on\n");
		wifi_power_ctrl(1);
		mdelay(10);
		wifi_power_ctrl(0);
		mdelay(10);
		wifi_power_ctrl(1);
		msleep(200);
		wmt_detect_sdio2();
		printk("exit platform_wifi_power_on\n");
}
#endif // CONFIG_MMC

	return ret;
}

void platform_wifi_power_off(void)
{
#ifdef CONFIG_MMC
	printk("begin to platform_wifi_power_off\n");
	wifi_power_ctrl(0);
	force_remove_sdio2();
	printk("exit platform_wifi_power_off\n");
#endif // CONFIG_MMC
}
