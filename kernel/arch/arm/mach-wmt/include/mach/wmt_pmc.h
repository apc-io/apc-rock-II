/*++
linux/include/asm-arm/arch-wmt/wmt_pmc.h

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

/* Be sure that virtual mapping is defined right */
#ifndef __ASM_ARCH_HARDWARE_H
#error "You must include hardware.h, not vt8500_pmc.h"
#endif

#ifndef __VT8500_PMC_H
#define __VT8500_PMC_H

/******************************************************************************
 *
 * Define the register access macros.
 *
 * Note: Current policy in standalone program is using register as a pointer.
 *
 ******************************************************************************/
#include "wmt_mmap.h"

/******************************************************************************
 *
 * VT8500 Power Management Controller Base Address.
 *
 ******************************************************************************/
#ifdef __PMC_BASE
#error "__RTC_BASE has already been defined in another file."
#endif
#ifdef PM_CTRL_BASE_ADDR               /* From vt8500.mmap.h */
#define __PMC_BASE      PM_CTRL_BASE_ADDR
#else
#define __PMC_BASE      0xD8130000      /* 64K */
#endif

/******************************************************************************
 *
 * VT8500 Power Management (PM) control registers.
 *
 * Registers Abbreviations:
 *
 * PMCS_REG     	PM (Current) Status Register.
 *
 * PMIR_REG     	PM Idle processor Request Register.
 *
 * PMTC_REG     	PM power-up Time Control Register.
 *
 * PMHV_REG     	PM Hibernation Value Register.
 *
 * PMHC_REG     	PM Hibernation Control Register.
 *
 * PMWS_REG     	PM Wake-up Status register.
 *
 * PMWE_REG     	PM Wake-up event Enable Register.
 *
 * PMWT_REG     	PM Wake-up event Type Register.
 *
 * HSP0_REG     	PM Hibernation Scratch Pad Register 0
 *
 * HSP1_REG     	PM Hibernation Scratch Pad Register 1
 *
 * HSP2_REG     	PM Hibernation Scratch Pad Register 2
 *
 * HSP3_REG     	PM Hibernation Scratch Pad Register 3
 *
 * PMRS_REG     	PM Reset Status Register.
 *
 * PMPB_REG			PM Button Control Register
 *
 * PMSR_REG     	PM Software Reset request Register.
 *
 * PMPATA_REG     	PM PATA I/Os Drive strength Register
 *
 * OSM0_REG     	OS Timer Match Register 0
 *
 * OSM1_REG     	OS Timer Match Register 1
 *
 * OSM2_REG     	OS Timer Match Register 2
 *
 * OSM3_REG     	OS Timer Match Register 3
 *
 * OSCR_REG     	OS Timer Count Register.
 *
 * OSTS_REG     	OS Timer Status Register.
 *
 * OSTW_REG     	OS Timer Watchdog enable Register.
 *
 * OSTI_REG     	OS Timer Interrupt enable Register.
 *
 * OSTC_REG     	OS Timer Control Register.
 *
 * OSTA_REG     	OS Timer Access status Register.
 *
 * PMMISC_REG    	PM miscellaneous (Peripherals) Clock Control Register.
 *
 * PMPMA_REG    	PM PLL_A Multiplier and range values Register.
 *
 * PMPMB_REG    	PM PLL_B Multiplier and range values Register.
 *
 * PMPMC_REG    	PM PLL_C Multiplier and range values Register.
 *
 * PMPMD_REG    	PM PLL_D Multiplier and range values Register.
 *
 * PMCEL_REG	  	PM	Clock Enables Lower Register
 *
 * PMCEU_REG	 	PM	Clock Enables Upper Register
 *
 * PMZD_REG    		PM ZAC2_MA clock's "P" Divisor value Register.
 *
 * PMZH_REG    		PM ZAC2_MA clock's High pulse is the wide pulse Register.
 *
 * PMAD_REG    		PM AHB clock's "A" Divisor value Register.
 *
 * PMMC_REG		 	PM DDR Memory Control Clock Divisor Register
 *
 * PMSF_REG    		PM Serial Flash controller clock's Divisor value Register.
 *
 * PMSFH_REG   		PM Serial flash controller clock's High pulse is the wide
 *             		pulse Register.
 *
 * PMCC_REG    		PM Compact flash clock Control
 *
 * PMCCH_REG   		PM Compact flash controller clock's High pulse is the wide
 *
 * PMSDMMC_REG 		PM SD/MMC clock Control
 *
 * PMSDMMCH_REG   	PM SD/MMC controller clock's High pulse is the wide
 *
 * PMMS_REG    		PM MS&MS-pro clock Control
 *
 * PMMSH_REG   		PM MS&MS-pro controller clock's High pulse is the wide
 *
 * PMNAND_REG  		PM nand clock Control
 *
 * PMNANDH_REG 		PM nand controller clock's High pulse is the wide
 *
 * PMLPC_REG    	PM LPC memory clock Control
 *
 * PMLPCH_REG  		PM LPC memory controller clock's High pulse is the wide
 *
 * PMSPI_REG    	PM SPI clock Control
 *
 * PMSPIH_REG  		PM SPI controller clock's High pulse is the wide
 *
 ******************************************************************************/
/******************************************************************************
 *
 * Address constant for each register.
 *
 ******************************************************************************/
#define PMCS_ADDR       (__PMC_BASE + 0x0000)
#define PMCSH_ADDR      (__PMC_BASE + 0x0004)
#define PMIR_ADDR       (__PMC_BASE + 0x0008)
#define PMTC_ADDR       (__PMC_BASE + 0x000C)
#define PMHV_ADDR       (__PMC_BASE + 0x0010)
#define PMHC_ADDR       (__PMC_BASE + 0x0012)
#define PMWS_ADDR       (__PMC_BASE + 0x0014)
#define PMCS2_ADDR			(__PMC_BASE + 0x0018)
#define PMWE_ADDR       (__PMC_BASE + 0x001C)
#define PMWT_ADDR       (__PMC_BASE + 0x0020)
#define PMWTC_ADDR      (__PMC_BASE + 0x0024)

#define PMCWS_ADDR      (__PMC_BASE + 0x0028)       /* Card_UDC wakeup status */
#define PMCAD_ADDR      (__PMC_BASE + 0x002C)       /* Card attach debounce control */

#define HSP0_ADDR       (__PMC_BASE + 0x0030)
#define HSP1_ADDR       (__PMC_BASE + 0x0034)
#define HSP2_ADDR       (__PMC_BASE + 0x0038)
#define HSP3_ADDR       (__PMC_BASE + 0x003C)
#define HSP4_ADDR       (__PMC_BASE + 0x0040)
#define HSP5_ADDR       (__PMC_BASE + 0x0044)
#define HSP6_ADDR       (__PMC_BASE + 0x0048)
#define HSP7_ADDR       (__PMC_BASE + 0x004C)
#define PMRS_ADDR       (__PMC_BASE + 0x0050)
#define PMPB_ADDR       (__PMC_BASE + 0x0054)
#define PMAXILPI_ADDR   (__PMC_BASE + 0x0058)
#define DCDET_STS_ADDR  (__PMC_BASE + 0x005C)
#define PMSR_ADDR       (__PMC_BASE + 0x0060)
#define TIOUT_RST_ADDR  (__PMC_BASE + 0x0064)
#define BROM_PD_ADDR	  (__PMC_BASE + 0x0068)
#define CA9MP_RSTC_ADDR (__PMC_BASE + 0x006C)
#define CA9MP_RSTS_ADDR (__PMC_BASE + 0x0070)

#define PMCIS_ADDR      (__PMC_BASE + 0x0074)      /* Interrupt status from wakeup source */
#define PMCIE_ADDR      (__PMC_BASE + 0x007C)      /* Interrupt enable from wakeup source */
#define INT_TYPE0_ADDR    (__PMC_BASE + 0x0080)
#define INT_TYPE1_ADDR    (__PMC_BASE + 0x0084)
#define INT_TYPE2_ADDR    (__PMC_BASE + 0x0088)


#define RST_VECT_MAP_ADDR (__PMC_BASE + 0x0090)      /* USB OTG operation mode select */
#define RTCCM_ADDR      	(__PMC_BASE + 0x0094)      /* RTC Clock Exist Monitor */
#define PMSTM_ADDR      	(__PMC_BASE + 0x0098)      /* Suspend to DRAM */
#define WK_EVT_TYPE_ADDR	(__PMC_BASE + 0x00A0)      /* WAKE UP EVENT TYPE */
#define WK_TRG_EN_ADDR		(__PMC_BASE + 0x00B0)
#define INT_TRG_EN_ADDR		(__PMC_BASE + 0x00B4)
#define CA9MPC0_ADDR			(__PMC_BASE + 0x00C0)
#define CA9MPC1_ADDR			(__PMC_BASE + 0x00C4)

#define PWRUP_SRC_ADDR			(__PMC_BASE + 0x00D0)

#define OSM4_ADDR       (__PMC_BASE + 0x00F0)
#define OSM5_ADDR       (__PMC_BASE + 0x00F4)
#define OSM6_ADDR       (__PMC_BASE + 0x00F8)
#define OSM7_ADDR       (__PMC_BASE + 0x00FC)
#define OSM0_ADDR       (__PMC_BASE + 0x0100)
#define OSM1_ADDR       (__PMC_BASE + 0x0104)
#define OSM2_ADDR       (__PMC_BASE + 0x0108)
#define OSM3_ADDR       (__PMC_BASE + 0x010C)
#define OSCR_ADDR       (__PMC_BASE + 0x0110)
#define OSTS_ADDR       (__PMC_BASE + 0x0114)
#define OSTW_ADDR       (__PMC_BASE + 0x0118)
#define OSTI_ADDR       (__PMC_BASE + 0x011C)
#define OSTC_ADDR       (__PMC_BASE + 0x0120)
#define OSTA_ADDR       (__PMC_BASE + 0x0124)

#define PMMISC_ADDR     (__PMC_BASE + 0x01FC)

#define PMPMA_ADDR      (__PMC_BASE + 0x0200)
#define PMPMB_ADDR      (__PMC_BASE + 0x0204)
#define PMPMC_ADDR      (__PMC_BASE + 0x0208)
#define PMPMD_ADDR      (__PMC_BASE + 0x020C)
#define PMPME_ADDR      (__PMC_BASE + 0x0210)
#define PMPMF_ADDR      (__PMC_BASE + 0x0214)     /* PLL Audio(I2S) Control Register */
#define PMPMG_ADDR      (__PMC_BASE + 0x0218)

#define PMCEL_ADDR      (__PMC_BASE + 0x0250)
#define PMCEU_ADDR      (__PMC_BASE + 0x0254)
#define PMCE2_ADDR      (__PMC_BASE + 0x0258)
#define PMCE3_ADDR      (__PMC_BASE + 0x025C)
#define DVFSSTS_ADDR    (__PMC_BASE + 0x0260)
#define DVFSE0_ADDR     (__PMC_BASE + 0x0280)
#define DVFSE1_ADDR     (__PMC_BASE + 0x0284)
#define DVFSE2_ADDR     (__PMC_BASE + 0x0288)
#define DVFSE3_ADDR     (__PMC_BASE + 0x028C)
#define DVFSE4_ADDR     (__PMC_BASE + 0x0290)
#define DVFSE5_ADDR     (__PMC_BASE + 0x0294)
#define DVFSE6_ADDR     (__PMC_BASE + 0x0298)
#define DVFSE7_ADDR     (__PMC_BASE + 0x029C)
#define DVFSE8_ADDR     (__PMC_BASE + 0x02A0)
#define DVFSE9_ADDR     (__PMC_BASE + 0x02A4)
#define DVFSE10_ADDR    (__PMC_BASE + 0x02A8)
#define DVFSE11_ADDR    (__PMC_BASE + 0x02AC)
#define DVFSE12_ADDR    (__PMC_BASE + 0x02B0)
#define DVFSE13_ADDR    (__PMC_BASE + 0x02B4)
#define DVFSE14_ADDR    (__PMC_BASE + 0x02B8)
#define DVFSE15_ADDR    (__PMC_BASE + 0x02BC)

#define PMARM_ADDR      (__PMC_BASE + 0x0300)			/* ARM */
#define PMARMH_ADDR     (__PMC_BASE + 0x0301)
#define PMAHB_ADDR      (__PMC_BASE + 0x0304)			/* AHB */
#define PML2C_ADDR      (__PMC_BASE + 0x030C)			/* L2C */
#define PML2CH_ADDR     (__PMC_BASE + 0x030D)
#define PMMC_ADDR       (__PMC_BASE + 0x0310)

