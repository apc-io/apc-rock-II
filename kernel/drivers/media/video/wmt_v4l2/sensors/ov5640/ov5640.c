/*++
	linux/drivers/media/video/wmt_v4l2/sensors/ov5640/ov5640.c

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

#include <linux/gpio.h>
#include <asm/errno.h>
#include "../cmos-subdev.h"
#include "../../wmt-vid.h"
#include "ov5640.h"

#define sensor_write_array(sd, arr, sz) \
	cmos_init_16bit_addr(arr, sz, (sd)->i2c_addr)

#define sensor_read(sd, reg) \
	wmt_vid_i2c_read16addr(sd->i2c_addr, reg)

#define sensor_write(sd, reg, val) \
	wmt_vid_i2c_write16addr(sd->i2c_addr, reg, val)

struct cmos_win_size {
	char		*name;
	int		width;
	int		height;
	uint32_t 	*regs;
	size_t		size;
};

#define CMOS_WIN_SIZE(n, w, h, r) \
	{.name = n, .width = w , .height = h, .regs = r, .size = ARRAY_SIZE(r) }

struct ov5640_af {
	int pos_x;
	int pos_y;
};

static struct ov5640_af ov5640_af;
static int sensor_fps;

static const struct cmos_win_size cmos_supported_win_sizes[] = {
	CMOS_WIN_SIZE("QVGA",   320,  240, ov5640_320_240_regs),
	CMOS_WIN_SIZE("VGA",    640,  480, ov5640_640_480_regs),
	CMOS_WIN_SIZE("SVGA",   800,  600, ov5640_800_600_regs),
	CMOS_WIN_SIZE("720p",  1280,  720, ov5640_1280_720_regs),
	CMOS_WIN_SIZE("200w",  1600, 1200, ov5640_1600_1200_regs),
	CMOS_WIN_SIZE("300w",  2048, 1536, ov5640_2048_1536_regs),
	CMOS_WIN_SIZE("QSXGA", 2592, 1944, ov5640_2592_1944_regs),
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
	uint32_t *regs;
	size_t size;

	switch (value) {
	case WHITE_BALANCE_AUTO:
		regs = ov5640_wb_auto;
		size = ARRAY_SIZE(ov5640_wb_auto);
		break;
	case WHITE_BALANCE_INCANDESCENCE:
		regs = ov5640_wb_incandescent;
		size = ARRAY_SIZE(ov5640_wb_incandescent);
		break;
	case WHITE_BALANCE_DAYLIGHT:
		regs = ov5640_wb_daylight;
		size = ARRAY_SIZE(ov5640_wb_daylight);
		break;
	case WHITE_BALANCE_CLOUDY:
		regs = ov5640_wb_cloudy;
		size = ARRAY_SIZE(ov5640_wb_cloudy);
		break;
	case WHITE_BALANCE_FLUORESCENT:
		regs = ov5640_wb_fluorescent;
		size = ARRAY_SIZE(ov5640_wb_fluorescent);
		break;
	default:
		return -EINVAL;
	}

	sensor_write_array(sd, regs, size);
	return 0;
}

static int sensor_s_scenemode(struct cmos_subdev *sd, enum v4l2_scene_mode value)
{
	uint32_t *regs;
	size_t size;

	switch (value) {
	case SCENE_MODE_AUTO:
		regs = ov5640_scene_mode_auto;
		size = ARRAY_SIZE(ov5640_scene_mode_auto);
		break;
	case SCENE_MODE_NIGHTSHOT:
		regs = ov5640_scene_mode_night;
		size = ARRAY_SIZE(ov5640_scene_mode_night);
		break;
	default:
		return -EINVAL;
	}

	sensor_write_array(sd, regs, size);
	return 0;
}

static int sensor_s_autoexposure(struct cmos_subdev *sd, int value)
{
	switch (value) {
	case 0:
		break;
	case 1:
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static int sensor_s_exposure(struct cmos_subdev *sd, enum v4l2_exposure_mode value)
{
	uint32_t *regs;
	size_t size;

	switch (value) {
	case -2:
		regs = ov5640_exposure_neg6;
		size = ARRAY_SIZE(ov5640_exposure_neg6);
		break;
	case -1:
		regs = ov5640_exposure_neg3;
		size = ARRAY_SIZE(ov5640_exposure_neg3);
		break;
	case 0:
		regs = ov5640_exposure_zero;
		size = ARRAY_SIZE(ov5640_exposure_zero);
		break;
	case 1:
		regs = ov5640_exposure_pos3;
		size = ARRAY_SIZE(ov5640_exposure_pos3);
		break;
	case 2:
		regs = ov5640_exposure_pos6;
		size = ARRAY_SIZE(ov5640_exposure_pos6);
		break;
	default:
		return -EINVAL;
	}

	sensor_write_array(sd, regs, size);
	return 0;
}

static int sensor_s_hflip(struct cmos_subdev *sd, int value)
{
	uint32_t data = sensor_read(sd, 0x3821);

	switch (value) {
	case 0:
		data &= ~0x06;
		break;
	case 1:
		data |= 0x06;
		break;
	default:
		return -EINVAL;
	}

	return sensor_write(sd, 0x3821, data);
}

static int sensor_s_vflip(struct cmos_subdev *sd, int value)
{
	uint32_t data = sensor_read(sd, 0x3820);

	switch (value) {
	case 0:
		data &= ~0x06;
		break;
	case 1:
		data |= 0x06;
		break;
	default:
		return -EINVAL;
	}

	return sensor_write(sd, 0x3820, data);
}

static inline int sensor_s_af_area_x(struct cmos_subdev *sd, int value)
{
	ov5640_af.pos_x = value;
	return 0;
}

static inline int sensor_s_af_area_y(struct cmos_subdev *sd, int value)
{
	ov5640_af.pos_y = value;
	return 0;
}

// auto focus
static int sensor_s_af(struct cmos_subdev *sd, int value)
{
	printk(KERN_DEBUG "%s, AF mode %d, (%d,%d)\n", sd->name, value, ov5640_af.pos_x, ov5640_af.pos_y);
	switch (value) {
	case FOCUS_MODE_AUTO:
		sensor_write(sd, 0x3024, ov5640_af.pos_x/8);
		sensor_write(sd, 0x3025, ov5640_af.pos_y/8);
		sensor_write(sd, 0x3026, 80);
		sensor_write(sd, 0x3027, 60);
		sensor_write(sd, 0x3023, 0x01);
		sensor_write(sd, 0x3022, 0x81);
		msleep(10);
		return sensor_write(sd, 0x3022, 0x03); // Single AF
	case FOCUS_MODE_CONTINUOUS:
		return sensor_write(sd, 0x3022, 0x04); // Continous AF
	case FOCUS_MODE_FIXED:
		return sensor_write(sd, 0x3022, 0x06); // Stop AF
	case FOCUS_MODE_INFINITY:
		return sensor_write(sd, 0x3022, 0x08); // Release AF
	case FOCUS_MODE_CONTINUOUS_VIDEO:
		return sensor_write(sd, 0x3022, 0x08); // Release AF
	default:
		return -EINVAL;
	}
	return 0;
}

static int sensor_g_af(struct cmos_subdev *sd, int *value)
{
	int state;

	state = sensor_read(sd, 0x3029);
	switch (state) {
	case 0x00:     /* focusing */
		*value = FOCUS_RESULT_FOCUSING;
		break;
	case 0x10:
	case 0x20:
		*value = FOCUS_RESULT_SUCCEED;
		break;
	case 0x70:     /* focused, moto released */
	default:
		*value = FOCUS_RESULT_FAILED;
		break;
	}
	return 0;
}

