/*++ 
 * linux/drivers/media/video/wmt_v4l2/wmt-vid.c
 * WonderMedia v4l video input device driver
 *
 * Copyright c 2010  WonderMedia  Technologies, Inc.
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

#include <linux/i2c.h>
#include <linux/slab.h> 
#include <linux/module.h>
#include <linux/videodev2.h>
#include "wmt-vid.h"

//#define VID_REG_TRACE
#ifdef VID_REG_TRACE
#define VID_REG_SET32(addr, val)  \
        PRINT("REG_SET:0x%x -> 0x%0x\n", addr, val);\
        REG32_VAL(addr) = (val)
#else
#define VID_REG_SET32(addr, val)      REG32_VAL(addr) = (val)
#endif

//#define VID_DEBUG    /* Flag to enable debug message */
//#define VID_DEBUG_DETAIL
//#define VID_TRACE
//#define VID_INFO

#ifdef VID_DEBUG
#define DBG_MSG(fmt, args...)      PRINT("{%s} " fmt, __FUNCTION__ , ## args)
#else
#define DBG_MSG(fmt, args...)
#endif

#ifdef VID_DEBUG_DETAIL
#define DBG_DETAIL(fmt, args...)   PRINT("{%s} " fmt, __FUNCTION__ , ## args)
#else
#define DBG_DETAIL(fmt, args...)
#endif

#ifdef VID_TRACE
  #define TRACE(fmt, args...)      PRINT("{%s}:  " fmt, __FUNCTION__ , ## args)
#else
  #define TRACE(fmt, args...) 
#endif

#ifdef VID_INFO
  #define DBG_INFO(fmt, args...)      PRINT("[INFO] {%s}:  " fmt, __FUNCTION__ , ## args)
#else
  #define DBG_INFO(fmt, args...) 
#endif

#define DBG_ERR(fmt, args...)      PRINT("*E* {%s} " fmt, __FUNCTION__ , ## args)

#define VID_INT_MODE        

static vid_fb_t *_cur_fb;
static vid_fb_t *_prev_fb;

static spinlock_t vid_lock;
static unsigned int vid_i2c_gpio_en = 0;

static swi2c_reg_t vid_i2c0_scl;
static swi2c_reg_t vid_i2c0_sda;
static swi2c_handle_t vid_swi2c;

static struct i2c_adapter *vid_i2c_adapter[2];
static struct i2c_client *vid_i2c_client[2];

static int vid_dev_tot_num;
static int vid_dev_num ;

#define WMT_VID_I2C_CHANNEL  2

static void vid_gpio_init(vid_mode mode)
{
	auto_pll_divisor(DEV_C24MOUT, CLK_ENABLE, 0, 0); 
	auto_pll_divisor(DEV_VID, CLK_ENABLE, 0, 0); 

	GPIO_CTRL_GP8_VDIN_BYTE_VAL = 0x0;
	PULL_EN_GP8_VDIN_BYTE_VAL = 0x0;

	GPIO_CTRL_GP9_VSYNC_BYTE_VAL  &= ~(BIT0|BIT1|BIT2);
	PULL_EN_GP9_VSYNC_BYTE_VAL &= ~(BIT0|BIT1|BIT2);

	GPIO_CTRL_GP17_I2C_BYTE_VAL &= ~BIT6;	//24Mhz on , not set to GPIO

	return;
}

void vid_set_ntsc_656(void)
{
	VID_REG_SET32( REG_BASE_VID+0x08, 	0x0 );	
	VID_REG_SET32( REG_BASE_VID+0x30, 0x000006b4);	// N_LN_LENGTH : 133m: d1716(6b4h) , 166m: d1715(6b3) 
}

void vid_set_pal_656(void)
{
	VID_REG_SET32( REG_BASE_VID+0x08, 0x0 );	
	VID_REG_SET32( REG_BASE_VID+0x34, 0x000006c0 );	// P_LN_LENGTH : 133m: d1728(6c0h) , 166m: d1727(6bf) 
}

