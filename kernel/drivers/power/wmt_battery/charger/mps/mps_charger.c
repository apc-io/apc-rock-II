/*
 *  Clobal Mixed-mode Technology GMT2144 Battery Charger Driver
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
#include <linux/mutex.h>
#include <linux/power_supply.h>
#include <linux/seq_file.h>
#include <linux/delay.h>
#include <mach/hardware.h>
#include <linux/suspend.h>
#include <linux/syscore_ops.h>
#include <linux/slab.h>
#include "mps_charger.h"

extern int wmt_getsyspara(char *varname, char *varval, int *varlen);
//extern int wmt_chg_done(struct batt_status);

static int env_parser(void){
	int varlen = 800;
	char buf[varlen];
	char varname[] = "wmt.io.chg";
	char tmp_buf[varlen];
	int retval;
	int i = 0;
	int j = 0;
	int chgen = 0;
	/* printk("  [%s]_%d\n",__func__,__LINE__); */
		
	retval = wmt_getsyspara(varname, buf, &varlen);
	if (retval == 0) {
		chgen = (buf[i] - '0' == 1)?1:0;
		if (chgen == 0) {        
			chgen  = 0;
			goto out;
		}
		i += 2;
		for (; i < varlen; ++i) {
			if (buf[i] == ':')
				break;
			tmp_buf[j] = buf[i];
			++j;
		}
		if (strncmp(tmp_buf,"mps",3)) {
			goto out;
		}
	}else{
		goto out;
	}
	return 0;
out:	
	return 1;	
}

static int mps_irq_enable(int en)
{
	if(en == TRUE)
		pmc_enable_wakeup_isr(WKS_DCDET,4);
	else
		pmc_disable_wakeup_isr(WKS_DCDET);
		
	return 0;
}

static int mps_irq_clear_status(void)
{
	pmc_clear_intr_status(WKS_DCDET);
	return 0;
}

static int chgdon_irq_enable(int en)
{
	if(en == TRUE)
		GPIO5_INT_REQ_TYPE_VAL = GPIO5_INT_REQ_TYPE_VAL | CHDONEN;
	else
		GPIO5_INT_REQ_TYPE_VAL = GPIO5_INT_REQ_TYPE_VAL & (~(CHDONEN));

	return 0;
}

static int chgdon_irq_clear_status(void)
{
	GPIO0_INT_REQ_STS_VAL =  CHGDON;
	return 0;
}

static int mps_chg_done_status(void)
{
	int chg_done; 

	if(GPIO_ID_GP0_BYTE_VAL & CHGDON)
		chg_done = 1;
	else
		chg_done = 0;

	return chg_done;
}

static void mps_chg_current_setting(int pcmode)
{
        if(pcmode)
		GPIO_OD_GP62_WAKEUP_SUS_BYTE_VAL = GPIO_OD_GP62_WAKEUP_SUS_BYTE_VAL | CHGCURR;
        else
		GPIO_OD_GP62_WAKEUP_SUS_BYTE_VAL = GPIO_OD_GP62_WAKEUP_SUS_BYTE_VAL & (~(CHGCURR));
			
}

static int check_dc_is_in(void)
{
        int dc_is_in; 
		
        dc_is_in = (DCDET_STS_VAL & DCINSTS)>>8;

	return dc_is_in;
}


