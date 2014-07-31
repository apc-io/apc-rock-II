/*++
	drivers/regulator/gmt2214.c

	Copyright (c) 2013 WonderMedia Technologies, Inc.

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
#include <linux/err.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <linux/regulator/driver.h>
#include <linux/slab.h>
#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif
#include <linux/regulator/machine.h>
#include <mach/hardware.h>
#include <linux/interrupt.h>
#include <asm/unaligned.h>
#include <linux/delay.h>
#include <mach/gmt-core.h>

#undef DEBUG
#ifdef  DEBUG
#define DPRINTK(fmt, args...) do { printk(KERN_DEBUG fmt , ##args); } while (0)
#else
#define DPRINTK(fmt, args...) do { } while (0)
#endif

#define GMT_DC1_MAX_STAGE 16
#define GMT_DC2_MAX_STAGE 16
#define GMT_LDOX_MAX_STAGE 2
#define GMT_LDO56_MAX_STAGE 4

#define GMT_DC1_MIN_UV  1050000 
#define GMT_DC1_MAX_UV  1500000

#define GMT_DC2_MIN_UV  1050000 
#define GMT_DC2_MAX_UV  1500000

#define GMTV1_DC1_MIN_UV  1000000 
#define GMTV1_DC1_MAX_UV  1400000

#define GMTV1_DC2_MIN_UV  1000000 
#define GMTV1_DC2_MAX_UV  1400000

#define DC1_REG_IDX 4
#define DC2_REG_IDX 4
#define DC1_REG_SHIFT 4
#define DC2_REG_SHIFT 0
#define DC1_REG_MASK 0xf0
#define DC2_REG_MASK 0xf
#define LDXEN_REG_IDX 2

#define GMT_LDOX_MIN_UV  1050000 
#define GMT_LDOX_MAX_UV  1500000

#define LDO56_REG_IDX 3
#define LDO5_REG_SHIFT 2
#define LDO6_REG_SHIFT 0
#define LDO5_REG_MASK 0x0c
#define LDO6_REG_MASK 0x03
#define GMT_LDO56_MIN_UV  1500000 
#define GMT_LDO56_MAX_UV  3300000

#define SUPPORT_DC_NUM 7
/*
#define DEBUG
*/

extern int wmt_getsyspara(char *varname, unsigned char *varval, int *varlen);
static unsigned int g_i2cbus_id = 2;
static unsigned int g_pmic_en = 0;
static unsigned int gmt_version = 0;

struct gmt_data {
	struct i2c_client *client;
	struct gmt2214_dev *gmt_dev;
	struct device *dev;
	unsigned int dc1_min_uV;
	unsigned int dc1_max_uV;
	unsigned int dc1_current_uV;
	unsigned int dc2_current_uV;
	unsigned int dc2_min_uV;
	unsigned int dc2_max_uV;
	unsigned char enabled;
	char regs[14];
	struct regulator_dev *rdev;
};
static struct gmt_data *g_gmtdata;
static unsigned int delay_time = 400;/*us*/
static unsigned int swap_dc1_dc2 = 0;
static int early_suspend_stage = 0;

static int i2cWriteToGMT(struct gmt2214_dev *gmt2214, unsigned char bufferIndex, unsigned char value)
{
	return gmt2214_reg_write(gmt2214, bufferIndex, value);
}

#ifdef DEBUG
static int i2cReadFromGMT(struct i2c_client *client, unsigned char bufferIndex,
		unsigned char dataBuffer[], unsigned short dataLength)
{
	int ret;
	struct i2c_msg msgs[2] = {
		{
			.addr = client->addr,
			.flags = I2C_M_NOSTART,
			.len = 1,
			.buf = &bufferIndex
		},
		{
			.addr = client->addr,
			.flags = I2C_M_RD,
			.len = dataLength,
			.buf = dataBuffer
		}
	};

	memset(dataBuffer, 0x00, dataLength);
	ret = i2c_transfer(client->adapter, msgs, 2);
	return ret;
}
#endif
static int gmt_read_i2c(struct gmt2214_dev *gmt_2214, 
                u8 reg, int *rt_value)
{
	int err;

	if (!rt_value)
		return -EINVAL;

	err = gmt2214_reg_read(gmt_2214, reg, rt_value);

	if (err < 0)
		return err;

	g_gmtdata->regs[reg] = *rt_value;
	/*
	printk("reg = %x, value = %x\n", reg, g_gmtdata->regs[reg]);
	*/

	return 0;
}

