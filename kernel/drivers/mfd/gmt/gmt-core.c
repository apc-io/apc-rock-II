/*++
	drivers/mtd/gmt/gmt-core.c - GMT Core driver

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

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/interrupt.h>
#include <linux/pm_runtime.h>
#include <linux/mutex.h>
#include <linux/mfd/core.h>
#include <mach/gmt-core.h>
#include <linux/regmap.h>

static struct mfd_cell gmt_devs[] = {
	{
		.name = "gmt-pmic",
	}, {
		.name = "gmt-charger",
	},
};

int gmt2214_reg_read(struct gmt2214_dev *gmt2214, u8 reg, void *dest)
{
	struct i2c_msg msg[2];
	unsigned char data[4];
	int err;
	struct i2c_client *client;

	client = gmt2214->i2c; 

	if (!client || !client->adapter)
		return -ENODEV;

	if (!dest)
		return -EINVAL;

	data[0] = reg;
	msg[0].addr = client->addr;
	msg[0].flags = 0 | I2C_M_NOSTART;
	msg[0].len = 1;
	msg[0].buf = (unsigned char *)data;

	msg[1].addr = client->addr;
	msg[1].flags = (I2C_M_RD);
	msg[1].len = 1;
	msg[1].buf = (unsigned char *)dest;

        err = i2c_transfer(client->adapter, msg, sizeof(msg)/sizeof(struct i2c_msg));
	return err;
}
EXPORT_SYMBOL_GPL(gmt2214_reg_read);

int gmt2214_reg_write(struct gmt2214_dev *gmt2214, u8 reg, u8 value)
{
        unsigned char buffer4Write[256];
	struct i2c_client *client = gmt2214->i2c;
        struct i2c_msg msgs[1] = {
                {
                        .addr = client->addr,
                        .flags = 0,
                        .len = 2,
                        .buf = buffer4Write
                }
        };

        buffer4Write[0] = reg;
	buffer4Write[1] = value;

        return i2c_transfer(client->adapter, msgs, 1);

}
EXPORT_SYMBOL_GPL(gmt2214_reg_write);

static struct regmap_config gmt2214_regmap_config = {
	.reg_bits = 8,
	.val_bits = 8,
};

static int gmt2214_i2c_probe(struct i2c_client *i2c,
			    const struct i2c_device_id *id)
{
	struct gmt2214_platform_data *pdata = i2c->dev.platform_data;
	struct gmt2214_dev *gmt2214;
	int ret;

	gmt2214 = devm_kzalloc(&i2c->dev, sizeof(struct gmt2214_dev),
				GFP_KERNEL);
	if (gmt2214 == NULL)
		return -ENOMEM;

	i2c_set_clientdata(i2c, gmt2214);
	gmt2214->dev = &i2c->dev;
	gmt2214->i2c = i2c;

	if (pdata)
		gmt2214->device_type = pdata->device_type;

	gmt2214->regmap = regmap_init_i2c(i2c, &gmt2214_regmap_config);
	if (IS_ERR(gmt2214->regmap)) {
		ret = PTR_ERR(gmt2214->regmap);
		dev_err(&i2c->dev, "Failed to allocate register map: %d\n",
			ret);
		goto err;
	}

	pm_runtime_set_active(gmt2214->dev);

	ret = mfd_add_devices(gmt2214->dev, -1, gmt_devs,
				ARRAY_SIZE(gmt_devs), NULL, 0);

	if (ret < 0)
		goto err;

	return ret;

err:
	mfd_remove_devices(gmt2214->dev);
	regmap_exit(gmt2214->regmap);
	return ret;
}

static int gmt2214_i2c_remove(struct i2c_client *i2c)
{
	struct gmt2214_dev *gmt2214 = i2c_get_clientdata(i2c);

	mfd_remove_devices(gmt2214->dev);
	regmap_exit(gmt2214->regmap);
	return 0;
}

static struct i2c_board_info gmt_i2c_board_info = {
	.type          = "gmt2214",
	.flags         = 0x00,
	.addr          = 0x12,
	.platform_data = NULL,
	.archdata      = NULL,
	.irq           = -1,
};

static const struct i2c_device_id gmt2214_i2c_id[] = {
	{"gmt2214", 0},
	{}
};
MODULE_DEVICE_TABLE(i2c, gmt2214_i2c_id);

static struct i2c_driver gmt2214_i2c_driver = {
	.driver = {
		   .name = "gmt2214",
		   .owner = THIS_MODULE,
	},
	.probe = gmt2214_i2c_probe,
	.remove = gmt2214_i2c_remove,
	.id_table = gmt2214_i2c_id,
};

extern int wmt_getsyspara(char *varname, unsigned char *varval, int *varlen);

#define par_len 80
static unsigned int g_i2cbus_id = 3;
static unsigned int g_pmic_en = 0;

static int parse_pmic_param(void)
{
	int retval;
	unsigned char buf[par_len];
	unsigned char tmp_buf[par_len];
	unsigned int delay_time;
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

		sscanf((buf + i), "%d:%d", &g_i2cbus_id, &delay_time);
	} else {
		g_pmic_en = 0;
		return -1;
	}
	g_pmic_en = 1;
	return 0;
}

static unsigned int g_chg_en = 0;
int parse_charger_param(void)
{
	int retval;
	unsigned char buf[240];
	int varlen = par_len;
	//char *varname = "wmt.io.chg";
	char *varname = "wmt.charger.param";
	retval = wmt_getsyspara(varname, buf, &varlen);
	if (retval == 0) {
		g_chg_en = (buf[0] - '0' == 1)?1:0;
		if (g_chg_en == 0)
			return -1;
		if (strncmp((buf + 2), "g2214", 5)) {
			g_chg_en = 0;
			return -1;
		}
	} else {
		g_chg_en = 0;
		return -1;
	}
	g_chg_en = 1;
	return 1;
}
static int __init gmt2214_i2c_init(void)
{
	struct i2c_board_info *gmt2214_i2c_bi;
	struct i2c_adapter *adapter = NULL;
	struct i2c_client *client   = NULL;

	parse_pmic_param();
	parse_charger_param();
	if (g_pmic_en == 0 && g_chg_en == 0) {
		printk("Don't support GMT2214\n");
		return -ENODEV;
	}
	gmt2214_i2c_bi = &gmt_i2c_board_info;
	adapter = i2c_get_adapter(g_i2cbus_id);/*in bus 3*/

	if (NULL == adapter) {
		printk("can not get i2c adapter, client address error\n");
		return -ENODEV;
	}

	client = i2c_new_device(adapter, gmt2214_i2c_bi);
	if (client == NULL) {
		printk("allocate i2c client failed\n");
		return -ENODEV;
	}
	i2c_put_adapter(adapter);

	return i2c_add_driver(&gmt2214_i2c_driver);
}

subsys_initcall_sync(gmt2214_i2c_init);

static void __exit gmt2214_i2c_exit(void)
{
	i2c_del_driver(&gmt2214_i2c_driver);
}
module_exit(gmt2214_i2c_exit);

MODULE_AUTHOR("WonderMedia Technologies, Inc.");
MODULE_DESCRIPTION("GMT2214 Core Driver");
MODULE_LICENSE("GPL");
