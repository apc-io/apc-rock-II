/*++
 * linux/drivers/video/wmt/hdmi.c
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

#define HDMI_C
#undef DEBUG
/* #define DEBUG */
/*----------------------- DEPENDENCE -----------------------------------------*/
#include "hdmi.h"
#include "vout.h"
#ifdef __KERNEL__
#include <asm/div64.h>
#endif

/*----------------------- PRIVATE MACRO --------------------------------------*/

/*----------------------- PRIVATE CONSTANTS ----------------------------------*/
/* #define HDMI_XXXX    1     *//*Example*/
/* #define CONFIG_HDMI_INFOFRAME_DISABLE */
/* #define CONFIG_HDMI_EDID_DISABLE */

#define HDMI_I2C_FREQ	80000

#define MAX_ACR_TV_NUM  128
/*----------------------- PRIVATE TYPE  --------------------------------------*/
/* typedef  xxxx hdmi_xxx_t; *//*Example*/
enum hdmi_fifo_slot_t {
	HDMI_FIFO_SLOT_AVI = 0,
	HDMI_FIFO_SLOT_VENDOR = 1,
	HDMI_FIFO_SLOT_AUDIO = 2,
	HDMI_FIFO_SLOT_CONTROL = 3,
	HDMI_FIFO_SLOT_MAX = 15
};

/*----------EXPORTED PRIVATE VARIABLES are defined in hdmi.h  -------------*/
/*----------------------- INTERNAL PRIVATE VARIABLES - -----------------------*/
/* int  hdmi_xxx;        *//*Example*/

/*
Added by howayhuo
Some TV need fix the HDMI ACR ration, otherwise these TV will show "overclock"
Fill in the TV vendor name and TV monitor name to this list if your TV needs fix acr
*/
static const tv_name_t fix_acr_tv_list[] = {
	{"TCL", "RTD2662"},  //another name: TCL L19E09
};

static tv_name_t *p_fix_acr_tv;

static int fixed_acr_ration_val = 2300;
/*--------------------- INTERNAL PRIVATE FUNCTIONS ---------------------------*/
/* void hdmi_xxx(void); *//*Example*/

/*----------------------- Function Body --------------------------------------*/
/*---------------------------- HDMI COMMON API -------------------------------*/
unsigned int hdmi_reg32_write(U32 offset, U32 mask, U32 shift, U32 val)
{
	unsigned int new_val;

	new_val = (vppif_reg32_in(offset) & ~(mask))
			| (((val) << (shift)) & mask);
	if (offset == REG_HDMI_GENERAL_CTRL) {
		new_val &= ~(BIT24 | BIT25 | BIT26);
		new_val |= g_vpp.hdmi_ctrl;
		DBG_MSG("[HDMI] reg32_wr 0x%x ctrl 0x%x\n",
					new_val, g_vpp.hdmi_ctrl);
	}
	vppif_reg32_out(offset, new_val);
	return new_val;
}

unsigned char hdmi_ecc(unsigned char *buf, int bit_cnt)
{
	#define HDMI_CRC_LEN	9

	int crc[HDMI_CRC_LEN], crc_o[HDMI_CRC_LEN];
	int i, j;
	int input, result, result_rev = 0;

	for (i = 0; i < HDMI_CRC_LEN; i++)
		crc[i] = 0;

	for (i = 0; i < bit_cnt; i++) {
		for (j = 0; j < HDMI_CRC_LEN; j++)
			crc_o[j] = crc[j];
		input = (buf[i/8] & (1<<(i%8))) ? 1 : 0;
		crc[0] = crc_o[7] ^ input;
		crc[1] = crc_o[0];
		crc[2] = crc_o[1];
		crc[3] = crc_o[2];
		crc[4] = crc_o[3];
		crc[5] = crc_o[4];
		crc[6] = crc_o[5] ^ crc_o[7] ^ input;
		crc[7] = crc_o[6] ^ crc_o[7] ^ input;
		crc[8] = crc_o[7];

		result     = 0;
		result_rev = 0;
		for (j = 0; j < HDMI_CRC_LEN - 1; j++) {
			result     += (crc[j] << j);
			result_rev += (crc[j] << (HDMI_CRC_LEN - 2 - j));
		}
	}

/*	DPRINT("[HDMI] crc 0x%x, %x %x %x %x %x %x %x\n",result_rev,
			buf[0],buf[1],buf[2],buf[3],buf[4],buf[5],buf[6]); */
	return result_rev;
}

unsigned char hdmi_checksum(unsigned char *header,
					unsigned char *buf, int cnt)
{
	unsigned char sum;
	int i;

	for (i = 0, sum = 0; i < cnt; i++)
		sum += buf[i];
	for (i = 0; i < 3; i++)
		sum += header[i];
	return 0 - sum;
}

#ifdef WMT_FTBLK_HDMI
/*---------------------------- HDMI HAL --------------------------------------*/
void hdmi_set_power_down(int pwrdn)
{
	DBG_DETAIL("(%d)\n", pwrdn);

	if ((vppif_reg32_read(HDMI_PD) == 0) && (pwrdn == 0)) {
		return; /* avoid HDMI reset */
	}

	vppif_reg32_write(HDMI_INTERNAL_LDO, (pwrdn) ? 0 : 1);
	vppif_reg32_write(HDMI_PD, pwrdn);
	if (!pwrdn) {
		vppif_reg32_write(HDMI_RESET_PLL, 1);
		mdelay(1);
		vppif_reg32_write(HDMI_RESET_PLL, 0);
	}
	mdelay(1);
	vppif_reg32_write(HDMI_PD_L2HA, pwrdn);
}

int hdmi_get_power_down(void)
{
	return vppif_reg32_read(HDMI_PD);
}

void hdmi_set_enable(vpp_flag_t enable)
{
	hdmi_reg32_write(HDMI_ENABLE, enable);
	vppif_reg32_write(HDMI_MODE, (enable) ? 0 : 1);
}

void hdmi_set_avmute(vpp_flag_t mute)
{
	vppif_reg32_write(HDMI_AVMUTE_SET_ENABLE, mute);
}

void hdmi_set_dvi_enable(vpp_flag_t enable)
{
	hdmi_reg32_write(HDMI_DVI_MODE_ENABLE, enable);
}

void hdmi_set_sync_low_active(vpp_flag_t hsync, vpp_flag_t vsync)
{
	hdmi_reg32_write(HDMI_HSYNC_LOW_ACTIVE, hsync);
	hdmi_reg32_write(HDMI_VSYNC_LOW_ACTIVE, vsync);
}

void hdmi_get_sync_polar(int *hsync_hi, int *vsync_hi)
{
	*hsync_hi = (vppif_reg32_read(HDMI_HSYNC_LOW_ACTIVE)) ? 0 : 1;
	*vsync_hi = (vppif_reg32_read(HDMI_VSYNC_LOW_ACTIVE)) ? 0 : 1;
}

void hdmi_set_output_colfmt(vdo_color_fmt colfmt)
{
	unsigned int val;

	switch (colfmt) {
	default:
	case VDO_COL_FMT_ARGB:
		val = 0;
		break;
	case VDO_COL_FMT_YUV444:
		val = 1;
		break;
	case VDO_COL_FMT_YUV422H:
	case VDO_COL_FMT_YUV422V:
		val = 2;
		break;
	}
	hdmi_reg32_write(HDMI_CONVERT_YUV422, (val == 2) ? 1 : 0);
	hdmi_reg32_write(HDMI_OUTPUT_FORMAT, val);
}

vdo_color_fmt hdmi_get_output_colfmt(void)
{
	unsigned int val;

	val = vppif_reg32_read(HDMI_OUTPUT_FORMAT);
	switch (val) {
	default:
	case 0:
		return VDO_COL_FMT_ARGB;
	case 1:
		return VDO_COL_FMT_YUV444;
	case 2:
		return VDO_COL_FMT_YUV422H;
	}
	return VDO_COL_FMT_ARGB;
}

