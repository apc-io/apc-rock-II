/* drivers/input/touchscreen/cyp140_i2c.c
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * ZEITEC Semiconductor Co., Ltd
 * Tel: +886-3-579-0045
 * Fax: +886-3-579-9960
 * http://www.zeitecsemi.com
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/input.h>
#include <linux/platform_device.h>
#include <linux/jiffies.h>
#include <linux/io.h>
#include <linux/i2c.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif 
#include <linux/slab.h>
#include <linux/irq.h>
#include <linux/errno.h>
#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/bitops.h>

#include "wmt_ts.h"
#define TP_POINTS_CNT	5
#define U8 unsigned char
//fw update.
//#include "cyp140_fw.h"

//****************************add for cyp140 2013-1-6
//extern struct tpd_device *tpd;
static struct i2c_client *i2c_client = NULL;
static struct task_struct *thread = NULL;

static DECLARE_WAIT_QUEUE_HEAD(waiter);

#define TPD_DEVICE "cyp140"
static int tpd_load_status = 0;//add !!!2013-1-6
//static struct early_suspend early_suspend;

#ifdef CONFIG_HAS_EARLYSUSPEND
static struct early_suspend early_suspend;
static void tpd_early_suspend(struct early_suspend *handler);
static void tpd_late_resume(struct early_suspend *handler);
#endif 

static int tilt = 1, rev_x = -1, rev_y = 1;
static int max_x = 1024, max_y = 600;
//static int max_x = 800, max_y = 480;

//extern void mt65xx_eint_unmask(unsigned int line);
//extern void mt65xx_eint_mask(unsigned int line);
//extern void mt65xx_eint_set_hw_debounce(kal_uint8 eintno, kal_uint32 ms);
//extern kal_uint32 mt65xx_eint_set_sens(kal_uint8 eintno, kal_bool sens);
//extern mt65xx_eint_set_polarity(unsigned int eint_num, unsigned int pol);
//extern void mt65xx_eint_registration(kal_uint8 eintno, kal_bool Dbounce_En,
 //       kal_bool ACT_Polarity, void (EINT_FUNC_PTR)(void),
 //       kal_bool auto_umask);


static irqreturn_t tpd_eint_interrupt_handler(int irq, void *dev_id);
//static int tpd_get_bl_info(int show);
static int __devinit tpd_probe(struct i2c_client *client);
//static int tpd_detect(struct i2c_client *client, int kind, struct i2c_board_info *info);
//static int __devexit tpd_remove(struct i2c_client *client);
static int touch_event_handler(void *unused);
//static int tpd_initialize(struct i2c_client * client);


volatile static int tpd_flag = 0;//0; debug 2013-5-6

#ifdef TPD_HAVE_BUTTON 
static int tpd_keys_local[TPD_KEY_COUNT] = TPD_KEYS;
static int tpd_keys_dim_local[TPD_KEY_COUNT][4] = TPD_KEYS_DIM;
#endif

#define TPD_OK 					0
//#define TPD_EREA_Y                             799
//#define TPD_EREA_X                             479
#define TPD_EREA_Y                             479
#define TPD_EREA_X                             319

#define TPD_DISTANCE_LIMIT                     100

#define TPD_REG_BASE 0x00
#define TPD_SOFT_RESET_MODE 0x01
#define TPD_OP_MODE 0x00
#define TPD_LOW_PWR_MODE 0x04
#define TPD_SYSINFO_MODE 0x10
#define GET_HSTMODE(reg)  ((reg & 0x70) >> 4)  // in op mode or not 
#define GET_BOOTLOADERMODE(reg) ((reg & 0x10) >> 4)  // in bl mode 
//#define GPIO_CTP_EN_PIN_M_GPIO 0
//#define GPIO_CTP_EN_PIN 0xff

static u8 bl_cmd[] = {
    0x00, 0x00, 0xFF, 0xA5,
    0x00, 0x01, 0x02,
    0x03, 0x04, 0x05,
    0x06, 0x07};
//exit bl mode
struct tpd_operation_data_t{
	U8 hst_mode;
	U8 tt_mode;
	U8 tt_stat;

	U8 x1_M,x1_L;
	U8 y1_M,y1_L;
	U8 x5_M;
	U8 touch12_id;

	U8 x2_M,x2_L;
	U8 y2_M,y2_L;
	U8 x5_L;
	U8 gest_cnt;
	U8 gest_id;
	//U8 gest_set;


	U8 x3_M,x3_L;
	U8 y3_M,y3_L;
	U8 y5_M;
	U8 touch34_id;

	U8 x4_M,x4_L;
	U8 y4_M,y4_L;
	U8 y5_L;

    //U8 x5_M,x5_L;
	U8 Undefinei1B;
	U8 Undefined1C;
	U8 Undefined1D;
	U8 GEST_SET;
	U8 touch5_id;
};

struct tpd_bootloader_data_t{
	U8 bl_file;
	U8 bl_status;
	U8 bl_error;
	U8 blver_hi,blver_lo;
	U8 bld_blver_hi,bld_blver_lo;

	U8 ttspver_hi,ttspver_lo;
	U8 appid_hi,appid_lo;
	U8 appver_hi,appver_lo;

	U8 cid_0;
	U8 cid_1;
	U8 cid_2;

};

struct tpd_sysinfo_data_t{
	U8   hst_mode;
	U8  mfg_cmd;
	U8  mfg_stat;
	U8 cid[3];
	u8 tt_undef1;

	u8 uid[8];
	U8  bl_verh;
	U8  bl_verl;

	u8 tts_verh;
	u8 tts_verl;

	U8 app_idh;
	U8 app_idl;
	U8 app_verh;
	U8 app_verl;

	u8 tt_undef2[6];
	U8  act_intrvl;
	U8  tch_tmout;
	U8  lp_intrvl;	 

};

struct touch_info {
	int x[5];
	int y[5];
	int p[5];
	int id[5];
	int count;
};

struct id_info{
	int pid1;
	int pid2;
	int reportid1;
	int reportid2;
	int id1;
	int id2;

};
static struct tpd_operation_data_t g_operation_data;
//static struct tpd_bootloader_data_t g_bootloader_data;
//static struct tpd_sysinfo_data_t g_sysinfo_data;

//********************************************************

/* -------------- global variable definition -----------*/
#define _MACH_MSM_TOUCH_H_

