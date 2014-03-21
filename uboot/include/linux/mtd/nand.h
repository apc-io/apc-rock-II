/*
 *  linux/include/linux/mtd/nand.h
 *
 *  Copyright (c) 2000 David Woodhouse <dwmw2@mvhi.com>
 *                     Steven J. Hill <sjhill@cotw.com>
 *		       Thomas Gleixner <gleixner@autronix.de>
 *
 * $Id: nand.h,v 1.7 2003/07/24 23:30:46 a0384864 Exp $
 *
 *  Copyright (c) 2008 WonderMedia Technologies, Inc.
 *
 *  This program is free software: you can redistribute it and/or modify it under the
 *  terms of the GNU General Public License as published by the Free Software Foundation,
 *  either version 2 of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful, but WITHOUT
 *  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 *  PARTICULAR PURPOSE.  See the GNU General Public License for more details.
 *  You should have received a copy of the GNU General Public License along with
 *  this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *  WonderMedia Technologies, Inc.
 *  10F, 529, Chung-Cheng Road, Hsin-Tien, Taipei 231, R.O.C.
 *
 *  Info:
 *   Contains standard defines and IDs for NAND flash devices
 *
 *  Changelog:
 *   01-31-2000 DMW     Created
 *   09-18-2000 SJH     Moved structure out of the Disk-On-Chip drivers
 *			so it can be used by other NAND flash device
 *			drivers. I also changed the copyright since none
 *			of the original contents of this file are specific
 *			to DoC devices. David can whack me with a baseball
 *			bat later if I did something naughty.
 *   10-11-2000 SJH     Added private NAND flash structure for driver
 *   10-24-2000 SJH     Added prototype for 'nand_scan' function
 *   10-29-2001 TG	changed nand_chip structure to support
 *			hardwarespecific function for accessing control lines
 *   02-21-2002 TG	added support for different read/write adress and
 *			ready/busy line access function
 *   02-26-2002 TG	added chip_delay to nand_chip structure to optimize
 *			command delay times for different chips
 *   04-28-2002 TG	OOB config defines moved from nand.c to avoid duplicate
 *			defines in jffs2/wbuf.c
 */
#ifndef __LINUX_MTD_NAND_H
#define __LINUX_MTD_NAND_H

/*
 * Standard NAND flash commands
 */
#define NAND_CMD_READ0		0
#define NAND_CMD_READ1		1
#define NAND_CMD_PAGEPROG	0x10
#define NAND_CMD_READOOB	0x50
#define NAND_CMD_ERASE1		0x60
#define NAND_CMD_STATUS		0x70
#define NAND_CMD_SEQIN		0x80
#define NAND_CMD_READID		0x90
#define NAND_CMD_ERASE2		0xd0
#define NAND_CMD_RESET		0xff

/*
 * Enumeration for NAND flash chip state
 */
enum nand_state_t {
	FL_READY,
	FL_READING,
	FL_WRITING,
	FL_ERASING,
	FL_SYNCING
};

/*
 * NAND Private Flash Chip Data
 *
 * Structure overview:
 *
 *  IO_ADDR - address to access the 8 I/O lines of the flash device
 *
 *  hwcontrol - hardwarespecific function for accesing control-lines
 *
 *  dev_ready - hardwarespecific function for accesing device ready/busy line
 *
 *  chip_lock - spinlock used to protect access to this structure
 *
 *  wq - wait queue to sleep on if a NAND operation is in progress
 *
 *  state - give the current state of the NAND device
 *
 *  page_shift - number of address bits in a page (column address bits)
 *
 *  data_buf - data buffer passed to/from MTD user modules
 *
 *  data_cache - data cache for redundant page access and shadow for
 *		 ECC failure
 *
 *  ecc_code_buf - used only for holding calculated or read ECCs for
 *                 a page read or written when ECC is in use
 *
 *  reserved - padding to make structure fall on word boundary if
 *             when ECC is in use
 */

#ifdef CFG_WMT_NFC

#define __NFC_BASE 0xd8009000

struct WMT_NFC_t {
	struct WMT_NFC_CFG	*const reg; /* register set */
	unsigned int	ecc_type;
};

