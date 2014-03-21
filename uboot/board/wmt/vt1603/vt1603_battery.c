/*++
  Copyright (c) 2008  WonderMedia Technologies, Inc.

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

#include <common.h>
#include <command.h>
#include <linux/ctype.h>
#include <asm/arch/common_def.h>
#include <asm/errno.h>

#include "../include/wmt_pmc.h"
#include "../include/wmt_spi.h"
#include "../include/wmt_clk.h"
#include "../include/wmt_gpio.h"

#include "../wmt_battery.h"
#include "vt1603_battery.h"
#include "vt1603.h"

#define CONFIG_VT1603_BATTERY

//#define BAT_DEBUG

#undef dbg
#undef dbg_err
#ifdef  BAT_DEBUG
#define dbg(fmt, args...) printf("[%s]_%d: " fmt, __func__ , __LINE__, ## args)
#else
#define dbg(fmt, args...)
#endif

#define dbg_err(fmt, args...) printf("## VT1603A/VT1609 BAT##: " fmt,  ## args)

extern int auto_pll_divisor(enum dev_id dev, enum clk_cmd cmd, int unit, int freq);

static int vt1603_spi_write(u8 addr, const u8 data)
{
	u8 wbuf[3], rbuf[3];

	wbuf[0] = ((addr & 0xFF) | BIT7);
	wbuf[1] = ((addr & 0xFF) >> 7);
	wbuf[2] = data;

	spi_write_then_read_data(wbuf, rbuf, sizeof(wbuf),
				 SPI_MODE_3, 0);

	udelay(10);
	return 0;
}

static int vt1603_spi_read(u8 addr, u8 *data)
{
	u8 wbuf[5] = {0};
	u8 rbuf[5] = {0};

	memset(wbuf,0,sizeof(wbuf));
	memset(rbuf,0,sizeof(rbuf));

	wbuf[0] = ((addr & 0xFF) & (~BIT7));
	wbuf[1] = ((addr & 0xFF) >> 7);

	spi_write_then_read_data(wbuf, rbuf, sizeof(wbuf),
				 SPI_MODE_3, 0);

	if (0) {
		int i;
		for (i = 0; i < sizeof(rbuf); i++)
			printf("0x%02x ", rbuf[i]);
		printf("\n");
	}
	data[0] = rbuf[4];
	return 0;
}

/*
 * vt1603_set_reg8 - set register value of vt1603
 * @bat_drv: vt1603 driver data
 * @reg: vt1603 register address
 * @val: value register will be set
 */
static int vt1603_set_reg8(u8 reg, u8 val)
{
	int ret =0;

	ret = vt1603_spi_write(reg,val);
	if(ret < 0){
		dbg_err("vt1603 battery write error, errno%d\n", ret);
		return ret;
	}

	return 0;
}

/*
 * vt1603_get_reg8 - get register value of vt1603
 * @bat_drv: vt1603 driver data
 * @reg: vt1603 register address
 */
static u8 vt1603_get_reg8(u8 reg)
{
	u8 val = 0;
	int ret = 0;

	ret = vt1603_spi_read(reg,&val);
	if (ret < 0){
		dbg_err("vt1603 battery read error, errno%d\n", ret);
		return 0;
	}

	return val;
}


static int vt1603_read8(u8 reg,u8* data)
{
	int ret = 0;

	ret = vt1603_spi_read(reg,data);
	if (ret < 0){
		dbg_err("vt1603 battery read error, errno%d\n", ret);
		return ret;
	}

	return 0;
}

/*
 * vt1603_setbits - write bit1 to related register's bit
 * @bat_drv: vt1603 battery driver data
 * @reg: vt1603 register address
 * @mask: bit setting mask
 */
static void vt1603_setbits(u8 reg, u8 mask)
{
	u8 tmp = vt1603_get_reg8(reg) | mask;
	vt1603_set_reg8(reg, tmp);
}

/*
 * vt1603_clrbits - write bit0 to related register's bit
 * @bat_drv: vt1603 battery driver data
 * @reg: vt1603 register address
 * @mask:bit setting mask
 */
static void vt1603_clrbits(u8 reg, u8 mask)
{
	u8 tmp = vt1603_get_reg8(reg) & (~mask);
	vt1603_set_reg8(reg, tmp);
}

/*
 * vt1603_reg_dump - dubug function, for dump vt1603 related registers
 * @bat_drv: vt1603 battery driver data
 */
