/* linux/arch/arm/mach-wmt/gpio.c
 *
 * Copyright (c) 2013 WonderMedia Technologies, Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#include <linux/module.h>
#include <linux/device.h>
#include <linux/gpio.h>
#include <linux/init.h>
#include <linux/irq.h>
#include <linux/io.h>
#include <linux/seq_file.h>
#include <linux/kobject.h>
#include <linux/debugfs.h>

#include <mach/hardware.h>
#include <mach/wmt_iomux.h>

#undef	WMT_PIN
#define WMT_PIN(__gp, __bit, __irq, __name)	\
	{ .label = #__name, .regoff = __gp, .shift = __bit, .irqnum = __irq },

const static struct wmt_gpio {
	const char *label;
	unsigned char regoff;
	unsigned char shift;
	int irqnum;
} wmt_gpios[] = {
	#include <mach/iomux.h>
};

#define to_wmt(__chip)	container_of(__chip, struct wmt_gpio_port, chip)

struct wmt_gpio_port {
	uint32_t base;
	uint32_t gpio_irq_no_base;
	int gpio_irq_nr;
	struct gpio_chip chip;
	spinlock_t lock;
};

static struct wmt_gpio_port wmt_gpio_port;

#define INVALUE_REGS		0x00
#define ENABLE_REGS		0x40
#define DIRECTION_REGS		0x80
#define OUTVALUE_REGS		0xc0

#define INTMASK_REGS		0x300
#define INTSTAT_REGS		0x360

#define PULLENABLE_REGS		0x480
#define PULLCONTROL_REGS	0x4c0

#define GPIO_INT_LOW_LEV	0x0
#define GPIO_INT_HIGH_LEV	0x1
#define GPIO_INT_FALL_EDGE	0x2
#define GPIO_INT_RISE_EDGE	0x3
#define GPIO_INT_BOTH_EDGE	0x4

#define CHIP_GPIO_BASE	0

static void _clear_gpio_irqstatus(u32 irqindex)
{
	struct wmt_gpio_port *port = &wmt_gpio_port;
	u32 addr = port->base + INTSTAT_REGS + (irqindex >> 3);
	u8 index = irqindex & 0x7;

	__raw_writeb(1 << index, addr);
}

static int _gpio_irqstatus(u32 irqindex)
{
	u8 l;
	struct wmt_gpio_port *port = &wmt_gpio_port;
	u32 addr = port->base + INTSTAT_REGS + (irqindex >> 3);
	u8 index = irqindex & 0x7;
	l = __raw_readb(addr);
	return l & (1<<index);
}

int gpio_irqstatus(unsigned int gpio)
{
	int offset = gpio - CHIP_GPIO_BASE;
	int irqindex = wmt_gpios[offset].irqnum;	
	return _gpio_irqstatus(irqindex);
}
EXPORT_SYMBOL(gpio_irqstatus);

static void _set_gpio_irqenable(u32 irqindex, int enable)
{
	struct wmt_gpio_port *port = &wmt_gpio_port;
	u32 addr = port->base + INTMASK_REGS + irqindex;
	u8 l;

	l = __raw_readb(addr);
	l = (l & 0x7f) | (!!enable << 7);
	__raw_writeb(l, addr);
}

int is_gpio_irqenable(unsigned int gpio)
{
	u8 l;
	int offset = gpio - CHIP_GPIO_BASE;
	int irqindex = wmt_gpios[offset].irqnum;
	struct wmt_gpio_port *port = &wmt_gpio_port;
	u32 addr = port->base + INTMASK_REGS + irqindex;
	
	l = __raw_readb(addr);
	return (l & 0x80);
}
EXPORT_SYMBOL(is_gpio_irqenable);

void wmt_gpio_ack_irq(unsigned int gpio)
{
	int offset = gpio - CHIP_GPIO_BASE;
	int irqindex = wmt_gpios[offset].irqnum;

	_clear_gpio_irqstatus(irqindex);
}
EXPORT_SYMBOL(wmt_gpio_ack_irq);

void wmt_gpio_mask_irq(unsigned int gpio)
{
	int offset = gpio - CHIP_GPIO_BASE;
	int irqindex = wmt_gpios[offset].irqnum;

	_set_gpio_irqenable(irqindex, 0);
}
EXPORT_SYMBOL(wmt_gpio_mask_irq);

void wmt_gpio_unmask_irq(unsigned int gpio)
{
	int offset = gpio - CHIP_GPIO_BASE;
	int irqindex = wmt_gpios[offset].irqnum;

	_set_gpio_irqenable(irqindex, 1);
}
EXPORT_SYMBOL(wmt_gpio_unmask_irq);

int wmt_gpio_set_irq_type(unsigned int gpio, u32 type)
{
	struct wmt_gpio_port *port = &wmt_gpio_port;
	int offset = gpio - CHIP_GPIO_BASE;
	int irqindex = wmt_gpios[offset].irqnum;
	int edge;
	u32 reg, val;

	switch (type) {
	case IRQ_TYPE_EDGE_RISING:
		edge = GPIO_INT_RISE_EDGE;
		break;
	case IRQ_TYPE_EDGE_FALLING:
		edge = GPIO_INT_FALL_EDGE;
		break;
	case IRQ_TYPE_EDGE_BOTH:
		edge = GPIO_INT_BOTH_EDGE;
		break;
	case IRQ_TYPE_LEVEL_LOW:
		edge = GPIO_INT_LOW_LEV;
		break;
	case IRQ_TYPE_LEVEL_HIGH:
		edge = GPIO_INT_HIGH_LEV;
		break;
	default:
		return -EINVAL;
	}

	reg  = port->base + INTMASK_REGS + irqindex;
	val = __raw_readb(reg) & 0xf8;
	__raw_writeb(val | edge, reg);

	_clear_gpio_irqstatus(irqindex);

	return 0;
}
EXPORT_SYMBOL(wmt_gpio_set_irq_type);

#ifdef WMT_GPIO_IRQ
static void gpio_ack_irq(struct irq_data *d)
{
	struct wmt_gpio_port *port = &wmt_gpio_port;
	int irqindex = d->irq - port->gpio_irq_no_base;

	_clear_gpio_irqstatus(irqindex);
}

static void gpio_mask_irq(struct irq_data *d)
{
	struct wmt_gpio_port *port = &wmt_gpio_port;
	int irqindex = d->irq - port->gpio_irq_no_base;

	_set_gpio_irqenable(irqindex, 0);
}

static void gpio_unmask_irq(struct irq_data *d)
{
	struct wmt_gpio_port *port = &wmt_gpio_port;
	int irqindex = d->irq - port->gpio_irq_no_base;

	_set_gpio_irqenable(irqindex, 1);
}

static int gpio_set_irq_type(struct irq_data *d, u32 type)
{
	struct wmt_gpio_port *port = &wmt_gpio_port;
	int edge, irqindex;
	u32 reg, val;

	switch (type) {
	case IRQ_TYPE_EDGE_RISING:
		edge = GPIO_INT_RISE_EDGE;
		break;
	case IRQ_TYPE_EDGE_FALLING:
		edge = GPIO_INT_FALL_EDGE;
		break;
	case IRQ_TYPE_EDGE_BOTH:
		edge = GPIO_INT_BOTH_EDGE;
		break;
	case IRQ_TYPE_LEVEL_LOW:
		edge = GPIO_INT_LOW_LEV;
		break;
	case IRQ_TYPE_LEVEL_HIGH:
		edge = GPIO_INT_HIGH_LEV;
		break;
	default:
		return -EINVAL;
	}

	irqindex = d->irq - port->gpio_irq_no_base;
	reg  = port->base + INTMASK_REGS + irqindex;
	val = __raw_readb(reg) & 0xf8;
	__raw_writeb(val | edge, reg);

	_clear_gpio_irqstatus(irqindex);

	return 0;
}

/* WMT has one interrupt *for all* gpio ports */
static void wmt_gpio_irq_handler(u32 irq, struct irq_desc *desc)
{
	int i;
	u32 irq_msk, irq_stat;
	struct wmt_gpio_port *port = &wmt_gpio_port;

	/* walk through all interrupt status registers */
	for (i = 0; i < 4; i++) {
		irq_msk = __raw_readl(port->base + INTMASK_REGS + i*4);
		if (!irq_msk)
			continue;

		irq_stat = __raw_readl(port->base + INTSTAT_REGS + i*4) & irq_msk;
		while (irq_stat != 0) {
			int irqoffset = fls(irq_stat) - 1;

			generic_handle_irq(port->gpio_irq_no_base + i*32 + irqoffset);

			irq_stat &= ~(1 << irqoffset);
		}
	}
}