struct WMT_NFC_t nfc = {
	(struct WMT_NFC_CFG *)(__NFC_BASE),
#ifdef CONFIG_NFC_HW_ECC
	USE_HW_ECC
#else
	USE_SW_ECC
#endif
};

#endif

struct Nand {
	char floor, chip;
	unsigned long curadr;
	unsigned char curmode;
	/* Also some erase/write/pipeline info when we get that far */
};

struct nand_oobinfo {
	unsigned int useecc;
	unsigned int eccbytes;
	unsigned int oobfree[8][2];
	unsigned int eccpos[48];
};

struct nand_oobfree {
	unsigned int offset;
	unsigned int length;
};

struct nand_ecclayout {
	unsigned int eccbytes;
	unsigned int eccpos[128];
	unsigned int oobavail;
	struct nand_oobfree oobfree[8];	
};

struct nand_ecc_ctrl {
	unsigned int steps;
	unsigned int size;
	unsigned int bytes;
	unsigned int total;
	struct nand_ecclayout *layout;
};

#define DWORD	unsigned int
#define MAX_PRODUCT_NAME_LENGTH 0x20

struct nand_chip {
	DWORD dwFlashID;       /*composed by the first byte and the second byte*/
	DWORD dwBlockCount;      /*how many blocks one chip have*/
	DWORD dwPageSize;       /*page size*/
	
	DWORD dwSpareSize;       /*spare area size*/
	DWORD dwBlockSize;       /*block size*/
	DWORD dwAddressCycle;      /*address cycle 4 or 5*/
	DWORD dwBI0Position;      /*BI0 page postion in block*/
	DWORD dwBI1Position;      /*BI1 page postion in block*/
	DWORD dwBIOffset;       /*BI offset in page*/
	DWORD dwDataWidth;      /*data with X8 or X16*/
	DWORD dwPageProgramLimit;     /*chip can program PAGE_PROGRAM_LIMIT times within the same page*/
	DWORD dwSeqRowReadSupport;    /*whether support sequential row read, 1 = support 0 = not support*/
	DWORD dwSeqPageProgram;     /*chip need sequential page program in a block. 1 = need*/
	DWORD dwNandType;       /*MLC or SLC*/
	DWORD dwECCBitNum;      /*ECC bit number needed*/
	DWORD dwRWTimming;
	DWORD dwTwbTwhrTadl;      		//NFC write TADL timeing config
	DWORD dwDDR;      		  //NFC support toshia ddr(toggle) mode = 1 not support = 0
	DWORD dwRetry;    		  //NFC Read Retry support = 1 no
	int dwRdmz; 			//NFC Randomizer support = 1,
	DWORD dwFlashID2;      //composed by additional 2 bytes of ID from original 4 bytes.
	DWORD dwSpeedUpCmd;		/* supprot speed up nand command ex: 2-plane cache read and so on.. */
	char cProductName[MAX_PRODUCT_NAME_LENGTH]; /*product name. for example "HYNIX_NF_HY27UF081G2A"*/
	/*other informations*/
	unsigned int  col; /* addr_cycle; */
	unsigned int  row; /* page_cycle; */

	unsigned int col_offset;
	//unsigned int 	page[2];
	//unsigned int  bbpos;
	//struct WMT_nand_flash_dev *nand_dev_info;
	int (*nfc_read_page)(struct nand_chip *nand, unsigned int start, unsigned int maddr, unsigned int len);
	int (*update_table_inflash)(struct nand_chip *nand, unsigned int last, int chip);
	int (*update_table_inram)(struct nand_chip *nand, unsigned int addr, int chip);
	int (*isbadblock)(struct nand_chip *nand, unsigned int addr, int chip);
	DWORD dwPageSizeControl;
	DWORD dwPageCount;
	DWORD dwSectorCount;
	DWORD dwECCType;
	DWORD dwSectorSize;
	int nChipOffset;
	int nChipCnt;
	DWORD dwSectorCntPerPage;
	DWORD dwTBLItemCntPerPage;     /* 4 blocks for 1 item*/
	DWORD dwReservedBlockCnt;
	DWORD dwECCENGINBitNum;
	/*------------above is defined by WMT---------------------------*/
	/*------------below is defined by open source-------------------*/
	int oob_flag;// write oob data in page program function
	int pagebuf;
	int    page_shift;
	u_char *data_buf;
	u_char *data_cache;
	int    cache_page;
	u_char ecc_code_buf[6];
	u_char *buffers;
	u_char *oob_poi;
	struct nand_ecc_ctrl ecc;
	u_char reserved[2];
	char   ChipID; /* Type of DiskOnChip */
	struct Nand *chips;
	int  chipshift;
	char *chips_name;
	unsigned long erasesize;
	unsigned long mfr; /* Flash IDs - only one type of flash per device */
	unsigned long id;
	unsigned int  id2;
	char *name;
	int  numchips;
	char page256;
	char pageadrlen;
	unsigned long IO_ADDR;  /* address to access the 8 I/O lines to the flash device */
	long long totlen;
	uint oobblock;  /* Size of OOB blocks (e.g. 512) */
	uint oobsize;   /* Amount of OOB data per block (e.g. 16) */
	uint eccsize;
	uint ooblen;
	int bus16;
};

