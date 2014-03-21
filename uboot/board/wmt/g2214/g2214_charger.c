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
//#include <asm/arch/common_def.h>
#include <asm/errno.h>
#include "../include/wmt_pmc.h"
#include "../include/wmt_spi.h"
#include "../include/wmt_clk.h"
#include "../include/wmt_gpio.h"
#include "../include/common_def.h"
#include "../wmt_battery.h"
#include "g2214_charger.h"

//#define BAT_DEBUG
#undef dbg
#undef dbg_err
#ifdef  BAT_DEBUG
#define dbg(fmt, args...) printf("[%s]_%d: " fmt, __func__ , __LINE__, ## args)
#else
#define dbg(fmt, args...)
#endif

#define dbg_err(fmt, args...) printf("## MPS/MPS CHG##: " fmt,  ## args)

extern struct charger_param charger_param;
extern int wmt_i2c_transfer(struct i2c_msg_s msgs[], int num, int adap_id);

struct charger_dev g2214_charger_dev;

static int g2214_read(u8 reg)
{
	int status;
	unsigned char ret;
	unsigned char data[2] = {
		reg,
		SECURITY_KEY,
	};

	struct i2c_msg_s msg_buf1[2] = {
			{	G2214_I2C_SLAVE_ADDRESS,	I2C_M_WR,	1,	&data[0],	},
			{	G2214_I2C_SLAVE_ADDRESS,	I2C_M_RD,	1,	&ret,		},
	};
	struct i2c_msg_s msg_buf2[2] = {
			{	G2214_I2C_SLAVE_ADDRESS,	I2C_M_WR,	2,	&data[0],	},
			{	G2214_I2C_SLAVE_ADDRESS,	I2C_M_RD,	1,	&ret,		},
	};

	if(reg < 0x80) {
		status = wmt_i2c_transfer(msg_buf1, 2, 3);
	} else {
		status = wmt_i2c_transfer(msg_buf2, 2, 3);
	}

	if(status > 0) {
		return ret;
	}
	return (-1);
}

static int g2214_write(u8 reg, int rt_value)
{
	int status;
	unsigned char data1[2] = {
		reg,
		rt_value,
	};
	unsigned char data2[3] = {
		reg,
		SECURITY_KEY,
		rt_value,
	};
	struct i2c_msg_s msg_buf[2] = {
		{	G2214_I2C_SLAVE_ADDRESS,	I2C_M_WR,	2,	&data1[0],	},
		{	G2214_I2C_SLAVE_ADDRESS,	I2C_M_WR,	3,	&data2[0],	}
	};

	if(reg < 0x80) {
		status = wmt_i2c_transfer(&msg_buf[0], 1,3);
	} else {
		status = wmt_i2c_transfer(&msg_buf[1], 1,3);
	}

	if(status > 0) {
		return (0);
	}
	return (-1);
}