#define ZET_TS_ID_NAME "cyp140-ts"

#define MJ5_TS_NAME "cyp140_touchscreen"

//#define TS_INT_GPIO		S3C64XX_GPN(9)  /*s3c6410*/
//#define TS1_INT_GPIO        AT91_PIN_PB17 /*AT91SAM9G45 external*/
//#define TS1_INT_GPIO        AT91_PIN_PA27 /*AT91SAM9G45 internal*/
//#define TS_RST_GPIO		S3C64XX_GPN(10)

#define TS_RST_GPIO
#define TPINFO	1
#define X_MAX	800 //1024
#define Y_MAX	480 //576
#define FINGER_NUMBER 5
#define KEY_NUMBER 3 //0
#define P_MAX	1
#define D_POLLING_TIME	25000
#define U_POLLING_TIME	25000
#define S_POLLING_TIME  100
#define REPORT_POLLING_TIME  5

#define MAX_KEY_NUMBER      	8
#define MAX_FINGER_NUMBER	16
#define TRUE 		1
#define FALSE 		0

//#define debug_mode 1
//#define DPRINTK(fmt,args...)	do { if (debug_mode) printk(KERN_EMERG "[%s][%d] "fmt"\n", __FUNCTION__, __LINE__, ##args);} while(0)

//#define TRANSLATE_ENABLE 1
#define TOPRIGHT 	0
#define TOPLEFT  	1
#define BOTTOMRIGHT	2
#define BOTTOMLEFT	3
#define ORIGIN		BOTTOMRIGHT

#define TIME_CHECK_CHARGE 3000

struct msm_ts_platform_data {
	unsigned int x_max;
	unsigned int y_max;
	unsigned int pressure_max;
};

struct tpd_device{
	struct i2c_client * client;//i2c_ts;
	struct work_struct work1;
	struct input_dev *input;
	struct timer_list polling_timer;
	struct delayed_work work; // for polling
	struct workqueue_struct *queue;
#ifdef CONFIG_HAS_EARLYSUSPEND	
	struct early_suspend early_suspend;
#endif	
	unsigned int gpio; /* GPIO used for interrupt of TS1*/
	unsigned int irq;
	unsigned int x_max;
	unsigned int y_max;
	unsigned int pressure_max;
};
//
struct tpd_device *tpd;


//static int l_suspend = 0; // 1:suspend, 0:normal state

//static int resetCount = 0;          //albert++ 20120807


//static u16 polling_time = S_POLLING_TIME;

//static int l_powermode = -1;
//static struct mutex i2c_mutex;


//static int __devinit cyp140_ts_probe(struct i2c_client *client, const struct i2c_device_id *id);
//static int __devexit cyp140_ts_remove(struct i2c_client *dev);





//static int filterCount = 0; 
//static u32 filterX[MAX_FINGER_NUMBER][2], filterY[MAX_FINGER_NUMBER][2]; 

//static u8  key_menu_pressed = 0x1;
//static u8  key_back_pressed = 0x1;
//static u8  key_search_pressed = 0x1;

//static u16 ResolutionX=X_MAX;
//static u16 ResolutionY=Y_MAX;
//static u16 FingerNum=0;
//static u16 KeyNum=0;
//static int bufLength=0;	
//static u8 xyExchange=0;
//static u16 inChargerMode = 0;
//static struct i2c_client *this_client;
struct workqueue_struct *ts_wq = NULL;
#if 0
static int l_tskey[4][2] = {
	{KEY_BACK,0},
	{KEY_MENU,0},
	{KEY_HOME,0},
	{KEY_SEARCH,0},
};
#endif
u8 pc[8];
// {IC Model, FW Version, FW version,Codebase Type=0x08, Customer ID, Project ID, Config Board No, Config Serial No}

//Touch Screen
/*static const struct i2c_device_id cyp140_ts_idtable[] = {
       { ZET_TS_ID_NAME, 0 },
       { }
};

static struct i2c_driver cyp140_ts_driver = {
	.driver = {
		.owner = THIS_MODULE,
		.name  = ZET_TS_ID_NAME,
	},
	.probe	  = cyp140_ts_probe,
	.remove		= __devexit_p(cyp140_ts_remove),
	.id_table = cyp140_ts_idtable,
};
*/


/***********************************************************************
    [function]: 
		        callback: Timer Function if there is no interrupt fuction;
    [parameters]:
			    arg[in]:  arguments;
    [return]:
			    NULL;
************************************************************************/


//extern int wmt_i2c_xfer_continue_if_4(struct i2c_msg *msg, unsigned int num,int bus_id);
/***********************************************************************
    [function]: 
		        callback: read data by i2c interface;
    [parameters]:
			    client[in]:  struct i2c_client — represent an I2C slave device;
			    data [out]:  data buffer to read;
			    length[in]:  data length to read;
    [return]:
			    Returns negative errno, else the number of messages executed;
************************************************************************/
int cyp140_i2c_read_tsdata(struct i2c_client *client, u8 *data, u8 length)
{
	struct i2c_msg msg;
	msg.addr = client->addr;
	msg.flags = I2C_M_RD;
	msg.len = length;
	msg.buf = data;
	return i2c_transfer(client->adapter,&msg, 1);
	
	/*int rc = 0;
	
	memset(data, 0, length);
	rc = i2c_master_recv(client, data, length);
	if (rc <= 0)
	{
		errlog("error!\n");
		return -EINVAL;
	} else if (rc != length)
	{
		dbg("want:%d,real:%d\n", length, rc);
	}
	return rc;*/
}

/***********************************************************************
    [function]: 
		        callback: write data by i2c interface;
    [parameters]:
			    client[in]:  struct i2c_client — represent an I2C slave device;
			    data [out]:  data buffer to write;
			    length[in]:  data length to write;
    [return]:
			    Returns negative errno, else the number of messages executed;
************************************************************************/
int cyp140_i2c_write_tsdata(struct i2c_client *client, u8 *data, u8 length)
{
	struct i2c_msg msg;
	msg.addr = client->addr;
	msg.flags = 0;
	msg.len = length;
	msg.buf = data;
	return i2c_transfer(client->adapter,&msg, 1);
	
	/*int ret = i2c_master_recv(client, data, length);
	if (ret <= 0)
	{
		errlog("error!\n");
	}
	return ret;
	*/
}