void wmt_vid_set_common_mode(vid_tvsys_e tvsys)	
{
	TRACE("wmt_vid_set_common_mode() \n");

	if (tvsys == VID_NTSC)
		vid_set_ntsc_656();
	else
		vid_set_pal_656();

	VID_REG_SET32( REG_BASE_VID+0x04, 0x100     );	// TMODE[0],REG_UPDATE[8]

	if (tvsys==VID_NTSC) {
		VID_REG_SET32( REG_BASE_VID+0x14, 0x10a0004 );		// VSBB[25:16]:VSBT[9:0]; 139/1=PAL  10A/4=NTSC
		VID_REG_SET32( REG_BASE_VID+0x1c, 0x1370018 );		// PAL_TACTEND[25:16], PAL_TACTBEG[9:0]
		VID_REG_SET32( REG_BASE_VID+0x20, 0x26f0151 );		// PAL_BACTEND[25:16], PAL_BACTBEG[9:0]

		VID_REG_SET32( REG_BASE_VID+0x68, 720        );	// WIDTH[7:0];
		VID_REG_SET32( REG_BASE_VID+0x6C, 768        );	// LN_WIDTH[7:0]
		VID_REG_SET32( REG_BASE_VID+0x70, 480        );	// VID_HEIGHT[9:0]
	} else {
		VID_REG_SET32( REG_BASE_VID+0x14, 0x1390001 );		// VSBB[25:16]:VSBT[9:0]; 139/1=PAL  10A/4=NTSC
		VID_REG_SET32( REG_BASE_VID+0x1c, 0x1360017 );		// PAL_TACTEND[25:16], PAL_TACTBEG[9:0]
		VID_REG_SET32( REG_BASE_VID+0x20, 0x26f0150 );		// PAL_BACTEND[25:16], PAL_BACTBEG[9:0]

		VID_REG_SET32( REG_BASE_VID+0x68, 720        );	// WIDTH[7:0];
		VID_REG_SET32( REG_BASE_VID+0x6C, 768        );	// LN_WIDTH[7:0]
		VID_REG_SET32( REG_BASE_VID+0x70, 576        );	// VID_HEIGHT[9:0]
	}

	VID_REG_SET32( REG_BASE_VID+0x10, 0x20001   );		// HSB[9:0];HSW[9:0];HSP[28]
	VID_REG_SET32( REG_BASE_VID+0x18, 0x3f0002  );		// HVDLY @[25:16];VSW @[9:0];VSP[28]
	VID_REG_SET32( REG_BASE_VID+0x24, 0x1060017 );		// NTSC_TACTEND[25:16],NTSC_TACTBEG[9:0]
	VID_REG_SET32( REG_BASE_VID+0x28, 0x20d011e );		// NTSC_BACTEND[25:16],NTSC_BACTBEG[9:0]

	VID_REG_SET32( REG_BASE_VID+0x78, 0 	     );	// REG_OUTPUT444_EN,( 0:422 format, 1:444 format )
	VID_REG_SET32( REG_BASE_VID+0x7C, 0 	     );	// REG_HSCALE_MODE(0:bypass , 1: horizontal scale 1/2)
	VID_REG_SET32( REG_BASE_VID+0x74, 0x1 	      );	// VID Memory write Enable
	VID_REG_SET32( REG_BASE_VID+0xfc, 0x1 	      );	// BIT SWAP
	//VID_REG_SET32( REG_BASE_VID+0x00, 0x1 	     );	// VID Enable
}


int wmt_vid_i2c_xfer(struct i2c_msg *msg, unsigned int num, int bus_id)
{
	int  ret = 1;
	int i = 0;

	if (num > 1) {
		for (i = 0; i < num - 1; ++i)
			msg[i].flags |= I2C_M_NOSTART;
	}
	ret = i2c_transfer(vid_i2c_adapter[vid_dev_num], msg, num);

	if (ret <= 0) {
		printk("[%s] fail ret %d \n", __func__, ret);
		return ret;
	}

	return ret;
}

int write_array_2_i2c(int slave_addr, char *array_addr, int array_size)
{
	int i=0;
	
	for (i = 0; i < array_size; i += 2) {
		wmt_vid_i2c_write(slave_addr, array_addr[i], array_addr[i+1]);
	}

	return 0;
}

