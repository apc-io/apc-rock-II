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

#include <linux/init.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/errno.h>
#include <linux/uaccess.h>
#include <linux/kdev_t.h>
#include <linux/cdev.h>
#include <linux/fb.h>
#include <linux/dma-mapping.h>
#include <asm/page.h>
#include <linux/mm.h>
#include <linux/sched.h>

#include "ge_accel.h"

#define HAVE_MALI
#define WMT_MB

#ifdef HAVE_MALI
#include "../mali.h"
static struct mali_device *malidev;
#define UMP_INVALID_SECURE_ID    ((unsigned int)-1)
#define GET_UMP_SECURE_ID        _IOWR('m', 310, unsigned int)
#define GET_UMP_SECURE_ID_BUF1   _IOWR('m', 311, unsigned int)
#define GET_UMP_SECURE_ID_BUF2   _IOWR('m', 312, unsigned int)
#define MALI_GET_UMP_SECURE_ID   _IOWR('m', 320, unsigned int)
#define MALI_PUT_UMP_SECURE_ID   _IOWR('m', 321, unsigned int)
#endif /* HAVE_MALI */

#define USE_SID_ALIAS
/*
#define DEBUG_SID_ALIAS
*/

#ifdef USE_SID_ALIAS
#define SID_IDX_MAX 16
#define SID_GET_INDEX_FROM_ALIAS _IOWR('s', 100, unsigned int)
#define SID_SET_ALIAS            _IOWR('s', 101, unsigned int)
#define SID_GET_ALIAS            _IOWR('s', 102, unsigned int)
#define SID_GET_AND_RESET_ALIAS  _IOWR('s', 103, unsigned int)
#define SID_DUMP                 _IOWR('s', 104, unsigned int)

struct sid_alias {
	int sid;
	int alias;
};

static struct sid_alias sid_alias_buf[SID_IDX_MAX];
static spinlock_t sid_lock;

int sid_get_index_from_empty(int *index)
{
	int i;

	for (i = 0; i < SID_IDX_MAX; i++) {
		if (!sid_alias_buf[i].sid && !sid_alias_buf[i].alias) {
			*index = i;
			return 0;
		}
	}
	return -1;
}

int sid_get_index_from_alias(int alias, int *index)
{
	int i;

	for (i = 0; i < SID_IDX_MAX; i++) {
		if (sid_alias_buf[i].alias == alias) {
			*index = i;
			return 0;
		}
	}
	return -1;
}

int sid_clear_alias(int sid)
{
	int idx;

	for (idx = 0; idx < SID_IDX_MAX; idx++) {
		if (sid == sid_alias_buf[idx].sid ||
		    sid == sid_alias_buf[idx].alias) {
			sid_alias_buf[idx].sid = 0;
			sid_alias_buf[idx].alias = 0;
		}
	}

	return 0;
}

int sid_set_alias(int sid, int alias)
{
	int idx;

	if (alias <= 0)
		return sid_clear_alias(sid);

	sid_clear_alias(alias);

	if (sid_get_index_from_alias(sid, &idx) == 0) {
		sid_alias_buf[idx].alias = alias;
		if (alias <= 0)
			sid_alias_buf[idx].sid = 0;
		return 0;
	}

	sid_clear_alias(sid);

	if (sid_get_index_from_empty(&idx) == 0) {
		sid_alias_buf[idx].sid = sid;
		sid_alias_buf[idx].alias = alias;
		return 0;
	}

	return -1;
}

int sid_get_alias(int sid, int *alias)
{
	int i;
	int val = -1;

	for (i = 0; i < SID_IDX_MAX; i++) {
		if (sid_alias_buf[i].sid == sid) {
			if (sid_alias_buf[i].alias > 0) {
				val = sid_alias_buf[i].alias;
				if (sid != val)
					break;
			} else {
				/* remove invalid alias */
				sid_alias_buf[i].sid = 0;
				sid_alias_buf[i].alias = 0;
			}
		}
	}

	if (val > 0) {
		*alias = val;
		return 0;
	} else
		return -1;
}

