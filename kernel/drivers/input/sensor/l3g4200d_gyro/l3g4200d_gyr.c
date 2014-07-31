/******************** (C) COPYRIGHT 2010 STMicroelectronics ********************
*
* File Name     : l3g4200d_gyr_sysfs.c
* Authors       : MH - C&I BU - Application Team
*               : Carmine Iascone (carmine.iascone@st.com)
*               : Matteo Dameno (matteo.dameno@st.com)
*               : Both authors are willing to be considered the contact
*               : and update points for the driver.
* Version       : V 1.1 sysfs
* Date          : 2011/02/28
* Description   : L3G4200D digital output gyroscope sensor API
*
********************************************************************************
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 as
* published by the Free Software Foundation.
*
* THE PRESENT SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES
* OR CONDITIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED, FOR THE SOLE
* PURPOSE TO SUPPORT YOUR APPLICATION DEVELOPMENT.
* AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY DIRECT,
* INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING FROM THE
* CONTENT OF SUCH SOFTWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING
* INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
*
********************************************************************************
* REVISON HISTORY
*
* VERSION	| DATE		| AUTHORS		| DESCRIPTION
* 1.0		| 2010/11/19	| Carmine Iascone | First Release
* 1.1		| 2011/02/28	| Matteo Dameno   | Self Test Added
* 1.2		| 2013/04/29	| Howay Huo       | Android Interface Added
*******************************************************************************/
#include <linux/module.h>
#include <linux/init.h>
#include <linux/i2c.h>
#include <linux/mutex.h>
#include <linux/input-polldev.h>
#include <linux/slab.h>
#include <linux/workqueue.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#include <mach/hardware.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/gpio.h>
#include <mach/wmt_iomux.h>

#include "l3g4200d.h"

#undef DEBUG
#define DEBUG 1

#define L3G4200D_I2C_ADDR        0x69

/* WM8880 interrupt pin connect to the DRDY Pin of LL3G4200D */
#define L3G4200D_IRQ_PIN	 WMT_PIN_GP2_GPIO17

/** Maximum polled-device-reported rot speed value value in dps*/
#define FS_MAX			32768

/* l3g4200d gyroscope registers */
#define WHO_AM_I              0x0F

#define CTRL_REG1             0x20
#define CTRL_REG2             0x21
#define CTRL_REG3             0x22
#define CTRL_REG4             0x23
#define CTRL_REG5             0x24
#define REF_DATACAP           0x25
#define OUT_TEMP              0x26
#define STATUS_REG            0x27
#define OUT_X_L               0x28
#define OUT_X_H               0x29
#define OUT_Y_L               0x2A
#define OUT_Y_H               0x2B
#define OUT_Z_L               0x2C
#define OUT_Z_H               0x2D
#define FIFO_CTRL_REG         0x2E
#define FIFO_SRC_REG          0x2F
#define INT1_CFG              0x30
#define INT1_SRC              0x31
#define INT1_THS_XH           0x32
#define INT1_THS_XL           0x33
#define INT1_THS_YH           0x34
#define INT1_THS_YL           0x35
#define INT1_THS_ZH           0x36
#define INT1_THS_ZL           0x37
#define INT1_DURATION         0x38

/* CTRL_REG1 */
#define PM_MASK     0x08
#define PM_NORMAL	0x08
#define ENABLE_ALL_AXES	   0x07

/* CTRL_REG3 */
#define I2_DRDY     0x08

/* CTRL_REG4 bits */
#define	FS_MASK				0x30

#define	SELFTEST_MASK			0x06
#define L3G4200D_SELFTEST_DIS		0x00
#define L3G4200D_SELFTEST_EN_POS	0x02
#define L3G4200D_SELFTEST_EN_NEG	0x04

#define FUZZ			0
#define FLAT			0
#define AUTO_INCREMENT		0x80

/* STATUS_REG */
#define ZYXDA               0x08
#define ZDA                 0x04
#define YDA                 0x02
#define XDA                 0x01

/** Registers Contents */
#define WHOAMI_L3G4200D		0x00D3	/* Expected content for WAI register*/

#define ODR_MASK           0xF0
#define ODR100_BW25        0x10
#define ODR200_BW70        0x70
#define ODR400_BW110       0xB0
#define ODR800_BW110       0xF0

#define L3G4200D_PU_DELAY               320   //ms

static struct i2c_client *l3g4200d_i2c_client = NULL;
static int enable_print_log;
static int interrupt_mode = 0;
static int flush_polling_data;

/*
 * L3G4200D gyroscope data
 * brief structure containing gyroscope values for yaw, pitch and roll in
 * signed short
 */

struct output_rate{
	u8 odr_mask;
	u32 delay_us;
};

static const struct output_rate gyro_odr_table[] = {
	{ ODR800_BW110, 1250 },
	{ ODR400_BW110, 2500 },
	{ ODR200_BW70,  5000 },
	{ ODR100_BW25,  10000},
};

struct l3g4200d_data {
	struct i2c_client *client;
	struct l3g4200d_gyr_platform_data *pdata;

	struct mutex lock;

	struct delayed_work enable_work;
	struct delayed_work input_work;
	struct input_dev *input_dev;
	int hw_initialized;
	int selftest_enabled;
	atomic_t enabled;

	u8 reg_addr;
	struct reg_value_t resume_state;
};

extern int wmt_getsyspara(char *varname, char *varval, int *varlen);

static int l3g4200d_i2c_read(struct l3g4200d_data *gyro,
				  u8 *buf, int len)
{
	int err;

	struct i2c_msg msgs[] = {
		{
		 .addr = gyro->client->addr,
		 .flags = gyro->client->flags & I2C_M_TEN,
		 .len = 1,
		 .buf = buf,
		 },
		{
		 .addr = gyro->client->addr,
		 .flags = (gyro->client->flags & I2C_M_TEN) | I2C_M_RD,
		 .len = len,
		 .buf = buf,
		 },
	};

	err = i2c_transfer(gyro->client->adapter, msgs, 2);

	if (err != 2) {
		dev_err(&gyro->client->dev, "read transfer error: %d\n",err);
		return -EIO;
	}

	return 0;
}

static int l3g4200d_i2c_write(struct l3g4200d_data *gyro,
						u8 *buf,
						int len)
{
	int err;

	struct i2c_msg msgs[] = {
		{
		 .addr = gyro->client->addr,
		 .flags = gyro->client->flags & I2C_M_TEN,
		 .len = len + 1,
		 .buf = buf,
		 },
	};

	err = i2c_transfer(gyro->client->adapter, msgs, 1);

	if (err != 1) {
		dev_err(&gyro->client->dev, "write transfer error\n");
		return -EIO;
	}

	return 0;
}