int wmt_vid_i2c_write_page(int chipId ,unsigned int index,char *pdata,int len)
{
    struct i2c_msg msg[1];
    unsigned char buf[4];
    int ret;
    
    buf[0] = index;
    memcpy(&buf[1],pdata,len);
    msg[0].addr = chipId;
    msg[0].flags = 0 ;
    msg[0].flags &= ~(I2C_M_RD);
    msg[0].len = len;
    msg[0].buf = buf;

    ret = wmt_vid_i2c_xfer(msg,1,1);
    
    return ret;
}

int wmt_vid_i2c_read_page(int chipId ,unsigned int index,char *pdata,int len) 
{
    struct i2c_msg msg[2];
    unsigned char buf[4];
    
    memset(buf,0x55,len+1);
    buf[0] = index;

    msg[0].addr = chipId;
    msg[0].flags = 0 ;
    msg[0].flags &= ~(I2C_M_RD);
    msg[0].len = 1;
    msg[0].buf = buf;

    msg[1].addr = chipId;
    msg[1].flags = 0 ;
    msg[1].flags |= (I2C_M_RD);
    msg[1].len = len;
    msg[1].buf = buf;

    wmt_vid_i2c_xfer(msg,2,1);

    memcpy(pdata,buf,len);

    return 0;
}

int wmt_vid_i2c_read(int chipId ,unsigned int index) 
{
	char retval;

	if (vid_i2c_gpio_en) {
		wmt_swi2c_read(&vid_swi2c, chipId*2, index, &retval, 1);
	} else {
		wmt_vid_i2c_read_page( chipId ,index,&retval,1) ;   
	}

	return retval;
}

int wmt_vid_i2c_write(int chipId ,unsigned int index,char data)
{
	if (vid_i2c_gpio_en) {
		wmt_swi2c_write(&vid_swi2c, chipId*2,  index, &data, 2);
	} else {
		wmt_vid_i2c_write_page(chipId ,index, &data, 2);
	}

	return 0;
}

int wmt_vid_i2c_write16addr(int chipId ,unsigned int index,unsigned int data)
{
	int ret=0;
	struct i2c_msg msg[1];
	unsigned char buf[4];

	if (vid_i2c_gpio_en) {
		DBG_ERR("wmt_vid_i2c_write16addr() Not support GPIO mode now");
		return -1;
	}

	buf[0]=((index>>8) & 0xff);
	buf[1]=(index & 0xff);
	buf[2]=(data & 0xff);

	msg[0].addr = chipId;
	msg[0].flags = 0;
	msg[0].flags &= ~(I2C_M_RD);
	msg[0].len = 3;
	msg[0].buf = buf;
	ret = wmt_vid_i2c_xfer(msg,1,1);

	return ret;
}

int wmt_vid_i2c_write16data(int chipId ,unsigned int index,unsigned int data)
{
	int ret=0;
	struct i2c_msg msg[1];
	unsigned char buf[4];

	if (vid_i2c_gpio_en) {
		DBG_ERR("wmt_vid_i2c_write16data() Not support GPIO mode now");
		return -1;
	}

	buf[0]=((index>>8) & 0xff);
	buf[1]=(index & 0xff);
	buf[2]=((data>>8) & 0xff);
	buf[3]=(data & 0xff);

	msg[0].addr = chipId;
	msg[0].flags = 0;
	msg[0].flags &= ~(I2C_M_RD);
	msg[0].len = 4;
	msg[0].buf = buf;
	ret = wmt_vid_i2c_xfer(msg,1,1);

	return ret;
}

int wmt_vid_i2c_read16addr(int chipId ,unsigned int index)
{
	int ret=0;
	struct i2c_msg msg[2];
	unsigned char buf[2];
	unsigned int val;

	if (vid_i2c_gpio_en) {
		DBG_ERR("wmt_vid_i2c_read16addr() Not support GPIO mode now");
		return -1;
	}

	buf[0]=((index>>8) & 0xff);
	buf[1]=(index & 0xff);

	msg[0].addr = chipId;
	msg[0].flags = 0;
	msg[0].flags &= ~(I2C_M_RD);
	msg[0].len = 2;
	msg[0].buf = buf;

	msg[1].addr = chipId;
	msg[1].flags = 0;
	msg[1].flags |= (I2C_M_RD);
	msg[1].len = 1;
	msg[1].buf = buf;

	ret = wmt_vid_i2c_xfer(msg,2,1);

	val=buf[0];
	return val;
}

