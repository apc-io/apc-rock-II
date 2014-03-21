/*++
	linux/drivers/video/backlight/wmt_bl.h

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

#ifndef _LINUX_WMT_BL_H
#define _LINUX_WMT_BL_H


/* Backlight close or open events */
#define BL_CLOSE	0x0001 /* Close the backlight */
#define BL_OPEN		0x0002 /* Open the backlight */

//extern int bl_notifier_call_chain(unsigned long val);

#endif

