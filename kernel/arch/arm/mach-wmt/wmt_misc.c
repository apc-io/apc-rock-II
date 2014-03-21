/*++
	linux/arch/arm/mach-wmt/wmt_misc.c

	Copyright (c) 2013  WonderMedia Technologies, Inc.

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

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/delay.h> // for msleep
#include <mach/hardware.h>//for REG32_VAL
#include <linux/gpio.h>
#include <mach/wmt_iomux.h>
#include <asm/atomic.h> // for atomic_inc atomis_dec added by rubbitxiao
#include <linux/etherdevice.h>

//added begin by rubbit
extern int wmt_getsyspara(char *varname, unsigned char *varval, int *varlen);
extern int wmt_setsyspara(char *varname, char *varval);                      

//#define WMT_PIN_GP2_SUSGPIO1  19

#define WIFI_POWER_PIN WMT_PIN_GP62_SUSGPIO1

//if you use API in the file ,you should #include <mach/wmt_misc.h>

void detect_wifi_module(void * pbool)
{
	int retval = -1;
	int varlen = 127;
	char buf[200] = {0};
	char *wifi;
	char *bt;
	char *gps;
	char *tmp;

	printk("detect_wifi_module\n");
  retval = wmt_getsyspara("wmt.wifi.bluetooth.gps", buf, &varlen);
  if(!retval)
	{
		//sscanf(buf, "%s:%s:%s",wifi,bt,gps);
		if(strlen(buf) == 0) {
			printk("uboot enviroment variant(wmt.wifi.bluetooth.gp) is not connect\n");
			*(bool*)pbool = false;
			return ;
		}
		tmp = buf;
		printk("buf:%s\n",buf);
		wifi = strsep(&tmp,":");
		bt = strsep(&tmp,":");
		gps = strsep(&tmp,":");
		
		printk("wifi:%s, bt:%s, gps:%s\n",wifi,bt,gps);

		if(!strncmp(wifi,"mtk_6620",8)) {
			*(bool*)pbool = true;
			printk("detect mtk_6620 modules:%d\n",*(bool*)pbool);
		} else {
			*(bool*)pbool = false;
			printk("wifi_module:%s---%d\n",wifi,*(bool*)pbool);
		}
		return ;
	}
	printk("have not set uboot enviroment variant:wmt.wifi.bluetooth.gps\n");
	return;
}

int bt_is_mtk6622 = -1;

int is_mtk6622(void)
{
	if(bt_is_mtk6622==-1){
		int retval = -1;
		int varlen = 127;
		char buf[200] = {0};
		retval = wmt_getsyspara("wmt.bt.param", buf, &varlen);
		bt_is_mtk6622 = 0;
		if(!retval)
		{
			if(!strcmp(buf,"mtk_6622"))
				bt_is_mtk6622 = 1;
		}
	}
	return bt_is_mtk6622;
}
                                                                             
                                                                            
struct wifi_gpio {                                                           
	int name;                                                                  
	int active;
	int delay;
	int skip;                                                                                                                  
};                                                                           

enum LEVEL {
	LOW = 0,
	HIGH,
};

                                                                        
static struct wifi_gpio l_wifi_gpio = {                                      
	.name = WIFI_POWER_PIN,                                                                 
	.active = 0,
	.delay = 1000,
	.skip = 0x0,                                                               
};                                                                           
                                                                             

int wifi_power_pin(int gpioNum, unsigned int active, int open)
{
		int ret = 0;


		if(open){
				ret = gpio_request(gpioNum, "wifi power pin");
				if(ret < 0) {
						printk("reques gpio:%x failed!!! for wifi\n",gpioNum);
						return -1;
				}else{
						printk("request gpio:%d for wifi success!!!\n");
				}
				
				if(active)
					gpio_direction_output(gpioNum, HIGH);
				else
					gpio_direction_output(gpioNum, LOW);
					
				printk("power on wifi\n");
		} else {
				if(active)
					gpio_direction_output(gpioNum, LOW);
				else
					gpio_direction_output(gpioNum, HIGH);
						
				printk("power down wifi\n");
				gpio_free(gpioNum);
				printk("release gpio\n");
		}
}
                                                                        

atomic_t gVwifiPower = ATOMIC_INIT(0);

/*
*
*		wmt.gpo.wifi format is the following:
*   gpionum : active level : delay
*   
*   if mdelay is not set, then use l_wifi_gpio.delay as a open delay
*   if mdelay is set, then use medlay as a open delay, althrough l_wifi_gpio.delay 
*   is specified by uboot var, if l_wifi_gpio.delay is not specified by uboot var,
*   which have a default value of 1000ms
*/
                                                                             