int sid_get_and_reset_alias(int sid, int *alias)
{
	int i;
	int val = -1;

	for (i = 0; i < SID_IDX_MAX; i++) {
		if (sid_alias_buf[i].sid == sid) {
			if (sid_alias_buf[i].alias > 0) {
				val = sid_alias_buf[i].alias;
				sid_alias_buf[i].sid = val;
				if (sid != val)
					break;
			} else {
				/* remove invalid alias */
				sid_alias_buf[i].sid = 0;
				sid_alias_buf[i].alias = 0;
			}
		}
	}

	if (val > 0) {
		*alias = val;
		return 0;
	} else
		return -1;
}

void sid_dump(void)
{
	int i;

	for (i = 0; i < SID_IDX_MAX; i++)
		printk(KERN_ERR "sid_alias_buf[%d] = { %d, %d }\n",
			i, sid_alias_buf[i].sid, sid_alias_buf[i].alias);
}
#endif

#ifndef FBIO_WAITFORVSYNC
#define FBIO_WAITFORVSYNC _IOW('F', 0x20, u_int32_t)
#endif

#define GEIO_MAGIC		0x69

#ifdef GEIO_MAGIC
#define GEIOGET_CHIP_ID		_IOR(GEIO_MAGIC, 5, unsigned int)
#endif

static int vtotal = 32;
static int mbsize;

module_param(vtotal, int, S_IRUSR | S_IWUSR | S_IWGRP | S_IRGRP | S_IROTH);
MODULE_PARM_DESC(vtotal, "Maximum GE memory size in MiB");

module_param(mbsize, int, S_IRUSR | S_IWUSR | S_IWGRP | S_IRGRP | S_IROTH);
MODULE_PARM_DESC(mbsize, "WMT-MB size in MiB");

static struct fb_fix_screeninfo gefb_fix = {
	.id             = "gefb",
	.smem_start     = 0,
	.smem_len       = 0,
	.type           = FB_TYPE_PACKED_PIXELS,
	.type_aux       = 0,
	.visual         = FB_VISUAL_TRUECOLOR,
	.xpanstep       = 1,
	.ypanstep       = 1,
	.ywrapstep      = 1,
	.line_length    = 0,
#ifdef HAVE_MALI
	.mmio_start     = 0xd8080000,
	.mmio_len       = 0x10000,
#else
	.mmio_start     = 0,
	.mmio_len       = 0,
#endif
	.accel          = FB_ACCEL_WMT
};

static struct fb_var_screeninfo gefb_var = {
	.xres           = CONFIG_DEFAULT_RESX,
	.yres           = CONFIG_DEFAULT_RESY,
	.xres_virtual   = CONFIG_DEFAULT_RESX,
	.yres_virtual   = CONFIG_DEFAULT_RESY,
	/*
	.bits_per_pixel = 32,
	.red            = {16, 8, 0},
	.green          = {8, 8, 0},
	.blue           = {0, 8, 0},
	.transp         = {0, 0, 0},
	*/
	.bits_per_pixel = 16,
	.red            = {11, 5, 0},
	.green          = {5, 6, 0},
	.blue           = {0, 5, 0},
	.transp         = {0, 0, 0},
	.activate       = FB_ACTIVATE_NOW | FB_ACTIVATE_FORCE,
	.height         = -1,
	.width          = -1,
	.pixclock       = 39721,
	.left_margin    = 40,
	.right_margin   = 24,
	.upper_margin   = 32,
	.lower_margin   = 11,
	.hsync_len      = 96,
	.vsync_len      = 2,
	.vmode          = FB_VMODE_NONINTERLACED
};

static int gefb_open(struct fb_info *info, int user)
{
	return 0;
}

static int gefb_release(struct fb_info *info, int user)
{
	return ge_release(info);
}

