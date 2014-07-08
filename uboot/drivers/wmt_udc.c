/*++
Copyright (c) 2010 WonderMedia Technologies, Inc.

This program is free software: you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free Software
Foundation, either version 2 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details. You
should have received a copy of the GNU General Public License along with this
program. If not, see http://www.gnu.org/licenses/>.

WonderMedia Technologies, Inc.
10F, 529, Chung-Cheng Road, Hsin-Tien, Taipei 231, R.O.C.
--*/


#include "common.h"
#include <malloc.h>

#ifndef _CONFIG_H_
#include "config.h"
#endif

#ifndef _CHIPTOP_H_
#include "../board/wmt/include/chiptop.h"
#endif

#ifndef _USBREG_H_
#include "wmt_udcreg.h"
#endif
#include <fastboot.h>

#ifndef size_t
#define size_t int

#endif
#define writel(val, addr)  (*(volatile unsigned long *)(addr) = (val))
#define writew(val, addr)  (*(volatile unsigned short *)(addr) = (val))
#define writeb(val, addr)  (*(volatile unsigned char *)(addr) = (val))
#define readl(addr)   (*(volatile unsigned long *)(addr))
#define readw(addr)   (*(volatile unsigned short *)(addr))
#define readb(addr)   (*(volatile unsigned char *)(addr))

/*---------------------------------------------------------------------*/
unsigned int chip_axid;

extern int bench_status;
#define BENCH_NO_OPERATION  0
#define BENCH_RUNING        1
#define BENCH_FAIL         -1

#define BENCH_RESULT_INIT   0
#define BENCH_RESULT_OK     1
#define BENCH_RESULT_FAIL   2

#define CHIP_A3   0x00000032	/* Chip ID just use Bit 0 ~ 7 */
#define CHIP_A4   0x00000042	/* Chip ID just use Bit 0 ~ 7 */
#define CHIP_A5   0x00000052	/* Chip ID just use Bit 0 ~ 7 */

//#define udc_debug  1
#ifdef udc_debug
#define Nprintf	printf
#else
#define Nprintf(...)
#endif

#ifndef NULL
#define NULL	0
#endif

//typedef unsigned int dma_addr_t ;
/*---------------------------------------------------------------------*/
enum WMT_UDC_STATE {
	WMT_UDC_DISCONNECTD = 0,
	WMT_UDC_CONNECTD    = 1,
	WMT_UDC_CONFIGURED  = 2,
};

int g_udc_connected = 0;

unsigned char ControlState; /* the state the control pipe*/


#if defined(CONFIG_WMT_UDC) && defined(CONFIG_USB_DEVICE)
unsigned char *bulk_out_dma1;
unsigned char *bulk_out_dma2;
unsigned char *bulk_in_dma1;
unsigned char *bulk_in_dma2;
unsigned char *int_in_dma1;
int udc_config = 0;
int udc_download_flag = 0;
int udc_cmd_flag = 0;
unsigned int udc_download_last_bytes = 0;
unsigned int d_bytes=0,d_bytes_total=0;
unsigned int UBE_MAX_DMA = 0x80000 ; /*512K*/
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define MAX(a,b) ((a) > (b) ? (a) : (b))
static struct urb *ep0_urb = NULL;
#define UDCDBG(str) Nprintf("[%s] %s:%d: " str "\n", __FILE__,__FUNCTION__,__LINE__)


static struct usb_device_instance *udc_device;	/* Used in interrupt handler */

volatile unsigned int Global_Port_0_Reset_Count;
volatile unsigned int Global_Port_0_Suspend_Count;
volatile unsigned int Global_Port_0_Resume_Count;

volatile unsigned int Host_TD_Complete_Count;
volatile unsigned int Host_SOFInt_Count;

volatile unsigned int Device_SOF_Num;
volatile unsigned int Device_Babble_Count;

volatile unsigned int G_Rx0E;
volatile unsigned int H_Rx07;
volatile unsigned int D_Rx02;

 static unsigned int UdcPdmaAddrLI, UdcPdmaAddrSI;
 static unsigned int UdcPdmaAddrLO, UdcPdmaAddrSO;
unsigned char *pUSBMiscControlRegister5;

#ifdef OTGIP
static struct USB_GLOBAL_REG *pGlobalReg;
#endif

static struct UDC_REGISTER *pDevReg;
static struct UDC_DMA_REG *pUdcDmaReg;

PSETUPCOMMAND pSetupCommand;
UCHAR *pSetupCommandBuf;
UCHAR *SetupBuf;
UCHAR *IntBuf;
/*HW pointer*/

unsigned char *MemoryBuffer;
unsigned char *pDescBase;
unsigned char *pCurDesc;
unsigned char *pNextDesc;
unsigned char *pHDataBase; // The host data area.
unsigned char *pCurHData;
unsigned char *pDDataBase; // the device data area.
unsigned char *pCurDData;
unsigned char *pDEp1Base; // the data area for device endpoint 1 DMA
unsigned char *pDEp2Base; // the data area for device endpoint 2 DMA
unsigned char *pDEp3Base; // the data area for device endpoint 3 DMA
unsigned char *pDEp4Base; // the data area for device endpoint 4 DMA
unsigned char *MemBufStr;
unsigned char *DescStr;
unsigned char *DbgHdr;

int parking_high_speed;

#define FD_INC          0x01
#define FD_CONST        0x00
#define FD_DEC          -1

#define USB_ENDPOINT_NUMBER_MASK	0x0f	/* in bEndpointAddress */
#define USB_ENDPOINT_DIR_MASK		0x80

#define USB_ENDPOINT_XFERTYPE_MASK	0x03	/* in bmAttributes */
#define USB_ENDPOINT_XFER_CONTROL	0
#define USB_ENDPOINT_XFER_ISOC		1
#define USB_ENDPOINT_XFER_BULK		2
#define USB_ENDPOINT_XFER_INT		3
#define USB_REQ_GET_STATUS		0x00
#define USB_REQ_CLEAR_FEATURE		0x01
#define USB_REQ_SET_FEATURE		0x03
#define USB_REQ_SET_ADDRESS		0x05
#define USB_REQ_GET_DESCRIPTOR		0x06
#define USB_REQ_SET_DESCRIPTOR		0x07
#define USB_REQ_GET_CONFIGURATION	0x08
#define USB_REQ_SET_CONFIGURATION	0x09
#define USB_REQ_GET_INTERFACE		0x0A
#define USB_REQ_SET_INTERFACE		0x0B
#define USB_REQ_SYNCH_FRAME		0x0C
#define GET_MAX_LUN 0xFE
#define USB_BULK_RESET_REQUEST		0xff
#define USB_ENDPOINT_HALT		0	/* IN/OUT will STALL */

#define USB_RECIP_OTHER			0x03
#define GLOBAL_P0_STS_CHG_INT       0x00000020
#define GLOBAL_P1_STS_CHG_INT       0x00000040
#define USB_RECIP_DEVICE		0x00
#define UDC_IS_IOC_INT                   0x00000001
#define UDC_IS_BABBLE_INT                0x00000002
#define UDC_IS_SETUP_INT                 0x00000004

#define UDC_EP0_DIR                      0x00800000
#define UDC_EPX_PKT_DONE                 0x01000000
#define UDC_EPX_PKT_ERR                  0x02000000
#define UDC_EPX_PKT_SHORT                0x04000000
#define UDC_EPX_PKT_BABBLE               0x08000000

#define USB_RECIP_MASK			0x1f
#define USB_RECIP_DEVICE		0x00
#define USB_RECIP_INTERFACE		0x01
#define USB_RECIP_ENDPOINT		0x02
#define USB_RECIP_OTHER			0x03


#define UDC_EP0_ACT                      0x00000001
#define UDC_EP0_SYNC                     0x00000002
#define UDC_EP0_STALL                    0x00000004
#define UDC_EP0_DMABUSY                  0x00000008
#define UDC_EP0_IOCEN                    0x00000010
#define UDC_EP0_BABBLE_INTEN             0x00000020
#define UDC_EP0_TRAN_TYPE_MASK           0x000000C0

#define UDC_ADC_BOTH_ADDR                0x00000080
#define UDC_ADC_IGNORE_RESET             0x00000100


#define GLOBAL_REG_FLOAT           0x00000001
#define GLOBAL_REG_LOWDEVCON       0x00000002
#define GLOBAL_REG_FULLDEVCON      0x00000004
#define GLOBAL_REG_HIGHDEVCON      0x00000008
#define GLOBAL_REG_DISCON          0x00000010
#define GLOBAL_REG_RESUME          0x00000020
#define GLOBAL_REG_SRPREQ          0x00000040
#define GLOBAL_REG_PORTFAIL        0x00000080
#define GLOBAL_REG_VBUSRISE        0x00000100
#define GLOBAL_REG_VBUSFALL        0x00000200

#define GLOBAL_REG_FULLHSTCON      0x00000400
#define GLOBAL_REG_HIGHHSTCON      0x00000800

#define GLOBAL_DETECT_RESET_INT    0x00001000
#define GLOBAL_DETECT_SUSPEND_INT  0x00002000
#define GLOBAL_DETECT_RESUME_INT   0x00004000
#define GLOBAL_DETECT_LINE_INT     0x00008000

#define GLOBAL_REG_DIFFLINE        0x00008000
#define GLOBAL_REG_READINTEVENT    0x00010000
#define GLOBAL_REG_READSIGNAL      0x00000000
#define GLOBAL_REG_INTERNALDRVVBUS 0x00020000
#define GLOBAL_REG_INTERNALVBUSVLD 0x00040000
#define GLOBAL_REG_INTERNALSESSVLD 0x00080000
#define GLOBAL_REG_DISCHRGVBUS     0x00100000
#define GLOBAL_REG_CHRGVBUS        0x00200000
#define GLOBAL_REG_LINESTATE_MASK  0x30000000
#define GLOBAL_REG_CURIDPIN        0x40000000
#define GLOBAL_REG_IDPINVLD        0x80000000


#define UDC_DC_DEV_RUN                   0x00000001
#define UDC_DC_DEV_RST                   0x00000002
#define UDC_DC_SELF_POWER                0x00000004
#define UDC_DC_NO_SUSPEND                0x00000010
#define UDC_DC_PERIODIC_PING_ACK         0x00000020

#define UDC_IE_IOC_INT_EN                0x00000001
#define UDC_IE_BABBLE_INT_EN             0x00000002
#define UDC_IE_SETUP_INT_EN              0x00000004


enum usb_device_speed {
	USB_SPEED_UNKNOWN = 0,			/* enumerating */
	USB_SPEED_LOW, USB_SPEED_FULL,		/* usb 1.1 */
	USB_SPEED_HIGH				/* usb 2.0 */
};


#define GLOBAL_FLOAT_INT_EN        0x00010000
#define GLOBAL_LOWDEVCON_INT_EN    0x00020000
#define GLOBAL_FULLDEVCON_INT_EN   0x00040000
#define GLOBAL_HIGHDEVCON_INT_EN   0x00080000
#define GLOBAL_DISCON_INT_EN       0x00100000
#define GLOBAL_RESUME_INT_EN       0x00200000
#define GLOBAL_SRPREQ_INT_EN       0x00400000
#define GLOBAL_PORTFAIL_INT_EN     0x00800000
#define GLOBAL_VBUSRISE_INT_EN     0x01000000
#define GLOBAL_VBUSFALL_INT_EN     0x02000000
#define GLOBAL_FULLHSTCON_INT_EN   0x04000000
#define GLOBAL_HIGHHSTCON_INT_EN   0x08000000


#define GLOBAL_DETECT_RESET_INT_EN    0x10000000
#define GLOBAL_DETECT_SUSPEND_INT_EN  0x20000000
#define GLOBAL_DETECT_RESUME_INT_EN   0x40000000
#define GLOBAL_DETECT_LINE_STATE_EN   0x80000000


#define USB_DT_DEVICE			0x01
#define USB_DT_CONFIG			0x02
#define USB_DT_STRING			0x03
#define USB_DT_INTERFACE		0x04
#define USB_DT_ENDPOINT			0x05
#define USB_DT_DEVICE_QUALIFIER		0x06
#define USB_DT_OTHER_SPEED_CONFIG	0x07
#define USB_DT_INTERFACE_POWER		0x08


#define STRING_LANGUAGE     0
#define STRING_MANUFACTURER	1
#define STRING_PRODUCT		2
#define STRING_SERIAL	    3
#define STRING_CONFIG       4
#define STRING_INTERFACE    5
#define STRING_EE    0xEE //for ubuntu
//Rx20 ~ RX24 : EndPoint X IRP Handler Transfer Descriptor
#define UDC_TD_EPX_DT                    0x00000001
#define UDC_TD_EP0_DIR                   0x00000002
#define UDC_TD_EPX_DONE                  0x00000004
#define UDC_TD_EPX_IOCEN                 0x00000008
#define UDC_TD_EPX_RUN                   0x00000010

struct wmt_usb_device_descriptor {
	volatile u8  bLength;
	volatile u8  bDescriptorType;

	volatile u16 bcdUSB;
	volatile u8  bDeviceClass;
	volatile u8  bDeviceSubClass;
	volatile u8  bDeviceProtocol;
	volatile u8  bMaxPacketSize0;
	volatile u16 idVendor;
	volatile u16 idProduct;
	volatile u16 bcdDevice;
	volatile u8  iManufacturer;
	volatile u8  iProduct;
	volatile u8  iSerialNumber;
	volatile u8  bNumConfigurations;
};

#define USB_DT_DEVICE_SIZE		18

#define USB_CLASS_VENDOR_SPEC		0xff
#define REG_VAL(addr) ( *((volatile unsigned int *)(addr)) )
/*-------------------------------------------------------------------------*/

/* USB_DT_CONFIG: Configuration descriptor information.
 *
 * USB_DT_OTHER_SPEED_CONFIG is the same descriptor, except that the
 * descriptor type is different.  Highspeed-capable devices can look
 * different depending on what speed they're currently running.  Only
 * devices with a USB_DT_DEVICE_QUALIFIER have any OTHER_SPEED_CONFIG
 * descriptors.
 */

struct usb_config_descriptor {
	volatile u8  bLength;
	volatile u8  bDescriptorType;
	volatile u16 wTotalLength;
	volatile u8  bNumInterfaces;
	volatile u8  bConfigurationValue;
	volatile u8  iConfiguration;
	volatile u8  bmAttributes;
	volatile u8  bMaxPower;
};

#define USB_DT_CONFIG_SIZE		9

/* from config descriptor bmAttributes */
#define USB_CONFIG_ATT_ONE		(1 << 7)	/* must be set */
#define USB_CONFIG_ATT_SELFPOWER	(1 << 6)	/* self powered */
#define USB_CONFIG_ATT_WAKEUP		(1 << 5)	/* can wakeup */

/*-------------------------------------------------------------------------*/

/* USB_DT_STRING: String descriptor */
struct vt3400_usb_string_descriptor {
	volatile u8  bLength;
	volatile u8  bDescriptorType;
	volatile u16 wData[1];		/* UTF-16LE encoded */
};

/* note that "string" zero is special, it holds language codes that
 * the device supports, not Unicode characters.
 */

/*-------------------------------------------------------------------------*/

/* USB_DT_INTERFACE: Interface descriptor */
struct vt3400_usb_interface_descriptor {
	volatile u8  bLength;
	volatile u8  bDescriptorType;
	volatile u8  bInterfaceNumber;
	volatile u8  bAlternateSetting;
	volatile u8  bNumEndpoints;
	volatile u8  bInterfaceClass;
	volatile u8  bInterfaceSubClass;
	volatile u8  bInterfaceProtocol;
	volatile u8  iInterface;
};

#define USB_DT_INTERFACE_SIZE		9

/*-------------------------------------------------------------------------*/

/* USB_DT_ENDPOINT: Endpoint descriptor */
struct vt3400_usb_endpoint_descriptor {
	volatile u8  bLength;
	volatile u8  bDescriptorType;
	volatile u8  bEndpointAddress;
	volatile u8  bmAttributes;
	volatile u16 wMaxPacketSize;
	volatile u8  bInterval;
};


