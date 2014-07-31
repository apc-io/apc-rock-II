#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/irq.h>
#include <linux/miscdevice.h>
#include <linux/gpio.h>
#include <asm/uaccess.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/workqueue.h>
#include <linux/freezer.h>
#include <linux/platform_device.h>
#include <linux/proc_fs.h>
#include <mach/hardware.h>
#include "mma8x5x.h" //"mma8452q.h"
#include "../sensor.h"

//#define DEBUG 1

#undef dbg

//#if 0
	#define dbg(fmt, args...) if (l_sensorconfig.isdbg) 	printk(KERN_ALERT "[%s]: " fmt, __FUNCTION__, ## args) 
	//#define dbg(fmt, args...) printk(KERN_ALERT "[%s]: " fmt, __FUNCTION__, ## args) 
	
#define klog(fmt, args...) 	printk(KERN_ALERT "[%s]: " fmt, __FUNCTION__, ## args) 
	
	//#define dbg(format, arg...) printk(KERN_ALERT format, ## arg)


//#else
//	#define dbg(format, arg...)
//#endif

/////////////////////Macro constant
#define SENSOR_POLL_WAIT_TIME   1837
#define MAX_FAILURE_COUNT 10
#define MMA8452_ADDR	0x1C  //slave 0x1D??high
#define LAND_PORT_MASK 0x1C
#define LAND_LEFT 0x1
#define LAND_RIGHT 0x2
#define PORT_INVERT 0x5
#define PORT_NORMAL 0x6

#define LANDSCAPE_LOCATION 0
#define PORTRAIT_LOCATION  1

#define SENSOR_UI_MODE 0
#define SENSOR_GRAVITYGAME_MODE 1

#define UI_SAMPLE_RATE 0xFC

#define GSENSOR_PROC_NAME "gsensor_config"
#define GSENSOR_MAJOR   161
#define GSENSOR_NAME    "mma8452q"
#define GSENSOR_DRIVER_NAME  "mma8452q_drv"


#define sin30_1000  500
#define	cos30_1000  866

#define DISABLE 0
#define ENABLE 1

////////////////////////the rate of g-sensor/////////////////////////////////////////////
#define SENSOR_DELAY_FASTEST      0
#define SENSOR_DELAY_GAME        20
#define SENSOR_DELAY_UI          60
#define SENSOR_DELAY_NORMAL     200

#define FASTEST_MMA_AMSR 0 // 120 samples/sec
#define GAME_MMA_AMSR    1 // 1,   (64, samples/sec)
#define UI_MMA_AMSR      3 // 2, 3,4,   (16, 8,32 samples/sec)
#define NORMAL_MMA_AMSR  5 // 5, 6, 7   (4, 2, 1 samples/sec)

#define MMA8451_ID			0x1A
#define MMA8452_ID			0x2A
#define MMA8453_ID			0x3A
#define MMA8652_ID			0x4A
#define MMA8653_ID			0x5A
#define MODE_CHANGE_DELAY_MS	100
/* register enum for mma8x5x registers */
enum {
	MMA8X5X_STATUS = 0x00,
	MMA8X5X_OUT_X_MSB,
	MMA8X5X_OUT_X_LSB,
	MMA8X5X_OUT_Y_MSB,
	MMA8X5X_OUT_Y_LSB,
	MMA8X5X_OUT_Z_MSB,
	MMA8X5X_OUT_Z_LSB,

	MMA8X5X_F_SETUP = 0x09,
	MMA8X5X_TRIG_CFG,
	MMA8X5X_SYSMOD,
	MMA8X5X_INT_SOURCE,
	MMA8X5X_WHO_AM_I,
	MMA8X5X_XYZ_DATA_CFG,
	MMA8X5X_HP_FILTER_CUTOFF,

	MMA8X5X_PL_STATUS,
	MMA8X5X_PL_CFG,
	MMA8X5X_PL_COUNT,
	MMA8X5X_PL_BF_ZCOMP,
	MMA8X5X_P_L_THS_REG,

	MMA8X5X_FF_MT_CFG,
	MMA8X5X_FF_MT_SRC,
	MMA8X5X_FF_MT_THS,
	MMA8X5X_FF_MT_COUNT,