int wmt_vid_i2c_read16data(int chipId ,unsigned int index)
{
	int ret=0;
	struct i2c_msg msg[2];
	unsigned char buf[2];
	unsigned int val;

	if (vid_i2c_gpio_en) {
		DBG_ERR("wmt_vid_i2c_read16data() Not support GPIO mode now");
		return -1;
	}

	buf[0]=((index>>8) & 0xff);
	buf[1]=(index & 0xff);

	msg[0].addr = chipId;
	msg[0].flags = 0;
	msg[0].flags &= ~(I2C_M_RD);
	msg[0].len = 2;
	msg[0].buf = buf;

	msg[1].addr = chipId;
	msg[1].flags = 0;
	msg[1].flags |= (I2C_M_RD);
	msg[1].len = 2;
	msg[1].buf = buf;

	ret = wmt_vid_i2c_xfer(msg,2,1);

	//printk("wmt_vid_i2c_read16data(%x,%x)\n",buf[0],buf[1]);

	val = ((buf[0]&0xff)<<8) | (buf[1]&0xff);

	return val;
}

//---dev_0---------------------------------------------

struct wmt_vid_i2c_priv_0 {
	unsigned int var;
};

static int __devinit wmt_vid_i2c_probe_0(struct i2c_client *i2c,
			    const struct i2c_device_id *id)
{
	struct wmt_vid_i2c_priv *wmt_vid_i2c;
	int ret=0;

	printk("wmt_vid_i2c_probe_0() \n");
	if (!i2c_check_functionality(vid_i2c_client[0]->adapter, I2C_FUNC_I2C)) {
		printk(KERN_ERR "wmt_vid_i2c_probe: need I2C_FUNC_I2C\n");
		return -ENODEV;
	}

	wmt_vid_i2c = kzalloc(sizeof(struct wmt_vid_i2c_priv_0), GFP_KERNEL);
	if (wmt_vid_i2c == NULL) {
		printk("wmt_vid_i2c_probe kzalloc fail");
		return -ENOMEM;
	}
	i2c_set_clientdata(i2c, wmt_vid_i2c);

	return ret;
}

static int  wmt_vid_i2c_remove_0(struct i2c_client *client)
{
	kfree(i2c_get_clientdata(vid_i2c_client[0]));
	return 0;
}

