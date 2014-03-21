/*++
Copyright (c) 2012 WonderMedia Technologies, Inc. All Rights Reserved.

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

/*  [Description]*/
/*    1st function will be called by post core. */
/*    In this funciton, it will initialize the H/W and Data Structure.*/
/*  [Arguments]*/
/*    NONE*/

#include <common.h>
#include <spi.h>
#include "include/common_def.h"
#include "include/wmt_spi.h"
#include "include/wmt_clk.h"
#include <malloc.h>

#define CTRL_GP12_SPI_byte (*(volatile char *)0xD811004C)
#define PULL_EN_GP12_SPI_byte (*(volatile char *)0xD811048C)
#define PULL_CTRL_GP12_SPI_byte (*(volatile char *)0xD81104CC)

#define PIN_SHARING_SEL_4byte (*(volatile unsigned long *)0xD8110200)
#define CTRL_GP18_SPI1_byte (*(volatile char *)0xD8110052)
#define PULL_EN_GP18_SPI1_byte (*(volatile char *)0xD8110492)
#define PULL_CTRL_GP18_SPI1_byte (*(volatile char *)0xD81104d2)

static unsigned int spi_freq = 2000;
static unsigned int spi_pllb = 0;
extern int auto_pll_divisor(enum dev_id dev, enum clk_cmd cmd, int unit, int freq);

static struct spi_info_s SPI_INFO[SPI_PORT_NUM];
static struct spi_info_s *SPI_PORT[SPI_PORT_NUM];

int spi_init_clock(void)
{
        auto_pll_divisor(DEV_SPI0, CLK_ENABLE, 0, 0);
        spi_pllb = auto_pll_divisor(DEV_SPI0, SET_DIV, 2, 100)/1000;/*enable spi0 clock*/
        auto_pll_divisor(DEV_SPI1, CLK_ENABLE, 0, 0);
        spi_pllb = auto_pll_divisor(DEV_SPI1, SET_DIV, 2, 100)/1000;/*enable spi0 clock*/
        return 0;
}

void spi_init(void)
{
	int port;

	spi_init_clock();
	memset(&SPI_INFO[0], 0x0, sizeof(SPI_INFO));
	//memset(&SPI_INFO[0], 0x0, sizeof(struct spi_info_s));
	//memset(&SPI_INFO[1], 0x0, sizeof(struct spi_info_s));

	/*set gpio*/
	/*spi 0*/
	CTRL_GP12_SPI_byte &= ~(GPIO_SPI0_CLK |
					GPIO_SPI0_MOSI|
					GPIO_SPI0_SS|
					GPIO_SPI0_MISO );

	PULL_EN_GP12_SPI_byte |= (GPIO_SPI0_CLK_PULL_EN |
					GPIO_SPI0_SS_PULL_EN |
					GPIO_SPI0_MOSI_PULL_EN |
					GPIO_SPI0_MISO_PULL_EN);

	/*mosi,miso,ssn*/
	PULL_CTRL_GP12_SPI_byte |= (BIT3 | BIT2 | BIT1);
	/*clk*/
	PULL_CTRL_GP12_SPI_byte &= ~(BIT0);

	// spi1
	PIN_SHARING_SEL_4byte |= BIT10;
	CTRL_GP18_SPI1_byte &= ~(BIT7 | BIT6 | BIT3 | BIT2);
	PULL_EN_GP18_SPI1_byte |= (BIT7 | BIT6 | BIT3 | BIT2);
	// ss0, mosi, miso
	PULL_CTRL_GP18_SPI1_byte |= (BIT7 | BIT3 | BIT2);
	// clk
	PULL_CTRL_GP18_SPI1_byte &= (BIT6);

#if 0
	/*spi0 ss2 ss3*/
	pGpio_Reg->CTRL_GP31_byte &= ~(GPIO_C24MHZCLKI |
					GPIO_C25MHZCLKI);
	pGpio_Reg->PULL_EN_GP31_byte |= (GPIO_C24MHZCLKI_PULL_EN |
					GPIO_C25MHZCLKI_PULL_EN);
	pGpio_Reg->PULL_CTRL_GP31_byte |= (BIT2 | BIT1);

	/*spi0 ss1*/	
	/*turn off GPIO 10*/
	pGpio_Reg->CTRL_GP1_byte &= ~(BIT2);
	pGpio_Reg->PULL_EN_GP1_byte |= BIT2;
	pGpio_Reg->PULL_CTRL_GP1_byte |= BIT2;

	/*set to SPI0SS1 and SPI0SS3*/
	pGpio_Reg->PIN_SHARING_SEL_4byte &= ~(BIT5 | BIT4);
	/*set to SPI0SS2*/
	pGpio_Reg->PIN_SHARING_SEL_4byte |= BIT12;
#endif

	/* Reset SPI*/
	for (port = 0; port < SPI_PORT_NUM; port++) {
		*(SPI_INFO[port].regs.cr) = (SPI_CR_RESET_MASK | SPI_CR_RFR_MASK | SPI_CR_TFR_MASK);
		/* *sr = 0x0;*/
		*(SPI_INFO[port].regs.sr) |= 0x00007F10;
		/* *dfcr = SPI_DFCR_RESET_MASK;*/
		*(SPI_INFO[port].regs.dfcr) = SPI_DFCR_RESET_MASK;
		/* Reset SPI Property*/
		spi_info_reset(port);
	}
}

