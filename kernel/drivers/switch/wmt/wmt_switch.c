/*++ 
 *
 * Copyright c 2010  WonderMedia  Technologies, Inc.
 *
 * This program is free software: you can redistribute it and/or modify 
 * it under the terms of the GNU General Public License as published by 
 * the Free Software Foundation, either version 2 of the License, or 
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, 
 * but WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
 * GNU General Public License for more details. 
 *
 * You should have received a copy of the GNU General Public License 
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * WonderMedia Technologies, Inc.
 * 4F, 533, Chung-Cheng Road, Hsin-Tien, Taipei 231, R.O.C
--*/

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/pm.h>
#include <linux/platform_device.h>
#include <linux/version.h>
#include <linux/interrupt.h>
#include <asm/irq.h>
#include <linux/workqueue.h>
#include <linux/switch.h>
#include <linux/slab.h>
#include <linux/list.h>
#include <linux/input.h>
#include <linux/suspend.h>

#include <linux/gpio.h>
#include <mach/wmt_iomux.h>

#define WMT_SWITCH_TIMER_INTERVAL    250 // ms
#define WMT_WIREDKEY_TIMER_INTERVAL  100 // ms

#define WMT_WIREDKEY_PLUGIN_DEBOUNCE 2000 // ms
#define WMT_WIREDKEY_PRESS_COUNT     1

extern int wmt_getsyspara(char *varname, unsigned char *varval, int *varlen);

struct wmt_switch_priv {
	struct work_struct switch_work;
	struct timer_list switch_timer;

	struct input_dev *idev;
	struct work_struct wiredkey_work;
	struct timer_list wiredkey_timer;
	struct notifier_block pm_notifier;
	
	int prepare_suspend;
};

struct wmt_switch {
	struct list_head list;
	int gpio;
	int active;
	int state;
	struct switch_dev switch_dev;
};

struct wmt_wiredkey {
	struct list_head list;
	int gpio;
	int active;
	unsigned int keycode;
	int count;
};

static LIST_HEAD(wmt_switch_list);
static LIST_HEAD(wmt_wiredkey_list);

static void wmt_switch_do_work(struct work_struct *work)
{
	struct wmt_switch_priv *priv = container_of(work, struct wmt_switch_priv, switch_work);
	struct wmt_switch *wmt_switch;
	int new;

	list_for_each_entry(wmt_switch, &wmt_switch_list, list) {
		new = gpio_get_value(wmt_switch->gpio);
		if (new != wmt_switch->state) {
			if (new == wmt_switch->active)
				switch_set_state(&wmt_switch->switch_dev, 1);
			else
				switch_set_state(&wmt_switch->switch_dev, 0);
			
			wmt_switch->state = new;

			if (!list_empty(&wmt_wiredkey_list) && !strcmp(wmt_switch->switch_dev.name, "h2w")) {
				if (wmt_switch->switch_dev.state > 0) {
					// headset plugin, start hook-key detect in 2s for debounce
					mod_timer(&priv->wiredkey_timer, jiffies + msecs_to_jiffies(WMT_WIREDKEY_PLUGIN_DEBOUNCE));
				}
				else {
					// headset plugout, stop hook-key detect
					del_timer_sync(&priv->wiredkey_timer);
				}
			}
		}
	}
}

static void wmt_switch_timer_handler(unsigned long data)
{
	struct wmt_switch_priv *priv = (struct wmt_switch_priv *)data;
	schedule_work(&priv->switch_work);
	mod_timer(&priv->switch_timer, jiffies + msecs_to_jiffies(WMT_SWITCH_TIMER_INTERVAL));
}

