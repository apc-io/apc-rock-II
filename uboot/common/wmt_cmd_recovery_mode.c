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

static int wmt_check_recovery_mode(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	int ret = 0;
	ac_boot = *(volatile unsigned int *)(PMC_SCR4);
	switch (argc) {
	default:
		if (ac_boot == 2) {
			ret = 0;/*fast mode*/
			return ret;
		} else
			return 1;
	}
}

U_BOOT_CMD(
	check_recovery_mode,	5,	1,	wmt_check_recovery_mode,
	"show    - \n",
	NULL
);
