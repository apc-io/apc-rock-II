/*
 * Clobal Mixed-mode Technology GMT2144 Battery Charger Driver
 *
 * Copyright (C) 2011, Intel Corporation
 *
 * Authors: 
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/platform_device.h>
#include <linux/debugfs.h>
#include <linux/gpio.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/mutex.h>
#include <linux/power_supply.h>
#include <linux/seq_file.h>
#include <linux/delay.h>
#include <mach/hardware.h>
#include <linux/suspend.h>
#include <linux/syscore_ops.h>
#include <mach/gmt-core.h>
#include <linux/reboot.h>
#include "g2214_charger.h"
#include <linux/slab.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>

extern int wmt_getsyspara(char *varname, char *varval, int *varlen);
extern int wmt_usb_connected(void);

static int wmt_otg_power = 1;

static int parse_otg_power(void)
{
	uint8_t buf[64];
	size_t l = sizeof(buf);

	if (wmt_getsyspara("wmt.otg.power", buf, &l) == 0) {
		sscanf(buf, "%d", &wmt_otg_power);
	}

	return 0;
}

static int g2214_read(u8 reg)
{
	unsigned int rt_value =0;

	gmt2214_reg_read(gmt->gmt_dev, reg, &rt_value);

	return rt_value;
}

static int g2214_write( u8 reg, u8 val)
{
	int ret;

	ret = gmt2214_reg_write(gmt->gmt_dev, reg, val);

	return ret; 
}

static int env_parser(void)
{
	//char varname[] = "wmt.io.chg";
	char varname[] = "wmt.charger.param";
	int varlen = 128;
	char buf[varlen];
	char tmp_buf[varlen];
	int retval;
	int i = 0;
	int j = 0;

	retval = wmt_getsyspara(varname, buf, &varlen);
	if (retval == 0) {
		gmt->chgen = (buf[i] - '0' == 1)?1:0;
		if (gmt->chgen == 0) {        
			gmt->chgen  = 0;
			goto out;
		}
		i += 2;
		for (; i < varlen; ++i) {
			if (buf[i] == ':')
				break;
			tmp_buf[j] = buf[i];
			++j;
		}
		if (!(strncmp(tmp_buf,"g2214",5))) {
			gmt->g2214en = 1;
			gmt->g2214_debg_en = 0;

			if (!(strncmp(tmp_buf,"g2214k",6)))
				gmt->g2214_debg_en = 1;

		}else{
			gmt->g2214en = 0;
			goto out;
		}

		++i;

		/*        Unit : mA        */
		sscanf((buf+i),"%d:%d:%d:%d:%d:%d:%d:%d:%d:%d:%d",
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
		       &gmt->safety_time);

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
	}else{  
		gmt->chgen = 1;
		gmt->g2214en = 1;
		gmt->iset_dcin = 2;
		gmt->iset_vbus = 1;
		gmt->vseta = 2;
		gmt->sus_set.dcin = 2;
		gmt->sus_set.otgpc = 2;
		gmt->sus_set.otgadp = 2;
		gmt->resum_set.dcin = 2;
		gmt->resum_set.otgpc = 2;
		gmt->resum_set.otgadp = 2;
		gmt->safety_time = 0; 
	} ;

	return 0;
out:	
	return 1;	
}

#ifdef G2214_DUMP
static void g2214reg_dump(void)
{
}
#endif

static inline int g2214_irq_enable(void)
{
	pmc_enable_wakeup_isr(WKS_WK0,2);
	return 0;
}

static inline int g2214_irq_disable(void)
{
	pmc_disable_wakeup_isr(WKS_WK0);
	return 0;
}

static inline int g2214_irq_clear_status(void)
{
	pmc_clear_intr_status(WKS_WK0);
	return 0;
}

static inline int dcdet_irq_enable(void)
{
	pmc_enable_wakeup_isr(WKS_DCDET,4);
	return 0;
}

static inline int dcdet_irq_disable(void)
{
	pmc_disable_wakeup_isr(WKS_DCDET);
	return 0;
}

static inline int dcdet_irq_clear_status(void)
{
	pmc_clear_intr_status(WKS_DCDET);
	return 0;
}