static int g2214_reg_init(void)
{
	unsigned int tmp_data = 2 ;
	
	/*    Set safety time = 8hours */
       	tmp_data = g2214_read(REG_A6);
	tmp_data = tmp_data & (~(BIT4|BIT5|BIT6|BIT7));
	tmp_data = tmp_data | (gmt->safety_time<<4);
	g2214_write(REG_A6,tmp_data);
	

	if(gmt->no_batt == 0){
	        /*          Enable USB hardware mode     */
		REG8_VAL(USB_BASE_ADD+0x400+0x3F) = REG8_VAL(USB_BASE_ADD+0x400+0x3F) |  BIT5;
		udelay(100000);
		/*        Set battery charge voltage level    */	
	         g2214_write(REG_A8,(gmt->vseta << 6));   /* default value = 0x80  */
	        /*    Set ISET_DCIN & ISET_VBUS & ISETA current level */
		if(!gmt->otg_check ){ /*  Check OTG    */
			gmt->usb_otg_enabled = 1;
			gmt->current_state = USB_OTG;
		         g2214_write(REG_A8,((gmt->vseta << 6) | BIT3));
			printf("USB OTG Mode. \n");
	        }else if(g2214_read(REG_A10) & BIT2){  /*  DCIN  */
			gmt->current_state = DCIN;
			tmp_data = ((gmt->iset_dcin<<6) | (gmt->iset_vbus<<4) | gmt->resum_set.dcin);
			g2214_write(REG_A5,tmp_data);
			printf("DC Mode. \n");
	        }else if(gmt->usb_connected){ 
			gmt->usb_otg_enabled = 0;
			if(REG8_VAL(USB_BASE_ADD+0x400+0x22) & BIT6){ /*    Check adapter mode or pc mode    */
				gmt->current_state = USB_ADP;
				tmp_data = ((gmt->iset_dcin<<6) | (ADP_MODE<<4) | gmt->resum_set.otgadp);
				g2214_write(REG_A5,tmp_data);
			         g2214_write(REG_A8,((gmt->vseta << 6) & ( ~(BIT3))));
				printf("USB Adapter Mode. \n");
		        }else{
				gmt->current_state = USB_PC;
				tmp_data = ((gmt->iset_dcin<<6) | (PC_MODE<<4) | gmt->resum_set.otgpc);
				g2214_write(REG_A5,tmp_data);
			         g2214_write(REG_A8,((gmt->vseta << 6) & ( ~(BIT3))));
				printf("USB PC Mode. \n");
			}
		}else{ /*  N/A IN  */
			gmt->current_state = USB_PC;
			tmp_data = ((gmt->iset_dcin<<6) | (PC_MODE<<4) | gmt->resum_set.otgpc);
			g2214_write(REG_A5,tmp_data);
			printf("N/A power source. \n");
		}	
	        /*          Disable USB hardware mode     */
		REG8_VAL(USB_BASE_ADD+0x400+0x3F) = REG8_VAL(USB_BASE_ADD+0x400+0x3F) & (~( BIT5));
	}
	return 0;
}

static int bat_status_init(void)
{
	unsigned int g2214_status_01;
	unsigned int g2214_status_02;
	unsigned int tmp;

	g2214_write(REG_A9,0xFF);
	g2214_write(REG_A11,0xFF);
	tmp = g2214_read(REG_A0);
	if(tmp & BIT3){
	        tmp = tmp & (~BIT3);	
		g2214_write(REG_A0,tmp);
	}
	g2214_status_01 = g2214_read(REG_A10);
	g2214_status_02 = g2214_read(REG_A12);

	if(g2214_status_01 & 0x10){
		printf("No batt");
		gmt->no_batt = 1;
	}else	{     /* Check is no battery. */
		gmt->no_batt = 0;		
		if(g2214_status_01 & 0x0E){
	                if(g2214_status_01 & 0x08){
				gmt->batt_dead=1;
			}
			if(gmt->otg_check){
			        if((g2214_status_01 & (BIT1|BIT2)) && (g2214_status_01 & BIT2)){
					gmt->current_state = DCIN;
					gmt->mains_online = 1;
					gmt->usb_online = 0;
				}else if(g2214_status_01 & BIT1){
					gmt->current_state = USB_PC;
					gmt->usb_online =1;
					gmt->mains_online = 0;
				}
			}else{
				gmt->current_state = USB_PC;
				gmt->usb_online =0;
				gmt->mains_online = 0;
			}
		}else{
			gmt->batt_dead=0;
			gmt->usb_online =0;
			gmt->mains_online =0;
		}
		
		if (g2214_status_02 & 0x30){
	                if(((g2214_status_02 & 0x10)>>4) & (gmt->mains_online || gmt->usb_online)){
				gmt->is_fully_charged = 1;
			}
	                if(g2214_status_02 & 0x20){				
				printf("  Battery Low \n");
			}
		}else{
			gmt->is_fully_charged = 0;
			gmt->batt_low = 0;
		}
	}
        return 0;
}

