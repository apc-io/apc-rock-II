/*++
 * WonderMedia Codec Lock driver
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

#include <linux/fs.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/cdev.h>
#include <linux/pci.h>
#include <asm/uaccess.h>
#include <linux/proc_fs.h>
#include <linux/sched.h>
#include <linux/major.h>

#include "com-lock.h"

#define WMT_LOCK_NAME     "wmt-lock"
#define WMT_LOCK_MAJOR    MB_MAJOR   /* same as memblock driver */
#define WMT_LOCK_MINOR    1      /* 0 is /dev/mbdev, 1 is /dev/wmt-lock */

#define MAX_LOCK_JDEC     1       /* Max lock number for JPEG decoder */

/* Even for multi-decoding, this value should <= 4 */
#define MAX_LOCK_VDEC     4

#define MAX_LOCK_TYPE     3       /* JPEG, MSVD & encoders */

#define MAX_LOCK_OWNER  (MAX_LOCK_JDEC + MAX_LOCK_VDEC)


static struct class *wmt_lock_class;


#undef WMT_LOCK_DEBUG
/*#define WMT_LOCK_DEBUG*/

#ifdef WMT_LOCK_DEBUG
#define P_DEBUG(fmt, args...)    printk(KERN_INFO "["WMT_LOCK_NAME"] " fmt , ## args)
#else
#define P_DEBUG(fmt, args...)    ((void)(0))
#endif

#define P_INFO(fmt, args...)     printk(KERN_INFO "[wmt-lock] " fmt , ## args)
#define P_WARN(fmt, args...)     printk(KERN_WARNING "[wmt-lock] *W* " fmt, ## args)
#define P_ERROR(fmt, args...)    printk(KERN_ERR "[wmt-lock] *E* " fmt , ## args)


static struct cdev wmt_lock_cdev;

struct lock_owner {
	pid_t  pid;
	void  *private_data;
	char   comm[TASK_COMM_LEN];
	const char        *type;
};

struct semaphore lock_sem_jpeg;
struct semaphore lock_sem_video;
struct semaphore lock_sem_encoder;

struct lock_owner_s {
	const char        *type;
	struct semaphore  *sem;
	unsigned int       max_lock_num;
	struct lock_owner  owners[MAX_LOCK_OWNER];
};

struct lock_owner_s gLockers[MAX_LOCK_TYPE];

static spinlock_t   gSpinlock;


/*!*************************************************************************
* get_sema
*
* Private Function
*
* \brief
*
* \retval  ponters to specified semaphore
*/
static struct semaphore *get_sema(long lock_type)
{
	return gLockers[lock_type].sem;
}

/*!*************************************************************************
* set_owner
*
* Private Function
*
* \brief
*
* \retval  0 if success, error code if fail
*/
static struct lock_owner *set_owner(long lock_type, struct file *filp)
{
	struct lock_owner *o;
	int i;

	for (i = 0; i < gLockers[lock_type].max_lock_num; i++) {
		if (gLockers[lock_type].owners[i].pid == 0) {
			o = &gLockers[lock_type].owners[i];
			o->private_data = filp->private_data;
			o->pid          = current->pid;
			memcpy(o->comm, current->comm, TASK_COMM_LEN);
			return o;
		}
	}
	P_ERROR("Set owner fail because of %s lock full\n",
		gLockers[lock_type].type);
	return 0;
}

/*!*************************************************************************
* get_owner
*
* Private Function
*
* \brief
*
* \retval  0 if success, error code if fail
*/
static struct lock_owner *get_owner(long lock_type)
{
	struct lock_owner *o;
	int i;

	for (i = 0; i < gLockers[lock_type].max_lock_num; i++) {
		o = &gLockers[lock_type].owners[i];
		if (o->pid == current->pid)
			return o;
	}
	return NULL;
}

/*!*************************************************************************
* check_busy
*
* Private Function
*
* \brief
*
* \retval  0 if not busy, otherwise return 1
*/
static int check_busy(long lock_type)
{
	struct lock_owner *o;
	int i;

	for (i = 0; i < gLockers[lock_type].max_lock_num; i++) {
		o = &gLockers[lock_type].owners[i];
		if (o->pid == current->pid)
			return 1; /* Busy */
	}
	return 0;
}

/*!*************************************************************************
* lock_read_proc
*
* Private Function
*
* \brief
*
* \retval  0 if success, error code if fail
*/
static int lock_read_proc(
	char *page, char **start,
	off_t offset,
	int len,
	int *eof,
	void *data)
{
	char *p = page;
	struct lock_owner *o;
	int i, j;

	for (i = 0; i < MAX_LOCK_TYPE; i++) {
		p += sprintf(p, "------ %s lock ------\n", gLockers[i].type);
		for (j = 0; j < gLockers[i].max_lock_num; j++) {
			o = &gLockers[i].owners[j];
			if (o->pid != 0)
				p += sprintf(p, "%s lock : occupied by [%s,%d]\n",
					gLockers[i].type, o->comm, o->pid);
			else
				p += sprintf(p, "%s lock : Free\n", gLockers[i].type);
		}
	}
	return p - page;
}

