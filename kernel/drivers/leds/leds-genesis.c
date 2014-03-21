/* LEDs driver for Genesis MID
*
* Copyright (c) 2013 WonderMedia Technologies, Inc.
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 as
* published by the Free Software Foundation.
*
*/
/*********************************** History ***********************************
* Version 1.0 , HowayHuo, 2013/5/7
* First version: Created by Howayhuo
*
********************************************************************************/
#include <linux/init.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <linux/leds.h>
#include <linux/gpio.h>
#include <mach/wmt_iomux.h>
#include <linux/platform_device.h>

#define ENV_GENESIS_LED "wmt.gpo.logoled"

extern int wmt_getsyspara(char *varname, unsigned char *varval, int *varlen);

struct led_operation_t {
	unsigned int light;
	unsigned int gpiono;
	unsigned int act;
};

static struct led_operation_t *led_gpio_pin;
static int genesis_led_enable;

static void genesis_led_light(int is_light)
{
	if(!led_gpio_pin)
		return;

	if(is_light) {
		if(led_gpio_pin->act)
			gpio_direction_output(led_gpio_pin->gpiono, 1);
		else
			gpio_direction_output(led_gpio_pin->gpiono, 0);
	}else {
		if(led_gpio_pin->act)
			gpio_direction_output(led_gpio_pin->gpiono, 0);
		else
			gpio_direction_output(led_gpio_pin->gpiono, 1);
	}
}

static void genesis_led_turn_on(int on)
{
	if(genesis_led_enable)
		genesis_led_light(on);
}

static void set_led_brightness(struct led_classdev *cdev, enum led_brightness b)
{
	genesis_led_turn_on(b);
}

static ssize_t attr_led_enable_show(struct device *dev,
			       struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", genesis_led_enable);
}

static ssize_t attr_led_enable_store(struct device *dev,
			       struct device_attribute *attr,
			       const char *buf, size_t size)
{
	unsigned long val;

	if (strict_strtoul(buf, 10, &val))
		return -EINVAL;

	genesis_led_enable = val;

	if(genesis_led_enable)
		genesis_led_light(1);
	else
		genesis_led_light(0);

	return size;
}

static DEVICE_ATTR(enable, 0644, attr_led_enable_show, attr_led_enable_store);

static int __devinit genesis_led_probe(struct platform_device *pdev)
{
	int i, num;
	int ret = 0;
	const struct gpio_led_platform_data	*pdata;
	struct led_classdev	*leds;
	unsigned char buf[100];
	int buflen = 100;

	pdata = pdev->dev.platform_data;
	if (!pdata || pdata->num_leds < 1)
		return -ENODEV;

	leds = kzalloc(pdata->num_leds * sizeof(*leds), GFP_KERNEL);
	if (!leds)
		return -ENOMEM;

	for (i = 0; i < pdata->num_leds; i++) {
		struct led_classdev 	*led = leds + i;
		const struct gpio_led	*dat = pdata->leds + i;

		led->name = dat->name;
		led->brightness = 0;
		led->brightness_set = set_led_brightness;
		led->default_trigger = dat->default_trigger;

		ret = led_classdev_register(&pdev->dev, led);
		if (ret < 0)
			goto err0;

		ret = device_create_file(led->dev, &dev_attr_enable);
		if(ret)
			goto err0;
	}

	platform_set_drvdata(pdev, leds);

	if(wmt_getsyspara(ENV_GENESIS_LED, buf, &buflen))
		goto exit0;

	led_gpio_pin = kzalloc(sizeof(struct led_operation_t), GFP_KERNEL);
	if(!led_gpio_pin)
		goto exit0;

	num = sscanf(buf, "%d:%d:%d", &led_gpio_pin->light, &led_gpio_pin->gpiono,
		&led_gpio_pin->act);

	if(num != 3) {
		pr_err("the param of genesis-led is error. param num = %d\n", num);
		goto exit0;
	}

	ret = gpio_request(led_gpio_pin->gpiono, "genesis-led");
	if(ret < 0) {
		pr_err("gpio(%d) request fail for genesis-ed\n", led_gpio_pin->gpiono);
		goto exit0;
	}

	genesis_led_light(led_gpio_pin->light);

	return 0;

err0:
	if (i > 0) {
		for (i = i - 1; i >= 0; i--) {
			device_remove_file((leds + i)->dev, &dev_attr_enable);
			led_classdev_unregister(leds + i);
		}
	}

	kfree(leds);

	return ret;

exit0:
	if(led_gpio_pin) {
		kfree(led_gpio_pin);
		led_gpio_pin = NULL;
	}

	return 0;
}

static int __devexit genesis_led_remove(struct platform_device *pdev)
{
	const struct gpio_led_platform_data	*pdata;
	struct led_classdev				*leds;
	unsigned				i;

	pdata = pdev->dev.platform_data;
	leds = platform_get_drvdata(pdev);

	for (i = 0; i < pdata->num_leds; i++) {
		struct led_classdev		*led = leds + i;
		device_remove_file(led->dev, &dev_attr_enable);
		led_classdev_unregister(led);
	}

	kfree(leds);
	platform_set_drvdata(pdev, NULL);

	if(led_gpio_pin) {
		genesis_led_light(0);
		gpio_free(led_gpio_pin->gpiono);
		kfree(led_gpio_pin);
		led_gpio_pin = NULL;
	}

	return 0;
}

static int genesis_led_suspend(struct platform_device *pdev,
	pm_message_t state)
{
	return 0;
}

static int genesis_led_resume(struct platform_device *pdev)
{

	if(!genesis_led_enable || !led_gpio_pin)
		return 0;

	gpio_re_enabled(led_gpio_pin->gpiono);

	return 0;
}

static void genesis_led_release(struct device *device)
{
    return;
}


static struct gpio_led genesis_leds[] = {
	{
		.name			= "genesis-led",
		.default_trigger	= "genesis",
	},
};

static struct gpio_led_platform_data lcd_backlight_data = {
	.leds		= genesis_leds,
	.num_leds	= ARRAY_SIZE(genesis_leds),
};

static struct platform_device genesis_led_device = {
	.name           = "genesis-led",
	.id             = 0,
	.dev			= {
		.platform_data = &lcd_backlight_data,
		.release = genesis_led_release,
	},
};

static struct platform_driver genesis_led_driver = {
	.probe		= genesis_led_probe,
	.remove		= __devexit_p(genesis_led_remove),
	.suspend    = genesis_led_suspend,
	.resume     = genesis_led_resume,
	.driver		= {
		.name	= "genesis-led",
		.owner	= THIS_MODULE,
	},
};

static int __init genesis_led_init(void)
{
	int ret;

	ret = platform_device_register(&genesis_led_device);
	if(ret) {
		pr_err("Can not register genesis led device\n");
		return ret;
	}

	ret = platform_driver_register(&genesis_led_driver);
	if(ret) {
		pr_err("Can not register genesis led driver\n");
		platform_device_unregister(&genesis_led_device);
		return ret;
	}

	return 0;
}

static void __exit genesis_led_exit(void)
{
	platform_driver_unregister(&genesis_led_driver);
	platform_device_unregister(&genesis_led_device);
}

module_init(genesis_led_init);
module_exit(genesis_led_exit);

MODULE_AUTHOR("WMT ShenZhen Driver Team");
MODULE_DESCRIPTION("Genesis LED driver");
MODULE_LICENSE("GPL");

