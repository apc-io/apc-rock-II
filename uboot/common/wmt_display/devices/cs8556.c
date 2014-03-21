/*++
 * linux/drivers/video/wmt/cs8556.c
 * WonderMedia video post processor (VPP) driver
 *
 * Copyright c 2013  WonderMedia  Technologies, Inc.
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

#define CS8556_C
// #define DEBUG
/*----------------------- DEPENDENCE -----------------------------------------*/
#include "../vout.h"

/*----------------------- PRIVATE MACRO --------------------------------------*/
/* #define  VT1632_XXXX  xxxx    *//*Example*/

/*----------------------- PRIVATE CONSTANTS ----------------------------------*/
/* #define VT1632_XXXX    1     *//*Example*/
#define CS8556_ADDR 0x3d
#define CS8556_NAME          "CS8556"

/*----------------------- PRIVATE TYPE  --------------------------------------*/
/* typedef  xxxx vt1632_xxx_t; *//*Example*/
typedef enum {
	TV_PAL,
	TV_NTSC,
	TV_UNDEFINED,
	TV_MAX
} vout_tvformat_t;

/*----------EXPORTED PRIVATE VARIABLES are defined in vt1632.h  -------------*/
/*----------------------- INTERNAL PRIVATE VARIABLES - -----------------------*/
/* int  vt1632_xxx;        *//*Example*/
static int s_cs8556_ready;
static int s_cs8556_init;
static int i2c_no = 1;
static vout_tvformat_t s_tvformat = TV_MAX;

static unsigned char s_CS8556_Original_Offset0[]={
	0xF0,0x7F,0x00,0x00,0x00,0x00,0x80,0x80,0x80,0x00,0x00,0x02,0x01,0x00,0x00,0x01,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
};

static unsigned char s_RGB888_To_PAL_Offset0[]={
	0x01,0x80,0x00,0x00,0x80,0x00,0x00,0x00,0x00,0x00,0x02,0x02,0x00,0x00,0x00,0x00,
	0x20,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x5F,0x03,0x3F,0x00,0x7D,0x00,0x53,0x03,0x00,0x00,0x00,0x10,0x00,0x00,0x00,0x00,
	0x70,0x02,0x04,0x00,0x2E,0x00,0x62,0x02,0x00,0x00,0x84,0x00,0x2B,0x00,0x36,0x00,
	0x12,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0xBF,0x06,0x7F,0x00,0xFE,0x00,0xA4,0x06,0x00,0x00,0x2D,0x11,0x3C,0x01,0x3A,0x01,
	0x70,0x02,0x04,0x00,0x12,0x00,0x34,0x01,0x00,0x00,0x70,0x70,0x70,0x00,0x00,0x00,
	0x22,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x04,0x41,0x18,0x09,0x00,0x00,0x00,0x02,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0xFF,0xFF,0x00,0x00,0xFF,0xFF,0x00,0x00,0x80,0x80,0x00,0x00,0x80,0x80,0x00,0x00,
	0xFF,0xFF,0xFF,0xFF,0x00,0x00,0x00,0x00,0x80,0x80,0x80,0x80,0x00,0x00,0x00,0x00,
	0xFF,0x00,0xFF,0x00,0xFF,0x00,0xFF,0x00,0x80,0x00,0x80,0x00,0x80,0x00,0x80,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x01,0x24,0x1A,0x00,0x01,0x03,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x04,0x01,0xA4,0x06,0x0B,0x00,0x07,0x01,0xF0,0x00,0x00,0x00,0x00,0x04,0x40,0x01
};

static unsigned char s_RGB888_To_NTSC_Offset0[]={
	0x01,0x80,0x00,0x00,0x80,0x00,0x00,0x00,0x00,0x00,0x02,0x02,0x00,0x00,0x00,0x00,
	0x20,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x59,0x03,0x3D,0x00,0x7E,0x00,0x49,0x03,0x00,0x00,0x00,0x10,0x00,0x00,0x00,0x00,
	0x0C,0x02,0x05,0x00,0x21,0x00,0x03,0x02,0x00,0x00,0x7A,0x00,0x23,0x00,0x16,0x00,
	0x12,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0xB3,0x06,0x7F,0x00,0x00,0x01,0xA4,0x06,0x00,0x00,0x05,0x50,0x00,0x01,0x07,0x01,
	0x0C,0x02,0x02,0x00,0x12,0x00,0x07,0x01,0x00,0x00,0x70,0x70,0x70,0x00,0x00,0x00,
	0x22,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x04,0x41,0x18,0x09,0x00,0x00,0x00,0x02,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0xFF,0xFF,0x00,0x00,0xFF,0xFF,0x00,0x00,0x80,0x80,0x00,0x00,0x80,0x80,0x00,0x00,
	0xFF,0xFF,0xFF,0xFF,0x00,0x00,0x00,0x00,0x80,0x80,0x80,0x80,0x00,0x00,0x00,0x00,
	0xFF,0x00,0xFF,0x00,0xFF,0x00,0xFF,0x00,0x80,0x00,0x80,0x00,0x80,0x00,0x80,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x01,0x24,0x1A,0x00,0x01,0x03,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x04,0x01,0xA4,0x06,0x0B,0x00,0x07,0x01,0xF0,0x00,0x00,0x00,0x00,0x04,0x00,0x00
};