static int l3g4200d_register_write(struct l3g4200d_data *gyro, u8 *buf,
		u8 reg_address, u8 new_value)
{
	int err = -1;

		/* Sets configuration register at reg_address
		 *  NOTE: this is a straight overwrite  */
		buf[0] = reg_address;
		buf[1] = new_value;
		err = l3g4200d_i2c_write(gyro, buf, 1);
		if (err < 0)
			return err;

	return err;
}

static int l3g4200d_register_read(struct l3g4200d_data *gyro, u8 *buf,
		u8 reg_address)
{

	int err = -1;
	buf[0] = (reg_address);
	err = l3g4200d_i2c_read(gyro, buf, 1);
	return err;
}

static int l3g4200d_register_update(struct l3g4200d_data *gyro, u8 *buf,
		u8 reg_address, u8 mask, u8 new_bit_values)
{
	int err = -1;
	u8 init_val;
	u8 updated_val;
	err = l3g4200d_register_read(gyro, buf, reg_address);
	if (!(err < 0)) {
		init_val = buf[0];
		if((new_bit_values & mask) != (init_val & mask)) {
			updated_val = ((mask & new_bit_values) | ((~mask) & init_val));
			err = l3g4200d_register_write(gyro, buf, reg_address,
				updated_val);
		}
		else
			return 0;
	}
	return err;
}

static int l3g4200d_register_store(struct l3g4200d_data *gyro, u8 reg)
{
	int err = -1;
	u8 buf[2];

	err = l3g4200d_register_read(gyro, buf, reg);
	if (err)
		return err;

	switch (reg) {
		case CTRL_REG1:
			gyro->resume_state.ctrl_reg1 = buf[0];
		break;
		case CTRL_REG2:
			gyro->resume_state.ctrl_reg2 = buf[0];
		break;
		case CTRL_REG3:
			gyro->resume_state.ctrl_reg3 = buf[0];
		break;
		case CTRL_REG4:
			gyro->resume_state.ctrl_reg4 = buf[0];
		break;
		case CTRL_REG5:
			gyro->resume_state.ctrl_reg5 = buf[0];
		break;
		case REF_DATACAP:
			gyro->resume_state.ref_datacap = buf[0];
		break;
		case FIFO_CTRL_REG:
			gyro->resume_state.fifo_ctrl_reg = buf[0];
		break;
		case INT1_CFG:
			gyro->resume_state.int1_cfg = buf[0];
		break;
		case INT1_THS_XH:
			gyro->resume_state.int1_ths_xh = buf[0];
		break;
		case INT1_THS_XL:
			gyro->resume_state.int1_ths_xl = buf[0];
		break;
		case INT1_THS_YH:
			gyro->resume_state.int1_ths_yh = buf[0];
		break;
		case INT1_THS_YL:
			gyro->resume_state.int1_ths_yl = buf[0];
		break;
		case INT1_THS_ZH:
			gyro->resume_state.int1_ths_zh = buf[0];
		break;
		case INT1_THS_ZL:
			gyro->resume_state.int1_ths_zl = buf[0];
		break;
		case INT1_DURATION:
			gyro->resume_state.int1_duration = buf[0];
		break;
		default:
			pr_err("%s: can't support reg (0x%02X) store\n", L3G4200D_GYR_DEV_NAME, reg);
		return -1;
	}

	return 0;
}

static int l3g4200d_update_fs_range(struct l3g4200d_data *gyro,
							u8 new_fs)
{
	int res ;
	u8 buf[2];

	buf[0] = CTRL_REG4;

	res = l3g4200d_register_update(gyro, buf, CTRL_REG4,
							FS_MASK, new_fs);

	if (res < 0) {
		pr_err("%s : failed to update fs:0x%02x\n",
			__func__, new_fs);
		return res;
	}

	l3g4200d_register_store(gyro, CTRL_REG4);

	return 0;
}


static int l3g4200d_selftest(struct l3g4200d_data *gyro, u8 enable)
{
	int err = -1;
	u8 buf[2] = {0x00,0x00};
	char reg_address, mask, bit_values;

	reg_address = CTRL_REG4;
	mask = SELFTEST_MASK;
	if (enable > 0)
		bit_values = L3G4200D_SELFTEST_EN_POS;
	else
		bit_values = L3G4200D_SELFTEST_DIS;
	if (atomic_read(&gyro->enabled)) {
		mutex_lock(&gyro->lock);
		err = l3g4200d_register_update(gyro, buf, reg_address,
				mask, bit_values);
		gyro->selftest_enabled = enable;
		mutex_unlock(&gyro->lock);
		if (err < 0)
			return err;

		l3g4200d_register_store(gyro, CTRL_REG4);
	}
	return err;
}


static int l3g4200d_update_odr(struct l3g4200d_data *gyro,
				int poll_interval)
{
	int err = -1;
	int i;
	u8 config[2];

	for (i = ARRAY_SIZE(gyro_odr_table) - 1; i >= 0; i--) {
		if (gyro_odr_table[i].delay_us <= poll_interval){
			break;
		}
	}

	gyro->pdata->poll_interval = gyro_odr_table[i].delay_us;

	config[1] = gyro_odr_table[i].odr_mask;
	config[1] |= (ENABLE_ALL_AXES + PM_NORMAL);

	/* If device is currently enabled, we need to write new
	 *  configuration out to it */
	if (atomic_read(&gyro->enabled)) {
		config[0] = CTRL_REG1;
		err = l3g4200d_i2c_write(gyro, config, 1);
		if (err < 0)
			return err;

		l3g4200d_register_store(gyro, CTRL_REG1);
	}

	return err;
}

/* gyroscope data readout */
static int l3g4200d_get_data(struct l3g4200d_data *gyro,
			     struct l3g4200d_triple *data)
{
	int err;
	unsigned char gyro_out[6];
	/* y,p,r hardware data */
	s16 hw_d[3] = { 0 };

	gyro_out[0] = (AUTO_INCREMENT | OUT_X_L);

	err = l3g4200d_i2c_read(gyro, gyro_out, 6);

	if (err < 0)
		return err;

	hw_d[0] = (s16) (((gyro_out[1]) << 8) | gyro_out[0]);
	hw_d[1] = (s16) (((gyro_out[3]) << 8) | gyro_out[2]);
	hw_d[2] = (s16) (((gyro_out[5]) << 8) | gyro_out[4]);

