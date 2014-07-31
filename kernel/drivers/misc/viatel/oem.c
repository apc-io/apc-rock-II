/*
 * viatel_cbp_oem.c
 *
 * VIA CBP driver for Linux
 *
 * Copyright (C) 2011 VIA TELECOM Corporation, Inc.
 * Author: VIA TELECOM Corporation, Inc.
 *
 * This package is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * THIS PACKAGE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#include <linux/interrupt.h>
#include <mach/viatel.h>
#include <linux/gpio.h>
#include <mach/wmt_iomux.h>
#include <mach/hardware.h>
#include "linux/delay.h"

#if 1

//#define VIATEL_DEBUG
#ifdef VIATEL_DEBUG
#define wmt_dbg(fmt, arg...)   printk(fmt, ##arg)
#else 
#define wmt_dbg(fmt, arg...) 
#endif

int isviatelcom = 0;
int wk1_high_active = 0;
int gpio_viatel_4wire[4];
int ap_ready_always = 0;
unsigned int wakeup_reg_val;
unsigned int wakeup_shift_bit;
int wakeup_type_mask;

extern int wmt_getsyspara(char *varname, char *varval, int *varlen);

int oem_gpio_convert_init(void){
	char buf[256];
	int varlen = 256;	
	if( wmt_getsyspara("wmt.modem.viatel.4wire",buf,&varlen) == 0) 
	{
		sscanf(buf,"%d:%d:%d:%d",&gpio_viatel_4wire[GPIO_VIATEL_USB_AP_RDY],
            &gpio_viatel_4wire[GPIO_VIATEL_USB_MDM_RDY],
            &gpio_viatel_4wire[GPIO_VIATEL_USB_AP_WAKE_MDM],
            &gpio_viatel_4wire[GPIO_VIATEL_USB_MDM_WAKE_AP]);
        
		printk("4wire %d:%d:%d:%d\n",gpio_viatel_4wire[GPIO_VIATEL_USB_AP_RDY],
            gpio_viatel_4wire[GPIO_VIATEL_USB_MDM_RDY],
            gpio_viatel_4wire[GPIO_VIATEL_USB_AP_WAKE_MDM],
            gpio_viatel_4wire[GPIO_VIATEL_USB_MDM_WAKE_AP]);
        
		isviatelcom = 1;
		printk("disable wakeup1 %s %d\n",__FUNCTION__,__LINE__);


        if(gpio_viatel_4wire[GPIO_VIATEL_USB_MDM_WAKE_AP]==149){
            wakeup_reg_val=0x1L;
            wakeup_shift_bit = 0;
            wakeup_type_mask = 0xfffffff8;
        }else{
            wakeup_reg_val=0x4L;
            wakeup_shift_bit = 8;
            wakeup_type_mask = 0xfffff8ff;
        }


   		PMCIE_VAL &=(~wakeup_reg_val);
    	INT_TRG_EN_VAL&=(~wakeup_reg_val);

        if(wmt_getsyspara("wmt.modem.viatel.aprdy",buf,&varlen) == 0){
            sscanf(buf,"%d",&ap_ready_always);
        }else{
            ap_ready_always = 0;/* default value */
        }
        
        return 0;
	}

	return -1;	
}

int oem_gpio_request(int gpio, const char *label)
{
	wmt_dbg("%s index=%d gpio=%d\n",__FUNCTION__,gpio,gpio_viatel_4wire[gpio]);
    gpio_request(gpio_viatel_4wire[gpio], label);
	return 0;
}

void oem_gpio_free(int gpio)
{
	wmt_dbg("%s index=%d gpio=%d\n",__FUNCTION__,gpio,gpio_viatel_4wire[gpio]);
    gpio_free(gpio_viatel_4wire[gpio]);
	return ;
}