//#define BOOT_MAGIC "ANDROID!"
#define BOOT_MAGIC_SIZE 8
#define BOOT_NAME_SIZE 16
#define BOOT_ARGS_SIZE 512
struct boot_img_hdr
{
    unsigned char magic[BOOT_MAGIC_SIZE];

    unsigned kernel_size;  /* size in bytes */
    unsigned kernel_addr;  /* physical load addr */

    unsigned ramdisk_size; /* size in bytes */
    unsigned ramdisk_addr; /* physical load addr */

    unsigned second_size;  /* size in bytes */
    unsigned second_addr;  /* physical load addr */

    unsigned tags_addr;    /* physical addr for kernel tags */
    unsigned page_size;    /* flash page size we assume */
    unsigned unused[2];    /* future expansion: should be 0 */

    unsigned char name[BOOT_NAME_SIZE]; /* asciiz product name */
    
    unsigned char cmdline[BOOT_ARGS_SIZE];

    unsigned id[8]; /* timestamp / checksum / sha1 / etc */
};

// read retry
#define ESLC_MODE 0
#define READ_RETRY_MODE 1
#define TEST_MODE 2
#define DEFAULT_VALUE 0
#define ECC_ERROR_VALUE 1

//All
struct nand_read_retry_param {
	char magic[32];
	unsigned int  nand_id;
	unsigned int  nand_id_5th;
	unsigned int  eslc_reg_num;
	unsigned char eslc_offset[32];
	unsigned char eslc_def_value[32];
	unsigned char eslc_set_value[32];
	unsigned int  retry_reg_num;
	unsigned char  retry_offset[32];
	unsigned char retry_def_value[32];
	unsigned char retry_value[256];
	unsigned int otp_len;
	unsigned char otp_offset[32];
	unsigned char otp_data[32];
	unsigned int total_try_times;
	int cur_try_times;
	int (*set_parameter)(struct nand_chip *nand, int mode, int def_mode);
	int (*get_parameter)(struct nand_chip *nand, int mode);
	int (*get_otp_table)(struct nand_chip *nand);
	int retry; //1: in retry mode 
	int read_table;
};

/*
 * NAND Flash Manufacturer ID Codes
 */
#define NAND_MFR_TOSHIBA	0x98
#define NAND_MFR_SAMSUNG	0xec
#define NAND_MFR_HYNIX    0xad

/*
 * NAND Flash Device ID Structure
 *
 * Structure overview:
 *
 *  name - Complete name of device
 *
 *  manufacture_id - manufacturer ID code of device.
 *
 *  model_id - model ID code of device.
 *
 *  chipshift - total number of address bits for the device which
 *              is used to calculate address offsets and the total
 *              number of bytes the device is capable of.
 *
 *  page256 - denotes if flash device has 256 byte pages or not.
 *
 *  pageadrlen - number of bytes minus one needed to hold the
 *               complete address into the flash array. Keep in
 *               mind that when a read or write is done to a
 *               specific address, the address is input serially
 *               8 bits at a time. This structure member is used
 *               by the read/write routines as a loop index for
 *               shifting the address out 8 bits at a time.
 *
 *  erasesize - size of an erase block in the flash device.
 */
