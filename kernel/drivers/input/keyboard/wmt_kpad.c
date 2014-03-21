/*++
linux/drivers/input/keyboard/wmt_kpad.c

Some descriptions of such software. Copyright (c) 2008  WonderMedia Technologies, Inc.

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
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/types.h>
#include <linux/input.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/cpufreq.h>
#include <linux/delay.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/errno.h>
#include <asm/mach-types.h>
#include <mach/hardware.h>
/*#include <mach/kpad.h> */
#include <linux/suspend.h>



/* Debug macros */
#if 0
#define DPRINTK(fmt, args...) printk(KERN_ALERT "[%s]: " fmt, __func__ , ## args)
#else
#define DPRINTK(fmt, args...)
#endif

/* #define USE_HOME */
#define wmt_kpad_timeout ((HZ/100)*10)

#define WMT_KPAD_FUNCTION_NUM 2


static unsigned int wmt_kpad_codes[WMT_KPAD_FUNCTION_NUM] = {
	[0] = KEY_VOLUMEUP,
	[1] = KEY_VOLUMEDOWN,
};
#define WMT_GPIO8_KEY_NUM 0
#define WMT_GPIO10_KEY_NUM 1


static struct timer_list   wmt_kpad_timer_vu;
static struct timer_list   wmt_kpad_timer_vd;




static struct input_dev *kpad_dev;


#define KEYPAD_GPIO_BIT_VU BIT0
#define KEYPAD_GPIO_BIT_VD BIT2

#define KEYPAD_GPIO_CTRL_VAL_VU          GPIO_CTRL_GP1_BYTE_VAL
#define KEYPAD_GPIO_OC_VAL_VU            GPIO_OC_GP1_BYTE_VAL
#define KEYPAD_GPIO_ID_VAL_VU            GPIO_ID_GP1_BYTE_VAL
#define KEYPAD_GPIO_PULL_CTRL_VAL_VU     PULL_CTRL_GP1_BYTE_VAL
#define KEYPAD_GPIO_PULL_EN_VAL_VU       PULL_EN_GP1_BYTE_VAL
#define KEYPAD_GPIO_INT_REQ_STS_VAL_VU   GPIO1_INT_REQ_STS_VAL
#define KEYPAD_GPIO_INT_REQ_TYPE_VAL_VU  GPIO8_INT_REQ_TYPE_VAL

#define KEYPAD_GPIO_CTRL_VAL_VD          GPIO_CTRL_GP1_BYTE_VAL
#define KEYPAD_GPIO_OC_VAL_VD            GPIO_OC_GP1_BYTE_VAL
#define KEYPAD_GPIO_ID_VAL_VD            GPIO_ID_GP1_BYTE_VAL
#define KEYPAD_GPIO_PULL_CTRL_VAL_VD     PULL_CTRL_GP1_BYTE_VAL
#define KEYPAD_GPIO_PULL_EN_VAL_VD       PULL_EN_GP1_BYTE_VAL
#define KEYPAD_GPIO_INT_REQ_STS_VAL_VD   GPIO1_INT_REQ_STS_VAL
#define KEYPAD_GPIO_INT_REQ_TYPE_VAL_VD  GPIO10_INT_REQ_TYPE_VAL




#ifdef CONFIG_CPU_FREQ
/*
 * Well, the debounce time is not very critical while zac2_clk
 * rescaling, but we still do it.
 */

/* kpad_clock_notifier()
 *
 * When changing the processor core clock frequency, it is necessary
 * to adjust the KPMIR register.
 *
 * Returns: 0 on success, -1 on error
 */
static int kpad_clock_notifier(struct notifier_block *nb, unsigned long event,
	void *data)
{
	return 0;
}

/*
 * Notify callback while issusing zac2_clk rescale.
 */
static struct notifier_block kpad_clock_nblock = {
	.notifier_call  = kpad_clock_notifier,
	.priority = 1
};
#endif

