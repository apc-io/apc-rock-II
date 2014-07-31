/* drivers/input/touchscreen/zet6221_i2c.c
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
#include <linux/kthread.h>
#include <linux/i2c.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/wakelock.h>
#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif 
#include <linux/slab.h>
#include <linux/irq.h>
#include <linux/errno.h>
#include <linux/fs.h> 
#include <linux/file.h> 
#include <asm/uaccess.h>

#include <linux/input/mt.h>
#include "wmt_ts.h"
#include <linux/power/wmt_charger_common.h>
#include "../../../video/backlight/wmt_bl.h"


//fw update.
//#include "zet6221_fw.h"

/* -------------- global variable definition -----------*/
#define _MACH_MSM_TOUCH_H_

#define ZET_TS_ID_NAME "zet6221-ts"

#define MJ5_TS_NAME "touch_zet6221"

//#define TS_INT_GPIO		S3C64XX_GPN(9)  /*s3c6410*/
//#define TS1_INT_GPIO        AT91_PIN_PB17 /*AT91SAM9G45 external*/
//#define TS1_INT_GPIO        AT91_PIN_PA27 /*AT91SAM9G45 internal*/
//#define TS_RST_GPIO		S3C64XX_GPN(10)

//#define MT_TYPE_B

#define TS_RST_GPIO
#define X_MAX	800 //1024
#define Y_MAX	480 //576
#define FINGER_NUMBER 5
#define KEY_NUMBER 3 //0
//#define P_MAX	1
#define P_MAX	255 //modify 2013-1-1
#define D_POLLING_TIME	25000
#define U_POLLING_TIME	25000
#define S_POLLING_TIME  100
#define REPORT_POLLING_TIME  3
#define RETRY_DOWNLOAD_TIMES 2

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

#define I2C_MAJOR 126
#define I2C_MINORS 256


///=============================================================================================///
/// IOCTL control Definition
///=============================================================================================///
#define ZET_IOCTL_CMD_FLASH_READ				(20)
#define ZET_IOCTL_CMD_FLASH_WRITE			(21)
#define ZET_IOCTL_CMD_RST      					(22)
#define ZET_IOCTL_CMD_RST_HIGH 		   		(23)
#define ZET_IOCTL_CMD_RST_LOW    				(24)

#define ZET_IOCTL_CMD_DYNAMIC					(25)

#define ZET_IOCTL_CMD_FW_FILE_PATH_GET		(26)
#define ZET_IOCTL_CMD_FW_FILE_PATH_SET   		(27)

#define ZET_IOCTL_CMD_MDEV   					(28)
#define ZET_IOCTL_CMD_MDEV_GET   				(29)

#define ZET_IOCTL_CMD_TRAN_TYPE_PATH_GET	(30)
#define ZET_IOCTL_CMD_TRAN_TYPE_PATH_SET	(31)

#define ZET_IOCTL_CMD_IDEV   					(32)
#define ZET_IOCTL_CMD_IDEV_GET   				(33)

#define ZET_IOCTL_CMD_MBASE   					(34)
#define ZET_IOCTL_CMD_MBASE_GET  				(35)

#define ZET_IOCTL_CMD_INFO_SET				(36)
#define ZET_IOCTL_CMD_INFO_GET				(37)

#define ZET_IOCTL_CMD_TRACE_X_SET				(38)
#define ZET_IOCTL_CMD_TRACE_X_GET				(39)
	
#define ZET_IOCTL_CMD_TRACE_Y_SET				(40)
#define ZET_IOCTL_CMD_TRACE_Y_GET				(41)

#define ZET_IOCTL_CMD_IBASE   					(42)
#define ZET_IOCTL_CMD_IBASE_GET   				(43)

#define ZET_IOCTL_CMD_DRIVER_VER_GET		(44)
#define ZET_IOCTL_CMD_MBASE_EXTERN_GET  		(45)

#define IOCTL_MAX_BUF_SIZE          				(1024)

///----------------------------------------------------///
/// IOCTL ACTION
///----------------------------------------------------///
#define IOCTL_ACTION_NONE			(0)
#define IOCTL_ACTION_FLASH_DUMP			(1<<0)

static int ioctl_action = IOCTL_ACTION_NONE;

///=============================================================================================///
///  Transfer type
///=============================================================================================///
#define TRAN_TYPE_DYNAMIC		        (0x00)
#define TRAN_TYPE_MUTUAL_SCAN_BASE         	(0x01)
#define TRAN_TYPE_MUTUAL_SCAN_DEV           	(0x02)
#define TRAN_TYPE_INIT_SCAN_BASE 		(0x03)
#define TRAN_TYPE_INIT_SCAN_DEV		      	(0x04)
#define TRAN_TYPE_KEY_MUTUAL_SCAN_BASE		(0x05)
#define TRAN_TYPE_KEY_MUTUAL_SCAN_DEV 		(0x06)
#define TRAN_TYPE_KEY_DATA  			(0x07)
#define TRAN_TYPE_MTK_TYPE  			(0x0A)
#define TRAN_TYPE_FOCAL_TYPE  		        (0x0B)

///=============================================================================================///
///  TP Trace
///=============================================================================================///
#define TP_DEFAULT_ROW 				(10)
#define TP_DEFAULT_COL 				(15)

#define DRIVER_VERSION "$Revision: 44 $"
//static char const *revision="$Revision: 44 $";

///=============================================================================================///
/// Macro Definition
///=============================================================================================///
#define MAX_FLASH_BUF_SIZE			(0x10000)

///---------------------------------------------------------------------------------///
///  18. IOCTRL Debug
///---------------------------------------------------------------------------------///
#define FEATURE_IDEV_OUT_ENABLE
#define FEATURE_MBASE_OUT_ENABLE
#define FEATURE_MDEV_OUT_ENABLE
#define FEATURE_INFO_OUT_EANBLE
#define FEATURE_IBASE_OUT_ENABLE



///-------------------------------------///
///  firmware save / load
///-------------------------------------///
u32 data_offset 			= 0;
struct inode *inode 			= NULL;
mm_segment_t old_fs;

char driver_version[128];

//#define FW_FILE_NAME 			"/vendor/modules/zet62xx.bin"
char fw_file_name[128];
///-------------------------------------///
///  Transmit Type Mode Path parameters 
///-------------------------------------///
///  External SD-Card could be
///      "/mnt/sdcard/"
///      "/mnt/extsd/"
///-------------------------------------///

// It should be the path where adb tools can push files in
#define TRAN_MODE_FILE_PATH		"/data/local/tmp/"
char tran_type_mode_file_name[128];
u8 *tran_data = NULL;

///-------------------------------------///
///  Mutual Dev Mode  parameters 
///-------------------------------------///
///  External SD-Card could be
///      "/mnt/sdcard/zetmdev"
///      "/mnt/extsd/zetmdev"
///-------------------------------------///
#ifdef FEATURE_MDEV_OUT_ENABLE
	#define MDEV_FILE_NAME		"zetmdev"
	#define MDEV_MAX_FILE_ID	(10)
	#define MDEV_MAX_DATA_SIZE	(2048)
///-------------------------------------///
///  mutual dev variables
///-------------------------------------///
	u8 *mdev_data = NULL;
	int mdev_file_id = 0;
#endif ///< FEATURE_MDEV_OUT_ENABLE

///-------------------------------------///
///  Initial Base Mode  parameters 
///-------------------------------------///
///  External SD-Card could be
///      "/mnt/sdcard/zetibase"
///      "/mnt/extsd/zetibase"
///-------------------------------------///
#ifdef FEATURE_IBASE_OUT_ENABLE
	#define IBASE_FILE_NAME		"zetibase"
	#define IBASE_MAX_FILE_ID	(10)
	#define IBASE_MAX_DATA_SIZE	(512)

///-------------------------------------///
///  initial base variables
///-------------------------------------///
	u8 *ibase_data = NULL;
	int ibase_file_id = 0;
#endif ///< FEATURE_IBASE_OUT_ENABLE

///-------------------------------------///
///  Initial Dev Mode  parameters 
///-------------------------------------///
///  External SD-Card could be
///      "/mnt/sdcard/zetidev"
///      "/mnt/extsd/zetidev"
///-------------------------------------///
#ifdef FEATURE_IDEV_OUT_ENABLE
	#define IDEV_FILE_NAME		"zetidev"
	#define IDEV_MAX_FILE_ID	(10)	
	#define IDEV_MAX_DATA_SIZE	(512)

///-------------------------------------///
///  initial dev variables
///-------------------------------------///
	u8 *idev_data = NULL;
	int idev_file_id = 0;
#endif ///< FEATURE_IDEV_OUT_ENABLE

///-------------------------------------///
///  Mutual Base Mode  parameters 
///-------------------------------------///
///  External SD-Card could be
///      "/mnt/sdcard/zetmbase"
///      "/mnt/extsd/zetmbase"
///-------------------------------------///
#ifdef FEATURE_MBASE_OUT_ENABLE
	#define MBASE_FILE_NAME		"zetmbase"
	#define MBASE_MAX_FILE_ID	(10)
	#define MBASE_MAX_DATA_SIZE	(2048)

///-------------------------------------///
///  mutual base variables
///-------------------------------------///
	u8 *mbase_data = NULL;
	int mbase_file_id = 0;
#endif ///< FEATURE_MBASE_OUT_ENABLE

///-------------------------------------///
///  infomation variables
///-------------------------------------///
#ifdef FEATURE_INFO_OUT_EANBLE
	#define INFO_MAX_DATA_SIZE	(64)
	#define INFO_DATA_SIZE		(17)
	#define ZET6221_INFO		(0x00)
	#define ZET6231_INFO		(0x0B)
	#define ZET6223_INFO		(0x0D)
	#define ZET6251_INFO		(0x0C)	
	#define UNKNOW_INFO		(0xFF)
	u8 *info_data = NULL;
#endif ///< FEATURE_INFO_OUT_EANBLE
///-------------------------------------///
///  Default transfer type
///-------------------------------------///
u8 transfer_type = TRAN_TYPE_DYNAMIC;

///-------------------------------------///
///  Default TP TRACE
///-------------------------------------///
int row = TP_DEFAULT_ROW;
int col = TP_DEFAULT_COL;

struct msm_ts_platform_data {
	unsigned int x_max;
	unsigned int y_max;
	unsigned int pressure_max;
};