/***********************************************************************
    [function]: 
		        callback: coordinate traslating;
    [parameters]:
			    px[out]:  value of X axis;
			    py[out]:  value of Y axis;
				p [in]:   pressed of released status of fingers;
    [return]:
			    NULL;
************************************************************************/
void touch_coordinate_traslating(u32 *px, u32 *py, u8 p)
{
	int i;
	u8 pressure;

	#if ORIGIN == TOPRIGHT
	for(i=0;i<MAX_FINGER_NUMBER;i++){
		pressure = (p >> (MAX_FINGER_NUMBER-i-1)) & 0x1;
		if(pressure)
		{
			px[i] = X_MAX - px[i];
		}
	}
	#elif ORIGIN == BOTTOMRIGHT
	for(i=0;i<MAX_FINGER_NUMBER;i++){
		pressure = (p >> (MAX_FINGER_NUMBER-i-1)) & 0x1;
		if(pressure)
		{
			px[i] = X_MAX - px[i];
			py[i] = Y_MAX - py[i];
		}
	}
	#elif ORIGIN == BOTTOMLEFT
	for(i=0;i<MAX_FINGER_NUMBER;i++){
		pressure = (p >> (MAX_FINGER_NUMBER-i-1)) & 0x1;
		if(pressure)
		{
			py[i] = Y_MAX - py[i];
		}
	}
	#endif
}

/***********************************************************************
    [function]: 
		        callback: reset function;
    [parameters]:
			    void;
    [return]:
			    void;
************************************************************************/
void ctp_reset(void)
{
#if defined(TS_RST_GPIO)
	//reset mcu
   /* gpio_direction_output(TS_RST_GPIO, 1);
	msleep(1);
    gpio_direction_output(TS_RST_GPIO, 0);
	msleep(10);
	gpio_direction_output(TS_RST_GPIO, 1);
	msleep(20);*/
	wmt_rst_output(1);
	msleep(1);
	wmt_rst_output(0);
	msleep(10);
	wmt_rst_output(1);
	msleep(20);
	dbg("has done\n");
#else
	u8 ts_reset_cmd[1] = {0xb0};
	cyp140_i2c_write_tsdata(this_client, ts_reset_cmd, 1);
#endif

}

//*************************************************
#if 1
#include <linux/sched.h>   //wake_up_process()  
#include <linux/kthread.h> //kthread_create()、kthread_run()  
//#include <err.h> //IS_ERR()、PTR_ERR()  

void cyttsp_sw_reset(void);
//static struct task_struct *esd_task;  
volatile bool need_rst_flag = 0;
volatile int tp_interrupt_flag = 0;
volatile int tp_suspend_flag = 0;
volatile int tp_reseting_flag = 0;

void cyttsp_print_reg(struct i2c_client *client)
{
#if 1
    char buffer[20];
    int status=0;
	int i;
	
    status = i2c_smbus_read_i2c_block_data(i2c_client, 0x00, 16, &(buffer[0]));

	printk("++++cyttsp_print_reg=%d: ",status);
	for(i = 0; i<16;i++)
		printk(" %02x", buffer[i]);
	printk("\n");
#endif

}

int exit_boot_mode(void)
{	 

	//int retval = TPD_OK;

	char buffer[2];
	int status=0;
	status = i2c_smbus_read_i2c_block_data(i2c_client, 0x01, 1, &(buffer[0]));
	if(status<0) {
		printk ("++++exit_boot_mode failed---1\n");
		return status;
	}
	else
	{
		if(buffer[0] &  0x10)
		{
			status = i2c_master_send(i2c_client, bl_cmd, 12);
			if( status < 0)
			{
				printk ("++++exit_boot_mode failed---2\n");
				return status;
			}
			else
			{
				//printk("++++exit_boot_mode ok\n");
			}
			msleep(300);
			status = i2c_smbus_read_i2c_block_data(i2c_client, 0x01, 1, &(buffer[0]));
			if(status<0) {
				printk ("++++exit_boot_mode  set failed\n");
				return status;
			}
//			printk("++++exit_boot_mode  set: 0x%x\n",buffer[0]);
			cyttsp_print_reg(i2c_client);
		}
		else
		{
		//	printk("++++exit_boot_mode-- not in bootmode\n");
		}
		
	}
	return 0;

}

void esd_check(void)
{
	if(need_rst_flag)
	{
		if(tp_suspend_flag == 0)
		{
			printk("++++esd_check---rst\n");  
			//mt65xx_eint_mask(CUST_EINT_TOUCH_PANEL_NUM);  
			tp_reseting_flag = 1;
			cyttsp_sw_reset();
			tp_reseting_flag = 0;
			//mt65xx_eint_unmask(CUST_EINT_TOUCH_PANEL_NUM);  
		}
		need_rst_flag = 0;
	}
}
static int fp_count = 0;
#if 0 //close 2013-1-6
void esd_thread(void)
{
	static int i = 0, j = 0;
	while(1)
	{
		printk("++++esd_thread, need_rst_flag=%d, fp_count=%d\n", need_rst_flag,fp_count);  
		fp_count = 0;
		if(need_rst_flag)
		{
			j = 0;
			while(tp_interrupt_flag==1 && j<200)	//wujinyou
			{
				j ++;
				if(tp_suspend_flag)
					msleep(1000);
				else
					msleep(10);
			}
			if(tp_suspend_flag == 0)
			{
				printk("++++esd_thread, start reset, mask int\n");  
				//mt65xx_eint_mask(CUST_EINT_TOUCH_PANEL_NUM);  
				tp_reseting_flag = 1;
				cyttsp_sw_reset();
				i = 0;
				need_rst_flag = 0;
				tp_reseting_flag = 0;
				//mt65xx_eint_unmask(CUST_EINT_TOUCH_PANEL_NUM);  
			}
		}
		msleep(1000);
		i ++;
		if(i == 10)
		{
			i = 0;
			//cyttsp_sw_reset();
			//need_rst_flag = 1;
		}
	}
}
static int esd_init_thread(void)  
{  
	int err;  
	printk("++++%s, line %d----\n", __FUNCTION__, __LINE__);

	esd_task = kthread_create(esd_thread, NULL, "esd_task");  

	if(IS_ERR(esd_task)){  
		printk("++++Unable to start kernel thread.\n");  
		err = PTR_ERR(esd_task);  
		esd_task = NULL;  
		return err;  
	}  

	wake_up_process(esd_task);  

	return 0;  

} 
#endif //close 2013-1-6
 