#define PMSF_ADDR       (__PMC_BASE + 0x0314)     /* SF */
#define PMSFH_ADDR      (__PMC_BASE + 0x0315)
#define PMNAND_ADDR     (__PMC_BASE + 0x0318)     /* NAND */
#define PMNANDH_ADDR    (__PMC_BASE + 0x0319)
#define PMNOR_ADDR 	    (__PMC_BASE + 0x031C)     /* NOR */
#define PMNORH_ADDR	    (__PMC_BASE + 0x031D)
#define PMAPB0_ADDR	    (__PMC_BASE + 0x0320)     /* APB 0 */
#define PMAPB0H_ADDR 	  (__PMC_BASE + 0x0321)
#define PMPCM0_ADDR	    (__PMC_BASE + 0x0324)     /* PCM 0 */
#define PMPCM1_ADDR	    (__PMC_BASE + 0x0328)     /* PCM 1 */
#define PMSDMMC_ADDR    (__PMC_BASE + 0x0330)     /* SD/MMC 0 */
#define PMSDMMCH_ADDR   (__PMC_BASE + 0x0331)
#define PMSDMMC1_ADDR   (__PMC_BASE + 0x0334)    	/* SD/MMC 1 */
#define PMSDMMC1H_ADDR  (__PMC_BASE + 0x0335)
#define PMSDMMC2_ADDR   (__PMC_BASE + 0x0338)    	/* SD/MMC 2 */
#define PMSDMMC2H_ADDR  (__PMC_BASE + 0x0339)
#define PMSDMMC3_ADDR   (__PMC_BASE + 0x033C)    	/* SD/MMC 3 */
#define PMSDMMC3H_ADDR  (__PMC_BASE + 0x033D)                   
#define PMSPI_ADDR      (__PMC_BASE + 0x0340)     /* SPI 0 */
#define PMSPIH_ADDR	    (__PMC_BASE + 0x0341)
#define PMSPI1_ADDR     (__PMC_BASE + 0x0344)     /* SPI 1 */
#define PMSPI1H_ADDR	  (__PMC_BASE + 0x0345)
#define PMSE_ADDR  		  (__PMC_BASE + 0x0348)     /* SE */
#define PMSEH_ADDR		  (__PMC_BASE + 0x0349)
#define PMPWM_ADDR      (__PMC_BASE + 0x0350)     /* PWM */
#define PMPWMH_ADDR	    (__PMC_BASE + 0x0351)
#define PMPAXI_ADDR     (__PMC_BASE + 0x0354)     /* PAXI */
#define PMPAXIH_ADDR	  (__PMC_BASE + 0x0355)
#define PMWMTNA_ADDR	  (__PMC_BASE + 0x0358)     /* NA 01 */
#define PMWMTNAH_ADDR	  (__PMC_BASE + 0x0359)
#define PMNA12_ADDR	    (__PMC_BASE + 0x035C)     /* NA 12 */
#define PMNA12H_ADDR	  (__PMC_BASE + 0x035D)
#define PMCNMNA_ADDR    (__PMC_BASE + 0x0360)     /* CNM NA */
#define PMCNMNAH_ADDR   (__PMC_BASE + 0x0361)
#define PMWMTVDU_ADDR	  (__PMC_BASE + 0x0368)     /* WMT VDU */
#define PMWMTVDUH_ADDR	(__PMC_BASE + 0x0369)
#define PMHDMITV_ADDR	  (__PMC_BASE + 0x036C)     /* HDMITV */
#define PMHDMITVH_ADDR  (__PMC_BASE + 0x036D)
#define PMDVO_ADDR  	  (__PMC_BASE + 0x0370)     /* DVO */
#define PMDVOH_ADDR     (__PMC_BASE + 0x0371)
#define PMAUDIO_ADDR		(__PMC_BASE + 0x0374) 		/* AUDIO/I2S */
#define PMAUDIOH_ADDR	  (__PMC_BASE + 0x0375)
#define PMCSI0_ADDR			(__PMC_BASE + 0x0380) 		/* CSI0 */
#define PMCSI0H_ADDR	  (__PMC_BASE + 0x0381)
#define PMCSI1_ADDR			(__PMC_BASE + 0x0384) 		/* CSI1 */
#define PMCSI1H_ADDR	  (__PMC_BASE + 0x0385)
#define PMMALI_ADDR	 		(__PMC_BASE + 0x0388)     /* MALI */
#define PMMALIH_ADDR	  (__PMC_BASE + 0x0389)
#define PMCNMVDU_ADDR		(__PMC_BASE + 0x038C)     /* CNM VDU */
#define PMCNMVDUH_ADDR  (__PMC_BASE + 0x038D)
#define PMHDI2C_ADDR    (__PMC_BASE + 0x0390)     /* HDMII2C */
#define PMHDI2CH_ADDR	  (__PMC_BASE + 0x0391)
#define PMADC_ADDR		  (__PMC_BASE + 0x0394)     /* ADC */
#define PMADCH_ADDR		  (__PMC_BASE + 0x0395)
#define PMI2C4_ADDR	    (__PMC_BASE + 0x039C)     /* I2C 4 */
#define PMI2C4H_ADDR	  (__PMC_BASE + 0x039D)
#define PMI2C0_ADDR	    (__PMC_BASE + 0x03A0)     /* I2C 0 */
#define PMI2C0H_ADDR	  (__PMC_BASE + 0x03A1)
#define PMI2C1_ADDR	    (__PMC_BASE + 0x03A4)     /* I2C 1 */
#define PMI2C1H_ADDR	  (__PMC_BASE + 0x03A5)
#define PMI2C2_ADDR	    (__PMC_BASE + 0x03A8)     /* I2C 2 */
#define PMI2C2H_ADDR	  (__PMC_BASE + 0x03A9)
#define PMI2C3_ADDR	    (__PMC_BASE + 0x03AC)     /* I2C 3 */
#define PMI2C3H_ADDR	  (__PMC_BASE + 0x03AD)
#define PML2CAXI_ADDR	  (__PMC_BASE + 0x03B0)     /* L2C AXI*/
#define PML2CAXIH_ADDR	(__PMC_BASE + 0x03B1)
#define PMATCLK_ADDR	  (__PMC_BASE + 0x03B4)     /* AT CLK*/
#define PMATCLKH_ADDR		(__PMC_BASE + 0x03B5)
#define PMPERI_ADDR			(__PMC_BASE + 0x03B8)     /* PERI CLK*/
#define PMPERIH_ADDR		(__PMC_BASE + 0x03B9)
#define PMTRACE_ADDR		(__PMC_BASE + 0x03BC)     /* TRACE CLK*/
#define PMTRACEH_ADDR		(__PMC_BASE + 0x03BD)
#define PMDBGAPB_ADDR		(__PMC_BASE + 0x03D0)     /* DBG APB*/
#define PMDBGAPBH_ADDR 	(__PMC_BASE + 0x03D1)
#define PM24MHZ_ADDR  	(__PMC_BASE + 0x03E4)     /* 24MHZ */
#define PM24MHZH_ADDR   (__PMC_BASE + 0x03E5)
#define PML2CTAG_ADDR	  (__PMC_BASE + 0x03F0)     /* L2C TAG */
#define PML2CTAGH_ADDR  (__PMC_BASE + 0x03F1)
#define PML2CDATA_ADDR	(__PMC_BASE + 0x03F4)     /* L2C DATA */
#define PML2CDATAH_ADDR (__PMC_BASE + 0x03F5)
#define PMCA9PMWDOD_ADDR 	(__PMC_BASE + 0x0480)		/* WATCH DOG RESET */
#define PMSDPS_ADDR			 	(__PMC_BASE + 0x0500)		/* SD 0~2 POWER SWITCH */
#define PMMALIGPPWR_ADDR	(__PMC_BASE + 0x0600)		/* MALI GP Power Shut Off Control and Status Register */
#define PMWMTVDUPWR_ADDR	(__PMC_BASE + 0x0604)		/* WMT VDU Power Shut Off Control and Status Register */
#define PMCA9C0PWR_ADDR 	(__PMC_BASE + 0x0608)	  /* CA9 CORE 0 Power Shut Off Control and Status Register */
#define PML2CRAMPWR_ADDR 	(__PMC_BASE + 0x060C)		/* L2CRAM Power Shut Off Control and Status Register */
#define PMNEON0PWR_ADDR 	(__PMC_BASE + 0x0610)	  /* NEON 0 Power Shut Off Control and Status Register */
#define PMCA9C1PWR_ADDR 	(__PMC_BASE + 0x0614)	  /* CA9 CORE 1 Power Shut Off Control and Status Register */
#define PMNEON1PWR_ADDR 	(__PMC_BASE + 0x0618)	  /* NEON 1 Power Shut Off Control and Status Register */
#define PMC_MPWR_ADDR   	(__PMC_BASE + 0x061C)		  /* C&M Power Shut Off Control and Status Register */
#define PMMALIL2CPWR_ADDR	(__PMC_BASE + 0x0620)		/* MALI L2C Power Shut Off Control and Status Register */
#define PMMALIPP0PWR_ADDR	(__PMC_BASE + 0x0624)		/* MALI PP0 Power Shut Off Control and Status Register */
#define PMMALIPP1PWR_ADDR	(__PMC_BASE + 0x0628)		/* MALI PP1 Power Shut Off Control and Status Register */
#define AXI2AHB_ADDR     (__PMC_BASE + 0x0650)		/* AXI TO AHB POWER control */
#define EBMCTS_ADDR     (__PMC_BASE + 0x0700)			/* EBM control and status */
#define EBMINTCTS_ADDR  (__PMC_BASE + 0x0704)     /* EBM interrupt control and status */

/******************************************************************************
 *
 * Register pointer.
 *
 ******************************************************************************/
#define PMCS_REG			(REG32_PTR(PMCS_ADDR))/*0x00*/
#define PMCSH_REG			(REG32_PTR(PMCSH_ADDR))/*0x04*/
#define PMIR_REG			(REG8_PTR(PMIR_ADDR))/*0x08*/
#define PMTC_REG			(REG8_PTR(PMTC_ADDR))/*0x0C*/
#define PMHV_REG			(REG16_PTR(PMHV_ADDR))/*0x10*/
#define PMHC_REG			(REG16_PTR(PMHC_ADDR))/*0x12*/
#define PMWS_REG			(REG32_PTR(PMWS_ADDR))/*0x14*/
#define PMCS2_REG			(REG32_PTR(PMCS2_ADDR))/*0x18*/
#define PMWE_REG			(REG32_PTR(PMWE_ADDR))/*0x1C*/
#define PMWT_REG			(REG32_PTR(PMWT_ADDR))/*0x20*/
#define PMWTC_REG			(REG32_PTR(PMWTC_ADDR))/*0x24*/

#define PMCWS_REG			(REG32_PTR(PMCWS_ADDR))/*0x28*/
#define PMCAD_REG			(REG32_PTR(PMCAD_ADDR))/*0x2C*/

#define HSP0_REG			(REG32_PTR(HSP0_ADDR))/*0x30*/
#define HSP1_REG			(REG32_PTR(HSP1_ADDR))/*0x34*/
#define HSP2_REG			(REG32_PTR(HSP2_ADDR))/*0x38*/
#define HSP3_REG			(REG32_PTR(HSP3_ADDR))/*0x3c*/
#define PMRS_REG			(REG32_PTR(PMRS_ADDR))/*0x50*/
#define PMPB_REG			(REG32_PTR(PMPB_ADDR))/*0x54*/
#define DCDET_STS_REG	(REG32_PTR(DCDET_STS_ADDR))/*0x5c*/
#define PMSR_REG			(REG32_PTR(PMSR_ADDR))/*0x60*/
#define TIOUT_RST_REG		(REG32_PTR(TIOUT_RST_ADDR))/*0x64*/
#define BROM_PD_REG			(REG32_PTR(BROM_PD_ADDR))/*0x68*/
#define CA9MP_RSTC_REG	(REG32_PTR(CA9MP_RSTC_ADDR))/*0x6C*/
#define CA9MP_RSTS_REG	(REG32_PTR(CA9MP_RSTS_ADDR))/*0x70*/

#define PMCIS_REG				(REG32_PTR(PMCIS_ADDR))/*0x74*/
#define PMCIE_REG				(REG32_PTR(PMCIE_ADDR))/*0x7C*/
#define INT_TYPE0_REG		(REG32_VAL(INT_TYPE0_ADDR))/*0x80*/
#define INT_TYPE1_REG		(REG32_VAL(INT_TYPE1_ADDR))/*0x84*/
#define INT_TYPE2_REG		(REG32_VAL(INT_TYPE2_ADDR))/*0x88*/

#define RST_VECT_MAP_REG	(REG32_PTR(RST_VECT_MAP_ADDR))/*0x90*/
#define RTCCM_REG			(REG32_PTR(RTCCM_ADDR))/*0x94*/
#define PMSTM_REG			(REG32_PTR(PMSTM_ADDR))/*0x98*/