struct zet6221_tsdrv {
	struct i2c_client *i2c_ts;
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

struct i2c_dev
{
	struct list_head list;	
	struct i2c_adapter *adap;
	struct device *dev;
};

static struct i2c_dev *zet_i2c_dev;
static struct class *i2c_dev_class;
static LIST_HEAD (i2c_dev_list);
static DEFINE_SPINLOCK(i2c_dev_list_lock);

struct zet6221_tsdrv * l_ts = NULL;
static int l_suspend = 0; // 1:suspend, 0:normal state

//static int resetCount = 0;          //albert++ 20120807


//static u16 polling_time = S_POLLING_TIME;

static int l_powermode = -1;
static struct mutex i2c_mutex;
static struct wake_lock downloadWakeLock;


//static int __devinit zet6221_ts_probe(struct i2c_client *client, const struct i2c_device_id *id);
//static int __devexit zet6221_ts_remove(struct i2c_client *dev);
extern int register_bl_notifier(struct notifier_block *nb);

extern int unregister_bl_notifier(struct notifier_block *nb);

extern int  zet6221_downloader( struct i2c_client *client/*, unsigned short ver, unsigned char * data */);
extern int zet622x_resume_downloader(struct i2c_client *client);
extern u8 zet6221_ts_version(void);
extern u8 zet6221_ts_get_report_mode_t(struct i2c_client *client);
extern u8 zet622x_ts_option(struct i2c_client *client);
extern int zet6221_load_fw(void);
extern int zet6221_free_fwmem(void);

void zet6221_ts_charger_mode_disable(void);
void zet6221_ts_charger_mode(void);
static int  zet_fw_size(void);
static void zet_fw_save(char *file_name);
static void zet_fw_load(char *file_name);
static void zet_fw_init(void);
#ifdef FEATURE_MDEV_OUT_ENABLE
static void zet_mdev_save(char *file_name);
#endif ///< FEATURE_MDEV_OUT_ENABLE
#ifdef FEATURE_IDEV_OUT_ENABLE
static void zet_idev_save(char *file_name);
#endif ///< FEATURE_IDEV_OUT_ENABLE
#ifdef FEATURE_IBASE_OUT_ENABLE
static void zet_ibase_save(char *file_name);
#endif ///< FEATURE_IBASE_OUT_ENABLE
#ifdef FEATURE_MBASE_OUT_ENABLE
static void zet_mbase_save(char *file_name);
#endif ///< FEATURE_MBASE_OUT_ENABLE
static void zet_information_save(char *file_name);

static struct task_struct *resume_download_task;



//static int filterCount = 0; 
//static u32 filterX[MAX_FINGER_NUMBER][2], filterY[MAX_FINGER_NUMBER][2]; 

//static u8  key_menu_pressed = 0x1;
//static u8  key_back_pressed = 0x1;
//static u8  key_search_pressed = 0x1;

static u16 ResolutionX=X_MAX;
static u16 ResolutionY=Y_MAX;
static u16 FingerNum=0;
static u16 KeyNum=0;
static int bufLength=0;	
static u8 xyExchange=0;
static u16 inChargerMode = 0;
static struct i2c_client *this_client;
struct workqueue_struct *ts_wq = NULL;
static int l_tskey[4][2] = {
	{KEY_BACK,0},
	{KEY_MENU,0},
	{KEY_HOME,0},
	{KEY_SEARCH,0},
};

u8 pc[8];
// {IC Model, FW Version, FW version,Codebase Type=0x08, Customer ID, Project ID, Config Board No, Config Serial No}

//Touch Screen
/*static const struct i2c_device_id zet6221_ts_idtable[] = {
       { ZET_TS_ID_NAME, 0 },
       { }
};

static struct i2c_driver zet6221_ts_driver = {
	.driver = {
		.owner = THIS_MODULE,
		.name  = ZET_TS_ID_NAME,
	},
	.probe	  = zet6221_ts_probe,
	.remove		= __devexit_p(zet6221_ts_remove),
	.id_table = zet6221_ts_idtable,
};
*/

void zet6221_set_tskey(int index,int key)
{
	l_tskey[index][0] = key;
}


void check_charger(void)
{
	mutex_lock(&i2c_mutex);
	if (!wmt_charger_is_dc_plugin())
	{
		klog("disable_mode\n");
		zet6221_ts_charger_mode_disable();
	} else {
		klog("charge mode\n");
		zet6221_ts_charger_mode();
	}
	mutex_unlock(&i2c_mutex);
	l_powermode = wmt_charger_is_dc_plugin();
}


void check_charger_polling(void)
{
	if(l_suspend == 1)
	{
		return;
	}
	
	if (wmt_charger_is_dc_plugin() != l_powermode)
	{
		check_charger();
	}

	///-------------------------------------------------------------------///
	/// IOCTL Action
	///-------------------------------------------------------------------///
	if(ioctl_action  & IOCTL_ACTION_FLASH_DUMP)
	{
		printk("[ZET]: IOCTL_ACTION: Dump flash\n");
		zet_fw_save(fw_file_name);
		ioctl_action &= ~IOCTL_ACTION_FLASH_DUMP;
	}

	return;
}



//extern unsigned int wmt_bat_is_batterypower(void);
/***********************************************************************
    [function]: 
		        callback: Timer Function if there is no interrupt fuction;
    [parameters]:
			    arg[in]:  arguments;
    [return]:
			    NULL;
************************************************************************/

static void polling_timer_func(struct work_struct *work)
{
	struct zet6221_tsdrv *ts = l_ts;
	//schedule_work(&ts->work1);
	//queue_work(ts_wq,&ts->work1);
	//dbg("check mode!\n");
/*
	if (wmt_bat_is_batterypower() != l_powermode)
	{
		mutex_lock(&i2c_mutex);
		if (wmt_bat_is_batterypower())
		{
			klog("disable_mode\n");
			zet6221_ts_charger_mode_disable();
		} else {
			klog("charge mode\n");
			zet6221_ts_charger_mode();
		}
		mutex_unlock(&i2c_mutex);
		l_powermode = wmt_bat_is_batterypower();
	}
*/

	check_charger_polling();
	queue_delayed_work(ts->queue, &ts->work, msecs_to_jiffies(TIME_CHECK_CHARGE));

	
	//mod_timer(&ts->polling_timer,jiffies + msecs_to_jiffies(TIME_CHECK_CHARGE));
}



///**********************************************************************
///   [function]:  zet622x_i2c_get_free_dev
///   [parameters]: adap
///   [return]: void
///**********************************************************************
static struct i2c_dev *zet622x_i2c_get_free_dev(struct i2c_adapter *adap) 
{
	struct i2c_dev *i2c_dev;

	if (adap->nr >= I2C_MINORS)
	{
		printk("[ZET] : i2c-dev:out of device minors (%d) \n",adap->nr);
		return ERR_PTR (-ENODEV);
	}

	i2c_dev = kzalloc(sizeof(*i2c_dev), GFP_KERNEL);
	if (!i2c_dev)
	{
		return ERR_PTR(-ENOMEM);
	}
	i2c_dev->adap = adap;

	spin_lock(&i2c_dev_list_lock);
	list_add_tail(&i2c_dev->list, &i2c_dev_list);
	spin_unlock(&i2c_dev_list_lock);
	
	return i2c_dev;
}

///**********************************************************************
///   [function]:  zet622x_i2c_dev_get_by_minor
///   [parameters]: index
///   [return]: i2c_dev
///**********************************************************************
static struct i2c_dev *zet622x_i2c_dev_get_by_minor(unsigned index)
{
	struct i2c_dev *i2c_dev;
	spin_lock(&i2c_dev_list_lock);
	
	list_for_each_entry(i2c_dev, &i2c_dev_list, list)
	{
		printk(" [ZET] : line = %d ,i2c_dev->adapt->nr = %d,index = %d.\n",__LINE__,i2c_dev->adap->nr,index);
		if(i2c_dev->adap->nr == index)
		{
		     goto LABEL_FOUND;
		}
	}
	i2c_dev = NULL;
	
LABEL_FOUND: 
	spin_unlock(&i2c_dev_list_lock);
	
	return i2c_dev ;
}

	

//extern int wmt_i2c_xfer_continue_if_4(struct i2c_msg *msg, unsigned int num,int bus_id);
/***********************************************************************
    [function]: 
		        callback: read data by i2c interface;
    [parameters]:
			    client[in]:  struct i2c_client â€?represent an I2C slave device;
			    data [out]:  data buffer to read;
			    length[in]:  data length to read;
    [return]:
			    Returns negative errno, else the number of messages executed;
************************************************************************/
int zet6221_i2c_read_tsdata(struct i2c_client *client, u8 *data, u8 length)
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
			    client[in]:  struct i2c_client â€?represent an I2C slave device;
			    data [out]:  data buffer to write;
			    length[in]:  data length to write;
    [return]:
			    Returns negative errno, else the number of messages executed;
************************************************************************/
int zet6221_i2c_write_tsdata(struct i2c_client *client, u8 *data, u8 length)
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
	msleep(5);
	dbg("has done\n");
#else
	u8 ts_reset_cmd[1] = {0xb0};
	zet6221_i2c_write_tsdata(this_client, ts_reset_cmd, 1);
#endif

}


///**********************************************************************
///   [function]:  zet622x_ts_parse_mutual_dev
///   [parameters]: client
///   [return]: u8
///**********************************************************************
#ifdef FEATURE_MDEV_OUT_ENABLE
u8 zet622x_ts_parse_mutual_dev(struct i2c_client *client)
{
	int mdev_packet_size = (row+2) * (col + 2);
	int ret = 0;
	int idx = 0;
	int len =  mdev_packet_size;
	char mdev_file_name_out[128];
	
	int step_size = col + 2;
	
	while(len > 0)
	{
		if(len < step_size)
		{
			step_size = len;
		}

		ret = zet6221_i2c_read_tsdata(client, &tran_data[idx], step_size);
		len -= step_size;
		idx += step_size;
	}
	
	sprintf(mdev_file_name_out, "%s%s%02d.bin", tran_type_mode_file_name, MDEV_FILE_NAME, mdev_file_id);	
	zet_mdev_save(mdev_file_name_out);
	mdev_file_id  =  (mdev_file_id +1)% (MDEV_MAX_FILE_ID);
	return ret;
}
#endif ///< FEATURE_MDEV_OUT_ENABLE

///**********************************************************************
///   [function]:  zet622x_ts_parse_initial_base
///   [parameters]: client
///   [return]: u8
///**********************************************************************
#ifdef FEATURE_IBASE_OUT_ENABLE
u8 zet622x_ts_parse_initial_base(struct i2c_client *client)
{
	int ibase_packet_size = (row + col) * 2;
	int ret = 0;
	int idx = 0;
	int len =  ibase_packet_size;
	char ibase_file_name_out[128];
	
	int step_size = ibase_packet_size;
	
	while(len > 0)
	{
		ret = zet6221_i2c_read_tsdata(client, &tran_data[idx], step_size);
		len -= step_size;
	}
	sprintf(ibase_file_name_out, "%s%s%02d.bin", tran_type_mode_file_name, IBASE_FILE_NAME, ibase_file_id);	
	zet_ibase_save(ibase_file_name_out);
	ibase_file_id  =  (ibase_file_id +1)% (IBASE_MAX_FILE_ID);
	return ret;
}
#endif ///< FEATURE_IBASE_OUT_ENABLE