static void g2214_chr_work(struct work_struct *work)
{
	int g2214_status_01;
	int g2214_status_02;
	int tmp_data = 0;

	//printk(" [%s]_%d\n",__func__,__LINE__);
	g2214_write(REG_A9,0xFF);
	g2214_write(REG_A11,0xFF);
	g2214_status_01 = g2214_read(REG_A10);
	g2214_status_02 = g2214_read(REG_A12);
	g2214_write(REG_A12,0x00);
	gmt->usb_connected = REG8_VAL(USB_BASE_ADD+0x70D) & BIT0;

	if (g2214_status_01 & 0x0E){
		if(g2214_status_01 & BIT0){
			printk(" Charg time out. \n");
			gmt->batt_dead=1;
		}
		//if(gmt->otg_check){
			if((g2214_status_01 & (BIT1|BIT2)) && (g2214_status_01 & BIT2)){
				printk(" DCIN Charging\n");
				//g2214_write(REG_A8,(gmt->vseta << 6));
				gmt->current_state = DCIN;
				gmt->mains_online = 1;
				gmt->usb_online = 0;
				tmp_data = ((gmt->iset_dcin<<6) | (PC_MODE<<4) | gmt->resum_set.dcin);
				g2214_write(REG_A5,tmp_data);
			}else if((g2214_status_01 & BIT1) && gmt->otg_check){

				//printk(" USB Charging\n");
				g2214_write(REG_A8,(gmt->vseta << 6));
				gmt->usb_online =1;
				gmt->mains_online = 0;
				if(gmt->usb_connected){
					/*    Check D+ & D- Status    */
					if(!wmt_usb_connected()){
						printk("  [Adapter charger.]\n");
						gmt->current_state = USB_ADP;
						tmp_data = ((gmt->iset_dcin<<6) | (ADP_MODE<<4) | gmt->resum_set.otgadp);
						g2214_write(REG_A5,tmp_data);
					}else{
						printk("  [PC charger.]\n");
						gmt->current_state = USB_PC;
						tmp_data = ((gmt->iset_dcin<<6) | (PC_MODE<<4) | gmt->resum_set.otgpc);
						g2214_write(REG_A5,tmp_data);
					}
				}else{
					printk("  [USB disconnect.]\n");
					gmt->current_state = USB_PC;
					tmp_data = ((gmt->iset_dcin<<6) | (PC_MODE<<4) | gmt->resum_set.otgpc);
					g2214_write(REG_A5,tmp_data);
				}
			}
		//}
#if 0
		else{
			printk(" USB OTG Discharging\n");
			g2214_write(REG_A8,((gmt->vseta << 6)|BIT3));
			gmt->current_state = USB_OTG;
			gmt->usb_online =0;
			gmt->mains_online = 0;
		}
#endif		
	}else{
		gmt->batt_dead=0;
		gmt->usb_online =0;
		gmt->mains_online =0;
		gmt->current_state = USB_PC;
		tmp_data = 1;
		//g2214_write(REG_A8,(gmt->vseta << 6));
		printk(" g2214_chr_work  All plug-out.\n");
	}

	if (g2214_status_02 & 0x30){
		if(((g2214_status_02 & 0x10)>>4) & (gmt->mains_online || gmt->usb_online)){
			gmt->is_fully_charged = 1;
			printk("  Charg Done\n");
		}else{
			gmt->is_fully_charged = 0;
		}
		if(g2214_status_02 & 0x20){				
			printk("  Battery Low \n");
		}
	}else{
		gmt->is_fully_charged = 0;
		gmt->batt_low = 0;
	}

	if(gmt->is_fully_charged){
		power_supply_changed(&gmt->battery);
	}else{
		power_supply_changed(&gmt->mains);
//		power_supply_changed(&gmt->usb);
		power_supply_changed(&gmt->battery);
	}

	if(gmt->otg_check)
		g2214_write(REG_A9,0xF0);
	else
		g2214_write(REG_A9,0xF2);
	g2214_write(REG_A11,0xCF);

#ifdef G2214_DUMP
	g2214reg_dump();
#endif
}

static void otg_det_work(struct work_struct *work)
{
	char tmp_data;

	gmt->otg_check = REG8_VAL(USB_BASE_ADD+0x7F2) & BIT0;
	if (!gmt->otg_check){
		printk("  [OTG mode]  \n");
		gmt->current_state = USB_OTG;
		//gmt->usb_online =0;
		//gmt->mains_online = 0;
		gmt->usb_otg_enabled = 1;

		if (wmt_otg_power == 0)
			tmp_data = (gmt->vseta << 6) & ~BIT3;
		else
			tmp_data = (gmt->vseta << 6) | BIT3;

		g2214_write(REG_A8,tmp_data);
		//tmp_data = ((gmt->iset_dcin<<6) | (gmt->iset_vbus<<4) | gmt->resum_set.otgpc);
		//g2214_write(REG_A5,tmp_data);
		tmp_data = g2214_read(REG_A9);
//		printk("A9 = 0x%x",tmp_data);
		tmp_data |= BIT1;
//		printk("A9_after = 0x%x",tmp_data);
		g2214_write(REG_A9,tmp_data);
	}else{
		printk("  [USB OTG disconnect.]\n");
		gmt->current_state = USB_PC;
		//gmt->usb_online =0;
		//gmt->mains_online = 0;
		gmt->usb_otg_enabled = 0;
		tmp_data = (gmt->vseta << 6) & (~(BIT3));
		g2214_write(REG_A8,tmp_data);
		//tmp_data = ((gmt->iset_dcin<<6) | (PC_MODE<<4) | gmt->resum_set.otgpc);
		//g2214_write(REG_A5,tmp_data);
		tmp_data = g2214_read(REG_A9);
//		printk("A9 dis= 0x%x",tmp_data);	
		tmp_data &= ~BIT1;
		g2214_write(REG_A9,tmp_data);	
//		printk("A9_after dis= 0x%x",tmp_data);	
	}
	//printk("  g2214 otg_det_work_power_supply_changed \n");
	if (gmt->usb_online  || gmt->mains_online){
		if(gmt->is_fully_charged){
//			power_supply_changed(&gmt->usb);
		}
	}
}