#define USB_DT_ENDPOINT_SIZE		7
#define SCSI_dirmware_upgrade

#ifdef SCSI_dirmware_upgrade
struct download_command_queue {
	unsigned int	next;	/* 0x0409 for en-us */
	unsigned short wValue;
	unsigned short wIndex;
};
#else
struct download_command_queue {
	unsigned int	*next;	/* 0x0409 for en-us */
	unsigned short wValue;
	unsigned short wIndex;
};
#endif

struct FWtag{
	unsigned int KernelVersion;
	unsigned int kernelStartAddr;
	unsigned int KernelLength;
	unsigned int FSVersion;
	unsigned int FSStartAddr;
	unsigned int FSLength;
	unsigned int CustomerID;
};

struct command_queue{
	unsigned short wValue;
	unsigned short wIndex;
};

struct MP_command_queue{
	unsigned int addr;
	unsigned int length;
};


#define USB_DIR_OUT			0		/* to device */
#define USB_DIR_IN			0x80		/* to host */
#define CONFIG_VALUE		1
#define VT3357 1

#if VT3357

static struct wmt_usb_device_descriptor
device_desc = {
	.bLength         =	0x12,
	.bDescriptorType =	USB_DT_DEVICE, //0x01
	.bcdUSB          =  0x0200,
	.bDeviceClass    =  0,//USB_CLASS_VENDOR_SPEC,
	.bDeviceSubClass = 0,// 0xff,
	.bDeviceProtocol =  0,//0xff,
	.bMaxPacketSize0 =  0x40,
    .idVendor        =  CONFIG_USBD_VENDORID,//,0x040d,
	.idProduct       = CONFIG_USBD_PRODUCTID,//0x8453,// 0x8506,
	.bcdDevice       =  0x0005,
	.iManufacturer   =	STRING_MANUFACTURER,
	.iProduct        =	STRING_PRODUCT,
	.iSerialNumber   =	STRING_SERIAL,
	.bNumConfigurations =	1,
};

static struct usb_config_descriptor
config_desc = {
	.bLength =		USB_DT_CONFIG_SIZE,
	.bDescriptorType =	USB_DT_CONFIG,
	.wTotalLength  = 9,
	.bNumInterfaces =	1,
	.bConfigurationValue =	CONFIG_VALUE,
	.iConfiguration      =  STRING_CONFIG,
	.bmAttributes =		USB_CONFIG_ATT_ONE,	//bit 7(must be set) + bit 6(self powered) + bit 5(can wakeup) : GigaHsu : same as VT3214
	.bMaxPower    =		0x02,	// 50 * 2 = 100mA ; GigaHsu : same as VT3214
};


static struct vt3400_usb_interface_descriptor
intf_desc = {
	.bLength =		USB_DT_INTERFACE_SIZE,
	.bDescriptorType =	USB_DT_INTERFACE,
    .bInterfaceNumber = 0,
	.bAlternateSetting = 0,
	.bNumEndpoints =	2,	   // Adjusted during fsg_bind()
	.bInterfaceClass =	  0x08,	//0xff,
	.bInterfaceSubClass =0x06,//	 0x02,//(SFF-8020i, MMC-2, ATAPI (CD-ROM)) GigaHsu : same as VT3214//0xff,
	.bInterfaceProtocol =	0x50, // Adjusted during fsg_bind() //Default : 0x50(Bulk-only)//0xff,
	.iInterface         = STRING_INTERFACE, // GigaHsu 2007.6.5 : Add for MSC Compliance Test.
};

static struct vt3400_usb_endpoint_descriptor
fs_bulk_in_desc = {
	.bLength =		USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,
	//.bEndpointAddress =	(USB_DIR_IN | 2),   //Charles
	.bEndpointAddress =	(USB_DIR_IN | 1),
	.bmAttributes =		USB_ENDPOINT_XFER_BULK,
#ifdef FULL_SPEED_ONLY
    .wMaxPacketSize = 64,
#else
	.wMaxPacketSize = 512,
#endif
	.bInterval = 0x00,
};

static struct vt3400_usb_endpoint_descriptor
fs_bulk_out_desc = {
	.bLength =		USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,
	.bEndpointAddress =	(USB_DIR_OUT | 2),  //Charles
//	.bEndpointAddress =	(USB_DIR_IN | 3),
	.bmAttributes =		USB_ENDPOINT_XFER_BULK,
#ifdef FULL_SPEED_ONLY
    .wMaxPacketSize = 64,
#else
	.wMaxPacketSize = 512,
#endif
	.bInterval = 0x00,
};

static struct vt3400_usb_endpoint_descriptor
fs_interrupt_desc = {
	.bLength =		USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,
	.bEndpointAddress =	(USB_DIR_IN | 5),
//	.bEndpointAddress =	(USB_DIR_IN | 3),
	.bmAttributes =		USB_ENDPOINT_XFER_BULK,
#ifdef FULL_SPEED_ONLY
    .wMaxPacketSize = 64,
#else
	.wMaxPacketSize = 512,
#endif
	.bInterval = 0x01,//0x00,
};

#endif

//string descriptor
#define LANGID_US_H			0x04
#define LANGID_US_L			0x09

static const u8 string_desc_0[] = { // codes representing languages
	(0x02 + 2), 0x03,
	LANGID_US_L, LANGID_US_H
};

static const u8 string_desc_1[] = { // STRING_MANUFACTURER
	(0x3A + 2),0x03,
	'W',0x0,'o',0x0,'n',0x0,'d',0x0,'e',0x0,'r',0x0,'M',0x0,
	'e',0x0,'d',0x0,'i',0x0,'a',0x0,' ',0x0,'T',0x0,'e',0x0,'c',0x0,
	'h',0x0,'n',0x0,'o',0x0,'l',0x0,'o',0x0,'g',0x0,'i',0x0,
	'e',0x0,'s',0x0,' ',0x0,'I',0x0,'n',0x0,'c',0x0,'.',0x0
};

static const u8 string_desc_2[] = { // STRING_PRODUCT
	(0x22 + 2),0x03,
	'W',0x0,'M',0x0,'8',0x0,'9',0x0,'5',0x0,'0',0x0,' ',0x0,
	'U',0x0,'S',0x0,'B',0x0,' ',0x0,'D',0x0,'e',0x0,'v',0x0,
	'i',0x0,'c',0x0,'e',0x0
};

static const u8 string_desc_3[] = { // STRING_SERIAL
	(0x18 + 2),0x03,
	'0',0x0,'0',0x0,'0',0x0,'0',0x0,'0',0x0,'0',0x0,'0',0x0,
	'0',0x0,'0',0x0,'0',0x0,'0',0x0,'1',0x0
};
//static const u8 generated_string_desc_3[] = { // STRING_SERIAL
//	(0x18 + 2),0x03,
//	'0',0x0,'0',0x0,'0',0x0,'0',0x0,'0',0x0,'0',0x0,'0',0x0,
//	'0',0x0,'0',0x0,'0',0x0,'0',0x0,'1',0x0
//};
static const u8 string_desc_4[] = { //  STRING_CONFIG
	(0x2E + 2),0x03,
	'F',0x0,'a',0x0,'s',0x0,'t',0x0,'b',0x0,'o',0x0,'o',0x0,
	't',0x0,' ',0x0,'P',0x0,'r',0x0,'o',0x0,'t',0x0,'o',0x0,
	'c',0x0,'o',0x0,'l',0x0,' ',0x0,' ',0x0,' ',0x0,' ',0x0,
	' ',0x0,' ',0x0
};
static const u8 string_desc_5[] = { // STRING_INTERFACE
	(0x26 + 2),0x03,
	'F',0x0,'a',0x0,'s',0x0,'t',0x0,'b',0x0,'o',0x0,'o',0x0,
	't',0x0,' ',0x0,'I',0x0,'n',0x0,'t',0x0,'e',0x0,
	'r',0x0,'f',0x0,'a',0x0,'c',0x0,'e',0x0,' ',0x0
};
static const u8 string_desc_6[] = { // STRING_EE  
	(0x26 + 2),0x03, 
	'F',0x0,'a',0x0,'s',0x0,'t',0x0,'b',0x0,'o',0x0,'o',0x0,
	't',0x0,' ',0x0,'I',0x0,'n',0x0,'t',0x0,'e',0x0,
	'r',0x0,'f',0x0,'a',0x0,'c',0x0,'e',0x0,' ',0x0
};

#define RESPOK		0
#define RESPFAIL	1

unsigned int EP0_DMA;
#define RUN 1

unsigned int EP0_0_byte_DIR;
#define out		0
#define in		1

unsigned int EP0_INT=0;
unsigned int DMA_INT=0;
unsigned int 	SET_ADDR=0;


#define UDC_REG_BASE_ADDR    0xD80E6700
//Just for ep2~ep3 (bulk in & out endpoints)
#define UDC_EPX_PAYLOAD_ADR_REG(num)  UDC_REG(0x24 + (num-2) * 4) //Rx09 => offset 0x24(36)
//for ep0~ep4
#define UDC_EPX_CSR_REG(num)	      UDC_REG(0x40 + (num * 4))
//for ep0~ep4
#define UDC_EPX_IRP_DESC_REG(num)     UDC_REG(0x80 + (num * 4))

#define UDC_EP0_ACT                      0x00000001
#define UDC_EP0_BABBLE_INTEN             0x00000020

struct usb_ctrlrequest {
	unsigned char bRequestType;
	unsigned char bRequest;
	unsigned short wValue;
	unsigned short wIndex;
	unsigned short wLength;
} ;

struct wmt_ep {
	unsigned long		irqs;
	const struct vt3400_usb_endpoint_descriptor *desc;
	char			    name[14];
	u16				    maxpacket;
	u8			     	bEndpointAddress;
	u8			     	bmAttributes;
	unsigned			double_buf:1;
	unsigned			stopped:1;
	unsigned			fnf:1;
	unsigned			has_dma:1;
	unsigned            toggle_bit:1;
	unsigned            stall:1;
	unsigned            rndis:1;
	u8				    ackwait;
	u32				    dma_counter;
	struct wmt_udc	*udc;
	volatile u32		*reg_payload_addr;
	volatile u32		*reg_control_status;
	volatile u32		*reg_irp_descriptor;
	volatile u32        temp_payload_addr;
	volatile u32        temp_control_status;
	volatile u32        temp_buffer_address;
	volatile u32        temp_dma_phy_address;
	volatile u32        temp_buffer_address2;
	volatile u32        temp_dma_phy_address2;
	volatile u32        temp_req_length;
	volatile u32        temp_irp_descriptor;
	volatile u32        rndis_buffer_address;
	volatile u32        rndis_dma_phy_address;
};

struct wmt_udc {
	u16	                      devstat;
	u16            ep0_in_status;
	volatile u32   cbw_virtual_address;
	unsigned			   softconnect:1;
	unsigned			   vbus_active:1;
	unsigned			   ep0_pending:1;
	unsigned			   ep0_in:1;
	unsigned			   ep0_set_config:1;
	unsigned			   ep0_reset_config:1;
	unsigned			   ep0_setup:1;
	unsigned               ep0_status_0_byte:1;
	unsigned               usb_connect:1;
	unsigned               file_storage_set_halt:1;
	unsigned               bulk_out_dma_write_error:1;
	unsigned  int          chip_version;
	u8               setup_command[8];
};

unsigned int DEV_DMA_BASE = 0x00400;    //mapping to host address [31:12] = start addrrss = start at 4M with  host memory address
unsigned int DEV_OFFA_EP1	= 0xf;
#define DEV_OFFA_EP1_ 0xf
#define EP0_MEM_BASE_ADDR 	0x00400000
#define EP1_MEM_BASE_ADDR		EP0_MEM_BASE_ADDR | (DEV_OFFA_EP1_<<4)
#define EP2_DMA_BASE_ADDR		EP0_MEM_BASE_ADDR|0x00002000
#define EP3_DMA_BASE_ADDR		EP0_MEM_BASE_ADDR|0x00004000
//UCHAR *EP0_BUF;

UCHAR *EP0_BUF =(UCHAR *)(EP0_MEM_BASE_ADDR);
UCHAR *EP1_BUF =(UCHAR *)(EP1_MEM_BASE_ADDR);
UCHAR *EP2_Bulk_IN_BUF =(UCHAR *)(EP2_DMA_BASE_ADDR);
UCHAR *EP3_Bulk_OUT_BUF =(UCHAR *)(EP3_DMA_BASE_ADDR);

UCHAR control_buf[256];
unsigned int start_vendor_command=0,func_sel_length=0,bulk_round = 0,SF_start_addr = 0xff000000,sector_size = 0x10000;
unsigned int fw_upgrade=0,MP_execute=0,exec_at = 0,tag_addr = 0xfffc0000,recive_setup = 0,csw_status=0,cbw_tag,recive_CBW = 0;
unsigned int direction = 0,reconnect = 0,reconnect_status = 0,to_get_reconnect=0,bulk_in_stall=0,round = 1,data_transfer_complete=0;
 unsigned int SF_stop,N_dma_complete=0;
int MP_round = -1;
#define	DEV_GLOBAL_SETUP_INT_EN	0x00000004
#define	DEV_GLOBAL_BAB_INT_EN	0x00000002
#define	DEV_GLOBAL_IOC_INT_EN	0x00000001
#define	DEV_GLOBAL_ALL_INT_EN	DEV_GLOBAL_SETUP_INT_EN | DEV_GLOBAL_BAB_INT_EN | DEV_GLOBAL_IOC_INT_EN

#define INQUIRY 0x12
#define REQUEST_SENSE 0x03
#define VENDOR_COMMAND 0xf0
#define READ_FORMAT_CAPACITY 0x23

#define FIRMWARE_DOWNLOAD_COMMAND			0x80
#define DEVICE_RECONNECT_COMMAND			0x81
#define DOWNLOAD_COMPLETE_COMMAND			0x82
#define CONFIG_DOWNLOAD_COMMAND				0x83
#define GET_FW_VERSION_COMMAND				0x84
#define FIRMWARE_UPLOAD_COMMAND				0x86
#define ERASE_TAG_COMMAND							0x87
#define MP_DOWNLOAD_COMMAND						0x88
#define EXECUTE_MP_COMMAND						0x89
#define GET_PROGRESS								0x8a


//#define USB_TEST_SUCCESS        			0
#define USB_TEST_CONTROL_PIPE_FAIL		1
#define USB_TEST_BULK_PIPE_FAIL				2
#define USB_INCOMPLETE								3
#define BULK_COMPLETE										4
#define BULK_go_on										5
#define BULK_IN_STALL						6
#define EXECUTE_MP						6

struct usb_request {
	void			*buf;
	unsigned		length;
	dma_addr_t		dma;
	unsigned		no_interrupt:1;
	unsigned		zero:1;
	unsigned		short_not_ok:1;
	void			*context;
	int			status;
	unsigned		actual;
};


struct wmt_req {
	struct usb_request	 req;
	unsigned			 dma_bytes;
	unsigned			 mapped:1;
};



static struct wmt_udc  udc;
static struct wmt_ep   ep[5];

unsigned int progress_length,get_progress = 0;
unsigned int version_length;
unsigned char Inquiry_data[]={
	0x00,0x80,0x02,0x02,0x1f,0x00,0x00,0x00,
	0x4b,0x4f,0x49,0x54,0x45,0x43,0x48,0x20,
	0x52,0x4f,0x56,0x49,0x4f,0x20,0x20,0x20,
	0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20
};

unsigned char sense_key_data[]={
	0x70,0x00,0x02,0x00,0x00,0x00,0x00,0x0a,
	0x00,0x00,0x00,0x00,0x3a,0x00,0x00,0x00,
	0x00,0x00
};



#define cbw
	#ifdef cbw
	#define cbwprintf	printf
	#else
	#define cbwprintf(...)
	#endif

UCHAR toggle;