/*  [Description]*/
/*    SPI property Reset.*/
/*  [Arguments]*/
/*    Port Number*/
void spi_info_reset(int port)
{
	if (port == 0) {
		SPI_INFO[0].regs.cr = (unsigned int volatile *)(SPI_REG_BASE + SPICR) ;
		SPI_INFO[0].regs.sr = (unsigned int volatile *)(SPI_REG_BASE + SPISR);
		SPI_INFO[0].regs.dfcr = (unsigned int volatile *)(SPI_REG_BASE + SPIDFCR);
		SPI_INFO[0].regs.cre = (unsigned int volatile *)(SPI_REG_BASE + SPICRE);
		SPI_INFO[0].regs.rfifo = (unsigned char volatile *)(SPI_REG_BASE + SPIRXFIFO);
		SPI_INFO[0].regs.wfifo = (unsigned char volatile *)(SPI_REG_BASE + SPITXFIFO);
		SPI_INFO[0].regs.srcv_cnt= (unsigned int volatile *)(SPI_REG_BASE + SPISRCV_CNT);
		SPI_INFO[0].regs.srcv_add_cnt= (unsigned int volatile *)(SPI_REG_BASE + SPISRCV_Add_CNT);
	} else if (port == 1) {
		SPI_INFO[1].regs.cr = (unsigned int volatile *)(SPI_REG_BASE1 + SPICR) ;
		SPI_INFO[1].regs.sr = (unsigned int volatile *)(SPI_REG_BASE1 + SPISR );
		SPI_INFO[1].regs.dfcr = (unsigned int volatile *)(SPI_REG_BASE1 + SPIDFCR );
		SPI_INFO[1].regs.cre = (unsigned int volatile *)(SPI_REG_BASE1 + SPICRE);
		SPI_INFO[1].regs.rfifo = (unsigned char volatile *)(SPI_REG_BASE1 + SPIRXFIFO);
		SPI_INFO[1].regs.wfifo = (unsigned char volatile *)(SPI_REG_BASE1 + SPITXFIFO);
		SPI_INFO[1].regs.srcv_cnt= (unsigned int volatile *)(SPI_REG_BASE1 + SPISRCV_CNT);
		SPI_INFO[1].regs.srcv_add_cnt= (unsigned int volatile *)(SPI_REG_BASE1 + SPISRCV_Add_CNT);
	}

	/* Setting default frequence is 1Mhz*/
	SPI_INFO[port].freq = spi_freq; /*20000;*/
	/* SSN Port Mode = Point to point*/
	SPI_INFO[port].ssn_port_mode = 1;
	/* Setting TX Drive Output Value = 0xFF*/
	SPI_INFO[port].tx_nodata_value = 1;
	/* SPI_INFO[port].slave_select = 1;*/
}

/*  [Description]*/
/*    Get SPI Information.*/
/*  [Arguments]*/
/*    Port Number*/
struct spi_info_s* spi_get_info(int port)
{
	if (port >= SPI_PORT_NUM)  /* error port number*/
		return NULL;
	else
		return &SPI_INFO[port];
}

/*  [Description]*/
/*    Map SPI Information to register setting.*/
/*  [Arguments]*/
/*    Port Number*/

