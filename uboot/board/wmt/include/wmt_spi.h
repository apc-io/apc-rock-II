/*++
Copyright (c) 2008 WonderMedia Technologies, Inc. All Rights Reserved.

This PROPRIETARY SOFTWARE is the property of WonderMedia Technologies, Inc.
and may contain trade secrets and/or other confidential information of
WonderMedia Technologies, Inc. This file shall not be disclosed to any third
party, in whole or in part, without prior written consent of WonderMedia.

THIS PROPRIETARY SOFTWARE AND ANY RELATED DOCUMENTATION ARE PROVIDED AS IS,
WITH ALL FAULTS, AND WITHOUT WARRANTY OF ANY KIND EITHER EXPRESS OR IMPLIED,
AND WonderMedia TECHNOLOGIES, INC. DISCLAIMS ALL EXPRESS OR IMPLIED WARRANTIES
OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR
NON-INFRINGEMENT.
--*/

#ifndef __SPI_H__
#define __SPI_H__

#define SPI_REG_BASE     0xD8240000     /* SPI Base Address */
#define SPI_REG_BASE1    0xD8250000     /* SPI1 Base Address */

#define SPI_PORT_MAX        2

#define SPI_PORT_NUM        2

#define AHB_FREQ          100000

#define SPI_PORT_REG_BASE_OFFSET  0x10000

#define SPICR           0x00
#define SPISR           0x04
#define SPIDFCR           0x08
#define SPICRE              0x0C
#define SPITXFIFO         0x10
#define SPIRXFIFO         0x30
#define SPISRCV_CNT    0x50
#define SPISRCV_Add_CNT 0x54

#define GPIO_SPI0_SS    BIT3
#define GPIO_SPI0_MOSI  BIT2
#define GPIO_SPI0_MISO  BIT1
#define GPIO_SPI0_CLK   BIT0
#define GPIO_SPI0_SS_PULL_EN    BIT3
#define GPIO_SPI0_MOSI_PULL_EN  BIT2
#define GPIO_SPI0_MISO_PULL_EN  BIT1
#define GPIO_SPI0_CLK_PULL_EN   BIT0

/**/
/*  SPI CFG setting*/
/**/

/************ Control Register ************/
/* Transmit Clock Driver*/
#define SPI_CR_TCD_SHIFT    21
#define SPI_CR_TCD_MASK     (BIT31|BIT30|BIT29|BIT28|BIT27|BIT26|BIT25|BIT24|BIT23|BIT22|BIT21)
/* Slave Selection*/
#define SPI_CR_SS_SHIFT     19
#define SPI_CR_SS_MASK      (BIT20|BIT19)
/* Transmit FIFO Byte Write Method*/
#define SPI_CR_WM_SHIFT     18
#define SPI_CR_WM_MASK      (BIT18)
/* Receive FIFO Reset*/
#define SPI_CR_RFR_SHIFT    17
#define SPI_CR_RFR_MASK     (BIT17)
/* Transmit FIFO Reset*/
#define SPI_CR_TFR_SHIFT    16
#define SPI_CR_TFR_MASK     (BIT16)
/* DMA Request Control*/
#define SPI_CR_DRC_SHIFT    15
#define SPI_CR_DRC_MASK     (BIT15)
/* Receive FIFO Threshold Selection*/
#define SPI_CR_RFTS_SHIFT   14
#define SPI_CR_RFTS_MASK    (BIT14)
/* Transmit FIFO Threshold Selection*/
#define SPI_CR_TFTS_SHIFT   13
#define SPI_CR_TFTS_MASK    (BIT13)
/* Transmit FIFO Under-run Interrupt*/
#define SPI_CR_TFUI_SHIFT   12
#define SPI_CR_TFUI_MASK    (BIT12)
/* Transmit FIFO Empty Interrupt*/
#define SPI_CR_TFEI_SHIFT   11
#define SPI_CR_TFEI_MASK    (BIT11)
/* Receive FIFO Over-run Interrupt*/
#define SPI_CR_RFOI_SHIFT   10
#define SPI_CR_RFOI_MASK    (BIT10)
/* Receive FIFO Full Interrupt*/
#define SPI_CR_RFFI_SHIFT   9
#define SPI_CR_RFFI_MASK    (BIT9)
/* Receive FIFO Empty Interrupt*/
#define SPI_CR_RFEI_SHIFT   8
#define SPI_CR_RFEI_MASK    (BIT8)
/* Threshold IRQ/DMA Selection*/
#define SPI_CR_TIDS_SHIFT   7
#define SPI_CR_TIDS_MASK    (BIT7)
/* Interrupt Enable*/
#define SPI_CR_IE_SHIFT     6
#define SPI_CR_IE_MASK      (BIT6)
/* Module Enable*/
#define SPI_CR_ME_SHIFT     5
#define SPI_CR_ME_MASK      (BIT5)
/* Module Fault Error Interrupt*/
#define SPI_CR_MFEI_SHIFT   4
#define SPI_CR_MFEI_MASK    (BIT4)
/* Master/Slave Mode Select*/
#define SPI_CR_MSMS_SHIFT   3
#define SPI_CR_MSMS_MASK    (BIT3)
/* Clock Polarity Select*/
#define SPI_CR_CPS_SHIFT    2
#define SPI_CR_CPS_MASK     (BIT2)
/* Clock Phase Select*/
#define SPI_CR_CPHS_SHIFT   1
#define SPI_CR_CPHS_MASK    (BIT1)
/* Module Fault Error Feature*/
#define SPI_CR_MFEF_SHIFT   0
#define SPI_CR_MFEF_MASK    (BIT0)
/* SPI Control Register Reset Value*/
#define SPI_CR_RESET_MASK   SPI_CR_MSMS_MASK

