/*
 * (C) Copyright 2008 - 2009
 * Windriver, <www.windriver.com>
 * Tom Rix <Tom.Rix@windriver.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 *
 * The logical naming of flash comes from the Android project
 * Thse structures and functions that look like fastboot_flash_*
 * They come from bootloader/legacy/include/boot/flash.h
 *
 * The boot_img_hdr structure and associated magic numbers also
 * come from the Android project.  They are from
 * system/core/mkbootimg/bootimg.h
 *
 * Here are their copyrights
 *
 * Copyright (C) 2008 The Android Open Source Project
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#ifndef FASTBOOT_H
#define FASTBOOT_H

#include <common.h>
#include <command.h>

/* This is the interface file between the common cmd_fastboot.c and
   the board specific support.

   To use this interface, define CONFIG_FASTBOOT in your board config file.
   An example is include/configs/omap3430labrador.h
   ...
   #define CONFIG_FASTBOOT	        1    / * Using fastboot interface * /
   ...

   An example of the board specific spupport for omap3 is found at
   cpu/omap3/fastboot.c

*/

/* From fastboot client.. */
#define FASTBOOT_INTERFACE_CLASS     0xff
#define FASTBOOT_INTERFACE_SUB_CLASS 0x42
#define FASTBOOT_INTERFACE_PROTOCOL  0x03

#define FASTBOOT_VERSION "0.5"

/* The fastboot client uses a value of 2048 for the
   page size of it boot.img file format.
   Reset this in your board config file as needed. */
#ifndef CONFIG_FASTBOOT_MKBOOTIMAGE_PAGE_SIZE
#define CONFIG_FASTBOOT_MKBOOTIMAGE_PAGE_SIZE 2048
#endif

/* Fastboot bulk packet sizes */
#define	CONFIG_USBD_FASTBOOT_BULK_PKTSIZE_HS	512
#define	CONFIG_USBD_FASTBOOT_BULK_PKTSIZE_FS	64

/* Flags */
#define	FASTBOOT_FLAG_RESPONSE	1
#define	FASTBOOT_FLAG_DOWNLOAD	1

/* To change design of memory handling (SD/MMC, NAND) faster, if required */

//#define FASTBOOT_PORT_WMT_NAND_FLASHING  0  //Charles
//#define FASTBOOT_PORT_WMT_EMMC_FLASHING  1  //Charles

/* To activate-deactivate fastboot upload command (not part of OmapZoom) */
// #define	FASTBOOT_UPLOAD
//typedef enum { NAND=0, EMMC=1} storage_medium_type;
#define NAND 0
#define EMMC 1

#define FASTBOOT_RESPONSE_BUF_SZ 64

struct cmd_fastboot_interface{

	/* A getvar string for the product name
	   It can have a maximum of 60 characters

	   Set by board	*/
	char *product_name;

	/* A getvar string for the serial number
	   It can have a maximum of 60 characters

	   Set by board */
	char *serial_no;

	/* Nand block size
	   Supports the write option WRITE_NEXT_GOOD_BLOCK

	   Set by board */
	unsigned int nand_block_size;

	/* Nand oob size
	   Set by board */
	unsigned int nand_oob_size;

	/* Transfer buffer, for handling flash updates
	   Should be multiple of the nand_block_size
	   Care should be take so it does not overrun bootloader memory
	   Controlled by the configure variable CONFIG_FASTBOOT_TRANSFER_BUFFER

	   Set by board */
	unsigned char *transfer_buffer;

	/* How big is the transfer buffer
	   Controlled by the configure variable
	   CONFIG_FASTBOOT_TRANSFER_BUFFER_SIZE

	   Set by board	*/
	unsigned int transfer_buffer_size;

	/* Download size, if download has to be done. This can be checked to find
		whether next packet is a command or a data */
	unsigned int d_size;

	/* Data downloaded so far */
	unsigned int d_bytes;

	/* Upload size, if download has to be done */
	unsigned int u_size;

	/* Data uploaded so far */
	unsigned int u_bytes;

	/* XXX: what should be the size, 64 or 65 ? */
	//char response[65];
	char response[FASTBOOT_RESPONSE_BUF_SZ+1];

	/* Indicate response to be sent, data to be recieved */
	unsigned int flag;

	/* configuration status  */
	unsigned int configured;

//#ifdef	FASTBOOT_PORT_WMT_NAND_FLASHING
	unsigned int download_bytes_unpadded;
//#endif

