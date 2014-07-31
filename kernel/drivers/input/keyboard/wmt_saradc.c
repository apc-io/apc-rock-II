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
#include <mach/wmt_saradc.h>
#include <linux/suspend.h>


/* #define COUNTTIMER */
#ifdef COUNTTIMER
unsigned int start_time;
#endif

/* Debug macros */
#if 0
#define DPRINTK(fmt, args...) printk(KERN_ALERT "[%s]: " fmt, __func__ , ## args)
#else
#define DPRINTK(fmt, args...)
#endif

/* the shortest response time is 20 ms, original timeout (HZ/100)*100 */
#define wmt_saradc_timeout ((HZ/100)*2)
#define WMT_SARADC_FUNCTION_NUM 6
#define SAMETIMES 2
unsigned int HW_Hz;
unsigned int SW_timeout;
unsigned int INT_timeout;

static unsigned int wmt_saradc_codes[WMT_SARADC_FUNCTION_NUM] = {
	[0] = KEY_VOLUMEUP,
	[1] = KEY_VOLUMEDOWN,
	[2] = KEY_BACK,
	[3] = KEY_MENU,
	[4] = KEY_HOME,
	[5] = KEY_RESERVED,
};

/*high resolution timer */
static struct hrtimer wmt_saradc_hrtimer;
static struct input_dev *saradc_dev;

static struct wmt_saradc_s saradc = {
	.ref	= 0,
	.res	= NULL,
	.regs   = NULL,
	.irq	= 0,
};

int count_sample_rate(unsigned APB_clock, int Hz)
{
	int temp_slot;
	/* the Hz that we want */
	temp_slot = APB_clock/(4096 * Hz); /* (APB clock/32)/ (128 * HZ)*/
	return temp_slot;
}

int saradc_event_table(unsigned int eventcode)
{
	DPRINTK("eventcode = %d\n", eventcode);
	if (eventcode >= 39 && eventcode <= 42)
		return 0;
	else if (eventcode >= 63 && eventcode <= 64)
		return 1;
	else if (eventcode >= 84 && eventcode <= 85)
		return 2;
	else if (eventcode >= 18 && eventcode <= 20)
		return 3;
	else if (eventcode == 0)
		return 4;
	else if (eventcode >= 29 && eventcode <= 31)
		return 5;
	else if (eventcode == 127)
		return 5;
	else
		return 5;
}

static void wmt_saradc_hw_init(void)
{
	unsigned int auto_temp_slot;
	unsigned int APB_clk;
	DPRINTK("Start\n");

	/*
	 * Turn on saradc clocks.
	 */
	auto_pll_divisor(DEV_ADC, CLK_ENABLE, 0, 0);

	/* Turn on SARADC controll power */
	saradc.regs->Ctr0 &= ~PD;

	/* Enable SARADC digital clock */
	saradc.regs->Ctr0 |= DigClkEn;

	/* Simply clean all previous saradc status. */
	saradc.regs->Ctr0 |= (ClrIntADC | ClrIntTOut);
	saradc.regs->Ctr1 |= ClrIntValDet;

	if (((saradc.regs->Ctr2 & EndcIntStatus) == EndcIntStatus) ||
		((saradc.regs->Ctr2 & TOutStatus) == TOutStatus)       ||
		((saradc.regs->Ctr2 & ValDetIntStatus) == ValDetIntStatus))
		printk(KERN_ERR "[saradc] clear status failed! status = %x\n", saradc.regs->Ctr2);

	/*Set Timeout Value*/
	saradc.regs->Ctr0 &= 0xffff0000;
	saradc.regs->Ctr0 |= TOutDly(0xffff);

	/* get APB clock & count sample rate*/
	APB_clk = auto_pll_divisor(DEV_APB, GET_FREQ, 0, 0);
	/* sample rate: 1000 Hz , 1 ms/sample */
	auto_temp_slot = count_sample_rate(APB_clk, HW_Hz);
	DPRINTK("[%s] APB_clk = %d\n", __func__, APB_clk);
	/*Set Sample Rate*/
	saradc.regs->Ctr1 &= 0x0000ffff;
	saradc.regs->Ctr1 |= (auto_temp_slot << 16);
	DPRINTK("[%s] auto_temp_slot = %x ctr1: %x\n", __func__, auto_temp_slot, saradc.regs->Ctr1);
	/* Set saradc as auto mode */
	saradc.regs->Ctr0 |= AutoMode;

	msleep(200);

	/* Enable value changing interrupt and Buffer data valid */
	saradc.regs->Ctr1 |= (ValDetIntEn | BufRd);
	/* saradc.regs->Ctr0 |= TOutEn; */

	DPRINTK("End\n");
}