	MMA8X5X_TRANSIENT_CFG = 0x1D,
	MMA8X5X_TRANSIENT_SRC,
	MMA8X5X_TRANSIENT_THS,
	MMA8X5X_TRANSIENT_COUNT,

	MMA8X5X_PULSE_CFG,
	MMA8X5X_PULSE_SRC,
	MMA8X5X_PULSE_THSX,
	MMA8X5X_PULSE_THSY,
	MMA8X5X_PULSE_THSZ,
	MMA8X5X_PULSE_TMLT,
	MMA8X5X_PULSE_LTCY,
	MMA8X5X_PULSE_WIND,

	MMA8X5X_ASLP_COUNT,
	MMA8X5X_CTRL_REG1,
	MMA8X5X_CTRL_REG2,
	MMA8X5X_CTRL_REG3,
	MMA8X5X_CTRL_REG4,
	MMA8X5X_CTRL_REG5,

	MMA8X5X_OFF_X,
	MMA8X5X_OFF_Y,
	MMA8X5X_OFF_Z,

	MMA8X5X_REG_END,
};

enum {
	MODE_2G = 0,
	MODE_4G,
	MODE_8G,
};

static int mma8x5x_chip_id[] ={
 	MMA8451_ID,
 	MMA8452_ID,
 	MMA8453_ID,
 	MMA8652_ID,
	MMA8653_ID,  
};

static struct i2c_client *this_client = NULL;
/////////////////////////////////////////////////////////////////////////

static struct platform_device *this_pdev;

static struct class* l_dev_class = NULL;
static struct device *l_clsdevice = NULL;



struct mma8452q_config
{
	int op;
	int int_gpio; //0-3
	int xyz_axis[3][2]; // (axis,direction)
	int rxyz_axis[3][2];
	int irq;
	struct proc_dir_entry* sensor_proc;
	int sensorlevel;
	int shake_enable; // 1--enable shake, 0--disable shake
	int manual_rotation; // 0--landance, 90--vertical
	struct input_dev *input_dev;
	//struct work_struct work;
	struct delayed_work work; // for polling
	struct workqueue_struct *queue;
	int isdbg; // 0-- no debug log, 1--show debug log
	int sensor_samp; // 1,2,4,8,16,32,64,120
	int sensor_enable;  // 0 --> disable sensor, 1 --> enable sensor
	int test_pass;
	spinlock_t spinlock;
	int pollcnt; // the counts of polling
	int offset[3];
};

static struct mma8452q_config l_sensorconfig = {
	.op = 0,
	.int_gpio = 3,
	.xyz_axis = {
		{ABS_X, -1},
		{ABS_Y, 1},
		{ABS_Z, -1},
		},
	.irq = 6,
	.int_gpio = 3,
	.sensor_proc = NULL,
	.sensorlevel = SENSOR_GRAVITYGAME_MODE,
	.shake_enable = 0, // default enable shake
	.isdbg = 0,
	.sensor_samp = 10,  // 4sample/second
	.sensor_enable = 1, // enable sensor
	.test_pass = 0, // for test program
	.pollcnt = 0, // Don't report the x,y,z when the driver is loaded until 2~3 seconds
	.offset = {0,0,0},
};



struct work_struct poll_work;
static struct mutex sense_data_mutex;
static int revision = -1;
static int l_resumed = 0; // 1: suspend --> resume;2: suspend but not resumed; other values have no meaning

//////////////////Macro function//////////////////////////////////////////////////////

#define SET_MMA_SAMPLE(buf,samp) { \
	buf[0] = 0; \
	sensor_i2c_write(/*MMA8452_ADDR,*/7,buf,1); \
	buf[0] = samp; \
	sensor_i2c_write(/*MMA8452_ADDR,*/8,buf,1); \
	buf[0] = 0x01; \
	sensor_i2c_write(/*MMA8452_ADDR,*/7,buf,1); \
}

////////////////////////Function define/////////////////////////////////////////////////////////

static unsigned int mma_sample2AMSR(unsigned int samp);

////////////////////////Function implement/////////////////////////////////////////////////
// rate: 1,2,4,8,16,32,64,120
static unsigned int sample_rate_2_memsec(unsigned int rate)
{
	return (1000/rate);
}



