/*
 * Copyright (C) 2010 Texas Instruments
 *
 * Author : Mohammed Afzal M A <afzal@ti.com>
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
 *
 * Fastboot is implemented using gadget stack, many of the ideas are
 * derived from fastboot implemented in OmapZoom by
 * Tom Rix <Tom.Rix@windriver.com>, and portion of the code has been
 * ported from OmapZoom.
 *
 * Part of OmapZoom was copied from Android project, Android source
 * (legacy bootloader) was used indirectly here by using OmapZoom.
 *
 * This is Android's Copyright:
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
 */

#include <common.h>
#include <command.h>
#include <fastboot.h>
#include <linux/mtd/nand.h>   
#include <sparse.h>
#include <../include/configs/wmt.h>   
#include <../include/version.h>  

#include "../define.h"
#define WMT_UBOOT_BUILD_ID "BUILDID_"  BUILD_TIME

//#define	ERR       1
//#define	WARN    1
//#define	INFO     1
//#define      DEBUG   1

#ifdef DEBUG
#define FBTDBG(fmt,args...)\
        printf("DEBUG: [%s]: %d: \n"fmt, __FUNCTION__, __LINE__,##args)
#else
#define FBTDBG(fmt,args...) do{}while(0)
#endif

#ifdef INFO
#define FBTINFO(fmt,args...)\
        printf("INFO: [%s]: "fmt, __FUNCTION__, ##args)
#else
#define FBTINFO(fmt,args...) do{}while(0)
#endif

#ifdef WARN
#define FBTWARN(fmt,args...)\
        printf("WARNING: [%s]: "fmt, __FUNCTION__, ##args)
#else
#define FBTWARN(fmt,args...) do{}while(0)
#endif

#ifdef ERR
#define FBTERR(fmt,args...)\
        printf("ERROR: [%s]: "fmt, __FUNCTION__, ##args)
#else
#define FBTERR(fmt,args...) do{}while(0)
#endif

#include <nand.h>
#include <environment.h>
/* USB specific */
#include <usb_defs.h>
#ifndef _GLOBAL_H_
#include "../board/wmt/include/global.h"
#endif
#include "compiler.h"
#include "usbdevice.h"


extern void restore_original_string_serial(void);
extern struct nand_chip* get_current_nand_chip(void);
extern int tellme_nandinfo(struct nand_chip *nand);
extern int do_reset (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[]);  
extern void udc_endpoint_write (struct usb_endpoint_instance *endpoint);
extern int  udc_init (void);
extern void udc_enable(struct usb_device_instance *device);
extern void udc_disable(void);
extern void udc_connect(void);
extern void udc_disconnect(void);
extern void udc_setup_ep (struct usb_device_instance *device,
		   unsigned int ep, struct usb_endpoint_instance *endpoint);
extern int wmt_udc_irq(void);
extern void udc_startup_events(struct usb_device_instance *device);
extern int WMTSaveImageToNAND(struct nand_chip *nand, unsigned long long naddr, unsigned int dwImageStart,
		unsigned int dwImageLength);
extern int WMTSaveImageToNAND2(struct nand_chip *nand, unsigned long long naddr, unsigned int dwImageStart,
	    unsigned int dwImageLength, int oob_offs, unsigned int need_erase, unsigned long long *eaddr);
extern int WMTEraseNAND(struct nand_chip *nand, unsigned int block, unsigned int block_nums, unsigned int all);
void *memcpy_itp(void *d, const void *s, size_t n);
extern int mmc_fb_write(unsigned dev_num, ulong blknr, ulong blkcnt, uchar *src);
extern flash_info_t flash_info[];	/* info for FLASH chips */
extern int do_flerase (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[]);
extern int do_mem_cp ( cmd_tbl_t *cmdtp, int flag, int argc, char *argv[]);
extern int mmc_part_update(unsigned char *buffer);
extern int do_fat_fsstore (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[]);
extern int image_rsa_check(
unsigned int image_mem_addr,
unsigned int image_size,
unsigned int sig_addr,
unsigned int sig_size);
/*extern void memdump(void *pv, int num);*/
extern env_t *env_ptr;
extern int curr_device;
extern int udc_download_flag;

#define STR_LANG		0x00
#define STR_MANUFACTURER	0x01
#define STR_PRODUCT		0x02
#define STR_SERIAL		0x03
#define STR_CONFIGURATION	0x04
#define STR_INTERFACE		0x05
#define STR_COUNT		0x06

#define CONFIG_USBD_CONFIGURATION_STR	"Android Fastboot Configuration"
#define CONFIG_USBD_INTERFACE_STR	"Android Fastboot Interface"

#define USBFBT_BCD_DEVICE	0x00
#define	USBFBT_MAXPOWER		0x32

#define	NUM_CONFIGS	1
#define	NUM_INTERFACES	1
#define	NUM_ENDPOINTS	2  

#define	RX_EP_INDEX	1
#define	TX_EP_INDEX	2

#define SZ_1M                           0x00100000
#define SZ_2M                           0x00200000
#define SZ_4M                           0x00400000
#define SZ_8M                           0x00800000
#define SZ_16M                         0x01000000
#define SZ_31M                         0x01F00000
#define SZ_32M                         0x02000000
#define SZ_64M                         0x04000000
#define SZ_128M                       0x08000000
#define SZ_256M                       0x10000000
#define SZ_288M                       0x12000000
#define SZ_320M                       0x14000000
#define SZ_512M                       0x20000000
#define SZ_1024M                     0x40000000
#define SZ_640M                       0x28000000
#define SZ_768M                       0x30000000
#define SZ_896M                       0x38000000
#define SIZE_REMAINING           (~0U)

#define MAX_DOWNLOAD_SIZE      SZ_256M

//for emmc
#define PART_SYSTEM 1
#define PART_BOOT 2
#define PART_RECOVERY 3
#define PART_CACHE 5
#define PART_SWAP 6
#define PART_U_BOOT_LOGO 7
#define PART_KERNEL_LOGO 8
#define PART_MISC 9
#define PART_EFS 10
#define PART_RADIO 11
#define PART_KEYDATA 12
#define PART_USERDATA 13

//for nand flash
#define NF_PART_LOGO 0
#define NF_PART_BOOT 1
#define NF_PART_RECOVERY 2
#define NF_PART_MISC 3
#define NF_PART_KEYDATA 4
#define NF_PART_SYSTEM 5
#define NF_PART_CACHE 6
#define NF_PART_DATA 7
#define NF_PART_MAX  8


#define STR_EXT4 1
#define STR_SIG 2
#define STR_ROM 3
#define STR_FAT 4
#define STR_NOFS 5
#define STR_RAM 6
#define STR_NAND 7
#define STR_UBI 8

#define ROM_WLOAD 0
#define ROM_UBOOT 1
#define ROM_ENV1 2
#define ROM_ENV2 3


#define EP0_MAX_PACKET_SIZE 64
#define PHYS_SDRAM_F         0
#define CONFIG_FASTBOOT_TRANSFER_BUFFER         	(PHYS_SDRAM_F + SZ_128M)
#define CONFIG_FASTBOOT_TRANSFER_BUFFER_SIZE    	(SZ_1024M - SZ_128M)
/*define signature buffer addr 127M*/
#define CONFIG_FASTBOOT_SIG_BUFFER 				(PHYS_SDRAM_F + SZ_128M - SZ_1M)
/* configuration modifiers
 */
#define BMATTRIBUTE_RESERVED		0x80
#define BMATTRIBUTE_SELF_POWERED	0x40

#define    FASTBOOT_PRODUCT_NAME         "wmid"
#define    FASTBOOT_NAND_BLOCK_SIZE                 8192
#define    FASTBOOT_NAND_OOB_SIZE                   64
#define 	 MTDPART_OFS_APPEND	(-1)



struct _fbt_config_desc {
	struct usb_configuration_descriptor configuration_desc;
	struct usb_interface_descriptor interface_desc;
	struct usb_endpoint_descriptor endpoint_desc[NUM_ENDPOINTS];
};

static int fbt_handle_response(void);

/* defined and used by gadget/ep0.c */
extern struct usb_string_descriptor **usb_strings;

//static struct cmd_fastboot_interface priv;
struct cmd_fastboot_interface priv;
/* USB Descriptor Strings */
static char serial_number[28]; /* what should be the length ?, 28 ? */
static u8 wstr_lang[4] = {4,USB_DT_STRING,0x9,0x4};
static u8 wstr_manufacturer[2 + 2*(sizeof(CONFIG_USBD_MANUFACTURER)-1)];
static u8 wstr_product[2 + 2*(sizeof(CONFIG_USBD_PRODUCT_NAME)-1)];
static u8 wstr_serial[2 + 2*(sizeof(serial_number) - 1)];
static u8 wstr_configuration[2 + 2*(sizeof(CONFIG_USBD_CONFIGURATION_STR)-1)];
static u8 wstr_interface[2 + 2*(sizeof(CONFIG_USBD_INTERFACE_STR)-1)];

/* USB descriptors */
static struct usb_device_descriptor device_descriptor __attribute__((aligned (32))) = {
	.bLength = sizeof(struct usb_device_descriptor),
	.bDescriptorType =	USB_DT_DEVICE,
	.bcdUSB =		cpu_to_le16(USB_BCD_VERSION),
	.bDeviceClass =		0xFF,
	.bDeviceSubClass =	0x00,
	.bDeviceProtocol =	0x00,
	.bMaxPacketSize0 =	EP0_MAX_PACKET_SIZE,
	.idVendor =		cpu_to_le16(CONFIG_USBD_VENDORID),
	.bcdDevice =		cpu_to_le16(USBFBT_BCD_DEVICE),
	.iManufacturer =	STR_MANUFACTURER,
	.iProduct =		STR_PRODUCT,
	.iSerialNumber =	STR_SERIAL,
	.bNumConfigurations =	NUM_CONFIGS
};

static struct _fbt_config_desc fbt_config_desc __attribute__((aligned (32)))= {
	.configuration_desc = {
		.bLength = sizeof(struct usb_configuration_descriptor),
		.bDescriptorType = USB_DT_CONFIG,
		.wTotalLength =	cpu_to_le16(sizeof(struct _fbt_config_desc)),
		.bNumInterfaces = NUM_INTERFACES,
		.bConfigurationValue = 1,
		.iConfiguration = STR_CONFIGURATION,
		.bmAttributes =	BMATTRIBUTE_SELF_POWERED | BMATTRIBUTE_RESERVED,
		.bMaxPower = USBFBT_MAXPOWER,
	},
	.interface_desc = {
		.bLength  = sizeof(struct usb_interface_descriptor),
		.bDescriptorType = USB_DT_INTERFACE,
		.bInterfaceNumber = 0,
		.bAlternateSetting = 0,
		.bNumEndpoints = 0x2,
		.bInterfaceClass = FASTBOOT_INTERFACE_CLASS,
		.bInterfaceSubClass = FASTBOOT_INTERFACE_SUB_CLASS,
		.bInterfaceProtocol = FASTBOOT_INTERFACE_PROTOCOL,
		.iInterface = STR_INTERFACE,
	},
	.endpoint_desc = {
		{
			.bLength = sizeof(struct usb_endpoint_descriptor),
			.bDescriptorType = USB_DT_ENDPOINT,
			.bEndpointAddress = RX_EP_INDEX | USB_DIR_IN,
			.bmAttributes =	USB_ENDPOINT_XFER_BULK,
			.bInterval = 0,
		},
		{
			.bLength = sizeof(struct usb_endpoint_descriptor),
			.bDescriptorType = USB_DT_ENDPOINT,
			.bEndpointAddress = TX_EP_INDEX | USB_DIR_OUT,			
			.bmAttributes = USB_ENDPOINT_XFER_BULK,
			.bInterval = 0,
		},
	},
};

static struct usb_interface_descriptor interface_descriptors[NUM_INTERFACES]__attribute__((aligned (32)));
static struct usb_endpoint_descriptor *ep_descriptor_ptrs[NUM_ENDPOINTS]__attribute__((aligned (32)));
static struct usb_string_descriptor *fbt_string_table[STR_COUNT]__attribute__((aligned (32)));
static struct usb_device_instance device_instance[1]__attribute__((aligned (32)));
static struct usb_bus_instance bus_instance[1]__attribute__((aligned (32)));
static struct usb_configuration_instance config_instance[NUM_CONFIGS]__attribute__((aligned (32)));
static struct usb_interface_instance interface_instance[NUM_INTERFACES]__attribute__((aligned (32)));
static struct usb_alternate_instance alternate_instance[NUM_INTERFACES]__attribute__((aligned (32)));
static struct usb_endpoint_instance endpoint_instance[NUM_ENDPOINTS + 1]__attribute__((aligned (32)));

/* FASBOOT specific */

#define	GETVARLEN	30
#define	SECURE		"yes"
#define  fb_version_baseband  "0.01.01"
/* U-boot version */
#define fb_version_bootloader 	U_BOOT_VERSION" (" __DATE__ " - " __TIME__ ")"

//static struct cmd_fastboot_interface priv =
struct cmd_fastboot_interface priv =
{
        .transfer_buffer       = (unsigned char *)CONFIG_FASTBOOT_TRANSFER_BUFFER,
        .transfer_buffer_size  = (unsigned int)CONFIG_FASTBOOT_TRANSFER_BUFFER_SIZE,
};
/*define for all image signature buffer */
unsigned int  sig_buf  = CONFIG_FASTBOOT_SIG_BUFFER;
unsigned int  sig_size;
extern struct nand_chip nand_dev_desc[CFG_MAX_NAND_DEVICE];

static int fbt_init_endpoints (void);
/* Use do_bootm and do_go for fastboot's 'boot' command */
extern int do_go (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[]);
/* Use do_setenv and do_saveenv to permenantly save data */
extern int do_saveenv (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[]);
extern int do_setenv ( cmd_tbl_t *cmdtp, int flag, int argc, char *argv[]);
extern int do_nand(cmd_tbl_t * cmdtp, int flag, int argc, char *argv[]);
    
/* Use do_setenv and do_saveenv to permenantly save data */ 
extern int do_saveenv (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[]); 
extern int do_setenv ( cmd_tbl_t *cmdtp, int flag, int argc, char *argv[]); 
extern int do_syncenv(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[]);
extern int do_switch_ecc(cmd_tbl_t * cmdtp, int flag, int argc, char *argv[]); 
extern int do_nand(cmd_tbl_t * cmdtp, int flag, int argc, char *argv[]); 
extern int do_mmc (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[]);
extern int do_mmc_write (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[]);
extern int do_bootm (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[]);
extern int fastboot_oem(const char *cmd);

#define MTDPART_SIZ_FULL 0

/* Initialize nand the name of fastboot mappings */ 
   static fastboot_ptentry ptable[16] = { 
   	{
		.name		= "logo",
		.start		= MTDPART_OFS_APPEND, 
		.length		= 0x01000000, /*16M*/
		.flags  = FASTBOOT_PTENTRY_FLAGS_WRITE_I | 
   		          FASTBOOT_PTENTRY_FLAGS_WRITE_SW_ECC, 
	},
	{
		.name		= "boot",
		.start		= MTDPART_OFS_APPEND, 
		.length		= 0x01000000, /*16M*/
		.flags  = FASTBOOT_PTENTRY_FLAGS_WRITE_I | 
   		          FASTBOOT_PTENTRY_FLAGS_WRITE_SW_ECC, 
	},
	{
		.name		= "recovery",
		.start		= MTDPART_OFS_APPEND,  
		.length		= 0x01000000, /*16M*/
		.flags  = FASTBOOT_PTENTRY_FLAGS_WRITE_I | 
   		          FASTBOOT_PTENTRY_FLAGS_WRITE_SW_ECC, 
	},
	{
		.name		= "misc",
		.start		= MTDPART_OFS_APPEND, 
		.length		= 0x01000000, /*16M*/
		.flags  = FASTBOOT_PTENTRY_FLAGS_WRITE_I | 
   		          FASTBOOT_PTENTRY_FLAGS_WRITE_SW_ECC, 		
	},
	{
		.name		= "keydata",
		.start		= MTDPART_OFS_APPEND, 
		.length		= 0x04000000, /*64M*/
		.flags  = FASTBOOT_PTENTRY_FLAGS_WRITE_I | 
   		          FASTBOOT_PTENTRY_FLAGS_WRITE_SW_ECC, 		
	},	
	{
		.name		= "system",
		.start		= MTDPART_OFS_APPEND, 
		.length		= 0x30000000, /*768M*/
 		.flags  = FASTBOOT_PTENTRY_FLAGS_WRITE_SW_ECC | 
   		FASTBOOT_PTENTRY_FLAGS_WRITE_JFFS2, 
	},
	{
		.name		= "cache",
		.start		= MTDPART_OFS_APPEND, 
		.length		= 0x20000000, /*512M*/
 		.flags  = FASTBOOT_PTENTRY_FLAGS_WRITE_SW_ECC | 
   		FASTBOOT_PTENTRY_FLAGS_WRITE_JFFS2, 
	},	
	{
		.name		= "data",
		.start		= MTDPART_OFS_APPEND,
		.length		= MTDPART_SIZ_FULL, /*rest*/
 		.flags  = FASTBOOT_PTENTRY_FLAGS_WRITE_SW_ECC | 
   		FASTBOOT_PTENTRY_FLAGS_WRITE_JFFS2, 
	},
   };

/* To support the Android-style naming of flash */
#define MAX_PTN 16
static unsigned int pcount=0;
static int static_pcount = 16;
static fastboot_ptentry ptable[16];

static unsigned long long sparse_download_size = 0;
static unsigned long long sparse_remained_size = 0;
static unsigned long long sparse_nand_addr = 0;

int mmc_controller_no = 0;
#define MBR  0
#define GPT  1
unsigned char mmc_part_type = MBR; 
/* USB specific */

/* utility function for converting char* to wide string used by USB */
static void str2wide (char *str, u16 * wide)
{
	int i;
	for (i = 0; i < strlen (str) && str[i]; i++) {
		#if defined(__LITTLE_ENDIAN)
			wide[i] = (u16) str[i];
		#elif defined(__BIG_ENDIAN)
			wide[i] = ((u16)(str[i])<<8);
		#endif
	}
}

/* fastboot_init has to be called before this fn to get correct serial string */
static int fbt_init_strings(void)
{
	struct usb_string_descriptor *string;
	FBTDBG("%s\n",__FUNCTION__);

	fbt_string_table[STR_LANG] =
		(struct usb_string_descriptor*)wstr_lang;

	string = (struct usb_string_descriptor *) wstr_manufacturer;
	string->bLength = sizeof(wstr_manufacturer);
	string->bDescriptorType = USB_DT_STRING;
	str2wide (CONFIG_USBD_MANUFACTURER, string->wData);
	fbt_string_table[STR_MANUFACTURER] = string;

	string = (struct usb_string_descriptor *) wstr_product;
	string->bLength = sizeof(wstr_product);
	string->bDescriptorType = USB_DT_STRING;
	str2wide (CONFIG_USBD_PRODUCT_NAME, string->wData);
	fbt_string_table[STR_PRODUCT] = string;

	string = (struct usb_string_descriptor *) wstr_serial;
	string->bLength = sizeof(wstr_serial);
	string->bDescriptorType = USB_DT_STRING;
	str2wide (serial_number, string->wData);
	fbt_string_table[STR_SERIAL] = string;

	string = (struct usb_string_descriptor *) wstr_configuration;
	string->bLength = sizeof(wstr_configuration);
	string->bDescriptorType = USB_DT_STRING;
	str2wide (CONFIG_USBD_CONFIGURATION_STR, string->wData);
	fbt_string_table[STR_CONFIGURATION] = string;

	string = (struct usb_string_descriptor *) wstr_interface;
	string->bLength = sizeof(wstr_interface);
	string->bDescriptorType = USB_DT_STRING;
	str2wide (CONFIG_USBD_INTERFACE_STR, string->wData);
	fbt_string_table[STR_INTERFACE] = string;

	/* Now, initialize the string table for ep0 handling */
	usb_strings = fbt_string_table;

	return 0;
}

static void fbt_event_handler (struct usb_device_instance *device,
				  usb_device_event_t event, int data)
{
	FBTDBG("%s\n",__FUNCTION__);

	switch (event) {
	case DEVICE_RESET:
	case DEVICE_BUS_INACTIVE:
		priv.configured = 0;
		sparse_remained_size = 0;
        sparse_download_size = 0;			
		restore_original_string_serial();
		break;
	case DEVICE_CONFIGURED:
		priv.configured = 1;
		break;

	case DEVICE_ADDRESS_ASSIGNED:
		fbt_init_endpoints ();

	default:
		break;
	}
}

/* fastboot_init has to be called before this fn to get correct serial string */
static int fbt_init_instances(void)
{
	int i;
	FBTDBG("%s\n",__FUNCTION__);

	/* initialize device instance */
	memset (device_instance, 0, sizeof (struct usb_device_instance));
	device_instance->device_state = STATE_INIT;
	device_instance->device_descriptor = &device_descriptor;
	device_instance->event = fbt_event_handler;
	device_instance->cdc_recv_setup = NULL;
	device_instance->bus = bus_instance;
	device_instance->configurations = NUM_CONFIGS;
	device_instance->configuration_instance_array = config_instance;

	/* XXX: what is this bus instance for ?, can't it be removed by moving
	    endpoint_array and serial_number_str is moved to device instance */
	/* initialize bus instance */
	memset (bus_instance, 0, sizeof (struct usb_bus_instance));
	bus_instance->device = device_instance;
	bus_instance->endpoint_array = endpoint_instance;
	/* XXX: what is the relevance of max_endpoints & maxpacketsize ? */
	bus_instance->max_endpoints = 3;
	bus_instance->maxpacketsize = 64;
	bus_instance->serial_number_str = serial_number;

	/* configuration instance */
	memset (config_instance, 0,
		sizeof (struct usb_configuration_instance));
	config_instance->interfaces = NUM_INTERFACES;
	config_instance->configuration_descriptor =
		(struct usb_configuration_descriptor *)&fbt_config_desc;
	config_instance->interface_instance_array = interface_instance;

	/* XXX: is alternate instance required in case of no alternate ? */
	/* interface instance */
	memset (interface_instance, 0,
		sizeof (struct usb_interface_instance));
	interface_instance->alternates = 1;
	interface_instance->alternates_instance_array = alternate_instance;

	/* alternates instance */
	memset (alternate_instance, 0,
		sizeof (struct usb_alternate_instance));
	alternate_instance->interface_descriptor = interface_descriptors;
	alternate_instance->endpoints = NUM_ENDPOINTS;
	alternate_instance->endpoints_descriptor_array = ep_descriptor_ptrs;

	/* endpoint instances */
	memset (&endpoint_instance[0], 0,
		sizeof (struct usb_endpoint_instance));
	endpoint_instance[0].endpoint_address = 0;
	endpoint_instance[0].rcv_packetSize = EP0_MAX_PACKET_SIZE;
	endpoint_instance[0].rcv_attributes = USB_ENDPOINT_XFER_CONTROL;
	endpoint_instance[0].tx_packetSize = EP0_MAX_PACKET_SIZE;
	endpoint_instance[0].tx_attributes = USB_ENDPOINT_XFER_CONTROL;
	/* XXX: following statement to done along with other endpoints
		at another place ? */
	udc_setup_ep (device_instance, 0, &endpoint_instance[0]);
	for (i = 1; i <= NUM_ENDPOINTS; i++) {
	
		memset (&endpoint_instance[i], 0,
			sizeof (struct usb_endpoint_instance));

		endpoint_instance[i].endpoint_address =
			ep_descriptor_ptrs[i - 1]->bEndpointAddress;

		endpoint_instance[i].rcv_attributes =
			ep_descriptor_ptrs[i - 1]->bmAttributes;
            if (i == 1)
			endpoint_instance[i].rcv_packetSize = 512;
		else 
			endpoint_instance[i].rcv_packetSize = 512 ;		
			
		endpoint_instance[i].tx_attributes =
			ep_descriptor_ptrs[i - 1]->bmAttributes;
		if (i == 1)
			endpoint_instance[i].tx_packetSize = 512;
		else 
			endpoint_instance[i].tx_packetSize = 512 ;
		
		endpoint_instance[i].tx_attributes =
			ep_descriptor_ptrs[i - 1]->bmAttributes;

		urb_link_init (&endpoint_instance[i].rcv);
		urb_link_init (&endpoint_instance[i].rdy);
		urb_link_init (&endpoint_instance[i].tx);
		urb_link_init (&endpoint_instance[i].done);

		if (endpoint_instance[i].endpoint_address & USB_DIR_IN) {
			endpoint_instance[i].tx_urb =
				usbd_alloc_urb (device_instance,
				&endpoint_instance[i]);
			} else {
				endpoint_instance[i].rcv_urb =
				usbd_alloc_urb (device_instance,
				&endpoint_instance[i]);
		}	
	}

	return 0;
}

/* XXX: ep_descriptor_ptrs can be removed by making better use of
	fbt_config_desc.endpoint_desc */
static int fbt_init_endpoint_ptrs(void)
{
	FBTDBG("%s\n",__FUNCTION__);

	ep_descriptor_ptrs[0] = &fbt_config_desc.endpoint_desc[0];
	ep_descriptor_ptrs[1] = &fbt_config_desc.endpoint_desc[1];
	ep_descriptor_ptrs[2] = &fbt_config_desc.endpoint_desc[2];

	return 0;
}

static int fbt_init_endpoints(void)
{
	int i;
	FBTDBG("%s\n",__FUNCTION__);

	/* XXX: should it be moved to some other function ? */
	bus_instance->max_endpoints = NUM_ENDPOINTS + 1; 

	/* XXX: is this for loop required ?, yes for MUSB it is */
	for (i = 1; i <= NUM_ENDPOINTS; i++) {

		/* configure packetsize based on HS negotiation status */
		if (device_instance->speed == USB_SPEED_FULL) {
			FBTINFO("setting up FS USB device ep%x\n",
				endpoint_instance[i].endpoint_address);
			ep_descriptor_ptrs[i - 1]->wMaxPacketSize =
				CONFIG_USBD_FASTBOOT_BULK_PKTSIZE_FS;
		} else if (device_instance->speed == USB_SPEED_HIGH) {
			FBTINFO("setting up HS USB device ep%x\n",
				endpoint_instance[i].endpoint_address);
			ep_descriptor_ptrs[i - 1]->wMaxPacketSize =
				CONFIG_USBD_FASTBOOT_BULK_PKTSIZE_HS;
		}

		endpoint_instance[i].tx_packetSize =
			le16_to_cpu(ep_descriptor_ptrs[i - 1]->wMaxPacketSize);
		endpoint_instance[i].rcv_packetSize =
			le16_to_cpu(ep_descriptor_ptrs[i - 1]->wMaxPacketSize);

		udc_setup_ep(device_instance, i, &endpoint_instance[i]);

	}

	return 0;
}

static struct urb *next_urb (struct usb_device_instance *device,
			     struct usb_endpoint_instance *endpoint)
{
	struct urb *current_urb = NULL;
	int space;
	FBTDBG("%s\n",__FUNCTION__);

	/* If there's a queue, then we should add to the last urb */
	if (!endpoint->tx_queue) {
		current_urb = endpoint->tx_urb;
	} else {
		/* Last urb from tx chain */
		current_urb =
			p2surround (struct urb, link, endpoint->tx.prev);
	}

	/* Make sure this one has enough room */
	space = current_urb->buffer_length - current_urb->actual_length;
	if (space > 0) {
		return current_urb;
	} else {		/* No space here */
		/* First look at done list */
		current_urb = first_urb_detached (&endpoint->done);
		if (!current_urb) {
			current_urb = usbd_alloc_urb (device, endpoint);
		}

		urb_append (&endpoint->tx, current_urb);
		endpoint->tx_queue++;
	}
	return current_urb;
}

/* FASBOOT specific */
void fastboot_flash_reset_ptn(void)
{
	pcount = 0;
}

/*
 * Android style flash utilties */
void fastboot_flash_add_ptn(fastboot_ptentry *ptn)
{
    if(pcount < MAX_PTN) {
        memcpy(ptable + pcount, ptn, sizeof(*ptn));
        pcount++;
    }
}

void fastboot_flash_dump_ptn(void)
{
    unsigned int n;	
    for(n = 0; n < pcount; n++) {
       fastboot_ptentry *ptn = ptable + n;
        printf("dump ptn %d name='%s' start=%d len=%d\n",
                n, ptn->name, ptn->start, ptn->length);
    }
}
/*support GPT partiton*/
fastboot_ptentry *fastboot_flash_find_ptn(const char *name)
{
    unsigned int n;

    for(n = 0; n < pcount; n++) {
	        FBTINFO("strlen(name):%d, ptable[%d].name:%s\n",
               strlen(name),n, ptable[n].name);
	
	    /* Make sure a substring is not accepted */
	    if (strlen(name) == strlen(ptable[n].name))
	    {
		    if(0 == strcmp(ptable[n].name, name))
			    return ptable + n;
	    }
    }
    return 0;
}

fastboot_ptentry *fastboot_flash_find_nand_ptn(int part_no)
{
    unsigned int n, i;
    n = part_no;
   for (i = 0; i <= part_no; i++)
    	FBTINFO("ptable[%d].name:%s,ptable[%d].start:0x%x,ptable[%d].length:0x%x\n", 
    				i, ptable[i].name, i, ptable[i].start, i, ptable[i].length);	
    return ptable + n;
   
}


static int get_partition_name(const char *src, char** endpp, char* buffer)
{
	int i = 0;
	if(NULL == src || NULL == buffer)
	{
		return -1;
	}

	while(*src != ':')
	{
		*buffer++ = *src++;	
		i++;
	}
	*endpp = (char *)src;
	buffer[i] = '\0';
	return i;
}

static int get_partition_idx(char *string, int *ret)
{
	int i, err = -1;
	for (i = 0; i < NF_PART_MAX; i++) {
	//	printk(KERN_DEBUG "MTD dev%d size: %8.8llx \"%s\"\n",
	//i, nand_partitions[i].size, nand_partitions[i].name);
		if (strcmp(string, ptable[i].name) == 0) {
			*ret = i;
			break;
		}
	}
	return err;
}

void init_nand_partitions(void){
    char partition_name[32];
	char  *s = NULL, *tmp = NULL;
    char str[128]={0};
	struct nand_chip * chip = get_current_nand_chip();
	__u64 part_size, start_off, chip_size;
	int index, i, n;

	if(chip == NULL) return;
	
	if(chip->mfr == NAND_HYNIX){
        //hynix nand flash support eslc , which have different partition table
        ptable[NF_PART_LOGO].length = 0x02000000;//32M
        ptable[NF_PART_BOOT].length = 0x02000000;//32M
        ptable[NF_PART_RECOVERY].length = 0x02000000;//32M		
	}
    
	chip_size = (__u64)chip->dwBlockCount * (__u64)chip->dwPageCount * (__u64)chip->oobblock;

    n = getenv_f("wmt.nand.partition", str, sizeof(str));
	//pstr = getenv("wmt.nand.parition");
    printf("pstr is %s\n", str);
	if(n > 0){
		s = str;
		while(*s != '\0')
		{
			index = NF_PART_MAX;
			memset(partition_name, 0, 32);
			get_partition_name(s, &tmp, partition_name);
			get_partition_idx(partition_name, &index);
			s = tmp + 1;
			part_size = simple_strtoul(s, &tmp, 16);
			s = tmp;
			if(*s == ':')
				s++;

			//data can't be resized by uboot env, its size is left whole nand.
			if((index >= 0) &&  (index < NF_PART_MAX) && (part_size < chip_size)) { 
				ptable[index].length = part_size;
			} else {
				printf("Invalid parameter \"wmt.nand.partition\". Use default partition size for \"%s\" partition.\n", partition_name);
			}
		}
	}

    	
    start_off = 0;
	for(i = 0; i < NF_PART_MAX; i++){
        ptable[i].start = start_off;
		start_off += ptable[i].length;
	}

	if(NF_PART_MAX > 0){
		if(ptable[NF_PART_MAX-1].length == MTDPART_SIZ_FULL){
	        ptable[NF_PART_MAX-1].length = chip_size - ptable[NF_PART_MAX-1].start;
		}
     }	

	printf("Nand Partitions: 0x%x x 0x%x x 0x%x = 0x%04x%08x\n",
		chip->dwBlockCount, chip->dwPageCount, chip->oobblock, (unsigned long)(chip_size>>32), (unsigned long)chip_size);
	for(i = 0; i < NF_PART_MAX; i++){
        //uboot printf have problem with __u64
        start_off = ptable[i].start + ptable[i].length;
		printf("0x%04x%08x -  0x%04x%08x \"%s\"\n",(unsigned long)(ptable[i].start>>32), (unsigned long)ptable[i].start, 
		       (unsigned long)(start_off>>32), (unsigned long)start_off, ptable[i].name);
	}
}

extern block_dev_desc_t *get_dev (char*, int);
extern int mmc_init(int verbose, int device_num);
unsigned int  part_no = -1; //init
unsigned char rsa_check_flag = 0;
unsigned char logo_name = 0;
int sys_sub_image = -1;
unsigned char sub_sys_lt_256M = 0;
unsigned char sub_sys_cnt = -1;	

fastboot_ptentry *fastboot_flash_get_ptn(unsigned int n)
{
    if(n < pcount) {
        return ptable + n;
    } else {
        return 0;
    }
}

unsigned int fastboot_flash_get_ptn_count(void)
{
    return pcount;
}
static void set_env(char *var, char *val)
{
	char *setenv[4]  = { "setenv", NULL, NULL, NULL, };

	setenv[1] = var;
	setenv[2] = val;

	do_setenv(NULL, 0, 3, setenv);
}

static int write_to_ptn(int part_no, int ss, char *key, int haveoob)
{
	int ret = 1;
	char length[32];
	char wstart[32], wlength[32], addr[32], oobsize[32];
	char *write[6]  = { "nandrw", "w", NULL, NULL, NULL, NULL, };
	static unsigned char last_ss = 0xFF;
	int status;
	unsigned int image_start;
	unsigned int image_size;
	static unsigned int offset = 0;
	static unsigned int res_len = 0;
	struct fastboot_ptentry *ptn;
	
	ptn = fastboot_flash_find_nand_ptn(part_no);
	
	write[2] = addr;
	write[3] = wstart;
	write[4] = wlength;

	FBTINFO("Flashing nand part_no:%d\n", part_no);

	/* Which flavor of write to use, WMT not support */

	sprintf(length, "0x%x", ptn->length);
				
	/* Normal case */
	if ((part_no == NF_PART_SYSTEM) ||
		(part_no == NF_PART_DATA) ||
		(part_no == NF_PART_CACHE)) {
		if(haveoob){
			write[1] = "wo";
		    write[5] = oobsize;
	    }
        if(sparse_download_size != 0){
			unsigned long long theaddr = sparse_nand_addr;
            struct nand_chip * chip = get_current_nand_chip();
			image_start = SZ_128M;
			image_size = priv.d_bytes;			
			
            printf("Nand write sparse image, naddr:0x%04x%08x image_start:0x%x image_size:0x%x, oob%d\n", 
				(unsigned long)(sparse_nand_addr>>32), (unsigned long)sparse_nand_addr, image_start, image_size, haveoob);			
			status = WMTSaveImageToNAND2(chip, theaddr, image_start, image_size, haveoob?FASTBOOT_NAND_OOB_SIZE:0, 0, &sparse_nand_addr);
			if(!status){
				last_ss = 0xFF;
				priv.transfer_buffer = (unsigned char *)SZ_128M;
				sprintf(priv.response, "OKAY");
				return 0;				
			}else{
				printf("Nand part_no:%d FAILED!\n", part_no);
				sprintf(priv.response, "FAIL nand write FAIL");
				goto fail;			
			}
		}
		/*cache part_no = 3, system part_no = 4,userdata part_no = 11*/
		else if (ss == 0xFF) { /* system, userdata, cache with yaffs2 system */
			
			if (key) {
			/*if len over 256MB ,rsa check 256M*/	
			if (priv.d_bytes > SZ_256M)  
				status = image_rsa_check((unsigned int)priv.transfer_buffer, 0x10000000, sig_buf, sig_size);
			else 
				status = image_rsa_check((unsigned int)priv.transfer_buffer, priv.d_bytes, sig_buf, sig_size);			
				if (status != 0 ) {
					printf("part_no:%d rsa check fail, rcode :%x\n", part_no, status);
					sprintf(priv.response, "FAIL nand rsa check FAIL");
					goto fail;
				}else {
					printf("nand part_no:%d rsa check OK\n", part_no);
				}
			}		
			image_start = SZ_128M;
			image_size = priv.d_bytes;
			if (last_ss % 2 == 0) {
				image_size = priv.d_bytes + SZ_256M;
			}

			//uboot printf's have problem with __u64
			printf("Nand image last_ss:0x%x , image_start:0x%x, image_size:0x%x, priv.transfer_buffer:0x%x, ptn->start:0x%04x%08x, offset:0x%x, oob:%d\n",
				    last_ss, image_start, image_size, priv.transfer_buffer, (unsigned long)(ptn->start>>32), (unsigned long)ptn->start, offset, haveoob);
			sprintf(addr, "0x%x", image_start);
			sprintf(wstart, "0x%04x%08x", (unsigned long)((ptn->start + offset)>>32), (unsigned long)(ptn->start + offset));
			sprintf(wlength, "0x%x", image_size);	
			if(haveoob)sprintf(oobsize, "0x%x", FASTBOOT_NAND_OOB_SIZE);	
			
			ret = do_nand(NULL, 0, haveoob?6:5, write);
			if (!ret) {
				last_ss = 0xFF;
				priv.transfer_buffer = (unsigned char *)SZ_128M;
				printf("Nand part_no:%d DONE!\n", part_no);
				sprintf(priv.response, "OKAY");
				return 0;
			} else {
				printf("Nand part_no:%d FAILED!\n", part_no);
				sprintf(priv.response, "FAIL nand write FAIL");
				goto fail;
			}
		} else { /* sub-system  */
			last_ss = ss;
			image_start = SZ_128M;
			image_size = priv.d_bytes;

			if (ss % 2 == 0) {
				image_start = SZ_128M;
			} else {
				image_start = SZ_128M + SZ_256M;
			}
			priv.transfer_buffer = (unsigned char *)(SZ_128M + 0x10000000 * ((ss % 2) ? 0 : 1));
			printf("Nand sub-system last_ss:0x%x , image_start:0x%x, priv.transfer_buffer:0x%x\n", last_ss, image_start, priv.transfer_buffer);

			if (key) {
					status = image_rsa_check(image_start, image_size, sig_buf, sig_size);
					if (status != 0 ) {
						printf("nand ss-%d rsa check fail, rcode:0x%X\n", ss, status);
						sprintf(priv.response, "FAIL nand rsa check FAIL");
						goto fail;
					} else {
						if (ss % 2 == 0) {
							printf("nand ss-%d rsa check OK\n",ss);
							sprintf(priv.response, "OKAY");
							return 0;
						} else {
							printf("nand ss-%d rsa check OK\n",ss);
						}
				}
			}

			if (ss % 2 != 0) {
				/*boundary need handle page count and bad block number ?*/
				printf("Nand last_ss:0x%x, offset:0x%x, res_len:0x%x\n", last_ss, offset,res_len);
				sprintf(addr, "0x%x", (unsigned char *)(SZ_128M - res_len));
                //uboot printf have __u64 problem, but i didn't fix it here, as i don't know what sub-system's usage
				sprintf(wstart, "0x%x", ptn->start + offset ); //?
				if(haveoob)sprintf(oobsize, "0x%x", FASTBOOT_NAND_OOB_SIZE);	
				res_len = 0x20000000 % (0x100000 + 0x80 * 0x40);//mode 1M
				sprintf(wlength, "0x%x", (0x20000000 - res_len));
				offset +=  (0x20000000 - res_len);
     				memcpy((void*)(SZ_128M - res_len), (void*)(0x20000000 + SZ_128M - res_len), res_len);
 
				/* sub-system1,3,5,... */
				ret = do_nand(NULL, 0, haveoob?6:5, write);

				if (!ret) {
					printf("Writing nand part_no:%d DONE!\n", part_no);
					sprintf(priv.response, "OKAY");				
					return 0;
				} else {
					printf("Writing nand part_no:%d FAILED!\n", part_no);
					sprintf(priv.response, "FAIL nand write FAIL");
					goto fail;
				}
			} else {
					/* sub-system0,2,4,... */
					printf("Writing nand part_no:%d DONE!\n", part_no);
					sprintf(priv.response, "OKAY");				
					return 0;
			}
		}		
	} else {
 		/*raw data*/			
		if (key && (part_no != 5) && (part_no != 6) ) {
			status = image_rsa_check((unsigned int)priv.transfer_buffer, priv.d_bytes, sig_buf, sig_size);
			if (status != 0 ) {
				printf("nand part_no:%d rsa check fail, rcode :%x\n", part_no,status);
				sprintf(priv.response, "FAIL nand rsa check FAIL");
				goto fail;
			}else {			
				printf("nand part_no:%d rsa check OK\n", part_no);			
			}
		}
		image_start = SZ_128M;
		image_size = priv.d_bytes;
		offset = 0;
		//uboot printf's have problem with __u64
		printf("Nand  raw image_start:0x%x, image_size:0x%x, priv.transfer_buffer:0x%x, ptn->start:0x%04x%08x\n",
			    image_start, image_size, priv.transfer_buffer, (unsigned long)(ptn->start>>32), (unsigned long)ptn->start);
		
		sprintf(addr, "0x%x", image_start);
		sprintf(wstart, "0x%04x%08x", (unsigned long)((ptn->start + offset)>>32), (unsigned long)(ptn->start + offset));
		sprintf(wlength, "0x%x", image_size);			
		ret = do_nand(NULL, 0, 5, write);

		if (!ret) {
			priv.transfer_buffer = (unsigned char *)SZ_128M;
			printf("Writing nand part_no:%d DONE!\n", part_no);
			sprintf(priv.response, "OKAY");
			return 0;
		} else {
			printf("Writing nand part_no:%d FAILED!\n", part_no);
			sprintf(priv.response, "FAIL nand write FAIL");
			goto fail;				
		}
	
	}
fail:
	/* reset variables as fail */
	last_ss = 0xFF;
	priv.transfer_buffer = (unsigned char *)SZ_128M;
	
	return ret;
}

static unsigned long long memparse(char *ptr, char **retptr)
{
	char *endptr;	/* local pointer to end of parsed string */
	FBTDBG("%s\n",__FUNCTION__);

	unsigned long ret = simple_strtoul(ptr, &endptr, 0);

	switch (*endptr) {
	case 'M':
	case 'm':
		ret <<= 10;
	case 'K':
	case 'k':
		ret <<= 10;
		endptr++;
	default:
		break;
	}

	if (retptr)
		*retptr = endptr;

	return ret;
}
static int add_partition_from_environment(char *s, char **retptr)
{
	unsigned long size;
	unsigned long offset = 0;
	char *name;
	int name_len;
	int delim;
	unsigned int flags;
	struct fastboot_ptentry part;
	FBTDBG("%s\n",__FUNCTION__);

	/* fetch the partition size */
	size = memparse(s, &s);
	if (0 == size) {
		FBTERR("size of partition is 0\n");
		/*for x0,x4 reservered partition , do not return error*/
		//return 1;
	}
	FBTINFO("size of partition is 0x%x\n", size);

	/* fetch partition name and flags */
	flags = 0; /* this is going to be a regular partition */
	delim = 0;
	/* check for offset */
	if (*s == '@') {
		s++;
		offset = memparse(s, &s);
	} else {
		FBTERR("offset of parition is not given\n");
		return 1;
	}
	FBTINFO("offset of partition is 0x%x\n", offset);

	/* now look for name */
	if (*s == '(')
		delim = ')';

	if (delim) {
		char *p;

		name = ++s;
		p = strchr((const char *)name, delim);
		if (!p) {
			FBTERR("no closing %c found in partition name\n", delim);
			return 1;
		}
		name_len = p - name;
		s = p + 1;
	} else {
		FBTERR("no partition name for \'%s\'\n", s);
		return 1;
	}
	FBTINFO("name of partition is %s ,name_len :0x%x\n", name, name_len);

	/* test for options */
	while (1) {
		if (strncmp(s, "i", 1) == 0) {
			flags |= FASTBOOT_PTENTRY_FLAGS_WRITE_I;
			s += 1;
		} else if (strncmp(s, "jffs2", 5) == 0) {
			/* yaffs */
			flags |= FASTBOOT_PTENTRY_FLAGS_WRITE_JFFS2;
			s += 5;
		} else if (strncmp(s, "swecc", 5) == 0) {
			/* swecc */
			flags |= FASTBOOT_PTENTRY_FLAGS_WRITE_SW_ECC;
			s += 5;
		} else if (strncmp(s, "hwecc", 5) == 0) {
			/* hwecc */
			flags |= FASTBOOT_PTENTRY_FLAGS_WRITE_HW_ECC;
			s += 5;
		} else {
			break;
		}
		if (strncmp(s, "|", 1) == 0)
			s += 1;
	}

	/* enter this partition (offset will be calculated later if it is zero at this point) */
	part.length = size;
	part.start = offset;
	part.flags = flags;
	FBTINFO("offset 0x%8.8x, size 0x%8.8x, flags 0x%8.8x\n",
		       part.start, part.length, part.flags);

	if (name) {
		if (name_len >= sizeof(part.name)) {
			FBTERR("partition name is too long\n");
			return 1;
		}
		strncpy(&part.name[0], name, name_len);
		/* name is not null terminated */
		part.name[name_len] = '\0';
	} else {
		FBTERR("no name\n");
		return 1;
	}


		FBTINFO("Adding: %s, offset 0x%8.8x, size 0x%8.8x, flags 0x%8.8x\n",
		       part.name, part.start, part.length, part.flags);
		fastboot_flash_add_ptn(&part);

	/* return (updated) pointer command line string */
	*retptr = s;

	/* return partition table */
	return 0;
}

static int fbt_add_partitions_from_environment(void)
{
	char fbparts[4096], *env;
	FBTDBG("%s\n",__FUNCTION__);

	/*
	 * Place the runtime partitions at the end of the
	 * static paritions.  First save the start off so
	 * it can be saved from run to run.
	 */
	 
	env = getenv("fbparts");
	if (env) {
		unsigned int len;
		len = strlen(env);
		if (len && len < 4096) {
			char *s, *e;
			if (memcmp(env, "WMT.nand:", 9) == 0) {

				memcpy(&fbparts[0], env, len + 1);
				printf("Adding partitions from environment\n");
				s = &fbparts[9];
				e = s + len - 9;
				
				while (s < e) {
					if (add_partition_from_environment(s, &s)) {
						printf("Abort adding partitions\n");
						/* reset back to static */
						pcount = static_pcount;
						break;
					}
					/* Skip a bunch of delimiters */
					while (s < e) {
						if ((' ' == *s) ||
						    ('\t' == *s) ||
						    ('\n' == *s) ||
						    ('\r' == *s) ||
						    (',' == *s)) {
							s++;
						} else {
							break;
						}
					}
				}
			} else {
				printf("ERROR! illegal fbparts found. \n");
			}
	
		}
	}else {
		printf("ERROR! no fbparts found. \n");
		return 1;
	}
	printf("pcount:0x%x,static_pcount:0x%x\n",pcount,static_pcount);

	return 0;
}

static void set_serial_number(void)
{
	FBTDBG("%s\n",__FUNCTION__);

	char *dieid = getenv("dieid#");
	if (dieid == NULL) {
		priv.serial_no = "00123";
	} else {
		int len;

		memset(&serial_number[0], 0, 28);
		len = strlen(dieid);
		if (len > 28)
			len = 26;

		strncpy(&serial_number[0], dieid, len);

		priv.serial_no = &serial_number[0];
	}
}

#define  fb_env_name  "wmt.fb.param"
struct fb_operation_t g_fb_param;

static int fbt_fastboot_init(void)
{
	unsigned char status = 0;
	
	if (parse_fb_param(fb_env_name) == 0) {
		if(g_fb_param.device > 0x01 ) {
			printf("wmt.fb.param device set error!, set 1:0:0:0 for NAND, 1:1:1:0 for EMMC1 \n");
			return -1;
		} 
	} else {
			printf("Fastboot device disabled.  \n");
			return -1;
	}

	if (g_fb_param.device == NAND) {
		status = fbt_add_partitions_from_environment();
		if (status)  {
			printf("Warning !Fastboot nand device no partitions.\n");
			printf("Please flash env1 and env2.\n");		
		}

		//init nand parrtition here
		init_nand_partitions();
	}

	priv.flag = 0;
	priv.d_size = 0;
	priv.d_bytes = 0;
	priv.u_size = 0;
	priv.u_bytes = 0;
	priv.exit = 0;
	priv.product_name = FASTBOOT_PRODUCT_NAME;
	set_serial_number();

	if (g_fb_param.device == NAND) {

		priv.nand_block_size               = FASTBOOT_NAND_BLOCK_SIZE;
		priv.nand_oob_size                 = FASTBOOT_NAND_OOB_SIZE;
		priv.storage_medium               = NAND;
	} else {
		priv.storage_medium               = EMMC;
		mmc_controller_no = g_fb_param.controller;
		mmc_part_type = g_fb_param.part_type;
		/*CharlesTu,2013.03.28,patch boot press enter before run logocmd*/
		if (mmc_init(1, mmc_controller_no)) {
			printf("mmc%d init failed!\n",mmc_controller_no);
			return -1;
		}

	}
	FBTDBG("%s,priv.storage_medium %x,mmc_controller_no %x\n",__FUNCTION__,priv.storage_medium,mmc_controller_no );

	return 0;
}

static int fbt_handle_erase(int part_no)
{
	char start[32], length[32];
	int status = 0;
	char *erase[5]  = { "nandrw", "erase",  NULL, NULL, NULL, };
	FBTDBG("%s,storage_medium:0x%x \n",__FUNCTION__,priv.storage_medium);
	if (priv.storage_medium == NAND) {
		struct fastboot_ptentry *ptn;
		ptn = fastboot_flash_find_nand_ptn(part_no);

		if (ptn == 0) {
			printf("FAILpartition does not exist\n");
			sprintf(priv.response, "FAILpartition does not exist");
		} else {
		    struct nand_chip *chip = get_current_nand_chip();
			unsigned long block_size = chip->dwPageCount * chip->oobblock;
            unsigned long startblk = ptn->start / block_size;
			unsigned long blknum = ptn->length / block_size;
			//FBTINFO("erasing '%s'\n", ptn->name);
			erase[2] = start;
			erase[3] = length;
			printf("erasing '%s',start blk = 0x%x , blk num = 0x%x\n",
			 ptn->name, startblk, blknum);		
			sprintf (start, "0x%08x", startblk);
			sprintf (length, "0x%08x",  blknum);			
			status = do_nand (NULL, 0, 4, erase);

			if (status) {
				printf("Nand partition '%s' erased FAIL\n", ptn->name);				
				sprintf(priv.response, "FAILfailed to erase nand partition");
			} else {
				printf("Nand partition '%s' erased\n", ptn->name);
				sprintf(priv.response, "OKAY");
			}
		}
	} 
	return status;			
}

extern int do_protect (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[]);

/**
emmc:
# /dev/sdx1  system      1024MB   ext4
# /dev/sdx2  boot        16MB    ext4
# /dev/sdx3  recovery    16MB    ext4
# /dev/sdx4  extended partition
# /dev/sdx5  cache       512MB   ext4
# /dev/sdx6  swap        128MB   vfat
# /dev/sdx7  u-boot-logo 2MB     vfat
# /dev/sdx8  kernel-logo 2MB     vfat
# /dev/sdx9  misc        2MB     ext4
# /dev/sdx10 efs         2MB     ext4
# /dev/sdx11 radio       16MB    ext4
# /dev/sdx12 keydata     2MB     ext4
# /dev/sdx13 userdata            ext4
 */
/**
 * return:
 *    Bit[7:0]  :STR_TYPE including STR_EXT4, STR_SIG, STR_ROM, STR_FAT.
 *    Bit[15:8] :Partition No.
 *    Bit[23:16]:Sub information for Partition No.
 *    Bit[31]   :Status bit. 1: Fail, 0: Success.
 */
unsigned int parse_flash_cmd(char *cmdbuf)
{	
	char *system_str[] = {
		"sub-system0", "sub-system1", "sub-system2", "sub-system3",
		"sub-system4", "sub-system5", "sub-system6", "sub-system7",
		"sub-system8", "sub-system9", "sub-system10", "sub-system11",
		"sub-system12", "sub-system13", "sub-system14", "sub-system15"
		};
	char *userdata_str[] = {
		"sub-userdata0", "sub-userdata1", "sub-userdata2", "sub-userdata3",
		"sub-userdata4", "sub-userdata5", "sub-userdata6", "sub-userdata7",
		"sub-userdata8", "sub-userdata9", "sub-userdata10", "sub-userdata11",
		"sub-userdata12", "sub-userdata13", "sub-userdata14", "sub-userdata15"
		};
	char *cache_str[] = {
		"sub-cache0", "sub-cache1", "sub-cache2", "sub-cache3",
		"sub-cache4", "sub-cache5", "sub-cache6", "sub-cache7",
		"sub-cache8", "sub-cache9", "sub-cache10", "sub-cache11",
		"sub-cache12", "sub-cache13", "sub-cache14", "sub-cache15"
		};		
	
	char *rom_str[] = {"w-load", "u-boot", "env1", "env2"};
	//char *u_boot_logo_str[] = {"u-boot-logo", "u-boot-logo-1", "u-boot-logo-2", "u-boot-logo-3","u-boot-logo-4", "u-boot-logo-5"};
	//char *kernel_logo_str[] = {"kernel-logo", "kernel-logo-1", "kernel-logo-2", "kernel-logo-3", "kernel-logo-4", "kernel-logo-5"};

	unsigned int i;
	
	/* For STR_RAM */
	if (memcmp(cmdbuf + 6, "ram:", 4) == 0) {
		return STR_RAM;
	}

	/* For STR_NOFS */
	/* For STR_EXT4, partition == 1 */
	if (strcmp("boot", cmdbuf + 6) == 0) {
		if (priv.storage_medium == EMMC) {
			return (STR_NOFS | (PART_BOOT << 8) | 0xFF0000);
		} else {
			return (STR_NAND | (NF_PART_BOOT << 8) | 0xFF0000);
		}
	}
		
	/* For STR_EXT4, partition == 1 */
	if (strcmp("recovery", cmdbuf + 6) == 0) {
		if (priv.storage_medium == EMMC) 		
			return (STR_NOFS | (PART_RECOVERY << 8) | 0xFF0000);
		else 
			return (STR_NAND |  (NF_PART_RECOVERY << 8) | 0xFF0000);
			
	}
	if (strcmp("misc", cmdbuf + 6) == 0) {
		if (priv.storage_medium == EMMC) 		
			return (STR_NOFS | (PART_MISC << 8) | 0xFF0000);
		else 
			return (STR_NAND |  (NF_PART_MISC << 8) | 0xFF0000);
			
	}
    /*
	if (strcmp("efs", cmdbuf + 6) == 0) {
		if (priv.storage_medium == EMMC) 		
			return (STR_NOFS | (PART_EFS << 8) | 0xFF0000);
		else 
			return (STR_NAND |  (NF_PART_EFS << 8) | 0xFF0000);
			
	}
	if (strcmp("radio", cmdbuf + 6) == 0) {
		if (priv.storage_medium == EMMC) 		
			return (STR_NOFS | (PART_RADIO<< 8) | 0xFF0000);
		else 
			return (STR_NAND |  (NF_PART_RADIO << 8) | 0xFF0000);
			
	}
	if (strcmp("swap", cmdbuf + 6) == 0) {
		if (priv.storage_medium == EMMC) 		
			return (STR_NOFS | (PART_SWAP << 8) | 0xFF0000);
		else 
			return (STR_NAND |  (NF_PART_SWAP << 8) | 0xFF0000);
			
	}
	*/
	if (strcmp("keydata", cmdbuf + 6) == 0) {
		if (priv.storage_medium == EMMC) 		
			return (STR_NOFS | (PART_KEYDATA<< 8) | 0xFF0000);
		else 
			return (STR_NAND |  (NF_PART_KEYDATA << 8) | 0xFF0000);
			
	}
	/* For STR_EXT4, partition == PART_SYSTEM */
	for (i = 0; i < 16; i++)
		if (strcmp(system_str[i], cmdbuf + 6) == 0) {
			if (priv.storage_medium == EMMC) 		
				return (STR_EXT4 | (PART_SYSTEM << 8) | (i << 16));
			else 
				return (STR_NAND | (NF_PART_SYSTEM << 8) | (i << 16));
		}			

    if (strcmp("system:ubi", cmdbuf+6) == 0){
        return (STR_UBI | (NF_PART_SYSTEM << 8) | 0xFF0000);	
	}
	
	/* For STR_EXT4, partition == 1 */
	if (strcmp("system", cmdbuf + 6) == 0) {
		if (priv.storage_medium == EMMC) 				
			return (STR_EXT4 | (PART_SYSTEM << 8) | 0xFF0000);
		else 
			return (STR_NAND | (NF_PART_SYSTEM << 8) | 0xFF0000);	
	}

	/* For STR_EXT4, partition == PART_USERDATA */
	for (i = 0; i < 16; i++) {
		if (strcmp(userdata_str[i], cmdbuf + 6) == 0) {
			if (priv.storage_medium == EMMC) 							
				return (STR_EXT4 | (PART_USERDATA << 8) | (i << 16));	
			else
				return (STR_NAND | (NF_PART_DATA << 8) | (i << 16));	
		}
	}

	if (strcmp("data:ubi", cmdbuf + 6) == 0) {
        return (STR_UBI | (NF_PART_DATA << 8) | 0xFF0000);
	}
	/* For STR_EXT4, partition other than PART_USERDATA */
	if (strcmp("data", cmdbuf + 6) == 0) {
		if (priv.storage_medium == EMMC) 									
			return (STR_EXT4 | (PART_USERDATA << 8) | 0xFF0000);
		else
			return (STR_NAND | (NF_PART_DATA << 8) | 0xFF0000);
			
	}


	
	
	/* For STR_EXT4, partition == PART_CACHE */
	for (i = 0; i < 16; i++) {
		if (strcmp(cache_str[i], cmdbuf + 6) == 0) {
			if (priv.storage_medium == EMMC) 												
				return (STR_EXT4 | (PART_CACHE << 8) | (i<<16));	
			else 
				return (STR_NAND | (NF_PART_CACHE << 8) | (i<<16));			
		}	
	}
	if (strcmp("cache", cmdbuf + 6) == 0) {
		if (priv.storage_medium == EMMC) 														
			return (STR_EXT4 | (PART_CACHE << 8) | 0xFF0000);
		else
			return (STR_NAND | (NF_PART_CACHE << 8) | 0xFF0000);

	}

	/* For STR_SIG */
	if (strcmp("sig", cmdbuf + 6) == 0) {
		return STR_SIG;
	}

	/* For STR_ROM */
	for (i = 0; i < 4; i++)
		if (strcmp(rom_str[i], cmdbuf + 6) == 0)
			return (STR_ROM | (i << 8));

	if (strcmp("logo", cmdbuf + 6) == 0) {
		if (priv.storage_medium == EMMC) 														
			return (STR_EXT4 | (PART_U_BOOT_LOGO << 8) | 0xFF0000);
		else
			return (STR_NAND | (NF_PART_LOGO << 8) | 0xFF0000);

	}
	/* For STR_FAT(u_boot_logo) */
    /*
	for (i = 0;i < 6; i++) {
		if (strcmp(u_boot_logo_str[i], cmdbuf + 6) == 0) {
			if (priv.storage_medium == EMMC) 															
				return (STR_FAT | (PART_U_BOOT_LOGO << 8) | (i << 16));
			else
				return (STR_NAND | (NF_PART_U_BOOT_LOGO << 8) | (i << 16));
		}	
	}
	*/
	/* For STR_FAT(kernel_logo) */
    /*
	for (i = 0; i < 6; i++) {
		if (strcmp(kernel_logo_str[i], cmdbuf + 6) == 0) {
			if (priv.storage_medium == EMMC) 															
				return (STR_FAT | (PART_KERNEL_LOGO << 8) | (i << 16));
			else
				return (STR_NAND | (NF_PART_KERNEL_LOGO << 8) | (i << 16));
				
		}
	}
	*/
	return 0x80000000;
}

int handle_flash_nand(int part_no, int ss, char *key, int haveoob)
{
	static unsigned char erase_part = 0;
	struct fastboot_ptentry *ptn;

	/*erase partition first*/
	if ((part_no == NF_PART_SYSTEM) ||
		(part_no == NF_PART_CACHE) ||
		(part_no == NF_PART_DATA)) {
		if(sparse_download_size != 0){
		   if(sparse_download_size == sparse_remained_size){
		       fbt_handle_erase(part_no);
               ptn = fastboot_flash_find_nand_ptn(part_no);
               sparse_nand_addr = ptn->start;			   
		   	}		   
		    sparse_remained_size -= priv.d_bytes;
		}else{		
			if (ss == 0 ) {
				fbt_handle_erase(part_no);
				erase_part = 1;
			}
			if ((ss == 0xff) && (erase_part == 0)) {
				fbt_handle_erase(part_no);
			} else if ((ss == 0xff) && (erase_part == 1)) {
				erase_part = 0;//clear
			}
		}
	} else {
		fbt_handle_erase(part_no);
	}
	
	/* Normal case */
	if (write_to_ptn(part_no, ss, key, haveoob)){
		printf("write to nand part_no:%d FAIL\n", part_no);
	} else {
		sprintf(priv.response, "OKAY");
	}
	return 0;
}

int handle_flash_ext4(int part_no, int ss, char *key)
{
	static unsigned char last_ss = 255;
	int status;
	unsigned int image_start;
	unsigned int image_size;
	block_dev_desc_t *dev_desc = NULL;
	disk_partition_t info;
	char slot_no[32];
      unsigned char init;

	sprintf(slot_no, "%d", mmc_controller_no);

	/*get mmc dev descriptor*/
	dev_desc = get_dev("mmc", mmc_controller_no);
	if (dev_desc == NULL) {
		printf ("mmc Invalid boot device\n");
		goto fail;
	}
	/**
	 * info.start, info.size, info.blksz
	 */
	if(!get_partition_info(dev_desc, part_no, &info)) {
		printf("MBR part_no:%d, part_start:0x%x, part_length:%d MB\n",
			part_no, info.start, (info.blksz * info.size)/(1024*1024));			
	} else {
		printf ("** Partition %d not valid on device %d **", part_no, dev_desc->dev);
		sprintf(priv.response, "FAIL partition not valid");
		goto fail;
	}

	if (ss == 0xFF) { /* system, userdata, cache */
		
		if (key) {
			if (priv.d_bytes > 0x10000000)
				status = image_rsa_check((unsigned int)priv.transfer_buffer, 0x10000000, sig_buf, sig_size);
			else 
				status = image_rsa_check((unsigned int)priv.transfer_buffer, priv.d_bytes, sig_buf, sig_size);
			if (status != 0 ) {
				printf("Ext4 rsa check fail, rcode :%x\n",status);
				sprintf(priv.response, "FAIL Ext4 rsa check FAIL");
				goto fail;
			}else {
				printf("Ext4 rsa check OK\n");			
			}
		}		
		image_start = SZ_128M;
		image_size = priv.d_bytes;
		if (last_ss % 2 == 0) {
			image_size = priv.d_bytes + 0x10000000;
		}
		init = ((last_ss == 0xFF) | (last_ss == 0)) ? 1 : 0; /* system only or system + ss0 or system + ss0 + ssXX */
		if (priv.d_bytes > 0x10000000)
			init = 2;
		if (!do_unsparse(
				init,
				(unsigned char *)SZ_128M, //priv.transfer_buffer,
				info.start,
				(info.blksz >> 9) * info.size,	/* in unit of 512B */
				slot_no)) {
			last_ss = 0xFF;
			priv.transfer_buffer = (unsigned char *)SZ_128M;
			printf("Writing sparsed part_no:%d DONE!\n", part_no);
			sprintf(priv.response, "OKAY");
			return 0;
		} else {
			printf("Writing sparsed part_no:%d FAILED!\n", part_no);
			sprintf(priv.response, "FAIL Sparsed Write");
			goto fail;
		}
	} else { /* sub-system */
		last_ss = ss;
		image_start = SZ_128M;
		image_size = priv.d_bytes;

		if (ss % 2 == 0)
			image_start = SZ_128M;
		else
			image_start = SZ_128M + 0x10000000;
			
		priv.transfer_buffer = (unsigned char *)(SZ_128M + 0x10000000 * ((ss % 2) ? 0 : 1));

		if (key) {
			status = image_rsa_check(image_start, image_size, sig_buf, sig_size);
			if (status != 0 ) {
				printf("ss-%d rsa check fail, rcode:0x%X\n", ss, status);
				sprintf(priv.response, "FAIL image RSA check fail");
				goto fail;
			} else {
				if (ss % 2 == 0) {
					printf("ss-%d rsa check OK\n",ss);
					sprintf(priv.response, "OKAY");
					return 0;
				}
			}
		}
		if (ss % 2 != 0) {
			/* sub-system1,3,5,... */
			if (!do_unsparse(
					(ss == 1) ? 1 : 0, /* ss0 + ss1, it is necessary to trigger init of unsparse */
					(unsigned char *)SZ_128M, //priv.transfer_buffer,
					info.start,
					(info.blksz >> 9) * info.size,	/* in unit of 512B */
					slot_no)) {
				printf("Writing sparsed part_no:%d DONE!\n", part_no);
				sprintf(priv.response, "OKAY");				
				return 0;
			} else {
				printf("Writing sparsed part_no:%d FAILED!\n", part_no);
				sprintf(priv.response, "FAIL Sparsed Write");
				goto fail;
			}
		} else {
			/* sub-system0,2,4,... */
			printf("Writing sparsed part_no:%d DONE!\n", part_no);
			sprintf(priv.response, "OKAY");				
			return 0;
		}
		
			
	}
fail:
	/* reset variables as fail */
	last_ss = 0xFF;
	priv.transfer_buffer = (unsigned char *)SZ_128M;
	
	return 1;
}

static env_t *flash_addr_1 = (env_t *)CFG_ENV_ADDR;
static env_t *flash_addr_new_2 = (env_t *)CFG_ENV_ADDR_REDUND;
int handle_flash_rom(int part_no, int ss, char *key)
{
	int status;
	char *sf_prot[4] = {"protect", "off", "bank", "1"};
	char *sf_copy[4]  = {"cp.b", NULL, NULL, NULL};
	char *sf_erase[3] = {"erase", NULL, NULL};	
	char start_addr[32], dest_addr[32], length[32];	
	char spi_addr[32], spi_size[32];	
	char *p = NULL;
	static unsigned char flash_env1 = 0, flash_env2 = 0;
	int crc1_ok=0, crc2_ok=0;
	FBTINFO("Protect off all spi flash\n" );

	if (key) {
		/*spi flash rsa check*/
		status = image_rsa_check((unsigned int)priv.transfer_buffer, priv.d_bytes, (unsigned int)sig_buf, sig_size);
		if (status != 0 ) {
			printf("Rom rsa check fail, rcode :%x\n", status);
			sprintf(priv.response, "FAIL rom rsa check FAIL");	
			return status;  /* rsa verify fail */
		} else {
			printf("Rom rsa check OK\n");
			sprintf(priv.response, "OKAY");
		}
	}

	sf_erase[1] = spi_addr;
	sf_erase[2] = spi_size;
	sf_copy[1] = start_addr;
	sf_copy[2] = dest_addr;
	sf_copy[3] = length;
	
	status = do_protect(NULL, 0, 4, sf_prot);
	if (status) {
		printf("FAIL protect off all SF\n");
		sprintf(priv.response, "FAIL protect off all SF");
		return 1;
	} else {
		sprintf(priv.response, "OKAY");							
	}

	switch (part_no) {
	case ROM_WLOAD : /* w-load */
		sprintf(spi_addr, "0x%x", 0xffff0000);
		sprintf(spi_size, "+%x", priv.d_bytes); // 1 sectors 64KB
		sprintf(start_addr, "0x%x", priv.transfer_buffer);
		sprintf(dest_addr, "0x%x", 0xffff0000);
		sprintf(length, "0x%x", priv.d_bytes);	
		break;
	case ROM_UBOOT : /* u-boot */
		sprintf(spi_addr, "0x%x", 0xfff80000);
		sprintf(spi_size, "+%x", priv.d_bytes); 
		sprintf(start_addr, "0x%x", priv.transfer_buffer);
		sprintf(dest_addr, "0x%x", 0xfff80000);
		sprintf(length, "0x%x", priv.d_bytes);	
		break;	
	case ROM_ENV1 : /* env1 */
		sprintf(spi_addr, "0x%x", 0xfffd0000);
		sprintf(spi_size, "+%x", priv.d_bytes);
		sprintf(start_addr, "0x%x", priv.transfer_buffer);
		sprintf(dest_addr, "0x%x", 0xfffd0000);
		sprintf(length, "0x%x", priv.d_bytes);
		flash_env1 = 1;		
		break;	
	case ROM_ENV2: /* env2 */
		sprintf(spi_addr, "0x%x", 0xfffe0000);
		sprintf(spi_size, "+%x", priv.d_bytes);
		sprintf(start_addr, "0x%x", priv.transfer_buffer);
		sprintf(dest_addr, "0x%x", 0xfffe0000);
		sprintf(length, "0x%x", priv.d_bytes);
		flash_env2 = 1;
		break;	
	default :
		printf("FAIL unknown partition no\n");
		sprintf(priv.response, "FAIL unknown partition no");
		return 1;
		break;
	}

	status = do_flerase(NULL, 0, 3, sf_erase);
	if (status) {
		printf("FAIL erase of SF\n");
		sprintf(priv.response, "FAIL erase of SF");
		return 1;
	} else {
		sprintf(priv.response, "OKAY");							
	}
	status = do_mem_cp(NULL, 0, 4, sf_copy);
	if (status) {
		sprintf(priv.response, "FAIL copy of SF");
		printf("FAIL copy of SF");
		return 1;								
	} else {
			sprintf(priv.response, "OKAY");
			if (flash_env1 && flash_env2) {
				FBTINFO("flash_addr_1->data:0x%x, flash_addr_new_2->data:0x%x, ENV_SIZE:0x%x\n", 
					flash_addr_1->data, flash_addr_new_2->data,ENV_SIZE);
				crc1_ok = (crc32(0, flash_addr_1->data, ENV_SIZE) == flash_addr_1->crc);	
				if (crc1_ok) {
					if (flash_addr_1->flags) {
						memcpy (env_ptr, (void*)flash_addr_1, CFG_ENV_SIZE);
						p=getenv("fbparts");
						printf("new SF env fbparts= %s, crc1_ok:%x\n", p,crc1_ok);
					} else {
						crc2_ok = (crc32(0, flash_addr_new_2->data, ENV_SIZE) == flash_addr_new_2->crc);	
						if (crc2_ok) {
							if (flash_addr_new_2->flags) {
								memcpy (env_ptr, (void*)flash_addr_new_2, CFG_ENV_SIZE);
								p=getenv("fbparts");
								printf("new SF env fbparts= %s,crc2_ok:%x\n", p,crc2_ok);
							}
						}
					}
					if ((g_fb_param.device == NAND) && p) {
						status = fbt_add_partitions_from_environment();
						if (status) 
							return 1;
					}					
				}
				flash_env1 = 0;
				flash_env2 = 0;				
			}
		}	
		return 0;
}

int handle_flash_fat(int part_no, int ss, char *key)
{
	int status;
	char slot_no[32], source[32], length[32];
	char *fat_write[6]  = {"fatstore", NULL, NULL, NULL, NULL, NULL};

	if (key) {
		/*spi flash rsa check*/
		status = image_rsa_check((unsigned int)priv.transfer_buffer, priv.d_bytes, (unsigned int)sig_buf, sig_size);
		if (status != 0 ) {
			printf("Fat rsa check fail, rcode :%x\n", status);
			sprintf(priv.response, "FAIL image RSA Check Fail");	
			return status;  /* rsa verify fail */
		} else {
			printf("Fat rsa check OK\n");
			sprintf(priv.response, "OKAY");
		}
	}
	
	sprintf(slot_no, "%d", mmc_controller_no);
	
	fat_write[1] = "mmc";
	fat_write[2] = slot_no;
	fat_write[3] = source;
	fat_write[5] = length;
	
	sprintf(slot_no, "%d:%d", mmc_controller_no, part_no);
	sprintf(source, "0x%x", priv.transfer_buffer);
	sprintf(length, "0x%x", priv.d_bytes);	

	if (part_no == PART_U_BOOT_LOGO) {
		if (ss==0)
			fat_write[4] = "u-boot-logo.data";
		else if (ss==1)
			fat_write[4] = "u-boot-logo-1.data";
		else if (ss==2)
			fat_write[4] = "u-boot-logo-2.data";
		else if (ss==3)
			fat_write[4] = "u-boot-logo-3.data";
		else if (ss==4)
			fat_write[4] = "u-boot-logo-4.data";
		else if (ss==5)
			fat_write[4] = "u-boot-logo-5.data";
	} else {
		if (ss==0)
			fat_write[4] = "kernel-logo.data";
		else if (ss==1)
			fat_write[4] = "kernel-logo-1.data";
		else if (ss==2)
			fat_write[4] = "kernel-logo-2.data";
		else if (ss==3)
			fat_write[4] = "kernel-logo-3.data";
		else if (ss==4)
			fat_write[4] = "kernel-logo-4.data";
		else if (ss==5)
			fat_write[4] = "kernel-logo-5.data";	
	}
	
	if (do_fat_fsstore(NULL, 0, 6, fat_write)) {
		printf("Writing part_no:'%d' FAILED!\n", part_no);
		sprintf(priv.response, "FAIL write logo");
		return 1;
	} else {
		printf("Writing part_no:'%d' DONE!\n", part_no);
		sprintf(priv.response, "OKAY");
	}
	
	return 0;
}

int handle_flash_nofs(int part_no, int ss, char *key)
{
	int status;
	block_dev_desc_t *dev_desc = NULL;
	disk_partition_t info;	
	char slot_no[32], source[32], dest[32], length[32];
	char *mmc_write[6]  = {"mmc", NULL, "write", NULL, NULL, NULL};

	/*get mmc dev descriptor*/
	dev_desc = get_dev("mmc", mmc_controller_no);
	if (dev_desc == NULL) {
		printf ("mmc Invalid boot device \n");
		goto fail;
	}

	/**
	 * info.start, info.size, info.blksz
	 */
	if(!get_partition_info(dev_desc, part_no, &info)) {
		printf("MBR part_no:%d, info.start:0x%x, length:%d MB \n",
				part_no, info.start, info.size * info.blksz / (1024*1024));			
	} else {
		printf ("** Partition %d not valid on device %d **", 
				part_no, dev_desc->dev);
		goto fail;
	}		
	/*no need to do rsa check*/
	if (0) {
		status = image_rsa_check((unsigned int)priv.transfer_buffer, priv.d_bytes, (unsigned int)sig_buf, sig_size);
		if (status != 0 ) {
			printf("Nofs rsa check fail, rcode :%x\n", status);
			sprintf(priv.response, "FAIL nofs rsa check FAIL");	
			return status;  /* rsa verify fail */
		} else {
			printf("Nofs rsa check OK\n");
			sprintf(priv.response, "OKAY");
		}
	}

	sprintf(slot_no, "%d", mmc_controller_no);

	mmc_write[1] = slot_no;
	mmc_write[2] = source;
	mmc_write[3] = dest;
	mmc_write[4] = length;
							
	sprintf(slot_no, "%d:%d", mmc_controller_no, part_no);
	sprintf(source, "0x%x", priv.transfer_buffer);
	sprintf(dest, "0x%x", info.start);	
	sprintf(length, "0x%x", priv.d_bytes);	

	if (do_mmc_write(NULL, 0, 5, mmc_write)) {
		printf("Writing part_no:'%d' FAILED!\n", part_no);
		sprintf(priv.response, "FAIL write partition");
	} else {
		printf("Writing part_no:'%d' DONE!\n", part_no);
		sprintf(priv.response, "OKAY");
	}
fail:	
	return 0;
}

int fbt_handle_flash(char *cmdbuf)
{
	unsigned int flash_cmd_sts, ram_addr, cp_loop, i;
	unsigned char str_type, part_no, ss;
	char *key = NULL;
	
	printf("\ncmd:[%s]\n",cmdbuf);
	key = getenv("wmt.rsa.pem");
	if (!key)
		printf("No RSA public key !! \n");
	
	flash_cmd_sts = parse_flash_cmd(cmdbuf);
	if (flash_cmd_sts & 0x80000000) {
		printf("FAIL parse flash cmd fail\n");
		sprintf(priv.response, "FAIL parse flash cmd FAIL");
		return 1;
	}
	
	str_type = flash_cmd_sts & 0xFF;
	part_no = (flash_cmd_sts & 0xFF00) >> 8;
	ss = (flash_cmd_sts & 0xFF0000) >> 16;

	printf("str_type:%d, part_no:%d, ss:%d\n", str_type, part_no, ss);
	
	switch (str_type) {
	case STR_EXT4 :
		handle_flash_ext4(part_no, ss, key);
		break;
	case STR_SIG :
		memset((void *)sig_buf, 0, priv.d_bytes);
		memcpy((void *)sig_buf, priv.transfer_buffer, priv.d_bytes);
		sig_size = priv.d_bytes;
		printf("cmd:[%s]:OKAY,sig_size:0x%x\n", cmdbuf, sig_size);
		sprintf(priv.response, "OKAY");
		break;
	case STR_ROM :
		handle_flash_rom(part_no, ss, key);
		break;
	case STR_FAT :
		handle_flash_fat(part_no, ss, key);
		break;
	case STR_NOFS :
		handle_flash_nofs(part_no, ss, key);
		break;
	case STR_RAM :
		ram_addr =  simple_strtoul ((void *)(cmdbuf + 10), NULL, 16);
		/*uboot u-boot.map start from 0x03f80000 to 0x0414990c*/
	    	if ((ram_addr >= 0x03f80000) && (ram_addr <= 0x04200000)) {
			printf("cmd:[%s]:FAIL, ram addr between u-boot.map, please change ram addr.\n", cmdbuf);				
			sprintf(priv.response, "FAILu-boot.map address");
	    	} else {		
		    	if (ram_addr == 0x8000000) {	 				
				printf("ram_addr = 0x8000000, no need copy\n");				
		    	} else if ((ram_addr > 0x8000000) && (ram_addr <= (0x8000000 + priv.d_bytes))) {
		    		cp_loop = priv.d_bytes / 32;
				if (priv.d_bytes % 32)
					cp_loop++;
				for (i = cp_loop; i > 0; i--)
					memcpy((void *)ram_addr + (i-1) * 32  , priv.transfer_buffer + (i - 1) * 32, 32);	
		    	} else {
				memcpy_itp((void *)ram_addr, priv.transfer_buffer, priv.d_bytes);			    	
		    	}
				printf("cmd:[%s]:OKAY,ram addr:0x%x\n", cmdbuf, ram_addr);				
				sprintf(priv.response, "OKAY");				
	    	}
		
		break;
	case STR_NAND :
		handle_flash_nand(part_no, ss, key, 1);
		break;
	case STR_UBI:
		handle_flash_nand(part_no, ss, key, 0);
		break;
	default:
		printf("FAIL unknown storage type\n");
		sprintf(priv.response, "FAIL unknown storage type");
		break;
	}
	return 0;
}

static int fbt_handle_getvar(char *cmdbuf)
{
	FBTDBG("%s\n",__FUNCTION__);

	strcpy(priv.response, "OKAY");
	if (!memcmp(cmdbuf + strlen("getvar:"), "version",7)) {
		FBTINFO("getvar version\n");
		strcpy(priv.response + 4, FASTBOOT_VERSION);
	} 
	if (!memcmp(cmdbuf + strlen("getvar:"), "version-bootload", 16)) {
		FBTINFO("getvar version-bootloader\n");
		strcpy(priv.response + 4, fb_version_bootloader);
	}
	if (!memcmp(cmdbuf + strlen("getvar:"), "version-baseband", 16)) {
		FBTINFO("getvar version-baseband\n");
		strcpy(priv.response + 4, fb_version_baseband);	
	}
	if (!memcmp(cmdbuf + strlen("getvar:"), "secure", 6)) {
		FBTINFO("getvar secure\n");
		strcpy(priv.response + 4, SECURE);
	} 
	if (!memcmp(cmdbuf + strlen("getvar:"), "product", 7)) {
		FBTINFO("getvar product\n");
		if (priv.product_name)
			strcpy(priv.response + 4, priv.product_name);
	} 
	if (!memcmp(cmdbuf + strlen("getvar:"), "serialno", 8)) {
		FBTINFO("getvar serialno\n");
		if (priv.serial_no)
			strcpy(priv.response + 4, priv.serial_no);
	}
		priv.flag |= FASTBOOT_FLAG_RESPONSE;

	return 0;
}
static int fbt_handle_reboot(char *cmdbuf)
{
	FBTDBG("%s\n", __FUNCTION__);

	strcpy(priv.response, "OKAY");
	priv.flag |= FASTBOOT_FLAG_RESPONSE;
	fbt_handle_response();
	udelay (1000000); /* 1 sec */
 	disable_interrupts();
	reset_cpu (0);
	return 0;
}

static int fbt_handle_boot(char *cmdbuf)
{
	FBTDBG("%s\n",__FUNCTION__);

	if ((priv.d_bytes) &&
		(CONFIG_FASTBOOT_MKBOOTIMAGE_PAGE_SIZE < priv.d_bytes)) {
		char start[32];
		char *bootm[3] = { "bootm", NULL, NULL, };
		char *go[3]    = { "go",    NULL, NULL, };

		/*
		 * Use this later to determine if a command line was passed
		 * for the kernel.
		 */
		struct fastboot_boot_img_hdr *fb_hdr =
			(struct fastboot_boot_img_hdr *) priv.transfer_buffer;

		/* Skip the mkbootimage header */
		image_header_t *hdr = (image_header_t *)
		  &priv.transfer_buffer[CONFIG_FASTBOOT_MKBOOTIMAGE_PAGE_SIZE];

		bootm[1] = go[1] = start;
		sprintf (start, "0x%x", hdr);

		/* Execution should jump to kernel so send the response
		   now and wait a bit.  */
		sprintf(priv.response, "OKAY");
		priv.flag |= FASTBOOT_FLAG_RESPONSE;
		fbt_handle_response();
		udelay (1000000); /* 1 sec */

		if (ntohl(hdr->ih_magic) == IH_MAGIC) {
			/* Looks like a kernel.. */
			FBTINFO("Booting kernel..\n");

			/*
			 * Check if the user sent a bootargs down.
			 * If not, do not override what is already there
			 */
			if (strlen ((char *) &fb_hdr->cmdline[0]))
				set_env ("bootargs", (char *) &fb_hdr->cmdline[0]);

			do_bootm (NULL, 0, 2, bootm);
		} else {
			/* Raw image, maybe another uboot */
			FBTINFO("Booting raw image..\n");

			do_go (NULL, 0, 2, go);
		}

		FBTERR("booting failed, reset the board\n");
	}
	sprintf(priv.response, "FAILinvalid boot image");

	return 0;
}

#ifdef	FASTBOOT_UPLOAD
static int fbt_handle_upload(char *cmdbuf)
{
	unsigned int adv, delim_index, len;
	struct fastboot_ptentry *ptn;
	unsigned int is_raw = 0;

	/* Is this a raw read ? */
	if (memcmp(cmdbuf, "uploadraw:", 10) == 0) {
		is_raw = 1;
		adv = 10;
	} else {
		adv = 7;
	}

	/* Scan to the next ':' to find when the size starts */
	len = strlen(cmdbuf);
	for (delim_index = adv;	delim_index < len; delim_index++) {
		if (cmdbuf[delim_index] == ':') {
			/* WARNING, cmdbuf is being modified. */
			*((char *) &cmdbuf[delim_index]) = 0;
			break;
		}
	}

	ptn = fastboot_flash_find_ptn(cmdbuf + adv);
	if (ptn == 0) {
		sprintf(priv.response, "FAIL partition does not exist");
	} else {
		/* This is how much the user is expecting */
		unsigned int user_size;
		/*
		 * This is the maximum size needed for
		 * this partition
		 */
		unsigned int size;
		/* This is the length of the data */
		unsigned int length;
		/*
		 * Used to check previous write of
		 * the parition
		 */
		char env_ptn_length_var[128];
		char *env_ptn_length_val;

		user_size = 0;
		if (delim_index < len)
			user_size = simple_strtoul(cmdbuf + delim_index +
				1, NULL, 16);
		/* Make sure output is padded to block size */
		length = ptn->length;
		sprintf(env_ptn_length_var, "%s_nand_size", ptn->name);
		env_ptn_length_val = getenv(env_ptn_length_var);
		if (env_ptn_length_val) {
			length = simple_strtoul(env_ptn_length_val, NULL, 16);
			/* Catch possible problems */
			if (!length)
				length = ptn->length;
		}
		size = length / priv.nand_block_size;
		size *= priv.nand_block_size;
		if (length % priv.nand_block_size)
			size += priv.nand_block_size;
		if (is_raw)
			size += (size / priv.nand_block_size) *
				priv.nand_oob_size;
		if (size > priv.transfer_buffer_size) {
			sprintf(priv.response, "FAILdata too large");
		} else if (user_size == 0) {
			/* Send the data response */
			sprintf(priv.response, "DATA%08x", size);
		} else if (user_size != size) {
			/* This is the wrong size */
			sprintf(priv.response, "FAIL");
		} else {
			/*
			 * This is where the transfer
			 * buffer is populated
			 */
			unsigned char *buf = priv.transfer_buffer;
			char start[32], length[32], type[32], addr[32];
			char *read[6] = { "nand", NULL, NULL,
				NULL, NULL, NULL, };
			/*
			 * Setting upload_size causes
			 * transfer to happen in main loop
			 */
			priv.u_size = size;
			priv.u_bytes = 0;

			/*
			 * Poison the transfer buffer, 0xff
			 * is erase value of nand
			 */
			memset(buf, 0xff, priv.u_size);
			/* Which flavor of read to use */
			if (is_raw)
				sprintf(type, "read.raw");
			else
				sprintf(type, "read.i");

			sprintf(addr, "0x%x", priv.transfer_buffer);
			sprintf(start, "0x%x", ptn->start);
			sprintf(length, "0x%x", priv.u_size);

			read[1] = type;
			read[2] = addr;
			read[3] = start;
			read[4] = length;

			do_nand(NULL, 0, 5, read);

			/* Send the data response */
			sprintf(priv.response, "DATA%08x", size);
		}
	}

	return 0;
}
#endif 

/* cmdbuf max size is URB_BUF_SIZE , which is 128 byte */
static int fbt_handle_setenv(char *cmdbuf)
{
    char *p1, *p2;
	char name[64]={0};
	char value[128]={0};
	int  nameLen=0;
	FBTDBG("%s\n",__FUNCTION__);

	strcpy(priv.response, "OKAY");

    p1 = cmdbuf + strlen("setenv:");
	p2 = strstr(p1, "=");
    nameLen = p2 - p1;

	strncpy(name, p1, nameLen);
	strcpy(value, p2+1);

    set_env(name, value);

	return 0;
}

static int fbt_handle_syncenv(char* cmdbuf){
    char *p1, *p2;
	char envaddr[16]={0};
	char blacklistaddr[16]={0};
	char *syncenv[4]  = { "syncenv", NULL, NULL, NULL, };
    int len = 0;

	FBTDBG("%s\n",__FUNCTION__);

	strcpy(priv.response, "OKAY");

	syncenv[1] = envaddr;
	syncenv[2] = blacklistaddr;

	p1 = cmdbuf + strlen("syncenv:");
	p2 = strstr(p1, " ");
	len = p2 - p1;

	strncpy(envaddr, p1, len);
	strcpy(blacklistaddr, p2+1);

	do_syncenv(NULL, 0, 3, syncenv);

	return 0;
}

static int wload_buildid(char *ver, int len){
    char *wload =(char *)0xffff0000;
	int wload_len = 0x10000;
    int i = 0, found = 0;

	for(;i<wload_len; i++){
		if(memcmp(wload+i, "BUILDID_", 8) == 0){
            found = 1;
			break;
		}
	}

    if(found){
        int begin = i, end = 0; 
		int vlen = 0;
		for(end = begin; end < wload_len && (wload[end]!='\0' && wload[end]!=' '); end++);
		if(end >= wload_len || end == begin) return -1;
		vlen = end-begin;
		if(vlen > len){
            printf("version value is too long\n");
			return -1;
		} 
		strncpy(ver, wload+begin, vlen);
		return 0;
	}
	
	return -1;
}

/* XXX: Replace magic number & strings with macros */
static int fbt_rx_process(unsigned char *buffer, int length)
{
	int ret = 1;
	unsigned int erase_cmd_sts;
	/* Generic failed response */
	strcpy(priv.response, "FAIL");

	if (!priv.d_size) {
		/* command */
		char *cmdbuf = (char *) buffer;
		//{JHT
		*(cmdbuf + length)=0;
		//}

		FBTDBG("command %s\n", cmdbuf);

		if(memcmp(cmdbuf, "getvar:", 7) == 0) {
			FBTDBG("getvar\n");
			fbt_handle_getvar(cmdbuf);
		}

		if(memcmp(cmdbuf, "erase:", 6) == 0) {
			//fbt_handle_erase(cmdbuf);
			erase_cmd_sts = parse_flash_cmd(cmdbuf);
			if (erase_cmd_sts & 0x80000000) {
				printf("FAIL parse flash cmd fail\n");
				sprintf(priv.response, "FAIL parse flash cmd FAIL");
				return 1;
			}
			part_no = (erase_cmd_sts & 0xFF00) >> 8;
			//part_no  = simple_strtoul ((void *)(cmdbuf + 6), NULL, 16);
			FBTDBG("erase nand part_no:0x%x\n", part_no);			
			fbt_handle_erase(part_no);
		}

		if(memcmp(cmdbuf, "flash:", 6) == 0) {
			FBTDBG("flash\n");
			fbt_handle_flash(cmdbuf);
		}
		if((strcmp("reboot", cmdbuf) == 0) ||						
			(strcmp("reboot-bootloader", cmdbuf) == 0)) {
			FBTDBG("reboot/reboot-bootloader\n");
			if (strcmp("reboot-bootloader", cmdbuf) == 0) {
				printf("reboot-bootloader\n");
				*((unsigned int *)0xd8130040) |= 0x100;
			}	
				
			fbt_handle_reboot(cmdbuf);
		}

		if(memcmp(cmdbuf, "continue", 8) == 0) {
			FBTDBG("continue\n");
			strcpy(priv.response,"OKAY");
			priv.exit = 1;
		}

		if(memcmp(cmdbuf, "boot", 4) == 0) {
			FBTDBG("boot\n");
			fbt_handle_boot(cmdbuf);
		}

		if(memcmp(cmdbuf, "download:", 9) == 0) {
			FBTDBG("download\n");

			/* XXX: need any check for size & bytes ? */
			priv.d_size = simple_strtoul (cmdbuf + 9, NULL, 16);
			priv.d_bytes = 0;

			FBTINFO ("Starting download of %d bytes\n",
				priv.d_size);

			if (priv.d_size == 0) {
				strcpy(priv.response, "FAILdata invalid size");
			} else if (priv.d_size >
					priv.transfer_buffer_size) {
				priv.d_size = 0;
				strcpy(priv.response, "FAILdata too large");
			} else {
				sprintf(priv.response, "DATA%08x", priv.d_size);
			}

		}
		if (memcmp(cmdbuf, "oem ", 4) == 0) {
			cmdbuf += 4;
            FBTDBG("oem\n");
			if (priv.storage_medium == EMMC) {

				/* fastboot oem format */
				if(memcmp(cmdbuf, "format", 6) == 0) {
					FBTDBG("oem format\n");

					ret = fastboot_oem(cmdbuf);
					if (ret < 0) {
						strcpy(priv.response,"FAIL");
					} else {
						strcpy(priv.response,"OKAY");
					}
				}

				/* fastboot oem recovery */
				else if(memcmp(cmdbuf, "recovery", 8) == 0) {
					sprintf(priv.response,"OKAY");
					fbt_handle_response();
					
					while(1);
				}

				/* fastboot oem unlock */
				else if(memcmp(cmdbuf, "unlock", 6) == 0) {
					sprintf(priv.response,"FAIL");
					printf("\nfastboot: oem unlock "\
							"not implemented yet!!\n");
				}
				
			}

			if(memcmp(cmdbuf, "nand:", 5) == 0){
                /*only one nand now*/
                FBTDBG("oem %s\n", cmdbuf);
				char * nandmfr = "UNKNOWN";
				struct nand_chip * chip = get_current_nand_chip();
                tellme_nandinfo(chip);
				strcpy(priv.response, "OKAY");
				switch (chip->mfr) {
				case NAND_HYNIX:
					nandmfr="HYNIX";
					break;
				case NAND_SAMSUNG:
					nandmfr="SAMSUNG";
					break;
				case NAND_TOSHIBA:
					nandmfr="TOSHIBA";
					break;
				case NAND_SANDISK:
					nandmfr="SANDISK";
					break;
				case NAND_MICRON:
					nandmfr="MICRON";
					break;
				case NAND_INTEL:
					nandmfr="INTEL";
					break;
				case NAND_MXIC:
					nandmfr="MXIC";
					break;
				case NAND_MIRA:
					nandmfr="MIRA";
					break;
				}
				sprintf(priv.response+4, "mfr=%s:block=%d:page=%d:oob=%d",
					    nandmfr, chip->erasesize, chip->oobblock, FASTBOOT_NAND_OOB_SIZE);
			}

			else if(memcmp(cmdbuf, "setenv:", 7) == 0){
				fbt_handle_setenv(cmdbuf);
			}

			else if(memcmp(cmdbuf, "saveenv", 7) == 0){
				char *saveenv[2]  = { "saveenv", NULL,};
                strcpy(priv.response, "OKAY");
				do_saveenv(NULL, 0, 1, saveenv);
			}

			else if(memcmp(cmdbuf, "syncenv:", 8) == 0){
				fbt_handle_syncenv(cmdbuf);
			} 

			else if(memcmp(cmdbuf, "uboot-buildid", 13) == 0){
				//priv.response is 65 bytes
                sprintf(priv.response, "OKAY%s", WMT_UBOOT_BUILD_ID);
			}
			
			else if(memcmp(cmdbuf, "wload-buildid", 13) == 0){
                char wloadbid[64]={0};
				
				if(wload_buildid(wloadbid, sizeof(wloadbid)) >= 0){
                    sprintf(priv.response, "OKAY%s", wloadbid);
				}
			}

			else if(memcmp(cmdbuf, "max-download-size", 17) == 0){
				struct nand_chip * chip = get_current_nand_chip();
				if(memcmp(cmdbuf + 18, "yaffs2", 6) == 0){
	                unsigned long blockSize = chip->oobblock * chip->dwPageCount;
					unsigned long blockSizeWithOOB = (chip->oobblock + FASTBOOT_NAND_OOB_SIZE) * chip->dwPageCount;
					sprintf(priv.response, "OKAY%d", (MAX_DOWNLOAD_SIZE/blockSize)*blockSizeWithOOB);
				}else{
                    unsigned long blockSize = chip->oobblock * chip->dwPageCount;
					sprintf(priv.response, "OKAY%d", (MAX_DOWNLOAD_SIZE/blockSize)*blockSize);
				}
			}

			else if(memcmp(cmdbuf, "sparsedownload:", 15) == 0){
                char *p = cmdbuf+15;
				if(sparse_download_size == 0){
					sparse_download_size = simple_strtoull(p, NULL, 10);
                    sparse_remained_size = sparse_download_size;
					printf("sparese download 0x%04x%08x\n", (unsigned long)(sparse_download_size>>32), (unsigned long)sparse_download_size);
	                strcpy(priv.response, "OKAY");
				}else{
				    sparse_remained_size = 0;
                    sparse_download_size = 0;
					strcpy(priv.response, "FAIL");
				}
				sparse_nand_addr = 0;
			}

			else if(memcmp(cmdbuf, "sparsedownload end", 18) == 0){
				if(sparse_remained_size == 0){
				   strcpy(priv.response, "OKAY");
				}else{
				   strcpy(priv.response, "FAIL");
                   sparse_remained_size = 0;
				}
				sparse_download_size = 0;
			}
			sparse_nand_addr = 0;			
		}
		
		if((memcmp(cmdbuf, "upload:", 7) == 0) ||
			(memcmp(cmdbuf, "uploadraw", 10) == 0)) {
			FBTDBG("upload/uploadraw\n");
			if (g_fb_param.device == NAND) {
#ifdef	FASTBOOT_UPLOAD
				fbt_handle_upload(cmdbuf);
#endif
			}
		}
		priv.flag |= FASTBOOT_FLAG_RESPONSE;
	} else {
		if (length) {
			unsigned int xfr_size;
			xfr_size = priv.d_size - priv.d_bytes;
			if (xfr_size > length)
				xfr_size = length;
			priv.d_bytes += xfr_size;

            //FBTDBG("buffer : 0x%x, reeived bytes: 0x%x, required bytes: 0x%x\n", buffer, priv.d_bytes, priv.d_size);

			if (! (priv.d_bytes % (16 * priv.nand_block_size)))
				printf(".");
			
			if (priv.d_bytes >= priv.d_size) {
				udc_download_flag = 0;
				priv.d_size = 0;
				strcpy(priv.response, "OKAY");
				priv.flag |= FASTBOOT_FLAG_RESPONSE;
#ifdef	INFO
				printf(".\n");
#endif
				FBTINFO("\ndownloaded %d bytes\n", priv.d_bytes);
			}
		} else
			FBTWARN("empty buffer download\n");
	}

	return 0;
}

extern unsigned char *bulk_out_dma1;
extern int udc_cmd_flag;
extern unsigned int UBE_MAX_DMA;
static int fbt_handle_rx(void)
{
	struct usb_endpoint_instance *ep = &endpoint_instance[TX_EP_INDEX];
	if (ep->rcv_urb->actual_length) {
		FBTDBG("ep->rcv_urb->actual_length: 0x%x\n", ep->rcv_urb->actual_length);
		if (udc_download_flag && !udc_cmd_flag) {
			fbt_rx_process(priv.transfer_buffer + priv.d_bytes, ep->rcv_urb->actual_length);			
		} else { 
			fbt_rx_process(ep->rcv_urb->buffer, ep->rcv_urb->actual_length);
		}	
		 if (udc_cmd_flag || (ep->rcv_urb->actual_length < UBE_MAX_DMA)) {
				memset(ep->rcv_urb->buffer, 0, ep->rcv_urb->actual_length);
		 }
		ep->rcv_urb->actual_length = 0;
	}

	return 0;
}
#define MIN(a,b) ((a) < (b) ? (a) : (b))  
static int fbt_response_process(void)
{
	struct usb_endpoint_instance *ep = &endpoint_instance[RX_EP_INDEX];
	struct urb *current_urb = NULL;
	unsigned char *dest = NULL;
	int n, ret = 0;
	FBTDBG("%s\n",__FUNCTION__);

	current_urb = next_urb (device_instance, ep);
	if (!current_urb) {
		FBTERR("%s: current_urb NULL", __func__);
		return -1;
	}

	dest = current_urb->buffer + current_urb->actual_length;
	n = MIN(FASTBOOT_RESPONSE_BUF_SZ, strlen(priv.response));
	memcpy(dest, priv.response, n);
	current_urb->actual_length += n;
	printf("response urb length: %u (%s)\n", current_urb->actual_length, priv.response);
	if (ep->last == 0) {
		udc_endpoint_write(ep);
	}

	return ret;
}

static int fbt_handle_response(void)
{

	if (priv.flag & FASTBOOT_FLAG_RESPONSE) {
		fbt_response_process();
		priv.flag &= ~FASTBOOT_FLAG_RESPONSE;
	}

	return 0;
}

#ifdef	FASTBOOT_UPLOAD
static int fbt_tx_process(void)
{
	struct usb_endpoint_instance *ep = &endpoint_instance[RX_EP_INDEX];
	struct urb *current_urb = NULL;
	unsigned char *dest = NULL;
	int n = 0, ret = 0;

	current_urb = next_urb (device_instance, ep);
	if (!current_urb) {
		FBTERR("%s: current_urb NULL", __func__);
		return -1;
	}

	dest = current_urb->buffer + current_urb->actual_length;
	n = MIN (64, priv.u_size - priv.u_bytes);
	memcpy(dest, priv.transfer_buffer + priv.u_bytes, n);
	current_urb->actual_length += n;
	if (ep->last == 0) {
		 udc_endpoint_write (ep);
		/* XXX: "ret = n" should be done iff n bytes has been
		 * transmitted, "udc_endpoint_write" to be changed for it,
		 * now it always return 0.
		 */
		return n;
	}

	return ret;
}

static int fbt_handle_tx(void)
{
	if (priv.u_size) {
		int bytes_written = fbt_tx_process();

		if (bytes_written > 0) {
			/* XXX: is this the right way to update priv.u_bytes ?,
			 * may be "udc_endpoint_write()" can be modified to
			 * return number of bytes transmitted or error and
			 * update based on hence obtained value
			 */
			priv.u_bytes += bytes_written;
#ifdef	INFO
			/* Inform via prompt that upload is happening */
			if (! (priv.d_bytes % (16 * priv.nand_block_size)))
				printf(".");
			if (! (priv.d_bytes % (80 * 16 * priv.nand_block_size)))
				printf("\n");
#endif
			if (priv.u_bytes >= priv.u_size)
#ifdef	INFO
				printf(".\n");
#endif
				priv.u_size = priv.u_bytes = 0;
				FBTINFO("data upload finished\n");
		} else {
			FBTERR("bytes_written: %d\n", bytes_written);
			return -1;
		}

	}

	return 0;
}
#endif 

//--> added by howayhuo for checking if the device's USB connects to PC
static int s_udc_init_ok;

void udc_fastboot_exit(void)
{
	if(s_udc_init_ok == 0)
		return;

	udc_disable();
	udc_disconnect();

	s_udc_init_ok = 0;
	printf("Fastboot end\n");
}

int udc_fastboot_init(void)
{
	int ret = 0;

	if(s_udc_init_ok)
		udc_fastboot_exit();

	ret = fbt_fastboot_init();
	if (ret  < 0) {
		printf("Fastboot init failure\n");
		s_udc_init_ok = 0;
		return ret;
	}

	ret = fbt_init_endpoint_ptrs();

	if ((ret = udc_init()) < 0) {
		printf("MUSB UDC init failure\n");
		s_udc_init_ok = 0;
		return ret;
	}

	ret = fbt_init_strings();
	ret = fbt_init_instances();

	udc_startup_events (device_instance);
	udc_connect();

	s_udc_init_ok = 1;
	printf("Fastboot initialized\n");

	return 0;
}

void udc_fastboot_transfer(void)
{
	wmt_udc_irq();
	if (priv.configured) {
		fbt_handle_rx();
		fbt_handle_response();
	}
}

int udc_fastboot_is_init(void)
{
	return s_udc_init_ok;
}
//<-- end add

/* command */
int do_fastboot(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	int ret = 0;
	ret = fbt_fastboot_init();
	if (ret  < 0) {
		printf("Fastboot init failure\n");
		return ret;
	}

	ret = fbt_init_endpoint_ptrs();

	if ((ret = udc_init()) < 0) {
		printf("MUSB UDC init failure\n");
		return ret;
	}

	ret = fbt_init_strings();
	ret = fbt_init_instances();

	udc_startup_events (device_instance);
	udc_connect();

	printf("Fastboot initialized\n");

	while(1) {
		wmt_udc_irq();  
		if (priv.configured) {
			fbt_handle_rx();
			fbt_handle_response();
#ifdef	FASTBOOT_UPLOAD
			fbt_handle_tx();
#endif
		}
		priv.exit |= ctrlc();
		if (priv.exit) {
			udc_disable();
			udc_disconnect();
			restore_original_string_serial();
            priv.configured = 0;
			sparse_remained_size = 0;
            sparse_download_size = 0;			
			FBTINFO("Fastboot end\n");
			break;
		}
		/* TODO: It is a workaround for fastboot file download hang issue */
		udelay(1000); /* 1 msec of delay */
	}

	return ret;
}

int parse_fb_param(char *name)
{   
	unsigned char idx_max = 8;
	char *p;
	char ps[idx_max];
	char * endp;
	int i = 0;
	/*default EMMC1*/

	p = getenv(name);
	if (!p) {
		printf("Please set %s , 1:0:0:0 for NAND ,1:1:1:0 for EMMC\n", name);
	} else
		printf("wmt.fb.param  %s\n", p);

	while (i < idx_max) {
		ps[i++] = simple_strtoul(p, &endp, 16);
		if (*endp == '\0')
			break;
		p = endp + 1;
		if (*p == '\0')
			break;        
	}
	if (ps[0] == 1) {
		g_fb_param.enable= ps[0];
		g_fb_param.device= ps[1];
		g_fb_param.controller = ps[2];
		g_fb_param.part_type= ps[3];
		
	} else {
		g_fb_param.enable = 0;
		g_fb_param.device = 0;
		g_fb_param.controller = 0;
		g_fb_param.part_type= 0;
             return -1;
	}
	FBTINFO("g_fb_param.enable:%d, g_fb_param.device:%d, g_fb_param.controller :%d\n",g_fb_param.enable, g_fb_param.device, g_fb_param.controller);
	return 0;
}

U_BOOT_CMD(fastboot, 2,	1, do_fastboot,
	"fastboot- use USB Fastboot protocol\n", NULL);