	data->x = ((gyro->pdata->direction_x < 0) ? (-hw_d[gyro->pdata->axis_map_x])
		   : (hw_d[gyro->pdata->axis_map_x]));
	data->y = ((gyro->pdata->direction_y < 0) ? (-hw_d[gyro->pdata->axis_map_y])
		   : (hw_d[gyro->pdata->axis_map_y]));
	data->z = ((gyro->pdata->direction_z < 0) ? (-hw_d[gyro->pdata->axis_map_z])
		   : (hw_d[gyro->pdata->axis_map_z]));

	if(enable_print_log)
		printk("x = %d, y = %d, z = %d\n", data->x, data->y, data->z);

	return err;
}

static void l3g4200d_report_values(struct l3g4200d_data *l3g,
						struct l3g4200d_triple *data)
{
	struct input_dev *input = l3g->input_dev;

	input_report_abs(input, ABS_X, data->x);
	input_report_abs(input, ABS_Y, data->y);
	input_report_abs(input, ABS_Z, data->z);
	input_sync(input);
}

static int l3g4200d_hw_init(struct l3g4200d_data *gyro)
{
	int err = -1;
	u8 buf[8];

	printk(KERN_INFO "%s hw init\n", L3G4200D_GYR_DEV_NAME);

	buf[0] = (AUTO_INCREMENT | CTRL_REG1);
	buf[1] = gyro->resume_state.ctrl_reg1;
	buf[2] = gyro->resume_state.ctrl_reg2;
	buf[3] = gyro->resume_state.ctrl_reg3;
	buf[4] = gyro->resume_state.ctrl_reg4;
	buf[5] = gyro->resume_state.ctrl_reg5;
	buf[6] = gyro->resume_state.ref_datacap;
	err = l3g4200d_i2c_write(gyro, buf, 6);
	if(err)
		return err;

	buf[0] = FIFO_CTRL_REG;
	buf[1] = gyro->resume_state.fifo_ctrl_reg;
	err = l3g4200d_i2c_write(gyro, buf, 1);
	if(err)
		return err;

	buf[0] = INT1_CFG;
	buf[1] = gyro->resume_state.int1_cfg;
	err = l3g4200d_i2c_write(gyro, buf, 1);
	if(err)
		return err;

	buf[0] = (AUTO_INCREMENT | INT1_THS_XH);
	buf[1] = gyro->resume_state.int1_ths_xh;
	buf[2] = gyro->resume_state.int1_ths_xl;
	buf[3] = gyro->resume_state.int1_ths_yh;
	buf[4] = gyro->resume_state.int1_ths_yl;
	buf[5] = gyro->resume_state.int1_ths_zh;
	buf[6] = gyro->resume_state.int1_ths_zl;
	buf[7] = gyro->resume_state.int1_duration;
	err = l3g4200d_i2c_write(gyro, buf, 7);
	if(err)
		return err;

	gyro->hw_initialized = 1;

	return 0;
}

static void l3g4200d_gpio_irq_enable(void)
{
	wmt_gpio_unmask_irq(L3G4200D_IRQ_PIN);
}

static void l3g4200d_gpio_irq_disable(void)
{
	wmt_gpio_mask_irq(L3G4200D_IRQ_PIN);
}

static int l3g4200d_irq_hw_init(int resume)
{
	int ret;

	if(!resume) {
		ret = gpio_request(L3G4200D_IRQ_PIN, "l3g4200d-gyro irq"); //enable gpio
		if(ret < 0) {
			pr_err("gpio(%d) request fail for l3g4200d-gyro irq\n", L3G4200D_IRQ_PIN);
			return ret;
		}
	}else
		gpio_re_enabled(L3G4200D_IRQ_PIN); //re-enable gpio

	gpio_direction_input(L3G4200D_IRQ_PIN); //gpio input

	wmt_gpio_setpull(L3G4200D_IRQ_PIN, WMT_GPIO_PULL_DOWN); //enable pull and pull-down

	wmt_gpio_mask_irq(L3G4200D_IRQ_PIN);       //disable interrupt

	wmt_gpio_set_irq_type(L3G4200D_IRQ_PIN, IRQ_TYPE_EDGE_RISING); //rise edge and clear interrupt

	return 0;
}

static void l3g4200d_irq_hw_free(void)
{
	l3g4200d_gpio_irq_disable();
	gpio_free(L3G4200D_IRQ_PIN);

}

static int l3g4200d_gpio_get_value(void)
{
	return (REG8_VAL(__GPIO_BASE + 0x0002) & BIT1);
}

static int l3g4200d_flush_gyro_data(struct l3g4200d_data *gyro)
{
	struct l3g4200d_triple data;
	int i;

	for (i = 0; i < 5; i++) {
		if (l3g4200d_gpio_get_value()) {
			l3g4200d_get_data(gyro, &data);
			pr_info("%s: flush_gyro_data: %d\n", L3G4200D_GYR_DEV_NAME, i);
		}
		else {
			return 0;
		}
	}

	return -EIO;
}

static void l3g4200d_device_power_off(struct l3g4200d_data *gyro)
{
	int err;
	u8 buf[2];

	pr_info("%s power off\n", L3G4200D_GYR_DEV_NAME);

	buf[0] = CTRL_REG1;
	err = l3g4200d_i2c_read(gyro, buf, 1);
	if(err < 0) {
		dev_err(&gyro->client->dev, "read ctrl_reg1 failed\n");
		buf[0] = (gyro->resume_state.ctrl_reg1 | PM_MASK);
	}

	if(buf[0] & PM_MASK) {
		buf[1] = buf[0] & ~PM_MASK;
		buf[0] = CTRL_REG1;

		err = l3g4200d_i2c_write(gyro, buf, 1);
		if (err)
			dev_err(&gyro->client->dev, "soft power off failed\n");

		l3g4200d_register_store(gyro, CTRL_REG1);
	}

	if (gyro->pdata->power_off) {
		gyro->pdata->power_off();
//		gyro->hw_initialized = 0;
	}

//	if (gyro->hw_initialized)
//		gyro->hw_initialized = 0;

}

static int l3g4200d_device_power_on(struct l3g4200d_data *gyro)
{
	int err;
	u8 buf[2];

	pr_info("%s power on\n", L3G4200D_GYR_DEV_NAME);

	if (gyro->pdata->power_on) {
		err = gyro->pdata->power_on();
		if (err < 0)
			return err;
	}

	if (!gyro->hw_initialized) {
		err = l3g4200d_hw_init(gyro);
		if (err) {
			l3g4200d_device_power_off(gyro);
			return err;
		}
	}

	buf[0] = CTRL_REG1;
	err = l3g4200d_i2c_read(gyro, buf, 1);
	if(err) {
		dev_err(&gyro->client->dev, "read ctrl_reg1 failed\n");
		buf[0] = (gyro->resume_state.ctrl_reg1 & ~PM_MASK);
	}

	if(!(buf[0] & PM_MASK)) {
		buf[1] = buf[0] | PM_MASK;
		buf[0] = CTRL_REG1;
		err = l3g4200d_i2c_write(gyro, buf, 1);
		if(err) {
			dev_err(&gyro->client->dev, "soft power on failed\n");
			return err;
		}
		l3g4200d_register_store(gyro, CTRL_REG1);
	}

	return 0;
}