static int gefb_check_var(struct fb_var_screeninfo *var,
			      struct fb_info *info)
{
#ifdef HAVE_VPP
	return wmtfb_check_var(var, info);
#else
	switch (var->bits_per_pixel) {
	case 1:
	case 8:
		if (var->red.offset > 8) {
			/* LUT8 */
			var->red.offset = 0;
			var->red.length = 8;
			var->green.offset = 0;
			var->green.length = 8;
			var->blue.offset = 0;
			var->blue.length = 8;
			var->transp.offset = 0;
			var->transp.length = 0;
		}
		break;
	case 16:
		if (var->transp.length) {
			/* ARGB 1555 */
			var->red.offset = 10;
			var->red.length = 5;
			var->green.offset = 5;
			var->green.length = 5;
			var->blue.offset = 0;
			var->blue.length = 5;
			var->transp.offset = 15;
			var->transp.length = 1;
		} else {
			/* RGB 565 */
			var->red.offset = 11;
			var->red.length = 5;
			var->green.offset = 5;
			var->green.length = 6;
			var->blue.offset = 0;
			var->blue.length = 5;
			var->transp.offset = 0;
			var->transp.length = 0;
		}
		break;
	case 24:
		/* RGB 888 */
		var->red.offset = 16;
		var->red.length = 8;
		var->green.offset = 8;
		var->green.length = 8;
		var->blue.offset = 0;
		var->blue.length = 8;
		var->transp.offset = 0;
		var->transp.length = 0;
		break;
	case 32:
		/* ARGB 8888 */
		var->red.offset = 16;
		var->red.length = 8;
		var->green.offset = 8;
		var->green.length = 8;
		var->blue.offset = 0;
		var->blue.length = 8;
		var->transp.offset = 0;
		var->transp.length = 0;
		break;
	}
	return 0;
#endif
}

static int gefb_set_par(struct fb_info *info)
{
	struct fb_var_screeninfo *var = &info->var;

	/* init your hardware here */
	if (var->bits_per_pixel == 8)
		info->fix.visual = FB_VISUAL_PSEUDOCOLOR;
	else
		info->fix.visual = FB_VISUAL_TRUECOLOR;

	if (ge_init(info))
		return -ENOMEM;

#ifdef HAVE_VPP
	vpp_set_par(info);
#endif

	info->fix.line_length = var->xres_virtual * var->bits_per_pixel / 8;

	return 0;
}

static int gefb_setcolreg(unsigned regno, unsigned red,
			      unsigned green, unsigned blue,
			      unsigned transp, struct fb_info *info)
{
	if (regno >= 256)  /* no. of hw registers */
		return -EINVAL;

	/* grayscale */

	if (info->var.grayscale) {
		/* grayscale = 0.30*R + 0.59*G + 0.11*B */
		red = green = blue = (red * 77 + green * 151 + blue * 28) >> 8;
	}

	/*  The following is for fbcon. */

	if (info->fix.visual == FB_VISUAL_TRUECOLOR ||
		info->fix.visual == FB_VISUAL_DIRECTCOLOR) {

		if (regno >= 16)
			return -EINVAL;

		switch (info->var.bits_per_pixel) {
		case 16:
			((unsigned int *)(info->pseudo_palette))[regno] =
				(red & 0xf800) |
				((green & 0xfc00) >> 5) |
				((blue & 0xf800) >> 11);
				break;
		case 24:
		case 32:
			red   >>= 8;
			green >>= 8;
			blue  >>= 8;
			((unsigned int *)(info->pseudo_palette))[regno] =
				(red << info->var.red.offset) |
				(green << info->var.green.offset) |
				(blue  << info->var.blue.offset);
			break;
		}
	}
	return 0;
}

