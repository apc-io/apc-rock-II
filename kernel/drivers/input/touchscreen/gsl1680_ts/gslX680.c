/*
 * drivers/input/touchscreen/gslX680.c
 *
 * Copyright (c) 2012 Shanghai Basewin
 *	Guan Yuwei<guanyuwei@basewin.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 */



#include <linux/module.h>
#include <linux/slab.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/delay.h>
//#include <mach/gpio.h>
//#include <mach/gpio_data.h>
#include <linux/jiffies.h>
#include <linux/cdev.h>
#include <asm/uaccess.h>
#include <linux/pm_runtime.h>

#if defined(CONFIG_HAS_EARLYSUSPEND)
#include <linux/earlysuspend.h>
#endif
#include <linux/input/mt.h>

#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/async.h>
#include <linux/hrtimer.h>
#include <linux/init.h>
#include <linux/ioport.h>
#include <linux/irq.h>
#include <linux/kthread.h>
#include <linux/wakelock.h>


//#include <asm/irq.h>
//#include <asm/io.h>

//#include <mach/irqs.h>
//#include <mach/system.h>
//#include <mach/hardware.h>
#include "gslX680.h"
#include "wmt_ts.h"
#include "../../../video/backlight/wmt_bl.h"

//#define GSL_DEBUG
//#define GSL_TIMER
//#define REPORT_DATA_ANDROID_4_0

#define HAVE_TOUCH_KEY

#define SCREEN_MAX_X 		480
#define SCREEN_MAX_Y 		800


#define GSLX680_I2C_NAME 	"touch_gslX680"
#define GSLX680_I2C_ADDR 	0x40
#define IRQ_PORT			INT_GPIO_0

#define GSL_DATA_REG		0x80
#define GSL_STATUS_REG		0xe0
#define GSL_PAGE_REG		0xf0

#define PRESS_MAX    		255
#define MAX_FINGERS 		5
#define MAX_CONTACTS 		10
#define DMA_TRANS_LEN		0x20

#ifdef	GSL_NOID_VERSION
int gsl_noid_ver = 0;
unsigned int gsl_config_data_id[512] = {0}; 
#endif

#ifdef HAVE_TOUCH_KEY
static u16 key = 0;
static int key_state_flag = 0;
struct key_data {
	u16 key;
	u16 x_min;
	u16 x_max;
	u16 y_min;
	u16 y_max;	
};

const u16 key_array[]={
                                      KEY_BACK,
                                      KEY_HOME,
                                      KEY_MENU,
                                      KEY_SEARCH,
                                     }; 
#define MAX_KEY_NUM     (sizeof(key_array)/sizeof(key_array[0]))

struct key_data gsl_key_data[MAX_KEY_NUM] = {
	{KEY_BACK, 2048, 2048, 2048, 2048},
	{KEY_HOME, 2048, 2048, 2048, 2048},	
	{KEY_MENU, 2048, 2048, 2048, 2048},
	{KEY_SEARCH, 2048, 2048, 2048, 2048},
};
#endif

struct gsl_ts_data {
	u8 x_index;
	u8 y_index;
	u8 z_index;
	u8 id_index;
	u8 touch_index;
	u8 data_reg;
	u8 status_reg;
	u8 data_size;
	u8 touch_bytes;
	u8 update_data;
	u8 touch_meta_data;
	u8 finger_size;
};

static struct gsl_ts_data devices[] = {
	{
		.x_index = 6,
		.y_index = 4,
		.z_index = 5,
		.id_index = 7,
		.data_reg = GSL_DATA_REG,
		.status_reg = GSL_STATUS_REG,
		.update_data = 0x4,
		.touch_bytes = 4,
		.touch_meta_data = 4,
		.finger_size = 70,
	},
};

struct gsl_ts {
	struct i2c_client *client;
	struct input_dev *input;
	struct work_struct work;
	struct workqueue_struct *wq;
	struct gsl_ts_data *dd;
	u8 *touch_data;
	u8 device_id;
	u8 prev_touches;
	bool is_suspended;
	bool int_pending;
	struct mutex sus_lock;
//	uint32_t gpio_irq;
	int irq;
#if defined(CONFIG_HAS_EARLYSUSPEND)
	struct early_suspend early_suspend;
#endif
#ifdef GSL_TIMER
	struct timer_list gsl_timer;
#endif

};

struct gsl_ts *l_ts=NULL;

static u32 id_sign[MAX_CONTACTS+1] = {0};
static u8 id_state_flag[MAX_CONTACTS+1] = {0};
static u8 id_state_old_flag[MAX_CONTACTS+1] = {0};
static u16 x_old[MAX_CONTACTS+1] = {0};
static u16 y_old[MAX_CONTACTS+1] = {0};
static u16 x_new = 0;
static u16 y_new = 0;

static struct fw_data* GSLX680_FW = NULL;
static int l_fwlen = 0;
static struct task_struct *resume_download_task;
static struct wake_lock downloadWakeLock;
static int is_delay = 0;

extern int register_bl_notifier(struct notifier_block *nb);

extern int unregister_bl_notifier(struct notifier_block *nb);

extern int wmt_getsyspara(char *varname, unsigned char *varval, int *varlen);

////////////////////////////////////////////////////////////////////
static int wmt_get_fwdata(void);

