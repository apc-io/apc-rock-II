/*++
	linux/drivers/media/video/wmt_v4l2/sensors/flash/flash_gpio.c

	Copyright (c) 2013  WonderMedia Technologies, Inc.

	This program is free software: you can redistribute it and/or modify it under the
	terms of the GNU General Public License as published by the Free Software Foundation,
	either version 2 of the License, or (at your option) any later version.

	This program is distributed in the hope that it will be useful, but WITHOUT
	ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
	PARTICULAR PURPOSE.  See the GNU General Public License for more details.
	You should have received a copy of the GNU General Public License along with
	this program.  If not, see <http://www.gnu.org/licenses/>.

	WonderMedia Technologies, Inc.
	10F, 529, Chung-Cheng Road, Hsin-Tien, Taipei 231, R.O.C.
--*/

#include <linux/gpio.h>
#include <mach/wmt_env.h>
#include "../cmos-subdev.h"
#include "flash.h"

static int fl_gpio = -1;

static int flash_dev_set_mode(int mode)
{
	if (fl_gpio < 0)
		return -EINVAL;

	switch (mode) {
	case FLASH_MODE_OFF:
		gpio_direction_output(fl_gpio, 0);
		break;
	case FLASH_MODE_ON:
		gpio_direction_output(fl_gpio, 1);
		break;
	case FLASH_MODE_STROBE:
		gpio_direction_output(fl_gpio, 1);
		break;
	case FLASH_MODE_TORCH:
		gpio_direction_output(fl_gpio, 1);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int parse_charger_param(void)
{
	char *dev_name = "gpio";
	char *env = "wmt.camera.flash";
	char s[64];
	size_t l = sizeof(s);
	int gpio;
	int rc;

	if (wmt_getsyspara(env, s, &l)) {
		pr_err("read %s fail!\n", env);
		return -EINVAL;
	}

	if (strncmp(s, dev_name, strlen(dev_name))) {
		return -EINVAL;
	}

	rc = sscanf(s, "gpio:%d", &gpio);
	if (rc < 1) {
		pr_err("bad uboot env: %s\n", env);
		return -EINVAL;
	}

	rc = gpio_request(gpio, "flash gpio");
	if (rc) {
		pr_err("flash gpio(%d) request failed\n", gpio);
		return rc;
	}

	fl_gpio = gpio;
	printk("flash gpio%d register success\n", fl_gpio);
	return 0;
}

static int flash_dev_init(void)
{
	return parse_charger_param();
}

static void flash_dev_exit(void)
{
	if (fl_gpio >= 0) {
		gpio_free(fl_gpio);
		fl_gpio = -1;
	}
}

struct flash_dev flash_dev_gpio = {
	.name		= "gpio",
	.init		= flash_dev_init,
	.set_mode	= flash_dev_set_mode,
	.exit		= flash_dev_exit,
};

