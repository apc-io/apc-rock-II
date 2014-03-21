/*++
 * linux/drivers/video/wmt/cs8556.c
 * WonderMedia video post processor (VPP) driver
 *
 * Copyright c 2013  WonderMedia  Technologies, Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * WonderMedia Technologies, Inc.
 * 4F, 533, Chung-Cheng Road, Hsin-Tien, Taipei 231, R.O.C
--*/
/********************************************************************************
* REVISON HISTORY
*
* VERSION	| DATE		| AUTHORS	  | DESCRIPTION
* 1.0		| 2013/08/24	| Howay Huo	      | First Release
*******************************************************************************/

#define CS8556_C
// #define DEBUG
/*----------------------- DEPENDENCE -----------------------------------------*/
#include <linux/i2c.h>
#include <mach/hardware.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/gpio.h>
#include <mach/wmt_iomux.h>
#include "../vout.h"

/*----------------------- PRIVATE MACRO --------------------------------------*/
/* #define  VT1632_XXXX  xxxx    *//*Example*/
//#define CONFIG_CS8556_INTERRUPT

/*----------------------- PRIVATE CONSTANTS ----------------------------------*/
/* #define VT1632_XXXX    1     *//*Example*/
#define CS8556_ADDR 0x3d
#define CS8556_NAME          "CS8556"

/*----------------------- PRIVATE TYPE  --------------------------------------*/
/* typedef  xxxx vt1632_xxx_t; *//*Example*/
typedef struct {
	unsigned int flag;
	unsigned int gpiono;
	unsigned int act;
}avdetect_gpio_t;

/*----------EXPORTED PRIVATE VARIABLES are defined in vt1632.h  -------------*/
/*----------------------- INTERNAL PRIVATE VARIABLES - -----------------------*/
/* int  vt1632_xxx;        *//*Example*/
static int s_cs8556_ready;
static int s_cs8556_init;
static struct i2c_client *s_cs8556_client;
static int s_irq_init;
static avdetect_gpio_t s_avdetect_gpio = {0, WMT_PIN_GP0_GPIO5, 1};

static unsigned char s_CS8556_Original_Offset0[]={
	0xF0,0x7F,0x00,0x00,0x00,0x00,0x80,0x80,0x80,0x00,0x00,0x02,0x01,0x00,0x00,0x01,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
};

static unsigned char s_RGB888_To_PAL_Offset0[]={
	0x01,0x80,0x00,0x00,0x80,0x00,0x00,0x00,0x00,0x00,0x02,0x02,0x00,0x00,0x00,0x00,
	0x20,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x5F,0x03,0x3F,0x00,0x7D,0x00,0x53,0x03,0x00,0x00,0x00,0x10,0x00,0x00,0x00,0x00,
	0x70,0x02,0x04,0x00,0x2E,0x00,0x62,0x02,0x00,0x00,0x84,0x00,0x2B,0x00,0x36,0x00,
	0x12,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0xBF,0x06,0x7F,0x00,0xFE,0x00,0xA4,0x06,0x00,0x00,0x2D,0x11,0x3C,0x01,0x3A,0x01,
	0x70,0x02,0x04,0x00,0x12,0x00,0x34,0x01,0x00,0x00,0x70,0x70,0x70,0x00,0x00,0x00,
	0x22,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x04,0x41,0x18,0x09,0x00,0x00,0x00,0x02,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0xFF,0xFF,0x00,0x00,0xFF,0xFF,0x00,0x00,0x80,0x80,0x00,0x00,0x80,0x80,0x00,0x00,
	0xFF,0xFF,0xFF,0xFF,0x00,0x00,0x00,0x00,0x80,0x80,0x80,0x80,0x00,0x00,0x00,0x00,
	0xFF,0x00,0xFF,0x00,0xFF,0x00,0xFF,0x00,0x80,0x00,0x80,0x00,0x80,0x00,0x80,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x01,0x24,0x1A,0x00,0x01,0x03,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x04,0x01,0xA4,0x06,0x0B,0x00,0x07,0x01,0xF0,0x00,0x00,0x00,0x00,0x04,0x40,0x01
};

