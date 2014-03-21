/*
 * Copyright (c) 2008-2011 WonderMedia Technologies, Inc. All Rights Reserved.
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
 */

/* Written by Vincent Chen, WonderMedia Technologies, Inc., 2008-2011 */

#include <asm/cacheflush.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/spinlock.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include "mali.h"
#include <mach/hardware.h>

/* Mali-400 Power Management Kernel Driver */

#define REG_MALI_BADDR (0xD8080000 + WMT_MMAP_OFFSET)
/*
#define MALI_PMU_CONTROL
#define MALI_CLK_CONTROL
#define USE_PMC_POWER_STATUS
*/

static spinlock_t mali_spinlock;

static unsigned int mali_max_freq;
static unsigned int mali_cur_freq;

static int on = 1;
static int off;
static int acpi;
static int mem_size = 0x100000; /* 1 MiB */
static int debug;

#define MALI_POWER_MASK 0xf

#define OPT_EQUAL(opt, name) (!strncmp(opt, name, strlen(name)))
#define OPT_INTVAL(opt, name) kstrtoul(opt + strlen(name) + 1, 0, NULL)
#define OPT_STRVAL(opt, name) (opt + strlen(name))

static inline char *get_opt_string(const char *this_opt, const char *name)
{
	const char *p;
	int i;
	char *ret;

	p = OPT_STRVAL(this_opt, name);
	i = 0;
	while (p[i] && p[i] != ' ' && p[i] != ',')
		i++;
	ret = kmalloc(i + 1, GFP_KERNEL);
	if (ret) {
		strncpy(ret, p, i);
		ret[i] = '\0';
	}
	return ret;
}

static inline int get_opt_int(const char *this_opt, const char *name,
				  int *ret)
{
	if (!ret)
		return 0;

	if (!OPT_EQUAL(this_opt, name))
		return 0;

	*ret = OPT_INTVAL(this_opt, name);

	return 1;
}

static inline int get_opt_bool(const char *this_opt, const char *name,
				   int *ret)
{
	if (!ret)
		return 0;

	if (OPT_EQUAL(this_opt, name)) {
		if (this_opt[strlen(name)] == '=')
			*ret = kstrtoul(this_opt + strlen(name) + 1,
					      0, NULL);
		else
			*ret = 1;
	} else {
		if (OPT_EQUAL(this_opt, "no") && OPT_EQUAL(this_opt + 2, name))
			*ret = 0;
		else
			return 0;
	}
	return 1;
}

static int __init malipm_setup(char *options)
{
	char *this_opt;

	if (!options || !*options)
		return 0;

	/* The syntax is:
	 *     malipm=[<param>][,<param>=<val>] ...
	 * e.g.,
	 *     malipm=on:acpi,mem_size=0x100000
	 */

	while ((this_opt = strsep(&options, ",")) != NULL) {
		if (!*this_opt)
			continue;
		if (get_opt_bool(this_opt, "on", &on))
			;
		else if (get_opt_bool(this_opt, "off", &off))
			;
		else if (get_opt_bool(this_opt, "acpi", &acpi))
			;
		else if (get_opt_int(this_opt, "mem_size", &mem_size))
			;
		else if (get_opt_bool(this_opt, "debug", &debug))
			;
	}

	on = !off;
	printk(KERN_DEBUG
		"malipm: on %d, off %d, acpi %d, debug %d, %d MiB\n",
		on, off, acpi, debug, mem_size >> 20);

	return 0;
}
__setup("malipm=", malipm_setup);

static int mali_suspend(u32 cores);
static int mali_resume(u32 cores);
static void mali_enable_clock(int enab1le);
static void mali_enable_power(int enable);
static void mali_set_memory_base(unsigned int val);
static void mali_set_memory_size(unsigned int val);
static void mali_set_mem_validation_base(unsigned int val);
static void mali_set_mem_validation_size(unsigned int val);
static void mali_get_memory_base(unsigned int *val);
static void mali_get_memory_size(unsigned int *val);
static void mali_get_mem_validation_base(unsigned int *val);
static void mali_get_mem_validation_size(unsigned int *val);