/*--------------------- INTERNAL PRIVATE FUNCTIONS ---------------------------*/
/* void vt1632_xxx(void); *//*Example*/
static int I2CMultiPageRead(uchar maddr, uchar page, uchar saddr, int number, uchar *value)
{
	int ret;
	uchar wbuf[2];
	struct i2c_msg_s rd[2];

	wbuf[0] = page;
	wbuf[1] = saddr;

	rd[0].addr  = maddr;
	rd[0].flags = I2C_M_WR;
	rd[0].len   = 2;
	rd[0].buf   = wbuf;

	rd[1].addr  = maddr;
	rd[1].flags = I2C_M_RD;
	rd[1].len   = number;
	rd[1].buf   = value;

	switch(i2c_no) {
		case 0:
			ret = i2c_transfer(rd, 2);
		break;
		case 1:
			ret = i2c1_transfer(rd, 2);
		break;
		case 2:
			ret = i2c2_transfer(rd, 2);
		break;
		case 3:
			ret = i2c3_transfer(rd, 2);
		break;
		default:
			DBG_ERR("i2c%d isn't exist in wm8880\n", i2c_no);
		return -1;

	}

	if (ret != 2) {
		DBG_ERR("fail\n", i2c_no);
		return -1;
	}

	return 0;
}

static int I2CMultiPageWrite(uchar maddr,uchar page,uchar saddr,int number,uchar *value)
{
	int ret;
	uchar *pbuf;
	struct i2c_msg_s wr[1];

	pbuf = calloc(number + 2, sizeof(uchar));

	pbuf[0] = page;
	pbuf[1] = saddr;

	memcpy(pbuf + 2, value, number);

	wr[0].addr  = maddr;
	wr[0].flags = I2C_M_WR;
	wr[0].len   = number + 2;
	wr[0].buf   = pbuf;

	switch(i2c_no) {
		case 0:
			ret = i2c_transfer(wr, 1);
		break;
		case 1:
			ret = i2c1_transfer(wr, 1);
		break;
		case 2:
			ret = i2c2_transfer(wr, 1);
		break;
		case 3:
			ret = i2c3_transfer(wr, 1);
		break;
		default:
			DBG_ERR("i2c%d isn't exist in wm8880\n", i2c_no);
		return -1;

	}

	if (ret != 1) {
		DBG_ERR("fail\n", i2c_no);
		free(pbuf);
		return -1;
	}

	free(pbuf);
	return 0 ;
}

/*----------------------- Function Body --------------------------------------*/

static int cs8556_check_plugin(int hotplug)
{
	return 1;
}

