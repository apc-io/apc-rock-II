/*
 * us5182.c - us5182 ALS & Proximity Driver
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
//#include <linux/earlysuspend.h>
#include <linux/types.h>
#include "../sensor.h"
#include "us5182.h"
/* Insmod parameters */
//I2C_CLIENT_INSMOD_1(us5182);

#define MODULE_NAME	"us5182"

#undef dbg
#define dbg(fmt, args...) 

#undef errlog
#undef klog
#define errlog(fmt, args...) printk(KERN_ERR "[%s]: " fmt, __FUNCTION__, ## args)
#define klog(fmt, args...) printk(KERN_ALERT "[%s]: " fmt, __FUNCTION__, ## args)

struct us_device {
	struct input_polled_dev* input_poll_devl;
	struct input_polled_dev* input_poll_devp;
	struct i2c_client* client;
	struct class* class;
	struct device *lsdev;
	dev_t devno;
#ifdef CONFIG_HAS_EARLYSUSPEND
	struct early_suspend earlysuspend;
#endif
	u8     enable_id;

};
static int psh_l8th = 90;
static int psh_h8th = 0;
	
static int psl_l8th = 50;
static int psl_h8th = 0;

static struct i2c_client *this_client = NULL;
/*=====Global variable===============================*/
static u8  error_flag, debounces;
static int previous_value, this_value;
static struct i2c_client *gclient = NULL;
/*===================================================*/
static u8 reg_cache[us5182_NUM_CACHABLE_REGS];

static struct us_device* l_sensorconfig = NULL;
static int l_enable = 0; // 0:don't report data
static int p_enable = 0; // 0:don't report data

extern int wmt_getsyspara(char *varname, unsigned char *varval, int *varlen);
static int no_adc_map = 1;

static DEFINE_MUTEX(mutex);

static int us5182_i2c_read(struct i2c_client *client,u8 reg) 
{
#if 0
	int val;
        val = i2c_smbus_read_byte_data(client, reg);
        if (val < 0)
                printk("%s %d i2c transfer error\n", __func__, __LINE__);
        return val;
#endif
//in default our i2c controller, will not send repeatStart signal in read process.(stop-start)
//well this sensor must have the repeatStart signal to work normally
//so we have to pass I2C_M_NOSTART flag to controller 2013-7-5	
	char rdData[2] = {0};
	
	struct i2c_msg msgs[2] = 
	{
		{.addr = client->addr, .flags = 0|I2C_M_NOSTART, .len = 1, .buf = rdData,}, 
		{.addr = client->addr, .flags = I2C_M_RD, .len = 1, .buf = rdData,},
	};
	rdData[0] = reg;
	
	if (i2c_transfer(client->adapter, msgs, 2) < 0) {
		printk( "%s: transfer failed.", __func__);
		return -EIO;
	}
	
	return rdData[0];
}

static int get_als_resolution(struct i2c_client *client)
{		
        return (us5182_i2c_read(client,REGS_CR01) & 0x18) >> 3;
}


static int isl_get_lux_datal(struct i2c_client* client)
{
	
        int lsb, msb, bitdepth;

        mutex_lock(&mutex);
        lsb = us5182_i2c_read(client, REGS_LSB_SENSOR);//

        if (lsb < 0) {
                mutex_unlock(&mutex);
                return lsb;
        }

        msb = us5182_i2c_read(client, REGS_MSB_SENSOR);//
        mutex_unlock(&mutex);

        if (msb < 0)
                return msb;

	bitdepth = get_als_resolution(client);//?????
	switch(bitdepth){
	case 0:	
		lsb &= 0xF0; // 12bit??
		lsb >>= 4;  //add
		return ((msb << 4) | lsb);
		break;
	case 1:
		lsb &= 0xFC;  //?? 14bit
		lsb >>= 2;
		return ((msb << 6) | lsb);
		break;
	}	
	
	return ((msb << 8) | lsb);
}


static int get_ps_resolution(struct i2c_client *client)
{
        u8 data;

	data = (us5182_i2c_read(client,REGS_CR02) & 0x18) >> 3;

	return data;
}

