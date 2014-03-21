/*******************************************************************************
*	Copyright (c) 2013 WonderMedia Technologies, Inc.
*
*	Abstract:
*		This program is designed to show the charge animation
*
******************************Release History ***********************************
*
* Version 1.0 , HowayHuo, 2013/5/8
* First version: Created by Howayhuo
*
**********************************************************************************/
#include <common.h>
#include <command.h>
#include <asm/arch/common_def.h>
#include <asm/byteorder.h>
#include <bmp_layout.h>
#include "../../../board/wmt/include/wmt_pmc.h"
#include "../../../board/wmt/wmt_battery.h"
#include "../minivgui.h"
#include "../wmt_display.h"

#define HSP3_STATUS (0xD8130000 + 0x3C)
#define REBOOT_BIT 0x10

#define POWERUP_SOURCE_STATUS (0xD8130000 + 0xD0)
#define PWRBTN_BIT           0x01
#define VBAT_RISING_BIT      0x02
#define DCDET_RISING_BIT     0x04
#define DCDET_FALLING_BIT    0x08
#define DCDET_BIT            0x10

static unsigned char *sp_mem_addr = (unsigned char *)0x3000000;

static int s_usb_to_pc;
static int s_pwm_duty_decreased;
static int s_battery_init_ok = -1;
static int s_animation_init_ok = -1;
static int s_lcd_first_light = 1;

extern int g_tf_boot;

//extern char* CMD_LOAD_SCRIPT;
extern int g_show_logo;
extern int udc_fastboot_init(void);
extern void udc_fastboot_transfer(void);
extern void udc_fastboot_exit(void);
extern int wmt_udc_connected(void);
extern int udc_fastboot_is_init(void);
extern void do_wmt_poweroff(void);

extern struct nand_chip* get_current_nand_chip(void);
extern int WMTAccessNandEarier(unsigned long long naddr, unsigned int maddr,
	unsigned int size, int write);

extern unsigned int pwm_get_enable(int no);
extern void lcd_blt_enable(int no,int enable);
extern void lcd_set_power_down(int enable);
//extern void hint_battery_checking(void);

static int sleep_sec(int sec)
{
	ulong start = get_timer(0);
	ulong delay;

	delay = sec * CFG_HZ;

	while (get_timer(start) < delay) {
		if (ctrlc ()) {
			return (-1);
		}
		udelay (100);
	}

	return 0;
}

//return level ( 0 ~ 5 ), level < 0 is error
static int get_battery_level(int *p_percent)
{
	int ret = 0;

	if(wmt_bat_is_charging_full() == 1) {
		printf("power is full\n");
		*p_percent = 100;
		return 5;
	}

	ret = wmt_bat_get_capacity();
	*p_percent = ret;

	if(ret < 0)
		return ret;

	if(ret >= 0 && ret < 20)
		return 0;

	if(ret >= 20 && ret < 40)
		return 1;

	if(ret >= 40 && ret < 60)
		return 2;

	if(ret >= 60 && ret < 80)
		return 3;

	if(ret >= 80 && ret < 100)
		return 4;

	if(ret >= 100)
		return 5;

	return -1;
}

static void decrease_pwm_duty(int duty_percent)
{
	int duty;

	if(!(g_display_vaild & DISPLAY_ENABLE))
		return;

	//if(s_pwm_duty_decreased == 1)
	//	return;

	if (g_display_param.vout == VPP_VOUT_LCD) {
		if ((g_display_vaild&PWMDEFMASK) == PWMDEFTP) {
			if(duty_percent < g_pwm_setting.duty) {
				lcd_blt_set_pwm(g_pwm_setting.pwm_no,
					duty_percent, g_pwm_setting.period);

				s_pwm_duty_decreased = 1;

				mdelay(10);
			}
		} else {
			duty = (duty_percent * g_pwm_setting.period) / 100;
			if(duty < g_pwm_setting.duty) {
				pwm_set_duty(g_pwm_setting.pwm_no, duty - 1);

				s_pwm_duty_decreased = 1;

				mdelay(10);
			}
		}
	}
}

