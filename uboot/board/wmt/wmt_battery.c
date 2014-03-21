/*++ 
Copyright (c) 2013 WonderMedia Technologies, Inc.

This program is free software: you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free Software 
Foundation, either version 2 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details. You
should have received a copy of the GNU General Public License along with this
program. If not, see http://www.gnu.org/licenses/>.

WonderMedia Technologies, Inc.
10F, 529, Chung-Cheng Road, Hsin-Tien, Taipei 231, R.O.C.
--*/

#include <common.h>
#include <command.h>
#include <asm/errno.h>

#include "include/wmt_iomux.h"
#include "include/common_def.h"
#include "wmt_battery.h"
#include "../../board/wmt/include/wmt_pmc.h"

#define GPIO_BASE_ADDR 		0xD8110000
#define PM_CTRL_BASE_ADDR	0xD8130000

#define mdelay(n)       udelay((n)*1000)

#undef bool
typedef enum _bool {false, true} bool;

extern struct charger_dev gpio_charger_dev;
extern struct charger_dev g2214_charger_dev;
extern struct bat_dev vt1603_battery_dev;
extern struct bat_dev ug31xx_battery_dev;

struct charger_param charger_param;
static struct battery_param battery_param;

static struct bat_dev * battery_devices[] = {
	&vt1603_battery_dev,
	&ug31xx_battery_dev,
};

static struct charger_dev * charger_devices[] = {
	&gpio_charger_dev,
	&g2214_charger_dev,
};

static int parse_battery_param(struct battery_param *bp)
{
	static char UBOOT_ENV[] = "wmt.gpi.bat";
	char *env;
	int i;

	env = getenv(UBOOT_ENV);
	if (!env) {
		printf("please setenv wmt.gpi.bat\n");
		return -EINVAL;
	}

	for (i = 0; i < ARRAY_SIZE(battery_devices); i++) {
		if (!strncmp(battery_devices[i]->name, env,
			     strlen(battery_devices[i]->name))) {
			bp->dev = battery_devices[i];
			goto match;
		}
	}

	printf("No batt device found\n");
	return -ENODEV;

match:
	{
		char *s;
		char * endp;

		if (!(s = strchr(env, ':')))
			return -EINVAL;

		s++;
		bp->charging_pin = simple_strtol(s, &endp, 0);
		if (*endp == '\0')
			return -EINVAL;

		s = endp + 1;
		bp->charging_active = simple_strtol(s, &endp, 0);

		printf("Found batt dev %s: charging_pin %d, charging_active %d\n",
		       bp->dev->name, bp->charging_pin, bp->charging_active);

		if (bp->charging_pin >= 0)
			gpio_direction_input(bp->charging_pin);
	}

	return 0;
}

struct led_param {
	int red;
	int green;
};
static struct led_param led_param = { -1, -1 };

static int parse_battery_led_param(struct led_param *led)
{
	enum {
		idx_red,
		idx_green,
		idx_max
	};

	long ps[idx_max] = {-1, -1};
	char *p, *endp;
	int i = 0, rc = 0;

	p = getenv("wmt.battery.led");
	if (!p) {
		rc = -1;
		goto out;
	}

	while (i < idx_max) {
		ps[i++] = simple_strtoul(p, &endp, 10);

		if (*endp == '\0')
			break;
		p = endp + 1;

		if (*p == '\0')
			break;
	}

out:
	led->red = ps[idx_red];
	led->green = ps[idx_green];
	printf("battery led: %d, %d\n", led->red, led->green);
	return 0;
}

static inline void light_led(int r, int g)
{
	if (led_param.red > 0)
		gpio_direction_output(led_param.red, r);
	if (led_param.green > 0)
		gpio_direction_output(led_param.green, g);
}

void hw_set_large_current(void)
{
	struct charger_param *cp = &charger_param;
	if (cp && cp->chg_dev->set_large_current)
		cp->chg_dev->set_large_current();
	printf("Set large charging current\n");
}

void hw_set_small_current(void)
{
	struct charger_param *cp = &charger_param;
	if (cp && cp->chg_dev->set_small_current)
		cp->chg_dev->set_small_current();
	printf("Set small charging current\n");
}

int hw_is_dc_plugin(void)
{
	return REG8_VAL(PM_CTRL_BASE_ADDR + 0x005d) & 0x01;
}

static int hw_is_bat_charging(void)
{
	struct battery_param *bp = &battery_param;
	if (bp->charging_pin >= 0)
		return gpio_get_value(bp->charging_pin) == bp->charging_active;

	struct charger_param *cp = &charger_param;
	if (cp && cp->chg_dev->check_full)
		return !cp->chg_dev->check_full();
	
	return 0;
}

int wmt_bat_get_current(void)
{
	struct bat_dev *dev = battery_param.dev;

	if (!dev || !dev->get_current)
		return -1;

	return dev->get_current();
}

int wmt_bat_get_voltage(void)
{
	struct bat_dev *dev = battery_param.dev;

	if (!dev || !dev->get_voltage)
		return -1;

	return dev->get_voltage();
}

