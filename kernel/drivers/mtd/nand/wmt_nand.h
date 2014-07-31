/*++
	Copyright (c) 2008  WonderMedia Technologies, Inc.   All Rights Reserved.

	This PROPRIETARY SOFTWARE is the property of WonderMedia Technologies, Inc.
	and may contain trade secrets and/or other confidential information of
	WonderMedia Technologies, Inc. This file shall not be disclosed to any third
	party, in whole or in part, without prior written consent of WonderMedia.

	THIS PROPRIETARY SOFTWARE AND ANY RELATED DOCUMENTATION ARE PROVIDED AS IS,
	WITH ALL FAULTS, AND WITHOUT WARRANTY OF ANY KIND EITHER EXPRESS OR IMPLIED,
	AND WonderMedia TECHNOLOGIES, INC. DISCLAIMS ALL EXPRESS OR IMPLIED WARRANTIES
	OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR
	NON-INFRINGEMENT.

	Module Name:

		$Workfile: post_nand.c $

	Abstract:

		POST functions, called by main().

	Revision History:

		Dec.04.2008 First created
		Dec.19.2008 Dannier change coding style and support spi flash boot with nor accessible.
	$JustDate: 2008/12/19 $
--*/

#ifndef __NFC_H__
#define __NFC_H__

/* include/asm-arm/arch-vt8630.h or arch-vt8620.h or include/asm-zac/arch-vt8620.h or arch-vt8630.h */
/* #define NFC_BASE_ADDR 0xd8009000 */

#define NFCR0_DATAPORT		              	0x00
#define NFCR1_COMCTRL			              	0x04
#define NFCR2_COMPORT0		              	0x08
#define NFCR3_COMPORT1_2	              	0x0c
#define NFCR4_COMPORT3_4	              	0x10
#define NFCR5_COMPORT5_6	              	0x14
#define NFCR6_COMPORT7		              	0x18
#define NFCR7_DLYCOMP		                	0x1c
#define NFCR8_DMA_CNT			              	0x20
#define NFCR9_ECC_BCH_CTRL              	0x24
#define NFCRa_NFC_STAT			            	0x28
#define NFCRb_NFC_INT_STAT		          	0x2c
#define NFCRc_CMD_ADDR				            0x30
#define NFCRd_OOB_CTRL	                  0x34
#define NFCRe_CALC_TADL	                  0x38
#define NFCRf_CALC_RDMZ			              0x3c
#define NFCR10_OOB_ECC_SIZE               0x40
#define NFCR11_SOFT_RST					          0x44
#define NFCR12_NAND_TYPE_SEL              0x48
#define NFCR13_INT_MASK							      0x4c
#define NFCR14_READ_CYCLE_PULE_CTRL	      0x50
#define NFCR15_IDLE_STAT		              0x54
#define NFCR16_TIMER_CNT_CFG              0x58
#define NFCR17_ECC_BCH_ERR_STAT			      0x5c
#define NFCR18_ECC_BCH_ERR_POS	          0x60

#define ECC_FIFO_0   0x1c0
#define ECC_FIFO_1   0x1c4
#define ECC_FIFO_2   0x1c8
#define ECC_FIFO_3   0x1cc
#define ECC_FIFO_4   0x1d0
#define ECC_FIFO_5   0x1d4
#define ECC_FIFO_6   0x1d8
#define ECC_FIFO_7   0x1dc
#define ECC_FIFO_8   0x1e0
#define ECC_FIFO_9   0x1e4
#define ECC_FIFO_a   0x1e8
#define ECC_FIFO_b   0x1ec
#define ECC_FIFO_c   0x1f0
#define ECC_FIFO_d   0x1f4
#define ECC_FIFO_e   0x1f8
#define ECC_FIFO_f   0x1fc

