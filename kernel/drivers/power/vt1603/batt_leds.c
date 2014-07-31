/*
 * batt_leds.c - WonderMedia Battery LED Driver.
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

#include <linux/leds.h>
#include <linux/rtc.h>
#include <linux/platform_device.h>
#include <linux/power_supply.h>
#include <linux/gpio.h>
#include <mach/wmt_iomux.h>
#include <mach/wmt_env.h>

enum {
	LED_GREEN = 0,
	LED_RED,
};

struct batt_led_trigger {
	char *name;
	struct led_trigger *trigger;
};

#define RTC_NAME	"rtc0"
static struct rtc_device *rtc_dev = NULL;
static bool battery_led_registered = false;

static struct batt_led_trigger batt_led_triggers[] = {
	[LED_GREEN] = {
		.name = "bat-green",
	},
	[LED_RED] = {
		.name = "bat-red",
	}
};

static struct gpio_led batt_leds[] __initdata = {
	[LED_GREEN] = {
		.name			= "bat-green",
		.default_trigger	= "bat-green",
		.retain_state_suspended	= 1,
		.active_low = 0,
	},
	[LED_RED] = {
		.name			= "bat-red",
		.default_trigger	= "bat-red",
		.retain_state_suspended	= 1,
		.active_low = 0,
	},
};

static struct gpio_led_platform_data batt_leds_data = {
	.leds = batt_leds,
	.num_leds = ARRAY_SIZE(batt_leds),
};

static struct platform_device batt_leds_dev = {
	.name		= "leds-gpio",
	.id		= -1,
	.dev		= {
		.platform_data = &batt_leds_data,
	},
};

static int parse_battery_led_param(void)
{
	char env[] = "wmt.battery.led";
	char s[64];
	size_t l = sizeof(s);
	int rc;
	int r, g;

	if (wmt_getsyspara(env, s, &l)) {
		pr_err("read %s fail!\n", env);
		return -EINVAL;
	}

	rc = sscanf(s, "%d:%d", &r, &g);
	if (rc < 2) {
		pr_err("Bad uboot env: %s %s\n", env, s);
		return -EINVAL;
	}

	if (!gpio_is_valid(r) || !gpio_is_valid(g))
		return -EINVAL;

	batt_leds[LED_GREEN].gpio = g;
	batt_leds[LED_RED].gpio = r;

	pr_info("battery-led: red(gpio%d), green(gpio%d)\n", r, g);
	return 0;
}

int batt_leds_update(int status)
{
	struct led_trigger *r = batt_led_triggers[LED_RED].trigger;
	struct led_trigger *g = batt_led_triggers[LED_GREEN].trigger;

	if (battery_led_registered == false)
		return 0;

	switch (status) {
	case POWER_SUPPLY_STATUS_CHARGING:
		led_trigger_event(r, LED_FULL);
		led_trigger_event(g, LED_OFF);
		break;
	case POWER_SUPPLY_STATUS_FULL:
		led_trigger_event(r, LED_OFF);
		led_trigger_event(g, LED_FULL);
		break;
	case POWER_SUPPLY_STATUS_DISCHARGING:
	default:
		led_trigger_event(r, LED_OFF);
		led_trigger_event(g, LED_OFF);
		break;
	}
	return 0;
}

void batt_leds_suspend_prepare(void)
{
	struct rtc_wkalrm tmp;
	unsigned long time, now;
	unsigned long add = 30;   /* seconds */

	if (battery_led_registered == false)
		return;

	if (!rtc_dev) {
		rtc_dev = rtc_class_open(RTC_NAME);
		if (IS_ERR_OR_NULL(rtc_dev)) {
			rtc_dev = NULL;
			pr_err("Cannot get RTC %s, %ld.\n", RTC_NAME, PTR_ERR(rtc_dev));
			return;
		}
	}

	tmp.enabled = 1;
	rtc_read_time(rtc_dev, &tmp.time);
	rtc_tm_to_time(&tmp.time, &now);
	time = now + add;

	rtc_time_to_tm(time, &tmp.time);
	rtc_set_alarm(rtc_dev, &tmp);

}

void batt_leds_resume_complete(void)
{
	if (rtc_dev) {
		rtc_class_close(rtc_dev);
		rtc_dev = NULL;
	}
}

int batt_leds_setup(void)
{
	int i;

	if (parse_battery_led_param())
		return -EINVAL;

	platform_device_register(&batt_leds_dev);

	for (i = 0; i < ARRAY_SIZE(batt_led_triggers); ++i)
		led_trigger_register_simple(batt_led_triggers[i].name,
					    &batt_led_triggers[i].trigger);
	battery_led_registered = true;
	return 0;
}

void batt_leds_cleanup(void)
{
	int i;

	if (battery_led_registered == true) {
		platform_device_unregister(&batt_leds_dev);
		for (i = 0; i < ARRAY_SIZE(batt_led_triggers); ++i)
			led_trigger_unregister_simple(batt_led_triggers[i].trigger);
		battery_led_registered = false;
	}
}