static ssize_t gsensor_vendor_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	ssize_t ret = 0;

	sprintf(buf, "MMA8452_%#x\n", revision);
	ret = strlen(buf) + 1;

	return ret;
}

static DEVICE_ATTR(vendor, 0444, gsensor_vendor_show, NULL);

static struct kobject *android_gsensor_kobj;
static int gsensor_sysfs_init(void)
{
	int ret ;

	android_gsensor_kobj = kobject_create_and_add("android_gsensor", NULL);
	if (android_gsensor_kobj == NULL) {
		printk(KERN_ERR
		       "mma8452q gsensor_sysfs_init:"\
		       "subsystem_register failed\n");
		ret = -ENOMEM;
		goto err;
	}

	ret = sysfs_create_file(android_gsensor_kobj, &dev_attr_vendor.attr);
	if (ret) {
		printk(KERN_ERR
		       "mma8452q gsensor_sysfs_init:"\
		       "sysfs_create_group failed\n");
		goto err4;
	}

	return 0 ;
err4:
	kobject_del(android_gsensor_kobj);
err:
	return ret ;
}

static int gsensor_sysfs_exit(void)
{
	sysfs_remove_file(android_gsensor_kobj, &dev_attr_vendor.attr);
	kobject_del(android_gsensor_kobj);
	return 0;
}

//extern int wmt_i2c_xfer_continue_if_4(struct i2c_msg *msg, unsigned int num, int bus_id);
extern int i2c_api_do_send(int bus_id, char chip_addr, char sub_addr, char *buf, unsigned int size);
extern int i2c_api_do_recv(int bus_id, char chip_addr, char sub_addr, char *buf, unsigned int size);

int sensor_i2c_write(/*unsigned int addr,*/unsigned int index,char *pdata,int len)
{
	
	char wrData[12] = {0};
	struct i2c_client *client = this_client;
	
	struct i2c_msg msgs = 
		{.addr = client->addr, .flags = 0, .len = len+1, .buf = wrData,};
	

	if (!client || (!pdata))
	{	
		printk("%s NULL client!\n", __FUNCTION__);
		return -EIO;
	}
	
	wrData[0] = index;
	strncpy(&wrData[1], pdata, len);
	
	if (i2c_transfer(client->adapter, &msgs, 1) < 0) {
		printk( "%s: transfer failed.", __func__);
		return -EIO;
	}

	return 0;
} /* End of sensor_i2c_write */

int sensor_i2c_read(/*unsigned int addr,*/unsigned int index,char *pdata,int len)
{
	char rdData[2] = {0};
	struct i2c_client *client = this_client;
	
	struct i2c_msg msgs[2] = 
	{
		{.addr = client->addr, .flags = 0|I2C_M_NOSTART, .len = 1, .buf = rdData,}, 
		{.addr = client->addr, .flags = I2C_M_RD, .len = len, .buf = pdata,},
	};
	rdData[0] = index;
	if (!client || (!pdata))
	{	
		printk("%s NULL client!\n", __FUNCTION__);
		return -EIO;
	}
	if (i2c_transfer(client->adapter, msgs, 2) < 0) {
		printk( "%s: transfer failed.", __func__);
		return -EIO;
	}
	
	return  0;//rdData[0]; i2c read ok!!

} /* End of sensor_i2c_read */

//****************add for mma8452q******************************

static int mma8452q_chip_init(void)
{
	char txData[1] = {0};
	
	int result;
	
	txData[0] = 0x1;  //active mode
	result = sensor_i2c_write(/*MMA8452_ADDR,*/MMA8X5X_CTRL_REG1,txData,1);
	if (result < 0)
		goto out;
	
	txData[0] = MODE_2G;
	result = sensor_i2c_write(/*MMA8452_ADDR,*/MMA8X5X_XYZ_DATA_CFG,txData,1);
	if (result < 0)
		goto out;
	
	txData[0] = 0x1;  //wake mode
	result = sensor_i2c_write(/*MMA8452_ADDR,*/MMA8X5X_SYSMOD,txData,1);
	if (result < 0)
		goto out;
		
	msleep(MODE_CHANGE_DELAY_MS);
	return 0;
out:
	
	return result;
	
}

