/*++
	drivers/power/wmt_battery.c

	Copyright (c) 2008 WonderMedia Technologies, Inc.

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
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/power_supply.h>
#include <linux/spinlock.h>
#include <linux/interrupt.h>
#include <linux/gpio.h>
#include <linux/io.h>
#include <mach/hardware.h>
#include "wmt_battery.h"
#include <linux/suspend.h>


static void gpio_ini(void)
{
	char buf[800];                                                                                                  
	char varname[] = "wmt.gpi.bat";                                                                                
	int varlen = 800;                                                                                               
	if (wmt_getsyspara(varname, buf, &varlen) == 0){                                                                                                                               
		    sscanf(buf,"[%x:%x:%x:%x:%x:%x:%x:%x]",              

	            &batlow.name,                                                                             
	            &batlow.active,                                                                           
	            &batlow.bitmap,                                                                           
	            &batlow.ctradd,                                                                           
	            &batlow.icaddr,                                                                           
	            &batlow.idaddr,                                                                           
	            &batlow.ipcaddr,                                                                          
	            &batlow.ipdaddr);                                                                       

		batlow.ctradd += WMT_MMAP_OFFSET;
		batlow.icaddr += WMT_MMAP_OFFSET;
		batlow.idaddr += WMT_MMAP_OFFSET;
		batlow.ipcaddr += WMT_MMAP_OFFSET;
		batlow.ipdaddr += WMT_MMAP_OFFSET;

		if(batlow.name != 2 ){
			batt_operation = 0;
			return;
		}
		if(batlow.name){
			REG32_VAL(batlow.ctradd) = REG32_VAL(batlow.ctradd) | batlow.bitmap;
			REG32_VAL(batlow.icaddr) = REG32_VAL(batlow.icaddr) & (~batlow.bitmap);
			REG32_VAL(batlow.ipcaddr) = REG32_VAL(batlow.ipcaddr) | batlow.bitmap;
			if(batlow.active & 0x10){
				REG32_VAL(batlow.ipdaddr) = REG32_VAL(batlow.ipdaddr) | batlow.bitmap;
			}else{
			        REG32_VAL(batlow.ipdaddr) = REG32_VAL(batlow.ipdaddr) & (~batlow.bitmap);
			}
		}
	}else{
			batt_operation = 0;
	}
}

static int wmt_batt_init(void){
	int varlen = 800;
	char buf[varlen];
	char varname[] = "wmt.io.bat";
	char tmp_buf[varlen];
	int batt_en = 0 ;
	int retval;
	int i = 0;
	int j = 0;

	printk(" wmt_batt_init  \n");
	retval = wmt_getsyspara(varname, buf, &varlen);
	if (retval == 0) {
		batt_en = (buf[i] - '0' == 1)?1:0;
		if (batt_en == 0) {        /*For EVB*/
			batt_operation = 0;
			ADC_USED= 0;
			batt_cal = 0;	
			goto out;
		}
		i += 2;
		for (; i < varlen; ++i) {
			if (buf[i] == ':')
				break;
			tmp_buf[j] = buf[i];
			++j;
		}
		if (!(strncmp(tmp_buf,"vt1603",6))) {/*For MID*/
			batt_operation = 1;
			ADC_USED= 1;
			batt_cal = 0;	
			if (!(strncmp(tmp_buf,"vt1603k",7))) {/*For MID ADC value calibration*/
				batt_cal = 1;
			}
		}else{

			batt_operation = 1;
			ADC_USED= 0;
			batt_cal = 0;	
			goto out;
		}

		++i;

		sscanf((buf+i),"%d:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x",
			    &batt_update_time,
			    &batt_charge.maxi,
			    &batt_charge.mini,
			    &batt_discharge.maxi,
			    &batt_discharge.v9,
			    &batt_discharge.v8,
			    &batt_discharge.v7,
			    &batt_discharge.v6,
			    &batt_discharge.v5,
			    &batt_discharge.v4,
			    &batt_discharge.v3,
			    &batt_discharge.v2,
			    &batt_discharge.v1,
			    &batt_discharge.mini,
			    &batt_charge.v9,
			    &batt_charge.v8,
			    &batt_charge.v7,
			    &batt_charge.v6,
			    &batt_charge.v5,
			    &batt_charge.v4,
			    &batt_charge.v3,
			    &batt_charge.v2,
			    &batt_charge.v1
		);

		polling_interval = batt_update_time;
	} else {
		batt_operation = 0;
		ADC_USED=0;
		batt_cal = 0;	
		goto out;
	}
