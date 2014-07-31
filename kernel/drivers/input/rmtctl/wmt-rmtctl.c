/*++ 
 * linux/drivers/input/rmtctl/wmt-rmtctl.c
 * WonderMedia input remote control driver
 *
 * Copyright c 2010  WonderMedia  Technologies, Inc.
 *
 * This program is free software: you can redistribute it and/or modify 
 * it under the terms of the GNU General Public License as published by 
 * the Free Software Foundation, either version 2 of the License, or 
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, 
 * but WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
 * GNU General Public License for more details. 
 *
 * You should have received a copy of the GNU General Public License 
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * WonderMedia Technologies, Inc.
 * 4F, 533, Chung-Cheng Road, Hsin-Tien, Taipei 231, R.O.C
--*/
  
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/device.h>
#include <linux/version.h>
#include <linux/input.h>
#include <linux/platform_device.h>
#include <asm/irq.h>
#include <mach/hardware.h>
#include <mach/wmt_pmc.h>
#include <linux/delay.h>
#include <linux/firmware.h>
#include <linux/workqueue.h>
#include <linux/slab.h>
#include <linux/gpio.h>

#include "wmt-rmtctl.h"

enum CIR_CODEC_TYPE {
	CIR_CODEC_NEC = 0,
	CIR_CODEC_TOSHIBA = 1,
	CIR_CODEC_MAX,
};

struct cir_param {
	enum CIR_CODEC_TYPE codec;
	unsigned int param[7];
	unsigned int repeat_timeout;
};

struct cir_vendor_info {
	char *vendor_name;
	enum CIR_CODEC_TYPE codec;
	
	unsigned int vendor_code;
	unsigned int wakeup_code;
	unsigned int key_codes[255]; // usb-keyboard standard
};

#define RMTCTL_DEBUG 0
#define REL_DELTA    20
#define WMT_RMTCTL_VENDOR_ENV "wmt.io.rmtctl.vendorcode"
struct rmtctl_led{
    int gpio;
    int active;
    int on;
    int enable;
    struct timer_list timer;
};

struct rmtctl_rel {
	int on;
	unsigned int code;
	struct timer_list timer;
	struct input_dev *input;
};

struct rmtctl_priv {
	enum CIR_CODEC_TYPE codec;
	int vendor_index;
    int table_index;
    unsigned int saved_vcode;
	unsigned int vendor_code;
	unsigned int scan_code;

	struct input_dev *idev;
	struct timer_list timer;
	struct delayed_work delaywork;

    struct rmtctl_led led;//led control
    
    struct rmtctl_rel rel_dev;//remote cursor removing
};

static struct cir_param rmtctl_params[] = {
	// NEC : |9ms carrier wave| + |4.5ms interval|
	[CIR_CODEC_NEC] = {
		.codec = CIR_CODEC_NEC,
		.param = { 0x10a, 0x8e, 0x42, 0x55, 0x9, 0x13, 0x13 },
		.repeat_timeout = 17965000,
	},

	// TOSHIBA : |4.5ms carrier wave| + |4.5ms interval|
	[CIR_CODEC_TOSHIBA] = {
		.codec = CIR_CODEC_TOSHIBA,
		.param = { 0x8e, 0x8e, 0x42, 0x55, 0x9, 0x13, 0x13 },
		.repeat_timeout = 17965000,
	},
};

static struct cir_vendor_info rmtctl_vendors[] = {
	// SRC1804
	{
		.vendor_name = "SRC1804",
		.codec = CIR_CODEC_NEC,
		.vendor_code = 0x02fd,
		.wakeup_code = 0x57,
		.key_codes = {
			[0x57] = KEY_POWER,      /* power down */
			[0x56] = KEY_VOLUMEDOWN, /* vol- */
			[0x14] = KEY_VOLUMEUP,   /* vol+ */
			[0x53] = KEY_HOME,       /* home */
			[0x11] = KEY_MENU,       /* menu */
			[0x10] = KEY_BACK,       /* back */
			[0x4b] = KEY_ZOOMOUT,    /* zoom out */
			[0x08] = KEY_ZOOMIN,     /* zoom in */
			[0x0d] = KEY_UP,         /* up */
			[0x4e] = KEY_LEFT,       /* left */
			[0x19] = KEY_ENTER,      /* OK */
			[0x0c] = KEY_RIGHT,      /* right */
			[0x4f] = KEY_DOWN,       /* down */
			[0x09] = KEY_PAGEUP,     /* page up */
			[0x47] = KEY_PREVIOUSSONG, /* rewind */
			[0x05] = KEY_PAGEDOWN,   /* page down */
			[0x04] = KEY_NEXTSONG    /* forward */
		},
	},

	// IH8950
	{
		.vendor_name = "IH8950",
		.codec = CIR_CODEC_NEC,
		.vendor_code = 0x0909,
		.wakeup_code = 0xdc,
		.key_codes = {
			[0xdc] = KEY_POWER,	     /* power down */
			[0x81] = KEY_VOLUMEDOWN, /* vol- */
			[0x80] = KEY_VOLUMEUP,	 /* vol+ */
			[0x82] = KEY_HOME,		 /* home */
			[0xc5] = KEY_BACK,		 /* back */
			[0xca] = KEY_UP,		 /* up */
			[0x99] = KEY_LEFT,		 /* left */
			[0xce] = KEY_ENTER, 	 /* OK */
			[0xc1] = KEY_RIGHT, 	 /* right */
			[0xd2] = KEY_DOWN,		 /* down */
			[0x9c] = KEY_MUTE,
			[0x95] = KEY_PLAYPAUSE,
			[0x88] = KEY_MENU,
		},
	},

