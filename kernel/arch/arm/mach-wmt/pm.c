/*++
linux/arch/arm/mach-wmt/pm.c

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
#include <linux/init.h>
#include <linux/suspend.h>
#include <linux/errno.h>
#include <linux/time.h>
#include <linux/sysctl.h>
#include <linux/interrupt.h>
#include <linux/vmalloc.h>
#include <linux/delay.h>
#include <linux/module.h>
#include <linux/timer.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/cpufreq.h>

#include <mach/hardware.h>
#include <asm/memory.h>
#include <asm/system.h>
#include <asm/leds.h>
#include <asm/io.h>
#include <linux/rtc.h>

#include <asm/cacheflush.h>
#include <asm/hardware/cache-l2x0.h>

#include <mach/wmt_secure.h>

#include <linux/gpio.h>
#include <mach/wmt_iomux.h>





//#define CONFIG_KBDC_WAKEUP
//#define KB_WAKEUP_SUPPORT
//#define MOUSE_WAKEUP_SUPPORT

#define	SOFT_POWER_SUPPORT
#define	RTC_WAKEUP_SUPPORT
//#ifdef CONFIG_RMTCTL_WonderMedia //define if CIR menuconfig enable
#define CIR_WAKEUP_SUPPORT
//#endif
#define KEYPAD_POWER_SUPPORT
#define PMWT_C_WAKEUP(src, type)  ((type & PMWT_TYPEMASK) << (((src - 24) & PMWT_WAKEUPMASK) * 4))

enum wakeup_intr_tri_src_e {
	WKS_T_WK0         = 0,    /* General Purpose Wakeup Source 0          */
	WKS_T_WK2,                /* General Purpose Wakeup Source 1          */
	WKS_T_WK3,                /* General Purpose Wakeup Source 2          */
	WKS_T_WK4,                /* General Purpose Wakeup Source 3          */
	WKS_T_SUS0,               /* General Purpose Wakeup Source 4          */
	WKS_T_SUS1,               /* General Purpose Wakeup Source 5          */
	WKS_T_USBATTA0,           /* USBATTA0          */
	WKS_T_CIRIN,              /* CIRIN          */	
	WKS_T_USBOC0,			    /* WKS_USBOC0 as wakeup            */
	WKS_T_USBOC1,    			/* WKS_USBOC0 as wakeup            */
	WKS_T_USBOC2,    			/* WKS_USBOC0 as wakeup            */
	WKS_T_USBOC3,    			/* WKS_USBOC0 as wakeup            */		
	WKS_T_UHC,    			/* UHC interrupt as wakeup                  */
	WKS_T_UDC,    			/* WKS_UDC interrupt as wakeup              */	
	WKS_T_CIR,    			/* CIR interrupt as wakeupr                 */
	WKS_T_USBSW0,    			/* USBSW0 interrupt as wakeupr          */
	WKS_T_SD3			= 18,   /* SD3 interrupt as wakeupr                 */	
	WKS_T_DCDET		= 19,   /* DCDET interrupt as wakeupr               */	
	WKS_T_SD2			= 20,   /* SD2 interrupt as wakeupr                 */	
	WKS_T_HDMICEC		= 21,   /* HDMICEC interrupt as wakeupr             */	
	WKS_T_SD0			= 22,   /* SD0 interrupt as wakeupr                 */
	WKS_T_WK5			= 23,   /* Wakeup event number                      */	
	WKS_T_PWRBTN		= 24,   /* PWRBTN as wakeup            */
	WKS_T_RTC			= 25,   /* RTC as wakeup            */	
	CA9MP_RST		= 26,   /* CA9MP_RST            */	
	SOFT_RST		= 27,   /* SOFT_RST            */	
	CORE0_WD_RST	= 28,   /* CORE0_WD_RST            */	
	CORE1_WD_RST	= 29    /* CORE1_WD_RST            */	
};

static struct wakeup_source *wmt_ws;

static struct workqueue_struct *wakeup_queue;
static struct delayed_work wakeupwork;

static struct {
	bool have_switch;
	unsigned int gpio_no;
	unsigned int wakeup_source;
}hall_switch;


#define DRIVER_NAME	"PMC"
#if defined(CONFIG_PM_RTC_IS_GMT) && defined(RTC_WAKEUP_SUPPORT)
#include <linux/rtc.h>
#endif

/*
 *  Debug macros
 */
#ifdef DEBUG
#  define DPRINTK(fmt, args...) printk("%s: " fmt, __FUNCTION__ , ## args)
#else
#  define DPRINTK(fmt, args...)
#endif

/*
 * For saving discontinuous registers during hibernation.
 */
#define SAVE(x)		(saved[SAVED_##x] = (x##_VAL))
#define RESTORE(x)	((x##_VAL) = saved[SAVED_##x])

enum {
	SAVED_SP = 0,
	SAVED_OSTW, SAVED_OSTI,
	SAVED_PMCEL, SAVED_PMCEU,
	SAVED_PMCE2, SAVED_PMCE3,
	/* SAVED_ATAUDMA, */
	SAVED_SIZE
};

struct apm_dev_s {
    char id;
};


extern unsigned int wmt_read_oscr(void);
extern void wmt_read_rtc(unsigned int *date, unsigned int *time);
extern void wmt_serial_set_reg(void);

/* Hibernation entry table physical address */
#define LOADER_ADDR												0xffff0000
#define HIBERNATION_ENTER_EXIT_CODE_BASE_ADDR	0xFFFFFFC0
#define DO_POWER_ON_SLEEP			            (HIBERNATION_ENTER_EXIT_CODE_BASE_ADDR + 0x00)
#define DO_POWER_OFF_SUSPEND			        (HIBERNATION_ENTER_EXIT_CODE_BASE_ADDR + 0x04)
#define DO_WM_IO_SET							        (HIBERNATION_ENTER_EXIT_CODE_BASE_ADDR + 0x34)

static unsigned int exec_at = (unsigned int)-1;

/*from = 4 high memory*/
static void (*theKernel)(int from);
//static void (*theKernel_io)(int from);

#if defined(SOFT_POWER_SUPPORT) && defined(CONFIG_PROC_FS)
//static struct proc_dir_entry *proc_softpower;
static unsigned int softpower_data;
#endif

static long rtc2sys;

struct work_struct	PMC_shutdown;
struct work_struct	PMC_sync;


extern int wmt_getsyspara(char *varname, char *varval, int *varlen);
static int power_on_debounce_value = 100;  /*power button debounce time when power on state*/
static int resume_debounce_value = 2000;	/*power button debounce time when press button to resume system*/
static int power_up_debounce_value = 2000; /*power button debounce time when press button to power up*/
#define min_debounce_value 0
#define max_debounce_value 4000

char hotplug_path[256] = "/sbin/hotplug";
static int sync_counter = 0;
static unsigned int time1, time2;

#define REG_VAL(addr) (*((volatile unsigned int *)(addr)))


#ifdef KEYPAD_POWER_SUPPORT
#include <linux/input.h>
#define KPAD_POWER_FUNCTION_NUM  1
#define power_button_timeout (HZ/10)
static struct input_dev *kpadPower_dev;
static unsigned int kpadPower_codes[KPAD_POWER_FUNCTION_NUM] = {
        [0] = KEY_POWER
};
static unsigned int powerKey_is_pressed;
static unsigned int pressed_jiffies;
static struct timer_list   kpadPower_timer;
static spinlock_t kpadPower_lock;
#endif

#ifdef CONFIG_BATTERY_WMT
static unsigned int battery_used;
#endif

#ifdef CONFIG_CACHE_L2X0
unsigned int l2x0_onoff;
unsigned int l2x0_aux;
unsigned int l2x0_prefetch_ctrl;
unsigned int en_static_address_filtering = 0;
unsigned int address_filtering_start = 0xD8000000;
unsigned int address_filtering_end = 0xD9000000;
static volatile unsigned int l2x0_base;
unsigned int cpu_trustzone_enabled = 0;
#endif

//gri
static unsigned int var_fake_power_button=0;
static unsigned int var_wake_type2=0;
static unsigned int var_wake_type=0;
static unsigned int var_wake_param=0;
static unsigned int var_wake_en=0;
static unsigned int var_1st_flag=0;

volatile unsigned int Wake_up_sts_mask = 0;// all static we add pwbn
static	unsigned int dynamic_wakeup = 0;
static	unsigned int dynamic_pmc_intr = 0;

static unsigned int pmlock_1st_flag=0;
static unsigned int pmlock_intr_1st_flag=0;

spinlock_t	wmt_pm_lock;
spinlock_t	wmt_pm_intr_lock;

unsigned int WMT_WAKE_UP_EVENT;//for printing wakeup event

/* wmt_pwrbtn_debounce_value()
 *
 * Entry to set the power button debounce value, the time unit is ms.
 */
static void wmt_pwrbtn_debounce_value(unsigned int time)
{
	volatile unsigned long debounce_value = 0 ;
	unsigned long pmpb_value = 0;

	/*add a delay to wait pmc & rtc  sync*/
	udelay(130);
	
	/*Debounce value unit is 1024 * RTC period ,RTC is 32KHz so the unit is ~ 32ms*/
	if (time % 32)
		debounce_value = (time / 32) + 1;
	else
		debounce_value = (time / 32);

	pmpb_value = PMPB_VAL;
	pmpb_value &= ~ PMPB_DEBOUNCE(0xff);
	pmpb_value |= PMPB_DEBOUNCE(debounce_value);

	PMPB_VAL = pmpb_value;
	//udelay(100);
	DPRINTK("[%s] PMPB_VAL = 0x%.8X \n",__func__,PMPB_VAL);
}

/* wmt_power_up_debounce_value()
 *
 * Entry to set the power button debounce value, the time unit is ms.
 */
void wmt_power_up_debounce_value(void) {

	//printk("[%s] power_up_debounce_value = %d \n",__func__,power_up_debounce_value);
	wmt_pwrbtn_debounce_value(power_up_debounce_value);

}