static void mps_chr_work(struct work_struct *work){

	mps->is_fully_charged = mps_chg_done_status();
	mps->mains_online = check_dc_is_in();
	msleep(100);
	mps->usb_online = REG8_VAL(USB_BASE_ADD+0x70D) & BIT0;
	
	if(mps->usb_online || mps->mains_online){
		if(mps->is_fully_charged ){
			//printk("  Charg Done\n");
			mps->current_state = PC_MODE;
		        mps_chg_current_setting(PC_MODE);
			//wmt_chg_done(TRUE);
		}else{
			//if (mps->usb_online && mps->mains_online){
				if(mps->usb_online){
					//mps->mains_online = 0;
					msleep(100);
			                 /*    Check D+ & D- Status    */
					if(REG8_VAL(USB_BASE_ADD+0x400+0x22) & BIT6){
						//printk("  [Adapter charger.]\n");
						mps->usb_online = 0;
						mps->current_state = ADP_MODE;
					        mps_chg_current_setting(ADP_MODE);
					}else{
						//printk("  [PC charger.]\n");
						mps->mains_online = 0;
						mps->current_state = PC_MODE;
					        mps_chg_current_setting(PC_MODE);
					}
				}else if(mps->mains_online){
					//printk(" DCIN Charging\n");
					mps->usb_online = 0;
					mps->current_state = ADP_MODE;
					mps_chg_current_setting(ADP_MODE);
				}
			}
		//}
	}else{
		mps->usb_online =0;
		mps->mains_online =0;
		mps->is_fully_charged = 0;
		mps->current_state = PC_MODE;
	        mps_chg_current_setting(PC_MODE);
	}

	power_supply_changed(&mps->battery);
	power_supply_changed(&mps->mains);
	power_supply_changed(&mps->usb);
}

static void chg_done_work(struct work_struct *work){
	/*printk(" [%s]_%d\n",__func__,__LINE__);*/

	mps->is_fully_charged = mps_chg_done_status();
	mps->mains_online = check_dc_is_in();
	msleep(100);
	mps->usb_online = REG8_VAL(USB_BASE_ADD+0x70D) & BIT0;
	if(mps->usb_online || mps->mains_online){
		if(mps->is_fully_charged ){
			//printk("  Charg Done\n");
			mps->current_state = PC_MODE;
		        mps_chg_current_setting(PC_MODE);
			//wmt_chg_done(TRUE);
		}else{
			//if (mps->usb_online && mps->mains_online){
				if(mps->usb_online){
					//mps->mains_online = 0;
					//msleep(100);
			                 /*    Check D+ & D- Status    */
					if(REG8_VAL(USB_BASE_ADD+0x400+0x22) & BIT6){
						//printk("  [Adapter charger.]\n");
						mps->usb_online = 0;
						mps->current_state = ADP_MODE;
					        mps_chg_current_setting(ADP_MODE);
					}else{
						//printk("  [PC charger.]\n");
						mps->mains_online = 0;
						mps->current_state = PC_MODE;
					        mps_chg_current_setting(PC_MODE);
					}
				}else if(mps->mains_online){
					//printk(" DCIN Charging\n");
					mps->usb_online = 0;
					mps->current_state = ADP_MODE;
					mps_chg_current_setting(ADP_MODE);
				}
			}
		//}
	}else{
		mps->usb_online =0;
		mps->mains_online =0;
		mps->is_fully_charged = 0;
		mps->current_state = PC_MODE;
	        mps_chg_current_setting(PC_MODE);
	}

}


static irqreturn_t mps_interrupt(int irq, void *data)  // DCIN interrupt
{
        int intstatus;

	//printk("  [%s]_%d \n",__func__,__LINE__);
	intstatus = PMCIS_VAL & BIT27;
	/*        Clear Wakeup0 status*/
	mps_irq_clear_status();

	if(intstatus)
	        schedule_work(&mps->mps_work);

        return 0;
}

static irqreturn_t chgdone_interrupt(int irq, void *data) //Chage done interrupt
{
        int intstatus;

	/*printk("  [%s]_%d\n",__func__,__LINE__);*/
	intstatus = (GPIO0_INT_REQ_STS_VAL & CHGDON)>>5;
	/*        Clear Wakeup0 status*/
	chgdon_irq_clear_status();

	if(intstatus)
	        schedule_work(&mps->chgdon_work);

        return 0;
}

static int mps_irq_init(void)
{
	int ret = 0;

        ret = request_irq(IRQ_PMC_WAKEUP,
                                      mps_interrupt,
                                      IRQF_SHARED,
                                      "mps",mps->dev);
           
	if (ret < 0){
		goto fail;
	}

        ret = request_irq(IRQ_GPIO,
                                      chgdone_interrupt,
                                      IRQF_SHARED,
                                      "chgdone",mps->dev);
           
	if (ret < 0){
		goto fail;
	}
	return 0;

fail:
	//mps->g2144_irq = 0;

	return ret;
}