static void restore_pwm_duty(void)
{
	if(s_pwm_duty_decreased == 0)
		return;

	if(!(g_display_vaild & DISPLAY_ENABLE))
		return;

	if (g_display_param.vout == VPP_VOUT_LCD) {
		if ((g_display_vaild&PWMDEFMASK) == PWMDEFTP) {
			lcd_blt_set_pwm(g_pwm_setting.pwm_no,
				g_pwm_setting.duty, g_pwm_setting.period);
		} else {
			pwm_set_duty(g_pwm_setting.pwm_no,
				g_pwm_setting.duty - 1);
		}
	}

	s_pwm_duty_decreased = 0;
}

static void set_lcd_backlight(int on)
{
	if(on) {
		if(s_usb_to_pc) {
			if(wmt_bat_is_lowlevel() > 0)
				decrease_pwm_duty(30);
			else
				restore_pwm_duty();
		}

		if(s_lcd_first_light) {
			if(!(REG32_VAL(HSP3_STATUS) & REBOOT_BIT)
				&& !hw_is_dc_plugin()
				&& !(REG8_VAL(POWERUP_SOURCE_STATUS) & PWRBTN_BIT)) {
				printf("Adapter is removed when open lcd backlight.\nPower off\n");
				do_wmt_poweroff();
			}
			s_lcd_first_light = 0;
		}

		 lcd_blt_enable(g_pwm_setting.pwm_no, 1);
	} else
		 lcd_blt_enable(g_pwm_setting.pwm_no, 0);
}

static void low_battery_picture_show(void)
{
	//clear_charge_percent(sp_mem_addr);
	mv_clearFB();
	show_charge_picture(sp_mem_addr, 6);
}

static void update_current_battery_picture(void)
{
	int index, percent;

	if(wmt_bat_is_lowlevel() > 0)
		low_battery_picture_show();
	else {
		index = get_battery_level(&percent);
		if(index >= 0) {
			display_charge_percent(sp_mem_addr, percent);
			show_charge_picture(sp_mem_addr, index);
		}
	}
}

static int charge_animation_init(void)
{
	int ret;
	unsigned int nand_addr;
	char *s, *s_logosize_uboot, *s_logosize_charge;
	unsigned int logo_size, total_size;

	//printf("charging animation init\n");

	if(s_animation_init_ok == 0)
		return -1;
	else if(s_animation_init_ok == 1)
		return 0;

	if(!g_tf_boot) {
		//1: check nand flash env
		s = getenv("wmt.nfc.mtd.u-boot-logo");
		if(s == NULL) {
			printf("wmt.nfc.mtd.u-boot-logo isn't set\n");
			s_animation_init_ok = 0;
			return -1;
		} else {
			nand_addr = simple_strtoul (s, NULL, 16);

			total_size = 0;
			s_logosize_uboot = getenv("wmt.logosize.uboot");
			if(s_logosize_uboot) {
				s_logosize_charge = getenv("wmt.logosize.charge");
				if(s_logosize_charge)
					total_size = simple_strtoul(s_logosize_uboot, NULL, 0)
					             + simple_strtoul(s_logosize_charge, NULL, 0);
			}

			if(total_size == 0)
				total_size = 0x380000;

			printf("anim init: nand_addr = 0x%x, mem_addr = 0x%x, read_size = 0x%x\n",
			       nand_addr, (unsigned int)sp_mem_addr, total_size);

			REG32_VAL(GPIO_BASE_ADDR + 0x200) &= ~(1 << 11); //PIN_SHARE_SDMMC1_NAND
			ret = WMTAccessNandEarier(nand_addr, (unsigned int)sp_mem_addr, total_size, 0);
			if(ret) {
				printf("load charge-anim fail\n");
				s_animation_init_ok = 0;
				return -1;
			}
		}

		//2: check bmp header
		if(*sp_mem_addr != 'B') {
			printf("logo isn't BMP format\n");
			s_animation_init_ok = 0;
			return -1;
		}

		logo_size = (*(unsigned short *)(sp_mem_addr + 4) << 16) + (*(unsigned short *)(sp_mem_addr + 2));

		if(*(sp_mem_addr + logo_size) != 'B') {
			printf("charge-anim isn't BMP format\n");
			s_animation_init_ok = 0;
			return -1;
		}
	} else {
		ret = run_command("mmcinit 1", 0);
		if(ret == -1) {
			printf("TF: run \"mmcinit 1\" failed\n");
			s_animation_init_ok = 0;
			return -1;
		} else {
			char tmp[100] = {0};
			sprintf(tmp, "fatload mmc 1 0x%x charge-logo.bmp", (unsigned int)sp_mem_addr);
			ret = run_command(tmp, 0);
			if(ret == -1) {
				printf("TF: fatload charge-logo.bmp failed\n");
				s_animation_init_ok = 0;
				return -1;
			}
		}

		if(*sp_mem_addr != 'B') {
			printf("TF: charge-anim isn't BMP format\n");
			s_animation_init_ok = 0;
			return -1;
		}
	}

	s_animation_init_ok = 1;

	printf("load charge-animation ok\n");

	return 0;
}

