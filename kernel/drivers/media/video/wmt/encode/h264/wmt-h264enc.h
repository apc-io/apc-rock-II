/*++ 
 * WonderMedia H.264 hardware encoder driver
 *
 * Copyright (c) 2013  WonderMedia  Technologies, Inc.
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

/*----------------- MODULE DEPENDENCY ----------------------------------------*/
#ifndef WMT_H264ENC_H
#define WMT_H264ENC_H
/* To assert that only one occurrence is included */
//------------------------------------------------
//  Include Header
//------------------------------------------------
#ifndef COM_H264ENC_H
  #include "com-h264enc.h"
#endif

#ifndef WMT_VE_H
#include "../wmt-ve.h"
#endif

#ifndef __KERNEL__
#include <stdio.h>
#include <stdlib.h>
#endif
/*----------------- EXPORTED PRIVATE CONSTANTS -------------------------------*/
//------------------------------------------------
#ifdef __KERNEL__
  #define DPRINT  printk
#else
  #define DPRINT  printf
  #define KERN_EMERG
  #define KERN_ALERT
  #define KERN_CRIT
  #define KERN_ERR
  #define KERN_WARNING
  #define KERN_NOTICE
  #define KERN_INFO
  #define KERN_DEBUG
#endif
#define EMERG(fmt,args...)   DPRINT(KERN_EMERG   "[%s](%d):" fmt, __FUNCTION__, __LINE__, ## args)
#define ALERT(fmt,args...)   DPRINT(KERN_ALERT   "[%s](%d):" fmt, __FUNCTION__, __LINE__, ## args)
#define CRIT(fmt,args...)    DPRINT(KERN_CRIT    "[%s](%d):" fmt, __FUNCTION__, __LINE__, ## args)
#define ERR(fmt,args...)     DPRINT(KERN_ERR     "[%s](%d):" fmt, __FUNCTION__, __LINE__, ## args)
#define WARNING(fmt,args...) DPRINT(KERN_WARNING "[%s](%d):" fmt, __FUNCTION__, __LINE__, ## args)
#define NOTICE(fmt,args...)  DPRINT(KERN_NOTICE  "[%s](%d):" fmt, __FUNCTION__, __LINE__, ## args)
#define INFO(fmt,args...)    DPRINT(KERN_INFO    "[%s](%d):" fmt, __FUNCTION__, __LINE__, ## args)
#define DEBUG(fmt,args...)   DPRINT(KERN_DEBUG   "[%s](%d):" fmt, __FUNCTION__, __LINE__, ## args)
//------------------------------------------------
//  DEBUG CTL
//------------------------------------------------
#undef NAME 
#undef THIS_TRACE
#undef THIS_DEBUG
#undef THIS_DEBUG_DETAIL
#undef THIS_REG_TRACE

#define NAME H264ENC
//#define THIS_TRACE                 	NAME##_TRACE
//#define THIS_DEBUG                 	NAME##_DEBUG
//#define THIS_DEBUG_DETAIL         	NAME##_DEBUG_DETAIL
//#define THIS_REG_TRACE             	NAME##_REG_TRACE

#ifdef THIS_DEBUG_DETAIL
#define THIS_DEBUG                 NAME##_DEBUG
#define DBG_DETAIL(fmt, args...) DPRINT("[%s](%d):" fmt, __FUNCTION__, __LINE__, ## args)
#else
#define DBG_DETAIL(fmt, args...)
#endif

#ifdef THIS_DEBUG
#define DBG(fmt, args...)        DPRINT("[%s](%d):" fmt, __FUNCTION__, __LINE__, ## args)
#else
#define DBG(fmt, args...)
#endif

#ifdef THIS_TRACE
#define TRACE(fmt, args...)      DPRINT("[%s](%d):" fmt, __FUNCTION__, __LINE__, ## args)
#else
#define TRACE(fmt, args...)
#endif

#define REG_GET32(addr)	REG32_VAL(addr)

#ifdef THIS_REG_TRACE
#define H264_REG_SET32(addr, val)  \
	DPRINT("REG_SET:0x%x -> 0x%0x\n", addr, val);\
	REG32_VAL(addr) = (val)
#else
#define H264_REG_SET32(addr, val)      REG32_VAL(addr) = (val)
#endif


#define H264ENC_IRQ IRQ_H264
#define MVC_EXTENSION_ENABLE      1
static const unsigned char QP_SCALE_CR[52]=
{
  0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,
  28,29,29,30,31,32,32,33,34,34,35,35,36,36,37,37, 37,38,38,38,39,39,39,39
};