static int mps_reg_init(void){

	mps->is_fully_charged = mps_chg_done_status();
	mps->mains_online = check_dc_is_in();
	mps->usb_online = REG8_VAL(USB_BASE_ADD+0x70D) & BIT0;

	if(mps->usb_online || mps->mains_online){
		 /*          Enable USB hardware mode     */
		REG8_VAL(USB_BASE_ADD+0x400+0x3F) = REG8_VAL(USB_BASE_ADD+0x400+0x3F) |  BIT5;
		if(mps->is_fully_charged ){
			mps->current_state = PC_MODE;
		        mps_chg_current_setting(PC_MODE);
			//wmt_chg_done(TRUE);
		}else{
			//if (mps->usb_online && mps->mains_online){
				if(mps->usb_online){
					msleep(100);
			                 /*    Check D+ & D- Status    */
					if(REG8_VAL(USB_BASE_ADD+0x400+0x22) & BIT6){
						mps->current_state = ADP_MODE;
						mps->usb_online = 0;
					        mps_chg_current_setting(ADP_MODE);
					}else{
						mps->current_state = PC_MODE;
						mps->mains_online = 0;
					        mps_chg_current_setting(PC_MODE);
					}
				}else if(mps->mains_online){
					mps->usb_online = 0;
					mps->current_state = ADP_MODE;
					mps_chg_current_setting(ADP_MODE);
				}
			//}
		}
	        /*          Disable USB hardware mode     */
		REG8_VAL(USB_BASE_ADD+0x400+0x3F) = REG8_VAL(USB_BASE_ADD+0x400+0x3F) & (~( BIT5));
	}else{
		mps->usb_online =0;
		mps->mains_online =0;
		mps->is_fully_charged = 0;
		mps->current_state = PC_MODE;
	        mps_chg_current_setting(PC_MODE);
	}
	
	power_supply_changed(&mps->battery);
	power_supply_changed(&mps->mains);
	power_supply_changed(&mps->usb);
	return 0;
}

static int wmt_hw_initial(void){

	int tmp = 0;
        /*  Enable PID check interrupt status */
	//REG8_VAL(USB_BASE_ADD+0x7F2) = REG8_VAL(USB_BASE_ADD+0x7F2) | BIT1;

	/*  Wakeup 4 GPIO for charge current setting initial */
	GPIO_OC_GP62_WAKEUP_SUS_BYTE_VAL = GPIO_OC_GP62_WAKEUP_SUS_BYTE_VAL | CHGCURR; /* Output data enable */
	PULL_CTRL_GP62_WAKEUP_SUS_BYTE_VAL = PULL_CTRL_GP62_WAKEUP_SUS_BYTE_VAL & (~CHGCURR); /* Pull Down */
	PULL_EN_GP62_WAKEUP_SUS_BYTE_VAL = PULL_EN_GP62_WAKEUP_SUS_BYTE_VAL |CHGCURR;
	GPIO_CTRL_GP62_WAKEUP_SUS_BYTE_VAL = GPIO_CTRL_GP62_WAKEUP_SUS_BYTE_VAL | CHGCURR;
	
	/*  GPIO 5 initial for charg done. */
	GPIO_OC_GP0_BYTE_VAL = GPIO_OC_GP0_BYTE_VAL & (~CHGDON);      /* Output data disable */
	PULL_CTRL_GP0_BYTE_VAL = PULL_CTRL_GP0_BYTE_VAL | CHGDON;  /* Pull Up */
	PULL_EN_GP0_BYTE_VAL = PULL_EN_GP0_BYTE_VAL |CHGDON;    /* Pull up/down enable */
	GPIO_CTRL_GP0_BYTE_VAL = GPIO_CTRL_GP0_BYTE_VAL | CHGDON;     /* GPIO enable */
	GPIO0_INT_REQ_STS_VAL =  CHGDON;     /* Clean interrupt status */
	tmp = GPIO5_INT_REQ_TYPE_VAL;     /* Set interrupt type */
	tmp = tmp & (~(BIT0|BIT1));
	tmp = tmp | BIT2;
        GPIO5_INT_REQ_TYPE_VAL = tmp;     /* Set  interrupt type */
		
	   /* Set wakeup 0 interrupt type*/
	//INT_TYPE0_VAL = INT_TYPE0_VAL | BIT1;
	return 0;
}