	// sunday
	{
		.vendor_name = "sunday",
		.codec = CIR_CODEC_NEC,
		.vendor_code = 0x02fd,
		.wakeup_code = 0x1a,
		.key_codes = {
			[0x1a] = KEY_POWER,	     /* power down */
			[0x16] = KEY_VOLUMEDOWN, /* vol- */
			[0x44] = KEY_VOLUMEUP,	 /* vol+ */
			[0x59] = KEY_HOME,		 /* home */
			[0x1b] = KEY_BACK,		 /* back */
			[0x06] = KEY_UP,		 /* up */
			[0x5d] = KEY_LEFT,		 /* left */
			[0x1e] = KEY_ENTER, 	 /* OK */
			[0x5c] = KEY_RIGHT, 	 /* right */
			[0x1f] = KEY_DOWN,		 /* down */
			[0x55] = KEY_PLAYPAUSE,
			[0x54] = KEY_REWIND,
			[0x17] = KEY_FASTFORWARD,
			[0x58] = KEY_AGAIN,      /* recent app */
		},
	},

	/* F1 - F12 */
	// KT-9211
	{
		.vendor_name = "KT-9211",
		.codec = CIR_CODEC_NEC,
		.vendor_code = 0x02fd,
		.wakeup_code = 0x57,
		.key_codes = {
			[0x57] = KEY_POWER,      /* power down */
			[0x56] = KEY_VOLUMEDOWN, /* volume- */
			[0x15] = KEY_MUTE,       /* mute */ 
			[0x14] = KEY_VOLUMEUP,   /* volume+ */
			[0x0d] = KEY_UP,         /* up */
			[0x4e] = KEY_LEFT,       /* left */
			[0x19] = KEY_ENTER,      /* OK */
			[0x0c] = KEY_RIGHT,      /* right */
			[0x4f] = KEY_DOWN,       /* down */
			[0x53] = KEY_HOME,       /* home */ 
			[0x09] = KEY_F5,		 /* my photo in default */ 
			[0x11] = KEY_F12,        /* setting apk in default */ 
			[0x47] = KEY_F3,         /* My Music in default */
			[0x10] = KEY_BACK,       /* Back */ 
			[0x08] = KEY_F4,		 /* my video in default */ 
			[0x17] = KEY_F1,         /* web browser in default  */ 

			//Following items are configured manually.
			[0x05] = KEY_F10,       /* file browser in default  */ 
			[0x04] = KEY_F2,        /* camera in default */ 
			[0x4b] = KEY_F11,       /* calendar in default */ 
			[0x16] = KEY_SCREEN,    /* calculator in default */ 
			[0x18] = KEY_F9,        /* recorder in default */ 
		},
	},

	// KT-8830
	{
		.vendor_name = "KT-8830",
		.codec = CIR_CODEC_NEC,
		.vendor_code = 0x866b,
		.wakeup_code = 0x1c,
		.key_codes = {
			[0x1c] = KEY_POWER,
			[0x14] = KEY_MUTE,
			[0x09] = KEY_1,
			[0x1d] = KEY_2,
			[0x1f] = KEY_3,
			[0x0d] = KEY_4,
			[0x19] = KEY_5,
			[0x1b] = KEY_6,
			[0x11] = KEY_7,
			[0x15] = KEY_8,
			[0x17] = KEY_9,
			[0x12] = KEY_0,
			[0x16] = KEY_VOLUMEDOWN,
			[0x04] = KEY_VOLUMEUP,
			[0x40] = KEY_HOME,
			[0x4c] = KEY_BACK,
			[0x00] = KEY_F12, // setup
			[0x13] = KEY_MENU,
			[0x03] = KEY_UP,
			[0x0e] = KEY_LEFT,
			[0x1a] = KEY_RIGHT,
			[0x02] = KEY_DOWN,
			[0x07] = KEY_ENTER,
			[0x47] = KEY_F4, // video
			[0x10] = KEY_F3, // music
			[0x0f] = KEY_FASTFORWARD,
			[0x43] = KEY_REWIND,
			[0x18] = KEY_F1, // browser
			[0x0b] = KEY_YEN,  // multi-functions
			[0x4e] = KEY_PLAYPAUSE,
		},
	},

	// Haier-OTT 
	{
		.vendor_name = "Haier-OTT",
		.codec = CIR_CODEC_NEC,
		.vendor_code = 0xb34c,
		.wakeup_code = 0xdc,
		.key_codes = {
			[0xdc] = KEY_POWER,
			[0x9c] = KEY_MUTE,
			[0xca] = KEY_UP,
			[0x99] = KEY_LEFT,
			[0xc1] = KEY_RIGHT,
			[0xd2] = KEY_DOWN,
			[0xce] = KEY_ENTER,
			[0x80] = KEY_VOLUMEDOWN,
			[0x81] = KEY_VOLUMEUP,
			[0xc5] = KEY_HOME,
			[0x95] = KEY_BACK,
			[0x88] = KEY_MENU,
			[0x82] = KEY_F12,        /* setting apk in default */ 
		},
	},

	// GPRC-11933
	{
		.vendor_name = "GPRC-11933",
		.codec = CIR_CODEC_NEC,
		.vendor_code = 0x4040,
		.wakeup_code = 0x4D,
		.key_codes = {
			[0x4D] = KEY_POWER,      /* power down */
            [0x53] = KEY_PLAYPAUSE,      
            [0x5B] = KEY_F3,         /* My Music in default */
            [0x57] = KEY_F1,         /* web browser in default */ 
            [0x54] = KEY_F10,        /* file browser in default */ 
            
			[0x17] = KEY_VOLUMEDOWN, /* volume- */
			[0x43] = KEY_MUTE,       /* mute */ 
			[0x18] = KEY_VOLUMEUP,   /* volume+ */
			[0x1F] = KEY_PREVIOUSSONG, /* rewind */
			[0x1E] = KEY_NEXTSONG,    /* forward */
			
			[0x0B] = KEY_UP,         /* up */
			[0x10] = KEY_LEFT,       /* left */
			[0x0D] = KEY_ENTER,      /* OK */
			[0x11] = KEY_RIGHT,      /* right */
			[0x0E] = KEY_DOWN,       /* down */
			[0x1A] = KEY_HOME,       /* home */ 
			[0x42] = KEY_BACK,       /* Back */ 			

            [0x45] = KEY_F12,        /* setting apk in default */ 
            [0x47] = KEY_KATAKANA,        /* multi-functions */
			[0x01] = KEY_1,
			[0x02] = KEY_2,
			[0x03] = KEY_3,
			[0x04] = KEY_4,
			[0x05] = KEY_5,
			[0x06] = KEY_6,
			[0x07] = KEY_7,
			[0x08] = KEY_8,
			[0x09] = KEY_9,
			[0x00] = KEY_0,		
			[0x44] = KEY_MENU,
			[0x0C] = KEY_BACKSPACE,
		},
	},