static unsigned char s_RGB888_To_NTSC_Offset0[]={
	0x01,0x80,0x00,0x00,0x80,0x00,0x00,0x00,0x00,0x00,0x02,0x02,0x00,0x00,0x00,0x00,
	0x20,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x59,0x03,0x3D,0x00,0x7E,0x00,0x49,0x03,0x00,0x00,0x00,0x10,0x00,0x00,0x00,0x00,
	0x0C,0x02,0x05,0x00,0x21,0x00,0x03,0x02,0x00,0x00,0x7A,0x00,0x23,0x00,0x16,0x00,
	0x12,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0xB3,0x06,0x7F,0x00,0x00,0x01,0xA4,0x06,0x00,0x00,0x05,0x50,0x00,0x01,0x07,0x01,
	0x0C,0x02,0x02,0x00,0x12,0x00,0x07,0x01,0x00,0x00,0x70,0x70,0x70,0x00,0x00,0x00,
	0x22,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x04,0x41,0x18,0x09,0x00,0x00,0x00,0x02,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0xFF,0xFF,0x00,0x00,0xFF,0xFF,0x00,0x00,0x80,0x80,0x00,0x00,0x80,0x80,0x00,0x00,
	0xFF,0xFF,0xFF,0xFF,0x00,0x00,0x00,0x00,0x80,0x80,0x80,0x80,0x00,0x00,0x00,0x00,
	0xFF,0x00,0xFF,0x00,0xFF,0x00,0xFF,0x00,0x80,0x00,0x80,0x00,0x80,0x00,0x80,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x01,0x24,0x1A,0x00,0x01,0x03,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x04,0x01,0xA4,0x06,0x0B,0x00,0x07,0x01,0xF0,0x00,0x00,0x00,0x00,0x04,0x00,0x00
};

/*--------------------- INTERNAL PRIVATE FUNCTIONS ---------------------------*/
/* void vt1632_xxx(void); *//*Example*/
static int I2CMultiPageRead(unsigned char maddr, unsigned char page, unsigned char saddr, int number, unsigned char *value)
{
	int ret;
	unsigned char wbuf[2];
	struct i2c_msg rd[2];

	wbuf[0] = page;
	wbuf[1] = saddr;

	rd[0].addr  = maddr;
	rd[0].flags = 0;
	rd[0].len   = 2;
	rd[0].buf   = wbuf;

	rd[1].addr  = maddr;
	rd[1].flags = I2C_M_RD;
	rd[1].len   = number;
	rd[1].buf   = value;

	ret = i2c_transfer(s_cs8556_client->adapter, rd, ARRAY_SIZE(rd));

	if (ret != ARRAY_SIZE(rd)) {
		DBG_ERR("fail\n");
		return -1;
	}

	return 0;
}

static int I2CMultiPageWrite(unsigned char maddr, unsigned char page, unsigned char saddr, int number, unsigned char *value)
{
	int ret;
	unsigned char *pbuf;
	struct i2c_msg wr[1];

	pbuf = kmalloc(number + 2, GFP_KERNEL);
	if(!pbuf) {
        	DBG_ERR("alloc memory fail\n");
		return -1;
	}

	*pbuf = page;
	*(pbuf + 1) = saddr;

	memcpy(pbuf + 2, value, number);

	wr[0].addr  = maddr;
	wr[0].flags = 0;
	wr[0].len   = number + 2;
	wr[0].buf   = pbuf;

	ret = i2c_transfer(s_cs8556_client->adapter, wr, ARRAY_SIZE(wr));

	if (ret != ARRAY_SIZE(wr)) {
		DBG_ERR("fail\n");
		kfree(pbuf);
		return -1;
	}

	kfree(pbuf);
	return 0 ;
}

/************************** i2c device struct definition **************************/
static int __devinit cs8556_i2c_probe(struct i2c_client *i2c,
    const struct i2c_device_id *id)
{
    DBGMSG("cs8556_i2c_probe\n");

    return 0;
}

static int __devexit cs8556_i2c_remove(struct i2c_client *client)
{
	DBGMSG("cs8556_i2c_remove\n");

	return 0;
}


static const struct i2c_device_id cs8556_i2c_id[] = {
	{CS8556_NAME, 0},
	{ },
};
MODULE_DEVICE_TABLE(i2c, cs8556_i2c_id);

static struct i2c_board_info __initdata cs8556_i2c_board_info[] = {
    {
        I2C_BOARD_INFO(CS8556_NAME, CS8556_ADDR),
    },
};