static void run_sync(struct work_struct *work)
{
	int ret;
  char *argv[] = { "/system/etc/wmt/script/force.sh", "PMC", NULL };
	char *envp_shutdown[] =
	{ "HOME=/", "PATH=/sbin:/bin:/usr/sbin:/usr/bin", "", NULL };

	wmt_pwrbtn_debounce_value(power_up_debounce_value);
	DPRINTK("[%s] start\n",__func__);
	ret = call_usermodehelper(argv[0], argv, envp_shutdown, 0);
	DPRINTK("[%s] sync end\n",__func__);
}

static void run_shutdown(struct work_struct *work)
{
	int ret;
    char *argv[] = { hotplug_path, "PMC", NULL };
	char *envp_shutdown[] =
		{ "HOME=/", "PATH=/sbin:/bin:/usr/sbin:/usr/bin", "ACTION=shutdown", NULL };
	DPRINTK("[%s] \n",__func__);
   
	wmt_pwrbtn_debounce_value(power_up_debounce_value);
	ret = call_usermodehelper(argv[0], argv, envp_shutdown, 0);
}

//kevin add support wakeup3/wakeup0 to wakeup ap
#include <mach/viatel.h>
irqreturn_t viatelcom_irq_cp_wake_ap(int irq, void *data);
extern int gpio_viatel_4wire[4];

void pmc_enable_wakeup_isr(enum wakeup_src_e wakeup_event, unsigned int type)
{	  
	  unsigned long	pm_lock_flags;
	  unsigned int	wakeup_event_bit, wakeup_event_type;
	  unsigned int wakeup_event_temp;
	  
	  if (type > 4)
		type = 4;
	  
	  if (! pmlock_intr_1st_flag) {
	    pmlock_intr_1st_flag = 1;
	    spin_lock_init(&wmt_pm_intr_lock);
	  }

	  spin_lock_irqsave(&wmt_pm_intr_lock, pm_lock_flags);	  
	  switch (wakeup_event) {
		  case WKS_WK0:
			wakeup_event_bit = (INT_TRG_EN_VAL | (1 << WKS_T_WK0));
			wakeup_event_type = ((INT_TYPE0_VAL & (~(0xf << (WKS_T_WK0 << 2)))) | (type << (WKS_T_WK0 << 2)));
			do {
				INT_TYPE0_VAL = wakeup_event_type;
			} while(INT_TYPE0_VAL != wakeup_event_type);
			break;
		  case WKS_WK2:
			wakeup_event_bit = (INT_TRG_EN_VAL | (1 << WKS_T_WK2));
			wakeup_event_type = ((INT_TYPE0_VAL & (~(0xf << (WKS_T_WK2 << 2)))) | (type << (WKS_T_WK2 << 2)));
			do {
				INT_TYPE0_VAL = wakeup_event_type;
			} while(INT_TYPE0_VAL != wakeup_event_type);
			break;
		  case WKS_WK3:
			wakeup_event_bit = (INT_TRG_EN_VAL | (1 << WKS_T_WK3));
			wakeup_event_type = ((INT_TYPE0_VAL & (~(0xf << (WKS_T_WK3 << 2)))) | (type << (WKS_T_WK3 << 2)));
			do {
				INT_TYPE0_VAL = wakeup_event_type;
			} while(INT_TYPE0_VAL != wakeup_event_type);
			break;		
		  case WKS_WK4:
			wakeup_event_bit = (INT_TRG_EN_VAL | (1 << WKS_T_WK4));
			wakeup_event_type = ((INT_TYPE0_VAL & (~(0xf << (WKS_T_WK4 << 2)))) | (type << (WKS_T_WK4 << 2)));
			do {
				INT_TYPE0_VAL = wakeup_event_type;
			} while(INT_TYPE0_VAL != wakeup_event_type);
			break;
		  case WKS_SUS0:
			wakeup_event_bit = (INT_TRG_EN_VAL | (1 << WKS_T_SUS0));
			wakeup_event_type = ((INT_TYPE0_VAL & (~(0xf << (WKS_T_SUS0 << 2)))) | (type << (WKS_T_SUS0 << 2)));
			do {
				INT_TYPE0_VAL = wakeup_event_type;
			} while(INT_TYPE0_VAL != wakeup_event_type);
			break;
		  case WKS_SUS1:
			wakeup_event_bit = (INT_TRG_EN_VAL | (1 << WKS_T_SUS1));
			wakeup_event_type = ((INT_TYPE0_VAL & (~(0xf << (WKS_T_SUS1 << 2)))) | (type << (WKS_T_SUS1 << 2)));
			do {
				INT_TYPE0_VAL = wakeup_event_type;
			} while(INT_TYPE0_VAL != wakeup_event_type);
			break;
		  case WKS_USBATTA0:
			wakeup_event_bit = (INT_TRG_EN_VAL | (1 << WKS_T_USBATTA0));
			wakeup_event_type = ((INT_TYPE0_VAL & (~(0xf << (WKS_T_USBATTA0 << 2)))) | (type << (WKS_T_USBATTA0 << 2)));
			do {
				INT_TYPE0_VAL = wakeup_event_type;
			} while(INT_TYPE0_VAL != wakeup_event_type);
			break;
		  case WKS_CIRIN:
			wakeup_event_bit = (INT_TRG_EN_VAL | (1 << WKS_T_CIRIN));
			wakeup_event_type = ((INT_TYPE0_VAL & (~(0xf << (WKS_T_CIRIN << 2)))) | (type << (WKS_T_CIRIN << 2)));
			do {
				INT_TYPE0_VAL = wakeup_event_type;
			} while(INT_TYPE0_VAL != wakeup_event_type);
			break;
		  case WKS_PWRBTN:
			wakeup_event_bit = (INT_TRG_EN_VAL | (1 << WKS_T_PWRBTN));
			break;
		  case WKS_RTC:
			wakeup_event_bit = (INT_TRG_EN_VAL | (1 << WKS_T_RTC));
			break;
		  case WKS_USBOC0:
			wakeup_event_bit = (INT_TRG_EN_VAL | (1 << WKS_T_USBOC0));
			wakeup_event_temp = WKS_USBOC0 - WKS_USBOC0;
			wakeup_event_type = ((INT_TYPE2_VAL & (~(0xf << (wakeup_event_temp << 2)))) | (type << (wakeup_event_temp << 2)));
			do {
				INT_TYPE2_VAL = wakeup_event_type;
			} while(INT_TYPE2_VAL != wakeup_event_type);
			break;
		  case WKS_USBOC1:
			wakeup_event_bit = (INT_TRG_EN_VAL | (1 << WKS_T_USBOC1));
			wakeup_event_temp = WKS_USBOC1 - WKS_USBOC0;
			wakeup_event_type = ((INT_TYPE2_VAL & (~(0xf << (wakeup_event_temp << 2)))) | (type << (wakeup_event_temp << 2)));
			do {
				INT_TYPE2_VAL = wakeup_event_type;
			} while(INT_TYPE2_VAL != wakeup_event_type);
			break;
		  case WKS_USBOC2:
			wakeup_event_bit = (INT_TRG_EN_VAL | (1 << WKS_T_USBOC2));
			wakeup_event_temp = WKS_USBOC2 - WKS_USBOC0;
			wakeup_event_type = ((INT_TYPE2_VAL & (~(0xf << (wakeup_event_temp << 2)))) | (type << (wakeup_event_temp << 2)));
			do {
				INT_TYPE2_VAL = wakeup_event_type;
			} while(INT_TYPE2_VAL != wakeup_event_type);
			break;
		  case WKS_USBOC3:
			wakeup_event_bit = (INT_TRG_EN_VAL | (1 << WKS_T_USBOC3));
			wakeup_event_temp = WKS_USBOC3 - WKS_USBOC0;
			wakeup_event_type = ((INT_TYPE2_VAL & (~(0xf << (wakeup_event_temp << 2)))) | (type << (wakeup_event_temp << 2)));
			do {
				INT_TYPE2_VAL = wakeup_event_type;
			} while(INT_TYPE2_VAL != wakeup_event_type);
			break;
		  case WKS_UHC:
			wakeup_event_bit = (INT_TRG_EN_VAL | (1 << WKS_T_UHC));
			wakeup_event_temp = WKS_UHC - WKS_USBOC0;
			wakeup_event_type = ((INT_TYPE2_VAL & (~(0xf << (wakeup_event_temp << 2)))) | (type << (wakeup_event_temp << 2)));
			do {
				INT_TYPE2_VAL = wakeup_event_type;
			} while(INT_TYPE2_VAL != wakeup_event_type);
			break;	
		  case WKS_UDC:
			wakeup_event_bit = (INT_TRG_EN_VAL | (1 << WKS_T_UDC));
			wakeup_event_temp = WKS_UDC - WKS_USBOC0;
			wakeup_event_type = ((INT_TYPE2_VAL & (~(0xf << (wakeup_event_temp << 2)))) | (type << (wakeup_event_temp << 2)));
			do {
				INT_TYPE2_VAL = wakeup_event_type;
			} while(INT_TYPE2_VAL != wakeup_event_type);
			break;
		  case WKS_CIR:
			wakeup_event_bit = (INT_TRG_EN_VAL | (1 << WKS_T_CIR));
			wakeup_event_temp = WKS_CIR - WKS_USBOC0;
			wakeup_event_type = ((INT_TYPE2_VAL & (~(0xf << (wakeup_event_temp << 2)))) | (type << (wakeup_event_temp << 2)));
			do {
				INT_TYPE2_VAL = wakeup_event_type;
			} while(INT_TYPE2_VAL != wakeup_event_type);
			break;
		  case WKS_USBSW0:
			wakeup_event_bit = (INT_TRG_EN_VAL | (1 << WKS_T_USBSW0));
			wakeup_event_temp = WKS_USBSW0 - WKS_USBOC0;
			wakeup_event_type = ((INT_TYPE2_VAL & (~(0xf << (wakeup_event_temp << 2)))) | (type << (wakeup_event_temp << 2)));
			do {
				INT_TYPE2_VAL = wakeup_event_type;
			} while(INT_TYPE2_VAL != wakeup_event_type);
			break;
		  case WKS_SD3:
			wakeup_event_bit = (INT_TRG_EN_VAL | (1 << WKS_T_SD3));
			wakeup_event_temp = WKS_SD3 - WKS_SD3 + 2;
			wakeup_event_type = ((INT_TYPE1_VAL & (~(0xf << (wakeup_event_temp << 2)))) | (type << (wakeup_event_temp << 2)));
			do {
				INT_TYPE1_VAL = wakeup_event_type;
			} while(INT_TYPE1_VAL != wakeup_event_type);
			break;
		  case WKS_DCDET:
			wakeup_event_bit = (INT_TRG_EN_VAL | (1 << WKS_T_DCDET));
			wakeup_event_temp = WKS_DCDET - WKS_SD3 + 2;
			wakeup_event_type = ((INT_TYPE1_VAL & (~(0xf << (wakeup_event_temp << 2)))) | (type << (wakeup_event_temp << 2)));
			do {
				INT_TYPE1_VAL = wakeup_event_type;
			} while(INT_TYPE1_VAL != wakeup_event_type);
			break;
		  case WKS_SD2:
			wakeup_event_bit = (INT_TRG_EN_VAL | (1 << WKS_T_SD2));
			wakeup_event_temp = WKS_SD2 - WKS_SD3 + 2;
			wakeup_event_type = ((INT_TYPE1_VAL & (~(0xf << (wakeup_event_temp << 2)))) | (type << (wakeup_event_temp << 2)));
			do {
				INT_TYPE1_VAL = wakeup_event_type;
			} while(INT_TYPE1_VAL != wakeup_event_type);
			break;
		  case WKS_HDMICEC:
			wakeup_event_bit = (INT_TRG_EN_VAL | (1 << WKS_T_HDMICEC));
			wakeup_event_temp = WKS_HDMICEC - WKS_SD3 + 2;
			wakeup_event_type = ((INT_TYPE1_VAL & (~(0xf << (wakeup_event_temp << 2)))) | (type << (wakeup_event_temp << 2)));
			do {
				INT_TYPE1_VAL = wakeup_event_type;
			} while(INT_TYPE1_VAL != wakeup_event_type);
			break;
		  case WKS_SD0:
			wakeup_event_bit = (INT_TRG_EN_VAL | (1 << WKS_T_SD0));
			wakeup_event_temp = WKS_SD0 - WKS_SD3 + 2;
			wakeup_event_type = ((INT_TYPE1_VAL & (~(0xf << (wakeup_event_temp << 2)))) | (type << (wakeup_event_temp << 2)));
			do {
				INT_TYPE1_VAL = wakeup_event_type;
			} while(INT_TYPE1_VAL != wakeup_event_type);
			break;
		  case WKS_WK5:
			wakeup_event_bit = (INT_TRG_EN_VAL | (1 << WKS_T_WK5));
			wakeup_event_temp = WKS_WK5 - WKS_SD3 + 2;
			wakeup_event_type = ((INT_TYPE1_VAL & (~(0xf << (wakeup_event_temp << 2)))) | (type << (wakeup_event_temp << 2)));
			do {
				INT_TYPE1_VAL = wakeup_event_type;
			} while(INT_TYPE1_VAL != wakeup_event_type);
			break;
		  default:
			goto pmc_enable_wakeup_isr_error;
			break;
	  }	  
	  
	  while (PMCIS_VAL & (1 << wakeup_event)) {
		PMCIS_VAL = (1 << wakeup_event);
	  }	  
	  
	  do {
		INT_TRG_EN_VAL = wakeup_event_bit;
	  } while(INT_TRG_EN_VAL != wakeup_event_bit);	  
	  dynamic_pmc_intr = INT_TRG_EN_VAL;

pmc_enable_wakeup_isr_error:	  
	  spin_unlock_irqrestore(&wmt_pm_intr_lock, pm_lock_flags);

}
EXPORT_SYMBOL(pmc_enable_wakeup_isr);

