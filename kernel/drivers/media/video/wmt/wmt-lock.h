/*++ 
 * Common interface for WonderMedia SoC hardware decoder drivers
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
#ifndef WMTLOCK_H
#define WMTLOCK_H

/*-------------------- MODULE DEPENDENCY -------------------------------------*/

/* Even for multi-decoding, this value should <= 4 */
#define MAX_LOCK_NUM_VDEC   4            /* Max lock number for MSVD decoders */



/*-------------------- EXPORTED PRIVATE VARIABLES -----------------------------*/
#ifdef WMTLOCK_C
    #define EXTERN
#else
    #define EXTERN   extern
#endif /* ifdef WMTLOCK_C */

#undef EXTERN

/*--------------------- EXPORTED PRIVATE FUNCTIONS  ---------------------------*/

/*=== END wmt-lock.h ==========================================================*/
#endif /* #ifndef WMTLOCK_H */

