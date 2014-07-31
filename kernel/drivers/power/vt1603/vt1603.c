/*
 * vt1603.c - WonderMedia VT1603 Adc Battery Driver.
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

#include <linux/init.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/power_supply.h>
#include <linux/platform_device.h>

#include <linux/power/wmt_charger_common.h>
#include "vt1603.h"
#include "batt_leds.h"

enum {
	COMPENSATION_VOLUME = 0,
	COMPENSATION_BRIGHTNESS,
	COMPENSATION_WIFI,
	COMPENSATION_VIDEO,
	COMPENSATION_USB,
	COMPENSATION_HDMI,
	COMPENSATION_COUNT
};

static const char *compensation_strings[] = {
	"volume",
	"brightness",
	"wifi",
	"video",
	"usb",
	"hdmi"
};

struct vt1603_device_info {
	struct vt1603		*vt1603;
	struct device 		*dev;
	struct power_supply	ps_bat;
	struct mutex		mutex;
	int compensation[COMPENSATION_COUNT];

	int capacity;
	int sleeping;
	int debug;
};

static struct vt1603_device_info *vt1603_dev_info = NULL;

static int vt1603_set_reg8(struct vt1603 *vt1603, u8 reg, u8 val)
{
	int ret = vt1603->reg_write(vt1603, reg, val);
	if (ret)
		pr_err("vt1603 battery write error, errno%d\n", ret);
	return ret;
}

static u8 vt1603_get_reg8(struct vt1603 *vt1603, u8 reg)
{
	u8 val = 0;
	int ret = 0;

	ret = vt1603->reg_read(vt1603, reg, &val);
	if (ret < 0){
		pr_err("vt1603 battery read error, errno%d\n", ret);
		return 0;
	}

	return val;
}

static int vt1603_read8(struct vt1603 *vt1603, u8 reg, u8 *data)
{
	int ret = vt1603->reg_read(vt1603, reg, data);
	if (ret)
		pr_err("vt1603 battery read error, errno%d\n", ret);
	return ret;
}

static void vt1603_setbits(struct vt1603 *vt1603, u8 reg, u8 mask)
{
	u8 tmp = vt1603_get_reg8(vt1603, reg) | mask;
	vt1603_set_reg8(vt1603, reg, tmp);
}

static void vt1603_clrbits(struct vt1603 *vt1603, u8 reg, u8 mask)
{
	u8 tmp = vt1603_get_reg8(vt1603, reg) & (~mask);
	vt1603_set_reg8(vt1603, reg, tmp);
}

#define ADC_DATA(low, high)	((((high) & 0x0F) << 8) + (low))

static int vt1603_get_bat_data(struct vt1603 *vt1603,int *data)
{
	int ret = 0;
	u8 data_l, data_h;

	ret |= vt1603_read8(vt1603, VT1603_DATL_REG, &data_l);
	ret |= vt1603_read8(vt1603, VT1603_DATH_REG, &data_h);

	*data = ADC_DATA(data_l, data_h);
	return ret;
}

static void vt1603_switch_to_bat_mode(struct vt1603 *vt1603)
{
	vt1603_set_reg8(vt1603, VT1603_CR_REG, 0x00);
	vt1603_set_reg8(vt1603, 0xc6, 0x00);
	vt1603_set_reg8(vt1603, VT1603_AMCR_REG, BIT0);
}

static inline void vt1603_bat_pen_manual(struct vt1603 *vt1603)
{
	vt1603_setbits(vt1603, VT1603_INTCR_REG, BIT7);
}

static void vt1603_bat_power_up(struct vt1603 *vt1603)
{
	if (vt1603_get_reg8(vt1603, VT1603_PWC_REG) != 0x08)
		vt1603_set_reg8(vt1603, VT1603_PWC_REG, 0x08);
}

static int vt1603_read_volt(struct vt1603 *vt1603)
{
	int timeout = 2000;
	int ret = 0;
	int value;

	// wait for interrupt that adc converted completed.
	do {
		if (vt1603_get_reg8(vt1603, VT1603_INTS_REG) & BIT0)
			break;
	} while (timeout--);

	if (!timeout) {
		pr_err("wait adc end timeout ?!\n");
		return -ETIMEDOUT;
	}

	ret = vt1603_get_bat_data(vt1603, &value);
	if (ret < 0) {
		pr_err("vt1603 get bat adc data Failed!\n");
		return ret;
	}

	return value;
}

static int vt1603_manual_read_adc(struct vt1603 *vt1603)
{
	int i;
	int ret = 0;
	uint32_t sum = 0;

	/* enable sar-adc power and clock        */
	vt1603_bat_power_up(vt1603);
	/* enable pen down/up to avoid miss irq  */
	vt1603_bat_pen_manual(vt1603);
	/* switch vt1603 to battery detect mode  */
	vt1603_switch_to_bat_mode(vt1603);
	/* do conversion use battery manual mode */
	vt1603_setbits(vt1603, VT1603_INTS_REG, BIT0);
	vt1603_set_reg8(vt1603, VT1603_CR_REG, BIT4);

	for (i = 0; i < 4; i++) {
		ret = vt1603_read_volt(vt1603);
		if (ret < 0)
			break;
		sum += ret;

		vt1603_setbits(vt1603, VT1603_INTS_REG, BIT0);
		vt1603_set_reg8(vt1603, VT1603_CR_REG, BIT4);	//start manual ADC mode
	}
	vt1603_clrbits(vt1603, VT1603_INTCR_REG, BIT7);
	vt1603_setbits(vt1603, VT1603_INTS_REG, BIT0 | BIT3);
	vt1603_set_reg8(vt1603, VT1603_CR_REG, BIT1);

	return (ret < 0) ? ret : (sum >> 2);
}

