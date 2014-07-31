/*
 * Copyright (c) 2012, uPI Semiconductor Corp. All Rights Reserved.
 */
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/param.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/power_supply.h>
#include <linux/idr.h>
#include <linux/reboot.h>
#include <linux/notifier.h>
#include <linux/jiffies.h>
#include <linux/err.h>
#include <asm/unaligned.h>
#include <linux/wakelock.h>
#include <linux/version.h>

#include <linux/power/wmt_charger_common.h>
#include <mach/wmt_env.h>

#include "ug31xx_gauge.h"
#include "uG31xx_API.h"

/* Functions Declaration */
static int ug31xx_battery_get_property(struct power_supply *psy,
				       enum power_supply_property psp,
				       union power_supply_propval *val);
static int ug31xx_update_psp(struct power_supply *psy,
			     enum power_supply_property psp,
			     union power_supply_propval *val);
static void ug31xx_external_power_changed(struct power_supply *psy);

#define UG31XX_CHECK_FILE_CNT_THRD      (20)

/* Global Variables */
static struct ug31xx_gauge *ug31 = NULL;
static char		chk_backup_file_cnt = UG31XX_CHECK_FILE_CNT_THRD;
static char		*pGGB = NULL;
static GG_DEVICE_INFO	gauge_dev_info;
static GG_CAPACITY	gauge_dev_capacity;
static struct workqueue_struct *ug31xx_gauge_wq = NULL;
static int charger_dc_in_before_suspend = 0;
static bool ug3105_in_suspend = false;
static bool ug3105_charger_enabled = true;
static unsigned char force_reset = 0;
static unsigned char enable_debug = 0;
static int external_termperature = true;

static __attribute__((unused)) char *chg_status[] = {
	"Unknown",
	"Charging",
	"Discharging",
	"Not charging",
	"Full"
};

static drv_status_t drv_status = DRV_NOT_READY;
static int charge_temperature_range[2] = { -150, 600 };

#include "ggb/ug31xx_ggb_data_wms8309_wm8_20130820_110949.h"
#include "ggb/ug31xx_ggb_data_wms8309_c7_20130725_164935.h"
#include "ggb/ug31xx_ggb_data_wms8309_c7_20130910_130553.h"
#include "ggb/ug31xx_ggb_data_wms7320_20130718_200031.h"
#include "ggb/ug31xx_ggb_data_cw500_20130801_103638.h"
#include "ggb/ug31xx_ggb_data_mp718_20131004_070110.h"
#include "ggb/ug31xx_ggb_data_t73v_20131120_001204.h"

enum {
	UG31XX_ID_3105,
	UG31XX_ID_3102,
};

static int g_ug31xx_id;

struct ggb_info {
	char *name;
	uint8_t *data;
};

static struct ggb_info *ggb;

/* Extern Function */
static struct ggb_info ggb_arrays[] = {
	{
		.name = "wms8309wm8",
		.data = FactoryGGBXFile_wms8309_wm8,
	}, {
		.name = "wms7320",
		.data = FactoryGGBXFile_wms7320,
	}, {
		.name = "wms8309c7_3900mAh",
		.data = FactoryGGBXFile_wms8309_c7_3900mAh,
	}, {
		.name = "wms8309c7_3000mAh",
		.data = FactoryGGBXFile_wms8309_c7_3000mAh,
	}, {
		.name = "cw500",
		.data = FactoryGGBXFile_cw500, 
	}, {
		.name = "mp718",
		.data = FactoryGGBXFile_mp718,
	}, {
		.name = "t73v",
		.data = FactoryGGBXFile_t73v,
	},
};

static bool inline export_external_termperature(void)
{
	return !!external_termperature;
}