///**********************************************************************
///   [function]:  zet622x_ts_parse_initial_dev
///   [parameters]: client
///   [return]: u8
///**********************************************************************
#ifdef FEATURE_IDEV_OUT_ENABLE
u8 zet622x_ts_parse_initial_dev(struct i2c_client *client)
{
	int idev_packet_size = (row + col);
	int ret = 0;
	int idx = 0;
	int len =  idev_packet_size;
	char idev_file_name_out[128];
	
	int step_size = idev_packet_size;
	
	while(len > 0)
	{
		ret = zet6221_i2c_read_tsdata(client, &tran_data[idx], step_size);
		len -= step_size;
	}
	sprintf(idev_file_name_out, "%s%s%02d.bin", tran_type_mode_file_name, IDEV_FILE_NAME, idev_file_id);	
	zet_idev_save(idev_file_name_out);
	idev_file_id  =  (idev_file_id +1)% (IDEV_MAX_FILE_ID);
	return ret;
}
#endif ///< FEATURE_IDEV_OUT_ENABLE

///**********************************************************************
///   [function]:  zet622x_ts_parse_mutual_base
///   [parameters]: client
///   [return]: u8
///**********************************************************************
#ifdef FEATURE_MBASE_OUT_ENABLE
u8 zet622x_ts_parse_mutual_base(struct i2c_client *client)
{
	int mbase_packet_size = (row * col * 2);
	int ret = 0;
	int idx = 0;
	int len =  mbase_packet_size;
	char mbase_file_name_out[128];
	
	int step_size = col*2;
	
	while(len > 0)
	{
		if(len < step_size)
		{
			step_size = len;
		}

		ret = zet6221_i2c_read_tsdata(client, &tran_data[idx], step_size);
		len -= step_size;
		idx += step_size;
	}
	sprintf(mbase_file_name_out, "%s%s%02d.bin",tran_type_mode_file_name, MBASE_FILE_NAME, mbase_file_id);	
	zet_mbase_save(mbase_file_name_out);
	mbase_file_id  =  (mbase_file_id +1)% (MBASE_MAX_FILE_ID);
	return ret;
}
#endif ///< FEATURE_MBASE_OUT_ENABLE

/***********************************************************************
    [function]: 
		        callback: read finger information from TP;
    [parameters]:
    			client[in]:  struct i2c_client â€?represent an I2C slave device;
			    x[out]:  values of X axis;
			    y[out]:  values of Y axis;
			    z[out]:  values of Z axis;
				pr[out]:  pressed of released status of fingers;
				ky[out]:  pressed of released status of keys;
    [return]:
			    Packet ID;
************************************************************************/
u8 zet6221_ts_get_xy_from_panel(struct i2c_client *client, u32 *x, u32 *y, u32 *z, u32 *pr, u32 *ky)
{
	u8  ts_data[70];
	int ret;
	int i;
	
	memset(ts_data,0,70);

	ret=zet6221_i2c_read_tsdata(client, ts_data, bufLength);
	
	*pr = ts_data[1];
	*pr = (*pr << 8) | ts_data[2];
		
	for(i=0;i<FingerNum;i++)
	{
		x[i]=(u8)((ts_data[3+4*i])>>4)*256 + (u8)ts_data[(3+4*i)+1];
		y[i]=(u8)((ts_data[3+4*i]) & 0x0f)*256 + (u8)ts_data[(3+4*i)+2];
		z[i]=(u8)((ts_data[(3+4*i)+3]) & 0x0f);
	}
		
	//if key enable
	if(KeyNum > 0)
		*ky = ts_data[3+4*FingerNum];

	return ts_data[0];
}

/***********************************************************************
    [function]: 
		        callback: get dynamic report information;
    [parameters]:
    			client[in]:  struct i2c_client â€?represent an I2C slave device;

    [return]:
			    1;
************************************************************************/
u8 zet6221_ts_get_report_mode(struct i2c_client *client)
{
	u8 ts_report_cmd[1] = {0xb2};
	//u8 ts_reset_cmd[1] = {0xb0};
	u8 ts_in_data[17] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
	
	int ret;
	int i;
	int count=0;

	ret=zet6221_i2c_write_tsdata(client, ts_report_cmd, 1);

	if (ret > 0)
	{
		while(1)
		{
			msleep(1);

			//if (gpio_get_value(TS_INT_GPIO) == 0)
			if (wmt_ts_irqinval() == 0)
			{
				dbg( "int low\n");
				ret=zet6221_i2c_read_tsdata(client, ts_in_data, 17);
				
				if(ret > 0)
				{
				
					for(i=0;i<8;i++)
					{
						pc[i]=ts_in_data[i] & 0xff;
					}				
					
					if(pc[3] != 0x08)
					{
						errlog("=============== zet6221_ts_get_report_mode report error ===============\n");
						return 0;
					}

					xyExchange = (ts_in_data[16] & 0x8) >> 3;
					if(xyExchange == 1)
					{
						ResolutionY= ts_in_data[9] & 0xff;
						ResolutionY= (ResolutionY << 8)|(ts_in_data[8] & 0xff);
						ResolutionX= ts_in_data[11] & 0xff;
						ResolutionX= (ResolutionX << 8) | (ts_in_data[10] & 0xff);
					}
					else
					{
						ResolutionX = ts_in_data[9] & 0xff;
						ResolutionX = (ResolutionX << 8)|(ts_in_data[8] & 0xff);
						ResolutionY = ts_in_data[11] & 0xff;
						ResolutionY = (ResolutionY << 8) | (ts_in_data[10] & 0xff);
					}
					
					FingerNum = (ts_in_data[15] & 0x7f);
					KeyNum = (ts_in_data[15] & 0x80);

					if(KeyNum==0)
						bufLength  = 3+4*FingerNum;
					else
						bufLength  = 3+4*FingerNum+1;

					//DPRINTK( "bufLength=%d\n",bufLength);
				
					break;
				
				}else
				{
					errlog ("=============== zet6221_ts_get_report_mode read error ===============\n");
					return 0;
				}
				
			}else
			{
				//DPRINTK( "int high\n");
				if(count++ > 30)
				{
					errlog ("=============== zet6221_ts_get_report_mode time out ===============\n");
					return 0;
				}
				
			}
		}

	}
	return 1;
}

#if 0
static int zet6221_is_ts(struct i2c_client *client)
{
	/*u8 ts_in_data[17] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

	ctp_reset();
	if (zet6221_i2c_read_tsdata(client, ts_in_data, 17) <= 0)
	{
		return 0;
	}
	return 1;*/
	return 1;
}
#endif

/***********************************************************************
    [function]: 
		        callback: get dynamic report information with timer delay;
    [parameters]:
    			client[in]:  struct i2c_client represent an I2C slave device;

    [return]:
			    1;
************************************************************************/

u8 zet6221_ts_get_report_mode_t(struct i2c_client *client)
{
	u8 ts_report_cmd[1] = {0xb2};
	u8 ts_in_data[17] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
	
	int ret;
	int i;
	
	ret=zet6221_i2c_write_tsdata(client, ts_report_cmd, 1);
	msleep(10);
	
	dbg("ret=%d,i2c_addr=0x%x\n", ret, client->addr);
	if (ret > 0)
	{
			//mdelay(10);
			//msleep(10);
			dbg("=============== zet6221_ts_get_report_mode_t ===============\n");
			ret=zet6221_i2c_read_tsdata(client, ts_in_data, 17);
			
			if(ret > 0)
			{
				
				for(i=0;i<8;i++)
				{
					pc[i]=ts_in_data[i] & 0xff;
				}
				
				if(pc[3] != 0x08)
				{
					errlog("=============== zet6221_ts_get_report_mode_t report error ===============\n");
					return 0;
				}

				xyExchange = (ts_in_data[16] & 0x8) >> 3;
				if(xyExchange == 1)
				{
					ResolutionY= ts_in_data[9] & 0xff;
					ResolutionY= (ResolutionY << 8)|(ts_in_data[8] & 0xff);
					ResolutionX= ts_in_data[11] & 0xff;
					ResolutionX= (ResolutionX << 8) | (ts_in_data[10] & 0xff);
				}
				else
				{
					ResolutionX = ts_in_data[9] & 0xff;
					ResolutionX = (ResolutionX << 8)|(ts_in_data[8] & 0xff);
					ResolutionY = ts_in_data[11] & 0xff;
					ResolutionY = (ResolutionY << 8) | (ts_in_data[10] & 0xff);
				}
					
				FingerNum = (ts_in_data[15] & 0x7f);
				KeyNum = (ts_in_data[15] & 0x80);
				inChargerMode = (ts_in_data[16] & 0x2) >> 1;

				if(KeyNum==0)
					bufLength  = 3+4*FingerNum;
				else
					bufLength  = 3+4*FingerNum+1;
				
			}else
			{
				errlog ("=============== zet6221_ts_get_report_mode_t READ ERROR ===============\n");
				return 0;
			}
							
	}else
	{
		errlog("=============== zet6221_ts_get_report_mode_t WRITE ERROR ===============\n");
		return 0;
	}
	return 1;
}

/***********************************************************************
    [function]: 
		        callback: interrupt function;
    [parameters]:
    			irq[in]:  irq value;
    			dev_id[in]: dev_id;

    [return]:
			    NULL;
************************************************************************/
static irqreturn_t zet6221_ts_interrupt(int irq, void *dev_id)
{
	struct zet6221_tsdrv *ts_drv = dev_id;
	int j = 0;
	if (wmt_is_tsint())
		{
			wmt_clr_int();
			if (wmt_is_tsirq_enable() && l_suspend == 0)
			{
				wmt_disable_gpirq();
				dbg("begin..\n");
				//if (!work_pending(&l_tsdata.pen_event_work))
				if (wmt_ts_irqinval() == 0)
				{
					queue_work(ts_wq, &ts_drv->work1);
				} else {
					if(KeyNum > 0)
					{
						//if (0 == ky)
						{
							for (j=0;j<4;j++)
							{
								if (l_tskey[j][1] != 0)
								{
									l_tskey[j][1] = 0;
								}
							}
							dbg("finish one key report!\n");
						} 
					}
					wmt_enable_gpirq();
				}
			}
			return IRQ_HANDLED;
		}
	
	return IRQ_NONE;

	/*//polling_time	= D_POLLING_TIME;

		if (gpio_get_value(TS_INT_GPIO) == 0)
		{
			// IRQ is triggered by FALLING code here
			struct zet6221_tsdrv *ts_drv = dev_id;
			schedule_work(&ts_drv->work1);
			//DPRINTK("TS1_INT_GPIO falling\n");
		}else
		{
			//DPRINTK("TS1_INT_GPIO raising\n");
		}

	return IRQ_HANDLED;*/
}