static void wmt_wiredkey_do_work(struct work_struct *work)
{
	struct wmt_switch_priv *priv = container_of(work, struct wmt_switch_priv, wiredkey_work);
	struct wmt_wiredkey *wmt_wiredkey;
	int new;
	if (priv->prepare_suspend)
		return;
	list_for_each_entry(wmt_wiredkey, &wmt_wiredkey_list, list) {
		new = gpio_get_value(wmt_wiredkey->gpio);
		if (new != wmt_wiredkey->active) {
			// key release
			if (wmt_wiredkey->count > WMT_WIREDKEY_PRESS_COUNT) {
				printk("Wired-key(%d): release\n", wmt_wiredkey->keycode);
				input_event(priv->idev, EV_KEY, wmt_wiredkey->keycode, 0);
				input_sync(priv->idev);
			}
			wmt_wiredkey->count = 0; // reset press count caused by key-release
		}
		else {
			// key pressed
			if (wmt_wiredkey->count == WMT_WIREDKEY_PRESS_COUNT) {
				printk("Wired-key(%d): pressed\n", wmt_wiredkey->keycode);
				input_event(priv->idev, EV_KEY, wmt_wiredkey->keycode, 1);
				input_sync(priv->idev);
			}
			wmt_wiredkey->count++; // increass press count caused by key-pressed
		}
	}
}

static void wmt_wiredkey_timer_handler(unsigned long data)
{
	struct wmt_switch_priv *priv = (struct wmt_switch_priv *)data;
	schedule_work(&priv->wiredkey_work);
	mod_timer(&priv->wiredkey_timer, jiffies + msecs_to_jiffies(WMT_WIREDKEY_TIMER_INTERVAL));
}

static int wmt_switch_pm_event(struct notifier_block *this, unsigned long event, void *ptr)
{
	struct wmt_switch_priv *priv = container_of(this, struct wmt_switch_priv, pm_notifier);
	//return 0;//it goes before driver suspend, well system fails to suspend.
	//our timer can't be added again, so move it to driver suspend. 2013-9-30  
	switch (event) {
	case PM_POST_HIBERNATION:
	case PM_POST_SUSPEND:
		break;
	case PM_HIBERNATION_PREPARE:
	case PM_SUSPEND_PREPARE:
		priv->prepare_suspend = 1;// add 2013-10-21
		//del_timer_sync(&priv->switch_timer);
		//del_timer_sync(&priv->wiredkey_timer);
		break;
	default:
		return NOTIFY_DONE;
	}
	return 0;
}

static int __devinit wmt_switch_probe(struct platform_device *pdev)
{
	int ret;
	struct wmt_switch_priv *priv;
	struct wmt_switch *wmt_switch, *next;
	struct wmt_wiredkey *wmt_wiredkey, *next2;

	priv = kzalloc(sizeof(struct wmt_switch_priv), GFP_KERNEL);
	if (priv == NULL)
		return -ENOMEM;
	platform_set_drvdata(pdev, priv);

	list_for_each_entry_safe(wmt_switch, next, &wmt_switch_list, list) {
		ret = gpio_request(wmt_switch->gpio, wmt_switch->switch_dev.name);
		if (ret < 0) {
			printk("Switch(%s): gpio(%d) request failed\n", 
					wmt_switch->switch_dev.name, wmt_switch->gpio);
			list_del_init(&wmt_switch->list);
			kfree(wmt_switch);
		}
		else {
			printk("Switch(%s): gpio(%d) request success\n", 
					wmt_switch->switch_dev.name, wmt_switch->gpio);
			gpio_direction_input(wmt_switch->gpio);
			wmt_switch->state = -1;
			switch_dev_register(&wmt_switch->switch_dev);
		}
	}

	if (list_empty(&wmt_switch_list)) {
		kfree(priv);
		return -EINVAL;
	}

	list_for_each_entry_safe(wmt_wiredkey, next2, &wmt_wiredkey_list, list) {
		ret = gpio_request(wmt_wiredkey->gpio, "Wired-key");
		if (ret < 0) {
			printk("Wired-key: gpio(%d) request failed\n", wmt_wiredkey->gpio);
			list_del_init(&wmt_wiredkey->list);
			kfree(wmt_wiredkey);
		}
		else {
			printk("Wired-key: gpio(%d) request success\n", wmt_wiredkey->gpio);
			gpio_direction_input(wmt_wiredkey->gpio);
			wmt_wiredkey->count = 0;
		}
	}

	// register input device for wired-key
	priv->idev = input_allocate_device();
	if (priv->idev == NULL) {
		printk(KERN_ERR "Wired-key: input_allocate_device failed\n");
		return -ENOMEM;
	}
	priv->idev->name = "Wired-key";
	priv->idev->phys = "Wired-key";
	set_bit(EV_KEY, priv->idev->evbit);
	list_for_each_entry(wmt_wiredkey, &wmt_wiredkey_list, list) {
		set_bit(wmt_wiredkey->keycode, priv->idev->keybit);
	}
	input_register_device(priv->idev);

	// init wired-key devices
	INIT_WORK(&priv->wiredkey_work, wmt_wiredkey_do_work);
	init_timer(&priv->wiredkey_timer);
	priv->wiredkey_timer.data = (unsigned long)priv;
	priv->wiredkey_timer.function = wmt_wiredkey_timer_handler;

	// init switch devices
	INIT_WORK(&priv->switch_work, wmt_switch_do_work);
	init_timer(&priv->switch_timer);
	priv->switch_timer.data = (unsigned long)priv;
	priv->switch_timer.function = wmt_switch_timer_handler;
	mod_timer(&priv->switch_timer, jiffies + msecs_to_jiffies(WMT_SWITCH_TIMER_INTERVAL));

	// register pm event
	priv->pm_notifier.notifier_call = wmt_switch_pm_event;
	register_pm_notifier(&priv->pm_notifier);
	
	return 0;
}

