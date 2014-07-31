/*++
 * linux/drivers/media/video/wmt_v4l2/sensors/wmt-cmos.c
 * WonderMedia v4l cmos device driver
 *
 * Copyright c 2010  WonderMedia  Technologies, Inc.
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

#include <linux/module.h>
#include <linux/version.h>
#include <linux/videodev2.h>
#include <media/v4l2-common.h>
#include <media/v4l2-ioctl.h>
#include <media/v4l2-device.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/major.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <linux/i2c.h>

#include <linux/wmt-mb.h>
#include <mach/wmt_env.h>


#include "wmt-cmos.h"
#include "cmos-subdev.h"
#include "../wmt-vid.h"

#include "flash/flash.h"

#define THE_MB_USER "CMOS-MB"

#undef pr_err
#define pr_err(fmt, args...)	printk("[" THE_MB_USER "] " "%s, %d:" fmt, __func__, __LINE__, ##args)

//#define CMOS_REG_TRACE
#ifdef CMOS_REG_TRACE
#define CMOS_REG_SET32(addr, val)  \
	PRINT("REG_SET:0x%x -> 0x%0x\n", addr, val);\
	REG32_VAL(addr) = (val)
#else
#define CMOS_REG_SET32(addr, val)      REG32_VAL(addr) = (val)
#endif

//#define CMOS_DEBUG    /* Flag to enable debug message */
//#define CMOS_TRACE

#ifdef CMOS_DEBUG
#define DBG_MSG(fmt, args...)    printk("{%s} " fmt, __FUNCTION__ , ## args)
#else
#define DBG_MSG(fmt, args...)
#endif

#ifdef CMOS_TRACE
#define TRACE(fmt, args...)      printk("{%s}:  " fmt, __FUNCTION__ , ## args)
#else
#define TRACE(fmt, args...)
#endif

#define MAX_FB_IN_QUEUE		10
#define NUM_INPUTS		3

#define CAMERA_TYPE_NA          0   /* not avaliable */
#define CAMERA_TYPE_CMOS        1   /* CMOS */
#define CAMERA_TYPE_USB         2   /* USB  */

struct regulator *cam_vccvid = NULL;
struct regulator *cam_dvdd = NULL;

typedef enum {
	STS_CMOS_READY    = 0,
	STS_CMOS_WAIT     = 0x0001,
	STS_CMOS_RUNNING  = 0x0002,
	STS_CMOS_FB_DONE  = 0x0004
} cmos_status;

typedef struct {
	VID_FB_M;
	struct list_head list;
} cmos_fb_t;

struct cmos_drvinfo {
	struct list_head  head;

	unsigned int  frame_size;
	unsigned int  width;
	unsigned int  height;

	unsigned int  dft_y_addr;
	unsigned int  dft_c_addr;

	cmos_fb_t     fb_pool[MAX_FB_IN_QUEUE];
	unsigned int  fb_cnt;

	unsigned int  streamoff;
	unsigned int  dqbuf;

	cmos_status   status;
};

// sync with UbootParam.h in Hal.
enum {
    CAMERA_CAP_FLASH        = 1 << 0,
    CAMERA_CAP_FLASH_AUTO   = 1 << 1,
    CAMERA_CAP_HDR          = 1 << 2,
};

struct cmos_dev {
	int	type;
	int	pwdn_gpio;
	int	pwdn_pol;
	int	mirror;
	u32	cap;

	int	id;
	struct cmos_subdev *sd;
};

struct cmos_info {
	int	input;
	int	input_count;

	struct cmos_drvinfo	drvinfo;
	struct cmos_dev		cd[NUM_INPUTS];
	struct flash_dev	*fl;

	struct mutex	lock;
};

static inline struct cmos_subdev *current_subdev(struct cmos_info *info)
{
	if (!info || info->input >= NUM_INPUTS)
		return NULL;

	return info->cd[info->input].sd;
}

static DECLARE_WAIT_QUEUE_HEAD(cmos_wait);
static spinlock_t cmos_lock;
static int cmos_dev_ref;

static int cmos_dev_setup(struct cmos_info *info)
{
	struct cmos_dev *cd;
	int rc = 0;
	int i;

	for (i = 0; i < info->input_count; i++) {
		cd = &info->cd[i];
		if (cd->type != CAMERA_TYPE_CMOS)
			continue;
		rc = gpio_request(cd->pwdn_gpio, "cmos pwdn");
		if (rc) {
			pr_err("cmos gpio%d request failed\n", cd->pwdn_gpio);
			goto err_gpio_req;
		}
		gpio_direction_output(cd->pwdn_gpio, !cd->pwdn_pol);
	}
	return 0;

err_gpio_req:
	while (i--)
		gpio_free(info->cd[i].pwdn_gpio);
	return rc;
}