#define WK_EVT_TYPE_REG	(REG32_PTR(WK_EVT_TYPE_ADDR))/*0xA0*/
#define WK_TRG_EN_REG		(REG32_PTR(WK_TRG_EN_ADDR))/*0xB0*/
#define INT_TRG_EN_REG	(REG32_PTR(INT_TRG_EN_ADDR))/*0xB4*/
#define CA9MPC0_REG			(REG32_PTR(CA9MPC0_ADDR))/*0xC0*/
#define CA9MPC1_REG			(REG32_PTR(CA9MPC1_ADDR))/*0xC4*/
#define PWRUP_SRC_REG		(REG32_PTR(PWRUP_SRC_ADDR))/*0xD0*/


#define OSM4_REG			(REG32_PTR(OSM4_ADDR))/*0xF0*/
#define OSM5_REG			(REG32_PTR(OSM5_ADDR))
#define OSM6_REG			(REG32_PTR(OSM6_ADDR))
#define OSM7_REG			(REG32_PTR(OSM7_ADDR))
#define OSM0_REG			(REG32_PTR(OSM0_ADDR))/*0x100*/
#define OSM1_REG			(REG32_PTR(OSM1_ADDR))
#define OSM2_REG			(REG32_PTR(OSM2_ADDR))
#define OSM3_REG			(REG32_PTR(OSM3_ADDR))
#define OSCR_REG			(REG32_PTR(OSCR_ADDR))
#define OSTS_REG			(REG32_PTR(OSTS_ADDR))
#define OSTW_REG			(REG32_PTR(OSTW_ADDR))
#define OSTI_REG			(REG32_PTR(OSTI_ADDR))
#define OSTC_REG			(REG32_PTR(OSTC_ADDR))
#define OSTA_REG			(REG32_PTR(OSTA_ADDR))/*0x124*/

#define PMMISC_REG		(REG32_PTR(PMMISC_ADDR))/*0x1FC*/
#define PMPMA_REG			(REG32_PTR(PMPMA_ADDR))/*0x200*/
#define PMPMB_REG			(REG32_PTR(PMPMB_ADDR))
#define PMPMC_REG			(REG32_PTR(PMPMC_ADDR))
#define PMPMD_REG			(REG32_PTR(PMPMD_ADDR))
#define PMPME_REG			(REG32_PTR(PMPME_ADDR))
#define PMPMF_REG 		(REG32_PTR(PMPMF_ADDR))
#define PMPMG_REG 		(REG32_PTR(PMPMG_ADDR))

#define PMCEL_REG			(REG32_PTR(PMCEL_ADDR))/*0x250*/
#define PMCEU_REG			(REG32_PTR(PMCEU_ADDR))
#define PMCE2_REG			(REG32_PTR(PMCE2_ADDR))
#define PMCE3_REG			(REG32_PTR(PMCE3_ADDR))
#define DVFSSTS_REG   (REG32_PTR(DVFSSTS_ADDR))/*0x260*/
#define DVFSE0_REG    (REG32_PTR(DVFSE0_ADDR))    
#define DVFSE1_REG    (REG32_PTR(DVFSE1_ADDR))    
#define DVFSE2_REG    (REG32_PTR(DVFSE2_ADDR))    
#define DVFSE3_REG    (REG32_PTR(DVFSE3_ADDR))    
#define DVFSE4_REG    (REG32_PTR(DVFSE4_ADDR))    
#define DVFSE5_REG    (REG32_PTR(DVFSE5_ADDR))    
#define DVFSE6_REG    (REG32_PTR(DVFSE6_ADDR))    
#define DVFSE7_REG    (REG32_PTR(DVFSE7_ADDR))    
#define DVFSE8_REG    (REG32_PTR(DVFSE8_ADDR))    
#define DVFSE9_REG    (REG32_PTR(DVFSE9_ADDR))    
#define DVFSE10_REG   (REG32_PTR(DVFSE10_ADDR))    
#define DVFSE11_REG   (REG32_PTR(DVFSE11_ADDR))    
#define DVFSE12_REG   (REG32_PTR(DVFSE12_ADDR))    
#define DVFSE13_REG   (REG32_PTR(DVFSE13_ADDR))    
#define DVFSE14_REG   (REG32_PTR(DVFSE14_ADDR))    
#define DVFSE15_REG   (REG32_PTR(DVFSE15_ADDR))
    
#define PMARM_REG				(REG8_PTR(PMARM_ADDR))
#define PMARMH_REG			(REG8_PTR(PMARMH_ADDR))
#define PMAHB_REG				(REG8_PTR(PMAHB_ADDR))
#define PMAHBH_REG			(REG8_PTR(PMAHBH_ADDR))
#define PMMC_REG				(REG8_PTR(PMMC_ADDR))
#define PML2C_REG				(REG8_PTR(PML2C_ADDR))
#define PML2CH_REG			(REG8_PTR(PML2CH_ADDR))

#define PMSF_REG				(REG8_PTR(PMSF_ADDR))
#define PMSFH_REG				(REG8_PTR(PMSFH_ADDR))
#define PMAPB1_REG			(REG8_PTR(PMAPB1_ADDR))
#define PMAPB1H_REG			(REG8_PTR(PMAPB1H_ADDR))
#define PMAPB0_REG			(REG8_PTR(PMAPB0_ADDR))
#define PMAPB0H_REG			(REG8_PTR(PMAPB0H_ADDR))
#define PMPCM0_REG			(REG8_PTR(PMPCM0_ADDR))
#define PMPCM1_REG			(REG8_PTR(PMPCM1_ADDR))
#define PMSDMMC_REG			(REG8_PTR(PMSDMMC_ADDR))
#define PMSDMMCH_REG		(REG8_PTR(PMSDMMCH_ADDR))
#define PMMSP_REG				(REG8_PTR(PMMSP_ADDR))
#define PMMSPH_REG			(REG8_PTR(PMMSPH_ADDR))
#define PMNAND_REG			(REG8_PTR(PMNAND_ADDR))
#define PMNANDH_REG			(REG8_PTR(PMNANDH_ADDR))
#define PMXD_REG			(REG8_PTR(PMXD_ADDR))
#define PMXDH_REG			(REG8_PTR(PMXDH_ADDR))
#define PMLCD_REG			(REG8_PTR(PMXD_ADDR))
#define PMLCDH_REG			(REG8_PTR(PMXDH_ADDR))
#define PMSPI_REG			(REG8_PTR(PMSPI_ADDR))
#define PMSPIH_REG			(REG8_PTR(PMSPIH_ADDR))
#define PMSPI1_REG			(REG8_PTR(PMSPI1_ADDR))
#define PMSPI1H_REG			(REG8_PTR(PMSPI1H_ADDR))
#define PMSE_REG			(REG8_PTR(PMSE_ADDR))
#define PMSEH_REG			(REG8_PTR(PMSEH_ADDR))
#define PMSDMMC1_REG		(REG8_PTR(PMSDMMC1_ADDR))
#define PMSDMMC1H_REG		(REG8_PTR(PMSDMMC1H_ADDR))
#define PMSDMMC2_REG		(REG8_PTR(PMSDMMC2_ADDR))
#define PMSDMMC2H_REG		(REG8_PTR(PMSDMMC2H_ADDR))
#define PMPWM_REG			(REG8_PTR(PMPWM_ADDR))
#define PMPWMH_REG			(REG8_PTR(PMPWMH_ADDR))
#define PMPAXI_REG			(REG8_PTR(PMPAXI_ADDR))
#define PMPAXIH_REG			(REG8_PTR(PMPAXIH_ADDR))
#define PMWMTNA_REG			(REG8_PTR(PMWMTNA_ADDR))
#define PMWMTNAH_REG		(REG8_PTR(PMWMTNAH_ADDR))

#define PMNA12_REG			(REG8_PTR(PMNA12_ADDR))
#define PMNA12H_REG			(REG8_PTR(PMNA12H_ADDR))
#define PMCNMNA_REG			(REG8_PTR(PMCNMNA_ADDR))
#define PMCNMNAH_REG		(REG8_PTR(PMCNMNAH_ADDR))
#define PMWMTVDU_REG		(REG8_PTR(PMWMTVDU_ADDR))
#define PMWMTVDUH_REG		(REG8_PTR(PMWMTVDUH_ADDR))
#define PMHDMITV_REG		(REG8_PTR(PMHDMITV_ADDR))
#define PMHDMITVH_REG		(REG8_PTR(PMHDMITVH_ADDR))
#define PMDVO_REG				(REG8_PTR(PMDVO_ADDR))
#define PMDVOH_REG			(REG8_PTR(PMDVOH_ADDR))
#define PMAUDIO_REG			(REG8_PTR(PMAUDIO_ADDR))
#define PMAUDIOH_REG		(REG8_PTR(PMAUDIOH_ADDR))
#define PMCSI0_REG			(REG8_PTR(PMCSI0_ADDR))
#define PMCSI0H_REG			(REG8_PTR(PMCSI0H_ADDR))
#define PMCSI1_REG			(REG8_PTR(PMCSI1_ADDR))
#define PMCSI1H_REG			(REG8_PTR(PMCSI1H_ADDR))

#define PMMALI_REG			(REG8_PTR(PMMALI_ADDR))
#define PMMALIH_REG			(REG8_PTR(PMMALIH_ADDR))
#define PMCNMVDU_REG		(REG8_PTR(PMCNMVDU_ADDR))
#define PMCNMVDUH_REG		(REG8_PTR(PMCNMVDUH_ADDR))
#define PMHDI2C_REG			(REG8_PTR(PMHDI2C_ADDR))
#define PMHDI2CH_REG		(REG8_PTR(PMHDI2CH_ADDR))
#define PMADC_REG				(REG8_PTR(PMADC_ADDR))
#define PMADCH_REG			(REG8_PTR(PMADCH_ADDR))

#define PMI2C4_REG			(REG8_PTR(PMI2C4_ADDR))
#define PMI2C4H_REG			(REG8_PTR(PMI2C4H_ADDR))
#define PMI2C0_REG			(REG8_PTR(PMI2C0_ADDR))
#define PMI2C0H_REG			(REG8_PTR(PMI2C0H_ADDR))
#define PMI2C1_REG			(REG8_PTR(PMI2C1_ADDR))
#define PMI2C1H_REG			(REG8_PTR(PMI2C1H_ADDR))
#define PMI2C2_REG			(REG8_PTR(PMI2C2_ADDR))
#define PMI2C2H_REG			(REG8_PTR(PMI2C2H_ADDR))
#define PMI2C3_REG			(REG8_PTR(PMI2C3_ADDR))
#define PMI2C3H_REG			(REG8_PTR(PMI2C3H_ADDR))

#define PML2CAXI_REG	(REG8_PTR(PML2CAXI_ADDR))
#define PML2CAXIH_REG	(REG8_PTR(PML2CAXIH_ADDR))
#define PMPERI_REG		(REG8_PTR(PMPERI_ADDR))
#define PMPERIH_REG		(REG8_PTR(PMPERIH_ADDR))
#define PMTRACE_REG		(REG8_PTR(PMTRACE_ADDR))
#define PMTRACEH_REG	(REG8_PTR(PMTRACEH_ADDR))
#define PMDBGAPB_REG	(REG8_PTR(PMDBGAPB_ADDR))
#define PMDBGAPBH_REG	(REG8_PTR(PMDBGAPBH_ADDR))
#define PML2CTAG_REG	(REG8_PTR(PML2CTAG_ADDR))
#define PML2CTAGH_REG	(REG8_PTR(PML2CTAGH_ADDR))
#define PML2CDATA_REG	(REG8_PTR(PML2CDATA_ADDR))
#define PML2CDATAH_REG	(REG8_PTR(PML2CDATAH_ADDR))