int hdmi_get_plugin(void)
{
	int plugin;

	if (vppif_reg32_read(HDMI_HOTPLUG_IN_INT)) {
		  plugin = vppif_reg32_read(HDMI_HOTPLUG_IN);
	} else {
		  int tre_en;

		  tre_en = vppif_reg32_read(HDMI_TRE_EN);
		  vppif_reg32_write(HDMI_TRE_EN, 0);
		  plugin = vppif_reg32_read(HDMI_RSEN);
		  vppif_reg32_write(HDMI_TRE_EN, tre_en);
	}
	return plugin;
}

int hdmi_get_plug_status(void)
{
	int reg;

	reg = vppif_reg32_in(REG_HDMI_HOTPLUG_DETECT);
	return reg & 0x3000000;
}

void hdmi_clear_plug_status(void)
{
	vppif_reg32_write(HDMI_HOTPLUG_IN_STS, 1);
	vppif_reg32_write(HDMI_HOTPLUG_OUT_STS, 1);
}

void hdmi_enable_plugin(int enable)
{
	vppif_reg32_write(HDMI_HOTPLUG_OUT_INT, enable);
	vppif_reg32_write(HDMI_HOTPLUG_IN_INT, enable);
}

void hdmi_write_fifo(enum hdmi_fifo_slot_t no, unsigned int *buf, int cnt)
{
	int i;

	if (no > HDMI_FIFO_SLOT_MAX)
		return;
#ifdef DEBUG
{
	char *ptr;

	DPRINT("[HDMI] wr fifo %d,cnt %d", no, cnt);
	ptr = (char *) buf;
	for (i = 0; i < cnt; i++) {
		if ((i % 4) == 0)
			DPRINT("\n %02d :", i);
		DPRINT(" 0x%02x", ptr[i]);
	}
	DPRINT("\n[HDMI] AVI info package end\n");
}
#endif

	vppif_reg32_out(REG_HDMI_FIFO_CTRL, (no << 8));
	cnt = (cnt + 3) / 4;
	for (i = 0; i < cnt; i++)
		vppif_reg32_out(REG_HDMI_WR_FIFO_ADDR + 4 * i, buf[i]);
	vppif_reg32_write(HDMI_INFOFRAME_WR_STROBE, 1);
}

void hdmi_read_fifo(enum hdmi_fifo_slot_t no, unsigned int *buf, int cnt)
{
	int i;
	int rdy;

	if (no > HDMI_FIFO_SLOT_MAX)
		return;

	rdy = vppif_reg32_read(HDMI_INFOFRAME_FIFO1_RDY);
	vppif_reg32_write(HDMI_INFOFRAME_FIFO1_RDY, 0);

	no = no - 1;
	vppif_reg32_out(REG_HDMI_FIFO_CTRL, (no << 8));
	vppif_reg32_write(HDMI_INFOFRAME_RD_STROBE, 1);
	cnt = (cnt + 3) / 4;
	for (i = 0; i < cnt; i++)
		buf[i] = vppif_reg32_in(REG_HDMI_RD_FIFO_ADDR + 4 * i);
	vppif_reg32_write(HDMI_INFOFRAME_FIFO1_RDY, rdy);
#ifdef DEBUG
{
	char *ptr;

	cnt *= 4;
	DPRINT("[HDMI] rd fifo %d,cnt %d", no, cnt);
	ptr = (char *) buf;
	for (i = 0; i < cnt; i++) {
		if ((i % 4) == 0)
			DPRINT("\n %02d :", i);
		DPRINT(" 0x%02x", ptr[i]);
	}
	DPRINT("\n[HDMI] AVI info package end\n");
}
#endif
}

#if 1
int hdmi_ddc_delay_us = 5;
int hdmi_ddc_ctrl_delay_us = 5;

#define HDMI_DDC_OUT
#define HDMI_DDC_DELAY		hdmi_ddc_delay_us
#define HDMI_DDC_CHK_DELAY	1
#define HDMI_DDC_CTRL_DELAY	hdmi_ddc_ctrl_delay_us
#else
#define HDMI_DDC_OUT
#define HDMI_DDC_DELAY		1
#define HDMI_DDC_CHK_DELAY	1
#define HDMI_DDC_CTRL_DELAY	1
#endif

#define HDMI_STATUS_START	BIT16
#define HDMI_STATUS_STOP	BIT17
#define HDMI_STATUS_WR_AVAIL	BIT18
#define HDMI_STATUS_CP_USE	BIT19
#define HDMI_STATUS_SW_READ	BIT25
int hdmi_DDC_check_status(unsigned int checkbits, int condition)
{
	int status = 1;
	unsigned int i = 0, maxloop;

	maxloop = 500 / HDMI_DDC_CHK_DELAY;
	udelay(HDMI_DDC_DELAY);

	if (condition) { /* wait 1 --> 0 */
		while ((vppif_reg32_in(REG_HDMI_I2C_CTRL2) & checkbits)
			&& (i < maxloop)) {
			udelay(HDMI_DDC_CHK_DELAY);
			if (++i == maxloop)
				status = 0;
		}
	} else { /* wait 0 --> 1 */
		while (!(vppif_reg32_in(REG_HDMI_I2C_CTRL2) & checkbits)
			&& (i < maxloop)) {
			udelay(HDMI_DDC_CHK_DELAY);
			if (++i == maxloop)
				status = 0;
		}
	}

	if ((status == 0) && (checkbits != HDMI_STATUS_SW_READ)) {
		unsigned int reg;

		reg = vppif_reg32_in(REG_HDMI_I2C_CTRL2);
		DBG_DETAIL("[HDMI] status timeout check 0x%x,wait to %s\n",
			checkbits, (condition) ? "0" : "1");
		DBG_DETAIL("[HDMI] 0x%x,sta %d,stop %d,wr %d,rd %d,cp %d\n",
			reg, (reg & HDMI_STATUS_START) ? 1 : 0,
			(reg & HDMI_STATUS_STOP) ? 1 : 0,
			(reg & HDMI_STATUS_WR_AVAIL) ? 1 : 0,
			(reg & HDMI_STATUS_SW_READ) ? 1 : 0,
			(reg & HDMI_STATUS_CP_USE) ? 1 : 0);
	}
	return status;
}

void hdmi_DDC_set_freq(unsigned int hz)
{
	unsigned int clock;
	unsigned int div;

	clock = 25000000*15/100;	/* RTC clock source */
	div = clock / hz;

	vppif_reg32_write(HDMI_I2C_CLK_DIVIDER, div);
	DBG_DETAIL("[HDMI] set freq(%d,clk %d,div %d)\n", hz, clock, div);
}

void hdmi_DDC_reset(void)
{
	vppif_reg32_write(HDMI_I2C_SW_RESET, 1);
	udelay(1);
	vppif_reg32_write(HDMI_I2C_SW_RESET, 0);
}