/*!*************************************************************************
* wmt_lock_ioctl
*
* Private Function
*
* \brief
*
* \retval  0 if success, error code if fail
*/
static long wmt_lock_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	int ret = 0;
	struct lock_owner *o;
	struct semaphore *sem;
	struct wmt_lock *lock_arg;
	long timeout;
	unsigned long flags = 0;

	/* check ioctl type and number, if fail return EINVAL */
	if (_IOC_TYPE(cmd) != LOCK_IOC_MAGIC) {
		P_WARN("ioctl unknown cmd %X, type %X by [%s,%d]\n",
			cmd, _IOC_TYPE(cmd), current->comm, current->pid);
		return -EINVAL;
	}

	if (!access_ok(VERIFY_READ, (void __user *)arg, sizeof(struct wmt_lock))) {
		P_WARN("ioctl access_ok failed, cmd %X, type %X by [%s,%d]\n",
			cmd, _IOC_TYPE(cmd), current->comm, current->pid);
		return -EFAULT;
	}

	lock_arg = (struct wmt_lock *)arg;

	if (lock_arg->lock_type >= MAX_LOCK_TYPE || lock_arg->lock_type < 0) {
		P_WARN("invalid lock type %ld by [%s,%d]\n",
			lock_arg->lock_type, current->comm, current->pid);
		return -E2BIG;
	}

	spin_lock_irqsave(&gSpinlock, flags);
	sem  = get_sema(lock_arg->lock_type);

	switch (cmd) {
	case IO_WMT_LOCK:
		timeout = lock_arg->arg2;
		/* check if the current thread already get the lock */
		if (check_busy(lock_arg->lock_type)) {
			P_WARN("Recursive %s lock by [%s,%d]\n",
				gLockers[lock_arg->lock_type].type,
				current->comm, current->pid);
			return -EBUSY;
		}

		if (timeout == 0) {
			ret = down_trylock(sem);
			if (ret)
				ret = -ETIME;  /* reasonable if lock holded by other */
		} else if (timeout == -1) {
			ret = down_interruptible(sem);
			if (ret)
				P_INFO("Require %s lock error %d by [%s,%d]\n",
					gLockers[lock_arg->lock_type].type, ret,
					current->comm, current->pid);
		} else {
			/* require lock with a timeout value, please beware
				this function can't exit when interrupt */
			spin_unlock_irqrestore(&gSpinlock, flags);
			ret = down_timeout(sem, msecs_to_jiffies(timeout));
			spin_lock_irqsave(&gSpinlock, flags);
		}
		if (ret == 0) {
			o = set_owner(lock_arg->lock_type, filp);
			if (o)
				P_DEBUG("%s is locked by [%s,%d], seq %d\n",
					o->type, o->comm, o->pid, (int)filp->private_data);
		}
		break;

	case IO_WMT_UNLOCK:
		o = get_owner(lock_arg->lock_type);
		if (o == NULL) {
			P_WARN("Unnecessary %s unlock from [%s,%d] when lock is free\n",
				gLockers[lock_arg->lock_type].type,
				current->comm, current->pid);
			ret = -EACCES;
		} else if (filp->private_data == o->private_data) {
			P_DEBUG("%s is unlocked by [%s,%d], seq %d\n",
				o->type, o->comm, o->pid, (int)filp->private_data);
			o->pid = 0;
			o->private_data = 0;
			up(sem);
			ret = 0;
		} else{
			P_WARN("Unexpected %s unlock from [%s,%d], hold by [%s,%d] now\n",
				o->type, current->comm, current->pid, o->comm, o->pid);
			ret = -EACCES;
		}
		break;

	case IO_WMT_CHECK_WAIT:
		if (sem->count > 0)
			lock_arg->is_wait = 0;
		else {
			if ((sem->wait_list.prev == 0) && (sem->wait_list.next == 0))
				lock_arg->is_wait = 0;
			else
				lock_arg->is_wait = 1;
		}
		P_INFO("sem->count: %d\n", sem->count);
		P_INFO("lock_arg->is_wait: %d\n", lock_arg->is_wait);
		break;

	default:
		P_WARN("ioctl unknown cmd 0x%X  by [%s,%d]\n",
			cmd, current->comm, current->pid);
		ret = -EINVAL;
	}
	spin_unlock_irqrestore(&gSpinlock, flags);

	return ret;
}

/*!*************************************************************************
* wmt_lock_open
*
* Private Function
*
* \brief
*
* \retval  0 if success, error code if fail
*/
static int wmt_lock_open(struct inode *inode, struct file *filp)
{
	static atomic_t lock_seq_id = ATOMIC_INIT(0);

	/* use a sequence number as the file open id */
	filp->private_data = (void *)(atomic_add_return(1, &lock_seq_id));
	P_DEBUG("open by [%s,%d], seq %d\n",
		current->comm, current->pid, (int)filp->private_data);
	return 0;
}