void spi_set_reg(int port)
{
	unsigned int  cr,sr,dfcr,cre;
	unsigned int control = 0x0;
	/* SSn Default is pull high,*/
	/* otherwise it is control by program if ssn control is enable*/
	unsigned int dataformat = SPI_DFCR_RESET_MASK;
	unsigned int add_recv_cnt = 0;
	unsigned int divisor = 0x0;
	unsigned char not_sync = 0x0;


	if (port >= SPI_PORT_MAX)  /* error port number*/
	  return;

	/*Reset Hardware*/
	*(SPI_INFO[port].regs.cr) = SPI_CR_RESET_MASK |
		SPI_CR_RFR_MASK |
		SPI_CR_TFR_MASK;
	*(SPI_INFO[port].regs.dfcr) = dataformat;
	sr = *(SPI_INFO[port].regs.sr);
	*(SPI_INFO[port].regs.sr) = sr;
	*(SPI_INFO[port].regs.sr) = 0x0;

	/* Set Frequence*/
	if (SPI_INFO[port].freq != 0)
		divisor = spi_pllb / (SPI_INFO[port].freq * 2);
	else
		divisor = 0;
	control |= ((divisor << SPI_CR_TCD_SHIFT) & SPI_CR_TCD_MASK);

	/* Setting output ssn signal*/
	control |= ((SPI_INFO[port].slave_select << SPI_CR_SS_SHIFT) & SPI_CR_SS_MASK);

	if (SPI_INFO[port].op_mode == SPI_DMA_MODE) {
		/*
		DMA Setting
		Enable Threshold Requst.
		Transfer/Receive Threshold is set to 8 bytes.
		(DMA burst size must less than 8 bytes)
		*/
		control |= SPI_CR_DRC_MASK/*|SPI_CR_TFTS_MASK|SPI_CR_RFTS_MASK*/;
		if (SPI_INFO[port].arbiter == SPI_BUS_MASTER)
			control |= SPI_CR_TFTS_MASK;
		else
			control |= SPI_CR_RFTS_MASK;
			/*control &= ~(SPI_CR_RFTS_MASK);*/

		/* Setting TX: Under-run, Empty Interrupt request*/
		/*  control |= SPI_CR_TFUI_MASK|SPI_CR_TFEI_MASK;*/
		/* Setting RX: Over-run, Full, Empty Interrupt request*/
		/*    control |= SPI_CR_RFOI_MASK|SPI_CR_RFFI_MASK|SPI_CR_RFEI_MASK;*/
		/* Enable IRQ*/
		/*    control |= SPI_CR_IE_MASK;*/
	} else if (SPI_INFO[port].op_mode == SPI_IRQ_MODE) {
		/* Interrupt Setting*/
		/* Pass Threshold Requst to SPI interrupt.*/
		/* Transfer/Receive Threshold is set to 8 bytes.*/
		/*   control |= SPI_CR_TIDS_MASK; //jakie*/
		if (SPI_INFO[port].arbiter == SPI_BUS_MASTER)
			control |= SPI_CR_TFEI_MASK | SPI_CR_RFFI_MASK;
		else
			control |= SPI_CR_RFFI_MASK;
	}

	/* Setting master and slave control*/
	if (SPI_INFO[port].arbiter == SPI_BUS_SLAVE) {
		/* Slave Mode*/
		control |= SPI_CR_MSMS_MASK;
		add_recv_cnt = SPI_INFO[port].rx_cnt;

		/* If work in slave mode, ssn can't be controled and port can't be Multi-Master*/
		if ((SPI_INFO[port].ssn_control == 1) ||
			(SPI_INFO[port].ssn_port_mode == 0))
			not_sync = 1;
		/* Check this, Should it set to Point to Point Mode a*/
		/*nd should ssn be set to auto control by hardware*/
		SPI_INFO[port].ssn_control = 0;
		SPI_INFO[port].ssn_port_mode = 1;
		dataformat |= SPI_DFCR_SPM_MASK;
	}


	/* Set Clock Mode*/
	if ((SPI_INFO[port].clk_mode == SPI_MODE_2) ||
		(SPI_INFO[port].clk_mode == SPI_MODE_3))
		control |= SPI_CR_CPS_MASK;
	if ((SPI_INFO[port].clk_mode == SPI_MODE_1) ||
		(SPI_INFO[port].clk_mode == SPI_MODE_3))
		control |= SPI_CR_CPHS_MASK;

	/* Use for output setting value when FIFO goes empty*/
	/* If enable TX driver Feature, Setting related register.*/
	/* else don't care*/
	if (SPI_INFO[port].tx_drive_enable) {
		dataformat |= SPI_DFCR_TDE_MASK;
		/* Setting Driver Value 0xFF*/
		dataformat |= ((SPI_INFO[port].tx_nodata_value << SPI_DFCR_TNDV_SHIFT) & SPI_DFCR_TNDV_MASK);
		/* Setting Driver Connt*/
		//dataformat |= ((SPI_INFO[port].tx_drive_count << SPI_DFCR_TDCNT_SHIFT) & SPI_DFCR_TDCNT_MASK);
		add_recv_cnt = SPI_INFO[port].tx_cnt;
	}

	/* Use for SSN Control correlated with slave_select*/
	/* If enable SSN Control, Setting related register.*/
	/* else pull high. (Defult setting)*/
	if (SPI_INFO[port].ssn_control) {
		/*  printf("*[SPI]*,P-%d Set SSn Control.\n",port);*/
		dataformat |= SPI_DFCR_SC_MASK;
		/* Setting SSN Pull High*/
		dataformat |= SPI_DFCR_DSV_MASK | SPI_DFCR_DSE_MASK;
	}

	if (SPI_INFO[port].ssn_port_mode == 0)
		/* Multi-Master Mode*/
		/* Enable Passed Mode Fault Error Interrupt request if interrupt is enabled*/
		control |= SPI_CR_MFEI_MASK | SPI_CR_MFEF_MASK;
	else
		/* otherwise, Point-to-Point mode*/
		dataformat |= SPI_DFCR_SPM_MASK;

	cr = *(SPI_INFO[port].regs.cr);
	sr = *(SPI_INFO[port].regs.sr);
	dfcr = *(SPI_INFO[port].regs.dfcr);
	cre = 0x20;
	dataformat &= ~(0xF0000000);
	dataformat |= (0x0 << 28);
	dataformat |= BIT8 | BIT9;
	dataformat |= BIT27; /*enable RX overrun*/

	*(SPI_INFO[port].regs.dfcr) = dataformat;
	*(SPI_INFO[port].regs.srcv_add_cnt) = add_recv_cnt;
	*(SPI_INFO[port].regs.sr) = 0x0;
	*(SPI_INFO[port].regs.cr) = control;
	*(SPI_INFO[port].regs.cre) = cre;
	*(SPI_INFO[port].regs.cr) |= SPI_CR_RFR_MASK; /* reset FIFO*/
	while (*(SPI_INFO[port].regs.cr) & SPI_CR_RFR_MASK)
		;
	*(SPI_INFO[port].regs.cr) |= SPI_CR_TFR_MASK; /* reset FIFO*/
	while (*(SPI_INFO[port].regs.cr) & SPI_CR_TFR_MASK)
		;
}