void g2214_pc_connected(void)
{
    if (gmt) {
		//printk("in g2214 pc connected callback\n");
		//schedule_work(&gmt->g2214_work);
		schedule_delayed_work(&gmt->g2214_work, HZ);
	}
}

static irqreturn_t g2214_interrupt(int irq, void *data)
{
	int intstatus;

	intstatus = PMCIS_VAL & BIT0;
	if(intstatus){
	/*        Clear Wakeup0 status*/
		//printk("wakeup0 irq\n");
		g2214_irq_clear_status();
		//schedule_work(&gmt->g2214_work);
		schedule_delayed_work(&gmt->g2214_work, 0);
		return IRQ_HANDLED;
	}

    return IRQ_NONE;
}

static irqreturn_t dcdet_interrupt(int irq, void *data)
{
	int intstatus;

	//printk("dcdet interrupt in\n");
	intstatus = PMCIS_VAL;
	if(intstatus & BIT27) {
		/* Clear dcdet status */
		//printk("dcdet irq\n");
		dcdet_irq_clear_status();
		//if (!gmt->usb_otg_enabled)
		schedule_delayed_work(&gmt->g2214_work, HZ);
		//schedule_work(&gmt->g2214_work);
		return IRQ_HANDLED;
	}
        return IRQ_NONE;
}

static irqreturn_t otg_interrupt(int irq, void *data)
{
        int int_status;

	int_status = REG8_VAL(USB_BASE_ADD+0x7F1) & BIT7;

	if (int_status){
		gmt->otg_check =  REG8_VAL(USB_BASE_ADD+0x7F2) & BIT0;;
		REG8_VAL(USB_BASE_ADD+0x7F1) =  BIT7;
		schedule_work(&gmt->otg_work);
	        return IRQ_HANDLED;
	}

        return IRQ_NONE;
}

static int g2214_irq_init(void)
{
	int ret = 0;

	//printk(" [%s]_%d\n",__func__,__LINE__);
	if (*(volatile unsigned int *)0xfe120000 == 0x35100101) {
		ret = request_irq(IRQ_PMC_WAKEUP,
				  g2214_interrupt,
				  (IRQF_SHARED | IRQF_NO_SUSPEND),
				  "g2214",gmt->dev);                 
	} else {
		ret = request_irq(IRQ_PMC_WAKEUP,
				  g2214_interrupt,
				  IRQF_SHARED,
				  "g2214",gmt->dev);
	}       


	if (ret < 0){
		goto fail;
	}

	if (*(volatile unsigned int *)0xfe120000 == 0x35100101) {
		ret = request_irq(IRQ_PMC_WAKEUP,
				  dcdet_interrupt,
				  (IRQF_SHARED | IRQF_NO_SUSPEND),
				  "dcdet",gmt->dev);                 
	} else {
		ret = request_irq(IRQ_PMC_WAKEUP,
				  dcdet_interrupt,
				  IRQF_SHARED,
				  "dcdet",gmt->dev);
	}       

	if (ret < 0){
		free_irq(IRQ_PMC_WAKEUP,gmt->dev);
		goto fail;
	}

	ret = request_irq(IRQ_UHDC,
			  otg_interrupt,
			  IRQF_SHARED,
			  "chg:usb", gmt->dev);

	if (ret < 0) {
		free_irq(IRQ_PMC_WAKEUP,gmt->dev);
		goto fail;
	}

	return 0;

fail:
	gmt->g2144_irq = 0;
	gmt->otg_irq = 0;
	//gmt->client->irq = 0;
	return ret;
}

