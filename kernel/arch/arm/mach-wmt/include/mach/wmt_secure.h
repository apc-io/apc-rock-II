/*++
linux/arch/arm/mach-wmt/include/mach/wmt_secure.h

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

#ifndef WMT_ARCH_WMT_SECURE_H
#define WMT_ARCH_WMT_SECURE_H

/* Monitor error code */
#define  API_HAL_RET_VALUE_NS2S_CONVERSION_ERROR	0xFFFFFFFE
#define  API_HAL_RET_VALUE_SERVICE_UNKNWON		0xFFFFFFFF

/* HAL API error codes */
#define  API_HAL_RET_VALUE_OK		0x00
#define  API_HAL_RET_VALUE_FAIL		0x01

/* Secure HAL API flags */
#define FLAG_START_CRITICAL		0x4
#define FLAG_IRQFIQ_MASK		0x3
#define FLAG_IRQ_ENABLE			0x2
#define FLAG_FIQ_ENABLE			0x1
#define NO_FLAG				0x0

/* Maximum Secure memory storage size */
#define WMT_SECURE_RAM_STORAGE	(88 * SZ_1K)

/* Secure low power HAL API index */
#define WMT4_HAL_SAVESECURERAM_INDEX	0x1a
#define WMT4_HAL_SAVEHW_INDEX		0x1b
#define WMT4_HAL_SAVEALL_INDEX		0x1c
#define WMT4_HAL_SAVEGIC_INDEX		0x1d

/* Secure Monitor mode APIs */
#define WMT_SMC_CMD_PL310CTRL           41
#define WMT_SMC_CMD_PL310AUX            42
#define WMT_SMC_CMD_PL310FILTER_START   43
#define WMT_SMC_CMD_PL310FILTER_END     44
#define WMT_SMC_CMD_PL310TAG_LATENCY    45
#define WMT_SMC_CMD_PL310DATA_LATENCY   46
#define WMT_SMC_CMD_PL310DEBUG          47
#define WMT_SMC_CMD_PL310PREFETCH       48
#define WMT_SMC_CMD_PL310POWER          49

#define WMT_SMC_CMD_LOGBUFOK            50
#define WMT_SMC_CMD_PRINTK_RET          51
#define WMT_SMC_CMD_LOGBUF_ADDR         52


#define WMT_SMC_CMD_IRQOK               53
#define WMT_SMC_CMD_IRQ_RET             54

/* Secure PPA(Primary Protected Application) APIs */
#define WMT4_PPA_L2_POR_INDEX		0x23
#define WMT4_PPA_CPU_ACTRL_SMP_INDEX	0x25

#ifndef __ASSEMBLER__

extern u32 wmt_secure_dispatcher(u32 idx, u32 flag, u32 nargs,
				u32 arg1, u32 arg2, u32 arg3, u32 arg4);

extern phys_addr_t wmt_secure_ram_mempool_base(void);
extern unsigned int wmt_smc(u32 fn, u32 arg);

#endif /* __ASSEMBLER__ */
#endif /* WMT_ARCH_WMT_SECURE_H */