static int parse_ug31xx_param(void)
{
	char env[] = "wmt.ug31xx.param";
	char buf[64];
	size_t l = sizeof(buf);
	int nr = 1;
	int i;

	if (wmt_getsyspara(env, buf, &l) == 0) {
		sscanf(buf, "%d:%d:%d:%d", &nr, &external_termperature,
		       &charge_temperature_range[0], &charge_temperature_range[1]);
	}

	for (i = 0; i < ARRAY_SIZE(ggb_arrays); i++) {
		if (strstr(buf, ggb_arrays[i].name)) {
			ggb = &ggb_arrays[i];
			break;
		}
	}

	if (i == ARRAY_SIZE(ggb_arrays))
		ggb = &ggb_arrays[0];

	if (g_ug31xx_id == UG31XX_ID_3102)
		external_termperature = 0;

	pr_info("%s i2c%d, ggb %s, use %s temperature, range [%d, %d]\n",
	       (g_ug31xx_id == UG31XX_ID_3105) ? "ug3105" : "ug3102",
		nr, ggb->name, external_termperature ? "external" : "internal",
		charge_temperature_range[0], charge_temperature_range[1]);
	return nr;
}

/*
 * WMT MCE: Use gpio3 on ug31xx as a switch to control charger.
 */
static void hw_charging_set(bool enable)
{
	if (g_ug31xx_id == UG31XX_ID_3105) {
		API_I2C_SingleWrite(0, 0, 0, 0x16, enable ? 0x2 : 0x0);
	} else if (g_ug31xx_id == UG31XX_ID_3102) {
		u8 data = 0;
		API_I2C_SingleRead(0, 0, 0 ,REG_CTRL1, &data);
		if (enable)
			data |= CTRL1_IO1DATA;
		else
			data &= ~CTRL1_IO1DATA;
		API_I2C_SingleWrite(0, 0, 0, REG_CTRL1, data);
	}
	ug3105_charger_enabled = enable;
}

static void inline hw_charging_disable(void)
{
	hw_charging_set(false);
}

static void inline hw_charging_enable(void)
{
	hw_charging_set(true);
}

static bool is_charging_full(void)
{
	return (ug3105_charger_enabled == true) && wmt_charger_is_charging_full();
}

static enum power_supply_property ug31xx_batt_props[] = {
	POWER_SUPPLY_PROP_STATUS,
	POWER_SUPPLY_PROP_HEALTH,
	POWER_SUPPLY_PROP_PRESENT,
	POWER_SUPPLY_PROP_VOLTAGE_NOW,
	POWER_SUPPLY_PROP_CURRENT_NOW,
	POWER_SUPPLY_PROP_CAPACITY,
	POWER_SUPPLY_PROP_TEMP,
	POWER_SUPPLY_PROP_CHARGE_NOW,
	POWER_SUPPLY_PROP_CHARGE_FULL,
};

static struct power_supply ug31xx_supply[] = {
	{
		.name		= "battery",
		.type		= POWER_SUPPLY_TYPE_BATTERY,
		.properties	= ug31xx_batt_props,
		.num_properties = ARRAY_SIZE(ug31xx_batt_props),
		.get_property	= ug31xx_battery_get_property,
		.external_power_changed = ug31xx_external_power_changed,
	},
};

static int ug31xx_battery_get_property(struct power_supply *psy,
				       enum power_supply_property psp,
				       union power_supply_propval *val)
{
	int ret = 0;

	switch (psp) {
	case POWER_SUPPLY_PROP_HEALTH:
		val->intval = POWER_SUPPLY_HEALTH_GOOD;
		break;
	case POWER_SUPPLY_PROP_PRESENT:
		val->intval = 1;
		break;
	case POWER_SUPPLY_PROP_STATUS:
	case POWER_SUPPLY_PROP_VOLTAGE_NOW:
	case POWER_SUPPLY_PROP_CURRENT_NOW:
	case POWER_SUPPLY_PROP_CAPACITY:
	case POWER_SUPPLY_PROP_TEMP:
	case POWER_SUPPLY_PROP_CHARGE_NOW:
	case POWER_SUPPLY_PROP_CHARGE_FULL:
		if (ug31xx_update_psp(psy, psp, val))
			return -EINVAL;
		break;
	default:
		return -EINVAL;
	}
	return ret;
}

static int ug31xx_update_psp(struct power_supply *psy,
			     enum power_supply_property psp,
			     union power_supply_propval *val)
{
	int rtn;

	if (drv_status != DRV_INIT_OK) {
		GAUGE_err("Gauge driver not init finish\n");
		return -EINVAL;
	}