void FillData(UCHAR *Buf, int Size, int InitValue, int Method)
/*
  Fill a buffer with some values
  Buf: the target buffer to store initialized data
  Size: the length of data to fill into the buffer
  InitValue: the initial value to fill into the buffer
  Method: instructment of how to fill the data
*/
{
 int i;
 for (i = 0; i<Size; i++)
     {
     Buf[i] = (InitValue&0xFF);
     InitValue += Method;
     }
}
#if 0
int CheckSetup(UINT SetupLo, UINT SetupHi)
/*
  Check for the received setup command
  SetupLo: the lower DWORD of setup command
  SetupHi: the higher DWORD of setup command
 */
{
 if (DReg->CurSetupCmd0 != SetupLo)
     return FALSE;
 if (DReg->CurSetupCmd1 != SetupHi)
     return FALSE;

 return TRUE;

}

int CheckMem(UCHAR *Mem0, UCHAR *Mem1, int Size)
/*
  Check whether two memory is identical or not
  Mem0: the first bulk of memory
  Mem1: the second bulk of memory
  Size: the length of data to be compared
 */
 {
  int i;

  for (i =0; i< Size; i++)
      if (Mem0[i]!= Mem1[i])
         {
          Nprintf(" offset %x data1 %x data2 %x\n", i, Mem0[i], Mem1[i]);
          return FALSE;
           }

  return TRUE;
 }
#endif
//-----------------------------------------------------------------------------------------------------

static struct usb_endpoint_instance *wmt_find_ep (int ep)
{
	int i;

	//for (i = 0; i < udc_device->bus->max_endpoints; i++) {
		for (i = 0; i < 3; i++) {
		Nprintf("udc_device->bus->endpoint_array[%d].endpoint_address = %8.8x\n",i,udc_device->bus->endpoint_array[i].endpoint_address);
		if (udc_device->bus->endpoint_array[i].endpoint_address == ep) {
			Nprintf("found udc_device->bus->endpoint_array[%d].endpoint_address = %8.8x,ep=%x\n",i,udc_device->bus->endpoint_array[i].endpoint_address,ep);
			return &udc_device->bus->endpoint_array[i];
		}
	}
	return NULL;
}
static void wmt_udc_pdma_des_prepare(unsigned int size, unsigned char *dma_phy, unsigned char des_type, unsigned char channel);

#ifdef CONFIG_USB_TTY
void BULK_EndPoint_EN(unsigned short EP2_max,unsigned short EP3_max,unsigned short EP1_max)
{
		u32 dcmd = 0,dma_ccr = 0;
		dcmd = 1;
		ep[2].temp_buffer_address = bulk_out_dma1;//((req->req.dma + req->req.actual) & 0xFFFFFFFC);
		wmt_udc_pdma_des_prepare(dcmd, bulk_out_dma1, DESCRIPTOT_TYPE_LONG, TRANS_OUT);

		pDevReg->Bulk2DesStatus = 0x00;
		pDevReg->Bulk2DesTbytes2 |= (dcmd >> 16) & 0x3;
		pDevReg->Bulk2DesTbytes1 = (dcmd >> 8) & 0xFF;
		pDevReg->Bulk2DesTbytes0 = dcmd & 0xFF;

		/* set endpoint data toggle*/
		if (ep[2].toggle_bit)
			pDevReg->Bulk2DesTbytes2 |= BULKXFER_DATA1;
		else
			pDevReg->Bulk2DesTbytes2 &= 0x3F;/*BULKXFER_DATA0;*/

		pDevReg->Bulk2DesStatus = BULKXFER_IOC;/*| BULKXFER_IN;*/
		dma_ccr = 0;
		dma_ccr = DMA_RUN;
		dma_ccr |= DMA_TRANS_OUT_DIR;

		pUdcDmaReg->DMA_Context_Control1 = dma_ccr;

		pDevReg->Bulk2DesStatus |= BULKXFER_ACTIVE;

/*bulk in*/
			pDevReg->Bulk1DesStatus = 0x00;
			pDevReg->Bulk1DesTbytes2 |= (0 >> 16) & 0x3;
			pDevReg->Bulk1DesTbytes1 = (0 >> 8) & 0xFF;
			pDevReg->Bulk1DesTbytes0 = 0 & 0xFF;

			/* set endpoint data toggle*/
			if (ep[1].toggle_bit)
				pDevReg->Bulk1DesTbytes2 |= 0x40;/* BULKXFER_DATA1;*/
			else
				pDevReg->Bulk1DesTbytes2 &= 0xBF;/*(~BULKXFER_DATA1);*/

			pDevReg->Bulk1DesStatus = (BULKXFER_IOC | BULKXFER_IN);
				if (ep[1].toggle_bit == 0)
					ep[1].toggle_bit = 1;
				else
					ep[1].toggle_bit = 0;
			pDevReg->Bulk1DesStatus |= BULKXFER_ACTIVE;

/*int in*/

			pDevReg->InterruptDes = 0x00;

			if (ep[3].toggle_bit) {
				ep[3].toggle_bit = 0;
				pDevReg->InterruptDes = (2 << 4);
				*(volatile unsigned short*)(USB_UDC_REG_BASE + 0x40) = 0x0005;
				pDevReg->InterruptDes |= INTXFER_DATA1;
			} else {
				ep[3].toggle_bit = 1;
				pDevReg->InterruptDes = (8 << 4);
				*(volatile unsigned int*)(USB_UDC_REG_BASE + 0x40) = 0x000020a1;
				*(volatile unsigned int*)(USB_UDC_REG_BASE + 0x44) = 0x00020000;
				pDevReg->InterruptDes &= 0xF7;
			}

			pDevReg->InterruptDes |= INTXFER_IOC;
			pDevReg->InterruptDes |= INTXFER_ACTIVE;
}
#else
void BULK_EndPoint_EN(struct usb_endpoint_instance *endpoint)
{
		u32 dcmd = 0,dma_ccr = 0;
		u8 *buf = 0;
		struct urb *urb = endpoint->rcv_urb;
		udc_cmd_flag = 1;
		dcmd = 0x100;
		buf = urb->buffer;
		endpoint->rcv_transferSize = 0 ;
		Nprintf("%s,dcmd:0x%x,urb->buffer:0x%x\n",__FUNCTION__,dcmd,buf);
		wmt_udc_pdma_des_prepare(dcmd, buf, DESCRIPTOT_TYPE_LONG, TRANS_OUT);
		/*bulk out */
		pDevReg->Bulk2DesStatus = 0x00;
		pDevReg->Bulk2DesTbytes2 |= (dcmd >> 16) & 0x3;
		pDevReg->Bulk2DesTbytes1 = (dcmd >> 8) & 0xFF;
		pDevReg->Bulk2DesTbytes0 = dcmd & 0xFF;

		/* set endpoint data toggle*/
		if (ep[2].toggle_bit) {
			pDevReg->Bulk2DesTbytes2 |= BULKXFER_DATA1;
			ep[2].toggle_bit = 0;
		}
		else {
			pDevReg->Bulk2DesTbytes2 &= 0x3F;/*BULKXFER_DATA0;*/
			ep[2].toggle_bit = 1;
		}

		pDevReg->Bulk2DesStatus = BULKXFER_IOC;/*| BULKXFER_IN;*/
		dma_ccr = 0;
		dma_ccr = DMA_RUN;
		dma_ccr |= DMA_TRANS_OUT_DIR;

		pUdcDmaReg->DMA_Context_Control1 = dma_ccr;

		pDevReg->Bulk2DesStatus |= BULKXFER_ACTIVE;
}
#endif
/*CharlesTu,2012.10.26,improved through put of memcpy function from 100 ms to 6 ms*/
void *memcpy_itp(void *d, const void *s, size_t n)

{

        asm volatile (
        ".Lv5:                       \n\t"
        "ldmia %1!, {r0-r7}  	\n\t"
        "stmia %0!, {r0-r7}  	\n\t"
        "subs %2, %2, #32   	\n\t"
        "beq .Lv         		\n\t"
        "cmp %2, #32		\n\t"
        "bge .Lv5         		\n\t"
        ".Lv4:				\n\t"
        "ldr  r0,[%1], #4  	\n\t"
        "str  r0,[%0], #4  	\n\t"
        "subs %2, %2, #4   	\n\t"
        "beq .Lv         		\n\t"
        "cmp %2, #4		\n\t"
        "bge .Lv4         		\n\t"
    	".Lv3:				\n\t"
        "ldrh r0,[%1]  	\n\t"
        "add  %1, #2  	\n\t"
        "strh r0,[%0]  	\n\t"
        "add %0,#2  	\n\t"
        "subs %2, %2, #2  	\n\t"
        "beq .Lv         		\n\t"
    	".Lv2:				\n\t"
        "ldrb r0,[%1]  	\n\t"
        "strb r0,[%0]  	\n\t"
        "subs %2, %2, #1  	\n\t"
        "beq .Lv         		\n\t"

        ".Lv:				\n\t"
        :
        : "r" (d), "r" (s), "r" (n)
        : "r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7"
        );

        return (void *) d;

}

/* Before this controller can enumerate, we need to pick an endpoint
 * configuration, or "fifo_mode"  That involves allocating 2KB of packet
 * buffer space among the endpoints we'll be operating.
 */

void wmt_ep_setup(char *name, u8 addr, u8 type,
		unsigned maxp)
{
	Nprintf("wmt_ep_setup()\n");

	if (type == USB_ENDPOINT_XFER_CONTROL) {
		/*pDevReg->ControlEpControl;*/
		pDevReg->ControlEpReserved = 0;
		pDevReg->ControlEpMaxLen = (maxp & 0xFF);
		pDevReg->ControlEpEpNum = 0;
	} else if (type == USB_ENDPOINT_XFER_BULK) {
		if (addr & USB_DIR_IN) {
			/*pDevReg->Bulk1EpControl;*/
			pDevReg->Bulk1EpOutEpNum = (addr & 0x7f) << 4;
			pDevReg->Bulk1EpMaxLen = (maxp & 0xFF);
			pDevReg->Bulk1EpInEpNum = (((addr & 0x7f) << 4) | ((maxp & 0x700) >> 8));
		} else {
			/*pDevReg->Bulk2EpControl;*/
			pDevReg->Bulk2EpOutEpNum = (addr & 0x7f) << 4;
			pDevReg->Bulk2EpMaxLen = (maxp & 0xFF);
			pDevReg->Bulk2EpInEpNum = (((addr & 0x7f) << 4) | ((maxp & 0x700) >> 8));
		}
	} else if (type == USB_ENDPOINT_XFER_INT) {
		/*pDevReg->InterruptEpControl; // Interrupt endpoint control           - 2C*/
		pDevReg->InterruptReserved = (addr & 0x7f) << 4;/*Interrupt endpoint reserved byte  - 2D*/
		pDevReg->InterruptEpMaxLen = (maxp & 0xFF); /* Interrupt maximum transfer length    - 2E*/
		pDevReg->InterruptEpEpNum = (((addr & 0x7f) << 4) | ((maxp & 0x700) >> 8));
	}
	Nprintf("wmt_ep_setup() - %s addr %02x  maxp %d\n",
		name, addr, maxp);

	//	strlcpy(ep->name, name, sizeof ep->name);
	ep[addr & 0x7f].bEndpointAddress = addr;
	ep[addr & 0x7f].bmAttributes = type;
	ep[addr & 0x7f].double_buf = 0;/*dbuf;*/
	ep[addr & 0x7f].toggle_bit = 0;
	ep[addr & 0x7f].maxpacket = maxp;

	switch((ep[addr & 0x7f].bEndpointAddress & 0x7F))
   	 {
	  case 0://Control
			pDevReg->ControlDesStatus = CTRLXFER_ACTIVE;
	  break;

	  case 1://Bulk
			pDevReg->Bulk1DesStatus |= BULKXFER_ACTIVE;
	  break;

	  case 2://Bulk
			pDevReg->Bulk2DesStatus |= BULKXFER_ACTIVE;
	  break;

	  case 3://Bulk
			pDevReg->InterruptDes |= INTXFER_ACTIVE;
	  break;
	}
}//wmt_ep_setup()

int wmt_udc_setup(void)
{
 	 Nprintf("wmt_udc_setup\n");

	/* ep0 is special; put it right after the SETUP buffer*/
	wmt_ep_setup("ep0", 0, USB_ENDPOINT_XFER_CONTROL, 64);

#define WMT_BULK_EP(name, addr) \
	wmt_ep_setup(name "-bulk", addr, \
		USB_ENDPOINT_XFER_BULK, 512);

#define WMT_INT_EP(name, addr, maxp) \
	wmt_ep_setup(name "-int", addr, \
		USB_ENDPOINT_XFER_INT, maxp);

	WMT_BULK_EP("ep1in",  USB_DIR_IN  | 1);
	WMT_BULK_EP("ep2out", USB_DIR_OUT | 2);
	WMT_INT_EP("ep3in",  USB_DIR_IN  | 3, 8);

	/*INFO("fifo mode %d, %d bytes not used\n", fifo_mode, 2048 - buf);*/
	return 0;
}

static void wmt_ep_setup_csr(char *name, u8 addr, u8 type, unsigned maxp)
{
	struct wmt_ep	*ep=&ep[0];
	/* OUT endpoints first, then IN*/
	ep = &ep[addr & 0x7f];
	Nprintf("wmt_ep_setup_csr()\n");

	if (type == USB_ENDPOINT_XFER_CONTROL) {
		/*pDevReg->ControlEpControl;*/
		pDevReg->ControlEpReserved = 0;
		pDevReg->ControlEpMaxLen = (maxp & 0xFF);
		pDevReg->ControlEpEpNum = 0;
	} else if (type == USB_ENDPOINT_XFER_BULK) {
		if (addr & USB_DIR_IN) {
			/*pDevReg->Bulk1EpControl;*/
			pDevReg->Bulk1EpOutEpNum = (addr & 0x7f) << 4;
			pDevReg->Bulk1EpMaxLen = (maxp & 0xFF);
			pDevReg->Bulk1EpInEpNum = (((addr & 0x7f) << 4) | ((maxp & 0x700) >> 8));
		} else {
			/*pDevReg->Bulk2EpControl;*/
			pDevReg->Bulk2EpOutEpNum = (addr & 0x7f) << 4;
			pDevReg->Bulk2EpMaxLen = (maxp & 0xFF);
			pDevReg->Bulk2EpInEpNum = (((addr & 0x7f) << 4) | ((maxp & 0x700) >> 8));
		}
	} else if (type == USB_ENDPOINT_XFER_INT) {
		/*pDevReg->InterruptEpControl; // Interrupt endpoint control           - 2C*/
		/* Interrupt endpoint reserved byte     - 2D*/
		pDevReg->InterruptReserved = (addr & 0x7f) << 4;
		pDevReg->InterruptEpMaxLen = (maxp & 0xFF); /* Interrupt maximum transfer length    - 2E*/
		pDevReg->InterruptEpEpNum = (((addr & 0x7f) << 4) | ((maxp & 0x700) >> 8));
	}

	/*Setting address pointer ...*/

	Nprintf("wmt_ep_setup_csr() - %s addr %02x  maxp %d\n",
	name, addr, maxp);

	ep->toggle_bit = 0;
}


static void
wmt_udc_csr(void)
{
	wmt_ep_setup_csr("ep1in" "-bulk", (USB_DIR_IN | 1), USB_ENDPOINT_XFER_BULK, 512);
	wmt_ep_setup_csr("ep2out" "-bulk", (USB_DIR_OUT | 2), USB_ENDPOINT_XFER_BULK, 512);
}




