/*
 * wmt_charger.c - WonderMedia Charger Driver.
 *
 * Copyright (C) 2013  WonderMedia Technologies, Inc.  
 *
 * This program is free software; you can redistribute it and/or modify it
 * under  the terms of the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the License, or (at your
 * option) any later version.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <linux/module.h>
#include <linux/slab.h>
#include <linux/power_supply.h>
#include <linux/platform_device.h>
#include <mach/hardware.h>
#include <linux/gpio.h>
#include <mach/wmt_iomux.h>
#include <linux/power/wmt_charger_common.h>
#include <mach/wmt_env.h>

#define DRVNAME	"wmt-charger"

enum {
	STATE_WORKING,
	STATE_SLEEPING,
};

#undef pr_err
#undef pr_info
#define pr_err(fmt, args...)	printk("[" DRVNAME "] " fmt, ##args)
#define pr_info(fmt, args...)	printk("[" DRVNAME "] " fmt, ##args)

extern int wmt_usb_connected(void);

enum power_type {
	CABLE_TYPE_AC = 0,
	CABLE_TYPE_USB,
	CABLE_TYPE_MAX,
};

struct wmt_charger {
	int	type;
	int	current_gpio;
	int	current_small_level;
	int	mode;        /* current control gpio working mode */
	int	pc_charging; /* pc charging enable */

	int	online;
	int	state;
	struct delayed_work work;
};

static struct wmt_charger *wmt_charger;

static int parse_charger_param(struct wmt_charger *wc)
{
	char env[] = "wmt.charger.param";
	char s[64];
	char *p = NULL;
	size_t l = sizeof(s);
	int rc;
	int enable, type, gpio, active, mode, pc_charging;

	if (wmt_getsyspara(env, s, &l)) {
		pr_err("read %s fail!\n", env);
		return -EINVAL;
	}

	type = gpio = -1;
	active = mode = pc_charging = 0;

	enable = s[0] - '0';

	p = strchr(s, ':');p++;
	if (strncmp(p, "gpio", 4)) {
		pr_err("Not gpio charger\n");
		return -ENODEV;
	}
	
	p = strchr(p, ':');p++;
	rc = sscanf(p, "%d:%d:%d:%d:%d", &type, &gpio, &active, &mode, &pc_charging);
	if (rc < 1) {
		pr_err("Bad uboot env: %s %s\n", env, s);
		return -EINVAL;
	}

	if (type != CABLE_TYPE_AC && type != CABLE_TYPE_USB) {
		pr_err("error type %d\n", type);
		return -EINVAL;
	}

	if (gpio_is_valid(gpio)) {
		if (gpio_request(gpio, "current ctrl")) {
			pr_err("gpio(%d) request failed\n", gpio);
			gpio = -1;
		}
	}

	pr_info("%s, gpio%d, active-%d, mode %d, pc_charging %d\n",
		type ? "Micro-USB" : "AC", gpio, active, mode, pc_charging);

	wc->type = type;
	wc->current_gpio = gpio;
	wc->current_small_level = active;
	wc->mode = mode;
	wc->pc_charging = pc_charging;
	return 0;
}

static inline void set_large_current(struct wmt_charger *wc, bool en)
{
	if (gpio_is_valid(wc->current_gpio)) {
		gpio_direction_output(wc->current_gpio,
				      en ? !wc->current_small_level :
					      wc->current_small_level);
		printk(KERN_DEBUG "set %s current\n", en ? "large" : "small");
	}
}

static void current_setting(struct wmt_charger *wc)
{
	/* cost-down charger: 
	 * 	system working - small current 
	 * 	system sleeping - large current
	 */
	if (wc->mode == 1) {
		if (wc->state == STATE_SLEEPING)
			set_large_current(wc, true);
		else
			set_large_current(wc, false);
		return;
	}

	/* normal charger:
	 * 	USB cable conntect to PC - small current
	 * 	USB cable conntect to adapter - large current
	 * 	AC cable - large current
	 */
	if (wmt_charger_is_dc_plugin()) {
		switch (wc->type) {
		case CABLE_TYPE_AC:
			set_large_current(wc, true);
			break;
		case CABLE_TYPE_USB:
			if (wmt_usb_connected())
				set_large_current(wc, false);
			else
				set_large_current(wc, true);
			break;
		default:
			set_large_current(wc, false);
			break;
		}
	} else
		set_large_current(wc, false);
}

static int wmt_power_get_property(struct power_supply *psy,
				  enum power_supply_property psp,
				  union power_supply_propval *val)
{
	struct wmt_charger *wc = wmt_charger;

	if (!wc)
		return -EINVAL;