/***********************************************************************
    [function]: 
		        callback: touch information handler;
    [parameters]:
    			_work[in]:  struct work_struct;

    [return]:
			    NULL;
************************************************************************/
static void zet6221_ts_work(struct work_struct *_work)
{
	u32 x[MAX_FINGER_NUMBER], y[MAX_FINGER_NUMBER], z[MAX_FINGER_NUMBER], pr, ky, points;
	u32 px,py,pz;
	u8 ret;
	u8 pressure;
	int i,j;
	int tx,ty;
	int xmax,ymax;
	int realnum = 0;
	struct zet6221_tsdrv *ts =
		container_of(_work, struct zet6221_tsdrv, work1);

	struct i2c_client *tsclient1 = ts->i2c_ts;

	if(l_suspend == 1)
	{
		return;
	}

	if (bufLength == 0)
	{
		wmt_enable_gpirq();
		return;
	}
	/*if(resetCount == 1)
	{
		resetCount = 0;
		wmt_enable_gpirq();
		return;
	}*/

	//if (gpio_get_value(TS_INT_GPIO) != 0)
	if (wmt_ts_irqinval() != 0)
	{
		/* do not read when IRQ is triggered by RASING*/
		//DPRINTK("INT HIGH\n");
		dbg("INT HIGH....\n");
		wmt_enable_gpirq();
		return;
	}

	///-------------------------------------------///
	/// Transfer Type : Mutual Dev Mode 
	///-------------------------------------------///
#ifdef FEATURE_MDEV_OUT_ENABLE
	if(transfer_type == TRAN_TYPE_MUTUAL_SCAN_DEV)
	{
		zet622x_ts_parse_mutual_dev(tsclient1);
		wmt_enable_gpirq();
		return;	
	}
#endif ///< FEATURE_MDEV_OUT_ENABLE

	///-------------------------------------------///
	/// Transfer Type : Initial Base Mode 
	///-------------------------------------------///
#ifdef FEATURE_IBASE_OUT_ENABLE
	if(transfer_type == TRAN_TYPE_INIT_SCAN_BASE)
	{
		zet622x_ts_parse_initial_base(tsclient1);
		wmt_enable_gpirq();
		return;	
	}	
#endif ///< FEATURE_IBASE_OUT_ENABLE

	///-------------------------------------------///
	/// Transfer Type : Initial Dev Mode 
	///-------------------------------------------///
#ifdef FEATURE_IDEV_OUT_ENABLE
	if(transfer_type == TRAN_TYPE_INIT_SCAN_DEV)
	{
		zet622x_ts_parse_initial_dev(tsclient1);
		wmt_enable_gpirq();
		return;	
	}
#endif ///< TRAN_TYPE_INIT_SCAN_DEV

	///-------------------------------------------///
	/// Transfer Type : Mutual Base Mode 
	///-------------------------------------------///
#ifdef FEATURE_MBASE_OUT_ENABLE
	if(transfer_type == TRAN_TYPE_MUTUAL_SCAN_BASE)
	{
		zet622x_ts_parse_mutual_base(tsclient1);
		wmt_enable_gpirq();
		return;	
	}
#endif ///< FEATURE_MBASE_OUT_ENABLE

	mutex_lock(&i2c_mutex);
	ret = zet6221_ts_get_xy_from_panel(tsclient1, x, y, z, &pr, &ky);
	mutex_unlock(&i2c_mutex);

	if(ret == 0x3C)
	{

		dbg( "x1= %d, y1= %d x2= %d, y2= %d [PR] = %d [KY] = %d\n", x[0], y[0], x[1], y[1], pr, ky);
		
		points = pr;
		
		#if defined(TRANSLATE_ENABLE)
		touch_coordinate_traslating(x, y, points);
		#endif
		realnum = 0;

		for(i=0;i<FingerNum;i++){
			pressure = (points >> (MAX_FINGER_NUMBER-i-1)) & 0x1;
			dbg( "valid=%d pressure[%d]= %d x= %d y= %d\n",points , i, pressure,x[i],y[i]);

			if(pressure)
			{
				px = x[i];
				py = y[i];
				pz = z[i];

				dbg("raw%d(%d,%d) xaxis:%d ResolutionX:%d ResolutionY:%d\n", i, px, py,wmt_ts_get_xaxis(),ResolutionX,ResolutionY);

				//input_report_abs(ts->input, ABS_MT_TRACKING_ID, i);
	    		//input_report_abs(ts->input, ABS_MT_TOUCH_MAJOR, P_MAX);
	    		//input_report_abs(ts->input, ABS_MT_POSITION_X, x[i]);
	    		//input_report_abs(ts->input, ABS_MT_POSITION_Y, y[i]);
	    		if (wmt_ts_get_xaxis() == 0)
	    		{
	    			tx = px;
	    			ty = py;
	    			xmax = ResolutionX;
	    			ymax = ResolutionY;
	    		} else {
	    			tx = py;
	    			ty = px;
	    			xmax = ResolutionY;
	    			ymax = ResolutionX;
	    		}
	    		if (wmt_ts_get_xdir() == -1)
	    		{
	    			tx = xmax - tx;
	    		}
	    		if (wmt_ts_get_ydir() == -1)
	    		{
	    			ty = ymax - ty;
	    		}
	    		//tx = ResolutionY - py;
	    		//ty = px;
	    		dbg("rpt%d(%d,%d)\n", i, tx, ty);
	    		//add for cross finger 2013-1-10
	    	#ifdef MT_TYPE_B
				input_mt_slot(ts->input, i);
				input_mt_report_slot_state(ts->input, MT_TOOL_FINGER,true);
        #endif
        input_report_abs(ts->input, ABS_MT_TRACKING_ID, i);
        //input_report_key(ts->input, BTN_TOUCH, 1);
	    	//input_report_abs(ts->input, ABS_MT_TOUCH_MAJOR, pz);
          //*******************************
	    		
	    		input_report_abs(ts->input, ABS_MT_POSITION_X, tx /*px*/);
	    		input_report_abs(ts->input, ABS_MT_POSITION_Y, ty /*py*/);
	    	
	    	#ifndef MT_TYPE_B	
	    		input_mt_sync(ts->input);
	    	#endif	
	    		realnum++;
	    		if (wmt_ts_ispenup())
	    		{
	    			wmt_ts_set_penup(0);
	    		}

			}else
			{
				//input_report_abs(ts->input, ABS_MT_TRACKING_ID, i);
				//input_report_abs(ts->input, ABS_MT_TOUCH_MAJOR, 0);
				//input_mt_sync(ts->input);
				#ifdef MT_TYPE_B
				input_mt_slot(ts->input, i);
				input_mt_report_slot_state(ts->input, MT_TOOL_FINGER,false);
				input_report_abs(ts->input, ABS_MT_TRACKING_ID, -1);
        #endif		//add cross finger 2013-1-10
				dbg("p%d not pen down\n",i);				
			}
		}
		
		#ifdef MT_TYPE_B
		input_mt_report_pointer_emulation(ts->input, true);
   #endif  //add finger cross 2013-1-10
		//printk("<<<realnum %d\n", realnum);
		if (realnum != 0)
		{
			input_sync(ts->input);
			dbg("report one point group\n");
		} else if (!wmt_ts_ispenup())
		{//********here no finger press 2013-1-10
			//add 2013-1-10 cross finger issue!
			#ifdef MT_TYPE_B
				for(i=0;i<FingerNum;i++){
					input_mt_slot(ts->input, i);
					input_mt_report_slot_state(ts->input, MT_TOOL_FINGER,false);
					input_report_abs(ts->input, ABS_MT_TRACKING_ID, -1);
				}
				input_mt_report_pointer_emulation(ts->input, true);
     #else
				//input_report_abs(ts->input, ABS_MT_TOUCH_MAJOR, 0);
				//input_mt_sync(ts->input);
				//input_report_abs(ts->input, ABS_MT_TRACKING_ID, -1);
				//input_report_key(ts->input, BTN_TOUCH, 0);
     #endif
			//**********************************
			input_mt_sync(ts->input);
			input_sync(ts->input);
			dbg("real pen up!\n");
			wmt_ts_set_penup(1);
		}

		if(KeyNum > 0)
		{
			//for(i=0;i<MAX_KEY_NUMBER;i++)
			if (0 == ky)
			{
				for (j=0;j<4;j++)
				{
					if (l_tskey[j][1] != 0)
					{
						l_tskey[j][1] = 0;
					}
				}
				dbg("finish one key report!\n");
			} else {
				for(i=0;i<4;i++)
				{			
					pressure = ky & ( 0x01 << i );
					if (pressure)
					{
						dbg("key%d\n", i);
						if (0 == l_tskey[i][1])
						{
							l_tskey[i][1] = 1; // key down
							input_report_key(ts->input, l_tskey[i][0], 1);
							input_report_key(ts->input, l_tskey[i][0], 0);
							input_sync(ts->input);
							dbg("report key_%d\n", l_tskey[i][0]);
							break;
						}
					}

				}
			}
		}
			
		dbg("normal end...\n");
	}else {
		dbg("do nothing!\n");
		if(KeyNum > 0)
		{
			//if (0 == ky)
			{
				for (j=0;j<4;j++)
				{
					if (l_tskey[j][1] != 0)
					{
						l_tskey[j][1] = 0;
					}
				}
				dbg("finish one key report!\n");
			} 
		}
	}
	wmt_enable_gpirq();

}

/***********************************************************************
    [function]: 
		        callback: charger mode enable;
    [parameters]:
    			void

    [return]:
			    void
************************************************************************/
void zet6221_ts_charger_mode()
{		
	//struct zet6221_tsdrv *zet6221_ts;
	u8 ts_write_charge_cmd[1] = {0xb5}; 
	int ret=0;
	ret=zet6221_i2c_write_tsdata(this_client, ts_write_charge_cmd, 1);
}
EXPORT_SYMBOL_GPL(zet6221_ts_charger_mode);

/***********************************************************************
    [function]: 
		        callback: charger mode disable;
    [parameters]:
    			void

    [return]:
			    void
************************************************************************/
void zet6221_ts_charger_mode_disable(void)
{
	//struct zet6221_tsdrv *zet6221_ts;
	u8 ts_write_cmd[1] = {0xb6}; 
	int ret=0;
	ret=zet6221_i2c_write_tsdata(this_client, ts_write_cmd, 1);
}
EXPORT_SYMBOL_GPL(zet6221_ts_charger_mode_disable);