out:	

#if 0
	printk(("tmp_buf = %s \n"),tmp_buf);
	printk(("batt_operation = %x \n"),batt_operation);
	printk(("batt_cal = %x \n"),batt_cal);
	printk(("ADC_USED = %x \n"),ADC_USED);
	printk(("batt_update_time = %d \n"),batt_update_time);
	printk(("batt_charge_max = 0x%x \n"),batt_charge.maxi);
	printk(("batt_charge_v9 = 0x%x \n"),batt_charge.v9);
	printk(("batt_charge_v8 = 0x%x \n"),batt_charge.v8);
	printk(("batt_charge_v7 = 0x%x \n"),batt_charge.v7);
	printk(("batt_charge_v6 = 0x%x \n"),batt_charge.v6);
	printk(("batt_charge_v5 = 0x%x \n"),batt_charge.v5);
	printk(("batt_charge_v4 = 0x%x \n"),batt_charge.v4);
	printk(("batt_charge_v3 = 0x%x \n"),batt_charge.v3);
	printk(("batt_charge_v2 = 0x%x \n"),batt_charge.v2);
	printk(("batt_charge_v1 = 0x%x \n"),batt_charge.v1);
	printk(("batt_charge_min = 0x%x \n"),batt_charge.mini);
	printk(("batt_discharge_max = 0x%x \n"),batt_discharge.maxi);
	printk(("batt_discharge_v9 = 0x%x \n"),batt_discharge.v9);
	printk(("batt_discharge_v8 = 0x%x \n"),batt_discharge.v8);
	printk(("batt_discharge_v7 = 0x%x \n"),batt_discharge.v7);
	printk(("batt_discharge_v6 = 0x%x \n"),batt_discharge.v6);
	printk(("batt_discharge_v5 = 0x%x \n"),batt_discharge.v5);
	printk(("batt_discharge_v4 = 0x%x \n"),batt_discharge.v4);
	printk(("batt_discharge_v3 = 0x%x \n"),batt_discharge.v3);
	printk(("batt_discharge_v2 = 0x%x \n"),batt_discharge.v2);
	printk(("batt_discharge_v1 = 0x%x \n"),batt_discharge.v1);
	printk(("batt_discharge_min = 0x%x \n"),batt_discharge.mini);
#endif
	return 0;
	
}


static int wmt_read_dcin_DT(struct power_supply *bat_ps)
{
	struct power_supply *status;
	union power_supply_propval val_charger;
	
	status = NULL;

	status = power_supply_get_by_name("ac");

	if(status != NULL)
	{
		status->get_property(status, POWER_SUPPLY_PROP_ONLINE, &val_charger);
	}else{
		val_charger.intval = 0;
	}
		
	return val_charger.intval;
}

static int wmt_read_usb_DT(struct power_supply *bat_ps)
{

	struct power_supply *status;
	union power_supply_propval val_charger;
	
	status = NULL;

	status = power_supply_get_by_name("usb");

	if(status != NULL)
	{
		status->get_property(status, POWER_SUPPLY_PROP_ONLINE, &val_charger);
	}else{
		val_charger.intval = 0;
	}

	return val_charger.intval;

}
static unsigned int wmt_read_status(struct power_supply *bat_ps)
{
	struct power_supply *status;
	union power_supply_propval val_charger;
	
	status = NULL;

	status = power_supply_get_by_name("charger");

	if(status != NULL)
	{
		status->get_property(status, POWER_SUPPLY_PROP_STATUS, &val_charger);
	}else{
		val_charger.intval = POWER_SUPPLY_STATUS_UNKNOWN;
	}
	
	return val_charger.intval;
}

#ifdef CONFIG_VT1603_BATTERY_ENABLE
extern unsigned int vt1603_get_bat_info(void);
#endif
static unsigned short hx_batt_read(void)
{
	unsigned short spi_buf = 0; 

    if(ADC_USED){	
#ifdef CONFIG_VT1603_BATTERY_ENABLE
		spi_buf = vt1603_get_bat_info();
#endif
    	}
         /*printk("spi_buf = 0x%x \n ",spi_buf);*/
	return spi_buf;
}

