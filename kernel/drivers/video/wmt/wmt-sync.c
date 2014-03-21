/*++
 * linux/drivers/video/wmt/wmt-sync.c
 * WonderMedia video post processor (VPP) driver
 *
 * Copyright c 2013  WonderMedia  Technologies, Inc.
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

#define DEV_SYNC_C

#include <linux/init.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/errno.h>
#include <asm/uaccess.h>
#include <linux/kdev_t.h>
#include <linux/cdev.h>
#include <linux/dma-mapping.h>
#include <linux/major.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/spinlock.h>
#include <mach/irqs.h>
#include <mach/hardware.h>
#include <linux/sched.h>
#include <linux/wmt-mb.h>
#include <linux/wmt-se.h>
#include <linux/poll.h>

#undef DEBUG
/* #define DEBUG */
/* #define DEBUG_DETAIL */
#include "vpp.h"

#define DEVICE_NAME "wmt-sync"

static DEFINE_SEMAPHORE(wmt_sync_sem);
static DECLARE_WAIT_QUEUE_HEAD(wmt_sync_wait);
static struct class *wmt_sync_class;
static int wmt_sync_major;
unsigned int wmt_vsync_flag;

int wmt_sync_set_vsync(void *arg)
{
	wmt_vsync_flag++;
	wake_up_interruptible(&wmt_sync_wait);
	vpp_mb_irqproc_sync(0);
	return 0;
}

static int wmt_sync_open(
	struct inode *inode,
	struct file *filp
)
{
	int ret = 0;
	vpp_int_t type;

	DBG_MSG("\n");

	down(&wmt_sync_sem);
	if (g_vpp.dual_display) {
	type = (vout_info[0].govr_mod == VPP_MOD_GOVRH) ?
			VPP_INT_GOVRH_VBIS : VPP_INT_GOVRH2_VBIS;
	} else {
		type = VPP_INT_GOVRH2_VBIS; /* HDMI should slow down */
	}
	vpp_irqproc_work(type, wmt_sync_set_vsync, 0, 0, 0);
	up(&wmt_sync_sem);
	return ret;
} /* End of wmt_sync_open() */

static int wmt_sync_release(
	struct inode *inode,
	struct file *filp
)
{
	int ret = 0;
	vpp_int_t type;

	DBG_MSG("\n");

	down(&wmt_sync_sem);
	type = (vout_info[0].govr_mod == VPP_MOD_GOVRH) ?
			VPP_INT_GOVRH_VBIS : VPP_INT_GOVRH2_VBIS;
	vpp_irqproc_del_work(type, wmt_sync_set_vsync);
	up(&wmt_sync_sem);
	return ret;
} /* End of wmt_sync_release() */

static long wmt_sync_ioctl(struct file *filp, unsigned int cmd,
						unsigned long arg)
{
	int ret = -EINVAL;

	DBG_MSG("0x%x,0x%x\n", cmd, arg);

	/* check type and number, if fail return ENOTTY */
	if (_IOC_TYPE(cmd) != WMT_CEC_IOC_MAGIC)
		return -ENOTTY;
	if (_IOC_NR(cmd) > WMT_CEC_IOC_MAXNR)
		return -ENOTTY;

	/* check argument area */
	if (_IOC_DIR(cmd) & _IOC_READ)
		ret = !access_ok(VERIFY_WRITE,
				(void __user *) arg, _IOC_SIZE(cmd));
	else if (_IOC_DIR(cmd) & _IOC_WRITE)
		ret = !access_ok(VERIFY_READ,
				(void __user *) arg, _IOC_SIZE(cmd));
	else
		ret = 0;

	if (ret)
		return -EFAULT;

	down(&wmt_sync_sem);
	switch (cmd) {
	default:
		DBG_ERR("*W* cmd 0x%x\n", cmd);
		break;
	}
	up(&wmt_sync_sem);
	return ret;
} /* End of wmt_sync_ioctl() */

static ssize_t wmt_sync_read(
	struct file *filp,
	char __user *buf,
	size_t count,
	loff_t *f_pos
)
{
	unsigned long data;
	ssize_t retval = 0;

	down(&wmt_sync_sem);
	if (filp->f_flags & O_NONBLOCK && !wmt_vsync_flag) {
		retval = -EAGAIN;
		goto read_end;
	}
	data = xchg(&wmt_vsync_flag, 0);
	retval = wait_event_interruptible(wmt_sync_wait, data);
	if (retval == 0)
		retval = put_user(data, (unsigned int __user *)buf);
read_end:	
	up(&wmt_sync_sem);
	return retval;
} /* wmt_sync_read */

static ssize_t wmt_sync_write(
	struct file *filp,
	const char __user *buf,
	size_t count,
	loff_t *f_pos
)
{
	ssize_t ret = 0;
	down(&wmt_sync_sem);
	/* TODO */
	up(&wmt_sync_sem);
	return ret;
} /* End of wmt_sync_write() */