static int gefb_ioctl(struct fb_info *info, unsigned int cmd,
			  unsigned long arg)
{
	int retval = 0;

	switch (cmd) {
	case FBIO_WAITFORVSYNC:
		ge_vo_wait_vsync();
		break;
#ifdef HAVE_MALI
	case GET_UMP_SECURE_ID:
	case GET_UMP_SECURE_ID_BUF1:
	case GET_UMP_SECURE_ID_BUF2: {
		unsigned int ump_id;
		if (mali_get_ump_secure_id)
			ump_id = (*mali_get_ump_secure_id)(info->fix.smem_start,
							   info->fix.smem_len);
		else
			ump_id = UMP_INVALID_SECURE_ID;
		return put_user((unsigned int) ump_id,
				(unsigned int __user *) arg);
	}
	case MALI_GET_UMP_SECURE_ID: {
		unsigned int args[3];
		unsigned int ump_id;
		copy_from_user(args, (void *)arg, sizeof(unsigned int) * 3);

		if (mali_get_ump_secure_id)
			ump_id = (*mali_get_ump_secure_id)(args[0], args[1]);
		else
			ump_id = UMP_INVALID_SECURE_ID;

		return put_user((unsigned int) ump_id,
				(unsigned int __user *) args[2]);
	}
	case MALI_PUT_UMP_SECURE_ID: {
		unsigned int ump_id = (unsigned int)arg;
		if (mali_put_ump_secure_id)
			(*mali_put_ump_secure_id)(ump_id);
		break;
	}
#endif /* HAVE_MALI */
#ifdef GEIO_MAGIC
	case GEIOGET_CHIP_ID: {
		unsigned int chip_id =
			(*(unsigned int *)SYSTEM_CFG_CTRL_BASE_ADDR);
		copy_to_user((void *)arg, (void *)&chip_id,
			sizeof(unsigned int));
		break;
	}
#endif /* GEIO_MAGIC */
#ifdef USE_SID_ALIAS
	case SID_GET_INDEX_FROM_ALIAS: {
		unsigned int args[2];
		unsigned int index = -1;
		copy_from_user(args, (void *)arg, sizeof(unsigned int) * 2);
		spin_lock(&sid_lock);
		retval = sid_get_index_from_alias(args[0], &index);
		spin_unlock(&sid_lock);
		put_user(index, (unsigned int __user *)args[1]);
		break;
	}
	case SID_SET_ALIAS: {
		unsigned int args[2];
		copy_from_user(args, (void *)arg, sizeof(unsigned int) * 2);
		spin_lock(&sid_lock);
		retval = sid_set_alias(args[0], args[1]);
#ifdef DEBUG_SID_ALIAS
		printk(KERN_DEBUG "sid_set_alias %d, %d, ret = %d\n",
		       args[0], args[1], retval);
		sid_dump();
#endif
		spin_unlock(&sid_lock);
		break;
	}
	case SID_GET_ALIAS: {
		unsigned int args[2];
		unsigned int alias = -1;
		copy_from_user(args, (void *)arg, sizeof(unsigned int) * 2);
		spin_lock(&sid_lock);
		retval = sid_get_alias(args[0], &alias);
		spin_unlock(&sid_lock);
		put_user(alias, (unsigned int __user *)args[1]);
		break;
	}
	case SID_GET_AND_RESET_ALIAS: {
		unsigned int args[2];
		unsigned int alias = -1;
		copy_from_user(args, (void *)arg, sizeof(unsigned int) * 2);
		spin_lock(&sid_lock);
		retval = sid_get_and_reset_alias(args[0], &alias);
#ifdef DEBUG_SID_ALIAS
		printk(KERN_DEBUG "sid_get_and_reset_alias %d, %d, ret = %d\n",
		       args[0], alias, retval);
		sid_dump();
#endif
		spin_unlock(&sid_lock);
		put_user(alias, (unsigned int __user *)args[1]);
		break;
	}
	case SID_DUMP: {
		spin_lock(&sid_lock);
		copy_to_user((void *)arg, sid_alias_buf,
			sizeof(struct sid_alias) * SID_IDX_MAX);
		retval = 0;
		spin_unlock(&sid_lock);
		break;
	}
#endif
	default:
		break;
	}

	return retval;
}

int gefb_hw_cursor(struct fb_info *info, struct fb_cursor *cursor)
{
	return 0;
}

