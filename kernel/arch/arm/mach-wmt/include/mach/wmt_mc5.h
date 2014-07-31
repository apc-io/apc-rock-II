/*++
linux/include/asm-arm/arch-wmt/wmt_mc5.h

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

/* Be sure that virtual mapping is defined right */
#ifndef __ASM_ARCH_HARDWARE_H
#error "You must include hardware.h, not vt8500_pmc.h"
#endif

#ifndef __VT8500_MC5_H
#define __VT8500_MC5_H

/******************************************************************************
 *
 * Define the register access macros.
 *
 * Note: Current policy in standalone program is using register as a pointer.
 *
 ******************************************************************************/
#include "wmt_mmap.h"

/******************************************************************************
 *
 * VT8500 Power Management Controller Base Address.
 *
 ******************************************************************************/
#ifdef __MC5_BASE
#error "__RTC_BASE has already been defined in another file."
#endif
#ifdef MEMORY_CTRL_V4_CFG_BASE_ADDR               /* From vt8500.mmap.h */
#define __MC5_BASE      MEMORY_CTRL_V4_CFG_BASE_ADDR
#else
#define __MC5_BASE      0xFE000400      /* 64K */
#endif

/******************************************************************************
 *
 * VT8500 memory control registers.
 *
 ******************************************************************************/
/******************************************************************************
 *
 * Address constant for each register.
 *
 ******************************************************************************/
#define MC_CLOCK_CTRL0_ADDR      (__MC5_BASE + 0x0024)
#define MC_CLOCK_CTRL1_ADDR      (__MC5_BASE + 0x0028)
#define MC_CONF_ADDR      (__MC5_BASE + 0x0034)


/******************************************************************************
 *
 * Register pointer.
 *
 ******************************************************************************/
#define MC_CLOCK_CTRL0_REG			(REG32_PTR(MC_CLOCK_CTRL0_ADDR))/*0x24*/
#define MC_CLOCK_CTRL1_REG			(REG32_PTR(MC_CLOCK_CTRL1_ADDR))/*0x28*/
#define MC_CONF_REG      (REG32_PTR(MC_CONF_ADDR))/*0x34*/

/******************************************************************************
 *
 * Register value.
 *
 ******************************************************************************/
#define MC_CLOCK_CTRL0_VAL			(REG32_VAL(MC_CLOCK_CTRL0_ADDR))/*0x24*/
#define MC_CLOCK_CTRL1_VAL			(REG32_VAL(MC_CLOCK_CTRL1_ADDR))/*0x28*/
#define MC_CONF_VAL			(REG32_VAL(MC_CONF_ADDR))/*0x28*/


//#define UDC_HOTPLUG_TIMER

#endif /* __VT8500_PMC_H */