void pmc_disable_wakeup_isr(enum wakeup_src_e wakeup_event)
{

	  unsigned long	pm_lock_flags;
	  unsigned int	wakeup_event_bit;
	  	  
	  if (! pmlock_intr_1st_flag) {
	    pmlock_intr_1st_flag = 1;
	    spin_lock_init(&wmt_pm_intr_lock);
	  }

	  spin_lock_irqsave(&wmt_pm_intr_lock, pm_lock_flags);		  
	  switch (wakeup_event) {
		  case WKS_WK0:
			wakeup_event_bit = (INT_TRG_EN_VAL & (~(1 << WKS_T_WK0)));
			break;
		  case WKS_WK2:
			wakeup_event_bit = (INT_TRG_EN_VAL & (~(1 << WKS_T_WK2)));
			break;
		  case WKS_WK3:
			wakeup_event_bit = (INT_TRG_EN_VAL & (~(1 << WKS_T_WK3)));
			break;		
		  case WKS_WK4:
			wakeup_event_bit = (INT_TRG_EN_VAL & (~(1 << WKS_T_WK4)));
			break;
		  case WKS_SUS0:
			wakeup_event_bit = (INT_TRG_EN_VAL & (~(1 << WKS_T_SUS0)));
			break;
		  case WKS_SUS1:
			wakeup_event_bit = (INT_TRG_EN_VAL & (~(1 << WKS_T_SUS1)));
			break;
		  case WKS_USBATTA0:
			wakeup_event_bit = (INT_TRG_EN_VAL & (~(1 << WKS_T_USBATTA0)));
			break;
		  case WKS_CIRIN:
			wakeup_event_bit = (INT_TRG_EN_VAL & (~(1 << WKS_T_CIRIN)));
			break;
		  case WKS_PWRBTN:
			wakeup_event_bit = (INT_TRG_EN_VAL & (~(1 << WKS_T_PWRBTN)));
			break;
		  case WKS_RTC:
			wakeup_event_bit = (INT_TRG_EN_VAL & (~(1 << WKS_T_RTC)));
			break;
		  case WKS_USBOC0:
			wakeup_event_bit = (INT_TRG_EN_VAL & (~(1 << WKS_T_USBOC0)));
			break;
		  case WKS_USBOC1:
			wakeup_event_bit = (INT_TRG_EN_VAL & (~(1 << WKS_T_USBOC1)));
			break;
		  case WKS_USBOC2:
			wakeup_event_bit = (INT_TRG_EN_VAL & (~(1 << WKS_T_USBOC2)));
			break;
		  case WKS_USBOC3:
			wakeup_event_bit = (INT_TRG_EN_VAL & (~(1 << WKS_T_USBOC3)));
			break;
		  case WKS_UHC:
			wakeup_event_bit = (INT_TRG_EN_VAL & (~(1 << WKS_T_UHC)));
			break;	
		  case WKS_UDC:
			wakeup_event_bit = (INT_TRG_EN_VAL & (~(1 << WKS_T_UDC)));
			break;
		  case WKS_CIR:
			wakeup_event_bit = (INT_TRG_EN_VAL & (~(1 << WKS_T_CIR)));
			break;
		  case WKS_USBSW0:
			wakeup_event_bit = (INT_TRG_EN_VAL & (~(1 << WKS_T_USBSW0)));
			break;
		  case WKS_SD3:
			wakeup_event_bit = (INT_TRG_EN_VAL & (~(1 << WKS_T_SD3)));
			break;
		  case WKS_DCDET:
			wakeup_event_bit = (INT_TRG_EN_VAL & (~(1 << WKS_T_DCDET)));
			break;
		  case WKS_SD2:
			wakeup_event_bit = (INT_TRG_EN_VAL & (~(1 << WKS_T_SD2)));
			break;
		  case WKS_HDMICEC:
			wakeup_event_bit = (INT_TRG_EN_VAL & (~(1 << WKS_T_HDMICEC)));
			break;
		  case WKS_SD0:
			wakeup_event_bit = (INT_TRG_EN_VAL & (~(1 << WKS_T_SD0)));
			break;
		  case WKS_WK5:
			wakeup_event_bit = (INT_TRG_EN_VAL & (~(1 << WKS_T_WK5)));
			break;
		  default:
			goto pmc_disable_wakeup_isr_error;
			break;
	  }	  
	  
	  do {
		INT_TRG_EN_VAL = wakeup_event_bit;
	  } while(INT_TRG_EN_VAL != wakeup_event_bit);	  	  	 
	  dynamic_pmc_intr = INT_TRG_EN_VAL;

pmc_disable_wakeup_isr_error:	  
	  spin_unlock_irqrestore(&wmt_pm_intr_lock, pm_lock_flags);
  
}
EXPORT_SYMBOL(pmc_disable_wakeup_isr);

void pmc_clear_intr_status(enum wakeup_src_e wakeup_event)
{
	  unsigned long	pm_lock_flags;
	  unsigned int	wakeup_event_bit;
	  	  
	  if (! pmlock_intr_1st_flag) {
	    pmlock_intr_1st_flag = 1;
	    spin_lock_init(&wmt_pm_intr_lock);
	  }

	  wakeup_event_bit = (1 << wakeup_event);
	  
	  spin_lock_irqsave(&wmt_pm_intr_lock, pm_lock_flags);	
	  
	  while (PMCIS_VAL & wakeup_event_bit) {
		PMCIS_VAL = wakeup_event_bit;
	  }

	  spin_unlock_irqrestore(&wmt_pm_intr_lock, pm_lock_flags);
}
EXPORT_SYMBOL(pmc_clear_intr_status);

void pmc_clear_wakeup_status(enum wakeup_src_e wakeup_event)
{
	  unsigned long	pm_lock_flags;
	  unsigned int	wakeup_event_bit;
	  	  
	  if (! pmlock_1st_flag) {
	    pmlock_1st_flag = 1;
	    spin_lock_init(&wmt_pm_lock);
	  }

	  wakeup_event_bit = (1 << wakeup_event);
	  
	  spin_lock_irqsave(&wmt_pm_lock, pm_lock_flags);	
	  
	  while (PMWS_VAL & wakeup_event_bit) {
		PMWS_VAL = wakeup_event_bit;
	  }

	  spin_unlock_irqrestore(&wmt_pm_lock, pm_lock_flags);
}
EXPORT_SYMBOL(pmc_clear_wakeup_status);

