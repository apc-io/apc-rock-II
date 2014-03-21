/*
 * Copyright (c) 2008-2013 WonderMedia Technologies, Inc.
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
 */

#ifndef GE_ACCEL_H
#define GE_ACCEL_H

#include <linux/init.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/fb.h>

#ifndef __KERNEL__
#define __KERNEL__
#endif

#define GE_DEBUG 0
#define FB_ACCEL_WMT 0x8910
#define MAX_XRES 1920
#define MAX_YRES 1080
#define GE_FB_NUM 3

#define HAVE_MALI
#define HAVE_VPP

extern int vbl;
extern int vsync;
extern int sync2;

unsigned int phy_mem_end(void);
unsigned int phy_mem_end_sub(unsigned int size);
int ge_init(struct fb_info *info);
int ge_exit(struct fb_info *info);
int ge_ioctl(struct fb_info *info, unsigned int cmd, unsigned long arg);
int ge_pan_display(struct fb_var_screeninfo *var, struct fb_info *info);
int ge_sync(struct fb_info *info);
int ge_release(struct fb_info *info);
int ge_blank(int mode, struct fb_info *info);
int ge_suspend(struct fb_info *info);
int ge_resume(struct fb_info *info);

void ge_vo_get_default_var(struct fb_var_screeninfo *var);
void ge_vo_wait_vsync(void);

#ifdef HAVE_VPP
#include "../vpp.h"
extern void vpp_wait_vsync(int idx, int cnt);
extern void vpp_set_mutex(int idx, int lock);
extern void vpp_get_info(int fbn, struct fb_var_screeninfo *var);
extern int vpp_pan_display(struct fb_var_screeninfo *var, struct fb_info *info);
extern int vpp_set_blank(struct fb_info *info, int blank);
extern int vpp_set_par(struct fb_info *info);
extern int wmtfb_check_var(struct fb_var_screeninfo *var, struct fb_info *info);
#endif /* HAVE_VPP */

#ifdef HAVE_MALI
extern unsigned int mali_ump_secure_id;
extern unsigned int (*mali_get_ump_secure_id)(unsigned int addr,
					      unsigned int size);
extern void         (*mali_put_ump_secure_id)(unsigned int ump_id);
#endif /* HAVE_MALI */

#endif