#endif  

static void tpd_down(int x, int y, int p) {

	
	//printk("<<<<<<x,y (%d, %d)\n", x, y);//debug 2013-5-6

//printk("++++tpd_down: %d,%d,%d\n", x,  y, p);
#if 0 //def TPD_HAVE_BUTTON
	if (boot_mode != NORMAL_BOOT) {
		if(y > 480) {
		    tpd_button(x, y, 1);
		}
	}
#endif
    //*****here process x y coord and then report!!!! 2013-1-7
	
#if 1//0
	int tmp;
	if (tilt)
	{
		tmp = x;
		x = y;
		y =tmp;
	}
	if (rev_x < 0)
		x = max_x -x;
	if (rev_y < 0)
		y = max_y -y;
	
	
#endif

    	//printk("<<<<<< transfer x,y (%d, %d)\n", x, y);//debug 2013-5-6
    
	input_report_abs(tpd->input, ABS_PRESSURE,p);
	input_report_key(tpd->input, BTN_TOUCH, 1);
    //input_report_abs(tpd->input,ABS_MT_TRACKING_ID,i);
	input_report_abs(tpd->input, ABS_MT_TOUCH_MAJOR, 1);
	input_report_abs(tpd->input, ABS_MT_POSITION_X, x);
	input_report_abs(tpd->input, ABS_MT_POSITION_Y, y);
    ////TPD_DEBUG("Down x:%4d, y:%4d, p:%4d \n ", x, y, p);
	input_mt_sync(tpd->input);
    //TPD_DOWN_DEBUG_TRACK(x,y);
    fp_count ++;
}

