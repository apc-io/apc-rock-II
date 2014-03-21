/*
 * arch/arm/mach-wmt/wmt_rfkill_bluetooth.c
 * Copyright (c) RubbitXiao RubbitXiao@wondermedia.com.cn
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of this archive for
 * more details.
 *
 *	    wonderMedia bluetooth "driver"
 *
 */

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/string.h>
#include <linux/ctype.h>
#include <linux/leds.h>
#include <linux/gpio.h>
#include <linux/rfkill.h>
#include <asm/io.h>//for readb/writeb
#include <mach/hardware.h>//WMT_MMAP_OFFSET
#include <linux/pwm.h> //for pwm
#include <mach/wmt_iomux.h>


#define DRV_NAME "wmt-bt"

static int pwren_num = -1;
static int active_level = -1;
static int gpio_int_num = -1;
static int src_32K =1;
enum LEVEL {
	LOW = 0,
	HIGH,
};
enum POWER_CTRL {
	POWER_DIS = 0,
	POWER_EN,
};

extern int wmt_getsyspara(char *varname, char *varval, int *varlen);
void pwm_set_enable(int no,int enable);
void pwm_set_freq(int no,unsigned int freq);
void pwm_set_level(int no,unsigned int level);
int bluetooth_power_ctrl(int gpio_num, int enable,int active_level);
//configure pwm1/2 pin
void config_pwm_pin(int pwm_num,int enable)
{
	int val;
	if(enable) {
		val = readb(0xd8110200+WMT_MMAP_OFFSET);
		val &= ~(1 << 7);
		writeb(val, 0xd8110200+WMT_MMAP_OFFSET);
	}else{
		val = readb(0xd8110200+WMT_MMAP_OFFSET);
		val |= (1 << 7);
		writeb(val, 0xd8110200+WMT_MMAP_OFFSET);
	}
}

static struct pwm_device * g_pwm =NULL;
DEFINE_SPINLOCK(pwm_lock);
static volatile int g_pwm_enable = 0x00;

static int enable_pwm_32KHz(int pwm_num,int enable)
{
	if(enable) {
		if(!g_pwm){
			g_pwm = pwm_request(pwm_num,"bluetooth 32KHz");
			if(!g_pwm){
				printk("can not request pwm1 for bluetooth mtk6622\n");
				return -ENOMEM;
			}	
		}
		//pwm_config(g_pwm, 15625, 31250);// configuration output 32KHZ
		pwm_config(g_pwm, 15258, 30517);// configuration output 32768HZ
		pwm_enable(g_pwm);
		config_pwm_pin(pwm_num,0x01);
		printk("enable 32khz output\n");
		mdelay(200);
	}else{
		pwm_disable(g_pwm);
		config_pwm_pin(pwm_num,0x00);
		printk("disable 32khz output\n");
	}
	return 0;
}

/*
* TODO: should atomis operation
*/
int enable_32Khz(int enable)
{
	int val;
	val = readb(0xd8110203+WMT_MMAP_OFFSET);
	if(enable)
	{		
		val |= (1 << 4);
	}else
	{
		val &= ~(1 << 4);
	}
	writeb(val, 0xd8110203+WMT_MMAP_OFFSET);
	return 0;
}


static void realease_pwm(void)
{
	if(g_pwm)
		pwm_free(g_pwm);
	g_pwm = NULL;
}


int pwm_32KHZ_control(int enable)
{
	unsigned long flags;
	spin_lock_irqsave(&pwm_lock, flags);
	if(enable){
		if(g_pwm_enable==0)
		{
			enable_pwm_32KHz(src_32K,1);
		}else{
		}
		g_pwm_enable++;
		
	}else{
		if(g_pwm_enable==0x00)
		{
			enable_pwm_32KHz(src_32K,0);
		}else if(g_pwm_enable > 0){
			g_pwm_enable--;
			if(g_pwm_enable==0x00)
			{
				enable_pwm_32KHz(src_32K,0);
			}
		}		
	}
	printk("enable:%d,g_pwm_enable:%d\n",enable,g_pwm_enable);
	spin_unlock_irqrestore(&pwm_lock, flags);
	
}
EXPORT_SYMBOL(pwm_32KHZ_control);

/* Bluetooth control */
static void bt_enable(int on)
{
	if (on) {
		printk("bt_enable on:%d\n",on);
		if(src_32K == 4){
			enable_32Khz(1);		
		}else{
			pwm_32KHZ_control(1);
		}
		mdelay(50);
		/* Power on the chip */
		//gpio_set_value(pwren_num, 1);,&
		bluetooth_power_ctrl(pwren_num, POWER_EN,active_level);
		mdelay(100);
	}
	else {
		printk("bt_enable on:%d\n",on);
		//gpio_set_value(pwren_num, 0);
		bluetooth_power_ctrl(pwren_num, POWER_DIS,active_level);
		mdelay(50);
		if(src_32K == 4){
		  //enable_32Khz(0);
		}else{
		//pwm_32KHZ_control(0);
		}
		mdelay(100);
	}
}

static int bt_set_block(void *data, bool blocked)
{
	bt_enable(!blocked);
	return 0;
}

static const struct rfkill_ops bt_rfkill_ops = {
	.set_block = bt_set_block,
};

//#include "test_skeleton.c"  //for test_skeleton_proc_init()


int bluetooth_power_ctrl(int gpio_num, int enable,int active_level)
{	
	if(enable){
		/*for power enable gpio pin*/
		if(active_level)
			gpio_direction_output(gpio_num, HIGH);
		else
			gpio_direction_output(gpio_num, LOW);
	}else{
		if(active_level)
			gpio_direction_output(gpio_num, LOW);
		else
			gpio_direction_output(gpio_num, HIGH);
	}
	
	return 0;
}


