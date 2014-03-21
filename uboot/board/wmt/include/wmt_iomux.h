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

#ifndef __MACH_WMT_IOMUX_H__
#define __MACH_WMT_IOMUX_H__

#include <linux/types.h>

#undef	WMT_PIN
#define WMT_PIN(__gp, __bit, __irq, __name)	__name,
enum iomux_pins {
	#include "iomux.h"
};

/* use gpiolib dispatchers */
#define gpio_get_value		__gpio_get_value
#define gpio_set_value		__gpio_set_value
#define gpio_cansleep		__gpio_cansleep

enum gpio_pulltype {
	GPIO_PULL_NONE = 0,
	GPIO_PULL_UP,
	GPIO_PULL_DOWN,
};

extern int	gpio_request		(unsigned gpio);
extern void	gpio_free		(unsigned gpio);
extern int	gpio_get_value		(unsigned gpio);
extern void	gpio_set_value		(unsigned gpio, int value);
extern int	gpio_direction_input	(unsigned gpio);
extern int	gpio_direction_output	(unsigned gpio, int value);
extern int	gpio_setpull		(unsigned int gpio, enum gpio_pulltype pull);

#endif /* #ifndef __MACH_WMT_IOMUX_H__ */