static void cmos_dev_power_up(struct cmos_dev *cd)
{
	if (cd->pwdn_gpio > 0) {
		printk(KERN_DEBUG "%s, gpio-%d\n", __func__, cd->pwdn_gpio);
		gpio_direction_output(cd->pwdn_gpio, cd->pwdn_pol);
		msleep(20);
	}
}

static void cmos_dev_power_down(struct cmos_dev *cd)
{
	if (cd->pwdn_gpio > 0) {
		printk(KERN_DEBUG "%s, gpio-%d\n", __func__, cd->pwdn_gpio);
		gpio_direction_output(cd->pwdn_gpio, !cd->pwdn_pol);
	}
}

static void cmos_dev_release(struct cmos_info *info)
{
	struct cmos_dev *cd;
	int i;

	for (i = 0; i < info->input_count; i++) {
		cd = &info->cd[i];
		if (cd->type != CAMERA_TYPE_CMOS)
			continue;
		cmos_dev_power_down(cd);
		gpio_free(cd->pwdn_gpio);
	}
}

static int parse_camera_param(struct cmos_info *info)
{
	static char env[] = "wmt.camera.param";
	struct cmos_dev cd[2];
	char buf[64];
	size_t l = sizeof(buf);
	int i, n;
	int rc;

	rc = wmt_getsyspara(env, buf, &l);
	if (rc) {
		pr_err("Invalid param, please set %s\n", env);
		return -EINVAL;
	}

	memset(cd, 0, sizeof(cd));
	sscanf(buf, "%d:%d:%d:%d:%d:%d:%d:%d:%d:%x",
	       &cd[1].type, &cd[1].pwdn_gpio, &cd[1].pwdn_pol, &cd[1].mirror, &cd[1].cap,
	       &cd[0].type, &cd[0].pwdn_gpio, &cd[0].pwdn_pol, &cd[0].mirror, &cd[0].cap);

	for (n = 0, i = 0; i < ARRAY_SIZE(cd); i++) {
		struct cmos_dev *c = &cd[i];

		if (c->type != CAMERA_TYPE_CMOS && c->type != CAMERA_TYPE_USB)
			continue;

		info->cd[n] = cd[i];
		info->cd[n].id = n;

		pr_info("#%d: type %d, pwdn_gpio %d, pwdn_pol %d, mirror %d, cap 0x%x\n",
			n, c->type, c->pwdn_gpio, c->pwdn_pol, c->mirror, c->cap);
		n++;
	}
	info->input_count = n;

	return (n == 0) ? -EINVAL : 0;
}

static int cam_enable(struct cmos_drvinfo *drv, bool en)
{
	drv->streamoff = en ? 0 : 1;

	CMOS_REG_SET32(REG_VID_CMOS_EN, en);	/* enable CMOS */

	return 0;
}

static int print_queue(struct cmos_drvinfo *drv)
{
#ifdef CMOS_DEBUG
	struct list_head *next;
	cmos_fb_t  *fb;

	TRACE("Enter\n");

	list_for_each(next, &drv->head) {
		fb = (cmos_fb_t *) list_entry(next, cmos_fb_t, list);
		DBG_MSG("[%d] y_addr: 0x%x, c_addr: 0x%x\n",
			fb->id, fb->y_addr, fb->c_addr );
	}
	TRACE("Leave\n");
#endif
	return 0;
}

static int put_queue(struct cmos_drvinfo *drv, cmos_fb_t *fb_in)
{
	TRACE("Enter\n");

	list_add_tail(&fb_in->list, &drv->head);
	wmb();
	print_queue(drv);

	TRACE("Leave\n");

	return 0;
}

static int pop_queue(struct cmos_drvinfo *drv, cmos_fb_t *fb_in)
{
	TRACE("Enter\n");

	list_del(&fb_in->list);
	wmb();
	print_queue(drv);

	TRACE("Leave\n");

	return 0;
}

static int cmos_cam_querycap(struct file *file, void *fh, struct v4l2_capability *cap)
{
	TRACE("Enter\n");

	strlcpy(cap->driver, "wm88xx", sizeof(cap->driver));

	cap->version = KERNEL_VERSION(0, 0, 1);

	cap->capabilities = V4L2_CAP_VIDEO_CAPTURE |
		V4L2_CAP_READWRITE | V4L2_CAP_STREAMING;

	TRACE("Leave\n");
	return 0;
}

static int cmos_cam_cropcap(struct file *file, void *fh, struct v4l2_cropcap *cropcap)
{
	pr_err("Not support now!\n");
	return -EINVAL;
}

struct wmt_fmt {
	char  *name;
	u32   fourcc;          /* v4l2 format id */
	int   depth;
};

static struct wmt_fmt formats[] = {
	{
		.name     = "4:2:2, packed, YUYV",
		.fourcc   = V4L2_PIX_FMT_YUYV,
		.depth    = 16,
	},
};