static struct irq_chip gpio_irq_chip = {
	.name = "GPIO",
	.irq_ack = gpio_ack_irq,
	.irq_mask = gpio_mask_irq,
	.irq_unmask = gpio_unmask_irq,
	.irq_set_type = gpio_set_irq_type,
};
#endif

static int wmt_gpio_request(struct gpio_chip *chip, unsigned offset)
{
	struct wmt_gpio_port *port = to_wmt(chip);
	uint32_t base = port->base;
	uint8_t regoff = wmt_gpios[offset].regoff;
	uint8_t shift  = wmt_gpios[offset].shift;
	uint8_t val;

	unsigned long flags;

	spin_lock_irqsave(&port->lock, flags);

	WARN_ON(offset >= ARRAY_SIZE(wmt_gpios));

	val = readb(base + ENABLE_REGS + regoff);
	val |= (1 << shift);
	writeb(val, base + ENABLE_REGS + regoff);

	spin_unlock_irqrestore(&port->lock, flags);
	return 0;
}

static void wmt_gpio_free(struct gpio_chip *chip, unsigned offset)
{
	struct wmt_gpio_port *port = to_wmt(chip);
//	uint32_t base = port->base;
//	uint8_t regoff = wmt_gpios[offset].regoff;
//	uint8_t shift  = wmt_gpios[offset].shift;
//	uint8_t val;

	unsigned long flags;

	spin_lock_irqsave(&port->lock, flags);

	WARN_ON(offset >= ARRAY_SIZE(wmt_gpios));

//	val = readb(base + ENABLE_REGS + regoff);
//	val &= ~(1 << shift);
//	writeb(val, base + ENABLE_REGS + regoff);

	spin_unlock_irqrestore(&port->lock, flags);
}