/*config the gpio to be input for irq if the SOC need*/
int oem_gpio_direction_input_for_irq(int gpio)
{
	if(!isviatelcom)
		return 0;
	wmt_dbg("%s index=%d gpio=%d\n",__FUNCTION__,gpio,gpio_viatel_4wire[gpio]);
    
	wmt_gpio_setpull(gpio_viatel_4wire[gpio], WMT_GPIO_PULL_DOWN);
    gpio_direction_input(gpio_viatel_4wire[gpio]);
    gpio_re_enabled(gpio_viatel_4wire[gpio]);
	
	return 0;
}

int oem_gpio_direction_output(int gpio, int value)
{
	if(!isviatelcom)
		return 0;
	wmt_dbg("%s index=%d gpio=%d\n",__FUNCTION__,gpio,gpio_viatel_4wire[gpio]);
    if(ap_ready_always&&(gpio == GPIO_VIATEL_USB_AP_RDY))
        return 0;

	
    gpio_re_enabled(gpio_viatel_4wire[gpio]);
	gpio_direction_output(gpio_viatel_4wire[gpio], value);
    
	return 0;
}

int oem_gpio_output(int gpio, int value)
{
	if(!isviatelcom)
		return 0;
	wmt_dbg("%s index=%d gpio=%d\n",__FUNCTION__,gpio,gpio_viatel_4wire[gpio]);
	
    gpio_re_enabled(gpio_viatel_4wire[gpio]);
	gpio_direction_output(gpio_viatel_4wire[gpio], value);
    
	return 0;
}

int oem_gpio_is_wakeup(int gpio)
{
	if(gpio==149||gpio==152) //wekup0 gpio index WMT_PIN_GP62_WAKEUP0
		return 1;
	else
		return 0;
}


/* 
 * Get the output level if the gpio is output type; 
 * Get the input level if the gpio is input type
 */
int oem_gpio_get_value(int gpio)
{
	int rtn;
	if(!isviatelcom)
		return 0;
	wmt_gpio_setpull(gpio_viatel_4wire[gpio], WMT_GPIO_PULL_DOWN);
    gpio_direction_input(gpio_viatel_4wire[gpio]);
    gpio_re_enabled(gpio_viatel_4wire[gpio]);
	rtn = __gpio_get_value(gpio_viatel_4wire[gpio]);
	wmt_dbg("%s index=%d gpio=%d,rtn=%d\n",__FUNCTION__,gpio,gpio_viatel_4wire[gpio],rtn);
	if(oem_gpio_is_wakeup(gpio_viatel_4wire[gpio])&&(!wk1_high_active))
		return !rtn;
	else
		return rtn;
}




int oem_gpio_to_irq(int gpio)
{
	wmt_dbg("%s index=%d gpio=%d\n",__FUNCTION__,gpio,gpio_viatel_4wire[gpio]);
	return IRQ_GPIO;
}

/* 
 * Set the irq type of the pin. 
 * Get the pin level and set the correct edge if the type is both edge and 
 * the SOC do not support both edge detection at one time
 */
int oem_gpio_set_irq_type(int gpio, unsigned int type)
{
	if(!isviatelcom)
		return 0;
	
	wmt_dbg("%s index=%d gpio=%d\n",__FUNCTION__,gpio,gpio_viatel_4wire[gpio]);
	if(oem_gpio_is_wakeup(gpio_viatel_4wire[gpio])){
		int wakeup_type;
		if(type==IRQF_TRIGGER_RISING){
			if(wk1_high_active)
				wakeup_type=(0x3<<wakeup_shift_bit);
			else
				wakeup_type=(0x2<<wakeup_shift_bit);
		}
		else if(type==(IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING))
			wakeup_type=(0x4<<wakeup_shift_bit);		
		else{
			printk("\n\n\n\n\n=======oem_gpio_set_irq_type error\n\n\n\n\n\n");
			return 0;
		}
        
		INT_TYPE0_VAL = (INT_TYPE0_VAL&wakeup_type_mask)|wakeup_type;
		return 0;
	}
	
	if(type < IRQF_TRIGGER_NONE ||type >=IRQF_TRIGGER_HIGH)
		printk("error %s %d gpio %d type 0x%x\n",__FUNCTION__,__LINE__,gpio_viatel_4wire[gpio],type);
	return wmt_gpio_set_irq_type(gpio_viatel_4wire[gpio], type);
}