static int __devinit bt_probe(struct platform_device *pdev)
{
	struct rfkill *rfk;
	int ret = 0;

	char buf[256];
	int varlen = 256;	

	
	/*
	* wmt.bt.brcm formater as follows:
	*   pwren_num:active_level:gpio_int_num:src_32K
	*   for example:  151:1:0
	*   src_32K:
	*    1:pwm1
	*    2:pwm2
	*    3:pwm3
	*    4:susgpio3 cpu output 32KHz
	*
	*    for 8723bs_bt modules on SK_WAVE7_FT5316_BET-CT070040  gpio14 as bt power enable
	*		setenv wmt.bt.brcm 14:1:0:4
	*	 for ap6330 bt modules on 30KT   susgpio2 as bt power enable
	*		setenv wmt.bt.brcm 151:1:0:4
	*/
	if(wmt_getsyspara("wmt.bt.brcm",buf,&varlen) == 0)
	{
	  sscanf(buf,"%d:%d:%d:%d",&pwren_num,&active_level,&gpio_int_num,&src_32K);
		printk("use customized value:p%d,a%d,i%d,32K src:%d for wmt rfkill bluetooth\n",pwren_num,active_level,gpio_int_num,src_32K);	  
	} else {
		pwren_num = WMT_PIN_GP62_WAKEUP2;
		active_level = 0x01;
		gpio_int_num = -1;
		src_32K = 1;
		printk("use default value:p%d,a%d,i%d for wmt rfkill bluetooth\n",pwren_num,active_level,gpio_int_num);
	
	}

#if 1
	/*for power enable gpio pin*/
	ret = gpio_request(pwren_num, "rfkill bluetooth power/reset pin");
	if(ret < 0) {
			printk("request gpio:%x failed!!! for rfkill bluetooth\n",pwren_num);
			goto err_rfk_alloc;
	}else{
			printk("request gpio:%d for rfkill bluetooth success!!!\n",pwren_num);
	}
#endif
	bluetooth_power_ctrl(pwren_num, POWER_DIS, active_level);
	
#if 0
	/*for bt wakeup gpio pin*/
	ret = gpio_request(WMT_PIN_GP1_GPIO14, "bluetooth wakeup pin");
	if(ret < 0) {
			printk("request gpio:%x failed!!! bluetooth wakeup pin\n",WMT_PIN_GP1_GPIO14);
			goto err_rfk_alloc;
	}else{
			printk("request gpio:%d for bluetooth wakeup pin success!!!\n",WMT_PIN_GP1_GPIO14);
	}
	gpio_direction_output(WMT_PIN_GP1_GPIO14, HIGH);
#endif
	
	/* Configures BT serial port GPIOs */
	//this jobs is doned in uart serial drivers
#if 0
	//first disable bluetooth module during system booting stage.
	if(active_level)
		gpio_direction_output(pwren_num, LOW);
	else
		gpio_direction_output(pwren_num, HIGH);
	//enable_32Khz(0x00);	//disable susgpio3 ouput 32Khz, which lead to system hangup and can not enter suspend.
#endif
	
	if(!g_pwm && src_32K < 4){
		printk("begint to pwm request\n");
		g_pwm = pwm_request(src_32K,"bluetooth 32KHz");
		if(!g_pwm){
			printk("can not request pwm1 for bluetooth mtk6622\n");
			ret = -ENOMEM;
			goto err_rfk_alloc;
		}	
	}else{
		enable_32Khz(1);	
	}
	
	rfk = rfkill_alloc(DRV_NAME, &pdev->dev, RFKILL_TYPE_BLUETOOTH,
			&bt_rfkill_ops, NULL);
	if (!rfk) {
		ret = -ENOMEM;
		goto err_rfk_alloc;
	}

	ret = rfkill_register(rfk);
	if (ret)
		goto err_rfkill;

	platform_set_drvdata(pdev, rfk);
	//test_skeleton_proc_init();
	return 0;

err_rfkill:
	rfkill_destroy(rfk);
err_rfk_alloc:
	return ret;
}





static int bt_remove(struct platform_device *pdev)
{
	struct rfkill *rfk = platform_get_drvdata(pdev);

	platform_set_drvdata(pdev, NULL);

	if (rfk) {
		rfkill_unregister(rfk);
		rfkill_destroy(rfk);
	}
	rfk = NULL;
	bt_enable(0);
	gpio_free(pwren_num);
	//gpio_free(WMT_PIN_GP1_GPIO14);
	if(src_32K<4)
	  realease_pwm();
	//deinit_proc_init();
	return 0;
}


static struct platform_driver bt_driver = {
	.driver		= {
		.name	= DRV_NAME,
		.owner = THIS_MODULE,
	},
	.probe		= bt_probe,
	.remove		= bt_remove,
};

static void bt_device_release(struct device * dev) {}

static struct platform_device device_bluetooth = {
	.name             = DRV_NAME,
	.id               = -1,
	.dev = {
		.release = bt_device_release,
	}
};

static int __init bt_init(void)
{
	platform_device_register(&device_bluetooth);
	return platform_driver_register(&bt_driver);
}

static void __exit bt_exit(void)
{
	platform_driver_unregister(&bt_driver);
	platform_device_unregister(&device_bluetooth);
}

module_init(bt_init);
module_exit(bt_exit);

MODULE_AUTHOR("RubbitXiao RubbitXiao@wondermedia.com.cn");
MODULE_DESCRIPTION("Driver for the common wondermedia bluetooth chip");
MODULE_LICENSE("GPL");
