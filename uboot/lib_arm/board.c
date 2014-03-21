/*
 * (C) Copyright 2002
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *
 * (C) Copyright 2002
 * Sysgo Real-Time Solutions, GmbH <www.elinos.com>
 * Marius Groeger <mgroeger@sysgo.de>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * Copyright (c) 2008 WonderMedia Technologies, Inc.
 *
 * This program is free software: you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software Foundation,
 * either version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE.  See the GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * WonderMedia Technologies, Inc.
 * 10F, 529, Chung-Cheng Road, Hsin-Tien, Taipei 231, R.O.C.
 */

#include <common.h>
#include <command.h>
#include <malloc.h>
#include <devices.h>
#include <version.h>
#include <net.h>
#include "../version.h"
#include "../common/wmt_display/wmt_display.h"
#include "../common/wmt_display/minivgui.h"
#include "../board/wmt/include/wmt_iomux.h"
int bootcmdflag;
int showinfoflag;
int g_abort;

#ifdef CONFIG_DRIVER_SMC91111
#include "../drivers/smc91111.h"
#endif
#ifdef CONFIG_DRIVER_LAN91C96
#include "../drivers/lan91c96.h"
#endif

DECLARE_GLOBAL_DATA_PTR;

#include "../define.h"
#define BUILD_ID "BUILDID_"  BUILD_TIME

extern int g_tf_boot;
extern int g_load_script_success;
extern char* CMD_LOAD_SCRIPT;
extern char* CMD_LOAD_USB_SCRIPT;

#define POWERUP_SOURCE_STATUS (0xD8130000 + 0xD0)
#define PWRBTN_BIT           0x01

#if (CONFIG_COMMANDS & CFG_CMD_NAND)
void nand_init(void);
extern ulong nand_probe(ulong physadr);
void nand_init(void)
{
	nand_probe(0xd8009000);
}
#endif

ulong monitor_flash_len;

#ifdef CONFIG_HAS_DATAFLASH
extern int  AT91F_DataflashInit(void);
extern void dataflash_print_info(void);
#endif

#ifndef CONFIG_IDENT_STRING
#define CONFIG_IDENT_STRING ""
#endif

const char version_string[] =
	U_BOOT_VERSION" (" __DATE__ " - " __TIME__ ")"CONFIG_IDENT_STRING;

#ifdef CONFIG_DRIVER_CS8900
extern void cs8900_get_enetaddr(uchar *addr);
#endif

#ifdef CONFIG_DRIVER_RTL8019
extern void rtl8019_get_enetaddr(uchar *addr);
#endif

/*
 * Begin and End of memory area for malloc(), and current "brk"
 */
static ulong mem_malloc_start;
static ulong mem_malloc_end;
static ulong mem_malloc_brk;

static
void mem_malloc_init(ulong dest_addr)
{
	mem_malloc_start = dest_addr;
	mem_malloc_end = dest_addr + CFG_MALLOC_LEN;
	mem_malloc_brk = mem_malloc_start;

	memset_32Byte((void *) mem_malloc_start, 0,
			mem_malloc_end - mem_malloc_start);
}

void *sbrk(ptrdiff_t increment)
{
	ulong old = mem_malloc_brk;
	ulong new = old + increment;

	if ((new < mem_malloc_start) || (new > mem_malloc_end))
		return NULL;

	mem_malloc_brk = new;

	return (void *)old;
}

void raise(void)
{
}

/************************************************************************
 * Init Utilities							*
 ************************************************************************
 * Some of this code should be moved into the core functions,
 * or dropped completely,
 * but let's get it working (again) first...
 */

static int init_baudrate(void)
{
	char tmp[64];	/* long enough for environment variables */
	int i = getenv_r("baudrate", tmp, sizeof(tmp));
	gd->bd->bi_baudrate = gd->baudrate = (i > 0)
			? (int) simple_strtoul(tmp, NULL, 10)
			: CONFIG_BAUDRATE;

	return 0;
}