int oem_gpio_request_irq(int gpio, irq_handler_t handler, unsigned long flags,
	    const char *name, void *dev)
{
	if(!isviatelcom)
		return 0;
	wmt_dbg("%s index=%d gpio=%d\n",__FUNCTION__,gpio,gpio_viatel_4wire[gpio]);
	if(oem_gpio_is_wakeup(gpio_viatel_4wire[gpio]))
		return 0;

    return request_irq(oem_gpio_to_irq(gpio), handler, flags, name, dev);
}

void oem_gpio_irq_mask(int gpio)
{
	if(!isviatelcom)
		return ;

	wmt_dbg("%s index=%d gpio=%d\n",__FUNCTION__,gpio,gpio_viatel_4wire[gpio]);
	if(oem_gpio_is_wakeup(gpio_viatel_4wire[gpio])){
		PMCIE_VAL &=(~wakeup_reg_val);
		INT_TRG_EN_VAL&=(~wakeup_reg_val);
		return ;
	}
	
	wmt_gpio_mask_irq(gpio_viatel_4wire[gpio]);

    return ;
}
void oem_gpio_irq_unmask(int gpio)
{
	if(!isviatelcom)
		return;
	
	wmt_dbg("%s index=%d gpio=%d\n",__FUNCTION__,gpio,gpio_viatel_4wire[gpio]);	
	if(oem_gpio_is_wakeup(gpio_viatel_4wire[gpio])){
        int i;
        for(i=0;i<100;i++){
    		PMCIE_VAL |=wakeup_reg_val;
    		INT_TRG_EN_VAL|=wakeup_reg_val;
            if(PMCIE_VAL&wakeup_reg_val){
                break;
            }
            printk("PMCIE_VAL  set error %x,retry %d\n", PMCIE_VAL,i);
            mdelay(10);
        }
		return ;
	}

	wmt_gpio_unmask_irq(gpio_viatel_4wire[gpio]);

    return ;
}

int oem_gpio_irq_isenable(int gpio){
	if(!isviatelcom)
		return 0;
	
	if(oem_gpio_is_wakeup(gpio_viatel_4wire[gpio]))
		return 1;
	
	return is_gpio_irqenable(gpio_viatel_4wire[gpio]);
}

int oem_gpio_irq_isint(int gpio){
	if(!isviatelcom)
		return 0;
	
	if(oem_gpio_is_wakeup(gpio_viatel_4wire[gpio]))
		return 1;

	return gpio_irqstatus(gpio_viatel_4wire[gpio]);

}
int oem_gpio_irq_clear(int gpio){
	if(!isviatelcom)
		return 1;
	
	if(oem_gpio_is_wakeup(gpio_viatel_4wire[gpio]))
		return 0;

	wmt_gpio_ack_irq(gpio_viatel_4wire[gpio]);

    return 0;
}

#endif
#if 0

#if defined(CONFIG_MACH_OMAP_KUNLUN)
int oem_gpio_request(int gpio, const char *label)
{
	return gpio_request(gpio, label);
}

void oem_gpio_free(int gpio)
{
    gpio_free(gpio);
}

/*config the gpio to be input for irq if the SOC need*/
int oem_gpio_direction_input_for_irq(int gpio)
{
	return gpio_direction_input(gpio);
}

int oem_gpio_direction_output(int gpio, int value)
{
	return gpio_direction_output(gpio, value);
}

/* 
 * Get the output level if the gpio is output type; 
 * Get the input level if the gpio is input type
 */
int oem_gpio_get_value(int gpio)
{
	return gpio_get_value(gpio);
}

int oem_gpio_to_irq(int gpio)
{
	return gpio_to_irq(gpio);
}

