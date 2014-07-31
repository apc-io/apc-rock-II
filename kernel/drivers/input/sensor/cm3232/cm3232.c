/*
 * cm3232.c - Intersil cm3232 ALS & Proximity Driver
 *
 * By Intersil Corp
 * Michael DiGioia
 *
 * Based on isl29011.c
 *	by Mike DiGioia <mdigioia@intersil.com>
 *  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA.
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/kernel.h>
#include <linux/hwmon.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/sysfs.h>
#include <linux/pm_runtime.h>
#include <linux/input-polldev.h>
#include <linux/miscdevice.h>
#include <asm/uaccess.h>

#include <linux/device.h>
#include <linux/platform_device.h>
//#include <linux/earlysuspend.h>

#include "../sensor.h"

/* Insmod parameters */
//I2C_CLIENT_INSMOD_1(cm3232);
#define SENSOR_I2C_NAME	"cm3232"
#define SENSOR_I2C_ADDR	0x10
#define MODULE_NAME	"cm3232"

#define REG_CMD_1		0x00
#define REG_CMD_2		0x01
#define REG_DATA_LSB		0x02
#define REG_DATA_MSB		0x03
#define ISL_MOD_MASK		0xE0
#define ISL_MOD_POWERDOWN	0
#define ISL_MOD_ALS_ONCE	1
#define ISL_MOD_IR_ONCE		2
#define ISL_MOD_RESERVED	4
#define ISL_MOD_ALS_CONT	5
#define ISL_MOD_IR_CONT		6
#define IR_CURRENT_MASK		0xC0
#define IR_FREQ_MASK		0x30
#define SENSOR_RANGE_MASK	0x03
#define ISL_RES_MASK		0x0C


#undef dbg
#define dbg(fmt, args...) //printk(KERN_ALERT "[%s]: " fmt, __FUNCTION__ , ## args)

#undef errlog
#undef klog
#define errlog(fmt, args...) printk(KERN_ERR "[%s]: " fmt, __FUNCTION__, ## args)
#define klog(fmt, args...) printk(KERN_ALERT "[%s]: " fmt, __FUNCTION__, ## args)

extern int wmt_getsyspara(char *varname, unsigned char *varval, int *varlen);
static int no_adc_map = 1;
static int last_mod;

static struct i2c_client *this_client = NULL;

struct isl_device {
	struct input_polled_dev* input_poll_dev;
	struct i2c_client* client;
	int resolution;
	int range;
	int isdbg;
#ifdef CONFIG_HAS_EARLYSUSPEND
	struct early_suspend earlysuspend;
#endif

};

static struct isl_device* l_sensorconfig = NULL;
static struct kobject *android_lsensor_kobj = NULL;
static int l_enable = 0; // 0:don't report data, 1 

static DEFINE_MUTEX(mutex);

#if 0
static int isl_set_mod(struct i2c_client *client, int mod)
{
	int ret, val, freq;

	switch (mod) {
	case ISL_MOD_POWERDOWN:
	case ISL_MOD_RESERVED:
		goto setmod;
	case ISL_MOD_ALS_ONCE:
	case ISL_MOD_ALS_CONT:
		freq = 0;
		break;
	case ISL_MOD_IR_ONCE:
	case ISL_MOD_IR_CONT:
		freq = 1;
		break;
	default:
		return -EINVAL;
	}
	/* set IR frequency */
	val = i2c_smbus_read_byte_data(client, REG_CMD_2);
	if (val < 0)
		return -EINVAL;
	val &= ~IR_FREQ_MASK;
	if (freq)
		val |= IR_FREQ_MASK;
	ret = i2c_smbus_write_byte_data(client, REG_CMD_2, val);
	if (ret < 0)
		return -EINVAL;

setmod:
	/* set operation mod */
	val = i2c_smbus_read_byte_data(client, REG_CMD_1);
	if (val < 0)
		return -EINVAL;
	val &= ~ISL_MOD_MASK;
	val |= (mod << 5);
	ret = i2c_smbus_write_byte_data(client, REG_CMD_1, val);
	if (ret < 0)
		return -EINVAL;

	if (mod != ISL_MOD_POWERDOWN)
		last_mod = mod;

	return mod;
}