static int g2214_reg_init(void)
{
	unsigned int tmp_data = 2 ;

	//printk(" [%s]_%d\n",__func__,__LINE__);

	/*    Set safety time = 8hours */
	tmp_data = g2214_read(REG_A6);
	tmp_data = tmp_data & (~(BIT4|BIT5|BIT6|BIT7));
	tmp_data = tmp_data | (gmt->safety_time<<4);
	g2214_write(REG_A6,tmp_data);


	if(gmt->no_batt == 0){
		/*        Set battery charge voltage level    */	
		g2214_write(REG_A8,(gmt->vseta << 6));   /* default value = 0x80  */
		/*    Set ISET_DCIN & ISET_VBUS & ISETA current level */
		if(!gmt->otg_check ){ /*  Check OTG    */
			gmt->usb_otg_enabled = 1;
			gmt->current_state = USB_OTG;

			if (wmt_otg_power == 0)
				g2214_write(REG_A8,((gmt->vseta << 6) & ~BIT3));
			else
				g2214_write(REG_A8,((gmt->vseta << 6) | BIT3));

		}
		if(g2214_read(REG_A10) & BIT2){  /*  DCIN  */
			//printk("Init: DCIN\n");
			gmt->current_state = DCIN;
			tmp_data = ((gmt->iset_dcin<<6) | (gmt->iset_vbus<<4) | gmt->resum_set.dcin);
			g2214_write(REG_A5,tmp_data);
		}else if(gmt->usb_connected){ 
			gmt->usb_otg_enabled = 0;
			if(!wmt_usb_connected()){ /*    Check adapter mode or pc mode    */
				//printk("Init: USB_ADP\n");
				gmt->current_state = USB_ADP;
				tmp_data = ((gmt->iset_dcin<<6) | (ADP_MODE<<4) | gmt->resum_set.otgadp);
				g2214_write(REG_A5,tmp_data);
				g2214_write(REG_A8,((gmt->vseta << 6) & ( ~(BIT3))));
			}else{
				//printk("Init: USB_PC\n");
				gmt->current_state = USB_PC;
				tmp_data = ((gmt->iset_dcin<<6) | (PC_MODE<<4) | gmt->resum_set.otgpc);
				g2214_write(REG_A5,tmp_data);
				g2214_write(REG_A8,((gmt->vseta << 6) & ( ~(BIT3))));
			}
		}else{ /*  N/A IN  */
			//printk("Init: N/A\n");
			gmt->current_state = USB_PC;
			tmp_data = ((gmt->iset_dcin<<6) | (PC_MODE<<4) | gmt->resum_set.otgpc);
			g2214_write(REG_A5,tmp_data);
		}	
	}
	return 0;
}

static int bat_status_init(void)
{

	unsigned int g2214_status_01;
	unsigned int g2214_status_02;
	unsigned int tmp;


	//printk(" [%s]_%d\n",__func__,__LINE__);
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
		printk("No batt");
		gmt->no_batt = 1;
	}else	{     /* Check is no battery. */
		gmt->no_batt = 0;		
		if(g2214_status_01 & 0x0E){
			if(g2214_status_01 & 0x08){
				gmt->batt_dead=1;
			}
			//if(gmt->otg_check){
				if((g2214_status_01 & (BIT1|BIT2)) && (g2214_status_01 & BIT2)){
					gmt->current_state = DCIN;
					gmt->mains_online = 1;
					gmt->usb_online = 0;
				}else if((g2214_status_01 & BIT1) && gmt->otg_check){
					gmt->current_state = USB_PC;
					gmt->usb_online =1;
					gmt->mains_online = 0;
				}
			//}else{
				//gmt->current_state = USB_PC;
				//gmt->usb_online =0;
				//gmt->mains_online = 0;
			//}
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
				printk("  Battery Low \n");
			}
		}else{
			gmt->is_fully_charged = 0;
			gmt->batt_low = 0;
		}
	}
	return 0;
}

static int wmt_hw_initial(void)
{
	/*  Enable PID check interrupt status */
	REG8_VAL(USB_BASE_ADD+0x7F2) = REG8_VAL(USB_BASE_ADD+0x7F2) | BIT1;
	/*  GPIO initial */
	//REG32_VAL(GPIO_BASE_ADD+0x40) = REG32_VAL(GPIO_BASE_ADD+0x40) | 0x00010000;
	//REG32_VAL(GPIO_BASE_ADD+0x80) = REG32_VAL(GPIO_BASE_ADD+0x80) & (~0x00010000);
	//REG32_VAL(GPIO_BASE_ADD+0x480) = REG32_VAL(GPIO_BASE_ADD+0x480) |0x00010000;
	//REG32_VAL(GPIO_BASE_ADD+0x4C0) = REG32_VAL(GPIO_BASE_ADD+0x4C0) | 0x00010000;

	/* Set wakeup 0 interrupt type*/
	//INT_TYPE0_VAL = INT_TYPE0_VAL | BIT1;
	return 0;
}

static int g2214_hw_init(void)
{
	//printk(" [%s]_%d\n",__func__,__LINE__);

	/*    Initial USB connect status,  */		
	gmt->usb_connected = REG8_VAL(USB_BASE_ADD+0x70D) & BIT0;
	gmt->otg_check = REG8_VAL(USB_BASE_ADD+0x7F2) & BIT0;

	bat_status_init();
	g2214_reg_init();
	wmt_hw_initial();

	return 0;
}

int g2214_is_dc_plugin(void)
{
    if (gmt)
        return (gmt->mains_online || gmt->usb_online);
    else 
        return 1;
}

static int g2214_mains_get_property(struct power_supply *psy,
				    enum power_supply_property prop,
				    union power_supply_propval *val)
{
	switch (prop) {
	case POWER_SUPPLY_PROP_HEALTH:
		if ( gmt->batt_dead)
			val->intval = POWER_SUPPLY_HEALTH_DEAD;
		else
			val->intval = POWER_SUPPLY_HEALTH_GOOD;
		break;
	case POWER_SUPPLY_PROP_ONLINE:
		if (gmt->mains_online || (gmt->current_state  ==  USB_ADP))
			val->intval =1;
		else		
			val->intval =0;
		break;

	default:
		return -EINVAL;
	}