/* 
 * Set the irq type of the pin. 
 * Get the pin level and set the correct edge if the type is both edge and 
 * the SOC do not support both edge detection at one time
 */
int oem_gpio_set_irq_type(unsigned gpio, unsigned int type)
{
    return set_irq_type(oem_gpio_to_irq(gpio), type);
}


int oem_gpio_request_irq(int gpio, irq_handler_t handler, unsigned long flags,
	    const char *name, void *dev)
{
    return request_irq(oem_gpio_to_irq(gpio), handler, flags, name, dev);
}

void oem_gpio_irq_mask(int gpio)
{
    return ;
}

void oem_gpio_irq_unmask(int gpio)
{
    return ;
}



#endif

#if defined(CONFIG_SOC_JZ4770)
int oem_gpio_request(int gpio, const char *label)
{
	return gpio_request(gpio, label);
}

void oem_gpio_free(int gpio)
{
    gpio_free(gpio);
}

/*config the gpio to be input for irq if the SOC need*/
int oem_gpio_direction_input_for_irq(int gpio)
{
	return 0;
}

int oem_gpio_direction_output(int gpio, int value)
{
       return gpio_direction_output(gpio, value);
}

/* 
 * Get the output level if the gpio is output type; 
 * Get the input level if the gpio is input type
 */
int oem_gpio_get_value(int gpio)
{
	return gpio_get_value(gpio);
}

int oem_gpio_to_irq(int gpio)
{
	return gpio_to_irq(gpio);
}

#define GPIO_DEBOUNCE (3)
int read_gpio_pin(int pin)
{
	int t, v;
	int i;

	i = GPIO_DEBOUNCE;

	v = t = 0;

	while (i--) {
		t = __gpio_get_pin(pin);
		if (v != t)
			i = GPIO_DEBOUNCE;

		v = t;
		ndelay(100);
	}

	return v;
}

int oem_gpio_set_irq_type(int gpio, unsigned int type)
{
       if(type == IRQ_TYPE_EDGE_BOTH){
            if(read_gpio_pin(gpio)){
                type = IRQ_TYPE_EDGE_FALLING;
            }else{
                type = IRQ_TYPE_EDGE_RISING;
            }
       }

       if(type == IRQ_TYPE_LEVEL_MASK){
            if(read_gpio_pin(gpio)){
                type = IRQ_TYPE_LEVEL_LOW;
            }else{
                type = IRQ_TYPE_LEVEL_HIGH;
            }
       }

       switch(type){
            case IRQ_TYPE_EDGE_RISING:
                __gpio_as_irq_rise_edge(gpio);
                break;
            case IRQ_TYPE_EDGE_FALLING:
                __gpio_as_irq_fall_edge(gpio);
                break;
            case IRQ_TYPE_LEVEL_HIGH:
                __gpio_as_irq_high_level(gpio);
                break;
            case IRQ_TYPE_LEVEL_LOW:
                __gpio_as_irq_low_level(gpio);
                break;
            default:
                return -EINVAL;
       }
 
       return 0;
}

int oem_gpio_request_irq(int gpio, irq_handler_t handler, unsigned long flags,
	    const char *name, void *dev)
{
    int ret = request_irq(oem_gpio_to_irq(gpio), handler, flags, name, dev);
    enable_irq_wake(oem_gpio_to_irq(gpio));
    return ret;
}

void oem_gpio_irq_mask(int gpio)
{
    return ;
}

void oem_gpio_irq_unmask(int gpio)
{
    return ;
}
#endif

#if defined(EVDO_DT_SUPPORT)
#include <linux/interrupt.h>
int oem_gpio_request(int gpio, const char *label)
{
    return 0;
}

void oem_gpio_free(int gpio)
{
    return ;
}

/*config the gpio to be input for irq if the SOC need*/
int oem_gpio_direction_input_for_irq(int gpio)
{
    mt_set_gpio_mode(gpio, GPIO_MODE_02);
    mt_set_gpio_dir(gpio, GPIO_DIR_IN);
    return 0;
}

