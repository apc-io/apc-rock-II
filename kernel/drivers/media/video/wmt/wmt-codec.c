/*++
 * Common interface for WonderMedia SoC hardware encoder and decoder drivers
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
#ifndef CODEC_C
#define CODEC_C

#include <linux/device.h>       /* For EXPORT_SYMBOL() */
#include <linux/spinlock.h>
#include <mach/hardware.h>
#include <linux/module.h>
#include <linux/slab.h>         /* for kmalloc and kfree */
#include <linux/semaphore.h>    /* for semaphore */

#include "wmt-codec.h"

#define CFG_CODEC_PERFORM_EN    /* Flag for codec performance analysis */


/*#define CODEC_DEBUG*/
#undef CODEC_DEBUG
#ifdef CODEC_DEBUG
#define DBG_MSG(fmt, args...) \
	do {\
		printk(KERN_INFO "{%s} " fmt, __func__, ## args);\
	} while (0)
#else
#define DBG_MSG(fmt, args...)
#endif

#undef DBG_ERR
#define DBG_ERR(fmt, args...) \
	do {\
		printk(KERN_ERR "*E* {%s} " fmt, __func__, ## args);\
	} while (0)

#undef PRINTK
#define PRINTK(fmt, args...)  printk(KERN_INFO "{%s} " fmt, __func__, ## args)

/*#define CODEC_REG_TRACE*/
#ifdef CODEC_REG_TRACE
#define REG_WRITE(addr, val)      \
	do {\
		PRINTK("REG_SET:0x%x -> 0x%0x\n", addr, val);\
		REG32_VAL(addr) = (val);\
	} while (0)
#else
#define REG_WRITE(addr, val)      (REG32_VAL(addr) = (val))
#endif


struct codec_lock {
	struct semaphore  sem;
	unsigned int      is_init;
	unsigned int      is_locked;
};

static struct codec_lock codec_lock[CODEC_MAX];
static int msvd_clk_enable_cnt;
DEFINE_MUTEX(wmt_clk_mutex);


#ifdef CFG_CODEC_PERFORM_EN
static int wmt_codec_debug;

struct wmt_tm {
	unsigned long  seq_id;          /* seqence number */
	char           name[16];        /* name */
	unsigned int   initial;         /* initial flag */
	unsigned int   total_tm;        /* total time */
	unsigned int   interval_tm;     /* interval time */
	unsigned int   reset;           /* reset counter */
	unsigned int   total_cnt;       /* total counter */
	unsigned int   count;           /* interval counter */
	unsigned int   max;             /* max time */
	unsigned int   min;             /* min time */
	unsigned int   threshold;
	struct timeval start;           /* start time */
	struct timeval end;             /* end time */
} ;
#endif

/*!*************************************************************************
* wmt_power_en
*
* Private Function
*
* \retval  0 if success
*/
static int wmt_power_en(int enable)
{
	static DEFINE_SPINLOCK(wmt_power_lock);
	unsigned long flags = 0;
	unsigned int  value;
	int 	ret = 0;
	static int wmt_open_cnt=0;
	
	if(enable){
	    spin_lock_irqsave(&wmt_power_lock, flags);
		wmt_open_cnt++;
	    spin_unlock_irqrestore(&wmt_power_lock, flags);
		if(wmt_open_cnt == 1){
			value = REG32_VAL(PM_CTRL_BASE_ADDR + 0x0604);
			*(volatile unsigned int *)(PM_CTRL_BASE_ADDR + 0x0604) = (value | 0x1);
			while((REG32_VAL(PM_CTRL_BASE_ADDR + 0x0604) & 0xf5) != 0xf1){};
		}
	}else {
	    spin_lock_irqsave(&wmt_power_lock, flags);
	    if( wmt_open_cnt >= 1) {
	    	wmt_open_cnt--;
	    }else {
	    	DBG_ERR("Unexpected WMT Codec power off, ignore it!\n");
	    	ret = -1;
	    }
	    spin_unlock_irqrestore(&wmt_power_lock, flags);
		if(wmt_open_cnt == 0){
			value = REG32_VAL(PM_CTRL_BASE_ADDR + 0x0604);
			*(volatile unsigned char *)(PM_CTRL_BASE_ADDR + 0x0604) = (value & ~(0x1));
			while((REG32_VAL(PM_CTRL_BASE_ADDR + 0x0604) & 0xf5) != 0x00){};
		}
	}
	return ret;
} /* End of wmt_power_en() */

/*!*************************************************************************
* msvd_power_en
*
* Private Function
*
* \retval  0 if success
*/
static int msvd_power_en(int enable)
{
	static DEFINE_SPINLOCK(msvd_power_lock);
	unsigned long flags = 0;
	unsigned int  value;
	int 	ret = 0;
	static int msvd_open_cnt = 0;

	if (enable) {
	    spin_lock_irqsave(&msvd_power_lock, flags);
		msvd_open_cnt++;
	    spin_unlock_irqrestore(&msvd_power_lock, flags);
		if(msvd_open_cnt == 1){
			value = REG32_VAL(PM_CTRL_BASE_ADDR + 0x062C);
			*(volatile unsigned int *)(PM_CTRL_BASE_ADDR + 0x062C) = (value | 0x1);
			while((REG32_VAL(PM_CTRL_BASE_ADDR + 0x062C) & 0xf5) != 0xf1){};
		}
	} else {
	    spin_lock_irqsave(&msvd_power_lock, flags);
	    if( msvd_open_cnt >= 1) {
	    	msvd_open_cnt--;
	    }else {
	    	DBG_ERR("Unexpected MSVD Codec power off, ignore it!\n");
	    	ret = -1;
	    }	    	
	    spin_unlock_irqrestore(&msvd_power_lock, flags);
		if(msvd_open_cnt == 0){
			value = REG32_VAL(PM_CTRL_BASE_ADDR + 0x062C);
			*(volatile unsigned int *)(PM_CTRL_BASE_ADDR + 0x062C) = (value & ~(0x1));
			while((REG32_VAL(PM_CTRL_BASE_ADDR + 0x062C) & 0xf5) != 0x00){};
		}
	}
	return 0;
} /* End of msvd_power_en() */

/*!*************************************************************************
* wmt_clock_en
* 
* Public Function
*/
/*!
* \brief
*	Set source PRD table
*
* \retval  0 if success
*/ 
int wmt_clock_en(int codec_type, int enable)
{
	static DEFINE_SPINLOCK(clk_lock);
	unsigned long flags = 0;

	if ((codec_type < 0 ) || (codec_type >= CODEC_MAX)) {
		DBG_ERR("Unsupported codec ID %d\n", codec_type);
		return -1;
	}

	if (codec_type == CODEC_VD_JPEG) {
		spin_lock_irqsave(&clk_lock, flags);
		if (enable) {
			auto_pll_divisor(DEV_JDEC, CLK_ENABLE, 0, 0);
			auto_pll_divisor(DEV_MSVD, CLK_ENABLE, 0, 0);
			auto_pll_divisor(DEV_WMTVDU, CLK_ENABLE, 0, 0);
			auto_pll_divisor(DEV_WMTNA, CLK_ENABLE, 0, 0);
		} else {
			auto_pll_divisor(DEV_WMTNA,  CLK_DISABLE, 0, 0);
			auto_pll_divisor(DEV_WMTVDU,  CLK_DISABLE, 0, 0);
			auto_pll_divisor(DEV_MSVD, CLK_DISABLE, 0, 0);
			auto_pll_divisor(DEV_JDEC, CLK_DISABLE, 0, 0);
		}
		spin_unlock_irqrestore(&clk_lock, flags);
	} else if (codec_type == CODEC_VD_MSVD) {
		mutex_lock(&wmt_clk_mutex);
		if (enable) {
			auto_pll_divisor(DEV_WMTNA, CLK_ENABLE, 0, 0);
			auto_pll_divisor(DEV_CNMNA, CLK_ENABLE, 0, 0);
			auto_pll_divisor(DEV_MSVD, CLK_ENABLE, 0, 0);
		} else {
			auto_pll_divisor(DEV_MSVD,  CLK_DISABLE, 0, 0);
			auto_pll_divisor(DEV_CNMNA, CLK_DISABLE, 0, 0);
			auto_pll_divisor(DEV_WMTNA, CLK_DISABLE, 0, 0);
		}
		mutex_unlock(&wmt_clk_mutex);
	} else if (codec_type == CODEC_VE_H264) {
		spin_lock_irqsave(&clk_lock, flags);
		if (enable) {
			auto_pll_divisor(DEV_WMTNA, CLK_ENABLE, 0, 0);
			auto_pll_divisor(DEV_MSVD, CLK_ENABLE, 0, 0);
			auto_pll_divisor(DEV_WMTVDU, CLK_ENABLE, 0, 0);
			auto_pll_divisor(DEV_H264, CLK_ENABLE, 0, 0);
		} else {
			auto_pll_divisor(DEV_H264, CLK_DISABLE, 0, 0);
			auto_pll_divisor(DEV_MSVD, CLK_DISABLE, 0, 0);
			auto_pll_divisor(DEV_WMTNA, CLK_DISABLE, 0, 0);
			auto_pll_divisor(DEV_WMTVDU, CLK_DISABLE, 0, 0);
		}
		spin_unlock_irqrestore(&clk_lock, flags);
	} else if (codec_type == CODEC_VE_JPEG) {
		spin_lock_irqsave(&clk_lock, flags);
		if (enable) {
			auto_pll_divisor(DEV_JENC, CLK_ENABLE, 0, 0);
			auto_pll_divisor(DEV_MSVD, CLK_ENABLE, 0, 0);
			auto_pll_divisor(DEV_WMTVDU,  CLK_ENABLE, 0, 0);
			auto_pll_divisor(DEV_WMTNA,  CLK_ENABLE, 0, 0);
		} else {
			auto_pll_divisor(DEV_WMTNA,  CLK_DISABLE, 0, 0);
			auto_pll_divisor(DEV_WMTVDU,  CLK_DISABLE, 0, 0);
			auto_pll_divisor(DEV_MSVD, CLK_DISABLE, 0, 0);
			auto_pll_divisor(DEV_JENC, CLK_DISABLE, 0, 0);
		}
		spin_unlock_irqrestore(&clk_lock, flags);
	}
	return 0;
} /* End of wmt_clock_en() */

/*!*************************************************************************
* wmt_get_codec_clock_count
*
* Private Function
*
* \retval  0 if success
*/
int wmt_get_codec_clock_count(void)
{
	return msvd_clk_enable_cnt;
}
EXPORT_SYMBOL(wmt_get_codec_clock_count);

/*!*************************************************************************
* wmt_reset_codec_clock_count
*
* Private Function
*
* \retval  0 if success
*/
void wmt_reset_codec_clock_count(void)
{
	msvd_clk_enable_cnt = 0;
}
EXPORT_SYMBOL(wmt_reset_codec_clock_count);

/*!*************************************************************************
* wmt_codec_clock_en
*
* Public Function
*/
/*!
* \brief
*
* \retval  0 if success
*/
int wmt_codec_clock_en(int codec_type, int enable)
{
    static DEFINE_SPINLOCK(clk_lock);
    unsigned long flags = 0;
    int ret = 0;
	 
    if ((codec_type < 0 ) || (codec_type >= CODEC_MAX)) {
        DBG_ERR("Unsupported codec ID %d\n", codec_type);
        return -1;
    }     
    if (codec_type == CODEC_VD_JPEG) {
        spin_lock_irqsave(&clk_lock, flags);
        if (enable) {
            REG_WRITE(MSVD_BASE_ADDR + 0x020, 0x0F);
        } else {
            REG_WRITE(MSVD_BASE_ADDR + 0x020, 0x00);
        }
        spin_unlock_irqrestore(&clk_lock, flags);
    } else if (codec_type == CODEC_VD_MSVD) {
        mutex_lock(&wmt_clk_mutex);
        if(enable) {
            msvd_clk_enable_cnt++; 	
            if(msvd_clk_enable_cnt == 1){
                auto_pll_divisor(DEV_CNMVDU, CLK_ENABLE, 0, 0);
                REG_WRITE(MSVD_BASE_ADDR + 0x000, 0x07);
            }
        }else {
            if( msvd_clk_enable_cnt >= 1) {
                msvd_clk_enable_cnt--;
                if(msvd_clk_enable_cnt == 0){
                    REG_WRITE(MSVD_BASE_ADDR + 0x000, 0x00);
                    auto_pll_divisor(DEV_CNMVDU, CLK_DISABLE, 0, 0);
                }
            }else {
                    DBG_ERR("Unexpected MSVD Codec power off, ignore it!\n");
                    ret = -1;
            }	    	
        }        
        mutex_unlock(&wmt_clk_mutex);
    } else if (codec_type == CODEC_VE_H264 ) {
        spin_lock_irqsave(&clk_lock, flags);
        if(enable) {
            REG_WRITE(MSVD_BASE_ADDR + 0x040, 0x0F);
        }else {
            REG_WRITE(MSVD_BASE_ADDR + 0x040, 0x00);
        }
        spin_unlock_irqrestore(&clk_lock, flags);
    } else if (codec_type == CODEC_VE_JPEG) {
        spin_lock_irqsave(&clk_lock, flags);
        if (enable) {
            REG_WRITE(MSVD_BASE_ADDR + 0x060, 0x0F);
        } else {
            REG_WRITE(MSVD_BASE_ADDR + 0x060, 0x00);                
        }
        spin_unlock_irqrestore(&clk_lock, flags);
    }
    return ret;
} /* End of wmt_codec_clock_en() */
EXPORT_SYMBOL(wmt_codec_clock_en);

/*!*************************************************************************
* wmt_codec_pmc_ctl
*
* Public Function
*/
/*!
* \brief
*
* \retval  0 if success
*/
int wmt_codec_pmc_ctl(int codec_type, int enable)
{   

    if ((codec_type < 0 ) || (codec_type >= CODEC_MAX)) {
        DBG_ERR("Unsupported codec ID %d\n", codec_type);
        return -1;
    }
    if (enable){
    		wmt_clock_en(codec_type, 1); /* WMT  clock on */
    		wmt_power_en(1); /* WMT Codec power cotrol  *//*PMC.0x604 */
    		if (codec_type == CODEC_VD_MSVD)
 			msvd_power_en(1); /* MSVD Codec power control */ /*PMC.0x62c */
    }else{
		if (codec_type == CODEC_VD_MSVD)
 			msvd_power_en(0); /* MSVD Codec power control */ /*PMC.0x62c */
    		wmt_power_en(0); /* WMT Codec power cotrol  *//*PMC.0x604 */
    		
    		wmt_clock_en(codec_type, 0); /* WMT  clock on */
    }
	return 0;
} /* End of wmt_codec_pmc_ctl() */
EXPORT_SYMBOL(wmt_codec_pmc_ctl);

/*!*************************************************************************
* wmt_codec_msvd_reset
*
* Public Function
*/
/*!
* \brief
*
* \retval  0 if success
*/
int wmt_codec_msvd_reset(int codec_type)
{
	if (codec_type == CODEC_VD_MSVD) {
		/* Do MSVD SW reset */
		REG_WRITE(MSVD_BASE_ADDR + 0x008, 0x07);
		REG_WRITE(MSVD_BASE_ADDR + 0x008, 0x00);

		/* automatic disable clock , BIT0 AHB, BIT1 core, BIT2 NA bus*/
		REG_WRITE(MSVD_BASE_ADDR + 0x004, 0x00);

		/* Disable SRAM Power Down */
		REG_WRITE(MSVD_BASE_ADDR + 0x00C, 0x00);
	} else if (codec_type == CODEC_VD_JPEG) {
		REG_WRITE(MSVD_BASE_ADDR + 0x028, 0xF); /* Enable MSVD SW reset */
		REG_WRITE(MSVD_BASE_ADDR + 0x028, 0x0); /* Disable MSVD SW reset */
		REG_WRITE(MSVD_BASE_ADDR + 0x02C, 0x0); /* Disable MSVD SW reset */
	} else if (codec_type == CODEC_VE_H264) {
		REG_WRITE(MSVD_BASE_ADDR + 0x048, 0xF); /* Enable MSVD SW reset */
		REG_WRITE(MSVD_BASE_ADDR + 0x048, 0x0); /* Disable MSVD SW reset */
		REG_WRITE(MSVD_BASE_ADDR + 0x044, 0xC); /* Enable Auto disable clock */
		REG_WRITE(MSVD_BASE_ADDR + 0x04C, 0x0); /* Disable SRAM Power down */
	} else if (codec_type == CODEC_VE_JPEG) {
		REG_WRITE(MSVD_BASE_ADDR + 0x068, 0x0F);
		REG_WRITE(MSVD_BASE_ADDR + 0x068, 0x00);
		REG_WRITE(MSVD_BASE_ADDR + 0x06C, 0x00);
	}
	return 0;
} /* End of wmt_codec_msvd_reset() */
EXPORT_SYMBOL(wmt_codec_msvd_reset);

/*!*************************************************************************
* wmt_codec_lock
*
* Public Function
*/
/*!
* \brief
*
* \retval  0 if success
*/
int wmt_codec_lock(int codec_type, int timeout)
{
	struct semaphore *sem;
	int ret = ret;

	if ((codec_type != CODEC_VD_JPEG) && (codec_type != CODEC_VE_JPEG))
		return -1;

	if (codec_lock[codec_type].is_init != 1) {
		sema_init(&codec_lock[codec_type].sem, 1);

		codec_lock[codec_type].is_init = 1;
	}
	sem = &codec_lock[codec_type].sem;

	if (timeout == 0) {
		ret = down_trylock(sem);
		if (ret)
			ret = -ETIME;  /* reasonable if lock holded by other */
	} else if (timeout == -1) {
		ret = down_interruptible(sem);
		if (ret)
			DBG_ERR("<%d> down_interruptible fail (ret: %d)\n",
				codec_type, ret);
	} else {
		ret = down_timeout(sem, msecs_to_jiffies(timeout));
		if (ret)
			DBG_MSG("<%d> down_timeout(%d ms) fail (ret: %d)\n",
				codec_type, timeout, ret);
	}
	if (ret == 0)
		codec_lock[codec_type].is_locked = 1;
	return ret;
} /* End of wmt_codec_lock() */
EXPORT_SYMBOL(wmt_codec_lock);

/*!*************************************************************************
* wmt_codec_unlock
*
* Public Function
*/
/*!
* \brief
*
* \retval  0 if success
*/
int wmt_codec_unlock(int codec_type)
{
	if ((codec_type != CODEC_VD_JPEG) && (codec_type != CODEC_VE_JPEG))
		return -1;

	if (codec_lock[codec_type].is_locked == 1) {
		up(&codec_lock[codec_type].sem);
		codec_lock[codec_type].is_locked = 0;
	} else {
		DBG_ERR("Try to unlock non-locked sem (%s)",
			(codec_type == CODEC_VD_JPEG) ? "jdec" : "jenc");
		return -1;
	}
	return 0;
} /* End of wmt_codec_unlock() */
EXPORT_SYMBOL(wmt_codec_unlock);

/*!*************************************************************************
* wmt_codec_write_prd
*
* Public Function
*/
/*!
* \brief
*   Transfer the buffer address as PRD foramt
*
* \parameter
*   prd_addr  [IN] Phyical address
*
* \retval  0 if success
*/
int wmt_codec_write_prd(
	unsigned int src_buf,
	unsigned int src_size,
	unsigned int prd_addr_in,
	unsigned int prd_buf_size)
{
	#define PRD_MAX_SIZE (60*1024)

	unsigned char *buf_addr = (unsigned char *)src_buf;
	unsigned int   buf_size = src_size;
	unsigned int  *prd_addr = (unsigned int  *)prd_addr_in;
	unsigned int   items;
	unsigned int   count = 0;
	/*----------------------------------------------------------------------
		Transfer the input address as PRD foramt
	----------------------------------------------------------------------*/
	DBG_MSG("src_buf: 0x%x, src_size: %d, prd_addr_in: 0x%x\n",
			src_buf, src_size, prd_addr_in);
	items = prd_buf_size/8;
	while (buf_size > 0) {
		if (buf_size > PRD_MAX_SIZE) {
			*prd_addr++ = (unsigned int)buf_addr;
			*prd_addr++ = PRD_MAX_SIZE;
			buf_size -= PRD_MAX_SIZE;
			buf_addr += PRD_MAX_SIZE;
		} else {
			*prd_addr++ = (unsigned int)buf_addr;
			*prd_addr++ = (0x80000000 | buf_size);
			buf_size = 0;
		}
		count++;
		if (count > items)
			return -1;
	}
	return 0;
} /* End of wmt_codec_write_prd() */
EXPORT_SYMBOL(wmt_codec_write_prd);

/*!*************************************************************************
* wmt_codec_dump_prd
*
* Public Function
*/
/*!
* \brief
*   Dump data in PRD foramt for buffer
*
* \parameter
*   prd_addr  [IN] Phyical address
*
* \retval  0 if success
*/
int wmt_codec_dump_prd(unsigned int prd_virt_in, int dump_data)
{
	unsigned int prd_addr_in, prd_data_size = 0;
	unsigned int addr, len;
	int i, j;

	prd_addr_in = prd_virt_in;
	for (i = 0; ; i += 2) {
		addr = *(unsigned int *)(prd_addr_in + i * 4);
		len  = *(unsigned int *)(prd_addr_in + (i + 1) * 4);
		prd_data_size += (len & 0xFFFF);

		PRINTK("[%02d]Addr: 0x%08x\n", i, addr);
		PRINTK("    Len:  0x%08x (%d)\n", len, (len & 0xFFFF));

		if (dump_data) {
			unsigned char *ptr;

			ptr = (unsigned char *)phys_to_virt(addr);
			for (j = 0; j < (len & 0xFFFF); j++) {
				if ((j%16) == 15)
					PRINTK("0x%02x\n", *ptr);
				else
					PRINTK("0x%02x ", *ptr);
				ptr++;
			}
		}
		if (len & 0x80000000)
			break;
	} /* for(i=0; ; i+=2) */
	PRINTK("Data size in PRD table: %d\n", prd_data_size);

	return 0;
} /* End of wmt_codec_dump_prd() */
EXPORT_SYMBOL(wmt_codec_dump_prd);

/*!*************************************************************************
* check_debug_option
*
* Private Function
*/
/*!
* \brief
*   Initial VD timer
*
* \retval  0 if success
*/
#ifdef CFG_CODEC_PERFORM_EN
static void check_debug_option(void)
{
	extern int wmt_getsyspara(char *varname, unsigned char *varval, int *varlen);

	static int s_first_run;

	if (s_first_run == 0) {
		char buf[80] = {0};
		int varlen = 80;

		/* Read u-boot parameter to decide value of wmt_codec_debug */
		if (wmt_getsyspara("wmt.codec.debug", buf, &varlen) == 0)
			wmt_codec_debug = simple_strtol(buf, NULL, 10);

		s_first_run = 1;
	}
} /* End of check_debug_option() */
#endif /* #ifdef CFG_CODEC_PERFORM_EN */

/*!*************************************************************************
* wmt_codec_timer_init
*
* API Function
*/
/*!
* \brief
*   Initial VD timer
*
* \retval  0 if success
*/
int wmt_codec_timer_init(
	void **pphandle,
	const char *name,
	unsigned int count,
	int threshold_ms)
{
#ifdef CFG_CODEC_PERFORM_EN
	static atomic_t s_Seq_id = ATOMIC_INIT(0);
	struct wmt_tm *pcodec;

	check_debug_option();

	if (wmt_codec_debug == 0)
		return 0;

	pcodec = kmalloc(sizeof(struct wmt_tm), GFP_KERNEL);
	if (pcodec == 0)
		DBG_ERR("Allocate %d bytes fail\n", sizeof(struct wmt_tm));

	memset(pcodec, 0, sizeof(struct wmt_tm));

	pcodec->reset     = count;
	pcodec->threshold = threshold_ms * 1000; /* us */
	pcodec->max       = 0;
	pcodec->min       = 0xFFFFFFF;
	pcodec->initial   = 1;
	strcpy(pcodec->name, name);

	pcodec->seq_id = (unsigned long)(atomic_add_return(1, &s_Seq_id));

	*pphandle = pcodec;
#endif /* #ifdef CFG_CODEC_PERFORM_EN */
	return 0;
} /* End of wmt_codec_timer_init() */
EXPORT_SYMBOL(wmt_codec_timer_init);

/*!*************************************************************************
* wmt_codec_timer_reset_count
*
* API Function
*/
/*!
* \brief
*   Release VD timer
*
* \retval  0 if success
*/
int wmt_codec_timer_reset(void *phandle, unsigned int count, int threshold_ms)
{
#ifdef CFG_CODEC_PERFORM_EN
	struct wmt_tm *pcodec = (struct wmt_tm *)phandle;

	if (pcodec) {
		pcodec->reset     = count;
		pcodec->threshold = threshold_ms * 1000; /* us */
	}
#endif /* #ifdef CFG_CODEC_PERFORM_EN */
	return 0;
} /* End of wmt_codec_timer_reset_count() */
EXPORT_SYMBOL(wmt_codec_timer_reset);

/*!*************************************************************************
* wmt_codec_timer_start
*
* API Function
*/
/*!
* \brief
*   Start a VD timer
*
* \retval  0 if success
*/
int wmt_codec_timer_start(void *phandle)
{
#ifdef CFG_CODEC_PERFORM_EN
	struct wmt_tm *pcodec = (struct wmt_tm *)phandle;

	if (wmt_codec_debug == 0)
		return 0;

	if (pcodec->initial != 1) {
		DBG_ERR("Timer was not initialized!\n");
		return -1;
	}
	do_gettimeofday(&pcodec->start);
#endif
	return 0;
} /* End of wmt_codec_timer_start()*/
EXPORT_SYMBOL(wmt_codec_timer_start);

/*!*************************************************************************
* wmt_codec_timer_stop
*
* API Function
*/
/*!
* \brief
*   Stop a VD timer
*
* \retval  0 if success
*/
int wmt_codec_timer_stop(void *phandle)
{
#ifdef CFG_CODEC_PERFORM_EN
	struct wmt_tm *pcodec = (struct wmt_tm *)phandle;
	int this_time;

	if (wmt_codec_debug == 0)
		return 0;

	if (pcodec->initial != 1) {
		DBG_ERR("timer was not initialized!\n");
		return -1;
	}
	do_gettimeofday(&pcodec->end);

	/* unit in us */
	if (pcodec->start.tv_sec == pcodec->end.tv_sec) {
		this_time = pcodec->end.tv_usec - pcodec->start.tv_usec;
	} else {
		this_time = (pcodec->end.tv_sec - pcodec->start.tv_sec)*1000000
			  + (pcodec->end.tv_usec - pcodec->start.tv_usec);
	}
	if (this_time < 0) {
		PRINTK("Start sec: %ld, usec: %ld\n",
			pcodec->start.tv_sec, pcodec->start.tv_usec);
		PRINTK("End   sec: %ld, usec: %ld\n",
			pcodec->end.tv_sec, pcodec->end.tv_usec);
	}

	pcodec->total_tm += this_time;
	pcodec->interval_tm += this_time;
	pcodec->total_cnt++;

	if (this_time >= pcodec->max)
		pcodec->max = this_time;
	if (this_time <= pcodec->min)
		pcodec->min = this_time;

	if (pcodec->threshold && (this_time >  pcodec->threshold)) {
		PRINTK("[%s] (%d) Decode time(%d) over %d (usec)\n",
			pcodec->name, pcodec->total_cnt,
			this_time, pcodec->threshold);
	}
	pcodec->count++;
	if ((pcodec->reset != 0) && (pcodec->count >= pcodec->reset)) {
		PRINTK("=================================================\n");
		PRINTK("[%s] Avg. time = %d (usec)\n",
			pcodec->name, pcodec->interval_tm/pcodec->count);
		PRINTK("(~ %d) Decode Time Range[%d ~ %d](usec)\n",
			pcodec->total_cnt, pcodec->min, pcodec->max);
		pcodec->interval_tm = 0;
		pcodec->count = 0;
	}
#endif /* #ifdef CFG_CODEC_PERFORM_EN */
	return 0;
} /* End of wmt_codec_timer_stop()*/
EXPORT_SYMBOL(wmt_codec_timer_stop);

/*!*************************************************************************
* wmt_codec_timer_exit
*
* API Function
*/
/*!
* \brief
*   Release VD timer
*
* \retval  0 if success
*/
int wmt_codec_timer_exit(void *phandle)
{
#ifdef CFG_CODEC_PERFORM_EN
	struct wmt_tm *pcodec = (struct wmt_tm *)phandle;

	if (wmt_codec_debug == 0)
		return 0;

	if (pcodec == 0)
		DBG_ERR("Illegal NULL handle!\n");

	if (pcodec->initial != 1) {
		DBG_ERR("Codec(%s) timer was not initialized!\n", pcodec->name);
		return -1;
	}
	if (pcodec->total_cnt) {
		unsigned int avg_tm = pcodec->total_tm/pcodec->total_cnt;
		PRINTK("=== [seq_id: %ld %s] Timer status:\n",
			pcodec->seq_id, pcodec->name);
		PRINTK("Total count = %d\n", pcodec->total_cnt);
		PRINTK("Total time  = %d (usec)\n", pcodec->total_tm);
		PRINTK("Avg. time   = %d (usec)\n", avg_tm);
		PRINTK("Max time    = %d (usec)\n", pcodec->max);
		PRINTK("Min time    = %d (usec)\n", pcodec->min);
		PRINTK("==========================================\n");
	}
	/* reset all */
	memset(pcodec, 0, sizeof(struct wmt_tm));
	kfree(pcodec);
#endif /* #ifdef CFG_CODEC_PERFORM_EN */
	return 0;
} /* End of wmt_codec_timer_exit()*/
EXPORT_SYMBOL(wmt_codec_timer_exit);

MODULE_AUTHOR("WonderMedia Technologies, Inc.");
MODULE_LICENSE("GPL");

/*--------------------End of Function Body -----------------------------------*/
#endif /* ifndef CODEC_C */