static struct i2c_driver cs8556_i2c_driver = {
    .driver = {
    	.name = CS8556_NAME,
    	.owner = THIS_MODULE,
    },
    .probe = cs8556_i2c_probe,
    .remove = __devexit_p(cs8556_i2c_remove),
    .id_table = cs8556_i2c_id,
};

/*----------------------- Function Body --------------------------------------*/
static void avdetect_irq_enable(void)
{
	wmt_gpio_unmask_irq(s_avdetect_gpio.gpiono);
}

static void avdetect_irq_disable(void)
{
	wmt_gpio_mask_irq(s_avdetect_gpio.gpiono);
}

int avdetect_irq_hw_init(int resume)
{
	int ret;

	if(!resume) {
		ret = gpio_request(s_avdetect_gpio.gpiono, "avdetect irq"); //enable gpio
		if(ret < 0) {
			DBG_ERR("gpio(%d) request fail for avdetect irq\n", s_avdetect_gpio.gpiono);
			return ret;
		}
	}else
		gpio_re_enabled(s_avdetect_gpio.gpiono); //re-enable gpio

	gpio_direction_input(s_avdetect_gpio.gpiono); //gpio input

	wmt_gpio_setpull(s_avdetect_gpio.gpiono, WMT_GPIO_PULL_UP); //enable pull and pull-up

	wmt_gpio_mask_irq(s_avdetect_gpio.gpiono);       //disable interrupt

	wmt_gpio_set_irq_type(s_avdetect_gpio.gpiono, IRQ_TYPE_EDGE_BOTH); //rise edge and clear interrupt

	return 0;
}

/*
static void avdetect_irq_hw_free(void)
{
	gpio_free(AVDETECT_IRQ_PIN);

}
*/

static irqreturn_t avdetect_irq_handler(int irq, void *dev_id)
{
	//printk("avdetect_irq_handler\n");

	if(!gpio_irqstatus(s_avdetect_gpio.gpiono))
		return IRQ_NONE;

	wmt_gpio_ack_irq(s_avdetect_gpio.gpiono);  //clear interrupt

	//printk("cvbs hotplug interrupt\n");
	if(!is_gpio_irqenable(s_avdetect_gpio.gpiono)) {
		//pr_err("avdetect irq is disabled\n");
		return IRQ_HANDLED;
	}else
		return IRQ_WAKE_THREAD;
}

static irqreturn_t avdetect_irq_thread(int irq, void *dev)
{
	//printk(cvbs_hotplug_irq_thread\n");

	if(s_avdetect_gpio.act == 1) {
		if(gpio_get_value(s_avdetect_gpio.gpiono))
			printk("av plug in\n");
		else
			printk("av plug out\n");
	}
	else {
		if(gpio_get_value(s_avdetect_gpio.gpiono))
			printk("av plug out\n");
		else
			printk("av plug in\n");
	}

	return IRQ_HANDLED;
}

void cs8556_set_tv_mode(int ntsc)
{
	int ret;
	unsigned char rbuf[256] = {0};

	if( !s_cs8556_ready )
		return;

	ret = I2CMultiPageRead(CS8556_ADDR, 0x00, 0x00, 256, rbuf);
	if(ret) {
		DBG_ERR("I2C read Offset0 fail\n");
		return;
	}

	if(ntsc < 0) {
		if(memcmp(rbuf, s_CS8556_Original_Offset0, 0x11) != 0) {
			ret = I2CMultiPageWrite(CS8556_ADDR, 0x00, 0x00, 256, s_CS8556_Original_Offset0);
			if(ret)
				DBG_ERR("I2C write Original_Offset0 fail\n");
			else {
				if(s_irq_init)
					avdetect_irq_disable();
			}
		}

		return;
	}

	if(ntsc) {
		if(memcmp(rbuf, s_RGB888_To_NTSC_Offset0, 0x50) !=0) {
			ret = I2CMultiPageWrite(CS8556_ADDR, 0x00, 0x00, 256, s_RGB888_To_NTSC_Offset0);
			if(ret)
				DBG_ERR("I2C write NTSC_Offset0 fail\n");
		}
	} else {
		if(memcmp(rbuf, s_RGB888_To_PAL_Offset0, 0x50) != 0) {
			ret = I2CMultiPageWrite(CS8556_ADDR, 0x00, 0x00, 256, s_RGB888_To_PAL_Offset0);
			if(ret)
				DBG_ERR("I2C write PAL_Offset0 fail\n");
		}
	}

}