int hdmi_DDC_read_func(char addr, int index, char *buf, int length)
{
	int status = 1;
	unsigned int i = 0;
	int err_cnt = 0;

	DBG_DETAIL("[HDMI] read DDC(index 0x%x,len %d),reg 0x%x\n",
		index, length, vppif_reg32_in(REG_HDMI_I2C_CTRL2));

#ifdef CONFIG_HDMI_EDID_DISABLE
	return status;
#endif

	hdmi_DDC_set_freq(g_vpp.hdmi_i2c_freq);
	/* enhanced DDC read */
	if (index >= 256) {
		/* sw start, write data avail */
		vppif_reg32_write(REG_HDMI_I2C_CTRL2, BIT18 + BIT16, 16, 0x5);
		udelay(HDMI_DDC_CTRL_DELAY);
		/* wait start & wr data avail */
		status = hdmi_DDC_check_status(HDMI_STATUS_START +
					HDMI_STATUS_WR_AVAIL, 1);
		if (status == 0) {
			DBGMSG("[HDMI] *E* start\n");
			err_cnt++;
			goto ddc_read_fail;
		}

		/* Slave address */
		vppif_reg32_write(HDMI_WR_DATA, 0x60);
		vppif_reg32_write(HDMI_WR_DATA_AVAIL, 1); /* write data avail */
		udelay(HDMI_DDC_CTRL_DELAY);
		/* wait wr data avail */
		status = hdmi_DDC_check_status(HDMI_STATUS_WR_AVAIL, 1);
		if (status == 0) {
			DBGMSG("[HDMI] *E* slave addr 0x%x\n", addr);
			err_cnt++;
			goto ddc_read_fail;
		}

		/* Offset */
		vppif_reg32_write(HDMI_WR_DATA, 0x1);
		vppif_reg32_write(HDMI_WR_DATA_AVAIL, 1); /* write data avail */
		udelay(HDMI_DDC_CTRL_DELAY);
		/* wait wr data avail */
		status = hdmi_DDC_check_status(HDMI_STATUS_WR_AVAIL, 1);
		if (status == 0) {
			DBGMSG("[HDMI] *E* index 0x%x\n", index);
			err_cnt++;
			goto ddc_read_fail;
		}
		index -= 256;
	}

	/* START */
#ifdef HDMI_DDC_OUT
	vppif_reg32_out(REG_HDMI_I2C_CTRL2, 0x50000);
#else
#if 0
	vppif_reg32_write(HDMI_SW_START_REQ, 1); /* sw start */
	vppif_reg32_write(HDMI_WR_DATA_AVAIL, 1); /* write data avail */
#endif
	/* sw start, write data avail */
	vppif_reg32_write(REG_HDMI_I2C_CTRL2, BIT18 + BIT16, 16, 0x5);
#endif
	udelay(HDMI_DDC_CTRL_DELAY);
	/* wait start & wr data avail */
	status = hdmi_DDC_check_status(HDMI_STATUS_START +
					HDMI_STATUS_WR_AVAIL, 1);
	if (status == 0) {
		DBGMSG("[HDMI] *E* start\n");
		err_cnt++;
		goto ddc_read_fail;
	}

	/* Slave address */
#ifdef HDMI_DDC_OUT
	vppif_reg32_out(REG_HDMI_I2C_CTRL2, 0x400A0);
#else
	vppif_reg32_write(HDMI_WR_DATA, addr);
	udelay(HDMI_DDC_DELAY);
	while (vppif_reg32_read(HDMI_WR_DATA) != addr)
		;
	udelay(HDMI_DDC_DELAY);
	vppif_reg32_write(HDMI_WR_DATA_AVAIL, 1); /* write data avail */
#endif
	udelay(HDMI_DDC_CTRL_DELAY);
	/* wait wr data avail */
	status = hdmi_DDC_check_status(HDMI_STATUS_WR_AVAIL, 1);
	if (status == 0) {
		DBGMSG("[HDMI] *E* slave addr 0x%x\n", addr);
		err_cnt++;
		goto ddc_read_fail;
	}

	/* Offset */
#ifdef HDMI_DDC_OUT
{
	unsigned int reg;

	reg = 0x40000;
	reg |= index;

	vppif_reg32_out(REG_HDMI_I2C_CTRL2, reg);
}
#else
	vppif_reg32_write(HDMI_WR_DATA, index);
	udelay(HDMI_DDC_DELAY);
	while (vppif_reg32_read(HDMI_WR_DATA) != index)
		;
	udelay(HDMI_DDC_DELAY);
	vppif_reg32_write(HDMI_WR_DATA_AVAIL, 1); /* write data avail */
#endif
	udelay(HDMI_DDC_CTRL_DELAY);
	/* wait wr data avail */
	status = hdmi_DDC_check_status(HDMI_STATUS_WR_AVAIL, 1);
	if (status == 0) {
		DBGMSG("[HDMI] *E* index 0x%x\n", index);
		err_cnt++;
		goto ddc_read_fail;
	}

/*	vppif_reg32_write(HDMI_WR_DATA,addr+1); */

	/* START */
#ifdef HDMI_DDC_OUT
	vppif_reg32_out(REG_HDMI_I2C_CTRL2, 0x50000);
#else
#if 0
	vppif_reg32_write(HDMI_SW_START_REQ, 1); /* sw start */
	vppif_reg32_write(HDMI_WR_DATA_AVAIL, 1); /* write data avail */
#endif
	/* sw start, write data avail */
	vppif_reg32_write(REG_HDMI_I2C_CTRL2, BIT18 + BIT16, 16, 0x5);
#endif
	udelay(HDMI_DDC_CTRL_DELAY);
	/* wait start & wr data avail */
	status = hdmi_DDC_check_status(HDMI_STATUS_START +
					HDMI_STATUS_WR_AVAIL, 1);
	if (status == 0) {
		DBGMSG("[HDMI] *E* restart\n");
		err_cnt++;
		goto ddc_read_fail;
	}

	/* Slave Address + 1 */
#ifdef HDMI_DDC_OUT
	vppif_reg32_out(REG_HDMI_I2C_CTRL2, 0x400A1);
#else
	vppif_reg32_write(HDMI_WR_DATA, addr + 1);
	udelay(HDMI_DDC_DELAY);
	while (vppif_reg32_read(HDMI_WR_DATA) != (addr + 1))
		;
	udelay(HDMI_DDC_DELAY);
	vppif_reg32_write(HDMI_WR_DATA_AVAIL, 1); /* write data avail */
#endif
	udelay(HDMI_DDC_CTRL_DELAY);
	/* wait wr data avail */
	status = hdmi_DDC_check_status(HDMI_STATUS_WR_AVAIL, 1);
	if (status == 0) {
		DBGMSG("[HDMI] *E* slave addr 0x%x\n", addr + 1);
		err_cnt++;
		goto ddc_read_fail;
	}

	/* Read Data */
	for (i = 0; i < length; i++) {
#ifdef HDMI_DDC_OUT
		vppif_reg32_out(REG_HDMI_I2C_CTRL2, 0x40000);
#else
		vppif_reg32_write(HDMI_WR_DATA_AVAIL, 1); /* write data avail */
		/* hdmi_reg32_write(HDMI_WR_DATA_AVAIL,1); */
#endif
		udelay(HDMI_DDC_CTRL_DELAY);
		/* wait wr data avail */
		status = hdmi_DDC_check_status(HDMI_STATUS_WR_AVAIL, 1);
		if (status == 0) {
			DBGMSG("[HDMI] *E* wr ACK(%d)\n", i);
			err_cnt++;
			goto ddc_read_fail;
			/* break; */
		}

		/* wait sw read not set */
		status = hdmi_DDC_check_status(HDMI_STATUS_SW_READ, 0);
		if (status == 0) {
			DBGMSG("[HDMI] *E* read avail(%d)\n", i);
			if (i == 0) {
				err_cnt++;
				/* goto ddc_read_fail; */
			} else {
				/* g_vpp.dbg_hdmi_ddc_read_err++; */
			}
			goto ddc_read_fail;
			/* break; */
		}

		*buf++ = vppif_reg32_read(HDMI_RD_DATA);
		udelay(HDMI_DDC_DELAY);
#ifdef HDMI_DDC_OUT
		vppif_reg32_out(REG_HDMI_I2C_CTRL2, 0x2000000);
#else
		vppif_reg32_write(HDMI_SW_READ, 1);
#endif
		udelay(HDMI_DDC_DELAY);
	}

	/* STOP */
#if 0
	vppif_reg32_write(HDMI_SW_STOP_REQ, 1); /* sw stop */
	vppif_reg32_write(HDMI_WR_DATA_AVAIL, 1); /* write data avail */
#endif
	/* sw stop, write data avail */
	vppif_reg32_write(REG_HDMI_I2C_CTRL2, BIT18 + BIT17, 17, 3);
	udelay(HDMI_DDC_CTRL_DELAY);
	/* wait start & wr data avail */
	status = hdmi_DDC_check_status(HDMI_STATUS_STOP +
				HDMI_STATUS_WR_AVAIL + HDMI_STATUS_CP_USE, 1);
	if (status == 0) {
		DBGMSG("[HDMI] *E* stop\n");
		err_cnt++;
		goto ddc_read_fail;
	}
	udelay(HDMI_DDC_DELAY);

ddc_read_fail:
	if (err_cnt)
		DBGMSG("[HDMI] *E* read DDC %d\n", err_cnt);
	return (err_cnt) ? 1 : 0;
}

int hdmi_DDC_read(char addr, int index, char *buf, int length)
{
	int retry = 3;
	do {
		if (hdmi_DDC_read_func(addr, index, buf, length) == 0)
			break;
		hdmi_DDC_reset();
		DPRINT("[HDMI] *W* DDC reset\n");
		retry--;
	} while (retry);
	return (retry == 0) ? 1 : 0;
}

