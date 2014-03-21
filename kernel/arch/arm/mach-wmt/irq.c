/*++
linux/arch/arm/mach-wmt/irq.c

IRQ settings for WMT

Copyright (c) 2009 WonderMedia Technologies, Inc.

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
#include <linux/irq.h>
#include <linux/io.h>
#include <linux/of.h>
#include <asm/hardware/gic.h>


/*
 * Follows are handlers for normal irq_chip
 */
static void wmt_mask_irq(struct irq_data *data)
{

}

static void wmt_unmask_irq(struct irq_data *data)
{

}

static void wmt_ack_irq(struct irq_data *data)
{

}

#ifdef CONFIG_OF
static const struct of_device_id wmt_dt_gic_match[] __initconst = {
	{ .compatible = "arm,cortex-a9-gic", .data = gic_of_init },
	{ }
};
#endif

void __init wmt_init_irq(void)
{
	gic_arch_extn.irq_ack = wmt_ack_irq;
	gic_arch_extn.irq_mask = wmt_mask_irq;
	gic_arch_extn.irq_unmask = wmt_unmask_irq;

	if (!of_have_populated_dt())
		gic_init(0, 29, (void __iomem *)WMT_GIC_DIST_BASE, (void __iomem *)WMT_GIC_CPU_BASE);
#ifdef CONFIG_OF
	else
		of_irq_init(wmt_dt_gic_match);
#endif
}