// shutter
static int sensor_s_shutter(struct cmos_subdev *sd)
{
        uint32_t temp;
	uint16_t temp1;
	uint16_t temp2;
	uint16_t temp3;

	sensor_write(sd, 0x3503,0x07); // 0x03  fix AE
	sensor_write(sd, 0x3a00,0x38); // disable night

	temp1 = sensor_read(sd, 0x3500);
	temp2 = sensor_read(sd, 0x3501);
	temp3 = sensor_read(sd, 0x3502);

	pr_debug("%s: 0x%x, 0x%x, 0x%x\n", __func__, temp1, temp2, temp3);

	temp = ((temp1<<12) | (temp2<<4) | (temp3>>4));
	temp = temp / 2;//这个系数根据preview mode和capture mode的frame rate进行修改
	temp = temp * 16;

	temp1 = (temp & 0x0000ff);
	temp2 = (temp & 0x00ff00) >> 8;
	temp3 = (temp & 0xff0000) >> 16;

	sensor_write(sd, 0x3500,temp3);
	sensor_write(sd, 0x3501,temp2);
	sensor_write(sd, 0x3502,temp1);

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
	case V4L2_CID_EXPOSURE_AUTO:
		return v4l2_ctrl_query_fill(qc, 0, 1, 1, 0);
	case V4L2_CID_EXPOSURE:
		return v4l2_ctrl_query_fill(qc, -2, 2, 1, 0);
	case V4L2_CID_FOCUS_AUTO:
		return v4l2_ctrl_query_fill(qc, 0, 1, 0, 0);	
	}
	return -EINVAL;
}

