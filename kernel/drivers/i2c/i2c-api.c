/*++
	linux/drivers/i2c/i2c-api.c

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

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/list.h>
#include <linux/i2c.h>
#include <linux/slab.h>

#define I2C_API_FAKE_ADDR 0x7f
#define I2C_MINORS	      256

struct i2c_api {
	struct list_head list;
	struct i2c_client *client;
};

static LIST_HEAD(i2c_api_list);
static DEFINE_SPINLOCK(i2c_api_list_lock);

static struct i2c_api *get_i2c_api(int bus_id)
{
	struct i2c_api *i2c_api;

	spin_lock(&i2c_api_list_lock);
	list_for_each_entry(i2c_api, &i2c_api_list, list) {
		if (i2c_api->client->adapter->nr == bus_id)
			goto found;
	}
	i2c_api = NULL;
	
found:
	spin_unlock(&i2c_api_list_lock);
	return i2c_api;
}

static struct i2c_api *add_i2c_api(struct i2c_client *client)
{
	struct i2c_api *i2c_api;

	if (client->adapter->nr >= I2C_MINORS) {
		printk(KERN_ERR "i2c_api: Out of device minors (%d)\n",
			client->adapter->nr);
		return NULL;
	}

	i2c_api = kzalloc(sizeof(*i2c_api), GFP_KERNEL);
	if (!i2c_api)
		return NULL;
	i2c_api->client = client;

	spin_lock(&i2c_api_list_lock);
	list_add_tail(&i2c_api->list, &i2c_api_list);
	spin_unlock(&i2c_api_list_lock);
	return i2c_api;
}

static void del_i2c_api(struct i2c_api *i2c_api)
{
	spin_lock(&i2c_api_list_lock);
	list_del(&i2c_api->list);
	spin_unlock(&i2c_api_list_lock);
	kfree(i2c_api);
}

static int i2c_api_do_xfer(int bus_id, char chip_addr, char sub_addr, int mode, 
	char *buf, unsigned int size)
{
/** you could define more transfer mode here, implement it following. */
#define I2C_API_XFER_MODE_SEND            0x0 /* standard send */
#define I2C_API_XFER_MODE_RECV            0x1 /* standard receive */
#define I2C_API_XFER_MODE_SEND_NO_SUBADDR 0x2 /* send without sub-address */
#define I2C_API_XFER_MODE_RECV_NO_SUBADDR 0x3 /* receive without sub-address */

	int ret = 0;
	char *tmp;
	struct i2c_api *i2c_api = get_i2c_api(bus_id);

	if (!i2c_api)
		return -ENODEV;
	
	i2c_api->client->addr = chip_addr;
	switch (mode) {
	case I2C_API_XFER_MODE_SEND:
		tmp = kmalloc(size + 1,GFP_KERNEL);
		if (tmp == NULL)
			return -ENOMEM;
		tmp[0] = sub_addr;
		memcpy(&tmp[1], buf, size);
		ret = i2c_master_send(i2c_api->client, tmp, size + 1);
		ret = (ret == size + 1) ? size : ret;
		kfree(tmp);
		break;
		
	case I2C_API_XFER_MODE_RECV:
		ret = i2c_master_send(i2c_api->client, &sub_addr, 1);
		if (ret < 0)
			return ret;
		ret = i2c_master_recv(i2c_api->client, buf, size);
		break;
	
	case I2C_API_XFER_MODE_SEND_NO_SUBADDR:
		ret = i2c_master_send(i2c_api->client, buf, size);
		break;
		
	case I2C_API_XFER_MODE_RECV_NO_SUBADDR:
		ret = i2c_master_recv(i2c_api->client, buf, size);
		break;
			
	default:
		return -EINVAL;
	}
	return ret;
}

int i2c_api_do_send(int bus_id, char chip_addr, char sub_addr, char *buf, unsigned int size)
{
	return i2c_api_do_xfer(bus_id, chip_addr, sub_addr, I2C_API_XFER_MODE_SEND, buf, size);
}

int i2c_api_do_recv(int bus_id, char chip_addr, char sub_addr, char *buf, unsigned int size)
{
	return i2c_api_do_xfer(bus_id, chip_addr, sub_addr, I2C_API_XFER_MODE_RECV, buf, size);
}

int i2c_api_do_send_without_subaddr(int bus_id, char chip_addr, char *buf, unsigned int size)
{
	return i2c_api_do_xfer(bus_id, chip_addr, 0, I2C_API_XFER_MODE_SEND_NO_SUBADDR, buf, size);
}

int i2c_api_do_recv_without_subaddr(int bus_id, char chip_addr, char *buf, unsigned int size)
{
	return i2c_api_do_xfer(bus_id, chip_addr, 0, I2C_API_XFER_MODE_RECV_NO_SUBADDR, buf, size);
}

int i2c_api_attach(struct i2c_adapter *adap)
{
	struct i2c_board_info info;
	struct i2c_client *client;
	
	memset(&info, 0, sizeof(struct i2c_board_info));
		strlcpy(info.type, "i2c_api", I2C_NAME_SIZE);
	info.addr = I2C_API_FAKE_ADDR;
	client = i2c_new_device(adap, &info);
	if (client)
		add_i2c_api(client);
	printk(KERN_INFO "i2c_api_attach adap[%d]\n", adap->nr);
	return 0;
}

int i2c_api_detach(struct i2c_adapter *adap)
{
	struct i2c_api *i2c_api;

	i2c_api = get_i2c_api(adap->nr);
	if (i2c_api)
		del_i2c_api(i2c_api);
	return 0;
}

static const unsigned short normal_addr[] = { I2C_API_FAKE_ADDR, I2C_CLIENT_END };
static const unsigned short ignore[]      = { I2C_CLIENT_END };
/*static struct i2c_client_address_data addr_data = 
{
	.normal_i2c = normal_addr,
	.probe      = ignore,
	.ignore     = ignore,
	.forces     = NULL,
};*/

static const struct i2c_device_id id[] = {
	{"I2C-API", 0},
	{} 
};      
MODULE_DEVICE_TABLE(i2c, id);

static struct i2c_driver i2c_api_driver = {
	.id_table       = id,
	.attach_adapter = i2c_api_attach,
	.detach_adapter	= i2c_api_detach,
 	.command        = NULL,
	.driver         = {
		.name  = "I2C-API",
		.owner = THIS_MODULE,
	},
	//.address_data   = &addr_data,
};

static int __init i2c_api_init(void)
{
	int ret = i2c_add_driver(&i2c_api_driver);
	
	if (ret) {
		printk(KERN_ERR "[%s] Driver registration failed, module not inserted.\n", __func__);
		return ret;
	}

	return 0 ;	
}

static void __exit i2c_api_exit(void)
{
	i2c_del_driver(&i2c_api_driver);
}

MODULE_AUTHOR("Loon, <Loonzhong@wondermedia.com.cn>");
MODULE_DESCRIPTION("I2C i2c_api Driver");
MODULE_LICENSE("GPL");

module_init(i2c_api_init);
module_exit(i2c_api_exit);

EXPORT_SYMBOL_GPL(i2c_api_do_send);
EXPORT_SYMBOL_GPL(i2c_api_do_recv);
EXPORT_SYMBOL_GPL(i2c_api_do_send_without_subaddr);
EXPORT_SYMBOL_GPL(i2c_api_do_recv_without_subaddr);