static int isl_get_res(struct i2c_client *client)
{
	int val;

 printk(KERN_INFO MODULE_NAME ": %s cm3232 get_res call, \n", __func__);
    val = i2c_smbus_read_word_data(client, 0)>>8 & 0xff;

	if (val < 0)
		return -EINVAL;

	val &= ISL_RES_MASK;
	val >>= 2;

	switch (val) {
	case 0:
		return 65536;
	case 1:
		return 4096;
	case 2:
		return 256;
	case 3:
		return 16;
	default:
		return -EINVAL;
	}
}

static int isl_get_range(struct i2c_client* client)
{
	switch (i2c_smbus_read_word_data(client, 0)>>8 & 0xff & 0x3) {
		case 0: return  1000;
		case 1: return  4000;
		case 2: return 16000;
		case 3: return 64000;
		default: return -EINVAL;
	}
}
#endif
//Fixme plan to transfer the adc value to the config.xml lux 2013-5-10
static __u16 uadc[8] = {2, 8, 100, 400, 900, 1000, 1500, 1900};//customize
static __u16 ulux[9] = {128, 200, 1300, 2000, 3000, 4000, 5000, 6000, 7000};
static __u16 adc_to_lux(__u16 adc)
{
	static long long var = 0;
	int i = 0; //length of array is 8,9
	for (i=0; i<8; i++) {
		if ( adc < uadc[i]){
			break;
		}
	}
	if ( i<9)
	{
		var++;
		if (var%2)
			return ulux[i]+0;
		else
			return ulux[i]+1;
	}
	return ulux[4];	
}



static int isl_get_lux_data(struct i2c_client* client)
{
	//struct isl_device* idev = i2c_get_clientdata(client);

	//__u16 resH = 0, resL = 0;
	__s16 resH = 0, resL = 0;
	//int range;
	resL = i2c_smbus_read_word_data(client, 0x50);
	//resH = i2c_smbus_read_word_data(client, 0x51)&0xff00;
	if ((resL < 0) || (resH < 0))
	{
		errlog("Error to read lux_data!\n");
		
		return 3000;//???
		//return -1;
	}
	//Fixme plan to transfer the adc value to the config.xml lux 2013-5-10
	if (!no_adc_map)
		resL = adc_to_lux(resL);
	//printk("<<<< lux %d\n", resL);
	return resL ;//* idev->range / idev->resolution;
	return (resH | resL) ;//* idev->range / idev->resolution;
}


static int isl_set_default_config(struct i2c_client *client)
{
	//struct isl_device* idev = i2c_get_clientdata(client);

	int ret=0;
	//ret = _cm3232_I2C_Write_Byte(CM3232_SLAVE_addr, CM3232_ALS_RESET);
	ret = i2c_smbus_write_byte_data(client, 0, (1 << 6));
	if (ret < 0)
		return -EINVAL;
	//if(ret<0)
		//return ret;  
	msleep(10);
		        
	//ret = _cm3232_I2C_Write_Byte(CM3232_SLAVE_addr, CM3232_ALS_IT_200ms | CM3232_ALS_HS_HIGH );
	ret = i2c_smbus_write_byte_data(client, 0, (1 << 2)|(1 << 1));
	if (ret < 0)
		return -EINVAL;
	msleep(10);
	return 0;
/* We don't know what it does ... */
//	ret = i2c_smbus_write_byte_data(client, REG_CMD_1, 0xE0);
//	ret = i2c_smbus_write_byte_data(client, REG_CMD_2, 0xC3);
/* Set default to ALS continuous */
	ret = i2c_smbus_write_byte_data(client, REG_CMD_1, 0xA0);
	if (ret < 0)
		return -EINVAL;
/* Range: 0~16000, number of clock cycles: 65536 */
	ret = i2c_smbus_write_byte_data(client, REG_CMD_2, 0x02); // vivienne
	if (ret < 0)
		return -EINVAL;
	//idev->resolution = isl_get_res(client);
	//idev->range = isl_get_range(client);;
    dbg("cm3232 set_default_config call, \n");

	return 0;
}

/* Return 0 if detection is successful, -ENODEV otherwise */
static int cm3232_detect(struct i2c_client *client/*, int kind,
                          struct i2c_board_info *info*/)
{
	

        return 0;
}

int isl_input_open(struct input_dev* input)
{
	return 0;
}

void isl_input_close(struct input_dev* input)
{
}