static const struct i2c_device_id wmt_vid_i2c_id_0[] = {
	{ "wmt_vid_i2c_0", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, wmt_vid_i2c_id_0);

static struct i2c_driver wmt_vid_i2c_driver_0 = {
	.probe = wmt_vid_i2c_probe_0,
	.remove = wmt_vid_i2c_remove_0,
	.id_table = wmt_vid_i2c_id_0,
	.driver = {
		.name = "wmt_vid_i2c_0",
	},
};

static struct i2c_board_info wmt_vid_i2c_board_info_0 = {
	.type          = "wmt_vid_i2c_0",
	.flags         = 0x00,
	.platform_data = NULL,
	.archdata      = NULL,
	.irq           = -1,
};

//---dev_1---------------------------------------------
struct wmt_vid_i2c_priv_1 {
	unsigned int var;
};

static int __devinit wmt_vid_i2c_probe_1(struct i2c_client *i2c,
					 const struct i2c_device_id *id)
{
	struct wmt_vid_i2c_priv *wmt_vid_i2c;
	int ret=0;

	printk("wmt_vid_i2c_probe_1() \n");

	if (!i2c_check_functionality(vid_i2c_client[1]->adapter, I2C_FUNC_I2C)) {
		printk(KERN_ERR "wmt_vid_i2c_probe: need I2C_FUNC_I2C\n");
		return -ENODEV;
	}

	wmt_vid_i2c = kzalloc(sizeof(struct wmt_vid_i2c_priv_1), GFP_KERNEL);
	if (wmt_vid_i2c == NULL) {
		printk("wmt_vid_i2c_probe kzalloc fail");
		return -ENOMEM;
	}

	i2c_set_clientdata(i2c, wmt_vid_i2c);

	return ret;
}

static int  wmt_vid_i2c_remove_1(struct i2c_client *client)
{
	kfree(i2c_get_clientdata(vid_i2c_client[1]));
	return 0;
}

static const struct i2c_device_id wmt_vid_i2c_id_1[] = {
	{ "wmt_vid_i2c_1", 0},
	{ }
};
MODULE_DEVICE_TABLE(i2c, wmt_vid_i2c_id_1);

static struct i2c_driver wmt_vid_i2c_driver_1 = {
	.probe = wmt_vid_i2c_probe_1,
	.remove = wmt_vid_i2c_remove_1,
	.id_table = wmt_vid_i2c_id_1,
	.driver = { .name = "wmt_vid_i2c_1", },
};

static struct i2c_board_info wmt_vid_i2c_board_info_1 = {
	.type          = "wmt_vid_i2c_1",
	.flags         = 0x00,
	.platform_data = NULL,
	.archdata      = NULL,
	.irq           = -1,
};

//---------------------------------------------------------
int wmt_vid_i2c_set_same_dev(void)
{
	vid_dev_tot_num = 2;
	vid_i2c_adapter[1] = vid_i2c_adapter[0];
	vid_i2c_client[1] = vid_i2c_client[0];

	return 0;
}

int wmt_vid_i2c_init(int dev_num, short dev_id)
{
	struct i2c_board_info *wmt_vid_i2c_bi ;
	struct i2c_adapter *adapter;
	struct i2c_client *client;

	int ret =0;

	printk("wmt_vid_i2c_init() dev_num %d  dev_id 0x%02x \n",dev_num,dev_id);

	if (dev_num == 0)
		wmt_vid_i2c_bi = &wmt_vid_i2c_board_info_0;
	else
		wmt_vid_i2c_bi = &wmt_vid_i2c_board_info_1;

	adapter = i2c_get_adapter(WMT_VID_I2C_CHANNEL);
	if (adapter == NULL) {
		printk("can not get i2c adapter, client address error");
		return -ENODEV;
	}

	wmt_vid_i2c_bi->addr = dev_id;

	client = i2c_new_device(adapter, wmt_vid_i2c_bi);
	if ( client == NULL) {
		printk("allocate i2c client failed");
		return -ENOMEM;
	}

	vid_dev_tot_num = dev_num+1;
	vid_i2c_client[dev_num] = client;
	vid_i2c_adapter[dev_num] = adapter;

	i2c_put_adapter(adapter);
	if (dev_num == 0)
		ret = i2c_add_driver(&wmt_vid_i2c_driver_0);
	else
		ret = i2c_add_driver(&wmt_vid_i2c_driver_1);

	if (ret) {
		printk("i2c_add_driver fail");
	}	

	return ret;
}

int wmt_vid_i2c_release(void)
{
	int i;

	for (i = 0; i < vid_dev_tot_num; i++) {
		i2c_unregister_device(vid_i2c_client[i]);
		if (i == 0) {
			wmt_vid_i2c_remove_0(vid_i2c_client[i]);
			i2c_del_driver(&wmt_vid_i2c_driver_0);
		} else {
			wmt_vid_i2c_remove_1(vid_i2c_client[i]);
			i2c_del_driver(&wmt_vid_i2c_driver_1);
		}
	}

	return 0;
}

int wmt_vid_set_mode(int width, int height, unsigned int pix_format)
{
	TRACE("Enter\n");

	VID_REG_SET32(REG_VID_WIDTH, width); /* VID output width */
	VID_REG_SET32(REG_VID_LINE_WIDTH, width);
	VID_REG_SET32(REG_VID_HEIGHT, height);    /* VID output height */

	if (pix_format == V4L2_PIX_FMT_YUYV) // YUV422
	{
		DBG_INFO("V4L2_PIX_FMT_YUYV \n");
		VID_REG_SET32( REG_BASE_VID+0x90, 0x0);    //CMOS Sensor Input mode, set to YUV
		VID_REG_SET32( REG_BASE_VID+0x98, 0x0);    //CMOS RGB Output Mode, only RGB565 need set to '1'
		VID_REG_SET32( REG_BASE_VID+0x78, 0x0);    //REG_OUTPUT444_EN, set to YUV 422
	}
	else if (pix_format == V4L2_PIX_FMT_YUV444) {
		DBG_INFO("V4L2_PIX_FMT_YUYV \n");
		VID_REG_SET32( REG_BASE_VID+0x90, 0x0);    //CMOS Sensor Input mode, set to YUV
		VID_REG_SET32( REG_BASE_VID+0x98, 0x0);    //CMOS RGB Output Mode, only RGB565 need set to '1'
		VID_REG_SET32( REG_BASE_VID+0x78, 0x1);    //REG_OUTPUT444_EN, set to YUV 444
	}
	else if (pix_format == V4L2_PIX_FMT_RGB565) {
		DBG_INFO("V4L2_PIX_FMT_RGB565 \n");
		VID_REG_SET32( REG_BASE_VID+0x90, 0x1);    //CMOS Sensor Input mode, Set to RGB
		VID_REG_SET32( REG_BASE_VID+0x98, 0x1);    //CMOS RGB Output Mode, only RGB565 need set to '1'
	}
	else if (pix_format == V4L2_PIX_FMT_RGB32) {
		DBG_INFO("V4L2_PIX_FMT_RGB888 \n");
		VID_REG_SET32( REG_BASE_VID+0x90, 0x1);    //CMOS Sensor Input mode, Set to RGB
		VID_REG_SET32( REG_BASE_VID+0x98, 0x0);    //CMOS RGB Output Mode , only RGB565 need set to '1'
	}
	else if (pix_format == V4L2_PIX_FMT_NV12) {
		DBG_INFO("V4L2_PIX_FMT_NV12 \n");
		VID_REG_SET32( REG_BASE_VID+0x90, 0x0);    //CMOS Sensor Input mode, set to YUV
		VID_REG_SET32( REG_BASE_VID+0x98, 0x0);    //CMOS RGB Output Mode, only RGB565 need set to '1'
		VID_REG_SET32( REG_BASE_VID+0x78, 0x10);
		VID_REG_SET32( REG_BASE_VID+0x74, 0x01);
	}
	else if (pix_format == V4L2_PIX_FMT_NV21) {
		DBG_INFO("V4L2_PIX_FMT_NV21 \n");
		VID_REG_SET32( REG_BASE_VID+0x90, 0x0);    //CMOS Sensor Input mode, set to YUV
		VID_REG_SET32( REG_BASE_VID+0x98, 0x0);    //CMOS RGB Output Mode, only RGB565 need set to '1'
		VID_REG_SET32( REG_BASE_VID+0x78, 0x10);
		VID_REG_SET32( REG_BASE_VID+0x74, 0x11);
	}
	else {
		pix_format = V4L2_PIX_FMT_YUYV;
		DBG_INFO("V4L2_PIX_unknow , set to V4L2_PIX_FMT_YUYV \n");
	}

	TRACE("Leave\n");

	return 0;
}

int wmt_vid_set_cur_fb(vid_fb_t *fb)
{
	unsigned long flags =0;

	spin_lock_irqsave(&vid_lock, flags);

	_prev_fb = _cur_fb;
	_cur_fb  = fb;

	fb->is_busy = 1;
	fb->done    = 0;
	wmt_vid_set_addr(fb->y_addr, fb->c_addr);		    

	spin_unlock_irqrestore(&vid_lock, flags);

	return 0;
}

vid_fb_t *wmt_vid_get_cur_fb(void)
{
	unsigned long flags =0;
	vid_fb_t *fb;    

	spin_lock_irqsave(&vid_lock, flags);
	fb = _cur_fb;
	spin_unlock_irqrestore(&vid_lock, flags);

	return fb;
}

int wmt_vid_set_addr(unsigned int y_addr, unsigned int c_addr)
{
	unsigned int  cur_y_addr;   // temp
	unsigned int  cur_c_addr;   // temp

	TRACE("Enter\n");

	cur_y_addr = REG32_VAL(REG_VID_Y0_SA);
	cur_c_addr = REG32_VAL(REG_VID_C0_SA);

	if ((y_addr != cur_y_addr) || (c_addr != cur_c_addr)) {
		VID_REG_SET32( REG_VID_Y0_SA, y_addr); /* VID Y FB address */
		VID_REG_SET32( REG_VID_C0_SA, c_addr); /* VID C FB address */
	}

	TRACE("Leave\n");
	return 0;
}

int wmt_vid_open(vid_mode mode, cmos_uboot_env_t *uboot_env)
{
	int value, int_ctrl;
	TRACE("Enter\n");

	//vid_i2c_gpio_en = uboot_env->i2c_gpio_en;
	DBG_INFO(" vid_i2c_gpio_en 0x%08x \n",vid_i2c_gpio_en);
	if ((vid_i2c_gpio_en) && (uboot_env != NULL))
	{
		memset(&vid_i2c0_scl, 0, sizeof(vid_i2c0_scl));
		memset(&vid_i2c0_sda, 0, sizeof(vid_i2c0_sda));

		vid_i2c0_scl.bit_mask = 1 << uboot_env->i2c_gpio_scl_binum;
		vid_i2c0_scl.pull_en_bit_mask = 1 << uboot_env->reg_i2c_gpio_scl_gpio_pe_bitnum;

		vid_i2c0_scl.data_in = uboot_env->reg_i2c_gpio_scl_gpio_in ;
		vid_i2c0_scl.gpio_en = uboot_env->reg_i2c_gpio_scl_gpio_en ;
		vid_i2c0_scl.out_en = uboot_env->reg_i2c_gpio_scl_gpio_od ;
		vid_i2c0_scl.data_out = uboot_env->reg_i2c_gpio_scl_gpio_oc ;
		vid_i2c0_scl.pull_en = uboot_env->reg_i2c_gpio_scl_gpio_pe ;


		vid_i2c0_sda.bit_mask =  1 << uboot_env->i2c_gpio_sda_binum;
		vid_i2c0_sda.pull_en_bit_mask = 1 << uboot_env->reg_i2c_gpio_sda_gpio_pe_bitnum;

		vid_i2c0_sda.data_in = uboot_env->reg_i2c_gpio_sda_gpio_in ;
		vid_i2c0_sda.gpio_en = uboot_env->reg_i2c_gpio_sda_gpio_en ;
		vid_i2c0_sda.out_en = uboot_env->reg_i2c_gpio_sda_gpio_od ;
		vid_i2c0_sda.data_out = uboot_env->reg_i2c_gpio_sda_gpio_oc ;
		vid_i2c0_sda.pull_en = uboot_env->reg_i2c_gpio_sda_gpio_pe ;

		if (vid_i2c0_scl.data_in & 0x1) {
			vid_i2c0_scl.bit_mask <<= 8;
			vid_i2c0_scl.pull_en_bit_mask <<= 8;
			vid_i2c0_scl.data_in -= 1;
			vid_i2c0_scl.gpio_en -= 1;
			vid_i2c0_scl.out_en -= 1;
			vid_i2c0_scl.data_out -= 1;
			vid_i2c0_scl.pull_en -= 1;

		}
		if (vid_i2c0_sda.data_in & 0x1) {
			vid_i2c0_sda.bit_mask <<= 8;
			vid_i2c0_sda.pull_en_bit_mask <<= 8;
			vid_i2c0_sda.data_in -= 1;
			vid_i2c0_sda.gpio_en -= 1;
			vid_i2c0_sda.out_en -= 1;
			vid_i2c0_sda.data_out -= 1;
			vid_i2c0_sda.pull_en -= 1;
		}

		vid_swi2c.scl_reg = &vid_i2c0_scl;
		vid_swi2c.sda_reg = &vid_i2c0_sda;

		DBG_INFO("SCL 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x \n",
			 vid_i2c0_scl.bit_mask, vid_i2c0_scl.pull_en_bit_mask,
			 vid_i2c0_scl.data_in, vid_i2c0_scl.gpio_en,
			 vid_i2c0_scl.out_en, vid_i2c0_scl.data_out,
			 vid_i2c0_scl.pull_en);
		DBG_INFO("SDA 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x \n",
			 vid_i2c0_sda.bit_mask, vid_i2c0_sda.pull_en_bit_mask,
			 vid_i2c0_sda.data_in, vid_i2c0_sda.gpio_en,
			 vid_i2c0_sda.out_en, vid_i2c0_sda.data_out,
			 vid_i2c0_sda.pull_en);
	}

	/*--------------------------------------------------------------------------
	  Step 1: Init GPIO for CMOS or TVDEC mode
	  --------------------------------------------------------------------------*/
	vid_gpio_init(mode);

	/*--------------------------------------------------------------------------
	  Step 2: Init CMOS or TVDEC module
	  --------------------------------------------------------------------------*/
	value = REG32_VAL(REG_VID_TVDEC_CTRL);
	VID_REG_SET32( REG_VID_MEMIF_EN, 0x1 );
	VID_REG_SET32( REG_VID_OUTPUT_FORMAT, 0x0 );  // 0: 422   1: 444

	int_ctrl = 0x00;
	if (mode == VID_MODE_CMOS) {
		VID_REG_SET32( REG_VID_TVDEC_CTRL, (value & 0xFFFFFFE)); /* disable TV decoder */
		VID_REG_SET32( REG_VID_CMOS_PIXEL_SWAP, 0x2);    /* 0x2 for YUYV */
#ifdef VID_INT_MODE
		int_ctrl = 0x0808;
#endif
		VID_REG_SET32( REG_VID_INT_CTRL, int_ctrl );

	}
	else {
		VID_REG_SET32( REG_VID_TVDEC_CTRL, (value | 0x1) );	/* enable TV decoder */
#ifdef VID_INT_MODE
		int_ctrl = 0x0404;
#endif
		VID_REG_SET32( REG_VID_INT_CTRL, int_ctrl );
		VID_REG_SET32( REG_VID_CMOS_EN, 0x0);	/* disable CMOS */
		wmt_vid_set_common_mode(VID_NTSC);

	}

	_cur_fb  = 0;
	_prev_fb = 0;

	spin_lock_init(&vid_lock);

	TRACE("Leave\n");

	return 0;
}

int wmt_vid_close(vid_mode mode)
{
	TRACE("Enter\n");

	auto_pll_divisor(DEV_VID, CLK_DISABLE, 0, 0); 
	auto_pll_divisor(DEV_C24MOUT, CLK_DISABLE, 0, 0); 

	GPIO_CTRL_GP17_I2C_BYTE_VAL |= BIT6;//24Mhz off , set to GPIO

	if(mode == VID_MODE_CMOS) {
		VID_REG_SET32( REG_VID_CMOS_EN, 0x0);	/* disable CMOS */
	}
	else {
		int value = REG32_VAL(REG_VID_TVDEC_CTRL);
		VID_REG_SET32( REG_VID_TVDEC_CTRL, (value & 0xFFFFFFE)); /* disable TV decoder */
	}
	TRACE("Leave\n");

	return 0;
}

EXPORT_SYMBOL(wmt_vid_set_mode);
EXPORT_SYMBOL(wmt_vid_i2c_write);
EXPORT_SYMBOL(wmt_vid_i2c_read);
EXPORT_SYMBOL(wmt_vid_i2c_write16addr);
EXPORT_SYMBOL(wmt_vid_i2c_read16addr);
EXPORT_SYMBOL(wmt_vid_i2c_write16data);
EXPORT_SYMBOL(wmt_vid_i2c_read16data);
EXPORT_SYMBOL(wmt_vid_set_cur_fb);
EXPORT_SYMBOL(wmt_vid_get_cur_fb);
EXPORT_SYMBOL(wmt_vid_set_addr);
EXPORT_SYMBOL(wmt_vid_close);
EXPORT_SYMBOL(wmt_vid_open);
EXPORT_SYMBOL(wmt_vid_set_common_mode);
EXPORT_SYMBOL(wmt_vid_i2c_init);
EXPORT_SYMBOL(wmt_vid_i2c_release);

MODULE_AUTHOR("WonderMedia SW Team Max Chen");
MODULE_DESCRIPTION("wmt-vid  driver");
MODULE_LICENSE("GPL");