static int l3g4200d_enable(struct l3g4200d_data *gyro)
{
	int err;

	pr_info("%s enable\n", L3G4200D_GYR_DEV_NAME);

	if (!atomic_cmpxchg(&gyro->enabled, 0, 1)) {
		err = l3g4200d_device_power_on(gyro);
		if (err) {
			atomic_set(&gyro->enabled, 0);
			return err;
		}

		//Android will call l3g4200d_set_delay() after l3g4200d_enable;
	}

	return 0;
}

static int l3g4200d_disable(struct l3g4200d_data *gyro)
{
	pr_info("%s disable\n", L3G4200D_GYR_DEV_NAME);

	if (atomic_cmpxchg(&gyro->enabled, 1, 0)){
		if(interrupt_mode) {
			cancel_delayed_work_sync(&gyro->enable_work);
			l3g4200d_gpio_irq_disable();
		}else
			cancel_delayed_work_sync(&gyro->input_work);

		l3g4200d_device_power_off(gyro);
	}
	return 0;
}

static int l3g4200d_set_delay(struct l3g4200d_data *gyro, u32 delay_us)
{
	int odr_value = ODR100_BW25;
	int err = -1;
	int i;
	u8 buf[2] = {CTRL_REG1, 0};

	pr_info("l3g4200d_set_delay: %d us\n", delay_us);

	/* do not report noise during ODR update */
	if(interrupt_mode) {
		cancel_delayed_work_sync(&gyro->enable_work);
		l3g4200d_gpio_irq_disable();
	}else
		cancel_delayed_work_sync(&gyro->input_work);

	for(i=0; i < ARRAY_SIZE(gyro_odr_table); i++)
		if(delay_us <= gyro_odr_table[i].delay_us) {
			odr_value = gyro_odr_table[i].odr_mask;
			delay_us = gyro_odr_table[i].delay_us;
			break;
		}

	if(delay_us >= gyro_odr_table[3].delay_us) {
		odr_value = gyro_odr_table[3].odr_mask;
		delay_us = gyro_odr_table[3].delay_us;
	}

	err = l3g4200d_register_update(gyro, buf, CTRL_REG1, ODR_MASK, odr_value);
	if(err) {
		dev_err(&gyro->client->dev, "update odr failed 0x%x,0x%x: %d\n",
			buf[0], odr_value, err);
		return err;
	}

	l3g4200d_register_store(gyro, CTRL_REG1);

	gyro->pdata->poll_interval = max((int)delay_us, gyro->pdata->min_interval);

	//do not report noise at IC power-up
	// flush data before really read
	if(interrupt_mode)
		schedule_delayed_work(&gyro->enable_work, msecs_to_jiffies(
			L3G4200D_PU_DELAY));
	else {
		flush_polling_data = 1;
		schedule_delayed_work(&gyro->input_work, msecs_to_jiffies(
			L3G4200D_PU_DELAY));
	}

	return 0;
}

static ssize_t attr_polling_rate_show(struct device *dev,
				     struct device_attribute *attr,
				     char *buf)
{
	int val;
	struct l3g4200d_data *gyro = dev_get_drvdata(dev);
	mutex_lock(&gyro->lock);
	val = gyro->pdata->poll_interval;
	mutex_unlock(&gyro->lock);
	return sprintf(buf, "%d\n", val);
}

static ssize_t attr_polling_rate_store(struct device *dev,
				     struct device_attribute *attr,
				     const char *buf, size_t size)
{
	struct l3g4200d_data *gyro = dev_get_drvdata(dev);
	unsigned long interval_us;

	if (strict_strtoul(buf, 10, &interval_us))
		return -EINVAL;
	if (!interval_us)
		return -EINVAL;
	mutex_lock(&gyro->lock);
	gyro->pdata->poll_interval = interval_us;
	l3g4200d_update_odr(gyro, interval_us);
	mutex_unlock(&gyro->lock);
	return size;
}

static ssize_t attr_range_show(struct device *dev,
			       struct device_attribute *attr, char *buf)
{
	struct l3g4200d_data *gyro = dev_get_drvdata(dev);
	int range = 0;
	char val;
	mutex_lock(&gyro->lock);
	val = gyro->pdata->fs_range;
	switch (val) {
	case L3G4200D_GYR_FS_250DPS:
		range = 250;
		break;
	case L3G4200D_GYR_FS_500DPS:
		range = 500;
		break;
	case L3G4200D_GYR_FS_2000DPS:
		range = 2000;
		break;
	}
	mutex_unlock(&gyro->lock);
	return sprintf(buf, "%d\n", range);
}

static ssize_t attr_range_store(struct device *dev,
			      struct device_attribute *attr,
			      const char *buf, size_t size)
{
	struct l3g4200d_data *gyro = dev_get_drvdata(dev);
	unsigned long val;
	if (strict_strtoul(buf, 10, &val))
		return -EINVAL;
	mutex_lock(&gyro->lock);
	gyro->pdata->fs_range = val;
	l3g4200d_update_fs_range(gyro, val);
	mutex_unlock(&gyro->lock);
	return size;
}

static ssize_t attr_enable_show(struct device *dev,
			       struct device_attribute *attr, char *buf)
{
	struct l3g4200d_data *gyro = dev_get_drvdata(dev);
	int val = atomic_read(&gyro->enabled);
	return sprintf(buf, "%d\n", val);
}

static ssize_t attr_enable_store(struct device *dev,
			       struct device_attribute *attr,
			       const char *buf, size_t size)
{
	struct l3g4200d_data *gyro = dev_get_drvdata(dev);
	unsigned long val;

	if (strict_strtoul(buf, 10, &val))
		return -EINVAL;

	if (val) {
		l3g4200d_enable(gyro);
		l3g4200d_set_delay(gyro, gyro->pdata->poll_interval);
	}
	else
		l3g4200d_disable(gyro);

	return size;
}

