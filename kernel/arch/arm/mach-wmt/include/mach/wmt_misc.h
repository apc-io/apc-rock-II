/*++
	linux/arch/arm/mach-wmt/include/mach/wmt_misc.h

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

extern void detect_wifi_module(void * pbool);
extern void wifi_power_ctrl(int open);
extern int is_mtk6622(void);
extern void wifi_power_ctrl_comm(int open,int mdelay);
//extern void force_remove_sdio2(void);
//extern void wmt_detect_sdio2(void);

