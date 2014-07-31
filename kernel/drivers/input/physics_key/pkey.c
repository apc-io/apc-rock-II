/*++ 
 * 
 * WonderMedia input remote control driver
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
  
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/device.h>
#include <linux/version.h>
#include <linux/input.h>
#include <linux/platform_device.h>
#include <asm/irq.h>
#include <mach/hardware.h>
#include <mach/wmt_pmc.h>
#include <linux/delay.h>
#include <linux/firmware.h>
#include <linux/workqueue.h>
#include <linux/slab.h>
#include <linux/gpio.h>


#include <linux/delay.h>
#include <mach/wmt_iomux.h>
#include <linux/irq.h>


static struct pkey_pdata {
	unsigned int gpio_no;
	
	struct input_dev *idev;	
	struct timer_list *p_key_timer;
	unsigned long timer_expires;
};

static int key_codes[2]={KEY_F1,KEY_BACK};

static inline void physics_key_timeout(unsigned long fcontext)
{
	struct pkey_pdata *p_key = (struct pkey_pdata *)fcontext;
	int keycode;
	
	keycode = __gpio_get_value(p_key->gpio_no)?key_codes[1]:key_codes[0];

	input_report_key(p_key->idev, keycode, 1); 
	input_sync(p_key->idev);
	mdelay(50);
	input_report_key(p_key->idev, keycode, 0); 
	input_sync(p_key->idev);

	p_key->timer_expires = 0;
}

static irqreturn_t physics_key_isr(int irq, void *dev_id)
{
	unsigned long expires;
	struct pkey_pdata *p_key = (struct pkey_pdata *)dev_id;
	
	if(gpio_irqstatus(p_key->gpio_no))
	{
		
		wmt_gpio_ack_irq(p_key->gpio_no);
		expires = jiffies + msecs_to_jiffies(50);
		if (!expires)
			expires = 1;
		
		if(!(p_key->timer_expires) || time_after(expires, p_key->timer_expires)){
			mod_timer(p_key->p_key_timer, expires);
			p_key->timer_expires = expires;
		}

		return IRQ_HANDLED;
	}

	return IRQ_NONE;
}

static int hw_init(struct platform_device *pdev)
{
	struct pkey_pdata *p_key = pdev->dev.platform_data;

	
	int ret = gpio_request(p_key->gpio_no,"physics_key");
	if(ret < 0) {
		printk(KERN_ERR"gpio request fail for physics_key\n");
		return ret;
	}

	gpio_direction_input(p_key->gpio_no);
	wmt_gpio_set_irq_type(p_key->gpio_no,IRQ_TYPE_EDGE_BOTH);
	wmt_gpio_unmask_irq(p_key->gpio_no);

	request_irq(IRQ_GPIO, physics_key_isr, IRQF_SHARED, "physics_key", p_key);

	return 0;
}

static int physics_key_probe(struct platform_device *pdev)
{
	int i;
	struct pkey_pdata *p_key = pdev->dev.platform_data;

	hw_init(pdev);

	if ((p_key->idev = input_allocate_device()) == NULL)
		return -ENOMEM;

	set_bit(EV_KEY, p_key->idev->evbit);
	for (i = 0; i < ARRAY_SIZE(key_codes); i++) {
		set_bit(key_codes[i], p_key->idev->keybit);
	}

	p_key->idev->name = "physics_key";
	p_key->idev->phys = "physics_key";
	input_register_device(p_key->idev);
	
	p_key->p_key_timer = (struct timer_list *)kzalloc(sizeof(struct timer_list), GFP_KERNEL);
	init_timer(p_key->p_key_timer);
	p_key->p_key_timer->data = (unsigned long)p_key;
	p_key->p_key_timer->function = physics_key_timeout;

	
	return 0;
}

static int physics_key_remove(struct platform_device *dev)
{
	struct pkey_pdata *p_key = dev->dev.platform_data;

	if(p_key->p_key_timer)
	{
		del_timer_sync(p_key->p_key_timer);
		free_irq(IRQ_GPIO, p_key);
		input_unregister_device(p_key->idev);
		input_free_device(p_key->idev);

		kfree(p_key);
	}

	return 0;
}

void pkey_pdevice_release(struct device *dev)
{
	
}

#ifdef CONFIG_PM
static int physics_key_suspend(struct platform_device *dev, pm_message_t state)
{
	struct pkey_pdata *p_key = dev->dev.platform_data;

	del_timer_sync(p_key->p_key_timer);

	wmt_gpio_mask_irq(p_key->gpio_no);
	
	return 0;
}

static int physics_key_resume(struct platform_device *dev)
{
	struct pkey_pdata *p_key = dev->dev.platform_data;

	wmt_gpio_set_irq_type(p_key->gpio_no,IRQ_TYPE_EDGE_BOTH);
	wmt_gpio_unmask_irq(p_key->gpio_no);
	
	return 0;
}
#else
#define physics_key_suspend NULL
#define physics_key_resume NULL
#endif




static struct platform_device pkey_pdevice = {
	.name           = "physics_key",
	.id             = 0,
	.dev            = {
    	.release = pkey_pdevice_release,
	},
};

static struct platform_driver pkey_driver = {
	.probe = physics_key_probe,
	.remove = physics_key_remove,
	.suspend = physics_key_suspend,
	.resume = physics_key_resume,
	
	.driver = {
		   .name = "physics_key",
		   },
};

extern int wmt_getsyspara(char *varname, unsigned char *varval, int *varlen);

static int __init physics_key_init(void)
{
	char buf[128];
	int ret = 0;
	int varlen;
	int ubootvar[1];

	
	memset(buf ,0, sizeof(buf));
	varlen = sizeof(buf);
	if (wmt_getsyspara("wmt.gpo.physics_switch", buf, &varlen)) {
		printk(KERN_ERR"wmt.gpo.physics_switch isn't set in u-boot env! -> Use default\n");
		return -1;
	}
	ret = sscanf(buf, "%d",
					   &ubootvar[0]
				       );

	struct pkey_pdata *p_key = (struct pkey_pdata *)kzalloc(sizeof(struct pkey_pdata), GFP_KERNEL);
	if (p_key == NULL)
		return -ENOMEM;
	
	p_key->gpio_no = ubootvar[0];
	
	pkey_pdevice.dev.platform_data = (void *)p_key;
	
	if (platform_device_register(&pkey_pdevice))
		return -1;
	
	ret = platform_driver_register(&pkey_driver);

	return ret;
}

static void __exit physics_key_exit(void)
{
	platform_driver_unregister(&pkey_driver);
	platform_device_unregister(&pkey_pdevice);
}

module_init(physics_key_init);
module_exit(physics_key_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("WonderMedia Technologies, Inc.");
MODULE_DESCRIPTION("WMT  driver");