	if (psp == POWER_SUPPLY_PROP_TEMP) {
		if (export_external_termperature())
			ug31->batt_temp = gauge_dev_info.ET;
		else
			ug31->batt_temp = gauge_dev_info.IT;
		val->intval = ug31->batt_temp;
		GAUGE_notice("I.Temperature=%d, E.T=%d\n", gauge_dev_info.IT, gauge_dev_info.ET);
	}

	if (psp == POWER_SUPPLY_PROP_CAPACITY) {
		if (ug31->batt_status == POWER_SUPPLY_STATUS_FULL)
			ug31->batt_capacity = 100;
		else
			ug31->batt_capacity = gauge_dev_capacity.RSOC;

		val->intval = ug31->batt_capacity;
		GAUGE_notice("Capacity=%d %%\n", val->intval);
	}

	if (psp == POWER_SUPPLY_PROP_VOLTAGE_NOW) {
		val->intval = ug31->batt_volt = gauge_dev_info.voltage_mV;
		GAUGE_notice("Voltage=%d mV\n", val->intval);
	}

	if (psp == POWER_SUPPLY_PROP_CURRENT_NOW) {
		val->intval = ug31->batt_current = gauge_dev_info.AveCurrent_mA;
		GAUGE_notice("Current=%d mA\n", val->intval);
	}

	if (psp == POWER_SUPPLY_PROP_STATUS) {
		if (wmt_charger_is_dc_plugin() && power_supply_am_i_supplied(psy)) {
			if (ug31->batt_capacity == 100)
				ug31->batt_status = POWER_SUPPLY_STATUS_FULL;
			else if (is_charging_full() && ug31->batt_capacity >= 90) {
				ug31->batt_status = POWER_SUPPLY_STATUS_FULL;
			} else
				ug31->batt_status = POWER_SUPPLY_STATUS_CHARGING;
		} else
			ug31->batt_status = POWER_SUPPLY_STATUS_DISCHARGING;

		val->intval = ug31->batt_status;
		GAUGE_notice("Status=%s\n", chg_status[val->intval]);
	}

	if (psp == POWER_SUPPLY_PROP_CHARGE_NOW) {
		val->intval = ug31->batt_charge_now = gauge_dev_capacity.NAC;
		GAUGE_notice("Charge_Now=%d mAh\n", val->intval);
	}

	if (psp == POWER_SUPPLY_PROP_CHARGE_FULL) {
		val->intval =ug31->batt_charge_full = gauge_dev_capacity.LMD;
		GAUGE_notice("Charge_Full=%d mAh\n", val->intval);
	}

	mutex_lock(&ug31->info_update_lock);
	chk_backup_file_cnt = chk_backup_file_cnt + 1;
	UG31_LOGI("chk_backup_file_cnt %d\n", chk_backup_file_cnt);
	if (chk_backup_file_cnt > UG31XX_CHECK_FILE_CNT_THRD) {
		rtn = upiGG_CheckBackupFile(pGGB);
		if(rtn == UPI_CHECK_BACKUP_FILE_MISMATCH)
		{
			force_reset = 1;
			UG31_LOGI("[%s]: Force reset due to version dismatched.\n", __func__);
		}
		else if(rtn == UPI_CHECK_BACKUP_FILE_EXIST)
		{
			chk_backup_file_cnt = 0;
			UG31_LOGI("[%s]: Backup file existed -> no need to check frequently.\n", __func__)
		}
		else
		{
			chk_backup_file_cnt = UG31XX_CHECK_FILE_CNT_THRD;
			UG31_LOGI("[%s]: Backup file not existed -> need to check frequently.\n", __func__)
		}
		UG31_LOGI("[%s]: Check backup file status = %d\n", __func__, rtn);
	}
	mutex_unlock(&ug31->info_update_lock);
	return 0;
}

static int ug31xx_powersupply_init(struct i2c_client *client)
{
	int i, ret;
	for (i = 0; i < ARRAY_SIZE(ug31xx_supply); i++) {
		ret = power_supply_register(&client->dev, &ug31xx_supply[i]);
		if (ret) {
			GAUGE_err("Failed to register power supply\n");
			while (i--)
				power_supply_unregister(&ug31xx_supply[i]);
			return ret;
		}
	}
	return 0;
}

