/*++
	linux/drivers/power/vt1609/wmt_battery.c

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
#include <linux/proc_fs.h>
#include <linux/leds.h>
#include <linux/rtc.h>
#include <linux/platform_device.h>
#include <linux/wmt_battery.h>
#include <linux/power_supply.h>
#include <linux/power/wmt_charger_common.h>
#include <linux/gpio.h>
#include <mach/wmt_iomux.h>
#include <mach/wmt_env.h>

#define DRVNAME	"wmt-battery"

#define BATTERY_PROC_NAME		"battery_config"
#define BATTERY_CALIBRATION_PRO_NAME	"battery_calibration"

#define ADAPTER_PLUG_OUT   		0
#define ADAPTER_PLUG_IN    		1

#define JUDGE_HOP_VOLTAGE_TIMES		4
#define ADAPTER_STATE_CHANGE_T_COUNT	4
#define SUSPEND_RESUME_T_COUNT		8
#define CHARGING_FULL_T_COUNT		115 //Count time that Battery voltage don't increase any more

#define batdbg(format, arg...)				\
do {							\
	if (bat_print)					\
		printk(KERN_ALERT "[%s, %d]: " format,	\
		       __func__, __LINE__, ## arg);	\
} while (0)

typedef enum {
	BM_VOLUME        = 0,
	BM_BRIGHTNESS    = 1,
	BM_WIFI          = 2,
	BM_VIDEO         = 3,
	BM_USBDEVICE     = 4,
	BM_HDMI          = 5,
	BM_COUNT
} BatteryModule;

static int module_usage[BM_COUNT]={0, 100, 0, 0, 0, 0};

typedef struct {
	int voltage_effnorm;
	int voltage_effmax;
} module_effect;

static struct battery_module_calibration {
	module_effect brightness_ef[10];//brghtness effect
	module_effect wifi_ef[10];//wifi effect
	module_effect adapter_ef[10];//adapter effect
} module_calibration;

static struct battery_read_cap {
	int adc_index;
	int oldCapacity;
} read_cap = {0, -1};

static struct battery_full_logic {
	int sCharging_full_cnt;
	unsigned int sOldAdcRecorder;
	bool isFull;
} bat_full_logic = { 0, 0, false };

struct battery_config {
	int update_time ;
	int wmt_operation;
	int ADC_USED;
	int ADC_TYP;		// 1- SPI;3-I2C
	int charge_max;
	int charge_min;
	int discharge_vp[11];	//battery capacity curve:v0--v10
	int lowVoltageLevel;
	int judge_full_by_time;
	int shutdown_capacity;
};

static struct battery_config bat_config = {
	.update_time = 5000,
	.wmt_operation = 0,
	.ADC_USED = 0,
	.ADC_TYP = 1,
	.charge_max = 0xb78,
	.charge_min = 0x972,
	.discharge_vp = { 0xF5A,0xEE2,0xE92,0xE55,
		0xE1C,0xDDB,0xDB5,0xD9D,
		0xD95,0xD6A,0xCD1 },
	.lowVoltageLevel = 0xd48,
	.judge_full_by_time = 0,
	.shutdown_capacity = 0,
};

static int debug_mode = 0; //report fake capacity
static int bat_print = 0;
static struct proc_dir_entry* bat_proc = NULL;
static struct proc_dir_entry* bat_module_change_proc = NULL;

static bool mBrightnessHasSet = false;
static int adapterStateChange = ADAPTER_STATE_CHANGE_T_COUNT;

static struct power_supply bat_ps;

struct battery_led {
	char *name;
	struct led_trigger *trigger;
};

static bool battery_led_registered = false;
static struct battery_led led_red = { .name = "bat-red", };
static struct battery_led led_green = { .name = "bat-green", };

enum {
	LED_GREEN = 0,
	LED_RED,
};

static struct gpio_led wmt_leds[] __initdata = {
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

static struct gpio_led_platform_data wmt_gpio_led_data = {
	.leds = wmt_leds,
	.num_leds = ARRAY_SIZE(wmt_leds),
};

static struct platform_device wmt_led_device = {
	.name		= "leds-gpio",
	.id		= -1,
	.dev		= {
		.platform_data = &wmt_gpio_led_data,
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

	wmt_leds[LED_GREEN].gpio = g;
	wmt_leds[LED_RED].gpio = r;

	pr_info("battery-led: red(gpio%d), green(gpio%d)\n", r, g);
	return 0;
}

#define RTC_NAME	"rtc0"
static struct rtc_device *rtc_dev;

static int wmt_leds_init(void)
{
	int rc;

	rc = parse_battery_led_param();
	if (rc)
		return rc;

	platform_device_register(&wmt_led_device);
	led_trigger_register_simple(led_red.name, &led_red.trigger);
	led_trigger_register_simple(led_green.name, &led_green.trigger);

	battery_led_registered = true;
	return 0;
}

static void wmt_leds_exit(void)
{
	if (!battery_led_registered)
		return;

	led_trigger_unregister_simple(led_red.trigger);
	led_trigger_unregister_simple(led_green.trigger);
	platform_device_unregister(&wmt_led_device);
	battery_led_registered = false;
}

static int adc_repair(int adc);
static int adc2cap(int adc);

static struct delayed_work bat_work;
static struct mutex work_lock;

struct battery_session {
	int dcin;
	int status;
	int capacity;
};

static struct battery_session bat_sesn = {
	.dcin = 1,
	.status = POWER_SUPPLY_STATUS_UNKNOWN,
	.capacity = 100,
};

static int sus_update_cnt = SUSPEND_RESUME_T_COUNT;
static bool bat_susp_resu_flag = false; //0: normal, 1--suspend then resume
static int judgeVoltageHopNextTime = 0;
static bool use_cur_cap_after_susp_delay = false; //use current capacity to return insetead regard it as hop voltage
static bool use_cur_cap_after_adapter_plug_out = false; //use current capacity to return insetead regard it as hop voltage

extern int wmt_setsyspara(char *varname, char *varval);

extern unsigned short vt1603_get_bat_info(int dcin_status);

static int wmt_bat_is_batterypower(void);

static int wmt_batt_initparam(void)
{
	char buf[200];
	char varname[] = "wmt.io.bat";
	int varlen = 200;

	if (wmt_getsyspara(varname, buf, &varlen) == 0){
		sscanf(buf,"%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x",
		       &bat_config.ADC_TYP,
		       &bat_config.wmt_operation,
		       &bat_config.update_time,
		       &bat_config.charge_max,
		       &bat_config.charge_min,
		       &bat_config.discharge_vp[10],
		       &bat_config.discharge_vp[9],
		       &bat_config.discharge_vp[8],
		       &bat_config.discharge_vp[7],
		       &bat_config.discharge_vp[6],
		       &bat_config.discharge_vp[5],
		       &bat_config.discharge_vp[4],
		       &bat_config.discharge_vp[3],
		       &bat_config.discharge_vp[2],
		       &bat_config.discharge_vp[1],
		       &bat_config.discharge_vp[0]);
		switch( bat_config.wmt_operation){
		case 0: /* for RDK */
			break;
		case 1: /* for droid book */
			bat_config.ADC_USED= 0;
			break;
		case 2: /* for MID */
			bat_config.ADC_USED= 1;
			break;
		default:
			bat_config.ADC_USED= 0;
			break;
		}
	}else{
		bat_config.charge_max = 0xE07;
		bat_config.charge_min = 0xC3A;
		bat_config.discharge_vp[10] = 0xeb3;
		bat_config.discharge_vp[9] = 0xe71;
		bat_config.discharge_vp[8] = 0xe47;
		bat_config.discharge_vp[7] = 0xe1c;
		bat_config.discharge_vp[6] = 0xdf0;
		bat_config.discharge_vp[5] = 0xdc7;
		bat_config.discharge_vp[4] = 0xda0;
		bat_config.discharge_vp[3] = 0xd6c;
		bat_config.discharge_vp[2] = 0xd30;
		bat_config.discharge_vp[1] = 0xceb;
		bat_config.discharge_vp[0] = 0xcb0;
		bat_config.ADC_USED = 0;
	}

	memset(&module_calibration, 0, sizeof(struct battery_module_calibration));

	if (wmt_getsyspara("wmt.io.bateff.brightness", buf, &varlen) == 0) {
		sscanf(buf,"%x-%x:%x-%x:%x-%x:%x-%x:%x-%x:%x-%x:%x-%x:%x-%x:%x-%x:%x-%x",
		       &module_calibration.brightness_ef[9].voltage_effnorm,
		       &module_calibration.brightness_ef[9].voltage_effmax,
		       &module_calibration.brightness_ef[8].voltage_effnorm,
		       &module_calibration.brightness_ef[8].voltage_effmax,
		       &module_calibration.brightness_ef[7].voltage_effnorm,
		       &module_calibration.brightness_ef[7].voltage_effmax,
		       &module_calibration.brightness_ef[6].voltage_effnorm,
		       &module_calibration.brightness_ef[6].voltage_effmax,
		       &module_calibration.brightness_ef[5].voltage_effnorm,
		       &module_calibration.brightness_ef[5].voltage_effmax,
		       &module_calibration.brightness_ef[4].voltage_effnorm,
		       &module_calibration.brightness_ef[4].voltage_effmax,
		       &module_calibration.brightness_ef[3].voltage_effnorm,
		       &module_calibration.brightness_ef[3].voltage_effmax,
		       &module_calibration.brightness_ef[2].voltage_effnorm,
		       &module_calibration.brightness_ef[2].voltage_effmax,
		       &module_calibration.brightness_ef[1].voltage_effnorm,
		       &module_calibration.brightness_ef[1].voltage_effmax,
		       &module_calibration.brightness_ef[0].voltage_effnorm,
		       &module_calibration.brightness_ef[0].voltage_effmax);
	}

	if (wmt_getsyspara("wmt.io.bateff.wifi", buf, &varlen) == 0) {
		sscanf(buf,"%x-%x:%x-%x:%x-%x:%x-%x:%x-%x:%x-%x:%x-%x:%x-%x:%x-%x:%x-%x",
		       &module_calibration.wifi_ef[9].voltage_effnorm,
		       &module_calibration.wifi_ef[9].voltage_effmax,
		       &module_calibration.wifi_ef[8].voltage_effnorm,
		       &module_calibration.wifi_ef[8].voltage_effmax,
		       &module_calibration.wifi_ef[7].voltage_effnorm,
		       &module_calibration.wifi_ef[7].voltage_effmax,
		       &module_calibration.wifi_ef[6].voltage_effnorm,
		       &module_calibration.wifi_ef[6].voltage_effmax,
		       &module_calibration.wifi_ef[5].voltage_effnorm,
		       &module_calibration.wifi_ef[5].voltage_effmax,
		       &module_calibration.wifi_ef[4].voltage_effnorm,
		       &module_calibration.wifi_ef[4].voltage_effmax,
		       &module_calibration.wifi_ef[3].voltage_effnorm,
		       &module_calibration.wifi_ef[3].voltage_effmax,
		       &module_calibration.wifi_ef[2].voltage_effnorm,
		       &module_calibration.wifi_ef[2].voltage_effmax,
		       &module_calibration.wifi_ef[1].voltage_effnorm,
		       &module_calibration.wifi_ef[1].voltage_effmax,
		       &module_calibration.wifi_ef[0].voltage_effnorm,
		       &module_calibration.wifi_ef[0].voltage_effmax);
	}

	if (wmt_getsyspara("wmt.io.bateff.adapter", buf, &varlen) == 0) {
		sscanf(buf,"%x-%x:%x-%x:%x-%x:%x-%x:%x-%x:%x-%x:%x-%x:%x-%x:%x-%x:%x-%x",
		       &module_calibration.adapter_ef[9].voltage_effnorm,
		       &module_calibration.adapter_ef[9].voltage_effmax,
		       &module_calibration.adapter_ef[8].voltage_effnorm,
		       &module_calibration.adapter_ef[8].voltage_effmax,
		       &module_calibration.adapter_ef[7].voltage_effnorm,
		       &module_calibration.adapter_ef[7].voltage_effmax,
		       &module_calibration.adapter_ef[6].voltage_effnorm,
		       &module_calibration.adapter_ef[6].voltage_effmax,
		       &module_calibration.adapter_ef[5].voltage_effnorm,
		       &module_calibration.adapter_ef[5].voltage_effmax,
		       &module_calibration.adapter_ef[4].voltage_effnorm,
		       &module_calibration.adapter_ef[4].voltage_effmax,
		       &module_calibration.adapter_ef[3].voltage_effnorm,
		       &module_calibration.adapter_ef[3].voltage_effmax,
		       &module_calibration.adapter_ef[2].voltage_effnorm,
		       &module_calibration.adapter_ef[2].voltage_effmax,
		       &module_calibration.adapter_ef[1].voltage_effnorm,
		       &module_calibration.adapter_ef[1].voltage_effmax,
		       &module_calibration.adapter_ef[0].voltage_effnorm,
		       &module_calibration.adapter_ef[0].voltage_effmax);
	}

	bat_config.lowVoltageLevel = bat_config.discharge_vp[0];

	//追加电压不变超过2分钟，判断为满电的逻辑
	if(wmt_getsyspara("wmt.io.bateff.full_by_time", buf, &varlen) == 0){
		sscanf(buf,"%d", &bat_config.judge_full_by_time);
	} else {
		bat_config.judge_full_by_time = 0;
	}
    
	//追加设置电池关机百分比的逻辑
	if(wmt_getsyspara("wmt.io.bateff.shutdown_capacity", buf, &varlen) == 0){
		sscanf(buf,"%d", &bat_config.shutdown_capacity);
	} else {
		bat_config.shutdown_capacity = 0;
	}

	return 0;
}

