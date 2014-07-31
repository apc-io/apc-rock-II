/*++
 * linux/drivers/video/wmt/vpp-osif.h
 * WonderMedia video post processor (VPP) driver
 *
 * Copyright c 2013  WonderMedia  Technologies, Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * WonderMedia Technologies, Inc.
 * 4F, 533, Chung-Cheng Road, Hsin-Tien, Taipei 231, R.O.C
--*/

#ifndef VPP_OSIF_H
#define VPP_OSIF_H

/*-------------------- DEPENDENCY -------------------------------------*/
#ifdef CFG_LOADER
	#define CONFIG_UBOOT

	#ifdef __KERNEL__
	#undef __KERNEL__
	#endif
#elif defined(__KERNEL__)
	#define CONFIG_KERNEL
#else
	#define CONFIG_VPOST
#endif

/* -------------------------------------------------- */
#ifdef DEBUG
  #define DBG_MSG(fmt, args...)	DPRINT("{%s} " fmt, __func__, ## args)
#else
  #define DBG_MSG(fmt, args...)
#endif

#ifdef DEBUG_DETAIL
#define DBG_DETAIL(fmt, args...) DPRINT("{%s} " fmt, __func__, ## args)
#else
#define DBG_DETAIL(fmt, args...)
#endif
#define MSG(fmt, args...)     DPRINT("" fmt, ## args)
#define DBG_ERR(fmt, args...) \
	DPRINT(KERN_ERR "*E* {%s} " fmt, __func__, ## args)
#define DMSG(fmt, args...) \
	DPRINT("{%s,%d} " fmt, __func__, __LINE__, ## args)

#ifdef DEBUG
#define DBGMSG(fmt, args...)  DPRINT("{%s} " fmt, __func__, ## args)
#else
#define DBGMSG(fmt, args...)
#endif

#if 0	/* disable all msg */
#undef MSG
#undef DBGMSG

#define MSG(fmt, args...)
#define DBGMSG(fmt, args...)
#endif

/* -------------------------------------------------- */
#ifdef CONFIG_KERNEL
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/delay.h>
#include <linux/timer.h>
#include <linux/interrupt.h>
#include <linux/spinlock.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/wmt-mb.h>
#include <linux/netlink.h>
#include <linux/switch.h>
#include <net/sock.h>
#include <mach/hardware.h>
#include <mach/wmt_mmap.h>

#define SA_INTERRUPT IRQF_DISABLED
#endif

/* -------------------------------------------------- */
#ifdef CONFIG_UBOOT
#define CONFIG_WMT_EDID
#define CONFIG_WMT_EXT_DEV_PLUG_DISABLE

#include <linux/types.h>
#include "../../board/wmt/include/common_def.h"
#include <common.h>
#include <malloc.h>
#include "hw_devices.h"
#include "hw/wmt_mmap.h"
#include "hw/wmt-pwm.h"
#include "hw/wmt-ost.h"
#include "hw/wmt_gpio.h"
#include "wmt_display.h"
#include "../../board/wmt/include/wmt_clk.h"
#include "../../board/wmt/include/i2c.h"

#define abs(a)  ((a >= 0) ? a : (-1 * a))
#endif

/* -------------------------------------------------- */
#ifdef CONFIG_VPOST
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "global.h"
#define __ASM_ARCH_HARDWARE_H
#include "../include/wmt_mmap.h"
#include "../pmc/wmt_clk.h"
#include "../i2c/i2c.h"
#include "linux/wmt-mb.h"
#endif

/*	following is the C++ header	*/
#ifdef	__cplusplus
extern	"C" {
#endif

/*-------------------- EXPORTED PRIVATE CONSTANTS ----------------------------*/
#ifdef CONFIG_KERNEL
#define THE_MB_USER "VPP-MB"
#define DPRINT printk
/* -------------------------------------------------- */
#endif

#ifdef CONFIG_UBOOT
#define IRQ_GPIO	0

#define mdelay(x) wmt_delayus(1000 * x)
#define udelay(x) wmt_delayus(x)
#define REG32_VAL(addr)	(*(volatile unsigned int *)(addr))
#define REG16_VAL(addr)	(*(volatile unsigned short *)(addr))
#define REG8_VAL(addr)	(*(volatile unsigned char *)(addr))

#define mb_alloc(a)	malloc(a)
#define kmalloc(a, b)	malloc(a)
#define kfree(a)	free(a)
#define GFP_KERNEL	0
#define module_init(a)

#define DPRINT printf
#define mb_phys_to_virt(a)	(a)
#define mb_virt_to_phys(a)	(a)
#define EXPORT_SYMBOL(a)

#define IRQF_SHARED	0
#define IRQF_DISABLED	0
#define SA_INTERRUPT	0

#define KERN_ALERT
#define KERN_ERR
#define KERN_DEBUG
#define KERN_WARNING
#define KERN_INFO

#define printk printf
#define BIT(x) (1 << x)
#endif

/*-------------------- EXPORTED PRIVATE TYPES---------------------------------*/
/* typedef  void  hdmi_xxx_t;  *//*Example*/

/*-------------------- EXPORTED PRIVATE VARIABLES ----------------------------*/
#ifdef VPP_OSIF_C
#define EXTERN
#else
#define EXTERN   extern
#endif /* ifdef VPP_OSIF_C */

/* EXTERN int      hdmi_xxx; *//*Example*/

#undef EXTERN

/*--------------------- EXPORTED PRIVATE MACROS ------------------------------*/
/* #define HDMI_XXX_YYY   xxxx *//*Example*/

/*--------------------- EXPORTED PRIVATE FUNCTIONS  --------------------------*/
/* extern void  hdmi_xxx(void); *//*Example*/
#ifdef CONFIG_KERNEL
extern void wmt_i2c_xfer_continue_if(struct i2c_msg *msg,
					unsigned int num);
extern void wmt_i2c_xfer_if(struct i2c_msg *msg);
extern int wmt_i2c_xfer_continue_if_4(struct i2c_msg *msg,
					unsigned int num, int bus_id);
#endif

#ifdef CONFIG_UBOOT
extern int wmt_getsyspara(char *varname, char *varval, int *varlen);
extern int auto_pll_divisor(enum dev_id dev, enum clk_cmd cmd,
				int unit, int freq);
extern struct fb_var_screeninfo vfb_var;
#endif

#ifdef CONFIG_VPOST
void vpp_initialization(int FunctionNumber);
void udelay(int us);
void mdelay(int ms);
extern int auto_pll_divisor(enum dev_id dev, enum clk_cmd cmd,
					int unit, int freq);
extern void vpp_post_delay(U32 tmr);
extern int get_num(unsigned int min, unsigned int max,
				char *message, unsigned int retry);
#endif

int wmt_getsyspara(char *varname, char *varval, int *varlen);
int vpp_request_irq(unsigned int irq_no, void *routine,
				unsigned int flags, char *name, void *arg);
void vpp_free_irq(unsigned int irq_no, void *arg);
int vpp_parse_param(char *buf, unsigned int *param,
				int cnt, unsigned int hex_mask);
void vpp_lock_l(void);
void vpp_unlock(void);

#define vpp_lock() vpp_lock_l(); \
	/* DPRINT("vpp_lock %s %d\n",__FUNCTION__,__LINE__); */

int vpp_i2c_write(int id, unsigned int addr, unsigned int index,
				char *pdata, int len);
int vpp_i2c_read(int id, unsigned int addr, unsigned int index,
				char *pdata, int len);
int vpp_i2c_enhanced_ddc_read(int id, unsigned int addr,
			unsigned int index, char *pdata, int len);
int vpp_i2c_init(int i2c_id, unsigned short addr);
int vpp_i2c_release(void);
void vpp_set_clock_enable(enum dev_id dev, int enable, int force);
void vpp_udelay(unsigned int us);

#ifdef	__cplusplus
}
#endif
#endif /* VPP_OSIF_H */

