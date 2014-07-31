/* 
 * drivers/input/touchscreen/ft5x0x/ft5x0x.c
 *
 * FocalTech ft5x0x TouchScreen driver. 
 *
 * Copyright (c) 2010  Focal tech Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 *
 *	note: only support mulititouch	Wenfs 2010-10-01
 */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/i2c.h>
#include <linux/input.h>
#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <mach/hardware.h>
#include <linux/platform_device.h>
#include <linux/suspend.h>
#include <linux/wait.h>
#include <asm/uaccess.h>
#include <linux/irq.h>
#include <linux/gpio.h>
#include <mach/wmt_iomux.h>
#include <linux/slab.h>
#include "ft5x0x.h"

struct ft5x0x_data *pContext=NULL;
static struct i2c_client *l_client=NULL;

#ifdef TOUCH_KEY 
static int keycodes[NUM_KEYS] ={
	KEY_MENU,
	KEY_HOME,
	KEY_BACK,
	KEY_SEARCH
};
#endif

#define FT5402_CONFIG_NAME "fttpconfig_5402public.ini"
extern int ft5x0x_read_fw_ver(void);
extern int ft5x0x_auto_clb(void);
extern int ft5x0x_upg_fw_bin(struct ft5x0x_data *ft5x0x, int check_ver);
extern int ft5402_Get_Param_From_Ini(char *config_name);
extern int ft5402_Init_IC_Param(struct i2c_client *client);
extern int ft5402_get_ic_param(struct i2c_client *client);
extern int ft5402_read_reg(struct i2c_client * client, u8 regaddr, u8 * regvalue);

int ft5x0x_i2c_rxdata(char *rxdata, int length)
{
	int ret;
	struct i2c_msg msg[2];

	msg[0].addr = pContext->addr;
	msg[0].flags = 0 | I2C_M_NOSTART;
	msg[0].len = 1;
	msg[0].buf = rxdata;

	msg[1].addr = pContext->addr;
	msg[1].flags = I2C_M_RD;
	msg[1].len = length;
	msg[1].buf = rxdata;

	ret = i2c_transfer(pContext->client->adapter, msg, 2);
	if (ret <= 0)
		dbg_err("msg i2c read error: %d\n", ret);

	return ret;
}


int ft5x0x_i2c_txdata(char *txdata, int length)
{
	int ret;
	struct i2c_msg msg[1];

	msg[0].addr = pContext->addr;
	msg[0].flags = 0;
	msg[0].len = length;
	msg[0].buf = txdata;

	ret = i2c_transfer(pContext->client->adapter, msg, 1);
	if (ret <= 0)
		dbg_err("msg i2c read error: %d\n", ret);

	return ret;
}

static void ft5x0x_penup(struct ft5x0x_data *ft5x0x)
{
	input_mt_sync(ft5x0x->input_dev);
	input_sync(ft5x0x->input_dev);
#ifdef TOUCH_KEY
	if(ft5x0x->tskey_used && ft5x0x->tkey_pressed && ft5x0x->tkey_idx < NUM_KEYS ){
		input_report_key(ft5x0x->input_dev, keycodes[ft5x0x->tkey_idx], 1);
		input_sync(ft5x0x->input_dev);
		input_report_key(ft5x0x->input_dev, keycodes[ft5x0x->tkey_idx], 0); 
		input_sync(ft5x0x->input_dev);
		dbg("report as key event %d \n",ft5x0x->tkey_idx);
	}
#endif 

	dbg("pen up\n");
	return;
}

#ifdef TOUCH_KEY
static int ft5x0x_read_tskey(struct ft5x0x_data *ft5x0x,int x,int y)
{
	int px,py;

	if(ft5x0x->tkey.axis){
		px = y;
		py = x;
	}else{
		px = x;
		py = y;
	}

	if(px >= ft5x0x->tkey.x_lower && px<=ft5x0x->tkey.x_upper){
		ft5x0x->tkey_pressed = 1;
		if(py>= ft5x0x->tkey.ypos[0].y_lower && py<= ft5x0x->tkey.ypos[0].y_upper){
			ft5x0x->tkey_idx= 0;
		}else if(py>= ft5x0x->tkey.ypos[1].y_lower && py<= ft5x0x->tkey.ypos[1].y_upper){
			ft5x0x->tkey_idx = 1;
		}else if(py>= ft5x0x->tkey.ypos[2].y_lower && py<= ft5x0x->tkey.ypos[2].y_upper){
			ft5x0x->tkey_idx = 2;
		}else if(py>= ft5x0x->tkey.ypos[3].y_lower && py<= ft5x0x->tkey.ypos[3].y_upper){
			ft5x0x->tkey_idx = 3;
		}else{
			ft5x0x->tkey_idx = NUM_KEYS;
		}

		return 1; 
	}

	ft5x0x->tkey_pressed = 0;
	return 0;
}
#endif