enum hrtimer_restart wmt_saradc_timeout_hrtimer(struct hrtimer *timer)
{
	unsigned int SARCODE = 0xffff;
	static unsigned int OLDCODE = 0xffff;
	static int time, same;
	static bool saradc_flag = 1; /* 0: report event state, 1: get SARCODE value state */
	int new_event = -1;
	static int pre_event = -1, button_press;
	ktime_t ktime;
	/* count timeout value */
#ifdef COUNTTIMER
	unsigned int end_time;
	end_time = wmt_read_oscr();
	printk(KERN_ERR "time = %d\n", (end_time - start_time)/3);
#endif
	ktime = ktime_set(0, SW_timeout * 1000);

	DPRINTK("[%s] Start\n", __func__);
	while ((saradc.regs->Ctr2 & EndcIntStatus) == 0)
		;
	SARCODE = SARCode(saradc.regs->Ctr1);

	if (saradc_flag && time < 10) {
		if ((SARCODE/4 - OLDCODE/4) <= 1 || (SARCODE/4 - OLDCODE/4) >= -1) {
			same++;
			DPRINTK("time:%d SARCODE=%u SARCODE/4=%u, OLDCODE=%u, OLDCODE/4=%u, same=%d\n",
					time, SARCODE, SARCODE/4, OLDCODE, OLDCODE/4, same);
			if (same == SAMETIMES)
				saradc_flag = 0; /* get the new event */
		} else
			same = 0;

		DPRINTK("time:%d SARCODE=%u SARCODE/4=%u, OLDCODE=%u, OLDCODE/4=%u, same=%d\n",
				time, SARCODE, SARCODE/4, OLDCODE, OLDCODE/4, same);
		OLDCODE = SARCODE;
		time++;

		/* don't call timer when 10th get SARCODE or enough same time */
		if (time < 10 && same != SAMETIMES) {
			hrtimer_start(&wmt_saradc_hrtimer, ktime, HRTIMER_MODE_REL);
		/* count timer from callback function to callback function */
#ifdef COUNTTIMER
			start_time = wmt_read_oscr();
#endif
		}
		/* if not get stable SARCODE value in 10 times, report SARACODE is NONE event */
		if (time == 10 && same != SAMETIMES) {
			SARCODE = 508;
			DPRINTK("time %d SARCODE %u", time, SARCODE);
		}
	}
	if (time == 10 || saradc_flag == 0)
		time = 0;

	/* disable BufRd */
	saradc.regs->Ctr1 &= ~BufRd;

	new_event = saradc_event_table(SARCODE/4);

	if (SARCODE == 0xffff) {
		printk(KERN_ERR "Auto mode witn INT test fail\n");
		/*Disable interrupt*/
		saradc.regs->Ctr1 &= ~ValDetIntEn;
		/* Clean all previous saradc status. */
		saradc.regs->Ctr0 |= (ClrIntTOut | ClrIntADC);
		saradc.regs->Ctr1 |= ClrIntValDet;
	} else {
		/*DPRINTK("Buf_rdata = %u Buf_rdata/4 = %u\n", data, data/4);*/
		DPRINTK("SARCODE = %u SARCODE/4 = %u\n", SARCODE, SARCODE/4);
		/*Disable interrupt*/
		saradc.regs->Ctr1 &= ~ValDetIntEn;
		/* Clean all previous saradc status. */
		saradc.regs->Ctr0 |= (ClrIntTOut | ClrIntADC);
		saradc.regs->Ctr1 |= ClrIntValDet;
	}

