/*++
	linux/drivers/input/sensor/sensor.h

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

#ifndef __SENSOR_H__
#define __SENSOR_H__
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <linux/proc_fs.h>
#include <linux/input.h>
#include <linux/delay.h>

#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif


//#define GSENSOR_I2C_NAME	"unused"
//#define GSENSOR_I2C_ADDR	0xff


#define GSENSOR_PROC_NAME "gsensor_config"
#define GSENSOR_INPUT_NAME "g-sensor"
#define GSENSOR_DEV_NODE "sensor_ctrl"

#define SENSOR_PROC_NAME "lsensor_config"
#define SENSOR_INPUT_NAME "l-sensor"
#define SENSOR_DEV_NODE "lsensor_ctrl"

#undef dbg
#define dbg(fmt, args...) if (l_sensorconfig.isdbg) printk(KERN_ALERT "[%s]: " fmt, __FUNCTION__, ## args)

#undef errlog
#undef klog
#define errlog(fmt, args...) printk(KERN_ERR "[%s]: " fmt, __FUNCTION__, ## args)
#define klog(fmt, args...) printk(KERN_ALERT "[%s]: " fmt, __FUNCTION__, ## args)

enum gsensor_id 
{
	MMA7660_DRVID = 0,
	MC3230_DRVID , 
	DMARD08_DRVID ,
	DMARD06_DRVID , 
	DMARD10_DRVID ,
	MXC622X_DRVID ,
	MMA8452Q_DRVID ,
	STK8312_DRVID ,
	KIONIX_DRVID,
	//add new gsensor id here, must be in order	
};

#define ISL29023_DRVID 0

struct wmt_gsensor_data{
	// for control
	int int_gpio; //0-3
	int op;
	int samp;
	int xyz_axis[3][2]; // (axis,direction)
	struct proc_dir_entry* sensor_proc;
	struct input_dev *input_dev;
	//struct work_struct work;
	struct delayed_work work; // for polling
	struct workqueue_struct *queue;
	int isdbg;
	int sensor_samp; // 
	int sensor_enable;  // 0 --> disable sensor, 1 --> enable sensor
	int test_pass;
	int offset[3];
	struct i2c_client *client;
#ifdef CONFIG_HAS_EARLYSUSPEND
	struct early_suspend earlysuspend;
#endif

};

///////////////////////// ioctrl cmd ////////////////////////
#define WMTGSENSOR_IOCTL_MAGIC  0x09
#define WMT_IOCTL_SENSOR_CAL_OFFSET  _IOW(WMTGSENSOR_IOCTL_MAGIC, 0x01, int) //offset calibration
#define ECS_IOCTL_APP_SET_AFLAG		 _IOW(WMTGSENSOR_IOCTL_MAGIC, 0x02, short)
#define ECS_IOCTL_APP_SET_DELAY		 _IOW(WMTGSENSOR_IOCTL_MAGIC, 0x03, short)
#define WMT_IOCTL_SENSOR_GET_DRVID	 _IOW(WMTGSENSOR_IOCTL_MAGIC, 0x04, unsigned int)
#define WMT_IOCTL_SENOR_GET_RESOLUTION		 _IOR(WMTGSENSOR_IOCTL_MAGIC, 0x05, short)

#define WMT_LSENSOR_IOCTL_MAGIC  0x10
#define LIGHT_IOCTL_SET_ENABLE		 _IOW(WMT_LSENSOR_IOCTL_MAGIC, 0x01, short)

/* Function prototypes */
extern struct i2c_client *sensor_i2c_register_device (int bus_no, int client_addr, const char *client_name);
extern struct i2c_client *sensor_i2c_register_device2(int bus_no, int client_addr, const char *client_name,void *pdata);
extern void sensor_i2c_unregister_device(struct i2c_client *client);


#endif