struct ECC_size_info{
	int ecc_engine;
	int oob_ECC_mode;
	int banks;
	int bank_size;
	int max_bit_error;
	int oob_max_bit_error;
	int ecc_bits_count;
	int oob_ecc_bits_count;
	int bank_offset; /* add data + data ecc ex: 512+8 4-bit engine */
	int ECC_bytes;
	int oob_ECC_bytes;
	int unprotect;
};
#define ECC1bit 0
#define ECC4bit 1
#define ECC8bit 2
#define ECC12bit 3
#define ECC16bit 4
#define ECC24bitPer1K 5
#define ECC40bitPer1K 6
#define ECC60bitPer1K 7
#define ECC44bitPer1K 7
#define ECC44bit 8
#define ECC1bit_bit_count 32
#define ECC4bit_bit_count 52
#define ECC8bit_bit_count 104
#define ECC12bit_bit_count 156
#define ECC16bit_bit_count 208
#define ECC24bitPer1K_bit_count 336
#define ECC40bitPer1K_bit_count 560
#define ECC60bitPer1K_bit_count 830
#define ECC44bitPer1K_bit_count 616
#define ECC44bit_bit_count 576
#define ECC1bit_byte_count 4
#define ECC4bit_byte_count 8
#define ECC8bit_byte_count 16
#define ECC12bit_byte_count 20
#define ECC16bit_byte_count 26
#define ECC24bitPer1K_byte_count 42
#define ECC40bitPer1K_byte_count 70
#define ECC60bitPer1K_byte_count 106
#define ECC44bitPer1K_byte_count 77
#define ECC44bit_byte_count 72
#define ECC1bit_unprotect 0
#define ECC4bit_unprotect 0
#define ECC8bit_unprotect 50
#define ECC12bit_unprotect 14
#define ECC16bit_unprotect 26
#define ECC24bitPer1K_unprotect 34
#define ECC40bitPer1K_unprotect 14
#define ECC60bitPer1K_unprotect 46	//8k+960
#define ECC44bitPer1K_unprotect 14
#define ECC44bit_unprotect 72
#define MAX_BANK_SIZE   1024
#define MAX_PARITY_SIZE 106
#define MAX_ECC_BIT_ERROR 60
/*
 *	NAND PDMA
 */
#define NAND_DESC_BASE_ADDR 0x00D00000

#define NFC_DMA_GCR			0x100
#define NFC_DMA_IER			0x104
#define NFC_DMA_ISR			0x108
#define NFC_DMA_DESPR		0x10C
#define NFC_DMA_RBR			0x110
#define NFC_DMA_DAR			0x114
#define NFC_DMA_BAR			0x118
#define NFC_DMA_CPR			0x11C
#define NFC_DMA_CCR			0X120

#define NAND_GET_FEATURE  0xEE
#define NAND_SET_FEATURE  0xEF
/*
 *	NAND PDMA - DMA_GCR : DMA Global Control Register
 */
#define NAND_PDMA_GCR_DMA_EN			0x00000001      /* [0] -- DMA controller enable */
#define NAND_PDMA_GCR_SOFTRESET		0x00000100      /* [8] -- Software rest */

/*
 *	NAND PDMA - DMA_IER : DMA Interrupt Enable Register
 */
#define NAND_PDMA_IER_INT_EN			0x00000001      /* [0] -- DMA interrupt enable */
/*
 *	NAND PDMA - DMA_ISR : DMA Interrupt Status Register
 */
#define NAND_PDMA_IER_INT_STS			0x00000001      /* [0] -- DMA interrupt status */
/*
 *	NAND PDMA - DMA_DESPR : DMA Descriptor base address Pointer Register
 */

/*
 *	NAND PDMA - DMA_RBR : DMA Residual Bytes Register
 */
#define NAND_PDMA_RBR_End				0x80000000      /* [31] -- DMA descriptor end flag */
#define NAND_PDMA_RBR_Format			0x40000000      /* [30] -- DMA descriptor format */
/*
 *	NAND PDMA - DMA_DAR : DMA Data Address Register
 */

/*
 *	NAND PDMA - DMA_BAR : DMA Rbanch Address Register
 */

/*
 *	NAND PDMA - DMA_CPR : DMA Command Pointer Register
 */

/*
 *	NAND PDMA - DMA_CCR : DMAContext Control Register for Channel 0
 */
#define NAND_PDMA_READ					0x00
#define NAND_PDMA_WRITE					0x01
#define NAND_PDMA_CCR_RUN					0x00000080
#define NAND_PDMA_CCR_IF_to_peripheral	0x00000000
#define NAND_PDMA_CCR_peripheral_to_IF	0x00400000
#define NAND_PDMA_CCR_EvtCode				0x0000000f
#define NAND_PDMA_CCR_Evt_no_status		0x00000000
#define NAND_PDMA_CCR_Evt_ff_underrun		0x00000001
#define NAND_PDMA_CCR_Evt_ff_overrun		0x00000002
#define NAND_PDMA_CCR_Evt_desp_read		0x00000003
#define NAND_PDMA_CCR_Evt_data_rw			0x00000004
#define NAND_PDMA_CCR_Evt_early_end		0x00000005
#define NAND_PDMA_CCR_Evt_success			0x0000000f