static void ug31xx_external_power_changed(struct power_supply *psy)
{
	if (ug3105_in_suspend == false)
		queue_delayed_work(ug31xx_gauge_wq, &ug31->ug31_gauge_info_work, 0*HZ);
}

#define	UG31XX_INITIAL_RETRY_CNT	(3)
#define	UG31XX_INITIAL_LOCK_ENABLE	(1)
#define	UG31XX_INITIAL_LOCK_DISABLE	(0)
#define	UG31XX_INITIAL_FIX_T_COUNT	(30)

static int ug31_gauge_info_reset(int enable_lock)
{
	int retry;
	int gg_status;

	force_reset = 0;

	if(enable_lock == UG31XX_INITIAL_LOCK_ENABLE)
	{
		mutex_lock(&ug31->info_update_lock);
	}

	retry = 0;
	while(retry < UG31XX_INITIAL_RETRY_CNT)
	{
		gg_status = UG_SUCCESS;
		if(pGGB != NULL)
		{
			gg_status = upiGG_UnInitial(&pGGB);
			pGGB = NULL;
			GAUGE_notice("Driver remove. gg_status=0x%02x\n", gg_status);
		}

		if(gg_status == UG_SUCCESS)
		{
			gg_status = upiGG_Initial(&pGGB, (GGBX_FILE_HEADER *)ggb->data, force_reset, UG31XX_INITIAL_FIX_T_COUNT);
			if(gg_status == UG_INIT_SUCCESS)
			{
				chk_backup_file_cnt = UG31XX_CHECK_FILE_CNT_THRD;
				if(enable_lock == UG31XX_INITIAL_LOCK_ENABLE)
				{
					mutex_unlock(&ug31->info_update_lock);
				}
				return (0);
			}
		}

		retry = retry + 1;
		GAUGE_err("GGB file read and init fail count = %d (%d)\n", retry, gg_status);
	}

	GAUGE_err("Reset uG31xx fail.\n");
	if(pGGB != NULL)
	{
		gg_status = upiGG_UnInitial(&pGGB);
		pGGB = NULL;
		GAUGE_notice("Driver remove. gg_status=0x%02x\n", gg_status);
	}

	if(enable_lock == UG31XX_INITIAL_LOCK_ENABLE)
	{
		mutex_unlock(&ug31->info_update_lock);
	}
	return (-1);
}