static int isl_get_lux_datap(struct i2c_client* client)
{
	
        int lsb, msb, bitdepth;

        mutex_lock(&mutex);
        lsb = us5182_i2c_read(client, REGS_LSB_SENSOR_PS);

        if (lsb < 0) {
                mutex_unlock(&mutex);
                return lsb;
        }

        msb = us5182_i2c_read(client, REGS_MSB_SENSOR_PS);
        mutex_unlock(&mutex);

        if (msb < 0)
                return msb;

	bitdepth = get_ps_resolution(client);
	switch(bitdepth){
	case 0:	
		lsb &= 0xF0; // 12bit ??
		lsb >>= 4;
		return ((msb << 4) | lsb);
		break;
	case 1:
		lsb &= 0xFC; // 14bit ??
		lsb >>= 2;
		return ((msb << 6) | lsb);
		break;
	}	
	
	return ((msb << 8) | lsb); //we use 16bit now
}



/* Return 0 if detection is successful, -ENODEV otherwise */
static int us5182_detect(struct i2c_client *client/*, int kind,
                          struct i2c_board_info *info*/)
{	
	
	char rxData[2] = {0xb2, 0};
	int ret = 0;

#if 1	
	//rxData[0] = 0xb2;
	
	struct i2c_msg msgs[2] = {
	
			{.addr = client->addr, .flags = 0|I2C_M_NOSTART, .len = 1, .buf = rxData,},
			{.addr = client->addr, .flags = I2C_M_RD, .len = 1, .buf = rxData,}
		};
	ret = i2c_transfer(client->adapter, msgs, 2);
	if (ret < 0){
		printk(KERN_ERR "%s i2c_transfer error!\n", __FUNCTION__);
		return -EIO;
	}
			
#endif 	

	if(0x26 == rxData[0])
	{
		printk(KERN_ALERT "us5182 detected OK\n");
		return 0;
	}
	else
		return -1;
}

int isl_input_open(struct input_dev* input)
{
	return 0;
}

void isl_input_close(struct input_dev* input)
{
}

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
			return ulux[i]-1;
	}
	return ulux[4];	
}

static void isl_input_lux_poll_l(struct input_polled_dev *dev)
{
	struct us_device* idev = dev->private;
	struct input_dev* input = idev->input_poll_devl->input;
	struct i2c_client* client = idev->client;
	int ret_val = 0;
	
	if (client == NULL){
		printk("%s client NULL!\n", __FUNCTION__);
		return;
	}
	
	//printk("%s\n", __FUNCTION__);
	if (l_enable != 0)
	{
		//mutex_lock(&mutex); //dead lock!! 2013-7-9!!!
		//printk(KERN_ALERT "by flashchen val is %x",val);
		ret_val = isl_get_lux_datal(client); //adc
		if (ret_val < 0)
			return;
		if (!no_adc_map)
		ret_val = adc_to_lux(ret_val);
		
		input_report_abs(input, ABS_MISC, ret_val);
		//printk("%s %d\n", __FUNCTION__, ret_val);
		input_sync(input);
		//mutex_unlock(&mutex);
	}
}

static void isl_input_lux_poll_p(struct input_polled_dev *dev)
{
	struct us_device* idev = dev->private;
	struct input_dev* input = idev->input_poll_devp->input;
	struct i2c_client* client = idev->client;
	
	int tmp_val = 0, debounce = 0;
	int ret_val = 0;
	
	//printk("%s\n", __FUNCTION__);
	if (p_enable != 0)
	{
		//mutex_lock(&mutex);
		//printk(KERN_ALERT "by flashchen val is %x",val);
		
	#if 0 //just read raw data out	2013-7-18
		for (debounce=0; debounce<10; debounce++){	
			ret_val = isl_get_lux_datap(client);
			if (ret_val < 0)
				return;
			
			tmp_val += ret_val;
			msleep(1);
		}
		tmp_val /= 10;
	//add for near/far detection!	
		if (tmp_val > 0x00ff)
			tmp_val = 6;
		else
			tmp_val = 0;
		input_report_abs(input, ABS_MISC, tmp_val);
		input_sync(input);	
	#endif	
		
		tmp_val = us5182_i2c_read(client, REGS_CR00); 
		
		if (tmp_val & CR0_PROX_MASK)  //approach
			input_report_abs(input, ABS_MISC, 0);
		else
			input_report_abs(input, ABS_MISC, 6);
			
		input_sync(input);
		//printk("%s %d\n", __FUNCTION__, tmp_val);
		//mutex_unlock(&mutex);
	}
}

