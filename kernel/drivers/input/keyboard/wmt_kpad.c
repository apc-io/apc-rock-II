/*++
linux/drivers/input/keyboard/wmt_kpad.c

Some descriptions of such software. Copyright (c) 2008  WonderMedia Technologies, Inc.

This program is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software Foundation,
either version 2 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License along with
this program.  If not, see <http://www.gnu.org/licenses/>.

WonderMedia Technologies, Inc.
10F, 529, Chung-Cheng Road, Hsin-Tien, Taipei 231, R.O.C.
--*/

#include <linux/module.h>
#include <linux/init.h> 
#include <linux/interrupt.h>
#include <linux/types.h>
#include <linux/input.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/cpufreq.h>
#include <linux/delay.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/errno.h>
#include <asm/mach-types.h>
#include <mach/hardware.h>
#include <linux/suspend.h>
#include <linux/gpio.h>
#include <mach/wmt_iomux.h>


/* Debug macros */
#if 0
#define DPRINTK(fmt, args...) printk(KERN_ALERT "[%s]: " fmt, __func__ , ## args)
#else
#define DPRINTK(fmt, args...)
#endif

//#define USE_HOME
#define wmt_kpad_timeout (HZ/50)

#define WMT_KPAD_FUNCTION_NUM 6


static unsigned int wmt_kpad_codes[WMT_KPAD_FUNCTION_NUM] = {  
	[0] = KEY_VOLUMEUP,
	[1] = KEY_VOLUMEDOWN,
	[2] = KEY_BACK,
	[3] = KEY_HOME,
	[4] = KEY_MENU,
	[5] = KEY_CAMERA,
};    


enum {
    KEY_ST_up,
    KEY_ST_down,
    KEY_ST_debounce,
};

static struct input_dev *kpad_dev;

int key_num = 0;

struct wmt_key{
    int gpio;
    int keycode;
    
    int status;
    int debounce;
    struct timer_list timer;
} ;
struct wmt_key gpio_key[5];

#ifdef CONFIG_CPU_FREQ
/*
 * Well, the debounce time is not very critical while zac2_clk
 * rescaling, but we still do it.
 */

/* kpad_clock_notifier()
 *
 * When changing the processor core clock frequency, it is necessary
 * to adjust the KPMIR register.
 *
 * Returns: 0 on success, -1 on error
 */
static int kpad_clock_notifier(struct notifier_block *nb, unsigned long event,
	void *data)
{
	return 0;
}

/*
 * Notify callback while issusing zac2_clk rescale.
 */
static struct notifier_block kpad_clock_nblock = {
	.notifier_call  = kpad_clock_notifier,
	.priority = 1
};
#endif

static int wmt_kpad_gpio_requst(void)
{
    int i,ret;
	DPRINTK("Start\n");
    for(i=0; i< key_num; i++){
        ret = gpio_request(gpio_key[i].gpio,"kpad");
        if(ret)
            goto exit;
    }
    
	DPRINTK("End\n");
    return ret;
exit:
    for(i=0; i<key_num; i++)
        gpio_free(gpio_key[i].gpio);
    return ret;
}

static int wmt_kpad_gpio_init(void)
{
    int i;
    for(i=0; i<key_num; i++){
        gpio_direction_input(gpio_key[i].gpio);
        wmt_gpio_setpull(gpio_key[i].gpio,WMT_GPIO_PULL_UP);
    }

    return 0;
}


static int wmt_kpad_gpio_free(void)
{
    int i;
    for(i=0; i<key_num; i++){
        gpio_free(gpio_key[i].gpio);
    }

    return 0;
}

