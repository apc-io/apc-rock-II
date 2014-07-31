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
#ifndef WMT_VE_H
#define WMT_VE_H

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
#include <linux/sched.h>
#include <linux/wmt-mb.h>

#ifdef CONFIG_PROC_FS
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#endif

#include "com-ve.h"


/*-------------------- EXPORTED PRIVATE CONSTANTS ------------------------*/

/*-------------------------------------------------------------------------
	Definitions of enum
--------------------------------------------------------------------------*/
enum {
	VE_STA_READY          = 0x00010000,
	VE_STA_CLOSE          = 0x00020000,
	VE_STA_PREPARE        = 0x00040000,
	VE_STA_DMA_START      = 0x00080000,
	VE_STA_ENCODE_DONE    = 0x00000001,
	VE_STA_ENCODEING      = 0x00000002,
	VE_STA_READ_FINISH    = 0x00000004,
	VE_STA_DMA_MOVE_DONE  = 0x00000008,
	/* Error Status */
	VE_STA_ERR_MEM_FULL   = 0x80000000,
	VE_STA_ERR_UNKNOWN    = 0x80800000
}; /* ve_status */

/*--------------------------------------------------------------------------
	Since the page size in kernel is 4 KB, so we may assume the max buffer
	size as input buffer size = (prd_size/8)*4KB
	In short, 1 KB PRD size could store about 0.5 MB data.
	If we support maximun input buffer size is 50 MB, we must set prd_size
	as 100 KB.
---------------------------------------------------------------------------*/
#define MAX_INPUT_BUF_SIZE  (100*1024) /* 100 KB */
#define MAX_OUTPUT_BUF_SIZE  (1920*1080)

struct ve_info {
	void        *prdt_virt;
	dma_addr_t  prdt_phys;
};

struct videoencoder {
	char name[32];
	int id;
	int (*setup)(void);
	int (*remove)(void);
	int (*suspend)(pm_message_t state);
	int (*resume)(void);
	const struct file_operations  fops;
#ifdef CONFIG_PROC_FS
	int (*get_info)(char *, char **, off_t, int);
#endif
	struct cdev *device;
	int	irq_num;
	irqreturn_t (*isr)(int irq, void *isr_data);
	void *isr_data;

	struct class *ve_class; /* for udev */
};

int wmt_ve_register(struct videoencoder *);
int wmt_ve_unregister(struct videoencoder *);

int wmt_ve_request_sram(int ve_id);
int wmt_ve_free_sram(int ve_id);

#endif /* WMT_VE_H */