static int gmt_dc1_set_voltage(struct regulator_dev *rdev, int min_uV, int max_uV,
			  unsigned *selector)
{
	struct gmt_data *gmt = rdev_get_drvdata(rdev);
	struct gmt2214_dev *gmt_dev = gmt->gmt_dev;
	u8 dc1_prog = 0;
	int ret = 0;
	int retry_count = 10;
	dc1_prog = gmt->regs[DC1_REG_IDX];

	if (min_uV > gmt->dc1_max_uV || max_uV < gmt->dc1_min_uV)
		return -EINVAL;
	if (max_uV > gmt->dc1_max_uV)
		return -EINVAL;

	if (min_uV < gmt->dc1_min_uV)
		min_uV = gmt->dc1_min_uV;

	if (gmt_version == 0) {
		if (min_uV < 1200000) {
			if ((min_uV - gmt->dc1_min_uV) % 50000) 
				*selector = ((min_uV - gmt->dc1_min_uV) / 50000) + 1;
			else
				*selector = (min_uV - gmt->dc1_min_uV) / 50000;
			
		} else {
			if ((min_uV - 1200000) % 25000) 
				*selector = ((min_uV - 1200000) / 25000) + 1;
			else
				*selector = (min_uV - 1200000) / 25000;
			*selector += 3;
		}
	} else {
		if (min_uV >= 1400000)
			*selector = 15;
		else {
			if ((min_uV - 1000000) % 25000) 
				*selector = ((min_uV - 1000000) / 25000) + 1;
			else
				*selector = (min_uV - 1000000) / 25000;
		}
	}

	if (!swap_dc1_dc2) {
		dc1_prog &= ~DC1_REG_MASK;
		dc1_prog |= *selector << DC1_REG_SHIFT;
	} else {
		dc1_prog &= ~DC2_REG_MASK;
		dc1_prog |= *selector << DC2_REG_SHIFT;
	}

	/*
	printk("changing voltage dc1 to %duv\n",
		min_uV);
	printk("dc1_prog = %x\n", dc1_prog);
	*/

	if (early_suspend_stage == 1)
		return ret;

	while (retry_count > 0) {
		ret = i2cWriteToGMT(gmt_dev, DC1_REG_IDX, dc1_prog);
		if (ret >= 0)
			break;
		--retry_count;
	}
	if (ret < 0)
		return ret;
	gmt->regs[DC1_REG_IDX] = dc1_prog;
	usleep_range(delay_time, delay_time + 50);
	gmt->dc1_current_uV = min_uV;
	return ret;
	
}
static int gmt_dc1_update_voltage(void)
{
	unsigned int tmp_buf = 0;
	int current_uV;
	int ret = 0;
	ret = gmt_read_i2c(g_gmtdata->gmt_dev, DC1_REG_IDX, &tmp_buf);
	if (!swap_dc1_dc2) {
		tmp_buf &= DC1_REG_MASK;
		tmp_buf >>= DC1_REG_SHIFT;
	} else {
		tmp_buf &= DC2_REG_MASK;
		tmp_buf >>= DC2_REG_SHIFT;
	}

	if (gmt_version == 0) {
		if (tmp_buf < 3)
			current_uV = 1050000 + tmp_buf * 50000;
		else {
			tmp_buf -= 3;
			current_uV = 1200000 + tmp_buf * 25000;
		}
	} else {
		if (tmp_buf == 0xf)
			current_uV = 1400000;
		else
			current_uV = 1000000 + tmp_buf * 25000;
	}
	g_gmtdata->dc1_current_uV = current_uV;
	return ret;
}

static int gmt_dc1_get_voltage(struct regulator_dev *rdev)
{
	struct gmt_data *gmt = rdev_get_drvdata(rdev);
	u32 tmp_buf = 0;
	unsigned int current_uV;
	int ret = 0;

	ret = gmt_read_i2c(gmt->gmt_dev, DC1_REG_IDX, &tmp_buf);
	if (!swap_dc1_dc2) {
		tmp_buf &= DC1_REG_MASK;
		tmp_buf >>= DC1_REG_SHIFT;
	} else {
		tmp_buf &= DC2_REG_MASK;
		tmp_buf >>= DC2_REG_SHIFT;
	}
	if (gmt_version == 0) {
		if (tmp_buf < 3)
			current_uV = 1050000 + tmp_buf * 50000;
		else {
			tmp_buf -= 3;
			current_uV = 1200000 + tmp_buf * 25000;
		}
	} else {
		if (tmp_buf == 0xf)
			current_uV = 1400000;
		else
			current_uV = 1000000 + tmp_buf * 25000;
	}
	gmt->dc1_current_uV = current_uV;
	return gmt->dc1_current_uV;
}

static int gmt_dc1_is_enabled(struct regulator_dev *rdev)
{
	struct gmt_data *gmt = rdev_get_drvdata(rdev);
	if (gmt->enabled == 1)
		return 0;
	else
		return -EDOM;
	return 0;
}

