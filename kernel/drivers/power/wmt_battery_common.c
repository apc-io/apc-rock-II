/*++
	linux/drivers/power/wmt_battery_common.c

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

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <mach/hardware.h>

#include <linux/gpio.h>
#include <mach/wmt_iomux.h>

#include <linux/power/wmt_charger_common.h>

extern int wmt_getsyspara(char *varname, char *varval, int *varlen);

struct wmt_batgpio_set {
	int active;
	int gpionum;
};

static struct wmt_batgpio_set pin_chargerstatus;

static int parse_batt_param(const char *devname)
{
	char env[] = "wmt.gpi.bat";
	char buf[64];
	char *s;
	size_t l = sizeof(buf);
	struct wmt_batgpio_set *p;
	int ret;

	if (wmt_getsyspara(env, buf, &l)) {
		pr_err("read %s fail!\n", env);
		return -EINVAL;
	}

	if (strncmp(buf, devname, strlen(devname))) {
		pr_err("Batt: match %s failed\n", devname);
		return -EINVAL;
	}

	p = &pin_chargerstatus;

	if (!(s = strchr(buf, ':')))
		return -EINVAL;

	ret = sscanf(s, ":%d:%d", &p->gpionum, &p->active);
	if (ret < 2) {
		pr_err("Batt: bat env param: %s\n", buf);
		return -EINVAL;
	}

	printk("Found batt dev %s: gpio-%c, active %d\n", devname, 
			(p->gpionum == -1) ? 'N' : ('0'+ p->gpionum), p->active);

	if (p->gpionum == -1) 
		return 0;

	ret = gpio_request(p->gpionum, "Charger Status");
	if (ret) {
		pr_err("request gpio %d failed\n", p->gpionum);
		return ret;
	}

	wmt_gpio_setpull(p->gpionum, (p->active) ? WMT_GPIO_PULL_DOWN : WMT_GPIO_PULL_UP);

	return 0;
}

int wmt_charger_gpio_probe(const char *devname)
{
	int rc;

	if ((rc = parse_batt_param(devname)) < 0)
		return rc;

	return 0;
}
EXPORT_SYMBOL(wmt_charger_gpio_probe);

void wmt_charger_gpio_release(void)
{
	struct wmt_batgpio_set *p = &pin_chargerstatus;
	if (p->gpionum >= 0) 
		gpio_free(p->gpionum);
}
EXPORT_SYMBOL(wmt_charger_gpio_release);

int wmt_charger_is_bat_charging(void)
{
	struct wmt_batgpio_set *p = &pin_chargerstatus;

	if (p->gpionum >= 0) 
		return (gpio_get_value(p->gpionum) == !!p->active);
	else
		return 1;
}
EXPORT_SYMBOL(wmt_charger_is_bat_charging);

extern int g2214_is_dc_plugin(void);
int wmt_charger_is_dc_plugin(void)
{
	return ((REG8_VAL(PM_CTRL_BASE_ADDR + 0x005d) & 0x01) && g2214_is_dc_plugin());
}
EXPORT_SYMBOL(wmt_charger_is_dc_plugin);

int wmt_charger_is_charging_full(void)
{
	return (wmt_charger_is_dc_plugin() && !wmt_charger_is_bat_charging()) ? 1 : 0;
}
EXPORT_SYMBOL(wmt_charger_is_charging_full);

extern void g2214_pc_connected(void);
void wmt_pc_connected(void)
{
    //ap5056_pc_connected();
	printk("PC connected, charger work callback\n");
	g2214_pc_connected();
}