static int g2214_hw_init(void)
{
	/*    Initial USB connect status,  */		
	gmt->usb_connected = REG8_VAL(USB_BASE_ADD+0x70D) & BIT0;
	gmt->otg_check = REG8_VAL(USB_BASE_ADD+0x7F2) & BIT0;

	bat_status_init();
	g2214_reg_init();

        return 0;
}

static int g2214_parse_param(void)
{
	static char UBOOT_ENV[] = "wmt.charger.param";
	struct charger_param *cp = &charger_param;
	char *env;
	int enable;
	int tmp[11] = {0};
	int i = 0;

	env = getenv(UBOOT_ENV);
	if (!env) {
		printf("please setenv %s\n", UBOOT_ENV);
		return -1;
	}

	/* [cable]:[pin]:[active] */
	{
		char *s;
		char *endp;

		s = env;
		enable = simple_strtol(s, &endp, 0);
		if (*endp == '\0')
			return -1;
		if (!enable)
			return -1;
		
		s = endp + 1;
		if(strncmp(s, "g2214", 5)) {
			printf("Can not find g2214 charger\n");
			return -ENODEV;
		}
		cp->chg_dev = &g2214_charger_dev; 

		if (!(s = strchr(s, ':')))
			return -EINVAL;
		s++;

		do {
			tmp[i++] = simple_strtol(s, &endp, 0);
			s = endp + 1;
		} while(*endp != '\0' && i < 11);

		gmt->type		= tmp[0];
		gmt->iset_dcin		= tmp[1];
		gmt->iset_vbus		= tmp[2];
		gmt->vseta		= tmp[3];
		gmt->sus.dcin		= tmp[4];
		gmt->sus.otgpc		= tmp[5];
		gmt->sus.otgadp		= tmp[6];
		gmt->resum.dcin		= tmp[7];
		gmt->resum.otgpc	= tmp[8];
		gmt->resum.otgadp	= tmp[9];
		gmt->safety_time	= tmp[10];

		printf("Fount g2214_charger, param- %d:%d:%d:%d:%d:%d:%d:%d:%d:%d:%d\n",
		       gmt->type,
		       gmt->iset_dcin,
		       gmt->iset_vbus,
		       gmt->vseta,
		       gmt->sus.dcin,
		       gmt->sus.otgpc,
		       gmt->sus.otgadp,
		       gmt->resum.dcin,
		       gmt->resum.otgpc,
		       gmt->resum.otgadp,
		       gmt->safety_time);
		/*        Unit : mA        */
		/*sscanf(s,"%d:%d:%d:%d:%d:%d:%d:%d:%d:%d:%d",
		       &gmt->type,
		       &gmt->iset_dcin,
		       &gmt->iset_vbus,
		       &gmt->vseta,
		       &gmt->sus.dcin,
		       &gmt->sus.otgpc,
		       &gmt->sus.otgadp,
		       &gmt->resum.dcin,
		       &gmt->resum.otgpc,
		       &gmt->resum.otgadp,
		       &gmt->safety_time);*/

		/*    Check DCIN current limit thershold   */
		if(gmt->iset_dcin > 2500 || gmt->iset_dcin < 1500)
			gmt->iset_dcin = 0;
		else if (gmt->iset_dcin < 2000)
			gmt->iset_dcin = 1;
		else if (gmt->iset_dcin < 2500)
			gmt->iset_dcin = 2;
		else if (gmt->iset_dcin == 2500)
			gmt->iset_dcin = 3;

		/*    Check VBUS current limit thershold   */
		if(gmt->iset_vbus > 1800 || gmt->iset_vbus <= 950)
			gmt->iset_vbus = 1;
		else if (gmt->iset_vbus < 1800)
			gmt->iset_vbus = 2;
		else if (gmt->iset_vbus == 1800)
			gmt->iset_vbus = 3;

		/*    Check battery charg voltage level   */
		if(gmt->vseta > 4350 || gmt->vseta < 4150)
			gmt->vseta = 0;
		else if (gmt->vseta < 4200)
			gmt->vseta = 1;
		else if (gmt->vseta < 4350)
			gmt->vseta = 2;
		else if (gmt->vseta == 4350)
			gmt->vseta = 3;

		/*    Check battery charge current level    */		
		if((gmt->sus.dcin <300) || (gmt->sus.dcin >1800)){
			gmt->sus_set.dcin = 2;
		}else{
			gmt->sus_set.dcin = ((gmt->sus.dcin-300) /100);
		};
		if((gmt->sus.otgpc <300) || (gmt->sus.otgpc >1800)){
			gmt->sus_set.otgpc = 2;
		}else{
			gmt->sus_set.otgpc = ((gmt->sus.otgpc-300) /100);
		};
		if((gmt->sus.otgadp <300) || (gmt->sus.otgadp >1800)){
			gmt->sus_set.otgadp = 2;
		}else{
			gmt->sus_set.otgadp = ((gmt->sus.otgadp-300) /100);
		};


		if((gmt->resum.dcin <300) || (gmt->resum.dcin >1800)){
			gmt->resum_set.dcin = 2;
		}else{
			gmt->resum_set.dcin = ((gmt->resum.dcin-300) /100); 
		};
		if((gmt->resum.otgpc <300) || (gmt->resum.otgpc >1800)){
			gmt->resum_set.otgpc = 2;
		}else{
			gmt->resum_set.otgpc = ((gmt->resum.otgpc-300) /100); 
		};
		if((gmt->resum.otgadp <300) || (gmt->resum.otgadp >1800)){
			gmt->resum_set.otgadp = 2;
		}else{
			gmt->resum_set.otgadp = ((gmt->resum.otgadp-300) /100); 
		};

		gmt->safety_time = gmt->safety_time -1;
		if(gmt->safety_time < 0) {
			gmt->safety_time = 0;
		}else if (gmt->safety_time > 16){
			gmt->safety_time = 15;
		}

		cp->id = gmt->type;

		cp->current_pin = -1;

		cp->current_sl = -1;

		//printf("Found charger %s: current_pin %d, small level %d\n",
		//       cp->id ? "micro-usb" : "ac", cp->current_pin, cp->current_sl);
	}

	return 0;
}