static void ug31_gauge_info_work_func(struct work_struct *work)
{
	int gg_status = 0, retry = 3;
	struct power_supply *psy = &ug31xx_supply[PWR_SUPPLY_BATTERY];
	int batt_status = ug31->batt_status;
	struct ug31xx_gauge *ug31_dev;

	ug31_dev = container_of(work, struct ug31xx_gauge, ug31_gauge_info_work.work);

	if(enable_debug == 0)
	{
		upiGG_DebugSwitch(_UPI_FALSE_);
		UG31_LOGI("Set upiGG_DebugSwitch to FALSE\n");
	}
	else
	{
		upiGG_DebugSwitch(_UPI_TRUE_);
		UG31_LOGI("Set upiGG_DebugSwitch to TRUE\n");
	}

	UG31_LOGI("Update gauge info!!\n");

	mutex_lock(&ug31->info_update_lock);
	while (retry-- > 0) {
		gg_status = upiGG_ReadDeviceInfo(pGGB,&gauge_dev_info);
		if (gg_status == UG_READ_DEVICE_INFO_SUCCESS)
			goto read_dev_info_ok;
	}
	GAUGE_err("Read device info fail. gg_status=%d\n", gg_status);
	if(gg_status == UG_MEAS_FAIL_BATTERY_REMOVED)
	{
		gauge_dev_capacity.NAC = 0;
		gauge_dev_capacity.LMD = 0;
		gauge_dev_capacity.RSOC = 0;
	}
	goto read_dev_info_fail;

read_dev_info_ok:
	if(gauge_dev_capacity.Ready != UG_CAP_DATA_READY)
	{
		if(gauge_dev_info.ET > UG31XX_MAX_TEMPERATURE_BEFORE_READY)
		{
			gauge_dev_info.ET = UG31XX_MAX_TEMPERATURE_BEFORE_READY;
		}
		if(gauge_dev_info.ET < UG31XX_MIN_TEMPERATURE_BEFORE_READY)
		{
			gauge_dev_info.ET = UG31XX_MIN_TEMPERATURE_BEFORE_READY;
		} 
	}

	upiGG_ReadCapacity(pGGB,&gauge_dev_capacity);
	UG31_LOGI("Gauge info updated !!\n");

read_dev_info_fail:
	mutex_unlock(&ug31->info_update_lock);

	if(force_reset == 1)
	{
		UG31_LOGI("Reset whole uG31xx driver.\n");
		gg_status = ug31_gauge_info_reset(UG31XX_INITIAL_LOCK_ENABLE);
		if(gg_status != 0)
		{
			goto set_polling_time;
		}
		gauge_dev_capacity.Ready = UG_CAP_DATA_READY;
	}

	/* Disable charging by temperture */

	if (g_ug31xx_id == UG31XX_ID_3105) {
		if (export_external_termperature()) {
			if (gauge_dev_info.ET < charge_temperature_range[0] ||
			    gauge_dev_info.ET > charge_temperature_range[1])
				hw_charging_disable();
			else
				hw_charging_enable();
		}
	} else if (g_ug31xx_id == UG31XX_ID_3102) {
		hw_charging_enable();
	}

	if (wmt_charger_is_dc_plugin() && power_supply_am_i_supplied(psy)) {
		if (gauge_dev_capacity.RSOC == 100)
			batt_status = POWER_SUPPLY_STATUS_FULL;
		else if (is_charging_full() && ug31->batt_capacity >= 90) {
			batt_status = POWER_SUPPLY_STATUS_FULL;
			gauge_dev_capacity.RSOC = 100;
			mutex_lock(&ug31->info_update_lock);
			upiGG_ForceTaper(pGGB, 1, 1, 1);
			mutex_unlock(&ug31->info_update_lock);
		} else
			batt_status = POWER_SUPPLY_STATUS_CHARGING;
	} else
		batt_status = POWER_SUPPLY_STATUS_DISCHARGING;

	/* Report uevent while capacity changed */
	if (ug31->batt_capacity != gauge_dev_capacity.RSOC ||
	    ug31->batt_status != batt_status) {
		GAUGE_notice("Capacity changed: %d -> %d\n", ug31->batt_capacity, gauge_dev_capacity.RSOC);
		GAUGE_notice("Status changed: %d -> %d\n", ug31->batt_status, batt_status);
		power_supply_changed(psy);
	}

set_polling_time:
	if (gauge_dev_capacity.Ready == UG_CAP_DATA_READY) {
		queue_delayed_work(ug31xx_gauge_wq, &ug31_dev->ug31_gauge_info_work, 5*HZ);
	} else if(force_reset == 1) {
		queue_delayed_work(ug31xx_gauge_wq, &ug31_dev->ug31_gauge_info_work, 0*HZ);
	} else {
		queue_delayed_work(ug31xx_gauge_wq, &ug31_dev->ug31_gauge_info_work, 1*HZ);
	}
}