static void __attribute__((unused)) vt1603_reg_dump(u8 addr, int len)
{
	u8 i;
	for (i = addr; i < addr + len; i += 2)
		printf("reg[%d]:0x%02X,  reg[%d]:0x%02X\n",
		    i, vt1603_get_reg8(i),
		    i + 1, vt1603_get_reg8(i + 1));
}

/*
 * vt1603_get_bat_convert_data - get battery converted data
 * @bat_drv: vt1603 battery driver data
 */
static int vt1603_get_bat_data(int *data)
{
	u8 data_l, data_h;

	if (vt1603_read8(VT1603_DATL_REG, &data_l)) {
		dbg("read VT1603_DATL_REG error!\n");
		return -1;
	}

	if (vt1603_read8(VT1603_DATH_REG, &data_h)) {
		dbg("red VT1603_DATH_REG error!\n");
		return -1;
	}

	*data = ADC_DATA(data_l, data_h);
	return 0;
}

/*
 * vt1603_work_mode_switch - switch VT1603 to battery mode
 * @bat_drv: vt1603 battery driver data
 */
static void vt1603_switch_to_bat_mode(void)
{
	dbg("Enter\n");
	vt1603_set_reg8(VT1603_CR_REG, 0x00);
	vt1603_set_reg8(VT1603_AMCR_REG, BIT0);
	dbg("Exit\n");
}

/*
 * vt1603_get_pen_state - get touch panel pen state from vt1603
 *         interrup status register
 * @bat_drv: vt1603 battery driver data
 */
static inline int vt1603_get_pen_state(void)
{
	u8 state = vt1603_get_reg8(VT1603_INTS_REG);
	return (((state & BIT4) == 0) ? TS_PENDOWN_STATE : TS_PENUP_STATE);
}

static inline void vt1603_bat_pen_manual(void)
{
	vt1603_setbits(VT1603_INTCR_REG, BIT7);
}

static void vt1603_bat_power_up(void)
{
	if (vt1603_get_reg8(VT1603_PWC_REG) != 0x08)
		vt1603_set_reg8(VT1603_PWC_REG, 0x08);

	return ;
}

static int vt1603_bat_avg(int *data, int num)
{
	int i = 0;
	int avg = 0;

	if(num == 0)
		return 0;

	for (i = 0; i < num; i++)
		avg += data[i];

	return (avg / num);
}

static int vt1603_read_adc(int *value)
{
	int timeout, i = 0;
	int bat_arrary[DFLT_BAT_VAL_AVG]={0};
	int ret = 0;

	dbg("Enter\n");

	// enable sar-adc power and clock
	vt1603_bat_power_up();
	// enable pen down/up to avoid miss irq
	vt1603_bat_pen_manual();
	// switch vt1603 to battery detect mode
	vt1603_switch_to_bat_mode();
	// do conversion use battery manual mode
	vt1603_setbits(VT1603_INTS_REG, BIT0);
	vt1603_set_reg8(VT1603_CR_REG, BIT4);

	dbg("before VT1603_INTS_REG=%x\n", vt1603_get_reg8(VT1603_INTS_REG));

	for (i=0; i<DFLT_BAT_VAL_AVG; i++) {
		timeout = 2000;
		while(--timeout && (vt1603_get_reg8(VT1603_INTS_REG) & BIT0)==0)
			/* wait for irq */;

		if(timeout){
			ret = vt1603_get_bat_data(&bat_arrary[i]);
			if(ret < 0) {
				dbg_err("vt1603 get bat adc data Failed!\n");
				goto out;
			}

			vt1603_setbits(VT1603_INTS_REG, BIT0);
			vt1603_set_reg8(VT1603_CR_REG, BIT4);//start manual ADC mode
		} else {
			dbg_err("wait adc end timeout ?!\n");
			ret = -1;
			goto out;
		}
	}

	*value = vt1603_bat_avg(bat_arrary, DFLT_BAT_VAL_AVG);

out:
	vt1603_clrbits(VT1603_INTCR_REG, BIT7);
	vt1603_setbits(VT1603_INTS_REG, BIT0 | BIT3);
	vt1603_set_reg8(VT1603_CR_REG, BIT1);
	dbg("Exit\n\n\n");
	return ret;
}

/*
 * vt1603_get_bat_info - get battery status, API for wmt_battery.c
 */
int vt1603_read_voltage(void)
{
	int volt_mV;
	int rc;

	rc = vt1603_read_adc(&volt_mV);
	if (rc < 0) {
		printf("vt1603 read voltage failed\n");
		return -1;
	}

	printf("vt1603 read voltage: %d mV\n", volt_mV);
	return volt_mV;
}