#define PMCA9PMWDOD_REG 	(REG8_PTR(PMCA9PMWDOD_ADDR))		/* WATCH DOG RESET */
#define PMSDPS_REG			 	(REG8_PTR(PMSDPS_ADDR))					/* SD 0~2 POWER SWITCH */
#define PMMALIGPPWR_REG		(REG8_PTR(PMMALIGPPWR_ADDR))		/* MALI GP Power Shut Off Control and Status Register */
#define PMWMTVDUPWR_REG		(REG8_PTR(PMWMTVDUPWR_ADDR))		/* WMT VDU Power Shut Off Control and Status Register */
#define PMCA9C0PWR_REG	 	(REG8_PTR(PMCA9C0PWR_ADDR))	  	/* CA9 CORE 0 Power Shut Off Control and Status Register */
#define PML2CRAMPWR_REG 	(REG8_PTR(PML2CRAMPWR_ADDR))		/* L2CRAM Power Shut Off Control and Status Register */
#define PMNEON0PWR_REG 		(REG8_PTR(PMNEON0PWR_ADDR))		  /* NEON 0 Power Shut Off Control and Status Register */
#define PMCA9C1PWR_REG 		(REG8_PTR(PMCA9C1PWR_ADDR))		  /* CA9 CORE 1 Power Shut Off Control and Status Register */
#define PMNEON1PWR_REG	 	(REG8_PTR(PMNEON1PWR_ADDR))		  /* NEON 1 Power Shut Off Control and Status Register */
#define PMC_MPWR_REG	   	(REG8_PTR(PMC_MPWR_ADDR))			  /* C&M Power Shut Off Control and Status Register */
#define PMMALIL2CPWR_REG	(REG8_PTR(PMMALIL2CPWR_ADDR))		/* MALI L2C Power Shut Off Control and Status Register */
#define PMMALIPP0PWR_REG	(REG8_PTR(PMMALIPP0PWR_ADDR))		/* MALI PP0 Power Shut Off Control and Status Register */
#define PMMALIPP1PWR_REG	(REG8_PTR(PMMALIPP1PWR_ADDR))		/* MALI PP1 Power Shut Off Control and Status Register */
#define AXI2AHB_REG     	(REG8_PTR(AXI2AHB_ADDR))				/* AXI TO AHB POWER control */

/******************************************************************************
 *
 * Register value.
 *
 ******************************************************************************/
#define PMCS_VAL			(REG32_VAL(PMCS_ADDR))/*0x00*/
#define PMCSH_VAL			(REG32_VAL(PMCSH_ADDR))/*0x04*/
#define PMIR_VAL			(REG8_VAL(PMIR_ADDR))/*0x08*/
#define PMTC_VAL			(REG8_VAL(PMTC_ADDR))/*0x0C*/
#define PMHV_VAL			(REG16_VAL(PMHV_ADDR))/*0x10*/
#define PMHC_VAL			(REG16_VAL(PMHC_ADDR))/*0x12*/
#define PMWS_VAL			(REG32_VAL(PMWS_ADDR))/*0x14*/
#define PMCS2_VAL			(REG32_VAL(PMCS2_ADDR))/*0x18*/
#define PMWE_VAL			(REG32_VAL(PMWE_ADDR))/*0x1C*/
#define PMWT_VAL			(REG32_VAL(PMWT_ADDR))/*0x20*/
#define PMWTC_VAL			(REG32_VAL(PMWTC_ADDR))/*0x24*/

#define PMCWS_VAL			(REG32_VAL(PMCWS_ADDR))/*0x28*/
#define PMCAD_VAL			(REG32_VAL(PMCAD_ADDR))/*0x2C*/

#define HSP0_VAL			(REG32_VAL(HSP0_ADDR))/*0x30*/
#define HSP1_VAL			(REG32_VAL(HSP1_ADDR))/*0x34*/
#define HSP2_VAL			(REG32_VAL(HSP2_ADDR))/*0x38*/
#define HSP3_VAL			(REG32_VAL(HSP3_ADDR))/*0x3c*/
#define HSP4_VAL			(REG32_VAL(HSP4_ADDR))/*0x40*/
#define HSP5_VAL			(REG32_VAL(HSP5_ADDR))/*0x44*/
#define HSP6_VAL			(REG32_VAL(HSP6_ADDR))/*0x48*/
#define HSP7_VAL			(REG32_VAL(HSP7_ADDR))/*0x4c*/
#define PMRS_VAL			(REG32_VAL(PMRS_ADDR))/*0x50*/
#define PMPB_VAL			(REG32_VAL(PMPB_ADDR))/*0x54*/
#define DCDET_STS_VAL		(REG32_VAL(DCDET_STS_ADDR))/*0x5c*/
#define PMSR_VAL			(REG32_VAL(PMSR_ADDR))/*0x60*/
#define TIOUT_RST_VAL		(REG32_VAL(TIOUT_RST_ADDR))/*0x64*/
#define BROM_PD_VAL			(REG32_VAL(BROM_PD_ADDR))/*0x68*/
#define CA9MP_RSTC_VAL		(REG32_VAL(CA9MP_RSTC_ADDR))/*0x6C*/
#define CA9MP_RSTS_VAL		(REG32_VAL(CA9MP_RSTS_ADDR))/*0x70*/

#define PMCIS_VAL			(REG32_VAL(PMCIS_ADDR))/*0x74*/
#define PMCIE_VAL			(REG32_VAL(PMCIE_ADDR))/*0x7C*/
#define INT_TYPE0_VAL		(REG32_VAL(INT_TYPE0_ADDR))/*0x80*/
#define INT_TYPE1_VAL		(REG32_VAL(INT_TYPE1_ADDR))/*0x84*/
#define INT_TYPE2_VAL		(REG32_VAL(INT_TYPE2_ADDR))/*0x88*/

#define RST_VECT_MAP_VAL	(REG32_VAL(RST_VECT_MAP_ADDR))/*0x90*/
#define RTCCM_VAL			(REG32_VAL(RTCCM_ADDR))/*0x94*/
#define PMSTM_VAL			(REG32_VAL(PMSTM_ADDR))/*0x98*/

#define WK_EVT_TYPE_VAL		(REG32_VAL(WK_EVT_TYPE_ADDR))/*0xA0*/
#define WK_TRG_EN_VAL		(REG32_VAL(WK_TRG_EN_ADDR))/*0xB0*/
#define INT_TRG_EN_VAL		(REG32_VAL(INT_TRG_EN_ADDR))/*0xB4*/
#define CA9MPC0_VAL			(REG32_VAL(CA9MPC0_ADDR))/*0xC0*/
#define CA9MPC1_VAL			(REG32_VAL(CA9MPC1_ADDR))/*0xC4*/
#define PWRUP_SRC_VAL		(REG32_VAL(PWRUP_SRC_ADDR))/*0xD0*/

#define OSM4_VAL			(REG32_VAL(OSM4_ADDR))
#define OSM5_VAL			(REG32_VAL(OSM5_ADDR))
#define OSM6_VAL			(REG32_VAL(OSM6_ADDR))
#define OSM7_VAL			(REG32_VAL(OSM7_ADDR))
#define OSM0_VAL			(REG32_VAL(OSM0_ADDR))
#define OSM1_VAL			(REG32_VAL(OSM1_ADDR))
#define OSM2_VAL			(REG32_VAL(OSM2_ADDR))
#define OSM3_VAL			(REG32_VAL(OSM3_ADDR))
#define OSCR_VAL			(REG32_VAL(OSCR_ADDR))
#define OSTS_VAL			(REG32_VAL(OSTS_ADDR))
#define OSTW_VAL			(REG32_VAL(OSTW_ADDR))
#define OSTI_VAL			(REG32_VAL(OSTI_ADDR))
#define OSTC_VAL			(REG32_VAL(OSTC_ADDR))
#define OSTA_VAL			(REG32_VAL(OSTA_ADDR))

#define PMMISC_VAL			(REG32_VAL(PMMISC_ADDR))/*0x1FC*/
#define PMPMA_VAL			(REG32_VAL(PMPMA_ADDR))/*0x200*/
#define PMPMB_VAL			(REG32_VAL(PMPMB_ADDR))
#define PMPMC_VAL			(REG32_VAL(PMPMC_ADDR))
#define PMPMD_VAL			(REG32_VAL(PMPMD_ADDR))
#define PMPME_VAL			(REG32_VAL(PMPME_ADDR))
#define PMPMF_VAL			(REG32_VAL(PMPMF_ADDR))
#define PMPMG_VAL			(REG32_VAL(PMPMG_ADDR))

#define PMCEL_VAL			(REG32_VAL(PMCEL_ADDR))
#define PMCEU_VAL			(REG32_VAL(PMCEU_ADDR))
#define PMCE2_VAL			(REG32_VAL(PMCE2_ADDR))
#define PMCE3_VAL			(REG32_VAL(PMCE3_ADDR))
#define DVFSSTS_VAL   		(REG32_VAL(DVFSSTS_ADDR))
#define DVFSE0_VAL    		(REG32_VAL(DVFSE0_ADDR))    
#define DVFSE1_VAL    		(REG32_VAL(DVFSE1_ADDR))    
#define DVFSE2_VAL    		(REG32_VAL(DVFSE2_ADDR))    
#define DVFSE3_VAL    		(REG32_VAL(DVFSE3_ADDR))    
#define DVFSE4_VAL    		(REG32_VAL(DVFSE4_ADDR))    
#define DVFSE5_VAL    		(REG32_VAL(DVFSE5_ADDR))    
#define DVFSE6_VAL    		(REG32_VAL(DVFSE6_ADDR))    
#define DVFSE7_VAL    		(REG32_VAL(DVFSE7_ADDR))    
#define DVFSE8_VAL    		(REG32_VAL(DVFSE8_ADDR))    
#define DVFSE9_VAL    		(REG32_VAL(DVFSE9_ADDR))    
#define DVFSE10_VAL   		(REG32_VAL(DVFSE10_ADDR))    
#define DVFSE11_VAL   		(REG32_VAL(DVFSE11_ADDR))    
#define DVFSE12_VAL   		(REG32_VAL(DVFSE12_ADDR))    
#define DVFSE13_VAL   		(REG32_VAL(DVFSE13_ADDR))    
#define DVFSE14_VAL   		(REG32_VAL(DVFSE14_ADDR))    
#define DVFSE15_VAL   		(REG32_VAL(DVFSE15_ADDR))

#define PMARM_VAL			(REG8_VAL(PMARM_ADDR))
#define PMARMH_VAL			(REG8_VAL(PMARMH_ADDR))
#define PMAHB_VAL			(REG8_VAL(PMAHB_ADDR))
#define PMAHBH_VAL			(REG8_VAL(PMAHBH_ADDR))
#define PMMC_VAL			(REG8_VAL(PMMC_ADDR))
#define PML2C_VAL			(REG8_VAL(PML2C_ADDR))
#define PML2CH_VAL			(REG8_VAL(PML2CH_ADDR))

#define PMSF_VAL			(REG8_VAL(PMSF_ADDR))
#define PMSFH_VAL			(REG8_VAL(PMSFH_ADDR))
#define PMAPB1_VAL			(REG8_VAL(PMMAPB1_ADDR))
#define PMAPB1H_VAL			(REG8_VAL(PMMAPB1H_ADDR))
#define PMAPB0_VAL			(REG8_VAL(PMAPB0_ADDR))
#define PMAPB0H_VAL			(REG8_VAL(PMAPB0H_ADDR))
#define PMPCM0_VAL			(REG8_VAL(PMPCM0_ADDR))
#define PMPCM0H_VAL			(REG8_VAL(PMPCM0H_ADDR))
#define PMPCM1_VAL			(REG8_VAL(PMPCM1_ADDR))
#define PMPCM1H_VAL			(REG8_VAL(PMPCM1H_ADDR))
#define PMSDMMC_VAL			(REG8_VAL(PMSDMMC_ADDR))
#define PMSDMMCH_VAL		(REG8_VAL(PMSDMMCH_ADDR))
#define PMMSP_VAL			(REG8_VAL(PMMSP_ADDR))
#define PMMSPH_VAL			(REG8_VAL(PMMSPH_ADDR))
#define PMNAND_VAL			(REG8_VAL(PMNAND_ADDR))
#define PMNANDH_VAL			(REG8_VAL(PMNANDH_ADDR))
#define PMXD_VAL			(REG8_VAL(PMXD_ADDR))
#define PMXDH_VAL			(REG8_VAL(PMXDH_ADDR))
#define PMLCD_VAL			(REG8_VAL(PMLCD_ADDR))
#define PMLCDH_VAL			(REG8_VAL(PMLCDH_ADDR))
#define PMSPI_VAL			(REG8_VAL(PMSPI_ADDR))
#define PMSPIH_VAL			(REG8_VAL(PMSPIH_ADDR))
#define PMSPI1_VAL			(REG8_VAL(PMSPI1_ADDR))
#define PMSPI1H_VAL			(REG8_VAL(PMSPI1H_ADDR))
#define PMSE_VAL			(REG8_VAL(PMSE_ADDR))
#define PMSEH_VAL			(REG8_VAL(PMSEH_ADDR))
#define PMSDMMC1_VAL		(REG8_VAL(PMSDMMC1_ADDR))
#define PMSDMMC1H_VAL		(REG8_VAL(PMSDMMC1H_ADDR))
#define PMSDMMC2_VAL		(REG8_VAL(PMSDMMC2_ADDR))
#define PMSDMMC2H_VAL		(REG8_VAL(PMSDMMC2H_ADDR))

