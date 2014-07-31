/*++
 * Common interface for WonderMedia SoC hardware H264 encoder driver
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
 * 4F, 535, Chung-Cheng Road, Hsin-Tien, Taipei 231, R.O.C
--*/
#ifndef COM_H264ENC_H
	/* To assert that only one occurrence is included */
#define COM_H264ENC_H

/*-------------------- MODULE DEPENDENCY -------------------------------------*/
#ifdef __KERNEL__
#include "../com-ve.h"
#else
#include "com-ve.h"
#endif

/*-------------------- EXPORTED PRIVATE CONSTANTS ----------------------------*/


/*------------------------------------------------------------------------------
	Definitions of enum
------------------------------------------------------------------------------*/
enum {
	P_SLICE = 0,
	B_SLICE = 1,
	I_SLICE = 2,
	SP_SLICE = 3,
	SI_SLICE = 4,
	NUM_SLICE_TYPES = 5
}; /* SliceType */

/*------------------------------------------------------------------------------
	Following structure is used for VDIOSET_ENCODE_LOOP
------------------------------------------------------------------------------*/
struct h264enc_prop {
	/* Common property for all encoders */
	VE_PROPERTY_M;

	/* Private data for H264 encoder only */
	int frame_num;
	int idr_flag;
	int QP;
	int profile_idc;
	int slice_type;
	int levelIDC;

	/* Following members are used in user space only */
	int bit_rate;
	int IDRPeriod;
	int framerate;
	int maxQP;
	int minQP;
	long long int limit_bits;
	int QPchange;
	int frame_not_changed;

	/* Returned information from driver */
	int NumberofTextureBits;
	int TotalMAD;
	int NumberofHeaderBits;
};

#endif /* ifndef COM_H264ENC_H */

/*=== END com-h264enc.h ==========================================================*/