void pmc_enable_wakeup_event(enum wakeup_src_e wakeup_event, unsigned int type)
{
	  unsigned int which_bit;
	  unsigned ori_type = 0;
	  unsigned long	pm_lock_flags;	

	  if ((wakeup_event > WKS_CIRIN) && (wakeup_event < WKS_PWRBTN))
	    return;
		
	  if ((wakeup_event > WKS_USBSW0) && (wakeup_event < WKS_SD3))
	    return;		
	  
	  if (type > 4)
	    return;
	  
	  if (! pmlock_1st_flag) {
	    pmlock_1st_flag = 1;
	    spin_lock_init(&wmt_pm_lock);
	  }

	  which_bit = (1 << wakeup_event);
	  
	  spin_lock_irqsave(&wmt_pm_lock, pm_lock_flags);
	  if (which_bit < 0x100) {

		ori_type = PMWT_VAL;
		ori_type &= (~(0xf << (wakeup_event << 2)));
	    ori_type |= (type << (wakeup_event << 2));
		do {
			PMWT_VAL = ori_type; 
		} while(PMWT_VAL != ori_type);
	  } else if (which_bit >= 0x1000000) {
	    unsigned int temp;
	
	    temp = wakeup_event - 24;

		ori_type = PMWTC_VAL;
	    ori_type &= (~(0xf << (temp << 2)));
	    ori_type |= (type << (temp << 2));
		do {
			PMWTC_VAL = ori_type;
		} while(PMWTC_VAL != ori_type);
	  } else if (which_bit >= 0x10000) {
	    unsigned int temp;
	    temp = wakeup_event - 16;

		ori_type = WK_EVT_TYPE_VAL;
	    ori_type &= (~(0xf << (temp << 2)));
	    ori_type |= (type << (temp << 2));
		do {
			WK_EVT_TYPE_VAL = ori_type;
		} while(WK_EVT_TYPE_VAL != ori_type);		
	  }
	  dynamic_wakeup |= (1 << wakeup_event);
 
	  spin_unlock_irqrestore(&wmt_pm_lock, pm_lock_flags);	  
}
EXPORT_SYMBOL(pmc_enable_wakeup_event);

void pmc_disable_wakeup_event(enum wakeup_src_e wakeup_event)
{
	  
	  if (! pmlock_1st_flag) {
	    pmlock_1st_flag = 1;
	    spin_lock_init(&wmt_pm_lock);
	  }

	  dynamic_wakeup &= (~(1 << wakeup_event));
		
}
EXPORT_SYMBOL(pmc_disable_wakeup_event);


static void wakeup_func(struct work_struct *work)
{	
	struct file *fp;
	mm_segment_t fs;
	loff_t pos=0;
	unsigned long flags;
	char buf[3];
	
	fp = filp_open("/sys/class/backlight/pwm-backlight.0/brightness", O_RDWR, 0777);

	if (IS_ERR(fp)) {
        printk(KERN_ERR"open file error\n");
        return;
    }
	
	fs = get_fs();
    set_fs(KERNEL_DS);
	vfs_read(fp, buf, sizeof(buf), &pos);
	filp_close(fp, NULL);
    set_fs(fs);
	//printk(KERN_ERR"%s buf %s\n",__FUNCTION__,buf);
	if(gpio_get_value(hall_switch.gpio_no)){
		if(strncmp(buf,"0",1))
		{
			return;
		}
			
	}
	else{		
		if(!strncmp(buf,"0",1))
		{
			return;
		}
	}

#ifdef KEYPAD_POWER_SUPPORT
			if(kpadPower_dev) {
	
				spin_lock_irqsave(&kpadPower_lock, flags);
				if(!powerKey_is_pressed) {
					powerKey_is_pressed = 1; 
					input_report_key(kpadPower_dev, KEY_POWER, 1); //power key is pressed
					input_sync(kpadPower_dev);
					pressed_jiffies = jiffies;
					wmt_pwrbtn_debounce_value(power_up_debounce_value);
					DPRINTK("\n[%s]power key pressed -->\n",__func__);
					time1 = jiffies_to_msecs(jiffies);
					__pm_wakeup_event(wmt_ws, (MSEC_PER_SEC >> 4));
				}
				//disable_irq(IRQ_PMC_WAKEUP);
				spin_unlock_irqrestore(&kpadPower_lock, flags);
				mod_timer(&kpadPower_timer, jiffies + power_button_timeout);
			}
	#endif
}

static irqreturn_t pmc_wakeup_isr(int this_irq, void *dev_id)
{
	unsigned int status_i;
	unsigned long flags;

	status_i = PMCIS_VAL;

	rmb();


    //kevin add for wakeup3 to wakeup ap
	if(status_i & (gpio_viatel_4wire[GPIO_VIATEL_USB_MDM_WAKE_AP]==149?BIT0:BIT2)){
		//printk("call viatelcom_irq_cp_wake_ap\n");
		viatelcom_irq_cp_wake_ap(this_irq,dev_id);
		PMCIS_VAL |= (gpio_viatel_4wire[GPIO_VIATEL_USB_MDM_WAKE_AP]==149?BIT0:BIT2);
		
	}

	/*
	 * TODO : wakeup event and  interrupt event share the same interrupt
	 * source IRQ_PMC. we should make a mechanism like 'request_irq' to
	 * register interrupt event callback function, and call the right
	 * function here.
	 */
	/* DCDET interrupt */
	//if (status_i & BIT27) {
	//	extern void dcdet_isr_callback(void);
	//	dcdet_isr_callback();
	//	PMCIS_VAL |= BIT0;
	//}

	if(hall_switch.have_switch && (status_i & (1<<(hall_switch.wakeup_source)))){
		//printk(KERN_ERR"call wakeup0\n");
		queue_delayed_work(wakeup_queue, &wakeupwork, msecs_to_jiffies(50));
		pmc_clear_intr_status(hall_switch.wakeup_source);
	}
	
	if (status_i & BIT14) {
		
			pmc_clear_intr_status(WKS_PWRBTN);
	#ifdef KEYPAD_POWER_SUPPORT
		if(kpadPower_dev) {

			spin_lock_irqsave(&kpadPower_lock, flags);
			if(!powerKey_is_pressed) {
				powerKey_is_pressed = 1; 
				input_report_key(kpadPower_dev, KEY_POWER, 1); //power key is pressed
				input_sync(kpadPower_dev);
				pressed_jiffies = jiffies;
				wmt_pwrbtn_debounce_value(power_up_debounce_value);
				DPRINTK("\n[%s]power key pressed -->\n",__func__);
				time1 = jiffies_to_msecs(jiffies);
				__pm_wakeup_event(wmt_ws, (MSEC_PER_SEC >> 4));
			}
			//disable_irq(IRQ_PMC_WAKEUP);
			spin_unlock_irqrestore(&kpadPower_lock, flags);
			mod_timer(&kpadPower_timer, jiffies + power_button_timeout);
		}
	#endif
		
	#if defined(SOFT_POWER_SUPPORT) && defined(CONFIG_PROC_FS)
		softpower_data = 1;
	#endif

	}
	
	return IRQ_HANDLED;		
}

extern int PM_device_PostSuspend(void);
extern int PM_device_PreResume(void);

void check_pmc_busy(void)
{
	while (PMCS2_VAL&0x3F0038)
	;
}
void save_plla_speed(unsigned int *plla_div)
{
	plla_div[0] = PMARM_VAL;/*arm_div*/
	check_pmc_busy();
	plla_div[1]= PML2C_VAL;;/*l2c_div*/
	check_pmc_busy();
	plla_div[2] = PML2CTAG_VAL;/*l2c_tag_div*/
	check_pmc_busy();
	plla_div[3] = PML2CDATA_VAL;/*l2c_data_div*/
	check_pmc_busy();
	plla_div[4] = PML2CAXI_VAL;/*axi_l2c_div*/
	check_pmc_busy();
	plla_div[5] = PMDBGAPB_VAL;/*dbg_apb_div*/
	check_pmc_busy();
	plla_div[6] = PMPMA_VAL;
	check_pmc_busy();
	
}

void save_pllb_speed(unsigned int *pllb_div)
{
	check_pmc_busy();
	pllb_div[0] = PMPWM_VAL;
	check_pmc_busy();
	pllb_div[1] = PMSDMMC_VAL;
	check_pmc_busy();
	pllb_div[2] = PMSDMMC1_VAL;
	check_pmc_busy();
	pllb_div[3] = PMSDMMC2_VAL;
	check_pmc_busy();
	pllb_div[4] = PMI2C0_VAL;
	check_pmc_busy();
	pllb_div[5] = PMI2C1_VAL;
	check_pmc_busy();
	pllb_div[6] = PMI2C2_VAL;
	check_pmc_busy();
	pllb_div[7] = PMI2C3_VAL;
	check_pmc_busy();
	pllb_div[8] = PMI2C4_VAL;
	check_pmc_busy();
	pllb_div[9] = PMSF_VAL;
	check_pmc_busy();
	pllb_div[10] = PMSPI_VAL;
	check_pmc_busy();
	pllb_div[11] = PMSPI1_VAL;
	check_pmc_busy();
	pllb_div[12] = PMNAND_VAL;
	check_pmc_busy();
	pllb_div[13] = PMNA12_VAL;
	check_pmc_busy();
	pllb_div[14] = PMADC_VAL;
	check_pmc_busy();
	pllb_div[15] = PMCSI0_VAL;
	check_pmc_busy();
	pllb_div[16] = PMCSI1_VAL;
	check_pmc_busy();
	pllb_div[17] = PMMALI_VAL;
	check_pmc_busy();
	pllb_div[18] = PMPAXI_VAL;
	check_pmc_busy();
	pllb_div[19] = PMSE_VAL;
	check_pmc_busy();
	pllb_div[20] = PMPCM0_VAL;
	check_pmc_busy();
	pllb_div[21] = PMPCM1_VAL;
	check_pmc_busy();
	pllb_div[22] = PMAHB_VAL;
	check_pmc_busy();
	pllb_div[23] = PMAPB0_VAL;
	check_pmc_busy();
	pllb_div[24] = PMPMB_VAL;
	check_pmc_busy();

}