static ssize_t attr_get_selftest(struct device *dev,
			       struct device_attribute *attr, char *buf)
{
	int val;
	struct l3g4200d_data *gyro = dev_get_drvdata(dev);
	mutex_lock(&gyro->lock);
	val = gyro->selftest_enabled;
	mutex_unlock(&gyro->lock);
	return sprintf(buf, "%d\n", val);
}

static ssize_t attr_set_selftest(struct device *dev,
			       struct device_attribute *attr,
			       const char *buf, size_t size)
{
	struct l3g4200d_data *gyro = dev_get_drvdata(dev);
	unsigned long val;

	if (strict_strtoul(buf, 10, &val))
		return -EINVAL;

	l3g4200d_selftest(gyro, val);

	return size;
}

#ifdef DEBUG
static ssize_t attr_reg_set(struct device *dev, struct device_attribute *attr,
				const char *buf, size_t size)
{
	int rc;
	struct l3g4200d_data *gyro = dev_get_drvdata(dev);
	u8 x[2];
	unsigned long val;

	if (strict_strtoul(buf, 16, &val))
		return -EINVAL;
	mutex_lock(&gyro->lock);
	x[0] = gyro->reg_addr;
	mutex_unlock(&gyro->lock);
	x[1] = val;
	rc = l3g4200d_i2c_write(gyro, x, 1);
	return size;
}

static ssize_t attr_reg_get(struct device *dev, struct device_attribute *attr,
				char *buf)
{
	ssize_t ret;
	struct l3g4200d_data *gyro = dev_get_drvdata(dev);
	int rc;
	u8 data;

	mutex_lock(&gyro->lock);
	data = gyro->reg_addr;
	mutex_unlock(&gyro->lock);
	rc = l3g4200d_i2c_read(gyro, &data, 1);
	ret = sprintf(buf, "0x%02x\n", data);
	return ret;
}

static ssize_t attr_addr_set(struct device *dev, struct device_attribute *attr,
				const char *buf, size_t size)
{
	struct l3g4200d_data *gyro = dev_get_drvdata(dev);
	unsigned long val;

	if (strict_strtoul(buf, 16, &val))
		return -EINVAL;

	mutex_lock(&gyro->lock);

	gyro->reg_addr = val;

	mutex_unlock(&gyro->lock);

	return size;
}

static ssize_t attr_get_printlog(struct device * dev,struct device_attribute * attr,
				char * buf)
{
	ssize_t ret;

	ret = sprintf(buf, "%d\n", enable_print_log);

	return ret;
}

static ssize_t attr_set_printlog(struct device *dev, struct device_attribute *attr,
				const char *buf, size_t size)
{
	unsigned long val;

	if (strict_strtoul(buf, 10, &val))
		return -EINVAL;

	enable_print_log = val;

	return size;
}

#endif /* DEBUG */

static struct device_attribute attributes[] = {
	__ATTR(pollrate_ms, 0666, attr_polling_rate_show,
						attr_polling_rate_store),
	__ATTR(range, 0666, attr_range_show, attr_range_store),
	__ATTR(enable_device, 0666, attr_enable_show, attr_enable_store),
	__ATTR(enable_selftest, 0666, attr_get_selftest, attr_set_selftest),
#ifdef DEBUG
	__ATTR(reg_value, 0600, attr_reg_get, attr_reg_set),
	__ATTR(reg_addr, 0200, NULL, attr_addr_set),
	__ATTR(printlog, 0600, attr_get_printlog, attr_set_printlog),
#endif
};

static int create_sysfs_interfaces(struct device *dev)
{
	int i;
	for (i = 0; i < ARRAY_SIZE(attributes); i++)
		if (device_create_file(dev, attributes + i))
			goto error;
	return 0;

error:
	for ( ; i >= 0; i--)
		device_remove_file(dev, attributes + i);
	dev_err(dev, "%s:Unable to create interface\n", __func__);
	return -1;
}

static int remove_sysfs_interfaces(struct device *dev)
{
	int i;
	for (i = 0; i < ARRAY_SIZE(attributes); i++)
		device_remove_file(dev, attributes + i);
	return 0;
}

static void l3g4200d_data_report(struct l3g4200d_data *gyro)
{
	struct l3g4200d_triple data_out;
	int err;

	err = l3g4200d_get_data(gyro, &data_out);
	if (err)
		dev_err(&gyro->client->dev, "get_gyroscope_data failed\n");
	else
		l3g4200d_report_values(gyro, &data_out);

}

static int l3g4200d_validate_pdata(struct l3g4200d_data *gyro)
{
	gyro->pdata->poll_interval = max(gyro->pdata->poll_interval,
			gyro->pdata->min_interval);

	if (gyro->pdata->axis_map_x > 2 ||
	    gyro->pdata->axis_map_y > 2 ||
	    gyro->pdata->axis_map_z > 2) {
		dev_err(&gyro->client->dev,
			"invalid axis_map value x:%u y:%u z%u\n",
			gyro->pdata->axis_map_x,
			gyro->pdata->axis_map_y,
			gyro->pdata->axis_map_z);
		return -EINVAL;
	}

	/* Only allow 1 and -1 for direction flag */
	if (abs(gyro->pdata->direction_x) != 1 ||
	    abs(gyro->pdata->direction_y) != 1 ||
	    abs(gyro->pdata->direction_z) != 1) {
		dev_err(&gyro->client->dev,
			"invalid direction value x:%d y:%d z:%d\n",
			gyro->pdata->direction_x,
			gyro->pdata->direction_y,
			gyro->pdata->direction_z);
		return -EINVAL;
	}

	/* Enforce minimum polling interval */
	if (gyro->pdata->poll_interval < gyro->pdata->min_interval) {
		dev_err(&gyro->client->dev,
			"minimum poll interval violated\n");
		return -EINVAL;
	}
	return 0;
}

static int l3g4200d_misc_open(struct inode *inode, struct file *file)
{
	int err;

	err = nonseekable_open(inode, file);
	if(err < 0)
		return err;

	file->private_data = i2c_get_clientdata(l3g4200d_i2c_client);

	return 0;
}

static int l3g4200d_misc_close(struct inode *inode, struct file *filp)
{
	return 0;
}