////////////////////////////////////////////////////////////////////
static int gslX680_chip_init(void)
{
	//gpio_set_status(PAD_GPIOA_6, gpio_status_out);
	//gpio_out(PAD_GPIOA_6, 1);
	// shutdown pin
	wmt_rst_output(1);
	// irq pin
	//gpio_set_status(PAD_GPIOA_16, gpio_status_in);
	//gpio_irq_set(PAD_GPIOA_16, GPIO_IRQ(INT_GPIO_0-INT_GPIO_0, GPIO_IRQ_RISING));
	wmt_set_gpirq(IRQ_TYPE_EDGE_RISING);//GIRQ_FALLING);
	wmt_disable_gpirq();
	msleep(20);
	return 0;
}

static int gslX680_shutdown_low(void)
{
	//gpio_set_status(PAD_GPIOA_6, gpio_status_out);
	//gpio_out(PAD_GPIOA_6, 0);
	wmt_rst_output(0);
	return 0;
}

static int gslX680_shutdown_high(void)
{
	//gpio_set_status(PAD_GPIOA_6, gpio_status_out);
	//gpio_out(PAD_GPIOA_6, 1);
	wmt_rst_output(1);
	return 0;
}

static inline u16 join_bytes(u8 a, u8 b)
{
	u16 ab = 0;
	ab = ab | a;
	ab = ab << 8 | b;
	return ab;
}

#if 0
static u32 gsl_read_interface(struct i2c_client *client, u8 reg, u8 *buf, u32 num)
{
	struct i2c_msg xfer_msg[2];

	xfer_msg[0].addr = client->addr;
	xfer_msg[0].len = 1;
	xfer_msg[0].flags = client->flags & I2C_M_TEN;
	xfer_msg[0].buf = &reg;

	xfer_msg[1].addr = client->addr;
	xfer_msg[1].len = num;
	xfer_msg[1].flags |= I2C_M_RD;
	xfer_msg[1].buf = buf;

	if (reg < 0x80) {
		i2c_transfer(client->adapter, xfer_msg, ARRAY_SIZE(xfer_msg));
		msleep(5);
	}

	return i2c_transfer(client->adapter, xfer_msg, ARRAY_SIZE(xfer_msg)) == ARRAY_SIZE(xfer_msg) ? 0 : -EFAULT;
}
#endif

static u32 gsl_write_interface(struct i2c_client *client, const u8 reg, u8 *buf, u32 num)
{
	struct i2c_msg xfer_msg[1];

	buf[0] = reg;

	xfer_msg[0].addr = client->addr;
	xfer_msg[0].len = num + 1;
	xfer_msg[0].flags = client->flags & I2C_M_TEN;
	xfer_msg[0].buf = buf;

	return i2c_transfer(client->adapter, xfer_msg, 1) == 1 ? 0 : -EFAULT;
}

static __inline__ void fw2buf(u8 *buf, const u32 *fw)
{
	u32 *u32_buf = (int *)buf;
	*u32_buf = *fw;
}

static int wmt_get_fwdata(void)
{
	char fwname[128];
	
	// get the firmware file name
	memset(fwname,0,sizeof(fwname));
	wmt_ts_get_firmwfilename(fwname);
	// load the data into GSLX680_FW
	l_fwlen = read_firmwfile(fwname, &GSLX680_FW, gsl_config_data_id);
	return ((l_fwlen>0)?0:-1);
}

static void gsl_load_fw(struct i2c_client *client)
{
	u8 buf[DMA_TRANS_LEN*4 + 1] = {0};
	u8 send_flag = 1;
	u8 *cur = buf + 1;
	u32 source_line = 0;
	u32 source_len = l_fwlen;//ARRAY_SIZE(GSLX680_FW);

	printk("=============gsl_load_fw start==============\n");

	for (source_line = 0; source_line < source_len; source_line++) 
	{
		/* init page trans, set the page val */
		if (GSL_PAGE_REG == GSLX680_FW[source_line].offset)
		{
			fw2buf(cur, &GSLX680_FW[source_line].val);
			gsl_write_interface(client, GSL_PAGE_REG, buf, 4);
			send_flag = 1;
		}
		else 
		{
			if (1 == send_flag % (DMA_TRANS_LEN < 0x20 ? DMA_TRANS_LEN : 0x20))
	    			buf[0] = (u8)GSLX680_FW[source_line].offset;

			fw2buf(cur, &GSLX680_FW[source_line].val);
			cur += 4;

			if (0 == send_flag % (DMA_TRANS_LEN < 0x20 ? DMA_TRANS_LEN : 0x20)) 
			{
	    			gsl_write_interface(client, buf[0], buf, cur - buf - 1);
	    			cur = buf + 1;
			}

			send_flag++;
		}
	}

	printk("=============gsl_load_fw end==============\n");

}


static int gsl_ts_write(struct i2c_client *client, u8 addr, u8 *pdata, int datalen)
{
	int ret = 0;
	u8 tmp_buf[128];
	unsigned int bytelen = 0;
	if (datalen > 125)
	{
		dbg("%s too big datalen = %d!\n", __func__, datalen);
		return -1;
	}
	
	tmp_buf[0] = addr;
	bytelen++;
	
	if (datalen != 0 && pdata != NULL)
	{
		memcpy(&tmp_buf[bytelen], pdata, datalen);
		bytelen += datalen;
	}
	
	ret = i2c_master_send(client, tmp_buf, bytelen);
	return ret;
}