/* symbols for ARM's Mali.ko */
unsigned int mali_memory_base;
EXPORT_SYMBOL(mali_memory_base);
unsigned int mali_memory_size;
EXPORT_SYMBOL(mali_memory_size);
unsigned int mali_mem_validation_base;
EXPORT_SYMBOL(mali_mem_validation_base);
unsigned int mali_mem_validation_size;
EXPORT_SYMBOL(mali_mem_validation_size);
unsigned int mali_ump_secure_id;
EXPORT_SYMBOL(mali_ump_secure_id);
unsigned int (*mali_get_ump_secure_id)(unsigned int addr, unsigned int size);
EXPORT_SYMBOL(mali_get_ump_secure_id);
void         (*mali_put_ump_secure_id)(unsigned int ump_id);
EXPORT_SYMBOL(mali_put_ump_secure_id);

/* PMU */

#define REG_MALI400_BASE        (0xd8080000 + WMT_MMAP_OFFSET)
#define REG_MALI400_GP          (REG_MALI400_BASE)
#define REG_MALI400_L2          (REG_MALI400_BASE + 0x1000)
#define REG_MALI400_PMU         (REG_MALI400_BASE + 0x2000)
#define REG_MALI400_MMU_GP      (REG_MALI400_BASE + 0x3000)
#define REG_MALI400_MMU_PP0     (REG_MALI400_BASE + 0x4000)
#define REG_MALI400_MMU_PP1     (REG_MALI400_BASE + 0x5000)
#define REG_MALI400_MMU_PP2     (REG_MALI400_BASE + 0x6000)
#define REG_MALI400_MMU_PP3     (REG_MALI400_BASE + 0x7000)
#define REG_MALI400_PP0         (REG_MALI400_BASE + 0x8000)
#define REG_MALI400_PP1         (REG_MALI400_BASE + 0xa000)
#define REG_MALI400_PP2         (REG_MALI400_BASE + 0xc000)
#define REG_MALI400_PP3         (REG_MALI400_BASE + 0xe000)

#define MMU_DTE_ADDR            0x00
#define MMU_STATUS              0x04
#define MMU_COMMAND             0x08
#define MMU_PAGE_FAULT_ADDR     0x0c
#define MMU_ZAP_ONE_LINE        0x10
#define MMU_INT_RAWSTAT         0x14
#define MMU_INT_CLEAR           0x18
#define MMU_INT_MASK            0x1c
#define MMU_INT_STATUS          0x20

#ifdef USE_PMC_POWER_STATUS
#define REG_PMC_BASE                  (0xd8130000 + WMT_MMAP_OFFSET)
#define REG_MALI_GP_SHUT_OFF_CONTROL  (REG_PMC_BASE + 0x0600)
#define REG_MALI_L2C_SHUT_OFF_CONTROL (REG_PMC_BASE + 0x0620)
#define REG_MALI_PP0_SHUT_OFF_CONTROL (REG_PMC_BASE + 0x0624)
#define REG_MALI_PP1_SHUT_OFF_CONTROL (REG_PMC_BASE + 0x0628)

#define PWR_SEQ_MSK 0x6
#define PWR_STA_MSK 0xf0

#ifndef REG_VAL32
#define REG_VAL32 REG_GET32
#endif

static int wait_powerup(unsigned int reg)
{
	int i = 10; /* 10 * 100 us = 1 ms */
	unsigned int val;

	while (i) {
		val = ioread32(reg);
		if ((val & PWR_SEQ_MSK) == 0)
			break;
		udelay(100);
		i--;
	}
	if (i == 0)
		printk(KERN_ERR "%s %d: *0x%08x = %08x\n",
			__func__, __LINE__, reg, val);
	while (i) {
		val = ioread32(reg);
		if (val & PWR_STA_MSK)
			break;
		udelay(100);
		i--;
	}
	if (i == 0)
		printk(KERN_ERR "%s %d: *0x%08x = %08x\n",
			__func__, __LINE__, reg, val);

	return i ? 0 : -1;
}