#ifdef CONFIG_HAS_EARLYSUSPEND
static void ts_early_suspend(struct early_suspend *handler)
{
	//Sleep Mode
/*	u8 ts_sleep_cmd[1] = {0xb1}; 
	int ret=0;
	ret=zet6221_i2c_write_tsdata(this_client, ts_sleep_cmd, 1);
        return;
       */
	wmt_disable_gpirq();
	l_suspend = 1;
	//del_timer(&l_ts->polling_timer);

}

static void ts_late_resume(struct early_suspend *handler)
{
	resetCount = 1;
	//if (l_suspend != 0)
	{
		//wmt_disable_gpirq();
		//ctp_reset();
		//wmt_set_gpirq(IRQ_TYPE_EDGE_FALLING);
		wmt_enable_gpirq();
		l_suspend = 0;
	}
	//l_powermode = -1;
	//mod_timer(&l_ts->polling_timer,jiffies + msecs_to_jiffies(TIME_CHECK_CHARGE));

}
#endif
static int zet_ts_suspend(struct platform_device *pdev, pm_message_t state)
{
	wmt_disable_gpirq();
	l_suspend = 1;
	return 0;
}


/***********************************************************************
   [function]: 
                       resume_download_thread
   [parameters]:
                       arg 

   [return]:
                       int;
************************************************************************/
int resume_download_thread(void *arg)
{
	wake_lock(&downloadWakeLock);
  	//printk("Thread : Enter\n");
//	if((iRomType == ROM_TYPE_SRAM) || 
//	   (iRomType == ROM_TYPE_OTP)) //SRAM,OTP
 // 	{
		zet622x_resume_downloader(this_client);
		check_charger();
		l_suspend = 0;
		//printk("zet622x download OK\n");
  //	}
  	//printk("Thread : Leave\n");
	wake_unlock(&downloadWakeLock);
	return 0;
}

static int zet_ts_resume(struct platform_device *pdev)
{
	wmt_disable_gpirq();
	ctp_reset();

	if(ic_model == ZET6251) {
		//upload bin to flash_buffer, just for debug
		char fw_name[64];
		sprintf(fw_name, "%szet62xx.bin", tran_type_mode_file_name);	
		zet_fw_load(fw_name);
		resume_download_task = kthread_create(resume_download_thread, NULL , "resume_download");
		if(IS_ERR(resume_download_task)) {
			errlog("cread thread failed\n");	
		}
  		wake_up_process(resume_download_task); 
	} else {
		check_charger();
		l_suspend = 0;
	}

	wmt_set_gpirq(IRQ_TYPE_EDGE_FALLING);
	if (!earlysus_en)
		wmt_enable_gpirq();

  ///--------------------------------------///
  /// Set transfer type to dynamic mode
  ///--------------------------------------///
	transfer_type = TRAN_TYPE_DYNAMIC;

	return 0;
}


///**********************************************************************
///   [function]:  zet622x_ts_set_transfer_type
///   [parameters]: void
///   [return]: void
///**********************************************************************
int zet622x_ts_set_transfer_type(u8 bTransType)
{
	u8 ts_cmd[10] = {0xC1, 0x02, TRAN_TYPE_DYNAMIC, 0x55, 0xAA, 0x00, 0x00, 0x00, 0x00, 0x00}; 
	int ret = 0;
	ts_cmd[2] = bTransType;
	ret = zet6221_i2c_write_tsdata(this_client, ts_cmd, 10);
        return ret;
}


///**********************************************************************
///   [function]:  zet622x_ts_set_transfer_type
///   [parameters]: void
///   [return]: void
///**********************************************************************
#ifdef FEATURE_INFO_OUT_EANBLE
int zet622x_ts_set_info_type(void)
{
	int ret = 1;
	char info_file_name_out[128];

	 /// ic type
	 switch(ic_model)
        {
        case ZET6221:
                tran_data[0] = ZET6221_INFO;
        	break;
        case ZET6223:
                tran_data[0] = ZET6223_INFO;
                break;
        case ZET6231:
                tran_data[0] = ZET6231_INFO;
                break;
        case ZET6251:
                tran_data[0] = ZET6251_INFO;
                break;
        default:
        	   tran_data[0] = UNKNOW_INFO;
                break;
        }
	 
	 /// resolution
	if(xyExchange== 1)
	{
		tran_data[16] = 0x8;
		tran_data[9] = ((ResolutionY >> 8)&0xFF);
		tran_data[8] = (ResolutionY &0xFF);
		tran_data[11] = ((ResolutionX >> 8)&0xFF);
		tran_data[10] = (ResolutionX &0xFF);
	}
	else
	{
		tran_data[16] = 0x00;
		tran_data[9] = ((ResolutionX >> 8)&0xFF);
		tran_data[8] = (ResolutionX &0xFF);
		tran_data[11] = ((ResolutionY >> 8)&0xFF);
		tran_data[10] = (ResolutionY &0xFF);
	}
	
	/// trace X
       tran_data[13] = TP_DEFAULT_COL;  ///< trace x
       /// trace Y
       tran_data[14] = TP_DEFAULT_ROW;  ///< trace y
        
	if(KeyNum > 0)
	{
		tran_data[15] = (0x80 | FingerNum);
	}
	else
	{
		tran_data[15] = FingerNum;
	}
	
        sprintf(info_file_name_out, "%sinfo.bin",tran_type_mode_file_name);
        zet_information_save(info_file_name_out);
        
	printk("[ZET] : ic:%d, traceX:%d, traceY:%d\n", tran_data[0],tran_data[13],tran_data[14]);
  	return ret;
}
#endif ///< FEATURE_INFO_OUT_EANBLE

///***********************************************************************
///   [function]:  zet_mdev_save
///   [parameters]: char *
///   [return]: void
///************************************************************************
static void zet_mdev_save(char *file_name)
{
        struct file *fp;
        int data_total_len  = (row+2) * (col + 2);

        ///-------------------------------------------------------///        
        /// create the file that stores the mutual dev data
        ///-------------------------------------------------------///        
        fp = filp_open(file_name, O_RDWR | O_CREAT, 0644);
        if(IS_ERR(fp))
        {
                printk("[ZET] : Failed to open %s\n", file_name);
                return;
        }
        old_fs = get_fs();
        set_fs(KERNEL_DS);

        vfs_write(fp, tran_data, data_total_len, &(fp->f_pos));
		memcpy(mdev_data, tran_data, data_total_len);
        set_fs(old_fs);
        filp_close(fp, 0);

        return;
}

///***********************************************************************
///   [function]:  zet_idev_save
///   [parameters]: char *
///   [return]: void
///************************************************************************
#ifdef FEATURE_IDEV_OUT_ENABLE
static void zet_idev_save(char *file_name)
{
        struct file *fp;
        int data_total_len  = (row + col);

        ///-------------------------------------------------------///        
        /// create the file that stores the initial dev data
        ///-------------------------------------------------------///        
        fp = filp_open(file_name, O_RDWR | O_CREAT, 0644);
        if(IS_ERR(fp))
        {
                printk("[ZET] : Failed to open %s\n", file_name);
                return;
        }
        old_fs = get_fs();
        set_fs(KERNEL_DS);

        vfs_write(fp, tran_data, data_total_len, &(fp->f_pos));
	memcpy(idev_data, tran_data, data_total_len);
        set_fs(old_fs);
        filp_close(fp, 0);

        return;
}
#endif ///< FEATURE_IDEV_OUT_ENABLE

///***********************************************************************
///   [function]:  zet_ibase_save
///   [parameters]: char *
///   [return]: void
///************************************************************************
#ifdef FEATURE_IBASE_OUT_ENABLE
static void zet_ibase_save(char *file_name)
{
        struct file *fp;
        int data_total_len  = (row + col) * 2;

        ///-------------------------------------------------------///        
        /// create the file that stores the initial base data
        ///-------------------------------------------------------///        
        fp = filp_open(file_name, O_RDWR | O_CREAT, 0644);
        if(IS_ERR(fp))
        {
                printk("[ZET] : Failed to open %s\n", file_name);
                return;
        }
        old_fs = get_fs();
        set_fs(KERNEL_DS);

        vfs_write(fp, tran_data, data_total_len, &(fp->f_pos));
	memcpy(ibase_data, tran_data, data_total_len);
        set_fs(old_fs);
        filp_close(fp, 0);

        return;
}
#endif ///< FEATURE_IBASE_OUT_ENABLE

///***********************************************************************
///   [function]:  zet_mbase_save
///   [parameters]: char *
///   [return]: void
///************************************************************************
#ifdef FEATURE_MBASE_OUT_ENABLE
static void zet_mbase_save(char *file_name)
{
        struct file *fp;
        int data_total_len  = (row * col * 2);

        ///-------------------------------------------------------///        
        /// create the file that stores the mutual base data
        ///-------------------------------------------------------///        
        fp = filp_open(file_name, O_RDWR | O_CREAT, 0644);
        if(IS_ERR(fp))
        {
                printk("[ZET] : Failed to open %s\n", file_name);
                return;
        }
        old_fs = get_fs();
        set_fs(KERNEL_DS);

        vfs_write(fp, tran_data, data_total_len, &(fp->f_pos));
	memcpy(mbase_data, tran_data, data_total_len);
        set_fs(old_fs);
        filp_close(fp, 0);

        return;
}
#endif ///< FEATURE_MBASE_OUT_ENABLE

///***********************************************************************
///   [function]:  zet_information_save
///   [parameters]: char *
///   [return]: void
///************************************************************************
#ifdef FEATURE_INFO_OUT_EANBLE
static void zet_information_save(char *file_name)
{
        struct file *fp;
        int data_total_len  = INFO_DATA_SIZE;

        ///-------------------------------------------------------///        
        /// create the file that stores the mutual base data
        ///-------------------------------------------------------///        
        fp = filp_open(file_name, O_RDWR | O_CREAT, 0644);
        if(IS_ERR(fp))
        {
                printk("[ZET] : Failed to open %s\n", file_name);
                return;
        }
        old_fs = get_fs();
        set_fs(KERNEL_DS);

        vfs_write(fp, tran_data, data_total_len, &(fp->f_pos));
	 memcpy(info_data, tran_data, data_total_len);
        set_fs(old_fs);
        filp_close(fp, 0);

        return;
}
#endif ///< FEATURE_INFO_OUT_EANBLE

///************************************************************************
///   [function]:  zet_dv_set_file_name
///   [parameters]: void
///   [return]: void
///************************************************************************
static void zet_dv_set_file_name(char *file_name)
{
	strcpy(driver_version, file_name);
}

///************************************************************************
///   [function]:  zet_fw_set_file_name
///   [parameters]: void
///   [return]: void
///************************************************************************
static void zet_fw_set_file_name(void)//char *file_name)
{
	char fwname[256] = {0};
	wmt_ts_get_firmwname(fwname);
	sprintf(fw_file_name,"/system/etc/firmware/%s",fwname);
	//strcpy(fw_file_name, file_name);
}