static void isl_input_lux_poll(struct input_polled_dev *dev)
{
	struct isl_device* idev = dev->private;
	struct input_dev* input = idev->input_poll_dev->input;
	struct i2c_client* client = idev->client;
	static unsigned int val=0;
	
	//printk("%s\n", __FUNCTION__);
	if (l_enable != 0)
	{
		mutex_lock(&mutex);
		//pm_runtime_get_sync(dev);
	#if 0	
		if(val>0x2000)
			val=0;
		val+=100;
	#endif	
		//printk(KERN_ALERT "by flashchen val is %x",val); 
		input_report_abs(input, ABS_MISC, isl_get_lux_data(client));
		//input_report_abs(input, ABS_MISC, val);//isl_get_lux_data(client));
		input_sync(input);
		//pm_runtime_put_sync(dev);
		mutex_unlock(&mutex);
	}
}

static struct i2c_device_id cm3232_id[] = {
	{"cm3232", 0},
	{}
};

#if 0
static int cm3232_runtime_suspend(struct device *dev)
{
	
	dev_dbg(dev, "suspend\n");

	mutex_lock(&mutex);
	pm_runtime_get_sync(dev);
	//isl_set_mod(client, ISL_MOD_POWERDOWN);
	pm_runtime_put_sync(dev);
	mutex_unlock(&mutex);

	printk(KERN_INFO MODULE_NAME ": %s cm3232 suspend call, \n", __func__);
	return 0;
}

static int cm3232_runtime_resume(struct device *dev)
{
	
	dev_dbg(dev, "resume\n");

	mutex_lock(&mutex);
	pm_runtime_get_sync(dev);
	//isl_set_mod(client, last_mod);
	pm_runtime_put_sync(dev);
	mutex_unlock(&mutex);

 printk(KERN_INFO MODULE_NAME ": %s cm3232 resume call, \n", __func__);
	return 0;
}
#endif
MODULE_DEVICE_TABLE(i2c, cm3232_id);

/*static const struct dev_pm_ops cm3232_pm_ops = {
	.runtime_suspend = cm3232_runtime_suspend,
	.runtime_resume = cm3232_runtime_resume,
};

static struct i2c_board_info isl_info = {
	I2C_BOARD_INFO("cm3232", 0x44),
};

static struct i2c_driver cm3232_driver = {
	.driver = {
		   .name = "cm3232",
		   .pm = &cm3232_pm_ops,
		   },
	.probe = cm3232_probe,
	.remove = cm3232_remove,
	.id_table = cm3232_id,
	.detect         = cm3232_detect,
	//.address_data   = &addr_data,
};*/

static int mmad_open(struct inode *inode, struct file *file)
{
	dbg("Open the l-sensor node...\n");
	return 0;
}

static int mmad_release(struct inode *inode, struct file *file)
{
	dbg("Close the l-sensor node...\n");
	return 0;
}

static ssize_t mmad_read(struct file *fl, char __user *buf, size_t cnt, loff_t *lf)
{
	int lux_data = 0;
	
	mutex_lock(&mutex);
	lux_data = isl_get_lux_data(l_sensorconfig->client);
	mutex_unlock(&mutex);
	if (lux_data < 0)
	{
		errlog("Failed to read lux data!\n");
		return -1;
	}
	printk(KERN_ALERT "lux_data is %x\n",lux_data); 
	//return 0;
	copy_to_user(buf, &lux_data, sizeof(lux_data));
	return sizeof(lux_data);
}

static long
mmad_ioctl(/*struct inode *inode,*/ struct file *file, unsigned int cmd,
	   unsigned long arg)
{
	void __user *argp = (void __user *)arg;
	//char rwbuf[5];
	short enable;  //amsr = -1;
	unsigned int uval;

	dbg("l-sensor ioctr...\n");
	//memset(rwbuf, 0, sizeof(rwbuf));
	switch (cmd) {
		case LIGHT_IOCTL_SET_ENABLE:
			// enable/disable sensor
			if (copy_from_user(&enable, argp, sizeof(short)))
			{
				printk(KERN_ERR "Can't get enable flag!!!\n");
				return -EFAULT;
			}
			dbg("enable=%d\n",enable);
			if ((enable >=0) && (enable <=1))
			{
				dbg("driver: disable/enable(%d) gsensor.\n", enable);
				
				//l_sensorconfig.sensor_enable = enable;
				dbg("Should to implement d/e the light sensor!\n");
				l_enable = enable;
				
			} else {
				printk(KERN_ERR "Wrong enable argument in %s !!!\n", __FUNCTION__);
				return -EINVAL;
			}
			break;
		case WMT_IOCTL_SENSOR_GET_DRVID:
#define CM3232_DRVID 0
			uval = CM3232_DRVID ;
			if (copy_to_user((unsigned int*)arg, &uval, sizeof(unsigned int)))
			{
				return -EFAULT;
			}
			dbg("cm3232_driver_id:%d\n",uval);
		default:
			break;
	}

	return 0;
}