#if 0
static struct i2c_device_id us5182_id[] = {
	{"us5182", 0},
	{}
};

MODULE_DEVICE_TABLE(i2c, us5182_id);
#endif // 2013-7-9

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

static ssize_t mmadl_read(struct file *fl, char __user *buf, size_t cnt, loff_t *lf)
{
	int lux_data = 0;
	//printk("%s try to mutex_lock \n", __FUNCTION__);
	//mutex_lock(&mutex);
	//printk("lock ok!\n");
	lux_data = isl_get_lux_datal(l_sensorconfig->client);
	//mutex_unlock(&mutex);
	if (lux_data < 0)
	{
		printk("Failed to read lux data!\n");
		return -1;
	}
	printk(KERN_ALERT "lux_data is %x\n",lux_data); 
	//return 0;
	copy_to_user(buf, &lux_data, sizeof(lux_data));
	return sizeof(lux_data);
}


static ssize_t mmadp_read(struct file *fl, char __user *buf, size_t cnt, loff_t *lf)
{
	int lux_data = 0;
	
	//mutex_lock(&mutex);
	lux_data = isl_get_lux_datap(l_sensorconfig->client);
	//mutex_unlock(&mutex);
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
mmadl_ioctl(/*struct inode *inode,*/ struct file *file, unsigned int cmd,
	   unsigned long arg)
{
	void __user *argp = (void __user *)arg;
	//char rwbuf[5];
	short enable;  //amsr = -1;
	unsigned int uval;

	//printk("l-sensor ioctr... cmd 0x%x arg %d\n", cmd, arg);
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
#define DRVID 0
			uval = DRVID ;
			if (copy_to_user((unsigned int*)arg, &uval, sizeof(unsigned int)))
			{
				return -EFAULT;
			}
			dbg("us5182_driver_id:%d\n",uval);
		default:
			break;
	}

	return 0;
}


static long
mmadp_ioctl(/*struct inode *inode,*/ struct file *file, unsigned int cmd,
	   unsigned long arg)
{
	void __user *argp = (void __user *)arg;
	//char rwbuf[5];
	short enable;  //amsr = -1;
	unsigned int uval;
	unsigned char regval;

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
				p_enable = enable;
			#if 1	
				if(p_enable)
				{
					regval = us5182_i2c_read(l_sensorconfig->client, 0);
					regval &= ~(3 << 4);
					i2c_smbus_write_byte_data(l_sensorconfig->client, 0, regval);
				}
				else
				{
					regval = us5182_i2c_read(l_sensorconfig->client, 0);
					regval &= ~(3 << 4);
					regval |= (1 << 4);
					i2c_smbus_write_byte_data(l_sensorconfig->client, 0, regval);
				}
			#endif	
				
			} else {
				printk(KERN_ERR "Wrong enable argument in %s !!!\n", __FUNCTION__);
				return -EINVAL;
			}
			break;
		case WMT_IOCTL_SENSOR_GET_DRVID:
#define DRVID 0
			uval = DRVID ;
			if (copy_to_user((unsigned int*)arg, &uval, sizeof(unsigned int)))
			{
				return -EFAULT;
			}
			dbg("us5182_driver_id:%d\n",uval);
		default:
			break;
	}

	return 0;
}


static struct file_operations mmadl_fops = {
	.owner = THIS_MODULE,
	.open = mmad_open,
	.release = mmad_release,
	.read = mmadl_read,
	.unlocked_ioctl = mmadl_ioctl,
};

static struct miscdevice mmadl_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "lsensor_ctrl",
	.fops = &mmadl_fops,
};

static struct file_operations mmadp_fops = {
	.owner = THIS_MODULE,
	.open = mmad_open,
	.release = mmad_release,
	.read = mmadp_read,
	.unlocked_ioctl = mmadp_ioctl,
};

static struct miscdevice mmadp_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "psensor_ctrl",
	.fops = &mmadp_fops,
};

