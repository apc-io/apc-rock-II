/*
 * Copyright (C) 2011 MEMSIC, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/jiffies.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <linux/miscdevice.h>
#include <linux/mutex.h>
#include <linux/mm.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/delay.h>
#include <linux/sysctl.h>
#include <asm/uaccess.h>
#include <linux/input.h>
//#include <linux/mmc328x.h>
#include "mmc328x.h"
#define DEBUG			0
#define MAX_FAILURE_COUNT	3
#define READMD			0

#define MMC328X_DELAY_TM	10	/* ms */
#define MMC328X_DELAY_RM	10	/* ms */
#define MMC328X_DELAY_STDN	1	/* ms */
#define MMC328X_DELAY_RRM       1       /* ms */

#define MMC328X_RETRY_COUNT	3
#define MMC328X_RRM_INTV	100


//******************************* move from memsicd 2013-4-26
#define MMC328X_OFFSET_X		4096
#define MMC328X_OFFSET_Y		4096
#define MMC328X_OFFSET_Z		4096
//******************************************

#define MMC328X_DEV_NAME	"mmc328x"
#define CONFIG_SENSORS_MMC328xMA_MAG //add rambo 2013-4-20
struct i2c_client  *g_client;

extern int wmt_getsyspara(char *varname, unsigned char *varval, int *varlen);
static u32 read_idx = 0;

static struct i2c_client *this_client;

static struct wmt_msensor_data l_sensorconfig = {
	.op = 0,
	.int_gpio = 3,
	.samp = 5,
	.xyz_axis = {
		{ABS_X, -1},
		{ABS_Y, 1},
		{ABS_Z, -1},
		},
	.sensor_proc = NULL,
	.isdbg = 0,
	.sensor_samp = 10,  // 1 sample/second
	.sensor_enable = 1, // enable sensor
	.test_pass = 0, // for test program
	.offset={0,0,0},
};

static int get_axisset(void)
{
	char varbuf[64];
	int n;
	int varlen;

	memset(varbuf, 0, sizeof(varbuf));
	varlen = sizeof(varbuf);
	if (wmt_getsyspara("wmt.io.msensor328x", varbuf, &varlen)) {
		printk("Can't get gsensor config in u-boot!!!!\n");
		//return -1;
	} else {
		n = sscanf(varbuf, "%d:%d:%d:%d:%d:%d",
				
				&(l_sensorconfig.xyz_axis[0][0]),
				&(l_sensorconfig.xyz_axis[0][1]),
				&(l_sensorconfig.xyz_axis[1][0]),
				&(l_sensorconfig.xyz_axis[1][1]),
				&(l_sensorconfig.xyz_axis[2][0]),
				&(l_sensorconfig.xyz_axis[2][1])
				
			);
		if (n != 6) {
			printk("gsensor format is error in u-boot!!!\n");
			return -1;
		}
		l_sensorconfig.sensor_samp = l_sensorconfig.samp;

		printk("get the sensor config: %d:%d:%d:%d:%d:%d\n",
			
			l_sensorconfig.xyz_axis[0][0],
			l_sensorconfig.xyz_axis[0][1],
			l_sensorconfig.xyz_axis[1][0],
			l_sensorconfig.xyz_axis[1][1],
			l_sensorconfig.xyz_axis[2][0],
			l_sensorconfig.xyz_axis[2][1]
		 );
	}
	return 0;
}


static int mmc328x_i2c_rx_data(char *buf, int len)
{
	uint8_t i;
	struct i2c_msg msgs[] = {
		{
			.addr	= this_client->addr,
			.flags	= 0,
			.len	= 1,
			.buf	= buf,
		},
		{
			.addr	= this_client->addr,
			.flags	= I2C_M_RD,
			.len	= len,
			.buf	= buf,
		}
	};

	for (i = 0; i < MMC328X_RETRY_COUNT; i++) {
		if (i2c_transfer(this_client->adapter, msgs, 2) >= 0) {
			break;
		}
		mdelay(10);
	}

	if (i >= MMC328X_RETRY_COUNT) {
		pr_err("%s: retry over %d\n", __FUNCTION__, MMC328X_RETRY_COUNT);
		return -EIO;
	}

	return 0;
}