#define PMPWM_VAL			(REG8_VAL(PMPWM_ADDR))
#define PMPWMH_VAL			(REG8_VAL(PMPWMH_ADDR))
#define PMPAXI_VAL			(REG8_VAL(PMPAXI_ADDR))
#define PMPAXIH_VAL			(REG8_VAL(PMPAXIH_ADDR))
#define PMWMTNA_VAL			(REG8_VAL(PMWMTNA_ADDR))
#define PMWMTNAH_VAL		(REG8_VAL(PMWMTNAH_ADDR))

#define PMNA12_VAL			(REG8_VAL(PMNA12_ADDR))
#define PMNA12H_VAL			(REG8_VAL(PMNA12H_ADDR))
#define PMCNMNA_VAL			(REG8_VAL(PMCNMNA_ADDR))
#define PMCNMNAH_VAL		(REG8_VAL(PMCNMNAH_ADDR))
#define PMWMTVDU_VAL		(REG8_VAL(PMWMTVDU_ADDR))
#define PMWMTVDUH_VAL		(REG8_VAL(PMWMTVDUH_ADDR))
#define PMHDMITV_VAL		(REG8_VAL(PMHDMITV_ADDR))
#define PMHDMITVH_VAL		(REG8_VAL(PMHDMITVH_ADDR))
#define PMDVO_VAL			(REG8_VAL(PMDVO_ADDR))
#define PMDVOH_VAL			(REG8_VAL(PMDVOH_ADDR))
#define PMAUDIO_VAL			(REG8_VAL(PMAUDIO_ADDR))
#define PMAUDIOH_VAL		(REG8_VAL(PMAUDIOH_ADDR))
#define PMCSI0_VAL			(REG8_VAL(PMCSI0_ADDR))
#define PMCSI0H_VAL			(REG8_VAL(PMCSI0H_ADDR))
#define PMCSI1_VAL			(REG8_VAL(PMCSI1_ADDR))
#define PMCSI1H_VAL			(REG8_VAL(PMCSI1H_ADDR))

#define PMMALI_VAL			(REG8_VAL(PMMALI_ADDR))
#define PMMALIH_VAL			(REG8_VAL(PMMALIH_ADDR))
#define PMCNMVDU_VAL		(REG8_VAL(PMCNMVDU_ADDR))
#define PMCNMVDUH_VAL		(REG8_VAL(PMCNMVDUH_ADDR))
#define PMHDI2C_VAL			(REG8_VAL(PMHDI2C_ADDR))
#define PMHDI2CH_VAL		(REG8_VAL(PMHDI2CH_ADDR))
#define PMADC_VAL			(REG8_VAL(PMADC_ADDR))
#define PMADCH_VAL			(REG8_VAL(PMADCH_ADDR))

#define PMI2C4_VAL			(REG8_VAL(PMI2C4_ADDR))
#define PMI2C4H_VAL			(REG8_VAL(PMI2C4H_ADDR))
#define PMI2C0_VAL			(REG8_VAL(PMI2C0_ADDR))
#define PMI2C0H_VAL			(REG8_VAL(PMI2C0H_ADDR))
#define PMI2C1_VAL			(REG8_VAL(PMI2C1_ADDR))
#define PMI2C1H_VAL			(REG8_VAL(PMI2C1H_ADDR))
#define PMI2C2_VAL			(REG8_VAL(PMI2C2_ADDR))
#define PMI2C2H_VAL			(REG8_VAL(PMI2C2H_ADDR))
#define PMI2C3_VAL			(REG8_VAL(PMI2C3_ADDR))
#define PMI2C3H_VAL			(REG8_VAL(PMI2C3H_ADDR))

#define PML2CAXI_VAL		(REG8_VAL(PML2CAXI_ADDR))
#define PML2CAXIH_VAL		(REG8_VAL(PML2CAXIH_ADDR))
#define PMPERI_VAL			(REG8_VAL(PMPERI_ADDR))
#define PMPERIH_VAL			(REG8_VAL(PMPERIH_ADDR))
#define PMTRACE_VAL			(REG8_VAL(PMTRACE_ADDR))
#define PMTRACEH_VAL		(REG8_VAL(PMTRACEH_ADDR))
#define PMDBGAPB_VAL		(REG8_VAL(PMDBGAPB_ADDR))
#define PMDBGAPBH_VAL		(REG8_VAL(PMDBGAPBH_ADDR))
#define PML2CTAG_VAL		(REG8_VAL(PML2CTAG_ADDR))
#define PML2CTAGH_VAL		(REG8_VAL(PML2CTAGH_ADDR))
#define PML2CDATA_VAL		(REG8_VAL(PML2CDATA_ADDR))
#define PML2CDATAH_VAL	(REG8_VAL(PML2CDATAH_ADDR))

#define PMCA9PMWDOD_VAL 	(REG8_VAL(PMCA9PMWDOD_ADDR))		/* WATCH DOG RESET */
#define PMSDPS_VAL			 	(REG8_VAL(PMSDPS_ADDR))					/* SD 0~2 POWER SWITCH */
#define PMMALIGPPWR_VAL		(REG8_VAL(PMMALIGPPWR_ADDR))		/* MALI GP Power Shut Off Control and Status Register */
#define PMWMTVDUPWR_VAL		(REG8_VAL(PMWMTVDUPWR_ADDR))		/* WMT VDU Power Shut Off Control and Status Register */
#define PMCA9C0PWR_VAL	 	(REG8_VAL(PMCA9C0PWR_ADDR))	  	/* CA9 CORE 0 Power Shut Off Control and Status Register */
#define PML2CRAMPWR_VAL 	(REG8_VAL(PML2CRAMPWR_ADDR))		/* L2CRAM Power Shut Off Control and Status Register */
#define PMNEON0PWR_VAL 		(REG8_VAL(PMNEON0PWR_ADDR))		  /* NEON 0 Power Shut Off Control and Status Register */
#define PMCA9C1PWR_VAL 		(REG8_VAL(PMCA9C1PWR_ADDR))		  /* CA9 CORE 1 Power Shut Off Control and Status Register */
#define PMNEON1PWR_VAL	 	(REG8_VAL(PMNEON1PWR_ADDR))		  /* NEON 1 Power Shut Off Control and Status Register */
#define PMC_MPWR_VAL	   	(REG8_VAL(PMC_MPWR_ADDR))			  /* C&M Power Shut Off Control and Status Register */
#define PMMALIL2CPWR_VAL	(REG8_VAL(PMMALIL2CPWR_ADDR))		/* MALI L2C Power Shut Off Control and Status Register */
#define PMMALIPP0PWR_VAL	(REG8_VAL(PMMALIPP0PWR_ADDR))		/* MALI PP0 Power Shut Off Control and Status Register */
#define PMMALIPP1PWR_VAL	(REG8_VAL(PMMALIPP1PWR_ADDR))		/* MALI PP1 Power Shut Off Control and Status Register */
#define AXI2AHB_VAL     	(REG8_VAL(AXI2AHB_ADDR))				/* AXI TO AHB POWER control */
#define PMDSPPWR_VAL			(REG16_VAL(PMDSPPWR_ADDR))

/*
 * (URRDR) Receive Data Regiser Description
 */
#define URRDR_PER               0x100   /* Parity Error. This bit is the same as URISR[8] */
#define URRDR_FER               0x200   /* Frame Error. This bit is the same as URISR[9] */

/******************************************************************************
 *
 * PMCS_REG     PM (Current) Status Register bits definitions.
 *
 ******************************************************************************/
#define PMCS_NORTC              BIT0    /* RTC Clock Logic Disabled       */
#define PMCS_IDLE               BIT1    /* IDLE Operation Active          */
#define PMCS_HIBER              BIT2    /* Hibernation Operation Active   */
#define PMCS_ANY_CLK_DIV        BIT4    /* Updating Any Clock Divisor     */
#define PMCS_ANY_PLL_MUL        BIT5    /* Updating Any PLL Multiplier    */
#define PMCS_ZAC2               BIT8    /* Updating ZAC2_MA Clock Divisor */
#define PMCS_AHB                BIT9    /* Updating AHB Clock Divisor     */
#define PMCS_DSP                BIT10   /* Updating DSP Clock Divisor     */
#define PMCS_LCD                BIT11   /* Updating LCD Clock Divisor     */
#define PMCS_MC                 BIT12   /* Updating Memory Controller Clock Divisor */
#define PMCS_CFC                BIT13   /* Updating Compact Flash Controller Clock Divisor */
#define PMCS_USB                BIT14   /* Updating USB Clock Divisor     */
#define PMCS_PCM                BIT15   /* Updating Pulse Code Modulation Clock Divisor */
#define PMCS_PLLA               BIT16   /* Updating PLL A Multiplier Value */
#define PMCS_PLLB               BIT17   /* Updating PLL B Multiplier Value */
#define PMCS_PLLC               BIT18   /* Updating PLL C Multiplier Value */
#define PMCS_SF                 BIT19   /* Updating Serial Flash Memory Cntrlr Divisor */
#define PMCS_PATA               BIT21   /* Updating PATA Clock Divisor     */
#define PMCS_SDMMC              BIT22   /* Updating SD/MMC Clock Divisor   */
#define PMCS_MSC                BIT23   /* Updating MS/MSPRO Clock Divisor */
#define PMCS_LPC                BIT24   /* Updating LPC Memory Cntrlr Clock Divisor */
#define PMCS_NAND               BIT25   /* Updating NAND Clock Divisor     */
#define PMCS_SPI                BIT26   /* Updating SPI Clock Divisor      */
#define PMCS_PLLD               BIT27   /* Updating PLL D Multiplier Value */
#define PMCS_BUSY               0xfffffffe

/******************************************************************************
 *
 * PMIR_REG     PM Idle processor Request Register bit function.
 *
 ******************************************************************************/
#define PMIR_IDLE                       /* IDLE Processor Request Bit */


/******************************************************************************
 *
 * PMHC_REG     PM Hibernation Control Register bits functions.
 *
 ******************************************************************************/
#define PMHC_SLEEP              0x03   /* A Power-on Hibernation Mode  */
#define PMHC_SUSPEND            0x201   /* A Power-off Hibernation Mode */
#define PMHC_SHUTDOWN           0x05   /* A Power-off Hibernation Mode */
#define PMHC_25M_OSCLR          BIT8    /* 25MHz Oscillator Enable      */

/******************************************************************************
 *
 * PMWS_REG     PM Wake-up Status register bits definitions.
 *
 ******************************************************************************/
#define PMWS_WAKEMASK           0xFF    /* General Purpose Wake-up Status */
#define PMWS_PWRBUTTON          BIT14   /* Power Button Wake-up Status    */
#define PMWS_RTC                BIT15   /* RTC Wake-up Status             */

/******************************************************************************
 *
 * PMWE_REG     PM Wake-up event Enable Register bits functions.
 *
 ******************************************************************************/
#define PMWE_WAKEMASK           0xFF                    /* General Purpose Wake-up Enable */
#define PMWE_WAKEUP(x)          (BIT0 << ((x) & 0x7))   /* Genaral Wake-up 0-7 Enable     */
#define PMWE_RTC                BIT15                   /* RTC Wake-up Enable             */

/******************************************************************************
 *
 * PMWT_REG     PM Wake-up event Type Register bits functions.
 *
 ******************************************************************************/
#define PMWT_ZERO               0x00            /* Wake-up signal is a zero */
#define PMWT_ONE                0x01            /* Wake-up signal is a one  */
#define PMWT_FALLING            0x02            /* Wake-up signal generates a falling edge */
#define PMWT_RISING             0x03            /* Wake-up signal generates a rising edge  */
#define PMWT_EDGE               0x04            /* Wake-up signal generates an edge        */

#define PMWT_TYPEMASK           0xFF   /* Wake-up event Type Mask                 */

#define PMWT_WAKEUP0(x) (((x) & PMWT_TYPEMASK) << 0)  /* General Purpose Wake-up 0 Type bits */

#define PMWT_WAKEUP1(x) (((x) & PMWT_TYPEMASK) << 4)  /* General Purpose Wake-up 1 Type bits */

#define PMWT_WAKEUP2(x) (((x) & PMWT_TYPEMASK) << 8)  /* General Purpose Wake-up 2 Type bits */

#define PMWT_WAKEUP3(x) (((x) & PMWT_TYPEMASK) << 12) /* General Purpose Wake-up 3 Type bits */

#define PMWT_WAKEUP4(x) (((x) & PMWT_TYPEMASK) << 16) /* General Purpose Wake-up 4 Type bits */

#define PMWT_WAKEUP5(x) (((x) & PMWT_TYPEMASK) << 20) /* General Purpose Wake-up 5 Type bits */

#define PMWT_WAKEUP6(x) (((x) & PMWT_TYPEMASK) << 24) /* General Purpose Wake-up 6 Type bits */

#define PMWT_WAKEUP7(x) (((x) & PMWT_TYPEMASK) << 28) /* General Purpose Wake-up 7 Type bits */