#ifdef CONFIG_HAS_EARLYSUSPEND
static void us5182_early_suspend(struct early_suspend *h)
{
	dbg("start\n");
	mutex_lock(&mutex);
	//pm_runtime_get_sync(dev);
	//isl_set_mod(client, ISL_MOD_POWERDOWN);
	//pm_runtime_put_sync(dev);
	mutex_unlock(&mutex);
	dbg("exit\n");
}

static void us5182_late_resume(struct early_suspend *h)
{
	struct i2c_client *client = l_sensorconfig->client;

	dbg("start\n");
	mutex_lock(&mutex);
	//pm_runtime_get_sync(dev);
	//isl_set_mod(client, last_mod);
	//isl_set_default_config(client);
	//pm_runtime_put_sync(dev);
	mutex_unlock(&mutex);	
	dbg("exit\n");
}
#endif

int us5182_i2c_write(struct i2c_client *client, u8 reg,u8 mask, u8 shift, int val ) {
        //struct us5182_data *data = i2c_get_clientdata(client);
	int err;
        u8 tmp;
	mutex_lock(&mutex);

	tmp = reg_cache[reg];
        tmp &= ~mask;
        tmp |= val << shift;

        err = i2c_smbus_write_byte_data(client, reg, tmp);
        if (!err)
                reg_cache[reg] = tmp;

        mutex_unlock(&mutex);
        if (err >= 0) return 0;

        printk("%s %d i2c transfer error\n", __func__, __LINE__);
        return err;
}


static int set_word_mode(struct i2c_client *client, int mode)
{
	//
	return us5182_i2c_write(client, REGS_CR00,
                CR0_WORD_MASK, CR0_WORD_SHIFT, mode);
}


static int set_oneshotmode(struct i2c_client *client, int mode)
{
        return us5182_i2c_write(client,REGS_CR00,CR0_ONESHOT_MASK, CR0_ONESHOT_SHIFT, mode);
}


static int set_opmode(struct i2c_client *client, int mode)
{
        return us5182_i2c_write(client,REGS_CR00,CR0_OPMODE_MASK, 
		CR0_OPMODE_SHIFT, mode);
}

/* power_status */
static int set_power_status(struct i2c_client *client, int status)
{
	if(status == CR0_SHUTDOWN_EN )
                return us5182_i2c_write(client,REGS_CR00,CR0_SHUTDOWN_MASK, 
                	CR0_SHUTDOWN_SHIFT,CR0_SHUTDOWN_EN);
	else
		return us5182_i2c_write(client,REGS_CR00,CR0_SHUTDOWN_MASK, 
			CR0_SHUTDOWN_SHIFT, CR0_OPERATION);
}