static int batt_writeproc(struct file *file, const char *buffer,
			  unsigned long count, void *data)
{
	if (sscanf(buffer, "debug_mode=%d", &debug_mode)) {
		batdbg("debug_mode=%d\n", debug_mode);
	}
	else if(sscanf(buffer, "bat_print=%d", &bat_print)) {
		batdbg("bat_print=%d\n", bat_print);
	}

	return count;
}

static int batt_readproc(char *page, char **start, off_t off,
			 int count, int *eof, void *data)
{
	int l = 0;

	l += sprintf(page + l, "debug_mode = %d\n", debug_mode);
	l += sprintf(page + l, "bat_print  = %d\n", bat_print);

	return l;
}

static int getEffect(int adc, int usage, module_effect* effects, int size)
{
	int expectAdc, effect, i;
	for(i = 0; i < size; i++){
		effect =  (effects[i].voltage_effmax - effects[i].voltage_effnorm) * usage / 100;
		expectAdc = effects[i].voltage_effnorm + effect;
		if(adc <= effects[i].voltage_effmax)
		{
			//printk("i is %d, adc is %d , effects_norm = %d,"
			//       "effects_max = %d, usage = %d, effect = %d\n",
			//i, adc, effects[i].voltage_effnorm, effects[i].voltage_effmax, usage, effect);
			return effect;
		}
	}
	//printk("adc is %d , no match in effects, usage = %d, \n", adc, usage);
	if(size > 0)
		return (effects[size-1].voltage_effmax - effects[size-1].voltage_effnorm) * usage / 100;
	return 0;
}

