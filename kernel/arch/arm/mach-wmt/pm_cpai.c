/*++
linux/arch/arm/mach-wmt/board.c

Copyright (c) 2012 WonderMedia Technologies, Inc.

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
#define fail -1
#include <mach/hardware.h>
#include <mach/irqs.h>
#include <linux/sysctl.h>

extern unsigned int wmt_i2c0_speed_mode;
extern unsigned int wmt_i2c1_speed_mode;

#if 0
static int PM_USB_PostSuspend(void)
{
	return 0;
}


static int PM_USB_PreResume(void)
{
	unsigned int tmp32;
	unsigned char tmp8;

	/* EHCI PCI configuration seting */
	/* set bus master */
	*(volatile unsigned char*) 0xd8007804 = 0x17;
	tmp8 = *(volatile unsigned char*) 0xd8007804;
	if (tmp8 != 0x17) {
		printk("PM_USB_init : 0xd8007104 = %02x\n", tmp8);
		return fail;
	}

	/* set interrupt line */
	*(volatile unsigned char*) 0xd800783c = 0x2b;
	tmp8 = *(volatile unsigned char*) 0xd800783c;
	if (tmp8 != 0x2b) {
		printk("PM_USB_init : 0xd800713c = %02x\n", tmp8);
		return fail;
	}

	/* enable AHB master cut data to 16/8/4 DW align */
	tmp32 = *(volatile unsigned char*) 0xd800784c;
	tmp32 &= 0xfffdffff;
	*(volatile unsigned char*) 0xd800784c = tmp32;
	tmp32 = *(volatile unsigned char*) 0xd800784c;
	if ((tmp32&0x00020000) != 0x00000000)
		printk("[EHCI] Programming EHCI PCI Configuration Offset Address : 4ch fail! \n");


	/* UHCI PCI configuration setting */
	/* set bus master */
	*(volatile unsigned char*) 0xd8007a04 = 0x07;
	tmp8 = *(volatile unsigned char*) 0xd8007a04;
	if (tmp8 != 0x07) {
		printk("PM_USB_init : 0xd8007304 = %02x\n", tmp8);
		return fail;
	}

	/* set interrupt line */
	*(volatile unsigned char*) 0xd8007a3c = 0x2b;
	tmp8 = *(volatile unsigned char*) 0xd8007a3c;
	if (tmp8 != 0x2b) {
		printk("PM_USB_init : 0xd800733c = %02x\n", tmp8);
		return fail;
	}

	return 0;
}
#endif

#if 0
static char MACVeeRom[32] = {0xFF};
static void PM_MAC_PreResume(void)
{
	int i = 0;
	*((volatile unsigned char*)0xd8004104) = 0x4;

	/* restore VEE */
	for (i = 0; i < 8; i++)
		*(volatile unsigned long *)(0xd800415C + i*4) = *((int *)MACVeeRom + i);

	/* reload VEE */
	*(volatile unsigned char*)0xd800417C |= 0x01;

	/* reload */
	*(volatile unsigned char*)0xd8004074 |= 0xa0;

	/* check reload VEE complete */
	while ((*(volatile unsigned char*)0xd800417C & 0x02) != 0x02);

	return;
}
static void PM_MAC_PostSuspend(void)
{
	int i = 0;

	/*save VEE*/
	for (i = 0; i < 32; i++)
		MACVeeRom[i] = *((volatile unsigned char*)0xd800415C + i);

	return;
}
#endif


inline unsigned int wmt_read_oscr(void);
static u32 cyc_mark = 0;
#define PRIVATE_TIMER_CTRL (MPCORE_PRIVATE_MEM + 0x600)
#define PRIVATE_TIMER_INTSTAT (MPCORE_PRIVATE_MEM + 0x60C)
#define GIC_PEN_CLR (0xFE019280)

static void wmt_timer_postsuspend(void)
{
	cyc_mark = wmt_read_oscr();
	if (REG32_VAL(PRIVATE_TIMER_INTSTAT)) {
		REG32_VAL(PRIVATE_TIMER_INTSTAT) = 0x1;
		REG32_VAL(GIC_PEN_CLR) = 0x20000000;
	}
}

static void wmt_timer_preresume(void)
{
	// set saved counter value
	OSCR_VAL = cyc_mark;
}

int PM_device_PreResume(void)
{
	int result;
	/* PM_MAC_PreResume(); */
    wmt_timer_preresume();

	/*
	result = PM_USB_PreResume();
	if (!result)
		goto FAIL;
	*/

	return 0;
/* FAIL: */
	return result;
}


int PM_device_PostSuspend(void)
{
	/* PM_MAC_PostSuspend(); */
	wmt_timer_postsuspend();
	return 0;
}
