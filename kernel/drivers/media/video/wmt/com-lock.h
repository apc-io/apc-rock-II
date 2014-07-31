/*++
 * WonderMedia Codec Lock driver
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
#ifndef COM_LOCK_H
/* To assert that only one occurrence is included */
#define COM_LOCK_H

/*-------------------- MODULE DEPENDENCY ---------------------------------*/

/*-------------------- EXPORTED PRIVATE TYPES-----------------------------*/


/*Define wmt lock types*/
enum {
	lock_jpeg = 0,  /* Lock for JPEG decoder */
	lock_video,     /* Lock for MSVD decoders */
	lock_max
}; /* wmt_lock_type */

struct wmt_lock {
	long lock_type;    /* TYPE_JPEG or TYPE_VIDEO */
	long arg2;         /* for IO_WMT_UNLOCK, the timeout value */
	int  is_wait;      /* is someone wait for this lock? */
};
#define wmt_lock_ioctl_arg  struct wmt_lock

/*--------------------- EXPORTED PRIVATE FUNCTIONS  ----------------------*/

/*--------------------------------------------------------------------------
	Macros below are used for driver in IOCTL
--------------------------------------------------------------------------*/
#define LOCK_IOC_MAGIC      'L'
#define IO_WMT_LOCK         _IOWR(LOCK_IOC_MAGIC, 0, unsigned long)
#define IO_WMT_UNLOCK       _IOWR(LOCK_IOC_MAGIC, 1, unsigned long)
#define IO_WMT_CHECK_WAIT   _IOR(LOCK_IOC_MAGIC, 2, unsigned long)


#endif /* ifndef COM_LOCK_H */

/*=== END com-lock.h =====================================================*/
