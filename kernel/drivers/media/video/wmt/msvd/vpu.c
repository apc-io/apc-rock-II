/*++
 * Common interface for WonderMedia SoC hardware encoder and decoder drivers
 *
 * Copyright (c) 2012-2013  WonderMedia  Technologies, Inc.
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

#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/interrupt.h>
#include <linux/ioport.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <linux/wait.h>
#include <linux/list.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/uaccess.h>
#include <linux/cdev.h> 
#include <linux/slab.h>
#include <linux/sched.h>


#include "vpu.h"


#define MAX_NUM_VPU_CORE 1

#define WMT    /* WonderMedia Porting */

#ifdef WMT
#include <asm/signal.h>
#include <asm-generic/siginfo.h>
#include <asm/cacheflush.h>
#include <linux/fs.h>
#include <linux/module.h>
#include <linux/major.h>
#include <linux/wmt-mb.h>
#include <mach/irqs.h>
#include <mach/hardware.h>
#include <linux/mutex.h>

#include "../wmt-codec.h"

#define THE_MB_USER "WMT-MSVD"
#define MAX_NUM_INSTANCE    4
#define MSVD_MINOR          2  /* 1 is used by jdec */

static struct class *msvd_class;

ulong vpu_mem_start = 0;
int vpu_mem_size = 0;
int vpu_mem_debug = 0;
int vpu_use_dma_alloc = 0;
int vpu_mem_is_valid = 0;

//suspend & resume
int wmt_clk_cnt = 0;
struct mutex wmt_suspend_resume_lock;

module_param(vpu_mem_start, ulong, S_IRUSR | S_IRGRP | S_IROTH);
MODULE_PARM_DESC(vpu_mem_start, "VPU memory start address");

module_param(vpu_mem_size, int, S_IRUSR | S_IRGRP | S_IROTH);
MODULE_PARM_DESC(vpu_mem_size, "VPU memory size");

module_param(vpu_use_dma_alloc, int, S_IRUSR | S_IRGRP | S_IROTH);
MODULE_PARM_DESC(vpu_use_dma_alloc, "VPU use dma alloc");

module_param(vpu_mem_debug, int, S_IRUSR | S_IRGRP | S_IROTH);
MODULE_PARM_DESC(vpu_mem_debug, "VPU memory debug");
#endif

// definitions to be changed as customer  configuration
#ifdef WMT
#define VPU_SUPPORT_CLOCK_CONTROL            //if you want to have clock gating scheme frame by frame
#define VPU_SUPPORT_ISR                      //if the driver want to use interrupt service from kernel ISR
#define VPU_SUPPORT_PLATFORM_DRIVER_REGISTER //if the platform driver knows the name of this driver. VPU_PLATFORM_DEVICE_NAME 
//#define VPU_SUPPORT_RESERVED_VIDEO_MEMORY  //if this driver knows the dedicated video memory address

#define VPU_PLATFORM_DEVICE_NAME "wmt_msvd"
#define VPU_CLK_NAME "vcodec"
#define VPU_DEV_NAME "msvd"
#else
//#define VPU_SUPPORT_CLOCK_CONTROL				//if you want to have clock gating scheme frame by frame
#define VPU_SUPPORT_ISR						//if the driver want to use interrupt service from kernel ISR
//#define VPU_SUPPORT_PLATFORM_DRIVER_REGISTER	//if the platform driver knows the name of this driver. VPU_PLATFORM_DEVICE_NAME 
#define VPU_SUPPORT_RESERVED_VIDEO_MEMORY		//if this driver knows the dedicated video memory address

#define VPU_PLATFORM_DEVICE_NAME "vdec"
#define VPU_CLK_NAME "vcodec"
#define VPU_DEV_NAME "vpu"
#endif

// if the platform driver knows this driver. the definition of VPU_REG_BASE_ADDR and VPU_REG_SIZE are not meaningful
#ifdef WMT
#define VPU_REG_BASE_ADDR  0xD80F8000
#define VPU_REG_SIZE       0x2000
#ifdef VPU_SUPPORT_ISR
#define VPU_IRQ_NUM        IRQ_MSVD
#endif
#else
#define VPU_REG_BASE_ADDR 0x75000000
#define VPU_REG_SIZE (0x4000*MAX_NUM_VPU_CORE)

#ifdef VPU_SUPPORT_ISR
#define VPU_IRQ_NUM (23+32)
#endif
#endif

typedef struct vpu_drv_context_t {
	struct fasync_struct *async_queue;	
} vpu_drv_context_t;

/* To track the allocated memory buffer */
typedef struct vpudrv_buffer_pool_t {
	struct list_head list;
	struct vpudrv_buffer_t vb;
} vpudrv_buffer_pool_t;

#ifdef VPU_SUPPORT_RESERVED_VIDEO_MEMORY
#define VPU_INIT_VIDEO_MEMORY_SIZE_IN_BYTE (62*1024*1024)
#define VPU_DRAM_PHYSICAL_BASE 0x86C00000
#include "vmm.h"
static video_mm_t s_vmem;	
static vpudrv_buffer_t s_video_memory = {0};
#endif //VPU_SUPPORT_RESERVED_VIDEO_MEMORY

#ifdef WMT
static void vpu_device_release(struct device *device)
{
	return;
}

static struct resource vpu_device_resources[] = {
	[0] = {
		.start  = VPU_REG_BASE_ADDR,
		.end    = VPU_REG_BASE_ADDR + VPU_REG_SIZE,
		.flags  = IORESOURCE_MEM,
	},
	[1] = {
		.start  = VPU_IRQ_NUM,
		.end    = VPU_IRQ_NUM,
		.flags  = IORESOURCE_IRQ,
	},
};

static struct platform_device vpu_device = {
	.name = VPU_PLATFORM_DEVICE_NAME,
	.id = 0,
	.dev = {
		.release = vpu_device_release,
	},
	.num_resources = ARRAY_SIZE(vpu_device_resources),
	.resource = vpu_device_resources,
};

typedef struct  {
	vpu_drv_context_t *dev;
	pid_t pid, tgid;
} msvd_ctx_t;

typedef struct { // Sync with msvd_instance_pool_info_t in vdi.h
	int layout_inited;
	int layout_size;
	int maximum_instances;
	int offsetof_vpu_instance_num;
	int offsetof_instance_pool_inited;
	int sizeof_CodecInst;
	int offsetof_pendingInst;
	int offsetof_codecInstPool;
	int offsetof_instIndex_from_CodecInst;
	int offsetof_inUse_from_CodecInst;
	int offsetof_pid_from_CodecInst;
	int offsetof_tid_from_CodecInst;
} msvd_instance_pool_layout_t;