static struct file_operations mmad_fops = {
	.owner = THIS_MODULE,
	.open = mmad_open,
	.release = mmad_release,
	.read = mmad_read,
	.unlocked_ioctl = mmad_ioctl,
};


static struct miscdevice mmad_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "lsensor_ctrl",
	.fops = &mmad_fops,
};

#ifdef CONFIG_HAS_EARLYSUSPEND
static void cm3232_early_suspend(struct early_suspend *h)
{
	struct i2c_client *client = l_sensorconfig->client;

	dbg("start\n");
	mutex_lock(&mutex);
	//pm_runtime_get_sync(dev);
	//isl_set_mod(client, ISL_MOD_POWERDOWN);
	//pm_runtime_put_sync(dev);
	mutex_unlock(&mutex);
	dbg("exit\n");
}

static void cm3232_late_resume(struct early_suspend *h)
{
	struct i2c_client *client = l_sensorconfig->client;

	dbg("start\n");
	mutex_lock(&mutex);
	//pm_runtime_get_sync(dev);
	//isl_set_mod(client, last_mod);
	isl_set_default_config(client);
	//pm_runtime_put_sync(dev);
	mutex_unlock(&mutex);	
	dbg("exit\n");
}
#endif
static ssize_t adc_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	
	int i;
	int size = sizeof(uadc)/sizeof(uadc[0]);
	printk("<<<%s\n", __FUNCTION__);
	for (i=0; i<size; i++)
	{
		printk(" %5d  ", uadc[i]);
		//sprintf(buf, "%d " &uadc[i]);
	}
	printk("\n");
	
	return sizeof(uadc);
	//printk(" %s \n", buf);
}

static ssize_t adc_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	__u32 tmp;
	int index;
	int n;
	int size = sizeof(uadc)/sizeof(uadc[0]);
	
	printk("<<<%s\n", __FUNCTION__);
	printk("<< %s >>>\n", buf);
	n = sscanf(buf, "%d:%d", &index, &tmp);
	printk("<<<<int n %d %d:%d \n", n, index, tmp);
	if ( n==2 && index>=0 && index<size){
	
		uadc[index] = tmp;
		no_adc_map = 0;
	}	
	else {
		printk("<<<param error!\n");
		no_adc_map = 1;
	}	
	return count;
}
static struct device_attribute cm3232_attr[] = 
{
	__ATTR(adc, 0644, adc_show, adc_store),
	__ATTR_NULL,
};

static struct device *sdevice;
static struct class *sclass;
static dev_t sdev_no;
static int device_create_attribute(struct device *dev, struct device_attribute *attr)
{
	int err, i;
	for (i=0; NULL != attr[i].attr.name; i++)
	{
		err = device_create_file(dev, &attr[i]); //&attr[i].attr
		if (err)
			break;
	}
	if (err)
	{
		for (; i>=0; i--)
		device_remove_file(dev, &attr[i]);//&attr[i].attr
	}
	return err;
}

static void device_remove_attribute(struct device *dev, struct device_attribute *attr)
{
	int i;
	for (i=0; attr[i].attr.name != NULL; i++)
		device_remove_file(dev, &attr[i]); //&attr[i].attr
}


static int get_adc_val(void)
{
	int i, varlen, n;
	__u32 buf[8];
	char varbuf[50];
	char *name = "wmt.io.lsensor";
	
	varlen = sizeof(varbuf);
	if (wmt_getsyspara(name, varbuf, &varlen))
	{
		printk("<<<<fail to wmt_syspara %s\n", "wmt.io.lsensor");
		no_adc_map = 1;
		return -1;
	}
	n = sscanf(varbuf, "%d:%d:%d:%d:%d:%d:%d:%d", &buf[0], &buf[1], &buf[2], &buf[3], &buf[4], &buf[5], \
	&buf[6], &buf[7]);
	//printk("<<< n %d \n", n);
	if (n != 8)
	{
		printk("<<<<<%s uboot env  wmt.io.lsensor param error!\n", __FUNCTION__);
		return -1;
	}
	for (i=0; i<8; i++)
	{
		//printk("<<<< %5d ", buf[i]);
		uadc[i] = buf[i];
	}
	no_adc_map = 0;
	//printk("\n");
	return 0;
}