int wmt_bat_get_capacity(void)
{
	struct bat_dev *dev = battery_param.dev;

	if (!dev || !dev->get_capacity)
		return -1;

	return dev->get_capacity();
}

/* wmt_bat_check_bl - check if battery low
 * return value:
 * 	< 0 - check failed
 * 	= 0 - battery power enough
 * 	> 0 - battery low.
 */
int wmt_bat_is_lowlevel(void)
{
	struct bat_dev *dev = battery_param.dev;

	if (!dev || !dev->check_bl)
		return -1;

	return dev->check_bl();
}

int wmt_bat_is_gauge(void)
{
	struct bat_dev *dev = battery_param.dev;

	if (!dev || !dev->check_bl)
		return -1;

	return dev->is_gauge;
}

/* wmt_bat_is_charging_full - check is charging full
 * return value:
 * 	-1 : discharging
 * 	 0 : charging.
 * 	 1 : full.
 */
int wmt_bat_is_charging_full(void)
{
	if (!hw_is_dc_plugin()) {
		light_led(0, 0);
		return -1;
	}

	if (hw_is_bat_charging()) {
		light_led(1, 0);
		return 0;
	} else {
		light_led(0, 1);
		return 1;
	}
}

int wmt_bat_init(void)
{
	struct bat_dev *dev;
	struct charger_param *cp = &charger_param;
	struct battery_param *bp = &battery_param;
	int rc = -1;
	int i;

	if (!cp->chg_dev) {
		for (i = 0; i < ARRAY_SIZE(charger_devices); i++) {
			if (charger_devices[i]->parse_param)
				rc &= charger_devices[i]->parse_param();
		}
		if (rc != 0)
			return -ENODEV;
	}

	if (cp && cp->chg_dev->init)
		cp->chg_dev->init();

	rc = parse_battery_param(bp);
	if (rc)
		return rc;

	rc = parse_battery_led_param(&led_param);

	dev = bp->dev;

	if (!dev || !dev->init)
		return -ENODEV;

	return dev->init();
}

int charger_param_init(void)
{
	int rc = -1;
	int i;
	struct charger_param *cp = &charger_param;

	if (cp->chg_dev)
		return 0;

	for (i = 0; i < ARRAY_SIZE(charger_devices); i++) {
		if (charger_devices[i]->parse_param)
			rc &= charger_devices[i]->parse_param();
	}
	return rc;
}

int wmt_charger_cable_type(void)
{
	return charger_param.id;
}

static int do_batt( cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	int rc;
	static int inited = 0;

	if (inited == 0) {
		if (wmt_bat_init()) {
			printf("wmt bat init failed\n");
			return -1;
		}
		++inited;
	}

	printf("Capacity: %d\n", wmt_bat_get_capacity());
	printf("current: %d mV\n", wmt_bat_get_voltage());
	printf("current: %d mA\n", wmt_bat_get_current());

	printf("Battery State: ");
	rc = wmt_bat_is_charging_full();
	switch (rc) {
	case -1:
		printf("Discharging");
		break;
	case 0:
		printf("Charing");
		break;
	case 1:
		printf("Full");
		break;
	}
	printf("\n");

	return 0;
}

U_BOOT_CMD(
	batt,	4,	0,	do_batt,	\
	"batt: display battery capacity\n",	\
	"" \
);

#define PMHC_HIBERNATE 0x205

void do_wmt_poweroff(void)
{
	/*
	 * Set scratchpad to zero, just in case it is used as a restart
	 * address by the bootloader. Since PB_RESUME button has been
	 * set to be one of the wakeup sources, clean the resume address
	 * will cause zacboot to issue a SW_RESET, for design a behavior
	 * to let PB_RESUME button be a power on button.
	 *
	 * Also force to disable watchdog timer, if it has been enabled.
	 */
	HSP0_VAL = 0;
	OSTW_VAL &= ~OSTW_WE;

	/*
	 * Well, I cannot power-off myself,
	 * so try to enter power-off suspend mode.
	 */
	PMHC_VAL = PMHC_HIBERNATE;

	/* Force ARM to idle mode*/
	asm volatile("ldr r0, =0xD813004C\n\t"
			  "adr r1, .Cpu1_wfi\n\t"
			  "str r1, [r0]\n\t"
			  "sev\n\t"
			  "ldr r0, =0xD8018008  @SCU\n\t"
			  "ldr r1, =0x0303\n\t"
			  "str  r1, [r0]\n\t"
			  "ldr  r0, =0xD8130012\n\t"
			  "ldr  r1, =0x205\n\t"
			  "strh r1, [r0]\n\t"
			  "wfi\n\t"
			  ".Cpu1_wfi:\n\t"
			  "wfi\n\t"
			  "b .Cpu1_wfi\n\t"
			  );
}

U_BOOT_CMD(
	poweroff, 1,	0,	do_wmt_poweroff,
	"poweroff - wmt device power off. \n",
	"- wmt device power off.\n"
);

