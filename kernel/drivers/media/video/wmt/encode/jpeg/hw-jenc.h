/*++
 * WonderMedia common SoC hardware JPEG encoder driver
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
#ifndef HW_JENC_H
/* To assert that only one occurrence is included */
#define HW_JENC_H

/*-------------------- MODULE DEPENDENCY ---------------------------------*/

#include <linux/wmt-mb.h>       /* for struct mmu_table_info */

#include "../../wmt-codec.h"
#include "../wmt-ve.h"
#include "com-jenc.h"

/*-------------------- EXPORTED PRIVATE CONSTANTS ------------------------*/

#ifdef __KERNEL__
#include <linux/delay.h>     /* for mdelay() only */
#else
#include "sim-kernel.h"
#endif

/* The followign two macros should be enable one and only one */
#define JENC_IRQ_MODE
/*#define JENC_POLL_MODE*/


/*#define JENC_DEBUG*/
/*#define JENC_DEBUG_DETAIL*/
/*#define JENC_TRACE*/


#undef DBG_MSG
#ifdef JENC_DEBUG
#define DBG_MSG(fmt, args...) \
	do {\
		printk(KERN_INFO "{%s} " fmt, __func__ , ## args);
	} while (0)
#else
#define DBG_MSG(fmt, args...)
#endif

#undef DBG_DETAIL
#ifdef JENC_DEBUG_DETAIL
#define DBG_DETAIL(fmt, args...) \
	do {\
		printk(KERN_INFO "{%s} " fmt, __func__ , ## args);\
	} while (0)
#else
#define DBG_DETAIL(fmt, args...)
#endif

#undef TRACE
#ifdef JENC_TRACE
#define TRACE(fmt, args...) \
	do {\
		printk(KERN_INFO "{%s}:  " fmt, __func__ , ## args);\
	} while (0)
#else
#define TRACE(fmt, args...)
#endif

#undef DBG_ERR
#define DBG_ERR(fmt, args...) \
	do {\
		printk(KERN_ERR "*E* {%s} " fmt, __func__ , ## args);\
	} while (0)

#undef DBG_WARN
#define DBG_WARN(fmt, args...) \
	do {\
		printk(KERN_ERR "*W* {%s} " fmt, __func__ , ## args);\
	} while (0)


/*-------------------- EXPORTED PRIVATE TYPES-----------------------------*/


/*-------------------------------------------------------------------------
	Definitions of structures
--------------------------------------------------------------------------*/
struct jenc_mmu {
	struct mmu_table_info tbl_y;    /* Addr of Page Dir of Y/RGB buf */
	struct mmu_table_info tbl_c;    /* Addr of Page Dir of C buf */
	unsigned int    color;          /* Src/Dst color for JENC DMA */
	unsigned int    work_buf_addr;  /* start addr of working buffer */
	unsigned int    work_stride;    /* stride of working buffer */
	unsigned int    work_size;      /* Buf size for one MCU line */
	unsigned int    mcu_height;
	unsigned int    mcu_height_c;
};

struct jenc_ctx {
	struct jenc_prop    prop;
	unsigned long       ref_num;    /* reference serial number */
	wait_queue_head_t   wait_event;

	unsigned int        src_addr;   /* Encoding buffer (source) address */
	unsigned int        src_size;   /* Encoding buffer size in bytes */
	unsigned int        c_addr;     /* Encoding buffer (source) address */
	unsigned int        c_size;     /* Encoding buffer size in bytes */
	unsigned int        stride;     /* Line size in source buffer */
	unsigned int        pic_width;  /* Source picture width */
	unsigned int        pic_height; /* Source picture height */
	struct jenc_mmu     mmu;
	vdo_color_fmt       src_color;  /* Source color format */
	unsigned int        enc_color;  /* Src/Dst color format for JENC Core */

	unsigned int        prd_virt;   /* virtual addr of PRDT for output */
	unsigned int        prd_phys;   /* physical addr of PRDT for output */

	unsigned int        bsbuf;  /* Alloc MB buffer for write prd_table */
	unsigned int        mb_mmubuf;

	unsigned int        y64bnum;
	unsigned int        cburstnum;
	unsigned int        pic_cburstnum;
	unsigned int        mcu_row_y;
	unsigned int        mcu_row_c;

	int                 _status;  /* ve_status */
	unsigned int        _timeout;         /* unit in ms */

	unsigned int        line_width_y;     /* Y or RGB */
	unsigned int        line_width_c;
	unsigned int        line_height;      /* Y/C or RGB */
	void               *codec_tm;

};

/*--------------------- EXPORTED PRIVATE FUNCTIONS  ----------------------*/

int wmt_jenc_open(struct jenc_ctx *ctx);
int wmt_jenc_close(struct jenc_ctx *ctx);

int wmt_jenc_encode_proc(struct jenc_ctx *ctx);
int wmt_jenc_encode_finish(struct jenc_ctx *ctx);


#endif /* ifndef HW_JENC_H */

/*=== END hw-jenc.h ======================================================*/