static int _gmt_dc2_set_voltage(struct regulator_dev *rdev, int uV)
{
	struct gmt_data *gmt = rdev_get_drvdata(rdev);
	struct gmt2214_dev *gmt_dev = gmt->gmt_dev;
	int retry_count = 10;
	unsigned selector;
	int ret = 0;
	u8 dc2_prog;
	dc2_prog = gmt->regs[DC2_REG_IDX];
	/*
	if ((uV - gmt->dc2_min_uV) % 25000) 
		selector = ((uV - gmt->dc2_min_uV) / 25000) + 1;
	else
		selector = (uV - gmt->dc2_min_uV) / 25000;
	*/
	if (gmt_version == 0) {
		if (uV < 1200000) {
			if ((uV - gmt->dc2_min_uV) % 50000)
				selector = ((uV - gmt->dc2_min_uV) / 50000) + 1;
			else
				selector = (uV - gmt->dc2_min_uV) / 50000;

		} else {
			if ((uV - 1200000) % 25000)
				selector = ((uV - 1200000) / 25000) + 1;
			else
				selector = (uV - 1200000) / 25000;
			selector += 3;
		}
	} else {
		if (uV >= 1400000)
			selector = 15;
		else {
			if ((uV - 1000000) % 25000) 
				selector = ((uV - 1000000) / 25000) + 1;
			else
				selector = (uV - 1000000) / 25000;
		}
	}

	if (!swap_dc1_dc2) {
		dc2_prog &= ~DC2_REG_MASK;
		dc2_prog |= selector << DC2_REG_SHIFT;
	} else {
		dc2_prog &= ~DC1_REG_MASK;
		dc2_prog |= selector << DC1_REG_SHIFT;
	}

	/*
	printk("dc2_prog = %x\n", dc2_prog);
	*/
	while (retry_count > 0) {
		ret = i2cWriteToGMT(gmt_dev, DC2_REG_IDX, dc2_prog);
		if (ret >= 0)
			break;
		--retry_count;
	}
	gmt->regs[DC2_REG_IDX] = dc2_prog;
	return selector;
}
extern void wmt_mc5_autotune(void);
static int gmt_dc2_set_voltage(struct regulator_dev *rdev, int min_uV, int max_uV,
			  unsigned *selector)
{
	struct gmt_data *gmt = rdev_get_drvdata(rdev);
	int ret = 0;
	int i = 0;
	int adj_uV;

	if (min_uV > gmt->dc2_max_uV || max_uV < gmt->dc2_min_uV)
		return -EINVAL;
	if (max_uV > gmt->dc2_max_uV)
		return -EINVAL;

	if (min_uV < gmt->dc2_min_uV)
		min_uV = gmt->dc2_min_uV;
	/*
	printk("current_uV = %d, min_uV = %d\n", gmt->dc2_current_uV, min_uV);
	*/
	if (gmt->dc2_current_uV > min_uV) {
		while (gmt->dc2_current_uV >= min_uV + i * 50000) {
			if (i != 0) {
				adj_uV = gmt->dc2_current_uV - i * 50000;
				/*
				printk("adj_uV = %d\n", adj_uV);
				*/
				*selector = _gmt_dc2_set_voltage(rdev, adj_uV);
				usleep_range(delay_time, delay_time + 50);
				wmt_mc5_autotune();
			}
			++i;
		}
		--i;
		if (gmt->dc2_current_uV != min_uV + i * 50000) {
			adj_uV = min_uV;
			*selector = _gmt_dc2_set_voltage(rdev, adj_uV);
			usleep_range(delay_time, delay_time + 50);
		}
	} else {
		while (gmt->dc2_current_uV + i * 50000 <= min_uV) {
			if (i != 0) {
				adj_uV = gmt->dc2_current_uV + i * 50000;
				/*
				printk("adj uV = %d\n", adj_uV);
				*/
				*selector = _gmt_dc2_set_voltage(rdev, gmt->dc2_current_uV + i * 50000);
				usleep_range(delay_time, delay_time + 50);
				wmt_mc5_autotune();
			}
			++i;
		}
		--i;
		if (gmt->dc2_current_uV != min_uV + i * 50000) {
			adj_uV = min_uV;
			*selector = _gmt_dc2_set_voltage(rdev, adj_uV);
			usleep_range(delay_time, delay_time + 50);
		}
	}

	gmt->dc2_current_uV = min_uV;
	return ret;
}
static unsigned int gmt_read_version(void)
{
	unsigned int ver = 0;
	int ret = 0;
	ret = gmt_read_i2c(g_gmtdata->gmt_dev, 13, &ver);
	ver &= 0x30;
	ver >>= 4;
	return ver;
#if 0
	unsigned int ver = 0;
	int ret = 0;
	g_gmtdata->gmt_dev->i2c->addr = 0x01;
	while(g_gmtdata->gmt_dev->i2c->addr < 0x80){
		printk("addr:0x%x, ret: %d\n", g_gmtdata->gmt_dev->i2c->addr, gmt_read_i2c(g_gmtdata->gmt_dev, 13, &ver));
		ver &= 0x30;
		ver >>= 4;
		g_gmtdata->gmt_dev->i2c->addr++;
	}
	return ver;
#endif
}

static int gmt_dc2_update_voltage(void)
{
	unsigned int tmp_buf = 0;
	int current_uV;
	int ret = 0;

	ret = gmt_read_i2c(g_gmtdata->gmt_dev, DC2_REG_IDX, &tmp_buf);

	if (!swap_dc1_dc2) {
		tmp_buf &= DC2_REG_MASK;
		tmp_buf >>= DC2_REG_SHIFT;
	} else {
		tmp_buf &= DC1_REG_MASK;
		tmp_buf >>= DC1_REG_SHIFT;
	}
	if (gmt_version == 0) {
		if (tmp_buf < 3)
			current_uV = 1050000 + tmp_buf * 50000;
		else {
			tmp_buf -= 3;
			current_uV = 1200000 + tmp_buf * 25000;
		}
	} else {
		if (tmp_buf == 0xf)
			current_uV = 1400000;
		else
			current_uV = 1000000 + tmp_buf * 25000;
	}

	g_gmtdata->dc2_current_uV = current_uV;
	return ret;
}

static int gmt_dc2_get_voltage(struct regulator_dev *rdev)
{
	struct gmt_data *gmt = rdev_get_drvdata(rdev);
	u32 tmp_buf = 0;
	unsigned int current_uV;
	int ret = 0;
	ret = gmt_read_i2c(gmt->gmt_dev, DC2_REG_IDX, &tmp_buf);

	if (!swap_dc1_dc2) {
		tmp_buf &= DC2_REG_MASK;
		tmp_buf >>= DC2_REG_SHIFT;
	} else {
		tmp_buf &= DC1_REG_MASK;
		tmp_buf >>= DC1_REG_SHIFT;
	}
	if (gmt_version == 0) {
		if (tmp_buf < 3)
			current_uV = 1050000 + tmp_buf * 50000;
		else {
			tmp_buf -= 3;
			current_uV = 1200000 + tmp_buf * 25000;
		}
	} else {
		if (tmp_buf == 0xf)
			current_uV = 1400000;
		else
			current_uV = 1000000 + tmp_buf * 25000;
	}

	gmt->dc2_current_uV = current_uV;
	return gmt->dc2_current_uV;
}