static int mps_hw_init(void)
{
	//printk(" [%s]_%d\n",__func__,__LINE__);

	/*    Initial USB connect status,  */		
	//mps->usb_connected = REG8_VAL(USB_BASE_ADD+0x70D) & BIT0;
        wmt_hw_initial();
	mps_reg_init();
	return 0;
}

static int mps_mains_get_property(struct power_supply *psy,
				     enum power_supply_property prop,
				     union power_supply_propval *val)
{
	switch (prop) {
	case POWER_SUPPLY_PROP_HEALTH:
		if ( mps->batt_dead)
			val->intval = POWER_SUPPLY_HEALTH_DEAD;
		else
			val->intval = POWER_SUPPLY_HEALTH_GOOD;
		break;
	case POWER_SUPPLY_PROP_ONLINE:
		if (mps->mains_online || (mps->current_state  ==  ADP_MODE))
		        val->intval =1;
		else		
		        val->intval =0;
		break;

	default:
		return -EINVAL;
	}

        return 0;
}

static enum power_supply_property mps_mains_properties[] = {
	POWER_SUPPLY_PROP_ONLINE,
	//POWER_SUPPLY_PROP_HEALTH,
};

static int mps_usb_get_property(struct power_supply *psy,
				   enum power_supply_property prop,
				   union power_supply_propval *val)
{
	switch (prop) {
	case POWER_SUPPLY_PROP_ONLINE:
		if ((mps->usb_online & (mps->current_state  !=  ADP_MODE)) ||
		    (mps->usb_online && (!mps->is_fully_charged)))
		        val->intval =1;
		else		
		        val->intval =0;
		break;

	case POWER_SUPPLY_PROP_USB_HC:
		val->intval = mps->usb_hc_mode;
		break;

	//case POWER_SUPPLY_PROP_USB_OTG:
	//	val->intval = mps->usb_otg_enabled;
	//	break;

	default:
		return -EINVAL;
	}
	return 0;
}

static enum power_supply_property mps_usb_properties[] = {
	POWER_SUPPLY_PROP_ONLINE,
	//POWER_SUPPLY_PROP_USB_HC,
	//POWER_SUPPLY_PROP_USB_OTG,
};


static int mps_battery_get_property(struct power_supply *psy,
				       enum power_supply_property prop,
				       union power_supply_propval *val)
{
	//printk("  [%s]_%d \n",__func__,__LINE__);
	switch (prop) {
		case POWER_SUPPLY_PROP_STATUS:
			if (mps->mains_online || mps->usb_online){
				if (mps->is_fully_charged){
					val->intval = POWER_SUPPLY_STATUS_FULL;
				}else{
					val->intval = POWER_SUPPLY_STATUS_CHARGING;
				}
			}else{
				val->intval = POWER_SUPPLY_STATUS_DISCHARGING;
			}
			break;
		default:
			return -EINVAL;
	}
	return 0;
}


static enum power_supply_property mps_battery_properties[] = {
	POWER_SUPPLY_PROP_STATUS,
};