#define PMWT_WAKEUPMASK         0x07            /* Max wakeup source number                */

#define PMWT_WAKEUP(src, type)  ((type & PMWT_TYPEMASK) << ((src & PMWT_WAKEUPMASK) * 4))

/******************************************************************************
 *
 * PMRS_REG     PM Reset Status Register bits definitions.
 *
 ******************************************************************************/
#define PMRS_PMR                BIT0    /* Power Managment Reset  */
#define PMRS_IOR                BIT1    /* I/O normal power Reset */
#define PMRS_HBR                BIT2    /* HiBernation Reset      */
#define PMRS_WDR                BIT3    /* WatchDog Reset         */
#define PMRS_SWR                BIT4    /* SoftWare Reset         */
#define PMRS_SHR                BIT5    /* Shutdown Reset         */
#define PMRS_PGR                BIT6    /* Power good reset       */
/* Bits 7-31: Reserved */

/******************************************************************************
 *
 * PMPB_REG     PM Power Button Control Register
 *
 ******************************************************************************/
#define PMPB_SOFTPWR            BIT0    /* Soft Power Enable      */
#define PMPB_DEBOUNCE(x)		(((x) & 0xFF) << 16) /* PWRBTN debounce value unit ~ 32ms*/
/* Bits 1-31: Reserved */

/******************************************************************************
 *
 * PMSR_REG     PM Software Reset request Register bit function.
 *
 ******************************************************************************/
#define PMSR_SWR                BIT0    /* SoftWare Reset request */
/* Bits 1-31: Reserved */

/******************************************************************************
 *
 * PMPATA_REG   PM PATA Interface Drive Strength Register (8-bit Register)
 *
 ******************************************************************************/
#define PMPATA_ONETHIRD         0x00    /* One-third Drive Strength */
#define PMPATA_ONEHALF          0x01    /* One-half Drive Strength  */
#define PMPATA_TWOTHIRD         0x02    /* Two-third Drive Strength */
#define PMPATA_FULL             0x03    /* Full Drive Strength      */
#define PMSR_SWR                BIT0    /* SoftWare Reset request */
/* Bits 2-7: Reserved */

/******************************************************************************
 *
 * OSTS_REG     OS Timer Status Register bits definitions.
 *
 ******************************************************************************/
#define OSTS_M0                 BIT0    /* OS Timer 0 Match detected */
#define OSTS_M1                 BIT1    /* OS Timer 1 Match detected */
#define OSTS_M2                 BIT2    /* OS Timer 2 Match detected */
#define OSTS_M3                 BIT3    /* OS Timer 3 Match detected */
#define OSTS_MASK               0xF
/* Bits 4-31: Reserved */

/******************************************************************************
 *
 * OSTW_REG     OS Timer Watchdog enable Register bit function.
 *
 ******************************************************************************/
#define OSTW_WE                 BIT0    /* OS Timer Channel 0 Watchdog Enable */
/* Bits 1-31: Reserved */

/******************************************************************************
 *
 * OSTI_REG     OS Timer Interrupt enable Register bits functions.
 *
 ******************************************************************************/
#define OSTI_E0                 BIT0    /* OS Timer Channel 0 Interrupt Enable */
#define OSTI_E1                 BIT1    /* OS Timer Channel 0 Interrupt Enable */
#define OSTI_E2                 BIT2    /* OS Timer Channel 0 Interrupt Enable */
#define OSTI_E3                 BIT3    /* OS Timer Channel 0 Interrupt Enable */
/* Bits 4-31: Reserved */
/******************************************************************************
 *
 * OSTC_REG     OS Timer Control Register bits functions.
 *
 ******************************************************************************/
#define OSTC_ENABLE             BIT0    /* OS Timer Enable bit             */
#define OSTC_RDREQ              BIT1    /* OS Timer Read Count Request bit */
/* Bits 2-31: Reserved */

/******************************************************************************
 *
 * OSTA_REG     OS Timer Access status Register bits definitions.
 *
 ******************************************************************************/
#define OSTA_MWA0               BIT0    /* OS Timer Match 0 Write Active */
#define OSTA_MWA1               BIT1    /* OS Timer Match 1 Write Active */
#define OSTA_MWA2               BIT2    /* OS Timer Match 2 Write Active */
#define OSTA_MWA3               BIT3    /* OS Timer Match 3 Write Active */
#define OSTA_CWA                BIT4    /* OS Timer Count Write Active   */
#define OSTA_RCA                BIT5    /* OS Timer Read Count Active    */
/* Bits 6-31: Reserved */

/******************************************************************************
 *
 * PMMISC_REG   PM Miscellaneous Clock Controls Register
 *
 ******************************************************************************/
#define PMMISC_24MHZ            BIT0    /* 24MHz Clock Source            */
/* Bits 1-31: Reserved */

/******************************************************************************
 *
 * Miscellaneous definitions
 *
 ******************************************************************************/
#define __OST_BASE              0xD8130100      /* OS Timers base address */
#define OST_MAX_CHANNEL         4               /* Four channels OS Timer */