static long l3g4200d_misc_ioctl(struct file *file,
								unsigned int cmd, unsigned long arg)
{
	void __user *argp = (void __user *)arg;
	int interval, val;
	int err = -1;
	struct l3g4200d_data *gyro = file->private_data;

	switch(cmd){
		case GYRO_IOCTL_GET_DELAY:
			interval = gyro->pdata->poll_interval;
			if(copy_to_user(argp, &interval, sizeof(interval)))
				return -EFAULT;
		break;

		case GYRO_IOCTL_SET_DELAY:
			if(copy_from_user(&interval, argp, sizeof(interval)))
				return -EFAULT;
			err = l3g4200d_set_delay(gyro, interval);
			if(err < 0)
				return err;
		break;

		case GYRO_IOCTL_SET_ENABLE:
			if(copy_from_user(&val, argp, sizeof(val)))
				return -EFAULT;
			if(val > 1)
				return -EINVAL;

			if(val)
				l3g4200d_enable(gyro);
			else
				l3g4200d_disable(gyro);
		break;

		case GYRO_IOCTL_GET_ENABLE:
			val = atomic_read(&gyro->enabled);
			if(copy_to_user(argp, &val, sizeof(val)))
				return -EINVAL;
		break;

		case WMT_IOCTL_SENSOR_GET_DRVID:
			val = L3G4200D_DRVID;
			if (copy_to_user(argp, &val, sizeof(val)))
				return -EFAULT;
		break;

		default:
			return -EINVAL;
	}

	return 0;
}

static const struct file_operations l3g4200d_misc_fops = {
	.owner = THIS_MODULE,
	.open = l3g4200d_misc_open,
	.unlocked_ioctl = l3g4200d_misc_ioctl,
	.release = l3g4200d_misc_close,
};

static struct miscdevice l3g4200d_misc_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = GYRO_MISCDEV_NAME,
	.fops = &l3g4200d_misc_fops,
};

static int l3g4200d_input_init(struct l3g4200d_data *gyro)
{
	int err = -1;
	struct input_dev *input;

	gyro->input_dev = input_allocate_device();
	if (!gyro->input_dev) {
		err = -ENOMEM;
		dev_err(&gyro->client->dev,
			"input device allocate failed\n");
		goto err0;
	}

	input = gyro->input_dev;

	input_set_drvdata(input, gyro);

	set_bit(EV_ABS, input->evbit);

	input_set_abs_params(input, ABS_X, -FS_MAX, FS_MAX, FUZZ, FLAT);
	input_set_abs_params(input, ABS_Y, -FS_MAX, FS_MAX, FUZZ, FLAT);
	input_set_abs_params(input, ABS_Z, -FS_MAX, FS_MAX, FUZZ, FLAT);

	input->name = GYRO_INPUT_NAME;

	err = input_register_device(input);
	if (err) {
		dev_err(&gyro->client->dev,
			"unable to register input polled device %s\n",
			input->name);
		goto err1;
	}

	return 0;

err1:
	input_free_device(input);
err0:
	return err;
}

static void l3g4200d_input_cleanup(struct l3g4200d_data *gyro)
{
	input_unregister_device(gyro->input_dev);
	input_free_device(gyro->input_dev);
}

static int l3g4300d_detect(struct i2c_client *client)
{
	int ret;
	u8 buf[1];
	struct l3g4200d_data gyro;

	gyro.client = client;

	ret = l3g4200d_register_read(&gyro, buf, WHO_AM_I);
	if(ret)
		return 0;

	if(buf[0] != WHOAMI_L3G4200D){
		dev_err(&client->dev, "chipId = 0x%02X, error", buf[0]);
		return 0;
	}

	return 1;
}

static irqreturn_t gyro_irq_handler(int irq, void *dev_id)
{
	if(!gpio_irqstatus(L3G4200D_IRQ_PIN))
		return IRQ_NONE;

	wmt_gpio_ack_irq(L3G4200D_IRQ_PIN);  //clear interrupt

	//printk("got gyro interrupt\n");
	if(!is_gpio_irqenable(L3G4200D_IRQ_PIN)) {
		//pr_err("l3g4200d irq is disabled\n");
		return IRQ_HANDLED;
	}else
		return IRQ_WAKE_THREAD;
}

static irqreturn_t gyro_irq_thread(int irq, void *dev)
{
	struct l3g4200d_data *gyro = dev;

	l3g4200d_data_report(gyro);

	return IRQ_HANDLED;
}

static void l3g4200d_enable_work_func(struct work_struct *work)
{
	struct delayed_work *dwork = to_delayed_work(work);
 	struct l3g4200d_data *gyro =
		container_of(dwork, struct l3g4200d_data, enable_work);

	l3g4200d_flush_gyro_data(gyro);
	l3g4200d_gpio_irq_enable();
}

static void l3g4200d_input_poll_func(struct work_struct *work)
{
	struct l3g4200d_triple data;
	struct delayed_work *dwork = to_delayed_work(work);
 	struct l3g4200d_data *gyro =
		container_of(dwork, struct l3g4200d_data, input_work);

	if(flush_polling_data == 0)
		l3g4200d_data_report(gyro);
	else {
		l3g4200d_get_data(gyro, &data); //flush the first data
		flush_polling_data = 0;
	}

	schedule_delayed_work(&gyro->input_work,
		usecs_to_jiffies(gyro->pdata->poll_interval));
}

static int l3g4200d_param_parse(int *p_int_mode, struct l3g4200d_gyr_platform_data *pdata)
{
	char varbuf[64] = {0};
	int n, ret, varlen;
	int tmp[7];

	varlen = sizeof(varbuf);

	ret = wmt_getsyspara("wmt.io.gyro.l3g4200d", varbuf, &varlen);
	if(ret)
		return 0;

	n = sscanf(varbuf, "%d:%d:%d:%d:%d:%d:%d",
		&tmp[0], &tmp[1], &tmp[2], &tmp[3], &tmp[4], &tmp[5], &tmp[6]);

	if(n != 7) {
		pr_err("l3g4200d-gyro param format error\n");
		return -1;
	}

	pdata->axis_map_x  = tmp[0];
	pdata->direction_x = tmp[1];
	pdata->axis_map_y  = tmp[2];
	pdata->direction_y = tmp[3];
	pdata->axis_map_z  = tmp[4];
	pdata->direction_z = tmp[5];
	*p_int_mode        = tmp[6];

	return 0;
}