static int gefb_mmap(struct fb_info *info, struct vm_area_struct *vma)
{
	unsigned long off;
	unsigned long start;
	u32 len;
	int ismmio = 0;

	if (!info)
		return -ENODEV;
	if (vma->vm_pgoff > (~0UL >> PAGE_SHIFT))
		return -EINVAL;
	off = vma->vm_pgoff << PAGE_SHIFT;

	/* frame buffer memory */
	start = info->fix.smem_start;
	len = PAGE_ALIGN((start & ~PAGE_MASK) + info->fix.smem_len);
	if (off >= len) {
		/* memory mapped io */
		off -= len;
		/*
		if (info->var.accel_flags) {
			return -EINVAL;
		}
		*/
		start = info->fix.mmio_start;
		len = PAGE_ALIGN((start & ~PAGE_MASK) + info->fix.mmio_len);
		ismmio = 1;
	}
	start &= PAGE_MASK;
	if ((vma->vm_end - vma->vm_start + off) > len)
		return -EINVAL;
	off += start;
	vma->vm_pgoff = off >> PAGE_SHIFT;
	/* This is an IO map - tell maydump to skip this VMA */
	vma->vm_flags |= VM_IO | VM_RESERVED;
	vma->vm_page_prot = vm_get_page_prot(vma->vm_flags);

	if (ismmio)
		vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
	else
		vma->vm_page_prot = pgprot_writecombine(vma->vm_page_prot);

	if (io_remap_pfn_range(vma, vma->vm_start, off >> PAGE_SHIFT,
			     vma->vm_end - vma->vm_start, vma->vm_page_prot))
		return -EAGAIN;
	return 0;
}

static struct fb_ops gefb_ops = {
	.owner          = THIS_MODULE,
	.fb_open        = gefb_open,
	.fb_release     = gefb_release,
	.fb_check_var   = gefb_check_var,
	.fb_set_par     = gefb_set_par,
	.fb_setcolreg   = gefb_setcolreg,
	.fb_pan_display = ge_pan_display,
	.fb_fillrect    = cfb_fillrect,
	.fb_copyarea    = cfb_copyarea,
	.fb_imageblit   = cfb_imageblit,
	.fb_blank       = ge_blank,
	.fb_cursor      = gefb_hw_cursor,
	.fb_ioctl       = gefb_ioctl,
	.fb_sync	= ge_sync,
	.fb_mmap	= gefb_mmap,
};

#define OPT_EQUAL(opt, name) (!strncmp(opt, name, strlen(name)))
#define OPT_INTVAL(opt, name) kstrtoul(opt + strlen(name) + 1, 0, NULL)
#define OPT_STRVAL(opt, name) (opt + strlen(name))

static inline char *get_opt_string(const char *this_opt, const char *name)
{
	const char *p;
	int i;
	char *ret;

	p = OPT_STRVAL(this_opt, name);
	i = 0;
	while (p[i] && p[i] != ' ' && p[i] != ',')
		i++;
	ret = kmalloc(i + 1, GFP_KERNEL);
	if (ret) {
		strncpy(ret, p, i);
		ret[i] = '\0';
	}
	return ret;
}

static inline int get_opt_int(const char *this_opt, const char *name,
				  int *ret)
{
	if (!ret)
		return 0;

	if (!OPT_EQUAL(this_opt, name))
		return 0;

	*ret = OPT_INTVAL(this_opt, name);

	return 1;
}

static inline int get_opt_bool(const char *this_opt, const char *name,
				   int *ret)
{
	if (!ret)
		return 0;

	if (OPT_EQUAL(this_opt, name)) {
		if (this_opt[strlen(name)] == '=')
			*ret = kstrtoul(this_opt + strlen(name) + 1, 0, NULL);
		else
			*ret = 1;
	} else {
		if (OPT_EQUAL(this_opt, "no") && OPT_EQUAL(this_opt + 2, name))
			*ret = 0;
		else
			return 0;
	}
	return 1;
}

static int __init gefb_setup(char *options)
{
	char *this_opt;

	if (!options || !*options)
		return 0;

	/* The syntax is:
	 *     video=gefb:[<param>][,<param>=<val>] ...
	 * e.g.,
	 *     video=gefb:vtotal=12,sync2
	 */

	while ((this_opt = strsep(&options, ",")) != NULL) {
		if (!*this_opt)
			continue;
		if (get_opt_int(this_opt, "vtotal", &vtotal))
			;
		else if (get_opt_int(this_opt, "mbsize", &mbsize))
			;
		else if (get_opt_bool(this_opt, "vbl", &vbl))
			;
		else if (get_opt_bool(this_opt, "vsync", &vsync))
			;
		else if (get_opt_bool(this_opt, "sync2", &sync2))
			;
	}

	return 0;
}

