/*
 * Copyright (c) 2012, ASUSTek, Inc. All Rights Reserved.
 */

#ifndef __UPI_ug31xx_GAUGE_H
#define __UPI_ug31xx_GAUGE_H

//#define	UG31XX_DYNAMIC_POLLING

#define UG31XX_DEV_NAME        "ug31xx-gauge"

#ifdef UG31XX_DEBUG_ENABLE
#define GAUGE_notice(format, arg...) \
	printk(KERN_DEBUG "GAUGE: [%s] " format , __func__ , ## arg);
#else
#define GAUGE_notice(format, arg...)
#endif

#define GAUGE_err(format, arg...)	\
	printk(KERN_ERR "GAUGE: [%s] " format , __FUNCTION__ , ## arg);

#define	UPI_DEBUG_STRING	(320)

typedef enum {
 	DRV_NOT_READY = 0,
 	DRV_INIT_OK,
} drv_status_t;

struct ug31xx_gauge {
	struct i2c_client	*client;
	struct device	*dev;
	struct delayed_work batt_info_update_work;
	struct delayed_work ug31_gauge_info_work;
	struct wake_lock cable_wake_lock;
	struct mutex		info_update_lock;
	u32 cable_status;
	u32 polling_time;
	u32 batt_volt;
 	u32 batt_capacity;
	u32 batt_charge_now;
	u32 batt_charge_full;
 	int batt_current;
 	int batt_temp;
	int batt_status;

	/// [AT-PM] : Add for version ; 01/30/2013
	char gauge_debug[UPI_DEBUG_STRING];

	/// [AT-PM] : Alarm status ; 04/19/2013
	u8 alarmSts;
};

enum {
	PWR_SUPPLY_BATTERY = 0,
	PWR_SUPPLY_AC,
	PWR_SUPPLY_USB
};

enum {
	NO_CABLE = 0,
 	USB_PC_CABLE = 1,
 	AC_ADAPTER_CABLE = 3
};

#endif /*__UPI_ug31xx_GAUGE_H */