	if (saradc_flag == 0) {
		/* switch other button means release button*/
		if ((pre_event != new_event) && (SARCODE/4 != 127)) {
			button_press = 0;
			DPRINTK("Different event, pre_event = %d, new_event = %d\n", pre_event, new_event);
			DPRINTK("WMT_ROW1_KEY_NUM release key = %d, event=%d\n", SARCODE/4, pre_event);
			input_report_key(saradc_dev, wmt_saradc_codes[pre_event], 0); /* key is release*/
			input_sync(saradc_dev);
		}

		if (SARCODE/4 == 127 || SARCODE/4 == 126) {   /*Active Low*/
			DPRINTK("WMT_ROW1_KEY_NUM release key = %d, event=%d\n", SARCODE/4, pre_event);
			input_report_key(saradc_dev, wmt_saradc_codes[pre_event], 0); /* key is release*/
			input_sync(saradc_dev);
			button_press = 0;
		} else {
			if (button_press == 0) {
				DPRINTK("new event = %d\n", new_event);
				input_report_key(saradc_dev, wmt_saradc_codes[new_event], 1);/* key is press*/
				input_sync(saradc_dev);
				DPRINTK("saradc code = %d\n", wmt_saradc_codes[new_event]);
				button_press = 1;
			}
			DPRINTK("WMT_ROW1_KEY_NUM keep press key = %d, event=%d\n", SARCODE/4, new_event);
		}
		pre_event = new_event;
		saradc_flag = 1; /* report new event to Android, get new SARCODE */
		same = 0;
	}
	saradc.regs->Ctr1 |= ValDetIntEn;
	DPRINTK("[%s] End\n", __func__);
	return HRTIMER_NORESTART; /* avoid to timer restart */
}

static irqreturn_t
saradc_interrupt(int irq, void *dev_id)
{
	ktime_t ktime;
	ktime = ktime_set(0, INT_timeout * 1000); /* ms */
	DPRINTK("[%s] Start\n", __func__);
	DPRINTK("status = %x\n", saradc.regs->Ctr2);
	/* Disable interrupt */
	/* disable_irq_nosync(saradc.irq);*/
	saradc.regs->Ctr1 &= ~ValDetIntEn;

	saradc.regs->Ctr1 |= BufRd;
	/*
	 * Get saradc interrupt status and clean interrput source.
	 */

	/* if (((saradc.regs->Ctr1 & ValDetIntEn) == ValDetIntEn) && */
	if ((saradc.regs->Ctr2 & ValDetIntStatus) == ValDetIntStatus) {
		/* clear value chaning interrupt */
		saradc.regs->Ctr1 |= ClrIntValDet;
		/* start hrtimer */
		hrtimer_start(&wmt_saradc_hrtimer, ktime, HRTIMER_MODE_REL);

		/* count timer from interrupt to callback function */
#ifdef COUNTTIMER
		start_time = wmt_read_oscr();
#endif
	}

	if ((saradc.regs->Ctr2 & ValDetIntStatus) == ValDetIntStatus)
		printk(KERN_ERR "[saradc] status clear failed!\n");


	/* Enable interrupt */
	/* saradc.regs->Ctr1 |= ValDetIntEn; // enable INT in wmt_saradc_timeout_timer*/
	DPRINTK("[%s] End\n", __func__);
	return IRQ_HANDLED;
}