int cs8556_check_plugin(int hotplug)
{
	return 1;
}

int cs8556_init(struct vout_s *vo)
{
	int ret;
	char buf[40] = {0};
	int varlen = 40;
	int no = 1; //default i2c1
	int num;
	unsigned char rbuf[256] = {0};
	struct i2c_adapter *adapter = NULL;
	vout_tvformat_t tvformat = TV_MAX;

	DPRINT("cs8556_init\n");

		if(wmt_getsyspara("wmt.display.tvformat", buf, &varlen) == 0) {
			if(!strnicmp(buf, "PAL", 3))
			tvformat = TV_PAL;
			else if(!strnicmp(buf, "NTSC", 4))
			tvformat = TV_NTSC;
			else
			tvformat = TV_UNDEFINED;
	} else
		tvformat = TV_UNDEFINED;


	if(tvformat == TV_UNDEFINED)
		goto err0;

	if(!s_cs8556_init) {
	if(wmt_getsyspara("wmt.cs8556.i2c", buf, &varlen) == 0)	{
		if(strlen(buf) > 0)
				no = buf[0] - '0';
	}

		adapter = i2c_get_adapter(no);
	if (adapter == NULL) {
		DBG_ERR("Can not get i2c adapter, client address error\n");
			goto err0;
	}

	s_cs8556_client = i2c_new_device(adapter, cs8556_i2c_board_info);
	if (s_cs8556_client == NULL) {
		DBG_ERR("allocate i2c client failed\n");
			goto err0;
	}

	i2c_put_adapter(adapter);

	ret = i2c_add_driver(&cs8556_i2c_driver);
	if (ret != 0) {
		DBG_ERR("Failed to register CS8556 I2C driver: %d\n", ret);
			goto err1;
		}

		if(wmt_getsyspara("wmt.io.avdetect", buf, &varlen) == 0) {
			num = sscanf(buf, "%d:%d:%d", &s_avdetect_gpio.flag, &s_avdetect_gpio.gpiono,
				&s_avdetect_gpio.act);

			if(num != 3)
				DBG_ERR("wmt.io.avdetect is error. param num = %d\n", num);
			else {
				if(s_avdetect_gpio.gpiono > 19)
					DBG_ERR("invalid avdetect gpio no: %d\n", s_avdetect_gpio.gpiono);
				else {
					ret = avdetect_irq_hw_init(0);
					if(!ret) {
						ret = request_threaded_irq(IRQ_GPIO, avdetect_irq_handler,
							avdetect_irq_thread, IRQF_SHARED,  CS8556_NAME, s_cs8556_client);

						if(ret)
							DBG_ERR("%s: irq request failed: %d\n", __func__, ret);
						else {
							s_irq_init = 1;
							DPRINT("avdetect irq request success\n");
						}
					}
				}
			}
	}

		s_cs8556_init = 1;
	}
	else {
		if(s_irq_init)
			avdetect_irq_hw_init(1);
	}

	ret = I2CMultiPageRead(CS8556_ADDR, 0x00, 0x00, 256, rbuf);
	if(ret) {
		DBG_ERR("I2C address 0x%02X is not found\n", CS8556_ADDR);
		goto err0;
	}

	//if(s_cs8556_ready) {
	//	DPRINT("cs8556_reinit");
		switch(tvformat) {
		case TV_PAL:
			ret = I2CMultiPageWrite(CS8556_ADDR, 0x00, 0x00, 256, s_RGB888_To_PAL_Offset0);
			if(ret) {
				DBG_ERR("PAL init fail\n");
				goto err0;
			}
		break;

		case TV_NTSC:
			ret = I2CMultiPageWrite(CS8556_ADDR, 0x00, 0x00, 256, s_RGB888_To_NTSC_Offset0);
			if(ret) {
				DBG_ERR("NTSC init fail\n");
				goto err0;
			}
		break;

		default:
			goto err0;
		break;
		}
	//}

	if(s_irq_init)
		avdetect_irq_enable();

	s_cs8556_ready = 1;

	return 0;
//err3:
//	cvbs_hotplug_irq_disable();
//	free_irq(IRQ_GPIO, s_cs8556_client);
//	cvbs_hotplug_irq_hw_free();
//err2:
//	i2c_del_driver(&cs8556_i2c_driver);
err1:
	i2c_unregister_device(s_cs8556_client);
err0:
	s_cs8556_ready = 0;
	return -1;
}