static int mmc328x_i2c_tx_data(char *buf, int len)
{
	uint8_t i;
	struct i2c_msg msg[] = {
		{
			.addr	= this_client->addr,
			.flags	= 0,
			.len	= len,
			.buf	= buf,
		}
	};
	
	for (i = 0; i < MMC328X_RETRY_COUNT; i++) {
		if (i2c_transfer(this_client->adapter, msg, 1) >= 0) {
			break;
		}
		mdelay(10);
	}

	if (i >= MMC328X_RETRY_COUNT) {
		pr_err("%s: retry over %d\n", __FUNCTION__, MMC328X_RETRY_COUNT);
		return -EIO;
	}
	return 0;
}

static int mmc328x_open(struct inode *inode, struct file *file)
{
	return nonseekable_open(inode, file);
}

static int mmc328x_release(struct inode *inode, struct file *file)
{
	return 0;
}

static int mmc328x_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	void __user *pa = (void __user *)arg;
	unsigned char data[16] = {0};
	int vec[3] = {0};
	int tmp[3] = {0};
	
        int MD_times = 0;

	switch (cmd) {
	case MMC328X_IOC_TM:
		data[0] = MMC328X_REG_CTRL;
		data[1] = MMC328X_CTRL_TM;
		if (mmc328x_i2c_tx_data(data, 2) < 0) {
			return -EFAULT;
		}
		/* wait TM done for coming data read */
		msleep(MMC328X_DELAY_TM);
		break;
	case MMC328X_IOC_RM:
		data[0] = MMC328X_REG_CTRL;
		data[1] = MMC328X_CTRL_RM;
		if (mmc328x_i2c_tx_data(data, 2) < 0) {
			return -EFAULT;
		}
		/* wait external capacitor charging done for next SET*/
		msleep(MMC328X_DELAY_RM);
		break;
	case MMC328X_IOC_RRM:
		data[0] = MMC328X_REG_CTRL;
		data[1] = MMC328X_CTRL_RRM;
		if (mmc328x_i2c_tx_data(data, 2) < 0) {
			return -EFAULT;
		}
		/* wait external capacitor charging done for next RRM */
		msleep(MMC328X_DELAY_RM);
		break;
	case MMC328X_IOC_READ:
		data[0] = MMC328X_REG_DATA;
		if (mmc328x_i2c_rx_data(data, 6) < 0) {
			return -EFAULT;
		}
		tmp[0] = data[1] << 8 | data[0];
		tmp[1] = data[3] << 8 | data[2];
		tmp[2] = data[5] << 8 | data[4];
		tmp[2] = 8192 - tmp[2] ;
	//add 2013-4-26	
		tmp[0]	-= MMC328X_OFFSET_X;
		tmp[1]	-= MMC328X_OFFSET_Y;
		tmp[2]	-= MMC328X_OFFSET_Z;
	//add end 		
		vec[0] = tmp[l_sensorconfig.xyz_axis[0][0]]*l_sensorconfig.xyz_axis[0][1];
	
	        vec[1] = tmp[l_sensorconfig.xyz_axis[1][0]]*l_sensorconfig.xyz_axis[1][1];
	
	        vec[2] = tmp[l_sensorconfig.xyz_axis[2][0]]*l_sensorconfig.xyz_axis[2][1];
	
	#if DEBUG
		printk("[X - %04x] [Y - %04x] [Z - %04x]\n", 
			vec[0], vec[1], vec[2]);
	#endif
		if (copy_to_user(pa, vec, sizeof(vec))) {
			return -EFAULT;
		}
		break;
	case MMC328X_IOC_READXYZ:
		/* do RM every MMC328X_RRM_INTV times read */
		if (!(read_idx %  MMC328X_RRM_INTV)) {
#ifdef CONFIG_SENSORS_MMC328xMA_MAG 
	            data[0] = MMC328X_REG_CTRL;
	            data[1] = MMC328X_CTRL_RRM;
	            mmc328x_i2c_tx_data(data, 2);
	            msleep(MMC328X_DELAY_RRM);
#endif
			/* RM */
			data[0] = MMC328X_REG_CTRL;
			data[1] = MMC328X_CTRL_RM;
			/* not check return value here, assume it always OK */
			mmc328x_i2c_tx_data(data, 2);
			/* wait external capacitor charging done for next RM */
			msleep(MMC328X_DELAY_RM);
		}
		read_idx++;

		/* send TM cmd before read */
		data[0] = MMC328X_REG_CTRL;
		data[1] = MMC328X_CTRL_TM;
		/* not check return value here, assume it always OK */
		mmc328x_i2c_tx_data(data, 2);
		/* wait TM done for coming data read */
		msleep(MMC328X_DELAY_TM);
#if READMD
		/* Read MD */
		data[0] = MMC328X_REG_DS;
		if (mmc328x_i2c_rx_data(data, 1) < 0) {
			return -EFAULT;
		}
		while (!(data[0] & 0x01)) {
			msleep(1);
			/* Read MD again*/
			data[0] = MMC328X_REG_DS;
			if (mmc328x_i2c_rx_data(data, 1) < 0) {
				return -EFAULT;
                        }
			
			if (data[0] & 0x01) break;
			MD_times++;
			if (MD_times > 2) {
		#if DEBUG
				printk("TM not work!!");
		#endif
				return -EFAULT;
			}
		}
#endif		
		/* read xyz raw data */
		data[0] = MMC328X_REG_DATA;
		if (mmc328x_i2c_rx_data(data, 6) < 0) {
			return -EFAULT;
		}
		tmp[0] = data[1] << 8 | data[0];
		tmp[1] = data[3] << 8 | data[2];
		tmp[2] = data[5] << 8 | data[4];
		tmp[2] = 8192 - tmp[2];
	//add 2013-4-26	
		tmp[0]	-= MMC328X_OFFSET_X;
		tmp[1]	-= MMC328X_OFFSET_Y;
		tmp[2]	-= MMC328X_OFFSET_Z;
	//add	
		vec[0] = tmp[l_sensorconfig.xyz_axis[0][0]]*l_sensorconfig.xyz_axis[0][1];
	
	        vec[1] = tmp[l_sensorconfig.xyz_axis[1][0]]*l_sensorconfig.xyz_axis[1][1];
	
	        vec[2] = tmp[l_sensorconfig.xyz_axis[2][0]]*l_sensorconfig.xyz_axis[2][1];
		
		

	#if DEBUG
		printk("[X - %04x] [Y - %04x] [Z - %04x]\n", 
			vec[0], vec[1], vec[2]);
	#endif
		if (copy_to_user(pa, vec, sizeof(vec))) {
			return -EFAULT;
		}

		break;
	default:
		break;
	}

	return 0;
}