static int recovery_ldox_state(void)
{
	unsigned int ret = 0;
	ret = i2cWriteToGMT(g_gmtdata->gmt_dev, LDXEN_REG_IDX, g_gmtdata->regs[LDXEN_REG_IDX]);
	return ret;
}
static int update_ldox_state(void)
{
	unsigned int tmp_buf;
	unsigned int ret;
	ret = gmt_read_i2c(g_gmtdata->gmt_dev, LDXEN_REG_IDX, &tmp_buf);
	g_gmtdata->regs[LDXEN_REG_IDX] = (unsigned char)tmp_buf;
	return ret;
}
static int gmt_dc2_is_enabled(struct regulator_dev *rdev)
{
	struct gmt_data *gmt = rdev_get_drvdata(rdev);
	if (gmt->enabled == 1)
		return 0;
	else
		return -EDOM;
	return 0;
}
static int gmt_ldox_enable(struct regulator_dev *rdev, unsigned int ldo_num)
{
	struct gmt_data *gmt = rdev_get_drvdata(rdev);
	unsigned char reg_val;
	unsigned int tmp_buf;
	if (ldo_num < 2 || ldo_num > 6)
		return -1;
	gmt_read_i2c(g_gmtdata->gmt_dev, LDXEN_REG_IDX, &tmp_buf);
	gmt->regs[LDXEN_REG_IDX] = (unsigned char)tmp_buf;
	reg_val =  gmt->regs[LDXEN_REG_IDX];
	switch (ldo_num) {
	case 2:
		reg_val |= BIT7;
		i2cWriteToGMT(gmt->gmt_dev, LDXEN_REG_IDX, reg_val);
		gmt->regs[LDXEN_REG_IDX] |= BIT7;
		break;
	case 3:
		reg_val |= BIT6;
		i2cWriteToGMT(gmt->gmt_dev, LDXEN_REG_IDX, reg_val);
		gmt->regs[LDXEN_REG_IDX] |= BIT6;
		break;
	case 4:
		reg_val |= BIT5;
		i2cWriteToGMT(gmt->gmt_dev, LDXEN_REG_IDX, reg_val);
		gmt->regs[LDXEN_REG_IDX] |= BIT5;
		break;
	case 5:
		reg_val |= BIT4;
		i2cWriteToGMT(gmt->gmt_dev, LDXEN_REG_IDX, reg_val);
		gmt->regs[LDXEN_REG_IDX] |= BIT4;
		break;
	case 6:
		reg_val |= BIT3;
		i2cWriteToGMT(gmt->gmt_dev, LDXEN_REG_IDX, reg_val);
		gmt->regs[LDXEN_REG_IDX] |= BIT3;
		break;
	}
	
	return 0;
}

static int gmt_ldox_disable(struct regulator_dev *rdev, unsigned int ldo_num)
{
	struct gmt_data *gmt = rdev_get_drvdata(rdev);
	unsigned char reg_val;
	unsigned int tmp_buf = 0;
	if (ldo_num < 2 || ldo_num > 6)
		return -1;
	gmt_read_i2c(g_gmtdata->gmt_dev, LDXEN_REG_IDX, &tmp_buf);
	gmt->regs[LDXEN_REG_IDX] = (unsigned char)tmp_buf;
	reg_val =  gmt->regs[LDXEN_REG_IDX];
	switch (ldo_num) {
	case 2:
		reg_val &= ~BIT7;
		i2cWriteToGMT(gmt->gmt_dev, LDXEN_REG_IDX, reg_val);
		gmt->regs[LDXEN_REG_IDX] &= ~BIT7;
		break;
	case 3:
		reg_val &= ~BIT6;
		i2cWriteToGMT(gmt->gmt_dev, LDXEN_REG_IDX, reg_val);
		gmt->regs[LDXEN_REG_IDX] &= ~BIT6;
		break;
	case 4:
		reg_val &= ~BIT5;
		i2cWriteToGMT(gmt->gmt_dev, LDXEN_REG_IDX, reg_val);
		gmt->regs[LDXEN_REG_IDX] &= ~BIT5;
		break;
	case 5:
		reg_val &= ~BIT4;
		i2cWriteToGMT(gmt->gmt_dev, LDXEN_REG_IDX, reg_val);
		gmt->regs[LDXEN_REG_IDX] &= ~BIT4;
		break;
	case 6:
		reg_val &= ~BIT3;
		i2cWriteToGMT(gmt->gmt_dev, LDXEN_REG_IDX, reg_val);
		gmt->regs[LDXEN_REG_IDX] &= ~BIT3;
		break;
	}

	return 0;
}
static int get_ldox_state(struct regulator_dev *rdev, unsigned int ldo_num)
{
	unsigned int tmp_buf = 0;
	struct gmt_data *gmt = rdev_get_drvdata(rdev);
	if (ldo_num < 2 || ldo_num > 6)
		return -1;
	gmt_read_i2c(gmt->gmt_dev, LDXEN_REG_IDX, &tmp_buf);
	if (tmp_buf & (0x1 << (9 - ldo_num))) 
		return 1;/*enabled*/
	else
		return 0;
} 
static int gmt_ldo2_is_enabled(struct regulator_dev *rdev)
{
	return get_ldox_state(rdev, 2);
}
static int gmt_ldo2_enable(struct regulator_dev *rdev)
{
	gmt_ldox_enable(rdev, 2);
	return 0;
}