//for debug
static int getEffectLevel(int adc, int usage, module_effect* effects, int size)
{
	int expectAdc, effect, i;
	for(i = 0; i < size; i++){
		effect =  (effects[i].voltage_effmax - effects[i].voltage_effnorm) * usage / 100;
		expectAdc = effects[i].voltage_effnorm + effect;
		if(adc <= effects[i].voltage_effmax) {
			return i;
		}
	}

	if(size > 0)
		return size - 1;
	return -1;
}

static int adc_repair(int adc)
{
	int adc_add = adc;
	int comp, usage;

	batdbg("compensation Start -> %d\n", adc_add);

	if (power_supply_am_i_supplied(&bat_ps)) {
		comp = getEffect(adc, 100, module_calibration.adapter_ef,
				 ARRAY_SIZE(module_calibration.adapter_ef));
		adc_add -= comp;
		batdbg("compensation Adapter - (%d) -> %d\n", comp, adc_add);
		return adc_add;
	}

	/* compensation with USB-PC */
	if (wmt_charger_is_dc_plugin()) {
		if (bat_sesn.capacity >= 0 && bat_sesn.capacity <= 100) {
			comp = (60 * (100 - bat_sesn.capacity)) / 100;
			adc_add -= comp;
			batdbg("compensation USB-PC(%d%%) - (%d) -> %d\n",
			       bat_sesn.capacity, comp, adc_add);
		}
	}

	// brightness
	usage = 100 - module_usage[BM_BRIGHTNESS];
	if (usage < 0)
		usage = 0;
	if (usage > 100)
		usage = 100;
	comp = getEffect(adc, usage, module_calibration.brightness_ef,
			 ARRAY_SIZE(module_calibration.brightness_ef));
	adc_add -= comp;
	batdbg("compensation Brightness - (%d) -> %d\n", comp, adc_add);

	// video
	comp = (module_usage[BM_VIDEO]*45) / 100;
	adc_add += comp;
	batdbg("compensation Video + (%d) -> %d\n", comp, adc_add);

	// usb
	comp = getEffect(adc, module_usage[BM_USBDEVICE], module_calibration.wifi_ef,
			 ARRAY_SIZE(module_calibration.wifi_ef));
	adc_add += comp; 
	batdbg("compensation USB - (%d) -> %d\n", comp, adc_add);

	// hdmi
	if (module_usage[BM_HDMI] > 0) {
		comp = 15;
		adc_add += comp;
		batdbg("compensation HDMI + (%d) -> %d\n", comp, adc_add);
	}

	return adc_add;
}