	// TS-Y118
	{
		.vendor_name = "TS-Y118",
		.codec = CIR_CODEC_NEC,
		.vendor_code = 0xb34c,
		.wakeup_code = 0xdc,
		.key_codes = {
			[0xdc] = KEY_POWER,
			[0x9c] = KEY_MUTE,
			[0x8d] = KEY_F12, // apk-settings
			[0x88] = KEY_HOME,
			[0xca] = KEY_UP,
			[0xd2] = KEY_DOWN,
			[0x99] = KEY_LEFT,
			[0xc1] = KEY_RIGHT,
			[0xce] = KEY_ENTER,
			[0x95] = KEY_PLAYPAUSE,
			[0xc5] = KEY_BACK,
			[0x80] = KEY_VOLUMEUP,
			[0x81] = KEY_VOLUMEDOWN,
			[0xdd] = KEY_PAGEUP,
			[0x8c] = KEY_PAGEDOWN,
			[0x92] = KEY_1,
			[0x93] = KEY_2,
			[0xcc] = KEY_3,
			[0x8e] = KEY_4,
			[0x8f] = KEY_5,
			[0xc8] = KEY_6,
			[0x8a] = KEY_7,
			[0x8b] = KEY_8,
			[0xc4] = KEY_9,
			[0x87] = KEY_0,
		},
	},

	// Hisense
	{
		.vendor_name = "Hisense",
		.codec = CIR_CODEC_NEC,
		.vendor_code = 0x00ff,
		.wakeup_code = 0x14,
		.key_codes = {
			[0x14] = KEY_POWER,
			[0x1c] = KEY_MUTE,
			[0x40] = KEY_PAGEUP,
			[0x44] = KEY_PAGEDOWN,
			[0x0b] = KEY_VOLUMEUP,
			[0x58] = KEY_VOLUMEDOWN,
			[0x01] = KEY_MENU,
			[0x03] = KEY_UP,
			[0x02] = KEY_DOWN,
			[0x0e] = KEY_LEFT,
			[0x1a] = KEY_RIGHT,
			[0x07] = KEY_ENTER,
			[0x48] = KEY_HOME,
			[0x5c] = KEY_BACK,
			[0x09] = KEY_1,
			[0x1d] = KEY_2,
			[0x1f] = KEY_3,
			[0x0d] = KEY_4,
			[0x19] = KEY_5,
			[0x1b] = KEY_6,
			[0x11] = KEY_7,
			[0x15] = KEY_8,
			[0x17] = KEY_9,
			[0x12] = KEY_0,
			[0x06] = KEY_DOT,
			[0x16] = KEY_DELETE,
			[0x0c] = KEY_ZOOMIN,
			[0x4c] = KEY_F11,
			[0x13] = KEY_YEN,
		},
	}, 

	// Mountain
	{
		.vendor_name = "TV BOX Mountain",
		.codec = CIR_CODEC_NEC,
		.vendor_code = 0x00df,
		.wakeup_code = 0x1c,
		.key_codes = {
			[0x1c] = KEY_POWER,      /* power down */
			[0x08] = KEY_MUTE,		 /* volume mute */
			[0x1a] = KEY_UP,         /* up */
			[0x47] = KEY_LEFT,       /* left */
			[0x06] = KEY_ENTER,      /* OK */
			[0x07] = KEY_RIGHT,      /* right */
			[0x48] = KEY_DOWN,       /* down */
			[0x4f] = KEY_VOLUMEDOWN, /* vol- */
			[0x4b] = KEY_VOLUMEUP,   /* vol+ */
			[0x0a] = KEY_BACK,       /* back */
			[0x03] = KEY_HOME,       /* home */
			[0x42] = KEY_KATAKANA,         /* TV */
			[0x55] = KEY_MENU,       /* menu */
		},
	},

    // GPRC-11933 10
	{
		.vendor_name = "GPRC-11933A",
		.codec = CIR_CODEC_NEC,
		.vendor_code = 0x00FF,
		.wakeup_code = 0x57,
		.key_codes = {
			[0x57] = KEY_POWER,      /* power down */
            [0x5B] = KEY_MUTE,       /* mute */ 

            [0x16] = KEY_F3,         /* My Musicin default */ 
            [0x5A] = KEY_F1,         /* web browser in default */
            [0x52] = KEY_PLAYPAUSE,
            [0x50] = KEY_KATAKANA,   /* cursor */

            [0x0F] = KEY_PREVIOUSSONG, /* rewind */
			[0x4C] = KEY_NEXTSONG,    /* forward */
            [0x58] = KEY_VOLUMEDOWN,        /* volume- */                  
            [0x1B] = KEY_VOLUMEUP,         /* volume+ */
                                 
            [0x4F] = KEY_F12,        /* setting apk in default */  
            [0x1A] = KEY_MENU,
            
			[0x43] = KEY_UP,         /* up */
			
			[0x06] = KEY_LEFT,       /* left */
			[0x02] = KEY_ENTER,      /* OK */
			[0x0E] = KEY_RIGHT,      /* right */
			
			[0x0A] = KEY_DOWN,       /* down */

            [0x4E] = KEY_HOME,       /* home */
            [0x4D] = KEY_BACK,       /* back */
            
			[0x10] = KEY_1,
			[0x11] = KEY_2,
			[0x12] = KEY_3,
			[0x13] = KEY_4,
			[0x14] = KEY_5,
			[0x15] = KEY_6,
			[0x17] = KEY_7,
			[0x18] = KEY_8,
			[0x19] = KEY_9,
			[0x1D] = KEY_0,	

            [0x1C] = KEY_F1,			
			[0x1E] = KEY_BACKSPACE,
		},
	},  
	