static int gsl_ts_read(struct i2c_client *client, u8 addr, u8 *pdata, unsigned int datalen)
{
	int ret = 0;

	if (datalen > 126)
	{
		dbg("%s too big datalen = %d!\n", __func__, datalen);
		return -1;
	}

	ret = gsl_ts_write(client, addr, NULL, 0);
	if (ret < 0)
	{
		dbg("%s set data address fail!\n", __func__);
		return ret;
	}
	
	return i2c_master_recv(client, pdata, datalen);
}

#if 0
static void test_i2c(struct i2c_client *client)
{
	u8 read_buf = 0;
	u8 write_buf = 0x12;
	int ret;
	ret = gsl_ts_read( client, 0xf0, &read_buf, sizeof(read_buf) );
	if  (ret  < 0)  
	{
		pr_info("I2C transfer error!\n");
	}
	else
	{
		pr_info("I read reg 0xf0 is %x\n", read_buf);
	}
	msleep(10);

	ret = gsl_ts_write(client, 0xf0, &write_buf, sizeof(write_buf));
	if  (ret  < 0)  
	{
		pr_info("I2C transfer error!\n");
	}
	else
	{
		pr_info("I write reg 0xf0 0x12\n");
	}
	msleep(10);

	ret = gsl_ts_read( client, 0xf0, &read_buf, sizeof(read_buf) );
	if  (ret  <  0 )
	{
		pr_info("I2C transfer error!\n");
	}
	else
	{
		pr_info("I read reg 0xf0 is 0x%x\n", read_buf);
	}
	msleep(10);

}
#endif

static int test_i2c(struct i2c_client *client)
{
	u8 read_buf = 0;
	u8 write_buf = 0x12;
	int ret, rc = 1;
	
	ret = gsl_ts_read( client, 0xf0, &read_buf, sizeof(read_buf) );
	if  (ret  < 0)  
    		rc --;
	else
		dbg("I read reg 0xf0 is %x\n", read_buf);
	
	msleep(2);
	ret = gsl_ts_write(client, 0xf0, &write_buf, sizeof(write_buf));
	if(ret  >=  0 )
		dbg("I write reg 0xf0 0x12\n");
	
	msleep(2);
	ret = gsl_ts_read( client, 0xf0, &read_buf, sizeof(read_buf) );
	if(ret <  0 )
		rc --;
	else
		dbg("I read reg 0xf0 is 0x%x\n", read_buf);

	return rc;
}

static void startup_chip(struct i2c_client *client)
{
	u8 tmp = 0x00;
#ifdef GSL_NOID_VERSION
	if (gsl_noid_ver)
		gsl_DataInit(gsl_config_data_id);
#endif
	gsl_ts_write(client, 0xe0, &tmp, 1);
	msleep(10);	
}

static void reset_chip(struct i2c_client *client)
{
	u8 buf[4] = {0x00};
	u8 tmp = 0x88;
	gsl_ts_write(client, 0xe0, &tmp, sizeof(tmp));
	msleep(10);

	tmp = 0x04;
	gsl_ts_write(client, 0xe4, &tmp, sizeof(tmp));
	msleep(10);

	gsl_ts_write(client, 0xbc, buf, sizeof(buf));
	msleep(10);
}

static void clr_reg(struct i2c_client *client)
{
	u8 write_buf[4]	= {0};

	write_buf[0] = 0x88;
	gsl_ts_write(client, 0xe0, &write_buf[0], 1); 	
	msleep(20);
	write_buf[0] = 0x01;
	gsl_ts_write(client, 0x80, &write_buf[0], 1); 	
	msleep(5);
	write_buf[0] = 0x04;
	gsl_ts_write(client, 0xe4, &write_buf[0], 1); 	
	msleep(5);
	write_buf[0] = 0x00;
	gsl_ts_write(client, 0xe0, &write_buf[0], 1); 	
	msleep(20);
}

static void init_chip(struct i2c_client *client)
{
	int rc;
	
	gslX680_shutdown_low();	
	msleep(20); 	
	gslX680_shutdown_high();	
	msleep(20); 		
	rc = test_i2c(client);
	if(rc < 0)
	{
		printk("------gslX680 test_i2c error------\n");	
		return;
	}	
	clr_reg(client);
	reset_chip(client);
	gsl_load_fw(client);			
	startup_chip(client);	
	reset_chip(client);	
	startup_chip(client);	
}

static void check_mem_data(struct i2c_client *client)
{
	/*char write_buf;
	char read_buf[4]  = {0};
	
	msleep(30);
	write_buf = 0x00;
	gsl_ts_write(client,0xf0, &write_buf, sizeof(write_buf));
	gsl_ts_read(client,0x00, read_buf, sizeof(read_buf));
	gsl_ts_read(client,0x00, read_buf, sizeof(read_buf));
	if (read_buf[3] != 0x1 || read_buf[2] != 0 || read_buf[1] != 0 || read_buf[0] != 0)
	{
		dbg("!!!!!!!!!!!page: %x offset: %x val: %x %x %x %x\n",0x0, 0x0, read_buf[3], read_buf[2], read_buf[1], read_buf[0]);
		init_chip(client);
	}*/

	u8 read_buf[4]  = {0};
	
	msleep(30);
	gsl_ts_read(client,0xb0, read_buf, sizeof(read_buf));
	
	if (read_buf[3] != 0x5a || read_buf[2] != 0x5a || read_buf[1] != 0x5a || read_buf[0] != 0x5a)
	{
		printk("#########check mem read 0xb0 = %x %x %x %x #########\n", read_buf[3], read_buf[2], read_buf[1], read_buf[0]);
		init_chip(client);
	}

}

