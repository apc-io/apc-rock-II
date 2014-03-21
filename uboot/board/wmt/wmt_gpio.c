/* 
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

#include <common.h>
#include <asm/io.h>
#include <asm/errno.h>
#include "include/wmt_iomux.h"

#undef	WMT_PIN
#define WMT_PIN(__gp, __bit, __irq, __name)	\
	{ .label = #__name, .regoff = __gp, .shift = __bit, .irqnum = __irq },

const static struct wmt_gpio {
	const char *label;
	unsigned char regoff;
	unsigned char shift;
	int irqnum;
} wmt_gpios[] = {
	#include "include/iomux.h"
};

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

#define CHIP_GPIO_BASE		0
#define GPIO_BASE_ADDR 		0xD8110000

static unsigned long port_base = GPIO_BASE_ADDR;

int gpio_request(unsigned gpio)
{
	uint8_t regoff = wmt_gpios[gpio].regoff;
	uint8_t shift  = wmt_gpios[gpio].shift;
	uint8_t val;

	val = __raw_readb(port_base + ENABLE_REGS + regoff);
	val |= (1 << shift);
	__raw_writeb(val, port_base + ENABLE_REGS + regoff);

	return 0;
}

void gpio_free(unsigned gpio)
{
	uint8_t regoff = wmt_gpios[gpio].regoff;
	uint8_t shift  = wmt_gpios[gpio].shift;
	uint8_t val;

	val = __raw_readb(port_base + ENABLE_REGS + regoff);
	val &= ~(1 << shift);
	__raw_writeb(val, port_base + ENABLE_REGS + regoff);
}

static void _set_gpio_direction(unsigned gpio, int dir)
{
	uint8_t regoff = wmt_gpios[gpio].regoff;
	uint8_t shift  = wmt_gpios[gpio].shift;
	uint8_t val;

	val = __raw_readb(port_base + DIRECTION_REGS + regoff);
	if (dir)
		val |= (1 << shift);
	else
		val &= ~(1 << shift);
	__raw_writeb(val, port_base + DIRECTION_REGS + regoff);
}

int gpio_get_value(unsigned gpio)
{
	uint8_t regoff = wmt_gpios[gpio].regoff;
	uint8_t shift  = wmt_gpios[gpio].shift;

	return (__raw_readb(port_base + INVALUE_REGS + regoff) >> shift) & 1;
}

void gpio_set_value(unsigned gpio, int value)
{
	uint8_t regoff = wmt_gpios[gpio].regoff;
	uint8_t shift  = wmt_gpios[gpio].shift;
	uint8_t val;

	val = __raw_readb(port_base + OUTVALUE_REGS + regoff);
	if (value)
		val |= (1 << shift);
	else
		val &= ~(1 << shift);
	__raw_writeb(val, port_base + OUTVALUE_REGS + regoff);
}

int gpio_direction_input(unsigned gpio)
{
	gpio_request(gpio);
	_set_gpio_direction(gpio, 0);
	return 0;
}

int gpio_direction_output(unsigned gpio, int value)
{
	gpio_request(gpio);
	gpio_set_value(gpio, value);
	_set_gpio_direction(gpio, 1);
	return 0;
}

static void _set_gpio_pullenable(unsigned gpio, int enable)
{
	uint8_t regoff = wmt_gpios[gpio].regoff;
	uint8_t shift  = wmt_gpios[gpio].shift;
	uint8_t val;

	val = __raw_readb(port_base + PULLENABLE_REGS + regoff);
	if (enable)
		val |= (1 << shift);
	else
		val &= ~(1 << shift);
	__raw_writeb(val, port_base + PULLENABLE_REGS + regoff);

}

static void _set_gpio_pullup(unsigned gpio, int up)
{
	uint8_t regoff = wmt_gpios[gpio].regoff;
	uint8_t shift  = wmt_gpios[gpio].shift;
	uint8_t val;

	val = __raw_readb(port_base + PULLCONTROL_REGS + regoff);
	if (up)
		val |= (1 << shift);
	else
		val &= ~(1 << shift);
	__raw_writeb(val, port_base + PULLCONTROL_REGS + regoff);
}

int gpio_setpull(unsigned int gpio, enum gpio_pulltype pull)
{
	switch (pull) {
	case GPIO_PULL_NONE:
		_set_gpio_pullenable(gpio, 0);
		break;
	case GPIO_PULL_UP:
		_set_gpio_pullenable(gpio, 1);
		_set_gpio_pullup(gpio, 1);
		break;
	case GPIO_PULL_DOWN:
		_set_gpio_pullenable(gpio, 1);
		_set_gpio_pullup(gpio, 0);
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