static int cs8556_init(struct vout_s *vo)
{
	int ret;
	char buf[40];
	int varlen = 40;
	unsigned char rbuf[256] = {0};

	if(s_tvformat == TV_MAX) {
	if(wmt_getsyspara("wmt.display.tvformat", buf, &varlen) == 0) {
			if(!strnicmp(buf, "PAL", 3))
				s_tvformat = TV_PAL;
			else if(!strnicmp(buf, "NTSC", 4))
				s_tvformat = TV_NTSC;
			else
				s_tvformat = TV_UNDEFINED;
		}
		else
			s_tvformat = TV_UNDEFINED;
	}

	if(s_tvformat == TV_UNDEFINED)
		return -1;

	if(!s_cs8556_init) {
	if(wmt_getsyspara("wmt.cs8556.i2c", buf, &varlen) == 0)	{
		if(strlen(buf) > 0)
			i2c_no = buf[0] - '0';
	}

	switch (i2c_no) {
		case 0:
			i2c_init(I2C_FAST_MODE, 0);
		break;
		case 1:
			i2c1_init(I2C_FAST_MODE, 0);
		break;
		case 2:
			i2c2_init(I2C_FAST_MODE, 0);
		break;
		case 3:
			i2c3_init(I2C_FAST_MODE, 0);
		break;
		default:
			DBG_ERR("i2c%d isn't exist in wm8850\n", i2c_no);
		return -1;
	}

		s_cs8556_init = 1;
	}

	ret = I2CMultiPageRead(CS8556_ADDR, 0x00, 0x00, 256, rbuf);
			if(ret) {
				DBG_ERR("I2C address 0x%02X is not found\n", CS8556_ADDR);
				return -1;
			}

	switch(s_tvformat) {
		case TV_PAL:
			ret = I2CMultiPageWrite(CS8556_ADDR, 0x00, 0x00, 256, s_RGB888_To_PAL_Offset0);
			if(ret) {
				DBG_ERR("PAL init fail\n");
				return -1;
		}
		break;

		case TV_NTSC:
			ret = I2CMultiPageWrite(CS8556_ADDR, 0x00, 0x00, 256, s_RGB888_To_NTSC_Offset0);
			if(ret) {
				DBG_ERR("NTSC init fail\n");
				return -1;
			}
			break;

		default:
			return -1;
		break;
	}

	s_cs8556_ready = 1;

	DPRINT("cs8556 init ok\n");

	return 0;
}

int cs8556_set_mode(unsigned int *option)
{
	if(!s_cs8556_ready )
		return -1;

	return 0;
}

void cs8556_set_power_down(int enable)
{
	int ret;
	unsigned char rbuf[256] = {0};

	if( !s_cs8556_ready )
		return;

	//DPRINT("cs8556_set_power_down(%d)\n",enable);

	if(enable == 0) {
		if(hdmi_get_plugin()) {
			DPRINT("[CVBS] HDMI plugin. CVBS power down\n");
			enable = 1;
		} else
			DPRINT("[CVBS] HDMI plugout. CVBS power up\n");
	} else
		DPRINT("[CVBS] CVBS power down\n");

	ret = I2CMultiPageRead(CS8556_ADDR, 0x00, 0x00, 256, rbuf);
	if(ret) {
		DBG_ERR("I2C read Offset0 fail\n");
		return;
	}

	if( enable ){
		if(memcmp(rbuf, s_CS8556_Original_Offset0, 5) != 0) {
			ret = I2CMultiPageWrite(CS8556_ADDR, 0x00, 0x00, 5, s_CS8556_Original_Offset0);
			if(ret)
				DBG_ERR("I2C write Original_Offset0 fail\n");
		}
	}
	else {
		switch(s_tvformat) {
			case TV_PAL:
				if(memcmp(rbuf, s_RGB888_To_PAL_Offset0, 0x50) != 0) {
				ret = I2CMultiPageWrite(CS8556_ADDR, 0x00, 0x00, 256, s_RGB888_To_PAL_Offset0);
				if(ret)
					DBG_ERR("I2C write PAL_Offset0 fail\n");
			}
			break;
			case TV_NTSC:
				if(memcmp(rbuf, s_RGB888_To_NTSC_Offset0, 0x50) !=0) {
				ret = I2CMultiPageWrite(CS8556_ADDR, 0x00, 0x00, 256, s_RGB888_To_NTSC_Offset0);
				if(ret)
					DBG_ERR("I2C write NTSC_Offset0 fail\n");
			}
			break;
			default:
			break;
		}
	}
}

static int cs8556_config(vout_info_t *info)
{
	return 0;
}

static int cs8556_get_edid(char *buf)
{
	return -1;
}

/*
static int cs8556_interrupt(void)
{
	return cs8556_check_plugin(1);
}
*/
/*----------------------- vout device plugin --------------------------------------*/
vout_dev_t cs8556_vout_dev_ops = {
	.name = CS8556_NAME,
	.mode = VOUT_INF_DVI,

	.init = cs8556_init,
	.set_power_down = cs8556_set_power_down,
	.set_mode = cs8556_set_mode,
	.config = cs8556_config,
	.check_plugin = cs8556_check_plugin,
	.get_edid = cs8556_get_edid,
	//.interrupt = cs8556_interrupt,
};

int cs8556_module_init(void)
{
	vout_device_register(&cs8556_vout_dev_ops);
	return 0;
} /* End of cs8556_module_init */
module_init(cs8556_module_init);

/*--------------------End of Function Body -----------------------------------*/
#undef CS8556_C