static int
cm3232_probe(struct i2c_client *client/*, const struct i2c_device_id *id*/)
{
	int res=0;
	int sret = 0;
	struct isl_device* idev = kzalloc(sizeof(struct isl_device), GFP_KERNEL);
	if(!idev)
		return -ENOMEM;

	l_sensorconfig = idev;
	//android_lsensor_kobj = kobject_create_and_add("android_lsensor", NULL);
	/*
	if (android_lsensor_kobj == NULL) {
		errlog(
		       "lsensor_sysfs_init:"\
		       "subsystem_register failed\n");
		res = -ENOMEM;
		goto err_kobjetc_create;
	}
	//res = sysfs_create_group(android_lsensor_kobj, &m_isl_gr);
	if (res) {
		//pr_warn("cm3232: device create file failed!!\n");
 		printk(KERN_INFO MODULE_NAME ": %s cm3232 device create file failed\n", __func__);
		res = -EINVAL;
		goto err_sysfs_create;
	}
	*/

/* last mod is ALS continuous */
	last_mod = 5;
	//pm_runtime_enable(&client->dev);	
	idev->input_poll_dev = input_allocate_polled_device();
	if(!idev->input_poll_dev)
	{
		res = -ENOMEM;
		goto err_input_allocate_device;
	}
	idev->client = client;
	idev->input_poll_dev->private = idev;
	idev->input_poll_dev->poll = isl_input_lux_poll;
	idev->input_poll_dev->poll_interval = 100;//50;
	idev->input_poll_dev->input->open = isl_input_open;
	idev->input_poll_dev->input->close = isl_input_close;
	idev->input_poll_dev->input->name = "lsensor_lux";
	idev->input_poll_dev->input->id.bustype = BUS_I2C;
	idev->input_poll_dev->input->dev.parent = &client->dev;
	input_set_drvdata(idev->input_poll_dev->input, idev);
	input_set_capability(idev->input_poll_dev->input, EV_ABS, ABS_MISC);
	input_set_abs_params(idev->input_poll_dev->input, ABS_MISC, 0, 16000, 0, 0);
	i2c_set_clientdata(client, idev);
	/* set default config after set_clientdata */
	res = isl_set_default_config(client);	
	res = misc_register(&mmad_device);
	if (res) {
		errlog("mmad_device register failed\n");
		goto err_misc_register;
	}
	res = input_register_polled_device(idev->input_poll_dev);
	if(res < 0)
		goto err_input_register_device;
	// suspend/resume register
#ifdef CONFIG_HAS_EARLYSUSPEND
        idev->earlysuspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
        idev->earlysuspend.suspend = cm3232_early_suspend;
        idev->earlysuspend.resume = cm3232_late_resume;
        register_early_suspend(&(idev->earlysuspend));
#endif

	dbg("cm3232 probe succeed!\n");
	//create class device sysdevice 2013-5-10
	//get_adc_val();
	sclass = class_create(THIS_MODULE, "cm3232");
	if (IS_ERR(sclass))
	{
		printk("<<<%s fail to create class!\n", __FUNCTION__);
		return 0;
	}
	
	
	sret = alloc_chrdev_region(&sdev_no, 0, 1, "cm3232_devno");
	if (sret)
	{
		printk("<<<<%s alloc_chrdev_region fail!\n", __FUNCTION__);
		class_destroy(sclass);
		return 0;
	}
	sdevice = device_create(sclass, NULL, sdev_no, NULL, "cm3232_dev");
	if (IS_ERR(sdevice))
	{
		printk("<<<%s device_create fail!\n", __FUNCTION__);
		class_destroy(sclass);
		return 0;
	}
	device_create_attribute(sdevice, cm3232_attr);
	
	return 0;
err_input_register_device:
	misc_deregister(&mmad_device);
	input_free_polled_device(idev->input_poll_dev);
err_misc_register:
err_input_allocate_device:
	//__pm_runtime_disable(&client->dev, false);

	kobject_del(android_lsensor_kobj);

	kfree(idev);
	return res;
}

