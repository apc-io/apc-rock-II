/*++
linux/arch/arm/mach-wmt/board.c

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
#ifndef _LINUX_WMT_BAT_H
#define _LINUX_WMT_BAT_H

static int batt_update_time = 5000;
static int ADC_USED = 0;
static int batt_operation = 0;
static int batt_cal = 0;
static int	g_boot_first = 1;		

/*  ADC device define */
#define vt1609 0


/* Debug macros */
#if 0
#define DPRINTK(fmt, args...) printk(KERN_ALERT "[%s]: " fmt, __func__ , ## args)
#else
#define DPRINTK(fmt, args...)
#endif

static struct timer_list polling_timer;

static DEFINE_MUTEX(bat_lock);
static struct work_struct bat_work;
struct mutex work_lock;
static int bat_low = 0;
static int bat_dcin = 1;
static int bat_usbin = 0;
static int bat_status = POWER_SUPPLY_STATUS_UNKNOWN;
static int bat_capacity = 30;
static int bat_low_old = 0;
static int bat_dcin_old = 1;
static int bat_status_old = POWER_SUPPLY_STATUS_UNKNOWN;
static int bat_capacity_old = 30;
static struct wmt_battery_info *pdata;
static unsigned int bat_temp_capacity = 30;
static unsigned int total_buffer = 0;
static int first_update;
static int charging_first_update = 1;
static int discharging_first_update = 1;
int polling_interval= 5000;

static int data_num = 0;
static int avg =0;
static int avgnum = 9;
static int temp_buffer[10];

extern int wmt_getsyspara(char *varname, char *varval, int *varlen);
extern unsigned short wmt_read_batstatus_if(void);

struct wmt_batgpio_set{ 
     int  name;        
     int  active;   
     int  bitmap;   
     int  ctradd;   
     int  icaddr;   
     int  idaddr;   
     int  ipcaddr;  
     int  ipdaddr;  
};  

struct wmt_battery_para{
    int maxi;
    int v9;	
    int v8;	
    int v7;	
    int v6;	
    int v5;	
    int v4;	
    int v3;	
    int v2;	
    int v1;	
    int mini;	
};

/*static struct wmt_batgpio_set dcin;    
static struct wmt_batgpio_set batstate;*/
static struct wmt_batgpio_set batlow;  

static struct wmt_battery_para batt_discharge;    
static struct wmt_battery_para batt_charge;


struct wmt_battery_info {
	int	batt_aux;
	int	temp_aux;
	int	charge_gpio;
	int	min_voltage;
	int	max_voltage;
	int	batt_div;
	int	batt_mult;
	int	temp_div;
	int	temp_mult;
	int	batt_tech;
	char *batt_name;
};

#endif
