/*++
 * Copyright (c) 2013 WonderMedia Technologies, Inc.
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

#ifndef COM_VD_H
/* To assert that only one occurrence is included */
#define COM_VD_H

/*-------------------- MODULE DEPENDENCY ---------------------------------*/
#ifdef __KERNEL__
#include <mach/com-video.h>
#else
#include "com-video.h"
#endif

/*-------------------- EXPORTED PRIVATE CONSTANTS ------------------------*/

#define VD_JPEG    0x0001
#define VD_MAX     0x0002

/*--------------------------------------------------------------------------
Following macros define hardware decoder type for vd_ioctl_cmd
--------------------------------------------------------------------------*/
#define IOCTL_CMD_INIT(cmd, _struct_type_, vd_type, version)

#define VD_IOCTL_CMD_M \
	unsigned int    identity; /* decoder type */\
	unsigned int    size      /* size of full structure */
/* End of VD_IOCTL_CMD_M */

/*-------------------------------------------------------------------------
	Definitions of Struct
--------------------------------------------------------------------------*/
struct vd_ioctl_cmd_s {
	VD_IOCTL_CMD_M;
};
#define vd_ioctl_cmd    struct vd_ioctl_cmd_s

struct vd_handle_s {
	int   vd_fd;
	int   mb_fd;
};
#define vd_handle_t     struct vd_handle_s

/*--------------------------------------------------------------------------
   Macros below are used for driver in IOCTL
--------------------------------------------------------------------------*/
#define VD_IOC_MAGIC              'k'
#define VD_IOC_MAXNR              1

/* VDIOSET_DECODE_LOOP: application send decode data to driver
						(blocking mode in driver) */
#define VDIOSET_DECODE_LOOP     _IOWR(VD_IOC_MAGIC, 0, vd_ioctl_cmd)

#endif /* ifndef COM_VD_H */

/*=== END com-vd.h ======================================================*/