static void wmt_kpad_poll(unsigned long fcontext)
{
    struct wmt_key *gpk = (struct wmt_key *)fcontext;
    
	//DPRINTK("Start\n");
	if (__gpio_get_value(gpk->gpio) == 0) {   /*Active Low*/
        if(gpk->status == KEY_ST_up){
            gpk->debounce = 5;
            gpk->status = KEY_ST_debounce;
            DPRINTK("vd down to debounce\n");
        }

        if(gpk->status == KEY_ST_debounce){
            if(--gpk->debounce == 0){
                gpk->status = KEY_ST_down;
                /* report volume down key down */
                input_report_key(kpad_dev, gpk->keycode, 1);                
                input_sync(kpad_dev);
                DPRINTK("WMT Volume up keep press\n");
            }
        }    
        //DPRINTK("vd level is low,status=%d\n",vu_status);
		
	} 
    else {/* Level High */
        if(gpk->status == KEY_ST_down){
            gpk->status = KEY_ST_up;
    		/*Volume down release*/
    		input_report_key(kpad_dev, gpk->keycode, 0); /*row4 key is release*/
    		input_sync(kpad_dev);
            DPRINTK("WMT_Volume down release key = %d \n", gpk->keycode);
        }

        if(gpk->status == KEY_ST_debounce){
            if(--gpk->debounce == 0){
                gpk->status = KEY_ST_up;
            }
        }
        
        //DPRINTK("vd level is high,status=%d\n",vu_status);
		
	}
    
    mod_timer(&gpk->timer, jiffies + wmt_kpad_timeout);
	//DPRINTK("End\n");

    return;
}

static int init_key_timer(void)
{
    int i;
    for(i=0; i<key_num;i++){
        init_timer(&gpio_key[i].timer);
	    gpio_key[i].timer.function = wmt_kpad_poll;
	    gpio_key[i].timer.data = (unsigned long)&gpio_key[i];
    }

    return 0;
}

static int start_key_timer(void)
{   int i;
    for(i=0;i<key_num;i++){
        gpio_key[i].status = KEY_ST_up;
        mod_timer(&gpio_key[i].timer, jiffies + HZ/10);
    }

    return 0;
}

static int del_key_timer(void)
{   int i;
    for(i=0;i<key_num;i++){
        gpio_key[i].status = KEY_ST_up;
        del_timer_sync(&gpio_key[i].timer);
    }

    return 0;
}
static int kpad_open(struct input_dev *dev)
{
	int ret = 0;
	DPRINTK("Start\n");
    
	/*init timer*/
    init_key_timer();	
    start_key_timer();
    wmt_kpad_gpio_init();
	DPRINTK("End2\n");

	return ret;
}

static void kpad_close(struct input_dev *dev)
{
	DPRINTK("Start\n");
    
    del_key_timer();
	/*
	 * Unregister input device driver
	 */
	input_unregister_device(dev);

	DPRINTK("End2\n");
}

static int wmt_kpad_probe(struct platform_device *pdev)
{
	int ret = 0;
	unsigned int i;
	DPRINTK("Start\n");
	kpad_dev = input_allocate_device();
	if (kpad_dev == NULL) {
		DPRINTK("End 1\n");
		return -1;
	}
	/*
	 * Simply check resources parameters.
	 */
	DPRINTK("pdev->num_resources = 0x%x\n",pdev->num_resources);
	if (pdev->num_resources < 0 || pdev->num_resources > 2) {
		ret = -ENODEV;
		goto kpad_probe_out;
	}
	
	/* Register an input event device. */
	kpad_dev->name = "keypad",
	kpad_dev->phys = "keypad",

	/*
	 *	Let kpad to implement key repeat.
	 */

	set_bit(EV_KEY, kpad_dev->evbit);

	for (i = 0; i < WMT_KPAD_FUNCTION_NUM; i++)
		set_bit(wmt_kpad_codes[i], kpad_dev->keybit);

	kpad_dev->keycode = wmt_kpad_codes;
	kpad_dev->keycodesize = sizeof(unsigned int);
	kpad_dev->keycodemax = WMT_KPAD_FUNCTION_NUM;

	/*
	 * For better view of /proc/bus/input/devices
	 */
	kpad_dev->id.bustype = 0;
	kpad_dev->id.vendor	= 0;
	kpad_dev->id.product = 0;
	kpad_dev->id.version = 0;

	input_register_device(kpad_dev);
	kpad_open(kpad_dev);
	DPRINTK("End2\n");
kpad_probe_out:

#ifndef CONFIG_SKIP_DRIVER_MSG
	printk(KERN_INFO "WMT keypad driver initialized: %s\n",
		  (ret == 0) ? "ok" : "failed");
#endif
	DPRINTK("End3\n");
	return ret;
}