	return 0;
}

static enum power_supply_property g2214_mains_properties[] = {
	POWER_SUPPLY_PROP_ONLINE,
	POWER_SUPPLY_PROP_HEALTH,
};

#if 0
static int g2214_usb_get_property(struct power_supply *psy,
				  enum power_supply_property prop,
				  union power_supply_propval *val)
{
	switch (prop) {
	case POWER_SUPPLY_PROP_ONLINE:
		if (gmt->usb_online & (gmt->current_state  !=  USB_ADP))
			val->intval =1;
		else		
			val->intval =0;
		break;

	case POWER_SUPPLY_PROP_USB_HC:
		val->intval = gmt->usb_hc_mode;
		break;

	case POWER_SUPPLY_PROP_USB_OTG:
		val->intval = gmt->usb_otg_enabled;
		break;

	default:
		return -EINVAL;
	}
	return 0;
}

static enum power_supply_property g2214_usb_properties[] = {
	POWER_SUPPLY_PROP_ONLINE,
	POWER_SUPPLY_PROP_USB_HC,
	POWER_SUPPLY_PROP_USB_OTG,
};
#endif

static int g2214_battery_get_property(struct power_supply *psy,
				      enum power_supply_property prop,
				      union power_supply_propval *val)
{
	switch (prop) {
	case POWER_SUPPLY_PROP_STATUS:
		if(gmt->no_batt){
			val->intval = POWER_SUPPLY_STATUS_UNKNOWN;
		}else{
			if (gmt->mains_online || gmt->usb_online){
				if (gmt->is_fully_charged){
					val->intval = POWER_SUPPLY_STATUS_FULL;
				}else{
					val->intval = POWER_SUPPLY_STATUS_CHARGING;
				}
			}else{
				val->intval = POWER_SUPPLY_STATUS_DISCHARGING;
			}
		}
		break;

	case POWER_SUPPLY_PROP_HEALTH:
		if(gmt->batt_dead)
			val->intval = POWER_SUPPLY_HEALTH_DEAD;
		else
			val->intval = POWER_SUPPLY_HEALTH_GOOD;
		break;

	case POWER_SUPPLY_PROP_TECHNOLOGY:	
		val->intval = POWER_SUPPLY_TECHNOLOGY_LION;
		break;

	default:
		return -EINVAL;
	}

	return 0;
}


static enum power_supply_property g2214_battery_properties[] = {
	POWER_SUPPLY_PROP_STATUS,
	POWER_SUPPLY_PROP_HEALTH,
	POWER_SUPPLY_PROP_TECHNOLOGY,		
};

static int g2214_reboot_notifie(struct notifier_block *nb, unsigned long event, void *unused)
{
	gmt->otg_check = REG8_VAL(USB_BASE_ADD+0x7F2) & BIT0;
	gmt->usb_con_sus = REG8_VAL(USB_BASE_ADD+0x70D) & BIT0;
	/*  Disable G2214 interrupt */
	g2214_write(REG_A9,0xFF);
	g2214_write(REG_A11,0xDF);
	g2214_write(REG_A12,0x00);
	g2214_write(REG_A8,((gmt->vseta << 6)  & ( ~(BIT3))));

	/*  Disable WAKEUP0  interrupt */
	/*g2214_irq_disable();*/
	/*  Disable USB_ID check interrupt */
	REG8_VAL(USB_BASE_ADD+0x7F2) = REG8_VAL(USB_BASE_ADD+0x7F2) & (~ BIT1);
	/*g2214_irq_clear_status();*/
	/*PMCIS_VAL =  BIT0;*/

	flush_delayed_work_sync(&gmt->g2214_work);
	//flush_work_sync(&gmt->g2214_work);
	flush_work_sync(&gmt->otg_work);
	return NOTIFY_OK;
}

static struct notifier_block g2214_reboot_notifier = {
	.notifier_call = g2214_reboot_notifie,
};

#ifdef CONFIG_PM
static int g2214_suspend(void)
{
	gmt->otg_check = REG8_VAL(USB_BASE_ADD+0x7F2) & BIT0;
	gmt->usb_con_sus = REG8_VAL(USB_BASE_ADD+0x70D) & BIT0;
	/*  Disable G2214 interrupt */
	g2214_write(REG_A9,0xFF);
	g2214_write(REG_A11,0xDF);
	g2214_write(REG_A12,0x00);
	g2214_write(REG_A8,((gmt->vseta << 6)  & ( ~(BIT3))));

	/*  Disable WAKEUP0  interrupt */
	/*g2214_irq_disable();*/
	/*  Disable USB_ID check interrupt */
	REG8_VAL(USB_BASE_ADD+0x7F2) = REG8_VAL(USB_BASE_ADD+0x7F2) & (~ BIT1);
	/*g2214_irq_clear_status();*/
	/*PMCIS_VAL =  BIT0;*/
	flush_delayed_work_sync(&gmt->g2214_work);
	//flush_work_sync(&gmt->g2214_work);
	flush_work_sync(&gmt->otg_work);

	return 0;
}