/*
static int need_battery_adjust(void)
{
	char *s;

	s = getenv("wmt.battery.adjust");
	if((s != NULL) && !strcmp(s, "0")) {
		return 0;
	}

	return 1;

}
*/

static int battery_init(void)
{
	int ret;

	if(s_battery_init_ok == 0)
		return -1;
	else if(s_battery_init_ok == 1)
		return 0;

	ret = wmt_bat_init();
	if(ret) {
		printf("battery init error\n");
		s_battery_init_ok = 0;
		return -1;
	}

	s_battery_init_ok = 1;

	return 0;
}

int low_power_detect(void)
{
	int ret;
	int delay;

	//printf("check low power\n");
	if(hw_is_dc_plugin())
		return 0;

	ret = battery_init();
	if(ret) {
		printf("battery init error. Skip Low power detect\n");
		return -1;
	}

	ret = wmt_bat_is_lowlevel();
	if(ret < 0) {
		printf("check battery low failed\n");
		return -1;
	}else if(ret == 0) {
		//printf("battery power enough\n");
		return 0;
	}else {
		printf("low power detected\n");
	}

	if(wmt_bat_is_gauge() == 1)
		delay = 3;
	else
		delay = 60;

	while(delay) {
		if(delay % 10 == 0) {
		if (tstc()) {
			if (getc() == 0xd)	{
					printf("Got 'Enter' key. Cancel Low power detect\n");
					g_show_logo = 1;
				return 0;
			}
		}
		}

		if(hw_is_dc_plugin()) {
			printf("Detected adapter plugin. Cancel Low power detect\n");
			break;
		}

		mdelay(50);

		ret = wmt_bat_is_lowlevel();
		if(ret < 0) {
			restore_pwm_duty();
			printf("check battery low failed\n");
			break;
		}else if(ret == 0) {
			restore_pwm_duty();
			printf("battery power enough\n");
			break;
		}

		delay--;
			}

	//printf("low power detect complete\n");

	//display low power picture
	if(delay == 0) {
			if (!(g_display_vaild & DISPLAY_ENABLE)) {
				ret = display_init(0, 0);
				if(ret == -1) {
					printf("Display init fail. Skip Low power detect\n");
					return -1;
				}
			}
		charge_animation_init();
		low_battery_picture_show();
		decrease_pwm_duty(30);
		set_lcd_backlight(1);

		delay = 500;
		while(delay) {
			if(delay % 100 == 0) {
			if (tstc()) {
				if (getc() == 0xd)	{
						printf("Got 'Enter' key. Cancel Low power picture display\n");
						g_show_logo = 1;
					return 0;
				}
			}
			}

			if(hw_is_dc_plugin()) {
				printf("Detected adapter plugin. Cancel Low power picture display\n");
				return 0;
		}

		mdelay(10);
		delay--;
	}

		set_lcd_backlight(0);
		lcd_set_power_down(1);
		mdelay(500); //delay for avoiding lcd blink
		printf("----> low power. power off\n");
		do_wmt_poweroff();
	}

	return 0;
}

