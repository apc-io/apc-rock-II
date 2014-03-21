/*++
	Copyright (c) 2008  WonderMedia Technologies, Inc.

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

#ifndef __MPS_CHARGER_H__
#define __MPS_CHARGER_H__

#include <linux/types.h>
#include <linux/power_supply.h>


#define DEV_NAME                   "mps_charger"
#define USB_BASE_ADD 0xD8007800+WMT_MMAP_OFFSET
#define GPIO_BASE_ADD 0xD8110000+WMT_MMAP_OFFSET
#define DCIN 0
#define USB_ADP 1
#define USB_PC 2
//#define SUPPORT_DCIN
#define CHGDON BIT5
#define CHDONEN BIT7
#define DCINSTS BIT8
#define CHGCURR BIT4
#define PC_MODE 0
#define ADP_MODE 1

#define TRUE 1
#define FALSE 0

struct mps_battery_info {
	int	batt_aux;
	int	temp_aux;
	int	charge_gpio;
	int	min_voltage;
	int	max_voltage;
	int	batt_div;
	int	batt_mult;
	int	temp_div;
	int	temp_mult;
	int	batt_tech;
	char *batt_name;
};

struct charger *mps;

struct charge_current_set {
	int dcin;
	int otgpc;
	int otgadp;
};

struct charger {
	struct mutex		lock;
	struct power_supply	mains;
	struct power_supply	usb;
	struct power_supply	battery;
	struct device *dev;		/* the device structure		*/
	struct work_struct mps_work;
	struct work_struct chgdon_work;

	unsigned int		mains_current_limit;
	int			en_gpio;
        int                   mps_irq;
	int                   current_state;
	bool			mains_online;
	bool			usb_online;
	bool			charging_enabled;
	bool			usb_hc_mode;
	bool			usb_otg_enabled;
	bool			is_fully_charged;
	bool			batt_dead;
	bool                 batt_low;
	bool                 otg_check;
	bool                 d_plus_check;
        bool                 chgen;
	bool                  g2214en;
	bool                usb_connected;
	bool                no_batt;
};


#endif  /* __MPS_CHARGER_H__ */