static int wmt_kpad_remove(struct platform_device *pdev)
{
    int ret;
	DPRINTK("Start\n");
	kpad_close(kpad_dev);
    wmt_kpad_gpio_free();
	DPRINTK("End\n");
#ifdef CONFIG_CPU_FREQ
	ret = cpufreq_unregister_notifier(&kpad_clock_nblock, \
		CPUFREQ_TRANSITION_NOTIFIER);

	if (ret) {
		printk(KERN_ERR "Unable to unregister CPU frequency " \
			"change notifier (%d)\n", ret);
	}
#endif
	return 0;
}

static int wmt_kpad_suspend(struct platform_device *pdev, pm_message_t state)
{
	DPRINTK("Start\n");
    
	switch (state.event) {
	case PM_EVENT_SUSPEND:             
        del_key_timer();
		break;
	case PM_EVENT_FREEZE:
	case PM_EVENT_PRETHAW:
        
	default:
		break;
	}

	DPRINTK("End2\n");
	return 0;
}

static int wmt_kpad_resume(struct platform_device *pdev)
{  
	DPRINTK("Start\n");
	wmt_kpad_gpio_init();
    start_key_timer();
	DPRINTK("End\n");
	return 0;
}

static void wmt_kpad_release(struct device *dev)
{
    return ;
}

static struct platform_driver wmt_kpad_driver = {
	.driver.name = "wmt-kpad",
	.probe = &wmt_kpad_probe,
	.remove = &wmt_kpad_remove,
	.suspend = &wmt_kpad_suspend,
	.resume	= &wmt_kpad_resume
};

static struct resource wmt_kpad_resources[] = {
	[0] = {
		.start  = IRQ_GPIO,
		.end    = IRQ_GPIO,
		.flags  = IORESOURCE_IRQ,
	},
};

static struct platform_device wmt_kpad_device = {
	.name			= "wmt-kpad",
	.id				= 0,
	.num_resources  = ARRAY_SIZE(wmt_kpad_resources),
	.resource		= wmt_kpad_resources,
	.dev            = {
        .release    = wmt_kpad_release,
    }
};

extern int wmt_getsyspara(char *varname, unsigned char *varval, int *varlen);

static int __init kpad_init(void)
{
	int i,ret;
	int retval;
	unsigned char buf[80];
	int varlen = 80;
	char *varname = "wmt.io.kpad";
	char *p=NULL;
    int gpio,code;
	
	DPRINTK(KERN_ALERT "Start\n");
	retval = wmt_getsyspara(varname, buf, &varlen);
	if (retval == 0) {
		sscanf(buf,"%d:", &key_num);
		if (key_num <= 0)
			return -ENODEV;
        p = buf;
        for(i=0; i<key_num && p!=NULL; i++){
            p = strchr(p,':');
            p++;
            sscanf(p,"[%d,%d]",&gpio,&code);
            gpio_key[i].gpio = gpio;
            gpio_key[i].keycode = code;
            printk("gpio=%d,code=%d\n",gpio,gpio_key[i].keycode);
        }

	} else {
		printk("##Warning: \"wmt.io.kpad\" not find\n");
        return -EIO;
	}
    ret = wmt_kpad_gpio_requst();
    if(ret){
        printk("##Warning:Request gpio failed.\n");
        return ret;
    }
	
#ifdef CONFIG_CPU_FREQ
	ret = cpufreq_register_notifier(&kpad_clock_nblock, \
		CPUFREQ_TRANSITION_NOTIFIER);

	if (ret) {
		printk(KERN_ERR "Unable to register CPU frequency " \
			"change notifier (%d)\n", ret);
	}
#endif
	ret = platform_device_register(&wmt_kpad_device);
	if (ret != 0) {
		DPRINTK("End1 ret = %x\n",ret);
		return -ENODEV;
	}

	ret = platform_driver_register(&wmt_kpad_driver);
	DPRINTK("End2 ret = %x\n",ret);
	return ret;
}

static void __exit kpad_exit(void)
{
	DPRINTK("Start\n");
	platform_driver_unregister(&wmt_kpad_driver);
	platform_device_unregister(&wmt_kpad_device);
	DPRINTK("End\n");
}

module_init(kpad_init);
module_exit(kpad_exit);

MODULE_AUTHOR("WonderMedia Technologies, Inc.");
MODULE_DESCRIPTION("WMT [generic keypad] driver");
MODULE_LICENSE("GPL");