static void tpd_up(int x, int y,int p) {

	input_report_abs(tpd->input, ABS_PRESSURE, 0);
	input_report_key(tpd->input, BTN_TOUCH, 0);
   // input_report_abs(tpd->input,ABS_MT_TRACKING_ID,i);
	input_report_abs(tpd->input, ABS_MT_TOUCH_MAJOR, 0);
    //input_report_abs(tpd->input, ABS_MT_POSITION_X, x);
    //input_report_abs(tpd->input, ABS_MT_POSITION_Y, y); //!!!!
    //TPD_DEBUG("Up x:%4d, y:%4d, p:%4d \n", x, y, 0);
	input_mt_sync(tpd->input);
   // TPD_UP_DEBUG_TRACK(x,y);
}
void test_retval(s32 ret)
{
#if 1
	if(ret<0)
	{
		need_rst_flag = 1;
		printk("++++test_retval=1-------\n");
	}
#endif
}
static int tpd_touchinfo(struct touch_info *cinfo, struct touch_info *pinfo)
{

	s32 retval;
	static u8 tt_mode;
    //pinfo->count = cinfo->count;
    u8 data0,data1;

	memcpy(pinfo, cinfo, sizeof(struct touch_info));
	memset(cinfo, 0, sizeof(struct touch_info));
//	printk("pinfo->count =%d\n",pinfo->count);

	retval = i2c_smbus_read_i2c_block_data(i2c_client, TPD_REG_BASE, 8, (u8 *)&g_operation_data);
	retval += i2c_smbus_read_i2c_block_data(i2c_client, TPD_REG_BASE + 8, 8, (((u8 *)(&g_operation_data)) + 8));
	retval += i2c_smbus_read_i2c_block_data(i2c_client, TPD_REG_BASE + 16, 8, (((u8 *)(&g_operation_data)) + 16));
	retval += i2c_smbus_read_i2c_block_data(i2c_client, TPD_REG_BASE + 24, 8, (((u8 *)(&g_operation_data)) + 24));


	//cyttsp_print_reg(i2c_client);
    ////TPD_DEBUG("received raw data from touch panel as following:\n");

    /*("hst_mode = %02X, tt_mode = %02X, tt_stat = %02X\n", \
            g_operation_data.hst_mode,\
            g_operation_data.tt_mode,\
            g_operation_data.tt_stat); */

	cinfo->count = (g_operation_data.tt_stat & 0x0f) ; //point count

    //TPD_DEBUG("cinfo->count =%d\n",cinfo->count);

    //TPD_DEBUG("Procss raw data...\n");

	cinfo->x[0] = (( g_operation_data.x1_M << 8) | ( g_operation_data.x1_L)); //point 1
	cinfo->y[0]  = (( g_operation_data.y1_M << 8) | ( g_operation_data.y1_L));
	cinfo->p[0] = 0;//g_operation_data.z1;

	//printk("Before:	cinfo->x0 = %3d, cinfo->y0 = %3d, cinfo->p0 = %3d cinfo->id0 = %3d\n", cinfo->x[0] ,cinfo->y[0] ,cinfo->p[0], cinfo->id[0]);
	if(cinfo->x[0] < 1) cinfo->x[0] = 1;
    	if(cinfo->y[0] < 1) cinfo->y[0] = 1;
	cinfo->id[0] = ((g_operation_data.touch12_id & 0xf0) >>4) -1;
	//printk("After:		cinfo->x0 = %3d, cinfo->y0 = %3d, cinfo->p0 = %3d cinfo->id0 = %3d\n", cinfo->x[0] ,cinfo->y[0] ,cinfo->p[0], cinfo->id[0]);

	if(cinfo->count >1)
	{
		 cinfo->x[1] = (( g_operation_data.x2_M << 8) | ( g_operation_data.x2_L)); //point 2
		 cinfo->y[1] = (( g_operation_data.y2_M << 8) | ( g_operation_data.y2_L));
		 cinfo->p[1] = 0;//g_operation_data.z2;

		//printk("before:	 cinfo->x2 = %3d, cinfo->y2 = %3d,  cinfo->p2 = %3d\n", cinfo->x2, cinfo->y2,  cinfo->p2);
		if(cinfo->x[1] < 1) cinfo->x[1] = 1;
		if(cinfo->y[1] < 1) cinfo->y[1] = 1;
		cinfo->id[1] = ((g_operation_data.touch12_id & 0x0f)) -1;
		//printk("After:	 cinfo->x[1] = %3d, cinfo->y[1] = %3d,  cinfo->p[1] = %3d, cinfo->id[1] = %3d\n", cinfo->x[1], cinfo->y[1], cinfo->p[1], cinfo->id[1]);

		if (cinfo->count > 2)
		{
			cinfo->x[2]= (( g_operation_data.x3_M << 8) | ( g_operation_data.x3_L)); //point 3
			cinfo->y[2] = (( g_operation_data.y3_M << 8) | ( g_operation_data.y3_L));
			cinfo->p[2] = 0;//g_operation_data.z3;
			cinfo->id[2] = ((g_operation_data.touch34_id & 0xf0) >> 4) -1;

			//printk("before:	 cinfo->x[2] = %3d, cinfo->y[2]  = %3d, cinfo->p[2]  = %3d\n", cinfo->x[2], cinfo->y[2], cinfo->p[2]);
			if(cinfo->x[2] < 1) cinfo->x[2] = 1;
			if(cinfo->y[2]< 1) cinfo->y[2] = 1;
			//printk("After:	 cinfo->x[2]= %3d, cinfo->y[2] = %3d, cinfo->p[2]= %3d, cinfo->id[2] = %3d\n", cinfo->x[2], cinfo->y[2], cinfo->p[2], cinfo->id[2]);

			if (cinfo->count > 3)
			{
				cinfo->x[3] = (( g_operation_data.x4_M << 8) | ( g_operation_data.x4_L)); //point 3
				cinfo->y[3] = (( g_operation_data.y4_M << 8) | ( g_operation_data.y4_L));
				cinfo->p[3] = 0;//g_operation_data.z4;
				cinfo->id[3] = ((g_operation_data.touch34_id & 0x0f)) -1;  

				//printk("before:	 cinfo->x[3] = %3d, cinfo->y[3] = %3d, cinfo->p[3] = %3d, cinfo->id[3] = %3d\n", cinfo->x[3], cinfo->y[3], cinfo->p[3], cinfo->id[3]);
				//printk("before:	 x4_M = %3d, x4_L = %3d\n", g_operation_data.x4_M,  g_operation_data.x4_L);
				if(cinfo->x[3] < 1) cinfo->x[3] = 1;
				if(cinfo->y[3] < 1) cinfo->y[3] = 1;
				//printk("After:	 cinfo->x[3] = %3d, cinfo->y[3] = %3d, cinfo->p[3]= %3d, cinfo->id[3] = %3d\n", cinfo->x[3], cinfo->y[3], cinfo->p[3], cinfo->id[3]);
			}
			if (cinfo->count > 4)
			{
				cinfo->x[4] = (( g_operation_data.x5_M << 8) | ( g_operation_data.x5_L)); //point 3
				cinfo->y[4] = (( g_operation_data.y5_M << 8) | ( g_operation_data.y5_L));
				cinfo->p[4] = 0;//g_operation_data.z4;
				cinfo->id[4] = ((g_operation_data.touch5_id & 0xf0) >> 4) -1;

				//printk("before:	 cinfo->x[4] = %3d, cinfo->y[4] = %3d, cinfo->id[4] = %3d\n", cinfo->x[4], cinfo->y[4], cinfo->id[4]);
				//printk("before:	 x5_M = %3d, x5_L = %3d\n", g_operation_data.x5_M,  g_operation_data.x5_L);
				if(cinfo->x[4] < 1) cinfo->x[4] = 1;
				if(cinfo->y[4] < 1) cinfo->y[4] = 1;
				//printk("After:	 cinfo->x[4] = %3d, cinfo->y[4] = %3d,  cinfo->id[4] = %3d\n", cinfo->x[4], cinfo->y[4], cinfo->id[4]);
			}
		}

	}

	if (!cinfo->count) return true; // this is a touch-up event

	if (g_operation_data.tt_mode & 0x20) {
        //TPD_DEBUG("uffer is not ready for use!\n");
        memcpy(cinfo, pinfo, sizeof(struct touch_info));
        return false;
    }//return false; // buffer is not ready for use// buffer is not ready for use

    // data toggle 

	data0 = i2c_smbus_read_i2c_block_data(i2c_client, TPD_REG_BASE, 1, (u8*)&g_operation_data);
    ////TPD_DEBUG("before hst_mode = %02X \n", g_operation_data.hst_mode);

	if((g_operation_data.hst_mode & 0x80)==0)
		g_operation_data.hst_mode = g_operation_data.hst_mode|0x80;
	else
		g_operation_data.hst_mode = g_operation_data.hst_mode & (~0x80);

	////TPD_DEBUG("after hst_mode = %02X \n", g_operation_data.hst_mode);
	data1 = i2c_smbus_write_i2c_block_data(i2c_client, TPD_REG_BASE, sizeof(g_operation_data.hst_mode), &g_operation_data.hst_mode);


	if (tt_mode == g_operation_data.tt_mode) {
            //TPD_DEBUG("sampling not completed!\n");
            memcpy(cinfo, pinfo, sizeof(struct touch_info));
            return false; 
    }// sampling not completed
    else 
		tt_mode = g_operation_data.tt_mode;

    return true;

};