static int adc2cap(int adc)
{
	int i;

	if (adc >= bat_config.discharge_vp[10])
		return 100;

	if (adc <= bat_config.discharge_vp[0])
		return 0;

	for (i = 10; i >= 1; i--) {
		if ((adc <= bat_config.discharge_vp[i]) &&
		    (adc > bat_config.discharge_vp[i-1])) {
			return ((adc - bat_config.discharge_vp[i-1]) * 10) / 
				(bat_config.discharge_vp[i]-bat_config.discharge_vp[i-1]) + (i-1)*10;
		}
	}

	return -1;
}

//去除最大最小值，然后返回一个平均值
static unsigned int get_best_value(unsigned int adcVal[], int len)
{
	unsigned int returnValue;
	unsigned int maxValue = adcVal[0], minValue = adcVal[0], sum = adcVal[0];
	int i = 1;
	for(; i < len; i++)
	{
		if(adcVal[i] > maxValue) {
			maxValue = adcVal[i];
		}
		if(adcVal[i] < minValue) {
			minValue = adcVal[i];
		}
		sum += adcVal[i];
	}
	sum = sum - maxValue - minValue;
	returnValue = sum/(len - 2);
	return returnValue;
}

static int convertValue(int cap)
{
	if(cap<0)
		cap = 0;
	if(cap>0)
		cap = ((cap - 1)/5 + 1)*5;
	if(cap>100)
		cap = 100;

	return cap;
}

static void reset_saved_adc(void)
{
	read_cap.oldCapacity = -1;
	read_cap.adc_index = 0;
}

static bool is_saved_adc_reset(void)
{
	if (read_cap.oldCapacity == -1 && read_cap.adc_index == 0)
		return true;
	else
		return false;
}

static unsigned short hx_batt_read(int fast)
{
	unsigned short adcval = 0;

	adcval = vt1603_get_bat_info(fast);
	return adcval;
}

static int wmt_bat_is_batterypower(void)
{
	return bat_sesn.dcin == 0;
}

static int wmt_bat_is_charging(void)
{
	return bat_sesn.status == POWER_SUPPLY_STATUS_CHARGING;
}

static int wmt_read_status(void)
{
	int status;

	if (power_supply_am_i_supplied(&bat_ps)) {
		if (wmt_charger_is_charging_full())
			status = POWER_SUPPLY_STATUS_FULL;
		else
			status = POWER_SUPPLY_STATUS_CHARGING;
	} else {
		status = POWER_SUPPLY_STATUS_DISCHARGING;
	}

	return status;
}

static int wmt_read_voltage(void)
{
	return hx_batt_read(1);
}

