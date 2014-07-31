/*++
 * WonderMedia SoC hardware JPEG encoder driver
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

#define WMT_JENC_C

#include <asm/cacheflush.h>         /* for dmac_flush_range() */

#include "hw-jenc.h"


#define THE_MB_USER         "WMT-JENC"
#define MB_MMU

#define ALIGN64(a)          (((a)+63) & (~63))
#define ALIGN128(a)         (((a)+127) & (~127))

#define PRINTK(fmt, args...)\
		do {\
			printk(KERN_INFO " " fmt, ## args);\
		} while (0)

/*#define JENC_REG_TRACE*/
#ifdef JENC_REG_TRACE
#define JENC_REG_SET(addr, val)  \
		do {\
			PRINTK("REG_SET:0x%x -> 0x%0x\n", addr, val);\
			REG32_VAL(addr) = (val);\
		} while (0)
#else
#define JENC_REG_SET(addr, val)       (REG32_VAL(addr) = (val))
#endif

#define DCT_SIZE            64

/*--------------------------------------------------------------------------
	JPEG_ENCODER_BASE_ADDR was defined in
	\arch\arm\mach-wmt\include\mach\wmt_mmap.h
--------------------------------------------------------------------------*/
#define JENC_BASE      JPEG_ENCODER_BASE_ADDR

enum {
	GRAY_TO_GRAY     = 0,
	YC444_TO_YC444   = 1,
	YC422V_TO_YC422V = 2,
	YC444_TO_YC422V  = 3,
	YC444_TO_YC420   = 4,
	YC422V_TO_YC420  = 5,
	YC420_TO_YC420   = 6,
	YC422H_TO_YC420  = 7,
	YC444_TO_YC422H  = 8,
	YC422H_TO_YC422H = 9,

	RGB_TO_YC444     = YC444_TO_YC444,
	RGB_TO_YC422V    = YC444_TO_YC422V,
	RGB_TO_YC422H    = YC444_TO_YC422H,
	RGB_TO_YC420     = YC444_TO_YC420
};


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
static void dbg_dump_registers(struct jenc_ctx *ctx)
{
	#define Dump_RegisterValue(addr) \
		do {\
			PRINTK("REG(0x%x): 0x%x\n", addr, REG32_VAL(addr));\
		} while (0)

	wmt_codec_dump_prd(ctx->prd_virt, 0);

	PRINTK("MSVD Regsiters:\n");
	Dump_RegisterValue(MSVD_BASE_ADDR + 0x060);
	Dump_RegisterValue(MSVD_BASE_ADDR + 0x064);
	Dump_RegisterValue(MSVD_BASE_ADDR + 0x068);
	Dump_RegisterValue(MSVD_BASE_ADDR + 0x06C);
	Dump_RegisterValue(MSVD_BASE_ADDR + 0x070);

	PRINTK("JENC Regsiters:\n");
	Dump_RegisterValue(JENC_BASE + 0x000);
	Dump_RegisterValue(JENC_BASE + 0x004);
	Dump_RegisterValue(JENC_BASE + 0x008);
	Dump_RegisterValue(JENC_BASE + 0x00C);
	Dump_RegisterValue(JENC_BASE + 0x018);
	Dump_RegisterValue(JENC_BASE + 0x020);
	Dump_RegisterValue(JENC_BASE + 0x024);
	Dump_RegisterValue(JENC_BASE + 0x030);
	Dump_RegisterValue(JENC_BASE + 0x034);
	Dump_RegisterValue(JENC_BASE + 0x038);
	Dump_RegisterValue(JENC_BASE + 0x040);
	Dump_RegisterValue(JENC_BASE + 0x044);
	Dump_RegisterValue(JENC_BASE + 0x048);
	Dump_RegisterValue(JENC_BASE + 0x050);
	Dump_RegisterValue(JENC_BASE + 0x060);
	Dump_RegisterValue(JENC_BASE + 0x064);
	Dump_RegisterValue(JENC_BASE + 0x068);
	Dump_RegisterValue(JENC_BASE + 0x070);
	Dump_RegisterValue(JENC_BASE + 0x074);
	Dump_RegisterValue(JENC_BASE + 0x078);

	Dump_RegisterValue(JENC_BASE + 0x100);
	Dump_RegisterValue(JENC_BASE + 0x104);
	Dump_RegisterValue(JENC_BASE + 0x108);
	Dump_RegisterValue(JENC_BASE + 0x10C);
	Dump_RegisterValue(JENC_BASE + 0x110);
	Dump_RegisterValue(JENC_BASE + 0x114);
	Dump_RegisterValue(JENC_BASE + 0x118);
	Dump_RegisterValue(JENC_BASE + 0x11C);
	Dump_RegisterValue(JENC_BASE + 0x120);
	Dump_RegisterValue(JENC_BASE + 0x124);
	Dump_RegisterValue(JENC_BASE + 0x128);
	Dump_RegisterValue(JENC_BASE + 0x130);
	Dump_RegisterValue(JENC_BASE + 0x134);
	Dump_RegisterValue(JENC_BASE + 0x138);
	Dump_RegisterValue(JENC_BASE + 0x140);
	Dump_RegisterValue(JENC_BASE + 0x144);

	Dump_RegisterValue(JENC_BASE + 0x200);
	Dump_RegisterValue(JENC_BASE + 0x204);
	Dump_RegisterValue(JENC_BASE + 0x208);
	Dump_RegisterValue(JENC_BASE + 0x20C);
	Dump_RegisterValue(JENC_BASE + 0x210);
	Dump_RegisterValue(JENC_BASE + 0x214);
	Dump_RegisterValue(JENC_BASE + 0x218);
	Dump_RegisterValue(JENC_BASE + 0x300);
	Dump_RegisterValue(JENC_BASE + 0x304);

} /* End of dbg_dump_registers() */

/*!*************************************************************************
* wait_encode_finish
*
* Private Function
*/
/*!
* \brief
*   Wait HW encoder finish job
*
* \retval  none
*/
#ifdef JENC_DEBUG
static void print_quantizer_table(unsigned char *pQT_in, int quality)
{
	unsigned char *pQT = pQT_in;
	int j, k;

	PRINTK("Luminance QT (quality: %d)\n", quality);
	for (j = 0; j < DCT_SIZE/8; j++) {
		for (k = 0; k < 8; k++) {
			PRINTK("%3d ", *pQT++);
		PRINTK("\n");
	}
	PRINTK("Chrominance QT (quality: %d)\n", quality);
	for (j = 0; j < DCT_SIZE/8; j++) {
		for (k = 0; k < 8; k++) {
			PRINTK("%3d ", *pQT++);
		PRINTK("\n");
	}
} /* End of print_quantizer_table() */
#endif /* #ifdef JENC_DEBUG */

/*!*************************************************************************
* wait_encode_finish
*
* Private Function
*/
/*!
* \brief
*   Wait HW encoder finish job
*
* \retval  none
*/
static int wait_encode_finish(struct jenc_ctx *ctx)
{
	struct jenc_prop *jprop = &ctx->prop;
	int ret;
	int err_happen = 0;

	if (ctx->_status & VE_STA_ERR_UNKNOWN) {
		DBG_ERR("Unexcepted status: 0x%x\n", ctx->_status);
		err_happen = 1;
	}
#ifdef JENC_IRQ_MODE
	/*----------------------------------------------------------------------
		Wait intrrupt (JENC_INTRQ)
	----------------------------------------------------------------------*/
	if (err_happen == 0) {
		unsigned long msecs;
		int ret;
		msecs = msecs_to_jiffies(ctx->_timeout);
		ret = wait_event_interruptible_timeout(ctx->wait_event,
			(ctx->_status & VE_STA_ENCODE_DONE), msecs);
		if (ret == 0) {
			DBG_ERR("Encoding timeout over %d ms\n", ctx->_timeout);
			err_happen = 2;
		}
	}
#else
	/* #ifdef JENC_POLL_MODE */
	unsigned int  reg_val;

	while (1) {
		reg_val = REG32_VAL(REG_JENC_INT_STATUS);
		if (reg_val & 0x01)
			break;
		else if (reg_val & 0x10) {
			DBG_ERR("reg_val: 0x%x at 0x%x\n",
				reg_val, REG_JENC_INT_STATUS);
			break;
		}

		count++;
		if (count >= 300000) {
			err_happen = 1;
			DBG_ERR("Timeout over %d (_status: 0x%x)\n",
				count, ctx->_status);
			JENC_REG_SET(REG_JENC_INT_ENALBE, 0x0);
			JENC_REG_SET(REG_JENC_INT_STATUS, 0x3F);
			JENC_REG_SET(REG_JENC_BSDMA_START_TX, 0);
			break;
		}
	}
#endif
	if (err_happen == 0) {
		jprop->enc_size = REG32_VAL(JENC_BASE + 0x070);
		DBG_MSG("[INT] Encode Finish (Len: %d)\n", jprop->enc_size);
		ret = 0;
	} else {
		DBG_ERR("Encode JEPG NOT complete\n");
		DBG_ERR("Time out: %d ms\n", ctx->_timeout);
		DBG_ERR("Driver status: 0x%08x\n", ctx->_status);

		dbg_dump_registers(ctx);
		DBG_ERR("Encode JPEG NOT complete\n");
		jprop->enc_size = 0;
		ret = -1;
	}
	/* Free share SRAM */
	wmt_ve_free_sram(VE_JPEG);

	/* Disable power & clock to save power consumption */
	wmt_codec_clock_en(CODEC_VE_JPEG, 0);
	wmt_codec_pmc_ctl(CODEC_VE_JPEG, 0);

	return ret;
} /* End of wait_encode_finish() */

/*!*************************************************************************
* get_quantization_table
*
* Private Function
*/
/*!
* \brief   Get quantization table
*
* \parameter
*
* \retval  0 if success, else return a negtive value
*/
static int get_quantization_table(unsigned char *pQT, int quality)
{
	const unsigned char luminance_quant_tbl[DCT_SIZE] = {
		16,  11,  10,  16,  24,  40,  51,  61,
		12,  12,  14,  19,  26,  58,  60,  55,
		14,  13,  16,  24,  40,  57,  69,  56,
		14,  17,  22,  29,  51,  87,  80,  62,
		18,  22,  37,  56,  68, 109, 103,  77,
		24,  35,  55,  64,  81, 104, 113,  92,
		49,  64,  78,  87, 103, 121, 120, 101,
		72,  92,  95,  98, 112, 100, 103,  99
	};
	const unsigned char chrominance_quant_tbl[DCT_SIZE] = {
		17,  18,  24,  47,  99,  99,  99,  99,
		18,  21,  26,  66,  99,  99,  99,  99,
		24,  26,  56,  99,  99,  99,  99,  99,
		47,  66,  99,  99,  99,  99,  99,  99,
		99,  99,  99,  99,  99,  99,  99,  99,
		99,  99,  99,  99,  99,  99,  99,  99,
		99,  99,  99,  99,  99,  99,  99,  99,
		99,  99,  99,  99,  99,  99,  99,  99
	};

	const unsigned char zig_zag_tbl[DCT_SIZE] = {
		 0,   1,   5,   6,  14,  15,  27,  28,
		 2,   4,   7,  13,  16,  26,  29,  42,
		 3,   8,  12,  17,  25,  30,  41,  43,
		 9,  11,  18,  24,  31,  40,  44,  53,
		10,  19,  23,  32,  39,  45,  52,  54,
		20,  22,  33,  38,  46,  51,  55,  60,
		21,  34,  37,  47,  50,  56,  59,  61,
		35,  36,  48,  49,  57,  58,  62,  63
	};

	unsigned char tmp_tbl[DCT_SIZE*2];
	int scale_factor, i, k;
	unsigned char *basic_table;
	unsigned char *ptr;
	long temp;

	if (quality <= 0)
		quality = 1;
	if (quality > 100)
		quality = 100;

	scale_factor = (quality < 50) ? (5000 / quality) : (200 - quality*2);

	for (k = 0; k < 2; k++) {
		if (k == 0)
			basic_table = (unsigned char *)(&luminance_quant_tbl);
		else
			basic_table = (unsigned char *)(&chrominance_quant_tbl);

		ptr = tmp_tbl + k * DCT_SIZE;
		for (i = 0; i < DCT_SIZE; i++) {
			temp = ((long)basic_table[i] * scale_factor + 50L)/100L;
			/* limit the values to the valid range */
			if (temp <= 0L)
				temp = 1L;
			if (temp > 32767L)
				temp = 32767L; /* max quanti need for 12 bits */
			if (temp > 255L)
				temp = 255L;   /* limit to baseline range */
			*ptr++ = (unsigned char) temp;
	}
	}
#ifdef JENC_DEBUG
	ptr = tmp_tbl;
	for (k = 0; k < DCT_SIZE*2; k++)
		pQT[k] = *ptr++;
	print_quantizer_table(pQT, quality);
#endif
	/*---------------------------------------------------------------------
		Do Zig Zag transform
	---------------------------------------------------------------------*/
	for (k = 0; k < 2; k++) {
		ptr = tmp_tbl + k * DCT_SIZE;
		for (i = 0; i < DCT_SIZE; i++)
			pQT[zig_zag_tbl[i] +  k * DCT_SIZE] = *ptr++;
	}
	return 0;
} /* End of get_quantization_table() */

/*!*************************************************************************
* jenc_initial
*
* Private Function
*/
/*!
* \brief
*
* \retval  0 if success
*/
static int jenc_initial(void)
{
	/* Enable power & clock */
	wmt_codec_pmc_ctl(CODEC_VE_JPEG, 1);
	wmt_codec_clock_en(CODEC_VE_JPEG, 1);

	if (wmt_ve_request_sram(VE_JPEG) != 0) {
		DBG_ERR("Reqest SRAM for JENC fail\n");
		return -1;
	}
	/* Software Reset */
	wmt_codec_msvd_reset(CODEC_VE_JPEG);

	return 0;
} /* End of jenc_initial() */

/*!*************************************************************************
* set_input_buf
*
* Private Function
*/
/*!
* \brief
*
* \retval  0 if success
*/
static int set_input_buf(struct jenc_ctx *ctx)
{
	struct jenc_prop *jprop = &ctx->prop;
	int pixel_num_y, pixel_num_c;
	int color;

	/*----------------------------------------------------------------------
		For RGB domain, pixel_num is all pixel sample number
		For YC domain, pixel_num is pixel sample number of Y plane
	----------------------------------------------------------------------*/
	pixel_num_y = ctx->pic_width * ctx->pic_height;
	pixel_num_c = pixel_num_y;
	DBG_MSG("Total pixel sample: %d\n", pixel_num_y);

	color = ctx->enc_color;
	switch (ctx->src_color) {
	case VDO_COL_FMT_YUV420:
		pixel_num_c >>= 2;
		break;
	case VDO_COL_FMT_YUV422H:
	case VDO_COL_FMT_YUV422V:
		pixel_num_c >>= 1;
		break;
	case VDO_COL_FMT_YUV444:
	case VDO_COL_FMT_ARGB:
	case VDO_COL_FMT_ABGR:
	case VDO_COL_FMT_RGB_565:
		break;
	case VDO_COL_FMT_NV21:
		pixel_num_c >>= 2;
		/* for Cr Cb. (default is Cb Cr)*/
		color = color | (1 << 4);
		break;
	default:
		DBG_WARN("Unexpected color format(%d)\n", ctx->src_color);
		break;
	}

	if (jprop->mem_mode == MEM_MODE_USR) {
		unsigned int work_buf_addr;

		/* Set source color format for MMU Mode */
		JENC_REG_SET(JENC_BASE + 0x104, ctx->mmu.color);

		/* Set MMU table */
		DBG_MSG("tbl_y.addr:   0x%x\n", ctx->mmu.tbl_y.addr);
		DBG_MSG("tbl_y.offset: 0x%x\n", ctx->mmu.tbl_y.offset);
		DBG_MSG("tbl_y.size:   %d\n",   ctx->mmu.tbl_y.size);
		DBG_MSG("tbl_c.addr:   0x%x\n", ctx->mmu.tbl_c.addr);
		DBG_MSG("tbl_c.offset: 0x%x\n", ctx->mmu.tbl_c.offset);
		DBG_MSG("tbl_c.size:   %d\n",   ctx->mmu.tbl_c.size);

		JENC_REG_SET(JENC_BASE + 0x108, ctx->mmu.tbl_y.addr);
		JENC_REG_SET(JENC_BASE + 0x10C, ctx->mmu.tbl_y.offset);
		if (ctx->mmu.tbl_c.addr) {
			JENC_REG_SET(JENC_BASE + 0x110, ctx->mmu.tbl_c.addr);
			JENC_REG_SET(JENC_BASE + 0x114, ctx->mmu.tbl_c.offset);
		}
		JENC_REG_SET(JENC_BASE + 0x038, color);
		JENC_REG_SET(JENC_BASE + 0x120,
			((ctx->cburstnum-1) << 16) + (ctx->y64bnum-1));
		JENC_REG_SET(JENC_BASE + 0x140, ctx->mcu_row_y);
		JENC_REG_SET(JENC_BASE + 0x144, ctx->mcu_row_c);
		JENC_REG_SET(JENC_BASE + 0x118, pixel_num_y);

		DBG_MSG("Total pixel sample in C Plane: %d\n", pixel_num_c);
		JENC_REG_SET(JENC_BASE + 0x11C, pixel_num_c);

		JENC_REG_SET(JENC_BASE + 0x124, ctx->pic_cburstnum-1);

		/* Set Working Buffers */
		work_buf_addr = ctx->mmu.work_buf_addr;
		JENC_REG_SET(JENC_BASE + 0x130, work_buf_addr); /* Y0 addr */
		work_buf_addr += ctx->mmu.work_size;
		JENC_REG_SET(JENC_BASE + 0x134, work_buf_addr); /* C0 addr */
		work_buf_addr += (ctx->mmu.work_size * 2);
		JENC_REG_SET(JENC_BASE + 0x138, work_buf_addr); /* Y1 addr */
		work_buf_addr += ctx->mmu.work_size;
		JENC_REG_SET(JENC_BASE + 0x13C, work_buf_addr); /* C1 addr */
	} else { /* if( jprop->mem_mode == MEM_MODE_USR ) */
		JENC_REG_SET(JENC_BASE + 0x038, color);

		JENC_REG_SET(JENC_BASE + 0x100, 0x00);  /* disable JENC DMA */

		JENC_REG_SET(JENC_BASE + 0x000, ctx->src_addr);
		JENC_REG_SET(JENC_BASE + 0x008, ctx->c_addr);
	}
	return 0;
} /* End of set_input_buf() */

/*!*************************************************************************
* set_jenc_core_param
*
* Private Function
*/
/*!
* \brief
*
* \retval  0 if success
*/
static int set_jenc_core_param(struct jenc_ctx *ctx)
{
	struct jenc_prop *jprop = &ctx->prop;
	unsigned char  QT[DCT_SIZE * 2]; /* quantization table after zig-zag */
	unsigned char *pQT;
	int i;

	/*----------------------------------------------------------------------
		Get quantization table
	----------------------------------------------------------------------*/
	get_quantization_table((unsigned char *)&QT, jprop->quality);

	JENC_REG_SET(JENC_BASE + 0x030, ctx->pic_width);  /* Src width */
	JENC_REG_SET(JENC_BASE + 0x034, ctx->pic_height); /* Srcheight */

	if (jprop->mem_mode == MEM_MODE_USR) {
		JENC_REG_SET(JENC_BASE + 0x100, 0x01);  /* Start JENC DMA */
		JENC_REG_SET(JENC_BASE + 0x01C, 1);
		ctx->_status |= VE_STA_DMA_START;
	}
	/*----------------------------------------------------------------------
		Set interrup mask
	----------------------------------------------------------------------*/
	JENC_REG_SET(JENC_BASE + 0x020, 0x03);

	/* Set Quantizer Matrix Read control */
	JENC_REG_SET(JENC_BASE + 0x048, 0x00);

	pQT = (unsigned char *)&QT;

	JENC_REG_SET(JENC_BASE + 0x040, 0x00);
	for (i = 0; i < 64; i++) { /* Quantizer Matrix for Y */
		JENC_REG_SET(JENC_BASE + 0x044, (unsigned int)(*pQT));
		pQT++;
	}
	for (i = 0; i < 64; i++) { /* Quantizer Matrix for C */
		JENC_REG_SET(JENC_BASE + 0x044, (unsigned int)(*pQT));
		pQT++;
	}
	JENC_REG_SET(JENC_BASE + 0x040, 0x00);

	return 0;
} /* End of set_jenc_core_param() */

/*!*************************************************************************
* set_output_buf
*
* Private Function
*/
/*!
* \brief
*
* \retval  0 if success
*/
static int set_output_buf(struct jenc_ctx *ctx)
{
	int image_width;

	/* Physical address of PRD table of output buffer */
	JENC_REG_SET(JENC_BASE + 0x060, ctx->prd_phys);

	JENC_REG_SET(JENC_BASE + 0x064, 0x01);
	JENC_REG_SET(JENC_BASE + 0x068, 0x01);

	/* Start JPEG Header Encode */
	JENC_REG_SET(JENC_BASE + 0x050, 0x01);

	image_width = ctx->stride;
	if (ctx->stride % 64)
		image_width = ((ctx->stride/64) + 1)*64;

	JENC_REG_SET(JENC_BASE + 0x004, image_width);

	switch (ctx->src_color) {
	case VDO_COL_FMT_ARGB:
	case VDO_COL_FMT_ABGR:
	case VDO_COL_FMT_YUV444:
	case VDO_COL_FMT_YUV422V:
		JENC_REG_SET(JENC_BASE + 0x00C, image_width * 2);
		break;
	default:
		JENC_REG_SET(JENC_BASE + 0x00C, image_width);
		break;
	}

	return 0;
} /*End of set_output_buf() */

/*!*************************************************************************
* set_mmu_table
*
* Private Function
*/
/*!
* \brief
*   Open JPEG hardware
*
* \retval  0 if success
*/
static int set_mmu_table(struct jenc_ctx *ctx)
{
	struct jenc_prop *jprop = &ctx->prop;
	int work_size;
	int is_rgb_domain = 0;
	int ret;

	/* MMU Mode */
	ve_fb_t *pic = &jprop->src_usr;

	DBG_MSG("mem_mode: MEM_MODE_USR (MMU Mode)\n");

	ctx->src_addr   = pic->src_addr;
	ctx->src_size   = pic->src_size; /* full FB size including C plane */
	ctx->pic_width  = pic->img_w;
	ctx->pic_height = pic->img_h;
	ctx->stride     = pic->stride;
	ctx->src_color  = pic->col_fmt;

	switch (ctx->src_color) {
	case VDO_COL_FMT_ARGB:
		ctx->mmu.color = 0;
		ctx->mmu.mcu_height = 8;
		break;
	case VDO_COL_FMT_ABGR:
		ctx->mmu.color = 1;
		ctx->mmu.mcu_height = 8;
		break;
	case VDO_COL_FMT_RGB_565:
		ctx->mmu.color = 2;
		ctx->mmu.mcu_height = 8;
		break;
	case VDO_COL_FMT_YUV444:
		ctx->mmu.color = 4;
		ctx->mmu.mcu_height = 8;
		break;
	case VDO_COL_FMT_YUV422H:
		ctx->mmu.color = 6;
		ctx->mmu.mcu_height = 8;
		break;
	case VDO_COL_FMT_YUV422V:
		ctx->mmu.color = 5;
		ctx->mmu.mcu_height = 16;
		break;
	case VDO_COL_FMT_YUV420:
		ctx->mmu.color = 7;
		ctx->mmu.mcu_height = 16;
		break;
	case VDO_COL_FMT_NV21:
		ctx->mmu.color = 7;
		ctx->mmu.mcu_height = 16;
		break;
	default:
		DBG_ERR("Unsupport Src color format: %d\n", ctx->src_color);
		return -EFAULT;
	}

	if ((ctx->src_color == VDO_COL_FMT_YUV420) ||
		(ctx->src_color == VDO_COL_FMT_YUV422H) ||
		(ctx->src_color == VDO_COL_FMT_YUV444) ||
		(ctx->src_color == VDO_COL_FMT_NV21)) {
		is_rgb_domain = 0;
		ctx->c_addr = ctx->src_addr + ctx->pic_height * ctx->stride;
		ctx->c_size = ctx->src_size - ctx->pic_height * ctx->stride;
	} else {
		is_rgb_domain = 1;
		ctx->c_addr = 0;
		ctx->c_size = 0;
	}
	/*--------------------------------------------------------------
		Allocate Working Buffer
	--------------------------------------------------------------*/
	ctx->mmu.work_stride = ALIGN128(pic->img_w);
	ctx->mmu.work_size   = ctx->mmu.work_stride * 16;
	work_size = ctx->mmu.work_size * 6;

	DBG_MSG("[MMU] work_stride: %d\n", ctx->mmu.work_stride);
	DBG_MSG("[MMU] mcu_height:  %d\n", ctx->mmu.mcu_height);
	DBG_MSG("[MMU] work_size:   %d\n", ctx->mmu.work_size);

	ctx->mmu.work_buf_addr = mb_alloc(work_size);
	if (ctx->mmu.work_buf_addr == 0) {
		DBG_ERR("Allocate work buffer fail(%d bytes)\n", work_size);
		return -EFAULT;
	}
	DBG_MSG("Allocate work buffer (0x%08x) %d bytes successfully\n",
		ctx->mmu.work_buf_addr, work_size);
#ifdef MB_MMU
	ctx->mb_mmubuf = mb_alloc(ctx->src_size);
	if (ctx->mb_mmubuf == 0) {
		DBG_ERR("Allocate mmubuf fail");
		return -EFAULT;
	}
	if (copy_from_user((void *)(mb_phys_to_virt(ctx->mb_mmubuf)),
				(void *)ctx->src_addr, ctx->src_size)) {
		DBG_ERR("copy src to mmubuf error\n");
		return -EFAULT;
	}
#endif

	if (is_rgb_domain) {
		DBG_MSG("[RGB] MMU (addr: 0x%08x, size: %d)\n",
			ctx->src_addr, ctx->src_size);
#ifdef MB_MMU
		ret = wmt_mmu_table_create(ctx->mb_mmubuf,
			ctx->src_size, 0, &ctx->mmu.tbl_y);
#else
		ret = wmt_mmu_table_create(ctx->src_addr,
			ctx->src_size, 1, &ctx->mmu.tbl_y);
#endif
		if (ret) {
			DBG_ERR("MMU for RGB fail (ret: %d)\n", ret);
			return -EFAULT;
		}
		memset(&ctx->mmu.tbl_c, 0, sizeof(ctx->mmu.tbl_c));
	} else {
		DBG_MSG("[Y] MMU (addr: 0x%08x, size: %d)\n",
			ctx->src_addr, (ctx->src_size - ctx->c_size));
#ifdef MB_MMU
		ret = wmt_mmu_table_create(ctx->mb_mmubuf,
			(ctx->src_size - ctx->c_size), 0, &ctx->mmu.tbl_y);
#else
		ret = wmt_mmu_table_create(ctx->src_addr,
			(ctx->src_size - ctx->c_size), 1, &ctx->mmu.tbl_y);
#endif
		if (ret) {
			DBG_ERR("MMU for Y/RGB fail (ret: %d)\n", ret);
			return -EFAULT;
		}
		/* wmt_mmu_table_dump(&ctx->mmu.tbl_y); */
		if (ctx->c_addr) {
			DBG_MSG("[C] MMU (addr: 0x%08x, size: %d)\n",
				ctx->c_addr, ctx->c_size);
#ifdef MB_MMU
			ret = wmt_mmu_table_create((ctx->mb_mmubuf +
				(ctx->src_size - ctx->c_size)),
				ctx->c_size, 0, &ctx->mmu.tbl_c);
#else
			ret = wmt_mmu_table_create(ctx->c_addr,
				ctx->c_size, 1, &ctx->mmu.tbl_c);
#endif
			if (ret) {
				DBG_ERR("[C] MMU fail (ret:%d)\n", ret);
				return -EFAULT;
			}
		}
	}
	return 0;
} /* End of set_mmu_table() */

/*!*************************************************************************
* wmt_jenc_encode_proc
*
* API Function
*/
/*!
* \brief
*   Open JPEG hardware
*
* \retval  0 if success
*/
int wmt_jenc_encode_proc(struct jenc_ctx *ctx)
{
	struct jenc_prop *jprop = &ctx->prop;
	int image_width_64;
	int cheight;
	int factor;
	int ret = 0;

	TRACE("Enter\n");

	ctx->_status = VE_STA_PREPARE;

	DBG_MSG("buf_addr:  0x%x\n", jprop->buf_addr);
	DBG_MSG("buf_size:  %d\n",   jprop->buf_size);
	DBG_MSG("enc_color: %d\n",   jprop->enc_color);
	DBG_MSG("quality:   %d\n",   jprop->quality);

	/*----------------------------------------------------------------------
		Clear memory to avoid the memory was not really allocated
	----------------------------------------------------------------------*/
#ifndef MB_MMU
	memset((void *)jprop->buf_addr, 0, jprop->buf_size);
#endif

	if (jprop->mem_mode == MEM_MODE_USR) {
		ret = set_mmu_table(ctx);
		if (ret)
			goto EXIT_wmt_jenc_encode_proc;
	} else {
		/* MEM_MODE_PHY mode (Legacy Mode) */
		DBG_MSG("mem_mode: MEM_MODE_PHY (Legacy Mode)\n");

		ctx->src_addr   = jprop->src_phy.y_addr;
		ctx->src_size   = jprop->src_phy.y_size;
		ctx->c_addr     = jprop->src_phy.c_addr;
		ctx->c_size     = jprop->src_phy.c_size;
		ctx->stride     = jprop->src_phy.fb_w;
		ctx->pic_width  = jprop->src_phy.img_w;
		ctx->pic_height = jprop->src_phy.img_h;
		ctx->src_color  = jprop->src_phy.col_fmt;

		if (ctx->src_addr != ALIGN64(ctx->src_addr)) {
			DBG_ERR("Source addr (0x%x) must 64 alignment\n",
				ctx->src_addr);
			ret = -EFAULT;
			goto EXIT_wmt_jenc_encode_proc;
		}

		switch (ctx->src_color) {
		case VDO_COL_FMT_ARGB:
		case VDO_COL_FMT_ABGR:
		case VDO_COL_FMT_RGB_565:
			DBG_ERR("RGB domain is not supported in Legacy Mode\n");
			ret = -EFAULT;
			goto EXIT_wmt_jenc_encode_proc;
		default:
			break;
		}
	} /* if( jprop->mem_mode == MEM_MODE_USR ) */

	DBG_MSG("src_addr:   0x%x\n", ctx->src_addr);
	DBG_MSG("src_size:   %d\n", ctx->src_size);
	DBG_MSG("img_width:  %d\n", ctx->pic_width);
	DBG_MSG("img_height: %d\n", ctx->pic_height);
	DBG_MSG("stride:     %d\n", ctx->stride);
	DBG_MSG("col_fmt:    %d\n", ctx->src_color);

	DBG_MSG("Y/RGB addr: 0x%x, C addr: 0x%x\n", ctx->src_addr, ctx->c_addr);

	/*----------------------------------------------------------------------
		Set Source and Target Image Color Format
	----------------------------------------------------------------------*/
	switch (jprop->enc_color) {
	case VDO_COL_FMT_YUV420:
		switch (ctx->src_color) {
		case VDO_COL_FMT_YUV444:
		case VDO_COL_FMT_RGB_565:
		case VDO_COL_FMT_ARGB:
		case VDO_COL_FMT_ABGR:
			ctx->enc_color = YC444_TO_YC420;
			break;
		case VDO_COL_FMT_YUV420: /* NV12 */
		case VDO_COL_FMT_NV21:
			ctx->enc_color = YC420_TO_YC420;
			break;
		case VDO_COL_FMT_YUV422H:
			ctx->enc_color = YC422H_TO_YC420;
			break;
		case VDO_COL_FMT_YUV422V:
			ctx->enc_color = YC422V_TO_YC420;
			break;
		default:
			DBG_ERR("Unsupported input color (%d) for YC420\n",
				ctx->src_color);
			break;
		}
		break;
	case VDO_COL_FMT_YUV444:
		switch (ctx->src_color) {
		case VDO_COL_FMT_YUV444:
		case VDO_COL_FMT_RGB_565:
		case VDO_COL_FMT_ARGB:
		case VDO_COL_FMT_ABGR:
			ctx->enc_color = YC444_TO_YC444;
			break;
		default:
			DBG_ERR("Unsupported input color (%d) for YC444\n",
				ctx->src_color);
			break;
		}
		break;
	case VDO_COL_FMT_YUV422H:
		switch (ctx->src_color) {
		case VDO_COL_FMT_YUV444:
		case VDO_COL_FMT_RGB_565:
		case VDO_COL_FMT_ARGB:
		case VDO_COL_FMT_ABGR:
			ctx->enc_color = YC444_TO_YC422H;
			break;
		case VDO_COL_FMT_YUV422H:
			ctx->enc_color = YC422H_TO_YC422H;
			break;
		default:
			DBG_ERR("Unsupported input color (%d) for YC422H\n",
				ctx->src_color);
			break;
		}
		break;
	case VDO_COL_FMT_YUV422V:
		switch (ctx->src_color) {
		case VDO_COL_FMT_YUV444:
		case VDO_COL_FMT_RGB_565:
		case VDO_COL_FMT_ARGB:
		case VDO_COL_FMT_ABGR:
			ctx->enc_color = YC444_TO_YC422V;
			break;
		case VDO_COL_FMT_YUV422V:
			ctx->enc_color = YC422V_TO_YC422V;
			break;
		default:
			DBG_ERR("Unsupported input color (%d) for YC422V\n",
				ctx->src_color);
			break;
		}
		break;
	default:
		DBG_ERR("Unsupported output color (%d)\n", jprop->enc_color);
		ret = -EFAULT;
		goto EXIT_wmt_jenc_encode_proc;
	} /* switch(jprop->enc_color) */

	image_width_64 = (ctx->pic_width + 63)/64;
	switch (ctx->enc_color) {
	case YC444_TO_YC444:
	case YC444_TO_YC422H:
	case YC422H_TO_YC422H:
		ctx->mcu_row_y = ctx->pic_width*8;
		ctx->y64bnum = image_width_64*8;
		break;
	case YC422V_TO_YC422V:
	case YC444_TO_YC422V:
	case YC444_TO_YC420:
	case YC422V_TO_YC420:
	case YC420_TO_YC420:
	case YC422H_TO_YC420:
		ctx->mcu_row_y = ctx->pic_width * 16;
		ctx->y64bnum = image_width_64 * 16;
		break;
	default:
		DBG_ERR("Unsupported encode color mode(%d)\n", ctx->enc_color);
		break;
	}
	cheight = ctx->pic_height;
	factor = 1;
	if ((ctx->src_color == VDO_COL_FMT_YUV420) ||
		(ctx->src_color == VDO_COL_FMT_YUV422H) ||
		(ctx->src_color == VDO_COL_FMT_NV21)) {
		factor = 2;
	}
	if ((ctx->src_color == VDO_COL_FMT_YUV420) ||
		(ctx->src_color == VDO_COL_FMT_YUV422V) ||
		(ctx->src_color == VDO_COL_FMT_NV21)) {
		cheight = (cheight + 1)/2;
	}
	ctx->mcu_row_c = ctx->pic_width/factor;

	switch (ctx->enc_color) {
	case YC444_TO_YC422V:
	case YC444_TO_YC420:
	case YC422H_TO_YC420:
		ctx->cburstnum = image_width_64*16;
		ctx->pic_cburstnum = ((cheight+15)/16)*ctx->cburstnum;
		ctx->mcu_row_c *= 16;
		break;
	default:
		ctx->cburstnum = image_width_64 * 8;
		ctx->pic_cburstnum = ((cheight+7)/8)*ctx->cburstnum;
		ctx->mcu_row_c *= 8;
		break;
	}

	/* for 1080p YC420, the encode time is about 20 ms */
	ctx->_timeout = ((ctx->pic_width * ctx->pic_height)/(1920*1080))*20*3;
	ctx->_timeout = (ctx->_timeout < 30) ? 30 : ctx->_timeout;

	DBG_MSG("set timeout as %d ms\n", ctx->_timeout);

	/*----------------------------------------------------------------------
		Transfer output buffer to PRD table format
	----------------------------------------------------------------------*/
	if (ctx->bsbuf == 0) {
		ctx->bsbuf = mb_alloc(jprop->buf_size);
		if (ctx->bsbuf == 0) {
			DBG_ERR("Alloc MB (%d bytes) fail!\n", jprop->buf_size);
			goto EXIT_wmt_jenc_encode_proc;
		}
	}
	if (wmt_codec_write_prd(ctx->bsbuf, jprop->buf_size, ctx->prd_virt,
			MAX_INPUT_BUF_SIZE)){
		DBG_ERR("Transfer output buffer to prd table fail\n");
		ret = -EFAULT;
		goto EXIT_wmt_jenc_encode_proc;
	}

	/*----------------------------------------------------------------------
		Start to prepare data for HW register
	----------------------------------------------------------------------*/
	wmt_codec_timer_start(ctx->codec_tm);

	jenc_initial();
	set_input_buf(ctx);
	set_jenc_core_param(ctx);
	set_output_buf(ctx);

	JENC_REG_SET(JENC_BASE + 0x018, 0x01);

	/* encoding engine is worknig now */
	ctx->_status |= VE_STA_ENCODEING;

	/* Now HW encoder is working... */

EXIT_wmt_jenc_encode_proc:
	if (ret)
		ctx->_status = VE_STA_ERR_UNKNOWN;
	TRACE("Leave(ret:%d)\n", ret);

	return ret;
} /* End of wmt_jenc_encode_proc() */

/*!*************************************************************************
* wmt_jenc_encode_finish
*
* API Function
*/
/*!
* \brief
*   Wait JPEG hardware finished
*
* \retval  0 if success
*/
int wmt_jenc_encode_finish(struct jenc_ctx *ctx)
{
	struct jenc_prop *jprop = &ctx->prop;
	int ret = 0;

	TRACE("Enter\n");
	/*======================================================================
		In this function, we just wait for HW decoding finished.
	======================================================================*/
	ret = wait_encode_finish(ctx);

	if (ret == 0) {
		copy_to_user((void *)jprop->buf_addr,
		   (const void *)mb_phys_to_virt(ctx->bsbuf), jprop->enc_size);
	}
	DBG_MSG("buf_addr:  0x%x\n", jprop->buf_addr);
	DBG_MSG("buf_size:  %d\n",   jprop->buf_size);
	DBG_MSG("enc_size:  %d\n",   jprop->enc_size);

	/*----------------------------------------------------------------------
		DO L2 cache flush
	----------------------------------------------------------------------*/
	if (!access_ok(VERIFY_WRITE, (void __user *) jprop->buf_addr,
		jprop->buf_size)) {
		TRACE("L2 cache flush(0x%08x,0x%08x)",
			jprop->buf_addr, __pa(jprop->buf_addr));
		dmac_flush_range((void *)jprop->buf_addr,
			(void *)(jprop->buf_addr + jprop->buf_size));
		outer_flush_range(__pa(jprop->buf_addr),
			__pa(jprop->buf_addr) + jprop->buf_size);
	}

	if (jprop->mem_mode == MEM_MODE_USR) {
		/*--------------------------------------------------------------
			Free MMU table since it is useless now
		--------------------------------------------------------------*/
		DBG_MSG("Free Y/RGB MMU table (%p)\n", &ctx->mmu.tbl_y);
		wmt_mmu_table_destroy(&ctx->mmu.tbl_y);
		if (ctx->c_addr) {
			DBG_MSG("Free C MMU table (%p)\n", &ctx->mmu.tbl_c);
			wmt_mmu_table_destroy(&ctx->mmu.tbl_c);
		}
		/*--------------------------------------------------------------
			Free work buffer since it is useless now
		--------------------------------------------------------------*/
		if (ctx->mmu.work_buf_addr) {
			DBG_MSG("Free MMU work buffer (0x%08x)\n",
				ctx->mmu.work_buf_addr);
			mb_free(ctx->mmu.work_buf_addr);
			ctx->mmu.work_buf_addr = 0;
		}
	}
	TRACE("Leave(ret:%d)\n", ret);

	return ret;
} /* End of wmt_jenc_encode_finish() */

/*!*************************************************************************
* wmt_jenc_isr
*
* API Function
*/
/*!
* \brief
*   Open JPEG hardware
*
* \retval  0 if not handle INT, others INT has been handled.
*/
irqreturn_t wmt_jenc_isr(int irq, void *isr_data)
{
	#define INT_BIT_CLEAR(bit) \
			JENC_REG_SET(JENC_BASE + 0x024, (1<<bit))

	#define INT_BIT_GET(val, bit)      ((val) & (1<<bit))

	unsigned int reg_val, irq_en;
	unsigned int int_read_finish, int_dma_finish;
	unsigned int int_encode_done, int_memory_full;

	struct jenc_ctx *ctx = isr_data;

	TRACE("Enter\n");

	if ((ctx->_status & VE_STA_ENCODEING) == 0) {
		/* Not in encoding state */
		DBG_WARN("Unexpected state (%d)\n", ctx->_status);
		return IRQ_HANDLED;
	}
	irq_en  = REG32_VAL(JENC_BASE + 0x020);
	reg_val = REG32_VAL(JENC_BASE + 0x024);

	DBG_MSG("irq_en: 0x%x, reg_val: 0x%0x\n", irq_en, reg_val);

	int_read_finish  = INT_BIT_GET(reg_val, 0);
	int_dma_finish   = INT_BIT_GET(reg_val, 1);
	int_encode_done  = INT_BIT_GET(reg_val, 2);
	int_memory_full  = INT_BIT_GET(reg_val, 3);

	if (!int_encode_done && !int_memory_full) {  /* other interrupt */
		return IRQ_NONE;
	}

	if (int_read_finish) {
		DBG_MSG("[INT] Read Finish\n");
		ctx->_status |= VE_STA_READ_FINISH;
		INT_BIT_CLEAR(0);
	}
	if (int_dma_finish) {
		DBG_MSG("[INT] DMA Move Done\n");
		ctx->_status |= VE_STA_DMA_MOVE_DONE;
		INT_BIT_CLEAR(1);
	}

	if (int_encode_done) {
		ctx->_status &= (~VE_STA_ENCODEING);
		ctx->_status |= VE_STA_ENCODE_DONE;

		/* Disable all interrupt status */
		INT_BIT_CLEAR(2);
		JENC_REG_SET(JENC_BASE + 0x020, 0x0F);

		wmt_codec_timer_stop(ctx->codec_tm);
	} else {
		DBG_ERR("Memory full! Input buffer size is too short\n");
		ctx->_status |= VE_STA_ERR_MEM_FULL;

		/* Disable all interrupt */
		INT_BIT_CLEAR(3);
	}
	wake_up_interruptible(&ctx->wait_event);

	TRACE("Leave\n");

	return IRQ_HANDLED;
} /* End of wmt_jenc_isr() */

/*!*************************************************************************
* wmt_jenc_open
*
* API Function
*/
/*!
* \brief
*   Open JPEG hardware
*
* \retval  0 if success
*/
int wmt_jenc_open(struct jenc_ctx *ctx)
{
	static atomic_t seq_id = ATOMIC_INIT(0);
	int ret = 0;

#ifdef JENC_IRQ_MODE
	ret = request_irq(IRQ_JENC, wmt_jenc_isr, IRQF_SHARED, "jenc", ctx);
	if (ret < 0) {
		DBG_ERR("Request %s IRQ %d Fail\n", "jenc", IRQ_JENC);
		return -1;
	}
	DBG_MSG("Request %s IRQ %d OK\n", "jenc", IRQ_JENC);
#endif

	ctx->ref_num = (unsigned long)(atomic_inc_return(&seq_id));

	DBG_MSG("Seq id: %ld\n", ctx->ref_num);

	init_waitqueue_head(&ctx->wait_event);

	ctx->_status = VE_STA_READY;

	wmt_codec_timer_init(&ctx->codec_tm, "<jenc>", 0, 0 /*ms*/);

	return ret;
} /* End of wmt_jenc_open() */

/*!*************************************************************************
* wmt_jenc_close
*
* API Function
*/
/*!
* \brief
*   Close JPEG hardware
*
* \retval  0 if success
*/
int wmt_jenc_close(struct jenc_ctx *ctx)
{
	ctx->_status = VE_STA_CLOSE;

	DBG_MSG("Seq id: %ld\n", ctx->ref_num);

	if (ctx->bsbuf) {
		mb_free(ctx->bsbuf);
		ctx->bsbuf = 0;
	}

#ifdef MB_MMU
	if (ctx->mb_mmubuf) {
		mb_free(ctx->mb_mmubuf);
		ctx->mb_mmubuf = 0;
	}
#endif

#ifdef JENC_IRQ_MODE
	free_irq(IRQ_JENC, (void *)ctx);
#endif

	wmt_codec_timer_exit(ctx->codec_tm);

	return 0;
} /* End of wmt_jenc_close() */

/*--------------------End of Function Body -------------------------------*/

#undef WMT_JENC_C
