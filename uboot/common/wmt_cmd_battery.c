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

extern int show_charger_main(unsigned int storage_id,
			char *dev_id,
			char *ram_offset,
			char *storage_offset);

static int wmt_charger_main(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	int storage_id = 0;
	if (argc < 5) {
		printf("usage: charger_mode <storage_id> <dev[:par]> <dram_offset> <storage_offset>\n");
		return 1;
	}
	switch (argc) {
	case 5:
		if (strcmp(argv[1], "mmc") == 0) {
			storage_id = 1;
			/*
			dev_id = (int)simple_strtoul (argv[2], &ep, 16);
			part_id = (int)simple_strtoul(++ep, NULL, 16);
			dram_offset = (int)simple_strtoul(argv[3], NULL, 16);
			*/
		}
		show_charger_main(storage_id, argv[2], argv[3], argv[4]);
		return 0;
	default:
		return 1;
	}
}

U_BOOT_CMD(
	charger_mode,	5,	1,	wmt_charger_main,
	"show    - \n",
	NULL
);