static int mc_writeproc(struct file *file, const char *buffer,
			unsigned long count, void *data)
{
	int bm;
	int usage;

	if (sscanf(buffer, "MODULE_CHANGE:%d-%d",&bm,&usage)) {
		if (bm<BM_VOLUME||bm>BM_COUNT-1) {
			printk("bm %d error,min %d max %d\n",bm,BM_VOLUME,BM_COUNT-1);
			return 0;
		}
		if (usage>100||usage<0) {
			printk("usage %d error\n",usage);
			return 0;
		}
		if (!mBrightnessHasSet && bm == BM_BRIGHTNESS) {
			mBrightnessHasSet = true;
		}
		printk("mc_write:%d--%d\n",bm,usage);
		// calculate the battery capacity curve
		mutex_lock(&work_lock);
		module_usage[bm]=usage;
		mutex_unlock(&work_lock);

	}

	printk("module_usage[BM_USBDEVICE] %d\n", module_usage[BM_USBDEVICE]);
	return count;
}

static int mc_readproc(char *page, char **start, off_t off,
		       int count, int *eof, void *data)
{
	int len = 0;
	int power_source; // 1--battery;2--external adapter.

	unsigned short adc_val,adc_rval = -1;
	int capacity,cap_add,wrong_repair_cap = 0;//,i;
	BatteryModule j;
	char usage[128];

	int bmUsage,wifiUsage;

	mutex_lock(&work_lock);
	adc_val= hx_batt_read(1);
	mutex_unlock(&work_lock);


	adc_rval = adc_repair(adc_val);
	capacity = adc2cap(adc_val);
	cap_add = adc2cap(adc_rval);

	len = sprintf(page, "adc:%d,cap1:%d,cap2:%d", adc_val,capacity,cap_add);

	//在电量比较高的情况下，低电gpio报错了（因调节背光等所导致的瞬间低电），之后
	//的低电就不能用gpio来判断，只能靠电量，此时的wrong_repair_cap就是返回的电量
	//（在原基础上自减5%）
	for(j=BM_VOLUME;j<BM_COUNT;j++){
		sprintf(usage,",%d-%d",j,module_usage[j]);
		strcat(page,usage);
		len+=strlen(usage);
	}

	bat_sesn.dcin = wmt_charger_is_dc_plugin();
	power_source = !wmt_bat_is_batterypower();

	bmUsage = 100 - module_usage[BM_BRIGHTNESS];
	if (bmUsage < 0)
		bmUsage = 0;
	if (bmUsage > 100)
		bmUsage = 100;

	wifiUsage = 100 - module_usage[BM_WIFI];

	sprintf(usage, ",bf:%d,adapter:%d,adc_repair:%d, bright_repair_level:%d(%d%%),"
		" adapter_repair_level:%d, wifi_repair_level:%d, wrCap3:%d \n",
		POWER_SUPPLY_STATUS_FULL == wmt_read_status(),
		power_source,
		adc_rval,
		getEffectLevel(adc_val, bmUsage, module_calibration.brightness_ef,
			       ARRAY_SIZE(module_calibration.brightness_ef)),
		bmUsage,
		getEffectLevel(adc_val, power_source*100, module_calibration.adapter_ef,
			       ARRAY_SIZE(module_calibration.adapter_ef)),
		getEffectLevel(adc_val, wifiUsage, module_calibration.wifi_ef,
			       ARRAY_SIZE(module_calibration.wifi_ef)),
		wrong_repair_cap);

	strcat(page, usage);
	len+=strlen(usage);

	return len;
}