static unsigned int mma_sample2AMSR(unsigned int samp)
{
	int i = 0;
	unsigned int amsr;
	
	if (samp >= 120)
	{
		return 0;
	}
	while (samp)
	{
		samp = samp >> 1;
		i++;
	}
	amsr = 8 - i;
	return amsr;
}

static int mma_enable_disable(int enable)
{
	char buf[1];
	
	// disable all interrupt of g-sensor
	memset(buf, 0, sizeof(buf));
	if ((enable < 0) || (enable > 1))
	{
		return -1;
	}
	buf[0] = 0;
	sensor_i2c_write(/*MMA8452_ADDR,*/7,buf,1);
	if (enable != 0)
	{
		buf[0] = (1 == l_sensorconfig.shake_enable) ? 0xF0:0x10;
	} else {
		buf[0] = 0;
	}
	sensor_i2c_write(/*MMA8452_ADDR,*/6,buf,1);
	buf[0] = 0xf9;
	sensor_i2c_write(/*MMA8452_ADDR,*/7,buf,1);
	return 0;	
}

// To contol the g-sensor for UI 
static int mmad_open(struct inode *inode, struct file *file)
{
	dbg("Open the g-sensor node...\n");
	return 0;
}

static int mmad_release(struct inode *inode, struct file *file)
{
	dbg("Close the g-sensor node...\n");
	return 0;
}

static long
mmad_ioctl(/*struct inode *inode,*/ struct file *file, unsigned int cmd,
	   unsigned long arg)
{
	void __user *argp = (void __user *)arg;
	char rwbuf[5];
	short delay, enable;  //amsr = -1;
	unsigned int uval = 0;

	dbg("g-sensor ioctr...\n");
	memset(rwbuf, 0, sizeof(rwbuf));
	switch (cmd) {
		case ECS_IOCTL_APP_SET_DELAY:
			// set the rate of g-sensor
			if (copy_from_user(&delay, argp, sizeof(short)))
			{
				printk(KERN_ALERT "Can't get set delay!!!\n");
				return -EFAULT;
			}
			klog("Get delay=%d\n", delay);
			//klog("before change sensor sample:%d...\n", l_sensorconfig.sensor_samp);
			if ((delay >=0) && (delay < 20))
			{
				delay = 20;
			} else if (delay > 200) 
			{
				delay = 200;
			}
			l_sensorconfig.sensor_samp = 1000/delay;			
			
			break;
		case ECS_IOCTL_APP_SET_AFLAG:
			// enable/disable sensor
			if (copy_from_user(&enable, argp, sizeof(short)))
			{
				printk(KERN_ERR "Can't get enable flag!!!\n");
				return -EFAULT;
			}
			klog("enable=%d\n",enable);
			if ((enable >=0) && (enable <=1))
			{
				dbg("driver: disable/enable(%d) gsensor.\n", enable);
				
				l_sensorconfig.sensor_enable = enable;
				
			} else {
				printk(KERN_ERR "Wrong enable argument in %s !!!\n", __FUNCTION__);
				return -EINVAL;
			}
			break;
		case WMT_IOCTL_SENSOR_GET_DRVID:
			uval = MMA8452Q_DRVID;
			if (copy_to_user((unsigned int*)arg, &uval, sizeof(unsigned int)))
			{
				return -EFAULT;
			}
			dbg("mma8452q_driver_id:%d\n",uval);
			break;
		case WMT_IOCTL_SENOR_GET_RESOLUTION:
		
		uval = (16<<8) | 4; // 16bit:4g 0xxx xx //mma8452Q
		if (copy_to_user((unsigned int *)arg, &uval, sizeof(unsigned int)))
		{
			return -EFAULT;
		}
		printk("<<<<<<<resolution:0x%x\n",uval);	
		default:
			break;
	}



	return 0;
}