static int cs8556_set_mode(unsigned int *option)
{
	if(!s_cs8556_ready)
		return -1;

	return 0;
}

static void cs8556_set_power_down(int enable)
{
	int ret;
	vout_t *vo;
	unsigned char rbuf[256] = {0};

	if( !s_cs8556_ready )
		return;

	vo = vout_get_entry(VPP_VOUT_NUM_DVI);
	if (vo->status & (VPP_VOUT_STS_BLANK + VPP_VOUT_STS_POWERDN)) {
		enable = 1;
	}

	DPRINT("cs8556_set_power_down(%d)\n",enable);

	ret = I2CMultiPageRead(CS8556_ADDR, 0x00, 0x00, 0x11, rbuf);
	if(ret) {
		DBG_ERR("I2C read Offset0 fail\n");
		return;
	}

	if( enable ){
		if(memcmp(rbuf, s_CS8556_Original_Offset0, 0x11) != 0) {
			ret = I2CMultiPageWrite(CS8556_ADDR, 0x00, 0x00, 0x11, s_CS8556_Original_Offset0);
			if(ret)
				DBG_ERR("I2C write Offset0 to power down CS8556 fail\n");
			else {
				if(s_irq_init)
					avdetect_irq_disable();
			}
		}
	} else {
		if(memcmp(rbuf, s_RGB888_To_PAL_Offset0, 0x11) != 0) {
			ret = I2CMultiPageWrite(CS8556_ADDR, 0x00, 0x00, 0x11, s_RGB888_To_PAL_Offset0);
					if(ret)
				DBG_ERR("I2C write offset0 to power up CS8556 fail\n");
		}
	}
}

static int cs8556_config(vout_info_t *info)
{
	int ntsc;

	if(!s_cs8556_ready)
		return -1;

	if(info->resx == 720 && (info->resy == 576 || info->resy == 480)) {
		ntsc = (info->resy == 480) ? 1 : 0;

		DPRINT("cs8556_config (%dx%d@%d) %s\n", info->resx, info->resy, info->fps, ntsc ? "NTSC" : "PAL");

		cs8556_set_tv_mode(ntsc);
	} else {

		DPRINT("cs8556_config (%dx%d@%d)\n", info->resx, info->resy, info->fps);
		cs8556_set_tv_mode(-1);
	}

	return 0;
}

static int cs8556_get_edid(char *buf)
{
	return -1;
}

#ifdef CONFIG_CS8556_INTERRUPT
static int cs8556_interrupt(void)
{
	return cs8556_check_plugin(1);
}
#endif

void cs8556_read(void)
{
	int i, ret;
	unsigned char rbuf[256] = {0};

	ret = I2CMultiPageRead(CS8556_ADDR, 0x00, 0x00, 256, rbuf);
	if(!ret) {
		printk("CS8556 Read offset0 data as follows:\n");
		for(i = 0; i < 256;) {
			printk("0x%02X,", rbuf[i]);
			if((++i) % 16 == 0)
			printk("\n");
		}

	}
}

/*----------------------- vout device plugin --------------------------------------*/
vout_dev_t cs8556_vout_dev_ops = {
	.name = CS8556_NAME,
	.mode = VOUT_INF_DVI,

	.init = cs8556_init,
	.set_power_down = cs8556_set_power_down,
	.set_mode = cs8556_set_mode,
	.config = cs8556_config,
	.check_plugin = cs8556_check_plugin,
	.get_edid = cs8556_get_edid,
#ifdef CONFIG_CS8556_INTERRUPT
	.interrupt = cs8556_interrupt,
#endif
};

int cs8556_module_init(void)
{
	vout_device_register(&cs8556_vout_dev_ops);
	return 0;
} /* End of cs8556_module_init */
module_init(cs8556_module_init);
/*--------------------End of Function Body -----------------------------------*/
#undef CS8556_C

