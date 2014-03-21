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
#include <linux/types.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/cpufreq.h>
#include <linux/delay.h>
#include <linux/clkdev.h>
#include <linux/clk-private.h>
#include <linux/clk-provider.h>
#include <mach/hardware.h>

//#define DEBUG
#ifdef DEBUG
#define fq_dbg(fmt, args...) \
		printk(KERN_ERR "[%s]_%d_%d: " fmt, __func__ , __LINE__, smp_processor_id(), ## args)
#define fq_trace() printk(KERN_ERR "trace in %s %d\n", __func__, __LINE__)
#else
#define fq_dbg(fmt, args...)
#define fq_trace()
#endif

#define DEV_NAME  "smp_twd"

static struct clk_lookup twd_lookup = {
	.dev_id = DEV_NAME,
};

static int wmt_twd_clk_enable(struct clk_hw *hw)
{
	return 0;
}

static void wmt_twd_clk_disable(struct clk_hw *hw)
{
	return ;
}

static int wmt_twd_clk_is_enabled(struct clk_hw *hw)
{
	return true;
}

static unsigned long 
wmt_twd_clk_recalc_rate(struct clk_hw *hw, unsigned long parent_rate)
{
	int freq = 0;

	/* return the ARM CPU clock rate */
	freq = auto_pll_divisor(DEV_ARM, GET_CPUTIMER, 0, 0) / 2;

	if (freq < 0)
		freq = 0;

	return freq;
}

static struct clk_ops wmt_twd_clk_ops = {
	.enable		 = wmt_twd_clk_enable,
	.disable	 = wmt_twd_clk_disable,
	.is_enabled	 = wmt_twd_clk_is_enabled,
	.recalc_rate = wmt_twd_clk_recalc_rate,
};

static int wmt3498_register_twd_clk(struct clk_ops *ops, struct clk_lookup *cl)
{
	struct clk_hw *hw = NULL;

	clkdev_add(cl);

	/* register twd clk here */
	hw = kzalloc(sizeof(*hw), GFP_KERNEL);
	if (!hw) {
		printk(KERN_ERR "Register smp_twd clock failed!\n");
		return -ENOMEM;
	}

	cl->clk = clk_register(NULL, DEV_NAME, ops, hw, NULL, 0, CLK_IS_ROOT);

	if (!cl->clk) {
		printk(KERN_ERR "Register smp_twd clock failed!\n");
		kfree(hw);
		return -ENOMEM;
	}

	return 0;    
}

static void wmt3498_register_clocks(void)
{
	wmt3498_register_twd_clk(&wmt_twd_clk_ops, &twd_lookup);

	return ;
}

static int wmt3498_setup_twd_clk(void)
{
	return 0;
}

static void wmt3498_setup_clocks(void)
{
	wmt3498_setup_twd_clk();

	return ;
}

/* this func should be called in machine early_init or io_map */
void wmt3498_init_clocks(void)
{
	wmt3498_register_clocks();
	wmt3498_setup_clocks();

	return ;
}

/* updates twd frequency when the cpu frequency changes. */
static int wmt_twd_cpufreq_transition(struct notifier_block *nb,
										unsigned long state, void *data)
{
	struct clk *wmt_twd_clk = twd_lookup.clk;
	struct cpufreq_freqs *freqs = data;	

	if (!wmt_twd_clk)
		goto out;

	if (state == CPUFREQ_POSTCHANGE || state == CPUFREQ_RESUMECHANGE) {
		// only boot-cpu do clk rate update
		if (freqs->cpu == 0) {
			wmt_twd_clk->rate = auto_pll_divisor(DEV_ARM, GET_CPUTIMER, 0, 0) / 2;
			wmt_twd_clk->new_rate = wmt_twd_clk->rate;
		}
	}

out:
	return NOTIFY_OK;
}

static struct notifier_block wmt_twd_cpufreq_nb = {
	.notifier_call = wmt_twd_cpufreq_transition,
	.priority = 10, /* higher than twd */
};

static int wmt_twd_cpufreq_init(void)
{
	return cpufreq_register_notifier(&wmt_twd_cpufreq_nb, CPUFREQ_TRANSITION_NOTIFIER);
}
core_initcall(wmt_twd_cpufreq_init);

MODULE_AUTHOR("WonderMedia Technologies, Inc");
MODULE_DESCRIPTION("WMT Common Clock Driver");
MODULE_LICENSE("Dual BSD/GPL");