static void devstate_irq(void)
{
	udc.usb_connect = 0;

	if (pDevReg->IntEnable & INTENABLE_DEVICERESET) {
		/*vt3357 hw issue : clear all device register.*/
		/*payload & address needs re-set again...*/
		pDevReg->IntEnable |= INTENABLE_SUSPENDDETECT;

		udc.ep0_set_config = 0;
		pDevReg->Bulk1DesStatus = 0;

		ep[0].toggle_bit = 0;
		ep[1].toggle_bit = 0;
		ep[2].toggle_bit = 0;
		ep[3].toggle_bit = 0;
		ep[4].toggle_bit = 0;

		udc.file_storage_set_halt = 0;
		udc.bulk_out_dma_write_error = 0;


	} /*if (pDevReg->IntEnable & INTENABLE_DEVICERESET)*/

	if (pDevReg->IntEnable & INTENABLE_SUSPENDDETECT) {
		pDevReg->IntEnable |= INTENABLE_SUSPENDDETECT;
	} /*if (pDevReg->IntEnable & INTENABLE_SUSPENDDETECT)*/

	if (pDevReg->IntEnable & INTENABLE_RESUMEDETECT) {
		pDevReg->IntEnable |= INTENABLE_RESUMEDETECT;
	} /*if (pDevReg->IntEnable & INTENABLE_RESUMEDETECT)*/

	/* USB Bus Connection Change*/
	/* clear connection change event*/
	if (pDevReg->PortControl & PORTCTRL_SELFPOWER)/* Device port control register         - 22)*/
		if (pDevReg->PortControl & PORTCTRL_CONNECTCHANGE)/* Device port control register   - 22)*/
			pDevReg->PortControl |= PORTCTRL_CONNECTCHANGE;/*   0x02 // connection change bit*/


	if (pDevReg->PortControl & PORTCTRL_FULLSPEEDMODE) {
		//udc.gadget.speed = USB_SPEED_FULL;
		udc.usb_connect = 1;
		/*2007-8.27 GigaHsu : enable float, reset, suspend and resume IE*/
		/*after host controller connected.*/
	} /*if(pDevReg->PortControl & PORTCTRL_FULLSPEEDMODE)*/

	if (pDevReg->PortControl & PORTCTRL_HIGHSPEEDMODE) {
		//udc.gadget.speed = USB_SPEED_HIGH;
		udc.usb_connect = 1;
		/*2007-8.27 GigaHsu : enable float, reset, suspend and resume IE*/
		/*after host controller connected.*/
	} /*if(pDevReg->PortControl & PORTCTRL_HIGHSPEEDMODE)*/

}

unsigned int get = 0;
#define USBDHyperTerminal

//static const u8 string_desc_3[] = { // STRING_SERIAL
//	(0x18 + 2),0x03,
//	'0',0x0,'0',0x0,'0',0x0,'0',0x0,'0',0x0,'0',0x0,'0',0x0,
//	'0',0x0,'0',0x0,'0',0x0,'0',0x0,'1',0x0
//};

//a simple / not thread safe rng
static u32 m_w = 0;    /* must not be zero */
static u32 m_z = 0;    /* must not be zero */

u32 get_random()
{
    if(m_w == 0){
       m_w = get_timer(0);//current time stamp
	}
	if(m_z == 0){
       m_z = get_timer(0);//current time stamp
	}

    m_z = 36969 * (m_z & 65535) + (m_z >> 16);
    m_w = 18000 * (m_w & 65535) + (m_w >> 16);
    return (m_z << 16) + m_w;  /* 32-bit result */
}

static int generated = 0;
static int ran_num = 0;
static void generate_random_string_serial(char * buf, int bufLen){

	if(generated == 0){
		int base = 10000000;
		int bit;
		int pos = 2;
        int ran = 0;
		if(ran_num == 0)
			ran_num = get_random() % 100000000;

        ran = ran_num;

		while(base > 0 && pos < bufLen){
	       bit = ran / base;
		   //bit is 0-9
		   buf[pos] = '0'+bit;
	       ran %= base;
		   base /= 10;
		   pos += 2;
		}
        printf("generated random string serial\n");
		generated = 1;
     }
}


void restore_original_string_serial(void){
     int pos = 2;  //escape first two bytes
	 int len = sizeof(string_desc_3);
	 __u8 * buf = (__u8*)string_desc_3;
	 while(pos < len){
	 	 if(pos != len - 2)
            buf[pos] = '0';
		 else
		 	buf[pos] = '1';
		 pos += 2;
	 }
	 generated = 0;
	 printf("restored to original serial\n");
}


static void printf_string_serial(u8 * buf, int len){
	 int pos = 2, i = 0;
	 char * str = malloc(len);
	 if(!str){
        printf("malloc fail in printf_string_serial\n");
		return;
	 }
     memset(str, 0, len);

	 while(pos < len){
	 	str[i] = buf[pos];
		pos += 2;
		i++;
	 }

	 printf("string serial : %s\n", str);

	 free(str);
}



extern struct cmd_fastboot_interface priv;