static int wmt_read_capacity(struct power_supply *bat_ps, int fast)
{
	//开机瞬间，系统默认背光为0，此时经过修正后 会把电压拉到一个很低很低的值
	//导致系统开机后因低电被关闭(此时电量其实比较高，但是因为默认的背光把它拉低了)
	//等整个系统启动结束后，背光才会第一次被设置成我们之前设置的值(大于40)
	//此时电量才不至于被拉得很低
	//所以如果检测到背光未被设值，我们就直接将背光默认为28。
	int capacity=0;
	unsigned int ADC_val = 0, ADC_repair_val = 0;
	static unsigned int adc_val[5];
	static int oldLowestAdc = -1;
	static int oldLowestCapacity = -1;
	static int lowVoltageCount = 0;

	batdbg("----begin---in wmt_read_capacity---\n");
	if (!mBrightnessHasSet)
		module_usage[BM_BRIGHTNESS] = 72;

	adc_val[read_cap.adc_index%5] = hx_batt_read(fast);

	//init adc_val
	if(is_saved_adc_reset()) {
		int i;
		batdbg("reset saved adc");
		for(i = 0; i < 5; i++) {
			adc_val[i] = adc_val[read_cap.adc_index%5];
		}
	}

	if(read_cap.adc_index++%5==0) {
		ADC_val = get_best_value(adc_val, sizeof(adc_val) / sizeof(unsigned int));
	} else {
		if (read_cap.oldCapacity == -1)
			ADC_val = get_best_value(adc_val, sizeof(adc_val) / sizeof(unsigned int));
		else {
			capacity = convertValue(read_cap.oldCapacity);
			batdbg("--return old Capacity = %d  \n",capacity);
			return capacity;
		}
	}

	if (oldLowestAdc == -1)
		oldLowestAdc = ADC_val;
	//新获取的电压值在非充电情况下，比前一个值大了0.005V
	//0.005V有可能是AD转换后的浮动误差，也可能是调节背光等引起的电压变化
	//若浮动范围在0.005V以内，则取上一个的值（比新取的值低0.005V以内）
	if( ADC_val - oldLowestAdc > 0 && ADC_val - oldLowestAdc < 5) {
		ADC_val = oldLowestAdc;
	} else {
		oldLowestAdc = ADC_val;//ADC_val比oldLowestAdc低，或者ADC_val比oldLowestAdc高0.005V以上
	}

	ADC_repair_val = adc_repair(ADC_val);
	capacity = adc2cap(ADC_repair_val);
	batdbg("ADC_val %d, ADC_repair_val %d, capacity %d\n",
	       ADC_val, ADC_repair_val, capacity);

	if(oldLowestCapacity == -1)
		oldLowestCapacity = capacity;
	//对电量作向下取值的平滑处理
	if (capacity - oldLowestCapacity > 0 &&
	    capacity - oldLowestCapacity < 8 &&
	    oldLowestCapacity < 90) { //手指对屏幕进行操作，电量会下降4~6%
		capacity = oldLowestCapacity;
	} else {
		oldLowestCapacity = capacity;//ADC_val比oldLowestAdc低，或者ADC_val比oldLowestAdc高0.005V以上
	}

	if(judgeVoltageHopNextTime > JUDGE_HOP_VOLTAGE_TIMES)
		judgeVoltageHopNextTime = JUDGE_HOP_VOLTAGE_TIMES;


	batdbg("-- use_cur_cap_after_susp_delay = %d \n", use_cur_cap_after_susp_delay);
	batdbg("-- use_cur_cap_after_adapter_plug_out = %d \n", use_cur_cap_after_adapter_plug_out);
	//跳变大于3%的点去掉，持续20秒还是这个电量，则重新返回新的电量值。
	if(/*!adapterStateChange &&*/ judgeVoltageHopNextTime!=JUDGE_HOP_VOLTAGE_TIMES &&
	   read_cap.oldCapacity != -1 &&
	   (capacity - read_cap.oldCapacity > 3 || read_cap.oldCapacity - capacity > 3) &&
	   !use_cur_cap_after_susp_delay &&
	   !use_cur_cap_after_adapter_plug_out) {

		batdbg("--*********judgeVoltageHopNextTime = %d, hopCap = %d \n",
		       judgeVoltageHopNextTime, capacity);
		++judgeVoltageHopNextTime;
		capacity = convertValue(read_cap.oldCapacity);
		batdbg("--*********return hop value ~  Capacity = %d  \n",capacity);
		return capacity;
	}
	/*adapterStateChange = false;*/
	judgeVoltageHopNextTime = 0;
	use_cur_cap_after_susp_delay = false;
	use_cur_cap_after_adapter_plug_out = false;

	if (wmt_bat_is_charging() && capacity <= bat_config.shutdown_capacity){
		capacity = bat_config.shutdown_capacity + 1;
	}

	read_cap.oldCapacity = capacity;
	capacity = convertValue(capacity);

	if (capacity <= bat_config.shutdown_capacity) {
		lowVoltageCount++;
		batdbg("--lower than voltagelevel, try shuting down... %d\n", lowVoltageCount);
		if (lowVoltageCount > 3) {
			batdbg("shuting down... \n");
			capacity = 0;
		}
		read_cap.oldCapacity = capacity;
		return capacity;
	} else {
		batdbg("new reset lowvoltage count... \n");
		lowVoltageCount = 0;
	}

	//If battery capacity > 90 and last for 2 minutes in charging mode,
	//and voltage no increase andy more,
	//then set current battery state is full.
	//Run this function every 5 seconds.
	//比较电量应该是本次和上次比较。而不是同开机后最高的电压比较，
	//这样有可能导致系统开机时获取到低负载下的高电量，
	//然后播视频，之后的电量都是相对低，导致误设满电

	batdbg("bat_status full = %d, oldCap = %d, bat_full_logic = %d \n",
	       bat_sesn.status == POWER_SUPPLY_STATUS_FULL,
	       read_cap.oldCapacity, bat_full_logic.isFull);

	batdbg("bat_config.judge_full_by_time = %d \n", bat_config.judge_full_by_time);

	if (bat_config.judge_full_by_time &&
	    bat_sesn.status == POWER_SUPPLY_STATUS_CHARGING &&
	    read_cap.oldCapacity >= 90 &&
	    !bat_full_logic.isFull) {
		batdbg("unrepair_adc_val = %d \n", ADC_val);
		if(ADC_val == bat_full_logic.sOldAdcRecorder) {
			batdbg("charging_cnt = %d, equal ADC = %d\n",
			       bat_full_logic.sCharging_full_cnt, ADC_val);
			bat_full_logic.sCharging_full_cnt+=5;
		} else {
			batdbg("update tempHigh ADC and reset full cnt\n");
			bat_full_logic.sCharging_full_cnt = 0;
		}
		bat_full_logic.sOldAdcRecorder = ADC_val;

		if (bat_full_logic.sCharging_full_cnt >= CHARGING_FULL_T_COUNT) {
			batdbg("set bat full \n");
			bat_full_logic.isFull = true;
		}
	} else if (read_cap.oldCapacity <= 90) {
		bat_full_logic.isFull = false;
	}

	batdbg("----end----get. Capacity = %d  \n",capacity);
	return capacity;
}