static void _set_gpio_direction(struct gpio_chip *chip, unsigned offset, int dir)
{
	struct wmt_gpio_port *port = to_wmt(chip);
	uint32_t base = port->base;
	uint8_t regoff = wmt_gpios[offset].regoff;
	uint8_t shift  = wmt_gpios[offset].shift;
	uint8_t val;
	unsigned long flags;

	spin_lock_irqsave(&port->lock, flags);

	WARN_ON(offset >= ARRAY_SIZE(wmt_gpios));

	val = readb(base + DIRECTION_REGS + regoff);
	if (dir)
		val |= (1 << shift);
	else
		val &= ~(1 << shift);
	writeb(val, base + DIRECTION_REGS + regoff);

	spin_unlock_irqrestore(&port->lock, flags);
}

static int wmt_gpio_get_value(struct gpio_chip *chip, unsigned offset)
{
	struct wmt_gpio_port *port = to_wmt(chip);
	uint32_t base = port->base;
	uint8_t regoff = wmt_gpios[offset].regoff;
	uint8_t shift  = wmt_gpios[offset].shift;
	uint8_t val;
	int dir;

	WARN_ON(offset >= ARRAY_SIZE(wmt_gpios));

	val = readb(base + DIRECTION_REGS + regoff);
	dir = (val >> shift) & 1;

	if (dir)
		return (readb(base + OUTVALUE_REGS + regoff) >> shift) & 1;
	else
		return (readb(base + INVALUE_REGS + regoff) >> shift) & 1;
}

static void wmt_gpio_set_value(struct gpio_chip *chip,
			       unsigned offset, int value)
{
	struct wmt_gpio_port *port = to_wmt(chip);
	uint32_t base = port->base;
	uint8_t regoff = wmt_gpios[offset].regoff;
	uint8_t shift  = wmt_gpios[offset].shift;
	uint8_t val;
	unsigned long flags;

	spin_lock_irqsave(&port->lock, flags);

	WARN_ON(offset >= ARRAY_SIZE(wmt_gpios));

	val = readb(base + OUTVALUE_REGS + regoff);

	if (value)
		val |= (1 << shift);
	else
		val &= ~(1 << shift);

	writeb(val, base + OUTVALUE_REGS + regoff);

	spin_unlock_irqrestore(&port->lock, flags);
}

static int wmt_gpio_direction_input(struct gpio_chip *chip, unsigned offset)
{
	_set_gpio_direction(chip, offset, 0);
	return 0;
}

static int wmt_gpio_direction_output(struct gpio_chip *chip,
				     unsigned offset, int value)
{
	wmt_gpio_set_value(chip, offset, value);
	_set_gpio_direction(chip, offset, 1);
	return 0;
}

static void _set_gpio_pullenable(unsigned offset, int enable)
{
	struct wmt_gpio_port *port = &wmt_gpio_port;
	uint32_t base = port->base;
	uint8_t regoff = wmt_gpios[offset].regoff;
	uint8_t shift  = wmt_gpios[offset].shift;
	uint8_t val;
	unsigned long flags;

	spin_lock_irqsave(&port->lock, flags);

	WARN_ON(offset >= ARRAY_SIZE(wmt_gpios));

	val = readb(base + PULLENABLE_REGS + regoff);
	if (enable)
		val |= (1 << shift);
	else
		val &= ~(1 << shift);
	writeb(val, base + PULLENABLE_REGS + regoff);

	spin_unlock_irqrestore(&port->lock, flags);
}

static void _set_gpio_pullup(unsigned offset, int up)
{
	struct wmt_gpio_port *port = &wmt_gpio_port;
	uint32_t base = port->base;
	uint8_t regoff = wmt_gpios[offset].regoff;
	uint8_t shift  = wmt_gpios[offset].shift;
	uint8_t val;
	unsigned long flags;

	spin_lock_irqsave(&port->lock, flags);

	WARN_ON(offset >= ARRAY_SIZE(wmt_gpios));

	val = readb(base + PULLCONTROL_REGS + regoff);
	if (up)
		val |= (1 << shift);
	else
		val &= ~(1 << shift);
	writeb(val, base + PULLCONTROL_REGS + regoff);

	spin_unlock_irqrestore(&port->lock, flags);
}

