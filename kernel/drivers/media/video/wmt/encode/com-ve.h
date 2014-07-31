/*++
 * Common interface for WonderMedia SoC hardware encoder drivers
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
#ifndef COM_VE_H
/* To assert that only one occurrence is included */
#define COM_VE_H

/*-------------------- MODULE DEPENDENCY ---------------------------------*/
#ifdef __KERNEL__
#include <mach/com-video.h>
#else
#include "com-video.h"
#endif


/*-------------------- EXPORTED PRIVATE CONSTANTS ------------------------*/

#define VE_BASE    16  /* Resever 16 for video decoders */
#define VE_JPEG    (VE_BASE + 1)
#define VE_H264    (VE_BASE + 2)
#define VE_MAX     (VE_BASE + 3)


/*--------------------------------------------------------------------------
	Following macros define hardware encoder type for ve_ioctl_cmd
--------------------------------------------------------------------------*/
/* The member of identity is defined as
	Bit 31~16: used for hardware encoder type
	Bit 15~0:  used for version
*/
#define VE_IOCTL_CMD_M \
	unsigned int    identity; /* encoder type */\
	unsigned int    size      /* size of full structure */
/* End of VE_IOCTL_CMD_M */

/*--------------------------------------------------------------------------
	Definitions of enum
--------------------------------------------------------------------------*/
enum {
	MEM_MODE_USR,
		/* memory of input buffer come from user space memory
			(Not physical continous). It usually comes from
			application allocated memory. */

	MEM_MODE_PHY
		/* memory of input buffer come from physical memory
			(physical continuous). It usually comes fomr HW video
			decoded frame buffer.
			It has some alignment limitation, e.g., 16 at least. */
}; /* ve_mem_mode */

/*--------------------------------------------------------------------------
	Definitions of structures
--------------------------------------------------------------------------*/
struct ve_fb_s {  /* Frame buffer  attribution */
	unsigned int   src_addr;  /* Encoding buffer (source) address */
			/*
				For YC color domain, src_addr points to Y start
				address src_addr
					+-------------------+
					|                   |
					|     Y - Plane     |
					|                   |
					+-------------------+
					|                   |
					|     C - Plane     |
					|                   |
					+-------------------+
				And start address of C-plane should be equal to
				c_addr = src_addr + stride * img_height
			*/
	unsigned int   src_size;  /* Encoding buffer size in bytes */
			/*
				For YC color domain, src_size should
				be equal to y_size + c_size */
	unsigned int   img_w;
	unsigned int   img_h;
	unsigned int   stride;    /* Line size in source buffer in bytes */
	vdo_color_fmt  col_fmt;
};
#define ve_fb_t     struct ve_fb_s

#define VE_PROPERTY_M \
	VE_IOCTL_CMD_M;\
	/* destination encoded picture settings */\
	unsigned int   buf_addr;  /* Encoding buffer (dest) address */\
	unsigned int   buf_size;  /* Encoding buffer size in bytes */\
	unsigned int   enc_size;  /* Really encoded data size in bytes */\
	vdo_color_fmt  enc_color;\
	int  mem_mode;  /* ve_mem_mode */\
	union { /* Source picture data settings */\
		ve_fb_t        src_usr;   /* for MEM_MODE_USR mode */\
		vdo_framebuf_t src_phy;   /* for MEM_MODE_PHY mode */\
	}
/* End of VE_PROPERTY_M */

/*--------------------------------------------------------------------------
	Definitions of Struct
--------------------------------------------------------------------------*/
/* Following structure is used for all HW encoder as input arguments */
struct ve_ioctl_cmd_s {
	VE_IOCTL_CMD_M;
};
#define ve_ioctl_cmd    struct ve_ioctl_cmd_s

/* Following structure is used for all HW encoder as input arguments */
struct ve_prop {
	VE_PROPERTY_M;
};

/* Following structure is used for all HW encoder internally */
struct ve_handle_s {
	int   ve_fd;
	int   ve_id;
	void *priv;   /* private data for video encoder used */
};
#define ve_handle_t    struct ve_handle_s

/*--------------------------------------------------------------------------
	Macros below are used for driver in IOCTL
--------------------------------------------------------------------------*/
#define VE_IOC_MAGIC              'e'
#define VE_IOC_MAXNR              1

/* VEIOSET_ENCODE_LOOP: application send encode data to driver */
#define VEIOSET_ENCODE_LOOP       _IOWR(VE_IOC_MAGIC, 0, ve_ioctl_cmd)

#endif /* ifndef COM_VE_H */

/*=== END com-ve.h =======================================================*/