static void mma_work_func(struct work_struct *work)
{
	struct mma8452q_config *data;
	/*unsigned*/ short rxData[3] = {0};
	//unsigned char tiltval = 0;
	int i = 0;
	int x,y,z;
	char tmp[6] = {0};

	data = dev_get_drvdata(&this_pdev->dev);
	if (!l_sensorconfig.sensor_enable)
	{
		queue_delayed_work(l_sensorconfig.queue, &l_sensorconfig.work, msecs_to_jiffies(sample_rate_2_memsec(l_sensorconfig.sensor_samp)));
		return;
	}
	mutex_lock(&sense_data_mutex);
	
	
	i = sensor_i2c_read(/*MMA8452_ADDR,*/MMA8X5X_OUT_X_MSB,tmp, sizeof(tmp));
#if 0	
	for (i=0; i<6; i++)
		sensor_i2c_read(/*MMA8452_ADDR,*/MMA8X5X_OUT_X_MSB+i,&tmp[i], 1);
	i = 0;	
#endif	
	
	if (0 == i)
	{
		rxData[0] = ((tmp[0] << 8 )&0xff00 ) | (tmp[1] );
		rxData[1] = ((tmp[2] << 8) &0xff00 ) | (tmp[3] );
		rxData[2] = ((tmp[4] << 8) &0xff00 ) | (tmp[5] ); 
		x = rxData[l_sensorconfig.xyz_axis[0][0]]*l_sensorconfig.xyz_axis[0][1];
		y = rxData[l_sensorconfig.xyz_axis[1][0]]*l_sensorconfig.xyz_axis[1][1];
		z = rxData[l_sensorconfig.xyz_axis[2][0]]*l_sensorconfig.xyz_axis[2][1];
	#if 0
		x = (x*9800) >> 14;
		y = (y*9800) >> 14;
		z = (z*9800) >> 14;
	#endif	
		//printk("x,y,z (%d, %d, %d) \n", rxData[0], rxData[1], rxData[2]);
		//printk("x,y,z (%d, %d, %d) \n", x, y, z);
		//printk("x 0x%x\n", tmp[0]);
		input_report_abs(data->input_dev, ABS_X, x);
		input_report_abs(data->input_dev, ABS_Y, y);
		input_report_abs(data->input_dev, ABS_Z, z);
		input_sync(data->input_dev);
	}
	
	
	mutex_unlock(&sense_data_mutex);
	queue_delayed_work(l_sensorconfig.queue, &l_sensorconfig.work, msecs_to_jiffies(sample_rate_2_memsec(l_sensorconfig.sensor_samp)));
}

static int sensor_writeproc( struct file   *file,
                           const char    *buffer,
                           unsigned long count,
                           void          *data )
{

	int inputval = -1;
	int enable, sample = -1;
	char tembuf[8];
	unsigned int amsr = 0;
	int test = 0;

	mutex_lock(&sense_data_mutex);
	// disable int
	//gsensor_int_ctrl(DISABLE);
	memset(tembuf, 0, sizeof(tembuf));
	// get sensor level and set sensor level
	if (sscanf(buffer, "level=%d\n", &l_sensorconfig.sensorlevel))
	{
	} else if (sscanf(buffer, "shakenable=%d\n", &l_sensorconfig.shake_enable))
	{
		
	} 
	else if (sscanf(buffer, "isdbg=%d\n", &l_sensorconfig.isdbg))
	{
		// only set the dbg flag
	} else if (sscanf(buffer, "init=%d\n", &inputval))
	{
		mma8452q_chip_init();
		dbg("Has reinit sensor !!!\n");
	} else if (sscanf(buffer, "samp=%d\n", &sample))
	{
		if (sample > 0)
		{
			if (sample != l_sensorconfig.sensor_samp)
			{
				amsr = mma_sample2AMSR(sample);
				SET_MMA_SAMPLE(tembuf, amsr);
				klog("sample:%d  ,amsr:%d \n", sample, amsr);
				l_sensorconfig.sensor_samp = sample;
			}
			printk(KERN_ALERT "sensor samp=%d(amsr:%d) has been set.\n", sample, amsr);
		} else {
			printk(KERN_ALERT "Wrong sample argumnet of sensor.\n");
		}
	} else if (sscanf(buffer, "enable=%d\n", &enable))
	{
		if ((enable < 0) || (enable > 1))
		{
			printk(KERN_ERR "The argument to enable/disable g-sensor should be 0 or 1  !!!\n");
		} else if (enable != l_sensorconfig.sensor_enable)
		{
			mma_enable_disable(enable);
			l_sensorconfig.sensor_enable = enable;
		}
	} else 	if (sscanf(buffer, "sensor_test=%d\n", &test))
	{ // for test begin
		l_sensorconfig.test_pass = 0;		
	} else if (sscanf(buffer, "sensor_testend=%d\n", &test))
	{	// Don nothing only to be compatible the before testing program		
	}
	mutex_unlock(&sense_data_mutex);
	return count;
}