static int display_banner(void)
{
	bootcmdflag = 1;
	printf("\n\n%s\n", version_string);
	printf("WonderMedia Technologies, Inc.\n");
	printf("U-Boot Version : %s\n", WMT_U_BOOT_VERSION);
	bootcmdflag = 0;
	printf("U-Boot code: %08lX -> %08lX  BSS: -> %08lX\n",
		_armboot_start, _bss_start, _bss_end);
	printf("U-Boot BUILD_ID:%s\n",BUILD_ID);
#ifdef CONFIG_MODEM_SUPPORT
	puts("Modem Support enabled\n");
#endif
#ifdef CONFIG_USE_IRQ
	printf("IRQ Stack: %08lx\n", IRQ_STACK_START);
	printf("FIQ Stack: %08lx\n", FIQ_STACK_START);
#endif

	return 0;
}

static void display_flash_config(ulong size)
{
	puts("Flash: ");
	print_size(size, "\n");
}

int end_logoshow_time = 0;
int g_show_logo = 1;

extern int WMTAccessNandEarier(unsigned long long naddr, unsigned int maddr,
	unsigned int size, int write);
extern int display_show(int format);

void show_logo(void)
{
	//logo sequency in nand: logo1 -- charge animation -- logo2
	char *p_logocmd, *p_logoindex, *p_logo1_size, *p_chargeanim_size, *p_logo2_size;
	char *p_naddr_env, *p_maddr_env;
	unsigned char *p_maddr;
	int logo1_size, chargeanim_size, logo2_size, total_size;
	unsigned int naddr, maddr, backup_img_phy;
	int ret;

	if(g_show_logo == 0)
		return;

	p_logocmd = getenv("logocmd");
	if (p_logocmd) {
		debug("logocmd=\"%s\"\n", p_logocmd);

		if(!g_tf_boot) {
			if((p_logoindex = getenv("wmt.logo.index"))
			    && (p_logoindex[0] == '1')
			    && (p_logo2_size = getenv("wmt.logosize.logo2"))
			    && (p_naddr_env = getenv("wmt.nfc.mtd.u-boot-logo"))
			    && (p_maddr_env = getenv("wmt.display.logoaddr"))
			    && (p_logo1_size = getenv("wmt.logosize.uboot"))
			    && (p_chargeanim_size = getenv("wmt.logosize.charge"))) {
				logo1_size = simple_strtoul(p_logo1_size, NULL, 0);
				chargeanim_size = simple_strtoul(p_chargeanim_size, NULL, 0);
				logo2_size = simple_strtoul(p_logo2_size, NULL, 0);
				naddr = simple_strtoul(p_naddr_env, NULL, 16);
				maddr = simple_strtoul(p_maddr_env, NULL, 16);
				p_maddr = (unsigned char *)maddr;
				if(logo1_size != 0 && chargeanim_size != 0 && logo2_size != 0) {
					total_size = logo1_size + chargeanim_size + logo2_size;
					REG32_VAL(GPIO_BASE_ADDR + 0x200) &= ~(1 << 11); //PIN_SHARE_SDMMC1_NAND
					ret = WMTAccessNandEarier(naddr, maddr, total_size, 0);
					if(!ret) {
						if(*(p_maddr + logo1_size + chargeanim_size) != 'B') {
							printf("logo2: 0x%x = 0x%x\n", p_maddr + logo1_size + chargeanim_size,
							       *(p_maddr + logo1_size + chargeanim_size));
							printf("Error : logo2 is NOT BMP format\n");
							show_text_to_screen("Can not show logo2. Logo2 is NOT BMP format", 0xFF00);
						} else {
							//arm_memmove(p_maddr, p_maddr + logo1_size + chargeanim_size, logo2_size);
							g_logo_x = -1;
							g_logo_y = -1;
							backup_img_phy = g_img_phy;
							g_img_phy = maddr + logo1_size + chargeanim_size;
							display_show(0);
							g_img_phy = backup_img_phy;
						}
					} else {
						printf("load logo2 fail\n");
						show_text_to_screen("Can not show logo2. Load logo2 fail", 0xFF00);
					}
				} else {
					printf("Error: logo1_size = %d, chargeanim_size = %d, logo2_size = %d\n",
					       logo1_size, chargeanim_size, logo2_size);
					if(logo1_size == 0)
						show_text_to_screen("Can not show logo2. 'wmt.logosize.uboot' is wrong", 0xFF00);
					else if(chargeanim_size == 0)
						show_text_to_screen("Can not show logo2. 'wmt.logosize.charge' is wrong", 0xFF00);
					else
						show_text_to_screen("Can not show logo2. 'wmt.logosize.logo2' is wrong", 0xFF00);
				}
			} else
				run_command(p_logocmd, 0);
		} else {
			if((p_logoindex = getenv("wmt.logo.index"))
			    && (p_logoindex[0] == '1')
			    && (p_maddr_env = getenv("wmt.display.logoaddr"))) {
				maddr = simple_strtoul(p_maddr_env, NULL, 16);
				ret = run_command("mmcinit 1", 0);
				if(ret != -1) {
					char tmp[100] = {0};
					sprintf(tmp, "fatload mmc 1 0x%x u-boot-logo2.bmp", maddr);
					ret = run_command(tmp, 0);
					if(ret != -1)  {
						g_logo_x = -1;
						g_logo_y = -1;
						display_show(0);
					} else {
						printf("TF: load u-boot-logo2.bmp failed\n");
						show_text_to_screen("Can not show logo2. fatload logo2 failed", 0xFF00);
					}
				} else {
					printf("TF: run \"mmcinit 1\" failed for show logo2\n");
					show_text_to_screen("Can not show logo2. 'mmcinit 1' failed", 0xFF00);
				}
			} else
				run_command(p_logocmd, 0);
		}
	} else
		debug("'logocmd' is not found , logocmd will not execute\n");

	wmt_read_ostc(&end_logoshow_time);
	g_show_logo = 0;
}