static int __devexit wmt_switch_remove(struct platform_device *pdev)
{
	struct wmt_switch_priv *priv = platform_get_drvdata(pdev);
	struct wmt_switch *wmt_switch, *next;
	struct wmt_wiredkey *wmt_wiredkey, *next2;

	unregister_pm_notifier(&priv->pm_notifier);

	del_timer_sync(&priv->switch_timer);
	del_timer_sync(&priv->wiredkey_timer);

	list_for_each_entry_safe(wmt_wiredkey, next2, &wmt_wiredkey_list, list) {
		gpio_free(wmt_wiredkey->gpio);
		list_del_init(&wmt_wiredkey->list);
		kfree(wmt_wiredkey);
	}

	list_for_each_entry_safe(wmt_switch, next, &wmt_switch_list, list) {
		switch_dev_unregister(&wmt_switch->switch_dev);
		gpio_free(wmt_switch->gpio);
		list_del_init(&wmt_switch->list);
		kfree(wmt_switch);
	}

	input_unregister_device(priv->idev);

	kfree(priv);
	return 0;
}

static int wmt_switch_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct wmt_switch_priv *priv = platform_get_drvdata(pdev);
	printk("<<<< %s\n", __FUNCTION__);
	del_timer_sync(&priv->wiredkey_timer);
	del_timer_sync(&priv->switch_timer);
	return 0;
}

static int wmt_switch_resume(struct platform_device *pdev)
{
	struct wmt_switch_priv *priv = platform_get_drvdata(pdev);
	struct wmt_switch *wmt_switch;
	printk("<<<< %s\n", __FUNCTION__);
	priv->prepare_suspend = 0;//add 2013-10-21
	
	mod_timer(&priv->switch_timer, jiffies + msecs_to_jiffies(500));

	// if headset plugin, start hook-key detect
	list_for_each_entry(wmt_switch, &wmt_switch_list, list) {
		if (!list_empty(&wmt_wiredkey_list) && !strcmp(wmt_switch->switch_dev.name, "h2w")) {
			if (wmt_switch->switch_dev.state > 0)
				mod_timer(&priv->wiredkey_timer, jiffies + msecs_to_jiffies(WMT_WIREDKEY_PLUGIN_DEBOUNCE));
		}
		
		if (!strcmp(wmt_switch->switch_dev.name, "rotation") && !wmt_switch->active) 
		{
			wmt_gpio_setpull(wmt_switch->gpio, WMT_GPIO_PULL_UP);
		}
	}
	return 0;
}