static ssize_t mmc328x_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	ssize_t ret = 0;

	sprintf(buf, "MMC328X");//!!!
	ret = strlen(buf) + 1;

	return ret;
}

static DEVICE_ATTR(mmc328x, S_IRUGO, mmc328x_show, NULL);

static struct file_operations mmc328x_fops = {
	.owner		= THIS_MODULE,
	.open		= mmc328x_open,
	.release	= mmc328x_release,
	.unlocked_ioctl = mmc328x_ioctl,
};

static struct miscdevice mmc328x_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = MMC328X_DEV_NAME,
	.fops = &mmc328x_fops,
};

static int mmc328x_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	unsigned char data[16] = {0};
	int res = 0;

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		pr_err("%s: functionality check failed\n", __FUNCTION__);
		res = -ENODEV;
		goto out;
	}
	this_client = client;

	res = misc_register(&mmc328x_device);//
	if (res) {
		pr_err("%s: mmc328x_device register failed\n", __FUNCTION__);
		goto out;
	}
	res = device_create_file(&client->dev, &dev_attr_mmc328x);//
	if (res) {
		pr_err("%s: device_create_file failed\n", __FUNCTION__);
		goto out_deregister;
	}

	/* send RM/RRM cmd to mag sensor first of all */
#ifdef CONFIG_SENSORS_MMC328xMA_MAG 
	data[0] = MMC328X_REG_CTRL;
	data[1] = MMC328X_CTRL_RRM;
	if (mmc328x_i2c_tx_data(data, 2) < 0) {
	}
	msleep(MMC328X_DELAY_RRM);
	data[0] = MMC328X_REG_CTRL;
	data[1] = MMC328X_CTRL_TM;
	if (mmc328x_i2c_tx_data(data, 2) < 0) {
	}
	msleep(5*MMC328X_DELAY_TM);