/*
 *	PDMA Descriptor short
 */
struct _NAND_PDMA_DESC_S{
	unsigned int volatile ReqCount : 16;		/* bit 0 -15 -Request count             */
	unsigned int volatile i : 1;				/* bit 16    -interrupt                 */
	unsigned int volatile reserve : 13;		/* bit 17-29 -reserved                  */
	unsigned int volatile format : 1;		/* bit 30    -The descriptor format     */
	unsigned int volatile end : 1;			/* bit 31    -End flag of descriptor list*/
	unsigned int volatile DataBufferAddr : 32;/* bit 31    -Data Buffer address       */
};

/*
 *	PDMA Descriptor long
 */
struct _NAND_PDMA_DESC_L{
	unsigned long volatile ReqCount : 16;		/* bit 0 -15 -Request count             */
	unsigned long volatile i : 1;				/* bit 16    -interrupt                 */
	unsigned long volatile reserve : 13;		/* bit 17-29 -reserved                  */
	unsigned long volatile format : 1;		/* bit 30    -The descriptor format     */
	unsigned long volatile end : 1;			/* bit 31    -End flag of descriptor list*/
	unsigned long volatile DataBufferAddr : 32;/* bit 31-0  -Data Buffer address       */
	unsigned long volatile BranchAddr : 32;	/* bit 31-2  -Descriptor Branch address	*/
	unsigned long volatile reserve0 : 32;		/* bit 31-0  -reserved */
};

struct NFC_RW_T {
	unsigned int	T_R_setup;
	unsigned int	T_R_hold;
	unsigned int	T_W_setup;
	unsigned int	T_W_hold;
	unsigned int	divisor;
	unsigned int	T_TADL;
	unsigned int	T_TWHR;
	unsigned int	T_TWB;
	unsigned int	T_RHC_THC;
};

/* cfg_1 */
#define TWHR 0x800
#define OLD_CMD 0x400 /* enable old command trigger mode */
#define DPAHSE_DISABLE 0x80 /*disable data phase */
#define NAND2NFC  0x40  /* direction : nand to controller */
#define SING_RW	0x20 /*	enable signal read/ write command */
#define MUL_CMDS 0x10  /*  support cmd+addr+cmd */
#define NFC_TRIGGER 0x01 /* start cmd&addr sequence */
/*cfg_9 */
#define READ_RESUME 1 //0x100
#define BCH_INT_EN  0x60
#define BANK_DR     0x10
#define DIS_BCH_ECC 0x08
#define USE_HW_ECC 0
#define ECC_MODE 7

/*cfg_a */
#define NFC_CMD_RDY 0x04
#define NFC_BUSY	0x02  /* command and data is being transfer in flash I/O */
#define FLASH_RDY 0x01  /* flash is ready */
/*cfg_b */
#define B2R	0x08 /* status form busy to ready */
#define ERR_CORRECT 0x2
#define BCH_ERR     0x1
/*cfg_d */
#define HIGH64FIFO 8 /* read high 64 bytes fifo */
#define OOB_READ	0x4 /* calculate the side info BCH decoder */
#define RED_DIS		0x2 /* do not read out oob area data to FIFO */
/*cfg_f */
#define RDMZ 0x10000 /* enable randomizer */
#define RDMZH 1	/* enable randomizer */
/*cfg_12 */
#define PAGE_512 0
#define PAGE_2K 1
#define PAGE_4K 2
#define PAGE_8K 3
#define PAGE_16K 4
#define PAGE_32K 5
#define WD8 0
#define WIDTH_16	(1<<3)
#define WP_DISABLE (1<<4) /*disable write protection */
#define DIRECT_MAP (1<<5)
#define RD_DLY (1<<6)
#define TOGGLE (1<<7)

/*cfg_13*/				 /* Dannier Add */
#define B2RMEN	0x08	 /* interrupt mask enable of nand flash form busy to ready */
/*cfg_15 */
#define NFC_IDLE	0x01
/*cfg_17 status */
#define BANK_NUM    0x1F00
#define BCH_ERR_CNT 0x3F
/*cfg_18 */
#define BCH_ERRPOS0 0x3fff
#define BCH_ERRPOS1 (BCH_ERRPOS0<<16)

