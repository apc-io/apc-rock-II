/*++ 
Copyright (c) 2013 WonderMedia Technologies, Inc.

This program is free software: you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free Software 
Foundation, either version 2 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details. You
should have received a copy of the GNU General Public License along with this
program. If not, see http://www.gnu.org/licenses/>.

WonderMedia Technologies, Inc.
10F, 529, Chung-Cheng Road, Hsin-Tien, Taipei 231, R.O.C.
--*/

#include <common.h>
#include <command.h>
#include <asm/errno.h>

#include "include/wmt_iomux.h"
#include "include/common_def.h"
#include "wmt_battery.h"


extern struct charger_param charger_param;

struct charger_dev gpio_charger_dev;

static int gpio_parse_param(void)
{
	static char UBOOT_ENV[] = "wmt.charger.param";
	struct charger_param *cp = &charger_param;
	char *env;
	int enable;

	env = getenv(UBOOT_ENV);
	if (!env) {
		printf("please setenv %s\n", UBOOT_ENV);
		return -1;
	}

	/* [cable]:[pin]:[active] */
	{
		char *s;
		char *endp;

		s = env;
		enable = simple_strtol(s, &endp, 0);
		if (*endp == '\0')
			return -1;
		if (!enable)
			return -1;

		s = endp + 1;
		if(strncmp(s, "gpio", 4)) {
			printf("Can not find gpio charger\n");
			return -ENODEV;
		}
		cp->chg_dev = &gpio_charger_dev; 

		if (!(s = strchr(s, ':')))
			return -EINVAL;
		s++;

		cp->id = simple_strtoul(s, &endp, 0);
		if (*endp == '\0')
			return -1;

		s = endp + 1;
		cp->current_pin = simple_strtoul(s, &endp, 0);
		if (*endp == '\0')
			return -1;

		s = endp + 1;
		cp->current_sl = simple_strtoul(s, &endp, 0);

		printf("Found gpio_charger, cable type: %s, current_pin: %d, small_level: %d\n",
		       cp->id ? "micro-usb" : "ac", cp->current_pin, cp->current_sl);
	}

	return 0;
}



static int gpio_chg_init(void)
{
	struct charger_param *cp = &charger_param;
	gpio_request(cp->current_pin);
	return 0;
}

static int gpio_set_small_current(void)
{
	struct charger_param *cp = &charger_param;
	gpio_direction_output(cp->current_pin, cp->current_sl);
	return 0;
}
	
static int gpio_set_large_current(void)
{
	struct charger_param *cp = &charger_param;
	gpio_direction_output(cp->current_pin, !cp->current_sl);
	return 0;
}


// export for extern interface.
struct charger_dev gpio_charger_dev = {
	.name		= "gpio",
	.parse_param	= gpio_parse_param,
	.init		= gpio_chg_init,
	.set_small_current = gpio_set_small_current,
	.set_large_current = gpio_set_large_current,
};