static unsigned int wmt_sync_poll(
	struct file *filp,
	struct poll_table_struct *wait
)
{
	unsigned int mask = 0;

	down(&wmt_sync_sem);
	poll_wait(filp, &wmt_sync_wait, wait);
	if (wmt_vsync_flag != 0)
		mask |= POLLIN | POLLRDNORM;
	up(&wmt_sync_sem);
	return mask;
}

const struct file_operations wmt_sync_fops = {
	.owner          = THIS_MODULE,
	.open           = wmt_sync_open,
	.release        = wmt_sync_release,
	.read           = wmt_sync_read,
	.write          = wmt_sync_write,
	.unlocked_ioctl = wmt_sync_ioctl,
	.poll           = wmt_sync_poll,
};

static int wmt_sync_probe
(
	struct platform_device *dev /*!<; // a pointer to struct device */
)
{
	return 0;
} /* End of wmt_sync_probe */

static int wmt_sync_remove
(
	struct platform_device *dev /*!<; // a pointer to struct device */
)
{
	return 0;
} /* End of wmt_sync_remove */

#ifdef CONFIG_PM
static int wmt_sync_suspend
(
	struct platform_device *pDev,	/*!<; // a pointer to struct device */
	pm_message_t state		/*!<; // suspend state */
)
{
	DPRINT("Enter wmt_sync_suspend\n");
	return 0;
} /* End of wmt_sync_suspend */

static int wmt_sync_resume
(
	struct platform_device *pDev	/*!<; // a pointer to struct device */
)
{
	DPRINT("Enter wmt_sync_resume\n");
	return 0;
} /* End of wmt_sync_resume */
#else
#define wmt_sync_suspend NULL
#define wmt_sync_resume NULL
#endif

/***************************************************************************
	device driver struct define
****************************************************************************/
static struct platform_driver wmt_sync_driver = {
	.driver.name    = DEVICE_NAME, /* equal to platform device name. */
/*	.bus            = &platform_bus_type, */
	.probe          = wmt_sync_probe,
	.remove         = wmt_sync_remove,
	.suspend        = wmt_sync_suspend,
	.resume         = wmt_sync_resume,
};

static void wmt_sync_platform_release(struct device *device)
{
} /* End of wmt_sync_platform_release() */

/***************************************************************************
	platform device struct define
****************************************************************************/
/* static u64 wmt_sync_dma_mask = 0xffffffffUL; */
static struct platform_device wmt_sync_device = {
	.name   = DEVICE_NAME,
	.id     = 0,
	.dev    = {
		.release = wmt_sync_platform_release,
#if 0
		.dma_mask = &wmt_sync_dma_mask,
		.coherent_dma_mask = ~0,
#endif
	},
	.num_resources  = 0,	/* ARRAY_SIZE(cipher_resources), */
	.resource       = NULL,	/* cipher_resources, */
};

static int __init wmt_sync_init(void)
{
	int ret;
	char buf[100];
	int varlen = 100;
	unsigned int sync_enable = 1; /* default enable */
	dev_t dev_no;

	if (wmt_getsyspara("wmt.display.sync", buf, &varlen) == 0)
		vpp_parse_param(buf, &sync_enable, 1, 0);

	if (sync_enable == 0)
		return 0;

	wmt_sync_major = register_chrdev(0, DEVICE_NAME, &wmt_sync_fops);
	if (wmt_sync_major < 0) {
		DBG_ERR("get gpio_dev_major failed\n");
		return -EFAULT;
	}

	wmt_sync_class = class_create(THIS_MODULE, DEVICE_NAME);
	if (IS_ERR(wmt_sync_class)) {
		ret = PTR_ERR(wmt_sync_class);
		DBG_ERR("Can't create class : %s !!\n", DEVICE_NAME);
		return ret;
	}

	dev_no = MKDEV(wmt_sync_major, 0);
	device_create(wmt_sync_class, NULL, dev_no, NULL, DEVICE_NAME);
	ret = platform_device_register(&wmt_sync_device);
	if (ret != 0)
		return -ENODEV;

	ret = platform_driver_register(&wmt_sync_driver);
	if (ret != 0) {
		platform_device_unregister(&wmt_sync_device);
		return -ENODEV;
	}
	return 0;
}

static void __exit wmt_sync_exit(void)
{
	dev_t dev_no;

	DBG_MSG("\n");

	platform_driver_unregister(&wmt_sync_driver);
	platform_device_unregister(&wmt_sync_device);
	dev_no = MKDEV(wmt_sync_major, 0);
	device_destroy(wmt_sync_class, dev_no);
	class_destroy(wmt_sync_class);
	unregister_chrdev(wmt_sync_major, DEVICE_NAME);
	return;
}

module_init(wmt_sync_init);
module_exit(wmt_sync_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("WMT SYNC driver");
MODULE_AUTHOR("WMT TECH");
MODULE_VERSION("1.0.0");