static struct platform_driver wmt_switch_driver = {
	.driver = {
		   .name = "wmt-switch",
		   .owner = THIS_MODULE,
		   },
	.probe   = wmt_switch_probe,
	.remove  = __devexit_p(wmt_switch_remove),
	.suspend = wmt_switch_suspend,
	.resume  = wmt_switch_resume,
};

static __init int wmt_switch_init(void)
{
	int ret, gpio, active, keycode;
	char buf[128];
	int len = ARRAY_SIZE(buf);
	struct wmt_switch *wmt_switch;
	struct wmt_wiredkey *wmt_wiredkey;
	
	ret = wmt_getsyspara("wmt.switch.sim", buf, &len);
	if (ret == 0) {
		sscanf(buf, "%d:%d", &gpio, &active);
		if (gpio_is_valid(gpio)) {
			wmt_switch = kzalloc(sizeof(*wmt_switch), GFP_KERNEL);
			if (wmt_switch) {
				wmt_switch->switch_dev.name = "sim";
				wmt_switch->gpio = gpio;
				wmt_switch->active = active;
				list_add_tail(&wmt_switch->list, &wmt_switch_list);
			}
		}
	}

	ret = wmt_getsyspara("wmt.switch.sim2", buf, &len);
	if (ret == 0) {
		sscanf(buf, "%d:%d", &gpio, &active);
		if (gpio_is_valid(gpio)) {
			wmt_switch = kzalloc(sizeof(*wmt_switch), GFP_KERNEL);
			if (wmt_switch) {
				wmt_switch->switch_dev.name = "sim2";
				wmt_switch->gpio = gpio;
				wmt_switch->active = active;
				list_add_tail(&wmt_switch->list, &wmt_switch_list);
			}
		}
	}

	ret = wmt_getsyspara("wmt.switch.hs", buf, &len);
	if (ret == 0) {
		sscanf(buf, "%d:%d", &gpio, &active);
		if (gpio_is_valid(gpio)) {
			wmt_switch = kzalloc(sizeof(*wmt_switch), GFP_KERNEL);
			if (wmt_switch) {
				wmt_switch->switch_dev.name = "h2w";
				wmt_switch->gpio = gpio;
				wmt_switch->active = active;
				list_add_tail(&wmt_switch->list, &wmt_switch_list);
			}
		}
	}
	//add by rambo 2013-8-28, for lock key. 
	//well also can be used for other ways,so we call emukey.
	ret = wmt_getsyspara("wmt.switch.rotation", buf, &len);
	if (ret == 0) { 
		sscanf(buf, "%d:%d", &gpio, &active);
		if (gpio_is_valid(gpio)) {
			wmt_switch = kzalloc(sizeof(*wmt_switch), GFP_KERNEL);
			if (wmt_switch) {
				wmt_switch->switch_dev.name = "rotation";
				wmt_switch->gpio = gpio;
				wmt_switch->active = active;
				list_add_tail(&wmt_switch->list, &wmt_switch_list);
			}
			if (active == 0)
				wmt_gpio_setpull(gpio, WMT_GPIO_PULL_UP);
		}
	}
	
	ret = wmt_getsyspara("wmt.wirekey.hook", buf, &len);
	if (ret == 0) {
		sscanf(buf, "%d:%d:%d", &gpio, &active, &keycode);
		if (gpio_is_valid(gpio)) {
			wmt_wiredkey = kzalloc(sizeof(*wmt_wiredkey), GFP_KERNEL);
			if (wmt_wiredkey) {
				wmt_wiredkey->gpio = gpio;
				wmt_wiredkey->active = active;
				wmt_wiredkey->keycode = keycode;
				list_add_tail(&wmt_wiredkey->list, &wmt_wiredkey_list);
			}
		}
	}
	
	return platform_driver_register(&wmt_switch_driver);
}
module_init(wmt_switch_init);

static __exit void wmt_switch_exit(void)
{
	platform_driver_unregister(&wmt_switch_driver);
}
module_exit(wmt_switch_exit);

MODULE_DESCRIPTION("WMT GPIO Switch");
MODULE_AUTHOR("WonderMedia Technologies, Inc.");
MODULE_LICENSE("GPL");