void save_plld_speed(unsigned int *plld_div)
{
	check_pmc_busy();
	plld_div[0] = PMWMTVDU_VAL;/*vdu_div*/
	check_pmc_busy();
	plld_div[1]= PMCNMVDU_VAL;;/*cnm_div*/
	check_pmc_busy();
	plld_div[2] = PMWMTNA_VAL;/*na_vdu_div*/
	check_pmc_busy();
	plld_div[3] = PMCNMNA_VAL;/*na_cnm_div*/
	check_pmc_busy();	
}


void restore_plla_speed(unsigned int *plla_div)
{

	auto_pll_divisor(DEV_ARM, SET_PLLDIV, 2, 300);
	PMARM_VAL = plla_div[0];/*arm_div*/
	wmb();
	check_pmc_busy();
	PML2C_VAL = plla_div[1];/*l2c_div*/
	wmb();
	check_pmc_busy();
	PML2CTAG_VAL = plla_div[2];/*l2c_tag_div*/
	wmb();
	check_pmc_busy();
	PML2CDATA_VAL = plla_div[3];/*l2c_data_div*/
	wmb();
	check_pmc_busy();
	PML2CAXI_VAL = plla_div[4];/*l2c_axi_div*/
	wmb();
	check_pmc_busy();
	PMDBGAPB_VAL = plla_div[5];/*dbg_apb_div*/
	wmb();
	check_pmc_busy();	
	PMPMA_VAL = plla_div[6];
	wmb();
	check_pmc_busy();
}

void restore_pllb_speed(unsigned int *pllb_div)
{
	check_pmc_busy();
	PMPWM_VAL = pllb_div[0];
	wmb();
	check_pmc_busy();
#if 0	
	PMSDMMC_VAL = pllb_div[1];
	wmb();
	check_pmc_busy();
	PMSDMMC1_VAL = pllb_div[2];
	wmb();
	check_pmc_busy();
	PMSDMMC2_VAL = pllb_div[3];
	wmb();
	check_pmc_busy();
#endif	
	PMI2C0_VAL = pllb_div[4];
	wmb();
	check_pmc_busy();
	PMI2C1_VAL = pllb_div[5];
	wmb();
	check_pmc_busy();
	PMI2C2_VAL = pllb_div[6];
	wmb();
	check_pmc_busy();
	PMI2C3_VAL = pllb_div[7];
	wmb();
	check_pmc_busy();
	PMI2C4_VAL = pllb_div[8];
	wmb();
	check_pmc_busy();
	PMSF_VAL = pllb_div[9];
	wmb();
	check_pmc_busy();
	PMSPI_VAL = pllb_div[10];
	wmb();
	check_pmc_busy();
	PMSPI1_VAL = pllb_div[11];
	wmb();
	check_pmc_busy();
	PMNAND_VAL = pllb_div[12];
	wmb();
	check_pmc_busy();
	PMNA12_VAL = pllb_div[13];
	wmb();
	check_pmc_busy();
	PMADC_VAL = pllb_div[14];
	wmb();
	check_pmc_busy();
	PMCSI0_VAL = pllb_div[15];
	wmb();
	check_pmc_busy();
	PMCSI1_VAL = pllb_div[16];
	wmb();
	check_pmc_busy();
	PMMALI_VAL = pllb_div[17];
	wmb();
	check_pmc_busy();
	PMPAXI_VAL = pllb_div[18];
	wmb();
	check_pmc_busy();
	PMSE_VAL = pllb_div[19];
	wmb();
	check_pmc_busy();
	PMPCM0_VAL = pllb_div[20];
	wmb();
	check_pmc_busy();
	PMPCM1_VAL = pllb_div[21];
	wmb();
	check_pmc_busy();
	PMAHB_VAL = pllb_div[22];
	wmb();
	check_pmc_busy();
	PMAPB0_VAL = pllb_div[23];
	wmb();
	check_pmc_busy();
	PMPMB_VAL = pllb_div[24];
	wmb();
	check_pmc_busy();

}

void restore_plld_speed(unsigned int *plld_div)
{
	check_pmc_busy();
	PMWMTVDU_VAL = plld_div[0]; /*vdu_div*/
	wmb();
	check_pmc_busy();
	PMCNMVDU_VAL = plld_div[1]; /*cnm_div*/
	wmb();
	check_pmc_busy();
	PMWMTNA_VAL = plld_div[2]; /*na_vdu_div*/
	wmb();
	check_pmc_busy();
	PMCNMNA_VAL = plld_div[3]; /*na_cnm_div*/
	wmb();
	check_pmc_busy();
}


/* wmt_pm_standby()
 *
 * Entry to the power-on sleep hibernation mode.
 */
static void wmt_pm_standby(void)
{
	volatile unsigned int hib_phy_addr = 0,base = 0;    

#ifdef CONFIG_CACHE_L2X0
	__u32 power_ctrl;
	
	if( l2x0_onoff == 1)
	{
		outer_cache.flush_all();
		outer_cache.disable();
		outer_cache.inv_all();
	}
#endif

	/* Get standby virtual address entry point */    
	base = (unsigned int)ioremap_nocache(LOADER_ADDR, 0x10000);

	exec_at = base + (DO_POWER_ON_SLEEP - LOADER_ADDR);
	hib_phy_addr = *(unsigned int *) exec_at;
	exec_at = base + (hib_phy_addr - LOADER_ADDR);

	//led_light(3);
	theKernel = (void (*)(int))exec_at;		/* set rom address */
	theKernel(4);					        /* jump to rom */
	//led_light(3);

	iounmap((void __iomem *)base);

#ifdef CONFIG_CACHE_L2X0
	if( l2x0_onoff == 1)
	{
		if (!(readl_relaxed(l2x0_base + L2X0_CTRL) & 1))
		{
			if(cpu_trustzone_enabled == 0)
			{
				/* l2x0 controller is disabled */
				writel_relaxed(l2x0_aux, l2x0_base + L2X0_AUX_CTRL);

				if( en_static_address_filtering == 1 )
				{
					writel_relaxed(address_filtering_end, l2x0_base + 0xC04);
					writel_relaxed((address_filtering_start | 0x01), l2x0_base + 0xC00);
				}

				writel_relaxed(0x110, l2x0_base + L2X0_TAG_LATENCY_CTRL);
				writel_relaxed(0x110, l2x0_base + L2X0_DATA_LATENCY_CTRL);
				power_ctrl = readl_relaxed(l2x0_base + L2X0_POWER_CTRL) | L2X0_DYNAMIC_CLK_GATING_EN | L2X0_STNDBY_MODE_EN;
				writel_relaxed(power_ctrl, l2x0_base + L2X0_POWER_CTRL);

				writel_relaxed(l2x0_prefetch_ctrl, l2x0_base + L2X0_PREFETCH_CTRL);

				outer_cache.inv_all();

				/* enable L2X0 */
				writel_relaxed(1, l2x0_base + L2X0_CTRL);
			}
			else
			{
				/* l2x0 controller is disabled */
				wmt_smc(WMT_SMC_CMD_PL310AUX, l2x0_aux);
				
				if( en_static_address_filtering == 1 )
				{
					wmt_smc(WMT_SMC_CMD_PL310FILTER_END, address_filtering_end);
					wmt_smc(WMT_SMC_CMD_PL310FILTER_START, (address_filtering_start | 0x01));
				}

				wmt_smc(WMT_SMC_CMD_PL310TAG_LATENCY, 0x110);
				wmt_smc(WMT_SMC_CMD_PL310DATA_LATENCY, 0x110);
				power_ctrl = readl_relaxed(l2x0_base + L2X0_POWER_CTRL) | L2X0_DYNAMIC_CLK_GATING_EN | L2X0_STNDBY_MODE_EN;
				wmt_smc(WMT_SMC_CMD_PL310POWER, power_ctrl);

				wmt_smc(WMT_SMC_CMD_PL310PREFETCH, l2x0_prefetch_ctrl);

				outer_cache.inv_all();

				/* enable L2X0 */
				wmt_smc(WMT_SMC_CMD_PL310CTRL, 1);
			}
		
		}
	
	}
#endif
}

extern void wmt_assem_suspend(void);
extern void wmt_assem_secure_suspend(void);

extern char use_dvfs;
/* wmt_pm_suspend()
 *
 * Entry to the power-off suspend hibernation mode.
 */