    //China Telecom White 11
    {
		.vendor_name = "TS-Y118A",
		.codec = CIR_CODEC_NEC,
		.vendor_code = 0xb34c,
		.wakeup_code = 0xdc,
		.key_codes = {
			[0xdc] = KEY_POWER,
                
            [0x98] =KEY_AUDIO,    
			[0x9c] = KEY_MUTE,
			
			[0x8d] = KEY_SETUP, 
			[0xd6] = KEY_LAST, 

            [0xcd] = KEY_RED,
			[0x91] = KEY_GREEN,
			[0x83] = KEY_YELLOW,
			[0xc3] = KEY_BLUE,

            [0x88] = KEY_HOMEPAGE,
			[0xca] = KEY_UP,
			[0x82] = KEY_HOME,
			
			[0x99] = KEY_LEFT,
			[0xce] = KEY_SELECT,
			[0xc1] = KEY_RIGHT,						
			
			[0x95] = KEY_PLAYPAUSE,
			[0xd2] = KEY_DOWN,
			[0xc5] = KEY_BACK,

            [0x80] = KEY_VOLUMEUP,
			[0x81] = KEY_VOLUMEDOWN,
			
			[0xdd] = KEY_PAGEUP,
			[0x8c] = KEY_PAGEDOWN,

            [0x85] = KEY_CHANNELUP,
			[0x86] = KEY_CHANNELDOWN,
			
			[0x92] = KEY_1,
			[0x93] = KEY_2,
			[0xcc] = KEY_3,
			[0x8e] = KEY_4,
			[0x8f] = KEY_5,
			[0xc8] = KEY_6,
			[0x8a] = KEY_7,
			[0x8b] = KEY_8,
			[0xc4] = KEY_9,
			[0x87] = KEY_0,
			
			[0xda] = KEY_NUMERIC_STAR,
			[0xd0] = KEY_NUMERIC_POUND,
		},
	},	

    //China Telecom Black 12
    {
		.vendor_name = "TS-Y118B",
		.codec = CIR_CODEC_NEC,
		.vendor_code = 0xb24d,
		.wakeup_code = 0xdc,
		.key_codes = {
			[0xdc] = KEY_POWER,
                
            [0x98] =KEY_AUDIO,    
			[0x9c] = KEY_MUTE,
			
			[0x8d] = KEY_SETUP, 
			[0xd6] = KEY_LAST, 

            [0xcd] = KEY_RED,
			[0x91] = KEY_GREEN,
			[0x83] = KEY_YELLOW,
			[0xc3] = KEY_BLUE,

            [0x88] = KEY_HOMEPAGE,
			[0xca] = KEY_UP,
			[0x82] = KEY_HOME,
			
			[0x99] = KEY_LEFT,
			[0xce] = KEY_SELECT,
			[0xc1] = KEY_RIGHT,						
			
			[0x95] = KEY_PLAYPAUSE,
			[0xd2] = KEY_DOWN,
			[0xc5] = KEY_BACK,

            [0x80] = KEY_VOLUMEUP,
			[0x81] = KEY_VOLUMEDOWN,
			
			[0xdd] = KEY_PAGEUP,
			[0x8c] = KEY_PAGEDOWN,

            [0x85] = KEY_CHANNELUP,
			[0x86] = KEY_CHANNELDOWN,
			
			[0x92] = KEY_1,
			[0x93] = KEY_2,
			[0xcc] = KEY_3,
			[0x8e] = KEY_4,
			[0x8f] = KEY_5,
			[0xc8] = KEY_6,
			[0x8a] = KEY_7,
			[0x8b] = KEY_8,
			[0xc4] = KEY_9,
			[0x87] = KEY_0,
			
			[0xda] = KEY_NUMERIC_STAR,
			[0xd0] = KEY_NUMERIC_POUND,		
		},
	},	       
	
	// Jensen 13
	{
		.vendor_name = "Jensen",
		.codec = CIR_CODEC_NEC,
		.vendor_code = 0x00ff,
		.wakeup_code = 0x46,
		.key_codes = {
			[0x08] = KEY_RESERVED,

			[0x5a] = KEY_VOLUMEUP,
			[0x4a] = KEY_VOLUMEDOWN,

			[0x19] = KEY_UP,
			[0x1c] = KEY_DOWN,
			[0x0c] = KEY_LEFT,
			[0x5e] = KEY_RIGHT,
			[0x18] = KEY_ENTER,

			[0x0d] = KEY_BACK,
			[0x45] = KEY_1,
			[0x46] = KEY_POWER,
			[0x47] = KEY_3,
			[0x44] = KEY_4,
			[0x40] = KEY_F1,
			[0x43] = KEY_6,
			[0x07] = KEY_7,
			[0x15] = KEY_BACK,
			[0x09] = KEY_9,
			[0x16] = KEY_0,
			
			[0x42] = KEY_PREVIOUSSONG, /* rewind */
			[0x52] = KEY_NEXTSONG,    /* forward */

		},
	},      
};

extern int wmt_getsyspara(char *varname, unsigned char *varval, int *varlen);
extern int wmt_setsyspara(char *varname, char *varval);