static int gmt_ldo2_disable(struct regulator_dev *rdev)
{
	gmt_ldox_disable(rdev, 2);
	return 0;
}

static int gmt_ldo3_is_enabled(struct regulator_dev *rdev)
{
	return get_ldox_state(rdev, 3);
}
static int gmt_ldo3_enable(struct regulator_dev *rdev)
{
	gmt_ldox_enable(rdev, 3);
	return 0;
}

static int gmt_ldo3_disable(struct regulator_dev *rdev)
{
	gmt_ldox_disable(rdev, 3);
	return 0;
}

static int gmt_ldo4_is_enabled(struct regulator_dev *rdev)
{
	return get_ldox_state(rdev, 4);
}
static int gmt_ldo4_enable(struct regulator_dev *rdev)
{
	gmt_ldox_enable(rdev, 4);
	return 0;
}

static int gmt_ldo4_disable(struct regulator_dev *rdev)
{
	gmt_ldox_disable(rdev, 4);
	return 0;
}

static int gmt_ldo5_is_enabled(struct regulator_dev *rdev)
{
	return get_ldox_state(rdev, 5);
}
static int gmt_ldo5_enable(struct regulator_dev *rdev)
{
	gmt_ldox_enable(rdev, 5);
	return 0;
}

static int gmt_ldo5_disable(struct regulator_dev *rdev)
{
	gmt_ldox_disable(rdev, 5);
	return 0;
}

static int gmt_ldo6_is_enabled(struct regulator_dev *rdev)
{
	return get_ldox_state(rdev, 6);
}
static int gmt_ldo6_enable(struct regulator_dev *rdev)
{
	gmt_ldox_enable(rdev, 6);
	return 0;
}

static int gmt_ldo6_disable(struct regulator_dev *rdev)
{
	gmt_ldox_disable(rdev, 6);
	return 0;
}

static int gmt_ldo56_list_voltage(struct regulator_dev *rdev, unsigned selector)
{
	int ret;
	switch (selector) {
	case 0:
		ret = 1500000;
		break;
	case 1:
		ret = 1800000;
		break;
	case 2:
		ret = 2800000;
		break;
	case 3:
		ret = 3300000;
		break;
	default:
		ret = -1;
		break;
	}
	return ret;
}

static int gmt_ldo56_set_voltage_sel(struct regulator_dev *rdev, unsigned selector)
{
	struct gmt_data *gmt = rdev_get_drvdata(rdev);
	const char *rdev_name = rdev->desc->name;
	u8 reg_val = gmt->regs[LDO56_REG_IDX];

	if (!strcmp(rdev_name, "ldo5")) {
		reg_val &= ~LDO5_REG_MASK; 
		reg_val |= ((selector << LDO5_REG_SHIFT) & LDO5_REG_MASK);
	} else if (!strcmp(rdev_name, "ldo6")) {
		reg_val &= ~LDO6_REG_MASK; 
		reg_val |= ((selector << LDO6_REG_SHIFT) & LDO6_REG_MASK);
	} else
		return -EINVAL;

	i2cWriteToGMT(gmt->gmt_dev, LDO56_REG_IDX, reg_val);
	gmt->regs[LDO56_REG_IDX] = reg_val;
	return 0;
}

static struct regulator_ops gmt_dc1_ops = {
	.set_voltage = gmt_dc1_set_voltage,
	.is_enabled	= gmt_dc1_is_enabled,
	.get_voltage	= gmt_dc1_get_voltage,
};

static struct regulator_ops gmt_dc2_ops = {
	.set_voltage = gmt_dc2_set_voltage,
	.is_enabled	= gmt_dc2_is_enabled,
	.get_voltage	= gmt_dc2_get_voltage,
};

static struct regulator_ops gmt_ldo2_ops = {
	.enable		= gmt_ldo2_enable,
	.disable	= gmt_ldo2_disable,
	.is_enabled	= gmt_ldo2_is_enabled,
};

static struct regulator_ops gmt_ldo3_ops = {
	.enable		= gmt_ldo3_enable,
	.disable	= gmt_ldo3_disable,
	.is_enabled	= gmt_ldo3_is_enabled,
};

static struct regulator_ops gmt_ldo4_ops = {
	.enable		= gmt_ldo4_enable,
	.disable	= gmt_ldo4_disable,
	.is_enabled	= gmt_ldo4_is_enabled,
};

static struct regulator_ops gmt_ldo5_ops = {
	.enable		= gmt_ldo5_enable,
	.disable	= gmt_ldo5_disable,
	.is_enabled	= gmt_ldo5_is_enabled,
	.list_voltage	= gmt_ldo56_list_voltage,
	.set_voltage_sel	= gmt_ldo56_set_voltage_sel,
};

static struct regulator_ops gmt_ldo6_ops = {
	.enable		= gmt_ldo6_enable,
	.disable	= gmt_ldo6_disable,
	.is_enabled	= gmt_ldo6_is_enabled,
	.list_voltage	= gmt_ldo56_list_voltage,
	.set_voltage_sel	= gmt_ldo56_set_voltage_sel,
};