static int wait_powerdown(unsigned int reg)
{
	int i = 10; /* 10 * 100 us = 1 ms */
	unsigned int val;

	while (i) {
		val = ioread32(reg);
		if ((val & PWR_SEQ_MSK) == 0)
			break;
		udelay(100);
		i--;
	}
	if (i == 0)
		printk(KERN_ERR "%s %d: *0x%08x = %08x\n",
			__func__, __LINE__, reg, val);
	while (i) {
		val = ioread32(reg);
		if ((val & PWR_STA_MSK) == 0)
			break;
		udelay(100);
		i--;
	}
	if (i == 0)
		printk(KERN_ERR "%s %d: *0x%08x = %08x\n",
			__func__, __LINE__, reg, val);

	return i ? 0 : -1;
}
#endif /* USE_PMC_POWER_STATUS */

int mali_platform_wait_powerup(int msk)
{
#ifdef USE_PMC_POWER_STATUS
	int err;

	if (debug)
		printk(KERN_DEBUG "%s\n", __func__);

	err = 0;

	if (msk & BIT0)
		err += wait_powerup(REG_MALI_GP_SHUT_OFF_CONTROL);
	if (msk & BIT1)
		err += wait_powerup(REG_MALI_L2C_SHUT_OFF_CONTROL);
	if (msk & BIT2)
		err += wait_powerup(REG_MALI_PP0_SHUT_OFF_CONTROL);
	if (msk & BIT3)
		err += wait_powerup(REG_MALI_PP1_SHUT_OFF_CONTROL);

	if (err)
		printk(KERN_ERR "%s error\n", __func__);

	return err;
#else
	return 0;
#endif /* USE_PMC_POWER_STATUS */
}
EXPORT_SYMBOL(mali_platform_wait_powerup);

int mali_platform_wait_powerdown(int msk)
{
#ifdef USE_PMC_POWER_STATUS
	int err;

	if (debug)
		printk(KERN_DEBUG "%s\n", __func__);

	err = 0;

	if (msk & BIT0)
		err += wait_powerdown(REG_MALI_GP_SHUT_OFF_CONTROL);
	if (msk & BIT1)
		err += wait_powerdown(REG_MALI_L2C_SHUT_OFF_CONTROL);
	if (msk & BIT2)
		err += wait_powerdown(REG_MALI_PP0_SHUT_OFF_CONTROL);
	if (msk & BIT3)
		err += wait_powerdown(REG_MALI_PP1_SHUT_OFF_CONTROL);

	if (err)
		printk(KERN_ERR "%s error\n", __func__);

	return err;
#else
	return 0;
#endif /* USE_PMC_POWER_STATUS */
}
EXPORT_SYMBOL(mali_platform_wait_powerdown);