struct nand_flash_dev {
	char *name;
	int manufacture_id;
	int model_id;
	int chipshift;
	char page256;
	char pageadrlen;
	unsigned long erasesize;
	int bus16;
};



/* Nand Product Table          We take HYNIX_NF_HY27UF081G2A for example.*/

struct WMT_nand_flash_dev{
 DWORD dwFlashID;            //composed by 4 bytes of ID. For example:0xADF1801D
 DWORD dwBlockCount;      //block count of one chip. For example: 1024
 DWORD dwPageSize;       //page size. For example:2048(other value can be 512 or 4096)
 DWORD dwSpareSize;       //spare area size. For example:16(almost all kinds of nand is 16)
 DWORD dwBlockSize;       //block size = dwPageSize * PageCntPerBlock. For example:131072
 DWORD dwAddressCycle;      //address cycle 4 or 5
 DWORD dwBI0Position;      //BI0 page postion in block
 DWORD dwBI1Position;      //BI1 page postion in block
 DWORD dwBIOffset;       //BI offset in page
 DWORD dwDataWidth;      //data with X8 or X16
 DWORD dwPageProgramLimit;     //chip can program PAGE_PROGRAM_LIMIT times within the same page
 DWORD dwSeqRowReadSupport;    //whether support sequential row read, 1 = support 0 = not support
 DWORD dwSeqPageProgram;     //chip need sequential page program in a block. 1 = need
 DWORD dwNandType;       //MLC or SLC
 DWORD dwECCBitNum;      //ECC bit number needed
 DWORD dwRWTimming;     //NFC Read/Write Pulse width and Read/Write hold time. default =0x12121010
 DWORD dwTwbTwhrTadl;   //NFC byte 3: TWB, byte 2:TWHR, byte 1-0: TADL (refer flash datasheet).
 DWORD dwDDR;      		  //NFC support toshia ddr(toggle) mode = 1 not support = 0
 DWORD dwRetry;    		  //NFC Read Retry support = 1 no
 DWORD dwRdmz; 			//NFC Randomizer support = 1,
 DWORD dwFlashID2;      //composed by additional 2 bytes of ID from original 4 bytes.
 DWORD dwSpeedUpCmd;		/* supprot speed up nand command ex: 2-plane cache read and so on.. */
 char ProductName[MAX_PRODUCT_NAME_LENGTH]; //product name. for example "HYNIX_NF_HY27UF081G2A"
};


/*
* Constants for oob configuration
*/
#define NAND_NOOB_ECCPOS0		0
#define NAND_NOOB_ECCPOS1		1
#define NAND_NOOB_ECCPOS2		2
#define NAND_NOOB_ECCPOS3		3
#define NAND_NOOB_ECCPOS4		6
#define NAND_NOOB_ECCPOS5		7
#define NAND_NOOB_BADBPOS		-1
#define NAND_NOOB_ECCVPOS		-1

#define NAND_JFFS2_OOB_ECCPOS0		0
#define NAND_JFFS2_OOB_ECCPOS1		1
#define NAND_JFFS2_OOB_ECCPOS2		2
#define NAND_JFFS2_OOB_ECCPOS3		3
#define NAND_JFFS2_OOB_ECCPOS4		6
#define NAND_JFFS2_OOB_ECCPOS5		7
#define NAND_JFFS2_OOB_BADBPOS		5
#define NAND_JFFS2_OOB_ECCVPOS		4

#define NAND_JFFS2_OOB8_FSDAPOS		6
#define NAND_JFFS2_OOB16_FSDAPOS	8
#define NAND_JFFS2_OOB8_FSDALEN		2
#define NAND_JFFS2_OOB16_FSDALEN	8

unsigned long nand_probe(unsigned long physadr);
int hynix_set_parameter(struct nand_chip *nand, int mode, int def_mode);
int hynix_get_parameter(struct nand_chip *nand, int mode);
int hynix_get_otp(struct nand_chip *nand);
int toshiba_get_parameter(struct nand_chip *nand, int mode);
int toshiba_set_parameter(struct nand_chip *nand, int mode, int def);

#endif /* __LINUX_MTD_NAND_H */