#ifdef HAVE_MALI
static struct mali_device *add_mali_device(unsigned int *smem_start_ptr,
					   unsigned int *smem_len_ptr)
{
	unsigned int len;
	struct mali_device *dev = create_mali_device();
	if (dev) {
		dev->get_memory_base(smem_start_ptr);
		dev->get_memory_size(&len);
		*smem_start_ptr += len;
		*smem_len_ptr -= len;
		dev->set_mem_validation_base(*smem_start_ptr);
		dev->set_mem_validation_size(*smem_len_ptr);
	}
	return dev;
}
#endif /* HAVE_MALI */

#ifdef WMT_MB
static int get_mbsize(void)
{
	/* It is bad to read U-Boot partition directly.
	 * I will remove this code soon.
	 * -- Vincent
	 */
	unsigned char buf[32];
	int varlen = 32;
	int val;

	if (wmt_getsyspara("mbsize", buf, &varlen) == 0)
		sscanf(buf, "%dM", &val);
	else
		val = 0;

	return val;
}

static void add_mb_device(unsigned int *smem_start_ptr,
			  unsigned int *smem_len_ptr)
{
	unsigned int len = mbsize << 20;

	if (*smem_len_ptr > len)
		*smem_len_ptr -= len;
}
#endif /* WMT_MB */

static int __devinit gefb_probe(struct platform_device *dev)
{
	struct fb_info *info;
	int cmap_len, retval;
	char mode_option[] = "1024x768@60";
	unsigned int smem_start;
	unsigned int smem_len;
	unsigned int len;
	unsigned int min_smem_len;

	/* Allocate fb_info and par.*/
	info = framebuffer_alloc(sizeof(unsigned int) * 16, &dev->dev);
	if (!info)
		return -ENOMEM;

	/* Set default fb_info */
	info->fbops = &gefb_ops;
	info->fix = gefb_fix;

	info->var = gefb_var;
	ge_vo_get_default_var(&info->var);

	smem_start = (num_physpages << PAGE_SHIFT);
	smem_len = phy_mem_end() - smem_start;

#ifdef HAVE_MALI
	malidev = add_mali_device(&smem_start, &smem_len);
#endif /* HAVE_MALI */

#ifdef WMT_MB
	add_mb_device(&smem_start, &smem_len);
#endif /* WMT_MB */

	/* Set frame buffer region */

	len = info->var.xres * info->var.yres *
		(info->var.bits_per_pixel >> 3);
	len *= GE_FB_NUM;
	min_smem_len = (len + PAGE_MASK) & ~PAGE_MASK;

	if (smem_len < min_smem_len) {
		printk(KERN_ERR "%s: claim region of 0x%08x-0x%08x failed!\n",
			__func__, smem_start, smem_start + min_smem_len);
		return -EIO;
	}

	info->fix.smem_start = smem_start;

	if (smem_len > (vtotal << 20))
		smem_len = (vtotal << 20);

	info->fix.smem_len = smem_len;

	if (!request_mem_region(info->fix.smem_start,
		info->fix.smem_len, "gefb")) {
		printk(KERN_WARNING
			"%s: request memory region failed at 0x%08lx\n",
			__func__, info->fix.smem_start);
	}

	info->screen_base = ioremap(info->fix.smem_start,
		info->fix.smem_len);
	if (!info->screen_base) {
		printk(KERN_ERR "%s: ioremap fail %d bytes at %p\n",
			__func__, info->fix.smem_len,
			(void *)info->fix.smem_start);
		return -EIO;
	}

	printk(KERN_INFO
		"gefb: phys 0x%08lx, virt 0x%08lx, total %d KB\n",
		info->fix.smem_start, (unsigned long)info->screen_base,
		info->fix.smem_len >> 10);

	/*
	 * The pseudopalette is an 16-member array for fbcon.
	 */
	info->pseudo_palette = info->par;
	info->par = NULL;
	info->flags = FBINFO_DEFAULT;	/* flag for fbcon */

	/*
	 * This should give a reasonable default video mode.
	 */
	retval = fb_find_mode(&info->var, info, mode_option,
			      NULL, 0, NULL, 8);

	if (!retval || retval == 4)
		return -EINVAL;

	/*
	 *  This has to been done !!!
	 */
	cmap_len = 256;	/* Be the same as VESA */
	retval = fb_alloc_cmap(&info->cmap, cmap_len, 0);
	if (retval < 0)
		printk(KERN_ERR "%s: fb_alloc_cmap fail.\n", __func__);

	/*
	 *  The following is done in the case of
	 *  having hardware with a static mode.
	 */
	info->var = gefb_var;

	/*
	 *  Get setting from video output device.
	 */
	ge_vo_get_default_var(&info->var);

	/*
	 *  For drivers that can...
	 */
	gefb_check_var(&info->var, info);

	/*
	 *  Apply setting
	 */
	gefb_set_par(info);
	ge_pan_display(&info->var, info);

	if (register_framebuffer(info) < 0) {
		ge_exit(info);
		return -EINVAL;
	}
	info->dev->power.async_suspend = 1; /* Add by Charles */
	dev_set_drvdata(&dev->dev, info);

#ifdef USE_SID_ALIAS
	spin_lock_init(&sid_lock);
	memset(sid_alias_buf, 0, sizeof(struct sid_alias) * SID_IDX_MAX);
#endif

	return 0;
}