static int is_usb_charge(void)
{
	static char UBOOT_ENV[] = "wmt.charger.param";
	char *env;
	char *s;
	char *endp;
	enum cable_type id;

	env = getenv(UBOOT_ENV);
	if (!env)
		return 0;

	s = env;
	id = simple_strtoul(s, &endp, 0);
	if (*endp == '\0')
		return 0;

	if(id == CABLE_TYPE_USB)
		return 1;

	return 0;
}

int wmt_udc_init(void)
{
	if(is_usb_charge())
		udc_fastboot_init();

		return 0;
	}

#define CHECK_UDC_TIMEOUT  200          // 200 ms

int wmt_check_udc_to_pc(int is_system_upgrade)
{
	int ret, delay = 0;

	//ret = battery_init();
	ret = charger_param_init();
	if(ret) {
		printf("charger_param_init error. wmt_check_udc_to_pc fail\n");
		return -1;
	}

	if(hw_is_dc_plugin()) {
		if(wmt_charger_cable_type() == CABLE_TYPE_USB) {
			//printf("USB charge\n");
			if(!udc_fastboot_is_init()) {
				ret = udc_fastboot_init();
				if(ret) {
					printf("udc_fastboot_init error. wmt_check_udc_to_pc fail\n");
					if(is_system_upgrade) {
						printf("regards as usb adapter plugin during system upgrade\n");
						s_usb_to_pc = 0;
						hw_set_large_current();
					}
					return -1;
				}
			}

			while(++delay <= CHECK_UDC_TIMEOUT) {
				udc_fastboot_transfer();
				if(wmt_udc_connected()) {
					udc_fastboot_exit();
					s_usb_to_pc = 1;
					printf("usb pc plugin, delay = %d\n", delay);
					printf("default small charging current\n");
					break;
				}

				mdelay(1);
			}

			if(delay > CHECK_UDC_TIMEOUT) {
				printf("usb adapter plugin\n");
				s_usb_to_pc = 0;
				hw_set_large_current();
			}
		}
		//else
		//	printf("AC charge\n");
	}

	return 0;
}

void wmt_udc_deinit(void)
{
	udc_fastboot_exit();
}

static void check_pmc_busy(void)
{
	while (PMCS2_VAL&0x3F0038)
	;
}

static void set_arm_freq(int freq)
{
	auto_pll_divisor(DEV_ARM,CLK_ENABLE, 0, 0);
	auto_pll_divisor(DEV_ARM, SET_PLLDIV, 2, freq);
	check_pmc_busy();
}

#define PERIOD_CHECK           10    		// 10 ms    //how long time to check
#define PERIOD_PLAY            250    		// 250 ms   //the play speed
#define PERIOD_PAUSE           700  		// 700 ms   //the pause time after a play peroid complete
#define PERIOD_DETECTADAPT     50           // 50 ms    //check if the power adapter is unplug
#define PERIOD_READBATT        1 * 1000  	// 1 s     //how long time to read battery
#define PERIOD_READPWRKEY      10           // 10ms    //how long time to detect power key
#define PERIOD_LONGPRESS       500          // 500 ms   //how long time is regarded as long press
#define PERIOD_BLTIMEOUT       10 * 1000    // 10 s     //how long time to close backlight
#define PERIOD_USBDETECT       100          // 100 ms   //how long time to detect usb
#define TIME_ENABLELARGECUR    2 * 1000     // 2 s      //how long time to enable large current charge
#define PERIOD_GAUGE_READBATT     10 * 1000  	// 10 s     //how long time to read battery