static int l3g4200d_probe(struct i2c_client *client,
					const struct i2c_device_id *devid)
{
	struct l3g4200d_data *gyro;
	int err = -1;
	u8 buf[2];

	pr_info("%s: probe start.\n", L3G4200D_GYR_DEV_NAME);

	if (client->dev.platform_data == NULL) {
		dev_err(&client->dev, "platform data is NULL. exiting.\n");
		err = -ENODEV;
		goto err0;
	}

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		dev_err(&client->dev, "client not i2c capable:1\n");
		err = -ENODEV;
		goto err0;
	}

	if(l3g4300d_detect(client) ==  0){
		dev_err(&client->dev, "not found the gyro\n");
		err = -ENODEV;
		goto err0;
	}

	gyro = kzalloc(sizeof(*gyro), GFP_KERNEL);
	if (gyro == NULL) {
		dev_err(&client->dev,
			"failed to allocate memory for module data\n");
		err = -ENOMEM;
		goto err0;
	}

	l3g4200d_param_parse(&interrupt_mode, client->dev.platform_data);
	if(interrupt_mode) {
		pr_info("l3g4200d-gyro in interrupt mode\n");
		err = l3g4200d_irq_hw_init(0);
		if(err)
			goto err0;
	}
	else
		pr_info("l3g4200d-gyro in polling mode\n");

	mutex_init(&gyro->lock);
	mutex_lock(&gyro->lock);
	gyro->client = client;

	gyro->pdata = kmalloc(sizeof(*gyro->pdata), GFP_KERNEL);
	if (gyro->pdata == NULL) {
		dev_err(&client->dev,
			"failed to allocate memory for pdata: %d\n", err);
		goto err1;
	}
	memcpy(gyro->pdata, client->dev.platform_data,
						sizeof(*gyro->pdata));

	err = l3g4200d_validate_pdata(gyro);
	if (err < 0) {
		dev_err(&client->dev, "failed to validate platform data\n");
		goto err1_1;
	}

	i2c_set_clientdata(client, gyro);

	if (gyro->pdata->init) {
		err = gyro->pdata->init();
		if (err < 0) {
			dev_err(&client->dev, "init failed: %d\n", err);
			goto err1_1;
		}
	}

	if(interrupt_mode)
		gyro->pdata->init_state.ctrl_reg3 |= I2_DRDY;

	//According to the introduction of L3G4200D's datasheet page 32,
	//the bit7 and bit6 of CTRL_REG2's value is loaded at boot. This value must not be changed
	buf[0] = CTRL_REG2;
	err = l3g4200d_i2c_read(gyro, buf, 1);
	if(err)
		goto err1_1;

	gyro->pdata->init_state.ctrl_reg2 = (gyro->pdata->init_state.ctrl_reg2 & 0x1F)
		| (buf[0] & 0xC0);

	gyro->pdata->init_state.ctrl_reg1 &= ~PM_MASK;

	memcpy(&gyro->resume_state, &gyro->pdata->init_state,
		sizeof(struct reg_value_t));

	err = l3g4200d_hw_init(gyro);
	if (err) {
		dev_err(&client->dev, "hardware init failed: %d\n", err);
		goto err2;
	}

	err = l3g4200d_input_init(gyro);
	if (err < 0)
		goto err3;

	err = create_sysfs_interfaces(&client->dev);
	if (err < 0) {
		dev_err(&client->dev,
			"%s device register failed\n", L3G4200D_GYR_DEV_NAME);
		goto err4;
	}

	/* As default, do not report information */
	atomic_set(&gyro->enabled, 0);

	if(interrupt_mode)
		INIT_DELAYED_WORK(&gyro->enable_work, l3g4200d_enable_work_func);
	else
		INIT_DELAYED_WORK(&gyro->input_work, l3g4200d_input_poll_func);

	err = misc_register(&l3g4200d_misc_device);
	if(err){
		dev_err(&client->dev, "l3g4200d_device register failed\n");
		goto err5;
	}

	if(interrupt_mode) {
		err = request_threaded_irq(gyro->client->irq, gyro_irq_handler,
			gyro_irq_thread, IRQF_SHARED,  L3G4200D_GYR_DEV_NAME, gyro);

		if(err) {
			pr_err("%s: irq request failed: %d\n", __func__, err);
			err = -ENODEV;
			goto err6;
		}
	}

	mutex_unlock(&gyro->lock);

#ifdef DEBUG
	pr_info("%s probed: device created successfully\n",
							L3G4200D_GYR_DEV_NAME);
#endif

	return 0;

err6:
	misc_deregister(&l3g4200d_misc_device);
err5:
	remove_sysfs_interfaces(&client->dev);
err4:
	l3g4200d_input_cleanup(gyro);
err3:
	l3g4200d_device_power_off(gyro);
err2:
	if (gyro->pdata->exit)
		gyro->pdata->exit();
err1_1:
	mutex_unlock(&gyro->lock);
	kfree(gyro->pdata);
err1:
	kfree(gyro);
	if(interrupt_mode)
		l3g4200d_irq_hw_free();
err0:
	pr_err("%s: Driver Initialization failed\n",
						L3G4200D_GYR_DEV_NAME);
	return err;
}

static int l3g4200d_remove(struct i2c_client *client)
{
	struct l3g4200d_data *gyro = i2c_get_clientdata(client);
#ifdef DEBUG
	pr_info(KERN_INFO "L3G4200D driver removing\n");
#endif

	l3g4200d_disable(gyro);
	if(interrupt_mode) {
		free_irq(gyro->client->irq, gyro);
		l3g4200d_irq_hw_free();
	}
	misc_deregister(&l3g4200d_misc_device);
	l3g4200d_input_cleanup(gyro);
	remove_sysfs_interfaces(&client->dev);
	kfree(gyro->pdata);
	kfree(gyro);
	return 0;
}

static void l3g4200d_shutdown(struct i2c_client *client)
{
	struct l3g4200d_data *gyro = i2c_get_clientdata(client);

	pr_info("l3g4200d_shutdown\n");

	l3g4200d_disable(gyro);
}


static int l3g4200d_suspend(struct device *dev)
{
	struct l3g4200d_data *gyro = i2c_get_clientdata(l3g4200d_i2c_client);
	int err;
	u8 buf[8];

	pr_info(KERN_INFO "l3g4200d_suspend\n");

	if(atomic_read(&gyro->enabled)) {
		if(interrupt_mode) {
			cancel_delayed_work_sync(&gyro->enable_work);
			l3g4200d_gpio_irq_disable();
		}
		else
			cancel_delayed_work_sync(&gyro->input_work);

		//store register value
		buf[0] = (AUTO_INCREMENT | CTRL_REG1);
		err = l3g4200d_i2c_read(gyro, buf, 6);
		if(err)
			goto err1;
		gyro->resume_state.ctrl_reg1   = buf[0];
		gyro->resume_state.ctrl_reg2   = buf[1];
		gyro->resume_state.ctrl_reg3   = buf[2];
		gyro->resume_state.ctrl_reg4   = buf[3];
		gyro->resume_state.ctrl_reg5   = buf[4];
		gyro->resume_state.ref_datacap = buf[5];

		buf[0] = FIFO_CTRL_REG;
		err = l3g4200d_i2c_read(gyro, buf, 1);
		if(err)
			goto err1;
		gyro->resume_state.fifo_ctrl_reg = buf[0];

		buf[0] = INT1_CFG;
		err = l3g4200d_i2c_read(gyro, buf, 1);
		if(err)
			goto err1;
		gyro->resume_state.int1_cfg = buf[0];

		buf[0] = (AUTO_INCREMENT | INT1_THS_XH);
		err = l3g4200d_i2c_read(gyro, buf, 7);
		if(err)
			goto err1;

		gyro->resume_state.int1_ths_xh   = buf[0];
		gyro->resume_state.int1_ths_xl   = buf[1];
		gyro->resume_state.int1_ths_yh   = buf[2];
		gyro->resume_state.int1_ths_yl   = buf[3];
		gyro->resume_state.int1_ths_zh   = buf[4];
		gyro->resume_state.int1_ths_zl   = buf[5];
		gyro->resume_state.int1_duration = buf[6];
	}

	goto exit1;

err1:
	dev_err(&gyro->client->dev, "save register value fail at suspend\n");
	memcpy(&gyro->resume_state, &gyro->pdata->init_state,
		sizeof(struct reg_value_t));
exit1:
	l3g4200d_device_power_off(gyro);

	if (gyro->hw_initialized)
		gyro->hw_initialized = 0;

	return 0;
}