static int sensor_readproc(char *page, char **start, off_t off,
			  int count, int *eof, void *data)
{
	int len = 0;

	len = sprintf(page, 
			"test_pass=%d\nisdbg=%d\nrate=%d\nenable=%d\n",
				l_sensorconfig.test_pass,
				l_sensorconfig.isdbg,
				l_sensorconfig.sensor_samp,
				l_sensorconfig.sensor_enable
				);
	return len;
}



static int mma8452q_init_client(struct platform_device *pdev)
{
	struct mma8452q_config *data;
//	int ret;

	data = dev_get_drvdata(&pdev->dev);
	mutex_init(&sense_data_mutex);
	/*Only for polling, not interrupt*/
	l_sensorconfig.sensor_proc = create_proc_entry(GSENSOR_PROC_NAME, 0666, NULL/*&proc_root*/);
	if (l_sensorconfig.sensor_proc != NULL)
	{
		l_sensorconfig.sensor_proc->write_proc = sensor_writeproc;
		l_sensorconfig.sensor_proc->read_proc = sensor_readproc;
	}


	return 0;

//err:
//	return ret;
}

static struct file_operations mmad_fops = {
	.owner = THIS_MODULE,
	.open = mmad_open,
	.release = mmad_release,
	.unlocked_ioctl = mmad_ioctl,
};


static struct miscdevice mmad_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "sensor_ctrl",
	.fops = &mmad_fops,
};

static int mma8452q_probe(struct platform_device *pdev)
{
	int err;

	this_pdev = pdev;
	l_sensorconfig.queue = create_singlethread_workqueue("sensor-intterupt-handle");
	INIT_DELAYED_WORK(&l_sensorconfig.work, mma_work_func);
	
	l_sensorconfig.input_dev = input_allocate_device();
	if (!l_sensorconfig.input_dev) {
		err = -ENOMEM;
		printk(KERN_ERR
		       "mma8452q_probe: Failed to allocate input device\n");
		goto exit_input_dev_alloc_failed;
	}
	l_sensorconfig.input_dev->evbit[0] = BIT(EV_ABS) | BIT_MASK(EV_KEY);
	set_bit(KEY_NEXTSONG, l_sensorconfig.input_dev->keybit);
	
	/* x-axis acceleration */
	input_set_abs_params(l_sensorconfig.input_dev, ABS_X, -65532, 65532, 0, 0);
	/* y-axis acceleration */
	input_set_abs_params(l_sensorconfig.input_dev, ABS_Y, -65532, 65532, 0, 0);
	/* z-axis acceleration */
	input_set_abs_params(l_sensorconfig.input_dev, ABS_Z, -65532, 65532, 0, 0);

	l_sensorconfig.input_dev->name = "g-sensor";

	err = input_register_device(l_sensorconfig.input_dev);

	if (err) {
		printk(KERN_ERR
		       "mma8452q_probe: Unable to register input device: %s\n",
		       l_sensorconfig.input_dev->name);
		goto exit_input_register_device_failed;
	}

	err = misc_register(&mmad_device);
	if (err) {
		printk(KERN_ERR
		       "mma8452q_probe: mmad_device register failed\n");
		goto exit_misc_device_register_failed;
	}
	
	dev_set_drvdata(&pdev->dev, &l_sensorconfig);
	mma8452q_chip_init();
	mma8452q_init_client(pdev);
	gsensor_sysfs_init();

	// satrt the polling work
	l_sensorconfig.sensor_samp = 10;
	queue_delayed_work(l_sensorconfig.queue, &l_sensorconfig.work, msecs_to_jiffies(sample_rate_2_memsec(l_sensorconfig.sensor_samp)));
	return 0;

exit_misc_device_register_failed:
exit_input_register_device_failed:
	input_free_device(l_sensorconfig.input_dev);

exit_input_dev_alloc_failed:

	return err;
}