/*!*************************************************************************
* wmt_lock_release
*
* Private Function
*
* \brief
*
* \retval  0 if success, error code if fail
*/
static int wmt_lock_release(struct inode *inode, struct file *filp)
{
	int i, j;

	for (i = 0; i < MAX_LOCK_TYPE; i++) {
		for (j = 0; j < gLockers[i].max_lock_num; j++) {
			struct lock_owner *o = &gLockers[i].owners[j];
			if (o->pid != 0 && filp->private_data == o->private_data) {
				P_WARN("Auto free %s lock hold by [%s,%d]\n",
					o->type, o->comm, o->pid);
				o->pid = 0;
				o->private_data = 0;
				up(get_sema(i));
			}
		}
	}
	P_DEBUG("Release by [%s,%d], seq %d\n",
		current->comm, current->pid, (int)filp->private_data);
	return 0;
}

static const struct file_operations wmt_lock_fops = {
	.owner   = THIS_MODULE,
	.open    = wmt_lock_open,
	.unlocked_ioctl = wmt_lock_ioctl,
	.release = wmt_lock_release,
};

static void check_multi_vd_count(int *max_vd_count)
{
	extern int wmt_getsyspara(char *varname, unsigned char *varval, int *varlen);

	char buf[80] = {0};
	int varlen = 80;
	int max_count = 1;

	/* Read u-boot parameter to decide value of wmt_codec_debug */
	/*----------------------------------------------------------------------
		Check wmt.codec.debug
	----------------------------------------------------------------------*/
	if (wmt_getsyspara("wmt.multi.vd.max", buf, &varlen) == 0)
		max_count = simple_strtol(buf, NULL, 10);

	if (max_count < 1)
		max_count = 1;
	if (max_count > MAX_LOCK_VDEC)
		max_count = MAX_LOCK_VDEC;

	*max_vd_count = max_count;

	return;
} /* End of check_debug_option() */

/*!*************************************************************************
* wmt_lock_init
*
* Private Function
*
* \brief
*
* \retval  0 if success, error code if fail
*/
static int __init wmt_lock_init(void)
{
	dev_t   dev_id;
	int max_vd_count;
	int ret, i, j;

	dev_id = MKDEV(WMT_LOCK_MAJOR, WMT_LOCK_MINOR);
	ret = register_chrdev_region(dev_id, 1, WMT_LOCK_NAME);
	if (ret < 0) {
		P_ERROR("can't register %s device %d:%d, ret %d\n",
			WMT_LOCK_NAME, WMT_LOCK_MAJOR, WMT_LOCK_MINOR, ret);
		return ret;
	}

	cdev_init(&wmt_lock_cdev, &wmt_lock_fops);
	ret = cdev_add(&wmt_lock_cdev, dev_id, 1);
	if (ret) {
		P_ERROR("cdev add error(%d).\n", ret);
		unregister_chrdev_region(dev_id, 1);
		return ret;
	}

	/* let udev to handle /dev/wmt-lock */
	wmt_lock_class = class_create(THIS_MODULE, WMT_LOCK_NAME);
	device_create(wmt_lock_class, NULL, dev_id, NULL, "%s", WMT_LOCK_NAME);

	create_proc_read_entry(WMT_LOCK_NAME, 0, NULL, lock_read_proc, NULL);
	P_INFO("init ok, major=%d, minor=%d\n", WMT_LOCK_MAJOR, WMT_LOCK_MINOR);

	spin_lock_init(&gSpinlock);

	check_multi_vd_count(&max_vd_count);

	/* Init sema for JPEG decoder */
	sema_init(&lock_sem_jpeg, MAX_LOCK_JDEC);
	gLockers[lock_jpeg].type = "jdec";
	gLockers[lock_jpeg].sem = &lock_sem_jpeg;
	gLockers[lock_jpeg].max_lock_num = MAX_LOCK_JDEC;

	/* Init sema for MSVD decoders */
	sema_init(&lock_sem_video, max_vd_count);
	gLockers[lock_video].type = "vdec";
	gLockers[lock_video].sem = &lock_sem_video;
	gLockers[lock_video].max_lock_num = max_vd_count;

	for (i = 0; i < MAX_LOCK_TYPE; i++) {
		for (j = 0; j < gLockers[i].max_lock_num; j++) {
			struct lock_owner *o = &gLockers[i].owners[j];
			o->type = gLockers[i].type;
		}
	}
	return ret;
}

/*!*************************************************************************
* wmt_lock_cleanup
*
* Private Function
*
* \brief
*
* \retval  0 if success, error code if fail
*/
static void __exit wmt_lock_cleanup(void)
{
	dev_t dev_id = MKDEV(WMT_LOCK_MAJOR, WMT_LOCK_MINOR);

	cdev_del(&wmt_lock_cdev);

	/* let udev to handle /dev/wmt-lock */
	device_destroy(wmt_lock_class, dev_id);
	class_destroy(wmt_lock_class);

	unregister_chrdev_region(dev_id, 1);
	remove_proc_entry(WMT_LOCK_NAME, NULL);

	P_INFO("cleanup done\n");
}

module_init(wmt_lock_init);
module_exit(wmt_lock_cleanup);


MODULE_AUTHOR("WonderMedia Technologies, Inc.");
MODULE_DESCRIPTION("WMT Codec Lock device driver");
MODULE_LICENSE("GPL");