static void wmt_kpad_hw_init(void)
{
	DPRINTK("Start\n");

	/*Falling edge irq type*/
	KEYPAD_GPIO_INT_REQ_TYPE_VAL_VU |= BIT1;
	KEYPAD_GPIO_INT_REQ_TYPE_VAL_VU &= ~(BIT0 | BIT2);

	KEYPAD_GPIO_INT_REQ_TYPE_VAL_VD |= BIT1;
	KEYPAD_GPIO_INT_REQ_TYPE_VAL_VD &= ~(BIT0 | BIT2);

	/*Enable GPIO*/
	KEYPAD_GPIO_CTRL_VAL_VU |= KEYPAD_GPIO_BIT_VU;
	KEYPAD_GPIO_CTRL_VAL_VD |= KEYPAD_GPIO_BIT_VD;

	/*Set GPIO as GPI*/
	KEYPAD_GPIO_OC_VAL_VU &= ~KEYPAD_GPIO_BIT_VU;
	KEYPAD_GPIO_OC_VAL_VD &= ~KEYPAD_GPIO_BIT_VD;

	/*Set pull up*/
	KEYPAD_GPIO_PULL_CTRL_VAL_VU |= KEYPAD_GPIO_BIT_VU;
	KEYPAD_GPIO_PULL_CTRL_VAL_VD |= KEYPAD_GPIO_BIT_VD;

	/*Pull enable*/
	KEYPAD_GPIO_PULL_EN_VAL_VU |= KEYPAD_GPIO_BIT_VU;
	KEYPAD_GPIO_PULL_EN_VAL_VD |= KEYPAD_GPIO_BIT_VD;

	/*Clear interrupt*/
	KEYPAD_GPIO_INT_REQ_STS_VAL_VU = KEYPAD_GPIO_BIT_VU;
	KEYPAD_GPIO_INT_REQ_STS_VAL_VD = KEYPAD_GPIO_BIT_VD;

	/*Enable interrupt*/
	KEYPAD_GPIO_INT_REQ_TYPE_VAL_VU |= BIT7;
	KEYPAD_GPIO_INT_REQ_TYPE_VAL_VD |= BIT7;

	DPRINTK("End\n");
}



static void wmt_kpad_timeout_vu(unsigned long fcontext)
{
	DPRINTK("Start\n");
	if ((KEYPAD_GPIO_ID_VAL_VU & KEYPAD_GPIO_BIT_VU) == 0) {   /*Active Low*/
		mod_timer(&wmt_kpad_timer_vu, jiffies + wmt_kpad_timeout);
		DPRINTK("WMT Volume up keep press\n");
	} else {
		/*Volume up release*/
		input_report_key(kpad_dev, KEY_VOLUMEUP, 0);
		input_sync(kpad_dev);
		DPRINTK("WMT_Volume up release key = %d \n", KEY_VOLUMEUP);
		/*Enable irq*/
		KEYPAD_GPIO_INT_REQ_TYPE_VAL_VU |= BIT7;
	}
	DPRINTK("End\n");
}

static void wmt_kpad_timeout_vd(unsigned long fcontext)
{
	DPRINTK("Start\n");
	if ((KEYPAD_GPIO_ID_VAL_VD & KEYPAD_GPIO_BIT_VD) == 0) {   /*Active Low*/
		mod_timer(&wmt_kpad_timer_vd, jiffies + wmt_kpad_timeout);
		DPRINTK("WMT Volume down keep press\n");
	} else {
		/*Volume down release*/
		input_report_key(kpad_dev, KEY_VOLUMEDOWN, 0); /*row4 key is release*/
		input_sync(kpad_dev);
		DPRINTK("WMT_Volume down release key = %d \n", KEY_VOLUMEDOWN);
		/*Enable irq*/
		KEYPAD_GPIO_INT_REQ_TYPE_VAL_VD |= BIT7;
	}
	DPRINTK("End\n");
}