int wmt_gpio_setpull(unsigned int gpio, enum wmt_gpio_pulltype pull)
{
	int offset = gpio - CHIP_GPIO_BASE;

	switch (pull) {
	case WMT_GPIO_PULL_NONE:
		_set_gpio_pullenable(offset, 0);
		break;
	case WMT_GPIO_PULL_UP:
		_set_gpio_pullenable(offset, 1);
		_set_gpio_pullup(offset, 1);
		break;
	case WMT_GPIO_PULL_DOWN:
		_set_gpio_pullenable(offset, 1);
		_set_gpio_pullup(offset, 0);
		break;
	default:
		return -EINVAL;
	}
	return 0;
}
EXPORT_SYMBOL(wmt_gpio_setpull);

#ifdef WMT_GPIO_IRQ
static int wmt_gpio_to_irq(struct gpio_chip *chip, unsigned offset)
{
	struct wmt_gpio_port *port = &wmt_gpio_port;

	WARN_ON(offset >= ARRAY_SIZE(wmt_gpios));

	if (wmt_gpios[offset].irqnum < 0)
		return -EINVAL;

	return port->gpio_irq_no_base + wmt_gpios[offset].irqnum;
}
#endif

static struct wmt_gpio_port wmt_gpio_port = {
	.base = GPIO_BASE_ADDR,
#ifdef WMT_GPIO_IRQ
	.gpio_irq_no_base = WMT_GPIO_IRQ_START,
	.gpio_irq_nr = WMT_GPIO_IRQS,
#endif
	.chip = {
		.label			= "wmt-gpio",
		.direction_input	= wmt_gpio_direction_input,
		.direction_output	= wmt_gpio_direction_output,
		.request		= wmt_gpio_request,
		.free			= wmt_gpio_free,
		.get			= wmt_gpio_get_value,
		.set			= wmt_gpio_set_value,
#ifdef WMT_GPIO_IRQ
		.to_irq			= wmt_gpio_to_irq,
#endif
		.can_sleep		= 0,
		.base			= CHIP_GPIO_BASE,
		.ngpio			= ARRAY_SIZE(wmt_gpios),
	},
};

void __init wmt_gpio_init(void)
{
#ifdef WMT_GPIO_IRQ
	int irq;
#endif

	spin_lock_init(&wmt_gpio_port.lock);
	BUG_ON(gpiochip_add(&wmt_gpio_port.chip) < 0);

	/* XXX conflict with touchscreen irq */
#ifdef WMT_GPIO_IRQ
	for (irq = wmt_gpio_port.gpio_irq_no_base;
	     irq < wmt_gpio_port.gpio_irq_no_base + wmt_gpio_port.gpio_irq_nr;
	     irq++) {
		irq_set_chip_and_handler(irq, &gpio_irq_chip, handle_level_irq);
		set_irq_flags(irq, IRQF_VALID);
	}

	irq_set_chained_handler(IRQ_GPIO, wmt_gpio_irq_handler);
#endif
}

const char *wmt_gpio_name(int gpio)
{
	if (gpio >= ARRAY_SIZE(wmt_gpios)) {
		pr_err("wmt: can not find gpio-%d\n", gpio);
		return NULL;
	}

	return wmt_gpios[gpio].label;
}

#ifdef CONFIG_DEBUG_FS

static int wmt_gpio_show(struct seq_file *s, void *unused)
{
	unsigned i;

	seq_printf(s, "GPIONUM  , REGOFF , SHIFT , MACRO NAME\n");
	seq_printf(s, "---------+--------+-------+-----------\n");

	for (i = 0; i < wmt_gpio_port.chip.ngpio; i++) {
		seq_printf(s, "gpio-%-3d ,  0x%02x  ,    %d  , %s",
			   i, wmt_gpios[i].regoff, wmt_gpios[i].shift, wmt_gpios[i].label);
		seq_printf(s, "\n");
	}
	return 0;
}

static int wmt_gpio_open(struct inode *inode, struct file *file)
{
	return single_open(file, wmt_gpio_show, NULL);
}

static const struct file_operations wmt_gpio_operations = {
	.open		= wmt_gpio_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};

static int __init wmt_gpio_debugfs_init(void)
{
	/* /sys/kernel/debug/wmt-gpio */
	(void) debugfs_create_file("wmt-gpio", S_IFREG | S_IRUGO,
				   NULL, NULL, &wmt_gpio_operations);
	return 0;
}
subsys_initcall(wmt_gpio_debugfs_init);

#endif	/* DEBUG_FS */

