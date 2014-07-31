/*++
	linux/drivers/media/video/wmt_v4l2/sensors/flash/flash.h

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

#ifndef __FLASH_H__
#define __FLASH_H__

struct flash_dev {
	char name[20];
	int (*init)(void);
	int (*set_mode)(int mode);
	void (*exit)(void);
};

extern struct flash_dev flash_dev_gpio;
extern struct flash_dev flash_dev_eup2471;

extern struct flash_dev *flash_instantiation(void);
extern int flash_set_mode(struct flash_dev *fl, int mode);
extern void flash_destroy(struct flash_dev *fl);

#endif 	/* #ifndef __FLASH_H__ */