static int us5182_init_client(struct i2c_client *client)
{
        
        struct i2c_adapter *adapter = to_i2c_adapter(client->dev.parent);
	int i = 0;
	int v = -1;
	if ( !i2c_check_functionality(adapter,I2C_FUNC_SMBUS_BYTE_DATA) ) {
                printk(KERN_INFO "byte op is not permited.\n");
                return -EIO;
        }

        /* read all the registers once to fill the cache.
         * if one of the reads fails, we consider the init failed */
	
        for (i = 0; i < ARRAY_SIZE(reg_cache); i++) {
                v = us5182_i2c_read(client, i);
		printk("reg 0x%x value 0x%x \n", i, v);
                if (v < 0)
                        return -ENODEV;
                reg_cache[i] = v;
        }

	/*Set Default*/
	set_word_mode(client, 0);//word enable? //just byte one time
	set_power_status(client,CR0_OPERATION); //power on?
	set_opmode(client,CR0_OPMODE_ALSONLY); //CR0_OPMODE_ALSANDPS CR0_OPMODE_ALSONLY
	set_oneshotmode(client, CR0_ONESHOT_DIS);
	
	us5182_i2c_write(client, REGS_CR03, CR3_LEDDR_MASK, CR3_LEDDR_SHIFT, CR3_LEDDR_50);
	
	//set als gain
	us5182_i2c_write(client, REGS_CR01, CR1_ALS_GAIN_MASK, CR1_ALS_GAIN_SHIFT, CR1_ALS_GAIN_X8);
	
	//set ps threshold --> lth, hth
	us5182_i2c_write(client, REGS_INT_LSB_TH_HI_PS, 0xff, 0, psh_l8th);  //
	us5182_i2c_write(client, REGS_INT_MSB_TH_HI_PS, 0xff, 0, psh_h8th); //0 default
	
	us5182_i2c_write(client, REGS_INT_LSB_TH_LO_PS, 0xff, 0, psl_l8th); //
	us5182_i2c_write(client, REGS_INT_MSB_TH_LO_PS, 0xff, 0, psl_h8th); // 0 default
	//set_resolution(client,us5182_RES_16);
	//set_range(client,3);
	//set_int_ht(client,0x3E8); //1000 lux
	//set_int_lt(client,0x8); //8 lux
	//dev_info(&data->client->dev, "us5182 ver. %s found.\n",DRIVER_VERSION);
	
	/*init global variable*/
	error_flag  = 0;
	previous_value  = 0;
	this_value = 0;
	debounces = 2;//debounces 5 times
	return 0;
}
//******add for sys debug devic_attribute
static ssize_t reg_show(struct device *dev, struct device_attribute *attr, char *buf) {

	int i = 0, val = 0;
	printk("<<<<<<<<<reg dump\n");
	
	for (i=0; i<=0x1f; i++) {
		val = us5182_i2c_read(gclient, i);
		printk("reg 0x%x: val 0x%x \n", i, val);
			
	}
	printk("\n");
	printk("<<<<<<<<<<<<<<<<<<<<dump end\n");
	return 0;
}

static ssize_t reg_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count) {

	int reg = 0, val = 0;
	int ret = 0;
	int res = 0;
	
	printk("buf is %s\n", buf);
	
	ret = sscanf(buf, "%x:%x", &reg, &val);
	printk("reg:val is 0x%x:0x%x\n", reg, val);
	
	if (reg<0 || val<0) {
		printk("param error!\n");
		return -1;
	}		
	
	res = us5182_i2c_read(gclient, reg);
	i2c_smbus_write_byte_data(gclient, reg, val);
	printk(KERN_ERR "reg 0x%x 0x%x -->0x%x\n", reg, res, val);
	
	return count;
}
//struct device_attribute dev_attr_reg = __ATTR(reg, 0644, reg_show, reg_store);
//DEVICE_ATTR(reg, 0644, reg_show, reg_store);

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

static struct device_attribute us5182_attr[] = 
{
	__ATTR(reg, 0644, reg_show, reg_store),
	__ATTR(adc, 0644, adc_show, adc_store),
	__ATTR_NULL,
};
  
