/*
 * linux/drivers/media/video/wmt_v4l2/cmos/cmos-subdev.c
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
 */

#include <linux/module.h>
#include <linux/videodev2.h>

#include "cmos-subdev.h"
#include "../wmt-vid.h"

void cmos_init_8bit_addr(uint8_t *array_addr, size_t array_size, uint8_t i2c_addr)
{
	uint32_t addr, data;
	int i;

	for(i=0;i<array_size;i+=2)
	{
		addr = array_addr[i];
		data = array_addr[i+1];
		wmt_vid_i2c_write(i2c_addr,addr,data);
	}
}
EXPORT_SYMBOL(cmos_init_8bit_addr);

void cmos_init_16bit_addr(uint32_t *array_addr, size_t array_size, uint32_t i2c_addr)
{
	unsigned int addr,data,i;

	for(i=0;i<array_size;i+=2)
	{
		addr = array_addr[i];
		data = array_addr[i+1];
		wmt_vid_i2c_write16addr(i2c_addr,addr,data);
	}
}
EXPORT_SYMBOL(cmos_init_16bit_addr);

void cmos_init_16bit_addr_16bit_data(uint32_t *array_addr, size_t array_size, uint32_t i2c_addr)
{
	unsigned int addr,data,i;

	for(i=0;i<array_size;i+=2)
	{
		addr = array_addr[i];
		data = array_addr[i+1];
		wmt_vid_i2c_write16data(i2c_addr,addr,data);
	}
}
EXPORT_SYMBOL(cmos_init_16bit_addr_16bit_data);

static LIST_HEAD(__cmos_subdev_list);

int cmos_register_sudbdev(struct cmos_subdev *sd)
{
	printk(KERN_INFO "register cmos: %s\n", sd->name);
	list_add_tail(&sd->list, &__cmos_subdev_list);
	return 0;
}
EXPORT_SYMBOL(cmos_register_sudbdev);

void cmos_unregister_subdev(struct cmos_subdev *sd)
{
	printk(KERN_INFO "unregister cmos: %s\n", sd->name);
	list_del(&sd->list);
}
EXPORT_SYMBOL(cmos_unregister_subdev);

struct cmos_subdev *cmos_subdev_get_by_try(void)
{
	struct cmos_subdev *sd;
	struct list_head *node, *next;

	list_for_each_safe(node, next, &__cmos_subdev_list) {
		sd = container_of(node, struct cmos_subdev, list);
		if (!cmos_subdev_call(sd, identify)) {
			return sd;
		}
	}
	return NULL;
}

// below init all cmos

extern struct cmos_subdev gc0307, gc0308, gc0309, gc0311, gc0329, gc0328, gc2015, gc2035;
extern struct cmos_subdev ov2640, ov2643, ov2659, ov3640, ov3660, ov5640,ov7675,ov9740;
extern struct cmos_subdev hi704, hi253,hi257;
extern struct cmos_subdev siv121d,siv120d, sid130b;
extern struct cmos_subdev sp0838, sp0718,sp0a19, sp2518;
extern struct cmos_subdev bf3a03, bf3920;

static struct cmos_subdev *devices[] = {
	/* SuperPix */
	&sp0718, &sp0a19,&sp2518, &sp0838,
	/* Seti */
	&siv121d,&siv120d, &sid130b,
	/* Hynix */
	&hi253,&hi257,
	/* Galaxyc */
	&gc0307,&gc0308, &gc0309, &gc0328, &gc0329, &gc2035,
	/* OminiVision */
	&ov2659, &ov5640,&ov7675,&ov9740,
	/*BYD*/
	&bf3a03,&bf3920,
	NULL,
};

int cmos_subdev_init(void)
{
	int ret = 0;
	struct cmos_subdev **sd = &devices[0];

	while (*sd) {
		ret = cmos_register_sudbdev(*sd);
		if (ret) {
			break;
		}
		sd++;
	}

	return ret;
}

void cmos_subdev_exit(void)
{
	struct cmos_subdev **sd = &devices[0];

	while (*sd) {
		cmos_unregister_subdev(*sd);
		sd++;
	}
}
