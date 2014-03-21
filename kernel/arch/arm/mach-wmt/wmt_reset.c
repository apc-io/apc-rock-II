/*++
linux/arch/arm/mach-wmt/wmt_reset.c

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
#include <linux/module.h>
#include <mach/hardware.h>
#include <asm/system.h>
#include <asm/cacheflush.h>

extern void setup_mm_for_reboot(void);
extern void call_with_stack(void (*fn)(void *), void *arg, void *sp);

/*
 * A temporary stack to use for CPU reset. This is static so that we
 * don't clobber it with the identity mapping. When running with this
 * stack, any references to the current task *will not work* so you
 * should really do as little as possible before jumping to your reset
 * code.
 */
static u64 soft_restart_stack[16];

void check_busy(void) {
	unsigned int tmp, tmp1 = 0x1000000;
	while (tmp1) {
		tmp = PMCS2_VAL;
		if (!(tmp & 0x7F0038))
			break;

		tmp1--;
		if (!tmp1)
			printk("wait PLL divisor ready fail -- check busy halted\n");
	}
}

static void wmt__soft_restart(void *addr)
{
	unsigned int tmp;
	//phys_reset_t phys_reset;

	/* Take out a flat memory mapping. */
	setup_mm_for_reboot();

	/* Clean and invalidate caches */
	flush_cache_all();

	/* Turn off caching */
	cpu_proc_fin();

	/* Push out any further dirty data, and ensure cache is empty */
	flush_cache_all();

	/* Switch to the identity mapping. */
	/*phys_reset = (phys_reset_t)(unsigned long)virt_to_phys(cpu_reset);
	phys_reset((unsigned long)addr);*/

	check_busy();
	PMPMC_VAL = 0x00170003;

	check_busy();
	tmp = (STRAP_STATUS_VAL>>12)&3;

	check_busy();
	PMARM_VAL = 2;

	check_busy();
	tmp = (STRAP_STATUS_VAL>>12)&3;
	if (tmp == 2)
		PMPMA_VAL = 0x00520101;//996
	else if (tmp == 1)
		PMPMA_VAL = 0x00420101;//804
	else //if (tmp == 0)
		PMPMA_VAL = 0x00420102;//402
	check_busy();

	/* Use on-chip reset capability */
	PMSR_VAL = PMSR_SWR;

	/* Should never get here. */
	BUG();
}

void wmt_soft_restart(unsigned long addr)
{
	u64 *stack = soft_restart_stack + ARRAY_SIZE(soft_restart_stack);

	/* Disable interrupts first */
	local_irq_disable();
	local_fiq_disable();

	/* Disable the L2 if we're the last man standing. */
	if (num_online_cpus() == 1)
		outer_disable();

	/* Change to the new stack and continue with the reset. */
	call_with_stack(wmt__soft_restart, (void *)addr, (void *)stack);

	/* Should never get here. */
	BUG();
}

void wmt_restart(char mode, const char *cmd)
{
	wmt_soft_restart(mode);
	
	/* Should never get here. */
	BUG();
}
EXPORT_SYMBOL(wmt_restart);