static int touch_event_handler(void *unused)
{
	int i,j;
	int keeppoint[5];
	struct touch_info cinfo, pinfo;
	struct sched_param param = { .sched_priority = 70/*RTPM_PRIO_TPD*/ };
	sched_setscheduler(current, SCHED_RR, &param);

	do
	{
	//printk("++++%s, line %d----unmask int\n", __FUNCTION__, __LINE__);
       // mt65xx_eint_unmask(CUST_EINT_TOUCH_PANEL_NUM);
        wmt_enable_gpirq();
        set_current_state(TASK_INTERRUPTIBLE); 
	tp_interrupt_flag = 0;
	//printk("++++%s, line %d----end\n", __FUNCTION__, __LINE__);	
        wait_event_interruptible(waiter,tpd_flag!=0);
//	printk("++++%s, line %d----start\n", __FUNCTION__, __LINE__);

        tpd_flag = 0; //debg 2013-5-6
        set_current_state(TASK_RUNNING);
		
	exit_boot_mode();
	    if (tpd_touchinfo(&cinfo, &pinfo))
   		{
			memset(keeppoint, 0x0, sizeof(keeppoint));
			if(cinfo.count >0 && cinfo.count < (TP_POINTS_CNT+1))
			{
				switch(cinfo.count)
				{
					case 5:
					{
						tpd_down(cinfo.x[4], cinfo.y[4], cinfo.p[4]);
					}
					case 4:
					{
						tpd_down(cinfo.x[3], cinfo.y[3], cinfo.p[3]);
					}
					case 3:
					{
						tpd_down(cinfo.x[2], cinfo.y[2], cinfo.p[2]);
					}
					case 2:
					{
						tpd_down(cinfo.x[1], cinfo.y[1], cinfo.p[1]);
					}
					case 1:
					{
						tpd_down(cinfo.x[0], cinfo.y[0], cinfo.p[0]);
					}
					default:
						break;
				}
				for(i = 0; i < cinfo.count; i++)
					for(j = 0; j < pinfo.count; j++)
					{
						if(cinfo.id[i] == pinfo.id[j])keeppoint[j] = 1;
						else if(keeppoint[j] != 1)keeppoint[j] = 0;
					}

				for(j = 0; j < pinfo.count; j++)
				{
					if(keeppoint[j] != 1)
					{
						tpd_up(pinfo.x[j], pinfo.y[j], pinfo.p[j]);
					}
				}

			}
			else if(cinfo.count == 0 && pinfo.count !=0)
			{
				switch(pinfo.count )
				{
					case 5:
					{
						tpd_up(pinfo.x[4], pinfo.y[4], pinfo.p[4]);
					}
					case 4:
					{
						tpd_up(pinfo.x[3], pinfo.y[3], pinfo.p[3]);
					}
					case 3:
					{
						tpd_up(pinfo.x[2], pinfo.y[2], pinfo.p[2]);
					}
					case 2:
					{
						tpd_up(pinfo.x[1], pinfo.y[1], pinfo.p[1]);
					}
					case 1:
					{
						tpd_up(pinfo.x[0], pinfo.y[0], pinfo.p[0]);
					}
					default:
						break;
				}
			}

			input_sync(tpd->input);

	    }

		

	}while(!kthread_should_stop());
	tp_interrupt_flag = 0;

    return 0;
}




static irqreturn_t tpd_eint_interrupt_handler(int irq, void *dev_id)
{
	static int i = 0;
	i ++;
	//printk("++++eint=%d\n",i);

	//mt65xx_eint_mask(CUST_EINT_TOUCH_PANEL_NUM);
//	printk("++++%s, line %d, tpd_flag=%d,i=%d\n", __FUNCTION__, __LINE__, tpd_flag,i);
	
	if (wmt_is_tsint())
	{ 
 	   // printk("<<<<in %s\n", __FUNCTION__);
	wmt_clr_int();
			//return IRQ_HANDLED;//!!!!!
	if (wmt_is_tsirq_enable())
	{
		wmt_disable_gpirq();
	}	 
	tp_interrupt_flag = 1;
	////TPD_DEBUG("TPD interrupt has been triggered\n");
	//if(tpd_flag)
		//return;
	tpd_flag = 1;
	wake_up_interruptible(&waiter);
    
	return IRQ_HANDLED;

	}
 return IRQ_NONE;
}  
static void ctp_power_on(int on)
{
	printk("++++ctp_power_on = %d\n",on);	
	//return ;

	if(on == 1)
	{
		//mt_set_gpio_mode(GPIO_CTP_EN_PIN, GPIO_CTP_EN_PIN_M_GPIO);
  		//mt_set_gpio_dir(GPIO_CTP_EN_PIN, GPIO_DIR_OUT);
		//mt_set_gpio_out(GPIO_CTP_EN_PIN, GPIO_OUT_ONE);
		;       

	}
	else
	{
		//return -EIO;
		;
	}
}
//}

#include "cyttsp.h"
extern void cyttsp_fw_upgrade(void);
void cyttsp_hw_reset(void)
{
	ctp_power_on(0);	//wujinyou
	msleep(200);

	ctp_power_on(1);	//wujinyou
	msleep(100);
	//mt_set_gpio_mode(GPIO_CTP_RST_PIN, GPIO_CTP_RST_PIN_M_GPIO);
//	mt_set_gpio_dir(GPIO_CTP_RST_PIN, GPIO_DIR_OUT);
	//mt_set_gpio_out(GPIO_CTP_RST_PIN, GPIO_OUT_ONE);
	msleep(100);
	//mt_set_gpio_out(GPIO_CTP_RST_PIN, GPIO_OUT_ZERO);    
	msleep(100);
}
void cyttsp_sw_reset(void)
{
	//int retval = TPD_OK;
//	int status = 0;
	printk("++++cyttsp_sw_reset---------start\n");	
#if 0//1
	mt_set_gpio_mode(GPIO_CTP_RST_PIN, GPIO_CTP_RST_PIN_M_GPIO);
	mt_set_gpio_dir(GPIO_CTP_RST_PIN, GPIO_DIR_OUT);
	mt_set_gpio_out(GPIO_CTP_RST_PIN, GPIO_OUT_ONE);
	msleep(20);
	ctp_power_on(0);
	msleep(200);
	#if 1
	ctp_power_on(1);
	#endif

	msleep(20);
	mt_set_gpio_out(GPIO_CTP_RST_PIN, GPIO_OUT_ZERO);    
	msleep(100);

	////TPD_DEBUG("TPD wake up\n");
	status = i2c_master_send(i2c_client, bl_cmd, 12);
	if( status < 0)
	{
		printk("++++ [cyttsp_sw_reset], cyttsp tpd exit bootloader mode failed--tpd_resume!\n");
		return status;
	}
	msleep(300);
	//exit_boot_mode();
	//mt65xx_eint_unmask(CUST_EINT_TOUCH_PANEL_NUM);  
#endif
	printk("++++cyttsp_sw_reset---------end\n");	
	//return retval;
}