void hdmi_audio_enable(vpp_flag_t enable)
{
	vppif_reg32_write(HDMI_AUD_ENABLE, enable);
}

void hdmi_audio_mute(vpp_flag_t enable)
{
	vppif_reg32_write(HDMI_AUD_MUTE, enable);
}

/*----------------------- HDMI API --------------------------------------*/
void hdmi_write_packet(unsigned int header, unsigned char *packet,
				int cnt)
{
	unsigned char buf[36];
	int i;
	enum hdmi_fifo_slot_t no;

#ifdef CONFIG_HDMI_INFOFRAME_DISABLE
	return;
#endif
	memcpy(&buf[0], &header, 3);
	buf[3] = hdmi_ecc((unsigned char *)&header, 24);
	for (i = 0; i < cnt / 7; i++) {
		memcpy(&buf[4+8*i], &packet[7*i], 7);
		buf[11+8*i] = hdmi_ecc(&packet[7*i], 56);
	}

	switch (header & 0xFF) {
	case HDMI_PACKET_INFOFRAME_AVI:
		no = HDMI_FIFO_SLOT_AVI;
		break;
	case HDMI_PACKET_INFOFRAME_AUDIO:
		no = HDMI_FIFO_SLOT_AUDIO;
		break;
	case HDMI_PACKET_INFOFRAME_VENDOR:
		no = HDMI_FIFO_SLOT_VENDOR;
		break;
	default:
		no = HDMI_FIFO_SLOT_CONTROL;
		break;
	}
	hdmi_write_fifo(no, (unsigned int *)buf, (4 + 8 * (cnt / 7)));
}

void hdmi_tx_null_packet(void)
{
	hdmi_write_packet(HDMI_PACKET_NULL, 0, 0);
}

void hdmi_tx_general_control_packet(int mute)
{
	unsigned char buf[7];
	memset(buf, 0x0, 7);
	buf[0] = (mute) ? 0x01 : 0x10;
	buf[1] = HDMI_COLOR_DEPTH_24 | (HDMI_PHASE_4 << 4);
	hdmi_write_packet(HDMI_PACKET_GENERAL_CTRL, buf, 7);
}

int hdmi_get_pic_aspect(hdmi_video_code_t vic)
{
	switch (vic) {
	case HDMI_640x480p60_4x3:
	case HDMI_720x480p60_4x3:
	case HDMI_1440x480i60_4x3:
	case HDMI_1440x240p60_4x3:
	case HDMI_2880x480i60_4x3:
	case HDMI_2880x240p60_4x3:
	case HDMI_1440x480p60_4x3:
	case HDMI_720x576p50_4x3:
	case HDMI_1440x576i50_4x3:
	case HDMI_1440x288p50_4x3:
	case HDMI_2880x576i50_4x3:
	case HDMI_2880x288p50_4x3:
	case HDMI_1440x576p50_4x3:
		return HDMI_PIC_ASPECT_4_3;
	default:
		break;
	}
	return HDMI_PIC_ASPECT_16_9;
}

int hdmi_get_vic(int resx, int resy, int fps, int interlace)
{
	hdmi_vic_t info;
	int i;

	info.resx = resx;
	info.resy = resy;
	info.freq = fps;
	info.option = (interlace) ? HDMI_VIC_INTERLACE : HDMI_VIC_PROGRESS;
	info.option |= (vout_check_ratio_16_9(resx, resy)) ?
				HDMI_VIC_16x9 : HDMI_VIC_4x3;
	for (i = 0; i < HDMI_VIDEO_CODE_MAX; i++) {
		if (memcmp(&hdmi_vic_info[i], &info, sizeof(hdmi_vic_t)) == 0)
			return i;
	}
	return HDMI_UNKNOW;
}

void hdmi_tx_avi_infoframe_packet(vdo_color_fmt colfmt,
						hdmi_video_code_t vic)
{
	unsigned int header;
	unsigned char buf[28];
	unsigned char temp;

	memset(buf, 0x0, 28);
	header = HDMI_PACKET_INFOFRAME_AVI + (0x2 << 8) + (0x0d << 16);
	buf[1] = HDMI_SI_NO_DATA + (HDMI_BI_V_H_VALID << 2) +
				(HDMI_AF_INFO_NO_DATA << 4);
	switch (colfmt) {
	case VDO_COL_FMT_YUV422H:
	case VDO_COL_FMT_YUV422V:
		temp = HDMI_OUTPUT_YUV422;
		break;
	case VDO_COL_FMT_YUV444:
		temp = HDMI_OUTPUT_YUV444;
		break;
	case VDO_COL_FMT_ARGB:
	default:
		temp = HDMI_OUTPUT_RGB;
		break;
	}
	buf[1] += (temp << 5);
	buf[2] = HDMI_ASPECT_RATIO_PIC + (hdmi_get_pic_aspect(vic) << 4) +
		(HDMI_COLORIMETRY_ITU709 << 6);
	buf[3] = 0x84;
	buf[4] = vic;
	switch (vic) {
	case HDMI_1440x480i60_16x9:
	case HDMI_1440x576i50_16x9:
		buf[5] = HDMI_PIXEL_REP_2;
		break;
	default:
		buf[5] = HDMI_PIXEL_REP_NO;
		break;
	}
	buf[0] = hdmi_checksum((unsigned char *)&header, buf, 28);
	hdmi_write_packet(header, buf, 28);
}

void hdmi_tx_audio_infoframe_packet(int channel, int freq)
{
	unsigned int header;
	unsigned char buf[28];

	memset(buf, 0x0, 28);
	header = HDMI_PACKET_INFOFRAME_AUDIO + (0x1 << 8) + (0x0a << 16);
	buf[1] = (channel - 1) + (HDMI_AUD_TYPE_REF_STM << 4);
	buf[2] = 0x0; /* HDMI_AUD_SAMPLE_24 + (freq << 2); */
	buf[3] = 0x00;
	buf[4] = 0x0;
	buf[5] = 0x0; /* 0 db */
	buf[0] = hdmi_checksum((unsigned char *)&header, buf, 28);
	hdmi_write_packet(header, buf, 28);
}

void hdmi_tx_vendor_specific_infoframe_packet(void)
{
	unsigned int header;
	unsigned char buf[28];
	unsigned char structure_3d, meta_present;
	unsigned char hdmi_video_format;

	/* 0-No,1-1 byte param,2-3D format */
	hdmi_video_format = (g_vpp.hdmi_3d_type) ? 2 : 0;
	/* HDMI_3D_STRUCTURE_XXX; */
	structure_3d = (g_vpp.hdmi_3d_type == 1) ? 0 : g_vpp.hdmi_3d_type;
	meta_present = 0;

	memset(buf, 0x0, 28);
	header = HDMI_PACKET_INFOFRAME_VENDOR + (0x1 << 8) + (0xa << 16);
	buf[1] = 0x3;
	buf[2] = 0xC;
	buf[3] = 0x0;
	buf[4] = (hdmi_video_format << 5);
	buf[5] = (structure_3d << 4) + ((meta_present) ? 0x8 : 0x0);
	buf[6] = 0x0;	/* 3D_Ext_Data */
#if 0	/* metadata present */
	buf[7] = 0x0;	/* 3D_Metadata_type,3D_Metadata_Length(N) */
	buf[8] = 0x0;	/* 3D Metadata 1_N */
#endif
	buf[0] = hdmi_checksum((unsigned char *)&header, buf, 28);
	hdmi_write_packet(header, buf, 28);
}

/*
--> Added by howayhuo.
Some TV (example: TCL L19E09) will overclock if ACR ratio too large,
So we need decrease the ACR ratio for some special TV
*/
static void print_acr(void)
{
	int i;

	if(p_fix_acr_tv != NULL) {
		for(i = 0; i < MAX_ACR_TV_NUM; i++) {
			if(strlen(p_fix_acr_tv[i].vendor_name) == 0
				|| strlen(p_fix_acr_tv[i].monitor_name) == 0)
			break;

			if(i == 0)
				printk("ACR TV Name:\n");

			printk("  %s,%s\n", p_fix_acr_tv[i].vendor_name, p_fix_acr_tv[i].monitor_name);
		}
	}
}