int mali_pmu_power_up(unsigned int msk)
{
	int timeout;
	unsigned int pmu0c;
	unsigned int pmu08;
	unsigned int val;

	val = ioread32(REG_MALI400_PMU + 8);
	if ((val & msk) == 0)
		return 0;

	pmu08 = ioread32(REG_MALI400_PMU + 8);
	pmu0c = ioread32(REG_MALI400_PMU + 0xc);
	iowrite32(0, REG_MALI400_PMU + 0xc);

	asm volatile("" : : : "memory");

	/* PMU_POWER_UP */
	iowrite32(msk, REG_MALI400_PMU);

	asm volatile("" : : : "memory");

	timeout = 10;

	do {
		val = ioread32(REG_MALI400_PMU + 8);
		if ((val & msk) == 0)
			break;
		msleep_interruptible(1);
		timeout--;
	} while (timeout > 0);

	if (debug) {
		val = ioread32(REG_MALI400_PMU + 8);
		if (timeout == 0)
			printk(KERN_DEBUG "%s: 0x%x, 0x%08x -> 0x%08x: fail\n",
				__func__, msk, pmu08, val);
		else
			printk(KERN_DEBUG "%s: 0x%x, 0x%08x -> 0x%08x: pass\n",
				__func__, msk, pmu08, val);
	}

	iowrite32(pmu0c, REG_MALI400_PMU + 0xc);

	msleep_interruptible(10);

	if (timeout == 0) {
		printk(KERN_DEBUG "mali pmu power up failure\n");
		return -1;
	}

	return 0;
}

int mali_pmu_power_down(unsigned int msk)
{
	unsigned int pmu08;
	unsigned int pmu0c;
	int timeout;
	unsigned int val;

	val = ioread32(REG_MALI400_PMU + 8);
	if ((val & msk) == msk)
		return 0;

	pmu08 = ioread32(REG_MALI400_PMU + 8);
	pmu0c = ioread32(REG_MALI400_PMU + 0xc);
	iowrite32(0, REG_MALI400_PMU + 0xc);

	asm volatile("" : : : "memory");

	/* PMU_POWER_DOWN */
	iowrite32(msk, REG_MALI400_PMU + 4);

	asm volatile("" : : : "memory");

	timeout = 10;

	do {
		val = ioread32(REG_MALI400_PMU + 8);
		if ((val & msk) == msk)
			break;
		msleep_interruptible(1);
		timeout--;
	} while (timeout > 0);

	if (debug) {
		val = ioread32(REG_MALI400_PMU + 8);
		if (timeout == 0)
			printk(KERN_DEBUG "%s: 0x%x, 0x%08x -> 0x%08x: fail\n",
				__func__, msk, pmu08, val);
		else
			printk(KERN_DEBUG "%s: 0x%x, 0x%08x -> 0x%08x: pass\n",
				__func__, msk, pmu08, val);
	}

	iowrite32(pmu0c, REG_MALI400_PMU + 0xc);

	msleep_interruptible(10);

	if (timeout == 0) {
		printk(KERN_DEBUG "mali pmu power down failure\n");
		return -1;
	}

	return 0;
}

static void mali_show_info(void)
{
	/* GP_CONTR_REG_VERSION */
	printk(KERN_INFO "maligp: version = 0x%08x\n",
		ioread32(REG_MALI400_GP + 0x6c));

	/* PP0_VERSION */
	printk(KERN_INFO "malipp: version = 0x%08x\n",
		ioread32(REG_MALI400_PP0 + 0x1000));
}

struct mali_device *create_mali_device(void)
{
	struct mali_device *dev;

	dev = kcalloc(1, sizeof(struct mali_device), GFP_KERNEL);
	dev->suspend = &mali_suspend;
	dev->resume = &mali_resume;
	dev->enable_clock = &mali_enable_clock;
	dev->enable_power = &mali_enable_power;
	dev->set_memory_base = &mali_set_memory_base;
	dev->set_memory_size = &mali_set_memory_size;
	dev->set_mem_validation_base = &mali_set_mem_validation_base;
	dev->set_mem_validation_size = &mali_set_mem_validation_size;
	dev->get_memory_base = &mali_get_memory_base;
	dev->get_memory_size = &mali_get_memory_size;
	dev->get_mem_validation_base = &mali_get_mem_validation_base;
	dev->get_mem_validation_size = &mali_get_mem_validation_size;

	return dev;
}
EXPORT_SYMBOL(create_mali_device);

void release_mali_device(struct mali_device *dev)
{
	kfree(dev);
}
EXPORT_SYMBOL(release_mali_device);

