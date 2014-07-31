/*++ 
 * linux/drivers/media/video/wmt_v4l2/cmos/cmos-dev.h
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

#ifndef CMOS_DEV_H
#define CMOS_DEV_H

#include <linux/module.h>
#include <linux/delay.h>
#include <linux/videodev2.h>
#include <media/v4l2-device.h>
#include <media/v4l2-chip-ident.h>
#include <media/v4l2-mediabus.h>
#include <asm/errno.h>

#include <linux/videodev2_wmt.h>

typedef struct {
	int width;
	int height;
	int cmos_v_flip;
	int cmos_h_flip;
	unsigned int pix_color_format;
	int is_rawdata;
	int mode_switch;
} cmos_init_arg_t;

struct cmos_subdev;

struct cmos_subdev_ops {
	/* core */
	int (*identify)(struct cmos_subdev *sd);
	int (*init)(struct cmos_subdev *sd);
	int (*exit)(struct cmos_subdev *sd);
	int (*queryctrl)(struct cmos_subdev *sd, struct v4l2_queryctrl *qc);
	int (*querymenu)(struct cmos_subdev *sd, struct v4l2_querymenu *qm);
	int (*g_ctrl)(struct cmos_subdev *sd, struct v4l2_control *ctrl);
	int (*s_ctrl)(struct cmos_subdev *sd, struct v4l2_control *ctrl);

	/* video */
	int (*enum_framesizes)(struct cmos_subdev *sd, struct v4l2_frmsizeenum *fsize);
	int (*enum_mbus_fmt)(struct cmos_subdev *sd, unsigned int index, enum v4l2_mbus_pixelcode *code);
	int (*g_mbus_fmt)(struct cmos_subdev *sd, struct v4l2_mbus_framefmt *fmt);
	int (*try_mbus_fmt)(struct cmos_subdev *sd, struct v4l2_mbus_framefmt *fmt);
	int (*s_mbus_fmt)(struct cmos_subdev *sd, struct v4l2_mbus_framefmt *fmt);
	int (*g_parm)(struct cmos_subdev *sd, struct v4l2_streamparm *param);
	int (*s_parm)(struct cmos_subdev *sd, struct v4l2_streamparm *param);
	int (*cropcap)(struct cmos_subdev *sd, struct v4l2_cropcap *cc);
	int (*g_crop)(struct cmos_subdev *sd, struct v4l2_crop *crop);
	int (*s_crop)(struct cmos_subdev *sd, struct v4l2_crop *crop);
};

struct cmos_subdev {
	char name[12];
	int i2c_addr;
	int id;
	int chipid;
	int max_width;
	int max_height;
	struct cmos_subdev_ops *ops;
	struct list_head list;
};

#define cmos_subdev_call(sd, f, args...)			\
	(!(sd) ? -ENODEV : (((sd)->ops && (sd)->ops->f) ?	\
		(sd)->ops->f((sd) , ##args) : -ENOIOCTLCMD))

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
#endif

extern struct regulator *regulator_get(struct device *dev, const char *id);
extern void regulator_put(struct regulator *regulator);
extern int regulator_enable(struct regulator *regulator);
extern int regulator_disable(struct regulator *regulator);
extern int regulator_set_voltage(struct regulator *regulator, int min_uV, int max_uV);
extern int cmos_register_sudbdev(struct cmos_subdev *cmos_subdev);
extern void cmos_unregister_subdev(struct cmos_subdev *cmos_subdev);

extern struct cmos_subdev *cmos_subdev_get_by_try(void);
extern int cmos_subdev_init(void);
extern void cmos_subdev_exit(void);

void cmos_init_8bit_addr(unsigned char* array_addr, unsigned int array_size, unsigned char i2c_addr);
void cmos_init_16bit_addr(unsigned int * array_addr, unsigned int array_size, unsigned int i2c_addr);
void cmos_init_16bit_addr_16bit_data(unsigned int * array_addr, unsigned int array_size, unsigned int i2c_addr);

#endif /* ifndef CMOS_DEV_H */