static void wmt_pm_suspend(void)
{
	unsigned int saved[SAVED_SIZE];
	int result;
	unsigned int plla_div[7];
	
	unsigned int pllb_div[25];

	unsigned int plld_div[4];


#ifdef CONFIG_CACHE_L2X0
	__u32 power_ctrl;
#endif

/* FIXME */
#if 1
	result = PM_device_PostSuspend();
	if (result)
		printk("PM_device_PostSuspend fail\n");
#endif

#ifdef CONFIG_CACHE_L2X0
	if( l2x0_onoff == 1)
	{
		outer_cache.flush_all();
		outer_cache.disable();
		outer_cache.inv_all();
	}
#endif

	SAVE(OSTW);	                /* save vital registers */
	SAVE(OSTI);	    
	SAVE(PMCEL);	            /* save clock gating */
	SAVE(PMCEU);
	SAVE(PMCE2);
	SAVE(PMCE3);

	*(volatile unsigned int *)0xfe018008 |= 0x03030303; //scu output pm	

	//for single core
#ifndef CONFIG_SMP	
	HSP7_VAL = 0xffffffb8;
	while(HSP7_VAL != 0xffffffb8);
	asm("sev" : : "r" (0));	
#endif		
	save_plld_speed(plld_div); /*save plld clock register*/

	save_pllb_speed(pllb_div); /*save pllb clock register*/
	if (!use_dvfs)
		save_plla_speed(plla_div);
	//led_light(2);	

	wmt_assem_suspend();


	RESTORE(PMCE3);
	RESTORE(PMCE2);
	RESTORE(PMCEU);             /* restore clock gating */
	RESTORE(PMCEL);             
	RESTORE(OSTI);              /* restore vital registers */
	RESTORE(OSTW);
	wmt_serial_set_reg();

	if (!use_dvfs)
		restore_plla_speed(plla_div);	/* restore plla clock register */

	restore_pllb_speed(pllb_div);	/* restore pllb clock register */
	
	restore_plld_speed(plld_div);	/* restore plld clock register */

	PMPB_VAL |= 1;				/* enable soft power */
	
	//* ((volatile unsigned int *)0xfe140054) = 0x0;

#ifdef CONFIG_CACHE_L2X0
	if( l2x0_onoff == 1)
	{
		if (!(readl_relaxed(l2x0_base + L2X0_CTRL) & 1))
		{
			if(cpu_trustzone_enabled == 0)
			{
				/* l2x0 controller is disabled */
				writel_relaxed(l2x0_aux, l2x0_base + L2X0_AUX_CTRL);
			
				if( en_static_address_filtering == 1 )
				{
					writel_relaxed(address_filtering_end, l2x0_base + 0xC04);
					writel_relaxed((address_filtering_start | 0x01), l2x0_base + 0xC00);
				}

				writel_relaxed(0x110, l2x0_base + L2X0_TAG_LATENCY_CTRL);
				writel_relaxed(0x110, l2x0_base + L2X0_DATA_LATENCY_CTRL);
				power_ctrl = readl_relaxed(l2x0_base + L2X0_POWER_CTRL) | L2X0_DYNAMIC_CLK_GATING_EN | L2X0_STNDBY_MODE_EN;
				writel_relaxed(power_ctrl, l2x0_base + L2X0_POWER_CTRL);

				writel_relaxed(l2x0_prefetch_ctrl, l2x0_base + L2X0_PREFETCH_CTRL);

	  			outer_cache.inv_all(); 

				/* enable L2X0 */
				writel_relaxed(1, l2x0_base + L2X0_CTRL);
			}
			else
			{
				/* l2x0 controller is disabled */
				wmt_smc(WMT_SMC_CMD_PL310AUX, l2x0_aux);
			
				if( en_static_address_filtering == 1 )
				{
					wmt_smc(WMT_SMC_CMD_PL310FILTER_END, address_filtering_end);
					wmt_smc(WMT_SMC_CMD_PL310FILTER_START, (address_filtering_start | 0x01));
				}

				wmt_smc(WMT_SMC_CMD_PL310TAG_LATENCY, 0x110);
				wmt_smc(WMT_SMC_CMD_PL310DATA_LATENCY, 0x110);
				power_ctrl = readl_relaxed(l2x0_base + L2X0_POWER_CTRL) | L2X0_DYNAMIC_CLK_GATING_EN | L2X0_STNDBY_MODE_EN;
				wmt_smc(WMT_SMC_CMD_PL310POWER, power_ctrl);

				wmt_smc(WMT_SMC_CMD_PL310PREFETCH, l2x0_prefetch_ctrl);

	  			outer_cache.inv_all(); 

				/* enable L2X0 */
				wmt_smc(WMT_SMC_CMD_PL310CTRL, 1);
			}
		}
	}
#endif

	result = PM_device_PreResume();
	if (result != 0)
		printk("PM_device_PreResume fail\n");
}

/* wmt_pm_enter()
 *
 * To Finally enter the sleep state.
 *
 * Note: Only support PM_SUSPEND_STANDBY and PM_SUSPEND_MEM
 */
int wmt_trigger_resume_kpad = 0;
int wmt_trigger_resume_notify = 0;

void wmt_resume_kpad(void)
{
	DPRINTK(KERN_ALERT "\n[%s]power key pressed\n",__func__);
	powerKey_is_pressed = 1; 
	input_report_key(kpadPower_dev, KEY_POWER, 1); /*power key is pressed*/
	input_sync(kpadPower_dev);
	pressed_jiffies = jiffies;
	mod_timer(&kpadPower_timer, jiffies + power_button_timeout);
}

void wmt_resume_notify(void)
{
	input_report_key(kpadPower_dev, KEY_POWER, 1); /*power key is pressed*/
	input_sync(kpadPower_dev);
	input_report_key(kpadPower_dev, KEY_POWER, 0); //power key is released
	input_sync(kpadPower_dev);		
}

static int wmt_pm_enter(suspend_state_t state)
{
	unsigned int status, status_i;
//	unsigned int wakeup_notify;

	int notify_framwork = 0;
	
	if (!((state == PM_SUSPEND_STANDBY) || (state == PM_SUSPEND_MEM))) {
		printk(KERN_ALERT "%s, Only support PM_SUSPEND_STANDBY and PM_SUSPEND_MEM\n", DRIVER_NAME);
		return -EINVAL;
	}

	/* only enable fiq in normal operation */
	//local_fiq_disable();
	//local_irq_disable();
	/* disable system OS timer */
	OSTC_VAL &= ~OSTC_ENABLE;
  
	
	/* FIXME, 2009/09/15 */
	//PMCDS_VAL = PMCDS_VAL;
	
	WMT_WAKE_UP_EVENT = 0;
	/*set power button debounce value*/
	wmt_pwrbtn_debounce_value(resume_debounce_value);

	/*
	 * We use pm_standby as apm_standby for power-on hibernation.
	 * but we still suspend memory in both case.
	 */

	if (state == PM_SUSPEND_STANDBY)
		wmt_pm_standby();    /* Go to standby mode*/
	else
		wmt_pm_suspend();    /* Go to suspend mode*/

	/*set power button debounce value*/
	wmt_pwrbtn_debounce_value(power_on_debounce_value);

	/*
	 * Clean wakeup source
	 */

	status = PMWS_VAL;
	status_i = PMCIS_VAL;
	WMT_WAKE_UP_EVENT = (PMWS_VAL & (WK_TRG_EN_VAL | 0x4000));//wmt_pm_enter
	status = (status & Wake_up_sts_mask);
	
	PMCIS_VAL = PMCIS_VAL;	
	
	//notify_framwork=1;

#ifdef KEYPAD_POWER_SUPPORT	
	if (status & (1 << WKS_PWRBTN)) {
		wmt_trigger_resume_kpad = 1;	
	}
#endif	

	if (status & (1 << WKS_WK5)){  
		DPRINTK(KERN_ALERT "\n[%s]WK5 event\n",__func__);
		if (var_fake_power_button & (1 << WKS_WK5))
			notify_framwork=1;
	}

	if (status & (1 << WKS_SD0)){  
		DPRINTK(KERN_ALERT "\n[%s]SD0 event\n",__func__);
		if (var_fake_power_button & (1 << WKS_SD0))
			notify_framwork=1; 
	}

	if (status & (1 << WKS_HDMICEC)){  
		DPRINTK(KERN_ALERT "\n[%s]HDMICEC event\n",__func__);
		if (var_fake_power_button & (1 << WKS_HDMICEC))
			notify_framwork=1;
	}
	
	if (status & (1 << WKS_SD2)){  
		DPRINTK(KERN_ALERT "\n[%s]SD2 event\n",__func__);
		if (var_fake_power_button & (1 << WKS_SD2))
			notify_framwork=1;
	}		

	if (status & (1 << WKS_DCDET)){  
		DPRINTK(KERN_ALERT "\n[%s]DCDET event\n",__func__);
		if (var_fake_power_button & (1 << WKS_DCDET))
			notify_framwork=1;
	}			

	if (status & (1 << WKS_SD3)){  
		DPRINTK(KERN_ALERT "\n[%s]SD3 event\n",__func__);
		if (var_fake_power_button & (1 << WKS_SD3))
			notify_framwork=1; 
	}

	if (status & (1 << WKS_USBSW0)){  
		DPRINTK(KERN_ALERT "\n[%s]USBSW0 event\n",__func__);
		if (var_fake_power_button & (1 << WKS_USBSW0))
			notify_framwork=1;
	}

#ifdef CIR_WAKEUP_SUPPORT   
	if (status & (1 << WKS_CIR)){  
		DPRINTK(KERN_ALERT "\n[%s]CIR event\n",__func__);
		if (var_fake_power_button & (1 << WKS_CIR))
			notify_framwork=1;
	}
#endif		

#ifdef CONFIG_USB_GADGET_WMT
/*UDC wake up source*/
	if (status & (1 << WKS_UDC)){
		DPRINTK(KERN_ALERT "\n[%s]UDC event\n",__func__);
		if (var_fake_power_button & (1 << WKS_UDC))
			notify_framwork=1;
	}
#endif

#ifdef CONFIG_USB   
//UHC  
	if (status & (1 << WKS_UHC)){
		DPRINTK(KERN_ALERT "\n[%s]UHC event\n",__func__);
		if (var_fake_power_button & (1 << WKS_UHC))
			notify_framwork=1;
	}
	
	if (status & (1 << WKS_USBOC3)){
		DPRINTK(KERN_ALERT "\n[%s]USBOC3 event\n",__func__);
		if (var_fake_power_button & (1 << WKS_USBOC3))
			notify_framwork=1;
	}

	if (status & (1 << WKS_USBOC2)){
		DPRINTK(KERN_ALERT "\n[%s]USBOC2 event\n",__func__);
		if (var_fake_power_button & (1 << WKS_USBOC2))
			notify_framwork=1;
	}

	if (status & (1 << WKS_USBOC1)){
		DPRINTK(KERN_ALERT "\n[%s]USBOC1 event\n",__func__);
		if (var_fake_power_button & (1 << WKS_USBOC1))
			notify_framwork=1;
	}

	if (status & (1 << WKS_USBOC0)){
		DPRINTK(KERN_ALERT "\n[%s]USBOC0 event\n",__func__);
		if (var_fake_power_button & (1 << WKS_USBOC0))
			notify_framwork=1;
	}		
#endif
	
#ifdef RTC_WAKEUP_SUPPORT
	if (status & (1 << WKS_RTC)){
		DPRINTK(KERN_ALERT "\n[%s]RTC event\n",__func__);
		if (var_fake_power_button & (1 << WKS_RTC))
			notify_framwork=1;
	}
#endif

	if (status & (1 << WKS_CIRIN)){
		DPRINTK(KERN_ALERT "\n[%s]CIRIN event\n",__func__);
		if (var_fake_power_button & (1 << WKS_CIRIN))
			notify_framwork=1;
	}

#ifdef CONFIG_USB_GADGET_WMT	
	/*UDC2 wake up source*/
	if (status & (1 << WKS_USBATTA0)){
		DPRINTK(KERN_ALERT "\n[%s]UDCATTA0 event\n",__func__);
		if (var_fake_power_button & (1 << WKS_USBATTA0))
			notify_framwork=1;
	}
#endif

	if(status & (1 << WKS_SUS1)){
		DPRINTK(KERN_ALERT "\n[%s]SUS1 event\n",__func__);
		if (var_fake_power_button & (1 << WKS_SUS1))
			notify_framwork=1;
	}

	if(status & (1 << WKS_SUS0)){
		DPRINTK(KERN_ALERT "\n[%s]SUS0 event\n",__func__);
		if (var_fake_power_button & (1 << WKS_SUS0))
			notify_framwork=1;
	}
	
	if(status & (1 << WKS_WK4)){
		DPRINTK(KERN_ALERT "\n[%s]WK4 event\n",__func__);
		if (var_fake_power_button & (1 << WKS_WK4))
			notify_framwork=1;
	}

	if (status & (1 << WKS_WK3)){  
		DPRINTK(KERN_ALERT "\n[%s]WK3 event\n",__func__);
		if (var_fake_power_button & (1 << WKS_WK3))
			notify_framwork=1;	
	}
	
	if (status & (1 << WKS_WK2)){  
		DPRINTK(KERN_ALERT "\n[%s]WK2 event\n",__func__);
		if (var_fake_power_button & (1 << WKS_WK2))
			notify_framwork=1;
	}		
	
	if (status & (1 << WKS_WK0)){  		
		DPRINTK(KERN_ALERT "\n[%s]WK0 event\n",__func__);
		if (var_fake_power_button & (1 << WKS_WK0))
			notify_framwork=1;	
	}

	if (notify_framwork) {
		wmt_trigger_resume_notify = 1;
	}

#ifdef RTC_WAKEUP_SUPPORT
	RTAS_VAL = 0x0;       	/* Disable RTC alarm */
#endif

	/*
	 * Force to do once CPR for system.
	 */
	OSM1_VAL = wmt_read_oscr() + LATCH;
	OSTC_VAL |= OSTC_ENABLE;

	//udelay(200);  /* delay for resume not complete */
	
	/*
	 * disable trigger wakeup event
	 */
	do {
		WK_TRG_EN_VAL = 0x4000;
	} while(WK_TRG_EN_VAL != 0x4000);

	return 0;
}