/*
 * vt1603_ts_reset - reset vt1603, auto postition conversion mode,
 *     do self calibration if enable
 */
static void vt1603_ts_reset(void)
{
	/* power control enable */
	vt1603_set_reg8(VT1603_PWC_REG, 0x18);

	/* clock divider */
	vt1603_set_reg8(VT1603_CDPR_REG, 0x08);

	/* clean debug register,for some 2 layer PCB machine enter debug mode unexpected */
	vt1603_set_reg8(VT1603_DCR_REG, 0x00);


	vt1603_set_reg8(VT1603_INTEN_REG,  BIT1);//Just Enable pendown IRQ

	// auto position conversion mode and panel type config
	/*if (ts_pdata->panel_type== PANEL_TYPE_4WIRED)
	  vt1603_set_reg8(ts_drv, VT1603_CR_REG, BIT1);
	  else*/
	vt1603_set_reg8(VT1603_CR_REG, BIT1 | BIT0);

	// interrupt control, pen up/down detection enable
	vt1603_set_reg8(VT1603_INTCR_REG, 0xff);

	// mask other module interrupts
	vt1603_set_reg8(VT1603_IMASK_REG27, 0xff);
	vt1603_set_reg8(VT1603_IMASK_REG28, 0xFF);
	vt1603_set_reg8(VT1603_IMASK_REG29, 0xFF);
	/* reset headphone detect irq */
	vt1603_set_reg8(VT1603_IMASK_REG27, 0xfd);

	vt1603_setbits(VT1603_IPOL_REG33, BIT5);

	vt1603_set_reg8(VT1603_ISEL_REG36, 0x04);/* vt1603 gpio1 as IRQ output */
	// clear irq
	//vt1603_clr_ts_irq(0x0F);
	vt1603_setbits(VT1603_INTS_REG, 0x0F);
	dbg("ok...\n");

	return;
}

static inline void i2s_pin_config(void)
{
	/* disable GPIO and Pull Down mode */
	GPIO_CTRL_GP10_I2S_BYTE_VAL &= ~0xFF;
	GPIO_CTRL_GP11_I2S_BYTE_VAL &= ~(BIT0 | BIT1 | BIT2);

	PULL_EN_GP10_I2S_BYTE_VAL &= ~0xFF;
	PULL_EN_GP11_I2S_BYTE_VAL &= ~(BIT0 | BIT1 | BIT2);

	/* set to 2ch input, 2ch output */
	PIN_SHARING_SEL_4BYTE_VAL &= ~(BIT13 | BIT14 | BIT15 | BIT17 | BIT19 | BIT20 | BIT22);
	PIN_SHARING_SEL_4BYTE_VAL |= (BIT1 | BIT16 | BIT18 | BIT21);
}

static inline void i2s_clk_config(void)
{
	/* set to 11.288MHz */
	auto_pll_divisor(DEV_I2S, CLK_ENABLE , 0, 0);
	auto_pll_divisor(DEV_I2S, SET_PLLDIV, 1, 11288);

	/* Enable BIT4:ARFP clock, BIT3:ARF clock */
	PMCEU_VAL |= (BIT4 | BIT3);

	/* Enable BIT2:AUD clock */
	PMCE3_VAL |= BIT2;
}

static int vt1603_bat_probe(void)
{
	i2s_pin_config();
	i2s_clk_config();

	vt1603_ts_reset();

	//vt1603_reg_dump(0xc0, 12);

	return 0;
}

struct bias_data {
	int normal;
	int max;
};

static struct bias_data bias_adapter[10];

static int parse_bias_data(void)
{
	char *s;
	char *endp;
	int i;

	struct bias_data *bias = bias_adapter;

	s = getenv("wmt.io.bateff.adapter");
	if (!s) {
		printf("no wmt.io.bateff.adapter param!\n");
		return -EINVAL;
	}

	// parse from the valid val
	for (i = 9; i >= 0 ; i--) {
		bias[i].normal = simple_strtoul(s, &endp, 16);
		if (*endp == '\0')
			break;
		s = endp + 1;
		if (*s == '\0')
			break;

		bias[i].max = simple_strtoul(s, &endp, 16);
		if (*endp == '\0')
			break;
		s = endp + 1;
		if (*s == '\0')
			break;
	}

	if ((i != 0) && (i != 5)) {
		printf("Error to get charging u-boot argument.! i = %d\n", i);
		return -1;
	}

	return 0;
}