static unsigned long wmt_read_capacity(struct power_supply *bat_ps)
{
	unsigned int capacity=0;
	unsigned short ADC_val = 0;
	
	/*printk("read capacity...\n");*/
	ADC_val = hx_batt_read();
	/*printk("ADC value = 0x%x  \n",ADC_val);*/

	if (batt_cal){
		printk("ADC value = 0x%x  \n",ADC_val);
	}

	if(bat_status== POWER_SUPPLY_STATUS_DISCHARGING){
		if(ADC_val >=  batt_discharge.maxi){
			capacity = 100;
		} else if(ADC_val >= batt_discharge.v9){
			capacity = (((ADC_val - batt_discharge.v9) * 10) / (batt_discharge.maxi - batt_discharge.v9))+90;
		}else if(ADC_val >= batt_discharge.v8){
			capacity =(((ADC_val - batt_discharge.v8) * 10) / (batt_discharge.v9 - batt_discharge.v8))+80;
		} else if(ADC_val >= batt_discharge.v7){
			capacity = (((ADC_val - batt_discharge.v7) * 10) / (batt_discharge.v8 - batt_discharge.v7))+70;
		}else if(ADC_val >= batt_discharge.v6){
			capacity = (((ADC_val - batt_discharge.v6) * 10) / (batt_discharge.v7 - batt_discharge.v6))+60;
		} else if(ADC_val >= batt_discharge.v5){
			capacity = (((ADC_val - batt_discharge.v5) * 10) / (batt_discharge.v6 - batt_discharge.v5))+50;
		}else if(ADC_val >= batt_discharge.v4){
			capacity = (((ADC_val - batt_discharge.v4) * 10) / (batt_discharge.v5 - batt_discharge.v4))+40;
		}else if(ADC_val >= batt_discharge.v3){
			capacity = (((ADC_val - batt_discharge.v3) * 10) / (batt_discharge.v4 - batt_discharge.v3))+30;
		} else if(ADC_val >= batt_discharge.v2){
			capacity = (((ADC_val - batt_discharge.v2) * 10) / (batt_discharge.v3 - batt_discharge.v2))+20;
		}else if(ADC_val >= batt_discharge.v1){
			capacity = (((ADC_val - batt_discharge.v1) * 10) / (batt_discharge.v2 - batt_discharge.v1))+10;
		} else if(ADC_val >= batt_discharge.mini){
			capacity = (((ADC_val - batt_discharge.mini) * 10) / (batt_discharge.v1 - batt_discharge.mini));
		} else {
			capacity = 0;
		}
		//printk("DISCHARGING Capacity = %d \n",capacity);
	}else{/*   For charging  */
		if(ADC_val >= batt_charge.maxi){
			capacity = 100;
		}else if(ADC_val >= batt_charge.v9){
			capacity = (((ADC_val - batt_charge.v9) * 10) / (batt_charge.maxi - batt_charge.v9))+90;
		}else if(ADC_val >= batt_charge.v8){
			capacity = (((ADC_val - batt_charge.v8) * 10) / (batt_charge.v9 - batt_charge.v8))+80;
		}else if(ADC_val >= batt_charge.v7){
			capacity = (((ADC_val - batt_charge.v7) * 10) / (batt_charge.v8 - batt_charge.v7))+70;
		}else if(ADC_val >= batt_charge.v6){
			capacity = (((ADC_val - batt_charge.v6) * 10) / (batt_charge.v7 - batt_charge.v6))+60;
		}else if(ADC_val >= batt_charge.v5){
			capacity =( ((ADC_val - batt_charge.v5) * 10) / (batt_charge.v6 - batt_charge.v5))+50;
		}else if(ADC_val >= batt_charge.v4){
			capacity = (((ADC_val - batt_charge.v4) * 10) / (batt_charge.v5 - batt_charge.v4))+40;
		}else if(ADC_val >= batt_charge.v3){
			capacity = (((ADC_val - batt_charge.v3) * 10) / (batt_charge.v4 - batt_charge.v3))+30;
		}else if(ADC_val >= batt_charge.v2){
			capacity = (((ADC_val - batt_charge.v2) * 10) / (batt_charge.v3 - batt_charge.v2))+20;
		}else if(ADC_val >= batt_charge.v1){
			capacity = (((ADC_val - batt_charge.v1) * 10) / (batt_charge.v2 - batt_charge.v1))+10;
		}else if(ADC_val >= batt_charge.mini){
			capacity = (((ADC_val - batt_charge.mini) * 10 )/ (batt_charge.v1 - batt_charge.mini));
		}else {
			capacity = 0;
		}
		//printk("CHARGING Capacity = %d \n",capacity);
	}

	return (int)capacity;
}


static int wmt_bat_get_property(struct power_supply *bat_ps,
			    enum power_supply_property psp,
			    union power_supply_propval *val)
{

