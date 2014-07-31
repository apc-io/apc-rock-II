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
#ifndef WMT_JENC_H
/* To assert that only one occurrence is included */
#define WMT_JENC_H

/*-------------------- MODULE DEPENDENCY -------------------------------------*/

/*-------------------- EXPORTED PRIVATE CONSTANTS ----------------------------*/

/*------------------------------------------------------------------------------
    JPEG_ENCODER_BASE_ADDR(0xD80F2000) was defined in 
    \arch\arm\mach-wmt\include\mach\wmt_mmap.h
------------------------------------------------------------------------------*/

#define JENC_BASE      JPEG_ENCODER_BASE_ADDR


/*-------------------- EXPORTED PRIVATE TYPES---------------------------------*/

/*------------------------------------------------------------------------------
    Definitions of enum
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
    Definitions of structures
------------------------------------------------------------------------------*/

/*-------------------- EXPORTED PRIVATE VARIABLES -----------------------------*/
#ifdef WMT_JENC_C
    #define EXTERN
#else
    #define EXTERN   extern
#endif

#undef EXTERN

/*--------------------- EXPORTED PRIVATE MACROS -------------------------------*/

/*--------------------- EXPORTED PRIVATE FUNCTIONS  ---------------------------*/

#endif /* ifndef WMT_JENC_H */

/*=== END wmt-jenc.h ==========================================================*/