static int __devinit ug31xx_i2c_probe(struct i2c_client *client,
				      const struct i2c_device_id *id)
{
	struct i2c_adapter *adapter = to_i2c_adapter(client->dev.parent);
	int gg_status;

	if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE))
		return -EIO;

	ug31 = kzalloc(sizeof(*ug31), GFP_KERNEL);
	if (!ug31)
		return -ENOMEM;

	ug31->client = client;
	ug31->dev = &client->dev;

	i2c_set_clientdata(client, ug31);
	ug31xx_i2c_client_set(ug31->client);

	gauge_dev_capacity.RSOC = 50;

	/* get GGB file */
	GAUGE_notice("[UPI]: Force to reset uG3105 driver (%d)\n", force_reset);
	gg_status = ug31_gauge_info_reset(UG31XX_INITIAL_LOCK_DISABLE);
	if(gg_status != 0)
	{
		goto ggb_init_fail;
	}

	mutex_init(&ug31->info_update_lock);
	wake_lock_init(&ug31->cable_wake_lock, WAKE_LOCK_SUSPEND, "cable_state_changed");

	ug31xx_gauge_wq = create_singlethread_workqueue("ug31xx_gauge_work_queue");
	INIT_DELAYED_WORK(&ug31->ug31_gauge_info_work, ug31_gauge_info_work_func);
	queue_delayed_work(ug31xx_gauge_wq, &ug31->ug31_gauge_info_work, 0*HZ);

	/* power supply registration */
	if (ug31xx_powersupply_init(client))
		goto pwr_supply_fail;

	force_reset = 0;
	drv_status = DRV_INIT_OK;

	GAUGE_notice(" Driver %s registered done\n", client->name);

	return 0;

pwr_supply_fail:
	kfree(ug31);
ggb_init_fail:
	if(!pGGB) {
		upiGG_UnInitial(&pGGB);
	}
	force_reset = 0;
	return gg_status;
}

static int __devexit ug31xx_i2c_remove(struct i2c_client *client)
{
	struct ug31xx_gauge *ug31_dev;
	int i = 0, gg_status;

	for (i = 0; i < ARRAY_SIZE(ug31xx_supply); i++) {
		power_supply_unregister(&ug31xx_supply[i]);
	}

	cancel_delayed_work_sync(&ug31->ug31_gauge_info_work);
	destroy_workqueue(ug31xx_gauge_wq);
	wake_lock_destroy(&ug31->cable_wake_lock);

	gg_status = upiGG_UnInitial(&pGGB);
	GAUGE_notice("Driver remove. gg_status=0x%02x\n", gg_status);

	ug31_dev = i2c_get_clientdata(client);
	if (ug31_dev) {
		kfree(ug31_dev);
	}
	return 0;
}

static int ug31xx_i2c_suspend(struct i2c_client *client, pm_message_t mesg)
{
	int gg_status;

	GAUGE_notice("ug31xx_i2c_suspend() start\n");
	ug3105_in_suspend = true;

	cancel_delayed_work_sync(&ug31->ug31_gauge_info_work);
	flush_workqueue(ug31xx_gauge_wq);

	mutex_lock(&ug31->info_update_lock);
	gg_status = upiGG_PreSuspend(pGGB);
	if (gg_status != UG_READ_DEVICE_INFO_SUCCESS) {
		GAUGE_err("Fail in suspend. gg_status=0x%02x\n", gg_status);
	} else
		GAUGE_notice("Driver suspend. gg_status=0x%02x\n", gg_status);
	mutex_unlock(&ug31->info_update_lock);

	charger_dc_in_before_suspend = wmt_charger_is_dc_plugin();

	if (g_ug31xx_id == UG31XX_ID_3102) {
		hw_charging_enable();
	}

	GAUGE_notice("ug31xx_i2c_suspend() end\n");
	return 0;
}

static int ug31xx_i2c_resume(struct i2c_client *client)
{
	int gg_status;

	GAUGE_notice("ug31xx_i2c_resume() start\n");

	mutex_lock(&ug31->info_update_lock);
	gg_status = upiGG_Wakeup(pGGB, charger_dc_in_before_suspend);
	if (gg_status != UG_READ_DEVICE_INFO_SUCCESS) {
		GAUGE_err("Fail in resume. gg_status=0x%02x\n", gg_status);
		if(gg_status == UG_MEAS_FAIL_BATTERY_REMOVED)
		{
			gauge_dev_capacity.NAC = 0;
			gauge_dev_capacity.LMD = 0;
			gauge_dev_capacity.RSOC = 0;
		}
	} else {
		GAUGE_notice("Driver resume. gg_status=0x%02x\n", gg_status);
	}
	mutex_unlock(&ug31->info_update_lock);

	/// [AT-PM] : Check charger status ; 08/14/2013
	mutex_lock(&ug31->info_update_lock);
	upiGG_ForceTaper(pGGB, is_charging_full(), charger_dc_in_before_suspend, wmt_charger_is_dc_plugin());
	mutex_unlock(&ug31->info_update_lock);

	ug3105_in_suspend = false;

	cancel_delayed_work(&ug31->ug31_gauge_info_work);
	queue_delayed_work(ug31xx_gauge_wq, &ug31->ug31_gauge_info_work, 0*HZ);

	GAUGE_notice("ug31xx_i2c_resume() end\n");
	return 0;
}