static int mma8452q_remove(struct platform_device *pdev)
{
	if (NULL != l_sensorconfig.queue)
	{
		cancel_delayed_work_sync(&l_sensorconfig.work);
		flush_workqueue(l_sensorconfig.queue);
		destroy_workqueue(l_sensorconfig.queue);
		l_sensorconfig.queue = NULL;
	}
	gsensor_sysfs_exit();
	misc_deregister(&mmad_device);
	input_unregister_device(l_sensorconfig.input_dev);
	if (l_sensorconfig.sensor_proc != NULL)
	{
		remove_proc_entry(GSENSOR_PROC_NAME, NULL);
		l_sensorconfig.sensor_proc = NULL;
	}
	
	return 0;
}

static int mma8452q_suspend(struct platform_device *pdev, pm_message_t state)
{
	//gsensor_int_ctrl(DISABLE);
	cancel_delayed_work_sync(&l_sensorconfig.work);
	
	dbg("...ok\n");
	//l_resumed = 2;

	return 0;
}

static int mma8452q_resume(struct platform_device *pdev)
{

	l_resumed = 1;
	
	mma8452q_chip_init();
	queue_delayed_work(l_sensorconfig.queue, \
		&l_sensorconfig.work, msecs_to_jiffies(sample_rate_2_memsec(l_sensorconfig.sensor_samp)));
	dbg("...ok\n");

	return 0;
}

static void mma8452q_platform_release(struct device *device)
{
    return;
}

static void mma8452q_shutdown(struct platform_device *pdev)
{
	flush_delayed_work_sync(&l_sensorconfig.work);
	cancel_delayed_work_sync(&l_sensorconfig.work);
	
}

static struct platform_device mma8452q_device = {
	.name           = "mma8452q",
	.id             = 0,
	.dev            = {
    	.release = mma8452q_platform_release,
	},
};

static struct platform_driver mma8452q_driver = {
	.probe = mma8452q_probe,
	.remove = mma8452q_remove,
	.suspend	= mma8452q_suspend,
	.resume		= mma8452q_resume,
	.shutdown	= mma8452q_shutdown,
	.driver = {
		   .name = "mma8452q",
		   },
};

/*
 * Brief:
 *	Get the configure of sensor from u-boot.
 * Input:
 *	no use.
 * Output:
 *	no use.
 * Return:
 *	0--success, -1--error.
 * History:
 *	Created by HangYan on 2010-4-19
 * Author:
 * 	Hang Yan in ShenZhen.
 */     
extern int wmt_getsyspara(char *varname, unsigned char *varval, int *varlen);
static int get_axisset(void* param)
{
	char varbuf[64];
	int n;
	int varlen;

	memset(varbuf, 0, sizeof(varbuf));
	varlen = sizeof(varbuf);
	if (wmt_getsyspara("wmt.io.mma8452qgsensor", varbuf, &varlen)) {
		printk(KERN_DEBUG "Can't get gsensor config in u-boot!!!!\n");
		//return -1;
	} else {
		n = sscanf(varbuf, "%d:%d:%d:%d:%d:%d:%d:%d:%d:%d",
				&l_sensorconfig.op,
				&(l_sensorconfig.xyz_axis[0][0]),
				&(l_sensorconfig.xyz_axis[0][1]),
				&(l_sensorconfig.xyz_axis[1][0]),
				&(l_sensorconfig.xyz_axis[1][1]),
				&(l_sensorconfig.xyz_axis[2][0]),
				&(l_sensorconfig.xyz_axis[2][1]),
				&(l_sensorconfig.offset[0]),
				&(l_sensorconfig.offset[1]),
				&(l_sensorconfig.offset[2]));
		if (n != 10) {
			printk(KERN_ERR "gsensor format is error in u-boot!!!\n");
			return -1;
		}

		printk("get the sensor config: %d:%d:%d:%d:%d:%d:%d:%d:%d:%d\n",
			l_sensorconfig.op,
			l_sensorconfig.xyz_axis[0][0],
			l_sensorconfig.xyz_axis[0][1],
			l_sensorconfig.xyz_axis[1][0],
			l_sensorconfig.xyz_axis[1][1],
			l_sensorconfig.xyz_axis[2][0],
			l_sensorconfig.xyz_axis[2][1],
			l_sensorconfig.offset[0],
			l_sensorconfig.offset[1],
			l_sensorconfig.offset[2]
		);
	}
	return 0;
}