	unsigned int exit;
	//storage_medium_type storage_medium;
	unsigned char storage_medium;
};

/* Android-style flash naming */
typedef struct fastboot_ptentry fastboot_ptentry;

/* flash partitions are defined in terms of blocks
** (flash erase units)
*/
struct fastboot_ptentry{

	/* The logical name for this partition, null terminated */
	char name[16];
	/* The start wrt the nand part, must be multiple of nand block size */
	__u64 start;
	/* The length of the partition, must be multiple of nand block size */
	__u64 length;
	/* Controls the details of how operations are done on the partition
	   See the FASTBOOT_PTENTRY_FLAGS_*'s defined below */
	unsigned int flags;
};

typedef struct system_entry system_entry;
/*define for system image*/
struct system_entry{

	char name[16];
	unsigned int image_start;
	unsigned int image_size;
	unsigned int sig_start;
	unsigned int sig_length;
	unsigned int flash_start;
	unsigned int flash_end;
	unsigned int flags;
};


/* Lower byte shows if the read/write/erase operation in
   repeated.  The base address is incremented.
   Either 0 or 1 is ok for a default */

#define	FASTBOOT_PTENTRY_FLAGS_REPEAT_MASK(n)	(n & 0x0f)
#define	FASTBOOT_PTENTRY_FLAGS_REPEAT_4		0x00000004

/* Writes happen a block at a time.
   If the write fails, go to next block
   NEXT_GOOD_BLOCK and CONTIGOUS_BLOCK can not both be set */
#define FASTBOOT_PTENTRY_FLAGS_WRITE_NEXT_GOOD_BLOCK  0x00000010

/* Find a contiguous block big enough for a the whole file
   NEXT_GOOD_BLOCK and CONTIGOUS_BLOCK can not both be set */
#define FASTBOOT_PTENTRY_FLAGS_WRITE_CONTIGUOUS_BLOCK 0x00000020

/* Sets the ECC to software before writing
   HW and SW ECC should not both be set. */
#define FASTBOOT_PTENTRY_FLAGS_WRITE_SW_ECC           0x00000040

/* Sets the ECC to hardware before writing
   HW and SW ECC should not both be set. */
#define FASTBOOT_PTENTRY_FLAGS_WRITE_HW_ECC           0x00000080

/* Sets the ECC to hardware before writing
   HW and SW ECC should not both be set. */
#define FASTBOOT_PTENTRY_FLAGS_WRITE_HW_BCH4_ECC      0x00000100

/* Sets the ECC to hardware before writing
   HW and SW ECC should not both be set. */
#define FASTBOOT_PTENTRY_FLAGS_WRITE_HW_BCH8_ECC      0x00000200

/* Sets the ECC to hardware before writing
   HW and SW ECC should not both be set. */
#define FASTBOOT_PTENTRY_FLAGS_WRITE_HW_BCH16_ECC     0x00000400

/* Write the file with write.i */
#define FASTBOOT_PTENTRY_FLAGS_WRITE_I                0x00000800

/* Write the file with write.jffs2 */
#define FASTBOOT_PTENTRY_FLAGS_WRITE_JFFS2            0x00001000

/* Write the file as a series of variable/value pairs
   using the setenv and saveenv commands */
#define FASTBOOT_PTENTRY_FLAGS_WRITE_ENV              0x00002000

/* Status values */
#define FASTBOOT_OK			0
#define FASTBOOT_ERROR			-1
#define FASTBOOT_DISCONNECT		1
#define FASTBOOT_INACTIVE		2

/* Android bootimage file format */
#define FASTBOOT_BOOT_MAGIC            "ANDROID!"
#define FASTBOOT_BOOT_MAGIC_SIZE       8
#define FASTBOOT_BOOT_NAME_SIZE        16
#define FASTBOOT_BOOT_ARGS_SIZE        512

struct fastboot_boot_img_hdr {
	unsigned char magic[FASTBOOT_BOOT_MAGIC_SIZE];

	unsigned kernel_size;  /* size in bytes */
	unsigned kernel_addr;  /* physical load addr */

	unsigned ramdisk_size; /* size in bytes */
	unsigned ramdisk_addr; /* physical load addr */

	unsigned second_size;  /* size in bytes */
	unsigned second_addr;  /* physical load addr */

	unsigned tags_addr;    /* physical addr for kernel tags */
	unsigned page_size;    /* flash page size we assume */
	unsigned unused[2];    /* future expansion: should be 0 */

