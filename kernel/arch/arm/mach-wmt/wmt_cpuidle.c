/*++
linux/arch/arm/mach-wmt/wmt_cpuidle.c

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

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/cpuidle.h>
#include <linux/module.h>
#include <linux/io.h>
#include <linux/export.h>
#include <asm/proc-fns.h>
#include <asm/cpuidle.h>

//#define DEBUG
#ifdef  DEBUG
static int dbg_mask = 0;
module_param(dbg_mask, int, S_IRUGO | S_IWUSR);
#define id_dbg(fmt, args...) \
        do {\
            if (dbg_mask) \
                printk(KERN_ERR "[%s]_%d: " fmt, __func__ , __LINE__, ## args);\
        } while(0)
#define id_trace() \
        do {\
            if (dbg_mask) \
                printk(KERN_ERR "trace in %s %d\n", __func__, __LINE__);\
        } while(0)
#else
#define id_dbg(fmt, args...)
#define id_trace()
#endif

#define WMT_CPU_IDLE_MAX_STATES	    2
extern int wmt_setsyspara(char *varname, unsigned char *varval);
extern int wmt_getsyspara(char *varname, unsigned char *varval, int *varlenex);

/* Actual code that puts the SoC in different idle states */
static int wmt_enter_idle(struct cpuidle_device *dev, 
                            struct cpuidle_driver *drv, int index)
{
	cpu_do_idle();

	return index;
}

static struct cpuidle_driver wmt_cpuidle_driver = {
	.name =         "wmt_cpuidle",
	.owner =        THIS_MODULE,
	.en_core_tk_irqen	= 1,
	/* ARM Wait for interrupt state */
	.states[0]		= ARM_CPUIDLE_WFI_STATE,
	/* Wait for interrupt and DDR self refresh state */
    .states[1]		= {
		.enter			= wmt_enter_idle,
		.exit_latency		= 10,
		.target_residency	= 100000,
		.flags			= CPUIDLE_FLAG_TIME_VALID,
		.name			= "ZAC_OFF",
		.desc			= "WFI and disable ZAC clock",
	},
	.state_count = WMT_CPU_IDLE_MAX_STATES,
};

static int __init wmt_cpuidle_check_env(void)
{
    int ret = 0;
    int varlen = 128;
    unsigned int drv_en = 0;
    unsigned char buf[128] = {0};

    /* uboot env name is: wmt.cpuidle.param/wmt.dvfs.param */
    ret = wmt_getsyspara("wmt.cpuidle.param", buf, &varlen);                                                                    
    if (ret) {
        printk(KERN_INFO "Can not find uboot env wmt.cpuidle.param\n");
        ret = -ENODATA;
        goto out;
    }
    id_dbg("wmt.cpuidle.param:%s\n", buf);

    sscanf(buf, "%d", &drv_en);
    if (!drv_en) {
        printk(KERN_INFO "wmt cpuidle driver disaled\n");
        ret = -ENODEV;
        goto out;
    }

out:
    return ret;
}

static DEFINE_PER_CPU(struct cpuidle_device, wmt_cpuidle_device);
static int __init wmt_cpuidle_driver_init(void)
{
    struct cpuidle_device *device = NULL;

    if (wmt_cpuidle_check_env()) {
        printk(KERN_WARNING "wmt_cpuidle check env failed!\n");
        return -EINVAL;
    }

    device = &per_cpu(wmt_cpuidle_device, smp_processor_id());
	device->state_count = WMT_CPU_IDLE_MAX_STATES;

	cpuidle_register_driver(&wmt_cpuidle_driver);
	if (cpuidle_register_device(device)) {
		printk(KERN_ERR "wmt_cpuidle_driver_init: Failed registering\n");
		return -EIO;
	}

    printk(KERN_INFO "WMT cpuidle driver register\n");
    return 0;
}
module_init(wmt_cpuidle_driver_init);

MODULE_AUTHOR("WonderMedia Technologies, Inc");
MODULE_DESCRIPTION("WMT CPU idle driver");
MODULE_LICENSE("Dual BSD/GPL");