static int device_create_attribute(struct device *dev, struct device_attribute *attr)
{
	int err = 0, i;
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
//add end
static int
us5182_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int res=0;

	struct us_device* idev = kzalloc(sizeof(struct us_device), GFP_KERNEL);
	if(!idev)
		return -ENOMEM;

	l_sensorconfig = idev;
	
	/*initial enable device id*/
	idev->enable_id = 0x00;
	
	gclient = client;
	/* initialize the us5182 chip */
        res = us5182_init_client(client);//
	
        if (res != 0)
              goto err_input_allocate_device;
/* last mod is ALS continuous */
	//pm_runtime_enable(&client->dev);	
	idev->input_poll_devl = input_allocate_polled_device();
	if(!idev->input_poll_devl)
	{
		res = -ENOMEM;
		goto err_input_allocate_device;
	}
	idev->input_poll_devp = input_allocate_polled_device();
	if(!idev->input_poll_devp)
	{
		res = -ENOMEM;
		goto err_input_allocate_device;
	}
	idev->client = client;

	idev->input_poll_devl->private = idev;
	idev->input_poll_devl->poll = isl_input_lux_poll_l;
	idev->input_poll_devl->poll_interval = 100;//50;
	idev->input_poll_devl->input->open = isl_input_open;
	idev->input_poll_devl->input->close = isl_input_close;
	idev->input_poll_devl->input->name = "lsensor_lux";
	idev->input_poll_devl->input->id.bustype = BUS_I2C;
	idev->input_poll_devl->input->dev.parent = &client->dev;

	input_set_drvdata(idev->input_poll_devl->input, idev);
	input_set_capability(idev->input_poll_devl->input, EV_ABS, ABS_MISC);
	input_set_abs_params(idev->input_poll_devl->input, ABS_MISC, 0, 16000, 0, 0);

	idev->input_poll_devp->private = idev;
	idev->input_poll_devp->poll = isl_input_lux_poll_p;
	idev->input_poll_devp->poll_interval = 10;//100; 50ms
	idev->input_poll_devp->input->open = isl_input_open;
	idev->input_poll_devp->input->close = isl_input_close;
	idev->input_poll_devp->input->name = "psensor_lux";
	idev->input_poll_devp->input->id.bustype = BUS_I2C;
	idev->input_poll_devp->input->dev.parent = &client->dev;

	input_set_drvdata(idev->input_poll_devp->input, idev);
	input_set_capability(idev->input_poll_devp->input, EV_ABS, ABS_MISC);
	input_set_abs_params(idev->input_poll_devp->input, ABS_MISC, 0, 16000, 0, 0);
	i2c_set_clientdata(client, idev);
	/* set default config after set_clientdata */
	//res = isl_set_default_config(client);	
	res = misc_register(&mmadl_device);
	if (res) {
		errlog("mmad_device register failed\n");
		goto err_misc_registerl;
	}
	res = misc_register(&mmadp_device);
	if (res) {
		errlog("mmad_device register failed\n");
		goto err_misc_registerp;
	}
	res = input_register_polled_device(idev->input_poll_devl);
	if(res < 0)
		goto err_input_register_devicel;
	res = input_register_polled_device(idev->input_poll_devp);
	if(res < 0)
		goto err_input_register_devicep;
	// suspend/resume register
#ifdef CONFIG_HAS_EARLYSUSPEND
        idev->earlysuspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
        idev->earlysuspend.suspend = us5182_early_suspend;
        idev->earlysuspend.resume = us5182_late_resume;
        register_early_suspend(&(idev->earlysuspend));
#endif

	dbg("us5182 probe succeed!\n");
	res = alloc_chrdev_region(&idev->devno, 0, 1, "us5182");
  	if(res)
	{
		printk("can't allocate chrdev\n");
		return 0;
	}
	idev->class = class_create(THIS_MODULE, "us5182-lsensor");
	if (IS_ERR(idev->class)) {
		printk("<<< %s class_create() error!\n", __FUNCTION__);
		return 0;
	}
	idev->lsdev = device_create(idev->class, NULL, idev->devno, NULL, "us5182");
	if (IS_ERR(idev->lsdev)) {
		printk("<<< %s device_create() error!\n", __FUNCTION__);
		return 0;
	}
	res = device_create_attribute(idev->lsdev, us5182_attr);
	return 0;
err_input_register_devicep:
	input_free_polled_device(idev->input_poll_devp);
err_input_register_devicel:
	input_free_polled_device(idev->input_poll_devl);
err_misc_registerp:
	misc_deregister(&mmadp_device);
err_misc_registerl:
	misc_deregister(&mmadl_device);
err_input_allocate_device:
	//__pm_runtime_disable(&client->dev, false);
	kfree(idev);
	return res;
}

static int us5182_remove(struct i2c_client *client)
{
	int i = 0;
	struct us_device* idev = i2c_get_clientdata(client);
#if 1	
	//device_remove_file(idev->lsdev, &dev_attr_reg);
	device_remove_attribute(idev->lsdev, us5182_attr);
	unregister_chrdev_region(idev->devno, 1);
	device_destroy(idev->class, idev->devno);
	class_destroy(idev->class);
#endif	
	printk("%s %d\n", __FUNCTION__, i++); // 0
	//unregister_early_suspend(&(idev->earlysuspend));
	misc_deregister(&mmadl_device);
	printk("%s %d\n", __FUNCTION__, i++);
	misc_deregister(&mmadp_device);
	printk("%s %d\n", __FUNCTION__, i++);
	input_unregister_polled_device(idev->input_poll_devl);//here block??
	printk("%s %d\n", __FUNCTION__, i++);
	input_unregister_polled_device(idev->input_poll_devp);
	printk("%s %d\n", __FUNCTION__, i++);
	input_free_polled_device(idev->input_poll_devl);
	printk("%s %d\n", __FUNCTION__, i++);
	input_free_polled_device(idev->input_poll_devp);
	printk("%s %d\n", __FUNCTION__, i++);
	//__pm_runtime_disable(&client->dev, false);

	kfree(idev);
	printk(KERN_INFO MODULE_NAME ": %s us5182 remove call, \n", __func__);
	return 0;
}
static void us5182_shutdown(struct i2c_client *client)
{
	l_enable = 0;
	p_enable = 0;	
}