static int mali_suspend(u32 cores)
{
	if (debug)
		printk(KERN_DEBUG "mali_suspend(%d)\n", cores);

	return 0;
}

static int mali_resume(u32 cores)
{
	if (debug)
		printk(KERN_DEBUG "mali_resume(%d)\n", cores);

	return 0;
}

static void mali_enable_clock(int enable)
{
	int clk_en;

	/*
	 * if your enable clock with auto_pll_divisor() twice,
	 * then you have to call it at least twice to disable clock.
	 * It is really bad.
	 */

	if (enable) {
		auto_pll_divisor(DEV_MALI, CLK_ENABLE, 0, 0);
		if (debug)
			printk(KERN_DEBUG "Mali clock enabled\n");
	} else {
		do {
			clk_en = auto_pll_divisor(DEV_MALI, CLK_DISABLE, 0, 0);
		} while (clk_en);
		if (debug)
			printk(KERN_DEBUG "Mali clock disabled\n");
	}
}

static void mali_enable_power(int enable)
{
	/* Mali-400's power was always enabled on WM3481. */
}

static void mali_set_memory_base(unsigned int val)
{
	spin_lock(&mali_spinlock);
	mali_memory_base = val;
	spin_unlock(&mali_spinlock);
}

static void mali_set_memory_size(unsigned int val)
{
	spin_lock(&mali_spinlock);
	mali_memory_size = val;
	spin_unlock(&mali_spinlock);
}

static void mali_set_mem_validation_base(unsigned int val)
{
	spin_lock(&mali_spinlock);
	mali_mem_validation_base = val;
	spin_unlock(&mali_spinlock);
}

static void mali_set_mem_validation_size(unsigned int val)
{
	spin_lock(&mali_spinlock);
	mali_mem_validation_size = val;
	spin_unlock(&mali_spinlock);
}

static void mali_get_memory_base(unsigned int *val)
{
	spin_lock(&mali_spinlock);
	*val = mali_memory_base;
	spin_unlock(&mali_spinlock);
}

static void mali_get_memory_size(unsigned int *val)
{
	spin_lock(&mali_spinlock);
	*val = mali_memory_size;
	spin_unlock(&mali_spinlock);
}

static void mali_get_mem_validation_base(unsigned int *val)
{
	spin_lock(&mali_spinlock);
	*val = mali_mem_validation_base;
	spin_unlock(&mali_spinlock);
}

static void mali_get_mem_validation_size(unsigned int *val)
{
	spin_lock(&mali_spinlock);
	*val = mali_mem_validation_size;
	spin_unlock(&mali_spinlock);
}

/* Export functions */

int mali_platform_init_impl(void *data)
{
	if (debug)
		printk(KERN_DEBUG "mali_platform_init_impl\n");

	return 0;
}
EXPORT_SYMBOL(mali_platform_init_impl);

int mali_platform_deinit_impl(void *data)
{
	if (debug)
		printk(KERN_DEBUG "mali_platform_deinit_impl\n");

	return 0;
}
EXPORT_SYMBOL(mali_platform_deinit_impl);

int mali_platform_powerdown_impl(u32 cores)
{
	unsigned int status;

	if (debug)
		printk(KERN_DEBUG "mali_platform_powerdown_impl(%d)\n", cores);

	status = MALI_POWER_MASK;

	if (acpi == 0)
		return 0;

#ifdef MALI_PMU_CONTROL
	status = ioread32(REG_MALI400_PMU + 8);

	if ((status & MALI_POWER_MASK) != MALI_POWER_MASK)
		mali_pmu_power_down(MALI_POWER_MASK);
#endif

#ifdef MALI_CLK_CONTROL
	spin_lock(&mali_spinlock);
	mali_enable_clock(0);
	mali_enable_power(0);
	spin_unlock(&mali_spinlock);
#endif

	return 0;
}
EXPORT_SYMBOL(mali_platform_powerdown_impl);