static int __devinit mps_probe(struct platform_device *pdev)
{
	static char *battery[] = {"ac","usb","charger","battery" };
	int ret;

	//printk(" Bow [%s]_%d\n",__func__,__LINE__);

	
	mps = devm_kzalloc(&pdev->dev, sizeof(*mps), GFP_KERNEL|GFP_ATOMIC);
	if (!mps)
		goto fail;
	

	//mps->mps_dev = dev_get_drvdata(pdev->dev.parent);
	mps->dev = &pdev->dev;
	platform_set_drvdata(pdev, mps);

	mutex_init(&mps->lock);
        INIT_WORK(&mps->mps_work, mps_chr_work);
        INIT_WORK(&mps->chgdon_work, chg_done_work);

	mps->mains.name = "ac";
	mps->mains.type = POWER_SUPPLY_TYPE_MAINS;
	mps->mains.get_property = mps_mains_get_property;
	mps->mains.properties = mps_mains_properties;
	mps->mains.num_properties = ARRAY_SIZE(mps_mains_properties);
	mps->mains.supplied_to = battery;
	mps->mains.num_supplicants = ARRAY_SIZE(battery);

	mps->usb.name = "usb";
	mps->usb.type = POWER_SUPPLY_TYPE_USB;
	mps->usb.get_property = mps_usb_get_property;
	mps->usb.properties = mps_usb_properties;
	mps->usb.num_properties = ARRAY_SIZE(mps_usb_properties);
	mps->usb.supplied_to = battery;
	mps->usb.num_supplicants = ARRAY_SIZE(battery);

	mps->battery.name = "charger";
	mps->battery.type = POWER_SUPPLY_TYPE_BATTERY;
	mps->battery.get_property = mps_battery_get_property;
	mps->battery.properties = mps_battery_properties;
	mps->battery.num_properties = ARRAY_SIZE(mps_battery_properties);


	
	ret = power_supply_register(mps->dev, &mps->mains);
	if (ret < 0)
		return ret;

	ret = power_supply_register(mps->dev, &mps->usb);
	if (ret < 0) {
		power_supply_unregister(&mps->mains);
		return ret;
	}

	ret = power_supply_register(mps->dev, &mps->battery);
	if (ret < 0) {
		power_supply_unregister(&mps->usb);
		power_supply_unregister(&mps->mains);
		return ret;
	}

	ret = mps_hw_init();
	if (ret < 0) {
		dev_warn(mps->dev, "Hardware initial fail. \n");
	}
	
	
	mps_irq_enable(FALSE);
	/*  Enable GPIO 5 interrupt */
        chgdon_irq_enable(FALSE);
        
	ret = mps_irq_init();
	if (ret < 0) {
		dev_warn(mps->dev, "failed to initialize IRQ: %d\n", ret);
		dev_warn(mps->dev, "disabling IRQ support\n");
		goto fail;
	}


	mps_irq_clear_status();
	chgdon_irq_clear_status();
	/* Enable wakeup 0 interrupt */
	mps_irq_enable(TRUE);
         
	/*  Enable GPIO 5 interrupt */
        chgdon_irq_enable(TRUE);
	return 0;

fail:
	return -ENODEV;
	
}

static int __devexit mps_remove(struct platform_device *pdev)
{
	mps_irq_enable(FALSE);
	power_supply_unregister(&mps->battery);
	power_supply_unregister(&mps->usb);
	power_supply_unregister(&mps->mains);
	kfree(mps);
	return 0;
}

#ifdef CONFIG_PM
static int mps_suspend(struct platform_device *dev, pm_message_t state)
{


	/*  Disable WAKEUP0  interrupt */
        chgdon_irq_enable(FALSE);
        chgdon_irq_clear_status();
	flush_work_sync(&mps->mps_work);
	flush_work_sync(&mps->chgdon_work);
	//mps->current_state = ADP_MODE;
	//mps_chg_current_setting(ADP_MODE);
#if 0 	
	printk("Bow $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$");
	printk(" bow [Suspend _ usb 0x7F1 = 0x%x] \n",REG8_VAL(USB_BASE_ADD+0x7F1));
	printk(" bow [Suspend _ usb 0x7F2 = 0x%x] \n",REG8_VAL(USB_BASE_ADD+0x7F2));
	printk(" bow [Suspend _ GPIO 0x7C = 0x%x] \n",REG32_VAL(PMCIE_ADDR));
	printk(" bow [Suspend _ GPIO 0x74 = 0x%x] \n",REG32_VAL(PMCIS_VAL));
	printk("Bow $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$");
#endif
	return 0;
}