static int rmtctl_report_rel(struct rmtctl_rel *rel_dev,const unsigned int code )
{
	switch (code) {                    
	case KEY_UP:
		input_report_rel(rel_dev->input, REL_X, 0);
		input_report_rel(rel_dev->input, REL_Y, -REL_DELTA);
		input_sync(rel_dev->input);
		break;
	case KEY_DOWN:
		input_report_rel(rel_dev->input, REL_X, 0);
		input_report_rel(rel_dev->input, REL_Y, REL_DELTA);
		input_sync(rel_dev->input);
		break;
	case KEY_LEFT:
		input_report_rel(rel_dev->input, REL_X, -REL_DELTA);
		input_report_rel(rel_dev->input, REL_Y, 0);
		input_sync(rel_dev->input);
		break;
	case KEY_RIGHT:
		input_report_rel(rel_dev->input, REL_X, REL_DELTA);
		input_report_rel(rel_dev->input, REL_Y, 0);
		input_sync(rel_dev->input);
		break;
	default:
		break;
	}
	return 0;
}

static void rmtctl_report_event(unsigned int *code, int repeat)
{
	static unsigned int last_code = KEY_RESERVED;
	int ret;
	struct rmtctl_priv *priv = container_of(code, struct rmtctl_priv, scan_code);

	ret = del_timer(&priv->timer);

	if (!repeat) {
		// new code coming
		if (ret == 1) {
			// del_timer() of active timer returns 1 means that active timer has been stoped by new key,
			// so report last key up event
			if (RMTCTL_DEBUG)
				printk("[%d] up reported caused by new key[%d]\n", last_code, *code);

			if (priv->rel_dev.on) {
				switch (*code) {
				case KEY_UP:
				case KEY_DOWN:
				case KEY_LEFT:
				case KEY_RIGHT:
					del_timer(&priv->rel_dev.timer); //stop report mouse
					break;
				case KEY_ENTER:
					input_report_key(priv->rel_dev.input, BTN_LEFT, 0);
					input_sync(priv->rel_dev.input);
					del_timer(&priv->rel_dev.timer); //stop report mouse
					break;
				default:
					input_report_key(priv->idev, last_code, 0);
					input_sync(priv->idev);
					break;
				}
			}
			else {
				input_report_key(priv->idev, last_code, 0);
				input_sync(priv->idev);
			}

			if (*code == KEY_KATAKANA) {
				if (priv->rel_dev.on == 0) {
					input_report_rel(priv->rel_dev.input, REL_X, 1);
					input_report_rel(priv->rel_dev.input, REL_Y, 1);
					input_sync(priv->rel_dev.input);
					priv->rel_dev.on = 1;
				}
				else
					priv->rel_dev.on = 0;
			}

		}

		if (RMTCTL_DEBUG)
			printk("[%d] down\n", *code);
		
		// report key down event, mod_timer() to report key up event later for measure key repeat.

		if (priv->rel_dev.on) {
			switch (*code) {
			case KEY_UP:
			case KEY_DOWN:
			case KEY_LEFT:
			case KEY_RIGHT:
				rmtctl_report_rel(&priv->rel_dev,*code);
				break;
			case KEY_ENTER:
				input_report_key(priv->rel_dev.input, BTN_LEFT, 1);
				input_sync(priv->rel_dev.input);
				break;
			default:
				input_event(priv->idev, EV_KEY, *code, 1);
				input_sync(priv->idev);
				break;
			}
		}
		else {
			input_event(priv->idev, EV_KEY, *code, 1);
			input_sync(priv->idev);
		}

        if(priv->led.enable){
            priv->led.on = 0;
            gpio_direction_output(priv->led.gpio, !priv->led.active);
            mod_timer(&priv->led.timer, jiffies+HZ/20);
        }
        
		priv->timer.data = (unsigned long)code;
		mod_timer(&priv->timer, jiffies + 400*HZ/1000);
	}
	else {
		// report key up event after report repeat event according to usb-keyboard standard
		priv->timer.data = (unsigned long)code;
		mod_timer(&priv->timer, jiffies + 400*HZ/1000);
		
		// detect 'repeat' flag, report repeat event
		if (*code != KEY_POWER && *code != KEY_END) {
			if (RMTCTL_DEBUG)
				printk("[%d] repeat\n", *code);

			if (priv->rel_dev.on) {
				switch (*code) {
				case KEY_UP:
				case KEY_DOWN:
				case KEY_LEFT:
				case KEY_RIGHT:
				case KEY_ENTER:
					priv->rel_dev.code = *code;
					mod_timer(&priv->rel_dev.timer, jiffies+HZ/100); //ready to repeat report mouse event
					break;
				default:
					input_event(priv->idev, EV_KEY, *code, 2);
					input_sync(priv->idev);
					break;
				}
			}
			else {
				input_event(priv->idev, EV_KEY, *code, 2);
				input_sync(priv->idev);
			}
		}
	}

	last_code = *code;
}

/** no hw repeat detect, so we measure repeat timeout event by timer */
static void rmtctl_report_event_without_hwrepeat(unsigned int *code)
{
	static unsigned int last_code = KEY_RESERVED;
	static unsigned long last_jiffies = 0;
	int repeat = (jiffies_to_msecs(jiffies - last_jiffies) <250 && *code==last_code) ? 1 : 0;

	last_code = *code;
	last_jiffies = jiffies;

	rmtctl_report_event(code, repeat);
}

static void rmtctl_timer_handler(unsigned long data)
{
	struct rmtctl_priv *priv = container_of((void *)data, struct rmtctl_priv, scan_code);
	unsigned int code = *(unsigned int *)data;
	
	// report key up event
	if (RMTCTL_DEBUG)
		printk("[%d] up reported by timer\n", code);

	if (priv->rel_dev.on) {
		switch (code) {
		case KEY_UP:
		case KEY_DOWN:
		case KEY_LEFT:
		case KEY_RIGHT:
			del_timer(&priv->rel_dev.timer);
			break;
		case KEY_ENTER:
			input_report_key(priv->rel_dev.input, BTN_LEFT, 0);
			input_sync(priv->rel_dev.input);
			break;
		default:
			input_report_key(priv->idev, code, 0);
    		input_sync(priv->idev);
			break;
		}
	}
	else {
		input_report_key(priv->idev, code, 0);
		input_sync(priv->idev);
	}

	if (code == KEY_KATAKANA) {
		if (priv->rel_dev.on == 0) {
			input_report_rel(priv->rel_dev.input, REL_X, 1);
			input_report_rel(priv->rel_dev.input, REL_Y, 1);
			input_sync(priv->rel_dev.input);
			priv->rel_dev.on = 1;
		} 
		else
			priv->rel_dev.on = 0;
	}

    if(priv->led.enable){
        del_timer_sync(&priv->led.timer);
        gpio_direction_output(priv->led.gpio,priv->led.active);
    }
}

