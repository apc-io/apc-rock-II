/**
 * @file timer_int.c
 *
 * @remark Copyright 2002 OProfile authors
 * @remark Read the file COPYING
 *
 * @author John Levon <levon@movementarian.org>
 */

#include <linux/kernel.h>
#include <linux/notifier.h>
#include <linux/smp.h>
#include <linux/oprofile.h>
#include <linux/profile.h>
#include <linux/init.h>
#include <linux/cpu.h>
#include <linux/hrtimer.h>
#include <asm/irq_regs.h>
#include <asm/ptrace.h>

/* 2013-01-14 YJChen: Add Begin */
#include <asm/mach/irq.h>
#include <linux/rtc.h>
#include <mach/hardware.h>
/* 2013-01-14 YJChen: Add End */

#include "oprof.h"

static DEFINE_PER_CPU(struct hrtimer, oprofile_hrtimer);
static int ctr_running;

static enum hrtimer_restart oprofile_hrtimer_notify(struct hrtimer *hrtimer)
{
	oprofile_add_sample(get_irq_regs(), 0);
	hrtimer_forward_now(hrtimer, ns_to_ktime(TICK_NSEC));
	return HRTIMER_RESTART;
}

static void __oprofile_hrtimer_start(void *unused)
{
	struct hrtimer *hrtimer = &__get_cpu_var(oprofile_hrtimer);

	if (!ctr_running)
		return;

	hrtimer_init(hrtimer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	hrtimer->function = oprofile_hrtimer_notify;

	hrtimer_start(hrtimer, ns_to_ktime(TICK_NSEC),
		      HRTIMER_MODE_REL_PINNED);
}

static int oprofile_hrtimer_start(void)
{
	get_online_cpus();
	ctr_running = 1;
	on_each_cpu(__oprofile_hrtimer_start, NULL, 1);
	put_online_cpus();
	return 0;
}

static void __oprofile_hrtimer_stop(int cpu)
{
	struct hrtimer *hrtimer = &per_cpu(oprofile_hrtimer, cpu);

	if (!ctr_running)
		return;

	hrtimer_cancel(hrtimer);
}

static void oprofile_hrtimer_stop(void)
{
	int cpu;

	get_online_cpus();
	for_each_online_cpu(cpu)
		__oprofile_hrtimer_stop(cpu);
	ctr_running = 0;
	put_online_cpus();
}

static int __cpuinit oprofile_cpu_notify(struct notifier_block *self,
					 unsigned long action, void *hcpu)
{
	long cpu = (long) hcpu;

	switch (action) {
	case CPU_ONLINE:
	case CPU_ONLINE_FROZEN:
		smp_call_function_single(cpu, __oprofile_hrtimer_start,
					 NULL, 1);
		break;
	case CPU_DEAD:
	case CPU_DEAD_FROZEN:
		__oprofile_hrtimer_stop(cpu);
		break;
	}
	return NOTIFY_OK;
}

static struct notifier_block __refdata oprofile_cpu_notifier = {
	.notifier_call = oprofile_cpu_notify,
};

static int oprofile_hrtimer_setup(void)
{
	return register_hotcpu_notifier(&oprofile_cpu_notifier);
}

static void oprofile_hrtimer_shutdown(void)
{
	unregister_hotcpu_notifier(&oprofile_cpu_notifier);
}

int oprofile_timer_init(struct oprofile_operations *ops)
{
	ops->create_files	= NULL;
	ops->setup		= oprofile_hrtimer_setup;
	ops->shutdown	= oprofile_hrtimer_shutdown;
	ops->start		= oprofile_hrtimer_start;
	ops->stop		= oprofile_hrtimer_stop;
	ops->cpu_type	= "timer";
	printk(KERN_INFO "oprofile: using timer interrupt.\n");
	return 0;
}

/* 2013-01-14 YJChen: Add Begin */
extern unsigned int wmt_read_oscr(void);

static int timer_notify(struct pt_regs *regs)
{
	oprofile_add_sample(regs, 0);
	return 0;
}

static irqreturn_t
wmt_timer2_interrupt(int irq, void *dev_id)
{
    unsigned int next_match;
    int flag = 0;
    do {
        struct pt_regs *regs;

        if (flag == 1)
            printk(KERN_INFO "wmt_timer2_interrupt while\n");

        flag = 1;

        //struct pt_regs *regs = get_irq_regs(); // fix compile warning: ISO C90 forbids mixed declarations and code
        regs = get_irq_regs();
        timer_notify(regs);

        /* Clear match on OS Timer 2 */
        OSTS_VAL = OSTS_M2;
        next_match = OSM2_VAL = wmt_read_oscr() + TIMER2_LATCH;
    } while ((signed long)(next_match - wmt_read_oscr()) <= 10/*0*/);

    return IRQ_HANDLED;
}

static int timer2_start(void)
{
    unsigned int nCount;
    unsigned int i = 0;

    /* Use OS Timer 2 as oprofile timer. */
    i = request_irq(IRQ_OST2, wmt_timer2_interrupt, IRQF_DISABLED, "timer2", NULL);
    if (i != 0) {
        printk(KERN_ERR "oprofile: unable to request IRQ%u for timer2\n", IRQ_OST2);
        return i;
    }

    nCount = wmt_read_oscr();
    //printk(KERN_INFO "%s Enter nCount=%d\n", __FUNCTION__, nCount);

    /* Set initial match */
    while (OSTA_VAL & OSTA_MWA2)
        ;

    /* Disable match on timer 2 to cause interrupts. */
    OSTI_VAL &= ~OSTI_E2;

    OSM2_VAL = nCount + TIMER2_LATCH;

    /* Enable match on timer 2 to cause interrupts. */
    OSTI_VAL |= OSTI_E2;

    /* Let OS Timer free run. */
    if ((OSTC_VAL & OSTC_ENABLE) == 0)
        OSTC_VAL |= OSTC_ENABLE;

	return 0;
}

static void timer2_stop(void)
{
    /* Disable match on timer 2 to cause interrupts. */
    OSTI_VAL &= ~OSTI_E2;

    /* Set initial match */
    while (OSTA_VAL & OSTA_MWA2)
        ;

    OSM2_VAL = 0;

    free_irq(IRQ_OST2, NULL);
}

int __init oprofile_timer2_init(struct oprofile_operations *ops)
{
	int rc;

	rc = register_hotcpu_notifier(&oprofile_cpu_notifier);
	if (rc)
		return rc;

    ops->create_files = NULL;
    ops->setup = oprofile_hrtimer_setup;
    ops->shutdown = oprofile_hrtimer_shutdown;
    ops->start = timer2_start;
    ops->stop = timer2_stop;
    ops->cpu_type = "timer";
	printk(KERN_INFO "oprofile: using timer2 interrupt.\n");
    return 0;
}
/* 2013-01-14 YJChen: Add End */