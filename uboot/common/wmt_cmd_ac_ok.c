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


#define DEDICATED_GPIO_CTRL_ADDR 0xD8110040
#define DEDICATED_GPIO_OC_ADDR 0xD8110080
#define DEDICATED_GPIO_ID_ADDR 0xD8110000
#define IC_BASE_ADDR 0xD8140000
#define PMC_BASE_ADDR 0xD8130000
#define PWHC (PMC_BASE_ADDR + 0x0012)
#define PWRESET (PMC_BASE_ADDR + 0x0060)
#define PMC_SCR3 (PMC_BASE_ADDR + 0x003C)
#define PMC_SCR4 (PMC_BASE_ADDR + 0x0040)

static volatile unsigned int ac_boot;

/*
 * level: 0:reboot
 *        1:shutdown
 */
static void wmt_reboot(int level)
{
	if (level == 1) {
		printf("shutdown\n");
		*(volatile unsigned char *)(IC_BASE_ADDR+ 0x56) = 0x00;
		*(volatile unsigned short *)PWHC |= 0x205;
		asm("wfi");
	} else if (level == 0) {
		printf("reboot\n");
		*(volatile unsigned char *)(IC_BASE_ADDR+ 0x56) = 0x00;/*turn off interrupt*/
		*(volatile unsigned int *)PWRESET = 0x1;
	} else
		printf("Wrong reboot level\n");
}
static void init_gpio(void)
{
	/*set SUS_GPIO0 to input*/
	*(volatile unsigned int *)DEDICATED_GPIO_CTRL_ADDR |= 0x00400000;
	*(volatile unsigned int *)DEDICATED_GPIO_OC_ADDR &= ~0x00400000;
}
static int check_ac_ok(void)
{
	if (*(volatile unsigned int *)DEDICATED_GPIO_ID_ADDR & 0x00400000 && ac_boot == 1)/*plug in, ac_boot*/
		return 0;
	else if ((*(volatile unsigned int *)DEDICATED_GPIO_ID_ADDR & 0x00400000) == 0 && ac_boot == 1)/*plug out, ac_boot*/
		wmt_reboot(1);/*shutdown*/
	else/*plug out*/
		return 1;
}
/*
 * return:
 *        0:soft-reset
 *        1:non
 */
static int check_soft_reboot(void)
{
	if (*(volatile unsigned int *)(PMC_SCR3) & 0x10)/*soft-reset*/
		return 1;
	else
		return 0;
}

static int wmt_ac_ok_main(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	int ret = 0;
	ac_boot = *(volatile unsigned int *)(PMC_SCR4);
	switch (argc) {
	default:
		if ((ac_boot == 0) || (ac_boot == 1)) {
			if (check_soft_reboot() == 0) {
				ret = check_ac_ok();
				if (ret == 0)
					printf("AC ok\n");
				else
					printf("normal boot\n");
			} else
				ret = 1;/*normal boot since soft-reset*/
			return ret;
		} else
			return 1;
	}
}

U_BOOT_CMD(
	check_ac_ok,	5,	1,	wmt_ac_ok_main,
	"show    - \n",
	NULL
);