static int ft5x0x_read_data(struct ft5x0x_data *ft5x0x)
{
	int ret = -1;
	int i = 0;
	u16 x,y,px,py;
	u8 buf[64] = {0}, id;
	struct ts_event *event = &ft5x0x->event;

	if(ft5x0x->nt == 10)
		ret = ft5x0x_i2c_rxdata(buf, 64);
	else if(ft5x0x->nt == 5)
		ret = ft5x0x_i2c_rxdata(buf, 31);

	if (ret <= 0) {
		dbg_err("read_data i2c_rxdata failed: %d\n", ret);
		return ret;
	}

	memset(event, 0, sizeof(struct ts_event));
	//event->tpoint = buf[2] & 0x03;// 0000 0011
	//event->tpoint = buf[2] & 0x07;// 000 0111
	event->tpoint = buf[2]&0x0F;
	if (event->tpoint == 0) {
		ft5x0x_penup(ft5x0x);
		return 1; 
	}

	if (event->tpoint > ft5x0x->nt){
		dbg_err("tounch pointnum=%d > max:%d\n", event->tpoint,ft5x0x->nt);
		return -1;
	}

	for (i = 0; i < event->tpoint; i++){
		id = (buf[5+i*6] >>4) & 0x0F;//get track id
		if(ft5x0x->swap){
			px = (buf[3+i*6] & 0x0F)<<8 |buf[4+i*6];
			py = (buf[5+i*6] & 0x0F)<<8 |buf[6+i*6];
		}else{
			px = (buf[5+i*6] & 0x0F)<<8 |buf[6+i*6];
			py = (buf[3+i*6] & 0x0F)<<8 |buf[4+i*6];
		}

		x = px;
		y = py;

		if(ft5x0x->xch)    
			x = ft5x0x->reslx - px;

		if(ft5x0x->ych)
			y = ft5x0x->resly - py;

		if(ft5x0x->dbg) printk("F%d: Tid=%d,px=%d,py=%d; x=%d,y=%d\n", i, id, px, py, x, y);

#ifdef TOUCH_KEY
		if(ft5x0x->tskey_used && event->tpoint==1) {
			if(ft5x0x_read_tskey(ft5x0x,px,py) > 0) return -1;
		}
#endif        
		event->x[i] = x;
		event->y[i] = y;
		event->tid[i] = id;

	}

	return 0;
}

static void ft5x0x_report(struct ft5x0x_data *ft5x0x)
{
	int i = 0;
	struct ts_event *event = &ft5x0x->event;

	for (i = 0; i < event->tpoint; i++){
		input_report_abs(ft5x0x->input_dev, ABS_MT_TRACKING_ID, event->tid[i]);
		input_report_abs(ft5x0x->input_dev, ABS_MT_POSITION_X, event->x[i]);
		input_report_abs(ft5x0x->input_dev, ABS_MT_POSITION_Y, event->y[i]);
		input_mt_sync(ft5x0x->input_dev);		
	}
	input_sync(ft5x0x->input_dev);

	return;
}

static void ft5x0x_read_work(struct work_struct *work)
{
	int ret = -1;
	struct ft5x0x_data *ft5x0x = container_of(work, struct ft5x0x_data, read_work);

	mutex_lock(&ft5x0x->ts_mutex);
	ret = ft5x0x_read_data(ft5x0x);

	if (ret == 0) ft5x0x_report(ft5x0x);

	wmt_gpio_unmask_irq(ft5x0x->irqgpio);
	mutex_unlock(&ft5x0x->ts_mutex);

	return;
}

