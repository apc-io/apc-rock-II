/*++
 * Common interface for WonderMedia SoC hardware JPEG decoder driver
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
 * 10F, 529, Chung-Cheng Road, Hsin-Tien, Taipei 231, R.O.C
--*/
#ifndef COM_JDEC_H
/* To assert that only one occurrence is included */
#define COM_JDEC_H

/*-------------------- MODULE DEPENDENCY ---------------------------------*/
#ifdef __KERNEL__
#include "../com-vd.h"
#else
#include "com-vd.h"
#endif

/*-------------------- EXPORTED PRIVATE CONSTANTS ------------------------*/

#define IOCTL_JPEG_INIT(cmd, _struct_type_) \
		memset((cmd), 0, sizeof(_struct_type_));


/*--------------------------------------------------------------------------
    Definitions of enum
--------------------------------------------------------------------------*/
enum {
	SCALE_ORIGINAL,       /* Original size */
	SCALE_HALF,           /* 1/2 of Original size */
	SCALE_QUARTER,        /* 1/4 of Original size */
	SCALE_EIGHTH,         /* 1/8 of Original size */
	SCALE_MAX
}; /* vd_scale_ratio */

enum {
	JT_FRAME,
	JT_FIELD_TOP,
	JT_FIELD_BOTTOM
}; /* jpeg_type */

/*--------------------------------------------------------------------------
	Definitions of structures
--------------------------------------------------------------------------*/
/* Used for AVI1 FOURCC in APP0 only */
struct avi1_s {
	int type; /* jpeg_type */
	int field_size; /* 0 if JT_FRAME; others: data size of a field */
};
#define avi1_t    struct avi1_s

/*--------------------------------------------------------------------------
	Following macro definition is used in struct jdec_prop_ex_t
--------------------------------------------------------------------------*/
#define DECFLAG_MJPEG_BIT      BIT(28)
	/* 0: JPEG still picture 1: MJPG movie */
#define FLAG_MULTI_SEGMENT_EN  BIT(29)
	/* 0: Multi-segment decoding is disable, 1: MSD enabled */

/*--------------------------------------------------------------------------
	Header Information
--------------------------------------------------------------------------*/
struct jdec_hdr_info_s {
	unsigned int    profile;       /* Profile */
	unsigned int    sof_w;         /* Picture width in SOF */
	unsigned int    sof_h;         /* Picture height in SOF */
	unsigned int    filesize;      /* File size of this JPEG */
	vdo_color_fmt   src_color;     /* Picture color format */
	avi1_t          avi1;
};
#define jdec_hdr_info_t       struct jdec_hdr_info_s

/*--------------------------------------------------------------------------
	Decoder Information
--------------------------------------------------------------------------*/
struct jdec_pd_s {
	unsigned int enable;   /* Enable HW Partial decode or not */
	unsigned int x;        /* must (16xN alignment) (Unit: Pixel) */
	unsigned int y;        /* must (16xN alignment) (Unit: Pixel) */
	unsigned int w;        /* must (16xN alignment) (Unit: Pixel) */
	unsigned int h;        /* must (16xN alignment) (Unit: Pixel) */
}; /* PD: Partial Decode */
#define jdec_pd       struct jdec_pd_s

/*--------------------------------------------------------------------------
	Following structure is used for all HW decoder as output arguments
--------------------------------------------------------------------------*/
struct jdec_fb_s {
	VD_IOCTL_CMD_M;
	vdo_framebuf_t   fb;
	unsigned int     y_addr_user;  /* Y address in user space */
	unsigned int     c_addr_user;  /* C address in user space */
	int              scaled;       /* vd_scale_ratio */
};
#define jdec_frameinfo_t     struct jdec_fb_s

/*--------------------------------------------------------------------------
	Following structure is used for VDIOSET_DECODE_LOOP
--------------------------------------------------------------------------*/
#define VD_DECODE_LOOP_M \
	unsigned int    scale_factor;   /* vd_scale_ratio */\
	vdo_color_fmt   dst_color;  /* Destination decoded color space */\
	unsigned char  *buf_in;\
	unsigned int    bufsize;\
	unsigned int    timeout  /* lock timeout */

struct jdec_prop_s {
	VD_DECODE_LOOP_M;
};
#define jdec_prop_t    struct jdec_prop_s

/* Following structure was used for WMT API and driver internally */
struct jdec_prop_ex_s {
	VD_DECODE_LOOP_M;
	jdec_hdr_info_t  hdr;
	unsigned int     mmu_mode;     /* 0: Phys mode, 1: MMU mode */
	unsigned int     y_addr_user;  /* Y address in user space */
	unsigned int     c_addr_user;  /* C address in user space */
	unsigned int     dst_y_addr;  /* output address in Y (Physical addr )*/
	unsigned int     dst_y_size;  /* output buffer size in Y */
	unsigned int     dst_c_addr;  /* output address in C (Physical addr )*/
	unsigned int     dst_c_size;  /* output buffer size in C */
	unsigned int     scanline;    /* Scanline offset of FB (Unit: pixel) */
	jdec_pd          pd;          /* Settings for HW Partial Decode */
	unsigned int     flags;       /* control flag for HW decoder */
	jdec_frameinfo_t jfb;
};
#define jdec_prop_ex_t    struct jdec_prop_ex_s

/*--------------------- EXPORTED PRIVATE FUNCTIONS  ----------------------*/

#endif /* ifndef COM_JDEC_H */

/*=== END com-jdec.h =====================================================*/