static int cm3232_remove(struct i2c_client *client)
{
	struct isl_device* idev = i2c_get_clientdata(client);
	if (!IS_ERR(sdevice))
	{
		device_remove_attribute(sdevice, cm3232_attr);
		device_destroy(sclass, sdev_no);
		class_destroy(sclass);
		
	}
	//unregister_early_suspend(&(idev->earlysuspend));
	misc_deregister(&mmad_device);
	input_unregister_polled_device(idev->input_poll_dev);
	input_free_polled_device(idev->input_poll_dev);
	//sysfs_remove_group(android_lsensor_kobj, &m_isl_gr);
	kobject_del(android_lsensor_kobj);
	//__pm_runtime_disable(&client->dev, false);
	kfree(idev);
	printk(KERN_INFO MODULE_NAME ": %s cm3232 remove call, \n", __func__);
	return 0;
}
//****************add platform_device & platform_driver for suspend &resume 2013-7-2
static int ls_probe(struct platform_device *pdev){
	//printk("<<<%s\n", __FUNCTION__);
	return 0;
}
static int ls_remove(struct platform_device *pdev){
	//printk("<<<%s\n", __FUNCTION__);
	return 0;
}
static int ls_suspend(struct platform_device *pdev, pm_message_t state){
	printk("<<<%s\n", __FUNCTION__);
	
	return 0;
}

static int ls_resume(struct platform_device *pdev){
	//return 0;  
	int ret = 0;
	int count = 0;
	printk("<<<%s\n", __FUNCTION__);
RETRY:	
	ret = isl_set_default_config(this_client);
	if (ret < 0){
		printk("%s isl_set_default_config fail!\n", __FUNCTION__);
		count++;
		if (count < 5){
			mdelay(2);
			goto RETRY;
		}	
		else
			return ret;
	}		
	return 0;
	
}
static void lsdev_release(struct device *dev)
{
	return;
}
static struct platform_device lsdev = {
	.name = "lsdevice",
	.id = -1,
	.dev = {
		.release = lsdev_release,
	},
};
static struct platform_driver lsdrv = {
	.probe = ls_probe,
	.remove = ls_remove,
	.suspend = ls_suspend,
	.resume = ls_resume,
	.driver = {
		.name = "lsdevice",
	},
};
//********************************************************************

static int __init sensor_cm3232_init(void)
{
	int ret = 0;
	printk(KERN_INFO MODULE_NAME ": %s cm3232 init call, \n", __func__);
	/*
	 * Force device to initialize: i2c-15 0x44
	 * If i2c_new_device is not called, even cm3232_detect will not run
	 * TODO: rework to automatically initialize the device
	 */
	//i2c_new_device(i2c_get_adapter(15), &isl_info);
	//return i2c_add_driver(&cm3232_driver);
	if (!(this_client = sensor_i2c_register_device(2, SENSOR_I2C_ADDR, SENSOR_I2C_NAME)))
	{
		printk(KERN_EMERG"Can't register gsensor i2c device!\n");
		return -1;
	}
	if (cm3232_detect(this_client))
	{
		errlog("Can't find light sensor cm3232!\n");
		goto detect_fail;
	}
	get_adc_val();
	if(cm3232_probe(this_client))
	{
		errlog("Erro for probe!\n");
		goto detect_fail;
	}
	
	ret = platform_device_register(&lsdev);
	if (ret){
		printk("<<<platform_device_register fail!\n");
		return ret;
	}
	ret = platform_driver_register(&lsdrv);
	if (ret){
		printk("<<<platform_driver_register fail!\n");
		platform_device_unregister(&lsdev);
		return ret;
	}
	return 0;

detect_fail:
	sensor_i2c_unregister_device(this_client);
	return -1;
}

static void __exit sensor_cm3232_exit(void)
{
 	printk(KERN_INFO MODULE_NAME ": %s cm3232 exit call \n", __func__);
 	cm3232_remove(this_client);
 	sensor_i2c_unregister_device(this_client);
	platform_driver_unregister(&lsdrv);
	platform_device_unregister(&lsdev);
	//i2c_del_driver(&cm3232_driver);
}

module_init(sensor_cm3232_init);
module_exit(sensor_cm3232_exit);

MODULE_AUTHOR("flash");
MODULE_ALIAS("cm3232 ALS");
MODULE_DESCRIPTION("Intersil cm3232 ALS Driver");
MODULE_LICENSE("GPL v2");