/*  [Description]*/
/*    Enable SPI Module.*/
/*  [Arguments]*/
/*    Port Number*/
void spi_enable(int port)
{
	unsigned int cr = SPI_CR_ME_MASK;

	if (port >= SPI_PORT_MAX)  /* error port number*/
		return;

	if (SPI_INFO[port].op_mode == SPI_IRQ_MODE ||
		SPI_INFO[port].op_mode == SPI_DMA_MODE)
		cr |= SPI_CR_IE_MASK;

	/* Master mode and ssn controled by programming*/
	if ((SPI_INFO[port].arbiter == SPI_BUS_MASTER) &&
		(SPI_INFO[port].ssn_control == 1))
		/* Pull low ssn for enable active spi*/
		*(SPI_INFO[port].regs.dfcr) &= ~(SPI_DFCR_DSV_MASK | SPI_DFCR_DSE_MASK);

	/*  printf("*[SPI]*,P-%d Module and Interrupt Enable.\n",port);*/
	/* Module enable*/
	*(SPI_INFO[port].regs.cr) |= cr;
}

/*  [Description]*/
/*    Disable SPI Module.*/
/*  [Arguments]*/
/*    Port Number*/
void spi_disable(int port)
{
	unsigned int cr = SPI_CR_ME_MASK;

	if (port >= SPI_PORT_MAX)  /* error port number*/
		return;

	if (SPI_INFO[port].op_mode == SPI_IRQ_MODE ||
		SPI_INFO[port].op_mode == SPI_DMA_MODE)
		cr |= SPI_CR_IE_MASK;

	/* Master mode and ssn controled by programming*/
	if ((SPI_INFO[port].arbiter == SPI_BUS_MASTER) &&
		(SPI_INFO[port].ssn_control == 1))
		/* Pull high ssn for enable inactive spi*/
		*(SPI_INFO[port].regs.dfcr) |= SPI_DFCR_DSV_MASK | SPI_DFCR_DSE_MASK;

	/* Module disable*/
	*(SPI_INFO[port].regs.cr) &= ~(cr);
	/*printf("*[SPI]*,P-%d Module and Interrupt Disable.\n",port);*/
}