	switch (psp) {

	case POWER_SUPPLY_PROP_CAPACITY:
		val->intval = bat_capacity;
		break;	

	default:
		return -EINVAL;
	}
	return 0;
}

static void wmt_bat_external_power_changed(struct power_supply *bat_ps)
{
	schedule_work(&bat_work);
}

static void wmt_bat_update(struct power_supply *bat_ps)
{
	unsigned int current_percent = 0;
	/* static unsigned int capacity_first_update; */
	static unsigned int  j;
	/*int i;*/
	
	mutex_lock(&work_lock);
	if(batt_operation){
		//bat_low = wmt_read_batlow_DT(bat_ps);
		bat_low =0;
		bat_dcin = wmt_read_dcin_DT(bat_ps);
		bat_usbin = wmt_read_usb_DT(bat_ps);
		bat_status = wmt_read_status(bat_ps);

		if((bat_low) && (!bat_dcin) && (!bat_usbin)){
	         /* If battery low signal occur. */ 
			 /*Setting battery capacity is empty. */ 
			 /* System will force shut down. */
			printk("Battery Low. \n");
			bat_capacity = 0;
		}else{		
		        if (!ADC_USED){
			/*printk(" Droid book. \n");*/
				/*bat_status = wmt_read_status(bat_ps);*/
				if(bat_dcin || bat_usbin){  /* Charging */
				    /*printk("\n bat_status = %d\n",bat_status);*/
					if(bat_status== POWER_SUPPLY_STATUS_DISCHARGING){
						bat_capacity = 100;
					}else{
						bat_capacity = 50;
					}         
				}else{ /* Discharging */
					bat_capacity = 100;
				}
			}else{
			     /*printk(" MID. \n");*/
				/*bat_status = wmt_read_status(bat_ps);*/
				if(bat_dcin || bat_usbin){   /* Charging */
					if(bat_status == POWER_SUPPLY_STATUS_FULL){
						bat_capacity = 100;
					}else{
					    /*printk("\ bat_status = %d\n",bat_status);*/
						//if(bat_status== POWER_SUPPLY_STATUS_DISCHARGING){
						//	bat_capacity = 100;
						//	bat_status = POWER_SUPPLY_STATUS_FULL;
						//}else{
							current_percent = wmt_read_capacity(bat_ps);
							
							/*  For AC plug from out to in.  */
							if (!charging_first_update) {
								temp_buffer[data_num] = current_percent;
								if(data_num >= avgnum){
									data_num=0;
								    avg = 1;  
								}else
									data_num++;
								
								total_buffer  = 0;
								for( j=0;j <= avgnum ; j++){
									total_buffer = total_buffer + temp_buffer[j];
								}
								
								if(avg)
								    current_percent = total_buffer/(avgnum+1);

								if(bat_temp_capacity < current_percent)
									bat_temp_capacity = current_percent;

								discharging_first_update = 1;
								charging_first_update =0;
							}else{
								discharging_first_update = 1;
								charging_first_update =0;
								if(total_buffer == 0){
		 						      if((current_percent <= (bat_temp_capacity -4)) ||
		 							  (current_percent >= (bat_temp_capacity + 4))	)
									      bat_temp_capacity = current_percent;
								         bat_capacity_old  = 0;
								    }
							}

							
			
							if((bat_temp_capacity < 0) || (bat_temp_capacity > 99)){
								if(bat_temp_capacity < 0){
									bat_capacity = 0;
								}else{
									bat_capacity = 99;
								}
							}else{
							        if(bat_usbin)
									bat_capacity = bat_temp_capacity;
								else if((bat_capacity < bat_temp_capacity) || bat_usbin)
									bat_capacity = bat_temp_capacity;
							}
						//}
					}
				}else{  /* Discharging */
				        current_percent = wmt_read_capacity(bat_ps);

					if (!discharging_first_update) {
						temp_buffer[data_num] = current_percent;
						if(data_num>= avgnum){
							data_num=0;
						    avg = 1;  
						}else
							data_num++;
						
						total_buffer = 0;
						for( j=0 ;j <= avgnum ; j++){
							total_buffer = total_buffer + temp_buffer[j];
						}

						if(avg)
						    current_percent = total_buffer/(avgnum+1);

						if(bat_temp_capacity > current_percent)
							bat_temp_capacity = current_percent;
					}else{
						charging_first_update = 1;
						discharging_first_update = 0;
						if(total_buffer == 0){
						      if((current_percent <= (bat_temp_capacity -4)) ||
							  (current_percent >= (bat_temp_capacity + 4))	)
						              bat_temp_capacity = current_percent;
						      bat_capacity_old  = 100;
						}
					}

				        if((bat_temp_capacity <= 0) || (bat_temp_capacity > 100)){
						if(bat_temp_capacity <= 0){
							bat_capacity = 0;
						}else{
							bat_capacity = 100;
						}
				        }else{
				                if(g_boot_first){
							g_boot_first = 0;		
							bat_capacity = bat_temp_capacity;
					        }else{
							if(bat_capacity > bat_temp_capacity ){
				                                if (bat_temp_capacity > (bat_capacity + 2)){
									bat_capacity = bat_capacity - 1;
								}else{
									bat_capacity = bat_temp_capacity;
								}
							}
					        }
					}
				}

			}

	    }
	}else{
		/*printk(" RDK \n");*/
		bat_status= POWER_SUPPLY_STATUS_FULL;
		bat_capacity = 100;
	}

#if 0
    printk("****************************************************************************  \n");
	printk("current_percent = %d \n",current_percent);
	printk("bat_temp_capacity = %d \n",bat_temp_capacity);
	printk("capacity %d \n",bat_capacity);
	//printk("bat_dcin = %s, bat_dcin_old = %s  \n",bat_dcin,bat_dcin_old);
	//printk("bat_usbin = %s \n",bat_usbin);	
    printk("****************************************************************************  \n");
#endif

#if 0
    printk("****************************************************************************  \n");
	printk(" avg = %d \n",avg);
	printk("total_buffer = %d \n",total_buffer);
	for( i=0;i <= avgnum;i++)
           	printk("temp_buffer[%d] = %d, \n",i ,temp_buffer[i]);
	printk("bat_low = %d, bat_low_old = %d  \n",bat_low,bat_low_old);
	printk("bat_dcin = %d, bat_dcin_old = %d  \n",bat_dcin,bat_dcin_old);
	/* printk("bat_health = %d, bat_health_old = %d \n",bat_health,bat_health_old); */
	/* printk("bat_online = %d, bat_online_old = %d  \n",bat_online,bat_online_old); */
	printk("bat_status = %d, bat_status_old = %d  \n",bat_status,bat_status_old);
	printk("cur_capacity = %d, bat_capacity_old = %d  \n",current_percent,bat_capacity_old);
	printk("bat_temp_capacity = %d ,bat_capacity = %d \n",bat_temp_capacity,bat_capacity);
	/* printk("first_update = %d  \n",first_update); */
	printk("discharging_first_update = %d  \n",discharging_first_update);
	printk("charging_first_update = %d  \n",charging_first_update);
	printk("****************************************************************************  \n");
#endif
	if (batt_cal){
		printk("****************************************************************************  \n");
		printk("bat_low = %d, bat_low_old = %d  \n",bat_low,bat_low_old);
		printk("bat_dcin = %d, bat_dcin_old = %d  \n",bat_dcin,bat_dcin_old);
		printk("bat_status = %d, bat_status_old = %d  \n",bat_status,bat_status_old);
		printk("bat_capacity = %d, bat_capacity_old = %d  \n",current_percent,bat_capacity_old);
		printk("****************************************************************************  \n");
                 bat_capacity = 100;
	}

	
	if((bat_capacity != bat_capacity_old) ||
            (bat_capacity <= 0)){
		power_supply_changed(bat_ps);
	}

	bat_capacity_old = bat_capacity;
	mutex_unlock(&work_lock);
}