static struct regulator_desc gmt_reg[] = {
	{
		.name = "wmt_corepower",
		.id = 0,
		.ops = &gmt_dc1_ops,
		.type = REGULATOR_VOLTAGE,
		.n_voltages = GMT_DC1_MAX_STAGE,
		.owner = THIS_MODULE,
	},
	{
		.name = "wmt_vdd",
		.id = 0,
		.ops = &gmt_dc2_ops,
		.type = REGULATOR_VOLTAGE,
		.n_voltages = GMT_DC2_MAX_STAGE,
		.owner = THIS_MODULE,
	},
	{
		.name = "ldo2",
		.id = 0,
		.ops = &gmt_ldo2_ops,
		.type = REGULATOR_VOLTAGE,
		.n_voltages = GMT_LDOX_MAX_STAGE,
		.owner = THIS_MODULE,
	},
	{
		.name = "ldo3",
		.id = 0,
		.ops = &gmt_ldo3_ops,
		.type = REGULATOR_VOLTAGE,
		.n_voltages = GMT_LDOX_MAX_STAGE,
		.owner = THIS_MODULE,
	},
	{
		.name = "ldo4",
		.id = 0,
		.ops = &gmt_ldo4_ops,
		.type = REGULATOR_VOLTAGE,
		.n_voltages = GMT_LDOX_MAX_STAGE,
		.owner = THIS_MODULE,
	},
	{
		.name = "ldo5",
		.id = 0,
		.ops = &gmt_ldo5_ops,
		.type = REGULATOR_VOLTAGE,
		.n_voltages = GMT_LDO56_MAX_STAGE,
		.owner = THIS_MODULE,
	},
	{
		.name = "ldo6",
		.id = 0,
		.ops = &gmt_ldo6_ops,
		.type = REGULATOR_VOLTAGE,
		.n_voltages = GMT_LDO56_MAX_STAGE,
		.owner = THIS_MODULE,
	},
};

static struct regulator_consumer_supply gmt_dc1_supply =
	REGULATOR_SUPPLY("wmt_corepower", NULL);
static struct regulator_consumer_supply gmt_dc2_supply =
	REGULATOR_SUPPLY("wmt_vdd", NULL);
static struct regulator_consumer_supply gmt_ldo2_supply =
	REGULATOR_SUPPLY("ldo2", NULL);
static struct regulator_consumer_supply gmt_ldo3_supply =
	REGULATOR_SUPPLY("ldo3", NULL);
static struct regulator_consumer_supply gmt_ldo4_supply =
	REGULATOR_SUPPLY("ldo4", NULL);
static struct regulator_consumer_supply gmt_ldo5_supply =
	REGULATOR_SUPPLY("ldo5", NULL);
static struct regulator_consumer_supply gmt_ldo6_supply =
	REGULATOR_SUPPLY("ldo6", NULL);

static struct regulator_init_data gmt_dc1_power = {
	.constraints = {
		.min_uV                 = GMT_DC1_MIN_UV,
		.max_uV                 = GMT_DC1_MAX_UV,
		.apply_uV               = true,
		.valid_modes_mask       = REGULATOR_MODE_NORMAL
					| REGULATOR_MODE_STANDBY,
		.valid_ops_mask         = REGULATOR_CHANGE_MODE
					| REGULATOR_CHANGE_STATUS
					| REGULATOR_CHANGE_VOLTAGE,
	},
	.num_consumer_supplies  = 1,
	.consumer_supplies      = &gmt_dc1_supply,
};
static struct regulator_init_data gmtv1_dc1_power = {
	.constraints = {
		.min_uV                 = GMTV1_DC1_MIN_UV,
		.max_uV                 = GMTV1_DC1_MAX_UV,
		.apply_uV               = true,
		.valid_modes_mask       = REGULATOR_MODE_NORMAL
					| REGULATOR_MODE_STANDBY,
		.valid_ops_mask         = REGULATOR_CHANGE_MODE
					| REGULATOR_CHANGE_STATUS
					| REGULATOR_CHANGE_VOLTAGE,
	},
	.num_consumer_supplies  = 1,
	.consumer_supplies      = &gmt_dc1_supply,
};

static struct regulator_init_data gmt_dc2_power = {
	.constraints = {
		.min_uV                 = GMT_DC2_MIN_UV,
		.max_uV                 = GMT_DC2_MAX_UV,
		.apply_uV               = true,
		.valid_modes_mask       = REGULATOR_MODE_NORMAL
					| REGULATOR_MODE_STANDBY,
		.valid_ops_mask         = REGULATOR_CHANGE_MODE
					| REGULATOR_CHANGE_STATUS
					| REGULATOR_CHANGE_VOLTAGE,
	},
	.num_consumer_supplies  = 1,
	.consumer_supplies      = &gmt_dc2_supply,
};
static struct regulator_init_data gmtv1_dc2_power = {
	.constraints = {
		.min_uV                 = GMTV1_DC2_MIN_UV,
		.max_uV                 = GMTV1_DC2_MAX_UV,
		.apply_uV               = true,
		.valid_modes_mask       = REGULATOR_MODE_NORMAL
					| REGULATOR_MODE_STANDBY,
		.valid_ops_mask         = REGULATOR_CHANGE_MODE
					| REGULATOR_CHANGE_STATUS
					| REGULATOR_CHANGE_VOLTAGE,
	},
	.num_consumer_supplies  = 1,
	.consumer_supplies      = &gmt_dc2_supply,
};

