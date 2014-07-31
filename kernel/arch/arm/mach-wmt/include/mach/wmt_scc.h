/*++
linux/include/asm-arm/arch-wmt/wmt_scc.h

Copyright (c) 2012  WonderMedia Technologies, Inc.

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
#error "You must include hardware.h, not wmt_scc.h"
#endif

#ifndef __WMT_SCC_H
#define __WMT_SCC_H

/*
 *   Refer SCC module register set.pdf ver. 0.15
 *
 */

/*#define SYSTEM_CFG_CTRL_BASE_ADDR                       0xF8400000  // 64K */

/*
 * Address
 */
#define SCC_CHIP_ID_ADDR                    (0x0000+SYSTEM_CFG_CTRL_BASE_ADDR)

/*
 * Registers
 */
#define SCC_CHIP_ID_REG                     REG32_PTR(0x0000+SYSTEM_CFG_CTRL_BASE_ADDR)

/*
 * VAL Registers
 */
#define SCC_CHIP_ID_VAL                     REG32_VAL(0x0000+SYSTEM_CFG_CTRL_BASE_ADDR)

/*
 *  SCC_CHIP_ID_REG
 *
 */
#define SCC_ID_PART_NUMBER_MASK             0xFFFF0000
#define SCC_ID_MAJOR_MASK                   0x0000FF00
#define SCC_ID_METAL_MASK                   0x000000FF
#define SCC_CHIP_ID_MASK                    0xFFFFFFFF
#define SCC_ID_DEFAULT_PART_NUMBER          0x33000000
#define SCC_ID_MAJOR_01                     0x00000100
#define SCC_ID_METAL_01                     0x00000001
#define SCC_CHIP_ID_01                      (SCC_ID_DEFAULT_PART_NUMBER|SCC_ID_MAJOR_01|SCC_ID_METAL_01)

#endif /* __WMT_SCC_H */