static void record_point(u16 x, u16 y , u8 id)
{
	u16 x_err =0;
	u16 y_err =0;

	id_sign[id]=id_sign[id]+1;
	
	if(id_sign[id]==1){
		x_old[id]=x;
		y_old[id]=y;
	}

	x = (x_old[id] + x)/2;
	y = (y_old[id] + y)/2;
		
	if(x>x_old[id]){
		x_err=x -x_old[id];
	}
	else{
		x_err=x_old[id]-x;
	}

	if(y>y_old[id]){
		y_err=y -y_old[id];
	}
	else{
		y_err=y_old[id]-y;
	}

	if( (x_err > 3 && y_err > 1) || (x_err > 1 && y_err > 3) ){
		x_new = x;     x_old[id] = x;
		y_new = y;     y_old[id] = y;
	}
	else{
		if(x_err > 3){
			x_new = x;     x_old[id] = x;
		}
		else
			x_new = x_old[id];
		if(y_err> 3){
			y_new = y;     y_old[id] = y;
		}
		else
			y_new = y_old[id];
	}

	if(id_sign[id]==1){
		x_new= x_old[id];
		y_new= y_old[id];
	}
	
}

void wmt_set_keypos(int index,int xmin,int xmax,int ymin,int ymax)
{
	gsl_key_data[index].x_min = xmin;
	gsl_key_data[index].x_max = xmax;
	gsl_key_data[index].y_min = ymin;
	gsl_key_data[index].y_max = ymax;
}

#ifdef HAVE_TOUCH_KEY
static void report_key(struct gsl_ts *ts, u16 x, u16 y)
{
	u16 i = 0;

	for(i = 0; i < MAX_KEY_NUM; i++) 
	{
		if((gsl_key_data[i].x_min < x) && (x < gsl_key_data[i].x_max)&&(gsl_key_data[i].y_min < y) && (y < gsl_key_data[i].y_max))
		{
			key = gsl_key_data[i].key;	
			input_report_key(ts->input, key, 1);
			input_sync(ts->input); 		
			key_state_flag = 1;
			dbg("rport key:%d\n",key);
			break;
		}
	}
}
#endif

static void report_data(struct gsl_ts *ts, u16 x, u16 y, u8 pressure, u8 id)
{
	//swap(x, y);
	int tx,ty;
	int keyx,keyy;

	dbg("#####id=%d,x=%d,y=%d######\n",id,x,y);

	
	tx = x;
	ty = y;
	keyx = x;
	keyy = y;
	if (wmt_ts_get_xaxis()==1)
	{
		tx = y;
		ty = x;
	}
	if (wmt_ts_get_xdir()==-1)
	{
		tx = wmt_ts_get_resolvX() - tx - 1;
	}
	if (wmt_ts_get_ydir()==-1)
	{
		ty = wmt_ts_get_resolvY() - ty - 1;
	}	
	/*if ((tx < 0) || (tx >= wmt_ts_get_resolvX()) ||
	   (ty < 0) || (ty >= wmt_ts_get_resolvY()))
	{
		dbg("Invalid point(%d,%d)\n");
		return;
	}*/
	x = tx;
	y = ty;
	if(x>=wmt_ts_get_resolvX()||y>= wmt_ts_get_resolvY())
	{
	#ifdef HAVE_TOUCH_KEY
		if (wmt_ts_if_tskey())
		{
			report_key(ts,keyx,keyy);
		}
	#endif
		return;
	}
	dbg("rpt%d(%d,%d)\n",id,x,y);
#ifdef REPORT_DATA_ANDROID_4_0
	input_mt_slot(ts->input, id);		
	input_report_abs(ts->input, ABS_MT_TRACKING_ID, id);
	//input_report_abs(ts->input, ABS_MT_TOUCH_MAJOR, pressure);
	input_report_abs(ts->input, ABS_MT_POSITION_X, x);
	input_report_abs(ts->input, ABS_MT_POSITION_Y, y);	
	//input_report_abs(ts->input, ABS_MT_WIDTH_MAJOR, 1);
#else
 //add for cross finger 2013-1-10
  input_report_key(ts->input, BTN_TOUCH, 1);
	input_report_abs(ts->input, ABS_MT_TRACKING_ID, id);
	//input_report_abs(ts->input, ABS_MT_TOUCH_MAJOR, pressure);
	input_report_abs(ts->input, ABS_MT_POSITION_X,x);
	input_report_abs(ts->input, ABS_MT_POSITION_Y, y);
	//input_report_abs(ts->input, ABS_MT_WIDTH_MAJOR, 1);
	input_mt_sync(ts->input);
#endif
}