static void acr_init(void)
{
	char buf[512] = {0};
	int buflen = 512;
	unsigned long val;
	int i, j, k, tv_num;
	int ret, to_save_vendor;
	tv_name_t tv_name;

	if(p_fix_acr_tv != NULL) {
		kfree(p_fix_acr_tv);
		p_fix_acr_tv = NULL;
	}

	if(wmt_getsyspara("wmt.acr.ratio", buf, &buflen) == 0) {
		ret = strict_strtoul(buf, 10, &val);
		if(ret) {
			printk("[HDMI] Wrong wmt.acr.ratio value: %s\n", buf);
			return;
		}
		if(val >= 0 && val < 0xFFFFF) // total 20 bits
			fixed_acr_ration_val = (int)val;
		else
			printk("[HDMI] Invalid Fixed ACR Ratio: %lu\n", val);
	}

	if(fixed_acr_ration_val == 0)
		return;

	/*
	For example: setenv wmt.acr.tv 'TCH,RTD2662;PHL,Philips 244E'
	*/
	if(wmt_getsyspara("wmt.acr.tv", buf, &buflen) != 0) {
		p_fix_acr_tv = (tv_name_t *)kzalloc(sizeof(fix_acr_tv_list) + sizeof(tv_name_t), GFP_KERNEL);
		if(p_fix_acr_tv) {
			memcpy(p_fix_acr_tv, fix_acr_tv_list, sizeof(fix_acr_tv_list));
			print_acr();
		} else
			printk("[HDMI] malloc for ACR fail. malloc len = %d\n",
				sizeof(fix_acr_tv_list) + sizeof(tv_name_t));

		return;
	}

	tv_num = 0;
	buflen = strlen(buf);
	if(buflen == 0)
		return;

	if(buflen == sizeof(buf)) {
		printk("[HDMI] wmt.acr.tv too long\n");
		return;
	}

	for(i = 0; i < buflen; i++) {
		if(buf[i] == ',')
			tv_num++;
	}

	/*
	Limit TV Number
	*/
	if(tv_num > MAX_ACR_TV_NUM)
		tv_num = MAX_ACR_TV_NUM;

	if(tv_num == 0)
		return;

	printk("acr_tv_num = %d\n", tv_num);
	p_fix_acr_tv = (tv_name_t *)kzalloc((tv_num + 1) * sizeof(tv_name_t), GFP_KERNEL);
	if(!p_fix_acr_tv) {
		printk("[HDMI] malloc for ACR fail. malloc len = %d\n",
				sizeof(fix_acr_tv_list) + sizeof(tv_name_t));
		return;
	}
	memset(&tv_name, 0, sizeof(tv_name_t));

	j = 0;
	k = 0;
	to_save_vendor= 1;
	for(i = 0; i < buflen + 1; i++) {
		if(buf[i] != ',' && buf[i] != ';' && buf[i] != '\0') {
			if(to_save_vendor) {
				if(k < VENDOR_NAME_LEN)
					tv_name.vendor_name[k] = buf[i];
			} else {
				if(k < MONITOR_NAME_LEN)
					tv_name.monitor_name[k] = buf[i];
			}
			k++;
		} else if(buf[i] == ',') {
			to_save_vendor = 0;
			k = 0;
		} else {
			if(strlen(tv_name.vendor_name) == 0 || strlen(tv_name.monitor_name) == 0) {
				printk("[HDMI] Wrong wmt.acr.tv format\n");
				kfree(p_fix_acr_tv);
				p_fix_acr_tv = NULL;
				break;
			} else {
				if(j < tv_num) {
					memcpy(p_fix_acr_tv + j, &tv_name, sizeof(tv_name_t));
					memset(&tv_name, 0, sizeof(tv_name_t));
					j++;
				}

				if(j == tv_num)
					break;
			}

			if(buf[i]== ';') {
				to_save_vendor = 1;
				k = 0;
			} else
				break;
		}
	}

	print_acr();
}

void acr_exit(void)
{
	if(p_fix_acr_tv != NULL) {
		kfree(p_fix_acr_tv);
		p_fix_acr_tv = NULL;
	}
}

static int use_fix_acr_ratio(void)
{
	int i;

	if(fixed_acr_ration_val == 0 || p_fix_acr_tv == NULL)
		return 0;

	for(i = 0; i < MAX_ACR_TV_NUM; i++) {
		if(strlen(p_fix_acr_tv[i].vendor_name) == 0
			|| strlen(p_fix_acr_tv[i].monitor_name) == 0)
			break;

		if(!strcmp(edid_parsed.tv_name.vendor_name, p_fix_acr_tv[i].vendor_name)
			&& !strcmp(edid_parsed.tv_name.monitor_name, p_fix_acr_tv[i].monitor_name)) {
			printk("TV is \"%s %s\". Use fixed HDMI ACR Ratio: %d\n",
				edid_parsed.tv_name.vendor_name,
				edid_parsed.tv_name.monitor_name,
				fixed_acr_ration_val);
			return 1;
		}
	}

	return 0;
}
/*
<-- end added by howayhuo
*/

void hdmi_set_audio_n_cts(unsigned int freq)
{
	unsigned int n, cts;

	n = 128 * freq / 1000;
#ifdef __KERNEL__
{
	unsigned int tmp;
	unsigned int pll_clk;

	pll_clk = auto_pll_divisor(DEV_I2S, GET_FREQ, 0, 0);
	tmp = (vppif_reg32_in(AUDREGF_BASE_ADDR+0x70) & 0xF);

	switch (tmp) {
	case 0 ... 4:
		tmp = 0x01 << tmp;
		break;
	case 9 ... 12:
		tmp = 3 * (0x1 << (tmp-9));
		break;
	default:
		tmp = 1;
		break;
	}

	{
	unsigned long long tmp2;
	unsigned long long div2;
	unsigned long mod;

	tmp2 = g_vpp.hdmi_pixel_clock;
	tmp2 = tmp2 * n * tmp;
	div2 = pll_clk;
	mod = do_div(tmp2, div2);
	cts = tmp2;
	}
	DBGMSG("[HDMI] i2s %d,cts %d,reg 0x%x\n", pll_clk, cts,
		vppif_reg32_in(AUDREGF_BASE_ADDR + 0x70));
}

	vppif_reg32_write(HDMI_AUD_N_20BITS, n);
	if(use_fix_acr_ratio())
		vppif_reg32_write(HDMI_AUD_ACR_RATIO, fixed_acr_ration_val);
	else
		vppif_reg32_write(HDMI_AUD_ACR_RATIO, cts - 1);
#else
#if 1
	cts = g_vpp.hdmi_pixel_clock / 1000;
#else
	cts = vpp_get_base_clock(VPP_MOD_GOVRH) / 1000;
#endif
	vppif_reg32_write(HDMI_AUD_N_20BITS, n);
	if(use_fix_acr_ratio())
		vppif_reg32_write(HDMI_AUD_ACR_RATIO, fixed_acr_ration_val);
	else
		vppif_reg32_write(HDMI_AUD_ACR_RATIO, cts - 2);
#endif

#if 1	/* auto detect CTS */
	vppif_reg32_write(HDMI_AUD_CTS_SELECT, 0);
	cts = 0;
#else
	vppif_reg32_write(HDMI_AUD_CTS_SELECT, 1);
#endif
	vppif_reg32_write(HDMI_AUD_CTS_LOW_12BITS, cts & 0xFFF);
	vppif_reg32_write(HDMI_AUD_CTS_HI_8BITS, (cts & 0xFF000) >> 12);

	DBGMSG("[HDMI] set audio freq %d,n %d,cts %d,tmds %d\n",
				freq, n, cts, g_vpp.hdmi_pixel_clock);
}
void hdmi_config_audio(vout_audio_t *info)
{
	unsigned int freq;

	hdmi_info.channel = info->channel;
	hdmi_info.freq = info->sample_rate;

	/* enable ARF & ARFP clock */
	REG32_VAL(PM_CTRL_BASE_ADDR + 0x254) |= (BIT4 | BIT3);
	hdmi_tx_audio_infoframe_packet(info->channel - 1, info->sample_rate);
	hdmi_audio_enable(VPP_FLAG_DISABLE);
	vppif_reg32_write(HDMI_AUD_LAYOUT, (info->channel == 8) ? 1 : 0);
	vppif_reg32_write(HDMI_AUD_2CH_ECO, 1);

	switch (info->sample_rate) {
	case 32000:
		freq = 0x3;
		break;
	case 44100:
		freq = 0x0;
		break;
	case 88200:
		freq = 0x8;
		break;
	case 176400:
		freq = 0xC;
		break;
	default:
	case 48000:
		freq = 0x2;
		break;
	case 96000:
		freq = 0xA;
		break;
	case 192000:
		freq = 0xE;
		break;
	case 768000:
		freq = 0x9;
		break;
	}
	vppif_reg32_out(REG_HDMI_AUD_CHAN_STATUS0, (freq << 24) + 0x4);
	vppif_reg32_out(REG_HDMI_AUD_CHAN_STATUS1, 0x0);
	vppif_reg32_out(REG_HDMI_AUD_CHAN_STATUS2, 0xb);
	vppif_reg32_out(REG_HDMI_AUD_CHAN_STATUS3, 0x0);
	vppif_reg32_out(REG_HDMI_AUD_CHAN_STATUS4, 0x0);
	vppif_reg32_out(REG_HDMI_AUD_CHAN_STATUS5, 0x0);

	hdmi_set_audio_n_cts(info->sample_rate);
	vppif_reg32_write(HDMI_AUD_ACR_ENABLE, VPP_FLAG_ENABLE);
	vppif_reg32_write(HDMI_AUD_AIPCLK_RATE, 0);
        hdmi_audio_enable(hdmi_get_plugin() ?
                VPP_FLAG_ENABLE : VPP_FLAG_DISABLE);

}