static int cmos_cam_enum_fmt_cap(struct file *file, void *fh, struct v4l2_fmtdesc *f)
{
	struct wmt_fmt *fmt;

	if (f->index >= ARRAY_SIZE(formats))
		return -EINVAL;

	fmt = &formats[f->index];

	strlcpy(f->description, fmt->name, sizeof(f->description));
	f->pixelformat = fmt->fourcc;
	return 0;
}

static int cmos_cam_enum_framesizes(struct file *file, void *fh,
				    struct v4l2_frmsizeenum *fsize)
{
	struct cmos_info *info = file->private_data;
	struct cmos_subdev *sd = current_subdev(info);
	int ret;

	mutex_lock(&info->lock);
	ret = cmos_subdev_call(sd, enum_framesizes, fsize);
	mutex_unlock(&info->lock);
	return ret;
}

static int cmos_cam_g_fmt_cap(struct file *file, void *fh, struct v4l2_format *f)
{
	struct cmos_info *info = file->private_data;
	struct cmos_drvinfo *drv = &info->drvinfo;

	f->fmt.pix.width = drv->width;
	f->fmt.pix.height = drv->height;
	return 0;
}

#define pixfmtstr(x) (x) & 0xff, ((x) >> 8) & 0xff, ((x) >> 16) & 0xff, \
	((x) >> 24) & 0xff

static int cmos_cam_s_fmt_cap(struct file *file, void *fh, struct v4l2_format *f)
{
	struct cmos_info *info = file->private_data;
	struct cmos_drvinfo *drv = &info->drvinfo;
	struct cmos_subdev *sd = current_subdev(info);
	struct v4l2_mbus_framefmt mf;
	int rc;

	TRACE("Enter\n");
	mutex_lock(&info->lock);

	printk(KERN_DEBUG "%s: S_FMT %dx%d, fmt %c%c%c%c\n",
	       sd->name, f->fmt.pix.width, f->fmt.pix.height,
	       pixfmtstr(f->fmt.pix.pixelformat));

	mf.width	= f->fmt.pix.width;
	mf.height	= f->fmt.pix.height;
	mf.field	= f->fmt.pix.field;
	mf.colorspace	= f->fmt.pix.colorspace;

	rc = cmos_subdev_call(sd, s_mbus_fmt, &mf);
	if (rc) {
		pr_err("%s, %s: s_mbus_fmt fail\n", __func__, sd->name);
		goto out;
	}

	/* host set fmt */
	rc = wmt_vid_set_mode(mf.width, mf.height, f->fmt.pix.pixelformat);
	if (rc) {
		goto out;
	}

	drv->width	= mf.width;
	drv->height	= mf.height;

	switch (f->fmt.pix.pixelformat) {
	case V4L2_PIX_FMT_RGB32:	// *4 for  ARGB display frambuffer
		drv->frame_size = drv->width * drv->height * 4;
		break;
	case V4L2_PIX_FMT_NV21:
	case V4L2_PIX_FMT_NV12:
		drv->frame_size = (drv->width * drv->height * 3) / 2;
		break;
	case V4L2_PIX_FMT_YUYV:		// YC422
		drv->frame_size = drv->width * drv->height * 2;
		break;
	default:
		drv->frame_size = 0;
		rc = -EINVAL;
		break;
	}

out:
	mutex_unlock(&info->lock);
	TRACE("Leave\n");
	return rc;
}

static int cmos_cam_try_fmt_cap(struct file *file, void *fh, struct v4l2_format *f)
{
	struct cmos_info *info = file->private_data;
	struct cmos_subdev *sd = current_subdev(info);
	struct v4l2_mbus_framefmt mf;
	int ret;

	TRACE("Enter\n");
	mutex_lock(&info->lock);

	mf.width	= f->fmt.pix.width;
	mf.height	= f->fmt.pix.height;
	mf.field	= f->fmt.pix.field;
	mf.colorspace	= f->fmt.pix.colorspace;

	ret = cmos_subdev_call(sd, try_mbus_fmt, &mf);
	if (!ret) {
		f->fmt.pix.width = mf.width;
		f->fmt.pix.height = mf.height;
	}

	mutex_unlock(&info->lock);
	TRACE("Leave\n");
	return ret;
}

