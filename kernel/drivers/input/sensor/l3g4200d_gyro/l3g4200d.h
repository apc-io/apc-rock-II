/******************** (C) COPYRIGHT 2010 STMicroelectronics ********************
*
* File Name		: l3g4200d.h
* Authors		: MH - C&I BU - Application Team
*               : Carmine Iascone (carmine.iascone@st.com)
*               : Matteo Dameno (matteo.dameno@st.com)
* Version		: V 1.1 sysfs
* Date			: 2010/Aug/19
* Description   : L3G4200D digital output gyroscope sensor API
*
********************************************************************************
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 as
* published by the Free Software Foundation.
*
* THE PRESENT SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES
* OR CONDITIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED, FOR THE SOLE
* PURPOSE TO SUPPORT YOUR APPLICATION DEVELOPMENT.
* AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY DIRECT,
* INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING FROM THE
* CONTENT OF SUCH SOFTWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING
* INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
*
********************************************************************************
* REVISON HISTORY
*
* VERSION	| DATE		| AUTHORS		| DESCRIPTION
* 1.0		| 2010/Aug/19	| Carmine Iascone	| First Release
* 1.1		| 2011/02/28	| Matteo Dameno		| Self Test Added
* 1.2		| 2013/04/29	| Howay Huo       | Android Interface Added
*******************************************************************************/

#ifndef __L3G4200D_H__
#define __L3G4200D_H__

#define L3G4200D_GYR_DEV_NAME   "l3g4200d_gyr"
#define GYRO_INPUT_NAME         "gyroscope"
#define GYRO_MISCDEV_NAME       "gyro_ctrl"

#define L3G4200D_DRVID 0

#define L3G4200D_GYR_FS_250DPS	0x00
#define L3G4200D_GYR_FS_500DPS	0x10
#define L3G4200D_GYR_FS_2000DPS	0x30

#define L3G4200D_GYR_ENABLED	1
#define L3G4200D_GYR_DISABLED	0

#define WMTGSENSOR_IOCTL_MAGIC  0x09
#define WMT_GYRO_IOCTL_MAGIC   0x11
#define GYRO_IOCTL_SET_ENABLE  _IOW(WMT_GYRO_IOCTL_MAGIC, 0, int)
#define GYRO_IOCTL_GET_ENABLE  _IOW(WMT_GYRO_IOCTL_MAGIC, 1, int)
#define GYRO_IOCTL_SET_DELAY   _IOW(WMT_GYRO_IOCTL_MAGIC, 2, int)
#define GYRO_IOCTL_GET_DELAY   _IOW(WMT_GYRO_IOCTL_MAGIC, 3, int)
#define WMT_IOCTL_SENSOR_GET_DRVID	 _IOW(WMTGSENSOR_IOCTL_MAGIC, 4, unsigned int)

#ifdef __KERNEL__

struct l3g4200d_triple {
	short	x,	/* x-axis angular rate data. */
		y,	/* y-axis angluar rate data. */
		z;	/* z-axis angular rate data. */
};


struct reg_value_t {
	u8 ctrl_reg1;
	u8 ctrl_reg2;
	u8 ctrl_reg3;
	u8 ctrl_reg4;
	u8 ctrl_reg5;
	u8 ref_datacap;
	u8 fifo_ctrl_reg;
	u8 int1_cfg;
	u8 int1_ths_xh;
	u8 int1_ths_xl;
	u8 int1_ths_yh;
	u8 int1_ths_yl;
	u8 int1_ths_zh;
	u8 int1_ths_zl;
	u8 int1_duration;
};

struct l3g4200d_gyr_platform_data {
	int (*init)(void);
	void (*exit)(void);
	int (*power_on)(void);
	int (*power_off)(void);
	int poll_interval;
	int min_interval;

	u8 fs_range;

	int axis_map_x;
	int axis_map_y;
	int axis_map_z;

	int direction_x;
	int direction_y;
	int direction_z;

	struct reg_value_t init_state;

};
#endif /* __KERNEL__ */

#endif  /* __L3G4200D_H__ */