/*UDC_IS_SETUP_INT*/
extern void usbd_device_event_irq (struct usb_device_instance *conf, usb_device_event_t, int);
static void udc_control_prepare_data_resp(void)
{
	struct wmt_ep	*ep0 = &ep[0];
	UCHAR Result = RESPFAIL;
	unsigned char CurXferLength = 0;
	int value = 0, i, test = 0;
	Nprintf("%s\n",__FUNCTION__);


	/*ep0->irqs++;*/
	Nprintf("ep0_irq()\n");
	ep0->toggle_bit = 1;


	/* SETUP starts all control transfers*/
	{
		union u {
			u8			bytes[8];
			struct usb_ctrlrequest	r;
		} u;
		struct wmt_ep *ep = &ep[0];
		int    ep0_status_phase_0_byte = 0;

		/* read the (latest) SETUP message*/
		for (i = 0; i <= 7; i++)
			u.bytes[i] = pSetupCommandBuf[i];

		/* Delegate almost all control requests to the gadget driver,*/
		/* except for a handful of ch9 status/feature requests that*/
		/* hardware doesn't autodecode _and_ the gadget API hides.*/
		/**/
		udc.ep0_in = (u.r.bRequestType & USB_DIR_IN) != 0;
		udc.ep0_set_config = 0;
		udc.ep0_pending = 1;
		udc.ep0_in_status = 0;

		ep0->stopped = 0;
		ep0->ackwait = 0;

		if ((u.r.bRequestType & USB_RECIP_OTHER) == USB_RECIP_OTHER) {/*USB_RECIP_OTHER(0x03)*/
			//status = 0;
			goto delegate;
		}

		switch (u.r.bRequest) {
		case 0x20:
			  ep0_status_phase_0_byte = 0;
				value=7;
			  //status = 0;
		break;
		case 0x21:
		    value = u.r.wLength;

        EP0_BUF[0] =0x80;
        EP0_BUF[1] =0x25;
        EP0_BUF[2] =0;
        EP0_BUF[3] =0;
        EP0_BUF[4] =0;
        EP0_BUF[5] =0;
        EP0_BUF[6] =8;

			  udc.ep0_set_config = 1;
                    CurXferLength = value;
                    Result = RESPOK;
		break;

		case 0x22:
			value=0;

		break;
		case USB_REQ_SET_CONFIGURATION:
			/* udc needs to know when ep != 0 is valid*/
			/*SETUP 00.09 v0001 i0000 l0000*/
			if (u.r.bRequestType != USB_RECIP_DEVICE)/*USB_RECIP_DEVICE(0x00)*/
				goto delegate;
			if (u.r.wLength != 0)
				goto do_stall;
			udc.ep0_set_config = 1;
			udc.ep0_reset_config = (u.r.wValue == 0);

			/* update udc NOW since gadget driver may start*/
			/* queueing requests immediately; clear config*/
			/* later if it fails the request.*/
			/**/

			ep[0].toggle_bit = 0;
			ep[1].toggle_bit = 0;
			ep[2].toggle_bit = 0;
			ep[3].toggle_bit = 0;
			ep[4].toggle_bit = 0;
			value=0;
			test = 1;		//Charles, for window
			goto delegate;


		 case GET_MAX_LUN:
		  	value = u.r.wLength;

		       if(value !=  1)
		          value = 1;

                    	EP0_BUF[0] =0;

			start_vendor_command = 1;
			udc.ep0_set_config = 1;
                    CurXferLength = value;
                    Result = RESPOK;
			 	break;


		case USB_REQ_GET_DESCRIPTOR:

		    switch (u.r.wValue >> 8)
		    {
		     case USB_DT_DEVICE:
			       value = u.r.wLength;
				if(value > sizeof (struct wmt_usb_device_descriptor))
					value = 18;

				memcpy((unsigned char*)control_buf, &device_desc, value);
				CurXferLength = value;

				for(i = 0; i < value; i++) {
					EP0_BUF[i] =control_buf[i];
				}

				CurXferLength = value;
				Result = RESPOK;


			 break;


			 case USB_DT_CONFIG:
			       value = u.r.wLength;

#if VT3357
			       if(value > sizeof (struct usb_config_descriptor))
			       {
		          value = config_desc.wTotalLength ;


			          memcpy((unsigned char*)control_buf, 0, value);
				      	memcpy((unsigned char*)EP0_BUF, 0, value);
			          memcpy( (unsigned char*)control_buf, &config_desc, value);
			          memcpy(((unsigned char*)control_buf + USB_DT_CONFIG_SIZE), &intf_desc, USB_DT_INTERFACE_SIZE);

				        memcpy(((unsigned char*)control_buf + USB_DT_CONFIG_SIZE + USB_DT_INTERFACE_SIZE), &fs_bulk_out_desc, USB_DT_ENDPOINT_SIZE);
				        memcpy(((unsigned char*)control_buf + USB_DT_CONFIG_SIZE + USB_DT_INTERFACE_SIZE + USB_DT_ENDPOINT_SIZE), &fs_bulk_in_desc, USB_DT_ENDPOINT_SIZE);
				        memcpy(((unsigned char*)control_buf + USB_DT_CONFIG_SIZE + USB_DT_INTERFACE_SIZE + USB_DT_ENDPOINT_SIZE*2), &fs_interrupt_desc, USB_DT_ENDPOINT_SIZE);



			       }
			       else
			         memcpy((unsigned char*)control_buf, &config_desc, value);

#else
				if(value > sizeof (struct usb_config_descriptor))
			       {
			          value = config_desc.wTotalLength  = USB_DT_CONFIG_SIZE +
			                                              USB_DT_INTERFACE_SIZE +
			                                              USB_DT_ENDPOINT_SIZE * 3;	//Two Bulk and on Interrupt

			          memcpy( (unsigned char*)control_buf, &config_desc, value);
			          memcpy(((unsigned char*)control_buf + USB_DT_CONFIG_SIZE), &intf_desc, USB_DT_INTERFACE_SIZE);


					memcpy(((unsigned char*)control_buf + USB_DT_CONFIG_SIZE + USB_DT_INTERFACE_SIZE
			                  ), &fs_int_in_desc, USB_DT_ENDPOINT_SIZE);
			          memcpy(((unsigned char*)control_buf + USB_DT_CONFIG_SIZE + USB_DT_INTERFACE_SIZE+ USB_DT_ENDPOINT_SIZE),
			                 &fs_bulk_in_desc, USB_DT_ENDPOINT_SIZE);
			          memcpy(((unsigned char*)control_buf + USB_DT_CONFIG_SIZE + USB_DT_INTERFACE_SIZE
			                  + USB_DT_ENDPOINT_SIZE*2), &fs_bulk_out_desc, USB_DT_ENDPOINT_SIZE);


			       }
			       else
			         memcpy((unsigned char*)control_buf, &config_desc, value);
#endif


				for(i=0; i<value; i++)
					EP0_BUF[i] =control_buf[i];

				CurXferLength = value;
				Result = RESPOK;

			 break;

		     case USB_DT_STRING:
			      value = u.r.wLength;

			      switch ((u.r.wValue & 0xff))
			      {
			       case STRING_LANGUAGE:

					 if(value > sizeof(string_desc_0))
					value = sizeof(string_desc_0);

				        memcpy((unsigned char*)control_buf, (u8 *)string_desc_0, sizeof(string_desc_0));

				   break;

			       case STRING_MANUFACTURER:
					 if(value > sizeof(string_desc_1))
					value = sizeof(string_desc_1);

				          memcpy((unsigned char*)control_buf, (u8 *)string_desc_1,sizeof(string_desc_1));

				   break;

			       case STRING_PRODUCT:
					 if(value > sizeof(string_desc_2))
					value = sizeof(string_desc_2);

				          memcpy((unsigned char*)control_buf, (u8 *)string_desc_2,sizeof(string_desc_2));
				   break;

				   case STRING_SERIAL:
				   	    //adb driver use 0x40 buffer
				   	    Nprintf("request string serial wLength=0x%x\n", value);
                        if(priv.configured == 1){
							generate_random_string_serial((u8 *)string_desc_3, sizeof(string_desc_3));

						}

						if(value > sizeof(string_desc_3))
						    value = sizeof(string_desc_3);

						memcpy((unsigned char*)control_buf, (u8 *)string_desc_3,sizeof(string_desc_3));

						printf_string_serial((u8*)string_desc_3, sizeof(string_desc_3));

					//if (set_config)
				     //    test = 1;	//for getvar
				   break;

				   case  STRING_CONFIG:
				   	 if(value > sizeof(string_desc_4))
					value = sizeof(string_desc_4);
				          memcpy((unsigned char*)control_buf, (u8 *)string_desc_4,sizeof(string_desc_4));
				        // test = 1;	//Charles
				   break;

				   case STRING_INTERFACE:
				   	 if(value > sizeof(string_desc_5))
					value = sizeof(string_desc_5);
				          memcpy((unsigned char*)control_buf, (u8 *)string_desc_5,sizeof(string_desc_5)); 
				          
				   break;
				   
				   case STRING_EE:
				   	Nprintf("STRING_EE\n");
				   	 if(value > sizeof(string_desc_6))
						value = sizeof(string_desc_6);
				          memcpy((unsigned char*)control_buf, (u8 *)string_desc_6,sizeof(string_desc_6)); 
				          
				   break;

				   default:
					Nprintf("STRING_default\n");
				   	 if(value > sizeof(string_desc_6))
						value = sizeof(string_desc_6);
				          memcpy((unsigned char*)control_buf, (u8 *)string_desc_6,sizeof(string_desc_6)); 

				   break; 						
			      }

		          for(i=0; i<value; i++)
                      	EP0_BUF[i] =control_buf[i];

		          CurXferLength = value;
				  Result = RESPOK;
			 break;
		    }//switch (u.r.wValue >> 8)

			ep0_status_phase_0_byte = 1;

		break;


		/* Giga Hsu : 2007.6.6 This would cause set interface status 0
			return to fast and cause bulk endpoint in out error*/
		case USB_REQ_SET_INTERFACE:

	//		status = 0;

			ep[0].toggle_bit = 0;
			ep[1].toggle_bit = 0;
			ep[2].toggle_bit = 0;
			ep[3].toggle_bit = 0;
			ep[4].toggle_bit = 0;

			goto delegate;
		    /*break;*/

		case USB_REQ_CLEAR_FEATURE:
#if 0
			/* clear endpoint halt*/
			if (u.r.bRequestType != USB_RECIP_ENDPOINT)
				goto delegate;

			if (u.r.wValue != USB_ENDPOINT_HALT
			|| u.r.wLength != 0)
				goto do_stall;

			ep = &ep[u.r.wIndex & 0xf];

			if (ep != ep0) {
				if (ep->bmAttributes == USB_ENDPOINT_XFER_ISOC || !ep->desc)
					goto do_stall;

				if (udc.file_storage_set_halt == 0) {
					switch ((ep->bEndpointAddress & 0x7F)) {
					case 0:/*Control In/Out*/
					   pDevReg->ControlEpControl &= 0xF7;
					break;

					case 1:/*Bulk In*/
						{
								u32 dma_ccr = 0;

						pDevReg->Bulk1EpControl &= 0xF7;

							wmt_pdma0_reset();
							wmt_udc_pdma_des_prepare(ep->temp_dcmd,
								ep->temp_bulk_dma_addr,
								DESCRIPTOT_TYPE_LONG, TRANS_IN);

							pDevReg->Bulk1DesStatus = 0x00;
							pDevReg->Bulk1DesTbytes2 |=
								(ep->temp_dcmd >> 16) & 0x3;
							pDevReg->Bulk1DesTbytes1 =
								(ep->temp_dcmd >> 8) & 0xFF;
							pDevReg->Bulk1DesTbytes0 =
								ep->temp_dcmd & 0xFF;

							/* set endpoint data toggle*/
							if (ep->ep_stall_toggle_bit) {
								/* BULKXFER_DATA1;*/
								pDevReg->Bulk1DesTbytes2 |= 0x40;
							} else {
								/*(~BULKXFER_DATA1);*/
								pDevReg->Bulk1DesTbytes2 &= 0xBF;
							}

							pDevReg->Bulk1DesStatus =
								(BULKXFER_IOC | BULKXFER_IN);

							/* DMA Channel Control Reg*/
							/* Software Request + Channel Enable*/
							dma_ccr = 0;
							dma_ccr = DMA_RUN;
							pUdcDmaReg->DMA_Context_Control0 = dma_ccr; //neil

							pDevReg->Bulk1DesStatus |= BULKXFER_ACTIVE;
					break;
				}
					case 2:/*Bulk Out*/
						pDevReg->Bulk2EpControl &= 0xF7;

						if (ep->stall_more_processing == 1) {
							u32 dma_ccr = 0;
							ep->stall_more_processing  = 0;
						//	wmt_pdma_reset();
							wmt_udc_pdma_des_prepare(ep->temp_dcmd,
								ep->temp_bulk_dma_addr,
							DESCRIPTOT_TYPE_LONG, TRANS_OUT);
							/* DMA Global Controller Reg*/
							/* DMA Controller Enable +*/
							/* DMA Global Interrupt Enable(if any TC, error,
								or abort status in any channels occurs)*/

							pDevReg->Bulk2DesStatus = 0x00;
							pDevReg->Bulk2DesTbytes2 |=
								(ep->temp_dcmd >> 16) & 0x3;
							pDevReg->Bulk2DesTbytes1 =
								(ep->temp_dcmd >> 8) & 0xFF;
							pDevReg->Bulk2DesTbytes0 =
								ep->temp_dcmd & 0xFF;

							if (ep->ep_stall_toggle_bit)
								pDevReg->Bulk2DesTbytes2 |= BULKXFER_DATA1;
							else
								pDevReg->Bulk2DesTbytes2 &= 0x3F;

							pDevReg->Bulk2DesStatus = BULKXFER_IOC;

							/* DMA Channel Control Reg*/
							/* Software Request + Channel Enable*/

							/*udc_bulk_dma_dump_register();*/
							dma_ccr = 0;
							dma_ccr = DMA_RUN;
							dma_ccr |= DMA_TRANS_OUT_DIR;
							pUdcDmaReg->DMA_Context_Control1 = dma_ccr;
							pDevReg->Bulk2DesStatus |= BULKXFER_ACTIVE;
						}
					break;

					case 3:/*Interrupt In*/
						pDevReg->InterruptEpControl &= 0xF7;
					break;
					}

					ep->stall = 0;
					ep->stopped = 0;
					/**ep->reg_irp_descriptor = ep->temp_irp_descriptor;*/
				}
			} /*if (ep != ep0)*/

			ep0_status_phase_0_byte = 1;
		//	VDBG("%s halt cleared by host\n", ep->name);
			/*goto ep0out_status_stage;*/
		//	status = 0;
			udc.ep0_pending = 0;
		/*break;*/
#endif
			goto delegate;


		case USB_REQ_SET_FEATURE:
			/* set endpoint halt*/

			if (u.r.bRequestType != USB_RECIP_ENDPOINT)
				goto delegate;

			if (u.r.wValue != USB_ENDPOINT_HALT || u.r.wLength != 0)
				goto do_stall;

			ep = &ep[u.r.wIndex & 0xf];

			if (ep->bmAttributes == USB_ENDPOINT_XFER_ISOC || ep == ep0 || !ep->desc)
				goto do_stall;

			switch ((ep->bEndpointAddress & 0x7F)) {
			case 0:/*Control In/Out*/
				pDevReg->ControlEpControl |= EP_STALL;
			break;

			case 1:/*Bulk In*/
				pDevReg->Bulk1EpControl |= EP_STALL;
			break;

			case 2:/*Bulk Out*/
				pDevReg->Bulk2EpControl |= EP_STALL;
			break;

			case 3:/*Interrupt In*/
				pDevReg->InterruptEpControl |= EP_STALL;
			break;
			}
			ep->stall = 1;
			ep->stopped = 1;

			ep0_status_phase_0_byte = 1;
			/*use_ep(ep, 0);*/
			/* can't halt if fifo isn't empty...*/
			/*UDC_CTRL_REG = UDC_CLR_EP;*/
			/*UDC_CTRL_REG = UDC_SET_HALT;*/
			/*ep0out_status_stage:*/

			udc.ep0_pending = 0;
			/*break;*/
			goto delegate;

		case USB_REQ_GET_STATUS:
			/* return interface status.  if we were pedantic,*/
			/* we'd detect non-existent interfaces, and stall.*/
			/**/
     			 value = u.r.wLength;
			if (u.r.bRequestType == (USB_DIR_IN|USB_RECIP_ENDPOINT)) {
				ep = &ep[u.r.wIndex & 0xf];

				if (ep->stall == 1) {
					udc.ep0_in_status = 0x01;

					/*GgiaHsu-B 2007.08.10 : patch HW Bug :
						MSC Compliance Test : Error Recovery Items.*/
					ep = &ep[3];
					if ((udc.file_storage_set_halt == 1) && (ep->stall == 1))
						ep->stall = 1;
					/*GgiaHsu-E 2007.08.10 :
						--------------------------------------------*/

					ep = &ep[0];
					ep->stall = 0;
				} else
					udc.ep0_in_status = 0x00;
			} else {
				udc.ep0_in_status = 0x00;
			}
                   	EP0_BUF[0] =0;
                   	EP0_BUF[1] =0;
			CurXferLength = value;
			Result = RESPOK;
			/* next, status stage*/
			//goto delegate;
			break;

		case USB_REQ_SET_ADDRESS:
			if (u.r.bRequestType == USB_RECIP_DEVICE) { /*USB_RECIP_DEVICE(0x00)*/
				pDevReg->DeviceAddr |= (DEVADDR_ADDRCHANGE | u.r.wValue);
				Nprintf("USB_REQ_SET_ADDRESS 0x%03d\n", u.r.wValue);
				ControlState = CONTROLSTATE_STATUS;
				ep0_status_phase_0_byte = 1;
			value =0;
			}
		break;

		case USB_BULK_RESET_REQUEST:

			Nprintf("USB_BULK_RESET_REQUEST\n");
			udc.file_storage_set_halt = 0;

			ep[0].toggle_bit = 0;
			ep[1].toggle_bit = 0;
			ep[2].toggle_bit = 0;
			ep[3].toggle_bit = 0;
			ep[4].toggle_bit = 0;


			goto delegate;
			/*break;*/

		default:
delegate:
			Nprintf("SETUP %02x,%02x,%04x,%04x,%04x\n",
			u.r.bRequestType, u.r.bRequest,
			u.r.wValue, u.r.wIndex, u.r.wLength);
			  /*
			// The gadget driver may return an error here,
			// causing an immediate protocol stall.
			//
			// Else it must issue a response, either queueing a
			// response buffer for the DATA stage, or halting ep0
			// (causing a protocol stall, not a real halt).  A
			// zero length buffer means no DATA stage.
			//
			// It's fine to issue that response after the setup()
			// call returns, and this IRQ was handled.
			//
			*/
			udc.ep0_setup = 1;
			/*usb gadget driver prepare setup data phase(control in)*/
			udc.ep0_setup = 0;
		} /*switch (u.r.bRequest)*/

//		if (status < 0) {
		if (0) {
do_stall:
			/* ep = &udc->ep[0];*/
			/**ep->reg_control_status |= UDC_EP0_STALL;*/
			/* fail in the command parsing*/
			pDevReg->ControlEpControl |= EP_STALL;  /* stall the pipe*/
			ControlState = CONTROLSTATE_SETUP;      /* go back to setup state*/

			ep->stall = 1;

			udc.ep0_pending = 0;
		} /*if (status < 0)*/

		if (u.r.bRequestType & 0x80) { //data phase In

			pDevReg->ControlDesTbytes = value | CTRLXFER_DATA1;
			pDevReg->ControlDesControl = CTRLXFER_IN + CTRLXFER_IOC;
			pDevReg->ControlDesStatus = CTRLXFER_ACTIVE;
			ControlState = CONTROLSTATE_DATA;
			while (1) {
				if (pDevReg->ControlEpControl & EP_COMPLETEINT) {
					pDevReg->ControlEpControl |= EP_COMPLETEINT;
					while (pDevReg->ControlEpControl & EP_COMPLETEINT) /* clear the event*/
						;
					break;
				}
			}

			pDevReg->ControlDesTbytes = CTRLXFER_DATA1;

			pDevReg->ControlDesControl = CTRLXFER_OUT + CTRLXFER_IOC;
			pDevReg->ControlDesStatus = CTRLXFER_ACTIVE;
			ControlState = CONTROLSTATE_DATA;

			while (1) {
				if (pDevReg->ControlEpControl & EP_COMPLETEINT) {
					pDevReg->ControlEpControl |= EP_COMPLETEINT;
					while (pDevReg->ControlEpControl & EP_COMPLETEINT) /* clear the event*/
						;
					break;
				}
			}

#ifdef USBDHyperTerminal
				//   printf("get = %d\n",get);
				if(u.r.bRequest == 0x21)
				{
					get++;
					if(get == 7)
					udc_config = 1;
				}
#endif

		} else if ((u.r.bRequestType == 0x21) && (u.r.bRequest == 0x20)) {
			pDevReg->ControlDesTbytes = value | CTRLXFER_DATA1;
			pDevReg->ControlDesControl = CTRLXFER_OUT + CTRLXFER_IOC;
			pDevReg->ControlDesStatus = CTRLXFER_ACTIVE;
			ControlState = CONTROLSTATE_DATA;

			while (1) {
				if (pDevReg->ControlEpControl & EP_COMPLETEINT) {
					pDevReg->ControlEpControl |= EP_COMPLETEINT;
					while (pDevReg->ControlEpControl & EP_COMPLETEINT) /* clear the event*/
						;
					break;
				}
			}

			pDevReg->ControlDesTbytes = CTRLXFER_DATA1;
			pDevReg->ControlDesControl = CTRLXFER_IN + CTRLXFER_IOC;
			pDevReg->ControlDesStatus = CTRLXFER_ACTIVE;
			ControlState = CONTROLSTATE_DATA;

			while (1) {
					if (pDevReg->ControlEpControl & EP_COMPLETEINT) {
						pDevReg->ControlEpControl |= EP_COMPLETEINT;
						while (pDevReg->ControlEpControl & EP_COMPLETEINT) /* clear the event*/
							;
						break;
					}
				}
			} else {
				pDevReg->ControlDesTbytes = value | CTRLXFER_DATA1;
				pDevReg->ControlDesControl = CTRLXFER_IN + CTRLXFER_IOC;
				pDevReg->ControlDesStatus = CTRLXFER_ACTIVE;
				ControlState = CONTROLSTATE_DATA;

				while (1) {
					if (pDevReg->ControlEpControl & EP_COMPLETEINT) {
						pDevReg->ControlEpControl |= EP_COMPLETEINT;
						while (pDevReg->ControlEpControl & EP_COMPLETEINT) /* clear the event*/
									;
						break;
					}
				}
			}
					/*Charles, for fastboot */
				if (test) {

				Nprintf("---------BULK_EndPoint_EN 1\n");
#ifdef CONFIG_USB_TTY
				BULK_EndPoint_EN(fs_bulk_in_desc.wMaxPacketSize,fs_bulk_out_desc.wMaxPacketSize,fs_interrupt_desc.wMaxPacketSize);
#else
				struct usb_endpoint_instance *endpoint = wmt_find_ep(2);
				BULK_EndPoint_EN(endpoint);
#endif
				test = 0;
				usbd_device_event_irq (udc_device, DEVICE_CONFIGURED,0);

#ifndef USBDHyperTerminal
			    udc_config = 1;
#endif
			}


	}
}


void USB_ControlXferComplete(void)
{
	struct wmt_ep *ept = &ep[0];

	ept = &ep[0];
	/* when ever a setup received, the Control state will reset*/
	/* check for the valid bit of the contol descriptor*/
	/*DBG("USB_ControlXferComplete()\n");*/
	if (pDevReg->ControlDesControl & CTRLXFER_CMDVALID) {
		ept->toggle_bit = 1;

			if (pDevReg->PortControl & PORTCTRL_HIGHSPEEDMODE) {
				wmt_udc_csr();
			}
		/* clear the command valid bit*/
		/* pDevReg->ControlDesControl |= CTRLXFER_CMDVALID;*/
		/* always clear control stall when SETUP received*/
		pDevReg->ControlEpControl &= 0x17; /*clear the stall*/
		ControlState = CONTROLSTATE_DATA;
		udc_control_prepare_data_resp();  /*processing SETUP command ...*/
		pDevReg->ControlDesControl &= 0xEF;/*(~CTRLXFER_CMDVALID);*/
		return;
	}

}