void ug31xx_i2c_shutdown(struct i2c_client *client)
{
	int gg_status;

	cancel_delayed_work(&ug31->ug31_gauge_info_work);
	mutex_lock(&ug31->info_update_lock);
	gg_status = upiGG_PrePowerOff(pGGB);
	mutex_unlock(&ug31->info_update_lock);
	GAUGE_notice("Driver shutdown. gg_status=0x%02x\n", gg_status);
}

static const struct i2c_device_id ug31xx_i2c_id[] = {
	{ UG31XX_DEV_NAME, 0 },
	{ },
};

MODULE_DEVICE_TABLE(i2c, ug31xx_i2c_id);

static struct i2c_driver ug31xx_i2c_driver = {
	.driver    = {
		.name  = UG31XX_DEV_NAME,
		.owner = THIS_MODULE,
	},
	.probe     = ug31xx_i2c_probe,
	.remove    = __devexit_p(ug31xx_i2c_remove),
	.suspend   = ug31xx_i2c_suspend,
	.resume    = ug31xx_i2c_resume,
	.shutdown  = ug31xx_i2c_shutdown,
	.id_table  = ug31xx_i2c_id,
};

static struct i2c_board_info ug31xx_i2c_board_info = {
	.type          = "ug31xx-gauge",
	.flags         = 0x00,
	.addr          = 0x70,
	.platform_data = NULL,
	.archdata      = NULL,
	.irq           = -1,
};

static struct i2c_client *i2c_client;
static struct i2c_adapter *i2c_adap;

static int __init ug31xx_i2c_init(void)
{
	int nr;
	int ret = 0;

	/* WMT GPIO initial */
	if (!wmt_charger_gpio_probe("ug31xx"))
		g_ug31xx_id = UG31XX_ID_3105;
	else if (!wmt_charger_gpio_probe("ug3102"))
		g_ug31xx_id = UG31XX_ID_3102;
	else
		return -EINVAL;

	nr = parse_ug31xx_param();

	i2c_adap = i2c_get_adapter(nr);
	if (!i2c_adap) {
		pr_err("Cannot get i2c adapter %d\n", nr);
		ret = -ENODEV;
		goto err1;
	}

	i2c_client = i2c_new_device(i2c_adap, &ug31xx_i2c_board_info);
	if (!i2c_client) {
		pr_err("Unable to add I2C device for 0x%x\n",
			 ug31xx_i2c_board_info.addr);
		ret = -ENODEV;
		goto err2;
	}

	ret =  i2c_add_driver(&ug31xx_i2c_driver);
	if (ret)
		goto err3;

	return 0;

err3:	i2c_unregister_device(i2c_client);
err2:	i2c_put_adapter(i2c_adap);
err1:	wmt_charger_gpio_release();
	return ret;
}
module_init(ug31xx_i2c_init);

static void __exit ug31xx_i2c_exit(void)
{
	wmt_charger_gpio_release();
	i2c_put_adapter(i2c_adap);
	i2c_del_driver(&ug31xx_i2c_driver);
	i2c_unregister_device(i2c_client);
}
module_exit(ug31xx_i2c_exit);

MODULE_DESCRIPTION("ug31xx gauge driver");
MODULE_LICENSE("GPL");

#if	LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,12)
MODULE_PARM (force_reset, "b");
MODULE_PARM (enable_debug, "b");
#else
module_param (force_reset, byte, 0);
module_param (enable_debug, byte, 0644);
#endif
MODULE_PARM_DESC(force_reset, "Set 1 to force reset driver as first power on.");
MODULE_PARM_DESC(enable_debug, "Set 1 to enable dumping debug message.");