	unsigned char name[FASTBOOT_BOOT_NAME_SIZE]; /* asciiz product name */

	unsigned char cmdline[FASTBOOT_BOOT_ARGS_SIZE];

	unsigned id[8]; /* timestamp / checksum / sha1 / etc */
};

#ifdef	CONFIG_CMD_FASTBOOT
/* A board specific test if u-boot should go into the fastboot command
   ahead of the bootcmd
   Returns 0 to continue with normal u-boot flow
   Returns 1 to execute fastboot */
extern int fastboot_preboot(void);

/* Initizes the board specific fastboot
   Returns 0 on success
   Returns 1 on failure */
extern int fastboot_init(struct cmd_fastboot_interface *interface);

/* Cleans up the board specific fastboot */
extern void fastboot_shutdown(void);

/*
 * Handles board specific usb protocol exchanges
 * Returns 0 on success
 * Returns 1 on disconnects, break out of loop
 * Returns 2 if no USB activity detected
 * Returns -1 on failure, unhandled usb requests and other error conditions
*/
extern int fastboot_poll(void);

/* Is this high speed (2.0) or full speed (1.1) ?
   Returns 0 on full speed
   Returns 1 on high speed */
extern int fastboot_is_highspeed(void);

/* Return the size of the fifo */
extern int fastboot_fifo_size(void);

/* Send a status reply to the client app
   buffer does not have to be null terminated.
   buffer_size must be not be larger than what is returned by
   fastboot_fifo_size
   Returns 0 on success
   Returns 1 on failure */
extern int fastboot_tx_status(const char *buffer, unsigned int buffer_size);

/*
 * Send some data to the client app
 * buffer does not have to be null terminated.
 * buffer_size can be larger than what is returned by
 * fastboot_fifo_size
 * Returns number of bytes written
 */
extern int fastboot_tx(unsigned char *buffer, unsigned int buffer_size);

/* A board specific variable handler.
   The size of the buffers is governed by the fastboot spec.
   rx_buffer is at most 57 bytes
   tx_buffer is at most 60 bytes
   Returns 0 on success
   Returns 1 on failure */
extern int fastboot_getvar(const char *rx_buffer, char *tx_buffer);

/* The Android-style flash handling */

/* tools to populate and query the partition table */
extern void fastboot_flash_reset_ptn(void);

extern void fastboot_flash_add_ptn(fastboot_ptentry *ptn);
extern fastboot_ptentry *fastboot_flash_find_ptn(const char *name);
extern fastboot_ptentry *fastboot_flash_get_ptn(unsigned n);
extern unsigned int fastboot_flash_get_ptn_count(void);
extern void fastboot_flash_dump_ptn(void);

extern int fastboot_flash_init(void);
extern int fastboot_flash_erase(fastboot_ptentry *ptn);
extern int fastboot_flash_read_ext(fastboot_ptentry *ptn,
				   unsigned extra_per_page, unsigned offset,
				   void *data, unsigned bytes);
#define fastboot_flash_read(ptn, offset, data, bytes) \
  flash_read_ext(ptn, 0, offset, data, bytes)
extern int fastboot_flash_write(fastboot_ptentry *ptn, unsigned extra_per_page,
				const void *data, unsigned bytes);
int parse_fb_param(char *name);
struct fb_operation_t {
	unsigned char enable;
	unsigned char device;
	unsigned char controller;	
	unsigned char part_type;
};


#else

/* Stubs for when CONFIG_FASTBOOT is not defined */
#define fastboot_preboot() 0
#define fastboot_init(a) 1
#define fastboot_shutdown()
#define fastboot_poll() 1
#define fastboot_is_highspeed() 0
#define fastboot_fifo_size() 0
#define fastboot_tx_status(a, b) 1
#define fastboot_getvar(a, b) 1
#define fastboot_tx(a, b) 1

#define fastboot_flash_add_ptn(a)
#define fastboot_flash_find_ptn(a) NULL
#define fastboot_flash_get_ptn(a) NULL
#define fastboot_flash_get_ptn_count() 0
#define fastboot_flash_dump_ptn()
#define fastboot_flash_init()
#define fastboot_flash_erase(a) 1
#define fastboot_flash_read_ext(a, b, c, d, e) 0
#define fastboot_flash_read(a, b, c, d, e) 0
#define fastboot_flash_write(a, b, c, d) 0

#endif /* CONFIG_FASTBOOT */
#endif /* FASTBOOT_H */