static void process_gslX680_data(struct gsl_ts *ts)
{
	u8 id, touches;
	u16 x, y;
	int i = 0;
#ifdef GSL_NOID_VERSION
	u32 tmp1;
	u8 buf[4] = {0};
	struct gsl_touch_info cinfo;
#endif
	touches = ts->touch_data[ts->dd->touch_index];

#ifdef GSL_NOID_VERSION
	if (gsl_noid_ver) {
		cinfo.finger_num = touches;
		dbg("tp-gsl  finger_num = %d\n",cinfo.finger_num);
		for(i = 0; i < (touches < MAX_CONTACTS ? touches : MAX_CONTACTS); i ++) {
			cinfo.x[i] = join_bytes( ( ts->touch_data[ts->dd->x_index  + 4 * i + 1] & 0xf),
				ts->touch_data[ts->dd->x_index + 4 * i]);
			cinfo.y[i] = join_bytes(ts->touch_data[ts->dd->y_index + 4 * i + 1],
				ts->touch_data[ts->dd->y_index + 4 * i ]);
			dbg("tp-gsl  x = %d y = %d \n",cinfo.x[i],cinfo.y[i]);
		}
		cinfo.finger_num=(ts->touch_data[3]<<24)|(ts->touch_data[2]<<16)
				|(ts->touch_data[1]<<8)|(ts->touch_data[0]);
		gsl_alg_id_main(&cinfo);
		tmp1=gsl_mask_tiaoping();
		dbg("[tp-gsl] tmp1=%x\n",tmp1);
		if(tmp1>0&&tmp1<0xffffffff) {
			buf[0]=0xa;buf[1]=0;buf[2]=0;buf[3]=0;
			gsl_ts_write(ts->client,0xf0,buf,4);
			buf[0]=(u8)(tmp1 & 0xff);
			buf[1]=(u8)((tmp1>>8) & 0xff);
			buf[2]=(u8)((tmp1>>16) & 0xff);
			buf[3]=(u8)((tmp1>>24) & 0xff);
			dbg("tmp1=%08x,buf[0]=%02x,buf[1]=%02x,buf[2]=%02x,buf[3]=%02x\n",
				tmp1,buf[0],buf[1],buf[2],buf[3]);
			gsl_ts_write(ts->client,0x8,buf,4);
		}
		touches = cinfo.finger_num;
	}
#endif

	for(i=1;i<=MAX_CONTACTS;i++)
	{
		if(touches == 0)
			id_sign[i] = 0;	
		id_state_flag[i] = 0;
	}
	for(i= 0;i < (touches > MAX_FINGERS ? MAX_FINGERS : touches);i ++)
	{
	#ifdef GSL_NOID_VERSION
		if (gsl_noid_ver) {
			id = cinfo.id[i];
			x =  cinfo.x[i];
			y =  cinfo.y[i];	
		}
		else {
			x = join_bytes( ( ts->touch_data[ts->dd->x_index  + 4 * i + 1] & 0xf),
					ts->touch_data[ts->dd->x_index + 4 * i]);
			y = join_bytes(ts->touch_data[ts->dd->y_index + 4 * i + 1],
					ts->touch_data[ts->dd->y_index + 4 * i ]);
			id = ts->touch_data[ts->dd->id_index + 4 * i] >> 4;
		}
	#endif
	
		if(1 <=id && id <= MAX_CONTACTS)
		{
			dbg("raw%d(%d,%d)\n", id, x, y);
			record_point(x, y , id);
			dbg("new%d(%d,%d)\n", id, x_new, y_new);
			report_data(ts, x_new, y_new, 10, id);		
			id_state_flag[id] = 1;
		}
	}
	for(i=1;i<=MAX_CONTACTS;i++)
	{	
		if( (0 == touches) || ((0 != id_state_old_flag[i]) && (0 == id_state_flag[i])) )
		{
		#ifdef REPORT_DATA_ANDROID_4_0
			input_mt_slot(ts->input, i);
			input_report_abs(ts->input, ABS_MT_TRACKING_ID, -1);
			input_mt_report_slot_state(ts->input, MT_TOOL_FINGER, false);
		#endif
			id_sign[i]=0;
		}
		id_state_old_flag[i] = id_state_flag[i];
	}
#ifndef REPORT_DATA_ANDROID_4_0
	if(0 == touches)
	{	
		//add 2013-1-10 cross fingers
		//input_report_abs(ts->input, ABS_MT_TRACKING_ID, -1);
		input_report_key(ts->input, BTN_TOUCH, 0);
		//**********************
		input_mt_sync(ts->input);
	#ifdef HAVE_TOUCH_KEY
		if (wmt_ts_if_tskey())
		{
			if(key_state_flag)
			{
	        	input_report_key(ts->input, key, 0);
				input_sync(ts->input);
				key_state_flag = 0;
			}
		}
	#endif			
	}
#endif
	input_sync(ts->input);
	ts->prev_touches = touches;
}


