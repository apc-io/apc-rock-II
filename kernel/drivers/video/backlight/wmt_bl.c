/*++
	linux/drivers/video/backlight/wmt_bl.c

	Copyright (c) 2013  WonderMedia Technologies, Inc.

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
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/pwm_backlight.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/leds.h>

#include <mach/wmt_env.h>
#include "wmt_bl.h"

#define LTH_BRIGHTNESS 0
#define DFT_BRIGHTNESS 180
#define DFT_PWM0_FREQ  4	/* kHz */

static struct {
	int gpio;
	int active;
} power = { -1, 0, };

static int backlight_delay = 0;

static struct led_trigger *genesis_led;
static BLOCKING_NOTIFIER_HEAD(bl_chain_head);

int register_bl_notifier(struct notifier_block *nb)
{
	return blocking_notifier_chain_register(&bl_chain_head, nb);
}
EXPORT_SYMBOL_GPL(register_bl_notifier);

int unregister_bl_notifier(struct notifier_block *nb)
{
	return blocking_notifier_chain_unregister(&bl_chain_head, nb);
}
EXPORT_SYMBOL_GPL(unregister_bl_notifier);

static inline int bl_notifier_call_chain(unsigned long val)
{
	int ret = blocking_notifier_call_chain(&bl_chain_head, val, NULL);
	return notifier_to_errno(ret);
}

static void backlight_on(bool on)
{
	static int old_state = -1;

	if (!gpio_is_valid(power.gpio))
		return;

	if (old_state != on) {
		if (old_state == false) {
			/* delay 100 ms between 'lcd power on' and 'backlight
			 * power on' at resume sequence. */
			msleep(100);
			/* wmt.backlight.delay */
			msleep(backlight_delay);
		}
		gpio_direction_output(power.gpio, on ? power.active : !power.active);
		led_trigger_event(genesis_led, on ? LED_FULL : LED_OFF);
		old_state = on;

		bl_notifier_call_chain(on ? BL_OPEN : BL_CLOSE);
	}
}

static int wmt_bl_notify(struct device *dev, int brightness)
{
	return brightness;
}

static void wmt_bl_notify_after(struct device *dev, int brightness)
{
	backlight_on(brightness != 0);
}

#define KHZ2PICOS(hz)	(1000000000/(hz))
static struct platform_pwm_backlight_data wm8880_pwmbl_data = {
	.pwm_id		= 0,
	.invert		= 1,
	.lth_brightness = LTH_BRIGHTNESS,	/* low threshold */
	.hth_brightness = 256,			/* high threshold */
	.max_brightness	= 256,
	.dft_brightness = DFT_BRIGHTNESS,
	.pwm_period_ns	= KHZ2PICOS(DFT_PWM0_FREQ*1000), /* revisit when clocks are implemented */
	.notify		= wmt_bl_notify,
	.notify_after	= wmt_bl_notify_after,
};

struct platform_device wm8880_device_pwmbl = {
	.name		= "pwm-backlight",
	.id		= 0,
	.dev		= {
		.platform_data = &wm8880_pwmbl_data,
	},
};

static int parse_backlight_param(void)
{
	static char env[] = "wmt.backlight.param";
	struct platform_pwm_backlight_data *pdata = &wm8880_pwmbl_data;
	uint8_t buf[64];
	size_t l = sizeof(buf);
	int pwm_freq = DFT_PWM0_FREQ;
	int rc;

	if (wmt_getsyspara(env, buf, &l)) {
		pr_err("please set %s\n", env);
		return -EINVAL;
	}

	/* wmt.backlight.param
	 * <pwmid>:<invert>:<gpio>:<active>:
	 * <lth_brightness>:<hth_brightness>:<pwm0freq>:<dft_brightness>
	 */
	rc = sscanf(buf, "%d:%d:%d:%d:%d:%d:%d:%d",
		    &pdata->pwm_id,
		    &pdata->invert,
		    &power.gpio, &power.active,
		    &pdata->lth_brightness,
		    &pdata->hth_brightness,
		    &pwm_freq,
		    &pdata->dft_brightness);

	if ((rc < 4)) {
		pr_err("bad env %s %s\n", env, buf);
		return -EINVAL;
	}
	if ((pdata->hth_brightness > pdata->max_brightness) ||
	    (pdata->lth_brightness > pdata->hth_brightness))
		pdata->hth_brightness = pdata->max_brightness;

	pdata->pwm_period_ns = KHZ2PICOS(pwm_freq * 1000);

	pr_info("backlight param: pwm%d(%dkHz), invert %d, brightness %d(%d~%d) ",
		pdata->pwm_id, pwm_freq, pdata->invert, pdata->dft_brightness,
		pdata->lth_brightness, pdata->hth_brightness);

	if (gpio_is_valid(power.gpio)) {
		if (gpio_request(power.gpio, "pwm_bl switch")) {
			pr_warning("gpio request %d failed\n", power.gpio);
			power.gpio = -1;
		}
		pr_info("gpio%d (active %d)\n", power.gpio, power.active);
	} else
		pr_info("no gpio control\n");

	/*  parse wmt.backlight.delay */
	l = sizeof(buf);
	if (wmt_getsyspara("wmt.backlight.delay", buf, &l) == 0) {
		sscanf(buf, "%d", &backlight_delay);
	}
	pr_info("backlight delay: %dms\n", backlight_delay);

	return 0;
}

static int __init wmt_bl_init(void)
{
	int rc;

	rc = parse_backlight_param();
	if (rc)
		return rc;

	led_trigger_register_simple("genesis", &genesis_led);

	return platform_device_register(&wm8880_device_pwmbl);
}

static void __exit wmt_bl_exit(void)
{
	led_trigger_unregister_simple(genesis_led);
	platform_device_unregister(&wm8880_device_pwmbl);
}

module_init(wmt_bl_init);
module_exit(wmt_bl_exit);