///************************************************************************
///   [function]:  zet_mdev_set_file_name
///   [parameters]: void
///   [return]: void
///************************************************************************
static void zet_tran_type_set_file_name(char *file_name)
{
	strcpy(tran_type_mode_file_name, file_name);
}


///***********************************************************************
///   [function]:  zet_fw_size
///   [parameters]: void
///   [return]: void
///************************************************************************
static int zet_fw_size(void)
{
	int flash_total_len 	= 0x8000;
	
	switch(ic_model)
	{
		case ZET6221:
			flash_total_len = 0x4000;
			break;
		case ZET6223: 
			flash_total_len = 0x10000;
			break;
		case ZET6231: 			
		case ZET6251: 
		default: 
			flash_total_len = 0x8000;
			break;
	}
	
	return flash_total_len;
}


///***********************************************************************
///   [function]:  zet_fw_save
///   [parameters]: file name
///   [return]: void
///************************************************************************
static void zet_fw_save(char *file_name)
{
	struct file *fp;
	int flash_total_len 	= 0;
	
	fp = filp_open(file_name, O_RDWR | O_CREAT, 0644);
	if(IS_ERR(fp))
	{
		printk("[ZET] : Failed to open %s\n", file_name);
		return;
	}
	old_fs = get_fs();
	set_fs(KERNEL_DS);

	flash_total_len = zet_fw_size();
	printk("[ZET] : flash_total_len = 0x%04x\n",flash_total_len );

	vfs_write(fp, flash_buffer, flash_total_len, &(fp->f_pos));
	
	set_fs(old_fs);

	filp_close(fp, 0);	

	
	return;
}

///***********************************************************************
///   [function]:  zet_fw_load
///   [parameters]: file name
///   [return]: void
///************************************************************************
static void zet_fw_load(char *file_name)
{	
	int file_length = 0;
	struct file *fp;
	loff_t *pos;
	
	//printk("[ZET]: find %s\n", file_name);
	fp = filp_open(file_name, O_RDONLY, 0644);
	if(IS_ERR(fp))
	{			
		//printk("[ZET]: No firmware file detected\n");
		return;
	}

	///----------------------------///
	/// Load from file
	///----------------------------///		
	printk("[ZET]: Load from %s\n", file_name);	

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	/// Get file size
	inode = fp->f_dentry->d_inode;
	file_length = (int)inode->i_size;
	//l_fwlen = file_length; 

	pos = &(fp->f_pos); 

	vfs_read(fp, &flash_buffer[0], file_length, pos);

	//file_length
	set_fs(old_fs);
	filp_close(fp, 0);


}

///************************************************************************
///   [function]:  zet_fw_init
///   [parameters]: void
///   [return]: void
///************************************************************************
static void zet_fw_init(void)
{
	//int i;
	
	if(flash_buffer == NULL)
	{
  		flash_buffer = kmalloc(MAX_FLASH_BUF_SIZE, GFP_KERNEL);	
	}

        ///---------------------------------------------///
        /// Init the mutual dev buffer
        ///---------------------------------------------///
	if(mdev_data== NULL)
	{
		mdev_data   = kmalloc(MDEV_MAX_DATA_SIZE, GFP_KERNEL);
	}
	if(idev_data== NULL)
	{
		idev_data   = kmalloc(IDEV_MAX_DATA_SIZE, GFP_KERNEL);
	}

	if(mbase_data== NULL)
	{
		mbase_data  = kmalloc(MBASE_MAX_DATA_SIZE, GFP_KERNEL);
	}
	if(ibase_data== NULL)
	{
		ibase_data  = kmalloc(IBASE_MAX_DATA_SIZE, GFP_KERNEL);
	}	
	
        if(tran_data == NULL)
        {
	        tran_data  = kmalloc(MBASE_MAX_DATA_SIZE, GFP_KERNEL);
        }
	
        if(info_data == NULL)
        {
	        info_data  = kmalloc(INFO_MAX_DATA_SIZE, GFP_KERNEL);
        }
        
	/*printk("[ZET]: Load from header\n");

	if(ic_model == ZET6221)
	{
		for(i = 0 ; i < sizeof(zeitec_zet6221_firmware) ; i++)
		{
			flash_buffer[i] = zeitec_zet6221_firmware[i];
		}
	}
	else if(ic_model == ZET6223)
	{
		for(i = 0 ; i < sizeof(zeitec_zet6223_firmware) ; i++)
		{
			flash_buffer[i] = zeitec_zet6223_firmware[i];
		}
	}
	else if(ic_model == ZET6231)
	{
		for(i = 0 ; i < sizeof(zeitec_zet6231_firmware) ; i++)
		{
			flash_buffer[i] = zeitec_zet6231_firmware[i];
		}
	}
	else if(ic_model == ZET6251)
	{
		for(i = 0 ; i < sizeof(zeitec_zet6251_firmware) ; i++)
		{
			flash_buffer[i] = zeitec_zet6251_firmware[i];
		}
	}
	
	/// Load firmware from bin file
	zet_fw_load(fw_file_name);*/
}

///************************************************************************
///   [function]:  zet_fw_exit
///   [parameters]: void
///   [return]: void
///************************************************************************
static void zet_fw_exit(void)
{
        ///---------------------------------------------///
	/// free mdev_data
        ///---------------------------------------------///
	if(mdev_data!=NULL)
	{
		kfree(mdev_data);
		mdev_data = NULL;
	}

	if(idev_data!=NULL)
	{			
		kfree(idev_data);
		idev_data = NULL;
	}

	if(mbase_data!=NULL)
	{	
		kfree(mbase_data);
		mbase_data = NULL;
	}

	if(ibase_data!=NULL)
	{	
		kfree(ibase_data);
		ibase_data = NULL;
	}
		
	if(tran_data != NULL)	
	{
		kfree(tran_data);
		tran_data = NULL;
	}

	if(info_data != NULL)	
	{
		kfree(info_data);
		info_data = NULL;
	}

	
        ///---------------------------------------------///
	/// free flash buffer
        ///---------------------------------------------///
	if(flash_buffer!=NULL)
	{
	kfree(flash_buffer);
	flash_buffer = NULL;
}

}

///************************************************************************
///   [function]:  zet_fops_open
///   [parameters]: file
///   [return]: int
///************************************************************************
static int zet_fops_open(struct inode *inode, struct file *file)
{
	int subminor;
	int ret = 0;	
	struct i2c_client *client;
	struct i2c_adapter *adapter;	
	struct i2c_dev *i2c_dev;	
	
	subminor = iminor(inode);
	printk("[ZET] : ZET_FOPS_OPEN ,  subminor=%d\n",subminor);
	
	i2c_dev = zet622x_i2c_dev_get_by_minor(subminor);	
	if (!i2c_dev)
	{	
		printk("error i2c_dev\n");		
		return -ENODEV;	
	}
	
	adapter = i2c_get_adapter(i2c_dev->adap->nr);	
	if(!adapter)
	{		
		return -ENODEV;	
	}	
	
	client = kzalloc(sizeof(*client), GFP_KERNEL);	
	
	if(!client)
	{		
		i2c_put_adapter(adapter);		
		ret = -ENOMEM;	
	}	
	snprintf(client->name, I2C_NAME_SIZE, "pctp_i2c_ts%d", adapter->nr);
	//client->driver = &zet622x_i2c_driver;
	client->driver = this_client->driver;
	client->adapter = adapter;
	file->private_data = client;
		
	return 0;
}


///************************************************************************
///   [function]:  zet_fops_release
///   [parameters]: inode, file
///   [return]: int
///************************************************************************
static int zet_fops_release (struct inode *inode, struct file *file) 
{
	struct i2c_client *client = file->private_data;

	printk("[ZET] : zet_fops_release -> line : %d\n",__LINE__ );
	
	i2c_put_adapter(client->adapter);
	kfree(client);
	file->private_data = NULL;
	return 0;	  
}

///************************************************************************
///   [function]:  zet_fops_read
///   [parameters]: file, buf, count, ppos
///   [return]: size_t
///************************************************************************
static ssize_t zet_fops_read(struct file *file, char __user *buf, size_t count,
			loff_t *ppos)
{
	int i;
	int iCnt = 0;
	char str[256];
	int len = 0;

	printk("[ZET] : zet_fops_read -> line : %d\n",__LINE__ );
	
	///-------------------------------///
	/// Print message
	///-------------------------------///	
	sprintf(str, "Please check \"%s\"\n", fw_file_name);
	len = strlen(str);

	///-------------------------------///
	/// if read out
	///-------------------------------///		
	if(data_offset >= len)
	{
		return 0;
        }		
	
	for(i = 0 ; i < count-1 ; i++)
	{
		buf[i] = str[data_offset];
		buf[i+1] = 0;
		iCnt++;
		data_offset++;
		if(data_offset >= len)
		{
			break;
		}
	}	
	
	///-------------------------------///
	/// Save file
	///-------------------------------///	
	if(data_offset == len)
	{
		zet_fw_save(fw_file_name);
	}	
	return iCnt;
}

///************************************************************************
///   [function]:  zet_fops_write
///   [parameters]: file, buf, count, ppos
///   [return]: size_t
///************************************************************************
static ssize_t zet_fops_write(struct file *file, const char __user *buf,
                                                size_t count, loff_t *ppos)
{	
	printk("[ZET]: zet_fops_write ->  %s\n", buf);
	data_offset = 0;
	return count;
}