static struct regulator_init_data gmt_ldo2_power = {
	.constraints = {
		.min_uV                 = GMT_LDOX_MIN_UV,
		.max_uV                 = GMT_LDOX_MAX_UV,
		.apply_uV               = true,
		.valid_modes_mask       = REGULATOR_MODE_NORMAL
					| REGULATOR_MODE_STANDBY,
		.valid_ops_mask         = REGULATOR_CHANGE_MODE
					| REGULATOR_CHANGE_STATUS
					| REGULATOR_CHANGE_VOLTAGE,
	},
	.num_consumer_supplies  = 1,
	.consumer_supplies      = &gmt_ldo2_supply,
};
static struct regulator_init_data gmt_ldo3_power = {
	.constraints = {
		.min_uV                 = GMT_LDOX_MIN_UV,
		.max_uV                 = GMT_LDOX_MAX_UV,
		.apply_uV               = true,
		.valid_modes_mask       = REGULATOR_MODE_NORMAL
					| REGULATOR_MODE_STANDBY,
		.valid_ops_mask         = REGULATOR_CHANGE_MODE
					| REGULATOR_CHANGE_STATUS
					| REGULATOR_CHANGE_VOLTAGE,
	},
	.num_consumer_supplies  = 1,
	.consumer_supplies      = &gmt_ldo3_supply,
};
static struct regulator_init_data gmt_ldo4_power = {
	.constraints = {
		.min_uV                 = GMT_LDOX_MIN_UV,
		.max_uV                 = GMT_LDOX_MAX_UV,
		.apply_uV               = true,
		.valid_modes_mask       = REGULATOR_MODE_NORMAL
					| REGULATOR_MODE_STANDBY,
		.valid_ops_mask         = REGULATOR_CHANGE_MODE
					| REGULATOR_CHANGE_STATUS
					| REGULATOR_CHANGE_VOLTAGE,
	},
	.num_consumer_supplies  = 1,
	.consumer_supplies      = &gmt_ldo4_supply,
};
static struct regulator_init_data gmt_ldo5_power = {
	.constraints = {
		.min_uV                 = GMT_LDO56_MIN_UV,
		.max_uV                 = GMT_LDO56_MAX_UV,
		.apply_uV               = true,
		.valid_modes_mask       = REGULATOR_MODE_NORMAL
					| REGULATOR_MODE_STANDBY,
		.valid_ops_mask         = REGULATOR_CHANGE_MODE
					| REGULATOR_CHANGE_STATUS
					| REGULATOR_CHANGE_VOLTAGE,
	},
	.num_consumer_supplies  = 1,
	.consumer_supplies      = &gmt_ldo5_supply,
};
static struct regulator_init_data gmt_ldo6_power = {
	.constraints = {
		.min_uV                 = GMT_LDO56_MIN_UV,
		.max_uV                 = GMT_LDO56_MAX_UV,
		.apply_uV               = true,
		.valid_modes_mask       = REGULATOR_MODE_NORMAL
					| REGULATOR_MODE_STANDBY,
		.valid_ops_mask         = REGULATOR_CHANGE_MODE
					| REGULATOR_CHANGE_STATUS
					| REGULATOR_CHANGE_VOLTAGE,
	},
	.num_consumer_supplies  = 1,
	.consumer_supplies      = &gmt_ldo6_supply,
};
#ifdef CONFIG_PM
static int g2214_read(u8 reg)
{
	unsigned int rt_value =0;

	gmt2214_reg_read(g_gmtdata->gmt_dev, reg, &rt_value);

	return rt_value;
}

static void g2214reg_dump(void)
{
	printk("reg A%d: 0x%x\n ", 0, g2214_read(0));
	printk("reg A%d: 0x%x\n ", 1, g2214_read(1));
	printk("reg A%d: 0x%x\n ", 2, g2214_read(2));
	printk("reg A%d: 0x%x\n ", 3, g2214_read(3));
	printk("reg A%d: 0x%x\n ", 4, g2214_read(4));
	printk("reg A%d: 0x%x\n ", 5, g2214_read(5));
	printk("reg A%d: 0x%x\n ", 6, g2214_read(6));
	printk("reg A%d: 0x%x\n ", 7, g2214_read(7));
	printk("reg A%d: 0x%x\n ", 8, g2214_read(8));
	printk("reg A%d: 0x%x\n ", 9, g2214_read(9));
	printk("reg A%d: 0x%x\n ", 10, g2214_read(10));
	printk("reg A%d: 0x%x\n ", 11, g2214_read(11));
	printk("reg A%d: 0x%x\n ", 12, g2214_read(12));
	printk("reg A%d: 0x%x\n ", 13, g2214_read(13));
}
static int gmt_i2c_suspend(struct device *dev)
{
	int ret=0;

	g_gmtdata->enabled = 0;
	early_suspend_stage = 1;

	printk(KERN_ERR "%s: =============\n", __func__);
#ifdef G2214_DUMP
	g2214reg_dump();
#endif

	if (ret) {
		printk(KERN_ERR "%s: Buck mode can't set to be auto mode.\n", __func__);
		return ret;
	}
	return ret;
}
static int gmt_i2c_resume(struct device *dev)
{
	int ret=0;
	printk(KERN_ERR "%s: =============\n", __func__);

	if (ret) {
		printk(KERN_ERR "%s: Buck mode can't set to be force PWM mode.\n", __func__);
		return ret;
	}

	gmt_dc2_update_voltage();
	gmt_dc1_update_voltage();
	recovery_ldox_state();
	g_gmtdata->enabled = 1;

#ifdef G2214_DUMP
	g2214reg_dump();
#endif
	early_suspend_stage = 0;
	return ret;
}
static SIMPLE_DEV_PM_OPS(gmt_dev_pm_ops,
			 gmt_i2c_suspend, gmt_i2c_resume);
#endif

