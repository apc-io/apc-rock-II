/*
 *	linux/include/asm-arm/arch-wmt/irqs.h
 *
 *	Copyright (c) 2008 WonderMedia Technologies, Inc.
 *
 *	This program is free software: you can redistribute it and/or modify it under the
 *	terms of the GNU General Public License as published by the Free Software Foundation,
 *	either version 2 of the License, or (at your option) any later version.
 *
 *	This program is distributed in the hope that it will be useful, but WITHOUT
 *	ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 *	PARTICULAR PURPOSE.  See the GNU General Public License for more details.
 *	You should have received a copy of the GNU General Public License along with
 *	this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *	WonderMedia Technologies, Inc.
 *	10F, 529, Chung-Cheng Road, Hsin-Tien, Taipei 231, R.O.C.
 */

#ifndef __ASM_ARCH_IRQS_H
#define __ASM_ARCH_IRQS_H

/* PPI: Private Peripheral Interrupt */
#define IRQ_PPI(x)			(x + 16)

/* SPI: Shared Peripheral Interrupt */
#define IRQ_SPI(x)			(x + 32)

#define WMT_GIC_DIST_BASE	0xFE019000
#define WMT_GIC_CPU_BASE	0xFE018100

/*
 *
 *  Interrupt sources.
 *
 */