static int g2214_resume(struct device *dev)
{
	unsigned int tmp;
	int tmp_data;
	int g2214_status;
	int dcin_status;

	tmp = g2214_read(REG_A0);
	if(tmp & BIT3){
		tmp = tmp & (~BIT3);	
		g2214_write(REG_A0,tmp);
	}
	gmt->otg_check = REG8_VAL(USB_BASE_ADD+0x7F2) & BIT0;
	gmt->usb_connected = REG8_VAL(USB_BASE_ADD+0x70D) & BIT0;
	dcin_status = ((DCDET_STS_VAL & BIT8)>>8);

	if(!gmt->otg_check ){ /*  Check OTG    */
		gmt->usb_otg_enabled = 1;
		//gmt->current_state = USB_OTG;
	}
	//if(gmt->usb_con_sus != gmt->usb_connected ){
		//printk("connected not the same\n");
		if((g2214_read(REG_A10) & BIT2)>>2){  /*  DCIN  */
			//printk("resume DCIN\n");
			gmt->current_state = DCIN;
		}else if(gmt->usb_connected){ 
			gmt->usb_otg_enabled = 0;
			if(!wmt_usb_connected()){ /* Check adapter mode or pc mode */
				//printk("resume USB_ADP\n");
				gmt->current_state = USB_ADP;
			}else{
				//printk("resume USB_PC\n");
				gmt->current_state = USB_PC;
			}
		}else{
			//printk("resume N/A IN\n");
			gmt->current_state = 4;    /*  N/A IN  */
		}
	//}else if( !dcin_status){ 
		//printk("not dcin_status\n");
		//gmt->current_state = 4;    /*  N/A IN  */
	//}	

	switch (gmt->current_state ) {
	case DCIN:		/* Command will be handled */
		tmp_data = ((gmt->iset_dcin<<6) |  (PC_MODE <<4) | gmt->resum_set.dcin);
		gmt->usb_online =0;
		gmt->mains_online =1;
		g2214_write(REG_A5,tmp_data);
		//g2214_write(REG_A8,((gmt->vseta << 6)  & ( ~(BIT3))));
		break;
	case USB_ADP:		/* Command will not be handled */
		tmp_data = ((gmt->iset_dcin<<6) |  (ADP_MODE <<4) | gmt->resum_set.otgadp);
		gmt->usb_online =0;
		gmt->mains_online =1;
		g2214_write(REG_A5,tmp_data);
		g2214_write(REG_A8,((gmt->vseta << 6)  & ( ~(BIT3))));
		break;
	case USB_PC:		/* Command will not be handled */
		tmp_data = ((gmt->iset_dcin<<6) |  (PC_MODE <<4) | gmt->resum_set.otgpc);
		gmt->usb_online =1;
		gmt->mains_online =0;
		g2214_write(REG_A5,tmp_data);
		g2214_write(REG_A8,((gmt->vseta << 6)  & ( ~(BIT3))));
		break;
	//case USB_OTG:		/* Command will not be handled */
		//gmt->usb_online =0;
		//gmt->mains_online = 0;
		//tmp_data = ((gmt->iset_dcin<<6) |  (PC_MODE <<4) | gmt->resum_set.otgpc);
		//g2214_write(REG_A5,tmp_data);

		//if (wmt_otg_power == 0)
			//g2214_write(REG_A8,((gmt->vseta << 6) & ~BIT3));
		//else
			//g2214_write(REG_A8,((gmt->vseta << 6) | BIT3));

		//break;
	default:
		gmt->batt_dead=0;
		gmt->usb_online =0;
		gmt->mains_online =0;
		gmt->current_state = USB_PC;
		gmt->usb_otg_enabled = 1;
		tmp_data = ((gmt->iset_dcin<<6) |  (PC_MODE <<4) | gmt->resum_set.otgpc);
		g2214_write(REG_A5,tmp_data);
		g2214_write(REG_A8,((gmt->vseta << 6)  & ( ~(BIT3))));
		break;
	}

	if (gmt->usb_otg_enabled) {
		if (wmt_otg_power == 0)
			g2214_write(REG_A8,((gmt->vseta << 6) & ~BIT3));
		else
			g2214_write(REG_A8,((gmt->vseta << 6) | BIT3));
	}

	g2214_status = g2214_read(REG_A12);
	if (g2214_status & 0x30){
		if(((g2214_status & 0x10)>>4) & (gmt->mains_online || gmt->usb_online)){
			gmt->is_fully_charged = 1;
		}
		if(g2214_status & 0x20){				
			printk("  Battery Low \n");
		}
	}else{
		gmt->is_fully_charged = 0;
		gmt->batt_low = 0;
	}

	//if (gmt->usb_online  || gmt->mains_online){
	if(gmt->is_fully_charged){
		power_supply_changed(&gmt->battery);
	}else{
		power_supply_changed(&gmt->mains);
//		power_supply_changed(&gmt->usb);
		power_supply_changed(&gmt->battery);
	}
	//}

	/*  Enable G2214 interrupt */
	g2214_write(REG_A12,0x00);
	g2214_write(REG_A9,0xF0);
	g2214_write(REG_A11,0xCF);
	/*  Enable PID check interrupt */
	REG8_VAL(USB_BASE_ADD+0x7F2) = REG8_VAL(USB_BASE_ADD+0x7F2) | BIT1;
	/*  Enable WAKEUP0  interrupt */
	/*g2214_irq_enable();*/

	return 0;
}