#endif

	data[0] = MMC328X_REG_CTRL;
	data[1] = MMC328X_CTRL_RM;
	if (mmc328x_i2c_tx_data(data, 2) < 0) {
		/* assume RM always success */
	}
#ifndef CONFIG_SENSORS_MMC328xMA_MAG 
	/* wait external capacitor charging done for next RM */
	msleep(MMC328X_DELAY_RM);
#else
	msleep(10*MMC328X_DELAY_RM);
	data[0] = MMC328X_REG_CTRL;
	data[1] = MMC328X_CTRL_TM;
	if (mmc328x_i2c_tx_data(data, 2) < 0) {
	}
#endif

	return 0;

out_deregister:
	misc_deregister(&mmc328x_device);
out:
	return res;
}

static int mmc328x_remove(struct i2c_client *client)
{
	device_remove_file(&client->dev, &dev_attr_mmc328x);
	misc_deregister(&mmc328x_device);

	return 0;
}

static const struct i2c_device_id mmc328x_id[] = {
	{ MMC328X_I2C_NAME, 0 },
	{ }
};

static struct i2c_driver mmc328x_driver = {
	.probe 		= mmc328x_probe,
	.remove 	= mmc328x_remove,
	.id_table	= mmc328x_id,
	.driver 	= {
		.owner	= THIS_MODULE,
		.name	= MMC328X_I2C_NAME,
	},
};

//****************************add by rambo to create i2c client 2013-4-18
static struct i2c_board_info mmc328x_board_info =
{
	.type	=	MMC328X_I2C_NAME,
	.addr	=	MMC328X_I2C_ADDR,	
};

//******************************************

static int __init mmc328x_init(void)
{
	int ret;
	pr_info("mmc328x driver: init\n");
	
	ret = get_axisset(); 
	struct i2c_adapter *adapter;
	adapter = i2c_get_adapter(0);
	if (!adapter)
	{
		printk("<<<<<%s i2c get adapter fail!\n", __FUNCTION__);
		return -1;
	}
	g_client = i2c_new_device(adapter, &mmc328x_board_info);
	if (!g_client)
	{
		printk("<<<<%s i2c new device fail!\n", __FUNCTION__);
		i2c_put_adapter(adapter);
		return -1;
	}
	i2c_put_adapter(adapter);	
	
	return i2c_add_driver(&mmc328x_driver);
}

static void __exit mmc328x_exit(void)
{
	if (g_client != NULL)
	{
		i2c_unregister_device(g_client);
	}
	pr_info("mmc328x driver: exit\n");
	i2c_del_driver(&mmc328x_driver);
}

module_init(mmc328x_init);
module_exit(mmc328x_exit);

MODULE_DESCRIPTION("MEMSIC MMC328X Magnetic Sensor Driver");
MODULE_LICENSE("GPL");