/*
 * Breathe some life into the board...
 *
 * Initialize a serial port as console, and carry out some hardware
 * tests.
 *
 * The first part of initialization is running from Flash memory;
 * its main purpose is to initialize the RAM so that we
 * can relocate the monitor code to RAM.
 */

/*
 * All attempts to come up with a "common" initialization sequence
 * that works for all boards and architectures failed: some of the
 * requirements are just _too_ different. To get rid of the resulting
 * mess of board dependent #ifdef'ed code we now make the whole
 * initialization sequence configurable to the user.
 *
 * The requirements for any new initalization function is simple: it
 * receives a pointer to the "global data" structure as it's only
 * argument, and returns an integer return code, where 0 means
 * "continue" and != 0 means "fatal error, hang the system".
 */
typedef int (init_fnc_t) (void);

/* {JHT */
#if defined(CONFIG_WMT)
int wmt_post(void);
#endif

init_fnc_t *init_sequence[] = {
	cpu_init,		/* basic cpu dependent setup */
	board_init,		/* basic board dependent setup */
	interrupt_init,		/* set up exceptions */
	//env_init,		/* initialize environment */
	init_baudrate,		/* initialze baudrate settings */
	serial_init,		/* serial communications setup */
	console_init_f,		/* stage 1 init of console */
	display_banner,		/* say that we are here */
	dram_init,		/* configure available RAM banks */
#if defined(CONFIG_VCMA9) || defined(CONFIG_CMC_PU2)
	checkboard,
#endif
    /* {JHT} */
#if defined(CONFIG_WMT)
    wmt_post,
#endif
	NULL,
};

extern int hw_recovery(void);
extern int wmt_udc_init(void);
extern void wmt_udc_deinit(void);
extern int wmt_check_udc_to_pc(int is_system_upgrade);
extern int low_power_detect(void);
extern int hw_is_dc_plugin(void);
extern int play_charge_animation(void);
extern int enter_fastboot_mode(void);
extern void do_wmt_poweroff(void);
static void led_control(const char *name);

#ifdef LOGO_DISPLAY_SPEED_TEST
int start_armboot_time;
#endif

#define HSP3_STATUS (0xD8130000 + 0x3C)
#define REBOOT_BIT 0x10

