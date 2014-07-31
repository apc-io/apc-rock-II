/*++
 * WonderMedia common SoC hardware JPEG decoder driver
 *
 * Copyright (c) 2008-2013 WonderMedia  Technologies, Inc.
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
#ifndef HW_JDEC_H
/* To assert that only one occurrence is included */
#define HW_JDEC_H

/*-------------------- MODULE DEPENDENCY ---------------------------------*/

#include <linux/wmt-mb.h>    /* struct mmu_table_info defined */

#include "../wmt-codec.h"
#include "../wmt-vd.h"
#include "com-jdec.h"

#define THE_MB_USER "WMT_JDEC"


/*-------------------- EXPORTED PRIVATE CONSTANTS ------------------------*/

/* The followign two macros should be enable one and only one */
#define JDEC_IRQ_MODE
/*#define JPEC_POLL_MODE*/


/*#define JDEC_DEBUG*/    /* Flag to enable debug message */
/*#define JDEC_DEBUG_DETAIL*/
/*#define JDEC_TRACE*/


#undef DBG_MSG
#ifdef JDEC_DEBUG
	#define DBG_MSG(fmt, args...)\
		do {\
			printk(KERN_INFO "{%s} " fmt, __func__, ## args);\
		} while (0)
#else
	#define DBG_MSG(fmt, args...)
#endif

#undef DBG_DETAIL
#ifdef JDEC_DEBUG_DETAIL
#define DBG_DETAIL(fmt, args...)\
		do {\
			printk(KERN_INFO "{%s} " fmt, __func__, ## args);\
		} while (0)
#else
#define DBG_DETAIL(fmt, args...)
#endif

#undef TRACE
#ifdef JDEC_TRACE
#define TRACE(fmt, args...)\
		do {\
			printk(KERN_INFO "{%s} " fmt, __func__, ## args);\
		} while (0)
#else
#define TRACE(fmt, args...)
#endif

#undef INFO
#define INFO(fmt, args...)\
		do {\
			printk(KERN_INFO "[JDEC] " fmt, ## args);\
		} while (0)

#undef DBG_ERR
#define DBG_ERR(fmt, args...)\
		do {\
			printk(KERN_ERR "*E* {%s} " fmt, __func__, ## args);\
		} while (0)


/*-------------------- EXPORTED PRIVATE TYPES-----------------------------*/

enum {
	STA_READY          = 0x00010000,
	STA_CLOSE          = 0x00020000,
	STA_ATTR_SET       = 0x00040000,
	STA_DMA_START      = 0x00080000,
	STA_DECODE_DONE    = 0x00000001,
	STA_DECODEING      = 0x00000002,
	STA_DMA_MOVE_DONE  = 0x00000008,
	STA_BD_BAND_DONE   = 0x00000010, /* BD: Banding decode */
	/* Error Status */
	STA_ERR_FLAG       = 0x80000000,
	STA_ERR_TIMEOUT    = 0x80100000,
	STA_ERR_DECODING   = 0x80200000,
	STA_ERR_BAD_PRD    = 0x80400000,
	STA_ERR_UNKNOWN    = 0x80800000
}; /* jdec_status */


/*------------------------------------------------------------------------------
	Definitions of structures
------------------------------------------------------------------------------*/
struct jdec_ctx_t {
	jdec_prop_ex_t    prop;
	jdec_pd           pd_mcu;

	unsigned int      decoded_w;      /* Decoded Picture width (pixel) */
	unsigned int      decoded_h;      /* Decoded Picture height (pixel) */
	unsigned int      src_mcu_width;  /* Width in unit MCU */
	unsigned int      src_mcu_height; /* Height in unit MCU */
	unsigned int      scale_ratio;

	unsigned int      prd_virt;
	unsigned int      prd_addr;
	unsigned int      line_width_y;   /* Y or RGB */
	unsigned int      line_width_c;
	unsigned int      line_height;    /* Y/C or RGB */

	unsigned int      bpp;            /* bits per pixel */

	unsigned int      req_y_size;     /* Y or RGB */
	unsigned int      req_c_size;

	/* Following member is used for multi-tasks */
	unsigned int      ref_num;

	/* Following members are used for hw-jpeg.c only */
	unsigned int      _timeout;
	int               _status;  /* jdec_status */
	unsigned int      _is_mjpeg;

	unsigned long     mb_phy_addr;
	void             *codec_tm;
	unsigned int      got_lock;
};

/*------------------- EXPORTED PRIVATE FUNCTIONS  ------------------------*/

int wmt_jdec_set_attr(struct jdec_ctx_t *drv);
int wmt_jdec_decode_proc(struct jdec_ctx_t *drv);
int wmt_jdec_decode_finish(struct jdec_ctx_t *drv);
int wmt_jdec_open(struct jdec_ctx_t *drv);
int wmt_jdec_close(struct jdec_ctx_t *drv);

#endif /* ifndef HW_JDEC_H */

/*=== END hw-jdec.h ======================================================*/