int mali_platform_powerup_impl(u32 cores)
{
	unsigned int status;

	if (debug)
		printk(KERN_DEBUG "mali_platform_powerup_impl(%d)\n", cores);

#ifdef MALI_PMU_CONTROL
	status = ioread32(REG_MALI400_PMU + 8);

	/* printk("mali pmu: status = 0x08%x\n", status); */

	if ((status & MALI_POWER_MASK) != 0)
		mali_pmu_power_up(MALI_POWER_MASK);
#else
	status = 0;
#endif

#ifdef MALI_CLK_CONTROL
	spin_lock(&mali_spinlock);
	mali_enable_power(1);
	mali_enable_clock(1);
	spin_unlock(&mali_spinlock);
#endif

	return 0;
}
EXPORT_SYMBOL(mali_platform_powerup_impl);

void mali_gpu_utilization_handler_impl(u32 utilization)
{
	unsigned int freq;
	unsigned int oldfreq;
	int ret;

	if (acpi < 2 || mali_max_freq == 0)
		return;

	utilization = (utilization + 63) & ~63;
	oldfreq = mali_cur_freq;
	freq = ((utilization * mali_max_freq) >> 8) + 64;
	ret = auto_pll_divisor(DEV_MALI, SET_DIV, 2, freq); /* MHz */
	if (ret > 0)
		mali_cur_freq = (ret >> 20) + 1; /* MHz */
	printk(KERN_DEBUG "%s: %d, %d -> %d (%d) MHz\n",
		__func__, utilization, oldfreq, mali_cur_freq, freq);
}
EXPORT_SYMBOL(mali_gpu_utilization_handler_impl);

void set_mali_parent_power_domain(struct platform_device *dev)
{
	/* No implemented yet */
}
EXPORT_SYMBOL(set_mali_parent_power_domain);

static int __init mali_init(void)
{
	unsigned long smem_start;
	unsigned long smem_len;
	int err = 0;
	int ret;

	spin_lock_init(&mali_spinlock);

	smem_start = num_physpages << PAGE_SHIFT;

	smem_len = mem_size;

	mali_set_memory_base(smem_start);
	mali_set_memory_size(smem_len);
	mali_set_mem_validation_base(0);
	mali_set_mem_validation_size(0);

	mali_ump_secure_id = (unsigned int) -1;
	mali_get_ump_secure_id = NULL;
	mali_put_ump_secure_id = NULL;

	if (off)
		return -1;

	mali_enable_power(1);
	mali_enable_clock(1);

	/* Wait for power stable */
	msleep_interruptible(1);

	/* Verify Mali-400 PMU */
	err += mali_pmu_power_down(MALI_POWER_MASK);
	if (!err)
		err += mali_pmu_power_up(MALI_POWER_MASK);
	if (!err)
		mali_show_info();

	if (acpi)
		err += mali_pmu_power_down(MALI_POWER_MASK);

	if (acpi > 1) {
		ret = auto_pll_divisor(DEV_MALI, GET_FREQ, 0, 0);
		if (ret > 0) {
			mali_max_freq = (ret >> 20) + 1; /* MHz */
			mali_cur_freq = mali_max_freq;
		}
	} else {
		mali_max_freq = mali_cur_freq = 0;
	}

	/* Power on all Mali core at bootup, otherwise Mali driver will fail
	 * at driver/src/devicedrv/mali/common/mali_pp.c: mali_pp_reset_wait().
	 */
	mali_pmu_power_up(MALI_POWER_MASK);

	return err;
}

static void __exit mali_exit(void)
{
	mali_enable_clock(0);
	mali_enable_power(0);
}

module_init(mali_init);
module_exit(mali_exit);

MODULE_AUTHOR("WonderMedia Technologies, Inc.");
MODULE_DESCRIPTION("Mali PM Kernel Driver");
MODULE_LICENSE("GPL");