#if 1
typedef struct _PMC_REG_ {
	volatile unsigned int PM_Div_Upt0_sts; /* [Rx00-03] Device clock update status 0 Register*/
	volatile unsigned int PM_Div_Upt1_sts; /* [Rx04-07] Device clock update status 1 Register*/
	volatile unsigned char Idle;/* [Rx08] IDEL Processor Request Register*/
	volatile unsigned char Resv9_0B[3];/* [Rx09 - 0B] Reserved*/
	volatile unsigned short PU_Time_Ctrl;/* [Rx0C] Power-up Tme Control Register*/
	volatile unsigned char Resv0E_0F[2];/* Reserved*/
	volatile unsigned short Hib_Val;/* [Rx10 - Rx11] Hibernation Value Register*/
	volatile unsigned short Hib_Ctrl;/* [Rx12 - Rx13] Hibernation Control Register*/
	volatile unsigned int Wakeup_Sts;/* [Rx14-17]Wake up Status register*/
	volatile unsigned int PM_Sts;/* [Rx18-1B] Power Management Status Register*/
	volatile unsigned int Wakeup_Event_Enable;/* [Rx1C-1F] Wake-up Event Enable Register*/
	volatile unsigned int Wakeup_Event_Type;/* [Rx20-23] Wake-up Event Type Register*/
	volatile unsigned int Wakeup_CardDet_Event_Type;/* [Rx24-27] Card Detect Wake-up Event Type Register*/
	volatile unsigned int CardDet_Sts_Int;/* [Rx28-2B] Card Detect Status And Card Detect Interrupt Register*/
	volatile unsigned int CardReader_Debounce_Int_Type;/* [Rx2C-2F] Card Reader Attachment Debounce Control and Interrupt Type Register*/
	volatile unsigned int Hib_Scratch0;/* [Rx30-33] Hibernate Scratch Pad Register0*/
	volatile unsigned int Hib_Scratch1;/* [Rx34-37] Hibernate Scratch Pad Register1*/
	volatile unsigned int Hib_Scratch2;/* [Rx38-3B] Hibernate Scratch Pad Register2*/
	volatile unsigned int Hib_Scratch3;/* [Rx3c-3F] Hibernate Scratch Pad Register3*/
	volatile unsigned int Hib_Scratch4;/* [Rx40-43] Hibernate Scratch Pad Register4*/
	volatile unsigned int Hib_Scratch5;/* [Rx44-47] Hibernate Scratch Pad Register5*/
	volatile unsigned int Hib_Scratch6;/* [Rx48-4B] Hibernate Scratch Pad Register6*/
	volatile unsigned int Hib_Scratch7;/* [Rx4c-4F] Hibernate Scratch Pad Register7*/
	volatile unsigned int Reset_Sts;/* [Rx50-53] Reset Status Register*/
	volatile unsigned int PB_Control;/* [Rx54-57] Power Button Control Register;*/
	volatile unsigned int AXI_LowPwr_Control;/* [Rx58-5B] AXI Low Power Interface Control Register;*/
	volatile unsigned int Resv5c_5F[1];
	volatile unsigned int SW_Reset_Req;/* [Rx60-63] Software Reset Request Register*/
	volatile unsigned int Tout_Rstart;/* [Rx64-67] time out restart Control Register */
	volatile unsigned int Broom_Powerdown;/* [Rx68-69] bootroom Powerdown, Cache-As-Ram, L2C RAM power force on, L2C bypass control*/
	volatile unsigned char Resv6A_6B[0x2];
	volatile unsigned int CA9MP_Sft_Rst_Ctrl;/* [Rx6C-6F] CA9MP soft reset control */
	volatile unsigned int CA9MP_Sft_Rst_Sts;/* [Rx70-73] CA9MP soft reset ststus */
	volatile unsigned int Int_Wak_Sts;/* [Rx74-77] interrupt status from wakeup source */
	volatile unsigned int Resv78_7B[0x1];
	volatile unsigned int Int_Wak_En;/* [Rx7C-7F] interrupt Enable from wakeup source */
	volatile unsigned int Int_Wak_Type0;/* [Rx80-83] interrupt type0 from wakeup source */
	volatile unsigned int Int_Wak_Type1;/* [Rx84-87] interrupt type1 from wakeup source */
	volatile unsigned int Int_Wak_Type2;/* [Rx88-8B] interrupt type2 from wakeup source */
	volatile unsigned int Resv8C_8F[0x1];
	volatile unsigned int Rst_Vector_Rmap;/* [Rx90-93] Reset vector remap address register */
	volatile unsigned int RTC_Clk_Exist_Monitor; /* [Rx94-97] RTC clock exist monitor Register */
	volatile unsigned int Suspend_To_Dram_En; /* [Rx98-9B] suspend to DRAM enable register */
	volatile unsigned char Resv9C_9F[0x4];
	volatile unsigned int Wak_Event_Type; /* [RxA0-A3] wake event type for USBSW0, CIR ..*/
	volatile unsigned int ResvA4_AC[0x3];
	volatile unsigned int Wak_Trig_En; /* [RxB0-B3] wake triggle enable */
	volatile unsigned int Int_Trig_En; /* [RxB4-B7] interrupt triggle enable */
	volatile unsigned int ResvB8_BF[0x2];
	volatile unsigned int CA9MP_Core0_Retvec; /* [RxC0-C3] CA9MP core 0 retvec register */
	volatile unsigned int CA9MP_Core1_Retvec; /* [RxC4-C7] CA9MP core 1 retvec register */
	volatile unsigned char PU_Src_Sts; /* [RxD0] Power up Source Status register */
	volatile unsigned char ResvD1_EF[0x1F];
	volatile unsigned int OS_Timer_Match4;/* [RxF0-RxF3] OS Timer Match Register4*/
	volatile unsigned int OS_Timer_Match5;/* [RxF4-RxF7] OS Timer Match Registe5*/
	volatile unsigned int OS_Timer_Match6;/* [RxF8-RxFB] OS Timer Match Register6*/
	volatile unsigned int OS_Timer_Match7;/* [RxFC-RxFF] OS Timer Match Register7*/
	volatile unsigned int OS_Timer_Match0;/* [Rx100-Rx103] OS Timer Match Register0*/
	volatile unsigned int OS_Timer_Match1;/* [Rx104-Rx107] OS Timer Match Registe1*/
	volatile unsigned int OS_Timer_Match2;/* [Rx108-Rx10B] OS Timer Match Register2*/
	volatile unsigned int OS_Timer_Match3;/* [Rx10C-Rx10F] OS Timer Match Register3*/
	volatile unsigned int OS_Timer_Count;/* [Rx110-113] OS Timer Counter Register*/
	volatile unsigned int OS_Timer_Sts;/* [Rx114-117] OS Timer Status Register*/
	volatile unsigned int OS_Timer_WatchDog_Enable;/* [Rx118-Rx11B]*/
	volatile unsigned int OS_Timer_Int_Enable;/* [Rx11C-Rx11F]*/
	volatile unsigned int OS_Timer_Ctrl;/* [Rx120-Rx123] OS Timer Control Register*/
	volatile unsigned int OS_Timer_Access_Sts;/* [Rx124-Rx127] OS Timer Access Status Register*/
	volatile unsigned int Resv128_1FB[0x35];
	volatile unsigned int Misc_Clk_Ctrl;/* [Rx1FC-Rx1FF] miscellaneous clock controls register*/
	volatile unsigned int PLLA;/* [Rx200-203] PLLA Multiplier and Range Values Register*/
	volatile unsigned int PLLB;/* [Rx204-207] PLLB Multiplier and Range Values Register*/
	volatile unsigned int PLLC;/* [Rx208-20B] PLLC Multiplier and Range Values Register*/
	volatile unsigned int PLLD;/* [Rx20C-20F] PLLD Multiplier and Range Values Register*/
	volatile unsigned int PLLE;/* [Rx210-213] PLLE Multiplier and Range Values Register*/
	volatile unsigned int PLLF;/* [Rx214-217] PLLF Multiplier and Range Values Register*/
	volatile unsigned int PLLG;/* [Rx218-21B] PLLG Multiplier and Range Values Register*/
	volatile unsigned int PLL_AUD;/* [Rx21C-21F] PLL_AUD Multiplier and Range Values Register*/
	volatile unsigned int PLL_Rdy_Sts;/* [Rx220-223] PLL Ready Status Register*/
	volatile unsigned int Resv224_24F[0x0B];
	volatile unsigned int Clock_Enable0;/* [Rx250-253] Clock Enable 0 Register*/
	volatile unsigned int Clock_Enable1;/* [Rx254-257] Clock Enable 1 Register*/
	volatile unsigned int Clock_Enable2;/* [Rx258-25B] Clock Enable 2 Register*/
	volatile unsigned int Clock_Enable3;/* [Rx25C-25F] Clock Enable 3 Register*/
	volatile unsigned int DVFS_Sts;/* [Rx260-263] DVFS Status Register*/
	volatile unsigned int Resv264_27F[0x7];
	volatile unsigned int DVFS_Entry0;/* [Rx280-283] DVFS Entry 0 Register*/
	volatile unsigned int DVFS_Entry1;/* [Rx284-287] DVFS Entry 1 Register*/
	volatile unsigned int DVFS_Entry2;/* [Rx288-28B] DVFS Entry 2 Register*/
	volatile unsigned int DVFS_Entry3;/* [Rx28C-28F] DVFS Entry 3 Register*/
	volatile unsigned int DVFS_Entry4;/* [Rx290-293] DVFS Entry 4 Register*/
	volatile unsigned int DVFS_Entry5;/* [Rx294-297] DVFS Entry 5 Register*/
	volatile unsigned int DVFS_Entry6;/* [Rx298-29B] DVFS Entry 6 Register*/
	volatile unsigned int DVFS_Entry7;/* [Rx29c-29F] DVFS Entry 7 Register*/
	volatile unsigned int DVFS_Entry8;/* [Rx2A0-2A3] DVFS Entry 8 Register*/
	volatile unsigned int DVFS_Entry9;/* [Rx2A4-2A7] DVFS Entry 9 Register*/
	volatile unsigned int DVFS_Entry10;/* [Rx2A8-2AB] DVFS Entry 10 Register*/
	volatile unsigned int DVFS_Entry11;/* [Rx2AC-2AF] DVFS Entry 11 Register*/
	volatile unsigned int DVFS_Entry12;/* [Rx2B0-2B3] DVFS Entry 12 Register*/
	volatile unsigned int DVFS_Entry13;/* [Rx2B4-2B7] DVFS Entry 13 Register*/
	volatile unsigned int DVFS_Entry14;/* [Rx2B8-2BB] DVFS Entry 14 Register*/
	volatile unsigned int DVFS_Entry15;/* [Rx2BC-2BF] DVFS Entry 15 Register*/
	volatile unsigned int Resv2C0_2FF[0x10];
	volatile unsigned char ARM_Clock_Divisor;/* [Rx300] ARM Clock Divisor Register*/
	/* [Rx301] ARM Clock High Pulse is the Wide Pulse Register*/
	volatile unsigned char ARM_Clock_HiPulse;
	volatile unsigned char Resv302_303[2];
	volatile unsigned char AHB_Clock_Divisor;/* [Rx304] AHB Clock Divisor Value Register*/
	volatile unsigned char Resv305_30B[7];
	volatile unsigned char L2C_Clock_Divisor;/* [Rx30C] Clock Divisor Value L2C Register*/
	volatile unsigned char L2C_Clock_HiPulse;
	volatile unsigned char Resv30E_30F[2];
	/* [Rx310] DDR Memory Controller Clock Divisor Value Register*/
	volatile unsigned char DDR_Clock_Divisor;
	volatile unsigned char Resv311_313[3];
	/* [Rx314] Serial Flash Memory Controller Clock Divisor Value Register*/
	volatile unsigned char SF_Clock_Divisor;
	volatile unsigned char SF_Clock_HiPulse;/* [Rx315]*/
	volatile unsigned char Resv316_317[2];
	volatile unsigned char NF_Clock_Divisor;/* [Rx318] NF Clock Divisor Value Register*/
	volatile unsigned char NF_Clock_HiPulse;/* [Rx319]*/
	volatile unsigned char Resv31A_31B[2];
	volatile unsigned char NOR_Clock_Divisor;/* [Rx31C] NOR Clock Divisor Value Register*/
	volatile unsigned char NOR_Clock_HiPulse;/* [Rx31D]*/
	volatile unsigned char Resv31E_31F[2];
	volatile unsigned char APB_Clock_Divisor;/* [Rx320] APB Clock Divisor Value Register*/
	volatile unsigned char Resv321_323[3];
	volatile unsigned char PCM0_Clock_Divisor;/* [Rx324] PCM0 Clock Divisor Value Reigster*/
	volatile unsigned char PCM0_Clock_HiPulse;/* [Rx325]*/
	volatile unsigned char Resv326_327[2];
	volatile unsigned char PCM1_Clock_Divisor;/* [Rx328] PCM1 Clock Divisor Value Reigster*/
	volatile unsigned char PCM1_Clock_HiPulse;/* [Rx329]*/
	volatile unsigned char Resv32A_32B[2];
	volatile unsigned char Resv32C_32F[4];
	volatile unsigned char SD_Clock_Divisor;/* [Rx330] SD/MMC Clock Divisor Value Reigster*/
	volatile unsigned char SD_Clock_HiPulse;/* [Rx331]*/
	volatile unsigned char Resv332_333[2];
	volatile unsigned char SD1_Clock_Divisor;/* [Rx334] SD/MMC1 Clock Divisor Value Reigster*/
	volatile unsigned char SD1_Clock_HiPulse;/* [Rx335]*/
	volatile unsigned char Resv336_337[2];
	volatile unsigned char SD2_Clock_Divisor;/* [Rx338] SD/MMC2 Clock Divisor Value Reigster*/
	volatile unsigned char SD2_Clock_HiPulse;/* [Rx339]*/
	volatile unsigned char Resv33A_33B[2];
	volatile unsigned char Resv33C_33F[4];
	volatile unsigned char SPI0_Clock_Divisor;/* [Rx340] SPI0 Clock Divisor Value Register*/
	/* [Rx341] SPI0 Clock High Pulse is the Wide Pulse Register*/
	volatile unsigned char SPI0_Clock_HiPulse;
	volatile unsigned char Resv342_343[2];
	volatile unsigned char SPI1_Clock_Divisor;/* [Rx344] SPI1 Clock Divisor Value Register*/
	/* [Rx345] SPI1 Clock High Pulse is the Wide Pulse Register*/
	volatile unsigned char SPI1_Clock_HiPulse;
	volatile unsigned char Resv346_347[2];
	volatile unsigned char SE_Clock_Divisor;/* [Rx348] SE Clock Divisor Value Register*/
	/* [Rx349] SE Clock High Pulse is the Wide Pulse Register*/
	volatile unsigned char SE_Clock_HiPulse;
	volatile unsigned char Resv34A_34F[6];
	volatile unsigned char PWM_Clock_Divisor;/* [Rx350] PWM Clock Divisor Register*/
	volatile unsigned char PWM_Clock_HiPulse;/* [Rx351] PWM Clock High Pulse is the Wide Pulse Register*/
	volatile unsigned char Resv352_353[2];
	volatile unsigned char PAXI_Clock_Divisor;/* [Rx354] PAXI Clock Divisor Value Register*/
	volatile unsigned char PAXI_Clock_HiPulse;/* [Rx355] PAXI Clock High Pulse is the Wide Pulse Register*/
	volatile unsigned char Resv356_357[2];
	volatile unsigned char WMT_NA_Clock_Divisor;/* [Rx358]*/
	volatile unsigned char WMT_NA_Clock_HiPulse;/* [Rx359] WMT NA0 Clock High Pulse is the Wide Pulse Register*/
	volatile unsigned char Resv35A_35B[2];
	volatile unsigned char NA12_Clock_Divisor;/* [Rx35C]*/
	volatile unsigned char NA12_Clock_HiPulse;/* [Rx35D] NA12 Clock High Pulse is the Wide Pulse Register*/
	volatile unsigned char Resv35E_35F[2];
	volatile unsigned char CNM_NA_Clock_Divisor;/* [Rx360]*/
	volatile unsigned char CNM_NA_Clock_HiPulse;/* [Rx361] CNM NA Clock High Pulse is the Wide Pulse Register*/
	volatile unsigned char Resv362_367[6];
	volatile unsigned char WMT_VDU_Clock_Divisor;/* [Rx368]*/
	volatile unsigned char WMT_VDU_Clock_HiPulse;/* [Rx369] WMT VDU Clock High Pulse is the Wide Pulse Register*/
	volatile unsigned char Resv36A_36B[2];
	volatile unsigned char DVOTV2_Clock_Divisor;/* [Rx36C]*/
	volatile unsigned char DVOTV2_Clock_HiPulse;/* [Rx36D] DVOTV2 Clock High Pulse is the Wide Pulse Register*/
	volatile unsigned char TV2_Encoder_En;/* [Rx36E]*/
	volatile unsigned char Resv36F[1];
	volatile unsigned char DVO2_Clock_Divisor;/* [Rx370]*/
	volatile unsigned char DVO2_Clock_HiPulse;/* [Rx371] DVO2 Clock High Pulse is the Wide Pulse Register*/
	volatile unsigned char Resv372_373[2];
	volatile unsigned char AUD_Clock_Divisor;/* [Rx374] AUD Clock Divisor Value Register*/
	volatile unsigned char AUD_Clock_HiPulse;/* [Rx375] AUD Clock High Pulse is the Wide Pulse Register*/
	volatile unsigned char Resv376_377[2];
	volatile unsigned char Ring1_Clock_Divisor;/* [Rx378] Ring OSC 1st divider Register*/
	volatile unsigned char Resv379_37B[3];
	volatile unsigned char Ring2_Clock_Divisor;/* [Rx37C] Ring OSC 2st divider Register*/
	volatile unsigned char Resv37D_37F[3];
	volatile unsigned char CSI0_Clock_Divisor;/* [Rx380]*/
	volatile unsigned char CSI0_Clock_HiPulse;/* [Rx381] CSI0 Clock High Pulse is the Wide Pulse Register*/
	volatile unsigned char Resv382_383[2];
	volatile unsigned char CSI1_Clock_Divisor;/* [Rx384]*/
	volatile unsigned char CSI1_Clock_HiPulse;/* [Rx385] CSI1 Clock High Pulse is the Wide Pulse Register*/
	volatile unsigned char Resv386_387[2];
	volatile unsigned char MALI_Clock_Divisor;/* [Rx388]*/
	volatile unsigned char MALI_Clock_HiPulse;/* [Rx389] MALI Clock High Pulse is the Wide Pulse Register*/
	volatile unsigned char Resv38A_38B[2];
	volatile unsigned char CNM_VDU_Clock_Divisor;/* [Rx38C]*/
	volatile unsigned char CNM_VDU_Clock_HiPulse;/* [Rx38D] CNM VDU Clock High Pulse is the Wide Pulse Register*/
	volatile unsigned char Resv38E_38F[2];
	volatile unsigned char HDMI_I2C_Clock_Divisor;/* [Rx390]*/
	volatile unsigned char HDMI_I2C_Clock_HiPulse;/* [Rx391] HDMI Clock High Pulse is the Wide Pulse Register*/
	volatile unsigned char Resv392_393[2];
	volatile unsigned char ADC_Clock_Divisor;/* [Rx394] ADC Clock Divisor Value Register*/
	volatile unsigned char ADC_Clock_HiPulse;/* [Rx395]*/
	volatile unsigned char Resv396_39F[6];
	volatile unsigned char I2C4_Clock_Divisor;/* [Rx39C]*/
	volatile unsigned char I2C4_Clock_HiPulse;/* [Rx39D] I2C4 Clock High Pulse is the Wide Pulse Register*/
	volatile unsigned char Resv39E_39F[2];
	volatile unsigned char I2C0_Clock_Divisor;/* [Rx3A0]*/
	volatile unsigned char I2C0_Clock_HiPulse;/* [Rx3A1] I2C0 Clock High Pulse is the Wide Pulse Register*/
	volatile unsigned char Resv3A2_3A3[2];
	volatile unsigned char I2C1_Clock_Divisor;/* [Rx3A4]*/
	volatile unsigned char I2C1_Clock_HiPulse;/* [Rx3A5] I2C1 Clock High Pulse is the Wide Pulse Register*/
	volatile unsigned char Resv3A6_3A7[2];
	volatile unsigned char I2C2_Clock_Divisor;/* [Rx3A8]*/
	volatile unsigned char I2C2_Clock_HiPulse;/* [Rx3A9] I2C2 Clock High Pulse is the Wide Pulse Register*/
	volatile unsigned char Resv3AA_3AB[2];
	volatile unsigned char I2C3_Clock_Divisor;/* [Rx3AC]*/
	volatile unsigned char I2C3_Clock_HiPulse;/* [Rx3AD] I2C3 Clock High Pulse is the Wide Pulse Register*/
	volatile unsigned char Resv3AE_3AF[2];
	volatile unsigned char L2C_AXI_Clock_Divisor;/* [Rx3B0]*/
	volatile unsigned char L2C_AXI_Clock_HiPulse;/* [Rx3B1] L2C_AXI Clock High Pulse is the Wide Pulse Register*/
	volatile unsigned char Resv3B2_3B3[2];
	volatile unsigned char ATCLK_Clock_Divisor;/* [Rx3B4]*/
	volatile unsigned char ATCLK_Clock_HiPulse;/* [Rx3B5] ATCLK Clock High Pulse is the Wide Pulse Register*/
	volatile unsigned char Resv3B6_3B7[2];
	volatile unsigned char PERICLK_Clock_Divisor;/* [Rx3B8]*/
	volatile unsigned char PERICLK_Clock_HiPulse;/* [Rx3B9] PERICLK Clock High Pulse is the Wide Pulse Register*/
	volatile unsigned char Resv3BA_3BB[2];
	volatile unsigned char TRACECLK_Clock_Divisor;/* [Rx3BC]*/
	volatile unsigned char TRACECLK_Clock_HiPulse;/* [Rx3BD] TRACECLK Clock High Pulse is the Wide Pulse Register*/
	volatile unsigned char Resv3BE_3BF[2];
	volatile unsigned char Resv3C0_3CF[0x10];
	volatile unsigned char DBUG_APB_Clock_Divisor;/* [Rx3D0]*/
	volatile unsigned char DBUG_APB_Clock_HiPulse;/* [Rx3D1] DBUG APB Clock High Pulse is the Wide Pulse Register*/
	volatile unsigned char Resv3D2_3D3[2];
	volatile unsigned char Resv3D4_3E3[0x10];
	volatile unsigned char Hz24M_Clock_Divisor;/* [Rx3E4]*/
	volatile unsigned char Hz24M_Clock_HiPulse;/* [Rx3E5] 24MHZ Clock High Pulse is the Wide Pulse Register*/
	volatile unsigned char Resv3E6_3EF[10];
	volatile unsigned char L2C_TAG_Clock_Divisor;/* [Rx3F0]*/
	volatile unsigned char L2C_TAG_Clock_HiPulse;/* [Rx3F1] L2C_TAG Clock High Pulse is the Wide Pulse Register*/
	volatile unsigned char Resv3F2_3F3[2];
	volatile unsigned char L2C_DATA_Clock_Divisor;/* [Rx3F4]*/
	volatile unsigned char L2C_DATA_Clock_HiPulse;/* [Rx3F5] L2C_DATA Clock High Pulse is the Wide Pulse Register*/
	volatile unsigned char Resv3F6_47F[0x8A];
	volatile unsigned char CA9MP_Watchdog_Rst_Ctrl;/* [Rx480]*/
	volatile unsigned char Resv481_4FF[0x7F];
	volatile unsigned char PS_Control;/* [Rx500] 1.1.1.85	CARD, SD0~2 Power Switch Control Register*/
	volatile unsigned char Resv501[0xFF];
	volatile unsigned int MALI_GP_PWR_Shut_Off_CTRL_STS;/* [Rx600-603] mali GP power shut off control and status Register*/
	volatile unsigned int WMT_VDU_PWR_Shut_Off_CTRL_STS;/* [Rx604-607] WMT VDU power shut off control and status Register*/
	volatile unsigned int CA9MP_CORE0_PWR_Shut_Off_CTRL_STS;/* [Rx608-60B] CA9MP CORE0 power shut off control and status Register*/
	volatile unsigned int L2C_DATA_PWR_Shut_Off_CTRL_STS;/* [Rx60C-60F] L2C DATA power shut off control and status Register*/
	volatile unsigned int NEON0_PWR_Shut_Off_CTRL_STS;/* [Rx610-613] NEON0 power shut off control and status Register*/
	volatile unsigned int CA9MP_CORE1_PWR_Shut_Off_CTRL_STS;/* [Rx614-617] CA9MP CORE1 power shut off control and status Register*/
	volatile unsigned int NEON1_PWR_Shut_Off_CTRL_STS;/* [Rx618-61B] NEON1 power shut off control and status Register*/
	volatile unsigned int CNM_NA_PWR_Shut_Off_CTRL_STS;/* [Rx61C-61F] C&M NA power shut off control and status Register*/
	volatile unsigned int MALI_L2C_PWR_Shut_Off_CTRL_STS;/* [Rx620-623] mali L2C power shut off control and status Register*/
	volatile unsigned int MALI_PP0_PWR_Shut_Off_CTRL_STS;/* [Rx624-627] mali PP0 power shut off control and status Register*/
	volatile unsigned int MALI_PP1_PWR_Shut_Off_CTRL_STS;/* [Rx628-62B] mali PP1 power shut off control and status Register*/
	volatile unsigned char Resv62C_64F[0x24];
	volatile unsigned int AXI_TO_AHB_Bridge_Pwr_Ctrl;/* [Rx650-653] AXI to AHB bridge power control and status Register*/
	volatile unsigned int PAXI_TO_AHB_Bridge_Pwr_Ctrl;/* [Rx654-657] PAXI to AHB bridge power control and status Register*/
} PMC_REG, *PPMC_REG;
#endif