static int gefb_remove(struct platform_device *dev)
{
	struct fb_info *info = dev_get_drvdata(&dev->dev);

	if (info) {
		ge_exit(info);
		unregister_framebuffer(info);
		fb_dealloc_cmap(&info->cmap);
		framebuffer_release(info);
	}
	return 0;
}

static int gefb_suspend(struct platform_device *dev, pm_message_t state)
{
	struct fb_info *info = dev_get_drvdata(&dev->dev);

	if (info)
		ge_suspend(info);

#ifdef HAVE_MALI
	if (malidev)
		malidev->suspend(1);
#endif

	return 0;
}

static int gefb_resume(struct platform_device *dev)
{
	struct fb_info *info = dev_get_drvdata(&dev->dev);

	if (info)
		ge_resume(info);

#ifdef HAVE_MALI
	if (malidev)
		malidev->resume(1);
#endif

	return 0;
}

static struct platform_driver gefb_driver = {
	.driver.name    = "gefb",
	.probe          = gefb_probe,
	.remove         = gefb_remove,
	.suspend        = gefb_suspend,
	.resume         = gefb_resume,
};

static u64 gefb_dma_mask = 0xffffffffUL;
static struct platform_device gefb_device = {
	.name   = "gefb",
	.dev    = {
		.dma_mask = &gefb_dma_mask,
		.coherent_dma_mask = ~0,
	},
};

#ifdef WMT_MB
static int __init mbsize_setup(char *options)
{
	char *this_opt;

	if (!options || !*options)
		return 0;

	while ((this_opt = strsep(&options, ",")) != NULL) {
		if (!*this_opt)
			continue;
		sscanf(this_opt, "%dM", &mbsize);
		printk(KERN_DEBUG "gefb: detected mbsize = %d MiB\n", mbsize);
	}

	return 0;
}
__setup("mbsize=", mbsize_setup);
#endif /* WMT_MB */

static int __init gefb_init(void)
{
	int ret;
	char *option = NULL;

	fb_get_options("gefb", &option);
	gefb_setup(option);

#ifdef WMT_MB
	/* It is bad to read U-Boot partition directly.
	 * I will remove this code soon.
	 * -- Vincent
	 */
	if (!mbsize) {
		mbsize = get_mbsize();
		printk(KERN_ERR "Please add \'mbsize=%dM\' in bootargs!",
			mbsize);
	}
#endif /* WMT_MB */

	ret = platform_driver_register(&gefb_driver);
	if (!ret) {
		ret = platform_device_register(&gefb_device);
		if (ret)
			platform_driver_unregister(&gefb_driver);
	}

	return ret;
}
module_init(gefb_init);

static void __exit gefb_exit(void)
{
	release_mali_device(malidev);

	platform_driver_unregister(&gefb_driver);
	platform_device_unregister(&gefb_device);
	return;
}

module_exit(gefb_exit);

MODULE_AUTHOR("WonderMedia Technologies, Inc.");
MODULE_DESCRIPTION("WMT GE driver");
MODULE_LICENSE("GPL");

