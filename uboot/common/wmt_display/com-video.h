/*++
Copyright (c) 2008 WonderMedia Technologies, Inc. All Rights Reserved.

This PROPRIETARY SOFTWARE is the property of WonderMedia Technologies, Inc.
and may contain trade secrets and/or other confidential information of
WonderMedia Technologies, Inc. This file shall not be disclosed to any third
party, in whole or in part, without prior written consent of WonderMedia.

THIS PROPRIETARY SOFTWARE AND ANY RELATED DOCUMENTATION ARE PROVIDED AS IS,
WITH ALL FAULTS, AND WITHOUT WARRANTY OF ANY KIND EITHER EXPRESS OR IMPLIED,
AND WonderMedia TECHNOLOGIES, INC. DISCLAIMS ALL EXPRESS OR IMPLIED WARRANTIES
OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR
NON-INFRINGEMENT.
--*/

#ifndef COM_VIDEO_H
/* To assert that only one occurrence is included */
#define COM_VIDEO_H

/*-------------------- MODULE DEPENDENCY -------------------------------------*/

//#include "sim-kernel.h"  // for POST only

/*-------------------- EXPORTED PRIVATE CONSTANTS ----------------------------*/

#define BIT(x)              (1<<x)


/*------------------------------------------------------------------------------

------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
    Definitions of enum
------------------------------------------------------------------------------*/

typedef enum {
    VDO_COL_FMT_YUV420,
    VDO_COL_FMT_YUV422H,  
    VDO_COL_FMT_YUV422V,
    VDO_COL_FMT_YUV444,
    VDO_COL_FMT_YUV411,
    VDO_COL_FMT_GRAY,
    VDO_COL_FMT_BGRA,     /* B G R A from offset 0 ~ 3 */
    VDO_COL_FMT_ARGB = VDO_COL_FMT_BGRA,
    VDO_COL_FMT_AUTO,
    VDO_COL_FMT_RGB_888,
    VDO_COL_FMT_RGB_666,
    VDO_COL_FMT_RGB_565,
    VDO_COL_FMT_RGB_1555,
    VDO_COL_FMT_RGB_5551,
    VDO_COL_FMT_RGBA,     /* R G B A from offset 0 ~ 3 */
    VDO_COL_FMT_MAX,
    VDO_COL_FMT_UNKNOWN
} vdo_color_fmt;


/*------------------------------------------------------------------------------
    Definitions of Struct
------------------------------------------------------------------------------*/

typedef struct {
    /* Physical address for kernel space */
    unsigned int    y_addr;   /* Addr of Y plane in YUV domain or RGB plane in ARGB domain */
    unsigned int    c_addr;   /* C plane address */
    unsigned int    y_size;   /* Buffer size in bytes */
    unsigned int    c_size;   /* Buffer size in bytes */
    unsigned int    img_w;    /* width of valid image (unit: pixel) */
    unsigned int    img_h;    /* height of valid image (unit: line) */
    unsigned int    fb_w;     /* width of frame buffer (scanline offset) (unit: pixel)*/
    unsigned int    fb_h;     /* height of frame buffer (unit: line) */
    unsigned int    bpp;      /* bits per pixel (8/16/24/32) */
    
    vdo_color_fmt   col_fmt;  /* Color format on frame buffer */

    unsigned int    h_crop;   /* Horental Crop (unit: pixel) */
    unsigned int    v_crop;   /* Vertical Crop (unit: pixel) */
	
	unsigned int 	flag;	  /* frame flags */
} vdo_framebuf_t;

#define VDO_FLAG_INTERLACE		BIT(0)
#define VDO_FLAG_MOTION_VECTOR	BIT(1)		/* frame buffer with motion vector table after C frame */
#define VDO_FLAG_MB_ONE			BIT(2)		/* Y/C frame alloc in one mb */
#define VDO_FLAG_MB_NO			BIT(3)		/* frame buffer is not alloc from mb */

typedef struct {
	unsigned int resx_src;       /* source x resolution */
	unsigned int resy_src;       /* source y resolution */
	unsigned int resx_virtual;   /* virtual x resolution */
	unsigned int resy_virtual;   /* virtual y resolution */
	unsigned int resx_visual;    /* visual x resolution */
	unsigned int resy_visual;    /* visual y resolution */
	unsigned int posx;           /* x position to display screen */
	unsigned int posy;           /* y postion to display screen */
	unsigned int offsetx;        /* x pixel offset from source left edge */
	unsigned int offsety;        /* y pixel offset from source top edge */
} vdo_view_t;

#endif /* ifndef COM_VIDEO_H */

/*=== END com-video.h ==========================================================*/
