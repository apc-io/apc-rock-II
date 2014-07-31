/*++
 * Common interface for WonderMedia SoC hardware JPEG encoder
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
#ifndef COM_JENC_H
/* To assert that only one occurrence is included */
#define COM_JENC_H

/*-------------------- MODULE DEPENDENCY -------------------------------------*/
#ifdef __KERNEL__
#include "../com-ve.h"
#else
#include "com-ve.h"
#endif

/*------------------------------------------------------------------------------
	Following structure is used for VDIOSET_ENCODE_LOOP
------------------------------------------------------------------------------*/
struct jenc_prop {
	/* Common property for all encoders */
	VE_PROPERTY_M;

	/* Private data for JPEG encoder only */
	int          quality;   /* 0(worst) - 100(best) */
	unsigned int timeout;  /* lock timeout */
};

#endif /* ifndef COM_JENC_H */

/*=== END com-jenc.h =========================================================*/