static int
g2214_pm_notify(struct notifier_block *nb, unsigned long event, void *dummy)
{
	if (event == PM_SUSPEND_PREPARE){
		g2214_suspend();
	}
	return NOTIFY_OK;
}

static struct notifier_block g2214_pm_notifier = {
	.notifier_call = g2214_pm_notify,
};
#endif

#define G2214_PROC_NAME "driver/g2214"

static int g2214_proc_show(struct seq_file *seq, void *offset)
{
	seq_printf(seq, "-------- Dump start  -------------------\n");
	seq_printf(seq, "reg A%d: 0x%x\n ", REG_A0, g2214_read(REG_A0));
	seq_printf(seq, "reg A%d: 0x%x\n ", REG_A1, g2214_read(REG_A1));
	seq_printf(seq, "reg A%d: 0x%x\n ", REG_A2, g2214_read(REG_A2));
	seq_printf(seq, "reg A%d: 0x%x\n ", REG_A3, g2214_read(REG_A3));
	seq_printf(seq, "reg A%d: 0x%x\n ", REG_A4, g2214_read(REG_A4));
	seq_printf(seq, "reg A%d: 0x%x\n ", REG_A5, g2214_read(REG_A5));
	seq_printf(seq, "reg A%d: 0x%x\n ", REG_A6, g2214_read(REG_A6));
	seq_printf(seq, "reg A%d: 0x%x\n ", REG_A7, g2214_read(REG_A7));
	seq_printf(seq, "reg A%d: 0x%x\n ", REG_A8, g2214_read(REG_A8));
	seq_printf(seq, "reg A%d: 0x%x\n ", REG_A9, g2214_read(REG_A9));
	seq_printf(seq, "reg A%d: 0x%x\n ", REG_A10, g2214_read(REG_A10));
	seq_printf(seq, "reg A%d: 0x%x\n ", REG_A11, g2214_read(REG_A11));
	seq_printf(seq, "reg A%d: 0x%x\n ", REG_A12, g2214_read(REG_A12));
	seq_printf(seq, "reg A%d: 0x%x\n ", REG_A13, g2214_read(REG_A13));
	seq_printf(seq, "-------- Dump end  -------------------\n");
	return 0;
}

static int g2214_proc_open(struct inode *inode, struct file *file)
{
	int ret;
	if (!try_module_get(THIS_MODULE))
		return -ENODEV;

	ret = single_open(file, g2214_proc_show, NULL);
	if (ret)
		module_put(THIS_MODULE);
	return ret;
}

static int g2214_proc_release(struct inode *inode, struct file *file)
{
	int res = single_release(inode, file);
	module_put(THIS_MODULE);
	return res;
}

static const struct file_operations g2214_proc_fops = {
	.open		= g2214_proc_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= g2214_proc_release,
};

static inline void g2214_proc_setup(void)
{
	proc_create_data(G2214_PROC_NAME, 0, NULL, &g2214_proc_fops, NULL);
}

static inline void g2214_proc_cleanup(void)
{
	remove_proc_entry(G2214_PROC_NAME, NULL);
}