static int cmos_cam_reqbufs(struct file *file, void *fh, struct v4l2_requestbuffers *rb)
{
	struct cmos_info *info = file->private_data;
	struct cmos_drvinfo *drv = &info->drvinfo;
	cmos_fb_t *fb;
	int  i, j;
	int ret = 0;

	TRACE("Enter\n");
	mutex_lock(&info->lock);

	if ((rb->type != V4L2_BUF_TYPE_VIDEO_CAPTURE) ||
	    (rb->memory != V4L2_MEMORY_MMAP) ||
	    (rb->count > MAX_FB_IN_QUEUE) ) {
		pr_err("rb->type: 0x%x, rb->memory: 0x%x, rb->count: %d\n",
		       rb->type, rb->memory, rb->count);
		ret = -EINVAL;
		goto out;
	}

	if (!drv->streamoff) {
		pr_err("reqbufs: streaming active\n");
		ret = -EBUSY;
		goto out;
	}

	if (rb->count == 0) {
		/* Free memory */
		for (i = 0; i < drv->fb_cnt; i++) {
			fb = &drv->fb_pool[i];
			if (fb->y_addr) {
				mb_free(fb->y_addr);
				memset(fb, 0, sizeof(cmos_fb_t));
			}
		}
		drv->fb_cnt = 0;
		INIT_LIST_HEAD(&drv->head);
		goto out;
	}

	drv->fb_cnt = rb->count;

	DBG_MSG(" Frame Size: %d Bytes\n", drv->frame_size);
	DBG_MSG(" fb_cnt:     %d\n", drv->fb_cnt);

	/* Allocate memory */
	for (i = 0; i < drv->fb_cnt; i++) {
		fb = &drv->fb_pool[i];
		fb->y_addr = mb_alloc(drv->frame_size);
		DBG_MSG("%d CMOS MB_allocate size %d\n", i,drv->frame_size);
		if (fb->y_addr == 0) {
			/* Allocate MB memory size fail */
			pr_err("[%d] >> Allocate MB memory (%d) fail!\n", i, drv->frame_size);
			for (j=0; j<i; j++) {
				fb = &drv->fb_pool[j];
				mb_free(fb->y_addr);
			}
			ret = -EINVAL;
			goto out;
		}
		fb->c_addr = fb->y_addr + drv->width * drv->height;
		fb->id = i;
		fb->is_busy = 0;
		fb->done    = 0;
		DBG_MSG("[%d] fb: 0x%x, y_addr: 0x%x, c_addr: 0x%x\n",
			i, (unsigned int)fb, fb->y_addr, fb->c_addr);
	}

out:
	mutex_unlock(&info->lock);
	TRACE("Leave\n");
	return ret;
}

static int cmos_cam_querybuf(struct file *file, void *fh, struct v4l2_buffer *b)
{
	struct cmos_info *info = file->private_data;
	struct cmos_drvinfo *drv = &info->drvinfo;

	TRACE("Enter (index: %d)\n", b->index);
	mutex_lock(&info->lock);

	if (b->index < MAX_FB_IN_QUEUE) {
		cmos_fb_t *fb = &drv->fb_pool[b->index];
		b->length   = drv->frame_size;
		b->m.offset = fb->y_addr;
	} else {
		b->length   = 0;
		b->m.offset = 0;
	}
	DBG_MSG(" b->length:     %d\n", b->length);
	DBG_MSG(" b->m.offset: 0x%x\n", b->m.offset);

	mutex_unlock(&info->lock);
	TRACE("Leave (index: %d)\n", b->index);
	return 0;
}

static int cmos_cam_qbuf(struct file *file, void *fh, struct v4l2_buffer *b)
{
	struct cmos_info *info = file->private_data;
	struct cmos_drvinfo *drv = &info->drvinfo;
	cmos_fb_t *fb;

	TRACE("Enter (index: %d)\n", b->index);
	mutex_lock(&info->lock);

	fb = &drv->fb_pool[b->index];
	put_queue(drv, fb);

	mutex_unlock(&info->lock);
	TRACE("Leave (index: %d)\n", b->index);
	return 0;
}

static int cmos_cam_dqbuf(struct file *file, void *fh, struct v4l2_buffer *b)
{
	struct cmos_info *info = file->private_data;
	struct cmos_drvinfo *drv = &info->drvinfo;
	unsigned long flags =0;
	cmos_fb_t *fb = 0;
	int ret = 0;

	TRACE("Enter (index: %d)\n", b->index);
	mutex_lock(&info->lock);

	if (drv->streamoff) {
#if 0
		struct list_head *next;

		/* CMOS sensor did not work now */
		list_for_each(next, &drv->head) {
			fb = (cmos_fb_t *)list_entry(next, cmos_fb_t, list);
			if (fb) {
				pop_queue(drv, fb);
				break;
			}
		}
#else
		ret = -EINVAL;
		pr_err("stream is off\n");
#endif
		goto EXIT_cmos_cam_dqbuf;
	}

	fb = (cmos_fb_t *)wmt_vid_get_cur_fb();
	if (!fb) {
		goto EXIT_cmos_cam_dqbuf;
	}

	spin_lock_irqsave(&cmos_lock, flags);

	DBG_MSG("Set DQBUF on\n");
	drv->dqbuf  = 1;

	spin_unlock_irqrestore(&cmos_lock, flags);

	if ((wait_event_interruptible_timeout(cmos_wait,
					      (drv->status & STS_CMOS_FB_DONE),
					      400) == 0)) {
		pr_err("CMOS Time out in 400 ms\n");
		ret = -ETIMEDOUT;
		goto EXIT_cmos_cam_dqbuf; 
	}
	drv->status &= (~STS_CMOS_FB_DONE);

	DBG_MSG("[%d] fb: 0x%p\n", fb->id, fb);
	pop_queue(drv, fb);

	b->length   = drv->frame_size;
	b->m.offset = fb->y_addr;
	b->index    = fb->id;

EXIT_cmos_cam_dqbuf:
	mutex_unlock(&info->lock);
	TRACE("Leave (index: %d)\n", b->index);
	return ret;
}

