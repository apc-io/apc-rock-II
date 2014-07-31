/*++
 * WonderMedia SoC hardware JPEG decoder driver
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

#define WMT_JDEC_C

#include "hw-jdec.h"


/*#define JDEC_REG_TRACE*/

#define PRINTK(fmt, args...)\
		do {\
			printk(KERN_INFO " " fmt, ## args);\
		} while (0)

#ifdef JDEC_REG_TRACE
#define JDEC_REG_SET32(addr, val)  \
		do {\
			PRINTK("REG_SET:0x%x -> 0x%0x\n", addr, val);\
			REG32_VAL(addr) = (val);\
		} while (0)
#else
#define JDEC_REG_SET32(addr, val)      (REG32_VAL(addr) = (val))
#endif

/*#define JDEC_PRD_DEBUG*/
#ifdef JDEC_PRD_DEBUG
#define DBG_PRD(fmt, args...)     PRINTK("{%s} " fmt, __func__, ## args)
#else
#define DBG_PRD(fmt, args...)
#endif

/*-------------------------------------------------------------------------
	JPEG_DECODER_BASE_ADDR was defined in
	\arch\arm\mach-wmt\include\mach\wmt_mmap.h
--------------------------------------------------------------------------*/
#define JDEC_BASE     JPEG_DECODER_BASE_ADDR

#define ALIGN64(a)    (((a) + 63) & (~63))
#define MAX(a, b)     (((a) >= (b)) ? (a) : (b))

#ifdef __KERNEL__
DECLARE_WAIT_QUEUE_HEAD(jdec_wait);
#endif


/*!*************************************************************************
* dbg_dump_registers
*
* Private Function
*/
/*!
* \brief
*   Dump important register values for debugging
*
* \parameter
*
* \retval  0 if success
*/
static void dbg_dump_registers(struct jdec_ctx_t *ctx)
{
	#define Dump_RegisterValue(addr) \
		do {\
			PRINTK("REG(0x%x): 0x%x\n", addr, REG32_VAL(addr));\
		} while (0)

/*	wmt_codec_dump_prd(ctx->prd_virt, 0); */

	PRINTK("MSVD Regsiters:\n");
	Dump_RegisterValue(MSVD_BASE_ADDR+0x020);
	Dump_RegisterValue(MSVD_BASE_ADDR+0x024);
	Dump_RegisterValue(MSVD_BASE_ADDR+0x028);
	Dump_RegisterValue(MSVD_BASE_ADDR+0x02C);

	PRINTK("JPEG Regsiters:\n");
	Dump_RegisterValue(JDEC_BASE + 0x020);
	Dump_RegisterValue(JDEC_BASE + 0x024);

	Dump_RegisterValue(JDEC_BASE + 0x100);
	Dump_RegisterValue(JDEC_BASE + 0x104);
	Dump_RegisterValue(JDEC_BASE + 0x108);
	Dump_RegisterValue(JDEC_BASE + 0x10C);
	Dump_RegisterValue(JDEC_BASE + 0x110);
	Dump_RegisterValue(JDEC_BASE + 0x114);
	Dump_RegisterValue(JDEC_BASE + 0x180);  /* debug register */
	Dump_RegisterValue(JDEC_BASE + 0x184);  /* debug register */
	Dump_RegisterValue(JDEC_BASE + 0x188);  /* debug register */

	Dump_RegisterValue(JDEC_BASE + 0x200);
	Dump_RegisterValue(JDEC_BASE + 0x210);
	Dump_RegisterValue(JDEC_BASE + 0x214);
	Dump_RegisterValue(JDEC_BASE + 0x218);
	Dump_RegisterValue(JDEC_BASE + 0x220);
	Dump_RegisterValue(JDEC_BASE + 0x230);
	Dump_RegisterValue(JDEC_BASE + 0x234);
	Dump_RegisterValue(JDEC_BASE + 0x238);
	Dump_RegisterValue(JDEC_BASE + 0x23C);
	Dump_RegisterValue(JDEC_BASE + 0x240);
	Dump_RegisterValue(JDEC_BASE + 0x244);
	Dump_RegisterValue(JDEC_BASE + 0x250);
	Dump_RegisterValue(JDEC_BASE + 0x254);
	Dump_RegisterValue(JDEC_BASE + 0x258);
	Dump_RegisterValue(JDEC_BASE + 0x25C);
	Dump_RegisterValue(JDEC_BASE + 0x260);
	Dump_RegisterValue(JDEC_BASE + 0x264);
	Dump_RegisterValue(JDEC_BASE + 0x268);
	Dump_RegisterValue(JDEC_BASE + 0x26C);

	Dump_RegisterValue(JDEC_BASE + 0x300);
	Dump_RegisterValue(JDEC_BASE + 0x304);
	Dump_RegisterValue(JDEC_BASE + 0x308);
	Dump_RegisterValue(JDEC_BASE + 0x30C);
	Dump_RegisterValue(JDEC_BASE + 0x320);
	Dump_RegisterValue(JDEC_BASE + 0x324);
	Dump_RegisterValue(JDEC_BASE + 0x328);
	Dump_RegisterValue(JDEC_BASE + 0x32C);

	Dump_RegisterValue(JDEC_BASE + 0x400);
	Dump_RegisterValue(JDEC_BASE + 0x404);

	Dump_RegisterValue(JDEC_BASE + 0x500);
	Dump_RegisterValue(JDEC_BASE + 0x504);
	Dump_RegisterValue(JDEC_BASE + 0x508);
	Dump_RegisterValue(JDEC_BASE + 0x50C);
	Dump_RegisterValue(JDEC_BASE + 0x510);

	Dump_RegisterValue(JDEC_BASE + 0x800);
	Dump_RegisterValue(JDEC_BASE + 0x810);
	Dump_RegisterValue(JDEC_BASE + 0x814);
	Dump_RegisterValue(JDEC_BASE + 0x820);
	Dump_RegisterValue(JDEC_BASE + 0x824);
	Dump_RegisterValue(JDEC_BASE + 0x828);
	Dump_RegisterValue(JDEC_BASE + 0x82C);
	Dump_RegisterValue(JDEC_BASE + 0x830);
	Dump_RegisterValue(JDEC_BASE + 0x834);
	Dump_RegisterValue(JDEC_BASE + 0x838);
	Dump_RegisterValue(JDEC_BASE + 0x83C);
	Dump_RegisterValue(JDEC_BASE + 0x840);
	Dump_RegisterValue(JDEC_BASE + 0x844);
	Dump_RegisterValue(JDEC_BASE + 0x848);
	Dump_RegisterValue(JDEC_BASE + 0x84C);
	Dump_RegisterValue(JDEC_BASE + 0x850);
	Dump_RegisterValue(JDEC_BASE + 0x854);
	Dump_RegisterValue(JDEC_BASE + 0x860);
	Dump_RegisterValue(JDEC_BASE + 0x864);
	Dump_RegisterValue(JDEC_BASE + 0x870);
	Dump_RegisterValue(JDEC_BASE + 0x880);
	Dump_RegisterValue(JDEC_BASE + 0x884);
	Dump_RegisterValue(JDEC_BASE + 0x890);
	Dump_RegisterValue(JDEC_BASE + 0x894);
	Dump_RegisterValue(JDEC_BASE + 0x898);
	Dump_RegisterValue(JDEC_BASE + 0x89C);
	Dump_RegisterValue(JDEC_BASE + 0x8A0);
	Dump_RegisterValue(JDEC_BASE + 0x8A4);

} /* End of dbg_dump_registers() */

/*!*************************************************************************
* get_mcu_unit
*
* Private Function
*/
/*!
* \brief
*   Get MCU size
*
* \parameter
*   sof_w         [IN]  Picture width in SOF
*   sof_h         [IN]  Picture height in SOF
*   color_format  [IN]  Picture color format
*   mcu_width     [OUT] Pixel per MCU in width
*   mcu_height    [OUT] Pixel per MCU in height
* \retval  0 if success
*/
static int get_mcu_unit(
	unsigned int    sof_w,
	unsigned int    sof_h,
	vdo_color_fmt   color_format,
	unsigned int   *mcu_width,
	unsigned int   *mcu_height)
{
	switch (color_format) {
	case VDO_COL_FMT_YUV420:
		*mcu_width  = (sof_w + 15)>>4;
		*mcu_height = (sof_h + 15)>>4;
		break;
	case VDO_COL_FMT_YUV422H:
		*mcu_width  = (sof_w + 15)>>4;
		*mcu_height = (sof_h + 7)>>3;
		break;
	case VDO_COL_FMT_YUV422V:
		*mcu_width  = (sof_w + 7)>>3;
		*mcu_height = (sof_h + 15)>>4;
		break;
	case VDO_COL_FMT_YUV444:
	case VDO_COL_FMT_GRAY:
		*mcu_width  = (sof_w + 7)>>3;
		*mcu_height = (sof_h + 7)>>3;
		break;
	case VDO_COL_FMT_YUV411:
		*mcu_width  = (sof_w + 31)>>5;
		*mcu_height = (sof_h + 7)>>3;
		break;
	default:
		*mcu_width  = 0;
		*mcu_height = 0;
		DBG_ERR("Unknown input color format(%d)\n", color_format);
		return -1;
	}
	DBG_MSG("Color format(%d), MCU w: %d, h: %d\n",
		color_format, *mcu_width, *mcu_height);

	return 0;
} /* End of get_mcu_unit() */

/*!*************************************************************************
* clean_interrup
*
* Private Function
*/
/*!
* \brief
*	Wait HW decoder finish job
*
* \retval  none
*/
static void clean_interrup(void)
{
	JDEC_REG_SET32(JDEC_BASE + 0x108, 0); /* stop bsdma */
	/* Clear all INT since we have got we want (decode finish) */
	JDEC_REG_SET32(JDEC_BASE + 0x020, 0x0);
	JDEC_REG_SET32(JDEC_BASE + 0x024, 0x7F); /* Clear INT staus */
} /* End of clean_interrup() */

/*!*************************************************************************
* wait_decode_finish
*
* Private Function
*/
/*!
* \brief
*	Wait HW decoder finish job
*
* \retval  none
*/
static int wait_decode_finish(struct jdec_ctx_t *ctx)
{
	jdec_prop_ex_t *jp = &ctx->prop;
	int ret = 0;
	int err_happen = 0;

	DBG_MSG("img_w:           %d\n", jp->jfb.fb.img_w);
	DBG_MSG("img_h:           %d\n", jp->jfb.fb.img_h);
	DBG_MSG("fb_w:            %d\n", jp->jfb.fb.fb_w);
	DBG_MSG("fb_h:            %d\n", jp->jfb.fb.fb_h);
	DBG_MSG("phy_addr_y:    0x%x\n", jp->jfb.fb.y_addr);
	DBG_MSG("y_size:          %d\n", jp->jfb.fb.y_size);
	DBG_MSG("phy_addr_c:    0x%x\n", jp->jfb.fb.c_addr);
	DBG_MSG("c_size:          %d\n", jp->jfb.fb.c_size);
	DBG_MSG("col_fmt:         %d\n", jp->jfb.fb.col_fmt);
	DBG_MSG("bpp:             %d\n", jp->jfb.fb.bpp);
	DBG_MSG("flags:         0x%x\n", jp->flags);

#ifdef JDEC_IRQ_MODE
	/* Wait decoding finish */
	if (wait_event_interruptible_timeout(jdec_wait,
		(ctx->_status & STA_DECODE_DONE),
		msecs_to_jiffies(ctx->_timeout)) == 0) {
			DBG_ERR("Timeout over %d ms\n", ctx->_timeout);
			err_happen = 1;
			ctx->_status |= STA_ERR_TIMEOUT;
	}
#else
/* if JPEC_POLL_MODE */
{
	unsigned int  reg_val, i = 100000;

	INFO("Run Polling Mode...\n");
	while (--i) {
		reg_val = REG32_VAL(JDEC_BASE + 0x024);
		if (reg_val & 0x01) {
			ctx->_status |= STA_DECODE_DONE;
			DBG_MSG("[%d] reg_val: 0x%x\n", i, reg_val);
			break;
		} else if (reg_val & 0x10) {
			err_happen = 1;
			DBG_ERR("reg_val: 0x%x at 0x%x\n",
				reg_val, JDEC_BASE + 0x024);
			ctx->_status = STA_ERR_DECODING;
			break;
		}
	}
	if (i == 0) {
		DBG_ERR("JPEG Decoding Timeout\n");
		err_happen = 1;
	}
#endif /* #ifdef JDEC_IRQ_MODE */

	if (err_happen) {
		unsigned int  read_byte;

		read_byte = REG32_VAL(JDEC_BASE + 0x110);
		DBG_ERR("read_byte(%d), input size(%d)\n",
			read_byte, jp->bufsize);
		DBG_ERR("Decode JEPG NOT complete\n");
		DBG_ERR("INT status: 0x%08x\n", REG32_VAL(JDEC_BASE + 0x024));
		DBG_ERR("Time out: %d ms\n", ctx->_timeout);
		DBG_ERR("SOF (%d x %d)\n", jp->hdr.sof_w, jp->hdr.sof_h);
		DBG_ERR("Driver status: 0x%08x\n", ctx->_status);

		if (ctx->_status & STA_DECODE_DONE)
			DBG_ERR("Already receive decode done (bit 0) INT\n");

		if (ctx->_is_mjpeg == 0)
			dbg_dump_registers(ctx);

		clean_interrup();
		ret = -1;
	}
	/* Disable power & clock to save power consumption */
	wmt_codec_clock_en(CODEC_VD_JPEG, 0);
	wmt_codec_pmc_ctl(CODEC_VD_JPEG, 0);

	return ret;
} /* End of wait_decode_finish() */

/*!*************************************************************************
* do_yuv2rgb
*
* Private Function
*/
/*!
* \brief
*	YUV to RGB formula
*
* \retval  none
*/
static void do_yuv2rgb(vdo_color_fmt dst_color)
{
	JDEC_REG_SET32(JDEC_BASE + 0x404, 0xff); /* alpha = 0xFF for Android */
	if (dst_color == VDO_COL_FMT_ARGB)
		JDEC_REG_SET32(JDEC_BASE + 0x400, 0x001);
	else if (dst_color == VDO_COL_FMT_ABGR)
		JDEC_REG_SET32(JDEC_BASE + 0x400, 0x101);
	else
		JDEC_REG_SET32(JDEC_BASE + 0x400, 0);  /* YUV2RGB Enable */
} /* End of do_yuv2rgb() */

/*!*************************************************************************
* do_chroma_scale_up
*
* Private Function
*/
/*!
* \brief
*	YUV to RGB formula (provided by IC1 TK)
*
* \retval  none
*/
int do_chroma_scale_up(struct jdec_ctx_t *ctx, jdec_prop_ex_t *jp)
{
	jdec_hdr_info_t    *hdr;
	int chroma_scale = 0x00;
	int CH_H_scale = 0, CH_V_scale = 0;

	hdr = &jp->hdr;

	CH_H_scale = CH_V_scale = ctx->scale_ratio;
	/*----------------------------------------------------------------------
		Do chroma Scale-up setting
	----------------------------------------------------------------------*/
	switch (jp->scale_factor) {
	case SCALE_ORIGINAL:
		/* Decode to 1:1 */
		if (jp->dst_color == VDO_COL_FMT_YUV444) {
			if (hdr->src_color == VDO_COL_FMT_YUV420)
				chroma_scale = 0x0101;
			else if (hdr->src_color == VDO_COL_FMT_YUV411)
				chroma_scale = 0x0100;
			else if (hdr->src_color == VDO_COL_FMT_YUV422H)
				chroma_scale = 0x0001;
			else if (hdr->src_color == VDO_COL_FMT_YUV422V)
				chroma_scale = 0x0100;
		} else if (jp->dst_color == VDO_COL_FMT_YUV422H) {
			if (hdr->src_color == VDO_COL_FMT_YUV420)
				chroma_scale = 0x0100;
			else if (hdr->src_color == VDO_COL_FMT_YUV444)
				CH_H_scale = 0x01;
		} else if (jp->dst_color == VDO_COL_FMT_YUV420) {
			if ((hdr->src_color == VDO_COL_FMT_YUV420) ||
				(hdr->src_color == VDO_COL_FMT_GRAY)) {
				/* Do nothing here */
			} else if (hdr->src_color == VDO_COL_FMT_YUV422H)
				CH_V_scale = 0x01;
			else if (hdr->src_color == VDO_COL_FMT_YUV422V)
				CH_H_scale = 0x01;
			else if (hdr->src_color == VDO_COL_FMT_YUV444) {
				CH_H_scale = 0x01;
				CH_V_scale = 0x01;
			} else {
				DBG_ERR("Not supported scale (%d -> %d)\n",
					hdr->src_color, jp->dst_color);
			}
		} else if ((jp->dst_color == VDO_COL_FMT_ARGB) ||
			(jp->dst_color == VDO_COL_FMT_ABGR)) {
			/* Do nothing here */
		}
		break;

	case SCALE_HALF:
		/* Decode to 1/2 */
		if (jp->dst_color == VDO_COL_FMT_YUV444) {
			if (hdr->src_color == VDO_COL_FMT_YUV420) {
				CH_H_scale = 0; /* 1/2 --> 1/1 */
				CH_V_scale = 0; /* 1/2 --> 1/1 */
			} else if (hdr->src_color == VDO_COL_FMT_YUV411)
				CH_H_scale = 0; /* 1/2 --> 1/1 */
			else if (hdr->src_color == VDO_COL_FMT_YUV422H)
				CH_H_scale = 0; /* 1/2 --> 1/1 */
			else if (hdr->src_color == VDO_COL_FMT_YUV422V)
				CH_V_scale = 0; /* 1/2 --> 1/1 */
		} else if (jp->dst_color == VDO_COL_FMT_YUV422H) {
			if (hdr->src_color == VDO_COL_FMT_YUV420)
				CH_V_scale = 0; /* 1/2 --> 1/1 */
			else if (hdr->src_color == VDO_COL_FMT_YUV411)
				CH_H_scale = 0; /* 1/2 --> 1/1 */
			else if (hdr->src_color == VDO_COL_FMT_YUV422V) {
				CH_H_scale = 2; /* 1/2 --> 1/4 */
				CH_V_scale = 0; /* 1/2 --> 1/1 */
			} else if (hdr->src_color == VDO_COL_FMT_YUV444)
				CH_H_scale = 2; /* 1/2 --> 1/4 */
		} else if (jp->dst_color == VDO_COL_FMT_YUV420) {
			if (hdr->src_color == VDO_COL_FMT_YUV411) {
				CH_H_scale = 0; /* 1/2 --> 1/1 */
				CH_V_scale = 2; /* 1/2 --> 1/4 */
			} else if (hdr->src_color == VDO_COL_FMT_YUV422H)
				CH_V_scale = 2; /* 1/2 --> 1/4 */
			else if (hdr->src_color == VDO_COL_FMT_YUV422V)
				CH_H_scale = 2; /* 1/2 --> 1/4 */
		} else if ((jp->dst_color == VDO_COL_FMT_ARGB) ||
				(jp->dst_color == VDO_COL_FMT_ABGR)) {
			if (hdr->src_color == VDO_COL_FMT_YUV411)
				CH_H_scale = 0; /* 1/2 --> 1/1 */
			else if (hdr->src_color == VDO_COL_FMT_YUV422V)
				CH_V_scale = 0; /* 1/2 --> 1/1 */
		}
		break;

	case SCALE_QUARTER:
		/* Decode to 1/4 */
		if (jp->dst_color == VDO_COL_FMT_YUV444) {
			if (hdr->src_color == VDO_COL_FMT_YUV420) {
				CH_H_scale = 1; /* 1/4 --> 1/2 */
				CH_V_scale = 1; /* 1/4 --> 1/2 */
			} else if (hdr->src_color == VDO_COL_FMT_YUV411)
				CH_H_scale = 0; /* 1/4 --> 1/1 */
			else if (hdr->src_color == VDO_COL_FMT_YUV422H)
				CH_H_scale = 1; /* 1/4 --> 1/2 */
			else if (hdr->src_color == VDO_COL_FMT_YUV422V)
				CH_V_scale = 1; /* 1/4 --> 1/2 */
		} else if (jp->dst_color == VDO_COL_FMT_YUV422H) {
			if (hdr->src_color == VDO_COL_FMT_YUV420)
				CH_V_scale = 1; /* 1/4 --> 1/2 */
			else if (hdr->src_color == VDO_COL_FMT_YUV411)
				CH_H_scale = 1; /* 1/4 --> 1/2 */
			else if (hdr->src_color == VDO_COL_FMT_YUV422V) {
				CH_H_scale = 3; /* 1/4 --> 1/8 */
				CH_V_scale = 1; /* 1/4 --> 1/2 */
			} else if (hdr->src_color == VDO_COL_FMT_YUV444)
				CH_H_scale = 3; /* 1/4 --> 1/8 */
		} else if (jp->dst_color == VDO_COL_FMT_YUV420) {
			if (hdr->src_color == VDO_COL_FMT_YUV411) {
				CH_H_scale = 1; /* 1/4 --> 1/2 */
				CH_V_scale = 3; /* 1/4 --> 1/8 */
			} else if (hdr->src_color == VDO_COL_FMT_YUV422H)
				CH_V_scale = 3; /* 1/4 --> 1/8 */
			else if (hdr->src_color == VDO_COL_FMT_YUV422V)
				CH_H_scale = 3; /* 1/4 --> 1/8 */
			else if (hdr->src_color == VDO_COL_FMT_YUV444) {
				CH_H_scale = 3; /* 1/4 --> 1/8 */
				CH_V_scale = 3; /* 1/4 --> 1/8 */
			}
		} else if (jp->dst_color == VDO_COL_FMT_ARGB) {
			if (hdr->src_color == VDO_COL_FMT_YUV411)
				CH_H_scale = 0; /* 1/4 --> 1/1 */
			else if (hdr->src_color == VDO_COL_FMT_YUV422V)
				CH_V_scale = 1; /* 1/4 --> 1/2 */
		} else if (jp->dst_color == VDO_COL_FMT_ABGR) {
			if (hdr->src_color == VDO_COL_FMT_YUV411)
				CH_H_scale = 0; /* 1/4 --> 1/1 */
			else if (hdr->src_color == VDO_COL_FMT_YUV422V)
				CH_V_scale = 1; /* 1/4 --> 1/2 */
		}
		break;

	case SCALE_EIGHTH:
		/* Decode to 1/8 */
		if (jp->dst_color == VDO_COL_FMT_YUV444) {
			if (hdr->src_color == VDO_COL_FMT_YUV420) {
				CH_H_scale = 2; /* 1/8 --> 1/4 */
				CH_V_scale = 2; /* 1/8 --> 1/4 */
			} else if (hdr->src_color == VDO_COL_FMT_YUV411)
				CH_H_scale = 1; /* 1/8 --> 1/2 */
			else if (hdr->src_color == VDO_COL_FMT_YUV422H)
				CH_H_scale = 2; /* 1/8 --> 1/4 */
			else if (hdr->src_color == VDO_COL_FMT_YUV422V)
				CH_V_scale = 2; /* 1/8 --> 1/4 */
		} else if (jp->dst_color == VDO_COL_FMT_YUV422H) {
			if (hdr->src_color == VDO_COL_FMT_YUV420)
				CH_V_scale = 2; /* 1/8 --> 1/4 */
			else if (hdr->src_color == VDO_COL_FMT_YUV411)
				CH_H_scale = 2; /* 1/8 --> 1/4 */
			else if (hdr->src_color == VDO_COL_FMT_YUV422V)
				DBG_ERR("Not support 422 case 1 for 1/8\n");
			else if (hdr->src_color == VDO_COL_FMT_YUV444)
				DBG_ERR("Not support 422 case 2 for 1/8\n");
		} else if (jp->dst_color == VDO_COL_FMT_YUV420) {
			if (hdr->src_color == VDO_COL_FMT_YUV411)
				DBG_ERR("Not support 420 case 1 for 1/8\n");
			else if (hdr->src_color == VDO_COL_FMT_YUV422H)
				DBG_ERR("Not support 420 case 2 for 1/8\n");
			else if (hdr->src_color == VDO_COL_FMT_YUV422V)
				DBG_ERR("Not support 420 case 3 for 1/8\n");
			else if (hdr->src_color == VDO_COL_FMT_YUV444)
				DBG_ERR("Not support 420 case 4 for 1/8\n");
		} else if ((jp->dst_color == VDO_COL_FMT_ARGB) ||
				(jp->dst_color == VDO_COL_FMT_ABGR)) {
			if (hdr->src_color == VDO_COL_FMT_YUV411)
				CH_H_scale = 2; /* 1/8 --> 1/4 */
			else if (hdr->src_color == VDO_COL_FMT_YUV422V)
				CH_V_scale = 2; /* 1/8 --> 1/4 */
		}
		break;

	default:
		DBG_ERR("color: %d -> %d\n", hdr->src_color, jp->dst_color);
		DBG_ERR("Not supported scale up (%d) now\n", jp->scale_factor);
		break;
	}
	JDEC_REG_SET32(JDEC_BASE + 0x30C, chroma_scale);

	JDEC_REG_SET32(JDEC_BASE + 0x300, ctx->scale_ratio); /* Y_SCALE_RATIO */
	JDEC_REG_SET32(JDEC_BASE + 0x304, CH_H_scale); /* C_SCALE_RATIO_H */
	JDEC_REG_SET32(JDEC_BASE + 0x308, CH_V_scale); /* C_SCALE_RATIO_V */

	return 0;
} /* End of do_chroma_scale_up() */

/*!*************************************************************************
* jdec_reset
*
* Private Function
*/
/*!
* \brief
*	JPEG hardware reset
*
* \retval  0 if success
*/
static int jdec_reset(void)
{
	TRACE("Enter\n");

	/* Enable power & clock */
	wmt_codec_pmc_ctl(CODEC_VD_JPEG, 1);
	wmt_codec_clock_en(CODEC_VD_JPEG, 1);

	/* Do MSVD SW reset */
	wmt_codec_msvd_reset(CODEC_VD_JPEG);

	/* Clear INT staus (write 1 clear) */
	JDEC_REG_SET32(JDEC_BASE + 0x024, 0x7F);

	TRACE("Leave\n");

	return 0;
} /* End of jdec_reset() */

/*!*************************************************************************
* wmt_jdec_set_attr
*
* API Function
*/
/*!
* \brief
*	JPEG hardware initialization
*
* \retval  0 if success
*/
int wmt_jdec_set_attr(struct jdec_ctx_t *ctx)
{
	jdec_prop_ex_t *jp = &ctx->prop;
	jdec_hdr_info_t    *hdr;
	vdo_color_fmt       src_color;
	vdo_color_fmt       dst_color;
	unsigned int        scanline;
	unsigned int        scale_value;
	unsigned int        scaled_width_luma, scaled_height_luma;
	unsigned int        scaled_width_chroma, scaled_height_chroma;
	vdo_color_fmt       tmp_dst_color;
	int factor_c;
	int pd_en = 0;

	hdr = &jp->hdr;

	ctx->_status = STA_ATTR_SET;
	jdec_reset();

	/*----------------------------------------------------------------------
		Set core settings
	----------------------------------------------------------------------*/
	get_mcu_unit(hdr->sof_w, hdr->sof_h, hdr->src_color,
				&ctx->src_mcu_width, &ctx->src_mcu_height);

	JDEC_REG_SET32(JDEC_BASE + 0x210, ctx->src_mcu_width);
	JDEC_REG_SET32(JDEC_BASE + 0x214, ctx->src_mcu_height);

	src_color = hdr->src_color;
	dst_color = jp->dst_color;

	if (ctx->_is_mjpeg == 1) {
		/* Set time out as 50 ms for MJPEG */
		ctx->_timeout = 50;
		wmt_codec_timer_reset(ctx->codec_tm, 0, ctx->_timeout);
	} else {
		/* Calculate time out value, According to experience,
			it is save to set 3x10e-5 ms per pixel */
		ctx->_timeout = ((hdr->sof_w * hdr->sof_h)*3/100000); /* ms */
		if (ctx->_timeout < 100)
			ctx->_timeout = 100; /* at least 300 ms for safety */

		/* Show decode time per frame */
		wmt_codec_timer_reset(ctx->codec_tm, 1, ctx->_timeout);
	}
	DBG_MSG("Timeout threshold: %d ms\n", ctx->_timeout);

	/* LOOK OUT: The order of vd_color_space is based on JDEC spce.
		If it is differernt with the order of your HW decoder,
		driver should tx it. */
	JDEC_REG_SET32(JDEC_BASE + 0x218, src_color);
	if (jp->pd.enable) {
		get_mcu_unit(jp->pd.x, jp->pd.y, src_color,
			&ctx->pd_mcu.x, &ctx->pd_mcu.y);
		get_mcu_unit(jp->pd.w, jp->pd.h, src_color,
			&ctx->pd_mcu.w, &ctx->pd_mcu.h);
		pd_en = 1;
	} else {
		pd_en = 0;
	}
	JDEC_REG_SET32(JDEC_BASE + 0x220, pd_en);  /* Partial decode */
	if (pd_en) {
		JDEC_REG_SET32(JDEC_BASE + 0x240, ctx->pd_mcu.w);
		JDEC_REG_SET32(JDEC_BASE + 0x244, ctx->pd_mcu.h);
	} else {
		JDEC_REG_SET32(JDEC_BASE + 0x240, ctx->src_mcu_width);
		JDEC_REG_SET32(JDEC_BASE + 0x244, ctx->src_mcu_height);
	}

	/* Settings for Partial Decode */
	JDEC_REG_SET32(JDEC_BASE + 0x230, (pd_en) ? ctx->pd_mcu.x : 0);
	JDEC_REG_SET32(JDEC_BASE + 0x234, (pd_en) ? ctx->pd_mcu.y : 0);
	JDEC_REG_SET32(JDEC_BASE + 0x238, (pd_en) ? ctx->pd_mcu.w : 0);
	JDEC_REG_SET32(JDEC_BASE + 0x23C, (pd_en) ? ctx->pd_mcu.h : 0);
	/*----------------------------------------------------------------------
		Picture scaling control
	----------------------------------------------------------------------*/
	switch (jp->scale_factor) {
	case SCALE_ORIGINAL:
		ctx->scale_ratio = 0;
		break;
	case SCALE_HALF:
		ctx->scale_ratio = 1;
		break;
	case SCALE_QUARTER:
		ctx->scale_ratio = 2;
		break;
	case SCALE_EIGHTH:
		ctx->scale_ratio = 3;
		break;
	default:
		DBG_ERR("Illegal scale ratio(%d)\n", jp->scale_factor);
		ctx->scale_ratio = 0;
		break;
	}
	/*----------------------------------------------------------------------
		Handle scanline offset issue
	----------------------------------------------------------------------*/
	ctx->decoded_w = (jp->pd.enable) ? jp->pd.w : hdr->sof_w;
	ctx->decoded_h = (jp->pd.enable) ? jp->pd.h : hdr->sof_h;
	scanline = ctx->decoded_w;

	DBG_MSG("decoded_w: %d, decoded_h: %d\n",
		ctx->decoded_w, ctx->decoded_h);
	DBG_MSG("dst_color: %d, scanline: %d\n", dst_color, scanline);

	/* Because of HW limitation, Y line width should be multiple of 64. */
	switch (dst_color) {
	case VDO_COL_FMT_YUV420:
		ctx->line_width_y =
			ALIGN64(ALIGN64(scanline) >> ctx->scale_ratio);
		ctx->line_width_y =
			ALIGN64(MAX(ctx->line_width_y, jp->scanline));
		ctx->line_width_c = ctx->line_width_y;
		ctx->bpp = 8;
		factor_c = 2;
		break;
	case VDO_COL_FMT_YUV422H:
		ctx->line_width_y =
			ALIGN64(ALIGN64(scanline) >> ctx->scale_ratio);
		ctx->line_width_y =
			ALIGN64(MAX(ctx->line_width_y, jp->scanline));
		ctx->line_width_c = ctx->line_width_y;
		ctx->bpp = 8;
		factor_c = 1;
		break;
	case VDO_COL_FMT_YUV422V:
		ctx->line_width_y =
			ALIGN64(ALIGN64(scanline) >> ctx->scale_ratio);
		ctx->line_width_y =
			ALIGN64(MAX(ctx->line_width_y, jp->scanline));
		ctx->line_width_c = ctx->line_width_y << 1;
		ctx->bpp = 8;
		factor_c = 2;
		break;
	case VDO_COL_FMT_YUV444:
		ctx->line_width_y =
			ALIGN64(ALIGN64(scanline) >> ctx->scale_ratio);
		ctx->line_width_y =
			ALIGN64(MAX(ctx->line_width_y, jp->scanline));
		ctx->line_width_c = ctx->line_width_y << 1;
		ctx->bpp = 8;
		factor_c = 1;
		break;
	case VDO_COL_FMT_YUV411:
		ctx->line_width_c =
			ALIGN64(ALIGN64(scanline) >> ctx->scale_ratio);
		ctx->line_width_c =
			ALIGN64(MAX(ctx->line_width_c, jp->scanline));
		ctx->line_width_y = ctx->line_width_c << 1;
		ctx->bpp = 8;
		factor_c = 2;
		break;
	case VDO_COL_FMT_ARGB:
	case VDO_COL_FMT_ABGR:
		ctx->line_width_y =
			ALIGN64(ALIGN64(scanline << 2) >> ctx->scale_ratio);
		ctx->line_width_y =
			ALIGN64(MAX(ctx->line_width_y, jp->scanline));
		ctx->line_width_c = 0;
		do_yuv2rgb(dst_color);
		ctx->bpp = 32;
		factor_c = 1;
		break;
	case VDO_COL_FMT_GRAY:
		ctx->line_width_y =
			ALIGN64(ALIGN64(scanline) >> ctx->scale_ratio);
		ctx->line_width_y =
			ALIGN64(MAX(ctx->line_width_y, jp->scanline));
		ctx->line_width_c = 0;
		ctx->bpp = 8;
		factor_c = 1;
		break;
	default:
		DBG_ERR("Unknown color format(%d)\n", dst_color);
		factor_c = 1; /* to avoid divid zero */
		break;
	}
	DBG_MSG("sof_w: %d, sof_h: %d\n", hdr->sof_w, hdr->sof_h);
	DBG_MSG("line_width_y: %d, line_width_c: %d\n",
		ctx->line_width_y, ctx->line_width_c);

	ctx->line_height = (((ctx->decoded_h + 15)>>4)<<4) >> ctx->scale_ratio;
	ctx->req_y_size  = (ctx->line_width_y * ctx->line_height);
	ctx->req_c_size  = (ctx->line_width_c * ctx->line_height)/factor_c;

	DBG_MSG("Required memory size(Y, C): (%d, %d)\n",
		ctx->req_y_size, ctx->req_c_size);

	do_chroma_scale_up(ctx, jp);

	JDEC_REG_SET32(JDEC_BASE + 0x254, ctx->line_width_y);
	JDEC_REG_SET32(JDEC_BASE + 0x25C, ctx->line_width_c);

	/*----------------------------------------------------------------------
		Set scaled width & hegith of Y & C
		The following codes are gotten from IC team. We cannot fully
			understand what it is.
	----------------------------------------------------------------------*/
	scale_value = (1 << (ctx->scale_ratio));
	scaled_width_luma  = ctx->decoded_w / scale_value;
	if ((ctx->decoded_w % scale_value) == 0)
		scaled_width_luma -= 1;

	scaled_height_luma = ctx->decoded_h / scale_value;
	if ((ctx->decoded_h % scale_value) == 0)
		scaled_height_luma -= 1;

	tmp_dst_color = dst_color;
	if ((tmp_dst_color == VDO_COL_FMT_ARGB) ||
		(tmp_dst_color == VDO_COL_FMT_ABGR)) {
		tmp_dst_color = src_color;
		if ((src_color == VDO_COL_FMT_YUV411) ||
			(src_color == VDO_COL_FMT_YUV422V) ||
			(src_color == VDO_COL_FMT_GRAY)) {
			tmp_dst_color = VDO_COL_FMT_YUV444;
		}
	}

	switch (tmp_dst_color) {
	case VDO_COL_FMT_YUV420:
		scaled_width_chroma = (((scaled_width_luma+1) + 1) / 2) * 2 - 1;
		scaled_height_chroma = (((scaled_height_luma+1) + 1) / 2) - 1;
		break;
	case VDO_COL_FMT_YUV422H:
		scaled_width_chroma = (((scaled_width_luma+1) + 1) / 2) * 2 - 1;
		scaled_height_chroma = scaled_height_luma;
		break;
	case VDO_COL_FMT_YUV444:
		scaled_width_chroma = (scaled_width_luma+1) * 2 - 1;
		scaled_height_chroma = scaled_height_luma;
		break;
	default:
		DBG_ERR("Unexcepted color (%d)\n", tmp_dst_color);
		return -1;
	}
	JDEC_REG_SET32(JDEC_BASE + 0x320, scaled_width_luma);
	JDEC_REG_SET32(JDEC_BASE + 0x324, scaled_height_luma);
	JDEC_REG_SET32(JDEC_BASE + 0x328, scaled_width_chroma);
	JDEC_REG_SET32(JDEC_BASE + 0x32C, scaled_height_chroma);

	return 0;
} /* End of wmt_jdec_set_attr() */

/*!*************************************************************************
* set_prd_table
*
* API Function
*/
/*!
* \brief
*	Set source PRD table
*
* \retval  0 if success
*/
static int set_prd_table(struct jdec_ctx_t *ctx)
{
	unsigned int  prd_addr_in;
	int i;

	DBG_PRD("Dma prd virt addr: 0x%08x\n", ctx->prd_virt);
	DBG_PRD("Dma prd phy  addr: 0x%08x\n", ctx->prd_addr);

	prd_addr_in = ctx->prd_virt;
	for (i = 0; ; i += 2) {
		unsigned int  addr, len;

		addr = *(unsigned int *)(prd_addr_in + i * 4);
		len  = *(unsigned int *)(prd_addr_in + (i + 1) * 4);

		DBG_PRD("[%02d]Addr: 0x%08x\n", i, addr);
		DBG_PRD("    Len:  0x%08x (%d)\n", len, (len & 0xFFFF));

		if (len & 0x80000000)
			break;
	}

	JDEC_REG_SET32(JDEC_BASE + 0x100, ctx->prd_addr);

	DBG_MSG("PRD_ADDR: 0x%x\n", REG32_VAL(JDEC_BASE + 0x100));

	return 0;
} /* End of set_prd_table() */

/*!*************************************************************************
* wmt_jdec_decode_proc
*
* API Function
*/
/*!
* \brief
*	Open JPEG hardware
*
* \retval  0 if success
*/
int wmt_jdec_decode_proc(struct jdec_ctx_t *ctx)
{
	jdec_prop_ex_t *jp = &ctx->prop;
	int ret = -EINVAL;

	/* Set PRD table */
	set_prd_table(ctx);

	/* Check whether space of decoded buffer is enough or not */
	if ((jp->dst_color != VDO_COL_FMT_ARGB) &&
		(jp->dst_color != VDO_COL_FMT_ABGR)) {
		/* YUV color domain */
		if (jp->dst_y_size < ctx->req_y_size) {
			DBG_ERR("Buffer size (Y) is not enough (%d < %d)\n",
				jp->dst_y_size, ctx->req_y_size);
			goto EXIT_wmt_jdec_decode_proc;
		}
		if (jp->dst_c_size < ctx->req_c_size) {
			DBG_ERR("Buffer size (C) is not enough (%d < %d)\n",
				jp->dst_c_size, ctx->req_c_size);
			goto EXIT_wmt_jdec_decode_proc;
		}
	} else {
		/* RGB color domain */
		if (jp->dst_y_size < ctx->req_y_size) {
			DBG_ERR("Buffer size (ARGB) is not enough (%d < %d)\n",
				jp->dst_y_size, ctx->req_y_size);
			DBG_ERR("line_width_y: %d, line_height: %d\n",
				ctx->line_width_y, ctx->line_height);
			DBG_ERR("SOF (%d x %d)\n",
				jp->hdr.sof_w, jp->hdr.sof_h);
			DBG_ERR("Decoded (%d x %d)\n",
				ctx->decoded_w, ctx->decoded_h);
			DBG_ERR(" scale_ratio: %d\n", ctx->scale_ratio);

			goto EXIT_wmt_jdec_decode_proc;
		}
	}
	if (jp->dst_y_addr == 0) {
		DBG_ERR("NULL Y Addr! Stop decoding!\n");
		goto EXIT_wmt_jdec_decode_proc;
	}
	ret = 0;

	if ((jp->scanline != 0) && (jp->scanline > ctx->line_width_y)) {
		DBG_MSG("scanline: %d\n", jp->scanline);
		/* In this case, we only support 4:2:0 mode */
		ctx->line_width_y = jp->scanline;
		ctx->line_width_c = ctx->line_width_y >> 1; /* temp */

		JDEC_REG_SET32(JDEC_BASE + 0x254, ctx->line_width_y);
		JDEC_REG_SET32(JDEC_BASE + 0x25C, ctx->line_width_c);
	}

	ctx->_status = STA_DMA_START;
	JDEC_REG_SET32(JDEC_BASE + 0x260, 0); /* Split Write enable/disable */
	JDEC_REG_SET32(JDEC_BASE + 0x264, 0); /* Split Write MCU height */

	JDEC_REG_SET32(JDEC_BASE + 0x200, 1); /* Picture Init */
	JDEC_REG_SET32(JDEC_BASE + 0x200, 0); /* End of Picture Init */

	JDEC_REG_SET32(JDEC_BASE + 0x250, jp->dst_y_addr);
	JDEC_REG_SET32(JDEC_BASE + 0x258, jp->dst_c_addr);
	/*----------------------------------------------------------------------
		Interrupt enable
			bit 4: JPEG bitstream ERROR INT enable
			bit 3: JBSDMA INT enable
			bit 2: JPEG HW parsing SOF INT enable
			bit 1: VLD decode finish INT enable
			bit 0: JPEG decoded frame out finish INT enable
	----------------------------------------------------------------------*/
	JDEC_REG_SET32(JDEC_BASE + 0x020, 0x11);

	ctx->_status |= STA_DECODEING;
	wmt_codec_timer_start(ctx->codec_tm);
	wmb();
	JDEC_REG_SET32(JDEC_BASE + 0x108, 1);  /* Set JBSDMA Start Enable */
	wmb();

EXIT_wmt_jdec_decode_proc:

	return ret;
} /* End of wmt_jdec_decode_proc() */

/*!*************************************************************************
* set_frame_info
*
* API Function
*/
/*!
* \brief
*	Set frame buffer information
*
* \retval  0 if success
*/
static int set_frame_info(struct jdec_ctx_t *ctx)
{
	jdec_prop_ex_t *jp = &ctx->prop;
	jdec_frameinfo_t *jfb;
	unsigned int      y_base, c_base;

	jfb = &jp->jfb;

	/* Physical address */
	y_base = jp->dst_y_addr;
	c_base = jp->dst_c_addr;

	/* Set information for application */
	memset(&jfb->fb, 0, sizeof(vdo_framebuf_t));
	jfb->fb.img_w   = ctx->decoded_w / (1 << ctx->scale_ratio);
	jfb->fb.img_h   = ctx->decoded_h / (1 << ctx->scale_ratio);
	jfb->fb.fb_w    = ctx->line_width_y/(ctx->bpp >> 3);
	jfb->fb.fb_h    = ctx->line_height;
	jfb->fb.col_fmt = jp->dst_color;
	jfb->fb.bpp     = ctx->bpp;
	jfb->scaled     = 0;

	jfb->fb.y_addr    = jp->dst_y_addr;
	jfb->fb.y_size    = jp->dst_y_size;

	if ((jfb->fb.col_fmt != VDO_COL_FMT_ARGB) &&
		(jfb->fb.col_fmt != VDO_COL_FMT_ABGR)) {
		/* YUV color domain */
		jfb->fb.c_addr    = jp->dst_c_addr;
		jfb->fb.c_size    = jp->dst_c_size;
	}

	if ((jp->hdr.src_color == VDO_COL_FMT_GRAY) &&
		(jp->dst_color != VDO_COL_FMT_ARGB) &&
		(jp->dst_color != VDO_COL_FMT_ABGR)) {
		/* Draw C plane for gray level JPEG under YUV color space */
		unsigned int vir_addr;
		/* Now Y and C are in same buffer (allocated once only) */
		/* We msut do y_addr re-mapping at first to get virt c_addr */
		vir_addr = (unsigned int)mb_phys_to_virt(jfb->fb.y_addr);
		vir_addr += jfb->fb.y_size;
		memset((void *)vir_addr, 0x80, jfb->fb.c_size);
	}

	if ((jp->scale_factor == SCALE_HALF) ||
		(jp->scale_factor == SCALE_QUARTER) ||
		(jp->scale_factor == SCALE_EIGHTH)) {
		jfb->scaled = jp->scale_factor;
	}
	return 0;
} /* End of set_frame_info()*/

/*!*************************************************************************
* wmt_jdec_decode_finish
*
* API Function
*/
/*!
* \brief
*	Wait JPEG hardware finished
*
* \retval  0 if success
*/
int wmt_jdec_decode_finish(struct jdec_ctx_t *ctx)
{
	int ret;

	ret = wait_decode_finish(ctx);
	set_frame_info(ctx);

	return ret;
} /* End of wmt_jdec_decode_finish() */

/*!*************************************************************************
* wmt_jdec_isr
*
* API Function
*/
/*!
* \brief
*	Open JPEG hardware
*
* \retval  0 if not handle INT, others INT has been handled.
*/
irqreturn_t wmt_jdec_isr(int irq, void *isr_data)
{
	#define INT_BIT_CLEAR(a, bit)\
		do {\
			a &= (0xFF & (~(1<<bit)));\
			JDEC_REG_SET32(JDEC_BASE + 0x020, irq_en);\
			JDEC_REG_SET32(JDEC_BASE + 0x024, (1<<bit));\
		} while (0)

	#define INT_BIT_GET(val, bit)      ((val) & (1<<bit))

#ifdef JDEC_IRQ_MODE
	struct jdec_ctx_t *ctx = isr_data;
	unsigned int    reg_val, irq_en;
	unsigned int    bsdma_int, error_int, picend_int;
	unsigned int    wake_up;

	TRACE("Enter\n");

	if ((ctx->_status & STA_DECODEING) == 0) {
		/* Not in decoding state */
		reg_val = REG32_VAL(JDEC_BASE + 0x024);
		DBG_MSG("[%d] Wrong state (0x%x), reg_val (0x%x)\n",
			ctx->ref_num, ctx->_status, reg_val);
		return IRQ_NONE;
	}
	irq_en  = REG32_VAL(JDEC_BASE + 0x020);
	reg_val = REG32_VAL(JDEC_BASE + 0x024);

	DBG_MSG("irq_en: 0x%x, reg_val: 0x%0x\n", irq_en, reg_val);

	picend_int  = INT_BIT_GET(reg_val, 0);
	bsdma_int   = INT_BIT_GET(reg_val, 3);
	error_int   = INT_BIT_GET(reg_val, 4);

	if (!bsdma_int && !error_int && !picend_int) {
		DBG_ERR("(id: %d) Unexpected reg_val (0x%x): state (0x%x)\n",
				ctx->ref_num, reg_val, ctx->_status);
		return IRQ_NONE;
	}

	wake_up = 0;
	if (error_int) {
		DBG_ERR("Some error happened(reg_val: 0x%x)!\n", reg_val);

		ctx->_status |= STA_ERR_DECODING;

		/* Disable all interrupt */
		INT_BIT_CLEAR(irq_en, 4);
		wake_up = 1;
	}
	if (bsdma_int) { /* All data are moved by DMA */
		ctx->_status &= (~STA_DMA_START);
		ctx->_status |= STA_DMA_MOVE_DONE;

		DBG_MSG("JBSDMA Move Done (_status: 0x%x)\n", ctx->_status);
		DBG_MSG("read_byte(%d)\n", REG32_VAL(JDEC_BASE + 0x110));

		/* Disable BSDMA interrupt */
		INT_BIT_CLEAR(irq_en, 3);

		wake_up = 1;
	}
	if (picend_int) { /* Full image are decoded and write to DRAM */
		ctx->_status &= (~STA_DECODEING);
		ctx->_status |= STA_DECODE_DONE;

		DBG_MSG("HW Decode Finish (_status: 0x%x)\n", ctx->_status);

		clean_interrup();
		wake_up = 1;
		wmt_codec_timer_stop(ctx->codec_tm);
	}
	if (wake_up)
		wake_up_interruptible(&jdec_wait);

#endif /* #ifdef JDEC_IRQ_MODE */
	TRACE("Leave\n");

return IRQ_HANDLED;
} /* End of wmt_jdec_isr() */

/*!*************************************************************************
* wmt_jdec_open
*
* API Function
*/
/*!
* \brief
*	Open JPEG hardware
*
* \retval  0 if success
*/
int wmt_jdec_open(struct jdec_ctx_t *ctx)
{
	int ret = 0;

#ifdef JDEC_IRQ_MODE
	ret = request_irq(IRQ_JDEC, wmt_jdec_isr, IRQF_SHARED, "jdec", ctx);
	if (ret < 0) {
		DBG_ERR("Request %s IRQ %d Fail\n", "jenc", IRQ_JDEC);
		return -1;
	}
	DBG_MSG("Request %s IRQ %d OK\n", "jenc", IRQ_JDEC);
#endif

	wmt_codec_timer_init(&ctx->codec_tm, "<jdec>", 0, 0 /*ms*/);

	return ret;
} /* End of wmt_jdec_open() */

/*!*************************************************************************
* wmt_jdec_close
*
* API Function
*/
/*!
* \brief
*	Close JPEG hardware
*
* \retval  0 if success
*/
int wmt_jdec_close(struct jdec_ctx_t *ctx)
{
#ifdef JDEC_IRQ_MODE
	if (ctx)
		free_irq(IRQ_JDEC, (void *)ctx);
#endif
	wmt_codec_timer_exit(ctx->codec_tm);

	return 0;
} /* End of wmt_jdec_close() */

/*--------------------End of Function Body -------------------------------*/

#undef WMT_JDEC_C
