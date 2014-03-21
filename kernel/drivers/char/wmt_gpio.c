/*
 * linux/drivers/char/wmt_gpio.c
 * 
 * Copyright (c) 2008  WonderMedia Technologies, Inc.
 * 
 * This program is free software: you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software Foundation,
 * either version 2 of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE.  See the GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 * 
 * WonderMedia Technologies, Inc.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/errno.h>
#include <asm/uaccess.h>
#include <linux/kdev_t.h>
#include <linux/cdev.h>
#include <linux/major.h>
#include <linux/slab.h>
#include <linux/cdev.h>
#include <linux/sysctl.h>
#include <linux/gpio.h>

#define DRVNAME "wmtgpio"

struct user_gpio {
	int gpio;
	int type;
	int value;
};

struct kern_gpio {
	struct user_gpio user;
	struct list_head list;
};

struct gpio_dev {
	int		major;
	struct class	*class;
	struct device	*dev;
	struct mutex	lock;
	struct list_head gpio_list;
};
enum {
        GPIO_DIRECTION_OUT = 0,
        GPIO_DIRECTION_IN,
};
static struct gpio_dev *gpio_device;

static int gpio_dev_open(struct inode *inode, struct file *filp)
{
	filp->private_data = gpio_device;
	return 0;
}

static int gpio_dev_release(struct inode *inode, struct file *filp)
{
	return 0;
}

static int register_gpio(struct gpio_dev *gd, struct user_gpio *u)
{
	struct kern_gpio *k;
	struct list_head *node, *next;
	int rc;

	list_for_each_safe(node, next, &gd->gpio_list) {
		k = container_of(node, struct kern_gpio, list);
		if (k->user.gpio == u->gpio)
			goto requested;
	}

	rc = gpio_request(u->gpio, "user gpio");
	if (rc) {
		pr_err("gpio%d requested failed\n", u->gpio);
		return rc;
	}

	k = kzalloc(sizeof(*k), GFP_KERNEL);
	if (!k){
        gpio_free(u->gpio);
		return -ENOMEM;
    }
	list_add_tail(&k->list, &gd->gpio_list);

requested:
	k->user = *u;
	if (u->type == GPIO_DIRECTION_OUT) { //output
		gpio_direction_output(k->user.gpio, k->user.value);
		printk(KERN_DEBUG "gpio%d direction output %d\n",
			k->user.gpio, k->user.value);
		return 0;
	}
	
	if (u->type == GPIO_DIRECTION_IN) { //input
		rc = gpio_direction_input(k->user.gpio);
		rc = __gpio_get_value(k->user.gpio);
		
		return rc;
	}
	
	return -1;
}

static ssize_t gpio_dev_write(struct file *filp, const char __user *buf,
			      size_t count, loff_t *f_pos)
{
	struct gpio_dev *gd = filp->private_data;
	struct user_gpio ug;
	int rc = 0;

	if (count != sizeof(ug))
		return -EINVAL;

	if (copy_from_user(&ug, buf, count)) {
		return -EFAULT;
	}

	mutex_lock(&gd->lock);
	rc = register_gpio(gd, &ug);
	if (!rc) {
		rc = count;
	}
	mutex_unlock(&gd->lock);
	return rc;
}

static ssize_t gpio_dev_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{

	struct gpio_dev *gd = filp->private_data;
	struct user_gpio ug;
	int rc = 0;
	
//	printk("<<<%s\n", __func__);
	
	if (copy_from_user(&ug, buf, sizeof(ug))) {
		printk("<<<copy_from_user fail!\n");
		return -EFAULT;
	}
	printk("<<<gpio:%d, type:%d\n", ug.gpio, ug.type);
	if (ug.type != GPIO_DIRECTION_IN) {
		printk("<<<gpio type error !\n");
		return -EFAULT;
	}
	
	mutex_lock(&gd->lock);
	
	rc = register_gpio(gd, &ug);
	
	mutex_unlock(&gd->lock);
	
	printk("<<<rc:%d\n", rc);
	if (rc == 0) {
		copy_to_user(buf, "0:", sizeof("0:"));
	}
	else if (rc == 1) {
		copy_to_user(buf, "1:", sizeof("1:"));
	}
	else {
		copy_to_user(buf, "-1:", sizeof("-1:"));//error !!
	}
	return count;
	
}

static struct file_operations gpio_dev_fops = {
	.owner	= THIS_MODULE,
	.open	= gpio_dev_open,
	.write	= gpio_dev_write,
	.read	= gpio_dev_read, //add 2013-12-18
	.release = gpio_dev_release,
};

static int gpio_dev_probe(struct platform_device *pdev)
{
	struct gpio_dev *gd;
	dev_t dev_no;
	int ret;

	gd = kzalloc(sizeof(*gd), GFP_KERNEL);
	if (!gd) {
		dev_err(&pdev->dev, "no mem");
		return -ENOMEM;
	}

	gd->major = register_chrdev(0, DRVNAME, &gpio_dev_fops);
	if (gd->major < 0) {
		ret = -EFAULT;
		pr_err("get gpio_dev_major failed\n");
		goto err_register_chrdev;
	}

	gd->class = class_create(THIS_MODULE, DRVNAME);
	if (IS_ERR(gd->class)) {
		ret = PTR_ERR(gd->class);
		pr_err("Can't create class : %s !!\n",DRVNAME);
		goto err_class_create;
	}

	dev_no = MKDEV(gd->major, 0);
	gd->dev = device_create(gd->class, NULL, dev_no, NULL, DRVNAME);
	if (IS_ERR(gd->dev)) {
		ret = PTR_ERR(gd->dev);
		pr_err("Failed to create device %s !!!", DRVNAME);
		goto err_device_create;
	}

	INIT_LIST_HEAD(&gd->gpio_list);
	mutex_init(&gd->lock);
	gpio_device = gd;

	dev_info(&pdev->dev, "wmt gpio dev registered: %d\n", gd->major);
	return 0;

err_device_create:
	class_destroy(gd->class);
err_class_create:
	unregister_chrdev(gd->major, DRVNAME);
err_register_chrdev:
	kfree(gd);
	return ret;
}

static int gpio_dev_remove(struct platform_device *pdev)
{
	struct gpio_dev *gd = gpio_device;
	struct list_head *node, *next;
	struct kern_gpio *k;

	if (!gd)
		return 0;

	dev_info(&pdev->dev, "wmt gpio dev unregistered: %d\n", gd->major);

	device_destroy(gd->class, MKDEV(gd->major, 0));
	class_destroy(gd->class);
	unregister_chrdev(gd->major, DRVNAME);

	list_for_each_safe(node, next, &gd->gpio_list) {
		k = container_of(node, struct kern_gpio, list);
		gpio_free(k->user.gpio);
		kfree(k);
	}

	kfree(gd);
	gpio_device = NULL;
	return 0;
}

static int gpio_dev_suspend(struct platform_device *pdev, pm_message_t state)
{
	return 0;
}

static int gpio_dev_resume(struct platform_device *pdev)
{
	struct gpio_dev *gd = gpio_device;
	struct list_head *node, *next;
	struct kern_gpio *k;

	if (!gd)
		return 0;

	dev_info(&pdev->dev, "wmt gpio dev resume\n");

	list_for_each_safe(node, next, &gd->gpio_list) {
		k = container_of(node, struct kern_gpio, list);
		printk(KERN_DEBUG "%s: gpio%d, type:%d, value %d\n",
		       __func__, k->user.gpio, k->user.type, k->user.value); //debug 2013-12-18
		gpio_re_enabled(k->user.gpio);
		if (k->user.type == GPIO_DIRECTION_OUT) { //output
			gpio_direction_output(k->user.gpio, k->user.value);
		}
		else if (k->user.type == GPIO_DIRECTION_IN) { //input
			gpio_direction_input(k->user.gpio);
		}	
	}

	return 0;
}

static struct platform_driver gpio_dev_driver = {
	.driver = {
		.name = DRVNAME,
	},
	.probe		= gpio_dev_probe,
	.remove		= gpio_dev_remove,
	.suspend	= gpio_dev_suspend,
	.resume		= gpio_dev_resume,
};

static void gpio_dev_platform_release(struct device *device)
{
}

static struct platform_device gpio_dev_device = {
	.name	= DRVNAME,
	.dev	= {
		.release = gpio_dev_platform_release,
	},
};

static int gpio_dev_init(void)
{
	int ret;

	ret = platform_device_register(&gpio_dev_device);
	if (ret)
		return ret;

	ret = platform_driver_register(&gpio_dev_driver);
	if (ret) {
		platform_device_unregister(&gpio_dev_device);
		return ret;
	}

	return ret;
}

static void gpio_dev_exit(void)
{
	platform_driver_unregister(&gpio_dev_driver);
	platform_device_unregister(&gpio_dev_device);
	return;
}

module_init(gpio_dev_init);
module_exit(gpio_dev_exit);

MODULE_AUTHOR("WonderMedia Technologies, Inc.");
MODULE_DESCRIPTION("WMT [gpio device] driver");
MODULE_LICENSE("GPL");

