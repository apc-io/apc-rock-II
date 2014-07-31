/*++
	linux/drivers/media/video/wmt_v4l2/sensors/flash/flash.c

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

#include <linux/init.h>
#include <linux/errno.h>
#include "flash.h"

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
#endif

static struct flash_dev *fl_devices[] = {
	&flash_dev_gpio,
	&flash_dev_eup2471,
};

struct flash_dev *flash_instantiation(void)
{
	int i;
	for (i = 0; i < ARRAY_SIZE(fl_devices); i++) {
		struct flash_dev *fl = fl_devices[i];
		if (fl->init && fl->init() == 0)
			return fl;
	}
	return NULL;
}

int flash_set_mode(struct flash_dev *fl, int mode)
{
	return (fl && fl->set_mode) ? fl->set_mode (mode) : -EINVAL;
}

void flash_destroy(struct flash_dev *fl)
{
	if (fl && fl->exit)
		fl->exit();
}