int oem_gpio_direction_output(int gpio, int value)
{
    mt_set_gpio_mode(gpio, GPIO_MODE_GPIO);
    mt_set_gpio_dir(gpio, GPIO_DIR_OUT);
    mt_set_gpio_out(gpio, !!value);
    return 0;
}

int oem_gpio_get_value(int gpio)
{
    if(GPIO_DIR_IN == mt_get_gpio_dir(gpio)){
        return mt_get_gpio_in(gpio);
    }else{
        return mt_get_gpio_out(gpio);
    }
}

typedef struct mtk_oem_gpio_des{
    int gpio;
    int irq;
    void (*redirect)(void);
    irq_handler_t handle;
    void *data;
}mtk_oem_gpio_des;

static void gpio_irq_handle_usb_mdm_rdy(void);
static void gpio_irq_handle_usb_mdm_wake_ap(void);
static void gpio_irq_handle_uart_mdm_wake_ap(void);
static void gpio_irq_handle_rst_ind(void);
static void gpio_irq_handle_pwr_ind(void);

mtk_oem_gpio_des oem_gpio_list[] = {
    {GPIO_VIATEL_USB_MDM_RDY,     4, gpio_irq_handle_usb_mdm_rdy, NULL, NULL},
    {GPIO_VIATEL_USB_MDM_WAKE_AP, 29, gpio_irq_handle_usb_mdm_wake_ap, NULL, NULL},
    {GPIO_VIATEL_MDM_RST_IND,  28, gpio_irq_handle_rst_ind, NULL, NULL},
    {GPIO_VIATEL_UART_MDM_WAKE_AP, 27, gpio_irq_handle_uart_mdm_wake_ap, NULL, NULL},
    {GPIO_VIATEL_MDM_PWR_IND, 27, gpio_irq_handle_pwr_ind, NULL, NULL},
};

static mtk_oem_gpio_des* gpio_des_find_by_gpio(int gpio)
{
    int i = 0;
    mtk_oem_gpio_des *des = NULL;

    if(gpio < 0){
        return NULL;
    }
    
    for(i=0; i < sizeof(oem_gpio_list) / sizeof(mtk_oem_gpio_des); i++){
        des = oem_gpio_list + i;
        if(des->gpio == gpio){
            return des;
        }
    }

    return NULL;
}

static mtk_oem_gpio_des* gpio_des_find_by_irq(int irq)
{
    int i = 0;
    mtk_oem_gpio_des *des = NULL;

    for(i=0; i < sizeof(oem_gpio_list) / sizeof(mtk_oem_gpio_des); i++){
        des = oem_gpio_list + i;
        if(des->irq == irq){
            return des;
        }
    }

    return NULL;
}
static void gpio_irq_handle_usb_mdm_rdy(void)
{
    int irq = 0;
    mtk_oem_gpio_des *des = NULL;

    des = gpio_des_find_by_gpio(GPIO_VIATEL_USB_MDM_RDY);
    if(des && des->handle){
        des->handle(des->irq, des->data);
    }
}
static void gpio_irq_handle_usb_mdm_wake_ap(void)
{
    int irq = 0;
    mtk_oem_gpio_des *des = NULL;

    des = gpio_des_find_by_gpio(GPIO_VIATEL_USB_MDM_WAKE_AP);
    if(des && des->handle){
        des->handle(des->irq, des->data);
    }
}

static void gpio_irq_handle_uart_mdm_wake_ap(void)
{
    int irq = 0;
    mtk_oem_gpio_des *des = NULL;

    des = gpio_des_find_by_gpio(GPIO_VIATEL_UART_MDM_WAKE_AP);
    if(des && des->handle){
        des->handle(des->irq, des->data);
    }
}

