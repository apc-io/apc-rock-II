/*++
 * Common interface for WonderMedia SoC hardware decoder drivers
 *
 * Copyright (c) 2008-2013  WonderMedia  Technologies, Inc.
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

#include "wmt-vd.h"

#define THE_MB_USER "WMT-VD"

#define DRIVER_NAME "wmt-vd"

#define VD_DEV_NAME "wmt-vd"

/*#define VD_DEBUG_DEF*/
/*#define VD_WORDY_DEF*/


#define VD_INFO(fmt, args...) \
	do {\
		printk(KERN_INFO "[wmt-vd] " fmt , ## args);\
	} while (0)

#define VD_WARN(fmt, args...) \
	do {\
		printk(KERN_WARNING "[wmt-vd] " fmt , ## args);\
	} while (0)

#define VD_ERROR(fmt, args...) \
	do {\
		printk(KERN_ERR "[wmt-vd] " fmt , ## args);\
	} while (0)

#ifdef VD_DEBUG_DEF
#define VD_DBG(fmt, args...) \
	do {\
		printk(KERN_DEBUG "[wmt-vd] %s: " fmt, __func__ , ## args);\
	} while (0)
#else
#define VD_DBG(fmt, args...)
#undef VD_WORDY_DEF
#endif

#ifdef VD_WORDY_DEF
#define VD_WDBG(fmt, args...)	VD_DBG(fmt, args...)
#else
#define VD_WDBG(fmt, args...)
#endif


static struct class *vd_class;
static int videodecoder_minor;
static int videodecoder_dev_nr = 1;
static struct cdev *videodecoder_cdev;

static struct videodecoder *decoders[VD_MAX] = {0};

static struct vd_resource vd_res = {
	.prdt         = { NULL, 0x0, MAX_INPUT_BUF_SIZE},
};

static DEFINE_SEMAPHORE(vd_sem);

static int wmt_vd_open(struct inode *inode, struct file *filp)
{
	int ret = -EINVAL;
	unsigned int idx = VD_MAX;

	down(&vd_sem);
	idx = iminor(inode);

	if (idx < VD_MAX && decoders[idx]) {
		struct videodecoder *vd = decoders[idx];

		if (vd && vd->fops.open) {
			if (idx == VD_JPEG)
				filp->private_data = (void *)&(vd_res.prdt);
			ret = vd->fops.open(inode, filp);
		}
		VD_DBG("decoder %s opened.\n", vd->name);
	}
	up(&vd_sem);

	return ret;
} /* End of wmt_vd_open() */

static int wmt_vd_release(struct inode *inode, struct file *filp)
{
	int ret = -EINVAL;
	unsigned int idx = VD_MAX;

	down(&vd_sem);
	idx = iminor(inode);

	if (idx < VD_MAX && decoders[idx]) {
		struct videodecoder *vd = decoders[idx];
		if (vd && vd->fops.release)
			ret = vd->fops.release(inode, filp);
		VD_DBG("decoder %s closed.\n", vd->name);
	}
	up(&vd_sem);

	return ret;
} /* End of wmt_vd_release() */

static long wmt_vd_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	int ret = -EINVAL;
	unsigned int idx = VD_MAX;
	struct inode *inode = filp->f_mapping->host;

	/* check type and number, if fail return ENOTTY */
	if ((_IOC_TYPE(cmd) != VD_IOC_MAGIC) || (_IOC_NR(cmd) > VD_IOC_MAXNR))
		return -ENOTTY;

	/* check argument area */
	if (_IOC_DIR(cmd) & _IOC_READ)
		ret = !access_ok(VERIFY_WRITE, (void __user *)arg,
						_IOC_SIZE(cmd));
	else if (_IOC_DIR(cmd) & _IOC_WRITE)
		ret = !access_ok(VERIFY_READ, (void __user *)arg,
						_IOC_SIZE(cmd));

	if (ret)
		return -EFAULT;

	down(&vd_sem);
	idx = iminor(inode);
	if (idx < VD_MAX && decoders[idx]) {
		struct videodecoder *vd = decoders[idx];

		if (vd && vd->fops.unlocked_ioctl)
			ret = vd->fops.unlocked_ioctl(filp, cmd, arg);
	}
	up(&vd_sem);

	return ret;
} /* End of wmt_vd_ioctl() */

static int wmt_vd_mmap(struct file *filp, struct vm_area_struct *vma)
{
	int ret = -EINVAL;
	unsigned int idx = VD_MAX;

	down(&vd_sem);
	if (filp && filp->f_dentry && filp->f_dentry->d_inode)
		idx = iminor(filp->f_dentry->d_inode);

	if (idx < VD_MAX && decoders[idx]) {
		struct videodecoder *vd = decoders[idx];
		if (vd && vd->fops.mmap)
			ret = vd->fops.mmap(filp, vma);
	}
	up(&vd_sem);

	return ret;
}

const struct file_operations videodecoder_fops = {
	.owner          = THIS_MODULE,
	.open           = wmt_vd_open,
	.release        = wmt_vd_release,
	.unlocked_ioctl = wmt_vd_ioctl,
	.mmap           = wmt_vd_mmap,
};

static int wmt_vd_probe(struct platform_device *dev)
{
	dev_t dev_no;
	int ret;

	dev_no = MKDEV(VD_MAJOR, videodecoder_minor);

	/* register char device */
	videodecoder_cdev = cdev_alloc();
	if (!videodecoder_cdev) {
		VD_ERROR("alloc dev error.\n");
		return -ENOMEM;
	}

	cdev_init(videodecoder_cdev, &videodecoder_fops);
	ret = cdev_add(videodecoder_cdev, dev_no, 1);

	if (ret) {
		VD_ERROR("reg char dev error(%d).\n", ret);
		cdev_del(videodecoder_cdev);
		return ret;
	}

	vd_res.prdt.virt = dma_alloc_coherent(NULL, vd_res.prdt.size,
				&vd_res.prdt.phys, GFP_KERNEL | GFP_DMA);
	if (!vd_res.prdt.virt) {
		VD_ERROR("allocate video PRDT buffer error.\n");
		cdev_del(videodecoder_cdev);
		vd_res.prdt.virt = NULL;
		vd_res.prdt.phys = 0;
		return -ENOMEM;
	}

	VD_DBG("prob /dev/%s major %d, minor %d, Resource:\n",
		DRIVER_NAME, VD_MAJOR, videodecoder_minor);

	VD_DBG("PRDT %p/%x, size %d KB\n",
		vd_res.prdt.virt, vd_res.prdt.phys, vd_res.prdt.size/1024);

	return ret;
}

static int wmt_vd_remove(struct platform_device *dev)
{
	unsigned int idx = 0;

	down(&vd_sem);
	while (idx < VD_MAX) {
		if (decoders[idx] && decoders[idx]->remove) {
			decoders[idx]->remove();
			decoders[idx] = NULL;
		}
		idx++;
	}
	up(&vd_sem);

	if (vd_res.prdt.virt)
		dma_free_coherent(NULL, MAX_INPUT_BUF_SIZE,
			vd_res.prdt.virt, vd_res.prdt.phys);

	return 0;
}

static int wmt_vd_suspend(struct platform_device *dev, pm_message_t state)
{
	int ret;
	unsigned int idx = 0;

	down(&vd_sem);
	while (idx < VD_MAX) {
		if (decoders[idx] && decoders[idx]->suspend) {
			ret = decoders[idx]->suspend(state);
			if (ret < 0)
				VD_WARN("vdec %s suspend fail. ret:%d\n",
					decoders[idx]->name, ret);
		}
		idx++;
	}
	up(&vd_sem);

	return 0;
}

static int wmt_vd_resume(struct platform_device *dev)
{
	int ret;
	unsigned int idx = 0;

	down(&vd_sem);
	while (idx < VD_MAX) {
		if (decoders[idx] && decoders[idx]->resume) {
			ret = decoders[idx]->resume();
			if (ret < 0) {
				VD_WARN("vdec %s resume fail. ret:%d\n",
					decoders[idx]->name, ret);
			}
		}
		idx++;
	}
	up(&vd_sem);

	return 0;
}

static void wmt_vd_elease(struct device *device)
{
	unsigned int idx = 0;

	down(&vd_sem);
	while (idx < VD_MAX) {
		if (decoders[idx] && decoders[idx]->remove) {
			decoders[idx]->remove();
			decoders[idx] = NULL;
		}
		idx++;
	}
	up(&vd_sem);

	if (vd_res.prdt.virt)
		dma_free_coherent(NULL, MAX_INPUT_BUF_SIZE,
			vd_res.prdt.virt, vd_res.prdt.phys);

	return;
}

static struct platform_driver videodecoder_driver = {
	.driver = {
	.name           = "wmt-vd",
	.bus            = &platform_bus_type,
	},
	.probe          = wmt_vd_probe,
	.remove         = wmt_vd_remove,
	.suspend        = wmt_vd_suspend,
	.resume         = wmt_vd_resume
};

static struct platform_device videodecoder_device = {
	.name           = "wmt-vd",
	.id             = 0,
	.dev            =   {
						.release =  wmt_vd_elease,
						},
	.num_resources  = 0,        /* ARRAY_SIZE(spi_resources), */
	.resource       = NULL,     /* spi_resources, */
};

#ifdef CONFIG_PROC_FS
static int wmt_vd_read_proc(
	char *page,
	char **start,
	off_t off,
	int count,
	int *eof,
	void *data)
{
	int size;
	unsigned int idx = 0;
	char *p;

	down(&vd_sem);
	p = page;
	p += sprintf(p, "***** video decoder information *****\n");

	while (idx < VD_MAX) {
		if (decoders[idx] && decoders[idx]->get_info) {
			size = decoders[idx]->get_info(p, start, off, count);
			count -= size;
			if (count <= 40)
				break;
			p += size;
		}
		idx++;
	}
	p += sprintf(p, "**************** end ****************\n");
	up(&vd_sem);

	*eof = 1;

	return p - page;
}
#endif

int wmt_vd_register(struct videodecoder *dec)
{
	int ret = 0;
	dev_t dev_no;

	if (!dec || dec->id >= VD_MAX) {
		VD_WARN("register invalid video decoder\n");
		return -1;
	}

	if (decoders[dec->id]) {
		VD_WARN("video decoder (ID=%d) exist.(E:%32s, N:%32s)\n",
			dec->id, decoders[dec->id]->name, dec->name);
		return -1;
	}

	dev_no = MKDEV(VD_MAJOR, dec->id);
	ret = register_chrdev_region(dev_no, videodecoder_dev_nr, dec->name);
	if (ret < 0) {
		VD_ERROR("can't get %s device minor %d\n", dec->name, dec->id);
		return ret;
	}

	dec->device = cdev_alloc();
	if (!dec->device) {
		unregister_chrdev_region(dev_no, videodecoder_dev_nr);
		VD_ERROR("alloc dev error\n");
		return -ENOMEM;
	}

	cdev_init(dec->device, &videodecoder_fops);
	ret = cdev_add(dec->device, dev_no, 1);
	if (ret) {
		VD_ERROR("reg char dev error(%d)\n", ret);
		unregister_chrdev_region(dev_no, videodecoder_dev_nr);
		cdev_del(dec->device);
		dec->device = NULL;
		return ret;
	}

	if (dec->irq_num && dec->isr) {
		ret = request_irq(dec->irq_num, dec->isr,
				IRQF_SHARED, dec->name, dec->isr_data);
		VD_INFO("%s Request IRQ %d %s.\n",
			dec->name, dec->irq_num, (ret < 0) ? "Fail " : "Ok");
		if (ret) {
			VD_ERROR("isr: 0x%p, isr_data: 0x%p  (ret: %d)\n",
				dec->isr, dec->isr_data, ret);
			goto EXIT_wmt_vd_register;
		}
	}

	if (!ret) {
		if (dec->setup)
			ret = dec->setup();
	}

	if (ret >= 0) {
		down(&vd_sem);
		decoders[dec->id] = dec;
		up(&vd_sem);
		VD_INFO("%s registered major %d minor %d\n",
			dec->name, VD_MAJOR, dec->id);

		/* let udev to handle /dev/ node */
		dec->vd_class = class_create(dec->fops.owner, dec->name);
		device_create(dec->vd_class, 0, dev_no, 0, "%s", dec->name);
	} else {
		VD_ERROR("%s register major %d minor %d fail\n",
			dec->name, VD_MAJOR, dec->id);
		free_irq(dec->irq_num, (void *)dec->isr_data);
		unregister_chrdev_region(dev_no, videodecoder_dev_nr);
		cdev_del(dec->device);
		dec->device = NULL;
	}
EXIT_wmt_vd_register:
	return ret;
}
EXPORT_SYMBOL(wmt_vd_register);

int wmt_vd_unregister(struct videodecoder *dec)
{
	int ret = 0;
	dev_t dev_no;

	if (!dec || dec->id >= VD_MAX || !dec->device) {
		VD_WARN("unregister invalid video decoder\n");
		return -1;
	}

	if (decoders[dec->id] != dec) {
		VD_WARN("unregiseter wrong video decoder. (E:%32s, R:%32s)\n",
			decoders[dec->id]->name, dec->name);
		return -1;
	}

	down(&vd_sem);
	decoders[dec->id] = NULL;
	up(&vd_sem);

	if (dec->remove)
		ret = dec->remove();

	dev_no = MKDEV(VD_MAJOR, dec->id);
	free_irq(dec->irq_num, (void *)dec->isr_data);

	/* let udev to handle /dev/ node */
	device_destroy(dec->vd_class, dev_no);
	class_destroy(dec->vd_class);

	unregister_chrdev_region(dev_no, videodecoder_dev_nr);
	cdev_del(dec->device);
	dec->device = NULL;

	return ret;
}
EXPORT_SYMBOL(wmt_vd_unregister);

static int __init videodecoder_init(void)
{
	int ret;
	dev_t dev_no;

	dev_no = MKDEV(VD_MAJOR, videodecoder_minor);
	ret = register_chrdev_region(dev_no, videodecoder_dev_nr, "wmt-vd");
	if (ret < 0) {
		VD_ERROR("can't get %s major %d\n", DRIVER_NAME, VD_MAJOR);
		return ret;
	}

#ifdef CONFIG_PROC_FS
	create_proc_read_entry("wmt-vd", 0, NULL, wmt_vd_read_proc, NULL);
	VD_DBG("create video decoder proc\n");
#endif

	ret = platform_driver_register(&videodecoder_driver);
	if (!ret) {
		ret = platform_device_register(&videodecoder_device);
		if (ret)
			platform_driver_unregister(&videodecoder_driver);
	}

	/* let udev to handle /dev/wmt-vd */
	vd_class = class_create(THIS_MODULE, VD_DEV_NAME);
	device_create(vd_class, NULL, dev_no, NULL, "%s", VD_DEV_NAME);

	VD_INFO("WonderMedia HW decoder driver inited\n");

	return ret;
}

void __exit videodecoder_exit(void)
{
	dev_t dev_no;

	platform_driver_unregister(&videodecoder_driver);
	platform_device_unregister(&videodecoder_device);
	dev_no = MKDEV(VD_MAJOR, videodecoder_minor);

	/* let udev to handle /dev/wmt-vd */
	device_destroy(vd_class, dev_no);
	class_destroy(vd_class);

	unregister_chrdev_region(dev_no, videodecoder_dev_nr);

	VD_INFO("WonderMedia HW decoder driver exit\n");

	return;
}

fs_initcall(videodecoder_init);
module_exit(videodecoder_exit);

MODULE_AUTHOR("WonderMedia Technologies, Inc.");
MODULE_DESCRIPTION("Video Codec device driver");
MODULE_LICENSE("GPL");