static void gsl_ts_xy_worker(struct work_struct *work)
{
	int rc;
	u8 read_buf[4] = {0};

	struct gsl_ts *ts = container_of(work, struct gsl_ts,work);

	dbg("---gsl_ts_xy_worker---\n");				 

	if (ts->is_suspended == true) {
		dev_dbg(&ts->client->dev, "TS is supended\n");
		ts->int_pending = true;
		goto schedule;
	}

	/* read data from DATA_REG */
	rc = gsl_ts_read(ts->client, 0x80, ts->touch_data, ts->dd->data_size);
	dbg("---touches: %d ---\n",ts->touch_data[0]);		
		
	if (rc < 0) 
	{
		dev_err(&ts->client->dev, "read failed\n");
		goto schedule;
	}

	if (ts->touch_data[ts->dd->touch_index] == 0xff) {
		goto schedule;
	}

	rc = gsl_ts_read( ts->client, 0xbc, read_buf, sizeof(read_buf));
	if (rc < 0) 
	{
		dev_err(&ts->client->dev, "read 0xbc failed\n");
		goto schedule;
	}
	dbg("//////// reg %x : %x %x %x %x\n",0xbc, read_buf[3], read_buf[2], read_buf[1], read_buf[0]);
		
	if (read_buf[3] == 0 && read_buf[2] == 0 && read_buf[1] == 0 && read_buf[0] == 0)
	{
		process_gslX680_data(ts);
	}
	else
	{
		reset_chip(ts->client);
		startup_chip(ts->client);
	}
	
schedule:
	//enable_irq(ts->irq);
	wmt_enable_gpirq();
		
}

static irqreturn_t gsl_ts_irq(int irq, void *dev_id)
{	
	struct gsl_ts *ts = dev_id;
	
	if (wmt_is_tsint())
	{
		wmt_clr_int();
		if (wmt_is_tsirq_enable() && ts->is_suspended == false)
		{
			wmt_disable_gpirq();
			dbg("begin..\n");
			//if (!work_pending(&l_tsdata.pen_event_work))
			{
				queue_work(ts->wq, &ts->work);
			}
		}
		return IRQ_HANDLED;
	}	
	return IRQ_NONE;


	/*disable_irq_nosync(ts->irq);

	if (!work_pending(&ts->work)) 
	{
		queue_work(ts->wq, &ts->work);
	}
	
	return IRQ_HANDLED;*/

}

#ifdef GSL_TIMER
static void gsl_timer_handle(unsigned long data)
{
	struct gsl_ts *ts = (struct gsl_ts *)data;

#ifdef GSL_DEBUG	
	dbg("----------------gsl_timer_handle-----------------\n");	
#endif

	disable_irq_nosync(ts->irq);	
	check_mem_data(ts->client);
	ts->gsl_timer.expires = jiffies + 3 * HZ;
	add_timer(&ts->gsl_timer);
	enable_irq(ts->irq);
	
}
#endif

static int gsl_ts_init_ts(struct i2c_client *client, struct gsl_ts *ts)
{
	struct input_dev *input_device;
	int i;
	int rc = 0;
	
	dbg("[GSLX680] Enter %s\n", __func__);

	
	ts->dd = &devices[ts->device_id];

	if (ts->device_id == 0) {
		ts->dd->data_size = MAX_FINGERS * ts->dd->touch_bytes + ts->dd->touch_meta_data;
		ts->dd->touch_index = 0;
	}

	ts->touch_data = kzalloc(ts->dd->data_size, GFP_KERNEL);
	if (!ts->touch_data) {
		pr_err("%s: Unable to allocate memory\n", __func__);
		return -ENOMEM;
	}

	ts->prev_touches = 0;

	input_device = input_allocate_device();
	if (!input_device) {
		rc = -ENOMEM;
		goto error_alloc_dev;
	}

	ts->input = input_device;
	input_device->name = GSLX680_I2C_NAME;
	input_device->id.bustype = BUS_I2C;
	input_device->dev.parent = &client->dev;
	input_set_drvdata(input_device, ts);

#ifdef REPORT_DATA_ANDROID_4_0
	__set_bit(EV_ABS, input_device->evbit);
	__set_bit(EV_KEY, input_device->evbit);
	__set_bit(EV_REP, input_device->evbit);
	__set_bit(INPUT_PROP_DIRECT, input_device->propbit);
	input_mt_init_slots(input_device, (MAX_CONTACTS+1));
#else
	//input_set_abs_params(input_device,ABS_MT_TRACKING_ID, 0, (MAX_CONTACTS+1), 0, 0);
	set_bit(EV_ABS, input_device->evbit);
	set_bit(EV_KEY, input_device->evbit);
	__set_bit(INPUT_PROP_DIRECT, input_device->propbit);
#endif

	input_device->evbit[0] = BIT_MASK(EV_SYN) | BIT_MASK(EV_KEY) | BIT_MASK(EV_ABS);
#ifdef HAVE_TOUCH_KEY
	//input_device->evbit[0] = BIT_MASK(EV_KEY);
	if (wmt_ts_if_tskey())
	{
		for (i = 0; i < MAX_KEY_NUM; i++)
			set_bit(key_array[i], input_device->keybit);
	}
#endif

	set_bit(ABS_MT_POSITION_X, input_device->absbit);
	set_bit(ABS_MT_POSITION_Y, input_device->absbit);
	//set_bit(ABS_MT_TOUCH_MAJOR, input_device->absbit);
	//set_bit(ABS_MT_WIDTH_MAJOR, input_device->absbit);
 //****************add 2013-1-10
  set_bit(BTN_TOUCH, input_device->keybit);
  set_bit(ABS_MT_TRACKING_ID, input_device->absbit);
  
	dbg("regsister:x=%d,y=%d\n",wmt_ts_get_resolvX(),wmt_ts_get_resolvY());
	input_set_abs_params(input_device,ABS_MT_POSITION_X, 0, wmt_ts_get_resolvX(), 0, 0);
	input_set_abs_params(input_device,ABS_MT_POSITION_Y, 0, wmt_ts_get_resolvY(), 0, 0);
	//input_set_abs_params(input_device,ABS_MT_TOUCH_MAJOR, 0, PRESS_MAX, 0, 0);
	//input_set_abs_params(input_device,ABS_MT_WIDTH_MAJOR, 0, 200, 0, 0);

	//client->irq = IRQ_PORT,
	//ts->irq = client->irq;

	ts->wq = create_singlethread_workqueue("kworkqueue_ts");
	if (!ts->wq) {
		dev_err(&client->dev, "Could not create workqueue\n");
		goto error_wq_create;
	}
	flush_workqueue(ts->wq);	

	INIT_WORK(&ts->work, gsl_ts_xy_worker);

	rc = input_register_device(input_device);
	if (rc)
		goto error_unreg_device;

	return 0;

error_unreg_device:
	destroy_workqueue(ts->wq);
error_wq_create:
	input_free_device(input_device);
error_alloc_dev:
	kfree(ts->touch_data);
	return rc;
}