static int l3g4200d_resume(struct device *dev)
{
	struct l3g4200d_data *gyro = i2c_get_clientdata(l3g4200d_i2c_client);
	int err;

	pr_info(KERN_INFO "l3g4200d_resume\n");

	if(interrupt_mode)
		l3g4200d_irq_hw_init(1);

	if(atomic_read(&gyro->enabled)) {

		err = l3g4200d_device_power_on(gyro);
		if(err)
		{
			dev_err(&gyro->client->dev, "power_on failed at resume\n");
			atomic_set(&gyro->enabled, 0);

			return 0;
		}

		//do not report noise at IC power-up
		// flush data before really read
		if(interrupt_mode)
			schedule_delayed_work(&gyro->enable_work, msecs_to_jiffies(
				L3G4200D_PU_DELAY));
		else {
			flush_polling_data = 1;
			schedule_delayed_work(&gyro->input_work, msecs_to_jiffies(
				L3G4200D_PU_DELAY));
		}
	}else {
		memcpy(&gyro->resume_state, &gyro->pdata->init_state,
			sizeof(struct reg_value_t));

		err = l3g4200d_hw_init(gyro);
		if (err)
			dev_err(&gyro->client->dev, "hardware init failed at resume\n");
	}

	return 0;
}

static const struct i2c_device_id l3g4200d_id[] = {
	{ L3G4200D_GYR_DEV_NAME , 0 },
	{},
};

MODULE_DEVICE_TABLE(i2c, l3g4200d_id);

static struct dev_pm_ops l3g4200d_pm = {
	.suspend = l3g4200d_suspend,
	.resume = l3g4200d_resume,
};

static struct i2c_driver l3g4200d_driver = {
	.driver = {
			.owner = THIS_MODULE,
			.name = L3G4200D_GYR_DEV_NAME,
			.pm = &l3g4200d_pm,
	},
	.probe = l3g4200d_probe,
	.remove = __devexit_p(l3g4200d_remove),
	.shutdown = l3g4200d_shutdown,
	.id_table = l3g4200d_id,
};

static struct l3g4200d_gyr_platform_data gyr_drvr_platform_data={
	.poll_interval 	= 10000, // us
	.min_interval 	= 1250,  // us

	.fs_range		= L3G4200D_GYR_FS_2000DPS,

	.axis_map_x		= 0,
	.axis_map_y		= 1,
	.axis_map_z		= 2,

	.direction_x		= 1,
	.direction_y		= -1,
	.direction_z		= -1,

	.init_state.ctrl_reg1      = 0x17,   //ODR100
	.init_state.ctrl_reg2      = 0x00,
	.init_state.ctrl_reg3      = 0x00,   //DRDY interrupt
	.init_state.ctrl_reg4      = 0xA0,   //BDU enable, 2000 dps
	.init_state.ctrl_reg5      = 0x00,
	.init_state.ref_datacap    = 0x00,
	.init_state.fifo_ctrl_reg  = 0x00,
	.init_state.int1_cfg       = 0x00,
	.init_state.int1_ths_xh    = 0x00,
	.init_state.int1_ths_xl    = 0x00,
	.init_state.int1_ths_yh    = 0x00,
	.init_state.int1_ths_yl    = 0x00,
	.init_state.int1_ths_zh    = 0x00,
	.init_state.int1_ths_zl    = 0x00,
	.init_state.int1_duration  = 0x00
};

static struct i2c_board_info __initdata l3g4200d_i2c_board[] = {
    {
        I2C_BOARD_INFO(L3G4200D_GYR_DEV_NAME, L3G4200D_I2C_ADDR),
		.irq = IRQ_GPIO,
		.platform_data = &gyr_drvr_platform_data,
    },
};

static int __init l3g4200d_init(void)
{
	int ret;
	struct i2c_adapter *adapter;

#ifdef DEBUG
	pr_info("%s: gyroscope sysfs driver init\n", L3G4200D_GYR_DEV_NAME);
#endif

	adapter = i2c_get_adapter(0);
	if (adapter == NULL) {
		pr_err("%s: i2c_get_adapter() error\n", L3G4200D_GYR_DEV_NAME);
		return -ENODEV;
	}

	l3g4200d_i2c_client = i2c_new_device(adapter, l3g4200d_i2c_board);
	if (l3g4200d_i2c_client == NULL) {
		pr_err("%s: i2c_new_device() error\n", L3G4200D_GYR_DEV_NAME);
		return -ENOMEM;
	}

	i2c_put_adapter(adapter);

	ret = i2c_add_driver(&l3g4200d_driver);
	if(ret){
		pr_err("%s: i2c_add_driver() failed\n", L3G4200D_GYR_DEV_NAME);
		i2c_unregister_device(l3g4200d_i2c_client);
		return ret;
	}

	return ret;
}

static void __exit l3g4200d_exit(void)
{
#ifdef DEBUG
	pr_info("L3G4200D exit\n");
#endif

	i2c_del_driver(&l3g4200d_driver);
	i2c_unregister_device(l3g4200d_i2c_client);

	return;
}

module_init(l3g4200d_init);
module_exit(l3g4200d_exit);

MODULE_DESCRIPTION("l3g4200d digital gyroscope sysfs driver");
MODULE_AUTHOR("Matteo Dameno, Carmine Iascone, STMicroelectronics");
MODULE_LICENSE("GPL");