void hdmi_config_video(hdmi_info_t *info)
{
	hdmi_set_output_colfmt(info->outfmt);
	hdmi_tx_avi_infoframe_packet(info->outfmt, info->vic);
	hdmi_tx_vendor_specific_infoframe_packet();
}

void hdmi_set_option(unsigned int option)
{
	vdo_color_fmt colfmt;
	int temp;

	hdmi_set_dvi_enable((option & EDID_OPT_HDMI) ?
		VPP_FLAG_DISABLE : VPP_FLAG_ENABLE);
	hdmi_audio_enable((option & EDID_OPT_AUDIO) ?
		VPP_FLAG_ENABLE : VPP_FLAG_DISABLE);

	colfmt = hdmi_get_output_colfmt();
	switch (colfmt) {
	case VDO_COL_FMT_YUV422H:
		temp = option & EDID_OPT_YUV422;
		break;
	case VDO_COL_FMT_YUV444:
		temp = option & EDID_OPT_YUV444;
		break;
	default:
		temp = 1;
		break;
	}
	if (temp == 0) {
		hdmi_set_output_colfmt(VDO_COL_FMT_ARGB);
		DBG_MSG("[HDMI] TV not support %s,use default RGB\n",
			vpp_colfmt_str[colfmt]);
	}
	DBG_MSG("[HDMI] set option(8-HDMI,6-AUDIO) 0x%x\n", option);
}

void hdmi_config(hdmi_info_t *info)
{
	vout_audio_t audio_info;
	int h_porch;
	int delay_cfg;
	vpp_clock_t clock;

	vppif_reg32_write(HDMI_HDEN, 0);
	vppif_reg32_write(HDMI_INFOFRAME_SELECT, 0);
	vppif_reg32_write(HDMI_INFOFRAME_FIFO1_RDY, 0);
	hdmi_config_video(info);

	govrh_get_tg(p_govrh, &clock);
	h_porch = clock.total_pixel_of_line - clock.end_pixel_of_active; /*fp*/
	delay_cfg = 47 - h_porch;
	if (delay_cfg <= 0)
		delay_cfg = 1;
	h_porch = clock.begin_pixel_of_active;	/* bp */
	h_porch = (h_porch - (delay_cfg + 1) - 26) / 32;
	if (h_porch <= 0)
		h_porch = 1;
	if (h_porch >= 8)
		h_porch = 0;
	hdmi_reg32_write(HDMI_CP_DELAY, delay_cfg);
	vppif_reg32_write(HDMI_HORIZ_BLANK_MAX_PCK, h_porch);
	DBGMSG("[HDMI] H blank max pck %d,delay %d\n", h_porch, delay_cfg);

	audio_info.fmt = 16;
	audio_info.channel = info->channel;
	audio_info.sample_rate = info->freq;
	hdmi_config_audio(&audio_info);

	vppif_reg32_write(HDMI_INFOFRAME_FIFO1_ADDR, 0);
	vppif_reg32_write(HDMI_INFOFRAME_FIFO1_LEN, 2);
	vppif_reg32_write(HDMI_INFOFRAME_FIFO1_RDY, 1);

	hdmi_set_option(info->option);
	vppif_reg32_write(HDMI_TRE_EN,
		(g_vpp.hdmi_pixel_clock < 40000000) ? 3 : 2);
}

/*----------------------- Module API --------------------------------------*/
void hdmi_set_cp_enable(vpp_flag_t enable)
{
	if (!g_vpp.hdmi_cp_enable)
		enable = 0;

	if (hdmi_cp)
		hdmi_cp->enable(enable);

#ifdef __KERNEL__
	if (hdmi_cp && hdmi_cp->poll) {
		vpp_irqproc_del_work(VPP_INT_GOVRH_VBIS, (void *)hdmi_cp->poll);
		if (enable)
			vpp_irqproc_work(VPP_INT_GOVRH_VBIS,
				(void *)hdmi_cp->poll, 0, 0, 0);
	}
#endif
}

int hdmi_check_cp_int(void)
{
	int ret = 0;

	if (hdmi_cp)
		ret = hdmi_cp->interrupt();
	return ret;
}

void hdmi_get_bksv(unsigned int *bksv)
{
	if (hdmi_cp)
		hdmi_cp->get_bksv(bksv);
}

#ifdef __KERNEL__
void hdmi_hotplug_notify(int plug_status)
{
		   if (g_vpp.hdmi_disable)
			     return;
		   if (g_vpp.virtual_display || (g_vpp.dual_display == 0)) {
			     vpp_netlink_notify_plug(VPP_VOUT_ALL, 0);
			     vpp_netlink_notify_plug(VPP_VOUT_ALL, 1);
			     return;
		   }
		   vpp_netlink_notify_plug(VPP_VOUT_NUM_HDMI, plug_status);
}
#else
#define hdmi_hotplug_notify
#endif

int hdmi_check_plugin(int hotplug)
{
           static int last_plugin = -1;
           int plugin;
           int flag;

           if (g_vpp.hdmi_disable)
                     return 0;

           plugin = hdmi_get_plugin();
           hdmi_clear_plug_status();
#ifdef __KERNEL__
           /* disable HDMI before change clock */
           if (plugin == 0) {
                     hdmi_set_enable(0);
                     hdmi_set_power_down(1);
           }
           vpp_set_clock_enable(DEV_HDMII2C, plugin, 1);
           vpp_set_clock_enable(DEV_HDCE, plugin, 1);

           /* slow down clock for plugout */
           flag = (auto_pll_divisor(DEV_HDMILVDS, GET_FREQ, 0, 0)
                                == 8000000) ? 0 : 1;
           if ((plugin != flag) && !g_vpp.virtual_display) {
                     int pixclk;

                     pixclk = (plugin) ? g_vpp.hdmi_pixel_clock : 8000000;
                     auto_pll_divisor(DEV_HDMILVDS, SET_PLLDIV, 0, pixclk);
           }
#endif
           if (last_plugin != plugin) {
                     DPRINT("[HDMI] HDMI plug%s,hotplug %d\n", (plugin) ?
                                                                "in" : "out", hotplug);
                     last_plugin = plugin;
           }
#if 0   /* Denzel test */
           if (plugin == 0)
                     hdmi_set_dvi_enable(VPP_FLAG_ENABLE);
#endif
           return plugin;
}

