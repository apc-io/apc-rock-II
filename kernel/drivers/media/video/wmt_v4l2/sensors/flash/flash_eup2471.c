/*++
	linux/drivers/media/video/wmt_v4l2/sensors/flash/flash_eup2471.c

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

/* wmt.camera.flash <flash>:<en>:<flen>
 * wmt.camera.flash eup2471:3:2
 */

#include <linux/gpio.h>
#include <mach/wmt_env.h>
#include "../cmos-subdev.h"
#include "../../wmt-vid.h"
#include "flash.h"

struct eup2471_struct {
	int gpio_en;
	int gpio_flen;
};

static struct eup2471_struct *eup;

#define EUP2471_I2CADDR		0x37

/*
 * TODO register flash as a subdev
 */
static int flash_dev_set_mode(int mode)
{
	if (!eup)
		return -EINVAL;

	switch (mode) {
	case FLASH_MODE_OFF:
		gpio_direction_output(eup->gpio_en, 0);
		gpio_direction_output(eup->gpio_flen, 0);
		break;
	case FLASH_MODE_ON:
		gpio_direction_output(eup->gpio_en, 1);
		gpio_direction_output(eup->gpio_flen, 0);
		msleep(1);
		wmt_vid_i2c_write(EUP2471_I2CADDR, 0x00, 0x00);
		wmt_vid_i2c_write(EUP2471_I2CADDR, 0x01, 0x33);
		break;
	case FLASH_MODE_STROBE:
		gpio_direction_output(eup->gpio_en, 1);
		gpio_direction_output(eup->gpio_flen, 1);
		msleep(1);
		gpio_direction_output(eup->gpio_flen, 0);
		break;
	case FLASH_MODE_TORCH:
		gpio_direction_output(2, 0);
		gpio_direction_output(3, 1);
		msleep(1);
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static int parse_charger_param(void)
{
	char *dev_name = "eup2471";
	char *env = "wmt.camera.flash";
	char s[64];
	size_t l = sizeof(s);
	int gpio_en, gpio_flen;
	int rc;

	if (wmt_getsyspara(env, s, &l)) {
		pr_err("read %s fail!\n", env);
		return -EINVAL;
	}

	if (strncmp(s, dev_name, strlen(dev_name))) {
		return -EINVAL;
	}

	rc = sscanf(s, "eup2471:%d:%d", &gpio_en, &gpio_flen);
	if (rc < 2) {
		pr_err("bad uboot env: %s\n", env);
		return -EINVAL;
	}

	rc = gpio_request(gpio_en, "flash eup2471 en");
	if (rc) {
		pr_err("flash en gpio(%d) request failed\n", gpio_en);
		return rc;
	}

	rc = gpio_request(gpio_flen, "flash eup2471 flen");
	if (rc) {
		pr_err("flash flen gpio(%d) request failed\n", gpio_flen);
		gpio_free(gpio_en);
		return rc;
	}

	eup = kzalloc(sizeof(*eup), GFP_KERNEL);
	if (!eup)
		return -ENOMEM;

	eup->gpio_en = gpio_en;
	eup->gpio_flen = gpio_flen;

	printk("flash eup2471 register ok: en %d, flen %d\n",
	       eup->gpio_en, eup->gpio_flen);
	return 0;
}

static int flash_dev_init(void)
{
	if (eup)
		return -EINVAL;

	return parse_charger_param();
}

static void flash_dev_exit(void)
{
	if (eup) {
		gpio_free(eup->gpio_en);
		gpio_free(eup->gpio_flen);
		kfree(eup);
		eup = NULL;
	}
}

struct flash_dev flash_dev_eup2471 = {
	.name		= "eup2471",
	.init		= flash_dev_init,
	.set_mode	= flash_dev_set_mode,
	.exit		= flash_dev_exit,
};

