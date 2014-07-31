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
#ifndef WMT_VD_H
#define WMT_VD_H

/*-------------------- MODULE DEPENDENCY -------------------------------------*/

#include <linux/init.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/errno.h>
#include <asm/uaccess.h>
#include <linux/kdev_t.h>
#include <linux/cdev.h>
#include <linux/major.h>
#include <linux/interrupt.h>
#include <linux/spinlock.h>
#include <linux/mm.h>
#include <asm/page.h>


#include <linux/dma-mapping.h>
#include <linux/irqreturn.h>

#include <mach/irqs.h>
#include <mach/hardware.h>
#include <linux/sched.h>     /* for 2.6.32 wait_event_interruptible() */

#include <linux/wmt-mb.h>

#ifdef CONFIG_PROC_FS
#include <linux/proc_fs.h>
    #include <linux/uaccess.h>
#endif

#include "com-vd.h"


/*-------------------- EXPORTED PRIVATE CONSTANTS ------------------------*/

/*--------------------------------------------------------------------------
	Since the page size in kernel is 4 KB, so we may assume the max buffer
	size as input buffer size = (prd_size/8)*4KB
	In short, 1 KB PRD size could store about 0.5 MB data.
	If we support maximun input buffer size is 50 MB, we must set prd_size
	as 100 KB.
---------------------------------------------------------------------------*/
#define MAX_INPUT_BUF_SIZE  (100*1024) /* 100 KB */

struct vd_mem_set {
	void            *virt;
	dma_addr_t      phys;
	unsigned int    size;
};

struct vd_resource {
	struct vd_mem_set prdt;
	struct vd_mem_set cnm_fw_real;
	struct vd_mem_set cnm_fw_align;
};

struct videodecoder {
	char name[32];
	int id;
	int (*setup)(void);
	int (*remove)(void);
	int (*suspend)(pm_message_t state);
	int (*resume)(void);
	const struct file_operations  fops;
	int (*get_info)(char *, char **, off_t, int);
	struct cdev *device;
	int irq_num;
	irqreturn_t (*isr)(int irq, void *isr_data);
	void *isr_data;

	struct class *vd_class; /* for udev */
};

int wmt_vd_register(struct videodecoder *);
int wmt_vd_unregister(struct videodecoder *);

#endif