static int cmos_cam_enum_input(struct file *file, void *fh, struct v4l2_input *inp)
{
	if (inp->index >= NUM_INPUTS)
		return -EINVAL;

	inp->type = V4L2_INPUT_TYPE_CAMERA;
	inp->std = V4L2_STD_ALL;
	sprintf(inp->name, "Camera %u", inp->index);
	return 0;
}

static int cmos_cam_g_input(struct file *file, void *fh, unsigned int *i)
{
	struct cmos_info *info = file->private_data;
	TRACE("Enter\n");
	mutex_lock(&info->lock);
	*i = info->input;
	mutex_unlock(&info->lock);
	TRACE("Leave\n");
	return 0;
}

static int cmos_cam_s_input(struct file *file, void *fh, unsigned int i)
{
	struct cmos_info *info = file->private_data;
	struct cmos_dev *cd;
	struct cmos_subdev *sd;
	int rc,j;

	TRACE("Enter\n");
	mutex_lock(&info->lock);

	if (i >= info->input_count) {
		rc = -EINVAL;
		goto out;
	}

	for (j = 0; j < info->input_count; j++) {
		if(j==i)
			continue;
		cd = &info->cd[j];
		if (cd->type != CAMERA_TYPE_CMOS)
			continue;
		cmos_dev_power_down(cd);
	}
	
	info->input = i;

	cd = &info->cd[i];
	sd = cd->sd;

	cmos_dev_power_up(cd);

	if (!sd) {
		sd = cmos_subdev_get_by_try();
		if (!sd) {
			pr_err("cmos detect failed\n");
			rc = -EINVAL;
			goto err_detect;
		}
		pr_info("cmos #%d: detected %s\n", i, sd->name);
		cd->sd = sd;
	}

	rc = cmos_subdev_call(sd, init);
	if (rc) {
		goto err_init;
	}

	pr_info("[cmos] %s init success\n", sd->name);
	goto out;

err_init:
err_detect:
	cmos_dev_power_down(cd);
out:
	mutex_unlock(&info->lock);
	TRACE("Leave\n");
	return rc;
}

static int cmos_cam_queryctrl(struct file *file, void *fh, struct v4l2_queryctrl *a)
{
	struct cmos_info *info = file->private_data;
	struct cmos_subdev *sd = current_subdev(info);
	int ret;

	TRACE("Enter\n");
	mutex_lock(&info->lock);

	if (a->id == V4L2_CID_CAMERA_FLASH_MODE) {
		struct cmos_dev *cd = &info->cd[info->input];
		ret =  (cd->cap & CAMERA_CAP_FLASH) ? v4l2_ctrl_query_fill(a, 0, 4, 1, 0) : -EINVAL;
	} else 
		ret = cmos_subdev_call(sd, queryctrl, a);

	mutex_unlock(&info->lock);
	TRACE("Leave\n");
	return ret;
}

static int cmos_cam_g_ctrl(struct file *file, void *fh, struct v4l2_control *a)
{
	struct cmos_info *info = file->private_data;
	struct cmos_subdev *sd = current_subdev(info);
	int ret;

	TRACE("Enter\n");
	mutex_lock(&info->lock);
	ret = cmos_subdev_call(sd, g_ctrl, a);
	mutex_unlock(&info->lock);
	TRACE("Leave\n");
	return ret;
}

static int cmos_cam_s_ctrl(struct file *file, void *fh, struct v4l2_control *a)
{
	struct cmos_info *info = file->private_data;
	struct cmos_subdev *sd = current_subdev(info);
	char *s;
	int ret;

	TRACE("Enter\n");
	mutex_lock(&info->lock);

	/* for debug */
	switch (a->id) {
	case V4L2_CID_CAMERA_SCENE_MODE:	s = "Scene Mode";	break;
	case V4L2_CID_DO_WHITE_BALANCE:		s = "White Balance";	break;
	case V4L2_CID_EXPOSURE:			s = "Exposure";		break;
	case V4L2_CID_HFLIP:			s = "H-flip";		break;
	case V4L2_CID_VFLIP:			s = "V-flip";		break;
	case V4L2_CID_CAMERA_FLASH_MODE:	s = "Flash Mode";	break;
	case V4L2_CID_CAMERA_FOCUS_MODE:	s = "Focus Mode";	break;
	case V4L2_CID_CAMERA_FOCUS_POSITION_X:	s = "Focus Area X";	break;
	case V4L2_CID_CAMERA_FOCUS_POSITION_Y:	s = "Focus Area Y";	break;
	default:				s = "Unknown";		break;
	}
	printk(KERN_DEBUG "%s: S_CTRL: %s, value %d\n", sd->name, s, a->value);

	if (a->id == V4L2_CID_CAMERA_FLASH_MODE) {
		struct cmos_dev *cd = &info->cd[info->input];
		if (!(cd->cap & CAMERA_CAP_FLASH))
			ret = -EINVAL;
		else
			ret = flash_set_mode(info->fl, a->value);
	} else
		ret = cmos_subdev_call(sd, s_ctrl, a);

	mutex_unlock(&info->lock);
	TRACE("Leave\n");
	return ret;
}