static int is_mma8452q(void)
{
	char rxData[2] = {0};
	int ret = 0;
	int i = 0;
	//char rdData[10] = {0};
	//char wbuf[6] = {0};
	//char rbuf[0x33] = {0};
	
	ret = sensor_i2c_read(MMA8X5X_WHO_AM_I,rxData,1);
	//printk("<<<<%s ret %d, val 0x%x\n", __FUNCTION__, ret, rxData[0]);
	for(i = 0 ; i < sizeof(mma8x5x_chip_id)/sizeof(mma8x5x_chip_id[0]);i++)
		if(rxData[0] == mma8x5x_chip_id[i])
			return 0;
#if 0	
	struct i2c_client* client = gsensor_get_i2c_client();
	if (!client){
		printk("client NULL!\n");
		return -1;
	}
	
	for (i=0; i<6; i++)
		wbuf[i] = i+2;
	//sensor_i2c_write(0x25, wbuf, 6);
	//rbuf[0] = 0x11;
	//sensor_i2c_read(0xb, rbuf, 0x1);
	mma8452q_chip_init();
	for (i=0; i<0x12; i++) {
		sensor_i2c_read(0xb+i, rbuf, 0x1);
		printk("<<<<%s reg 0x%x val 0x%x\n", __FUNCTION__, 0xb+i, rbuf[0]);
	}
#endif	
	return -1;
}

static int __init mma8452q_init(void)
{
	int ret = 0;
	if (!(this_client = sensor_i2c_register_device(0, GSENSOR_I2C_ADDR, GSENSOR_I2C_NAME)))
	{
		printk(KERN_EMERG"Can't register gsensor i2c device!\n");
		return -1;
	}
	
	if (is_mma8452q())
	{
		printk(KERN_ERR "Can't find mma8452q!!\n");
		sensor_i2c_unregister_device(this_client);
		return -1;
	}
	
	ret = get_axisset(NULL); 

	
	printk(KERN_INFO "mma8452qfc g-sensor driver init\n");

	spin_lock_init(&l_sensorconfig.spinlock);
	
	// Create device node
	
	l_dev_class = class_create(THIS_MODULE, GSENSOR_NAME);
	//for S40 module to judge whether insmod is ok
	if (IS_ERR(l_dev_class)){
		ret = PTR_ERR(l_dev_class);
		printk(KERN_ERR "Can't class_create gsensor device !!\n");
		return ret;
	}
	l_clsdevice = device_create(l_dev_class, NULL, MKDEV(GSENSOR_MAJOR, 0), NULL, GSENSOR_NAME);
	if (IS_ERR(l_clsdevice)){
		ret = PTR_ERR(l_clsdevice);
		printk(KERN_ERR "Failed to create device %s !!!",GSENSOR_NAME);
		return ret;
	}

	if((ret = platform_device_register(&mma8452q_device)))
	{
		printk(KERN_ERR "%s Can't register mma8452q platform devcie!!!\n", __FUNCTION__);
		return ret;
	}
	if ((ret = platform_driver_register(&mma8452q_driver)) != 0)
	{
		printk(KERN_ERR "%s Can't register mma8452q platform driver!!!\n", __FUNCTION__);
		return ret;
	}
        
	return 0;
}

static void __exit mma8452q_exit(void)
{	
	platform_driver_unregister(&mma8452q_driver);
	platform_device_unregister(&mma8452q_device);
	device_destroy(l_dev_class, MKDEV(GSENSOR_MAJOR, 0));
	
	class_destroy(l_dev_class);
	sensor_i2c_unregister_device(this_client);

}

module_init(mma8452q_init);
module_exit(mma8452q_exit);
MODULE_LICENSE("GPL");