static enum power_supply_property wmt_bat_props[] = {
    POWER_SUPPLY_PROP_CAPACITY,
};

static struct power_supply bat_ps = {
	.name					= "battery",
	.type					= POWER_SUPPLY_TYPE_BATTERY,
	.get_property			= wmt_bat_get_property,
	.external_power_changed = wmt_bat_external_power_changed,
	.properties 			= wmt_bat_props,
	.num_properties 		= ARRAY_SIZE(wmt_bat_props),
	.use_for_apm			= 1,
};

static void wmt_battery_work(struct work_struct *work)
{
	wmt_bat_update(&bat_ps);
}


static void polling_timer_func(unsigned long unused)
{
	schedule_work(&bat_work);
	mod_timer(&polling_timer,
		  jiffies + msecs_to_jiffies(polling_interval));
}

#ifdef CONFIG_PM
static int wmt_battery_suspend(struct platform_device *dev, pm_message_t state)
{
	/*flush_scheduled_work();*/
	del_timer(&polling_timer);
	
	if(ADC_USED){	
		
	}

	return 0;
}

static int wmt_battery_resume(struct platform_device *dev)
{
  int i;
	bool bat_low =0;
	
	wmt_batt_init();
	
	if(batt_operation){
		//gpio_ini();
	  //bat_low = wmt_read_batlow_DT(&bat_ps);
			  
	  if (bat_low) {
	      bat_capacity = 0;
			  printk("*** resume of bat low \n");
			  power_supply_changed(&bat_ps);	
		}else{
    		if(ADC_USED){	
			avg = 0;
			data_num = 0;
			for( i=0;i <= avgnum;i++)
				temp_buffer[i]=0;
			total_buffer =0;
    		}
	  }
	  /*schedule_work(&bat_work);*/
	  setup_timer(&polling_timer, polling_timer_func, 0);
	  mod_timer(&polling_timer,
		jiffies + msecs_to_jiffies(polling_interval));
    first_update = 0;
    charging_first_update = 1;
    discharging_first_update = 1;
  }
	return 0;
}
#else
#define wmt_battery_suspend NULL
#define wmt_battery_resume NULL
#endif