	switch (psp) {
	case POWER_SUPPLY_PROP_ONLINE:
		val->intval = wc->online;
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static enum power_supply_property wmt_power_props[] = {
	POWER_SUPPLY_PROP_ONLINE,
};

static char *wmt_power_supplied_to[] = {
	"wmt-bat",
	"battery",
};

static struct power_supply wmt_psy_ac = {
	.name = "wmt-ac",
	.type = POWER_SUPPLY_TYPE_MAINS,
	.supplied_to = wmt_power_supplied_to,
	.num_supplicants = ARRAY_SIZE(wmt_power_supplied_to),
	.properties = wmt_power_props,
	.num_properties = ARRAY_SIZE(wmt_power_props),
	.get_property = wmt_power_get_property,
};

static void wmt_charger_work(struct work_struct *work)
{
	struct wmt_charger *wc = wmt_charger;
	int online = wmt_charger_is_dc_plugin();

	/* report Discharging while USB connect to PC */
	if (!wc->pc_charging &&
	    wc->type == CABLE_TYPE_USB && wmt_usb_connected())
		online = 0;

	if (wc->online != online) {
		wc->online = online;
		power_supply_changed(&wmt_psy_ac);
	}
	current_setting(wc);
}

void wmt_charger_current_config(void)
{
	struct wmt_charger *wc = wmt_charger;
	if (wc) {
		schedule_delayed_work(&wc->work, 0);
	}
}
EXPORT_SYMBOL(wmt_charger_current_config);

static inline int dcdet_irq_enable(void)
{
	pmc_enable_wakeup_isr(WKS_DCDET,4);
	return 0;
}

static inline int dcdet_irq_disable(void)
{
	pmc_disable_wakeup_isr(WKS_DCDET);
	return 0;
}

static inline int dcdet_irq_clear_status(void)
{
	pmc_clear_intr_status(WKS_DCDET);
	return 0;
}

static irqreturn_t dcdet_interrupt(int irq, void *data)
{
	int intstatus = PMCIS_VAL;
	struct wmt_charger *wc = wmt_charger;

	if(intstatus & BIT27) {
		/* Clear Wakeup0 status */
		printk("dcdet irq\n");
		dcdet_irq_clear_status();
		schedule_delayed_work(&wc->work, HZ/2);
		return IRQ_HANDLED;
	}
    return IRQ_NONE;
}

static int dcdet_irq_init(void)
{
	int ret;
	if (*(volatile unsigned int *)0xfe120000 == 0x35100101) {
		ret = request_irq(IRQ_PMC_WAKEUP,
				  dcdet_interrupt,
				  (IRQF_SHARED | IRQF_NO_SUSPEND),
				  "gpio-charger",wmt_charger);                 
	} else {
		ret = request_irq(IRQ_PMC_WAKEUP,
				  dcdet_interrupt,
				  IRQF_SHARED,
				  "gpio-charger",wmt_charger);
	}       

	if (ret < 0)
		printk("Fail to request dcdet irq\n");

	return ret;
}

static int wmt_charger_probe(struct platform_device *pdev)
{
	int ret;
	struct wmt_charger *wc;

	wc = kzalloc(sizeof(*wc), GFP_KERNEL);
	if (!wc)
		return -ENOMEM;

	ret = parse_charger_param(wc);
	if (ret)
		goto err;

	wc->state = STATE_WORKING;
	wmt_charger = wc;

	ret = power_supply_register(&pdev->dev, &wmt_psy_ac);
	if (ret) {
		dev_err(&pdev->dev, "can not register charger power supply\n");
		goto err;
	}

	INIT_DELAYED_WORK(&wc->work, wmt_charger_work);
	schedule_delayed_work(&wc->work, 0);
	dcdet_irq_init();
	dcdet_irq_enable();

	return 0;

err:
	kfree(wc);
	wmt_charger = NULL;
	return ret;
}

static int wmt_charger_remove(struct platform_device *pdev)
{
	struct wmt_charger *wc = wmt_charger;

	if (!wc) {
		return -EINVAL;
	}

	dcdet_irq_disable();

	power_supply_unregister(&wmt_psy_ac);
	cancel_delayed_work_sync(&wc->work);
	if (wc->current_gpio)
		gpio_free(wc->current_gpio);
	kfree(wc);
	wmt_charger = NULL;

	return 0;
}

static int wmt_charger_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct wmt_charger *wc = wmt_charger;
	if (wc) {
		wc->state = STATE_SLEEPING;
		pr_info("wc->state %d\n", wc->state);
		schedule_delayed_work(&wc->work, 0);
		cancel_delayed_work_sync(&wc->work);
	}
	return 0;
}

static int wmt_charger_resume(struct platform_device *pdev)
{
	struct wmt_charger *wc = wmt_charger;
	if (wc) {
		wc->state = STATE_WORKING;
		pr_info("wc->state %d\n", wc->state);
		schedule_delayed_work(&wc->work, HZ/2);
	}
	return 0;
}

static struct platform_driver wmt_charger_driver = {
	.driver = {
		.name = DRVNAME,
	},
	.probe = wmt_charger_probe,
	.remove = wmt_charger_remove,
	.suspend = wmt_charger_suspend,
	.resume = wmt_charger_resume,
};

static struct platform_device *pdev;

static int __init wmt_charger_init(void)
{
	int ret;

	ret = platform_driver_register(&wmt_charger_driver);
	if (ret)
		return ret;

	pdev = platform_device_register_simple(DRVNAME, -1, NULL, 0);
	if (IS_ERR(pdev)) {
		ret = PTR_ERR(pdev);
		platform_driver_unregister(&wmt_charger_driver);
	}
	return ret;
}

static void __exit wmt_charger_exit(void)
{
	platform_device_unregister(pdev);
	platform_driver_unregister(&wmt_charger_driver);
}

module_init(wmt_charger_init);
module_exit(wmt_charger_exit);

MODULE_AUTHOR("Sam Mei");
MODULE_DESCRIPTION("WonderMedia Charger Driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:wmt-charger");