#define ADDR_COLUMN 1
#define ADDR_PAGE 2
#define ADDR_COLUMN_PAGE 3
#define WRITE_NAND_COMMAND(d, adr) do { *(volatile unsigned char *)(adr) = (unsigned char)(d); } while (0)
#define WRITE_NAND_ADDRESS(d, adr) do { *(volatile unsigned char *)(adr) = (unsigned char)(d); } while (0)


#define SOURCE_CLOCK 24
#define MAX_SPEED_MHZ 60
#define MAX_READ_DELAY 9 /* 8.182 = tSKEW 3.606 + tDLY 4.176 + tSETUP 0.4 */
#define MAX_WRITE_DELAY 9 /* 8.72 = tDLY 10.24 - tSKEW 1.52*/

#define DMA_SINGNAL 0
#define DMA_INC4 0x10
#define DMA_INC8 0x20
/*#define first4k218 0
#define second4k218 4314 *//* 4096 + 218 */
#define NFC_TIMEOUT_TIME (HZ*8)

int nand_init_pdma(struct mtd_info *mtd);
int nand_free_pdma(struct mtd_info *mtd);
int nand_alloc_desc_pool(unsigned int *DescAddr);
int nand_init_short_desc(unsigned int *DescAddr, unsigned int ReqCount, unsigned int *BufferAddr, int End);
int nand_init_long_desc(unsigned long *DescAddr, unsigned int ReqCount, unsigned long *BufferAddr,
unsigned long *BranchAddr, int End);
int nand_config_pdma(struct mtd_info *mtd, unsigned long *DescAddr, unsigned int dir);
int nand_pdma_handler(struct mtd_info *mtd);
void nand_hamming_ecc_1bit_correct(struct mtd_info *mtd);
void bch_data_ecc_correct(struct mtd_info *mtd);
void bch_redunt_ecc_correct(struct mtd_info *mtd);
void bch_data_last_bk_ecc_correct(struct mtd_info *mtd);
void copy_filename (char *dst, char *src, int size);
int wmt_getsyspara(char *varname, unsigned char *varval, int *varlen);
int wmt_setsyspara(char *varname, char *varval);
void calculate_ECC_info(struct mtd_info *mtd, struct ECC_size_info *ECC_size);
/*unsigned int wmt_bchencoder (unsigned char *data, unsigned char *bch_code, unsigned char bits, unsigned char *bch_codelen, unsigned int encode_len);*/
int bch_encoder(unsigned int *p_parity, u32 *p_data, u8 bits, u32 datacnt);
unsigned int Caculat_1b_bch( unsigned int *pariA, unsigned int *bch_GF2, unsigned int din, u8 pari_len, u8 pari_lb);
int Gen_GF2(u8 bits, unsigned int  *buf);
int nand_get_feature(struct mtd_info *mtd, int addr);
int nand_set_feature(struct mtd_info *mtd, int cmd, int addrss, int value);
void rdmzier(uint8_t *buf, int size, int page);
void rdmzier_oob(uint8_t *buf, uint8_t *src, int size, int page, int ofs);
int nand_hynix_get_retry_reg(struct mtd_info *mtd, uint8_t *addr, uint8_t *para, int size);
int nand_hynix_set_retry_reg(struct mtd_info *mtd, int reg);
int write_bytes_cmd(struct mtd_info *mtd, int cmd_cnt, int addr_cnt, int data_cnt, uint8_t *cmd, uint8_t *addr, uint8_t *data);
int hynix_read_retry_set_para(struct mtd_info *mtd, int reg);
int load_hynix_opt_reg(struct mtd_info *mtd, struct nand_chip *chip);
void wmt_init_nfc(struct mtd_info *mtd, unsigned int spec_clk, unsigned int spec_tadl, int busw);
void set_ecc_info(struct mtd_info *mtd);
int alloc_rdmz_buffer(struct mtd_info *mtd);
void set_partition_size(struct mtd_info *mtd);
int get_flash_info_from_env(unsigned int id, unsigned int id2, struct WMT_nand_flash_dev *type);

#define REG_SEED 1
#define BYTE_SEED 2112



#endif