static irqreturn_t rmtctl_interrupt(int irq, void *dev_id)
{
	unsigned int status, ir_data, vendor, repeat;
	unsigned char *key;
	int i;
	struct rmtctl_priv *priv = (struct rmtctl_priv *)dev_id;

	/* get IR status. */
	status = REG32_VAL(IRSTS);

	/* check 'IR received data' flag. */
	if ((status & 0x1) == 0x0) {
		printk("IR IRQ was triggered without data received. (0x%x)\n",
			status);
		return IRQ_NONE;
	}

	/* read IR data. */
	ir_data = REG32_VAL(IRDATA(0)) ;
	key = (char *) &ir_data;

	/* clear INT status*/
	REG32_VAL(IRSTS)=0x1 ;

	if (RMTCTL_DEBUG){
		printk("ir_data = 0x%08x, status = 0x%x \n", ir_data, status);
	}
	/* get vendor ID. */
	vendor = (key[0] << 8) | (key[1]);

	/* check if key is valid. Key[3] is XORed t o key[2]. */
	if (key[2] & key[3]) {
		printk("Invalid IR key received. (0x%x, 0x%x)\n", key[2], key[3]);
		return IRQ_NONE;
	}
	
	/* keycode mapping. */
	priv->scan_code = key[2];
	if ( priv->vendor_index >= 0 ) {
		if (vendor == rmtctl_vendors[priv->vendor_index].vendor_code && 
				rmtctl_vendors[priv->vendor_index].codec == priv->codec) {
			priv->table_index = priv->vendor_index;
			priv->scan_code = rmtctl_vendors[priv->vendor_index].key_codes[key[2]];
		}
        else{
            if(vendor != priv->vendor_code){
          		for (i = 0; i < ARRAY_SIZE(rmtctl_vendors); i++) {
        			if (vendor == rmtctl_vendors[i].vendor_code && 
                        rmtctl_vendors[i].codec == priv->codec) {
                        priv->table_index = i;
        				priv->scan_code = rmtctl_vendors[i].key_codes[key[2]];
        				break;
        			}
        		}

                if(i==ARRAY_SIZE(rmtctl_vendors))
                    return IRQ_HANDLED;
            }
            else{
                priv->scan_code = rmtctl_vendors[priv->table_index].key_codes[key[2]];
            }
        }
	}
	else {
        if(vendor != priv->vendor_code){
    		for (i = 0; i < ARRAY_SIZE(rmtctl_vendors); i++) {
    			if (vendor == rmtctl_vendors[i].vendor_code && 
                    rmtctl_vendors[i].codec == priv->codec) {
                    priv->table_index = i;
    				priv->scan_code = rmtctl_vendors[i].key_codes[key[2]];
    				break;
    			}
    		}
            
            if(i==ARRAY_SIZE(rmtctl_vendors))
                    return IRQ_HANDLED;
        }
        else{
            priv->scan_code = rmtctl_vendors[priv->table_index].key_codes[key[2]];
        }
	}
    
    //switch to a new remote controller shoud reset cursor mode
    if (priv->vendor_code != vendor) 
        priv->rel_dev.on = 0;
	
	/* check 'IR code repeat' flag. */
	repeat = status & 0x10;
	
	if ((status & 0x2) || (priv->scan_code == KEY_RESERVED)) {
		/* ignore repeated or reserved keys. */
	}
	else if (priv->codec == CIR_CODEC_NEC) {
		rmtctl_report_event(&priv->scan_code, repeat);
 	}
	else {
		rmtctl_report_event_without_hwrepeat(&priv->scan_code);
	}

	if (priv->vendor_code != vendor) {
		priv->vendor_code = vendor;        
		/* save new vendor code to env
         *1.remote controller configed index is active, no need to save it's vendor code 
         *2.current vendor code is saved, no need to save again
		 */
		if(priv->table_index != priv->vendor_index && vendor != priv->saved_vcode){
            priv->saved_vcode = vendor;
		    schedule_delayed_work(&priv->delaywork, msecs_to_jiffies(20));
        }
	}

	return IRQ_HANDLED;
}

static void rmtctl_hw_suspend(unsigned long data)
{
	unsigned int wakeup_code;
	int i;
	struct rmtctl_priv *priv = (struct rmtctl_priv *)data;

	if (priv->vendor_index >= 0 && priv->vendor_index == priv->table_index) {
		i = priv->vendor_index;
	}
	else {
		for (i = 0; i < ARRAY_SIZE(rmtctl_vendors); i++)
			if (priv->vendor_code == rmtctl_vendors[i].vendor_code)
				break;

		if (i == ARRAY_SIZE(rmtctl_vendors))
			goto out;
	}
	
	wakeup_code = 
		((unsigned char)(~rmtctl_vendors[i].wakeup_code) << 24) | 
		((unsigned char)(rmtctl_vendors[i].wakeup_code) << 16 ) |
		((unsigned char)(rmtctl_vendors[i].vendor_code & 0xff) << 8) |
		((unsigned char)((rmtctl_vendors[i].vendor_code) >> 8) << 0 );
	REG32_VAL(WAKEUP_CMD1(0)) = wakeup_code;
	REG32_VAL(WAKEUP_CMD1(1)) = 0x0;
	REG32_VAL(WAKEUP_CMD1(2)) = 0x0;
	REG32_VAL(WAKEUP_CMD1(3)) = 0x0;
	REG32_VAL(WAKEUP_CMD1(4)) = 0x0;

out:
	REG32_VAL(WAKEUP_CTRL) = 0x101;
}