static irqreturn_t ft5x0x_interrupt(int irq, void *dev)
{
	struct ft5x0x_data *ft5x0x = dev;

	if (gpio_irqstatus(ft5x0x->irqgpio))
	{
		wmt_gpio_ack_irq(ft5x0x->irqgpio);
		if (is_gpio_irqenable(ft5x0x->irqgpio))
		{
			wmt_gpio_mask_irq(ft5x0x->irqgpio);
#ifdef CONFIG_HAS_EARLYSUSPEND
			if(!ft5x0x->earlysus) queue_work(ft5x0x->workqueue, &ft5x0x->read_work);
#else
			queue_work(ft5x0x->workqueue, &ft5x0x->read_work);
#endif

		}
		return IRQ_HANDLED;
	}
	return IRQ_NONE;
}

static void ft5x0x_reset(struct ft5x0x_data *ft5x0x)
{
	gpio_set_value(ft5x0x->rstgpio, 1);
	mdelay(5);
	gpio_set_value(ft5x0x->rstgpio, 0);
	mdelay(20);
	gpio_set_value(ft5x0x->rstgpio, 1);
	mdelay(5);

	return;
}

#ifdef CONFIG_HAS_EARLYSUSPEND
static void ft5x0x_early_suspend(struct early_suspend *handler)
{
	struct ft5x0x_data *ft5x0x = container_of(handler, struct ft5x0x_data, early_suspend);
	ft5x0x->earlysus = 1;
	wmt_gpio_mask_irq(ft5x0x->irqgpio);
	return;
}

static void ft5x0x_late_resume(struct early_suspend *handler)
{
	struct ft5x0x_data *ft5x0x = container_of(handler, struct ft5x0x_data, early_suspend);

	ft5x0x_reset(ft5x0x);
	ft5x0x->earlysus = 0;

	wmt_gpio_set_irq_type(ft5x0x->irqgpio, IRQ_TYPE_EDGE_FALLING);
	wmt_gpio_unmask_irq(ft5x0x->irqgpio);

	return;
}
#endif  //CONFIG_HAS_EARLYSUSPEND

#ifdef CONFIG_PM
static int ft5x0x_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct ft5x0x_data *ft5x0x = dev_get_drvdata(&pdev->dev);
	ft5x0x->earlysus = 1;
	wmt_gpio_mask_irq(ft5x0x->irqgpio);
	return 0;
}

static int ft5x0x_resume(struct platform_device *pdev)
{
	struct ft5x0x_data *ft5x0x = dev_get_drvdata(&pdev->dev);
	ft5x0x_reset(ft5x0x);
	ft5x0x->earlysus = 0;

	if (ft5x0x->load_cfg) {
		msleep(350);
		ft5402_Init_IC_Param(ft5x0x->client);
		//msleep(50);
		ft5402_get_ic_param(ft5x0x->client);
	}
	wmt_gpio_set_irq_type(ft5x0x->irqgpio, IRQ_TYPE_EDGE_FALLING);
	wmt_gpio_unmask_irq(ft5x0x->irqgpio);
	return 0;
}

#else
#define ft5x0x_suspend NULL
#define ft5x0x_resume NULL
#endif

static ssize_t cat_dbg(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "dbg \n");
}

static ssize_t echo_dbg(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned int dbg = simple_strtoul(buf, NULL, 10);
	struct ft5x0x_data *ft5x0x = pContext;

	if(dbg){
		ft5x0x->dbg = 1;
	}else{
		ft5x0x->dbg = 0;
	}

	return count;
}
static DEVICE_ATTR(dbg, S_IRUGO | S_IWUSR, cat_dbg, echo_dbg);

static ssize_t cat_clb(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "calibrate \n");
}

static ssize_t echo_clb(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned int cal = simple_strtoul(buf, NULL, 10);

	if(cal){
		if(ft5x0x_auto_clb()) printk("Calibrate Failed.\n");
	}else{
		printk("calibrate --echo 1 >clb.\n");
	}

	return count;
}

static DEVICE_ATTR(clb, S_IRUGO | S_IWUSR, cat_clb, echo_clb);

static ssize_t cat_fupg(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "fupg \n");
}

static ssize_t echo_fupg(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	struct ft5x0x_data *ft5x0x = pContext;
	unsigned int upg = simple_strtoul(buf, NULL, 10);

	wmt_gpio_mask_irq(ft5x0x->irqgpio);
	if(upg){
		if(ft5x0x_upg_fw_bin(ft5x0x, 0)) printk("Upgrade Failed.\n");
	}else{
		printk("upgrade --echo 1 > fupg.\n");
	}
	wmt_gpio_unmask_irq(ft5x0x->irqgpio);

	return count;
}
static DEVICE_ATTR(fupg, S_IRUGO | S_IWUSR, cat_fupg, echo_fupg);