int play_charge_animation(void)
{
	const int max_play_count = (PERIOD_PLAY) / (PERIOD_CHECK);
	const int max_pause_count = (PERIOD_PAUSE - PERIOD_PLAY) / (PERIOD_CHECK);
	const int max_detectAdapter_count = (PERIOD_DETECTADAPT) / (PERIOD_CHECK);
	int max_readBatt_count = (PERIOD_READBATT) / (PERIOD_CHECK);
	const int max_readPwrKey_count = (PERIOD_READPWRKEY) / (PERIOD_CHECK);
	const int max_longPress_count = (PERIOD_LONGPRESS) / (PERIOD_CHECK);
	const int max_blTimeout_count = (PERIOD_BLTIMEOUT) / (PERIOD_CHECK);
	const int max_usbDetect_count = (PERIOD_USBDETECT) / (PERIOD_CHECK);
	const int max_enableLargeCur = (TIME_ENABLELARGECUR) / (PERIOD_CHECK);

	int play_count;             //play period
	int pause_count;            //pause time after a play period complete
	int detectAdapter_count;    //check if the adapter is unpluged
	int readBatt_count;         //how long time to read battery
	int readPwrKey_count;       //how long time to detect power key
	int longPress_count;        //how long time is regarded as long press
	int blTimeout_count;        //how long time to close backlight
	int usbDetect_count;        //how long time to detect usb
	int enableLargeCur_count;   ////how long time to enable large current charge

	int frame_num, frame_index, start_index, backup_start_index;
	int is_powerKey_press, backup_pwm_status;

	int battery_is_full = 0, udc_is_init = 0;
	int ret;
	char *s;
	int charger_type = CABLE_TYPE_MAX;
	int percent;
	int delay;
	int saved_arm_freq;
	//int current_arm_freq;
	//printf("play charging animation\n");

	//1: detect "enter key"
	if (tstc()) {
		if (getc() == 0xd) { // "Enter" key
			puts("Got 'Enter' key. Cancel charge animation play\n");
			return 0;
		}
	}

	// 2: check power source
	if(!hw_is_dc_plugin() && !(REG8_VAL(POWERUP_SOURCE_STATUS) & PWRBTN_BIT)) {
		printf("Adatper is removed when init charging-animation.\nPower off\n");
		do_wmt_poweroff();
	}

	//3: detect power key press
	if(PMPB_VAL & BIT24) {
		printf("Power key detected. Skip charge-anim play\n");
		return 1;
	}

	//4: check power on by reset
	if((REG32_VAL(HSP3_STATUS) & REBOOT_BIT) == REBOOT_BIT) {
		printf("system Reboot. Skip charge-anim play\n");
		return 1;
	}

	//5: check charge animation env
	s = getenv(ENV_CHARGE_ANIMATION);
	if((s != NULL) && !strcmp(s, "0"))  //if wmt.display.chargeanim is set to 0
		return 1;

	//6: check dircect boot env
	if(((s = getenv("wmt_sys_directboot")) != NULL && !strcmp(s,"1"))  \
		||((s = getenv("wmt_sys_restore")) != NULL && !strcmp(s,"1")))
		return 1;


	//7: battery init
	ret = battery_init();
	if(ret) {
		printf("battery init error. Don't play charge-anim\n");
		return -1;
	}

	if(wmt_bat_is_gauge() == 1) {
		printf("battery is gauge\n");
		max_readBatt_count = (PERIOD_GAUGE_READBATT) / (PERIOD_CHECK);
	}

	//printf("REG32_VAL(PM_CTRL_BASE_ADDR + 0x00D0) = 0x%x\n", REG8_VAL(PM_CTRL_BASE_ADDR + 0xd0));

	//8: lcd init. but don't open backlight
	if (!(g_display_vaild & DISPLAY_ENABLE)) {
		ret = display_init(0, 0);
		if(ret == -1) {
			printf("Display init fail. Don't play charge-anim\n");
			return -1;
		}
	}
	mv_clearFB();

	//9: check adapter
	charger_type = wmt_charger_cable_type();

	if(hw_is_dc_plugin()) {
		//charger_type = wmt_charger_cable_type();
		if(charger_type == CABLE_TYPE_AC)
			printf("AC charger plugin\n");
		else if(charger_type == CABLE_TYPE_USB){
			printf("usb plugin\n");
		} else {
			printf("charger type error. Don't play charge-anim\n");
			return -1;
		}
	} else {
		printf("Battery power supply\n");
		if((REG8_VAL(POWERUP_SOURCE_STATUS) & PWRBTN_BIT) == 0) {
			printf("POWERUP_SOURCE_STATUS = 0x%x\n",  REG8_VAL(POWERUP_SOURCE_STATUS));
			printf("Adatper is removed when start play charging-animation.\nPower off\n");
			do_wmt_poweroff();
		}
		low_power_detect();
		if(!hw_is_dc_plugin())
			return -1;

		//charger_type = wmt_charger_cable_type();

		if((charger_type == CABLE_TYPE_USB) && (s_usb_to_pc == 0)) {
			printf("check if usb connect to pc\n");
			if(!udc_fastboot_is_init()) {
				ret = udc_fastboot_init();
				if(ret){
					printf("udc_fastboot_init error. Don't play charge-anim\n");
			return -1;
	}
			}
			wmt_check_udc_to_pc(0);
			udc_fastboot_exit();
			if(s_usb_to_pc == 0)
				restore_pwm_duty();
		}
	}

	//10: charge animation init
	ret = charge_animation_init();
	if(ret) {
		printf("charge-anim init fail.\n");
		//return -1;
	}

	//11: get battery level
	start_index = get_battery_level(&percent);
	if(start_index < 0)	{
		printf("Battery level error(%d). Don't play charge-anim\n", start_index);
		return -1;
	}

	printf("Hit 'Enter' key to stop animation\n");

	//12: show the first charge picture
	frame_num = 6;
	play_count = -1;
	pause_count = 0;
	detectAdapter_count = 0;
	readBatt_count = 0;
	readPwrKey_count = 0;
	longPress_count = -1;
	is_powerKey_press = 0;
	backup_pwm_status = 0;
	blTimeout_count = 0;

	frame_index = start_index;

	init_charge_percent();
	display_charge_percent(sp_mem_addr, percent);
	show_charge_picture(sp_mem_addr, frame_index);

	if(start_index != frame_num - 1) //not last frame
		frame_index++;

	//13: init usb if using usb charge
	if((charger_type == CABLE_TYPE_USB) && (s_usb_to_pc == 0)) {
		if(!udc_fastboot_is_init()) {
		ret = udc_fastboot_init();
		if(ret){
			printf("udc_fastboot_init error. Don't play charge-anim\n");
			return -1;
		}
		}

		udc_is_init = 1;
		usbDetect_count = 0;
		enableLargeCur_count = 0;
	} else {
		usbDetect_count = -1;
		enableLargeCur_count = -1;
	}


	//14: detect low power if usb connect to pc
	if(s_usb_to_pc) {
		ret = wmt_bat_is_lowlevel();
		if(ret > 0) {
			printf("USB connect to PC. Low power\n");

			if(wmt_bat_is_gauge() == 1)
				delay = 3;
			else
				delay = 60;

			while(delay) {
			if (tstc()) {
					if (getc() == 0xd)	{
						printf("Got 'Enter' key. Cancel Low power detect during USB connect to PC\n");
					return -1;
				}
			}

				if(!hw_is_dc_plugin()) {
					low_battery_picture_show();
					decrease_pwm_duty(30);
					set_lcd_backlight(1);
					sleep_sec(5);
					set_lcd_backlight(0);
					lcd_set_power_down(1);
					mdelay(500); //delay for avoiding lcd blink
					printf("----> USB connect to PC. Low power. power off\n");
					do_wmt_poweroff();
				}

				mdelay(50);

				ret = wmt_bat_is_lowlevel();
				if(ret < 0) {
					printf("Low power detect failed during USB connect to PC\n");
					return -1;
				}else if(ret == 0) {
					printf("battery power enough during USB connect to PC\n");
				break;
			}

				delay--;
		}

		if(delay == 0)
				low_battery_picture_show();

		} else if(ret < 0) {
			printf("Low power detect failed during USB connect to PC\n");
			return -1;
		}else
			printf("USB connect to PC. Power enough\n");

		set_lcd_backlight(1);
	} else
		set_lcd_backlight(1);

	//15: set cpu freq to 300MHz
	saved_arm_freq = auto_pll_divisor(DEV_ARM, GET_FREQ, 0, 0);
	//printf("save cpu freq = %d\n", saved_arm_freq);
	set_arm_freq(300);
	//current_arm_freq = auto_pll_divisor(DEV_ARM, GET_FREQ, 0, 0);
	//printf("set cpu freq = %d\n", current_arm_freq);

	//16: enter the animation loop
	while(1) {
		if (tstc()) {//we got a key press
			printf("we got a key press\n");
			//Check if the key is 'Enter' which ascii code is 13
			// If the key is 'Enter', exit
			if (getc() == 13)
				break;
		}

		//check if usb connect to pc
		if((charger_type == CABLE_TYPE_USB) && (s_usb_to_pc == 0)) {
			if(usbDetect_count >= 0)
				usbDetect_count++;

			if(enableLargeCur_count >= 0)
				enableLargeCur_count++;

			if(usbDetect_count > max_usbDetect_count) {
					udc_fastboot_transfer();
					if(wmt_udc_connected()) {
						udc_fastboot_exit();
						udc_is_init = 0;
					s_usb_to_pc = 1;
						usbDetect_count = -1;    //cancel usb detect
					enableLargeCur_count = -1;  //cancel enable large Current
						printf("usb pc plugin\n");
						hw_set_small_current();
						update_current_battery_picture();
					}
					usbDetect_count = 0;
					}

			if(enableLargeCur_count > max_enableLargeCur) {
				enableLargeCur_count = -1;
					hw_set_large_current();
				}
			}

		readBatt_count++;
		readPwrKey_count++;
		detectAdapter_count++;

		if(longPress_count >= 0)
			longPress_count++;

		if(blTimeout_count >= 0)
			blTimeout_count++;

		if(detectAdapter_count > max_detectAdapter_count) {
			detectAdapter_count = 0;
			if(!hw_is_dc_plugin()) {
				if(s_usb_to_pc == 0) {
					display_charge_percent(sp_mem_addr, percent);
					show_charge_picture(sp_mem_addr, start_index);
				}

				printf("unplug power adapter\n");
				if(!pwm_get_enable(g_pwm_setting.pwm_no)) {
					set_lcd_backlight(1);
					if(s_usb_to_pc)
						update_current_battery_picture();
				}

				sleep_sec(3);
				set_lcd_backlight(0);
				lcd_set_power_down(1);
				mdelay(500); //delay for avoiding lcd blink
				printf("power off\n");
				do_wmt_poweroff();
			}
		}

		if(readBatt_count > max_readBatt_count)	{
			readBatt_count = 0;
			backup_start_index = start_index;
			start_index = get_battery_level(&percent);

			if(start_index < 0 || start_index >= frame_num)	{
				if(s_usb_to_pc == 0) {
					display_charge_percent(sp_mem_addr, percent);
					show_charge_picture(sp_mem_addr, backup_start_index);
				}
				printf("read battery error\n");
				if(!pwm_get_enable(g_pwm_setting.pwm_no)) {
					set_lcd_backlight(1);
					if(s_usb_to_pc)
						update_current_battery_picture();
				}

				sleep_sec(2);
				set_lcd_backlight(0);
				lcd_set_power_down(1);
				mdelay(500); //delay for avoiding lcd blink
				printf("power off\n");
				do_wmt_poweroff();
			}

			if(s_usb_to_pc) //if usb connect to pc, show picture right now
				update_current_battery_picture();
			else
				display_charge_percent(sp_mem_addr, percent);
		}

		if(blTimeout_count > max_blTimeout_count) {
			blTimeout_count = -1;
			if(pwm_get_enable(g_pwm_setting.pwm_no)) {
				printf("close backlight during charging\n");
				set_lcd_backlight(0);
			}
		}

		if(readPwrKey_count > max_readPwrKey_count)	{
			readPwrKey_count = 0;
			if((PMPB_VAL & BIT24) || (PMWS_VAL & BIT14)) {
				if(PMWS_VAL & BIT14)
					PMWS_VAL |= BIT14;
				if(is_powerKey_press == 0) {
					backup_pwm_status = pwm_get_enable(g_pwm_setting.pwm_no);
					printf("press pwr key, backup_pwm_status = %d\n", backup_pwm_status);
					if(!backup_pwm_status) {
						set_lcd_backlight(1);
						if(s_usb_to_pc)
							update_current_battery_picture();
					}

					is_powerKey_press = 1;
					blTimeout_count = -1;
					longPress_count = 0;
				}
			} else {
				if(is_powerKey_press == 1) {
					printf("release pwr key, backup_pwm_status = %d\n", backup_pwm_status);
					is_powerKey_press = 0;
					if(backup_pwm_status) {
						if(pwm_get_enable(g_pwm_setting.pwm_no))
							set_lcd_backlight(0);
						blTimeout_count = -1;
					} else {
						if(!pwm_get_enable(g_pwm_setting.pwm_no)) {
							set_lcd_backlight(1);
							if(s_usb_to_pc)
								update_current_battery_picture();
						}
						blTimeout_count = 0;
					}
				}
				longPress_count = -1;
			}
		}

		if(longPress_count > max_longPress_count) {
			printf("charge_animation: long press key detected\n");
			// if usb connect to pc and detect battery low, don't startup system, keep charging
			if(s_usb_to_pc && wmt_bat_is_lowlevel() > 0) {
				printf("USB connect to PC and low power. skip long press\n");
			} else
			break;
		}

		udelay(PERIOD_CHECK * 1000);

		if(s_usb_to_pc)  //if usb connect to pc, don't play animation
			continue;

		if(play_count >= 0)
			play_count++;

		if(pause_count >= 0)
			pause_count++;

		if(play_count > max_play_count)	{
			show_charge_picture(sp_mem_addr, frame_index);

			if((frame_index == start_index) || (frame_index == frame_num - 1)) { //a play period complete
				pause_count = 0;  //enable pause
				play_count = -1;  //disable play
			} else {
				play_count = 0;  //restart play period
				pause_count = -1;
			}

			frame_index++;
			if(frame_index >= frame_num)
				frame_index = start_index;
		}

		if(pause_count > max_pause_count) {
			if((start_index == frame_num - 1) && (frame_index == start_index)) { //the battery is full, always pause, don't play
				if(battery_is_full == 0) {
					display_charge_percent(sp_mem_addr, percent);
					show_charge_picture(sp_mem_addr, frame_index);
					battery_is_full = 1;
				}
				play_count = -1;  //disable play
				pause_count = 0;  //enable pause
			} else {
				play_count = 0; //enable play
				pause_count = -1; //disable pause
				battery_is_full = 0;
			}
		}
	}

	if(udc_is_init)
		udc_fastboot_exit();

	//restore cpu freq
	set_arm_freq(saved_arm_freq / 1000000);
	//current_arm_freq = auto_pll_divisor(DEV_ARM, GET_FREQ, 0, 0);
	//printf("restore cpu freq = %d\n", current_arm_freq);

	return 0;
}

static int do_check_usbtopc(void)
{
	int ret;
	int delay = 0;
	int max_delay = 300;
	int exit_flag = 0;

	ret = udc_fastboot_init();
	if(ret)
		return -1;

	while (++delay <= max_delay) {
		udc_fastboot_transfer();
		if(wmt_udc_connected()) {
			printf("usb connetct = 1, delay = %d\n", delay);
			break;
		}
		exit_flag |= ctrlc();
		if(exit_flag)
			break;
		mdelay(1);
	}

	if(delay > max_delay)
		printf("usb connetct = 0\n");

	udc_fastboot_exit();

	return 0;
}


U_BOOT_CMD(
	isusbtopc, 1,	0,	do_check_usbtopc,
	"isusbtopc - check if usb connect to pc\n",
	"- check if usb connect to pc\n"
);

