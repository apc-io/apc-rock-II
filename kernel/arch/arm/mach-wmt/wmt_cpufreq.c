/*++
	linux/arch/arm/mach-wmt/wmt_cpufreq.c

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
#include <linux/types.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/cpufreq.h>
#include <linux/delay.h>
#include <linux/reboot.h>
#include <linux/syscalls.h>
#include <linux/suspend.h>
#include <asm/smp_plat.h>
#include <asm/cpu.h>
#include <linux/regulator/consumer.h>

#include <mach/wmt_env.h>
#include <mach/hardware.h>

/*
#define DEBUG
*/
#ifdef DEBUG
static int dbg_mask = 1;
module_param(dbg_mask, int, S_IRUGO | S_IWUSR);
#define fq_dbg(fmt, args...) \
	do {\
		if (dbg_mask) \
			printk(KERN_ERR "[%s]_%d_%d: " fmt, __func__ , __LINE__, smp_processor_id(), ## args);\
	} while (0)
#define fq_trace() \
	do {\
		if (dbg_mask) \
			printk(KERN_ERR "trace in %s %d\n", __func__, __LINE__);\
	} while (0)
#else
#define fq_dbg(fmt, args...)
#define fq_trace()
#endif

#define DVFS_TABLE_NUM_MAX  20
#define DVFS_TABLE_NUM_MIN  2
#define PMC_BASE            PM_CTRL_BASE_ADDR
#define PLLA_FREQ_KHZ       (24 * 1000)
#define PMC_PLLA            (PM_CTRL_BASE_ADDR + 0x200)
#define ARM_DIV_OFFSET      (PM_CTRL_BASE_ADDR + 0x300)
#define MILLISEC_TO_MICROSEC  1000

struct wmt_dvfs_table {
	unsigned int freq;
	unsigned int vol;
	unsigned int l2c_div;
	unsigned int l2c_tag;
	unsigned int l2c_data;
	unsigned int axi;
	int index;
	struct list_head node;
};

struct wmt_dvfs_driver_data {
	unsigned int tbl_num;
	unsigned int sample_rate;
	struct list_head wmt_dvfs_list;
	struct wmt_dvfs_table *dvfs_table;
	struct cpufreq_frequency_table *freq_table;
};
static struct wmt_dvfs_driver_data wmt_dvfs_drvdata;
static struct regulator *re;
static unsigned int use_regulator = 0;

char	use_dvfs;/*flag for read env varalbe*/
static char	use_dvfs_debug;
static int wmt_dvfs_running;/*flag for reboot disable dvfs*/
static DEFINE_SEMAPHORE(wmt_cpufreq_sem);

/* register a reboot notifier, invoked in kernel_restart_prepare()
 * or kernel_shutdown_prepare() */
static int wmt_dvfs_reboot(struct notifier_block *, unsigned long, void *);
static struct notifier_block wmt_reboot_notifier = {
	.notifier_call = wmt_dvfs_reboot,
};

static int wmt_suspend_target(unsigned target, unsigned relation, unsigned is_suspend);
static int wmt_dvfs_reboot(struct notifier_block *nb, unsigned long event, void *unused)
{
	unsigned target = 0xFFFFFFFF;
	struct cpufreq_policy *policy = cpufreq_cpu_get(0); /* boot CPU */

	/* find the highest freq in table */
	if (policy)
		target = policy->max;

	/* change freq to highest to make sure reboot use the max volatage */
	wmt_suspend_target(target, CPUFREQ_RELATION_H, 0);

	return NOTIFY_OK;
}

static int wmt_init_cpufreq_table(struct wmt_dvfs_driver_data *drv_data)
{
	int i = 0;
	struct wmt_dvfs_table *dvfs_tbl = NULL;
	struct cpufreq_frequency_table *freq_tbl = NULL;

	freq_tbl = kzalloc(sizeof(struct cpufreq_frequency_table) * (drv_data->tbl_num + 1),
				GFP_KERNEL);
	if (freq_tbl == NULL) {
		printk(KERN_ERR "%s: failed to allocate frequency table\n", __func__);
		return -ENOMEM;
	}

	drv_data->freq_table = freq_tbl;
	list_for_each_entry(dvfs_tbl, &drv_data->wmt_dvfs_list, node) {
		fq_dbg("freq_table[%d]:freq->%dKhz", i, dvfs_tbl->freq);
		freq_tbl[i].index = i;
		freq_tbl[i].frequency = dvfs_tbl->freq;
		i++;
	}
	/* the last element must be initialized to CPUFREQ_TABLE_END */
	freq_tbl[i].index = i;
	freq_tbl[i].frequency = CPUFREQ_TABLE_END;

	return 0;
}

/* wmt_getspeed return unit is Khz */
static unsigned int wmt_getspeed(unsigned int cpu)
{
	int freq = 0;

	if (cpu >= NR_CPUS)
		return 0;

	freq = auto_pll_divisor(DEV_ARM, GET_FREQ, 0, 0) / 1000;

	if (freq < 0)
		freq = 0;

	return freq;
}

static struct wmt_dvfs_table *find_freq_ceil(unsigned int *target)
{
	struct wmt_dvfs_table *dvfs_tbl = NULL;
	unsigned int freq = *target;

	list_for_each_entry(dvfs_tbl, &wmt_dvfs_drvdata.wmt_dvfs_list, node) {
		if (dvfs_tbl->freq >= freq) {
			*target = dvfs_tbl->freq;
			return dvfs_tbl;
		}
	}

	return NULL;
}

static struct wmt_dvfs_table *find_freq_floor(unsigned int *target)
{
	struct wmt_dvfs_table *dvfs_tbl = NULL;
	unsigned int freq = *target;

	list_for_each_entry_reverse(dvfs_tbl, &wmt_dvfs_drvdata.wmt_dvfs_list, node) {
		if (dvfs_tbl->freq <= freq) {
			*target = dvfs_tbl->freq;
			return dvfs_tbl;
		}
	}

	return NULL;
}

static struct wmt_dvfs_table *
wmt_recalc_target_freq(unsigned *target_freq, unsigned relation)
{
	struct wmt_dvfs_table *dvfs_tbl = NULL;

	if (!target_freq)
		return NULL;

	switch (relation) {
	case CPUFREQ_RELATION_L:
		/* Try to select a new_freq higher than or equal target_freq */
		dvfs_tbl = find_freq_ceil(target_freq);
		break;
	case CPUFREQ_RELATION_H:
		/* Try to select a new_freq lower than or equal target_freq */
		dvfs_tbl = find_freq_floor(target_freq);
		break;
	default:
		dvfs_tbl = NULL;
		break;
	}

	return dvfs_tbl;
}

/* this is a debug func, for checking pmc divf, divr, divq value */
static int get_arm_plla_param(void)
{
#ifdef DEBUG
	unsigned ft, df, dr, dq, div, tmp;

	tmp =  *(volatile unsigned int *)PMC_PLLA;
	fq_dbg("PMC PLLA REG IS 0x%08x\n", tmp);

	ft = (tmp >> 24) & 0x03; /*bit24 ~ bit26*/
	df = (tmp >> 16) & 0xFF; /*bit16 ~ bit23*/
	dr = (tmp >>  8) & 0x1F; /*bit8  ~ bit12*/
	dq = tmp & 0x03;

	tmp = *(volatile unsigned int *)ARM_DIV_OFFSET;
	div = tmp & 0x1F;

	fq_dbg("ft:%d, df:%d, dr:%d, dq:%d, div:%d\n", ft, df, dr, dq, div);
#endif
	return 0;
}

static int wmt_change_plla_table(struct wmt_dvfs_table *plla_table, unsigned relation)
{
	int ret = 0;
	int ret_vol = 0;
	struct plla_param plla_env;

	plla_env.plla_clk = (plla_table->freq / 1000);
	plla_env.arm_div = 1;
	plla_env.l2c_div = plla_table->l2c_div;
	plla_env.l2c_tag_div = plla_table->l2c_tag;
	plla_env.l2c_data_div = plla_table->l2c_data;
	plla_env.axi_div = plla_table->axi;
	plla_env.tb_index = plla_table->index;

	switch (relation) {
	case CPUFREQ_RELATION_L:
		if (use_dvfs_debug)
			printk(KERN_INFO "change to L %dKhz\n", plla_table->freq);
		ret = set_plla_divisor(&plla_env);
		if (plla_table->vol) {
			if (use_dvfs_debug)
				printk(KERN_INFO "pllal_table = %d\n", plla_table->vol*1000);
			if (use_regulator) {
				ret_vol = regulator_set_voltage(re, plla_table->vol*1000, plla_table->vol*1000);
				if (ret_vol < 0)
					printk(KERN_INFO "xxx set vol fail %d\n", ret_vol);
			}
		}
		break;
	case CPUFREQ_RELATION_H:
		if ((use_regulator == 1) && (regulator_is_enabled(re) < 0)) {
			if (use_dvfs_debug)
				printk(KERN_INFO "xxx regulator is disabled\n");
			ret = -EINVAL;
		} else {
			if (plla_table->vol) {
				if (use_dvfs_debug)
					printk(KERN_INFO "pllah_table = %d\n", plla_table->vol*1000);
				if (use_regulator)
					ret = regulator_set_voltage(re, plla_table->vol*1000, plla_table->vol*1000);			
			}
			if (ret >= 0) {
				if (use_dvfs_debug)
					printk(KERN_INFO "change to H %dKhz\n", plla_table->freq);
				ret = set_plla_divisor(&plla_env);
			} else
				printk(KERN_INFO "xxx set vol fail %d\n", ret);
		}
		break;
	default:
		break;
	}

	return ret;
}

/* target is calculated by cpufreq governor, unit Khz */
static int freq_save = 0; // unit Khz
static int wmt_target(struct cpufreq_policy *, unsigned, unsigned);
static int wmt_suspend_target(unsigned target, unsigned relation, unsigned is_suspend)
{
	unsigned int cur_freq = wmt_getspeed(0);
	int voltage = 0;
	
	down(&wmt_cpufreq_sem);

	/*
	 * Set to the highest voltage before suspend/reboot
	 */
	if (use_regulator) {
		if (use_dvfs_debug)
			printk("Set to Max. Voltage: %dmV\n", wmt_dvfs_drvdata.dvfs_table[(wmt_dvfs_drvdata.tbl_num - 1)].vol);
		regulator_set_voltage(re, wmt_dvfs_drvdata.dvfs_table[(wmt_dvfs_drvdata.tbl_num - 1)].vol * 1000,
					wmt_dvfs_drvdata.dvfs_table[(wmt_dvfs_drvdata.tbl_num - 1)].vol * 1000);
		voltage = regulator_get_voltage(re);
		printk("Current voltage = %d\n", voltage);
	}
	/* for some governor, like userspace, performance, powersaving,
	 * need change frequency to pre-suspend when resume */
	wmt_dvfs_running = 0;
	freq_save = cur_freq;
	up(&wmt_cpufreq_sem);

	return 0;
}

/*
 * Note that loops_per_jiffy is not updated on SMP systems in
 * cpufreq driver. So, update the per-CPU loops_per_jiffy value
 * on frequency transition. We need to update all dependent CPUs.
 */
#ifdef CONFIG_SMP
struct lpj_info {
	unsigned long	ref;
	unsigned int	freq;
};
static DEFINE_PER_CPU(struct lpj_info, lpj_ref);
static struct lpj_info global_lpj_ref;

static void
wmt_update_lpj(struct cpufreq_policy *policy, struct cpufreq_freqs freqs)
{
	unsigned int i = 0;

	for_each_cpu(i, policy->cpus) {
		struct lpj_info *lpj = &per_cpu(lpj_ref, i);
		if (!lpj->freq) {
			lpj->ref = per_cpu(cpu_data, i).loops_per_jiffy;
			lpj->freq = freqs.old;
		}
		per_cpu(cpu_data, i).loops_per_jiffy = cpufreq_scale(lpj->ref, 
														lpj->freq, freqs.new);
	}
	/* And don't forget to adjust the global one */
	if (!global_lpj_ref.freq) {
		global_lpj_ref.ref = loops_per_jiffy;
		global_lpj_ref.freq = freqs.old;
	}
	loops_per_jiffy = cpufreq_scale(global_lpj_ref.ref, 
									global_lpj_ref.freq, freqs.new);
}
#else
static void wmt_update_lpj(struct cpufreq_policy *policy, struct cpufreq_freqs freqs)
{
	return ;
}
#endif

/* target is calculated by cpufreq governor, unit Khz */
static int
wmt_target(struct cpufreq_policy *policy, unsigned target, unsigned relation)
{
	int ret = 0;
	unsigned int i = 0;
	struct cpufreq_freqs freqs;
	struct wmt_dvfs_table *dvfs_tbl = NULL;

	if (policy->cpu > NR_CPUS)
		return -EINVAL;

	fq_dbg("cpu freq:%dMhz now, need %s to %dMhz\n", wmt_getspeed(policy->cpu) / 1000,
				(relation == CPUFREQ_RELATION_L) ? "DOWN" : "UP", target / 1000);
	if (use_regulator) {
		if (regulator_is_enabled(re) < 0) {
			ret = -EBUSY;
			fq_dbg("regulator is disabled\n\n");
			goto out;
		}
	}

	/* Ensure desired rate is within allowed range.  Some govenors
	 * (ondemand) will just pass target_freq=0 to get the minimum. */
	
	if ((strncmp(policy->governor->name, "performance", CPUFREQ_NAME_LEN)) && (wmt_dvfs_drvdata.tbl_num > 2))
	{		
		if (policy->max >= wmt_dvfs_drvdata.dvfs_table[(wmt_dvfs_drvdata.tbl_num - 1)].freq)
			policy->max = wmt_dvfs_drvdata.dvfs_table[(wmt_dvfs_drvdata.tbl_num - 2)].freq;
		if (policy->min >= wmt_dvfs_drvdata.dvfs_table[(wmt_dvfs_drvdata.tbl_num - 1)].freq)
			policy->min = wmt_dvfs_drvdata.dvfs_table[(wmt_dvfs_drvdata.tbl_num - 2)].freq;
	}
	 
	if (target < policy->min)
		target = policy->min;
	if (target > policy->max)
		target = policy->max;

	/* find out (freq, voltage) pair to do dvfs */
	dvfs_tbl = wmt_recalc_target_freq(&target, relation);
	if (dvfs_tbl == NULL) {
		fq_dbg("Can not change to target_freq:%dKhz", target);
		ret = -EINVAL;
		goto out;
	}
	fq_dbg("recalculated target freq is %dMhz\n", target / 1000);

	freqs.cpu = policy->cpu;
	freqs.new = target;
	freqs.old = wmt_getspeed(policy->cpu);

	if (freqs.new == freqs.old) {
		ret = 0;
		goto out;
	} else if (freqs.new > freqs.old)
		relation = CPUFREQ_RELATION_H;
	else
		relation = CPUFREQ_RELATION_L;

	/* notifier for all cpus */
	for_each_cpu(i, policy->cpus) {
		freqs.cpu = i;
		cpufreq_notify_transition(&freqs, CPUFREQ_PRECHANGE);
	}
	
	/* actually we just scaling CPU frequency here */
	get_arm_plla_param();
	ret = wmt_change_plla_table(dvfs_tbl, relation);
	get_arm_plla_param();
	fq_dbg("change to %dKhz\n", ret / 1000);

	if (ret < 0) {
		ret = -EFAULT;
		fq_dbg("wmt_cpufreq: auto_pll_divisor failed\n");
	} else {
		wmt_update_lpj(policy, freqs);

		for_each_cpu(i, policy->cpus) {
			freqs.cpu = i;
			cpufreq_notify_transition(&freqs, CPUFREQ_POSTCHANGE);
		}
	}

out:
	fq_dbg("cpu freq scaled to %dMhz now\n\n", wmt_getspeed(policy->cpu) / 1000);
	return ret;
}

static int
wmt_dvfs_target(struct cpufreq_policy *policy, unsigned target, unsigned relation)
{
	int ret = 0;

	down(&wmt_cpufreq_sem);

	if (!wmt_dvfs_running) {
		ret = -ENODEV;
		fq_dbg("dvfs is not running now!\n");
		goto out;
	}

	ret = wmt_target(policy, target, relation);
	
out:
	up(&wmt_cpufreq_sem);
	fq_dbg("cpu freq scaled to %dMhz now\n\n", wmt_getspeed(policy->cpu) / 1000);
	return ret;
}


static int
wmt_cpufreq_pm_notify(struct notifier_block *nb, unsigned long event, void *dummy)
{
	struct cpufreq_policy *policy = cpufreq_cpu_get(0);
	int default_voltage = 0;
	struct plla_param plla_env;
	struct wmt_dvfs_table *plla_table = NULL;

	plla_table = &wmt_dvfs_drvdata.dvfs_table[0];

	plla_env.plla_clk = (plla_table->freq / 1000);
	plla_env.arm_div = 1;
	plla_env.l2c_div = plla_table->l2c_div;
	plla_env.l2c_tag_div = plla_table->l2c_tag;
	plla_env.l2c_data_div = plla_table->l2c_data;
	plla_env.axi_div = plla_table->axi;
	plla_env.tb_index = plla_table->index;

	if (event == PM_SUSPEND_PREPARE)
		wmt_suspend_target(0, CPUFREQ_RELATION_L, 1);
	else if (event == PM_POST_SUSPEND) {
		if (use_regulator) {
			default_voltage = regulator_get_voltage(re);
			if (use_dvfs_debug)
				printk("default_voltage = %d\n", default_voltage);
			regulator_set_voltage(re, default_voltage, default_voltage);
			if (use_dvfs_debug)
				printk("Set Min. Freq = %d\n", plla_env.plla_clk);
			set_plla_divisor(&plla_env);
		}
		down(&wmt_cpufreq_sem);
		wmt_dvfs_running = 1;
		wmt_target(policy, freq_save, CPUFREQ_RELATION_H);
		up(&wmt_cpufreq_sem);
		/* when resume back, changed to suspended freq */
	}

	return NOTIFY_OK;
}

static struct notifier_block wmt_cpu_pm_notifier = {
	.notifier_call = wmt_cpufreq_pm_notify,
};

static int wmt_verify_speed(struct cpufreq_policy *policy)
{
	int ret = 0;
	struct cpufreq_frequency_table *freq_tbl = wmt_dvfs_drvdata.freq_table;

	if (policy->cpu >= NR_CPUS)
		return -EINVAL;

	if (NULL == freq_tbl)
		ret = -EINVAL;
	else
		ret = cpufreq_frequency_table_verify(policy, freq_tbl);

	return ret;
}

static int wmt_cpu_init(struct cpufreq_policy *policy)
{
	int ret = 0;
	struct cpufreq_frequency_table *wmt_freq_tbl = NULL;

	fq_dbg("CPU%d cpufreq init\n", policy->cpu);
	if (policy->cpu >= NR_CPUS)
		return -EINVAL;

	wmt_freq_tbl = wmt_dvfs_drvdata.freq_table;
	if (!wmt_freq_tbl)
		return -EINVAL;

	policy->cur = wmt_getspeed(policy->cpu);
	policy->min = wmt_getspeed(policy->cpu);
	policy->max = wmt_getspeed(policy->cpu);

	/*
	 * check each frequency and find max_freq
	 * min_freq in the table, then set:
	 *  policy->min = policy->cpuinfo.min_freq = min_freq;
	 *  policy->max = policy->cpuinfo.max_freq = max_freq;
	 */
	ret = cpufreq_frequency_table_cpuinfo(policy, wmt_freq_tbl);
	if (0 == ret)
		cpufreq_frequency_table_get_attr(wmt_freq_tbl, policy->cpu);

	/*
	 * On ARM-CortaxA9 configuartion, both processors share the voltage
	 * and clock. So both CPUs needs to be scaled together and hence
	 * needs software co-ordination. Use cpufreq affected_cpus
	 * interface to handle this scenario. Additional is_smp() check
	 * is to keep SMP_ON_UP build working.
	 */
	if (is_smp()) {
	    policy->shared_type = CPUFREQ_SHARED_TYPE_ANY;
	    cpumask_setall(policy->cpus);
	}
	/* only CPU0 can enable dvfs when cpufreq init.
	 * when cpu hotplug enable, cpu1 will re-init when plugin */
	if (!policy->cpu) {
		down(&wmt_cpufreq_sem);
		wmt_dvfs_running = 1;
		wmt_target(policy, policy->max, CPUFREQ_RELATION_H);
		up(&wmt_cpufreq_sem);
	}
	/*
	 * 1. make sure current frequency be covered in cpufreq_table
	 * 2. change cpu frequency to policy-max for fast booting
	 */

	policy->cur = wmt_getspeed(policy->cpu);
	policy->cpuinfo.transition_latency = wmt_dvfs_drvdata.sample_rate;

	fq_dbg("CPU%d,p->max:%d, p->min:%d, c->max=%d, c->min=%d, current:%d\n",
			policy->cpu, policy->max, policy->min, policy->cpuinfo.max_freq,
			policy->cpuinfo.min_freq, wmt_getspeed(policy->cpu));

	return ret;
}

static struct freq_attr *wmt_cpufreq_attr[] = {
	&cpufreq_freq_attr_scaling_available_freqs,
	NULL,
};

static struct cpufreq_driver wmt_cpufreq_driver = {
	.name       = "wmt_cpufreq",
	.owner      = THIS_MODULE,
	.flags		= CPUFREQ_STICKY,
	.init       = wmt_cpu_init,
	.verify     = wmt_verify_speed,
	.target     = wmt_dvfs_target,
	.get        = wmt_getspeed,
	.attr       = wmt_cpufreq_attr,
};

static int __init wmt_cpufreq_check_env(void)
{
	int i = 0;
	int ret = 0;
	int varlen = 512;
	char *ptr = NULL;
	unsigned int tbl_num = 0;
	unsigned int drv_en = 0;
	unsigned int sample_rate = 0;
	unsigned int freq = 0;
	unsigned int voltage = 0;
	unsigned int l2c_div = 0;
	unsigned int l2c_tag = 0;
	unsigned int l2c_data = 0;
	unsigned int axi = 0;
	struct wmt_dvfs_table *wmt_dvfs_tbl = NULL;
	unsigned char buf[512] = {0};
	char	f_is_bonding = 0;

	/* uboot env name is: wmt.cpufreq.param, format is:
	<enable>:<sample_rate>:<table_number>:<[freq,voltage,l2c_div,l2c_tag,l2c_data,axi]
	*/

	use_dvfs = 0;
	use_dvfs_debug = 0;

	if (BONDING_OPTION_4BYTE_VAL & 0x8)
		f_is_bonding = 1;
	
	if (f_is_bonding)
		ret = wmt_getsyspara("wmt.cpufreqlp.param", buf, &varlen);	
	else
		ret = wmt_getsyspara("wmt.cpufreq.param", buf, &varlen);
		
	if (ret) {
		printk(KERN_INFO "Can not find uboot env wmt.cpufreq.param\n");
		ret = -ENODATA;
		goto out;
	}
	fq_dbg("wmt.cpufreq.param:%s\n", buf);

	sscanf(buf, "%x:%d:%d", &drv_en, &sample_rate, &tbl_num);
	if (!drv_en) {
		printk(KERN_INFO "wmt cpufreq disaled\n");
		ret = -ENODEV;
		goto out;
	}

	/* 2 dvfs table at least */
	if (tbl_num < DVFS_TABLE_NUM_MIN) {
		printk(KERN_INFO "No frequency information found\n");
		ret = -ENODATA;
		goto out;
	}
	if (tbl_num > DVFS_TABLE_NUM_MAX)
		tbl_num = DVFS_TABLE_NUM_MAX;

	wmt_dvfs_tbl = kzalloc(sizeof(struct wmt_dvfs_table) * tbl_num, GFP_KERNEL);
	if (!wmt_dvfs_tbl) {
		ret = -ENOMEM;
		goto out;
	}
	wmt_dvfs_drvdata.tbl_num = tbl_num;
	wmt_dvfs_drvdata.sample_rate = sample_rate * MILLISEC_TO_MICROSEC;
	wmt_dvfs_drvdata.dvfs_table = wmt_dvfs_tbl;
	INIT_LIST_HEAD(&wmt_dvfs_drvdata.wmt_dvfs_list);

	/* copy freq&vol info from uboot env to wmt_dvfs_table */
	ptr = buf;
	for (i = 0; i < tbl_num; i++) {
		strsep(&ptr, "[");
		sscanf(ptr, "%d,%d,%d,%d,%d,%d]:[", &freq, &voltage, &l2c_div, &l2c_tag, &l2c_data, &axi);
		wmt_dvfs_tbl[i].freq = freq*1000;
		wmt_dvfs_tbl[i].vol  = voltage;
		wmt_dvfs_tbl[i].l2c_div = l2c_div;
		wmt_dvfs_tbl[i].l2c_tag  = l2c_tag;
		wmt_dvfs_tbl[i].l2c_data = l2c_data;
		wmt_dvfs_tbl[i].axi = axi;
		wmt_dvfs_tbl[i].index = i;
		INIT_LIST_HEAD(&wmt_dvfs_tbl[i].node);
		list_add_tail(&wmt_dvfs_tbl[i].node, &wmt_dvfs_drvdata.wmt_dvfs_list);
		fq_dbg("dvfs_table[%d]: freq %dMhz, voltage %dmV l2c_div %d l2c_tag %d l2c_data %d axi %d\n",
			i, freq, voltage, l2c_div, l2c_tag, l2c_data, axi);
//		printk("dvfs_table[%d]: freq %dMhz, voltage %dmV l2c_div %d l2c_tag %d l2c_data %d axi %d \n",
//			i, freq, voltage, l2c_div, l2c_tag, l2c_data, axi);//gri
	}
	use_dvfs = 1;

	if (drv_en & 0x10)
		use_dvfs_debug = 1;

out:
	return ret;
}

static int __init wmt_cpufreq_driver_init(void)
{
	int ret = 0;
	unsigned int chip_id = 0;
	unsigned int bondingid = 0;
	struct cpufreq_frequency_table *wmt_freq_tbl = NULL;

	wmt_dvfs_running = 0;
	sema_init(&wmt_cpufreq_sem, 1);
	wmt_getsocinfo(&chip_id, &bondingid);

	/* if cpufreq disabled, cpu will always run at current frequency
	 * which defined in wmt.plla.param */
	ret = wmt_cpufreq_check_env();
	if (ret) {
		printk(KERN_WARNING "wmt_cpufreq check env failed, current cpu "
				"frequency is %dKhz\n", wmt_getspeed(0));
		goto out;
	}
	/* copy dvfs info from uboot to cpufreq table, 
		generate cpu frequency table here */
	wmt_init_cpufreq_table(&wmt_dvfs_drvdata);
	wmt_freq_tbl = wmt_dvfs_drvdata.freq_table;
	if (NULL == wmt_freq_tbl) {
		printk(KERN_ERR "wmt_cpufreq create wmt_freq_tbl failed\n");
		ret = -EINVAL;
		goto out;
	}

	register_reboot_notifier(&wmt_reboot_notifier);
	register_pm_notifier(&wmt_cpu_pm_notifier);

	use_regulator = 1;
	re = regulator_get(NULL, "wmt_corepower");
	if (IS_ERR(re))
		use_regulator = 0;
	printk("[CPU-FREQ]use_regulator = %d\n", use_regulator);
	ret = cpufreq_register_driver(&wmt_cpufreq_driver);

out:
	return ret;
}
late_initcall(wmt_cpufreq_driver_init);

MODULE_AUTHOR("WonderMedia Technologies, Inc");
MODULE_DESCRIPTION("WMT CPU frequency driver");
MODULE_LICENSE("Dual BSD/GPL");