void start_armboot(void)
{
#ifdef LOGO_DISPLAY_SPEED_TEST
	wmt_read_ostc(&start_armboot_time);
#endif

	ulong size;
	init_fnc_t **init_fnc_ptr;
	char *s;
#if defined(CONFIG_VFD) || defined(CONFIG_LCD)
	unsigned long addr;
#endif
	int ret, skip_play_animation, abort = 0;

	/* Pointer is writable since we allocated a register for it */
	gd = (gd_t *)(_armboot_start - CFG_MALLOC_LEN - sizeof(gd_t));
	/* compiler optimization barrier needed for GCC >= 3.4 */
	__asm__ __volatile__("" : : : "memory");

	memset((void *)gd, 0, sizeof(gd_t));
	gd->bd = (bd_t *)((char *)gd - sizeof(bd_t));
	memset(gd->bd, 0, sizeof(bd_t));

	monitor_flash_len = _bss_start - _armboot_start;

	for (init_fnc_ptr = init_sequence; *init_fnc_ptr; ++init_fnc_ptr) {
		if ((*init_fnc_ptr)() != 0)
			hang();
	}

	//Detect power key at here. It can quictly skip charging animation if power key is pressed
	if(REG32_VAL(0xD8130054) & BIT24) {
		skip_play_animation = 1;
	}
	else
		skip_play_animation = 0;

	/* configure available FLASH banks */
	size = flash_init();
	display_flash_config(size);

#ifdef CONFIG_VFD
#	ifndef PAGE_SIZE
#	  define PAGE_SIZE 4096
#	endif
	/*
	 * reserve memory for VFD display (always full pages)
	 */
	/* bss_end is defined in the board-specific linker script */
	addr = (_bss_end + (PAGE_SIZE - 1)) & ~(PAGE_SIZE - 1);
	size = vfd_setmem(addr);
	gd->fb_base = addr;
#endif /* CONFIG_VFD */

#ifdef CONFIG_LCD
#	ifndef PAGE_SIZE
#	  define PAGE_SIZE 4096
#	endif
	/*
	 * reserve memory for LCD display (always full pages)
	 */
	/* bss_end is defined in the board-specific linker script */
	addr = (_bss_end + (PAGE_SIZE - 1)) & ~(PAGE_SIZE - 1);
	size = lcd_setmem(addr);
	gd->fb_base = addr;
#endif /* CONFIG_LCD */

	/* armboot_start is defined in the board-specific linker script */
	mem_malloc_init(_armboot_start - CFG_MALLOC_LEN);

#if 0 //(CONFIG_COMMANDS & CFG_CMD_NAND)
	if ((*((volatile unsigned int *)(0xd8110100))&0x400E) == 0x4008) {
		puts("nandboot: NAND init:");      /* nand boot*/
		nand_init();		/* go init the NAND */
	}
#endif

#ifdef CONFIG_HAS_DATAFLASH
	AT91F_DataflashInit();
	dataflash_print_info();
#endif

	/* initialize environment */
	env_init();
	env_relocate();

	devices_init();	/* get the devices list going. */

	/* get env to turn on/off uboot message if define/undefine "show_info"
	 * uboot parameter 2013/07/31
	 */
	s = getenv("show_info");
	if (s != NULL) {
		showinfoflag = 1;
		printf("show_info = %s\n", s);
	}

	//check power type
	if(REG32_VAL(HSP3_STATUS) & REBOOT_BIT)
		printf("Power on by reset\n");
	else if(REG8_VAL(POWERUP_SOURCE_STATUS) & PWRBTN_BIT)
		printf("Power on by power key\n");
	else
		printf("Power on by adapter\n");

	if (tstc()) {
		if (getc() == 0xd) {  /* "Enter" key */
			printf("Got 'Enter' key\n");
			abort = 1;
			g_abort = 1;
		}
	}

	if(abort == 0) {
		s = getenv("logocmd");
		//if(s != NULL && strstr(s, "display show"))
		if(s != NULL) {
			display_init(0, 0);
			s = getenv("wmt.display.delay");
			if(s != NULL) {
				int displaydelay;
				displaydelay = (int)simple_strtol(s, NULL, 10);
				if(displaydelay > 1000) // limit to 1s
					displaydelay = 1000;
				if(displaydelay > 0)
					mdelay(displaydelay);
			}
		}
	}

	wmt_udc_init();
	led_control(ENV_POWER_LED);
	led_control(ENV_LOGO_LED_CONTROL);

#if (CONFIG_COMMANDS & CFG_CMD_NAND)
	if ((*((volatile unsigned int *)(0xd8110100))&0x4008) == 0x4000) {
		puts("NAND init:");      /* sf boot*/
		nand_init();		/* go init the NAND */
	}
#endif

	if(abort == 0) {
		if(skip_play_animation == 0) {
			if(((s = getenv(ENV_CHARGE_ANIMATION)) != NULL && !strcmp(s, "0"))
				|| ((s = getenv("wmt_sys_directboot")) != NULL && !strcmp(s,"1"))
				|| ((s = getenv("wmt_sys_restore")) != NULL && !strcmp(s,"1"))
				|| (!hw_is_dc_plugin() && (REG8_VAL(POWERUP_SOURCE_STATUS) & PWRBTN_BIT))
				|| (REG32_VAL(0xD813003C) & 0x10))  //power by reset
					skip_play_animation = 1;
		}

		if(!(REG32_VAL(HSP3_STATUS) & REBOOT_BIT)
			&& !hw_is_dc_plugin()
			&& !(REG8_VAL(POWERUP_SOURCE_STATUS) & PWRBTN_BIT)) {
			printf("Adapter is removed\nPower off\n");
			do_wmt_poweroff();
		}

		if(skip_play_animation)
			show_logo();
	}

	hw_recovery();

	enter_fastboot_mode();

#ifdef CONFIG_VFD
	/* must do this after the framebuffer is allocated */
	drv_vfd_init();
#endif /* CONFIG_VFD */

#if 0
	/* IP Address */
	gd->bd->bi_ip_addr = getenv_IPaddr("ipaddr");

	/* MAC Address */
	{
		int i;
		ulong reg;
		char *s, *e;
		char tmp[64];

		i = getenv_r("ethaddr", tmp, sizeof(tmp));
		s = (i > 0) ? tmp : NULL;

		for (reg = 0; reg < 6; ++reg) {
			gd->bd->bi_enetaddr[reg] = s ? simple_strtoul(s, &e, 16) : 0;
			if (s)
				s = (*e) ? e + 1 : e;
		}
	}
#endif
	char *s_install_dev;
	if(abort == 0){
		if((s_install_dev=getenv("wmt.install.dev"))!=NULL && strcmp(s_install_dev,"usb")==0)
		{
			if (run_command("usb reset", 0) != -1 ) {
				if(run_command(CMD_LOAD_USB_SCRIPT, 0) != -1) {
					g_load_script_success = 1;
					printf("run \"%s\" success\n", CMD_LOAD_USB_SCRIPT);
					run_command("addfwcenv FirmwareInstall/config", 0);
					skip_play_animation = 1;
					wmt_check_udc_to_pc(1);
				}else {
					g_load_script_success = 0;
					wmt_check_udc_to_pc(0);
				}
			}else {
				g_load_script_success = 0;
				wmt_check_udc_to_pc(0);
			}
		}
		else
		{
			if (run_command("mmcinit 0", 0) != -1 ) {
				if(run_command(CMD_LOAD_SCRIPT, 0) != -1) {
					g_load_script_success = 1;
					printf("run \"%s\" success\n", CMD_LOAD_SCRIPT);
					run_command("addfwcenv FirmwareInstall/config", 0);
					skip_play_animation = 1;
					wmt_check_udc_to_pc(1);
					//display_init(0, 1);
				}else {
					g_load_script_success = 0;
					wmt_check_udc_to_pc(0);
				}
			}else {
				g_load_script_success = 0;
				wmt_check_udc_to_pc(0);
			}
		}

		if(skip_play_animation == 0) {
			ret = play_charge_animation();
			if(ret > 0)
				low_power_detect();

			g_show_logo = 1;
		} else
			low_power_detect();
	}
	else
		wmt_check_udc_to_pc(0);

	wmt_udc_deinit();

#ifdef CONFIG_CMC_PU2
	load_sernum_ethaddr();
#endif /* CONFIG_CMC_PU2 */

	jumptable_init();

	console_init_r();	/* fully init console as a device */

#if defined(CONFIG_MISC_INIT_R)
	/* miscellaneous platform dependent initialisations */
	misc_init_r();
#endif

	/* enable exceptions */
	enable_interrupts();

	/* Perform network card initialisation if necessary */
#ifdef CONFIG_DRIVER_CS8900
	cs8900_get_enetaddr(gd->bd->bi_enetaddr);
#endif

#if defined(CONFIG_DRIVER_SMC91111) || defined(CONFIG_DRIVER_LAN91C96)
	if (getenv("ethaddr"))
		smc_set_mac_addr(gd->bd->bi_enetaddr);
#endif /* CONFIG_DRIVER_SMC91111 || CONFIG_DRIVER_LAN91C96 */

	/* Initialize from environment */
	s = getenv("loadaddr");
	if (s != NULL)
		load_addr = simple_strtoul(s, NULL, 16);
#if (CONFIG_COMMANDS & CFG_CMD_NET)
	s = getenv("bootfile");
	if (s != NULL)
		copy_filename(BootFile, s, sizeof(BootFile));
#endif	/* CFG_CMD_NET */

#ifdef BOARD_LATE_INIT
	board_late_init();
#endif
#if (CONFIG_COMMANDS & CFG_CMD_NET)
#if defined(CONFIG_NET_MULTI)
	puts("Net:   ");
#endif
	eth_initialize(gd->bd);
#endif

	/* main_loop() can return to retry autoboot, if so just run it again. */
	for (;;)
		main_loop();

	/* NOTREACHED - no way out of command loop except booting */
}