static ssize_t cat_fver(struct device *dev, struct device_attribute *attr, char *buf)
{
	int fw_ver = ft5x0x_read_fw_ver();
	return sprintf(buf, "firmware version:0x%02x \n",fw_ver);
}

static ssize_t echo_fver(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{       
	return count;
}
static DEVICE_ATTR(fver, S_IRUGO | S_IWUSR, cat_fver, echo_fver);

static ssize_t cat_addr(struct device *dev, struct device_attribute *attr, char *buf)
{
	int ret;
	u8 addrs[32];
	int cnt=0;
	struct i2c_msg msg[2];
	struct ft5x0x_data *ft5x0x = pContext;
	u8 ver[1]= {0xa6};

	ft5x0x->addr = 1;

	msg[0].addr = ft5x0x->addr;
	msg[0].flags = 0;
	msg[0].len = 1;
	msg[0].buf = ver;

	msg[1].addr = ft5x0x->addr;
	msg[1].flags = I2C_M_RD;
	msg[1].len = 1;
	msg[1].buf = ver;

	while(ft5x0x->addr < 0x80){
		ret = i2c_transfer(ft5x0x->client->adapter, msg, 2);
		if(ret == 2) sprintf(&addrs[5*cnt++], " 0x%02x",ft5x0x->addr);

		ft5x0x->addr++;
		msg[0].addr = msg[1].addr = ft5x0x->addr;   
	}

	return sprintf(buf, "i2c addr:%s\n",addrs);
}

static ssize_t echo_addr(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{  
	unsigned int addr;
	struct ft5x0x_data *ft5x0x = pContext;

	sscanf(buf,"%x", &addr);
	ft5x0x->addr = addr;

	return count;
}
static DEVICE_ATTR(addr, S_IRUGO | S_IWUSR, cat_addr, echo_addr);

static struct attribute *ft5x0x_attributes[] = {
	&dev_attr_clb.attr,
	&dev_attr_fupg.attr,
	&dev_attr_fver.attr,
	&dev_attr_dbg.attr,
	&dev_attr_addr.attr,
	NULL
};

static const struct attribute_group ft5x0x_group = {
	.attrs = ft5x0x_attributes,
};

static int ft5x0x_sysfs_create_group(struct ft5x0x_data *ft5x0x, const struct attribute_group *group)
{
	int err;

	ft5x0x->kobj = kobject_create_and_add("wmtts", NULL) ;
	if(!ft5x0x->kobj){
		dbg_err("kobj create failed.\n");
		return -ENOMEM;
	}

	/* Register sysfs hooks */
	err = sysfs_create_group(ft5x0x->kobj, group);
	if (err < 0){
		kobject_del(ft5x0x->kobj);
		dbg_err("Create sysfs group failed!\n");
		return -ENOMEM;
	}

	return 0;
}

static void ft5x0x_sysfs_remove_group(struct ft5x0x_data *ft5x0x, const struct attribute_group *group)
{
	sysfs_remove_group(ft5x0x->kobj, group);
	kobject_del(ft5x0x->kobj);  
	return;
}

static int ft5x0x_probe(struct platform_device *pdev)
{
	int i,err = 0;
	u8 value = 0;
	u8 cfg_name[32];
	struct ft5x0x_data *ft5x0x = platform_get_drvdata( pdev);

	ft5x0x->client = l_client;
	INIT_WORK(&ft5x0x->read_work, ft5x0x_read_work);
	mutex_init(&ft5x0x->ts_mutex);

	ft5x0x->workqueue = create_singlethread_workqueue(ft5x0x->name);
	if (!ft5x0x->workqueue) {
		err = -ESRCH;
		goto exit_create_singlethread;
	}

	err = ft5x0x_sysfs_create_group(ft5x0x, &ft5x0x_group);
	if(err < 0){
		dbg("create sysfs group failed.\n");
		goto exit_create_group;
	}

	ft5x0x->input_dev = input_allocate_device();
	if (!ft5x0x->input_dev) {
		err = -ENOMEM;
		dbg("failed to allocate input device\n");
		goto exit_input_dev_alloc_failed;
	}

	ft5x0x->input_dev->name = ft5x0x->name;
	ft5x0x->input_dev->evbit[0] = BIT_MASK(EV_SYN) | BIT_MASK(EV_KEY) | BIT_MASK(EV_ABS);
	set_bit(INPUT_PROP_DIRECT, ft5x0x->input_dev->propbit);

	input_set_abs_params(ft5x0x->input_dev,
			ABS_MT_POSITION_X, 0, ft5x0x->reslx, 0, 0);
	input_set_abs_params(ft5x0x->input_dev,
			ABS_MT_POSITION_Y, 0, ft5x0x->resly, 0, 0);
	input_set_abs_params(ft5x0x->input_dev,
			ABS_MT_TRACKING_ID, 0, 20, 0, 0);
#ifdef TOUCH_KEY 	
	if(ft5x0x->tskey_used){
		for (i = 0; i <NUM_KEYS; i++)
			set_bit(keycodes[i], ft5x0x->input_dev->keybit);

		ft5x0x->input_dev->keycode = keycodes;
		ft5x0x->input_dev->keycodesize = sizeof(unsigned int);
		ft5x0x->input_dev->keycodemax = NUM_KEYS;
	}
#endif

	err = input_register_device(ft5x0x->input_dev);
	if (err) {
		dbg_err("ft5x0x_ts_probe: failed to register input device.\n");
		goto exit_input_register_device_failed;
	}

#ifdef CONFIG_HAS_EARLYSUSPEND
	ft5x0x->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
	ft5x0x->early_suspend.suspend = ft5x0x_early_suspend;
	ft5x0x->early_suspend.resume = ft5x0x_late_resume;
	register_early_suspend(&ft5x0x->early_suspend);
#endif

	if(ft5x0x->upg){
		if(ft5x0x_upg_fw_bin(ft5x0x, 1)) printk("Upgrade Failed.\n");
		else wmt_setsyspara("wmt.io.ts.upg","");
		ft5x0x->upg = 0x00;
	}

	if(request_irq(ft5x0x->irq, ft5x0x_interrupt, IRQF_SHARED, ft5x0x->name, ft5x0x) < 0){
		dbg_err("Could not allocate irq for ts_ft5x0x !\n");
		err = -1;
		goto exit_register_irq;
	}	

	{ // check if need to load config to IC or not
		err = ft5402_read_reg(ft5x0x->client, 0xa3, &value);
		if (err < 0)
			dbg_err("Read reg 0xa3 failed.\n");
		else
			printk("0xa3 reg = %d\n", value);
		if (value == 3)
			ft5x0x->load_cfg = 1;
		else
			ft5x0x->load_cfg = 0;
	}
	ft5x0x_reset(ft5x0x);

	if (ft5x0x->load_cfg) {
		msleep(350);  /*make sure CTP already finish startup process*/
		sprintf(cfg_name, "%s.ini", ft5x0x->cfg_name);
		printk("Config file name: %s\n", cfg_name);
		if (ft5402_Get_Param_From_Ini(cfg_name) >= 0)
			ft5402_Init_IC_Param(ft5x0x->client);
		else
			dbg_err("[FTS]-------Get ft5402 param from INI file failed\n");
		ft5402_get_ic_param(ft5x0x->client);
	}

	wmt_gpio_set_irq_type(ft5x0x->irqgpio, IRQ_TYPE_EDGE_FALLING);
	wmt_gpio_unmask_irq(ft5x0x->irqgpio);


	return 0;

exit_register_irq:
#ifdef CONFIG_HAS_EARLYSUSPEND
	unregister_early_suspend(&ft5x0x->early_suspend);
#endif
exit_input_register_device_failed:
	input_free_device(ft5x0x->input_dev);
exit_input_dev_alloc_failed:
	ft5x0x_sysfs_remove_group(ft5x0x, &ft5x0x_group);
exit_create_group:
	cancel_work_sync(&ft5x0x->read_work);
	destroy_workqueue(ft5x0x->workqueue);  
exit_create_singlethread:
	return err;
}

static int ft5x0x_remove(struct platform_device *pdev)
{
	struct ft5x0x_data *ft5x0x = platform_get_drvdata( pdev);

	cancel_work_sync(&ft5x0x->read_work);
	flush_workqueue(ft5x0x->workqueue);
	destroy_workqueue(ft5x0x->workqueue);

	free_irq(ft5x0x->irq, ft5x0x);
	wmt_gpio_mask_irq(ft5x0x->irqgpio);

#ifdef CONFIG_HAS_EARLYSUSPEND
	unregister_early_suspend(&ft5x0x->early_suspend);
#endif
	input_unregister_device(ft5x0x->input_dev);

	ft5x0x_sysfs_remove_group(ft5x0x, &ft5x0x_group);

	mutex_destroy(&ft5x0x->ts_mutex);
	dbg("remove...\n");
	return 0;
}

static void ft5x0x_release(struct device *device)
{
	return;
}

static struct platform_device ft5x0x_device = {
	.name  	    = DEV_FT5X0X,
	.id       	= 0,
	.dev    	= {.release = ft5x0x_release},
};

static struct platform_driver ft5x0x_driver = {
	.driver = {
		.name   	= DEV_FT5X0X,
		.owner	= THIS_MODULE,
	},
	.probe    = ft5x0x_probe,
	.remove   = ft5x0x_remove,
	.suspend  = ft5x0x_suspend,
	.resume   = ft5x0x_resume,
};

static int check_touch_env(struct ft5x0x_data *ft5x0x)
{
	int i,ret = 0;
	int len = 96;
	int Enable;
	char retval[96] = {0};
	char *p=NULL;
	char *s=NULL;

	// Get u-boot parameter
	ret = wmt_getsyspara("wmt.io.touch", retval, &len);
	if(ret){
		//printk("MST FT5x0x:Read wmt.io.touch Failed.\n");
		return -EIO;
	}
	sscanf(retval,"%d:",&Enable);
	//check touch enable
	if(Enable == 0){
		//printk("FT5x0x Touch Screen Is Disabled.\n");
		return -ENODEV;
	}

	p = strchr(retval,':');
	p++;
	if(strncmp(p,"ft5301",6)==0){//check touch ID
		ft5x0x->id = FT5301;
		ft5x0x->name = DEV_FT5301;
	}else if(strncmp(p,"ft5406",6)==0){
		ft5x0x->id = FT5406;
		ft5x0x->name = DEV_FT5406;
	}else if(strncmp(p,"ft5206",6)==0){
		ft5x0x->id = FT5206;
		ft5x0x->name = DEV_FT5206;
	}else if(strncmp(p,"ft5606",6)==0){
		ft5x0x->id = FT5606;
		ft5x0x->name = DEV_FT5606;
	}else if(strncmp(p,"ft5306",6)==0){
		ft5x0x->id = FT5306;
		ft5x0x->name = DEV_FT5306;
	}else if(strncmp(p,"ft5302",6)==0){
		ft5x0x->id = FT5302;
		ft5x0x->name = DEV_FT5302;
	}else if(strncmp(p,"ft5",3)==0)
	{
		ft5x0x->id = FT5X0X;
		ft5x0x->name = DEV_FT5X0X;    	
	}else{
		printk("FT5x0x touch disabled.\n");
		return -ENODEV;
	}

	s = strchr(p,':');
	strncpy(ft5x0x->cfg_name, p, s-p);

	p = s + 1;
	sscanf(p,"%d:%d:%d:%d:%d:%d:%d:%d:%x", &ft5x0x->irqgpio, &ft5x0x->reslx, &ft5x0x->resly, &ft5x0x->rstgpio, &ft5x0x->swap, &ft5x0x->xch, &ft5x0x->ych, &ft5x0x->nt, &ft5x0x->addr);

	ft5x0x->irq = IRQ_GPIO;
	printk("%s irqgpio=%d, reslx=%d, resly=%d, rstgpio=%d, swap=%d, xch=%d, ych=%d, nt=%d, addr=%x\n", ft5x0x->name,  ft5x0x->irqgpio, ft5x0x->reslx, ft5x0x->resly, ft5x0x->rstgpio, ft5x0x->swap, ft5x0x->xch, ft5x0x->ych, ft5x0x->nt, ft5x0x->addr);

	memset(retval,0x00,sizeof(retval));
	ret = wmt_getsyspara("wmt.io.ts.upg", retval, &len);
	if(!ret){
		ft5x0x->upg = 1;
		strncpy(ft5x0x->fw_name, retval, sizeof(ft5x0x->fw_name));
	}

#ifdef TOUCH_KEY
	memset(retval,0x00,sizeof(retval));
	ret = wmt_getsyspara("wmt.io.tskey", retval, &len);
	if(!ret){
		sscanf(retval,"%d:", &ft5x0x->nkeys);
		p = strchr(retval,':');
		p++;
		for(i=0; i < ft5x0x->nkeys; i++ ){
			sscanf(p,"%d:%d", &ft5x0x->tkey.ypos[i].y_lower, &ft5x0x->tkey.ypos[i].y_upper);
			p = strchr(p,':');
			p++;
			p = strchr(p,':');
			p++;
		}
		sscanf(p,"%d:%d:%d", &ft5x0x->tkey.axis, &ft5x0x->tkey.x_lower, &ft5x0x->tkey.x_upper);
		ft5x0x->tskey_used = 1;           
	}
#endif    
	return 0;
}

struct i2c_board_info ts_i2c_board_info = {
	.type          = TS_I2C_NAME,
	.flags         = 0x00,
	.addr          = FT5406_I2C_ADDR,
	.platform_data = NULL,
	.archdata      = NULL,
	.irq           = -1,
};

static int ts_i2c_register_device (void)
{
	struct i2c_board_info *ts_i2c_bi;
	struct i2c_adapter *adapter = NULL;
	//struct i2c_client *client   = NULL;

	//ts_i2c_board_info.addr =  FT5406_I2C_ADDR;
	ts_i2c_bi = &ts_i2c_board_info;
	adapter = i2c_get_adapter(FT5X0X_I2C_BUS);/*in bus 1*/

	if (NULL == adapter) {
		printk("can not get i2c adapter, client address error\n");
		return -1;
	}
	l_client = i2c_new_device(adapter, ts_i2c_bi);
	if (l_client == NULL) {
		printk("allocate i2c client failed\n");
		return -1;
	}
	i2c_put_adapter(adapter);
	return 0;
}

static void ts_i2c_unregister_device(void)
{
	if (l_client != NULL)
	{
		i2c_unregister_device(l_client);
		l_client = NULL;
	}
}

static int __init ft5x0x_init(void)
{
	int ret = -ENOMEM;
	struct ft5x0x_data *ft5x0x=NULL;

	if (ts_i2c_register_device()<0)
	{
		dbg("Error to run ts_i2c_register_device()!\n");
		return -1;
	}

	ft5x0x = kzalloc(sizeof(struct ft5x0x_data), GFP_KERNEL);
	if(!ft5x0x){
		dbg_err("mem alloc failed.\n");
		return -ENOMEM;
	}

	pContext = ft5x0x;
	ret = check_touch_env(ft5x0x);
	if(ret < 0)
		goto exit_free_mem;

	ret = gpio_request(ft5x0x->irqgpio, "ts_irq");
	if (ret < 0) {
		printk("gpio(%d) touchscreen irq request fail\n", ft5x0x->irqgpio);
		goto exit_free_mem;
	}
	wmt_gpio_setpull(ft5x0x->irqgpio, WMT_GPIO_PULL_UP);
	gpio_direction_input(ft5x0x->irqgpio);

	ret = gpio_request(ft5x0x->rstgpio, "ts_rst");
	if (ret < 0) {
		printk("gpio(%d) touchscreen reset request fail\n", ft5x0x->rstgpio);
		goto exit_free_irqgpio;
	}
	gpio_direction_output(ft5x0x->rstgpio, 1);


	ret = platform_device_register(&ft5x0x_device);
	if(ret){
		dbg_err("register platform drivver failed!\n");
		goto exit_free_gpio;
	}
	platform_set_drvdata(&ft5x0x_device, ft5x0x);

	ret = platform_driver_register(&ft5x0x_driver);
	if(ret){
		dbg_err("register platform device failed!\n");
		goto exit_unregister_pdev;
	}

	return ret;

exit_unregister_pdev:
	platform_device_unregister(&ft5x0x_device);
exit_free_gpio:
	gpio_free(ft5x0x->rstgpio);
exit_free_irqgpio:
	gpio_free(ft5x0x->irqgpio);
exit_free_mem:
	kfree(ft5x0x);
	pContext = NULL;
	return ret;
}

static void ft5x0x_exit(void)
{
	if(!pContext) return;

	gpio_free(pContext->rstgpio);
	gpio_free(pContext->irqgpio);
	platform_driver_unregister(&ft5x0x_driver);
	platform_device_unregister(&ft5x0x_device);
	kfree(pContext);
	ts_i2c_unregister_device();
	return;
}

late_initcall(ft5x0x_init);
module_exit(ft5x0x_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("FocalTech.Touch");
