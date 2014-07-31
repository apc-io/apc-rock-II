/*++
	linux/drivers/media/video/wmt_v4l2/sensors/gc0311/gc0311.c

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
#include "gc0311.h"

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

#define CMOS_WIN_SIZE(n, w, h, r) \
	{.name = n, .width = w , .height = h, .regs = r, .size = ARRAY_SIZE(r) }

static const struct cmos_win_size cmos_supported_win_sizes[] = {
	CMOS_WIN_SIZE("QVGA", 320, 240, gc0311_320x240),
	CMOS_WIN_SIZE("VGA",  640, 480, gc0311_640x480),
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

static int sensor_s_hflip(struct cmos_subdev *sd, int value)
{
	int data;

	data = sensor_read(sd, 0x14);

	switch (value) {
	case 0:
		data &= ~0x01;
		break;
	case 1:
		data |= 0x01;
		break;
	default:
		return -EINVAL;
	}

	return sensor_write(sd, 0x14, data);
}

static int sensor_s_vflip(struct cmos_subdev *sd, int value)
{
	int data;

	data = sensor_read(sd, 0x14);

	switch (value) {
	case 0:
		data &= ~0x02;
		break;
	case 1:
		data |= 0x02;
		break;
	default:
		return -EINVAL;
	}

	return sensor_write(sd, 0x14, data);
}

static int sensor_s_wb(struct cmos_subdev *sd, enum v4l2_wb_mode value)
{
	uint8_t *regs;
	size_t size;

	switch (value) {
	case WHITE_BALANCE_AUTO:
		regs = gc0311_wb_auto;
		size = ARRAY_SIZE(gc0311_wb_auto);
		break;
	case WHITE_BALANCE_INCANDESCENCE:
		regs = gc0311_wb_incandescent;
		size = ARRAY_SIZE(gc0311_wb_incandescent);
		break;
	case WHITE_BALANCE_DAYLIGHT:
		regs = gc0311_wb_daylight;
		size = ARRAY_SIZE(gc0311_wb_daylight);
		break;
	case WHITE_BALANCE_CLOUDY:
		regs = gc0311_wb_cloudy;
		size = ARRAY_SIZE(gc0311_wb_cloudy);
		break;
	case WHITE_BALANCE_FLUORESCENT:
		regs = gc0311_wb_fluorescent;
		size = ARRAY_SIZE(gc0311_wb_fluorescent);
		break;
	default:
		return -EINVAL;
	}

	sensor_write_array(sd, regs, size);
	return 0;
}

static int sensor_queryctrl(struct cmos_subdev *sd, struct v4l2_queryctrl *qc)
{
	switch (qc->id) {
	case V4L2_CID_VFLIP:
	case V4L2_CID_HFLIP:
		return v4l2_ctrl_query_fill(qc, 0, 1, 1, 0);
	case V4L2_CID_DO_WHITE_BALANCE:
		return v4l2_ctrl_query_fill(qc, 0, 3, 1, 0);
	}
	return -EINVAL;
}

static int sensor_s_ctrl(struct cmos_subdev *sd, struct v4l2_control *ctrl)
{
	switch (ctrl->id) {
	case V4L2_CID_VFLIP:
		return sensor_s_vflip(sd, ctrl->value);
	case V4L2_CID_HFLIP:
		return sensor_s_hflip(sd, ctrl->value);
	case V4L2_CID_DO_WHITE_BALANCE:
		return sensor_s_wb(sd, ctrl->value);	
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
	if (!win)
		return -EINVAL;

	sensor_write_array(sd, win->regs, win->size);
	return 0;
}

static int sensor_try_mbus_fmt(struct cmos_subdev *sd,
			       struct v4l2_mbus_framefmt *mf)
{
	return 0;
}

static int sensor_identify(struct cmos_subdev *sd)
{
	return (sensor_read(sd, 0) == sd->id) ? 0 : -EINVAL;
}

static int sensor_init(struct cmos_subdev *sd)
{
	if (!sensor_identify(sd)) {
		return -1;
	}

	sensor_write_array(sd, gc0311_default_regs_init,
			   ARRAY_SIZE(gc0311_default_regs_init));
	return 0;
}

static int sensor_exit(struct cmos_subdev *sd)
{
	return 0;
}

static struct cmos_subdev_ops gc0311_ops = {
	.identify	= sensor_identify,
	.init		= sensor_init,
	.exit		= sensor_exit,
	.queryctrl	= sensor_queryctrl,
	.s_ctrl		= sensor_s_ctrl,
	.s_mbus_fmt     = sensor_s_mbus_fmt,
	.g_mbus_fmt     = sensor_g_mbus_fmt,
	.try_mbus_fmt	= sensor_try_mbus_fmt,
};

struct cmos_subdev gc0311 = {
	.name		= "gc0311",
	.i2c_addr	= 0x21,
	.id		= 0xbb,
	.max_width	= 640,
	.max_height	= 480,
	.ops		= &gc0311_ops,
};

static int __init gc0311_init(void)
{
	return cmos_register_sudbdev(&gc0311);
}

static void __exit gc0311_exit(void)
{
	return cmos_unregister_subdev(&gc0311);
}

module_init(gc0311_init);
module_exit(gc0311_exit);

MODULE_LICENSE("GPL");