static inline int volt_reg_to_mV(int value)
{
	return ((value * 1047) / 1000);
}

static int vt1603_bat_read_voltage(struct vt1603_device_info *di, int *intval)
{
	int ret = vt1603_manual_read_adc(di->vt1603);
	if (ret < 0)
		return ret;

	*intval = volt_reg_to_mV(ret);
	return 0;
}

static bool g2214_is_charging_full(void)
{
	struct power_supply *status;
	union power_supply_propval val_charger;
	
	status = power_supply_get_by_name("charger");
	if (!status)
		return false;

	status->get_property(status, POWER_SUPPLY_PROP_STATUS, &val_charger);
	return val_charger.intval == POWER_SUPPLY_STATUS_FULL;
}

static int vt1603_bat_read_status(struct vt1603_device_info *di, int *intval)
{
	if (power_supply_am_i_supplied(&di->ps_bat)) {
		if (di->capacity == 100 || wmt_charger_is_charging_full() || 
				g2214_is_charging_full())
			*intval = POWER_SUPPLY_STATUS_FULL;
		else
			*intval = POWER_SUPPLY_STATUS_CHARGING;
	} else {
		*intval = POWER_SUPPLY_STATUS_DISCHARGING;
	}
	return batt_leds_update(*intval);
}

static int vt1603_proc_read(char *buf, char **start, off_t offset, int len,
			    int *eof, void *data)
{
	int l = 0, i;
	int ret, status, dcin, voltage, full;
	struct vt1603_device_info *di = vt1603_dev_info;

	mutex_lock(&di->mutex);

	ret = vt1603_bat_read_status(di, &status);
	if (ret) {
		pr_err("vt1603_bat_read_status failed\n");
		goto out;
	}
	voltage = vt1603_manual_read_adc(di->vt1603);
	if (voltage < 0) {
		pr_err("vt1603_manual_read_adc failed\n");
		goto out;
	}

	dcin = wmt_charger_is_dc_plugin();
	full = wmt_charger_is_charging_full() || g2214_is_charging_full();

	l += sprintf(buf + l, "status   : %d\n", status);
	l += sprintf(buf + l, "dcin     : %d\n", dcin);
	l += sprintf(buf + l, "voltage  : %d\n", voltage);
	l += sprintf(buf + l, "full     : %d\n", full);
	l += sprintf(buf + l, "sleeping : %d\n", di->sleeping);
	l += sprintf(buf + l, "debug    : %d\n", di->debug);

	for (i = 0; i < COMPENSATION_COUNT; i++) {
		l += sprintf(buf +l, "compensation %10s : %d\n",
			     compensation_strings[i], di->compensation[i]);
	}

	/* clear after read */
	di->sleeping = 0;
out:
	mutex_unlock(&di->mutex);
	return l;
}

static int vt1603_proc_write(struct file *file, const char *buffer,
			unsigned long count, void *data)
{
	int bm, usage;
	struct vt1603_device_info *di = vt1603_dev_info;
	int capacity;

	if (sscanf(buffer, "capacity=%d", &capacity)) {
		if (di->capacity != capacity) {
			di->capacity = capacity;
			power_supply_changed(&di->ps_bat);
		}
		return count;
	}

	if (sscanf(buffer, "debug=%d", &di->debug))
		return count;

	if (sscanf(buffer, "MODULE_CHANGE:%d-%d", &bm, &usage) < 2)
		return 0;

	if (bm < 0 || bm >= COMPENSATION_COUNT) {
		pr_err("bm %d error, [0, %d)\n", bm, COMPENSATION_COUNT);
		return 0;
	}

	if (usage > 100 || usage < 0) {
		pr_err("usage %d error\n", usage);
		return 0;
	}

	mutex_lock(&di->mutex);
	di->compensation[bm] = usage;
	mutex_unlock(&di->mutex);
	return count;
}

#define VT1603_PROC_NAME	"battery_calibration"

static void vt1603_proc_init(void)
{
	struct proc_dir_entry *entry;

	entry = create_proc_entry(VT1603_PROC_NAME, 0666, NULL);
	if (entry) {
		entry->read_proc = vt1603_proc_read;
		entry->write_proc = vt1603_proc_write;
	}
}

static void vt1603_proc_cleanup(void)
{
	remove_proc_entry(VT1603_PROC_NAME, NULL);
}

#define to_vt1603_device_info(x) container_of((x), \
				struct vt1603_device_info, ps_bat);