///************************************************************************
///   [function]:  ioctl
///   [parameters]: file , cmd , arg
///   [return]: long
///************************************************************************
static long zet_fops_ioctl(struct file *file, unsigned int cmd, unsigned long arg )
{
        u8 __user * user_buf = (u8 __user *) arg;

	u8 buf[IOCTL_MAX_BUF_SIZE];
	int data_size;
	
	if(copy_from_user(buf, user_buf, IOCTL_MAX_BUF_SIZE))
	{
		printk("[ZET]: zet_ioctl: copy_from_user fail\n");
		return 0;
	}

	printk("[ZET]: zet_ioctl ->  cmd = %d, %02x, %02x\n",  cmd, buf[0], buf[1]);

	if(cmd == ZET_IOCTL_CMD_FLASH_READ)
	{
		printk("[ZET]: zet_ioctl -> ZET_IOCTL_CMD_FLASH_DUMP  cmd = %d, file=%s\n",  cmd, (char *)buf);
		ioctl_action |= IOCTL_ACTION_FLASH_DUMP;
	}
	else if(cmd == ZET_IOCTL_CMD_FLASH_WRITE)
	{
		printk("[ZET]: zet_ioctl -> ZET_IOCTL_CMD_FLASH_WRITE  cmd = %d\n",  cmd);		
		{ //upload bin to flash_buffer
			char fw_name[64];
			sprintf(fw_name, "%szet62xx.bin", tran_type_mode_file_name);
			zet_fw_load(fw_name);
		}
	    zet622x_resume_downloader(this_client);		
	}
	else if(cmd == ZET_IOCTL_CMD_RST)
	{
		printk("[ZET]: zet_ioctl -> ZET_IOCTL_CMD_RST  cmd = %d\n",  cmd);
		//ctp_reset();
		wmt_rst_output(1);

		wmt_rst_output(0);
		msleep(20);
		wmt_rst_output(1);

		transfer_type = TRAN_TYPE_DYNAMIC;			
	}
	else if(cmd == ZET_IOCTL_CMD_RST_HIGH)
	{
		wmt_rst_output(1);
	}
	else if(cmd == ZET_IOCTL_CMD_RST_LOW)
	{
		wmt_rst_output(0);
	}
	else if(cmd == ZET_IOCTL_CMD_MDEV)
	{
		///---------------------------------------------------///
		/// set mutual dev mode
		///---------------------------------------------------///
		zet622x_ts_set_transfer_type(TRAN_TYPE_MUTUAL_SCAN_DEV);
		transfer_type = TRAN_TYPE_MUTUAL_SCAN_DEV;			
		
	}
	else if(cmd == ZET_IOCTL_CMD_IBASE)
	{
		///---------------------------------------------------///
		/// set initial base mode
		///---------------------------------------------------///
		zet622x_ts_set_transfer_type(TRAN_TYPE_INIT_SCAN_BASE);
		transfer_type = TRAN_TYPE_INIT_SCAN_BASE;
		
	}	
#ifdef FEATURE_IDEV_OUT_ENABLE 
	else if(cmd == ZET_IOCTL_CMD_IDEV)
	{
		///---------------------------------------------------///
		/// set initial dev mode
		///---------------------------------------------------///
		zet622x_ts_set_transfer_type(TRAN_TYPE_INIT_SCAN_DEV);
		transfer_type = TRAN_TYPE_INIT_SCAN_DEV;
		
	}
#endif ///< 	FEATURE_IDEV_OUT_ENABLE
#ifdef FEATURE_MBASE_OUT_ENABLE
	else if(cmd == ZET_IOCTL_CMD_MBASE)
	{
		///---------------------------------------------------///
		/// set Mutual Base mode
		///---------------------------------------------------///
		zet622x_ts_set_transfer_type(TRAN_TYPE_MUTUAL_SCAN_BASE);
		transfer_type = TRAN_TYPE_MUTUAL_SCAN_BASE;
		
	}
#endif ///< FEATURE_MBASE_OUT_ENABLE
 	else if(cmd == ZET_IOCTL_CMD_DYNAMIC)
        {
		zet622x_ts_set_transfer_type(TRAN_TYPE_DYNAMIC);
		transfer_type = TRAN_TYPE_DYNAMIC;
        }
	else if(cmd == ZET_IOCTL_CMD_FW_FILE_PATH_GET)
	{
		memset(buf, 0x00, 64);
		strcpy(buf, fw_file_name);		
		printk("[ZET]: zet_ioctl: Get FW_FILE_NAME = %s\n", buf);
	}
	else if(cmd == ZET_IOCTL_CMD_FW_FILE_PATH_SET)
	{
		strcpy(fw_file_name, buf);		
		printk("[ZET]: zet_ioctl: set FW_FILE_NAME = %s\n", buf);

	}
	else if(cmd == ZET_IOCTL_CMD_MDEV_GET)
        {
		data_size = (row+2)*(col+2);
		memcpy(buf, mdev_data, data_size);
                printk("[ZET]: zet_ioctl: Get MDEV data size=%d\n", data_size);
        }
	else if(cmd == ZET_IOCTL_CMD_TRAN_TYPE_PATH_SET)
	{
		strcpy(tran_type_mode_file_name, buf);		
		printk("[ZET]: zet_ioctl: Set ZET_IOCTL_CMD_TRAN_TYPE_PATH_ = %s\n", buf);
	}
	else if(cmd == ZET_IOCTL_CMD_TRAN_TYPE_PATH_GET)
	{
		memset(buf, 0x00, 64);
		strcpy(buf, tran_type_mode_file_name);	
		printk("[ZET]: zet_ioctl: Get ZET_IOCTL_CMD_TRAN_TYPE_PATH = %s\n", buf);
	}
	else if(cmd == ZET_IOCTL_CMD_IDEV_GET)
  	{
		data_size = (row + col);
		memcpy(buf, idev_data, data_size);
    		printk("[ZET]: zet_ioctl: Get IDEV data size=%d\n", data_size);
  	}
	else if(cmd == ZET_IOCTL_CMD_IBASE_GET)
  	{
		data_size = (row + col)*2;
		memcpy(buf, ibase_data, data_size);
    		printk("[ZET]: zet_ioctl: Get IBASE data size=%d\n", data_size);
  	}	
	else if(cmd == ZET_IOCTL_CMD_MBASE_GET)
	{
		data_size = (row*col*2);
		if(data_size > IOCTL_MAX_BUF_SIZE)
		{
			data_size = IOCTL_MAX_BUF_SIZE;
		}
		memcpy(buf, mbase_data, data_size);
		printk("[ZET]: zet_ioctl: Get MBASE data size=%d\n", data_size);
	}
	else if(cmd == ZET_IOCTL_CMD_INFO_SET)
  	{
		printk("[ZET]: zet_ioctl: ZET_IOCTL_CMD_INFO_SET\n");
		zet622x_ts_set_info_type();
	}
	else if(cmd == ZET_IOCTL_CMD_INFO_GET)
	{
		data_size = INFO_DATA_SIZE;
		memcpy(buf, info_data, data_size);
    		printk("[ZET]: zet_ioctl: Get INFO data size=%d,IC: %x,X:%d,Y:%d\n", data_size, info_data[0], info_data[13], info_data[14]);
  	}
	else if(cmd == ZET_IOCTL_CMD_TRACE_X_SET)
	{
		printk("[ZET]: zet_ioctl: ZET_IOCTL_CMD_TRACE_X_SET\n");
	}
	else if(cmd == ZET_IOCTL_CMD_TRACE_X_GET)
	{
		printk("[ZET]: zet_ioctl: Get TRACEX data\n");
	}
	else if(cmd == ZET_IOCTL_CMD_TRACE_Y_SET)
	{
		printk("[ZET]: zet_ioctl: ZET_IOCTL_CMD_TRACE_Y_SET\n");
	}
	else if(cmd == ZET_IOCTL_CMD_TRACE_Y_GET)
	{
		printk("[ZET]: zet_ioctl: Get TRACEY data \n");
	}
	else if(cmd == ZET_IOCTL_CMD_DRIVER_VER_GET)
	{
		memset(buf, 0x00, 64);
		strcpy(buf, driver_version);		
		printk("[ZET]: zet_ioctl: Get DRIVER_VERSION = %s\n", buf);
		printk("[ZET]: zet_ioctl: Get SVN = %s\n", DRIVER_VERSION);
	}
	else if(cmd == ZET_IOCTL_CMD_MBASE_EXTERN_GET)
	{
		data_size = (row*col*2) - IOCTL_MAX_BUF_SIZE;
		if(data_size < 1)
		{
			data_size = 1;
		}
		memcpy(buf, (mbase_data+IOCTL_MAX_BUF_SIZE), data_size);
		printk("[ZET]: zet_ioctl: Get MBASE extern data size=%d\n", data_size);
	}
	
	if(copy_to_user(user_buf, buf, IOCTL_MAX_BUF_SIZE))
	{
		printk("[ZET]: zet_ioctl: copy_to_user fail\n");
		return 0;
	}

	return 0;
}

///************************************************************************
///	file_operations
///************************************************************************
static const struct file_operations zet622x_ts_fops =
{	
	.owner		= THIS_MODULE, 	
	.open 		= zet_fops_open, 	
	.read 		= zet_fops_read, 	
	.write		= zet_fops_write, 
	.unlocked_ioctl = zet_fops_ioctl,
	.compat_ioctl	= zet_fops_ioctl,
	.release	= zet_fops_release, 
};