//***************cyp140 probe 2013-1-6
// wmtenv set wmt.io.touch 1:cyp140:7:600:1024:4:0:1:-1:5 //ok  2013-5-8
static int __devinit tpd_probe(struct i2c_client *client)
{	 
	struct input_dev *input_dev;
	int retval = TPD_OK;
	int gpio_irq = wmt_ts_get_irqgpnum();
	int gpio_rst = wmt_ts_get_resetgpnum();
	//int result;
	i2c_client = client;

	retval = gpio_request(gpio_irq, "ts_irq");
	if (retval < 0) {
		printk("gpio(%d) touchscreen irq request fail\n", gpio_irq);
		return retval;
	}

	retval = gpio_request(gpio_rst, "ts_rst");
	if (retval < 0) {
		printk("gpio(%d) touchscreen reset request fail\n", gpio_rst);
		goto Fail_request_rstgpio;
	}
	//char buffer[2];
	//int status=0;
    
	//int res_x, res_y;
	  
	printk("<<< enter %s: %d\n",__FUNCTION__, __LINE__);
	
#if 1 //0
	tilt = wmt_ts_get_xaxis();
	rev_x = wmt_ts_get_xdir();
  	rev_y = wmt_ts_get_ydir();
#if 0	
  	if (tilt){
		max_y = wmt_ts_get_resolvX();
		max_x = wmt_ts_get_resolvY();
	}
	else
	{
		max_x = wmt_ts_get_resolvX();
		max_y =wmt_ts_get_resolvY();
	}
#else	
	max_x = wmt_ts_get_resolvX();
	max_y =wmt_ts_get_resolvY();
#endif

#endif
#if 0  
	if (0)
	{
	  	res_x = max_y;
	  	res_y = max_x;
	}	
	else
	{
	  	res_x = max_x;
	  	res_y = max_y;
	}	
	max_x = res_x;
	max_y = res_y;
#endif 
	 //************************add input device 2013-1-6
	 tpd = kzalloc(sizeof(struct tpd_device), GFP_KERNEL);
	 
	 input_dev = input_allocate_device();
	 if (!input_dev || !tpd) {
		return -ENOMEM;
	}
	
	tpd->client/*i2c_ts*/ = client;
	i2c_set_clientdata(client, tpd);
	tpd->input = input_dev;
	
	input_dev->name = "touch_cyp140"; //MJ5_TS_NAME;
	input_dev->phys = "cyp140_touch/input0";
	input_dev->id.bustype = BUS_HOST;
	input_dev->id.vendor = 0x0001;
	input_dev->id.product = 0x0002;
	input_dev->id.version = 0x0100;
	
	input_dev->evbit[0] = BIT_MASK(EV_SYN) | BIT_MASK(EV_KEY) | BIT_MASK(EV_ABS);
	set_bit(INPUT_PROP_DIRECT, input_dev->propbit);
	input_set_abs_params(input_dev, ABS_MT_POSITION_X, 0, max_x/*480*//*600*//*ResolutionX*//*ResolutionX*/, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_POSITION_Y, 0, max_y /*ResolutionY*//*800*//* 1024*/, 0, 0);   

	set_bit(KEY_BACK, input_dev->keybit);
	set_bit(KEY_HOME, input_dev->keybit);
	set_bit(KEY_MENU, input_dev->keybit);
	retval = input_register_device(input_dev);
	if (retval)
	{
		printk("%s input register device error!!\n", __FUNCTION__);
		goto E_REG_INPUT;
	}	
	 //****************************
	//ctp_power_on(1);	//wujinyou //!!!!2013-1-6

	msleep(1000);

	//printk("<<<<here ??/\n"); 
 //************************add for wmt 2013-1-6
	wmt_tsreset_init();
	wmt_set_rst_pull(1);
	//wmt_enable_rst_pull(1);
	wmt_rst_output(1);
  
	wmt_set_gpirq(IRQ_TYPE_EDGE_FALLING);
	//wmt_set_gpirq(IRQ_TYPE_EDGE_RISING); //debug 2013-5-8 also no interrupt
	wmt_disable_gpirq();	
	
	tpd->irq = wmt_get_tsirqnum(); 
	retval = request_irq(tpd->irq, tpd_eint_interrupt_handler,IRQF_SHARED, "cypcm", tpd);	
 //****************************************
 
	printk("tpd_probe request_irq retval=%d!\n",retval);
	msleep(100);
	msleep(1000);
	
	cust_ts.client = i2c_client;
//	cyttsp_fw_upgrade(); //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

#if 0  //by linda 20130126
	status = i2c_smbus_read_i2c_block_data(i2c_client, 0x01, 1, &(buffer[0]));
	printk("tpd_probe request_irq status=%d!\n",status);

	retval = i2c_master_send(i2c_client, bl_cmd, 12);
	if( retval < 0)
	{
		printk("tpd_probe i2c_master_send retval=%d!\n",retval);
		
		//return retval;
		goto I2C_ERR;
	}
#else
	retval = exit_boot_mode(); 
	if (retval)
	{
		printk("%s exit_boot_mod error!\n", __FUNCTION__);
		goto I2C_ERR;
	}

#endif
/*
	msleep(1000);
    retval = i2c_smbus_read_i2c_block_data(i2c_client, 0x00, 1, &(buffer[0]));
    if(retval<0) {
        retval = i2c_smbus_read_i2c_block_data(i2c_client, 0x00, 1, &(buffer[0]));
        if(retval<0) {
        	printk("error read !%d\n", __LINE__); 
            
           goto I2C_ERR;
        }
    }
*/
    //TPD_DEBUG("[mtk-tpd], cyttsp tpd_i2c_probe success!!\n");		
	tpd_load_status = 1;
	thread = kthread_run(touch_event_handler, 0, TPD_DEVICE);
	if (IS_ERR(thread)) { 
		retval = PTR_ERR(thread);
		return retval;
       
	}

 
	msleep(100);
	printk("++++tpd_probe,retval=%d\n", retval);
#ifdef CONFIG_HAS_EARLYSUSPEND	
	tpd->early_suspend.suspend = tpd_early_suspend,
	tpd->early_suspend.resume = tpd_late_resume,
	tpd->early_suspend.level = EARLY_SUSPEND_LEVEL_DISABLE_FB + 1;//,EARLY_SUSPEND_LEVEL_DISABLE_FB + 2;
	register_early_suspend(&tpd->early_suspend);
#endif	
	//disable_irq(cyp140_ts->irq);
	
	wmt_enable_gpirq();
   
//cust_timer_init();	
//	esd_init_thread();  //close it 2013-1-6
	return 0; //retval;
	I2C_ERR:
		free_irq(tpd->irq, tpd);
		input_unregister_device(input_dev); //
	E_REG_INPUT:
		input_free_device(input_dev);
		kfree(tpd);
		//return retval;
	Fail_request_rstgpio:
		gpio_free(gpio_rst);
		gpio_free(gpio_irq);
		return retval;		

}
//*******************************

