/*++
	linux/drivers/media/video/wmt_v4l2/sensors/gc0307/gc0307.c

	Copyright (c) 2013  WonderMedia Technologies, Inc.

	This program is free software: you can redistribute it and/or modify it under the
	terms of the GNU General Public License as published by the Free Software Foundation,
	either version 2 of the License, or (at your option) any later version.

	This program is distributed in the hope that it will be useful, but WITHOUT
	ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
	PARTICULAR PURPOSE.  See the GNU General Public License for more details.
	You should have received a copy of the GNU General Public License along with
	this program.  If not, see <http://www.gnu.org/licenses/>.

	WonderMedia Technologies, Inc.
	10F, 529, Chung-Cheng Road, Hsin-Tien, Taipei 231, R.O.C.
--*/

#include "../cmos-subdev.h"
#include "../../wmt-vid.h"
#include "gc0307.h"

#define sensor_write_array(sd, arr, sz) \
	cmos_init_8bit_addr(arr, sz, (sd)->i2c_addr)

#define sensor_read(sd, reg) \
	wmt_vid_i2c_read(sd->i2c_addr, reg)

#define sensor_write(sd, reg, val) \
	wmt_vid_i2c_write(sd->i2c_addr, reg, val)

struct cmos_win_size {
	char		*name;
	int		width;
	int		height;
	uint8_t 	*regs;
	size_t		size;
};

struct sensor_flip{
	int h_flip;
	int v_flip;
	int cnt;
}gc0307_flip;

#define CMOS_WIN_SIZE(n, w, h, r) \
	{.name = n, .width = w , .height = h, .regs = r, .size = ARRAY_SIZE(r) }


static const struct cmos_win_size cmos_supported_win_sizes[] = {
//	CMOS_WIN_SIZE("QVGA", 320, 240, gc0307_320x240),
	CMOS_WIN_SIZE("VGA",  640, 480, gc0307_640x480),
};

static const struct cmos_win_size *cmos_select_win(u32 *width, u32 *height)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(cmos_supported_win_sizes); i++) {
		if (cmos_supported_win_sizes[i].width == *width &&
		    cmos_supported_win_sizes[i].height == *height) {
			*width = cmos_supported_win_sizes[i].width;
			*height = cmos_supported_win_sizes[i].height;
			return &cmos_supported_win_sizes[i];
		}
	}
	return NULL;
}

static int sensor_s_wb(struct cmos_subdev *sd, enum v4l2_wb_mode value)
{
	uint8_t *regs,data;
	
	size_t size;
	
	switch (value) {
	case WHITE_BALANCE_AUTO:
		sensor_write(sd, 0xf0, 0x00);
		data = sensor_read(sd, 0x41);
		data |=0x04;
		sensor_write(sd, 0x41, data);
		regs = gc0307_wb_auto;
		size = ARRAY_SIZE(gc0307_wb_auto);
		break;
	case WHITE_BALANCE_INCANDESCENCE:
		sensor_write(sd, 0xf0, 0x00);
		data = sensor_read(sd, 0x41);
		data &=0xfb;
		sensor_write(sd, 0x41, data);
		regs = gc0307_wb_incandescent;
		size = ARRAY_SIZE(gc0307_wb_incandescent);
		break;
	case WHITE_BALANCE_DAYLIGHT:
		sensor_write(sd, 0xf0, 0x00);
		data = sensor_read(sd, 0x41);
		data &=0xfb;
		sensor_write(sd, 0x41, data);
		regs = gc0307_wb_daylight;
		size = ARRAY_SIZE(gc0307_wb_daylight);
		break;
	case WHITE_BALANCE_CLOUDY:
		sensor_write(sd, 0xf0, 0x00);
		data = sensor_read(sd, 0x41);
		data &=0xfb;
		sensor_write(sd, 0x41, data);
		regs = gc0307_wb_cloudy;
		size = ARRAY_SIZE(gc0307_wb_cloudy);
		break;
	case WHITE_BALANCE_FLUORESCENT:
		sensor_write(sd, 0xf0, 0x00);
		data = sensor_read(sd, 0x41);
		data &=0xfb;
		sensor_write(sd, 0x41, data);
		regs = gc0307_wb_fluorescent;
		size = ARRAY_SIZE(gc0307_wb_fluorescent);
		break;
	default:
		return -EINVAL;
	}

	sensor_write_array(sd, regs, size);
	mdelay(80);
	return 0;
}