/************ Status Register *************/
/* RX FIFO Count*/
#define SPI_SR_RFCNT_SHIFT    24
#define SPI_SR_RFCNT_MASK   (BIT31|BIT30|BIT29|BIT28|BIT27|BIT26|BIT25|BIT24)
/* TX FIFO Count*/
#define SPI_SR_TFCNT_SHIFT    16
#define SPI_SR_TFCNT_MASK   (BIT23|BIT22|BIT21|BIT20|BIT19|BIT18|BIT17|BIT16)
/* TX FIFO Empty Status*/
#define SPI_SR_TFES_SHIFT   15
#define SPI_SR_TFES_MASK    (BIT15)
/* Receive FIFO Threshold Passed Interrupt*/
#define SPI_SR_RFTPI_SHIFT    14
#define SPI_SR_RFTPI_MASK   (BIT14)
/* Transmit FIFO Threshold Passed Interrupt*/
#define SPI_SR_TFTPI_SHIFT    13
#define SPI_SR_TFTPI_MASK   (BIT13)
/* Transmit FIFO Under-run Interrupt*/
#define SPI_SR_TFUI_SHIFT   12
#define SPI_SR_TFUI_MASK    (BIT12)
/* Transmit FIFO Empty Interrupt*/
#define SPI_SR_TFEI_SHIFT   11
#define SPI_SR_TFEI_MASK    (BIT11)
/* Receive FIFO Over-run Interrupt*/
#define SPI_SR_RFOI_SHIFT   10
#define SPI_SR_RFOI_MASK    (BIT10)
/* Receive FIFO Full Interrupt*/
#define SPI_SR_RFFI_SHIFT   9
#define SPI_SR_RFFI_MASK    (BIT9)
/* Receive FIFO Empty Interrupt*/
#define SPI_SR_RFEI_SHIFT   8
#define SPI_SR_RFEI_MASK    (BIT8)
/* SPI Busy*/
#define SPI_SR_BUSY_SHIFT   7
#define SPI_SR_BUSY_MASK    (BIT7)
/* Mode Fault Error Interrupt*/
#define SPI_SR_MFEI_SHIFT   4
#define SPI_SR_MFEI_MASK    (BIT4)

/****** Data Format Control Register ******/
/*Preset Counter*/
#define SPI_SSN_PRE_COUNTER_SHIFT 28
#define SPI_SSN_PRE_COUNTER_MASK (BIT31|BIT30|BIT29|BIT28)
/*HOLD EN*/
#define SPI_SSN_HOLD_EN BIT26
/*Microwire EN*/
#define SPI_MICROWIRE_EN BIT25
/*RX theshold Pass Interrupt Enable*/
#define SPI_RX_THESHOLD_INT_EN BIT24
/* Mode Fault Delay Count*/
#define SPI_DFCR_MFDCNT_SHIFT 16
#define SPI_DFCR_MFDCNT_MASK  (BIT23|BIT22|BIT21|BIT20|BIT19|BIT18|BIT17|BIT16)
/* TX Drive Count*/
#define SPI_DFCR_TDCNT_SHIFT  8
#define SPI_DFCR_TDCNT_MASK   (BIT15|BIT14|BIT13|BIT12|BIT11|BIT10|BIT9|BIT8)
/* TX Drive Enable*/
#define SPI_DFCR_TDE_SHIFT    7
#define SPI_DFCR_TDE_MASK   (BIT7)
/* TX No Data Value*/
#define SPI_DFCR_TNDV_SHIFT   6
#define SPI_DFCR_TNDV_MASK    (BIT6)
/* Direct SSN Enable*/
#define SPI_DFCR_DSE_SHIFT    5
#define SPI_DFCR_DSE_MASK   (BIT5)
/* Direct SSN Value*/
#define SPI_DFCR_DSV_SHIFT    4
#define SPI_DFCR_DSV_MASK   (BIT4)
/* SSN Control*/
#define SPI_DFCR_SC_SHIFT   3
#define SPI_DFCR_SC_MASK    (BIT3)
/* SSN Port Mode*/
#define SPI_DFCR_SPM_SHIFT    2
#define SPI_DFCR_SPM_MASK   (BIT2)
/* Receive Significant Bit Order*/
#define SPI_DFCR_RSBO_SHIFT   1
#define SPI_DFCR_RSBO_MASK    (BIT1)
/* Transmit Significant Bit Order*/
#define SPI_DFCR_TSBO_SHIFT   0
#define SPI_DFCR_TSBO_MASK    (BIT0)
/* SPI Data Format Control Register Reset Value*/
#define SPI_DFCR_RESET_MASK   (SPI_DFCR_DSV_MASK|SPI_DFCR_DSE_MASK)

