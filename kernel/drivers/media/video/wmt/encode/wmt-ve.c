/*++
 * Common interface for WonderMedia SoC hardware encoder drivers
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
#include "wmt-ve.h"

#define THE_MB_USER	"WMT-VE"

#define DRIVER_NAME "wmt-ve"

/*#define VE_DEBUG*/
/*#define VE_WORDY*/


#define VE_INFO(fmt, args...) \
	do {\
		printk(KERN_INFO "[wmt-ve] " fmt , ## args);\
	} while (0)

#define VE_WARN(fmt, args...) \
	do {\
		printk(KERN_WARNING "[wmt-ve] " fmt , ## args);\
	} while (0)

#define VE_ERR(fmt, args...) \
	do {\
		printk(KERN_ERR "[wmt-ve] " fmt , ## args);\
	} while (0)

#ifdef VE_DEBUG
#define VE_DBG(fmt, args...) \
	do {\
		printk(KERN_DEBUG "[wmt-ve] %s: " fmt, __func__ , ## args);\
	} while (0)
#else
#define VE_DBG(fmt, args...)
#undef VE_WORDY
#endif

#ifdef VE_WORDY
#define VE_WDBG(fmt, args...)   VE_DBG(fmt, args...)
#else
#define VE_WDBG(fmt, args...)
#endif

/*#define VE_REG_TRACE*/
#ifdef VE_REG_TRACE
#define VE_REG_SET(addr, val) \
	do {\
		printk("REG_SET:0x%x -> 0x%0x\n", addr, val);\
		REG32_VAL(addr) = (val);\
	} while (0)
#else
#define VE_REG_SET(addr, val)      (REG32_VAL(addr) = (val))
#endif

static int ve_minor;
static int ve_dev_nr = 1;
static struct cdev *ve_cdev;

static struct videoencoder *encoders[3] = {NULL, NULL, NULL};
struct ve_info veinfo = {NULL, 0};

static DEFINE_SEMAPHORE(ve_sem);
static DEFINE_SPINLOCK(sram_lock);
static int sram_owern;

static int wmt_ve_open(struct inode *inode, struct file *filp)
{
	int ret = -EINVAL;
	unsigned int idx = VE_MAX;
	unsigned int id;

	down(&ve_sem);
	idx = iminor(inode);
	id = idx - VE_JPEG;

	if ((idx < VE_MAX) && (idx > VE_BASE) && encoders[id]) {
		struct videoencoder *ve = encoders[id];

		if (ve && ve->fops.open) {
			filp->private_data = (void *)&veinfo;
			ret = ve->fops.open(inode, filp);
		}
		VE_DBG("encoder %s opened.\n", ve->name);
	}
	up(&ve_sem);

	return ret;
} /* End of videodecoder_open() */

static int wmt_ve_release(struct inode *inode, struct file *filp)
{
	int ret = -EINVAL;
	unsigned int idx = VE_MAX;
	unsigned int id;

	down(&ve_sem);
	idx = iminor(inode);
	id = idx - VE_JPEG;

	if (idx < VE_MAX && encoders[id]) {
		struct videoencoder *ve = encoders[id];

		if (ve && ve->fops.release)
			ret = ve->fops.release(inode, filp);

		VE_DBG("encoder %s closed.\n", ve->name);
	}
	up(&ve_sem);

	return ret;
} /* End of videodecoder_release() */

static long wmt_ve_ioctl(
	struct file *filp,
	unsigned int cmd,
	unsigned long arg)
{
	int ret = -EINVAL;
	unsigned int idx = VE_MAX;
	unsigned int id;
	struct inode *inode = filp->f_mapping->host;

	down(&ve_sem);
	idx = iminor(inode);
	id = idx - VE_JPEG;
	if (idx < VE_MAX && encoders[id]) {
		struct videoencoder *ve = encoders[id];
		if (ve && ve->fops.unlocked_ioctl)
			ret = ve->fops.unlocked_ioctl(filp, cmd, arg);
	}
	up(&ve_sem);

	return ret;
} /* End of videodecoder_ioctl() */

static int wmt_ve_mmap(struct file *filp, struct vm_area_struct *vma)
{
	int ret = -EINVAL;
	unsigned int idx = VE_MAX;
	unsigned int id;

	down(&ve_sem);
	if (filp && filp->f_dentry && filp->f_dentry->d_inode) {
		idx = iminor(filp->f_dentry->d_inode);
		id = idx - VE_JPEG;
	}

	if (idx < VE_MAX && encoders[id]) {
		struct videoencoder *ve = encoders[id];
		if (ve && ve->fops.mmap)
			ret = ve->fops.mmap(filp, vma);
	}
	up(&ve_sem);

	return ret;
}

static ssize_t wmt_ve_read(
	struct file *filp,
	char __user *buf,
	size_t count,
	loff_t *f_pos
)
{
	ssize_t ret = 0;
	unsigned int idx = VE_MAX;
	unsigned int id;

	down(&ve_sem);
	if (filp && filp->f_dentry && filp->f_dentry->d_inode) {
		idx = iminor(filp->f_dentry->d_inode);
		id = idx - VE_JPEG;
	}

	if (idx < VE_MAX && encoders[id]) {
		struct videoencoder *ve = encoders[id];
		if (ve && ve->fops.read)
			ret = ve->fops.read(filp, buf, count, f_pos);
	}
	up(&ve_sem);

	return ret;
} /* videodecoder_read */

static ssize_t wmt_ve_write(
	struct file *filp,
	const char __user *buf,
	size_t count,
	loff_t *f_pos
)
{
	ssize_t ret = 0;
	unsigned int idx = VE_MAX;
	unsigned int id;

	down(&ve_sem);
	if (filp && filp->f_dentry && filp->f_dentry->d_inode) {
		idx = iminor(filp->f_dentry->d_inode);
		id = idx - VE_JPEG;
	}

	if (idx < VE_MAX && encoders[id]) {
		struct videoencoder *ve = encoders[id];
		if (ve && ve->fops.write)
			ret = ve->fops.write(filp, buf, count, f_pos);
	}
	up(&ve_sem);

	return ret;
} /* End of videodecoder_write() */

const struct file_operations videoencoder_fops = {
	.owner          = THIS_MODULE,
	.open           = wmt_ve_open,
	.release        = wmt_ve_release,
	.read           = wmt_ve_read,
	.write          = wmt_ve_write,
	.unlocked_ioctl = wmt_ve_ioctl,
	.mmap           = wmt_ve_mmap,
};

static int wmt_ve_probe(struct platform_device *dev)
{
	dev_t dev_no;
	int ret = 0;

	dev_no = MKDEV(VD_MAJOR, ve_minor); /* Don't know VE MAJOR NUMBER? */

	/* register char device */
	ve_cdev = cdev_alloc();
	if (!ve_cdev) {
		VE_ERR("alloc dev error.\n");
		return -ENOMEM;
	}

	cdev_init(ve_cdev, &videoencoder_fops);
	ret = cdev_add(ve_cdev, dev_no, 1);

	if (ret) {
		VE_ERR("reg char dev error(%d)\n", ret);
		cdev_del(ve_cdev);
		return ret;
	}

	veinfo.prdt_virt = dma_alloc_coherent(NULL, MAX_INPUT_BUF_SIZE,
						&veinfo.prdt_phys, GFP_KERNEL|GFP_DMA);
	VE_DBG("veinfo.prdt_virt =0x%p, veinfo.prdt_phys =0x%08x\n",
			veinfo.prdt_virt, veinfo.prdt_phys);

	if (!veinfo.prdt_virt) {
		VE_ERR("allocate video input buffer error.\n");
		cdev_del(ve_cdev);
		veinfo.prdt_virt = NULL;
		veinfo.prdt_phys = 0;
		return -ENOMEM;
	}

	VE_DBG("prob /dev/%s major %d, minor %d, prdt %p/%x, size %d KB\n",
		DRIVER_NAME, VD_MAJOR, ve_minor, veinfo.prdt_virt,
		veinfo.prdt_phys, MAX_INPUT_BUF_SIZE/1024);

	return ret;
}

static int wmt_ve_remove(struct platform_device *dev)
{
	unsigned int idx = VE_JPEG;
	unsigned int id;

	down(&ve_sem);
	while (idx < VE_MAX) {
		id = idx - VE_JPEG;
		if (encoders[id] && encoders[id]->remove) {
			encoders[id]->remove();
			encoders[id] = NULL;
		}
		idx++;
		id++;
	}
	up(&ve_sem);

	if (veinfo.prdt_virt)
		dma_free_coherent(NULL, MAX_INPUT_BUF_SIZE,
			veinfo.prdt_virt, veinfo.prdt_phys);

	veinfo.prdt_virt = NULL;
	veinfo.prdt_phys = 0x0;

	return 0;
}

static int wmt_ve_suspend(struct platform_device *dev, pm_message_t state)
{
	int ret;
	unsigned int idx = VE_JPEG;
	unsigned int id;

	down(&ve_sem);
	while (idx < VE_MAX) {
		id = idx - VE_JPEG;
		if (encoders[id] && encoders[id]->suspend) {
			ret = encoders[id]->suspend(state);
			if (ret < 0) {
				VE_WARN("%s suspend error. ret = %d\n",
					encoders[id]->name, ret);
			}
		}
		idx++;
		id++;
	}
	up(&ve_sem);

	return 0;
}

static int wmt_ve_resume(struct platform_device * dev)
{
	int ret;
	unsigned int idx = VE_JPEG;
	unsigned int id;

	down(&ve_sem);
	while (idx < VE_MAX) {
		id = idx - VE_JPEG;
		if (encoders[id] && encoders[id]->resume) {
			ret = encoders[id]->resume();
			if (ret < 0) {
				VE_WARN("%s resume error. ret = %d\n",
					encoders[id]->name, ret);
			}
		}
		idx++;
		id++;
	}
	up(&ve_sem);

	return 0;
}

static void ve_platform_release(struct device *device)
{
	unsigned int idx = VE_JPEG;
	unsigned int id;

	down(&ve_sem);
	while (idx < VE_MAX) {
		id = idx - VE_JPEG;
		if (encoders[id] && encoders[id]->remove) {
			encoders[id]->remove();
			encoders[id] = NULL;
		}
		idx++;
		id++;
	}
	up(&ve_sem);

	if (veinfo.prdt_virt)
		dma_free_coherent(NULL, MAX_INPUT_BUF_SIZE,
			veinfo.prdt_virt, veinfo.prdt_phys);

	veinfo.prdt_virt = NULL;
	veinfo.prdt_phys = 0x0;

	return;
}

static struct platform_driver ve_driver = {
	.driver = {
		.name           = "wmt-ve",
		.bus            = &platform_bus_type,
	},
	.probe          = wmt_ve_probe,
	.remove         = wmt_ve_remove,
	.suspend        = wmt_ve_suspend,
	.resume         = wmt_ve_resume
};

static struct platform_device ve_device = {
	.name           = "wmt-ve",
	.id             = 0,
	.dev            =   {
						.release = ve_platform_release,
						},
	.num_resources  = 0,        /* ARRAY_SIZE(spi_resources), */
	.resource       = NULL,     /* spi_resources, */
};

#ifdef CONFIG_PROC_FS
static int ve_read_proc(
	char *page,
	char **start,
	off_t off,
	int count,
	int *eof,
	void *data
)
{
	int size;
	unsigned int idx = VE_JPEG;
	unsigned int id ;
	char *p;

	down(&ve_sem);
	p = page;
	p += sprintf(p, "***** video encoder information *****\n");

	while (idx < VE_MAX) {
		id = idx - VE_JPEG;
		if (encoders[id] && encoders[id]->get_info) {
			size = encoders[id]->get_info(p, start, off, count);
			count -= size;
			if (count <= 40)
				break;
			p += size;
		}
		idx++;
		id++;
	}
	p += sprintf(p, "**************** end ****************\n");
	up(&ve_sem);

	*eof = 1;
	return p - page;
}
#endif

int wmt_ve_register(struct videoencoder *encoder)
{
	int ret = 0;
	dev_t dev_no;
	unsigned int id = encoder->id - VE_JPEG;

	if (!encoder || encoder->id >= VE_MAX) {
		VE_WARN("register invalid video encoder\n");
		return -1;
	}

	if (encoders[id]) {
		VE_WARN("video encoder (ID=%d) exist.(E:%32s, N:%32s)\n",
			encoder->id, encoders[id]->name, encoder->name);
		return -1;
	}

	dev_no = MKDEV(VD_MAJOR, encoder->id);
	ret = register_chrdev_region(dev_no, ve_dev_nr, encoder->name);
	if (ret < 0) {
		VE_ERR("can't get %s device minor %d\n",
			encoder->name, encoder->id);
		return ret;
	}

	encoder->device = cdev_alloc();
	if (!encoder->device) {
		unregister_chrdev_region(dev_no, ve_dev_nr);
		VE_ERR("alloc dev error.\n");
		return -ENOMEM;
	}

	cdev_init(encoder->device, &videoencoder_fops);
	ret = cdev_add(encoder->device, dev_no, 1);
	if (ret) {
		VE_ERR("reg char dev error(%d)\n", ret);
		unregister_chrdev_region(dev_no, ve_dev_nr);
		cdev_del(encoder->device);
		encoder->device = NULL;
		return ret;
	}
	VE_DBG("encoder->irq_num = %d,encoder->isr = 0x%p\n",
		encoder->irq_num, encoder->isr);
	if (encoder->irq_num && encoder->isr) {
		ret = request_irq(encoder->irq_num, encoder->isr,
				IRQF_SHARED, encoder->name, encoder->isr_data);
		VE_INFO("%s Request IRQ %d %s.\n", encoder->name,
			encoder->irq_num, (ret < 0) ? "Fail" : "Ok");
	}

	if (!ret) {
		if (encoder->setup)
			ret = encoder->setup();
	}

	if (ret >= 0) {
		down(&ve_sem);
		encoders[id] = encoder;
		up(&ve_sem);
		VE_INFO("%s registered major %d minor %d\n",
			encoder->name, VD_MAJOR, encoder->id);
	} else {
		VE_ERR("%s register major %d minor %d fail\n",
			encoder->name, VD_MAJOR, encoder->id);
		free_irq(encoder->irq_num, (void *)encoder->isr_data);
		unregister_chrdev_region(dev_no, ve_dev_nr);
		cdev_del(encoder->device);
		encoder->device = NULL;
		return ret;
	}

	encoder->ve_class = class_create(THIS_MODULE, encoder->name);
	device_create(encoder->ve_class, NULL, dev_no, NULL, "%s",
		encoder->name);

	return ret;
}
EXPORT_SYMBOL(wmt_ve_register);

int wmt_ve_unregister(struct videoencoder *encoder)
{
	int ret = 0;
	dev_t dev_no;
	unsigned int id = encoder->id - VE_JPEG;

	if (!encoder || encoder->id >= VE_MAX || !encoder->device) {
		VE_WARN("unregister invalid video encoder\n");
		return -1;
	}

	if (encoders[id] != encoder) {
		VE_WARN("unregiseter wrong video encoder. (E:%32s, R:%32s)\n",
			encoders[id]->name, encoder->name);
		return -1;
	}

	down(&ve_sem);
	encoders[id] = NULL;
	up(&ve_sem);

	if (encoder->remove)
		ret = encoder->remove();

	dev_no = MKDEV(VD_MAJOR, encoder->id);
	free_irq(encoder->irq_num, (void *)encoder->isr_data);
	unregister_chrdev_region(dev_no, ve_dev_nr);
	cdev_del(encoder->device);
	encoder->device = NULL;

	/* let udev to handle node of video encoder */
	device_destroy(encoder->ve_class, dev_no);
	class_destroy(encoder->ve_class);

	return ret;
}
EXPORT_SYMBOL(wmt_ve_unregister);

static int __init wmt_ve_init(void)
{
	int ret;

#ifdef CONFIG_PROC_FS
	create_proc_read_entry("wmt-ve", 0, NULL, ve_read_proc, NULL);
	VE_DBG("create video encoder proc\n");
#endif

	ret = platform_device_register(&ve_device);
	if (!ret) {
		ret = platform_driver_register(&ve_driver);
		if (ret) {
			platform_device_unregister(&ve_device);
		}
	}
	VE_DBG("WonderMedia HW encoder driver inited\n");

	return ret;
}

void __exit wmt_ve_exit(void)
{
	dev_t dev_no;

	platform_device_unregister(&ve_device);
	platform_driver_unregister(&ve_driver);
	dev_no = MKDEV(VD_MAJOR, ve_minor);
	unregister_chrdev_region(dev_no, ve_dev_nr);

	VE_INFO("WonderMedia HW encoder driver exit\n");

	return;
}

/*!*************************************************************************
* wmt_ve_request_sram
*
* API Function
*/
/*!
* \brief
*   Request HW encoder SRAM
*
* \retval  0 if success
*/
int wmt_ve_request_sram(int ve_id)
{
	unsigned long flags = 0;
	int val = 0, ret = 0;

	/*----------------------------------------------------------------------
		Set SRAM Share Enable for H.264 & JPEG encoders
	----------------------------------------------------------------------*/
	spin_lock_irqsave(&sram_lock, flags);
	if (sram_owern != 0) {
		VE_ERR("Shared SRAM is occupied by %d\n", sram_owern);
		ret = -1;
		goto EXIT_wmt_ve_request_sram;
	}
	if (ve_id == VE_JPEG) {
		val = 0x01;
	} else if (ve_id == VE_H264) {
		val = 0x00;
	} else {
		VE_ERR("Unknow Encoder ID: %d\n", ve_id);
		ret = -1;
		goto EXIT_wmt_ve_request_sram;
	}
	sram_owern = ve_id;
	VE_REG_SET(MSVD_BASE_ADDR + 0x070, val);

EXIT_wmt_ve_request_sram:
	spin_unlock_irqrestore(&sram_lock, flags);

	return ret;
} /* End of wmt_ve_request_sram() */
EXPORT_SYMBOL(wmt_ve_request_sram);

/*!*************************************************************************
* wmt_ve_free_sram
*
* API Function
*/
/*!
* \brief
*   Request HW encoder SRAM
*
* \retval  0 if success
*/
int wmt_ve_free_sram(int ve_id)
{
	unsigned long flags = 0;

	spin_lock_irqsave(&sram_lock, flags);
	if (sram_owern == ve_id)
		sram_owern = 0;
	else
		VE_ERR("Wrong owner (%d) for caller (%d)\n", sram_owern, ve_id);
	spin_unlock_irqrestore(&sram_lock, flags);

	return 0;
} /* End of wmt_ve_free_sram() */
EXPORT_SYMBOL(wmt_ve_free_sram);

fs_initcall(wmt_ve_init);
module_exit(wmt_ve_exit);

MODULE_AUTHOR("WonderMedia Technologies, Inc.");
MODULE_DESCRIPTION("Video Encoder device driver");
MODULE_LICENSE("GPL");