void hang(void)
{
	puts("### ERROR ### Please RESET the board ###\n");
	for (;;)
		;
}

#ifdef CONFIG_MODEM_SUPPORT
/* called from main loop (common/main.c) */
extern void  dbg(const char *fmt, ...);
int mdm_init(void)
{
	char env_str[16];
	char *init_str;
	int i;
	extern char console_buffer[];
	static inline void mdm_readline(char *buf, int bufsiz);
	extern void enable_putc(void);
	extern int hwflow_onoff(int);

	enable_putc(); /* enable serial_putc() */

#ifdef CONFIG_HWFLOW
	init_str = getenv("mdm_flow_control");
	if (init_str && (strcmp(init_str, "rts/cts") == 0))
		hwflow_onoff(1);
	else
		hwflow_onoff(-1);
#endif

	for (i = 1; ; i++) {
		sprintf(env_str, "mdm_init%d", i);
		init_str = getenv(env_str);
		if (init_str != NULL) {
			serial_puts(init_str);
			serial_puts("\n");
			for ( ; ; ) {
				mdm_readline(console_buffer, CFG_CBSIZE);
				dbg("ini%d: [%s]", i, console_buffer);

				if ((strcmp(console_buffer, "OK") == 0) ||
					(strcmp(console_buffer, "ERROR") == 0)) {
					dbg("ini%d: cmd done", i);
					break;
				} else /* in case we are originating call ... */
					if (strncmp(console_buffer, "CONNECT", 7) == 0) {
						dbg("ini%d: connect", i);
						return 0;
					}
			}
		} else
			break; /* no init string - stop modem init */

		udelay(100000);
	}

	udelay(100000);

	/* final stage - wait for connect */
	for ( ; i > 1; ) { /* if 'i' > 1 - wait for connection
				  message from modem */
		mdm_readline(console_buffer, CFG_CBSIZE);
		dbg("ini_f: [%s]", console_buffer);
		if (strncmp(console_buffer, "CONNECT", 7) == 0) {
			dbg("ini_f: connected");
			return 0;
		}
	}

	return 0;
}

/* 'inline' - We have to do it fast */
static inline void mdm_readline(char *buf, int bufsiz)
{
	char c;
	char *p;
	int n;

	n = 0;
	p = buf;
	for ( ; ; ) {
		c = serial_getc();

		/*		dbg("(%c)", c); */

		switch (c) {
		case '\r':
			break;
		case '\n':
			*p = '\0';
			return;

		default:
			if (n++ > bufsiz) {
				*p = '\0';
				return; /* sanity check */
			}
			*p = c;
			p++;
			break;
		}
	}
}
#endif	/* CONFIG_MODEM_SUPPORT */

static void led_control(const char *name)
{
	int ret, is_light;
	struct gpio_param_t led_control_pin;

	ret = parse_gpio_param(name, &led_control_pin);

	if(ret)
		return;

	is_light = led_control_pin.flag;

	if(is_light){
		gpio_direction_output(led_control_pin.gpiono, led_control_pin.act);
	}else{
		gpio_direction_output(led_control_pin.gpiono, !led_control_pin.act);
	}

    return;
}