/* Clock Mode Define*/
#define SPI_CLK_MODE_LIST           \
	T(SPI_MODE_0),  /*  0, CLK Idles Low  + SS  activation */ \
	T(SPI_MODE_1),  /*  1, CLK Idles Low  + CLK activation */ \
	T(SPI_MODE_2),  /*  2, CLK Idles High + SS  activation */ \
	T(SPI_MODE_3) /*  3, CLK Idles High + CLK activation */

/* Bus Master Define*/
#define SPI_ARBITER_LIST            \
	T(SPI_BUS_MASTER),  /*  0 */  \
	T(SPI_BUS_SLAVE) /*  1 */

/* Bus Operate Mode Define*/
#define SPI_OP_MODE_LIST            \
	T(SPI_POLLING_MODE),  /*  0 */  \
	T(SPI_IRQ_MODE),  /*  1 */  \
	T(SPI_DMA_MODE) /*  2 */

#define T(x) x
enum {
	SPI_CLK_MODE_LIST
};

enum {
	SPI_ARBITER_LIST
};

enum {
	SPI_OP_MODE_LIST
};
#undef T

struct spi_reg_s {
	unsigned int volatile *cr;  /* Control Register*/
	unsigned int volatile *sr;  /* Status Register*/
	unsigned int volatile *dfcr;  /* Data Format Control Register*/
	unsigned int volatile *cre; /*Extended Control Register*/
	unsigned char volatile *rfifo;  /* Read FIFO, i.e. Receive FIFO*/
	unsigned char volatile *wfifo;  /* Write FIFO, i.e. Transfer FIFO*/
	unsigned int volatile *srcv_cnt;
	unsigned int volatile *srcv_add_cnt;
} ;

struct spi_info_s {
	/* Register Data*/
	struct spi_reg_s regs;
	/* Private Data*/
	unsigned int freq;          /* Bus Frequence, Unit Khz*/
	unsigned int  clk_mode;       /* Bus Clock Mode*/
	unsigned int  op_mode;       /* Operation Mode, polling/irq/dma*/
	/* Use for output setting value when FIFO goes empty*/
	unsigned char tx_drive_count;   /* Set the number of bytes to send out*/
	unsigned char tx_drive_enable;    /* Enable/Disable TX drive*/
	unsigned char tx_nodata_value;    /* Set output value, o => 0x0, 1 =>0xff*/
	/* Use for SSN Control correlated with slave_select*/
	unsigned char ssn_control;      /* Set ssn control by 1 => program or 0 => hardware*/
	/* user for master and slave control*/
	unsigned int arbiter;
	unsigned char ssn_port_mode;    /* set ssn port mode. 0 => multi-master, 1 => P2P*/
	unsigned char slave_select;     /* Set output ssn pin {a,b,c,d}*/

	unsigned int rx_cnt;
	unsigned int tx_cnt;

	/* interrup and dma control*/
	int irq_num;
	void (*isr)(void);          /* interrupt service routine*/
	void (*dsr)(void);          /* dma service routine*/
} ;

//struct spi_info_s SPI_INFO[SPI_PORT_NUM];
//struct spi_info_s *SPI_PORT[SPI_PORT_NUM];

/*---------------------------------------------------------------------------------------*/
/* GPIO Public functions declaration*/
/*---------------------------------------------------------------------------------------*/
void spi_init(void);

void spi_info_reset(int port);

void spi_set_reg(int port);

void spi_enable(int port);

void spi_disable(int port);

struct spi_info_s *spi_get_info(int port);

void spi_write_then_read_data(unsigned char *wbuf, unsigned char *rbuf, int num, int clk_mode, int chip_sel);

int spi_init_clock(void);

#endif  /*__SPI_H__*/