void wifi_power_ctrl_comm(int open,int mdelay)                                               
{  
	int ret=0;                                                                          
	int varlen = 127;                                                          
    int retval;                                                              
    char buf[200]={0};                                                       
	printk("wifi_power_ctrl %d\n",open);                                       


	if(!open){
		//wait 200 ms for drv to stop                                                  
		msleep(200);                                                          
	}                           
	
  if(open)
  {
  	if( atomic_inc_return(&gVwifiPower) > 1 )
  	{
  			printk("gVwifiPower:%d\n",atomic_read(&gVwifiPower));
  			return;
  	}
  	
  }else
  {
  	if(atomic_read(&gVwifiPower)<=0){
		printk("power off before power on\n");
		return;
  	}
  	if(atomic_dec_return(&gVwifiPower) > 0 )
  	{
  		printk("gVwifiPower:%d\n",atomic_read(&gVwifiPower));  		
  		return;
  	}
  }                                                                            
/*
*  setenv wmt.gpo.wifi 9c:1:0
*/                                                       
  retval = wmt_getsyspara("wmt.gpo.wifi", buf, &varlen);                   
  if(!retval)                                                              
	{                                                                          
		sscanf(buf, "%x:%x:%d", &(l_wifi_gpio.name), &(l_wifi_gpio.active), &(l_wifi_gpio.delay));    			                                                			                                                                                                                                               
		printk("wifi power up:%s\n", buf);   
		printk("name:0x%x,active:0x%x,open delay:%d\n", l_wifi_gpio.name,l_wifi_gpio.active,l_wifi_gpio.delay);         
//		l_wifi_gpio.skip = 0x01;                         
	}else{
		printk("use default wifi gpio: susgpio1\n");
		printk("name:0x%x,active:0x%x,open delay:%d\n", l_wifi_gpio.name,l_wifi_gpio.active,l_wifi_gpio.delay); 
//		l_wifi_gpio.skip = 0x01;                                  
	}                                                                          

next:                   
  //excute_gpio_op(open);
  wifi_power_pin(l_wifi_gpio.name, l_wifi_gpio.active, open);                                                    
	if(open){
		//wait 1 sec to hw init                                                  
		if(mdelay)
				msleep(mdelay);                                                            
		else	                                                                  
				msleep(l_wifi_gpio.delay);                                                            
	}                                                                          
		                                                                         
} 
void wifi_power_ctrl(int open) 
{
	wifi_power_ctrl_comm(open,1000);//unit is msec
	//wifi_power_ctrl_comm(open,0x0);//unit is msec
}

/*
* wifi mac uboot variant:
* wmt.wifi.mac xx:xx:xx:xx:xx:xx
*/
static int is_dir_exit(const char* filename)
{
	struct file *filep = NULL;
	filep = filp_open(filename, O_RDONLY, 0);
	if(IS_ERR(filep)){
		printk("file %s do not exit\n",filename);
		return 0x00;
	}
	filp_close(filep, 0);
	return 1;
}
static  int  get_wifi_mac(const char * mac_name,unsigned char *buf_mac)
{
	unsigned char buf[200];
	int varlen =127;
	int retval;
	memset(buf,0,sizeof(buf));
  	retval = wmt_getsyspara(mac_name, buf, &varlen);                   
  	if(!retval)                                                              
	{                                                                          
		sscanf(buf, "%x:%x:%x:%x:%x:%x", &(buf_mac[0]),&(buf_mac[1]),&(buf_mac[2]),
			&(buf_mac[3]),&(buf_mac[4]),&(buf_mac[5]));
		printk("wifi mac:%s\n", buf);   
		return 0x0;
	}else{
		printk("uboot variant:%s do not exit\n",mac_name);
		random_ether_addr(buf_mac);
		buf_mac[0] = 0x00;//prevent multi broadcast address
		sprintf(buf,"%x:%x:%x:%x:%x:%x",buf_mac[0],buf_mac[1],buf_mac[2],
			buf_mac[3],buf_mac[4],buf_mac[5]);
		wmt_setsyspara(mac_name, buf);
		return 0x01;
	}
}
int generate_wifi_mac(unsigned char *buf_mac)
{
	if(is_dir_exit("/system")){//means we do not generate mac address when firmware installing
		return get_wifi_mac("wmt.wifi.mac",buf_mac);
	}
	return -1;
}

int irq_int_status(void)
{
	unsigned long flags;
	flags = arch_local_save_flags();
	if(arch_irqs_disabled_flags(flags)){	
		printk("irq interruption disabled\n");
	}else{
		printk("irq interruption enabled\n");
	}
	
}

EXPORT_SYMBOL(generate_wifi_mac);
EXPORT_SYMBOL(is_mtk6622);
EXPORT_SYMBOL(wifi_power_ctrl);
EXPORT_SYMBOL(wifi_power_ctrl_comm);
EXPORT_SYMBOL(irq_int_status);