/******************************************************************************
 *
 * clock enable/disbale macro define
 * CLOCKSET(CLOCK_BIT,CLOCK_SET) 
 * example:
 * CLOCKSET(UART3_CB,EN_C);  --> enable uart3 clock
 * CLOCKSET(UART3_CB,DIS_C); --> disable uart3 clock
 *
 ******************************************************************************/

#if 0
enum CLOCK_BIT {
	I2C1_CB			= 0,	/* I2C1 clock					*/
	UART0_CB,					/* UART0 Clock				*/
	UART1_CB,					/* UART1 Clock				*/
	UART2_CB,					/* UART2 Clock				*/
	UART3_CB,					/* UART3 Clock				*/
	I2C0_CB,					/* I2C0 clock					*/
	RTC_CB			= 7,	/* RTC clock					*/
	KEYPAD_CB		= 9,	/* KEYPAD clock				*/
	PWM_CB,						/* PWM clock					*/
	GPIO_CB,					/* GPIO clock					*/
	SPI0_CB,					/* SPI0 clock					*/
	SPI1_CB,					/* SPI1 clock					*/
	AHB1_CB			=15,	/* AHB1 clock					*/
	I2S_CB,						/* I2S clock					*/
	CIR_CB,						/* CIR clock					*/
	DVO_CB,						/* DVO clock					*/
	AC97_CB,					/* AC97 clock					*/
	PCM_CB,						/* PCM clock					*/
	SCC_CB,						/* SCC clock					*/
	JDEC_CB,					/* JDEC clock					*/
	MSCD_CB,					/* MSCD clock					*/
	AMP_CB,						/* AMP clock					*/
	DSP_CB,						/* DSP clock					*/
	DISP_CB,					/* DISP clock					*/
	VPU_CB,						/* VPU clock					*/
	MBOX_CB,					/* MBOX clock					*/
	GE_CB,						/* GE clock						*/
	GOVRHD_CB,				/* GOVRHD clock				*/
	DDR_CB			=32,	/* DDR clock					*/
	NA0_CB,						/* NA0 clock					*/
	NA12_CB,					/* NA12 clock					*/
	ARF_CB,						/* ARF clock					*/
	ARFP_CB,					/* ARFP clock					*/
	DMA_CB,						/* DMA clock					*/
	ROT_CB,						/* ROT clock					*/
	UHDC_CB,					/* UHDC clock					*/
	PERM_CB,					/* PERM clock					*/
	PDMA_CB,					/* PDMA clock					*/
	SMARTCARD_CB,			/* SMARTCARD clock		*/
	IDE100_CB,				/* IDE100 clock				*/
	IDE133_CB,				/* IDE133 clock				*/
	AHBB_CB,					/* AHBB clock					*/
	SDTV_CB,					/* SDTV clock					*/
	XD_CB,						/* XD clock						*/
	NAND_CB,					/* NAND clock					*/
	MSP_CB,						/* MSP clock					*/
	SD0_CB,						/* SD0 clock					*/
	SD1_CB,						/* SD1 clock					*/
	MAC0_CB,					/* MAC0 clock					*/
	SYS_CB,						/* SYS clock					*/
	TSBK_CB,					/* TSBK clock					*/
	SF_CB,						/* SF clock						*/
	SAE_CB,						/* SAE clock					*/
	H264_CB,					/* H264 clock					*/
	EPHY_CB,					/* EPHY clock					*/
	SCL444U_CB	=60,	/* SCL444U clock			*/
	GOVW_CB,					/* GOVW clock					*/
	VID_CB,						/* VID clock					*/
	VPP_CB						/* VPP clock					*/
};
#endif

enum CLOCK_BIT {
        IDE100_CB = 43,	
	XD_CB     = 47					/* XD clock */		
};

enum CLOCK_SET {
	DIS_C         = 0,   /* Disabble clock          */
	EN_C                 /* Enable Clock            */
};

#define CLOCKDIS(x)	((x < 32) ? (PMCEL_VAL &= ~(1 << x)):(PMCEU_VAL &= ~(1 << (x-32))))
#define CLOCKEN(x)	((x < 32) ? (PMCEL_VAL |= (1 << x)):(PMCEU_VAL |= (1 << (x-32))))

#define CLOCKSET(x,op) ((op) ? CLOCKEN(x):CLOCKDIS(x))

#if 1
/*wakeup event*/
//#define PMWT_C_WAKEUP(src, type)  ((type & PMWT_TYPEMASK) << (((src - 24) & PMWT_WAKEUPMASK) * 4))

enum wakeup_src_e {
	WKS_WK0         = 0,    /* General Purpose Wakeup Source 0          */
	WKS_WK2,                /* General Purpose Wakeup Source 1          */
	WKS_WK3,                /* General Purpose Wakeup Source 2          */
	WKS_WK4,                /* General Purpose Wakeup Source 3          */
	WKS_SUS0,                /* General Purpose Wakeup Source 4          */
	WKS_SUS1,                /* General Purpose Wakeup Source 5          */
	WKS_USBATTA0,            /* USBATTA0          */
	WKS_CIRIN,               /* CIRIN          */	
	WKS_PWRBTN		= 14,    /* PWRBTN as wakeup            */
	WKS_RTC			= 15,    /* RTC as wakeup            */
	WKS_USBOC0		= 16,    /* WKS_USBOC0 as wakeup            */
	WKS_USBOC1		= 17,    /* WKS_USBOC0 as wakeup            */
	WKS_USBOC2		= 18,    /* WKS_USBOC0 as wakeup            */
	WKS_USBOC3		= 19,    /* WKS_USBOC0 as wakeup            */		
	WKS_UHC			= 20,    /* UHC interrupt as wakeup                  */
	WKS_UDC			= 21,    /* WKS_UDC interrupt as wakeup              */	
	WKS_CIR			= 22,    /* CIR interrupt as wakeupr                 */
	WKS_USBSW0		= 23,    /* USBSW0 interrupt as wakeupr          */
	WKS_SD3			= 26,    /* SD3 interrupt as wakeupr                 */	
	WKS_DCDET		= 27,    /* DCDET interrupt as wakeupr               */	
	WKS_SD2			= 28,    /* SD2 interrupt as wakeupr                 */	
	WKS_HDMICEC		= 29,    /* HDMICEC interrupt as wakeupr             */	
	WKS_SD0			= 30,    /* SD0 interrupt as wakeupr                 */
	WKS_WK5			= 31     /* Wakeup event number                      */
};

extern void pmc_enable_wakeup_isr(enum wakeup_src_e wakeup_event, unsigned int type);
extern void pmc_disable_wakeup_isr(enum wakeup_src_e wakeup_event);
extern void pmc_clear_intr_status(enum wakeup_src_e wakeup_event);
extern void pmc_clear_wakeup_status(enum wakeup_src_e wakeup_event);
extern void pmc_enable_wakeup_event(enum wakeup_src_e wakeup_event, unsigned int type);
extern void pmc_disable_wakeup_event(enum wakeup_src_e wakeup_event);

#endif

//#define UDC_HOTPLUG_TIMER

#endif /* __VT8500_PMC_H */