void hdmi_reg_dump(void)
{
	DPRINT("========== HDMI register dump ==========\n");
	vpp_reg_dump(REG_HDMI_BEGIN, REG_HDMI_END - REG_HDMI_BEGIN);
	vpp_reg_dump(REG_HDMI2_BEGIN, REG_HDMI2_END - REG_HDMI2_BEGIN);

	DPRINT("---------- HDMI common ----------\n");
	DPRINT("enable %d,hden %d,reset %d,dvi %d\n",
		vppif_reg32_read(HDMI_ENABLE), vppif_reg32_read(HDMI_HDEN),
		vppif_reg32_read(HDMI_RESET),
		vppif_reg32_read(HDMI_DVI_MODE_ENABLE));
	DPRINT("colfmt %d,conv 422 %d,hsync low %d,vsync low %d\n",
		vppif_reg32_read(HDMI_OUTPUT_FORMAT),
		vppif_reg32_read(HDMI_CONVERT_YUV422),
		vppif_reg32_read(HDMI_HSYNC_LOW_ACTIVE),
		vppif_reg32_read(HDMI_VSYNC_LOW_ACTIVE));
	DPRINT("dbg bus sel %d,state mach %d\n",
		vppif_reg32_read(HDMI_DBG_BUS_SELECT),
		vppif_reg32_read(HDMI_STATE_MACHINE_STATUS));
	DPRINT("eep reset %d,encode %d,eess %d\n",
		vppif_reg32_read(HDMI_EEPROM_RESET),
		vppif_reg32_read(HDMI_ENCODE_ENABLE),
		vppif_reg32_read(HDMI_EESS_ENABLE));
	DPRINT("verify pj %d,auth test %d,cipher %d\n",
		vppif_reg32_read(HDMI_VERIFY_PJ_ENABLE),
		vppif_reg32_read(HDMI_AUTH_TEST_KEY),
		vppif_reg32_read(HDMI_CIPHER_1_1));
	DPRINT("preamble %d\n", vppif_reg32_read(HDMI_PREAMBLE));

	DPRINT("---------- HDMI hotplug ----------\n");
	DPRINT("plug %s\n", vppif_reg32_read(HDMI_HOTPLUG_IN) ? "in" : "out");
	DPRINT("plug in enable %d, status %d\n",
		vppif_reg32_read(HDMI_HOTPLUG_IN_INT),
		vppif_reg32_read(HDMI_HOTPLUG_IN_STS));
	DPRINT("plug out enable %d, status %d\n",
		vppif_reg32_read(HDMI_HOTPLUG_OUT_INT),
		vppif_reg32_read(HDMI_HOTPLUG_OUT_STS));
	DPRINT("debounce detect %d,sample %d\n",
		vppif_reg32_read(HDMI_DEBOUNCE_DETECT),
		vppif_reg32_read(HDMI_DEBOUNCE_SAMPLE));

	DPRINT("---------- I2C ----------\n");
	DPRINT("enable %d,exit FSM %d,key read %d\n",
		vppif_reg32_read(HDMI_I2C_ENABLE),
		vppif_reg32_read(HDMI_FORCE_EXIT_FSM),
		vppif_reg32_read(HDMI_KEY_READ_WORD));
	DPRINT("clk divid %d,rd data 0x%x,wr data 0x%x\n",
		vppif_reg32_read(HDMI_I2C_CLK_DIVIDER),
		vppif_reg32_read(HDMI_RD_DATA),
		vppif_reg32_read(HDMI_WR_DATA));
	DPRINT("start %d,stop %d,wr avail %d\n",
		vppif_reg32_read(HDMI_SW_START_REQ),
		vppif_reg32_read(HDMI_SW_STOP_REQ),
		vppif_reg32_read(HDMI_WR_DATA_AVAIL));
	DPRINT("status %d,sw read %d,sw i2c req %d\n",
		vppif_reg32_read(HDMI_I2C_STATUS),
		vppif_reg32_read(HDMI_SW_READ),
		vppif_reg32_read(HDMI_SW_I2C_REQ));

	DPRINT("---------- AUDIO ----------\n");
	DPRINT("enable %d,sub pck %d,spflat %d\n",
		vppif_reg32_read(HDMI_AUD_ENABLE),
		vppif_reg32_read(HDMI_AUD_SUB_PACKET),
		vppif_reg32_read(HDMI_AUD_SPFLAT));
	DPRINT("aud pck insert reset %d,enable %d,delay %d\n",
		vppif_reg32_read(HDMI_AUD_PCK_INSERT_RESET),
		vppif_reg32_read(HDMI_AUD_PCK_INSERT_ENABLE),
		vppif_reg32_read(HDMI_AUD_INSERT_DELAY));
	DPRINT("avmute set %d,clr %d,pixel repete %d\n",
		vppif_reg32_read(HDMI_AVMUTE_SET_ENABLE),
		vppif_reg32_read(HDMI_AVMUTE_CLR_ENABLE),
		vppif_reg32_read(HDMI_AUD_PIXEL_REPETITION));
	DPRINT("acr ratio %d,acr enable %d,mute %d\n",
		vppif_reg32_read(HDMI_AUD_ACR_RATIO),
		vppif_reg32_read(HDMI_AUD_ACR_ENABLE),
		vppif_reg32_read(HDMI_AUD_MUTE));
	DPRINT("layout %d,pwr save %d,n 20bits %d\n",
		vppif_reg32_read(HDMI_AUD_LAYOUT),
		vppif_reg32_read(HDMI_AUD_PWR_SAVING),
		vppif_reg32_read(HDMI_AUD_N_20BITS));
	DPRINT("cts low 12 %d,hi 8 %d,cts sel %d\n",
		vppif_reg32_read(HDMI_AUD_CTS_LOW_12BITS),
		vppif_reg32_read(HDMI_AUD_CTS_HI_8BITS),
		vppif_reg32_read(HDMI_AUD_CTS_SELECT));
	DPRINT("aipclk rate %d\n", vppif_reg32_read(HDMI_AUD_AIPCLK_RATE));

	DPRINT("---------- INFOFRAME ----------\n");
	DPRINT("sel %d,hor blank pck %d\n",
		vppif_reg32_read(HDMI_INFOFRAME_SELECT),
		vppif_reg32_read(HDMI_HORIZ_BLANK_MAX_PCK));
	DPRINT("fifo1 ready %d,addr 0x%x,len %d\n",
		vppif_reg32_read(HDMI_INFOFRAME_FIFO1_RDY),
		vppif_reg32_read(HDMI_INFOFRAME_FIFO1_ADDR),
		vppif_reg32_read(HDMI_INFOFRAME_FIFO1_LEN));
	DPRINT("fifo2 ready %d,addr 0x%x,len %d\n",
		vppif_reg32_read(HDMI_INFOFRAME_FIFO2_RDY),
		vppif_reg32_read(HDMI_INFOFRAME_FIFO2_ADDR),
		vppif_reg32_read(HDMI_INFOFRAME_FIFO2_LEN));
	DPRINT("wr strobe %d,rd strobe %d,fifo addr %d\n",
		vppif_reg32_read(HDMI_INFOFRAME_WR_STROBE),
		vppif_reg32_read(HDMI_INFOFRAME_RD_STROBE),
		vppif_reg32_read(HDMI_INFOFRAME_FIFO_ADDR));

	{
	int i;
	unsigned int buf[32];

	for (i = 0; i <= vppif_reg32_read(HDMI_INFOFRAME_FIFO1_LEN); i++) {
		DPRINT("----- infoframe %d -----\n", i);
		hdmi_read_fifo(i, buf, 32);
		vpp_reg_dump((unsigned int) buf, 32);
	}
	}

	DPRINT("---------- HDMI test ----------\n");
	DPRINT("ch0 enable %d, data 0x%x\n",
		vppif_reg32_read(HDMI_CH0_TEST_MODE_ENABLE),
		vppif_reg32_read(HDMI_CH0_TEST_DATA));
	DPRINT("ch1 enable %d, data 0x%x\n",
		vppif_reg32_read(HDMI_CH1_TEST_MODE_ENABLE),
		vppif_reg32_read(HDMI_CH1_TEST_DATA));
	DPRINT("ch2 enable %d, data 0x%x\n",
		vppif_reg32_read(HDMI_CH2_TEST_MODE_ENABLE),
		vppif_reg32_read(HDMI_CH2_TEST_DATA));

	if (hdmi_cp)
		hdmi_cp->dump();
}