static void gpio_irq_handle_rst_ind(void)
{
    int irq = 0;
    mtk_oem_gpio_des *des = NULL;

    des = gpio_des_find_by_gpio(GPIO_VIATEL_MDM_RST_IND);
    if(des && des->handle){
        des->handle(des->irq, des->data);
    }
}
static void gpio_irq_handle_pwr_ind(void)
{
    int irq = 0;
    mtk_oem_gpio_des *des = NULL;

    des = gpio_des_find_by_gpio(GPIO_VIATEL_MDM_PWR_IND);
    if(des && des->handle){
        des->handle(des->irq, des->data);
    }
}

int oem_gpio_to_irq(int gpio)
{
    mtk_oem_gpio_des *des = NULL;

    des = gpio_des_find_by_gpio(gpio);
    if(NULL == des){
        printk("%s: no irq for gpio %d\n", __FUNCTION__, gpio);
        return -1;
    }else{
        return des->irq;
    }
}

int oem_irq_to_gpio(int irq)
{
    mtk_oem_gpio_des *des = NULL;

    des = gpio_des_find_by_irq(irq);
    if(NULL == des){
        printk("%s: no gpio for irq %d\n", __FUNCTION__, irq);
        return -1;
    }else{
        return des->gpio;
    }
}

int oem_gpio_set_irq_type(int gpio, unsigned int type)
{
    int irq, level;

    irq = oem_gpio_to_irq(gpio);
    if(irq < 0){
        return irq;
    }
   
    level = oem_gpio_get_value(gpio);
 
    if(type == IRQ_TYPE_EDGE_BOTH){
        if(level){
            type = IRQ_TYPE_EDGE_FALLING;
        }else{
            type = IRQ_TYPE_EDGE_RISING;
        }
    }

    if(type == IRQ_TYPE_LEVEL_MASK){
        if(level){
            type = IRQ_TYPE_LEVEL_LOW;
        }else{
            type = IRQ_TYPE_LEVEL_HIGH;
        }
    }

    mt65xx_eint_set_hw_debounce(irq, 3);
    switch(type){
        case IRQ_TYPE_EDGE_RISING:
            mt65xx_eint_set_sens(irq, MT65xx_EDGE_SENSITIVE);
            mt65xx_eint_set_polarity(irq, MT65xx_POLARITY_HIGH);
            break;
        case IRQ_TYPE_EDGE_FALLING:
            mt65xx_eint_set_sens(irq, MT65xx_EDGE_SENSITIVE);
            mt65xx_eint_set_polarity(irq, MT65xx_POLARITY_LOW);
            break;
        case IRQ_TYPE_LEVEL_HIGH:
            mt65xx_eint_set_sens(irq, MT65xx_LEVEL_SENSITIVE);
            mt65xx_eint_set_polarity(irq, MT65xx_POLARITY_HIGH);
            break;
        case IRQ_TYPE_LEVEL_LOW:
            mt65xx_eint_set_sens(irq, MT65xx_LEVEL_SENSITIVE);
            mt65xx_eint_set_polarity(irq, MT65xx_POLARITY_LOW);
            break;
        default:
            return -EINVAL;
   }
 
   return 0;
}

int oem_gpio_request_irq(int gpio, irq_handler_t handler, unsigned long flags,
	    const char *name, void *dev)
{
    mtk_oem_gpio_des *des = NULL;

    des = gpio_des_find_by_gpio(gpio);
    if(des == NULL){
        return -1;
    }
    des->data = dev;
    des->handle = handler;

    mt65xx_eint_registration(des->irq, 1, 1, des->redirect, 0);

    return 0;
}

void oem_gpio_irq_mask(int gpio)
{
    int irq;

    irq = oem_gpio_to_irq(gpio);
    if(irq < 0){
        return ;
    }

    mt65xx_eint_mask(irq);
}

void oem_gpio_irq_unmask(int gpio)
{
    int irq;

    irq = oem_gpio_to_irq(gpio);
    if(irq < 0){
        return ;
    }

    mt65xx_eint_unmask(irq);
}
#endif
#endif