static int cmos_cam_g_parm(struct file *file, void *fh,
		struct v4l2_streamparm *parm)
{
	struct cmos_info *info = file->private_data;
	struct cmos_subdev *sd = current_subdev(info);
	int ret;

	TRACE("Enter\n");
	mutex_lock(&info->lock);
	ret= cmos_subdev_call(sd,g_parm, parm);
	mutex_unlock(&info->lock);
	TRACE("Leave\n");
	
	return ret;
}

static int cmos_cam_s_parm(struct file *file, void *fh,
		struct v4l2_streamparm *parm)
{
	struct cmos_info *info = file->private_data;
	struct cmos_subdev *sd = current_subdev(info);
	int ret;

	TRACE("Enter\n");
	mutex_lock(&info->lock);
	ret= cmos_subdev_call(sd,s_parm, parm);
	mutex_unlock(&info->lock);
	TRACE("Leave\n");
	
	return ret;
}

static int cmos_cam_streamon(struct file *file, void *fh, enum v4l2_buf_type i)
{
	struct cmos_info *info = file->private_data;
	struct cmos_drvinfo *drv = &info->drvinfo;
	cmos_fb_t *fb;
	int ret = 0;

	TRACE("Enter\n");
	mutex_lock(&info->lock);

	if (!list_empty(&drv->head)) {
		fb = list_first_entry(&drv->head, cmos_fb_t, list);
		wmt_vid_set_cur_fb((vid_fb_t *)fb);
		cam_enable(drv, true);
	} else
		ret = -EINVAL;

	mutex_unlock(&info->lock);
	TRACE("Leave\n");
	return 0;
}

static int cmos_cam_streamoff(struct file *file, void *fh, enum v4l2_buf_type i)
{
	struct cmos_info *info = file->private_data;
	struct cmos_drvinfo *drv = &info->drvinfo;

	TRACE("Enter\n");
	mutex_lock(&info->lock);

	cam_enable(drv, false);

	mutex_unlock(&info->lock);
	TRACE("Leave\n");
	return 0;
}

static int cmos_cam_mmap(struct file *file, struct vm_area_struct *vma)
{
	int ret;

	TRACE("Enter\n");

	vma->vm_flags |= VM_IO | VM_RESERVED;
	vma->vm_page_prot = pgprot_writecombine(vma->vm_page_prot);

	ret = remap_pfn_range(vma, vma->vm_start, vma->vm_pgoff,
			      vma->vm_end - vma->vm_start, vma->vm_page_prot);

	TRACE("Leave\n");
	return ret;
}

static irqreturn_t cmos_isr(int irq, void *data)
{
	struct cmos_info *info = data;
	struct cmos_drvinfo *drv;
	struct list_head *next;
	cmos_fb_t *cur_fb, *fb;
	unsigned long flags = 0;
	int mb_count = 0;

	//TRACE("Enter\n");
//	printk("i");

	if (!info) {
		return IRQ_NONE;
	}

	drv = &info->drvinfo;

	if (drv->dqbuf == 1) {
		spin_lock_irqsave(&cmos_lock, flags);
		drv->dqbuf = 0;
//		DBG_MSG("Set DBBUF off\n");
		spin_unlock_irqrestore(&cmos_lock, flags);

		cur_fb = (cmos_fb_t *)wmt_vid_get_cur_fb();
		list_for_each(next, &drv->head) {
			fb = (cmos_fb_t *) list_entry(next, cmos_fb_t, list);
			if (fb == cur_fb) {
				/*--------------------------------------------------------------
				  Get next FB
				  --------------------------------------------------------------*/
				fb = (cmos_fb_t *) list_entry(cur_fb->list.next, cmos_fb_t, list);

				do {
					if (((vid_fb_t *)fb)->y_addr != 0)
						mb_count = mb_counter(((vid_fb_t *)fb)->y_addr);

					if (mb_count != 1) {
						fb = (cmos_fb_t *) list_entry(fb->list.next, cmos_fb_t, list);
//						DBG_MSG("mbcount skip 0x%08x mb_count %d \n",
//							((vid_fb_t *)fb)->y_addr,mb_count);
						continue;
					}
				} while ((mb_count != 1) && (fb != cur_fb));

				cur_fb->is_busy = 0;
				cur_fb->done    = 1;

//				DBG_MSG("[%d] done: %d, is_busy: %d\n",
//					cur_fb->id, cur_fb->done, cur_fb->is_busy);
//				DBG_MSG("[%d] New FB done: %d, is_busy: %d\n",
//					fb->id, fb->done, fb->is_busy);

				wmt_vid_set_cur_fb((vid_fb_t *)fb);
				drv->status |= STS_CMOS_FB_DONE;
				wake_up_interruptible(&cmos_wait);
				break;
			}
		}
	} /* if( drv->dqbuf == 1 ) */

	CMOS_REG_SET32(REG_VID_INT_CTRL, REG32_VAL(REG_VID_INT_CTRL));

	//TRACE("Leave\n");
//	printk("o");

	return IRQ_HANDLED;
}