#ifdef CONFIG_PM
static unsigned int *hdmi_pm_bk;
static unsigned int *hdmi_pm_bk2;
static unsigned int hdmi_pm_enable;
static unsigned int hdmi_pm_enable2;
static int hdmi_plug_enable = 0xFF;
extern struct switch_dev vpp_sdev;
static int hdmi_resume_plug_cnt;
#define HDMI_RESUME_PLUG_MS	50
#define HDMI_RESUME_PLUG_CNT	20
static void hdmi_do_resume_plug(struct work_struct *ptr)
{
	vout_t *vo;
	int plugin;
	struct delayed_work *dwork = to_delayed_work(ptr);

	plugin = hdmi_check_plugin(0);
	vo = vout_get_entry(VPP_VOUT_NUM_HDMI);
	vout_change_status(vo, VPP_VOUT_STS_PLUGIN, plugin);
	if (plugin)
		hdmi_hotplug_notify(1);
	hdmi_resume_plug_cnt --;
	if (hdmi_resume_plug_cnt && (vpp_sdev.state == 0))
		schedule_delayed_work(dwork,
			msecs_to_jiffies(HDMI_RESUME_PLUG_MS));
}

DECLARE_DELAYED_WORK(hdmi_resume_work, hdmi_do_resume_plug);

void hdmi_suspend(int sts)
{
	vo_hdmi_set_clock(1);
	switch (sts) {
	case 0:	/* disable module */
		cancel_delayed_work_sync(&hdmi_resume_work);
		hdmi_pm_enable = vppif_reg32_read(HDMI_ENABLE);
		hdmi_reg32_write(HDMI_ENABLE, 0);
		hdmi_pm_enable2 = vppif_reg32_read(HDMI_HDEN);
		vppif_reg32_write(HDMI_HDEN, 0);
		if (hdmi_plug_enable == 0xFF)
			hdmi_plug_enable =
				vppif_reg32_read(HDMI_HOTPLUG_OUT_INT);
		hdmi_enable_plugin(0);
		break;
	case 1: /* disable tg */
		break;
	case 2:	/* backup register */
		hdmi_pm_bk = vpp_backup_reg(REG_HDMI_BEGIN,
			(REG_HDMI_END - REG_HDMI_BEGIN));
		hdmi_pm_bk2 = vpp_backup_reg(REG_HDMI2_BEGIN,
			(REG_HDMI2_END - REG_HDMI2_BEGIN));
	 	switch_set_state(&vpp_sdev, 0);
		hdmi_resume_plug_cnt = 20;
		break;
	default:
		break;
	}
	vo_hdmi_set_clock(0);
}

void hdmi_resume(int sts)
{
	vo_hdmi_set_clock(1);
	switch (sts) {
	case 0:	/* restore register */
		vpp_restore_reg(REG_HDMI_BEGIN,
			(REG_HDMI_END - REG_HDMI_BEGIN), hdmi_pm_bk);
		vpp_restore_reg(REG_HDMI2_BEGIN,
			(REG_HDMI2_END - REG_HDMI2_BEGIN), hdmi_pm_bk2);
		hdmi_pm_bk = 0;
		hdmi_pm_bk2 = 0;
		hdmi_config(&hdmi_info); /* re-config HDMI info frame */
		if (g_vpp.hdmi_cp_p && hdmi_cp)
			hdmi_cp->init();
		break;
	case 1:	/* enable module */
		hdmi_reg32_write(HDMI_ENABLE, hdmi_pm_enable);
		vppif_reg32_write(HDMI_HDEN, hdmi_pm_enable2);
		break;
	case 2: /* enable tg */
		hdmi_check_plugin(0);
		hdmi_clear_plug_status();
		hdmi_enable_plugin(hdmi_plug_enable);
		hdmi_plug_enable = 0xFF;
		if (vpp_sdev.state == 0) {
			hdmi_resume_plug_cnt = HDMI_RESUME_PLUG_CNT;
			schedule_delayed_work(&hdmi_resume_work,
				msecs_to_jiffies(HDMI_RESUME_PLUG_MS));
		}
		break;
	default:
		break;
	}
	vo_hdmi_set_clock(0);
}
#else
#define hdmi_suspend NULL
#define hdmi_resume NULL
#endif

void hdmi_init(void)
{
	struct fb_videomode vmode;

	g_vpp.hdmi_pixel_clock = vpp_get_base_clock(VPP_MOD_GOVRH);
	g_vpp.hdmi_i2c_freq = HDMI_I2C_FREQ;
	g_vpp.hdmi_i2c_udelay = 0;
	g_vpp.hdmi_ctrl = 0x1000000;
	g_vpp.hdmi_audio_pb4 = 0x0;
	g_vpp.hdmi_audio_pb1 = 0x0;

	hdmi_info.outfmt = hdmi_get_output_colfmt();
	govrh_get_videomode(p_govrh, &vmode);
	hdmi_info.vic = hdmi_get_vic(vmode.xres, vmode.yres, vmode.refresh,
		(vmode.vmode & FB_VMODE_INTERLACED) ? 1 : 0);
	hdmi_info.channel = 2;
	hdmi_info.freq = 48000;
	hdmi_info.option = EDID_OPT_AUDIO + EDID_OPT_HDMI;

	if (g_vpp.govrh_preinit) {
		DBGMSG("[HDMI] hdmi_init for uboot logo\n");
	} else {
		/* bit8-HDMI SDA,bit9-HDMI SCL,bit10-Hotplug,bit26-CEC */
		/* GPIO disable GPIO function */
		vppif_reg32_write(GPIO_BASE_ADDR+0x54, 0x4000700, 0, 0);
		/* GPIO4 disable GPIO out */
		vppif_reg32_write(GPIO_BASE_ADDR+0x494, 0x4000700, 0, 0);
#if 0
		/* Suspend GPIO output enable */
		vppif_reg32_write(GPIO_BASE_ADDR+0x80, BIT23, 23, 1);
		/* Suspend GPIO output high */
		vppif_reg32_write(GPIO_BASE_ADDR+0xC0, BIT23, 23, 1);
		/* Wake3 disable pull ctrl */
		vppif_reg32_write(GPIO_BASE_ADDR+0x480, BIT19, 19, 0);
#endif
		vppif_reg32_write(HDMI_REG_LEVEL, 1);
		vppif_reg32_write(HDMI_REG_UPDATE, 1);
		vppif_reg32_write(HDMI_LDI_SHIFT_LEFT, 1);
		vppif_reg32_out(REG_HDMI_STATUS, 0x0008c000);
		vppif_reg32_out(REG_HDMI_TEST, 0x00450409);
		vppif_reg32_out(REG_HDMI_TEST2, 0x00005022);
		vppif_reg32_out(REG_HDMI_TEST3,
			(g_vpp.hdmi_sp_mode) ? 0x00010100 : 0x00000100);
		hdmi_set_enable(VPP_FLAG_DISABLE);
		hdmi_set_dvi_enable(VPP_FLAG_DISABLE);
		vppif_reg32_write(HDMI_CIPHER_1_1, 0);

		vppif_reg32_write(HDMI_INFOFRAME_SRAM_ENABLE, 1);
		vppif_reg32_write(HDMI_INFOFRAME_SELECT, 0);
		vppif_reg32_write(HDMI_INFOFRAME_FIFO1_RDY, 0);

		vppif_reg32_out(HDMI_BASE_ADDR+0x3ec, 0x0);
		vppif_reg32_out(HDMI_BASE_ADDR+0x3e8, 0x1);

	/*	vppif_reg32_write(HDMI_AUD_LAYOUT, 1); */
		hdmi_DDC_reset();
		hdmi_DDC_set_freq(g_vpp.hdmi_i2c_freq);
		vppif_reg32_write(HDMI_I2C_ENABLE, 1);
	}
	g_vpp.hdmi_init = 1;
	if (hdmi_cp)
		hdmi_cp->init();

	acr_init();
}
#endif /* WMT_FTBLK_HDMI */