//module_init(cyp140_ts_init);
static int tpd_local_init(void)
{
	if (tpd_probe(ts_get_i2c_client()))// ????
	{
		return -1;
	}
	
	
	if(tpd_load_status == 0){
	//return -1;
		;
	}

#ifdef TPD_HAVE_BUTTON
	tpd_button_setting(TPD_KEY_COUNT, tpd_keys_local, tpd_keys_dim_local);// initialize tpd button data
	boot_mode = get_boot_mode();
#endif
  return 0;//!!!!2013-1-7
}

//**********************suspend & resume
static int tpd_resume(/*struct i2c_client *client*/struct platform_device *pdev)
{
	int retval = TPD_OK;
	int status = 0;
	printk("++++%s, line %d----end\n", __FUNCTION__, __LINE__);
	//mt65xx_eint_mask(CUST_EINT_TOUCH_PANEL_NUM);  
	msleep(100);
	#if 1
	ctp_power_on(1);
	#endif
	
	msleep(1);
	wmt_rst_output(0);
	msleep(1);
	wmt_rst_output(1);    
	msleep(100);
	
	wmt_set_gpirq(IRQ_TYPE_EDGE_FALLING);//sometimes gpio7 will in low after resume 2013-5-9
	wmt_enable_gpirq();
	printk("++++%s, line %d----end\n", __FUNCTION__, __LINE__);

	//TPD_DEBUG("TPD wake up\n");

	#if 0 //0   // by linda 20120126 change rambo 2013-5-6
	status = i2c_master_send(i2c_client, bl_cmd, 12);
	#else
	exit_boot_mode();
	#endif
	printk("++++%s, line %d----end\n", __FUNCTION__, __LINE__);
	
	if( status < 0)
	{
		printk("++++ [mtk-tpd], cyttsp tpd exit bootloader mode failed--tpd_resume!\n");
		return status;
	}
	//exit_boot_mode();
	printk("++++%s, line %d----end\n", __FUNCTION__, __LINE__);
	msleep(300);
	//wmt_enable_gpirq(); //debg 2013-5-6
	tp_suspend_flag = 0;
	return retval;
}

static int tpd_suspend(/*struct i2c_client *client*/struct platform_device *pdev, pm_message_t message)
{
	int i = 0;
	int retval = TPD_OK;
	//u8 sleep_mode = 0x02;	// 0x02--CY_DEEP_SLEEP_MODE,   0x04--CY_LOW_PWR_MODE
	//TPD_DEBUG("TPD enter sleep\n");
	//u8 sleep_reg[2] = {0, 2};
	printk("++++%s, line %d----end\n", __FUNCTION__, __LINE__);
	wmt_disable_gpirq();
	//wmt_disable_gpirq(); //dbg 2013-5-6

	while((tp_reseting_flag || tp_interrupt_flag) && i<30)
	{
		i ++;
		msleep(100);
	}
	tp_suspend_flag = 1;
#if 1
	//retval = i2c_smbus_write_i2c_block_data(i2c_client,0x00,sizeof(sleep_mode), &sleep_mode);
	//retval = i2c_master_send(i2c_client, sleep_reg, 2); //send cmd error -5!
	msleep(1);
	ctp_power_on(0);
	mdelay(1);
#else
	mt_set_gpio_mode(GPIO_CTP_RST_PIN, GPIO_CTP_RST_PIN_M_GPIO);
	mt_set_gpio_dir(GPIO_CTP_RST_PIN, GPIO_DIR_OUT);
	mt_set_gpio_out(GPIO_CTP_RST_PIN, GPIO_OUT_ONE);    
#endif

	return retval;
}

#ifdef CONFIG_HAS_EARLYSUSPEND
static void tpd_early_suspend(struct early_suspend *handler)
{
	tpd_suspend(i2c_client, PMSG_SUSPEND);
}

static void tpd_late_resume(struct early_suspend *handler)
{
	tpd_resume(i2c_client);
}
#endif
//****************************


static void cyp140_ts_exit(void)
{
	printk("<<<%s\n", __FUNCTION__);

	wmt_disable_gpirq();	
	free_irq(tpd->irq, tpd);
	//kthread_stop(thread); //  halt rmmod??   
	input_unregister_device(tpd->input); //
	
	input_free_device(tpd->input);
	kfree(tpd);
	gpio_free(wmt_ts_get_irqgpnum());
	gpio_free(wmt_ts_get_resetgpnum());
}
//module_exit(cyp140_ts_exit);

void cyp140_set_ts_mode(u8 mode)
{
	dbg( "[Touch Screen]ts mode = %d \n", mode);
}
//EXPORT_SYMBOL_GPL(cyp140_set_ts_mode);

struct wmtts_device cyp140_tsdev = {
		.driver_name = WMT_TS_I2C_NAME,
		.ts_id = "cyp140",
		.init = tpd_local_init,
		.exit = cyp140_ts_exit,
		.suspend = tpd_suspend,
		.resume = tpd_resume,
};



MODULE_DESCRIPTION("cyp140 I2C Touch Screen driver");
MODULE_LICENSE("GPL v2");
