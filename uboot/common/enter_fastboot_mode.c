/*++ 
Copyright (c) 2010 WonderMedia Technologies, Inc.

This program is free software: you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free Software 
Foundation, either version 2 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details. You
should have received a copy of the GNU General Public License along with this
program. If not, see http://www.gnu.org/licenses/>.

WonderMedia Technologies, Inc.
cheney chen mce Shenzhen china
--*/

#include <common.h>

#define REG32               *(volatile unsigned int *)
#define REG8                *(volatile unsigned char *)
#define REG_GET8(addr)      ( REG8(addr) )      /* Read  8 bits Register */
#define REG_GET32(addr)     ( REG32(addr) )     /* Read 32 bits Register */
#define REG_SET8(addr, val) ( REG8(addr)  = (val) ) /* Write  8 bits Register */

#define ENV_FASTBOOTMODE_GPIO "wmt.fastbootmode.gpio"
#define ENV_DEFAULT_ENTER_FASTBOOT "wmt.default.enter.fastboot"

#define USB_CONNECTED_TIMTOUT_INTERVAL 5
#define TIMEOUT_INTERVAL_NUM 15      //100ms * 15
#define TIMEOUT_SLEEP_INTERVAL   100000  //100ms

//#define FASTBOOT_MODE_DEBUG

struct gpio_control_t {
     unsigned int  gpio_num; // gpio number
	 unsigned int  delay;
     unsigned int  active;	 // 0 or 1 
     unsigned int  bitmap;   // bitmap
     unsigned int  ctraddr;   // enable gpio function 
     unsigned int  icaddr;   // input control address
     unsigned int  idaddr;   // input data address 
     unsigned int  ipcaddr;  // input pull up/down control address 
     unsigned int  ipdaddr;  // input pull data address 
};

#define MAX_NUM 9

static struct gpio_control_t commb_gpio_key[] = {
	//volume- by default
	[0] = {
		.gpio_num = 10,
		.delay  =  10,
		.active = 0,
		.bitmap = 2,
		.idaddr = 0xd8110001,
		.ctraddr = 0xd8110041,
		.icaddr = 0xd8110081,
		.ipcaddr = 0xd8110481,
		.ipdaddr = 0xd81104c1,
	}
};


static int parse_commb_param(char* s_in,  int len)
{
    char *s1 = s_in;
    char *endp;
    int i = 0;
	unsigned int *param[MAX_NUM] = {
		&commb_gpio_key[0].gpio_num,
		&commb_gpio_key[0].delay,
		&commb_gpio_key[0].active,
		&commb_gpio_key[0].bitmap,
		&commb_gpio_key[0].ctraddr,
		&commb_gpio_key[0].icaddr,
		&commb_gpio_key[0].idaddr,
		&commb_gpio_key[0].ipcaddr,
		&commb_gpio_key[0].ipdaddr,
	};

	if(s1 == NULL)
		return -1;
    
#ifdef FASTBOOT_MODE_DEBUG
	printf("parse_commb_param: %s\n", s_in);
#endif
    
    while(i < len) {
        if (*s1 == ':')
        {
                s1++;
        }
        *param[i++] = simple_strtoul(s1, &endp, 16);
        if (*endp == '\0')
            break;
        s1 = endp + 1;
        if (*s1 == '\0') 
			break;
    }
	
#ifdef FASTBOOT_MODE_DEBUG
	printf("parse_commb_param: %x:%x:%x:%x:%x:%x:%x:%x:%x\n", commb_gpio_key[0].gpio_num, commb_gpio_key[0].delay, commb_gpio_key[0].active, commb_gpio_key[0].bitmap, commb_gpio_key[0].ctraddr, commb_gpio_key[0].icaddr, commb_gpio_key[0].idaddr, commb_gpio_key[0].ipcaddr, commb_gpio_key[0].ipdaddr );
#endif
	if(i != MAX_NUM)
		return -1;
	return 0;
}

int enter_fastboot_mode(void)
{
	char *s;
	int volume_btn_value = 0;
	//int times = 0;
	int timeout = 0;
	int ret = 0;

	s = getenv(ENV_DEFAULT_ENTER_FASTBOOT);
	//if ENV_DEFAULT_ENTER_FASTBOOT is true, enter fastboot mode directly
	 if(s != NULL && !strcmp(s, "true"))
	 	goto RUN;

	//default volume-
	if((s = getenv(ENV_FASTBOOTMODE_GPIO)) != NULL)
	{
		ret = parse_commb_param(s, MAX_NUM);
		if(ret < 0) 
		{
			printf("Invalid Parameter %s\n", ENV_FASTBOOTMODE_GPIO);
			return -1;
		}
	}

	REG_SET8(commb_gpio_key[0].ctraddr, REG_GET8(commb_gpio_key[0].ctraddr) | (1 << commb_gpio_key[0].bitmap));
	REG_SET8(commb_gpio_key[0].icaddr, REG_GET8(commb_gpio_key[0].icaddr) & ~(1 << commb_gpio_key[0].bitmap));
	REG_SET8(commb_gpio_key[0].ipcaddr, REG_GET8(commb_gpio_key[0].ipcaddr) | (1 << commb_gpio_key[0].bitmap));
	REG_SET8(commb_gpio_key[0].ipdaddr, REG_GET8(commb_gpio_key[0].ipdaddr) | (1 << commb_gpio_key[0].bitmap));

	udelay(1000);
	volume_btn_value = REG_GET8(commb_gpio_key[0].idaddr);
	if((volume_btn_value & (1 << commb_gpio_key[0].bitmap)) == commb_gpio_key[0].active)
	    run_command("textout 10 5 \"Volume- key is pressed\" 0xffff00;", 0);
	else
	    return -1;

RUN:
    if(udc_init() < 0){
	   printf("udc_init fail\n");
       return -1;
	}

	while(timeout < USB_CONNECTED_TIMTOUT_INTERVAL){
       if(check_udc_connection()) break;
       timeout++;
	   udelay(TIMEOUT_SLEEP_INTERVAL);
	}

	if(timeout >= USB_CONNECTED_TIMTOUT_INTERVAL){
        printf("check usb connection fail\n");
		return -1;	
	}
/*
    s = getenv(ENV_DEFAULT_ENTER_FASTBOOT);

    //if ENV_DEFAULT_ENTER_FASTBOOT is true, enter fastboot mode directly
    if(s == NULL || strcmp(s, "true")){
	    timeout = 0;
		//vol- by default
		while(timeout < TIMEOUT_INTERVAL_NUM)
		{
			
			volume_btn_value = REG_GET8(commb_gpio_key[0].idaddr);

#ifdef FASTBOOT_MODE_DEBUG
			printf("volume-: 0x%x\n",  volume_btn_value);
#endif
			
			if((volume_btn_value & (1 << commb_gpio_key[0].bitmap)) == commb_gpio_key[0].active)
				times++;

			if(times > commb_gpio_key[0].delay) //100ms * commb_gpio_key[0].delay = xx seconds
				break;     // go to comfirm

			timeout++;	
			udelay(TIMEOUT_SLEEP_INTERVAL);
		}

		if(timeout >= TIMEOUT_INTERVAL_NUM)
			return -1;
    }
*/	
	run_command("textout 10 30 \"Enter Fastboot Mode\" 0xffff00;", 0);
	run_command("fastboot;", 0);
	
	return 0;
}