static void rmtctl_hw_init(unsigned long data)
{
	unsigned int st;
	int i, retries = 0;
	struct rmtctl_priv *priv = (struct rmtctl_priv *)data;

	/*setting shared pin*/
	GPIO_CTRL_GP62_WAKEUP_SUS_BYTE_VAL  &= 0xFFFD;
	PULL_EN_GP62_WAKEUP_SUS_BYTE_VAL 	|= 0x02;
	PULL_CTRL_GP62_WAKEUP_SUS_BYTE_VAL 	|= 0x02;

	/* turn off CIR SW reset. */
	REG32_VAL(IRSWRST) = 1;
	REG32_VAL(IRSWRST) = 0;

	for (i = 0; i < ARRAY_SIZE(rmtctl_params[priv->codec].param); i++)
		REG32_VAL(PARAMETER(i)) = rmtctl_params[priv->codec].param[i];

	if (priv->codec == CIR_CODEC_NEC || priv->codec == CIR_CODEC_TOSHIBA) {
		REG32_VAL(NEC_REPEAT_TIME_OUT_CTRL) = 0x1;
		REG32_VAL(NEC_REPEAT_TIME_OUT_COUNT) = rmtctl_params[priv->codec].repeat_timeout;
		REG32_VAL(IRCTL) = 0X100; //NEC repeat key
	}
	else
		REG32_VAL(IRCTL) = 0; //NEC repeat key

	REG32_VAL(IRCTL) |= (0x0<<16) |(0x1<<25);

	REG32_VAL(INT_MASK_CTRL) = 0x1;
	REG32_VAL(INT_MASK_COUNT) =50*1000000*1/3; //0x47868C0/4;//count for 1 sec 0x47868C0

	/*  IR_EN */
	REG32_VAL(IRCTL) |= 0x1;
    while(retries++ < 100 && !(REG32_VAL(IRCTL)&0x01)){
        REG32_VAL(IRCTL) |= 0x1;
        udelay(5);
    }

	/* read CIR status to clear IR interrupt. */
	st = REG32_VAL(IRSTS);
}

static void rmtctl_refresh_vendorcode(struct work_struct *work)
{
	unsigned char data[64];
	struct rmtctl_priv *priv = container_of(work, struct rmtctl_priv, delaywork.work);
	sprintf(data, "0x%x", priv->vendor_code);
	wmt_setsyspara(WMT_RMTCTL_VENDOR_ENV, data);
}

static void rel_handler(unsigned long data)
{
	struct rmtctl_rel *rel = (struct rmtctl_rel *)data;
    struct rmtctl_priv *priv = container_of(rel, struct rmtctl_priv, rel_dev);
    
	rmtctl_report_rel(&priv->rel_dev, rel->code);
	mod_timer(&rel->timer, jiffies+HZ/15); // report rate 15pps
}

static void rmtctl_rel_init(struct rmtctl_rel *rel)
{
	struct input_dev *input;

	input = input_allocate_device();
	input->name = "rmtctl-rel";

	/* register the device as mouse */
	set_bit(EV_REL, input->evbit);
	set_bit(REL_X, input->relbit);
	set_bit(REL_Y, input->relbit);

	/* register device's buttons and keys */
	set_bit(EV_KEY, input->evbit);
	set_bit(BTN_LEFT, input->keybit);
	set_bit(BTN_RIGHT, input->keybit);

	input_register_device(input);
	rel->input = input;

	init_timer(&rel->timer);
	rel->timer.data = (unsigned long)rel;
	rel->timer.function = rel_handler;
}

void led_handler(unsigned long data)
{
    struct rmtctl_led *led = (struct rmtctl_led*)data;

    gpio_direction_output(led->gpio, led->on++%2);
    mod_timer(&led->timer, jiffies+HZ/20);
    return;
}

static void rmtctl_led_init(struct rmtctl_led *led)
{    
	init_timer(&led->timer);
	led->timer.data = (unsigned long)led;
	led->timer.function = led_handler; 

    gpio_direction_output(led->gpio, led->active);

    return;
}

