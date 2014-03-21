/*++
linux/drivers/input/keyboard/wmt_kpad.c

Some descriptions of such software. Copyright (c) 2008  WonderMedia Technologies, Inc.

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
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/types.h>
#include <linux/input.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/cpufreq.h>
#include <linux/delay.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/errno.h>
#include <asm/mach-types.h>
#include <mach/hardware.h>
#include <linux/suspend.h>


/*=============================================================================
//
// WM3498 SARADC control registers.
//
// Registers Abbreviations:
//
// ADCCtrl0_REG  ADC Control0 Register.
//
// ADCCtrl1_REG  ADC Control1 Register.
//
// ADCCTRL2_REG  ADC Control2 Register.
//
//=============================================================================*/
/*=============================================================================
//
// Address constant for each register.
//
//============================================================================*/
#define ADCCtl0_ADDR	(ADC_BASE_ADDR + 0x00)
#define ADCCtl1_ADDR	(ADC_BASE_ADDR + 0x04)
#define ADCCtl2_ADDR	(ADC_BASE_ADDR + 0x08)

/*=============================================================================
//
// Register pointer.
//
//=============================================================================*/
#define ADCCtl0_REG		(REG32_PTR(ADCCtl0_ADDR))
#define ADCCtl1_REG		(REG32_PTR(ADCCtl1_ADDR))
#define ADCCtl2_REG		(REG32_PTR(ADCCtl2_ADDR))

/*=============================================================================
//
// Register value.
//
//=============================================================================*/
#define ADCCtl0_VAL		(REG32_VAL(ADCCtl0_ADDR))
#define ADCCtl1_VAL		(REG32_VAL(ADCCtl1_ADDR))
#define ADCCtl2_VAL		(REG32_VAL(ADCCtl2_ADDR))

/*=============================================================================
//
// ADCCtl0_REG  ADC Control0 Register.
//
//=============================================================================*/
#define TOutDlyMask         0xFFFF          /* Time Out Interrupt Delay Value */
#define TOutDly(x)          ((x) & TOutDlyMask)
#define ClrIntTOut          BIT16           /* ADC Timeout Interrupt CLEAR signal */
#define ClrIntADC           BIT17           /* ADC sample point Conversion Finished Interrupt CLEAR signal */
#define EndcIntEn           BIT18           /* ADC Conversion Finished Interrupt Enable. */
#define TOutEn              BIT19           /* Time Out Interrupt Enable. */
#define TempEn              BIT20           /* Manual output valid. */
#define AutoMode            BIT21           /* Auto mode select. */
#define SDataSel            BIT22           /* Serial DATA select. */
#define DigClkEn            BIT23           /* Digital clock enable. */
#define StartEn             BIT24           /* A/D conversion starts by enable. */
#define DBNSMASK            0x00000007      /* DBNS_SIZE bits can detect certain MSB bits comparison. */
#define DbnsSize(x)         (((x) >>  26) & DBNSMASK)
#define PMSel               BIT29           /* SAR ADC works in Power Mode. */
#define PD                  BIT30           /* SAR ADC Power */
#define TestMode            BIT31           /* SAR ADC Test Mode */

/*=============================================================================
//
// ADCCtrl1_REG  ADC Control1 Register.
//
//=============================================================================*/
#define SARCodeMask         0x1FF           /* SARADC data output. */
#define SARCode(x)          ((x) & SARCodeMask)
#define BufRd               BIT13           /* Buffer read signal */
#define ValDetIntEn         BIT14           /* ADC value changing detection INTERRUPT ENABLE */
#define ClrIntValDet        BIT15           /* ADC value changing detection Interrupt CLEAR signal */
#define AutoTmpSlotMask     0xFFFF          /* This value controls SAMPLE RATE */
#define AutoTmpSlot(x)      (((x) >> 16) & AutoTmpSlotMask)

/*=============================================================================
//
// ADCCtrl2_REG  ADC Control1 Register.
//
//=============================================================================*/
#define SARCodeVld          BIT0            /* SARADC valid signal. */
#define BufEmpty            BIT8            /* Buffer empty signal */
#define TestDataMask        0x7F            /* Test DATA */
#define TestData(x)         (((x) >> 9) & TestDataMask)
#define EndcIntStatus       BIT16           /* ADC Conversion Finished Interrupt status */
#define BufDataMask         0x1FF           /* Buffer read data (changed value saved in buffer) */
#define BufData(x)	        (((x) >> 17) & BufDataMask)
#define TOutStatus          BIT26           /* Time Out interrupt status */
#define ValDetIntStatus     BIT27           /* Value changing detection interrupt status */


/*
 * Saradc register set structure
 */
struct saradc_regs_s {
	unsigned int volatile Ctr0;
	unsigned int volatile Ctr1;
	unsigned int volatile Ctr2;
};


/*
 * wmt keypad operation structure.
 */
struct wmt_saradc_s {
	/* Module reference counter */
	unsigned int ref;

	/* I/O Resource */
	struct resource *res;

	/* Saradc I/O register set. */
	struct saradc_regs_s *regs;

	/* Interrupt number and status counters. */
	unsigned int irq;

};

extern int wmt_getsyspara(char *varname, unsigned char *varval, int *varlen);
extern unsigned int wmt_read_oscr(void);

MODULE_AUTHOR("WonderMedia Technologies, Inc.");
MODULE_DESCRIPTION("WMT [generic keypad] driver");
MODULE_LICENSE("GPL");