static int zet6221_ts_probe(struct i2c_client *client/*, const struct i2c_device_id *id*/)
{
	int result = -1;
	int count = 0;
	int download_count = 0;
	int download_ok = 0;
	struct input_dev *input_dev;
	struct device *dev;

	
	struct zet6221_tsdrv *zet6221_ts;

	dbg( "[TS] zet6221_ts_probe \n");
	
	zet6221_ts = kzalloc(sizeof(struct zet6221_tsdrv), GFP_KERNEL);
	l_ts = zet6221_ts;
	zet6221_ts->i2c_ts = client;
	//zet6221_ts->gpio = TS_INT_GPIO; /*s3c6410*/
	//zet6221_ts->gpio = TS1_INT_GPIO;
	
	this_client = client;
	
	i2c_set_clientdata(client, zet6221_ts);

	//client->driver = &zet6221_ts_driver;
	ts_wq = create_singlethread_workqueue("zet6221ts_wq");
	if (!ts_wq)
	{
		errlog("Failed to create workqueue!\n");
		goto err_create_wq;
	}

	INIT_WORK(&zet6221_ts->work1, zet6221_ts_work);

	input_dev = input_allocate_device();
	if (!input_dev || !zet6221_ts) {
		result = -ENOMEM;
		goto fail_alloc_mem;
	}
	
	//i2c_set_clientdata(client, zet6221_ts);

	input_dev->name = MJ5_TS_NAME;
	input_dev->phys = "zet6221_touch/input0";
	input_dev->id.bustype = BUS_HOST;
	input_dev->id.vendor = 0x0001;
	input_dev->id.product = 0x0002;
	input_dev->id.version = 0x0100;
//bootloader
	zet622x_ts_option(client);
	msleep(100);

	download_count = 0;
	download_ok = 0;
	zet_fw_init();
	do{
		if (zet6221_load_fw())
		{
			errlog("Can't load the firmware of zet62xx!\n");
		} else {
			zet6221_downloader(client);
			//ctp_reset(); //cancel it? need to check
		}
		udelay(100);

		count=0;
		do{
			ctp_reset();
			
			if(zet6221_ts_get_report_mode_t(client)==0)  //get IC info by delay 
			{
				ResolutionX = X_MAX;
				ResolutionY = Y_MAX;
				FingerNum = FINGER_NUMBER;
				KeyNum = KEY_NUMBER;
				if(KeyNum==0)
					bufLength  = 3+4*FingerNum;
				else
					bufLength  = 3+4*FingerNum+1;
				errlog("[warning] zet6221_ts_get_report_mode_t report error!!use default value\n");
			}else
			{
					if(zet6221_ts_version()==1) 	// zet6221_ts_version() depends on zet6221_downloader() 
												// cancel download firmware, need to comment it.
					{
						dbg("get report mode ok!\n");
						download_ok = 1;
					}
			}
			count++;
		}while(count<REPORT_POLLING_TIME && download_ok != 1 );
		download_count++;
	}while( download_count < RETRY_DOWNLOAD_TIMES && download_ok != 1 );

	errlog( "ResolutionX=%d ResolutionY=%d FingerNum=%d KeyNum=%d\n",ResolutionX,ResolutionY,FingerNum,KeyNum);

	input_dev->evbit[0] = BIT_MASK(EV_SYN) | BIT_MASK(EV_KEY) | BIT_MASK(EV_ABS);
    	set_bit(INPUT_PROP_DIRECT, input_dev->propbit);
	input_set_abs_params(input_dev, ABS_MT_POSITION_X, 0, ResolutionY, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_POSITION_Y, 0, ResolutionX, 0, 0);

	set_bit(KEY_BACK, input_dev->keybit);
	set_bit(KEY_HOME, input_dev->keybit);
	set_bit(KEY_MENU, input_dev->keybit);
	
	//*******************************add 2013-1-10
	set_bit(ABS_MT_TRACKING_ID, input_dev->absbit);
	//set_bit(ABS_MT_TOUCH_MAJOR, input_dev->absbit); 
	input_set_abs_params(input_dev,ABS_MT_TRACKING_ID, 0, FingerNum, 0, 0);
	//input_set_abs_params(input_dev, ABS_MT_TOUCH_MAJOR, 0, P_MAX, 0, 0);
	//set_bit(BTN_TOUCH, input_dev->keybit);
	
	#ifdef MT_TYPE_B
	input_mt_init_slots(input_dev, FingerNum);	
	#endif
	//set_bit(KEY_SEARCH, input_dev->keybit);

	//input_dev->evbit[0] = BIT(EV_SYN) | BIT_MASK(EV_KEY) | BIT_MASK(EV_ABS);
	//input_dev->keybit[BIT_WORD(BTN_TOUCH)] = BIT_MASK(BTN_TOUCH);

	result = input_register_device(input_dev);
	if (result)
		goto fail_ip_reg;

	zet6221_ts->input = input_dev;

	input_set_drvdata(zet6221_ts->input, zet6221_ts);
	mutex_init(&i2c_mutex);
	wake_lock_init(&downloadWakeLock, WAKE_LOCK_SUSPEND, "resume_download");
	zet6221_ts->queue = create_singlethread_workqueue("ts_check_charge_queue");
	INIT_DELAYED_WORK(&zet6221_ts->work, polling_timer_func);

	//setup_timer(&zet6221_ts->polling_timer, polling_timer_func, (unsigned long)zet6221_ts);
	//mod_timer(&zet6221_ts->polling_timer,jiffies + msecs_to_jiffies(TIME_CHECK_CHARGE));
	
	
	//s3c6410
	//result = gpio_request(zet6221_ts->gpio, "GPN"); 
	wmt_set_gpirq(IRQ_TYPE_EDGE_FALLING);
	wmt_disable_gpirq();
	/*result = gpio_request(zet6221_ts->gpio, "GPN"); 
	if (result)
		goto gpio_request_fail;
	*/

	zet6221_ts->irq = wmt_get_tsirqnum();//gpio_to_irq(zet6221_ts->gpio);
	dbg( "[TS] zet6221_ts_probe.gpid_to_irq [zet6221_ts->irq=%d]\n",zet6221_ts->irq);

	result = request_irq(zet6221_ts->irq, zet6221_ts_interrupt,IRQF_SHARED /*IRQF_TRIGGER_FALLING*/, 
				ZET_TS_ID_NAME, zet6221_ts);
	if (result)
	{
		errlog("Can't alloc ts irq=%d\n", zet6221_ts->irq);
		goto request_irq_fail;
	}
	

	///-----------------------------------------------///
	/// Set the default firmware bin file name & mutual dev file name
	///-----------------------------------------------///
	zet_dv_set_file_name(DRIVER_VERSION);
	zet_fw_set_file_name();//FW_FILE_NAME);
	zet_tran_type_set_file_name(TRAN_MODE_FILE_PATH);

	///---------------------------------///
	/// Set file operations
	///---------------------------------///	
	result = register_chrdev(I2C_MAJOR, "zet_i2c_ts", &zet622x_ts_fops);
	if(result)
	{	
		printk(KERN_ERR "%s:register chrdev failed\n",__FILE__);	
		goto fail_register_chrdev;
	}
	///---------------------------------///
	/// Create device class
	///---------------------------------///
	i2c_dev_class = class_create(THIS_MODULE,"zet_i2c_dev");
	if(IS_ERR(i2c_dev_class))
	{		
		result = PTR_ERR(i2c_dev_class);		
		goto fail_create_class;
	}
	///--------------------------------------------///
	/// Get a free i2c dev
	///--------------------------------------------///
	zet_i2c_dev = zet622x_i2c_get_free_dev(client->adapter);	
	if(IS_ERR(zet_i2c_dev))
	{	
		result = PTR_ERR(zet_i2c_dev);		
		goto fail_get_free_dev;
	}
	dev = device_create(i2c_dev_class, &client->adapter->dev, 
				MKDEV(I2C_MAJOR,client->adapter->nr), NULL, "zet62xx_ts%d", client->adapter->nr);	
	if(IS_ERR(dev))
	{		
		result = PTR_ERR(dev);		
		goto fail_create_device;
	}

#ifdef CONFIG_HAS_EARLYSUSPEND
	zet6221_ts->early_suspend.suspend = ts_early_suspend,
	zet6221_ts->early_suspend.resume = ts_late_resume,
	zet6221_ts->early_suspend.level = EARLY_SUSPEND_LEVEL_DISABLE_FB + 1;//,EARLY_SUSPEND_LEVEL_DISABLE_FB + 2;
	register_early_suspend(&zet6221_ts->early_suspend);
#endif
	//disable_irq(zet6221_ts->irq);
	ctp_reset();
	wmt_enable_gpirq();
	queue_delayed_work(zet6221_ts->queue, &zet6221_ts->work, msecs_to_jiffies(TIME_CHECK_CHARGE));
	//mod_timer(&zet6221_ts->polling_timer,jiffies + msecs_to_jiffies(TIME_CHECK_CHARGE));
	dbg("ok\n");
	return 0;

fail_create_device:
	kfree(zet_i2c_dev);
fail_get_free_dev:
	class_destroy(i2c_dev_class);	
fail_create_class:
	unregister_chrdev(I2C_MAJOR, "zet_i2c_ts");
fail_register_chrdev:
	free_irq(zet6221_ts->irq, zet6221_ts);
request_irq_fail:
	destroy_workqueue(zet6221_ts->queue);
	cancel_delayed_work_sync(&zet6221_ts->work);
	//gpio_free(zet6221_ts->gpio);
//gpio_request_fail:
	free_irq(zet6221_ts->irq, zet6221_ts);
	wake_lock_destroy(&downloadWakeLock);
	input_unregister_device(input_dev);
	input_dev = NULL;
fail_ip_reg:
fail_alloc_mem:
	input_free_device(input_dev);
	destroy_workqueue(ts_wq);
	cancel_work_sync(&zet6221_ts->work1);
	zet_fw_exit();
err_create_wq:
	kfree(zet6221_ts);
	return result;
}

static int zet6221_ts_remove(void /*struct i2c_client *dev*/)
{
	struct zet6221_tsdrv *zet6221_ts = l_ts;//i2c_get_clientdata(dev);

	//del_timer(&zet6221_ts->polling_timer);
#ifdef CONFIG_HAS_EARLYSUSPEND
	unregister_early_suspend(&zet6221_ts->early_suspend);
#endif
	wmt_disable_gpirq();
	device_destroy(i2c_dev_class, MKDEV(I2C_MAJOR,this_client->adapter->nr));
	kfree(zet_i2c_dev);
	class_destroy(i2c_dev_class);	
	unregister_chrdev(I2C_MAJOR, "zet_i2c_ts");
	free_irq(zet6221_ts->irq, zet6221_ts);
	//gpio_free(zet6221_ts->gpio);
	//del_timer_sync(&zet6221_ts->polling_timer);		
	destroy_workqueue(zet6221_ts->queue);
	cancel_delayed_work_sync(&zet6221_ts->work);
	input_unregister_device(zet6221_ts->input);	
	wake_lock_destroy(&downloadWakeLock);
	cancel_work_sync(&zet6221_ts->work1);
	destroy_workqueue(ts_wq);
	zet_fw_exit();
	kfree(zet6221_ts);

	return 0;
}

static int wmt_wakeup_bl_notify(struct notifier_block *nb, unsigned long event,
	void *dummy)
{
	//printk("get notify\n");
	switch (event) {
		case BL_CLOSE:
			l_suspend = 1;
			//printk("\nclose backlight\n\n");
			//printk("disable irq\n\n");
			wmt_disable_gpirq();
			break;
		case BL_OPEN:
			l_suspend = 0;
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

static int zet6221_ts_init(void)
{
	//u8  ts_data[70];
	//int ret;

	/*ctp_reset();
	memset(ts_data,0,70);
	ret=zet6221_i2c_read_tsdata(ts_get_i2c_client(), ts_data, 8);
	if (ret <= 0)
	{
		dbg("Can't find zet6221!\n");
		return -1;
	}
	if (!zet6221_is_ts(ts_get_i2c_client()))
	{
		dbg("isn't zet6221!\n");
		return -1;
	}*/
	if (zet6221_ts_probe(ts_get_i2c_client()))
	{
		return -1;
	}
	if (earlysus_en)
		register_bl_notifier(&wmt_bl_notify);
	//i2c_add_driver(&zet6221_ts_driver);
	return 0;
}
//module_init(zet6221_ts_init);

static void zet6221_ts_exit(void)
{
	zet6221_ts_remove();
	if (earlysus_en)
		unregister_bl_notifier(&wmt_bl_notify);
    //i2c_del_driver(&zet6221_ts_driver);
}
//module_exit(zet6221_ts_exit);

void zet6221_set_ts_mode(u8 mode)
{
	dbg( "[Touch Screen]ts mode = %d \n", mode);
}
//EXPORT_SYMBOL_GPL(zet6221_set_ts_mode);

struct wmtts_device zet6221_tsdev = {
		.driver_name = WMT_TS_I2C_NAME,
		.ts_id = "ZET62",
		.init = zet6221_ts_init,
		.exit = zet6221_ts_exit,
		.suspend = zet_ts_suspend,
		.resume = zet_ts_resume,
};


MODULE_DESCRIPTION("ZET6221 I2C Touch Screen driver");
MODULE_LICENSE("GPL v2");