int check_udc_connection(){
#ifdef OTGIP
	u32 global_irq_src;
#endif

#ifdef OTGIP
	/*Global Interrupt Status*/
	global_irq_src = pGlobalReg->UHDC_Interrupt_Status;

	if (global_irq_src & UHDC_INT_UDC) {/*UDC Core + UDC DMA1 + UDC DMA2 */
		if (global_irq_src & UHDC_INT_UDC_CORE) /*UDC Core*/
#endif
    return (pDevReg->SelfPowerConnect & 0x01) && !(pDevReg->PortControl & 0x80);

#ifdef OTGIP
    return 0;
#endif
}

unsigned int irq_tmp=0,irq_zero=0;
int wmt_udc_irq(void)
{
  u32 udc_irq_src;
	int	status = 0;
#ifdef OTGIP
	u32 global_irq_src;
#endif

#ifdef OTGIP
	/*Global Interrupt Status*/
	global_irq_src = pGlobalReg->UHDC_Interrupt_Status;

	if (global_irq_src & UHDC_INT_UDC) {/*UDC Core + UDC DMA1 + UDC DMA2 */
		if (global_irq_src & UHDC_INT_UDC_CORE) {/*UDC Core*/
#endif

		//printf("pDevReg->SelfPowerConnect = 0x%x, pDevReg->PortControl = 0x%x, pDevReg->CommandStatus = 0x%x\n",
		//pDevReg->SelfPowerConnect, pDevReg->PortControl, pDevReg->CommandStatus);
		if((pDevReg->SelfPowerConnect & 0x01) && !(pDevReg->PortControl & 0x80)){
			g_udc_connected = 1;
		}else if(!(pDevReg->SelfPowerConnect & 0x01)){
            //unconnected / plug off?
            if(priv.configured)usbd_device_event_irq (udc_device, DEVICE_RESET, 0);
		}
			/*connection interrupt*/
		if (pDevReg->SelfPowerConnect & 0x02)
			pDevReg->SelfPowerConnect |= 0x02;

			/* Device Global Interrupt Pending Status*/
			udc_irq_src = pDevReg->CommandStatus;

			/* Device state change (usb ch9 stuff)*/
			if (udc_irq_src & USBREG_BUSACTINT) {/* the bus activity interrupt occured*/
				/*caused by Port 0 Statsu Change*/
				pDevReg->CommandStatus |= USBREG_BUSACTINT;
				devstate_irq();
			}

			if (udc_irq_src & USBREG_BABBLEINT) {/* the Babble interrupt ocuured*/
			/* check for Control endpoint for BABBLE error*/
				if (pDevReg->ControlEpControl & EP_BABBLE)
					pDevReg->ControlEpControl |= (EP_BABBLE + EP_STALL);

				if (pDevReg->Bulk1EpControl & EP_BABBLE)
					pDevReg->Bulk1EpControl |= (EP_BABBLE + EP_STALL);

				if (pDevReg->Bulk2EpControl & EP_BABBLE)
					pDevReg->Bulk2EpControl |= (EP_BABBLE + EP_STALL);
				printf("got BABBLEINT! udc_irq_src = 0x%x\n", udc_irq_src);
			}

			if (udc_irq_src & USBREG_COMPLETEINT) {/* the complete inerrupt occured*/
				if (pDevReg->ControlEpControl & EP_COMPLETEINT) {
					/* the control transfer complete event*/
					pDevReg->ControlEpControl |= EP_COMPLETEINT; /* clear the event*/
					USB_ControlXferComplete();
				}

				if (pDevReg->Bulk1EpControl & EP_COMPLETEINT) {
					/* the bulk transfer complete event*/
					pDevReg->Bulk1EpControl |= EP_COMPLETEINT;
					/*DBG("USB_Bulk 1 DMA()\n");*/
					dma_irq(0x81);
				}

				if (pDevReg->Bulk2EpControl & EP_COMPLETEINT) {
					/* the bulk transfer complete event*/
					pDevReg->Bulk2EpControl |= EP_COMPLETEINT;
					dma_irq(2);
				}

				if (pDevReg->InterruptEpControl & EP_COMPLETEINT) {
					/* the bulk transfer complete event*/
					pDevReg->InterruptEpControl |= EP_COMPLETEINT;
					/*DBG("USB_INT 3 DMA()\n");*/
					dma_irq(0x83);
				}
			}

#ifdef OTGIP
		}
		if (global_irq_src & UHDC_INT_UDC_DMA1)/*UDC Bulk DMA 1*/
			wmt_udc_dma_irq(UDC_IRQ_USB, udc, r);

		if (global_irq_src & UHDC_INT_UDC_DMA2)/*UDC Bulk DMA 1*/
			wmt_udc_dma_irq(UDC_IRQ_USB, udc, r);

	}
#endif

    if(!udc_config)
        status = 1;
    else
    	  status = 0;

	return status;
}
static void wmt_udc_pdma_des_prepare(unsigned int size, unsigned char *dma_phy
							, unsigned char des_type, unsigned char channel)
{
	unsigned int res_size = size;
	volatile unsigned int trans_dma_phy =(unsigned int)dma_phy;
	volatile unsigned int des_phy_addr = 0, des_vir_addr = 0;
	unsigned int cut_size =  0x40;
	Nprintf("%s\n",__FUNCTION__);

	switch (des_type) {
	case DESCRIPTOT_TYPE_SHORT:
	{
		struct _UDC_PDMA_DESC_S *pDMADescS;

		if (channel == TRANS_OUT) {
			des_phy_addr = UdcPdmaAddrSO;
			des_vir_addr = UdcPdmaAddrSO;
			pUdcDmaReg->DMA_Descriptor_Point1 = (unsigned int)(des_phy_addr);
			cut_size = pDevReg->Bulk2EpMaxLen & 0x3ff;
		}	else if (channel == TRANS_IN) {
			des_phy_addr = UdcPdmaAddrSI;
			des_vir_addr = UdcPdmaAddrSI;
			pUdcDmaReg->DMA_Descriptor_Point0 = (unsigned int)(des_phy_addr);
			cut_size = pDevReg->Bulk1EpMaxLen & 0x3ff;
		}	//else
			//DBG("!! wrong channel %d\n", channel);
		memset((void *)des_vir_addr, 0, 0x100);

		while (res_size) {
			pDMADescS = (struct _UDC_PDMA_DESC_S *) des_vir_addr;

			pDMADescS->Data_Addr = trans_dma_phy;
			pDMADescS->D_Word0_Bits.Format = 0;

			if (res_size <= 65535) {
				pDMADescS->D_Word0_Bits.i = 1;
				pDMADescS->D_Word0_Bits.End = 1;
				pDMADescS->D_Word0_Bits.ReqCount = res_size;

				res_size -= res_size;
			} else {
				pDMADescS->D_Word0_Bits.i = 0;
				pDMADescS->D_Word0_Bits.End = 0;
				pDMADescS->D_Word0_Bits.ReqCount = 0x10000 - cut_size;

				res_size -= 0x10000;
				des_vir_addr += (unsigned int)DES_S_SIZE;
				trans_dma_phy += (unsigned int)0x10000;
			}
		}
		break;
	}

	case DESCRIPTOT_TYPE_LONG:
	{
		struct _UDC_PDMA_DESC_L *pDMADescL;

		if (channel == TRANS_OUT) {
			des_phy_addr = UdcPdmaAddrLO;
			des_vir_addr = UdcPdmaAddrLO;
			pUdcDmaReg->DMA_Descriptor_Point1 = (unsigned int)(des_phy_addr);
			cut_size = pDevReg->Bulk2EpMaxLen & 0x3ff;//0x200
			memset((void *)des_vir_addr, 0, USB_DESCRIPT_LO_SIZE);
		}	else if (channel == TRANS_IN) {
			des_phy_addr = UdcPdmaAddrLI;
			des_vir_addr = UdcPdmaAddrLI;
			pUdcDmaReg->DMA_Descriptor_Point0 = (unsigned int)(des_phy_addr);
			cut_size = pDevReg->Bulk1EpMaxLen & 0x3ff;
			memset((void *)des_vir_addr, 0, 0x100);
		}
			//DBG("!! wrong channel %d\n", channel);
		while (res_size) {
			pDMADescL = (struct _UDC_PDMA_DESC_L *) des_vir_addr;

			pDMADescL->Data_Addr = trans_dma_phy;//dcmd
			pDMADescL->D_Word0_Bits.Format = 1;

			//if (res_size <= 65535) {
			if (res_size <= 32767) {
				pDMADescL->D_Word0_Bits.i = 1;
				pDMADescL->D_Word0_Bits.End = 1;
				pDMADescL->Branch_addr = 0;
				pDMADescL->D_Word0_Bits.ReqCount = res_size;

				res_size -= res_size;
			} else {
				pDMADescL->D_Word0_Bits.i = 0;
				pDMADescL->D_Word0_Bits.End = 0;
				pDMADescL->Branch_addr = des_vir_addr + (unsigned int)DES_L_SIZE;
				pDMADescL->D_Word0_Bits.ReqCount = 0x8000 - cut_size;

				res_size -= 0x8000;
				//res_size = res_size -  0x10000 + cut_size;
				des_vir_addr += (unsigned int)DES_L_SIZE;
				trans_dma_phy += (unsigned int) 0x8000;
			}
		}
		break;
	}

	case DESCRIPTOT_TYPE_MIX:
	break;
	default:
	break;
	}
}
#ifdef CONFIG_USB_TTY


static int next_out_dma_tty(struct usb_endpoint_instance *endpoint)
{
	/*unsigned packets;*/
	u32 dma_ccr = 0;
	u32	dcmd;
	u32	buf;
	struct urb *urb = endpoint->rcv_urb;
	int len = 0;
	len = 1;

	if ((endpoint->endpoint_address & 0x7F)  == 2) {/*Bulk Out*/

		unsigned char *cp = urb->buffer + urb->actual_length;
		/*fill dma & urb buffer*/
		len = 1;

		if(ep[2].temp_buffer_address == bulk_out_dma1){
			*(cp + len - 1) = bulk_out_dma1[0];
			Nprintf("[next_out_dma_test] bulk_out_dma1[0] = 0x%2.2x cp = 0x%2.2x\n",bulk_out_dma1[0],*(cp + len - 1));
			ep[2].temp_buffer_address = bulk_out_dma2;//((req->req.dma + req->req.actual) & 0xFFFFFFFC);
			buf = bulk_out_dma2;
		}else{
			*(cp + len - 1) = bulk_out_dma2[0];
			Nprintf("[next_out_dma_test] bulk_out_dma2[0] = 0x%2.2x cp = 0x%2.2x\n",bulk_out_dma2[0],*(cp + len - 1));
			ep[2].temp_buffer_address = bulk_out_dma1;//((req->req.dma + req->req.actual) & 0xFFFFFFFC);
			buf = bulk_out_dma1;
		}
		dcmd = 1;
		/* Set Address*/
		wmt_udc_pdma_des_prepare(dcmd, buf, DESCRIPTOT_TYPE_LONG, TRANS_OUT);
		/* DMA Global Controller Reg*/
		/* DMA Controller Enable +*/
		/* DMA Global Interrupt Enable(if any TC, error, or abort status in any channels occurs)*/
		pDevReg->Bulk2DesStatus = 0x00;
		pDevReg->Bulk2DesTbytes2 |= (dcmd >> 16) & 0x3;
		pDevReg->Bulk2DesTbytes1 = (dcmd >> 8) & 0xFF;
		pDevReg->Bulk2DesTbytes0 = dcmd & 0xFF;

		/* set endpoint data toggle*/
		if (ep->toggle_bit) {
			pDevReg->Bulk2DesTbytes2 |= BULKXFER_DATA1;
			ep->toggle_bit = 0;
		} else {
			pDevReg->Bulk2DesTbytes2 &= 0x3F;/*BULKXFER_DATA0;*/
			ep->toggle_bit = 1;
		}

		pDevReg->Bulk2DesStatus = BULKXFER_IOC;/*| BULKXFER_IN;*/

		dma_ccr = 0;
		dma_ccr = DMA_RUN;
		dma_ccr |= DMA_TRANS_OUT_DIR;

		pUdcDmaReg->DMA_Context_Control1 = dma_ccr;
		pDevReg->Bulk2DesStatus |= BULKXFER_ACTIVE;

	  return len;
	}

}

static void next_in_dma(struct usb_endpoint_instance *endpoint)
{
	u32 temp32;
	u32 dma_ccr = 0;
	unsigned int length;
	u32	dcmd;
    struct urb *urb = endpoint->tx_urb;

	if (urb) {
			unsigned int last = 0;
		if (((endpoint->endpoint_address & 0x7F) ==  1)) {/*Bulk In*/
			Nprintf("2 bulk in\n");
			if ((last = MIN (urb->actual_length - endpoint->sent,endpoint->tx_packetSize))) {
				u8 *cp = urb->buffer + endpoint->sent;

				Nprintf ("endpoint->sent %d, tx_packetSize %d, last %d\n", endpoint->sent, endpoint->tx_packetSize, last);

				endpoint->last = last;
				length = last;
				Nprintf("cp = 0x%8.8x\n",cp);

				while(last) {
					bulk_in_dma1[length-last] = cp[length-last];
					last--;
				}
			}
			wmt_udc_pdma_des_prepare(length, bulk_in_dma1, DESCRIPTOT_TYPE_LONG, TRANS_IN);

			pDevReg->Bulk1DesStatus = 0x00;
			pDevReg->Bulk1DesTbytes2 |= (length >> 16) & 0x3;
			pDevReg->Bulk1DesTbytes1 = (length >> 8) & 0xFF;
			pDevReg->Bulk1DesTbytes0 = length & 0xFF;

			/* set endpoint data toggle*/
			if (ep[1].toggle_bit)
				pDevReg->Bulk1DesTbytes2 |= 0x40;/* BULKXFER_DATA1;*/
			else
				pDevReg->Bulk1DesTbytes2 &= 0xBF;/*(~BULKXFER_DATA1);*/

			pDevReg->Bulk1DesStatus = (BULKXFER_IOC | BULKXFER_IN);

			if (length > ep[1].maxpacket) {
				/*ex : 512 /64 = 8  8 % 2 = 0*/
				temp32 = (length + ep[1].maxpacket - 1) / ep[1].maxpacket;
				ep[1].toggle_bit = ((temp32 + ep[1].toggle_bit) % 2);
			} else {
				if (ep[1].toggle_bit == 0)
					ep[1].toggle_bit = 1;
				else
					ep[1].toggle_bit = 0;
			}

			/* DMA Channel Control Reg*/
			/* Software Request + Channel Enable*/
			dma_ccr = 0;
			dma_ccr = DMA_RUN;
			if (dcmd) /* PDMA can not support 0 byte transfer*/
				pUdcDmaReg->DMA_Context_Control0 = dma_ccr;

			pDevReg->Bulk1DesStatus |= BULKXFER_ACTIVE;

		} else if ((endpoint->endpoint_address & 0x7F) == 3) {/*Interrupt In*/
			//*(volatile unsigned int *)(0xD81100C0) |= 0x100;
			if (ep[3].toggle_bit) {
					ep[3].toggle_bit = 0;
					pDevReg->InterruptDes = (2 << 4);
					*(volatile unsigned short*)(USB_UDC_REG_BASE + 0x40) = 0x0005;
					pDevReg->InterruptDes |= INTXFER_DATA1;
				} else {
					ep[3].toggle_bit = 1;
					pDevReg->InterruptDes = (8 << 4);
					*(volatile unsigned int*)(USB_UDC_REG_BASE + 0x40) = 0x000020a1;
					*(volatile unsigned int*)(USB_UDC_REG_BASE + 0x44) = 0x00020000;
					pDevReg->InterruptDes &= 0xF7;
				}

				pDevReg->InterruptDes |= INTXFER_IOC;
				pDevReg->InterruptDes |= INTXFER_ACTIVE;
			}
	}
}//static void next_in_dma()
extern void usbd_tx_complete (struct usb_endpoint_instance *endpoint);
static void wmt_udc_epn_tx (int ep)
{
		struct usb_endpoint_instance *endpoint = wmt_find_ep (ep);

		/* We need to transmit a terminating zero-length packet now if
		 * we have sent all of the data in this URB and the transfer
		 * size was an exact multiple of the packet size.
		 */
		if(ep == 0x83) /*interupt*/
		    next_in_dma (endpoint);
		else {
    		if (endpoint->tx_urb && (endpoint->last == endpoint->tx_packetSize)
    		    && (endpoint->tx_urb->actual_length - endpoint->sent - endpoint->last == 0)) {
    		    /* Prepare to transmit a zero-length packet. */
    		    endpoint->sent += endpoint->last;
    		    /* write 0 bytes of data to FIFO */
    		    next_in_dma (endpoint);

    		} else if (endpoint->tx_urb && endpoint->tx_urb->actual_length) {
    		    /* retire the data that was just sent */
    		    usbd_tx_complete (endpoint);
    		    /* Check to see if we have more data ready to transmit
    		     * now.
    		     */
    		    if (endpoint->tx_urb && endpoint->tx_urb->actual_length) {
    		        /* write data to FIFO */
    		        next_in_dma (endpoint);
    		    }
    		}else{
    			Nprintf("[wmt_udc_epn_tx] test\n");

    		}
		}
}

extern void usbd_rcv_complete(struct usb_endpoint_instance *endpoint, int len, int urb_bad);
static void wmt_udc_epn_rx (int ep)
{
	/* Check endpoint status */
	int nbytes;
	struct usb_endpoint_instance *endpoint = wmt_find_ep (ep);
	nbytes = next_out_dma_tty (endpoint);
	usbd_rcv_complete (endpoint, nbytes, 0);
}


#else

extern void usbd_rcv_complete(struct usb_endpoint_instance *endpoint, int len, int urb_bad);
static int next_out_dma(struct usb_endpoint_instance *endpoint,u8 *bufp, u32 len)
{
	u32 dma_ccr = 0;
	u32	temp32;
	Nprintf("%s,bufp:0x%x,len=0x%x\n",__FUNCTION__,bufp,len);

	if ((endpoint->endpoint_address & 0x7F)  == 2) {/*Bulk Out*/

		Nprintf("pDevReg->Bulk2DesStatus:0x%x,\n",pDevReg->Bulk2DesStatus);
		/*patch short packet data toggle */
		if (pDevReg->Bulk2DesStatus & BULKXFER_SHORTPKT) {
			if (pDevReg->Bulk2DesTbytes2 & BULKXFER_DATA1) {
				ep[2].toggle_bit = 1;
			} else
				ep[2].toggle_bit =0;
		}
	  	pUdcDmaReg->DMA_Global_Bits.SoftwareReset1 = 1;
		while (pUdcDmaReg->DMA_Global_Bits.SoftwareReset1)
		;
		pUdcDmaReg->DMA_Global_Bits.DMAConrollerEnable = 1;
		pUdcDmaReg->DMA_IER_Bits.DMAInterruptEnable1 = 1;
  		wmt_udc_pdma_des_prepare(len, bufp, DESCRIPTOT_TYPE_LONG, TRANS_OUT);
		pDevReg->Bulk2DesStatus = 0x00;
		pDevReg->Bulk2DesTbytes2 = (len >> 16) & 0x0F;
		pDevReg->Bulk2DesTbytes1 = (len >> 8) & 0xFF;
		pDevReg->Bulk2DesTbytes0 = len & 0xFF;
		Nprintf("init ep[2].toggle_bit:%d,\n",ep[2].toggle_bit);

		/* set endpoint data toggle*/
		if (ep[2].toggle_bit) {
			pDevReg->Bulk2DesTbytes2 |= BULKXFER_DATA1;
		} else {
			pDevReg->Bulk2DesTbytes2 &= 0x3F;/*BULKXFER_DATA0;*/
		}

		Nprintf("pDevReg->Bulk2DesTbytes2:0x%x,\n",pDevReg->Bulk2DesTbytes2);
		if (len > ep[2].maxpacket) {
			/*ex : 1024 /512 = 2  2 % 2 = 0*/
			temp32 = (len + ep[2].maxpacket - 1) / ep[2].maxpacket;
			ep[2].toggle_bit = ((temp32 + ep[2].toggle_bit) % 2);
		} else {
			if (ep[2].toggle_bit == 0) {
				ep[2].toggle_bit = 1;
			} else {
				ep[2].toggle_bit = 0;
			}
		}
		Nprintf("next ep[2].toggle_bit:%d,\n",ep[2].toggle_bit);

		pDevReg->Bulk2DesStatus = BULKXFER_IOC;

		dma_ccr = 0;
		dma_ccr = DMA_RUN;
		dma_ccr |= DMA_TRANS_OUT_DIR;

		pUdcDmaReg->DMA_Context_Control1 = dma_ccr;
		pDevReg->Bulk2DesStatus |= BULKXFER_ACTIVE;

	}
	return len;
}

static void next_in_dma(struct usb_endpoint_instance *endpoint)
{
	u32 temp32;
	u32 dma_ccr = 0;
	unsigned int length = 0;
	u8 *buf;
	struct urb *urb = endpoint->tx_urb;
	if (urb) {
		unsigned int last = 0;
		buf = urb->buffer;
		Nprintf("%s,buf :0x%x\n",__FUNCTION__,buf);

		if (((endpoint->endpoint_address & 0x7F) ==  1)) {/*Bulk In*/
			///*
			if ((last = MIN (urb->actual_length - endpoint->sent,endpoint->tx_packetSize))) {
				u8 *cp = urb->buffer + endpoint->sent;

				Nprintf ("endpoint->sent %d, tx_packetSize %d, last %d\n", endpoint->sent, endpoint->tx_packetSize, last);

				endpoint->last = last;
				length = last;
				Nprintf("cp = 0x%8.8x\n",cp);


				while(last) {
					buf[length-last] = cp[length-last];
					last--;
				}

			}
			pUdcDmaReg->DMA_Global_Bits.SoftwareReset0 = 1;
			while (pUdcDmaReg->DMA_Global_Bits.SoftwareReset0)
			;
			pUdcDmaReg->DMA_Global_Bits.DMAConrollerEnable = 1;
			pUdcDmaReg->DMA_IER_Bits.DMAInterruptEnable0 = 1;

			wmt_udc_pdma_des_prepare(length, buf, DESCRIPTOT_TYPE_LONG, TRANS_IN);
			pDevReg->Bulk1DesStatus = 0x00;
			pDevReg->Bulk1DesTbytes2 = (length >> 16) & 0x3;
			pDevReg->Bulk1DesTbytes1 = (length >> 8) & 0xFF;
			pDevReg->Bulk1DesTbytes0 = length & 0xFF;

			/* set endpoint data toggle*/
			if (ep[1].toggle_bit)
				pDevReg->Bulk1DesTbytes2 |= 0x40;/* BULKXFER_DATA1;*/
			else
				pDevReg->Bulk1DesTbytes2 &= 0xBF;/*(~BULKXFER_DATA1);*/

			pDevReg->Bulk1DesStatus = (BULKXFER_IOC | BULKXFER_IN);

			if (length > ep[1].maxpacket) {
				/*ex : 512 /64 = 8  8 % 2 = 0*/
				temp32 = (length + ep[1].maxpacket - 1) / ep[1].maxpacket;
				ep[1].toggle_bit = ((temp32 + ep[1].toggle_bit) % 2);
			} else {
				if (ep[1].toggle_bit == 0)
					ep[1].toggle_bit = 1;
				else
					ep[1].toggle_bit = 0;
			}

			/* DMA Channel Control Reg*/
			/* Software Request + Channel Enable*/
			dma_ccr = 0;
			dma_ccr = DMA_RUN;
			if (length) /* PDMA can not support 0 byte transfer*/
				pUdcDmaReg->DMA_Context_Control0 = dma_ccr;

			pDevReg->Bulk1DesStatus |= BULKXFER_ACTIVE;

		}
	}
}
extern void usbd_tx_complete (struct usb_endpoint_instance *endpoint);
static void wmt_udc_epn_tx (int ep)
{
		//unsigned short status;
		struct usb_endpoint_instance *endpoint = wmt_find_ep (ep);
		Nprintf("%s,ep:%x,endpoint:%x\n",__FUNCTION__,ep,endpoint);

		/* We need to transmit a terminating zero-length packet now if
		 * we have sent all of the data in this URB and the transfer
		 * size was an exact multiple of the packet size.
		 */

	    		if (endpoint->tx_urb && (endpoint->last == endpoint->tx_packetSize)
	    		    && (endpoint->tx_urb->actual_length - endpoint->sent - endpoint->last == 0)) {
	    		    /* Prepare to transmit a zero-length packet. */
	    		    endpoint->sent += endpoint->last;
	    		    /* write 0 bytes of data to FIFO */
	    		    next_in_dma (endpoint);

	    		} else if (endpoint->tx_urb && endpoint->tx_urb->actual_length) {

	    		    /* retire the data that was just sent */
	    		    usbd_tx_complete (endpoint);
	    		    /* Check to see if we have more data ready to transmit
	    		     * now.
	    		     */
	    		    if (endpoint->tx_urb && endpoint->tx_urb->actual_length) {
	    		        /* write data to FIFO */
	    		        next_in_dma (endpoint);
	    		    }
	    		} else {
	    			Nprintf("[wmt_udc_epn_tx] test\n");

	    		}


}
extern  struct cmd_fastboot_interface priv;
static void wmt_udc_epn_rx (int ep)
{
		Nprintf("%s\n",__FUNCTION__);

		int c_bytes = 0 ;
		u32  len;
		u32	dcmd,  temp32;

		struct usb_endpoint_instance *endpoint = wmt_find_ep (ep);
		struct urb *urb = endpoint->rcv_urb;
		Nprintf ("urb->actual_length 0x%x \n", urb->actual_length );
		udc_cmd_flag = 1;
		u8 *cp = urb->buffer ;
		Nprintf("%s,ep:%x,endpoint:%x\n", __FUNCTION__, ep, endpoint);
		if ((endpoint->rcv_transferSize) && udc_download_flag)  {
			Nprintf ("udc_download_flag=1,endpoint->rcv_transferSize=0x%x \n" ,endpoint->rcv_transferSize);
			udc_cmd_flag = 0;
			if ( endpoint->rcv_transferSize  > UBE_MAX_DMA) {
				temp32 = UBE_MAX_DMA;
			} else {
				temp32 = endpoint->rcv_transferSize;
			}
			udc_download_last_bytes = temp32;
			/*Note: it need copy length UBE_MAX_DMA*/
			memcpy_itp(priv.transfer_buffer + priv.d_bytes, cp, UBE_MAX_DMA);
			/*Next dma length temp32*/
			next_out_dma (endpoint, cp, temp32);
			/*receive length UBE_MAX_DMA*/
			usbd_rcv_complete (endpoint, UBE_MAX_DMA , 0);
			endpoint->rcv_transferSize -=  temp32;
			return;/*avoid corner case of following cmd compare*/
		} else if ((endpoint->rcv_transferSize == 0) && udc_download_flag ) {
			Nprintf ("udc_download_flag=0 ,udc_download_last_bytes=%d\n" ,udc_download_last_bytes);
			udc_cmd_flag = 0;
			memcpy_itp(priv.transfer_buffer + priv.d_bytes, cp, udc_download_last_bytes);
			next_out_dma (endpoint, cp, 0x100);
			usbd_rcv_complete (endpoint, udc_download_last_bytes , 0);
			d_bytes =0;
			return;/*avoid corner case of following cmd compare*/
		}
		if (memcmp(urb->buffer, "download:", 9) == 0 ) {
			dcmd = urb->buffer_length;//0x100
			len = (pDevReg->Bulk2DesTbytes0 & 0xff);
			c_bytes  = dcmd - len;  //transfer bytes
			/*CharlesTu,2013.01.04,need null terminate for simple_strtoul*/
			*(urb->buffer + c_bytes) = 0;
			endpoint->rcv_transferSize  = simple_strtoul ((void *)(urb->buffer + 9), NULL, 16);
			 d_bytes_total = endpoint->rcv_transferSize ;// request download bytes
			Nprintf ("download  endpoint->rcv_transferSize 0x%x bytes\n", endpoint->rcv_transferSize );
			 if ( endpoint->rcv_transferSize  > UBE_MAX_DMA)
				temp32 = UBE_MAX_DMA;
			else {
				temp32 = endpoint->rcv_transferSize;
				udc_download_last_bytes = temp32;
			}

			next_out_dma (endpoint,cp, temp32);
			udc_download_flag = 1;
			udc_cmd_flag = 1;
			usbd_rcv_complete (endpoint, c_bytes , 0);
			endpoint->rcv_transferSize -=  temp32;
		}
		if (memcmp(urb->buffer, "flash:",6 ) == 0 ) {
			dcmd = urb->buffer_length;//0x100
			len = (pDevReg->Bulk2DesTbytes0 & 0xff);
			c_bytes  = dcmd - len;  //transfer bytes
			Nprintf ("flash  of %d bytes\n", d_bytes_total );
			next_out_dma (endpoint,cp, 0x100);
			usbd_rcv_complete (endpoint, c_bytes , 0);
		}
		if (memcmp(urb->buffer, "getvar:", 7) == 0) {
			dcmd = urb->buffer_length;//0x100
			Nprintf("udc rx getvar\n");
			len = (pDevReg->Bulk2DesTbytes0 & 0xff);
			c_bytes  = dcmd - len;  //transfer bytes
			next_out_dma (endpoint,cp, 0x100);
			usbd_rcv_complete (endpoint, c_bytes , 0);
		}

		if (memcmp(urb->buffer, "erase:", 6) == 0) {
			Nprintf("udc rx erase\n");
			dcmd = urb->buffer_length;//0x100
			len = (pDevReg->Bulk2DesTbytes0 & 0xff);
			c_bytes  = dcmd - len;  //transfer bytes
			next_out_dma (endpoint,cp, 0x100);
			usbd_rcv_complete (endpoint, c_bytes , 0);
		}
		if ((memcmp(urb->buffer, "reboot", 6) == 0) ||
			(memcmp(urb->buffer, "reboot-bootloader", 17) == 0)) {
			Nprintf("udc rx reboot\n");
			dcmd = urb->buffer_length;//0x100
			len = (pDevReg->Bulk2DesTbytes0 & 0xff);
			c_bytes  = dcmd - len;  //transfer bytes
			next_out_dma (endpoint,cp, 0x100);
			usbd_rcv_complete (endpoint, c_bytes , 0);
		}

		if (memcmp(urb->buffer, "continue", 8) == 0) {
			Nprintf("udc rx reboot\n");
			dcmd = urb->buffer_length;//0x100
			len = (pDevReg->Bulk2DesTbytes0 & 0xff);
			c_bytes  = dcmd - len;  //transfer bytes
			next_out_dma (endpoint,cp, 0x100);
			usbd_rcv_complete (endpoint, c_bytes , 0);
		}
		if (memcmp(urb->buffer, "oem", 3) == 0 ) {
			Nprintf("udc rx oem\n");
			dcmd = urb->buffer_length;//0x100
			len = (pDevReg->Bulk2DesTbytes0 & 0xff);
			c_bytes  = dcmd - len;  //transfer bytes
			next_out_dma (endpoint,cp, 0x100);
			usbd_rcv_complete (endpoint, c_bytes , 0);

		}


}
#endif

void dma_irq(int addr)
{
	Nprintf("%s,addr 0x%x\n",__FUNCTION__,addr);
	if ((addr & 0x7F) == 1) {/*Bulk In*/
  	 wmt_udc_epn_tx(0x81);
	} else if ((addr & 0x7F) == 2) {/*Bulk Out*/
  	 wmt_udc_epn_rx(0x2);
	} else if ((addr & 0x7F) == 3) {/*Interrupt In*/
	 wmt_udc_epn_tx(0x83);
	} /*EP3 : Interrupt In*/
}

extern struct urb *usbd_alloc_urb (struct usb_device_instance *device, struct usb_endpoint_instance *endpoint);
void udc_enable (struct usb_device_instance *device)
{
	/* Save the device structure pointer */
  int i = 0;

	udc_device = device;

	/* Setup ep0 urb */
	if (!ep0_urb)
		ep0_urb =	usbd_alloc_urb(udc_device,	udc_device->bus->endpoint_array);
	else
		Nprintf("udc_enable: ep0_urb already allocated %p\n",ep0_urb);

  for (i = 0; i < 4; i++) {
		if (udc_device->bus->endpoint_array[i].endpoint_address == 2)
			udc_device->bus->endpoint_array[i].endpoint_address = 2;
		if (udc_device->bus->endpoint_array[i].endpoint_address == 0x81)
			udc_device->bus->endpoint_array[i].endpoint_address = 0x81;
		if (udc_device->bus->endpoint_array[i].endpoint_address == 0x85)
			udc_device->bus->endpoint_array[i].endpoint_address = 0x83;
	}
}

/* Switch off the UDC */
void udc_disable (void)
{
	printf("disable UDC\n");
	/* Free ep0 URB */
	if (ep0_urb) {
		/*usbd_dealloc_urb(ep0_urb); */
		ep0_urb = NULL;
	}
	/*disable DMA*/
	pUdcDmaReg->DMA_Global_Bits.DMAConrollerEnable = 0;
	pDevReg->ControlEpControl = 0;
	/* disable DMA and run bit of endpoint*/
	pDevReg->Bulk1EpControl = 0;
	pDevReg->Bulk2EpControl = 0;
	pDevReg->InterruptEpControl = 0;
	/*disable interrupt*/
	pDevReg->IntEnable = 0;
	/*stop controller*/
	pDevReg->CommandStatus &= ~0x1F;

}
void udc_startup_events (struct usb_device_instance *device)
{
	Nprintf("%s\n",__FUNCTION__);

	/* The DEVICE_INIT event puts the USB device in the state STATE_INIT. */
	usbd_device_event_irq (device, DEVICE_INIT, 0);

	/* The DEVICE_CREATE event puts the USB device in the state
	 * STATE_ATTACHED.
	 */
	usbd_device_event_irq (device, DEVICE_CREATE, 0);

	/* Some USB controller driver implementations signal
	 * DEVICE_HUB_CONFIGURED and DEVICE_RESET events here.
	 * DEVICE_HUB_CONFIGURED causes a transition to the state STATE_POWERED,
	 * and DEVICE_RESET causes a transition to the state STATE_DEFAULT.
	 * The OMAP USB client controller has the capability to detect when the
	 * USB cable is connected to a powered USB bus via the ATT bit in the
	 * DEVSTAT register, so we will defer the DEVICE_HUB_CONFIGURED and
	 * DEVICE_RESET events until later.
	 */
	Nprintf("[udc_startup_events]go to udc_enable\n");
	udc_enable (device);
}

static void wmt_pdma_init(void)
{
	Nprintf("%s\n",__FUNCTION__);

	/*descriptor locate*/
	UdcPdmaAddrLI = (unsigned int)malloc(0x120);//dma_alloc_coherent(pDMADescLI, 0x100, &UdcPdmaPhyAddrLI, GFP_KERNEL|GFP_ATOMIC);
	UdcPdmaAddrSI = (unsigned int)malloc(0x120);//dma_alloc_coherent(pDMADescSI, 0x100, &UdcPdmaPhyAddrSI, GFP_KERNEL|GFP_ATOMIC);
	UdcPdmaAddrLO = (unsigned int)malloc(USB_DESCRIPT_LO_SIZE+0x20);//dma_alloc_coherent(pDMADescLO, 0x100, &UdcPdmaPhyAddrLO, GFP_KERNEL|GFP_ATOMIC);
	UdcPdmaAddrSO = (unsigned int)malloc(0x120);//dma_alloc_coherent(pDMADescSO, 0x100, &UdcPdmaPhyAddrSO, GFP_KERNEL|GFP_ATOMIC);
	UdcPdmaAddrLI = (UdcPdmaAddrLI + 0x20) & ~0x1F;
	UdcPdmaAddrSI = (UdcPdmaAddrSI + 0x20) & ~0x1F;
	UdcPdmaAddrLO = (UdcPdmaAddrLO + 0x20) & ~0x1F;
	UdcPdmaAddrSO = (UdcPdmaAddrSO + 0x20) & ~0x1F;

	memset((void *)UdcPdmaAddrLI, 0, 0x100);
	memset((void *)UdcPdmaAddrSI, 0, 0x100);
	memset((void *)UdcPdmaAddrLO, 0, USB_DESCRIPT_LO_SIZE); //need loop descriptor for bulk out
	memset((void *)UdcPdmaAddrSO, 0, 0x100);

	/*descriptor locate*/

	/*pdma HW setting*/
	pUdcDmaReg->DMA_Global_Bits.DMAConrollerEnable = 1;/*enable DMA*/
	pUdcDmaReg->DMA_Global_Bits.SoftwareReset = 1;
	while (pUdcDmaReg->DMA_Global_Bits.SoftwareReset)/*wait reset complete*/
	;
	pUdcDmaReg->DMA_Global_Bits.DMAConrollerEnable = 1;/*enable DMA*/
	pUdcDmaReg->DMA_IER_Bits.DMAInterruptEnable0 = 1;
	pUdcDmaReg->DMA_IER_Bits.DMAInterruptEnable1 = 1;

	pUdcDmaReg->DMA_Context_Control0_Bis.TransDir = 0;
	pUdcDmaReg->DMA_Context_Control1_Bis.TransDir = 1;
	/*pdma HW setting*/
	/*buffer locate for temp buffer*/
	bulk_out_dma1 =  (unsigned char *)malloc(UBE_MAX_DMA);
	bulk_out_dma2 = (unsigned char *)malloc(512);
	bulk_in_dma1 = (unsigned char *)malloc(1024);
	bulk_in_dma2 = (unsigned char *)malloc(1024);
	int_in_dma1 = (unsigned char *)malloc(8*1024);

	memset(bulk_out_dma1, 0, 512);
	memset(bulk_out_dma2, 0, 512);
	memset(bulk_in_dma1, 0, 1024);
	memset(bulk_in_dma2, 0, 1024);
	memset(int_in_dma1, 0, 8*1024);
	/*buffer locate*/

	bulk_in_dma1[0] = 'N';
	bulk_in_dma1[1] = 'e';
	bulk_in_dma1[2] = 'i';
	bulk_in_dma1[3] = 'l';
	bulk_in_dma1[4] = '_';
	bulk_in_dma1[5] = 't';
	bulk_in_dma1[6] = 'e';
	bulk_in_dma1[7] = 's';
	bulk_in_dma1[8] = 't';
	bulk_in_dma1[9] = '\r';

	bulk_in_dma2[0] = 't';
	bulk_in_dma2[1] = 'e';
	bulk_in_dma2[2] = 's';
	bulk_in_dma2[3] = 't';
	bulk_in_dma2[4] = '\r';


}

int udc_init (void)
{
	Nprintf("%s\n",__FUNCTION__);

	/*UDC Register Space         0x400~0x7EF*/
	pDevReg    = (struct UDC_REGISTER *)USB_UDC_REG_BASE;
	pUdcDmaReg = (struct UDC_DMA_REG *)USB_UDC_DMA_REG_BASE;
	pSetupCommand = (PSETUPCOMMAND)(USB_UDC_REG_BASE + 0x300);
	pSetupCommandBuf = (unsigned char *)(USB_UDC_REG_BASE + 0x300);
	SetupBuf = (UCHAR *)(USB_UDC_REG_BASE + 0x340);
	EP0_BUF = SetupBuf;
	IntBuf = (UCHAR *)(USB_UDC_REG_BASE + 0x40);
	pUSBMiscControlRegister5 = (unsigned char *)(USB_UDC_REG_BASE + 0x1A0);

#ifdef OTGIP
	/*UHDC Global Register Space 0x7F0~0x7F7*/
	pGlobalReg = (struct USB_GLOBAL_REG *) USB_GLOBAL_REG_BASE;
#endif

	*pUSBMiscControlRegister5 = 0x01;
	pDevReg->CommandStatus &= 0x1F;
	pDevReg->CommandStatus |= USBREG_RESETCONTROLLER;

	while (pDevReg->CommandStatus & USBREG_RESETCONTROLLER)
		;

	wmt_pdma_init();

	pDevReg->Bulk1EpControl = 0; /* stop the bulk DMA*/
	while (pDevReg->Bulk1EpControl & EP_ACTIVE) /* wait the DMA stopped*/
		;
	pDevReg->Bulk2EpControl = 0; /* stop the bulk DMA*/
	while (pDevReg->Bulk2EpControl & EP_ACTIVE) /* wait the DMA stopped*/
		;
	pDevReg->Bulk1DesStatus = 0x00;
	pDevReg->Bulk2DesStatus = 0x00;

	pDevReg->Bulk1DesTbytes0 = 0;
	pDevReg->Bulk1DesTbytes1 = 0;
	pDevReg->Bulk1DesTbytes2 = 0;

	pDevReg->Bulk2DesTbytes0 = 0;
	pDevReg->Bulk2DesTbytes1 = 0;
	pDevReg->Bulk2DesTbytes2 = 0;

	/* enable DMA and run the control endpoint*/
	pDevReg->ControlEpControl = EP_RUN + EP_ENABLEDMA;
	/* enable DMA and run the bulk endpoint*/
	pDevReg->Bulk1EpControl = EP_RUN + EP_ENABLEDMA;
	pDevReg->Bulk2EpControl = EP_RUN + EP_ENABLEDMA;
	pDevReg->InterruptEpControl = EP_RUN + EP_ENABLEDMA;
	/* enable DMA and run the interrupt endpoint*/
	/* UsbControlRegister.InterruptEpControl = EP_RUN+EP_ENABLEDMA;*/
	/* run the USB controller*/
	pDevReg->MiscControl3 = 0x38;

	/* HW attach process evaluation enable bit For WM3426 and after project*/
	/*pDevReg->FunctionPatchEn |= 0x20;*/
#ifdef HW_BUG_HIGH_SPEED_PHY
	pDevReg->MiscControl0 &= ~0x80;
#endif
	pDevReg->MiscControl0 |= 0x02;
	pDevReg->CommandStatus = USBREG_RUNCONTROLLER;
	ControlState = CONTROLSTATE_SETUP;
	/*UDC setting*/
	/* enable all interrupt*/
	pDevReg->IntEnable = INTENABLE_ALL;/*0x70*/
	/* set IOC on the Setup decscriptor to accept the Setup request*/
	pDevReg->ControlDesControl = CTRLXFER_IOC;

	return 0;
}

void udc_disconnect (void)
{
	UDCDBG ("disconnect, disable Pullup");
	pDevReg->FunctionPatchEn &= ~0x20;

	g_udc_connected = 0;
}

void udc_connect (void)
{
	Nprintf("%s\n",__FUNCTION__);
	/* HW attach process evaluation enable bit For WM3426 and after project*/
	pDevReg->FunctionPatchEn |= 0x20;

	g_udc_connected = 0;
}

void udc_setup_ep (struct usb_device_instance *device,
		   unsigned int ep, struct usb_endpoint_instance *endpoint)
{
	Nprintf("%s\n",__FUNCTION__);

	//static struct wmt_usb_device_descriptor *usb_device_descriptor_tmp;// = device->device_descriptor;
	static struct usb_device_descriptor *usb_device_descriptor_tmp;

	static struct usb_alternate_instance *usb_alternate_instance_tmp;
	static struct usb_interface_instance *usb_interface_instance_tmp;
	static struct usb_configuration_instance *usb_configuration_instance_tmp;
	static struct usb_configuration_descriptor *usb_configuration_descriptor_tmp;
	static struct usb_interface_descriptor *usb_interface_descriptor_tmp;

	static struct usb_endpoint_instance *usb_endpoint_instance_bulk_in;
	static struct usb_endpoint_instance *usb_endpoint_instance_bulk_out;
	static struct usb_endpoint_instance *usb_endpoint_instance_int_in;
	usb_device_descriptor_tmp = device->device_descriptor;

	usb_configuration_instance_tmp = device->configuration_instance_array;
	usb_configuration_descriptor_tmp = usb_configuration_instance_tmp->configuration_descriptor;
	usb_interface_instance_tmp = usb_configuration_instance_tmp->interface_instance_array;

	usb_alternate_instance_tmp = usb_interface_instance_tmp->alternates_instance_array;
	usb_interface_descriptor_tmp = usb_alternate_instance_tmp->interface_descriptor;

	usb_endpoint_instance_bulk_in = endpoint + 2;
	usb_endpoint_instance_bulk_out = endpoint + 1;
	usb_endpoint_instance_int_in = endpoint + 3;

	device_desc.bLength = usb_device_descriptor_tmp->bLength;
	device_desc.bDescriptorType = usb_device_descriptor_tmp->bDescriptorType;
	device_desc.bcdUSB = usb_device_descriptor_tmp->bcdUSB;
	device_desc.bDeviceClass = 0xff;//Susb_device_descriptor_tmp->bDeviceClass;
	device_desc.bDeviceSubClass = usb_device_descriptor_tmp->bDeviceSubClass;
	device_desc.bDeviceProtocol = usb_device_descriptor_tmp->bDeviceProtocol;
	device_desc.bMaxPacketSize0 = usb_device_descriptor_tmp->bMaxPacketSize0;
	device_desc.bcdDevice = usb_device_descriptor_tmp->bcdDevice;
	device_desc.iManufacturer = usb_device_descriptor_tmp->iManufacturer;
	device_desc.iProduct = usb_device_descriptor_tmp->iProduct;
	device_desc.iSerialNumber = usb_device_descriptor_tmp->iSerialNumber;
	device_desc.bNumConfigurations = usb_device_descriptor_tmp->bNumConfigurations;

	config_desc.wTotalLength = 39;//usb_configuration_descriptor_tmp->wTotalLength;
	config_desc.bNumInterfaces = usb_configuration_descriptor_tmp->bNumInterfaces;
	config_desc.bConfigurationValue = usb_configuration_descriptor_tmp->bConfigurationValue;
	config_desc.iConfiguration = usb_configuration_descriptor_tmp->iConfiguration;
	config_desc.bmAttributes = usb_configuration_descriptor_tmp->bmAttributes;
	config_desc.bMaxPower = usb_configuration_descriptor_tmp->bMaxPower;

	intf_desc.bInterfaceNumber = usb_interface_descriptor_tmp->bInterfaceNumber;
	intf_desc.bAlternateSetting = usb_interface_descriptor_tmp->bAlternateSetting;
	 /* Charles, for Window x86 bNumEndpoints has to be 2*/
	intf_desc.bNumEndpoints = 2;//usb_interface_descriptor_tmp->bNumEndpoints;
#ifdef CONFIG_USB_TTY
	intf_desc.bInterfaceClass = 0x02;//usb_interface_descriptor_tmp->bInterfaceClass;
	intf_desc.bInterfaceSubClass = 0x0a;//usb_interface_descriptor_tmp->bInterfaceSubClass;
	intf_desc.bInterfaceProtocol = 0x00;//usb_interface_descriptor_tmp->bInterfaceProtocol;
#else
	intf_desc.bInterfaceClass = 0xff;//usb_interface_descriptor_tmp->bInterfaceClass;
	intf_desc.bInterfaceSubClass = 0x42;//usb_interface_descriptor_tmp->bInterfaceSubClass;
	intf_desc.bInterfaceProtocol = 0x03;//usb_interface_descriptor_tmp->bInterfaceProtocol;
#endif

	intf_desc.iInterface = usb_interface_descriptor_tmp->iInterface;

	fs_bulk_out_desc.bEndpointAddress = 2;
	//fs_bulk_out_desc.bEndpointAddress = pDevReg->DeviceAddr ;
	fs_bulk_out_desc.bmAttributes = 2;
	fs_bulk_out_desc.wMaxPacketSize = 512;//1024;//0x0040;
	fs_bulk_out_desc.bInterval = 0x00;

	fs_bulk_in_desc.bEndpointAddress = 0x81;
	fs_bulk_in_desc.bmAttributes = 2;
	fs_bulk_in_desc.wMaxPacketSize = 512;//1024;//0x0040;//512;
	fs_bulk_in_desc.bInterval = 0x00;

	fs_interrupt_desc.bEndpointAddress = 0x83;
	fs_interrupt_desc.bmAttributes = 0x03;
	fs_interrupt_desc.wMaxPacketSize = 8;
	fs_interrupt_desc.bInterval = 6;


	wmt_udc_setup();

}

void udc_endpoint_write (struct usb_endpoint_instance *endpoint)
{
	Nprintf("%s\n",__FUNCTION__);
	if (endpoint->tx_urb) {
		next_in_dma(endpoint);
	}
}

/* check if the usb connected to PC */
int wmt_udc_connected(void)
{
	return g_udc_connected;
}

/*
 void memdump (void *pv, int num)
{
	int i;
	unsigned char *pc = (unsigned char *) pv;
	unsigned int  temp = 0,temp1 = 0,tmp1,tmp,ba;
	ba=pv;
	for (tmp = 0;tmp < num/16; tmp++)
	printf("[%8.8x]%8.8x %8.8x %8.8x %8.8x\n",(0x10*tmp+ba),*(volatile unsigned int *)(ba+(tmp*0x10)),*(volatile unsigned int *)(ba+(tmp*0x10)+0x4),*(volatile unsigned int *)(ba+(tmp*0x10)+0x8),*(volatile unsigned int *)(ba+(tmp*0x10)+0xc));


}


 void udc_device_dump_register(void)
{
	volatile unsigned int  address;
	volatile unsigned char temp8;
	int i;

	for (i = 0x20; i <= 0x3F; i++) { //0x20~0x3F  (0x3F-0x20 + 1) /4 = 8 DWORD
		address =  (USB_UDC_REG_BASE + i);
		temp8 = REG_GET8(address);
		Nprintf("[UDC Device] Offset[0x%8.8X] = 0x%2.2X \n", address, temp8);
	}

	for (i = 0x308; i <= 0x30D; i++) {//0x308~0x30D
		address =  (USB_UDC_REG_BASE + i);
		temp8 = REG_GET8(address);
		Nprintf("[UDC Device] Offset[0x%8.8X] = 0x%2.2X \n", address, temp8);
	}
	for (i = 0x310; i <= 0x317; i++) { //0x310~0x317
		address =  (USB_UDC_REG_BASE + i);
		temp8 = REG_GET8(address);
		Nprintf("[UDC Device] Offset[0x%8.8X] = 0x%2.2X \n", address, temp8);
	}
}

void udc_bulk_dma_dump_register(void)
{
	volatile unsigned int  temp32;
	int i;
	for (i = 0; i <= 14; i++) {//0x100 ~ 0x138
		temp32 = REG_GET32((USB_UDC_DMA_REG_BASE + i*4));
		Nprintf("[UDC Bulk DMA1] Offset[0x%8.8X] = 0x%8.8X \n", (USB_UDC_DMA_REG_BASE + i*4), temp32);
	}

}
*/
#endif