#define instancepool_layout() (msvd_instance_pool_layout_t *)(s_instance_pool.base)

#define offsetof_instancepool(name, type, argu) \
	do { \
		msvd_instance_pool_layout_t *_loA = instancepool_layout(); \
		unsigned char *_ptrA = (unsigned char *)(s_instance_pool.base); \
		if (!_loA || !_loA->layout_inited || \
			_loA->layout_size != sizeof(msvd_instance_pool_layout_t)) {\
			printk(KERN_ERR "layout %p layout_inited %d layout_size %d/%d", \
				_loA,(_loA)?_loA->layout_inited:0, \
				(_loA)?_loA->layout_size:0, \
				sizeof(msvd_instance_pool_layout_t)); \
			argu = NULL; \
		} \
		argu = (type *)&_ptrA[_loA->offsetof_##name]; \
	} while(0)

#define offsetof_instance(idx, name, type, argu) \
	do { \
		msvd_instance_pool_layout_t *_loB; \
		int *_nInstB; \
		unsigned char *_ptrB; \
		_loB = instancepool_layout(); \
		offsetof_instancepool(vpu_instance_num, int, _nInstB); \
		offsetof_instancepool(codecInstPool, unsigned char, _ptrB); \
		if (!_loB || !_nInstB || !_ptrB || idx >= MAX_NUM_INSTANCE) {\
			printk(KERN_ERR "layout %p nInstance %d/%d instance %p", \
				_loB,(_nInstB)?*_nInstB:0, idx, _ptrB); \
			argu = NULL; \
		} \
		_ptrB += idx * _loB->sizeof_CodecInst; \
		argu = (type *)&_ptrB[_loB->offsetof_##name##_from_CodecInst]; \
	} while(0)
#endif

static int vpu_hw_reset(void);
static void vpu_clk_disable(struct clk *clk);
static int vpu_clk_enable(struct clk *clk);
static struct clk *vpu_clk_get(struct device *dev);
static void vpu_clk_put(struct clk *clk);
// end customer definition
static vpudrv_buffer_t s_instance_pool = {0};
static vpudrv_buffer_t s_common_memory = {0};
static vpu_drv_context_t s_vpu_drv_context;
static int s_vpu_major;
static struct cdev s_vpu_cdev;  
static u32 s_vpu_open_count;
static struct clk *s_vpu_clk;
#ifdef VPU_SUPPORT_ISR
static int s_vpu_irq = VPU_IRQ_NUM;
#endif
static u32 s_vpu_reg_phy_addr = VPU_REG_BASE_ADDR;
static void __iomem *s_vpu_reg_virt_addr;
#ifdef WMT
static void *s_codec_tm[MAX_NUM_INSTANCE];
static int s_pending_instance = -1;
#endif

static int s_interrupt_flag;
static wait_queue_head_t s_interrupt_wait_q;

static spinlock_t s_vpu_lock = __SPIN_LOCK_UNLOCKED(s_vpu_lock);
static struct list_head s_vbp_head = LIST_HEAD_INIT(s_vbp_head);

static vpu_bit_firmware_info_t s_bit_firmware_info[MAX_NUM_VPU_CORE];

#ifdef CONFIG_PM
/* implement to power management functions */
#ifdef WMT
#include "regdefine.h"
#else
#include "../../../vpuapi/regdefine.h"	
#endif
static u32	s_vpu_reg_store[MAX_NUM_VPU_CORE][64];
#endif

#define	ReadVpuRegister(addr)		__raw_readl((__force u32)(s_vpu_reg_virt_addr + s_bit_firmware_info[core].reg_base_offset + addr))
#define	WriteVpuRegister(addr, val)	__raw_writel(val, (__force u32)(s_vpu_reg_virt_addr + s_bit_firmware_info[core].reg_base_offset + addr))


#ifdef WMT
static void vd_dump_registers(void)
{
#define DUMP_VPU_REG // Dump VPU register for CnM debugging
#ifdef DUMP_VPU_REG
	#define Dump_RegisterValue(addr)   printk("REG(0x%x): 0x%x\n", addr, REG32_VAL(addr));

	int i;

	wmb();
	printk("BIT_CUR_PC: \n");
	Dump_RegisterValue(CNM_BIT_BASE_ADDR + BIT_CUR_PC);
	printk("\nDump CnM Register: \n");
	for ( i = 0 ; i < 128 ; i++)
		Dump_RegisterValue(CNM_BIT_BASE_ADDR + (i * 4));

	for ( i = 0 ; i < 33 ; i++)
		Dump_RegisterValue(CNM_BIT_BASE_ADDR + 0x1070 +(i * 4));
#endif
	printk(KERN_ERR "MSVD ISR Timeout [REG: %08x] %.08x %.08x %.08x %.08x\n", MSVD_BASE_ADDR,
		REG32_VAL(MSVD_BASE_ADDR+0x000), REG32_VAL(MSVD_BASE_ADDR+0x004),
		REG32_VAL(MSVD_BASE_ADDR+0x008), REG32_VAL(MSVD_BASE_ADDR+0x00C));
} /* End of vd_dump_registers() */
#endif

static int vpu_alloc_dma_buffer(vpudrv_buffer_t *vb)
{
	if (!vb)
		return -1;
	
#ifdef VPU_SUPPORT_RESERVED_VIDEO_MEMORY
	vb->phys_addr = (unsigned long long)vmem_alloc(&s_vmem, vb->size, 0);
	if (vb->phys_addr  == (unsigned long)-1)
	{
		printk(KERN_ERR "[VPUDRV] Physical memory allocation error size=%d\n", vb->size);
		return -1;
	}

	vb->base = (unsigned long)(s_video_memory.base + (vb->phys_addr - s_video_memory.phys_addr));
#else
#ifdef WMT
	if (vpu_use_dma_alloc) {
		struct device *dev = vpu_mem_is_valid ? &vpu_device.dev : NULL;
		if (dev == NULL) return -1;
		vb->base = (unsigned long)dma_alloc_coherent(dev, PAGE_ALIGN(vb->size), (dma_addr_t *) (&vb->phys_addr), GFP_DMA | GFP_KERNEL);
    } else {
		vb->phys_addr = mb_alloc(PAGE_ALIGN(vb->size));
		if (vb->phys_addr) {
			vb->base = (unsigned long)mb_phys_to_virt(vb->phys_addr);
			if (vb->base) memset((void *)vb->base, 0x0, PAGE_ALIGN(vb->size));
		} else
			vb->base = 0x0;
	}
	if (vpu_mem_debug) {
		printk(KERN_ERR "[VPUDRV] alloc pa %08lx, va %08lx, len %d\n",
					(long unsigned int)vb->phys_addr, vb->base, vb->size);
	}
#else
	vb->base = (unsigned long)dma_alloc_coherent(NULL, PAGE_ALIGN(vb->size), (dma_addr_t *) (&vb->phys_addr), GFP_DMA | GFP_KERNEL);
#endif
	if ((void *)(vb->base) == NULL) 
	{
		printk(KERN_ERR "[VPUDRV] Physical memory allocation error size=%d\n", vb->size);
		return -1;
	}
#endif
	return 0;
}

static void vpu_free_dma_buffer(vpudrv_buffer_t *vb)
{
	if (!vb)
		return;

#ifdef VPU_SUPPORT_RESERVED_VIDEO_MEMORY
	if (vb->base) 
		vmem_free(&s_vmem, vb->phys_addr, 0);
#else
#ifdef WMT
	if (vb->base) {
		if (vpu_mem_debug) {
			printk(KERN_ERR "[VPUDRV] free pa %08lx, va %08lx, len %d\n",
						(long unsigned int)vb->phys_addr, vb->base, vb->size);
		}
		if (vpu_use_dma_alloc) {
			struct device *dev = vpu_mem_is_valid ? &vpu_device.dev : NULL;
			if (dev == NULL) return;
			dma_free_coherent(dev, PAGE_ALIGN(vb->size), (void *)vb->base, vb->phys_addr);
		} else
			mb_free(vb->phys_addr);
    }
#else
	if (vb->base) 
		dma_free_coherent(0, PAGE_ALIGN(vb->size), (void *)vb->base, vb->phys_addr);
#endif
#endif
}

static int vpu_free_buffers(void)
{
	vpudrv_buffer_pool_t *pool, *n;
	vpudrv_buffer_t vb;

	list_for_each_entry_safe(pool, n, &s_vbp_head, list) 
	{
		vb = pool->vb;
		if (vb.base) 
		{
			vpu_free_dma_buffer(&vb);
			list_del(&pool->list);
			kfree(pool);
		}
	}

	return 0;
}

#ifdef VPU_SUPPORT_ISR
static irqreturn_t vpu_irq_handler(int irq, void *dev_id)
{
	vpu_drv_context_t *dev = (vpu_drv_context_t *)dev_id;

	// this can be removed. it also work in VPU_WaitInterrupt of API function
	int core;

	//printk(KERN_DEBUG "[VPUDRV] vpu_irq_handler\n");	

	for (core=0; core<MAX_NUM_VPU_CORE; core++)
	{
		if (s_bit_firmware_info[core].size == 0) // it means that we didn't get an information the current core from API layer. No core activated.
			continue;
		
		if (ReadVpuRegister(BIT_INT_STS)) {
			WriteVpuRegister(BIT_INT_CLEAR, 0x1); 
		}		
	}
	
	if (dev->async_queue)
		kill_fasync(&dev->async_queue, SIGIO, POLL_IN);	// notify the interrupt to user space
	
	s_interrupt_flag = 1;
	
	wake_up_interruptible(&s_interrupt_wait_q);
#ifdef WMT
	if (s_pending_instance != -1) {
		wmt_codec_timer_stop(s_codec_tm[s_pending_instance]);
		s_pending_instance = -1;
	}
	mutex_unlock(&wmt_suspend_resume_lock);
#endif
	return IRQ_HANDLED;
}
#endif

static int vpu_open(struct inode *inode, struct file *filp)
{
#ifdef WMT
	msvd_ctx_t *ctx;

	ctx = kmalloc(sizeof(msvd_ctx_t), GFP_KERNEL); 
	if (ctx == 0) {
		printk(KERN_ERR "alloc %d bytes for context fail\n", sizeof(msvd_ctx_t));
		return -ENOMEM;
	}
	spin_lock(&s_vpu_lock);
	s_vpu_open_count++;
	ctx->dev = (void *)(&s_vpu_drv_context);
	ctx->pid = current->pid;
	ctx->tgid = current->tgid;
	filp->private_data = (void *)ctx;
	spin_unlock(&s_vpu_lock);
	if (s_vpu_open_count == 1) {
		wmt_codec_pmc_ctl(CODEC_VD_MSVD, 1);
		wmt_codec_clock_en(CODEC_VD_MSVD, 1);
		wmt_codec_msvd_reset(CODEC_VD_MSVD);
#ifdef VPU_SUPPORT_CLOCK_CONTROL
		wmt_codec_clock_en(CODEC_VD_MSVD, 0);
#endif
    }
	printk(KERN_INFO "[VPUDRV] : opened. tgid %d pid %d\n", ctx->tgid, ctx->pid);
#else
	spin_lock(&s_vpu_lock);

	s_vpu_open_count++;

	filp->private_data = (void *)(&s_vpu_drv_context);
	spin_unlock(&s_vpu_lock);
#endif
	return 0;
}

//static int vpu_ioctl(struct inode *inode, struct file *filp, u_int cmd, u_long arg) // for kernel 2.6.9 of C&M
static long vpu_ioctl(struct file *filp, u_int cmd, u_long arg)
{
	int ret = 0; 
	switch (cmd) 
	{
	case VDI_IOCTL_ALLOCATE_PHYSICAL_MEMORY:
		{
			vpudrv_buffer_pool_t *vbp;

			vbp = kzalloc(sizeof(*vbp), GFP_KERNEL);
			if (!vbp)
				return -ENOMEM;

			ret = copy_from_user(&(vbp->vb), (vpudrv_buffer_t *)arg, sizeof(vpudrv_buffer_t));
			if (ret) 
			{
				kfree(vbp);
				return -EFAULT;
			}

			ret = vpu_alloc_dma_buffer(&(vbp->vb));
			if (ret == -1) 
			{
				ret = -ENOMEM;
				kfree(vbp);
				break;
			}			
			ret = copy_to_user((void __user *)arg, &(vbp->vb), sizeof(vpudrv_buffer_t));
			if (ret) 
			{
				kfree(vbp);
				ret = -EFAULT;
				break;
			}

			spin_lock(&s_vpu_lock);
			list_add(&vbp->list, &s_vbp_head);
			spin_unlock(&s_vpu_lock);		

		}
		break;
	case VDI_IOCTL_FREE_PHYSICALMEMORY:
		{

			vpudrv_buffer_pool_t *vbp, *n;
			vpudrv_buffer_t vb;

			ret = copy_from_user(&vb, (vpudrv_buffer_t *)arg, sizeof(vpudrv_buffer_t));
			if (ret)
				return -EACCES;

			if (vb.base) 
				vpu_free_dma_buffer(&vb);

			spin_lock(&s_vpu_lock);
			list_for_each_entry_safe(vbp, n, &s_vbp_head, list) 
			{
				if (vbp->vb.base == vb.base) 
				{
					list_del(&vbp->list);
					kfree(vbp);
					break;
				}
			}
			spin_unlock(&s_vpu_lock);

		}
		break;
#ifdef VPU_SUPPORT_RESERVED_VIDEO_MEMORY
	case VDI_IOCTL_GET_RESERVED_VIDEO_MEMORY_INFO:
		{
			spin_lock(&s_vpu_lock);
			if (s_video_memory.base != 0) 
			{
				ret = copy_to_user((void __user *)arg, &s_video_memory, sizeof(vpudrv_buffer_t));
				if (ret != 0) 
					ret = -EFAULT;
			} 
			else
			{
				ret = -EFAULT;
			}
			spin_unlock(&s_vpu_lock);
		}
		break;
#endif
	case VDI_IOCTL_WAIT_INTERRUPT:
		{
			u32 timeout = (u32) arg;
			if (!wait_event_interruptible_timeout(s_interrupt_wait_q, s_interrupt_flag != 0, msecs_to_jiffies(timeout))) 
			{
#ifdef WMT
				vd_dump_registers();
				mutex_unlock(&wmt_suspend_resume_lock);
#endif
				ret = -ETIME;
				break;
			} 
			
			if (signal_pending(current)) 
			{
				ret = -ERESTARTSYS;
				break;
			} 
			
			s_interrupt_flag = 0;			
		}
		break;
		
	case VDI_IOCTL_SET_CLOCK_GATE:
		{
			u32 clkgate;

			if (get_user(clkgate, (u32 __user *) arg))
				return -EFAULT;
#ifdef VPU_SUPPORT_CLOCK_CONTROL
			if (clkgate)
				vpu_clk_enable(s_vpu_clk);
			else
				vpu_clk_disable(s_vpu_clk);
#endif

		}
		break;
	case VDI_IOCTL_GET_INSTANCE_POOL:
		{
			if (s_instance_pool.base != 0) 
			{
				ret = copy_to_user((void __user *)arg, &s_instance_pool, sizeof(vpudrv_buffer_t));
				if (ret != 0) 
					ret = -EFAULT;
			} 
			else
			{
				ret = copy_from_user(&s_instance_pool, (vpudrv_buffer_t *)arg, sizeof(vpudrv_buffer_t));
				if (ret == 0)
				{
					spin_lock(&s_vpu_lock);
					if (vpu_alloc_dma_buffer(&s_instance_pool) != -1)
					{
						memset((void *)s_instance_pool.base, 0x0, s_instance_pool.size); 
						ret = copy_to_user((void __user *)arg, &s_instance_pool, sizeof(vpudrv_buffer_t));
						if (ret == 0)
						{
							// success to get memory for instance pool
							spin_unlock(&s_vpu_lock);
							break;
						}
					}
					spin_unlock(&s_vpu_lock);
				}
				
				ret = -EFAULT;
			}
			
		}
		break;
	case VDI_IOCTL_GET_COMMON_MEMORY:
		{
			if (s_common_memory.base != 0) 
			{
				ret = copy_to_user((void __user *)arg, &s_common_memory, sizeof(vpudrv_buffer_t));
				if (ret != 0) 
					ret = -EFAULT;
			} 
			else
			{
				ret = copy_from_user(&s_common_memory, (vpudrv_buffer_t *)arg, sizeof(vpudrv_buffer_t));
				if (ret == 0)
				{
					spin_lock(&s_vpu_lock);
					if (vpu_alloc_dma_buffer(&s_common_memory) != -1)
					{
						ret = copy_to_user((void __user *)arg, &s_common_memory, sizeof(vpudrv_buffer_t));
						if (ret == 0)
						{
							// success to get memory for common memory
							spin_unlock(&s_vpu_lock);
							break;
						}
					}	
					spin_unlock(&s_vpu_lock);
				}

				ret = -EFAULT;
			}

		}
		break;
	case VDI_IOCTL_RESET:
		{
			vpu_hw_reset();
		}
		break;
#ifdef WMT
	// without lock because this ioctl is called inside Instance Lock
	case VDI_IOCTL_START_TIME_DECODE:
		{
			u32 instIndex = (u32) arg;
			s_pending_instance = instIndex;
			if (!s_codec_tm[instIndex])
				wmt_codec_timer_init(&s_codec_tm[instIndex], "<msvd>", 0 /*frames*/, 45 /*ms*/);
			wmt_codec_timer_start(s_codec_tm[instIndex]);
			mutex_lock(&wmt_suspend_resume_lock);
		}
		break;
	// without lock because this ioctl is called inside Instance Lock
	case VDI_IOCTL_STOP_TIME_DECODE:
		{
			u32 instIndex = (u32) arg;
			if (s_codec_tm[instIndex])
				wmt_codec_timer_exit(s_codec_tm[instIndex]);
			s_codec_tm[instIndex] = NULL;
		}
		break;
#endif
	default:
		{
			printk(KERN_ERR "[VPUDRV] No such IOCTL, cmd is %d\n", cmd);
		}
		break;
	}
	return ret;
}

static ssize_t vpu_read(struct file *filp, char __user *buf, size_t len, loff_t *ppos)
{
	return -1;
}

static ssize_t vpu_write(struct file *filp, const char __user *buf, size_t len, loff_t *ppos)
{
	//printk(KERN_DEBUG "[VPUDRV] vpu_write\n");

	if (len == sizeof(vpu_bit_firmware_info_t))
	{
		vpu_bit_firmware_info_t *bit_firmware_info = (vpu_bit_firmware_info_t *)buf;

		//printk(KERN_DEBUG "[VPUDRV] vpu_write set bit_firmware_info coreIdx=0x%x, reg_base_offset=0x%x size=0x%x\n",bit_firmware_info->core_idx, (int)bit_firmware_info->reg_base_offset, bit_firmware_info->size);	

		if (bit_firmware_info->core_idx > MAX_NUM_VPU_CORE) 
		{
			printk(KERN_ERR "[VPUDRV] VDI_IOCTL_SET_BIT_FIRMWARE Device Not Found, coreIdx=%d\n", bit_firmware_info->core_idx);
			return -ENODEV;
		}
		memcpy((void *)&s_bit_firmware_info[bit_firmware_info->core_idx], bit_firmware_info, sizeof(vpu_bit_firmware_info_t));
		return len;
	}

	return -1;
}

static int vpu_release(struct inode *inode, struct file *filp)
{
	//printk(KERN_DEBUG "[VPUDRV] vpu_release\n");

	spin_lock(&s_vpu_lock);
	s_vpu_open_count--;

	if (s_vpu_open_count <= 0) 
	{
		/* found and free the not handled buffer by user applications */
		vpu_free_buffers();		
#ifdef WMT
#ifdef VPU_SUPPORT_CLOCK_CONTROL
#else
		wmt_codec_clock_en(CODEC_VD_MSVD, 0);
#endif
		wmt_codec_pmc_ctl(CODEC_VD_MSVD, 0);
#endif
	}

	spin_unlock(&s_vpu_lock);

#ifdef WMT
{
	msvd_ctx_t *ctx = filp->private_data;
	if (s_instance_pool.base) {
		msvd_instance_pool_layout_t *layout = instancepool_layout();
		int *pool_inited, *instance_num, *pendingInst, *mutex;
		int init = 0, i = 0;
		int msvd_clock_count = 0;
		dmac_flush_range((void *)s_instance_pool.base, (void *)(s_instance_pool.base + s_instance_pool.size));
		outer_flush_range(s_instance_pool.phys_addr, s_instance_pool.phys_addr + s_instance_pool.size);
		offsetof_instancepool(instance_pool_inited, int, pool_inited);
		offsetof_instancepool(vpu_instance_num, int, instance_num);
		offsetof_instancepool(pendingInst, int, pendingInst);
		mutex = instance_num - 3;
//		printk(KERN_DEBUG "[VPUDRV-IN] layout %p pool_inited %d instance_num %d mutex %d pendingInst %d\n",
//			layout, (pool_inited)?*pool_inited:-1, (instance_num)?*instance_num:-1,
//			(instance_num)?*mutex:-1, (pendingInst)?*pendingInst:-1);
		if (layout && pool_inited && *pool_inited && instance_num && *instance_num)
			init = 1;
		for (i = 0; init && i < MAX_NUM_INSTANCE; i++) {
			int* inUse;
			pid_t *pid, *tgid;
			offsetof_instance(i, inUse, int, inUse);
			offsetof_instance(i, pid, pid_t, tgid);
			offsetof_instance(i, tid, pid_t, pid);
			printk(KERN_DEBUG "[VPUDRV] inst %d inUse %d tgid %d/%d pid %d/%d\n", i,
				(inUse)?*inUse:-1, (tgid)?*tgid:-1, ctx->tgid, (pid)?*pid:-1, ctx->pid);
			if (!inUse || !tgid || !pid)
				continue;
			if (*inUse && *tgid == ctx->tgid /* && *pid == ctx->pid */) {
				printk(KERN_ERR "[VPUDRV] force recycle inst %d inUse %d tgid %d/%d pid %d/%d\n", i,
					(inUse)?*inUse:-1, (tgid)?*tgid:-1, ctx->tgid, (pid)?*pid:-1, ctx->pid);
				*inUse = 0;
				*pid = *tgid = 0;
				*instance_num = *instance_num - 1;
			}
		}
		msvd_clock_count = wmt_get_codec_clock_count();
		if (instance_num && (*instance_num) == 0) {
#ifdef VPU_SUPPORT_CLOCK_CONTROL
			if (msvd_clock_count) {
				printk(KERN_ERR "[VPUDRV] no instance. force disable msvd clock\n");
				for (i = 0; i < msvd_clock_count; i++)
					vpu_clk_disable(s_vpu_clk);
				wmt_reset_codec_clock_count();
			}
#endif
			*pool_inited = 0;
			*pendingInst = 0;
		}
		if ( msvd_clock_count > 0 ) {
			printk(KERN_DEBUG "[VPUDRV-OUT] layout %p pool_inited %d instance_num %d mutex %d pendingInst %d\n",
				layout, (pool_inited)?*pool_inited:-1, (instance_num)?*instance_num:-1,
				(instance_num)?*mutex:-1, (pendingInst)?*pendingInst:-1);
			printk(KERN_DEBUG "[VPUDRV] Clock Count %d (%08x %08x %08x %08x)\n", msvd_clock_count,
				REG32_VAL(MSVD_BASE_ADDR+0x000), REG32_VAL(MSVD_BASE_ADDR+0x004),
				REG32_VAL(MSVD_BASE_ADDR+0x008), REG32_VAL(MSVD_BASE_ADDR+0x00C));
		}
		dmac_flush_range((void *)s_instance_pool.base, (void *)(s_instance_pool.base + s_instance_pool.size));
		outer_flush_range(s_instance_pool.phys_addr, s_instance_pool.phys_addr + s_instance_pool.size);
	}
	printk(KERN_INFO "[VPUDRV] : closed. tgid %d pid %d\n", ctx->tgid, ctx->pid);

	if (ctx)
		kfree(ctx);
}
#endif

	return 0;
}


static int vpu_fasync(int fd, struct file *filp, int mode)
{
	struct vpu_drv_context_t *dev = (struct vpu_drv_context_t *)filp->private_data;
	return fasync_helper(fd, filp, mode, &dev->async_queue);
}


static int vpu_map_to_register(struct file *fp, struct vm_area_struct *vm)
{
	unsigned long pfn;

	vm->vm_flags |= VM_IO | VM_RESERVED;
	vm->vm_page_prot = pgprot_noncached(vm->vm_page_prot);
	pfn = s_vpu_reg_phy_addr >> PAGE_SHIFT;

	return remap_pfn_range(vm, vm->vm_start, pfn, vm->vm_end-vm->vm_start, vm->vm_page_prot) ? -EAGAIN : 0;
}


static int vpu_map_to_physical_memory(struct file *fp, struct vm_area_struct *vm)
{
	vm->vm_flags |= VM_IO | VM_RESERVED;
	vm->vm_page_prot = pgprot_writecombine(vm->vm_page_prot);

	return remap_pfn_range(vm, vm->vm_start, vm->vm_pgoff, vm->vm_end-vm->vm_start, vm->vm_page_prot) ? -EAGAIN : 0;
}
static int vpu_map_to_instance_pool_memory(struct file *fp, struct vm_area_struct *vm)
{
#ifdef WMT
	vm->vm_flags |= VM_IO | VM_RESERVED;
	vm->vm_page_prot =__pgprot_modify(vm->vm_page_prot, L_PTE_MT_MASK, 
		L_PTE_MT_BUFFERABLE | L_PTE_MT_WRITEBACK | L_PTE_XN);
#endif

	return remap_pfn_range(vm, vm->vm_start, vm->vm_pgoff, vm->vm_end-vm->vm_start, vm->vm_page_prot) ? -EAGAIN : 0;
}
/*!
 * @brief memory map interface for vpu file operation
 * @return  0 on success or negative error code on error
 */
static int vpu_mmap(struct file *fp, struct vm_area_struct *vm)
{
	if (vm->vm_pgoff) 
	{
		if(vm->vm_pgoff == (s_instance_pool.phys_addr>>PAGE_SHIFT))
			return  vpu_map_to_instance_pool_memory(fp, vm);
	
		return vpu_map_to_physical_memory(fp, vm);
	}
	else
		return vpu_map_to_register(fp, vm);
}

struct file_operations vpu_fops = {
	.owner = THIS_MODULE,
	.open = vpu_open,
	.read = vpu_read,
	.write = vpu_write,
	//.ioctl = vpu_ioctl, // for kernel 2.6.9 of C&M
	.unlocked_ioctl = vpu_ioctl,
	.release = vpu_release,
	.fasync = vpu_fasync,
	.mmap = vpu_mmap,
};




static int vpu_probe(struct platform_device *pdev)
{
	int err = 0;
	struct resource *res=NULL;
	//printk(KERN_INFO "[VPUDRV] vpu_probe\n");
#ifdef WMT
	if (pdev) {
		struct device *dev = &pdev->dev;
		if (vpu_mem_start && vpu_mem_size) {
			vpu_mem_is_valid = dma_declare_coherent_memory(
				dev, vpu_mem_start, vpu_mem_start, vpu_mem_size,
				DMA_MEMORY_MAP | DMA_MEMORY_IO | DMA_MEMORY_EXCLUSIVE);
			printk(KERN_INFO "[VPUDRV] : dma_declare_coherent_memory %08lx-%08lx (%d)\n",
				vpu_mem_start, vpu_mem_start + vpu_mem_size - 1, vpu_mem_is_valid);
		}
	}
#endif
	if(pdev)
		res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (res) 	// if platform driver is implemented
	{
		s_vpu_reg_phy_addr = res->start;
		s_vpu_reg_virt_addr = ioremap(res->start, res->end - res->start);
		printk(KERN_DEBUG "[VPUDRV] : vpu base address get from platform driver physical base addr=0x%x, virtual base=0x%x\n", (int)s_vpu_reg_phy_addr , (int)s_vpu_reg_virt_addr);		
	}
	else
	{
		s_vpu_reg_virt_addr = ioremap(s_vpu_reg_phy_addr, VPU_REG_SIZE);
		printk(KERN_DEBUG "[VPUDRV] : vpu base address get from defined value physical base addr=0x%x, virtual base=0x%x\n", (int)s_vpu_reg_phy_addr, (int)s_vpu_reg_virt_addr);
		
	}

#ifdef WMT
{
	int ret = 0;

	s_vpu_clk = vpu_clk_get(NULL); // always try to get s_vpu_clk
	if (!s_vpu_clk) {
		printk(KERN_INFO "fail to get clock controller, but, do not treat as error\n");
	} else {
		printk(KERN_INFO "get clock controller s_vpu_clk=0x%x\n", (int)s_vpu_clk);
	}
	s_vpu_major = MKDEV(VD_MAJOR, MSVD_MINOR);
	ret = register_chrdev_region(s_vpu_major, 1, "msvd");
	if (ret != 0) {
		printk(KERN_ERR "[VPUDRV] : get device region fail (%d)\n", VD_MAJOR);
		return ret;
	}
}
#else
	/* get the major number of the character device */  
	if ((alloc_chrdev_region(&s_vpu_major, 0, 1, VPU_DEV_NAME)) < 0) 
	{  
		err = -EBUSY;
		printk(KERN_ERR "could not allocate major number\n");  		
		goto ERROR_PROVE_DEVICE;
	}
#endif

	/* initialize the device structure and register the device with the kernel */  
	cdev_init(&s_vpu_cdev, &vpu_fops);  
	if ((cdev_add(&s_vpu_cdev, s_vpu_major, 1)) < 0) 
	{  
		err = -EBUSY;
		printk(KERN_ERR "could not allocate chrdev\n"); 
    #ifdef WMT
        unregister_chrdev_region(s_vpu_major, 1);
    #endif
		goto ERROR_PROVE_DEVICE;  
	}  

#ifdef WMT
	/* let udev to handle /dev/ node */
	msvd_class = class_create(THIS_MODULE, "msvd");
	device_create(msvd_class, NULL, s_vpu_major, NULL, "%s", "msvd");
#else
	if (pdev)
		s_vpu_clk = vpu_clk_get(&pdev->dev);
	else 
		s_vpu_clk = vpu_clk_get(NULL);
	
	if (!s_vpu_clk) 
		printk(KERN_ERR "[VPUDRV] : fail to get clock controller, but, do not treat as error, \n");
	else
		printk(KERN_INFO "[VPUDRV] : get clock controller s_vpu_clk=0x%x\n", (int)s_vpu_clk);					

#ifdef VPU_SUPPORT_CLOCK_CONTROL	
#else
	vpu_clk_enable(s_vpu_clk);	
#endif
#endif

#ifdef VPU_SUPPORT_ISR
	if(pdev)
		res = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
	if (res) 	// if platform driver is implemented
	{
		s_vpu_irq = res->start;
		printk(KERN_DEBUG "[VPUDRV] : vpu irq number get from platform driver irq=0x%x\n", s_vpu_irq );		
	}
	else
	{
		printk(KERN_DEBUG "[VPUDRV] : vpu irq number get from defined value irq=0x%x\n", s_vpu_irq );
	}


	err = request_irq(s_vpu_irq, vpu_irq_handler, 0, "VPU_CODEC_IRQ", (void *)(&s_vpu_drv_context));
	if (err)
	{
		printk(KERN_ERR "[VPUDRV] :  fail to register interrupt handler\n");
		goto ERROR_PROVE_DEVICE;
	}
#endif
	
	
#ifdef VPU_SUPPORT_RESERVED_VIDEO_MEMORY
	s_video_memory.size = VPU_INIT_VIDEO_MEMORY_SIZE_IN_BYTE;
	s_video_memory.phys_addr = VPU_DRAM_PHYSICAL_BASE;
	s_video_memory.base = (unsigned long)ioremap(s_video_memory.phys_addr, PAGE_ALIGN(s_video_memory.size));
	if (!s_video_memory.base)
	{	
		printk(KERN_ERR "[VPUDRV] :  fail to remap video memory physical phys_addr=0x%x, base=0x%x, size=%d\n", (int)s_video_memory.phys_addr, (int)s_video_memory.base, (int)s_video_memory.size);
		goto ERROR_PROVE_DEVICE;
	}
	
	if (vmem_init(&s_vmem, s_video_memory.phys_addr, s_video_memory.size) < 0)
	{
		printk(KERN_ERR "[VPUDRV] :  fail to init vmem system\n");
		goto ERROR_PROVE_DEVICE;
	}
	printk(KERN_INFO "[VPUDRV] success to probe vpu device with reserved video memory phys_addr=0x%x, base = 0x%x\n",(int) s_video_memory.phys_addr, (int)s_video_memory.base);
#else
	printk(KERN_INFO "[VPUDRV] success to probe vpu device with non reserved video memory\n");
#endif	
	
	
	return 0;


ERROR_PROVE_DEVICE:
	
	if (s_vpu_major)
		unregister_chrdev_region(s_vpu_major, 1);		

	if (s_vpu_reg_virt_addr)
		iounmap(s_vpu_reg_virt_addr);

	return err;
}

static int vpu_remove(struct platform_device *pdev)
{
#ifdef VPU_SUPPORT_PLATFORM_DRIVER_REGISTER	

	printk(KERN_INFO "[VPUDRV] vpu_remove\n");
	if (s_instance_pool.base) 
	{
		vpu_free_dma_buffer(&s_instance_pool);
		s_instance_pool.base = 0;
	}


	if (s_common_memory.base)
	{
		vpu_free_dma_buffer(&s_common_memory);
		s_common_memory.base = 0;
	}

#ifdef VPU_SUPPORT_RESERVED_VIDEO_MEMORY
	if (s_video_memory.base) 
	{
		iounmap((void *)s_video_memory.base);
		s_video_memory.base = 0;

		vmem_exit(&s_vmem);		
	}
#endif

#ifdef WMT
	if ( pdev && vpu_mem_is_valid)
		dma_release_declared_memory(&pdev->dev);

	/* let udev to handle /dev/ node */
	device_destroy(msvd_class, s_vpu_major);
	class_destroy(msvd_class);
#endif
	cdev_del(&s_vpu_cdev);  
	if (s_vpu_major > 0) 
	{
		unregister_chrdev_region(s_vpu_major, 1);
		s_vpu_major = 0;
	}

#ifdef VPU_SUPPORT_ISR	
	if (s_vpu_irq)
		free_irq(s_vpu_irq, &s_vpu_drv_context);
#endif

	if (s_vpu_reg_virt_addr)
		iounmap(s_vpu_reg_virt_addr);

	vpu_clk_put(s_vpu_clk);
#endif //VPU_SUPPORT_PLATFORM_DRIVER_REGISTER
	return 0;
}

#ifdef CONFIG_PM
static int vpu_suspend(struct platform_device *pdev, pm_message_t state)
{
	int i;
	int core;
	unsigned long timeout = jiffies + HZ;	// vpu wait timeout to 1sec

	printk(KERN_INFO "[VPUDRV] vpu_suspend\n");
#ifdef WMT
	mutex_lock(&wmt_suspend_resume_lock);
#endif
	if (s_vpu_open_count > 0) 
	{
		vpu_clk_enable(s_vpu_clk);

		for (core=0; core<MAX_NUM_VPU_CORE; core++)
		{
			if (s_bit_firmware_info[core].size == 0)
				continue;

			while (ReadVpuRegister(BIT_BUSY_FLAG)) 
			{
				if (time_after(jiffies, timeout))
					goto DONE_SUSPEND;
			}
		}
#ifdef WMT // Move code here to ensure clk is enabled before reg_store
		for (core=0; core<MAX_NUM_VPU_CORE; core++)
		{
			if (s_bit_firmware_info[core].size == 0)
				continue;
			for ( i = 0 ; i < 64 ; i++)
				s_vpu_reg_store[core][i] = ReadVpuRegister(BIT_BASE+(0x100+(i * 4)));
		}
#endif
		vpu_clk_disable(s_vpu_clk);
#ifdef WMT
		wmt_clk_cnt = wmt_get_codec_clock_count();
		for (i = 0; i < wmt_clk_cnt; i++) {
			printk(KERN_DEBUG "vpu suspend clk %d\n", i);
			wmt_codec_clock_en(CODEC_VD_MSVD, 0);
		}
		wmt_codec_pmc_ctl(CODEC_VD_MSVD, 0);
#endif
	}
	return 0;

DONE_SUSPEND:
	vpu_clk_disable(s_vpu_clk);
#ifdef WMT
	mutex_unlock(&wmt_suspend_resume_lock);
#endif
	return -EAGAIN;

}

static int vpu_resume(struct platform_device *pdev)
{
	int i;
	int core;
	u32 val;
	unsigned long timeout = jiffies + HZ;	// vpu wait timeout to 1sec

	printk(KERN_INFO "[VPUDRV] vpu_resume\n");
#ifdef WMT
	if (s_vpu_open_count <= 0) {
		goto EXIT_vpu_resume;
	}
	wmt_codec_pmc_ctl(CODEC_VD_MSVD, 1);
	for (i = 0; i < wmt_clk_cnt; i++){
		printk(KERN_DEBUG "vpu resume clk %d\n", i);
		wmt_codec_clock_en(CODEC_VD_MSVD, 1);
	}
	wmt_codec_clock_en(CODEC_VD_MSVD, 1);
	wmt_codec_msvd_reset(CODEC_VD_MSVD);
	wmt_codec_clock_en(CODEC_VD_MSVD, 0);
#endif

	vpu_clk_enable(s_vpu_clk);
	
	for (core=0; core<MAX_NUM_VPU_CORE; core++)
	{
		if (s_bit_firmware_info[core].size == 0)
			continue;
		
		WriteVpuRegister(BIT_CODE_RUN, 0);

		//---- LOAD BOOT CODE
		for( i = 0; i <512; i++ ) 
		{
			val = s_bit_firmware_info[core].bit_code[i];
			WriteVpuRegister(BIT_CODE_DOWN, ((i << 16) | val));			
		}

		for (i = 0 ; i < 64 ; i++) 
			WriteVpuRegister(BIT_BASE+(0x100+(i * 4)), s_vpu_reg_store[core][i]);

		WriteVpuRegister(BIT_BUSY_FLAG, 1);
		WriteVpuRegister(BIT_CODE_RESET, 1);
		WriteVpuRegister(BIT_CODE_RUN, 1);

		while (ReadVpuRegister(BIT_BUSY_FLAG)) 
		{
			if (time_after(jiffies, timeout))
				goto DONE_WAKEUP;
		}
	}
#ifdef WMT
#else
	if (s_vpu_open_count == 0)
		vpu_clk_disable(s_vpu_clk);
#endif

DONE_WAKEUP:

#ifdef WMT
	vpu_clk_disable(s_vpu_clk);
EXIT_vpu_resume:
	mutex_unlock(&wmt_suspend_resume_lock);
#else
	if (s_vpu_open_count > 0)
		vpu_clk_enable(s_vpu_clk);
#endif

	return 0;
}
#else
#define	vpu_suspend	NULL
#define	vpu_resume	NULL
#endif				/* !CONFIG_PM */

static struct platform_driver vpu_driver = {
	.driver = {
		   .name = VPU_PLATFORM_DEVICE_NAME,
		   },
	.probe = vpu_probe,
	.remove = vpu_remove,
	.suspend = vpu_suspend,
	.resume = vpu_resume,
};


static int __init vpu_init(void)
{
	int res;

	//printk(KERN_INFO "[VPUDRV] begin vpu_init\n");	
	init_waitqueue_head(&s_interrupt_wait_q);
	s_common_memory.base = 0;
	s_instance_pool.base = 0;
#ifdef WMT
	platform_device_register(&vpu_device);
	mutex_init(&wmt_suspend_resume_lock);
#endif
#ifdef VPU_SUPPORT_PLATFORM_DRIVER_REGISTER
	res = platform_driver_register(&vpu_driver);
#else
	res = platform_driver_register(&vpu_driver);
	res = vpu_probe(NULL);
#endif
	//printk(KERN_INFO "[VPUDRV] end vpu_init result=0x%x\n", res);	
	return res;
}

static void __exit vpu_exit(void)
{
//#ifdef VPU_SUPPORT_PLATFORM_DRIVER_REGISTER	
	printk(KERN_INFO "[VPUDRV] vpu_exit\n");
//#else	
	
	vpu_clk_put(s_vpu_clk);

	if (s_instance_pool.base) 
	{
		vpu_free_dma_buffer(&s_instance_pool);
		s_instance_pool.base = 0;
	}


	if (s_common_memory.base)
	{
		vpu_free_dma_buffer(&s_common_memory);
		s_common_memory.base = 0;
	}

#ifdef VPU_SUPPORT_RESERVED_VIDEO_MEMORY
	if (s_video_memory.base) 
	{
		iounmap((void *)s_video_memory.base);
		s_video_memory.base = 0;

		vmem_exit(&s_vmem);
	}
#endif

	cdev_del(&s_vpu_cdev);  
	if (s_vpu_major > 0) 
	{
		unregister_chrdev_region(s_vpu_major, 1);
		s_vpu_major = 0;
	}

#ifdef VPU_SUPPORT_ISR	
	if (s_vpu_irq)
		free_irq(s_vpu_irq, &s_vpu_drv_context);
#endif

	if (s_vpu_reg_virt_addr) {
		iounmap(s_vpu_reg_virt_addr);
		s_vpu_reg_virt_addr = (void *)0x00;
	}

//#endif

	platform_driver_unregister(&vpu_driver);
#ifdef WMT
	platform_device_unregister(&vpu_device);
#endif
	return;
}

MODULE_AUTHOR("WonderMedia using C&M VPU, Inc.");
MODULE_DESCRIPTION("VPU linux driver");
MODULE_LICENSE("GPL");

module_init(vpu_init);
module_exit(vpu_exit);

int vpu_hw_reset(void)
{
#ifdef WMT
#ifdef VPU_SUPPORT_CLOCK_CONTROL
	wmt_codec_clock_en(CODEC_VD_MSVD, 1);
	wmt_codec_msvd_reset(CODEC_VD_MSVD);
	wmt_codec_clock_en(CODEC_VD_MSVD, 0);
#else
	wmt_codec_msvd_reset(CODEC_VD_MSVD);
#endif
#else
	printk(KERN_INFO "[VPUDRV] request vpu reset from application. \n");
#endif
	return 0;
}

#ifdef WMT
struct clk {
	unsigned int enabled;
};
struct clk *clk_get(struct device *dev, const char *id)
{
	struct clk *clk = (struct clk *)kmalloc(sizeof(struct clk), GFP_KERNEL);
	if (clk) memset(clk, 0, sizeof(struct clk));
	return clk;
}
void clk_put(struct clk *clk)
{
	if (clk) kfree(clk);
}
int clk_enable(struct clk *clk)
{
	wmt_codec_clock_en(CODEC_VD_MSVD, 1);
	return 0;
}
void clk_disable(struct clk *clk)
{
	wmt_codec_clock_en(CODEC_VD_MSVD, 0);
}
#endif

struct clk *vpu_clk_get(struct device *dev)
{
	return clk_get(dev, VPU_CLK_NAME);
}
void vpu_clk_put(struct clk *clk)
{
	clk_put(clk);
}
int vpu_clk_enable(struct clk *clk)
{

	if (clk) {
#ifdef WMT
#else
		// the bellow is for C&M EVB.
		{
			struct clk *s_vpuext_clk = NULL;
			s_vpuext_clk = clk_get(NULL, "vcore");
			printk(KERN_INFO "[VPUDRV] vcore clk=%p\n", s_vpuext_clk);	
			if (s_vpuext_clk)
				clk_enable(s_vpuext_clk);

			s_vpuext_clk = clk_get(NULL, "vbus");
			printk(KERN_INFO "[VPUDRV] vbus clk=%p\n", s_vpuext_clk);	
			if (s_vpuext_clk)
				clk_enable(s_vpuext_clk);
		}
		// for C&M EVB.
#endif
		//printk(KERN_DEBUG "[VPUDRV] vpu_clk_enable\n");	
		return clk_enable(clk);
	}

	return 0;
}

void vpu_clk_disable(struct clk *clk)
{
	if (clk) {
		//printk(KERN_DEBUG "[VPUDRV] vpu_clk_disable\n");	
		clk_disable(clk);
	}
}
