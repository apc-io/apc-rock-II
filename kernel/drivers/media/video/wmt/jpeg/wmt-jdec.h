/*++
 * WonderMedia SoC hardware JPEG decoder driver
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
#ifndef WMT_JDEC_H
/* To assert that only one occurrence is included */
#define WMT_JDEC_H

/*-------------------- MODULE DEPENDENCY -------------------------------------*/

/*-------------------- EXPORTED PRIVATE CONSTANTS ----------------------------*/

/*------------------------------------------------------------------------------
    JPEG_DECODER_BASE_ADDR(0xD80FE000) was defined in
    \arch\arm\mach-wmt\include\mach\wmt_mmap.h
------------------------------------------------------------------------------*/

#define JDEC_BASE                   JPEG_DECODER_BASE_ADDR


/*------------------------------------------------------------------------------
    Definitions of JDEC Registers
    About following definitions, please refer "WM3451 JPEG Decoder Register List"
------------------------------------------------------------------------------*/
#define REG_JDEC_SW_RESET               (JDEC_BASE + 0x010)
#define REG_JDEC_INT_ENALBE             (JDEC_BASE + 0x020)
#define REG_JDEC_INT_STATUS             (JDEC_BASE + 0x024)
/* JPEG Bitstream Input DMA Register */
#define REG_JDEC_BSDMA_PRD_ADDR         (JDEC_BASE + 0x100)
#define REG_JDEC_BSDMA_INIT             (JDEC_BASE + 0x104)
#define REG_JDEC_BSDMA_START_TX         (JDEC_BASE + 0x108)
#define REG_JDEC_BSDMA_STATUS           (JDEC_BASE + 0x10C)
#define REG_JDEC_BSDMA_READ_BYTE        (JDEC_BASE + 0x110)
/* JPEG Decode Control Register */
#define REG_JDEC_INIT                   (JDEC_BASE + 0x200)
#define REG_JDEC_SRC_WIDTH_MCU          (JDEC_BASE + 0x210)
#define REG_JDEC_SRC_HEIGHT_MCU         (JDEC_BASE + 0x214)
#define REG_JDEC_SRC_COLOR_FORMAT       (JDEC_BASE + 0x218)
#define REG_JDEC_PARTIAL_ENABLE         (JDEC_BASE + 0x220)
#define REG_JDEC_PARTIAL_H_START        (JDEC_BASE + 0x230)
#define REG_JDEC_PARTIAL_V_START        (JDEC_BASE + 0x234)
#define REG_JDEC_PARTIAL_WIDTH          (JDEC_BASE + 0x238)
#define REG_JDEC_PARTIAL_HEIGHT         (JDEC_BASE + 0x23C)
#define REG_JDEC_DST_MCU_WIDTH          (JDEC_BASE + 0x240)
#define REG_JDEC_DST_MCU_HEIGHT         (JDEC_BASE + 0x244)
#define REG_JDEC_YBASE                  (JDEC_BASE + 0x250)
#define REG_JDEC_Y_LINE_WIDTH           (JDEC_BASE + 0x254) /* 64 bytes alignment */
#define REG_JDEC_CBASE                  (JDEC_BASE + 0x258)
#define REG_JDEC_C_LINE_WIDTH           (JDEC_BASE + 0x25C)
/* Split Write Register */
#define REG_JDEC_SPLIT_WRITE_ENABLE     (JDEC_BASE + 0x260)
#define REG_JDEC_SPLIT_WRITE_MCU_HEIGHT (JDEC_BASE + 0x264)
#define REG_JDEC_SPLIT_WRITE_START      (JDEC_BASE + 0x268)
/* Picture Scaling Control Register */
#define REG_JDEC_Y_SCALE_RATIO          (JDEC_BASE + 0x300)
#define REG_JDEC_C_SCALE_RATIO_H        (JDEC_BASE + 0x304)
#define REG_JDEC_C_SCALE_RATIO_V        (JDEC_BASE + 0x308)
#define REG_JDEC_CHROMA_ENABLE          (JDEC_BASE + 0x30C)
#define REG_JDEC_Y_SCALED_WIDTH         (JDEC_BASE + 0x320)
#define REG_JDEC_Y_SCALED_HEIGHT        (JDEC_BASE + 0x324)
#define REG_JDEC_C_SCALED_WIDTH         (JDEC_BASE + 0x328)
#define REG_JDEC_C_SCALED_HEIGHT        (JDEC_BASE + 0x32C)
/* YCbCr to RGB Conversion Control Register */
#define REG_JDEC_YUV2RGB_ENABLE         (JDEC_BASE + 0x400)
#define REG_JDEC_RGB_ALPHA              (JDEC_BASE + 0x404)
/* JPEG Decoder Status Register */
#define REG_JDEC_SOF_Y                  (JDEC_BASE + 0x500)
#define REG_JDEC_SOF_X                  (JDEC_BASE + 0x504)
#define REG_JDEC_SOF                    (JDEC_BASE + 0x508)
#define REG_JDEC_FIFO_STATUS            (JDEC_BASE + 0x510)


/*-------------------- EXPORTED PRIVATE VARIABLES ----------------------------*/
#ifdef WMT_JDEC_C
	#define EXTERN
#else
	#define EXTERN   extern
#endif

#undef EXTERN

/*--------------------- EXPORTED PRIVATE MACROS ------------------------------*/

/*--------------------- EXPORTED PRIVATE FUNCTIONS  --------------------------*/

#endif /* ifndef WMT_JDEC_H */

/*=== END wmt-jdec.h =========================================================*/