static int vt1603_battery_get_property(struct power_supply *psy,
				       enum power_supply_property psp,
				       union power_supply_propval *val)
{
	int ret = 0;
	struct vt1603_device_info *di = to_vt1603_device_info(psy);

	mutex_lock(&di->mutex);
	switch (psp) {
	case POWER_SUPPLY_PROP_STATUS:
		ret = vt1603_bat_read_status(di, &val->intval);
		break;
	case POWER_SUPPLY_PROP_CAPACITY:
		val->intval = di->capacity;
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_NOW:
		ret = vt1603_bat_read_voltage(di, &val->intval);
		break;
	case POWER_SUPPLY_PROP_TECHNOLOGY:
		val->intval = POWER_SUPPLY_TECHNOLOGY_LION;
		break;
	case POWER_SUPPLY_PROP_PRESENT:
		val->intval = 1;
		break;
	default:
		ret = -EINVAL;
		break;
	}
	mutex_unlock(&di->mutex);
	return ret;
}

static void vt1603_external_power_changed(struct power_supply *psy)
{
	power_supply_changed(psy);
}

static enum power_supply_property vt1603_battery_props[] = {
	POWER_SUPPLY_PROP_STATUS,
	POWER_SUPPLY_PROP_CAPACITY,
	POWER_SUPPLY_PROP_PRESENT,
	POWER_SUPPLY_PROP_VOLTAGE_NOW,
	POWER_SUPPLY_PROP_TECHNOLOGY,
};

static int __devinit vt1603_batt_probe(struct platform_device *pdev)
{
	struct vt1603_device_info *di;
	int ret;

	di = kzalloc(sizeof(*di), GFP_KERNEL);
	if (!di) {
		dev_err(&pdev->dev, "no memery\n");
		return -ENOMEM;
	}

	di->dev	= &pdev->dev;
	di->vt1603 = dev_get_platdata(&pdev->dev);
	di->capacity = 50;

	mutex_init(&di->mutex);

	di->ps_bat.name = "battery";
	di->ps_bat.type = POWER_SUPPLY_TYPE_BATTERY;
	di->ps_bat.properties = vt1603_battery_props;
	di->ps_bat.num_properties = ARRAY_SIZE(vt1603_battery_props);
	di->ps_bat.get_property = vt1603_battery_get_property;
	di->ps_bat.external_power_changed = vt1603_external_power_changed;

	ret = power_supply_register(di->dev, &di->ps_bat);
	if (ret) {
		dev_err(di->dev, "failed to register battery: %d\n", ret);
		kfree(di);
		return ret;
	}

	platform_set_drvdata(pdev, di);
	vt1603_dev_info = di;

	vt1603_proc_init();

	pr_info("VT1603 Battery Driver Installed!\n");
	return 0;
}

static int __devexit vt1603_batt_remove(struct platform_device *pdev)
{
	struct vt1603_device_info *di = platform_get_drvdata(pdev);
	vt1603_proc_cleanup();
	power_supply_unregister(&di->ps_bat);
	kfree(di);
	vt1603_dev_info = NULL;
	pr_info("VT1603 Battery Driver Removed!\n");
	return 0;
}

static int vt1603_batt_suspend_prepare(struct device *dev)
{
	struct vt1603_device_info *di = dev_get_drvdata(dev);
	int status, ret;

	ret = vt1603_bat_read_status(di, &status);
	if (ret) {
		pr_err("vt1603_bat_read_status failed\n");
		return 0;
	}

	if (status == POWER_SUPPLY_STATUS_CHARGING)
		batt_leds_suspend_prepare();

	return 0;
}

static int vt1603_batt_suspend(struct device *dev)
{
	return 0;
}

static int vt1603_batt_resume(struct device *dev)
{
	struct vt1603_device_info *di = dev_get_drvdata(dev);
	di->sleeping = 1;
	return 0;
}

static void vt1603_batt_resume_complete(struct device *dev)
{
	return batt_leds_resume_complete();
}

static const struct dev_pm_ops vt1603_batt_manager_pm = {
	.prepare	= vt1603_batt_suspend_prepare,
	.suspend	= vt1603_batt_suspend,
	.resume		= vt1603_batt_resume,
	.complete	= vt1603_batt_resume_complete,
};

static struct platform_driver vt1603_batt_driver = {
	.driver	= {
		.name	= "vt1603-batt",
		.owner	= THIS_MODULE,
		.pm	= &vt1603_batt_manager_pm,
	},
	.probe		= vt1603_batt_probe,
	.remove		= __devexit_p(vt1603_batt_remove),
};

static int __init vt1603_batt_init(void)
{
	if (wmt_charger_gpio_probe("vt1603"))
		return -EINVAL;

        batt_leds_setup();
	return platform_driver_register(&vt1603_batt_driver);
}

static void __exit vt1603_batt_exit(void)
{
	platform_driver_unregister(&vt1603_batt_driver);
        batt_leds_cleanup();
	wmt_charger_gpio_release();
}

module_init(vt1603_batt_init);
module_exit(vt1603_batt_exit);

MODULE_AUTHOR("Sam Mei");
MODULE_DESCRIPTION("WonderMedia VT1603 Adc Battery Driver");
MODULE_LICENSE("GPL");