static int __devinit gmt_pmic_probe(struct platform_device *pdev)
{
	struct regulator_dev *rdev;
	struct regulator_init_data	*initdata;
	struct gmt_data *gmt;
	int ret = 0;

	gmt = kzalloc(sizeof(struct gmt_data) +
			SUPPORT_DC_NUM * sizeof(struct regulator_dev *),
			GFP_KERNEL);
	if (!gmt)
		goto out;

	gmt->gmt_dev = dev_get_drvdata(pdev->dev.parent);
	gmt->dev = &pdev->dev;

	gmt->enabled = 1;

	g_gmtdata = gmt;
	gmt_version = gmt_read_version();
	rdev = &gmt->rdev[0];
	if (gmt_version == 1)
		initdata = &gmtv1_dc1_power;
	else
		initdata = &gmt_dc1_power;
	rdev = regulator_register(&gmt_reg[0], gmt->dev,
				initdata,
				gmt,
				NULL);
	rdev = &gmt->rdev[1];
	if (gmt_version == 1)
		initdata = &gmtv1_dc2_power;
	else
	initdata = &gmt_dc2_power;
	rdev = regulator_register(&gmt_reg[1], gmt->dev,
				initdata,
				gmt,
				NULL);
	rdev = &gmt->rdev[2];
	initdata = &gmt_ldo2_power;
	rdev = regulator_register(&gmt_reg[2], gmt->dev,
				initdata,
				gmt,
				NULL);
	rdev = &gmt->rdev[3];
	initdata = &gmt_ldo3_power;
	rdev = regulator_register(&gmt_reg[3], gmt->dev,
				initdata,
				gmt,
				NULL);
	rdev = &gmt->rdev[4];
	initdata = &gmt_ldo4_power;
	rdev = regulator_register(&gmt_reg[4], gmt->dev,
				initdata,
				gmt,
				NULL);
	rdev = &gmt->rdev[5];
	initdata = &gmt_ldo5_power;
	rdev = regulator_register(&gmt_reg[5], gmt->dev,
				initdata,
				gmt,
				NULL);
	rdev = &gmt->rdev[6];
	initdata = &gmt_ldo6_power;
	rdev = regulator_register(&gmt_reg[6], gmt->dev,
				initdata,
				gmt,
				NULL);

	platform_set_drvdata(pdev, gmt);
	if (gmt_version == 0) {
		gmt->dc1_min_uV = GMT_DC1_MIN_UV;
		gmt->dc1_max_uV = GMT_DC1_MAX_UV;
		gmt->dc2_min_uV = GMT_DC2_MIN_UV;
		gmt->dc2_max_uV = GMT_DC2_MAX_UV;
	} else {
		gmt->dc1_min_uV = GMTV1_DC1_MIN_UV;
		gmt->dc1_max_uV = GMTV1_DC1_MAX_UV;
		gmt->dc2_min_uV = GMTV1_DC2_MIN_UV;
		gmt->dc2_max_uV = GMTV1_DC2_MAX_UV;
	}
	gmt_dc2_update_voltage();
	gmt_dc1_update_voltage();
	update_ldox_state();

	printk("GMT PMIC ver:0x%x\n", gmt_version);
	dev_info(gmt->dev, "GMT regulator driver loaded\n");
	return 0;

out:
	return ret;
}

static int __devexit gmt_pmic_remove(struct platform_device *pdev)
{
	struct gmt_data *gmt = platform_get_drvdata(pdev);

	regulator_unregister(gmt->rdev);
	kfree(gmt);

	return 0;
}

#define par_len 80
static int parse_pmic_param(void)
{
	int retval;
	unsigned char buf[par_len];
	unsigned char tmp_buf[par_len];
	int i = 0;
	int j = 0;
	int varlen = par_len;
	char *varname = "wmt.pmic.param";
	unsigned int pmic_en = 0;
	retval = wmt_getsyspara(varname, buf, &varlen);
	if (retval == 0) {
		pmic_en = (buf[i] - '0' == 1)?1:0;
		if (pmic_en == 0) {/*disable*/
			g_pmic_en = 0;
			return -1;
		}
		i += 2;
		for (; i < par_len; ++i) {
			if (buf[i] == ':')
				break;
			tmp_buf[j] = buf[i];
			++j;
		}
		if (strncmp(tmp_buf,"gmt2214",7)) {
			g_pmic_en = 0;
			return -1;
		}
		++i;

		sscanf((buf + i), "%d:%d:%d", &g_i2cbus_id, &delay_time, &swap_dc1_dc2);
	} else {
		g_pmic_en = 0;
		return -1;
	}
	g_pmic_en = 1;
	return 0;
}

static struct platform_driver gmt_pmic_driver = {
	.probe	=	gmt_pmic_probe,
        .remove	=	__devexit_p(gmt_pmic_remove),
        .driver	= {
		.name	= "gmt-pmic",
		.owner	= THIS_MODULE,
#ifdef CONFIG_PM
		.pm	= &gmt_dev_pm_ops,
#endif
	},

};
static int __init gmt_pmic_init(void)
{
	parse_pmic_param();
	printk("GMT2214_PMIC_INIT\n");
	if (g_pmic_en == 0) {
		printk("No gmt pmic\n");
		return -ENODEV;
	}
	return platform_driver_register(&gmt_pmic_driver);
}
module_init(gmt_pmic_init);

static void __exit gmt_pmic_exit(void)
{
	platform_driver_unregister(&gmt_pmic_driver);
}
module_exit(gmt_pmic_exit);

MODULE_AUTHOR("WonderMedia Technologies, Inc.");
MODULE_DESCRIPTION("GMT2214 Driver");
MODULE_LICENSE("GPL");