static int g2214_chg_init(void)
{
	g2214_write(REG_A9,0xFF);
	g2214_write(REG_A11,0xFF);
	g2214_write(REG_A12,0x00);

	return g2214_hw_init();
}

static int g2214_check_full(void)
{
	uint8_t a12 = g2214_read(REG_A12);
	return !!(a12 & 0x10);
}

static int g2214_set_small_current(void)
{
	// 400 mA for charging
	int tmp_data = ((0x2<<6) | (PC_MODE<<4) | 0x1);
	g2214_write(REG_A5,tmp_data);
	return 0;
}
	
static int g2214_set_large_current(void)
{
	// 800 mA for charging
	int tmp_data = ((0x2<<6) | (ADP_MODE<<4) | 0x5);
	g2214_write(REG_A5,tmp_data);
	return 0;
}

static int do_g2214(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	int i;

	for (i = 0; i <= 13; i++) {
		printf(" reg%02d 0x%02x\n", i, g2214_read(i));
	}

	return 0;
}

U_BOOT_CMD(
	g2214,	1,	1,	do_g2214,
	"g2214\n",
	NULL
);


// export for extern interface.
struct charger_dev g2214_charger_dev = {
	.name		= "g2214",
	.parse_param	= g2214_parse_param,
	.init		= g2214_chg_init,
	.check_full	= g2214_check_full,
	.set_small_current = g2214_set_small_current,
	.set_large_current = g2214_set_large_current,
};