static int __devinit wmt_battery_probe(struct platform_device *dev)
{
	int ret = 0;

	/*printk(" Probe... +++++   \n");*/
	pdata = dev->dev.platform_data;
	if (dev->id != -1)
		return -EINVAL;

	mutex_init(&work_lock);

	if (!pdata) {
		dev_err(&dev->dev, "Please use wmt_bat_set_pdata\n");
		return -EINVAL;
	}

	INIT_WORK(&bat_work, wmt_battery_work);

	ret = power_supply_register(&dev->dev, &bat_ps);
	if (!ret)
		schedule_work(&bat_work);
	else
		goto err;

	setup_timer(&polling_timer, polling_timer_func, 0);
	/*mod_timer(&polling_timer,
			  jiffies + msecs_to_jiffies(polling_interval));*/
	mod_timer(&polling_timer,
			  jiffies + msecs_to_jiffies(60000));

	return 0;
err:
	return ret;
}

static int __devexit wmt_battery_remove(struct platform_device *dev)
{

	flush_scheduled_work();
	del_timer_sync(&polling_timer);
	
	/*printk("remove...\n");*/
	power_supply_unregister(&bat_ps);
	//power_supply_unregister(&ac_ps);

	kfree(wmt_bat_props);
	//kfree(wmt_ac_props);
	return 0;
}

static struct wmt_battery_info wmt_battery_pdata = {
        .charge_gpio    = 5,
        .max_voltage    = 4000,
        .min_voltage    = 3000,
        .batt_mult      = 1000,
        .batt_div       = 414,
        .temp_mult      = 1,
        .temp_div       = 1,
        .batt_tech      = POWER_SUPPLY_TECHNOLOGY_LION,
        .batt_name      = "battery",
};


static struct platform_device wmt_battery_device = {
	.name           = "wmt-battery",
	.id             = -1,
	.dev            = {
		.platform_data = &wmt_battery_pdata,
	
	},
};


static struct platform_driver wmt_battery_driver = {
	.driver	= {
		.name	= "wmt-battery",
		.owner	= THIS_MODULE,
	},
	.probe		= wmt_battery_probe, 
	.remove		= __devexit_p(wmt_battery_remove),
	.suspend	= wmt_battery_suspend,
	.resume		= wmt_battery_resume,
};

static int __init wmt_battery_init(void)
{
	int ret;
	
	wmt_batt_init();

	if ((batt_operation == 1) && (ADC_USED == 0 ) ){
	        printk(" 1603 battery driver Unmounted... \n");
		return -ENODEV;
	}
	
	gpio_ini();
	g_boot_first = 1;		
	printk(" 1603 battery driver mounted... \n");
	ret = platform_device_register(&wmt_battery_device);
	if (ret != 0) {
		/*DPRINTK("End1 ret = %x\n",ret);*/
		return -ENODEV;
	}
	ret = platform_driver_register(&wmt_battery_driver);
	
	return ret;
}
static void __exit wmt_battery_exit(void)
{
	platform_driver_unregister(&wmt_battery_driver);
	platform_device_unregister(&wmt_battery_device);
}


late_initcall(wmt_battery_init);
module_exit(wmt_battery_exit);

MODULE_AUTHOR("WonderMedia Technologies, Inc.");
MODULE_DESCRIPTION("WONDERMEDIA ADC battery driver");
MODULE_LICENSE("GPL");