static int gsl_ts_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct gsl_ts *ts = l_ts; //dev_get_drvdata(dev);
	//int rc = 0;

  	printk("I'am in gsl_ts_suspend() start\n");
	ts->is_suspended = true;	
	
#ifdef GSL_TIMER
	printk( "gsl_ts_suspend () : delete gsl_timer\n");

	del_timer(&ts->gsl_timer);
#endif
	//disable_irq_nosync(ts->irq);	
	wmt_disable_gpirq();
		   
	reset_chip(ts->client);
	gslX680_shutdown_low();
	msleep(10); 		

	return 0;
}

int resume_download_thread(void *arg)
{
	wake_lock(&downloadWakeLock);
	gslX680_chip_init();
	gslX680_shutdown_high();
	msleep(20); 	
	reset_chip(l_ts->client);
	startup_chip(l_ts->client);	
	//check_mem_data(l_ts->client);
	init_chip(l_ts->client);
	
	l_ts->is_suspended = false;
	if (!earlysus_en)
		wmt_enable_gpirq();
	wake_unlock(&downloadWakeLock);
	return 0;
}

static int gsl_ts_resume(struct platform_device *pdev)
{
#ifdef GSL_TIMER
	dbg( "gsl_ts_resume () : add gsl_timer\n");

	init_timer(&ts->gsl_timer);
	ts->gsl_timer.expires = jiffies + 3 * HZ;
	ts->gsl_timer.function = &gsl_timer_handle;
	ts->gsl_timer.data = (unsigned long)ts;
	add_timer(&ts->gsl_timer);
#endif
	if (is_delay) {
		resume_download_task = kthread_create(resume_download_thread, NULL , "resume_download");
		if(IS_ERR(resume_download_task)) {
			errlog("cread thread failed\n");	
		}
		wake_up_process(resume_download_task);
	} else
		resume_download_thread(NULL);
	
	return 0;
}

#ifdef CONFIG_HAS_EARLYSUSPEND
static void gsl_ts_early_suspend(struct early_suspend *h)
{
	//struct gsl_ts *ts = container_of(h, struct gsl_ts, early_suspend);
	dbg("[GSL1680] Enter %s\n", __func__);
	//gsl_ts_suspend(&ts->client->dev);
	wmt_disable_gpirq();
}

static void gsl_ts_late_resume(struct early_suspend *h)
{
	//struct gsl_ts *ts = container_of(h, struct gsl_ts, early_suspend);
	dbg("[GSL1680] Enter %s\n", __func__);
	//gsl_ts_resume(&ts->client->dev);
	wmt_enable_gpirq();
}
#endif

static void check_Backlight_delay(void)
{
	int ret;
	int len = 7;
    char retval[8];
	
	ret = wmt_getsyspara("wmt.backlight.delay", retval, &len);
	if(ret) {
		dbg("Read wmt.backlight.delay Failed.\n");
		is_delay = 0;
	} else
		is_delay = 1;
}
static int gsl_ts_probe(struct i2c_client *client)
{
	struct gsl_ts *ts;
	int rc;

	dbg("GSLX680 Enter %s\n", __func__);
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		dev_err(&client->dev, "I2C functionality not supported\n");
		return -ENODEV;
	}
 	if (wmt_get_fwdata())
 	{
 		errlog("Failed to load the firmware data!\n");
 		return -1;
 	}
	ts = kzalloc(sizeof(*ts), GFP_KERNEL);
	if (!ts) {
		rc = -ENOMEM;
		goto error_kfree_fw;
	}
	dbg("==kzalloc success=\n");
	l_ts = ts;

	ts->client = client;
	i2c_set_clientdata(client, ts);
	ts->device_id = 0;//id->driver_data;

	ts->is_suspended = false;
	ts->int_pending = false;
	wake_lock_init(&downloadWakeLock, WAKE_LOCK_SUSPEND, "resume_download");
	mutex_init(&ts->sus_lock);
	
	rc = gsl_ts_init_ts(client, ts);
	if (rc < 0) {
		dev_err(&client->dev, "GSLX680 init failed\n");
		goto error_mutex_destroy;
	}	

	gslX680_chip_init();
	init_chip(ts->client);
	check_mem_data(ts->client);
	
	ts->irq = wmt_get_tsirqnum();	
	rc=  request_irq(wmt_get_tsirqnum(), gsl_ts_irq, IRQF_SHARED, client->name, ts);
	if (rc < 0) {
		dbg( "gsl_probe: request irq failed\n");
		goto error_req_irq_fail;
	}

