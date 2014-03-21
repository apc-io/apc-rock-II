/*++
	linux/include/linux/power/wmt_charger_common.h

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
#ifndef _LINUX_WMT_CHARGER_COMMON_H_
#define _LINUX_WMT_CHARGER_COMMON_H_

extern int wmt_charger_gpio_probe(const char *devname);
extern void wmt_charger_gpio_release(void);
 
extern int wmt_charger_is_dc_plugin(void);
extern int wmt_charger_is_bat_charging(void);
extern int wmt_charger_is_charging_full(void);

extern void wmt_charger_current_control(void);

#endif 	/* #ifndef _LINUX_WMT_CHARGER_COMMON_H_ */