/*===========================================================================*/
/*  spi_tx_polling*/
/**/
/*  return:*/
/*===========================================================================*/
int spi_tx_polling(int portNum)
{
	unsigned int status;
	int timeout_cnt = 0x20000;
	while (timeout_cnt--) {
		status = *(SPI_INFO[portNum].regs.sr);
		if (status & SPI_SR_TFEI_MASK)
			return 0;
	}

	printf("spi%d timeout\n", portNum);
	return -1;
}

void set_spi_freq(unsigned int freq)
{
	spi_freq = freq;
}
void spi_write_then_read_data(
			unsigned char *wbuf,
			unsigned char *rbuf,
			int num,
			int clk_mode,
			int chip_sel)
{
	int i = 0;
	int transmitted_count = 0;
	int portNum = 0;
	SPI_INFO[portNum].clk_mode = clk_mode;
	SPI_INFO[portNum].slave_select = chip_sel;
	SPI_INFO[portNum].tx_cnt = num;
	spi_set_reg(portNum);
	while (transmitted_count + 32 < num) {
		spi_disable(portNum);
		for (i = transmitted_count; i < 32; ++i)
			*SPI_INFO[portNum].regs.wfifo = wbuf[i];
		spi_enable(portNum);
		while (spi_tx_polling(portNum))
			;
		for (i = transmitted_count; i <32; ++i)
			rbuf[i] = *SPI_INFO[portNum].regs.rfifo;
		transmitted_count += 32;
	}

	spi_disable(portNum);
	for (i = transmitted_count; i < num; ++i)
		*SPI_INFO[portNum].regs.wfifo = wbuf[i];
	spi_enable(portNum);
	while (spi_tx_polling(portNum))
		;
	for (i = transmitted_count; i < num; ++i)
		rbuf[i] = *SPI_INFO[portNum].regs.rfifo;

	spi_disable(portNum);
	return;
}

void spi1_write_then_read_data(
			unsigned char *wbuf,
			unsigned char *rbuf,
			int num,
			int clk_mode,
			int chip_sel)
{
	int i = 0;
	int transmitted_count = 0;
	int portNum = 1;
	SPI_INFO[portNum].clk_mode = clk_mode;
	SPI_INFO[portNum].slave_select = chip_sel;
	SPI_INFO[portNum].tx_cnt = num;
	spi_set_reg(portNum);
	while (transmitted_count + 32 < num) {
		spi_disable(portNum);
		for (i = transmitted_count; i < 32; ++i)
			*SPI_INFO[portNum].regs.wfifo = wbuf[i];
		spi_enable(portNum);
		while (spi_tx_polling(portNum))
			;
		for (i = transmitted_count; i <32; ++i)
			rbuf[i] = *SPI_INFO[portNum].regs.rfifo;
		transmitted_count += 32;
	}

	spi_disable(portNum);
	for (i = transmitted_count; i < num; ++i)
		*SPI_INFO[portNum].regs.wfifo = wbuf[i];
	spi_enable(portNum);
	while (spi_tx_polling(portNum))
		;
	for (i = transmitted_count; i < num; ++i)
		rbuf[i] = *SPI_INFO[portNum].regs.rfifo;

	spi_disable(portNum);
	return;
}

#if 0
int loopback_test(unsigned int reg)
{
	unsigned char addr[3];
	unsigned char data[3];
	//addr[0] = ((reg & 0XFF) & (~ BIT7));
	//addr[1] = ((reg & 0XFF) >> 7);
	addr[0] = 0xaa;
	addr[1] = 0x39;
	addr[2] = 0x55;
	data[0] = 0x00;
	data[1] = 0x00;
	data[2] = 0x00;

	spi_write_then_read_data(addr, data, 3, 0, 0);

	//data[2] &=0xff;
	printf("value 0 = %x\n", data[0]);
	printf("value 1 = %x\n", data[1]);
	printf("value 2 = %x\n", data[2]);
	return data[2];
}
#endif