static int getEffect(int adc, int usage, struct bias_data *bias, int size)
{
	int effect, i;

	for (i = 0; i < size; i++) {
		if (adc <= bias[i].max) {
			effect = (bias[i].max - bias[i].normal) * usage / 100;
			return effect;
		}
	}

	effect = bias[size-1].max - bias[size-1].normal;

	if (size > 0)
		return effect * usage / 100;

	return 0;
}

static int adc_repair(int adc_val)
{
	int adc_add = adc_val;

	if (hw_is_dc_plugin()) {
		adc_add -= getEffect(adc_val, 100, bias_adapter, ARRAY_SIZE(bias_adapter));
	}

	return adc_add;
}

static int volt_grade[11];

static int parse_bat_level(void)
{
	char *s;
	char *endp;
	int i = 0;

	s = getenv("wmt.io.bat");
	if (!s) {
		printf("no wmt.io.bat param!\n");
		return -EINVAL;
	}

	// skip 5 val
	while ((i < 5) && (*s != '\0')) {
		if (*s==':') {
			i++;
		}
		s++;
	}

	if ((i != 5) || (!s) || (*s == '\0')) {
		printf("wmt.io.bat param format error!\n");
		return -1;
	}

	// parse from the valid val
	for (i = 10; i >= 0; i--) {
		volt_grade[i] = simple_strtoul(s, &endp, 16);
		if (*endp == '\0')
			break;
		s = endp + 1;

		if (*s == '\0')
			break;
	}

	if (i) {
		printf("parse wmt.io.bat failed!\n");
		return -EINVAL;
	}

	return 0;
}

static int adc_to_capacity(int adc_val)
{
	int capacity = 0;

	if (adc_val <= volt_grade[0]) {
		capacity = 0;
	} else if (adc_val >= volt_grade[10]) {
		capacity = 100;
	} else {
		int i;
		for (i = 10; i >= 1 ; i--) {
			if ((adc_val<=volt_grade[i])&&(adc_val>volt_grade[i-1])) {
				capacity = ((adc_val-volt_grade[i-1]) * 10) /
						(volt_grade[i]-volt_grade[i-1]) + (i-1)*10;
			}
		}
	}

	return capacity;
}

int vt1603_read_capacity(void)
{
	int adc0, adc1;
	int cap0;

	adc0 = vt1603_read_voltage();
	if (adc0 < 0) {
		printf("read raw adc failed\n");
		return adc0;
	}
	printf("adc0 %dmV\n",adc0);

	adc1 = adc_repair(adc0);
	printf("adc1 %dmV\n",adc1);

	cap0 = adc_to_capacity(adc1);
	printf("cap0 %d\n", cap0);
	return cap0;
}

int vt1603_batt_init(void)
{
	if (parse_bias_data())
		return -EINVAL;

	if (parse_bat_level())
		return -EINVAL;

	return vt1603_bat_probe();
}

extern int hw_is_dc_plugin(void);

int vt1603_check_bl(void)
{
	int rc;
	int volt_mV;
	int low_mV;

	rc = vt1603_read_adc(&volt_mV);
	if (rc < 0) {
		printf("vt1603 read voltage failed\n");
		return -EIO;
	}

	if (hw_is_dc_plugin())
		low_mV = adc_repair(volt_grade[0]);
	else {
	/* FIXME Work around for the situation that android poweroff by
	 * capcity 0, but uboot do not report low battery. */
		low_mV = (volt_grade[0] + volt_grade[1]) / 2;
	}

	printf("current voltage %d, low bat %d, capacity %d\n",
	       volt_mV, low_mV, vt1603_read_capacity());
	return volt_mV < low_mV;
}

static int do_adc(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	int i;

	parse_bias_data();
	parse_bat_level();

	for (i = 0; i < ARRAY_SIZE(bias_adapter); i++) {
		printf("bias adapater [%2d]: %d -> %d (%d) mV\n",
		       i, bias_adapter[i].normal, bias_adapter[i].max,
		       bias_adapter[i].max - bias_adapter[i].normal);
	}

	for (i = 0; i < ARRAY_SIZE(volt_grade); i++) {
		printf("voltage[%2d] - %d(%dmV)\n", i, volt_grade[i], (volt_grade[i] * 1047) / 1000);
	}

	return 0;
}

U_BOOT_CMD(
	adc,	4,	1,	do_adc,	\
	"adc: display battery capacity\n",	\
	"" \
);

struct bat_dev vt1603_battery_dev = {
		.name		= "vt1603",
		.is_gauge	= 0,
		.init		= vt1603_batt_init,
		.get_capacity	= vt1603_read_capacity,
		.get_voltage	= vt1603_read_voltage,
		.check_bl	= vt1603_check_bl,
};
