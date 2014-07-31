/*++
	linux/drivers/video/wmt/animation.h

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

#ifndef _BOOTANIMATION_H_
#define _BOOTANIMATION_H_


struct animation_fb_info {
    unsigned char * addr;    // frame buffer start address
    unsigned int width;      // width
    unsigned int height;     // height
    unsigned int color_fmt;  // color format,  0 -- rgb565, 1 -- rgb888
};

int animation_start(struct animation_fb_info *info);
int animation_stop(void);

#endif