/* wmt_pm_prepare()
 *
 * Called after processes are frozen, but before we shut down devices.
 */
static int wmt_pm_prepare(void)
{
	unsigned int date, time;
	struct timeval tv;

	/*
	 * Estimate time zone so that wmt_pm_finish can update the GMT time
	 */

	rtc2sys = 0;
    
	if ((*(volatile unsigned int *)SYSTEM_CFG_CTRL_BASE_ADDR)>0x34260102) {        
		wmt_read_rtc(&date, &time);
	}

	do_gettimeofday(&tv);

	rtc2sys = mktime(RTCD_YEAR(date) + ((RTCD_CENT(date) * 100) + 2000),
						RTCD_MON(date),
						RTCD_MDAY(date),
						RTCT_HOUR(time),
						RTCT_MIN(time),
						RTCT_SEC(time));
	rtc2sys = rtc2sys-tv.tv_sec;
	if (rtc2sys > 0)
		rtc2sys += 10;
	else
		rtc2sys -= 10;
	rtc2sys = rtc2sys/60/60;
	rtc2sys = rtc2sys*60*60;

	return 0;
}

/* wmt_pm_finish()
 *
 * Called after devices are re-setup, but before processes are thawed.
 */
static void wmt_pm_finish(void)
{
	unsigned int date, time;
	struct timespec tv;

#if 0
	struct rtc_time tm;
	unsigned long tmp = 0;
	struct timeval tv1;
#endif
	/* FIXME:
	 * There are a warning when call iounmap here,
	 * please iounmap the mapped virtual ram later */
	// iounmap((void *)exec_at);

	/*
	 * Update kernel time spec.
	 */
	if ((*(volatile unsigned int *)SYSTEM_CFG_CTRL_BASE_ADDR)>0x34260102) {        
		wmt_read_rtc(&date, &time);
	}

	tv.tv_nsec = 0;
	tv.tv_sec = mktime(RTCD_YEAR(date) + ((RTCD_CENT(date) * 100) + 2000),
						RTCD_MON(date),
						RTCD_MDAY(date),
						RTCT_HOUR(time),
						RTCT_MIN(time),
						RTCT_SEC(time));
	/* RTC stores local time, adjust GMT time, tv */
	tv.tv_sec = tv.tv_sec-rtc2sys;
	do_settimeofday(&tv);

}

static int wmt_pm_valid(suspend_state_t state)
{
	switch (state) {
	case PM_SUSPEND_STANDBY:
	case PM_SUSPEND_MEM:
		return 1;
	default:
		return 0;
	}
}

static struct platform_suspend_ops wmt_pm_ops = {
	.valid			= wmt_pm_valid,
	.prepare        = wmt_pm_prepare,
	.enter          = wmt_pm_enter,
	.finish         = wmt_pm_finish,
};

#if 0
#if defined(SOFT_POWER_SUPPORT) && defined(CONFIG_PROC_FS)

static int procfile_read(char *page, char **start, off_t off, int count, int *eof, void *data)
{
	int len = 0;
	len = sprintf(page, "%d\n", softpower_data);
	return len;
}


static int procfile_write(struct file *file, const char *buffer, unsigned long count, void *data)
{
	char *endp;

	softpower_data = simple_strtoul(buffer, &endp, 0);

	if (softpower_data != 0)
		softpower_data = 1;

/*	printk("%s: %s, softpower_data=[0x%X]\n", DRIVER_NAME, __FUNCTION__, softpower_data );*/
/*	printk("%s: return [%d]\n", DRIVER_NAME, (count + endp - buffer ) );*/

	return (count + endp - buffer);
}

#endif	/* defined(SOFT_POWER_SUPPORT) && defined(CONFIG_PROC_FS)*/
#endif

#ifdef KEYPAD_POWER_SUPPORT
static inline void kpadPower_timeout(unsigned long fcontext)
{

	struct input_dev *dev = (struct input_dev *) fcontext;
	//printk("-------------------------> kpadPower time out\n");
	time2 = jiffies_to_msecs(jiffies);
	if ((time2 - time1) > 2000 && sync_counter > 4) { //2000 msec
		schedule_work(&PMC_sync);
		//DPRINTK("1[%s]dannier count=%d jiffies=%lu, %d\n",__func__, sync_counter, jiffies,jiffies_to_msecs(jiffies));
	} //else
		//DPRINTK("0[%s]dannier count=%d jiffies=%lu, %d\n",__func__, sync_counter, jiffies,jiffies_to_msecs(jiffies));
	DPRINTK(KERN_ALERT "\n[%s]kpadPower time out GPIO_ID_GPIO_VAL = %x\n",__func__,GPIO_ID_GPIO_VAL);
	if(!kpadPower_dev)
		return;

	spin_lock_irq(&kpadPower_lock);

	if(!(PMPB_VAL & BIT24)) {
		input_report_key(dev, KEY_POWER, 0); //power key is released
		input_sync(dev);
		powerKey_is_pressed = 0;
		wmt_pwrbtn_debounce_value(power_on_debounce_value);
		DPRINTK("[%s]power key released\n",__func__);
		sync_counter = 0;
	}else {
		DPRINTK("[%s]power key not released\n",__func__);
		mod_timer(&kpadPower_timer, jiffies + power_button_timeout);
		sync_counter++;
	}

	spin_unlock_irq(&kpadPower_lock);

}
#endif

static int wmt_wakeup_pm_notify(struct notifier_block *nb, unsigned long event,
	void *dummy)
{
	unsigned long	pm_lock_flags;
	
	spin_lock_irqsave(&wmt_pm_lock, pm_lock_flags);	
	if (event == PM_SUSPEND_PREPARE) {
		dynamic_pmc_intr = INT_TRG_EN_VAL;
		do {
			INT_TRG_EN_VAL = 0;
		} while (INT_TRG_EN_VAL != 0);	
				
		PMWS_VAL = PMWS_VAL;
				
		do {
			WK_TRG_EN_VAL = (Wake_up_sts_mask | dynamic_wakeup);
		} while (WK_TRG_EN_VAL != (Wake_up_sts_mask | dynamic_wakeup));		
		
	} else if (event == PM_POST_SUSPEND) {		
		do {
			INT_TRG_EN_VAL = dynamic_pmc_intr;
		} while (INT_TRG_EN_VAL != dynamic_pmc_intr);
		
		do {
			WK_TRG_EN_VAL = 0x4000;
		} while (WK_TRG_EN_VAL != 0x4000);				
		
	}
	spin_unlock_irqrestore(&wmt_pm_lock, pm_lock_flags);
	return NOTIFY_OK;
}

static struct notifier_block wmt_pmc_pm_notify = {
	.notifier_call = wmt_wakeup_pm_notify,
};

/*
 * Initialize power management interface
 */