static irqreturn_t
kpad_interrupt(int irq, void *dev_id)
{
	unsigned int vu_int_status, vd_int_status;

	DPRINTK("[%s]s\n", __func__);

	/*disable interrupt*/
	KEYPAD_GPIO_INT_REQ_TYPE_VAL_VU &= ~BIT7;
	KEYPAD_GPIO_INT_REQ_TYPE_VAL_VD &= ~BIT7;

	if ((!(KEYPAD_GPIO_INT_REQ_STS_VAL_VU & KEYPAD_GPIO_BIT_VU)) &&
		(!(KEYPAD_GPIO_INT_REQ_STS_VAL_VD & KEYPAD_GPIO_BIT_VD))) {

		/*Enable interrupt*/
		KEYPAD_GPIO_INT_REQ_TYPE_VAL_VU |= BIT7;
		KEYPAD_GPIO_INT_REQ_TYPE_VAL_VD |= BIT7;

		return IRQ_NONE;
	}

	vu_int_status = KEYPAD_GPIO_INT_REQ_STS_VAL_VU;
	vd_int_status = KEYPAD_GPIO_INT_REQ_STS_VAL_VD;

	/*Clear interrupt*/
	if (KEYPAD_GPIO_INT_REQ_STS_VAL_VU & KEYPAD_GPIO_BIT_VU)
		KEYPAD_GPIO_INT_REQ_STS_VAL_VU = KEYPAD_GPIO_BIT_VU;

	if (KEYPAD_GPIO_INT_REQ_STS_VAL_VD & KEYPAD_GPIO_BIT_VD)
		KEYPAD_GPIO_INT_REQ_STS_VAL_VD = KEYPAD_GPIO_BIT_VD;

	if (KEYPAD_GPIO_INT_REQ_STS_VAL_VU & KEYPAD_GPIO_BIT_VU)
		DPRINTK("[Volume up] Clear interrupt failed.\n");

	if (KEYPAD_GPIO_INT_REQ_STS_VAL_VD & KEYPAD_GPIO_BIT_VD)
		DPRINTK("[Volume down] Clear interrupt failed.\n");

	if (!(vu_int_status & KEYPAD_GPIO_BIT_VU)) {
		/*Enable interrupt*/
		KEYPAD_GPIO_INT_REQ_TYPE_VAL_VU |= BIT7;
	} else {
		/*Volume up is pressed*/
		input_report_key(kpad_dev, KEY_VOLUMEUP, 1);
		input_sync(kpad_dev);
		mod_timer(&wmt_kpad_timer_vu, jiffies + wmt_kpad_timeout);
		DPRINTK("[%s]WMT Volume up press = %d\n", __func__, KEY_VOLUMEUP);
	}

	if (!(vd_int_status & KEYPAD_GPIO_BIT_VD)) {
		/*Enable interrupt*/
		KEYPAD_GPIO_INT_REQ_TYPE_VAL_VD |= BIT7;
	} else {
		/*Volume up is pressed*/
		input_report_key(kpad_dev, KEY_VOLUMEDOWN, 1);
		input_sync(kpad_dev);
		mod_timer(&wmt_kpad_timer_vd, jiffies + wmt_kpad_timeout);
		DPRINTK("[%s]WMT Volume down press = %d\n", __func__, KEY_VOLUMEDOWN);
	}

	DPRINTK("[%s]e\n", __func__);
	return IRQ_HANDLED;
}


static int kpad_open(struct input_dev *dev)
{
	int ret = 0;
	DPRINTK("Start\n");


	ret = request_irq(IRQ_GPIO, kpad_interrupt, IRQF_SHARED, "keypad", dev);
	if (ret) {
		printk(KERN_ERR "%s: Can't allocate irq %d ret = %d\n", __func__, IRQ_GPIO, ret);
		free_irq(IRQ_GPIO, dev);
		goto kpad_open_out;
	}
	DPRINTK("1\n");
	/*init timer*/
	init_timer(&wmt_kpad_timer_vu);
	wmt_kpad_timer_vu.function = wmt_kpad_timeout_vu;
	wmt_kpad_timer_vu.data = (unsigned long)dev;

	init_timer(&wmt_kpad_timer_vd);
	wmt_kpad_timer_vd.function = wmt_kpad_timeout_vd;
	wmt_kpad_timer_vd.data = (unsigned long)dev;

	wmt_kpad_hw_init();
	DPRINTK("End2\n");
kpad_open_out:
	DPRINTK("End3\n");
	return ret;
}

static void kpad_close(struct input_dev *dev)
{
	DPRINTK("Start\n");

	/*
	 * Free interrupt resource
	 */
	free_irq(IRQ_GPIO, NULL);

	/*
	 * Unregister input device driver
	 */
	input_unregister_device(dev);
	DPRINTK("End2\n");
}

static int wmt_kpad_probe(struct platform_device *pdev)
{
	int ret = 0;
	unsigned int i;
	DPRINTK("Start\n");
	kpad_dev = input_allocate_device();
	if (kpad_dev == NULL) {
		DPRINTK("End 1\n");
		return -1;
	}
	/*
	 * Simply check resources parameters.
	 */
	DPRINTK("pdev->num_resources = 0x%x\n", pdev->num_resources);
	if (pdev->num_resources < 0 || pdev->num_resources > 2) {
		ret = -ENODEV;
		goto kpad_probe_out;
	}

	kpad_dev->open = kpad_open,
	kpad_dev->close = kpad_close,

	/* Register an input event device. */
	kpad_dev->name = "keypad",
	kpad_dev->phys = "keypad",

	/*
	 *	Let kpad to implement key repeat.
	 */

	set_bit(EV_KEY, kpad_dev->evbit);

	for (i = 0; i < WMT_KPAD_FUNCTION_NUM; i++)
		set_bit(wmt_kpad_codes[i], kpad_dev->keybit);

	kpad_dev->keycode = wmt_kpad_codes;
	kpad_dev->keycodesize = sizeof(unsigned int);
	kpad_dev->keycodemax = WMT_KPAD_FUNCTION_NUM;

	/*
	 * For better view of /proc/bus/input/devices
	 */
	kpad_dev->id.bustype = 0;
	kpad_dev->id.vendor	= 0;
	kpad_dev->id.product = 0;
	kpad_dev->id.version = 0;

	input_register_device(kpad_dev);

	DPRINTK("End2\n");
kpad_probe_out:

#ifndef CONFIG_SKIP_DRIVER_MSG
	printk(KERN_INFO "WMT keypad driver initialized: %s\n",
		  (ret == 0) ? "ok" : "failed");
#endif
	DPRINTK("End3\n");
	return ret;
}

