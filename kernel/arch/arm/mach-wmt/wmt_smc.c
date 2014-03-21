/*++
	linux/arch/arm/mach-wmt/wmt_clock.c

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

#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/platform_device.h>
#include <linux/debugfs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/sched.h>
#include <linux/list.h>
#include <linux/mutex.h>
#include <linux/io.h>
#include <linux/interrupt.h>
#include <linux/wait.h>
#include <linux/linkage.h>




#ifdef CONFIG_OTZONE_ASYNC_NOTIFY_SUPPORT
#include <linux/smp.h>
#endif

struct smc_cdata {
    unsigned int fn;
    unsigned int arg;
    unsigned int ret;
};



/**
 * @brief 
 *
 * @param otz_smc handler for secondary cores
 *
 * @return 
 */

static u32 wmt_smc1(u32 fn, u32 arg)
{
	register u32 r0 asm("r0") = fn;
	register u32 r1 asm("r1") = arg;
	
	do {
	asm volatile(
		".arch_extension sec\n\t"
		__asmeq("%0", "r0")
		__asmeq("%1", "r0")
		__asmeq("%2", "r1")
		"smc    #0  @ switch to secure world\n"
		: "=r" (r0)
		: "r" (r0), "r" (r1));
	} while (0);

	return r0;
}


static void wmt_secondary_smc_handler(void *info)
{
	struct smc_cdata *cd = (struct smc_cdata *)info;
	rmb();

	cd->ret = wmt_smc1(cd->fn, cd->arg);
	wmb();
}


/**
 * @brief 
 *
 * @param This function takes care of posting the smc to the 
 *        primary core
 *
 * @return 
 */
static unsigned int post_otz_smc(int cpu_id, u32 fn, u32 arg)
{
	struct smc_cdata cd;

	
	cd.fn = fn;
	cd.arg  = arg;
	wmb();

	smp_call_function_single(0, wmt_secondary_smc_handler, 
				 (void *)&cd, 1);
	rmb();

	return cd.ret;
}


/**
 * @brief 
 *
 * @param otz_smc wrapper to handle the multi core case
 *
 * @return 
 */
unsigned int wmt_smc(u32 fn, u32 arg)
{
	int cpu_id = smp_processor_id();
	unsigned int ret;
	
	if (cpu_id != 0) {
		mb();
		ret = post_otz_smc(cpu_id, fn, arg); /* post it to primary */
	} else {
		ret = wmt_smc1(fn, arg); /* called directly on primary core */
	}

	return ret;
}


MODULE_AUTHOR("WonderMedia Technologies, Inc");
MODULE_DESCRIPTION("WMT SMC Function");
MODULE_LICENSE("Dual BSD/GPL");