static int sensor_g_flash(struct cmos_subdev *sd, int *value)
{
	int environmental, critical;

	environmental = sensor_read(sd, 0x350b);
	critical = sensor_read(sd, 0x3a19);

	printk(KERN_DEBUG "%s environmental 0x%x,criticali 0x%x\n",
	       __func__, environmental,critical);

	*value = (environmental >= critical);
	return 0;
}

static int sensor_g_ctrl(struct cmos_subdev *sd, struct v4l2_control *ctrl)
{
	switch (ctrl->id) {
	case V4L2_CID_CAMERA_AUTO_FOCUS_RESULT:
		return sensor_g_af(sd, &ctrl->value);
	case V4L2_CID_CAMERA_FLASH_MODE_AUTO:
		return sensor_g_flash(sd, &ctrl->value);
	default:
		return -EINVAL;
	}
	return 0;
}

static int sensor_s_ctrl(struct cmos_subdev *sd, struct v4l2_control *ctrl)
{
	switch (ctrl->id) {
	case V4L2_CID_CAMERA_SCENE_MODE:
		return sensor_s_scenemode(sd, ctrl->value);
	case V4L2_CID_DO_WHITE_BALANCE:	
		return sensor_s_wb(sd, ctrl->value);	
	case V4L2_CID_EXPOSURE_AUTO:
		return sensor_s_autoexposure(sd, ctrl->value);
	case V4L2_CID_EXPOSURE:
		return sensor_s_exposure(sd, ctrl->value);	
	case V4L2_CID_HFLIP:
		return sensor_s_hflip(sd, ctrl->value);
	case V4L2_CID_VFLIP:
		return sensor_s_vflip(sd, ctrl->value);
	case V4L2_CID_CAMERA_FOCUS_MODE:
		return sensor_s_af(sd, ctrl->value);
	case V4L2_CID_CAMERA_FOCUS_POSITION_X:
		return sensor_s_af_area_x(sd, ctrl->value);
	case V4L2_CID_CAMERA_FOCUS_POSITION_Y:
		return sensor_s_af_area_y(sd, ctrl->value);
	default:
		return -EINVAL;
	}
	return 0;
}