static void wmt_bat_update(struct power_supply *psy)
{
	static int reportLowVoltageCount = 0;
	static int lowVoltageTimes = 120;
	int status, capacity;
	bool dcin, is_charging;

	status	= wmt_read_status();
	dcin	= wmt_charger_is_dc_plugin();
	is_charging = (bat_sesn.status == POWER_SUPPLY_STATUS_CHARGING);

	if (dcin != bat_sesn.dcin) {
		pr_debug("External adapter plug state changed: %d -> %d\n", bat_sesn.dcin, dcin);

		adapterStateChange = 0;
		bat_full_logic.sCharging_full_cnt = 0;

		use_cur_cap_after_adapter_plug_out = true;
		reset_saved_adc();

		bat_full_logic.isFull = false;
	}

	if (bat_susp_resu_flag == true) {
		sus_update_cnt = 0;
		bat_susp_resu_flag = false;
		judgeVoltageHopNextTime = 0;
		use_cur_cap_after_susp_delay = true;
		reset_saved_adc();
	}

	if (adapterStateChange < ADAPTER_STATE_CHANGE_T_COUNT ||
	    sus_update_cnt < SUSPEND_RESUME_T_COUNT) {
		capacity = bat_sesn.capacity;
		adapterStateChange++;
		sus_update_cnt++;
	} else {
		capacity = wmt_read_capacity(psy, 1);
	}

	if (status == POWER_SUPPLY_STATUS_FULL)
		capacity = 100;

	if (capacity < 10)
		lowVoltageTimes = 60;

	if (capacity <= 10 && !is_charging) {
		reportLowVoltageCount++;
	}

	if ((bat_sesn.dcin != dcin) || (bat_sesn.status != status) ||
	    (bat_sesn.capacity != capacity) || (capacity <= 0) ||
	    (reportLowVoltageCount >= lowVoltageTimes && capacity <= 10 && !is_charging)) {

		switch (status) {
		case POWER_SUPPLY_STATUS_CHARGING:
			led_trigger_event(led_red.trigger, LED_FULL);
			led_trigger_event(led_green.trigger, LED_OFF);
			break;
		case POWER_SUPPLY_STATUS_FULL:
			led_trigger_event(led_red.trigger, LED_OFF);
			led_trigger_event(led_green.trigger, LED_FULL);
			break;
		case POWER_SUPPLY_STATUS_DISCHARGING:
		default:
			led_trigger_event(led_red.trigger, LED_OFF);
			led_trigger_event(led_green.trigger, LED_OFF);
			break;
		}

		reportLowVoltageCount = 0;
		power_supply_changed(psy);

		mutex_lock(&work_lock);
		bat_sesn.dcin = dcin;
		bat_sesn.status = status;
		bat_sesn.capacity = capacity;
		mutex_unlock(&work_lock);
	}
}