static int wmt_kpad_remove(struct platform_device *pdev)
{
	DPRINTK("Start\n");
	kpad_close(kpad_dev);
	DPRINTK("End\n");
	return 0;
}

static int wmt_kpad_suspend(struct platform_device *pdev, pm_message_t state)
{
	DPRINTK("Start\n");

	switch (state.event) {
	case PM_EVENT_SUSPEND:
		/*disable interrupt*/
		KEYPAD_GPIO_INT_REQ_TYPE_VAL_VU &= ~BIT7;
		KEYPAD_GPIO_INT_REQ_TYPE_VAL_VD &= ~BIT7;
		break;
	case PM_EVENT_FREEZE:
	case PM_EVENT_PRETHAW:

	default:
		break;
	}

	DPRINTK("End2\n");
	return 0;
}

static int wmt_kpad_resume(struct platform_device *pdev)
{
	DPRINTK("Start\n");
	wmt_kpad_hw_init();
	DPRINTK("End\n");
	return 0;
}

static struct platform_driver wmt_kpad_driver = {
	.driver.name = "wmt-kpad",
	.probe = &wmt_kpad_probe,
	.remove = &wmt_kpad_remove,
	.suspend = &wmt_kpad_suspend,
	.resume	= &wmt_kpad_resume
};

static struct resource wmt_kpad_resources[] = {
	[0] = {
		.start  = IRQ_GPIO,
		.end    = IRQ_GPIO,
		.flags  = IORESOURCE_IRQ,
	},
};

static struct platform_device wmt_kpad_device = {
	.name			= "wmt-kpad",
	.id				= 0,
	.num_resources  = ARRAY_SIZE(wmt_kpad_resources),
	.resource		= wmt_kpad_resources,
};

extern int wmt_getsyspara(char *varname, unsigned char *varval, int *varlen);

static int __init kpad_init(void)
{
	int ret;
	int retval;
	unsigned char buf[80];
	int varlen = 80;
	char *varname = "wmt.keypad.param";
	int temp = 0, enable_keypad = 0, function_sel = 1;

	DPRINTK(KERN_ALERT "Start\n");
	/*read back&menu button integration enable >1 enable & the value is the timeout value*/
	/*read keypad enable*/
	retval = wmt_getsyspara(varname, buf, &varlen);
	if (retval == 0) {
		sscanf(buf, "%X", &temp);
		enable_keypad = temp & 0xf;
		function_sel = (temp >> 4) & 0xf;
		printk(KERN_ALERT "wmt.keypad.param = %x, enable=%x, function = %x\n",
			temp, enable_keypad, function_sel);
		if (enable_keypad != 1 || function_sel != 0) {
			printk(KERN_ALERT "Disable GPIO as keypad function!!\n");
			return -ENODEV;
		}
	} else {
		printk(KERN_ALERT "##Warning: \"wmt.keypad.param\" not find\n");
		printk(KERN_ALERT "Default wmt.keypad.param = %x\n", temp);
	}

#ifdef CONFIG_CPU_FREQ
	ret = cpufreq_register_notifier(&kpad_clock_nblock, \
		CPUFREQ_TRANSITION_NOTIFIER);

	if (ret) {
		printk(KERN_ERR "Unable to register CPU frequency " \
			"change notifier (%d)\n", ret);
	}
#endif
	ret = platform_device_register(&wmt_kpad_device);
	if (ret != 0) {
		DPRINTK("End1 ret = %x\n", ret);
		return -ENODEV;
	}

	ret = platform_driver_register(&wmt_kpad_driver);
	DPRINTK("End2 ret = %x\n", ret);
	return ret;
}

static void __exit kpad_exit(void)
{
	DPRINTK("Start\n");
	platform_driver_unregister(&wmt_kpad_driver);
	platform_device_unregister(&wmt_kpad_device);
	DPRINTK("End\n");
}

module_init(kpad_init);
module_exit(kpad_exit);

MODULE_AUTHOR("WonderMedia Technologies, Inc.");
MODULE_DESCRIPTION("WMT [generic keypad] driver");
MODULE_LICENSE("GPL");