#define ALIGN64(a)          (((a)+63) & 0xFFFFFFC0)


#define WMT_H264ENC_BASE      H264_ENCODER_BASE_ADDR

#define REG_BASE_MSVD         MSVD_BASE_ADDR


#define REG_BASE_IP           WMT_H264ENC_BASE + 0x100
#define REG_BASE_FT           WMT_H264ENC_BASE + 0x200
#define REG_BASE_FQIQ         WMT_H264ENC_BASE + 0x300
#define REG_BASE_QBF          WMT_H264ENC_BASE + 0x400
#define REG_BASE_CAVLC        WMT_H264ENC_BASE + 0x500
#define REG_BASE_H264BSOUT    WMT_H264ENC_BASE + 0x600
#define REG_BASE_MBENC        WMT_H264ENC_BASE + 0x700
#define REG_BASE_HEADER       WMT_H264ENC_BASE + 0x800
#define REG_BASE_DEBUG		  WMT_H264ENC_BASE + 0xF00



/*----------------- EXPORTED PRIVATE MACROS ----------------------------------*/

/*----------------- EXPORTED PRIVATE TYPES -----------------------------------*/
//Example// typedef void api_xxx_t;
//------------------------------------------------
//  Definitions of enum
//------------------------------------------------

//------------------------------------------------
//  Definitions of structures
//------------------------------------------------
typedef struct {
	// IP
	int 			ip_cbcr_hor_intp;
	int 			ip_cbcr_ver_intp;
	int 			ip_Y_StartAddress;
	int 			ip_C_StartAddress;
	int 			ip_lineoffset;
	int 			ip_first_mb;
	int 			lambda_md;        //!< Mode decision Lambda
	int    			lambda_mdfp;       //!< Fixed point mode decision lambda;
	int 			ip_cost;

	// common
	int 			h264_errcnt;
	int 			rd_data;
	int 			wr_data;
	int 			bk_wrcnt;
	int 			bk_rdcnt;
	int 			mb_xcnt;
	int 			mb_ycnt;
	int 			frame_cnt;
	int 			i;
	int 			luma_qp;
	int 			chroma_qp;
	unsigned int 	StartSimTime;
	unsigned int 	EndSimTime;
	int             tmp_data;

	// Hardware HEADER write
	int 			sps_en;
	int 			pps_en;
	int 			is_gop_first_unit;
	int 			slice_above;
	int 			hw_header_wen;
	int 			dw_mask;
	int 			dw_rshift;
	int 			byte_lshift;
	int 			last_enc;
	int 			last_len;
	int 			hw_total_byte_cnt;
	int 			hw_byte_cnt;
	int 			bytebf_ready;
	int 			hw_byte1;
	int 			hw_byte2;
	int 			hw_byte3;
	int 			hw_byte4;
	int 			hw_dwvalue;

	// bitstream out
	unsigned int 	h264bs_addr;
	unsigned int 	h264bs_adr_init;
	unsigned int 	h264bs_len;
	unsigned int 	h264bs_len_init;
	unsigned int 	h264prd_addr;
	unsigned int 	pred_bs_adr;
	unsigned int	pred_bs_adr_phys;
	unsigned int 	pred_bs_adr_init;
	unsigned int 	bs_bytecnt;

	int 			mbend_status;
	int 			bsend_status;
	int             ipend_status;
	int 			mem_buf_full;

	// frame layer
	int 			width;
	int 			height;
	int 			mb_width;
	int 			mb_height;
	unsigned char   byte_buf;
	int				bits_to_go;

	//FWP need
	int ip_fwp_inter_pred_max_err_sum ;  //[15:0]
	int ip_fwp_recon_1st_Y_StartAddress;
	int ip_fwp_recon_1st_C_StartAddress;
	int ip_fwp_recon_2nd_Y_StartAddress;
	int ip_fwp_recon_2nd_C_StartAddress;
	int ip_y4x4_x4_cost_min;
	int ip_y4x4_x4_cost_max;


} VEU;

typedef struct {
	unsigned int        size;     /* sizeof(h264enc_ctx_t) */
	struct h264enc_prop prop;

	unsigned int        prd_virt;
	unsigned int        prd_phys;
	unsigned int        bs_buf;
	unsigned int        frame_size;

	int                 status;

	volatile char       irqevent_flag;

	spinlock_t          ste_lock;
	void               *codec_tm;
} h264enc_ctx_t;

/*----------------- EXPORTED PRIVATE FUNCTIONS -------------------------------*/
#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
}
#endif

/*----------------------------------------------------------------------------*/
/*=== END ====================================================================*/
#endif