static int __devinit g2214_probe(struct platform_device *pdev)
{
	static char *battery[] = {"ac","usb","charger","battery"};
	int ret;

	gmt = devm_kzalloc(&pdev->dev, sizeof(*gmt), GFP_KERNEL|GFP_ATOMIC);
	if (!gmt)
		goto fail;

	ret = env_parser();
	if (ret != 0) {
		goto fail;
	}
	parse_otg_power();

	register_reboot_notifier(&g2214_reboot_notifier);
#ifdef CONFIG_PM
	register_pm_notifier(&g2214_pm_notifier);
#endif

	gmt->gmt_dev = dev_get_drvdata(pdev->dev.parent);
	gmt->dev = &pdev->dev;
	platform_set_drvdata(pdev, gmt);

	mutex_init(&gmt->lock);
	//INIT_WORK(&gmt->g2214_work, g2214_chr_work);
	INIT_DELAYED_WORK(&gmt->g2214_work, g2214_chr_work);
	INIT_WORK(&gmt->otg_work, otg_det_work);


	gmt->mains.name = "ac";
	gmt->mains.type = POWER_SUPPLY_TYPE_MAINS;
	gmt->mains.get_property = g2214_mains_get_property;
	gmt->mains.properties = g2214_mains_properties;
	gmt->mains.num_properties = ARRAY_SIZE(g2214_mains_properties);
	gmt->mains.supplied_to = battery;
	gmt->mains.num_supplicants = ARRAY_SIZE(battery);

#if 0
	gmt->usb.name = "usb";
	gmt->usb.type = POWER_SUPPLY_TYPE_USB;
	gmt->usb.get_property = g2214_usb_get_property;
	gmt->usb.properties = g2214_usb_properties;
	gmt->usb.num_properties = ARRAY_SIZE(g2214_usb_properties);
	gmt->usb.supplied_to = battery;
	gmt->usb.num_supplicants = ARRAY_SIZE(battery);
#endif

	gmt->battery.name = "charger";
	gmt->battery.type = POWER_SUPPLY_TYPE_UNKNOWN;
	gmt->battery.get_property = g2214_battery_get_property;
	gmt->battery.properties = g2214_battery_properties;
	gmt->battery.num_properties = ARRAY_SIZE(g2214_battery_properties);

	ret = power_supply_register(gmt->dev, &gmt->mains);
	if (ret < 0) {
		printk("Can't register ac supply\n");
		return ret;
	}

#if 0
	ret = power_supply_register(gmt->dev, &gmt->usb);
	if (ret < 0) {
		power_supply_unregister(&gmt->mains);
		return ret;
	}
#endif

	ret = power_supply_register(gmt->dev, &gmt->battery);
	if (ret < 0) {
		printk("Can't register charger supply\n");
//		power_supply_unregister(&gmt->usb);
		power_supply_unregister(&gmt->mains);
		return ret;
	}

	//register_pm_notifier(&g2214_pm_notifier);

	g2214_write(REG_A9,0xFF);
	g2214_write(REG_A11,0xFF);
	g2214_write(REG_A12,0x00);
	g2214_irq_disable();
	/*    Disable power buttom trigger  */
	//INT_TRG_EN_VAL = INT_TRG_EN_VAL &(~(BIT24)); 
	//PMCIS_VAL =  BIT14 | BIT0;

	ret = g2214_irq_init();
	if (ret < 0) {
		dev_warn(gmt->dev, "failed to initialize IRQ: %d\n", ret);
		dev_warn(gmt->dev, "disabling IRQ support\n");
		goto fail;
	}

	ret = g2214_hw_init();
	if (ret < 0) {
		dev_warn(gmt->dev, "Hardware initial fail. \n");
	}

	if(gmt->no_batt == 0){
		g2214_write(REG_A12,0x00);
		g2214_write(REG_A9,0xF0);
		g2214_write(REG_A11,0xCF);

		/* Enable wakeup 0 interrupt */
		g2214_irq_enable();

		/*  Enable PID check interrupt */
		REG8_VAL(USB_BASE_ADD+0x7F2) = REG8_VAL(USB_BASE_ADD+0x7F2) | BIT1;
		//if (gmt->usb_online  || gmt->mains_online){
		if(gmt->is_fully_charged){
			power_supply_changed(&gmt->battery);
		}else{
			power_supply_changed(&gmt->mains);
//			power_supply_changed(&gmt->usb);
			power_supply_changed(&gmt->battery);
		}
		//}
	}else{
		g2214_write(REG_A12,0x00);
		g2214_write(REG_A9,0xFF);
		g2214_write(REG_A11,0xFF);

		/* Disable wakeup 0 interrupt */
		g2214_irq_disable();

		/*  Disable PID check interrupt */
		REG8_VAL(USB_BASE_ADD+0x7F2) = REG8_VAL(USB_BASE_ADD+0x7F2) & (~BIT1);

	}

	g2214_proc_setup();

	/* Enable dcdet edge interrupt */
	dcdet_irq_enable();

	printk(" G2214 charger mounted !");
	return 0;

fail:
	printk(" G2214 charger unmount. ");
	return -ENODEV;
}

static int __devexit g2214_remove(struct platform_device *pdev)
{
	g2214_proc_cleanup();

	g2214_irq_disable();
	dcdet_irq_disable();
	//disable_irq_wake(client->irq);
	free_irq(gmt->g2144_irq, gmt);
	free_irq(gmt->otg_irq, gmt);
	//gpio_free(gmt->pdata->irq_gpio);

	power_supply_unregister(&gmt->battery);
//	power_supply_unregister(&gmt->usb);
	power_supply_unregister(&gmt->mains);
	kfree(gmt);
	return 0;
}

static const struct dev_pm_ops g2214_pm_ops = {
	.resume = g2214_resume,
};

static struct platform_driver g2214_driver = {
	.probe	=	g2214_probe,
	.remove	=	__devexit_p(g2214_remove),
	.driver	= {
		.name	= "gmt-charger",
		.owner	= THIS_MODULE,
#ifdef CONFIG_PM
		.pm	= &g2214_pm_ops,
#endif
	},

};

static int __init g2214_init(void)
{
	return platform_driver_register(&g2214_driver);

}
module_init(g2214_init);

static void __exit g2214_exit(void)
{
	platform_driver_unregister(&g2214_driver);
}
module_exit(g2214_exit);

MODULE_AUTHOR("WonderMedia Technologies, Inc.");
MODULE_DESCRIPTION("GMT2144 battery charger driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("i2c:g2214");