static int mps_resume(struct platform_device *dev)
{
	//int i;
	wmt_hw_initial();
	mps->is_fully_charged = mps_chg_done_status();
	mps->mains_online = check_dc_is_in();
	msleep(100);
	mps->usb_online = REG8_VAL(USB_BASE_ADD+0x70D) & BIT0;

	if(mps->usb_online || mps->mains_online){
		if(mps->is_fully_charged ){
			printk("  Charg Done\n");
			mps->current_state = PC_MODE;
		        mps_chg_current_setting(PC_MODE);
			//wmt_chg_done(TRUE);
		}else{
			//if (mps->usb_online && mps->mains_online){
				if(mps->usb_online){
					//mps->mains_online = 0;
					//msleep(100);
			                 /*    Check D+ & D- Status    */
					if(REG8_VAL(USB_BASE_ADD+0x400+0x22) & BIT6){
						mps->usb_online = 0;
						mps->current_state = ADP_MODE;
					        mps_chg_current_setting(ADP_MODE);
					}else{
						mps->mains_online = 0;
						mps->current_state = PC_MODE;
					        mps_chg_current_setting(PC_MODE);
					}
				}else if(mps->mains_online){
					mps->usb_online = 0;
					mps->current_state = ADP_MODE;
				        mps_chg_current_setting(ADP_MODE);
				}
			}
		//}
	}else{
		mps->usb_online =0;
		mps->mains_online =0;
		mps->is_fully_charged = 0;
		mps->current_state = PC_MODE;
	        mps_chg_current_setting(PC_MODE);
	}
		
	power_supply_changed(&mps->battery);
	power_supply_changed(&mps->mains);
	power_supply_changed(&mps->usb);
	/* Enable wakeup 0 interrupt */
#if 0
	for(i=0;i<35;i++){
		if(REG8_VAL(GPIO_BASE_ADD+0x300+i) & BIT7)
			printk("0x%x = 0x%x \n",0x300+i,REG8_VAL(GPIO_BASE_ADD+0x300+i));

	}
#endif         
	/*  Enable GPIO 5 interrupt */
        chgdon_irq_enable(TRUE);
	return 0;
}
#endif

static struct mps_battery_info mps_battery_pdata = {
        .charge_gpio    = 5,
        .max_voltage    = 4000,
        .min_voltage    = 3000,
        .batt_mult      = 1000,
        .batt_div       = 414,
        .temp_mult      = 1,
        .temp_div       = 1,
        .batt_tech      = POWER_SUPPLY_TECHNOLOGY_LION,
        .batt_name      = "mp-battery",
};

static struct platform_device mps_battery_device = {
	.name           = "mps-battery",
	.id             = -1,
	.dev            = {
		.platform_data = &mps_battery_pdata,
	
	},
};


static struct platform_driver mps_driver = {
        .driver	= {
		.name	= "mps-battery",
		.owner	= THIS_MODULE,
	},
	.probe	=	mps_probe,
        .remove	=	__devexit_p(mps_remove),
#ifdef CONFIG_PM
	.suspend = mps_suspend,
	.resume = mps_resume,
#endif

};

static int __init mps_init(void)
{
        int ret;
	//printk(" Bow [%s]_%d\n",__func__,__LINE__);

	ret = env_parser();
	if (ret != 0) {
		printk(" MPS driver unmount. \n");
		return -ENODEV;
	}

	ret = platform_device_register(&mps_battery_device);
	if (ret != 0) {
		return -ENODEV;
	}
#if 0	
#ifdef CONFIG_PM
	register_pm_notifier(&mps_pm_notifier);
#endif
#endif
	return platform_driver_register(&mps_driver);
}
module_init(mps_init);

static void __exit mps_exit(void)
{
	platform_driver_unregister(&mps_driver);
	platform_device_unregister(&mps_battery_device);
}
module_exit(mps_exit);

MODULE_AUTHOR("WondarMedia  <wondarmedia.com.tw>");
MODULE_DESCRIPTION("MPS battery charger driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("i2c:mps");