/* #define IRQ_REVERSED      0		*/
#define IRQ_SDC1			IRQ_SPI(1)		/* SD Host controller 0 */
#define IRQ_SDC1_DMA		IRQ_SPI(2)
/* IRQ_REVERSED				3      */
#define IRQ_PMC_AXI_PWR		IRQ_SPI(4)
#define IRQ_GPIO			IRQ_SPI(5)
/* IRQ_REVERSED				6      */
#define IRQ_I2C2			IRQ_SPI(7)
#define IRQ_TZPC_NS			IRQ_SPI(8)
#define IRQ_MC5_SECURE		IRQ_SPI(9)
/* IRQ_REVERSED				10      */
#define IRQ_SDC2			IRQ_SPI(11)
#define IRQ_SDC2_DMA		IRQ_SPI(12)
/* IRQ_REVERSED				13~14   */
#define IRQ_I2C3			IRQ_SPI(15)
#define IRQ_APBB			IRQ_SPI(16)
#define IRQ_DMA_SECURE		IRQ_SPI(17)
#define IRQ_I2C1			IRQ_SPI(18)		/* I2C controller */
#define IRQ_I2C0			IRQ_SPI(19)		/* I2C controller */
#define IRQ_SDC0			IRQ_SPI(20)		/* SD Host controller 1 */
#define IRQ_SDC0_DMA		IRQ_SPI(21)
#define IRQ_PMC_WAKEUP		IRQ_SPI(22)		/* PMC wakeup */
#define IRQ_PCM				IRQ_SPI(23)
#define IRQ_SPI0			IRQ_SPI(24)		/* Serial Peripheral Interface 0 */
#define IRQ_SPI1			IRQ_SPI(25)
#define IRQ_UHDC			IRQ_SPI(26)
#define IRQ_DMA_NONS		IRQ_SPI(27)
#define IRQ_NFC				IRQ_SPI(28)
#define IRQ_NFC_DMA			IRQ_SPI(29)
#define IRQ_PCM1			IRQ_SPI(30)
#define IRQ_I2C4			IRQ_SPI(31)
#define IRQ_UART0			IRQ_SPI(32)
#define IRQ_UART1			IRQ_SPI(33)
#define IRQ_TSC				IRQ_SPI(34)
/* IRQ_REVERSED				35      */
#define IRQ_OST0			IRQ_SPI(36)		/* OS Timer match 0 */
#define IRQ_OST1			IRQ_SPI(37)		/* OS Timer match 1 */
#define IRQ_OST2			IRQ_SPI(38)		/* OS Timer match 2 */
#define IRQ_OST3			IRQ_SPI(39)		/* OS Timer match 3 */
/* IRQ_REVERSED				40~41   */
#define IRQ_OST4			IRQ_SPI(42)
#define IRQ_OST5			IRQ_SPI(43)
#define IRQ_OST6			IRQ_SPI(44)
#define IRQ_OST7			IRQ_SPI(45)
/* IRQ_REVERSED				46      */
#define IRQ_UART2			IRQ_SPI(47)
#define IRQ_RTC1			IRQ_SPI(48)		/* RTC_PCLK_INTR */
#define IRQ_RTC2			IRQ_SPI(49)		/* RTC_PCLK_RTI */
#define IRQ_UART3			IRQ_SPI(50)
/* IRQ_REVERSED				51      */
#define IRQ_PMC_MDM_RDY		IRQ_SPI(52)
/* IRQ_REVERSED				53      */
#define IRQ_PMC_MDM_WAKE_AP	IRQ_SPI(54)
#define IRQ_CIR				IRQ_SPI(55)
#define IRQ_JDEC			IRQ_SPI(56)
#define IRQ_JENC			IRQ_SPI(57)
#define IRQ_SE				IRQ_SPI(58)
#define IRQ_VPP_IRQ0		IRQ_SPI(59)		/* SCL_INISH_INT */
#define IRQ_VPP_IRQ1		IRQ_SPI(60)		/* SCL_INIT */
#define IRQ_VPP_IRQ2		IRQ_SPI(61)		/* SCL444_TG_INT */
#define IRQ_MSVD			IRQ_SPI(62)
/* IRQ_REVERSED				63      */
#define IRQ_DZ_0			IRQ_SPI(64)		/* AUDPRF */
#define IRQ_DZ_1			IRQ_SPI(65)
#define IRQ_DZ_2			IRQ_SPI(66)
#define IRQ_DZ_3			IRQ_SPI(67)
#define IRQ_DZ_4			IRQ_SPI(68)
#define IRQ_DZ_5			IRQ_SPI(69)
#define IRQ_DZ_6			IRQ_SPI(70)
#define IRQ_DZ_7			IRQ_SPI(71)
#define IRQ_VPP_IRQ3		IRQ_SPI(72)		/* VPP_INT */
#define IRQ_VPP_IRQ4		IRQ_SPI(73)		/* GOVW_TG_INT */
#define IRQ_VPP_IRQ5		IRQ_SPI(74)		/* GOVW_INT */
#define IRQ_VPP_IRQ6		IRQ_SPI(75)		/* GOV_INT */
#define IRQ_VPP_IRQ7		IRQ_SPI(76)		/* GE_INT */
#define IRQ_VPP_IRQ8		IRQ_SPI(77)		/* GOVRHD_TG_INT */
#define IRQ_VPP_IRQ9		IRQ_SPI(78)		/* DVO_INT */
#define IRQ_VPP_IRQ10		IRQ_SPI(79)		/* VID_INT */
#define IRQ_H264			IRQ_SPI(80)
/* IRQ_REVERSED				81      */
#define IRQ_MALI_PMU		IRQ_SPI(82)
#define IRQ_MALI_GPMMU		IRQ_SPI(83)
#define IRQ_VPP_IRQ25		IRQ_SPI(84)
#define IRQ_VPP_IRQ26		IRQ_SPI(85)
#define IRQ_VPP_IRQ27		IRQ_SPI(86)
#define IRQ_VPP_IRQ28		IRQ_SPI(87)
/* IRQ_REVERSED				88~90   */
#define IRQ_MALI_PPMMU1		IRQ_SPI(91)
#define IRQ_MALI_PP1		IRQ_SPI(92)
#define IRQ_MALI_GP			IRQ_SPI(93)
#define IRQ_MALI_PPMMU0		IRQ_SPI(94)
#define IRQ_MALI_PP0		IRQ_SPI(95)
#define IRQ_VPP_IRQ19		IRQ_SPI(96)
#define IRQ_VPP_IRQ20		IRQ_SPI(97)
/* IRQ_REVERSED				98      */
#define IRQ_VPP_IRQ21		IRQ_SPI(99)
#define IRQ_VPP_IRQ22		IRQ_SPI(100)
#define IRQ_VPP_IRQ23		IRQ_SPI(101)
#define IRQ_VPP_IRQ24		IRQ_SPI(102)
#define IRQ_DZ_8			IRQ_SPI(103)
#define IRQ_VPP_IRQ11		IRQ_SPI(104)	/* GOVR_INT */
#define IRQ_VPP_IRQ12		IRQ_SPI(105)	/* GOVRSD_TG_INT */
#define IRQ_VPP_IRQ13		IRQ_SPI(106)	/* VPU_INT */
#define IRQ_VPP_IRQ14		IRQ_SPI(107)	/* VPU_TG_INT */
#define IRQ_VPP_IRQ15		IRQ_SPI(108)	/* unused */
#define IRQ_VPP_IRQ16		IRQ_SPI(109)	/* NA12 */
#define IRQ_VPP_IRQ17		IRQ_SPI(110)	/* NA12 */
#define IRQ_VPP_IRQ18		IRQ_SPI(111)	/* NA12 */
#define IRQ_OST0_NS			IRQ_SPI(112)
#define IRQ_OST1_NS			IRQ_SPI(113)
#define IRQ_OST2_NS			IRQ_SPI(114)
#define IRQ_OST3_NS			IRQ_SPI(115)
#define IRQ_OST4_NS			IRQ_SPI(116)
#define IRQ_OST5_NS			IRQ_SPI(117)
#define IRQ_OST6_NS			IRQ_SPI(118)
#define IRQ_OST7_NS			IRQ_SPI(119)
#define IRQ_END				IRQ_OST7_NS

#define NR_IRQS				192

#endif