static int cmos_cam_open(struct file *filp)
{
	struct cmos_info *info = video_drvdata(filp);
	struct cmos_drvinfo *drv;
	int ret = 0;

	printk(KERN_DEBUG "%s()\n", __func__);
	if(cam_vccvid)
		regulator_enable(cam_vccvid);
	
	if(cam_dvdd)
		regulator_enable(cam_dvdd);

	TRACE("Enter\n");
	mutex_lock(&info->lock);
	if (cmos_dev_ref) {
		ret = -EBUSY;
		goto out;
	}
	cmos_dev_ref++;

	ret = request_irq(WMT_VID_IRQ, (void *)&cmos_isr, IRQF_SHARED,
			  "wmt-cmos", (void *)info);
	if (ret) {
		pr_err("CMOS: Failed to register CMOS irq %i\n", WMT_VID_IRQ);
		cmos_dev_ref--;
		goto out;
	}

	drv = &info->drvinfo;
	drv->dqbuf = 0;
	drv->status = STS_CMOS_READY;
	INIT_LIST_HEAD(&drv->head);

	spin_lock_init(&cmos_lock);

	filp->private_data = info;

	wmt_vid_open(VID_MODE_CMOS, NULL);
	cam_enable(drv, false);

out:
	mutex_unlock(&info->lock);
	TRACE("Leave\n");
	return ret;
}

static int cmos_cam_release(struct file *filp)
{
	struct cmos_info *info = filp->private_data;
	struct cmos_drvinfo *drv = &info->drvinfo;
	struct cmos_subdev *sd;
	int i;

	TRACE("Enter\n");
	mutex_lock(&info->lock);

	free_irq(WMT_VID_IRQ, (void *)info);

	if (drv) {
		/* Free memory */
		for (i = 0; i < drv->fb_cnt; i++) {
			cmos_fb_t *fb = &drv->fb_pool[i];
			if (fb->y_addr) {
				mb_free(fb->y_addr);
				memset(fb, 0, sizeof(cmos_fb_t));
			}
		}
	}

	sd = current_subdev(info);
	if (sd) {
		cmos_subdev_call(sd, exit);
	}
	
	cmos_dev_power_down(&info->cd[info->input]);
	wmt_vid_close(VID_MODE_CMOS);
	cmos_dev_ref--;

//	if(cam_vccvid)
//		regulator_disable(cam_vccvid);
	
	if(cam_dvdd)
		regulator_enable(cam_dvdd);
	
	mutex_unlock(&info->lock);
	TRACE("Leave\n");
	return 0;
}

static const struct v4l2_ioctl_ops cmos_ioctl_ops = {
	.vidioc_querycap    		= cmos_cam_querycap,
	.vidioc_enum_input		= cmos_cam_enum_input,
	.vidioc_g_input			= cmos_cam_g_input,
	.vidioc_s_input			= cmos_cam_s_input,
	.vidioc_reqbufs			= cmos_cam_reqbufs,
	.vidioc_querybuf		= cmos_cam_querybuf,
	.vidioc_qbuf			= cmos_cam_qbuf,
	.vidioc_dqbuf			= cmos_cam_dqbuf,
	.vidioc_streamon		= cmos_cam_streamon,
	.vidioc_streamoff		= cmos_cam_streamoff,
	.vidioc_enum_fmt_vid_cap	= cmos_cam_enum_fmt_cap,
	.vidioc_enum_framesizes		= cmos_cam_enum_framesizes,
	.vidioc_g_fmt_vid_cap		= cmos_cam_g_fmt_cap,
	.vidioc_s_fmt_vid_cap  		= cmos_cam_s_fmt_cap,
	.vidioc_try_fmt_vid_cap  	= cmos_cam_try_fmt_cap,
	.vidioc_g_parm		= cmos_cam_g_parm,
	.vidioc_s_parm		= cmos_cam_s_parm,
	.vidioc_queryctrl 		= cmos_cam_queryctrl,
	.vidioc_s_ctrl       		= cmos_cam_s_ctrl,
	.vidioc_g_ctrl       		= cmos_cam_g_ctrl,
	.vidioc_cropcap			= cmos_cam_cropcap,
};

static const struct v4l2_file_operations cmos_cam_fops = {
	.owner		= THIS_MODULE,
	.open		= cmos_cam_open,
	.release	= cmos_cam_release,
	.unlocked_ioctl = video_ioctl2,
	.mmap		= cmos_cam_mmap,
};