static int __init wmt_pm_init(void)
{
	struct apm_dev_s *pm_dev;
	char buf[256];
	char varname[] = "wmt.pwbn.param";
	int varlen = 256;
	char wake_buf[100];
	char wake_varname[] = "wmt.pmc.param";
	int wake_varlen = 100;
#ifdef CONFIG_BATTERY_WMT
	char bat_varname[] = "wmt.io.bat";
#endif

#ifdef KEYPAD_POWER_SUPPORT
	int i;

	kpadPower_dev = input_allocate_device();
	if(kpadPower_dev) {
		//Initial the static variable
		spin_lock_init(&kpadPower_lock);
		powerKey_is_pressed = 0;
		pressed_jiffies = 0;
		init_timer(&kpadPower_timer);
		kpadPower_timer.function = kpadPower_timeout;
		kpadPower_timer.data = (unsigned long)kpadPower_dev;

		/* Register an input event device. */
		set_bit(EV_KEY,kpadPower_dev->evbit);
		for (i = 0; i < KPAD_POWER_FUNCTION_NUM; i++)
		set_bit(kpadPower_codes[i], kpadPower_dev->keybit);

		kpadPower_dev->name = "kpadPower",
		kpadPower_dev->phys = "kpadPower",


		kpadPower_dev->keycode = kpadPower_codes;
		kpadPower_dev->keycodesize = sizeof(unsigned int);
		kpadPower_dev->keycodemax = KPAD_POWER_FUNCTION_NUM;

		/*
		* For better view of /proc/bus/input/devices
		*/
		kpadPower_dev->id.bustype = 0;
		kpadPower_dev->id.vendor  = 0;
		kpadPower_dev->id.product = 0;
		kpadPower_dev->id.version = 0;
		input_register_device(kpadPower_dev);
	} else
		printk("[wmt_pm_init]Error: No memory for registering Kpad Power\n");
#endif

	register_pm_notifier(&wmt_pmc_pm_notify);

//gri
	if (! pmlock_1st_flag) {
	  pmlock_1st_flag = 1;
	  spin_lock_init(&wmt_pm_lock);
	}
	if (! pmlock_intr_1st_flag) {
	  pmlock_intr_1st_flag = 1;
	  spin_lock_init(&wmt_pm_intr_lock);
	}
	
	if (wmt_getsyspara(wake_varname, wake_buf, &wake_varlen) == 0) {
		sscanf(wake_buf,"%x:%x:%x:%x:%x",
						&var_wake_en,
						&var_wake_param,
						&var_wake_type,
						&var_wake_type2,
						&var_fake_power_button);		
	}
	else {
		var_wake_en = 1;
		var_wake_param = 0x0040C080;	//cir rtc pwrbtn dcdet(cirin)
		var_wake_type =  0x40000000;
		var_wake_type2 = 0x0;
		var_fake_power_button = 0x00400080;
	}
	
	Wake_up_sts_mask = (var_wake_param | BIT14); // add power button 
	
	var_1st_flag = 1;
	
	printk("[%s] var define var_wake_en=%x var_wake_param=%x var_wake_type=%x var_wake_type2=%x var_fake_power_button=%x\n",
	__func__, var_wake_en, var_wake_param, var_wake_type, var_wake_type2, var_fake_power_button);

#if 0//test pmc_enable_wakeup_event
	do {
		WK_EVT_TYPE_VAL = 0x33333333;
	}	while (WK_EVT_TYPE_VAL != 0x33333333);
	
	{
		unsigned int i_temp, en_temp, en_type;
		i_temp = 0;
		en_type = 0;
		do {
			en_temp = (var_wake_param & (1 << i_temp));
			if (en_temp) {
				if (en_temp < 0x100)
					en_type = ((var_wake_type >> (i_temp << 2)) & 0xf);
				else if(en_temp >= 0x1000000)
					en_type = ((var_wake_type2 >> ((i_temp - 24) << 2)) & 0xf);
				else if(en_temp == 0x200000)
					en_type = ((var_wake_type >> 24) & 0xf);
				else
					en_type = 3;
				printk("en_temp 0x%x en_type 0x%x\n",i_temp,en_type);
				pmc_enable_wakeup_event(i_temp, en_type);
			}
			i_temp++;
		} while (i_temp < 32);
	}
#else
	{
		unsigned int i_temp;
		do {
			PMWT_VAL = var_wake_type;
		}	while(PMWT_VAL != var_wake_type);
		do {
			PMWTC_VAL = var_wake_type2;
		}	while(PMWTC_VAL != var_wake_type2);	
		
		if (var_wake_param & BIT21)
			i_temp = 0x33433333;
		else
			i_temp = 0x33333333;
		do {
			WK_EVT_TYPE_VAL = i_temp;				
		}	while(WK_EVT_TYPE_VAL != i_temp);		
	}
#endif	

	printk("[%s] WK_TRG_EN_VAL=0x%x PMWT_VAL=0x%x PMWTC_VAL=0x%x WK_EVT_TYPE_VAL=0x%x\n",
	__func__, WK_TRG_EN_VAL, PMWT_VAL, PMWTC_VAL, WK_EVT_TYPE_VAL);

#ifdef CONFIG_BATTERY_WMT
	if (wmt_getsyspara(bat_varname, buf, &varlen) == 0) {
		sscanf(buf,"%x", &battery_used);
	}
	printk("[%s] battery_used = %x\n",__func__, battery_used);
#endif

#ifdef CONFIG_CACHE_L2X0
	if(wmt_getsyspara("wmt.l2c.param",buf,&varlen) == 0)
		sscanf(buf,"%d:%x:%x:%d:%x:%x",&l2x0_onoff, &l2x0_aux, &l2x0_prefetch_ctrl, &en_static_address_filtering, &address_filtering_start, &address_filtering_end);
        if( l2x0_onoff == 1)
                l2x0_base = (volatile unsigned int) ioremap(0xD9000000, SZ_4K);

	if (wmt_getsyspara("wmt.secure.param",buf,&varlen) == 0)
		sscanf(buf,"%d",&cpu_trustzone_enabled);
	if(cpu_trustzone_enabled != 1)
		cpu_trustzone_enabled = 0;
#endif

	/* Press power button (either hard-power or soft-power) will trigger a power button wakeup interrupt*/
	/* Press reset button will not trigger any PMC wakeup interrupt*/
	/* Hence, force write clear all PMC wakeup interrupts before request PMC wakeup IRQ*/

	//move to wload
	//PMWS_VAL = PMWS_VAL;
	//PMCIS_VAL = PMCIS_VAL;

	INIT_WORK(&PMC_shutdown, run_shutdown);
	INIT_WORK(&PMC_sync, run_sync);

	/*
	 *  set interrupt service routine
	 */
//	if (request_irq(IRQ_PMC_WAKEUP, &pmc_wakeup_isr,  IRQF_DISABLED, "pmc", &pm_dev) < 0)
	if (request_irq(IRQ_PMC_WAKEUP, &pmc_wakeup_isr, IRQF_SHARED, "pmc", &pm_dev) < 0)
		printk(KERN_ALERT "%s: [Wondermedia_pm_init] Failed to register pmc wakeup irq \n"
               , DRIVER_NAME);
			   
	 irq_set_irq_wake(IRQ_PMC_WAKEUP, 1);
	 
#ifndef CONFIG_SKIP_DRIVER_MSG
	/*
	 * Plan to remove it to recude core size in the future.
	 */
	printk(KERN_INFO "%s: WonderMedia Power Management driver\n", DRIVER_NAME);
#endif

	/*
	 * Setup PM core driver into kernel.
	 */
	suspend_set_ops(&wmt_pm_ops);

#if defined(SOFT_POWER_SUPPORT) && defined(CONFIG_PROC_FS)

	/* Power button is configured as soft power*/
	printk("%s: Power button is configured as soft power\n", DRIVER_NAME);
	PMPB_VAL |= PMPB_SOFTPWR;

#if 0
	/* Create proc entry*/
	proc_softpower = create_proc_entry("softpower", 0644, &proc_root);
	proc_softpower->data = &softpower_data;
	proc_softpower->read_proc = procfile_read;
	proc_softpower->write_proc =  procfile_write;
#endif 

#else
	/* Power button is configured as hard power*/
	printk("%s: Power button is configured as hard power\n", DRIVER_NAME);
	PMPB_VAL = 0;

#endif	/* defined(SOFT_POWER_SUPPORT) && defined(CONFIG_PROC_FS)*/

		/*read power button debounce value*/
	if (wmt_getsyspara(varname, buf, &varlen) == 0){
			sscanf(buf,"%d:%d:%d",
						&power_on_debounce_value,
						&resume_debounce_value,
						&power_up_debounce_value);

		if (power_on_debounce_value < min_debounce_value)
			power_on_debounce_value = min_debounce_value;
		if (power_on_debounce_value > max_debounce_value)
			power_on_debounce_value = max_debounce_value;

		if (resume_debounce_value < min_debounce_value)
			resume_debounce_value = min_debounce_value;
		if (resume_debounce_value > max_debounce_value)
			resume_debounce_value = max_debounce_value;

		if (power_up_debounce_value < min_debounce_value)
			power_up_debounce_value = min_debounce_value;
		if (power_up_debounce_value > max_debounce_value)
			power_up_debounce_value = max_debounce_value;
	}


	/*set power button debounce value*/
	printk("[%s] power_on = %d resume = %d power_up = %d\n",
	__func__,power_on_debounce_value, resume_debounce_value, power_up_debounce_value);
	wmt_pwrbtn_debounce_value(power_on_debounce_value);

	wmt_ws = wakeup_source_register("wmt_pwbn");

	//enable power button intr
	pmc_enable_wakeup_isr(WKS_PWRBTN, 0);

	memset(buf ,0, sizeof(buf));
	varlen = sizeof(buf);
	if ((wmt_getsyspara("wmt.gpo.hall_switch", buf, &varlen) == 0)) {
		int ret = sscanf(buf, "%d:%d",
					   &hall_switch.gpio_no,
					   &hall_switch.wakeup_source
				       );
		
		//printk(KERN_ERR"hall_switch gpiono:%d ws:%d\n",hall_switch.gpio_no,hall_switch.wakeup_source);
		ret = gpio_request(hall_switch.gpio_no, "hall_switch"); 
		if(ret < 0) {
			printk(KERN_ERR"gpio request fail for hall_switch\n");
			goto hswitch_done;
		}
		
		gpio_direction_input(hall_switch.gpio_no);
		wmt_gpio_setpull(hall_switch.gpio_no, WMT_GPIO_PULL_UP);
	
		pmc_enable_wakeup_isr(hall_switch.wakeup_source,4);
		wakeup_queue=create_workqueue("wakeup0");
		INIT_DELAYED_WORK(&wakeupwork, wakeup_func);
		hall_switch.have_switch = 1;
	}

hswitch_done:

	
	
	return 0;
}

late_initcall(wmt_pm_init);