static int sensor_s_scenemode(struct cmos_subdev *sd, enum v4l2_scene_mode value)
{
	uint8_t *regs;
	size_t size;

	switch (value) {
	case SCENE_MODE_AUTO:
		regs = gc0307_scene_mode_auto;
		size = ARRAY_SIZE(gc0307_scene_mode_auto);
		break;
	case SCENE_MODE_NIGHTSHOT:
		regs = gc0307_scene_mode_night;
		size = ARRAY_SIZE(gc0307_scene_mode_night);
		break;
	default:
		return -EINVAL;
	}

	sensor_write_array(sd, regs, size);
	return 0;
}

static int sensor_s_exposure(struct cmos_subdev *sd, int value)
{
	uint8_t *regs;
	size_t size;

	switch (value) {
	case -2:
		regs = gc0307_exposure_neg6;
		size = ARRAY_SIZE(gc0307_exposure_neg6);
		break;
	case -1:
		regs = gc0307_exposure_neg3;
		size = ARRAY_SIZE(gc0307_exposure_neg3);
		break;
	case 0:
		regs = gc0307_exposure_zero;
		size = ARRAY_SIZE(gc0307_exposure_zero);
		break;
	case 1:
		regs = gc0307_exposure_pos3;
		size = ARRAY_SIZE(gc0307_exposure_pos3);
		break;
	case 2:
		regs = gc0307_exposure_pos6;
		size = ARRAY_SIZE(gc0307_exposure_pos6);
		break;
	default:
		return -EINVAL;
	}

	sensor_write_array(sd, regs, size);
	return 0;
}

static int sensor_do_flip(struct cmos_subdev *sd)
{
	if ((gc0307_flip.h_flip == 0) &&(gc0307_flip.v_flip == 0)){
		sensor_write( sd, 0x0f, 0xb2);
		sensor_write( sd, 0x45, 0x27);
		sensor_write( sd, 0x47, 0x2c);
	}else if ((gc0307_flip.h_flip == 0) &&(gc0307_flip.v_flip == 1)){
		sensor_write( sd, 0x0f, 0xa2);
		sensor_write( sd, 0x45, 0x26);
		sensor_write( sd, 0x47, 0x28);
	}else if ((gc0307_flip.h_flip == 1) &&(gc0307_flip.v_flip == 0)){
		sensor_write( sd, 0x0f, 0x92);
		sensor_write( sd, 0x45, 0x25);
		sensor_write( sd, 0x47, 0x24);
	}else if ((gc0307_flip.h_flip == 1) &&(gc0307_flip.v_flip == 1)){
		sensor_write( sd, 0x0f, 0x82);
		sensor_write( sd, 0x45, 0x24);
		sensor_write( sd, 0x47, 0x20);
	}

	msleep(100);
	return 0;
}


static int sensor_s_hflip(struct cmos_subdev *sd, int value)
{
	gc0307_flip.cnt ++;
	
	switch (value) {
	case 0:
		gc0307_flip.h_flip=0;
		break;
	case 1:
		gc0307_flip.h_flip=1;
		break;
	default:
		return -EINVAL;
	}
	if(gc0307_flip.cnt==2)
		sensor_do_flip(sd);
	
	return 0;
}

static int sensor_s_vflip(struct cmos_subdev *sd, int value)
{
	gc0307_flip.cnt ++;
	
	switch (value) {
	case 0:
		gc0307_flip.v_flip=0;
		break;
	case 1:
		gc0307_flip.v_flip=1;
		break;
	default:
		return -EINVAL;
	}
	if(gc0307_flip.cnt==2)
		sensor_do_flip(sd);
	
	return 0;
}

static int sensor_queryctrl(struct cmos_subdev *sd, struct v4l2_queryctrl *qc)
{
	switch (qc->id) {
	case V4L2_CID_VFLIP:
	case V4L2_CID_HFLIP:
		return v4l2_ctrl_query_fill(qc, 0, 1, 1, 0);
	case V4L2_CID_CAMERA_SCENE_MODE:
		return v4l2_ctrl_query_fill(qc, 0, 1, 1, 0);
	case V4L2_CID_DO_WHITE_BALANCE:
		return v4l2_ctrl_query_fill(qc, 0, 3, 1, 0);
	case V4L2_CID_EXPOSURE:
		return v4l2_ctrl_query_fill(qc, -2, 2, 1, 0);
	}
	return -EINVAL;
}