static int sensor_set_fps(struct cmos_subdev *sd, int fps)
{
	uint32_t array[]={
		0x380c,0x07,
		0x380d,0x64,
		0x380e,0x02,
		0x380f,0xe4,
	};

	if(fps == 0)
		return 0;
	switch(fps){
		case 26 ... 30:
			break;
		case 21 ... 25:
			array[5] = 0x03;array[7] = 0x85;
			break;
		case 16 ... 20:
			array[5] = 0x04;array[7] = 0x60;
			break;
		case 11 ... 15:
			break;
		case 5 ... 10:
			array[5] = 0x04;array[7] = 0x75;
			break;
	}
	sensor_write_array(sd, array, ARRAY_SIZE(array));
	msleep(50);
	sensor_fps =0;
	return 0;
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

	if (mf->width >= 1024) {
		sensor_s_shutter(sd);
	}

	win = cmos_select_win(&mf->width, &mf->height);
	if (!win) {
		pr_err("%s, s_mbus_fmt failed, width %d, height %d\n", 
		       sd->name, mf->width, mf->height);
		return -EINVAL;
	}
	if((mf->width ==1280)&&(mf->height == 720)){
		if(sensor_fps != 0){
			if(sensor_fps >15)
				sensor_write(sd, 0x3035,0x21);
			else
				sensor_write(sd, 0x3035,0x41);
			msleep(60);
		}else{
			sensor_write(sd, 0x3035,0x21);
		}
	}
	sensor_write_array(sd, win->regs, win->size);
	if((mf->width ==1280)&&(mf->height == 720)){
		sensor_set_fps(sd,sensor_fps);
	}
	msleep(200);
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

static int sensor_g_parm(struct cmos_subdev *sd, struct v4l2_streamparm *parms)
{
	return 0;
}

static int sensor_s_parm(struct cmos_subdev *sd, struct v4l2_streamparm *parms)
{
	struct v4l2_captureparm *cp = &parms->parm.capture;
	struct v4l2_fract *tpf = &cp->timeperframe;

	if (tpf->numerator == 0 || tpf->denominator == 0)	{
		return 0;
	}
	
	sensor_fps = tpf->denominator;//cause tpf->numerator == 1 in HAL

		
	return 0;
}

static int sensor_identify(struct cmos_subdev *sd)
{
	uint32_t data = 0;

	data |= (sensor_read(sd, 0x300a) & 0xff) << 8;
	data |= (sensor_read(sd, 0x300b) & 0xff);

	return (data == sd->id) ? 0 : -EINVAL;
}

static int sensor_init(struct cmos_subdev *sd)
{
	if (sensor_identify(sd))
		return -1;
	sensor_fps = 0;
	sensor_write_array(sd, ov5640_firmware, ARRAY_SIZE(ov5640_firmware));
	msleep(10);

	sensor_write(sd, 0x3103, 0x11);
	sensor_write(sd, 0x3008, 0x82);
	msleep(5);
	sensor_write_array(sd, ov5640_default_regs_init,
			   ARRAY_SIZE(ov5640_default_regs_init));

	msleep(50);
	sensor_s_af(sd, FOCUS_MODE_CONTINUOUS);
	return 0;
}

static int sensor_exit(struct cmos_subdev *sd)
{
	sensor_write(sd, 0x3017, 0x80);
	sensor_write(sd, 0x3018, 0x03);
	return 0;
}

static struct cmos_subdev_ops ov5640_ops = {
	.identify	= sensor_identify,
	.init		= sensor_init,
	.exit		= sensor_exit,
	.queryctrl	= sensor_queryctrl,
	.g_ctrl		= sensor_g_ctrl,
	.s_ctrl		= sensor_s_ctrl,
	.s_mbus_fmt     = sensor_s_mbus_fmt,
	.g_mbus_fmt     = sensor_g_mbus_fmt,
	.try_mbus_fmt	= sensor_try_mbus_fmt,
	.g_parm			= sensor_g_parm,
	.s_parm			= sensor_s_parm,
	.enum_framesizes = sensor_enum_framesizes,
};

struct cmos_subdev ov5640 = {
	.name		= "ov5640",
	.i2c_addr	= 0x3c,
	.id		= 0x5640,
	.max_width	= 2592,
	.max_height	= 1944,
	.ops		= &ov5640_ops,
};

#if 0
static int __init ov5640_init(void)
{
	return cmos_register_sudbdev(&ov5640);
}

static void __exit ov5640_exit(void)
{
	cmos_unregister_subdev(&ov5640);
	return;
}

module_init(ov5640_init);
module_exit(ov5640_exit);

MODULE_LICENSE("GPL");
#endif