static int wmt_bat_get_property(struct power_supply *psy,
				enum power_supply_property psp,
				union power_supply_propval *val)
{
	switch (psp) {
	case POWER_SUPPLY_PROP_STATUS:
		val->intval = wmt_read_status();
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_NOW:
		val->intval = wmt_read_voltage();
		break;
	case POWER_SUPPLY_PROP_CAPACITY:
		if (debug_mode)
			val->intval = 100;
		else
			val->intval = bat_sesn.capacity;
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_MAX:
		val->intval = 4000;
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_MIN:
		val->intval = 3000;
		break;
	case POWER_SUPPLY_PROP_HEALTH:
		val->intval = POWER_SUPPLY_HEALTH_GOOD;
		break;
	case POWER_SUPPLY_PROP_TECHNOLOGY:
		val->intval = POWER_SUPPLY_TECHNOLOGY_LION;
		break;
	case POWER_SUPPLY_PROP_PRESENT:
		val->intval = 1;
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static void wmt_external_power_changed(struct power_supply *psy)
{
	schedule_delayed_work(&bat_work, 0*HZ);
}

static enum power_supply_property wmt_bat_props[] = {
	POWER_SUPPLY_PROP_PRESENT,
	POWER_SUPPLY_PROP_STATUS,
	POWER_SUPPLY_PROP_TECHNOLOGY,
	POWER_SUPPLY_PROP_VOLTAGE_NOW,
	POWER_SUPPLY_PROP_VOLTAGE_MAX,
	POWER_SUPPLY_PROP_VOLTAGE_MIN,
	POWER_SUPPLY_PROP_CAPACITY,
	POWER_SUPPLY_PROP_HEALTH,
};

static struct power_supply bat_ps = {
	.name		= "wmt-bat",
	.type		= POWER_SUPPLY_TYPE_BATTERY,
	.get_property	= wmt_bat_get_property,
	.properties 	= wmt_bat_props,
	.num_properties	= ARRAY_SIZE(wmt_bat_props),
	.use_for_apm	= 1,
	.external_power_changed = wmt_external_power_changed,
};

static void wmt_battery_work(struct work_struct *work)
{
	wmt_bat_update(&bat_ps);
	schedule_delayed_work(&bat_work, 1*HZ);
}

static int __devinit wmt_battery_probe(struct platform_device *dev)
{
	int ret;

	mutex_init(&work_lock);

	INIT_DELAYED_WORK(&bat_work, wmt_battery_work);

	ret = power_supply_register(&dev->dev, &bat_ps);
	if (ret) {
		return ret;
	}

	schedule_delayed_work(&bat_work, 0);

	printk("wmt_battery_probe done.\n");
	return 0;
}

static int __devexit wmt_battery_remove(struct platform_device *dev)
{
	cancel_delayed_work_sync(&bat_work);
	power_supply_unregister(&bat_ps);
	return 0;
}

static int wmt_battery_suspend(struct device *dev)
{
	cancel_delayed_work_sync(&bat_work);

	mutex_lock(&work_lock);
	bat_susp_resu_flag = true;
	bat_full_logic.sCharging_full_cnt = 0;
	sus_update_cnt = 0;
	mutex_unlock(&work_lock);
	return 0;
}

static int wmt_battery_resume(struct device *dev)
{
	schedule_delayed_work(&bat_work, 0*HZ);
	return 0;
}

static int wmt_battery_suspend_prepare(struct device *dev)
{
	struct rtc_wkalrm tmp;
	unsigned long time, now;
	unsigned long add = 30;   /* seconds */

	if (!battery_led_registered ||
	    !power_supply_am_i_supplied(&bat_ps))
		return 0;

	if (!rtc_dev) {
		rtc_dev = rtc_class_open(RTC_NAME);
		if (IS_ERR_OR_NULL(rtc_dev)) {
			rtc_dev = NULL;
			pr_err("Cannot get RTC %s, %ld.\n", RTC_NAME, PTR_ERR(rtc_dev));
			return 0;
		}
	}

	tmp.enabled = 1;
	rtc_read_time(rtc_dev, &tmp.time);
	rtc_tm_to_time(&tmp.time, &now);
	time = now + add;

	rtc_time_to_tm(time, &tmp.time);
	rtc_set_alarm(rtc_dev, &tmp);
	return 0;
}

static void wmt_battery_resume_complete(struct device *dev)
{
	if (rtc_dev) {
		rtc_class_close(rtc_dev);
		rtc_dev = NULL;
	}
	return;
}

static const struct dev_pm_ops wmt_battery_manager_pm = {
	.prepare = wmt_battery_suspend_prepare,
	.suspend = wmt_battery_suspend,
	.resume	= wmt_battery_resume,
	.complete = wmt_battery_resume_complete,
};

static struct platform_driver wmt_battery_driver = {
	.driver	= {
		.name = DRVNAME,
		.owner = THIS_MODULE,
		.pm = &wmt_battery_manager_pm,
	},
	.probe = wmt_battery_probe,
	.remove = __devexit_p(wmt_battery_remove),
};

extern int vt1603_bat_init(void);
extern void vt1603_bat_exit(void);

static struct platform_device *pdev;

static int __init wmt_battery_init(void)
{
	int ret;

	if (wmt_charger_gpio_probe("vt1603"))
		return -EINVAL;

	if (wmt_batt_initparam())
		return -EINVAL;

	ret = vt1603_bat_init();
	if (ret)
		return ret;

	ret = platform_driver_register(&wmt_battery_driver);
	if (ret)
		return ret;

	pdev = platform_device_register_simple(DRVNAME, -1, NULL, 0);
	if (IS_ERR(pdev)) {
		ret = PTR_ERR(pdev);
		platform_driver_unregister(&wmt_battery_driver);
		return ret;
	}

	bat_proc = create_proc_entry(BATTERY_PROC_NAME, 0666, NULL);
	if (bat_proc) {
		bat_proc->read_proc = batt_readproc;
		bat_proc->write_proc = batt_writeproc;
	}

	bat_module_change_proc = create_proc_entry(BATTERY_CALIBRATION_PRO_NAME, 0666, NULL);
	if (bat_module_change_proc) {
		bat_module_change_proc->read_proc = mc_readproc;
		bat_module_change_proc->write_proc = mc_writeproc;
	}

	wmt_leds_init();
	memset(module_usage,0,sizeof(module_usage));
	return ret;
}

static void __exit wmt_battery_exit(void)
{
	if (bat_proc)
		remove_proc_entry(BATTERY_PROC_NAME, NULL);
	if (bat_module_change_proc)
		remove_proc_entry(BATTERY_CALIBRATION_PRO_NAME, NULL);

	vt1603_bat_exit();
	wmt_leds_exit();
	platform_driver_unregister(&wmt_battery_driver);
	platform_device_unregister(pdev);
	wmt_charger_gpio_release();
}

module_init(wmt_battery_init);
module_exit(wmt_battery_exit);

MODULE_AUTHOR("WonderMedia Technologies, Inc.");
MODULE_DESCRIPTION("WonderMedia battery driver");
MODULE_LICENSE("GPL");