static int saradc_open(struct input_dev *dev)
{
	int ret = 0;
	unsigned int i;
	DPRINTK("Start saradc.ref = %d\n", saradc.ref);

	if (saradc.ref++) {
		/* Return success, but not initialize again. */
		DPRINTK("End 1 saradc.ref=%d\n", saradc.ref);
		return 0;
	}

	ret = request_irq(saradc.irq, saradc_interrupt, IRQF_DISABLED, "saradc", dev);

	if (ret) {
		printk(KERN_ERR "%s: Can't allocate irq %d\n", __func__, IRQ_TSC);
		saradc.ref--;
		free_irq(saradc.irq, dev);
		goto saradc_open_out;
	}

	/* Init hr timer */
	hrtimer_init(&wmt_saradc_hrtimer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	wmt_saradc_hrtimer.function = &wmt_saradc_timeout_hrtimer;

	/* Register an input event device. */
	dev->name = "saradc",
	dev->phys = "saradc",

	/*
	 *  Let kpad to implement key repeat.
	 */

	set_bit(EV_KEY, dev->evbit);

	for (i = 0; i < WMT_SARADC_FUNCTION_NUM; i++)
		set_bit(wmt_saradc_codes[i], dev->keybit);


	dev->keycode = wmt_saradc_codes;
	dev->keycodesize = sizeof(unsigned int);
	dev->keycodemax = WMT_SARADC_FUNCTION_NUM;

	/*
	 * For better view of /proc/bus/input/devices
	 */
	dev->id.bustype = 0;
	dev->id.vendor  = 0;
	dev->id.product = 0;
	dev->id.version = 0;

	input_register_device(dev);

	wmt_saradc_hw_init();

	DPRINTK("End2\n");
saradc_open_out:
	DPRINTK("End3\n");
	return ret;
}

static void saradc_close(struct input_dev *dev)
{
	DPRINTK("Start\n");
	if (--saradc.ref) {
		DPRINTK("End1\n");
		return;
	}

	/* Free interrupt resource */
	free_irq(saradc.irq, dev);

    /*Disable clock*/
    auto_pll_divisor(DEV_ADC, CLK_DISABLE, 0, 0);

	/* Unregister input device driver */
	input_unregister_device(dev);
	DPRINTK("End2\n");
}

static int wmt_saradc_probe(struct platform_device *pdev)
{
	unsigned long base;
	int ret = 0;
	DPRINTK("Start\n");
	saradc_dev = input_allocate_device();
	if (saradc_dev == NULL) {
		DPRINTK("End 1\n");
		return -1;
	}
	/*
	 * Simply check resources parameters.
	 */
	if (pdev->num_resources < 2 || pdev->num_resources > 3) {
		ret = -ENODEV;
		goto saradc_probe_out;
	}

	base = pdev->resource[0].start;

	saradc.irq = pdev->resource[1].start;

	saradc.regs = (struct saradc_regs_s *)ADC_BASE_ADDR;

	if (!saradc.regs) {
		ret = -ENOMEM;
		goto saradc_probe_out;
	}

	saradc_dev->open = saradc_open,
	saradc_dev->close = saradc_close,

	saradc_open(saradc_dev);
	DPRINTK("End2\n");
saradc_probe_out:

#ifndef CONFIG_SKIP_DRIVER_MSG
	printk(KERN_INFO "WMT saradc driver initialized: %s\n",
		  (ret == 0) ? "ok" : "failed");
#endif
	DPRINTK("End3\n");
	return ret;
}

static int wmt_saradc_remove(struct platform_device *pdev)
{
	DPRINTK("Start\n");
	saradc_close(saradc_dev);

	/*
	 * Free allocated resource
	 */
	/*kfree(kpad.res);
	kpad.res = NULL;

	if (kpad.regs) {
		iounmap(kpad.regs);
		kpad.regs = NULL;
	}*/

	saradc.ref = 0;
	saradc.irq = 0;

	DPRINTK("End\n");
	return 0;
}

static int wmt_saradc_suspend(struct platform_device *pdev, pm_message_t state)
{
	DPRINTK("Start\n");

	switch (state.event) {
	case PM_EVENT_SUSPEND:
		/*Disable clock*/
		auto_pll_divisor(DEV_ADC, CLK_DISABLE, 0, 0);
		break;
	case PM_EVENT_FREEZE:
	case PM_EVENT_PRETHAW:

	default:
		break;
	}

	DPRINTK("End2\n");
	return 0;
}

static int wmt_saradc_resume(struct platform_device *pdev)
{
	DPRINTK("Start\n");
	wmt_saradc_hw_init();
	DPRINTK("End\n");
	return 0;
}

static struct platform_driver wmt_saradc_driver = {
	.driver.name = "wmt-saradc",
	.probe = &wmt_saradc_probe,
	.remove = &wmt_saradc_remove,
	.suspend = &wmt_saradc_suspend,
	.resume	= &wmt_saradc_resume
};

static struct resource wmt_saradc_resources[] = {
	[0] = {
		.start  = ADC_BASE_ADDR,
		.end    = (ADC_BASE_ADDR + 0xFFFF),
		.flags  = IORESOURCE_MEM,
	},
	[1] = {
		.start  = IRQ_TSC,
		.end    = IRQ_TSC,
		.flags  = IORESOURCE_IRQ,
	},
};

static struct platform_device wmt_saradc_device = {
	.name			= "wmt-saradc",
	.id				= 0,
	.num_resources  = ARRAY_SIZE(wmt_saradc_resources),
	.resource		= wmt_saradc_resources,
};

static int __init saradc_init(void)
{
	int ret;
	int retval;
	unsigned char buf[80];
	int varlen = 80;
	char *varname = "wmt.keypad.param";
	int temp = 0, enable_saradc = 0, function_sel = 0;

	DPRINTK(KERN_ALERT "Start\n");
	/*read keypad enable*/
	retval = wmt_getsyspara(varname, buf, &varlen);
	if (retval == 0) {
		sscanf(buf, "%X:%d:%d:%d", &temp, &HW_Hz, &INT_timeout, &SW_timeout);
		enable_saradc = temp & 0xf;
		function_sel = (temp >> 4) & 0xf;
		printk(KERN_ALERT "wmt.keypad.param = %x:%d:%d:%d, enable = %x, function = %x\n",
			temp, HW_Hz, INT_timeout, SW_timeout, enable_saradc, function_sel);

		if (enable_saradc != 1 || function_sel != 1) {
			printk(KERN_ALERT "Disable SARADC as keypad function!!\n");
			return -ENODEV;
		} else if (enable_saradc == 1 && function_sel == 1)
			printk(KERN_ALERT "HW_HZ = %d, INT_time = %d, SW_timeout = %d\n",
				HW_Hz, INT_timeout, SW_timeout);
		if ((HW_Hz == 0) || (INT_timeout == 0) || (SW_timeout == 0)) {
			HW_Hz = 1000; /* 1000 Hz */
			INT_timeout = 20000; /* 20 ms */
			SW_timeout = 1000; /* 1 ms */
			printk(KERN_ALERT "wmt.keypad.param isn't correct. Set the default value\n");
			printk(KERN_ALERT "Default HW_HZ = %d, INT_time = %d, SW_timeout = %d\n",
				HW_Hz, INT_timeout, SW_timeout);
		}
	} else {
		printk(KERN_ALERT "##Warning: \"wmt.keypad.param\" not find\n");
		printk(KERN_ALERT "Default wmt.keypad.param = %x\n", temp);
		return -ENODEV;
	}

/* check saradc can switch freq.
#ifdef CONFIG_CPU_FREQ
	ret = cpufreq_register_notifier(&kpad_clock_nblock, \
		CPUFREQ_TRANSITION_NOTIFIER);

	if (ret) {
		printk(KERN_ERR "Unable to register CPU frequency " \
			"change notifier (%d)\n", ret);
	}
#endif
*/
	ret = platform_device_register(&wmt_saradc_device);
	if (ret != 0) {
		DPRINTK("End1 ret = %x\n", ret);
		return -ENODEV;
	}

	ret = platform_driver_register(&wmt_saradc_driver);
	DPRINTK("End2 ret = %x\n", ret);
	return ret;
}

static void __exit saradc_exit(void)
{
	DPRINTK("Start\n");
	platform_driver_unregister(&wmt_saradc_driver);
	platform_device_unregister(&wmt_saradc_device);
	DPRINTK("End\n");
}

module_init(saradc_init);
module_exit(saradc_exit);

MODULE_AUTHOR("WonderMedia Technologies, Inc.");
MODULE_DESCRIPTION("WMT [generic saradc] driver");
MODULE_LICENSE("GPL");
