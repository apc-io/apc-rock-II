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

#ifndef __WMT_BATTERY_H_
#define __WMT_BATTERY_H_


extern void hw_set_small_current(void);
extern void hw_set_large_current(void);
extern int hw_is_dc_plugin(void);

extern int wmt_bat_init(void);

extern int wmt_bat_get_capacity(void);
extern int wmt_bat_get_current(void);
extern int wmt_bat_get_voltage(void);

extern void do_wmt_poweroff(void);
extern int charger_param_init(void);

enum cable_type {
	CABLE_TYPE_AC = 0,
	CABLE_TYPE_USB,
	CABLE_TYPE_MAX,
};

/* you can use this function if wmt_bat_init() successed */
extern int wmt_charger_cable_type(void);

/* wmt_bat_check_bl - check if battery low
 * return value:
 * 	< 0 - check failed
 * 	= 0 - battery power enough
 * 	> 0 - battery low.
 */
extern int wmt_bat_is_lowlevel(void);
extern int wmt_bat_is_gauge(void);

extern int wmt_bat_is_charging_full(void);

struct bat_dev {
	const char	*name;
	int		is_gauge;
	int		(*init)		(void);
	int		(*get_capacity)	(void);
	int		(*get_voltage)	(void);
	int		(*get_current)	(void);
	int		(*check_bl)	(void);
};

struct battery_param {
	struct bat_dev	*dev;
	int		charging_pin;
	int		charging_active;
};

struct charger_dev {
	const char	*name;
	int		(*parse_param)	(void);
	int		(*init)		(void);
	int		(*check_full)	(void);
	int		(*set_small_current)(void);
	int		(*set_large_current)(void);
};

struct charger_param {
	struct charger_dev	*chg_dev;
	enum cable_type id;
	int		current_pin;
	int		current_sl;     /* small current level */
};

#endif /* #ifndef __WMT_BATTERY_H_ */