static int sensor_s_ctrl(struct cmos_subdev *sd, struct v4l2_control *ctrl)
{
	switch (ctrl->id) {
	case V4L2_CID_CAMERA_SCENE_MODE:
		return sensor_s_scenemode(sd, ctrl->value);
	case V4L2_CID_DO_WHITE_BALANCE:	
		return sensor_s_wb(sd, ctrl->value);	
	case V4L2_CID_EXPOSURE:
		return sensor_s_exposure(sd, ctrl->value);	
	case V4L2_CID_HFLIP:
		return sensor_s_hflip(sd, ctrl->value);
	case V4L2_CID_VFLIP:
		return sensor_s_vflip(sd, ctrl->value);
	default:
	case WMT_V4L2_CID_CAMERA_ANTIBANDING:
		return -EINVAL;
	}
	return -EINVAL;
}

static int sensor_g_mbus_fmt(struct cmos_subdev *sd,
			     struct v4l2_mbus_framefmt *mf)
{
	return -EINVAL;
}

static int sensor_s_mbus_fmt(struct cmos_subdev *sd,
			     struct v4l2_mbus_framefmt *mf)
{
	const struct cmos_win_size *win;

	win = cmos_select_win(&mf->width, &mf->height);
	if (!win) {
		pr_err("%s, s_mbus_fmt failed, width %d, height %d\n", 
		       sd->name, mf->width, mf->height);
		return -EINVAL;
	}

	sensor_write_array(sd, win->regs, win->size);
	return 0;
}

static int sensor_try_mbus_fmt(struct cmos_subdev *sd,
			       struct v4l2_mbus_framefmt *mf)
{
	return 0;
}

static int sensor_enum_framesizes(struct cmos_subdev *sd,
		struct v4l2_frmsizeenum *fsize)
{
	int i;
	int num_valid = -1;
	__u32 index = fsize->index;

	for (i = 0; i < ARRAY_SIZE(cmos_supported_win_sizes); i++) {
		const struct cmos_win_size *win = &cmos_supported_win_sizes[index];
		if (index == ++num_valid) {
			fsize->type = V4L2_FRMSIZE_TYPE_DISCRETE;
			fsize->discrete.width = win->width;
			fsize->discrete.height = win->height;
			return 0;
		}
	}

	return -EINVAL;
}

static int sensor_identify(struct cmos_subdev *sd)
{
	return (sensor_read(sd, 0) == sd->id) ? 0 : -EINVAL;
}

static int sensor_init(struct cmos_subdev *sd)
{
	if (sensor_identify(sd)) {
		return -1;
	}
	gc0307_flip.cnt=0;
	sensor_write_array(sd, gc0307_default_regs_init,
			   ARRAY_SIZE(gc0307_default_regs_init));
	return 0;
}

static int sensor_exit(struct cmos_subdev *sd)
{
	return 0;
}

static struct cmos_subdev_ops gc0307_ops = {
	.identify	= sensor_identify,
	.init		= sensor_init,
	.exit		= sensor_exit,
	.queryctrl	= sensor_queryctrl,
	.s_ctrl		= sensor_s_ctrl,
	.s_mbus_fmt     = sensor_s_mbus_fmt,
	.g_mbus_fmt     = sensor_g_mbus_fmt,
	.try_mbus_fmt	= sensor_try_mbus_fmt,
	.enum_framesizes = sensor_enum_framesizes,
};

struct cmos_subdev gc0307 = {
	.name		= "gc0307",
	.i2c_addr	= 0x21,
	.id		= 0x99,
	.max_width	= 640,
	.max_height	= 480,
	.ops		= &gc0307_ops,
};

#if 0
static int __init gc0307_init(void)
{
	return cmos_register_sudbdev(&gc0307);
}

static void __exit gc0307_exit(void)
{
	return cmos_unregister_subdev(&gc0307);
}

module_init(gc0307_init);
module_exit(gc0307_exit);

MODULE_LICENSE("GPL");
#endif