static int us5182_suspend(struct i2c_client *client, pm_message_t message)
{
	return 0;
}
static int us5182_resume(struct i2c_client *client)
{
	int res = 0;
	res = us5182_init_client(client);//
	return 0;
}

static const struct i2c_device_id us5182_id[] = {
	{ SENSOR_I2C_NAME , 0 },
	{},
};


static struct i2c_driver us5182_i2c_driver = 
{
	.probe		= us5182_probe,
	.remove		= us5182_remove,
	.suspend	= us5182_suspend,
	.resume 	= us5182_resume,
	.shutdown 	= us5182_shutdown,
	.driver 	= {
			.name	= SENSOR_I2C_NAME,
			.owner 	= THIS_MODULE,
			},
	.id_table	= us5182_id,	
};

static int get_adc_val(void)
{
	int i=0, varlen=0, n=0;
	__u32 buf[8] = {0};
	char varbuf[50] ={0};
	char *name = "wmt.io.lsensor";
	char *psth = "wmt.io.psensor";
	int thbuf[4] = {0};
	
	varlen = sizeof(varbuf);
	if (wmt_getsyspara(psth, varbuf, &varlen))
	{
		printk("<<<<fail to wmt_syspara %s\n", "wmt.io.psensor");
		
	}
	else
	{
		n = sscanf(varbuf, "%d:%d:%d:%d", &thbuf[0], &thbuf[1], &thbuf[2], &thbuf[3]);
		if (n == 4)
		{
			psh_h8th = thbuf[0];
			psh_l8th = thbuf[1];
			psl_h8th = thbuf[2];
			psl_l8th = thbuf[3];
		}
		else
			printk("wmt.io.psensor error!\n");
	}
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

static int __init sensor_us5182_init(void)
{
	 printk(KERN_INFO MODULE_NAME ": %s us5182 init call, \n", __func__);
	/*
	 * Force device to initialize: i2c-15 0x44
	 * If i2c_new_device is not called, even us5182_detect will not run
	 * TODO: rework to automatically initialize the device
	 */
	//i2c_new_device(i2c_get_adapter(15), &isl_info);
	//return i2c_add_driver(&us5182_driver);
	if (!(this_client = sensor_i2c_register_device(4, SENSOR_I2C_ADDR, SENSOR_I2C_NAME)))  
	{
		printk(KERN_EMERG"Can't register gsensor i2c device!\n");
		return -1;
	}
	if (us5182_detect(this_client)) //
	{
		errlog("Can't find light sensor us5182!\n");
		goto detect_fail;
	}
	
	get_adc_val();
/*	
	if(us5182_probe(this_client))
	{
		errlog("Erro for probe!\n");
		goto detect_fail;
	}
*/	
	if(i2c_add_driver(&us5182_i2c_driver) < 0)
	{
		errlog("Erro for i2c_add_driver()!\n");
		goto detect_fail;
	}
	return 0;

detect_fail:
	sensor_i2c_unregister_device(this_client);
	return -1;
}

static void __exit sensor_us5182_exit(void)
{
 	printk(KERN_INFO MODULE_NAME ": %s us5182 exit call \n", __func__);
 	//us5182_remove(this_client);
	i2c_del_driver(&us5182_i2c_driver);
 	sensor_i2c_unregister_device(this_client);
	
}

module_init(sensor_us5182_init);
module_exit(sensor_us5182_exit);

MODULE_AUTHOR("rambo");
MODULE_ALIAS("us5182 ALS");
MODULE_DESCRIPTION("us5182 Driver");
MODULE_LICENSE("GPL v2");