static struct video_device cmos_cam = {
	.name		= "cmos_cam",
	.fops		= &cmos_cam_fops,
	.ioctl_ops	= &cmos_ioctl_ops,
	.release	= video_device_release_empty,
	.tvnorms	= V4L2_STD_NTSC | V4L2_STD_PAL | V4L2_STD_SECAM,
	.minor		= -1
};

static int cmos_probe(struct platform_device *pdev)
{
	int ret;
	struct cmos_info *info;

	info = kzalloc(sizeof *info, GFP_KERNEL);
	if (!info)
		return -ENOMEM;

    // ldo6 can't disable.
	cam_vccvid = regulator_get(NULL, "ldo6");
	if (IS_ERR(cam_vccvid)){
		cam_vccvid = NULL;
		pr_err("failed to get camera vccvid\n");
	}

	cam_dvdd= regulator_get(NULL, "ldo5");
	if (IS_ERR(cam_dvdd)){
		cam_dvdd = NULL;
		pr_err("failed to get camera dvdd\n");
	}

	if(cam_vccvid){
		regulator_set_voltage(cam_vccvid, 2800000, 2800000);
		regulator_enable(cam_vccvid);
//		regulator_disable(cam_vccvid);
	}
	
	if(cam_dvdd){
		regulator_set_voltage(cam_dvdd, 1800000, 1800000);
		regulator_enable(cam_dvdd);
		regulator_disable(cam_dvdd);
	}

	ret = parse_camera_param(info);
	if (ret)
		goto err_param;

	ret = cmos_dev_setup(info);
	if (ret)
		goto err_dev_seup;

	ret = video_register_device(&cmos_cam, VFL_TYPE_GRABBER, -1);
	if (ret) {
		pr_err("WonderMedia CMOS camera register failed\n");
		goto err_video_register;
	}

	mutex_init(&info->lock);
	video_set_drvdata(&cmos_cam, info);

	info->fl = flash_instantiation();

	pr_info("WonderMedia CMOS camera register OK \n");
	return 0;

err_video_register:
err_dev_seup:
err_param:
	kfree(info);
	return ret;
}

static int cmos_remove(struct platform_device *pdev)
{
	struct cmos_info *info = video_get_drvdata(&cmos_cam);

	pr_info("%s()\n", __func__);

	flash_destroy(info->fl);
	video_unregister_device(&cmos_cam);
	cmos_dev_release(info);
	mutex_destroy(&info->lock);
	kfree(info);

//	if(cam_vccvid)
//		regulator_put(cam_vccvid);
	
	if(cam_dvdd)
		regulator_put(cam_dvdd);
	return 0;
}

static int cmos_suspend(struct platform_device *pdev, pm_message_t state)
{
	printk(KERN_DEBUG "%s()\n", __func__);
	return 0;
}

static int cmos_resume(struct platform_device *pdev)
{
	struct cmos_info *info = video_get_drvdata(&cmos_cam);
	struct cmos_dev *cd;
	int i;

	printk(KERN_DEBUG "%s()\n", __func__);

	for (i = 0; i < info->input_count; i++) {
		cd = &info->cd[i];
		if (cd->type != CAMERA_TYPE_CMOS)
			continue;
		gpio_re_enabled(cd->pwdn_gpio);
	}

	return 0;
}

static struct platform_driver cmos_driver = {
	.driver = {
		.name = "cmos",
	},
	.remove = cmos_remove,
	.suspend = cmos_suspend,
	.resume = cmos_resume,
};

static void cmos_platform_release(struct device *device) { }

static struct platform_device cmos_device = {
	.name = "cmos",
	.dev = {
		.release = cmos_platform_release,
	},
};

static int __init cmos_init(void)
{
	int ret;

	cmos_subdev_init();

	ret = wmt_vid_i2c_init(0, 0x52);
	if (ret) {
		goto err_i2c_init;
	}

	ret = platform_device_register(&cmos_device);
	if (ret) {
		goto err_dev_register;
	}

	ret = platform_driver_probe(&cmos_driver, cmos_probe);
	if (ret) {
		goto err_drv_probe;
	}

	return 0;

err_drv_probe:
	platform_device_unregister(&cmos_device);
err_dev_register:
	wmt_vid_i2c_release();
err_i2c_init:
	cmos_subdev_exit();
	return ret;
}

static void __exit cmos_exit(void)
{
	pr_info("%s()\n", __func__);

	platform_driver_unregister(&cmos_driver);
	platform_device_unregister(&cmos_device);
	wmt_vid_i2c_release();
	cmos_subdev_exit();
}

module_init(cmos_init);
module_exit(cmos_exit);

MODULE_AUTHOR("WonderMedia SW Team Max Chen");
MODULE_DESCRIPTION("cmos device driver");
MODULE_LICENSE("GPL");