static int rmtctl_probe(struct platform_device *dev)
{
	int i,ivendor, ikeycode;
	char buf[64];
	int varlen = sizeof(buf);
	struct rmtctl_priv *priv;

	priv = kzalloc(sizeof(struct rmtctl_priv), GFP_KERNEL);
	if (priv == NULL)
		return -ENOMEM;
	platform_set_drvdata(dev, priv);

	// get codec type of cir device
	if (wmt_getsyspara("wmt.io.rmtctl", buf, &varlen) == 0) {
		sscanf(buf, "%d", &priv->codec);
		if (priv->codec != CIR_CODEC_NEC && priv->codec != CIR_CODEC_TOSHIBA){
			kfree(priv);
			return -EINVAL;
		}
	}
	else {
		kfree(priv);
		return -EINVAL;
	}

	// using assigned cir device?
	if (wmt_getsyspara("wmt.io.rmtctl.index", buf, &varlen) == 0) {
		sscanf(buf, "%d", &priv->vendor_index);
		if (priv->vendor_index >= ARRAY_SIZE(rmtctl_vendors) ||
			rmtctl_vendors[priv->vendor_index].codec != priv->codec)
			priv->vendor_index = -1;
        
        priv->table_index = priv->vendor_index;
	}
	else
		priv->vendor_index = -1;

	// get last vendor code saved in uboot environment
	if (wmt_getsyspara(WMT_RMTCTL_VENDOR_ENV, buf, &varlen) == 0){
		sscanf(buf, "0x%x", &priv->vendor_code);
        priv->saved_vcode = priv->vendor_code;
        for (i = 0; i < ARRAY_SIZE(rmtctl_vendors); i++) {
			if (priv->vendor_code == rmtctl_vendors[i].vendor_code && 
                rmtctl_vendors[i].codec == priv->codec) {
                priv->table_index = i;
				break;
			}
        }
	}else
		priv->vendor_code = 0xffff;

    if (wmt_getsyspara("wmt.io.rmtctl.led", buf, &varlen) == 0) {
		sscanf(buf, "%d:%d", &priv->led.gpio, &priv->led.active);        
        priv->led.enable = 1;        
	}
	
	/* register an input device. */
	if ((priv->idev = input_allocate_device()) == NULL)
		return -ENOMEM;

	set_bit(EV_KEY, priv->idev->evbit);

	for (ivendor = 0; ivendor < ARRAY_SIZE(rmtctl_vendors); ivendor++) {
		if (priv->codec == rmtctl_vendors[ivendor].codec) {
			for (ikeycode = 0; ikeycode < ARRAY_SIZE(rmtctl_vendors[ivendor].key_codes); ikeycode++) {
				if (rmtctl_vendors[ivendor].key_codes[ikeycode]) {
					set_bit(rmtctl_vendors[ivendor].key_codes[ikeycode], priv->idev->keybit);
				}
			}
		}
	}

	priv->idev->name = "rmtctl";
	priv->idev->phys = "rmtctl";
	input_register_device(priv->idev);
	rmtctl_rel_init(&priv->rel_dev);
    if(priv->led.enable){
        gpio_request(priv->led.gpio,"rmtctl-led");
        rmtctl_led_init(&priv->led);
    }
	INIT_DELAYED_WORK(&priv->delaywork, rmtctl_refresh_vendorcode);
    
	init_timer(&priv->timer);
	priv->timer.data = (unsigned long)priv->idev;
	priv->timer.function = rmtctl_timer_handler;

    /* Register an ISR */
	request_irq(IRQ_CIR, rmtctl_interrupt, IRQF_SHARED, "rmtctl", priv);

	/* Initial H/W */
	rmtctl_hw_init((unsigned long)priv);

	if (RMTCTL_DEBUG)
		printk("WonderMedia rmtctl driver v0.98 initialized: ok\n");

	return 0;
}

static int rmtctl_remove(struct platform_device *dev)
{
	struct rmtctl_priv *priv = platform_get_drvdata(dev);
	
	if (RMTCTL_DEBUG)
		printk("rmtctl_remove\n");

	del_timer_sync(&priv->timer);
	cancel_delayed_work_sync(&priv->delaywork);	
    del_timer_sync(&priv->rel_dev.timer);
    input_unregister_device(priv->rel_dev.input);
	input_free_device(priv->rel_dev.input);
    if(priv->led.enable){
        del_timer_sync(&priv->led.timer);
        gpio_direction_output(priv->led.gpio, priv->led.active);
        gpio_free(priv->led.gpio);
    }

	free_irq(IRQ_CIR, priv);
	input_unregister_device(priv->idev);
	input_free_device(priv->idev);

	kfree(priv);

	return 0;
}

#ifdef CONFIG_PM
static int rmtctl_suspend(struct platform_device *dev, pm_message_t state)
{
	struct rmtctl_priv *priv = platform_get_drvdata(dev);
	
	del_timer_sync(&priv->timer);
	cancel_delayed_work_sync(&priv->delaywork);
    del_timer_sync(&priv->rel_dev.timer);
    if(priv->led.enable){
        priv->led.on = 0;
        del_timer(&priv->led.timer);
        gpio_direction_output(priv->led.gpio, priv->led.active);
    }
	
	/* Nothing to suspend? */
	rmtctl_hw_init((unsigned long)priv);
	rmtctl_hw_suspend((unsigned long)priv);
	
	disable_irq(IRQ_CIR);

	/* Enable rmt wakeup */
	PMWE_VAL |= (1 << WKS_CIR);
	
	return 0;
}

static int rmtctl_resume(struct platform_device *dev)
{
	volatile  unsigned int regval;
	int i =0 ;
	struct rmtctl_priv *priv = platform_get_drvdata(dev);

	/* Initial H/W */
	REG32_VAL(WAKEUP_CTRL) &=~ BIT0;

	for (i=0;i<10;i++)
	{
		regval = REG32_VAL(WAKEUP_STS) ;

		if (regval & BIT0){
			REG32_VAL(WAKEUP_STS) |= BIT4;

		}else{
			break;
		}
		msleep_interruptible(5);
	}
	
	regval = REG32_VAL(WAKEUP_STS) ;
	if (regval & BIT0)
		printk("CIR resume NG  WAKEUP_STS 0x%08x \n",regval);

	rmtctl_hw_init((unsigned long)priv);
    if(priv->led.enable)
        rmtctl_led_init(&priv->led);
	enable_irq(IRQ_CIR);
	return 0;
}
#else
#define rmtctl_suspend NULL
#define rmtctl_resume NULL
#endif

static struct platform_driver  rmtctl_driver = {
	.driver.name = "wmt-rmtctl", 
	//.bus = &platform_bus_type,
	//.probe = rmtctl_probe,
	.remove = rmtctl_remove,
	.suspend = rmtctl_suspend,
	.resume = rmtctl_resume
};

static void rmtctl_release(struct device *dev)
{
	/* Nothing to release? */
}

static u64 rmtctl_dmamask = 0xffffffff;

static struct platform_device rmtctl_device = {
	.name = "wmt-rmtctl",
	.id = 0,
	.dev = { 
		.release = rmtctl_release,
		.dma_mask = &rmtctl_dmamask,
	},
	.num_resources = 0,
	.resource = NULL,
};

static int __init rmtctl_init(void)
{
	int ret;

	if (platform_device_register(&rmtctl_device))
		return -1;
	ret = platform_driver_probe(&rmtctl_driver, rmtctl_probe);

	return ret;
}

static void __exit rmtctl_exit(void)
{
	platform_driver_unregister(&rmtctl_driver);
	platform_device_unregister(&rmtctl_device);
}

module_init(rmtctl_init);
module_exit(rmtctl_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("WonderMedia Technologies, Inc.");
MODULE_DESCRIPTION("WMT [Remoter] driver");