#ifdef GSL_TIMER
	dbg( "gsl_ts_probe () : add gsl_timer\n");

	init_timer(&ts->gsl_timer);
	ts->gsl_timer.expires = jiffies + 3 * HZ;	//¶¨Ê±3  ÃëÖÓ
	ts->gsl_timer.function = &gsl_timer_handle;
	ts->gsl_timer.data = (unsigned long)ts;
	add_timer(&ts->gsl_timer);
#endif

	/* create debug attribute */
	//rc = device_create_file(&ts->input->dev, &dev_attr_debug_enable);

#ifdef CONFIG_HAS_EARLYSUSPEND
	ts->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
	ts->early_suspend.suspend = gsl_ts_early_suspend;
	ts->early_suspend.resume = gsl_ts_late_resume;
	register_early_suspend(&ts->early_suspend);
#endif

	dbg("[GSLX680] End %s\n", __func__);
	wmt_enable_gpirq();
	
	check_Backlight_delay();
	return 0;

//exit_set_irq_mode:	
error_req_irq_fail:
    free_irq(ts->irq, ts);	

error_mutex_destroy:
	mutex_destroy(&ts->sus_lock);
	wake_lock_destroy(&downloadWakeLock);
	input_free_device(ts->input);
	kfree(ts);
error_kfree_fw:
	kfree(GSLX680_FW);
	return rc;
}

static int  gsl_ts_remove(struct i2c_client *client)
{


	struct gsl_ts *ts = i2c_get_clientdata(client);
	dbg("==gsl_ts_remove=\n");

#ifdef CONFIG_HAS_EARLYSUSPEND
	unregister_early_suspend(&ts->early_suspend);
#endif

	//device_init_wakeup(&client->dev, 0);
	cancel_work_sync(&ts->work);
	free_irq(ts->irq, ts);
	destroy_workqueue(ts->wq);
	input_unregister_device(ts->input);
	mutex_destroy(&ts->sus_lock);
	wake_lock_destroy(&downloadWakeLock);

	//device_remove_file(&ts->input->dev, &dev_attr_debug_enable);

	if(GSLX680_FW){
		kfree(GSLX680_FW);
		GSLX680_FW = NULL;
	}
	
	kfree(ts->touch_data);
	kfree(ts);

	return 0;
}

/*
static const struct i2c_device_id gsl_ts_id[] = {
	{GSLX680_I2C_NAME, 0},
	{}
};
MODULE_DEVICE_TABLE(i2c, gsl_ts_id);


static struct i2c_driver gsl_ts_driver = {
	.driver = {
		.name = GSLX680_I2C_NAME,
		.owner = THIS_MODULE,
	},
#ifndef CONFIG_HAS_EARLYSUSPEND
	.suspend	= gsl_ts_suspend,
	.resume	= gsl_ts_resume,
#endif
	.probe		= gsl_ts_probe,
	.remove		= __devexit_p(gsl_ts_remove),
	.id_table	= gsl_ts_id,
};
*/
static int wmt_wakeup_bl_notify(struct notifier_block *nb, unsigned long event,
	void *dummy)
{
	//printk("get notify\n");
	switch (event) {
		case BL_CLOSE:
			l_ts->is_suspended = true;
			//printk("\nclose backlight\n\n");
			//printk("disable irq\n\n");
			wmt_disable_gpirq();
			break;
		case BL_OPEN:
			l_ts->is_suspended = false;
			//printk("\nopen backlight\n\n");
			//printk("enable irq\n\n");
			wmt_enable_gpirq();
			break;
	}

	return NOTIFY_OK;
}

static struct notifier_block wmt_bl_notify = {
	.notifier_call = wmt_wakeup_bl_notify,
};

static int gsl_ts_init(void)
{
    int ret = 0;
    ret = gsl_ts_probe(ts_get_i2c_client());
	if (ret)
	{
		dbg("Can't load gsl1680 ts driver!\n");
	}
	//dbg("ret=%d\n",ret);
	if (earlysus_en)
		register_bl_notifier(&wmt_bl_notify);
	return ret;
}
static void gsl_ts_exit(void)
{
	dbg("==gsl_ts_exit==\n");
	//i2c_del_driver(&gsl_ts_driver);
	gsl_ts_remove(ts_get_i2c_client());
	if (earlysus_en)
		unregister_bl_notifier(&wmt_bl_notify);
	return;
}

struct wmtts_device gslx680_tsdev = {
		.driver_name = WMT_TS_I2C_NAME,
		.ts_id = "GSL1680",
		.init = gsl_ts_init,
		.exit = gsl_ts_exit,
		.suspend = gsl_ts_suspend,
		.resume = gsl_ts_resume,
};


//module_init(gsl_ts_init);
//module_exit(gsl_ts_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("GSLX680 touchscreen controller driver");
MODULE_AUTHOR("Guan Yuwei, guanyuwei@basewin.com");
MODULE_ALIAS("platform:gsl_ts");
