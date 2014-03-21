/*++ 
 * linux/drivers/media/video/wmt_v4l2/wmt-vid.h
 * WonderMedia v4l video input device driver
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

#ifndef WMT_VID_H
#define WMT_VID_H

#include <mach/hardware.h>
#include "wmt-vidreg.h"
#include "../../../video/wmt/sw_i2c.h"

#ifdef __KERNEL__
    #define PRINT           printk
#else
    #define PRINT           printf
    #define KERN_ERR     
    #define KERN_WARNING
    #define KERN_INFO
    #define KERN_DEBUG
#endif

#define BIT0	0x00000001
#define BIT1	0x00000002
#define BIT2	0x00000004
#define BIT3	0x00000008
#define BIT4	0x00000010
#define BIT5	0x00000020
#define BIT6	0x00000040
#define BIT7	0x00000080
#define BIT8	0x00000100
#define BIT9	0x00000200
#define BIT10	0x00000400
#define BIT11	0x00000800
#define BIT12	0x00001000
#define BIT13	0x00002000
#define BIT14	0x00004000
#define BIT15	0x00008000
#define BIT16	0x00010000
#define BIT17	0x00020000
#define BIT18	0x00040000
#define BIT19	0x00080000
#define BIT20	0x00100000
#define BIT21	0x00200000
#define BIT22	0x00400000
#define BIT23	0x00800000
#define BIT24	0x01000000
#define BIT25	0x02000000
#define BIT26	0x04000000
#define BIT27	0x08000000
#define BIT28	0x10000000
#define BIT29	0x20000000
#define BIT30	0x40000000
#define BIT31	0x80000000

typedef enum{
	VID_MODE_CMOS,   /* CMOS sensor mode */
	VID_MODE_TVDEC   /* TV Decoder mode */
} vid_mode;

/*------------------------------------------------------------------------------
    Definitions of structures
------------------------------------------------------------------------------*/
#define VID_FB_M \
    unsigned int  y_addr;   \
    unsigned int  c_addr;   \
    unsigned int  id;       \
    unsigned int  done;     \
    unsigned int  is_busy
/* End of VD_IOCTL_CMD_M */

/* Following structure is used for all HW decoder as input arguments */
typedef struct {
	VID_FB_M;
} vid_fb_t;

typedef struct {
	struct i2c_adapter *adapter;
	struct i2c_client *client;
	char name[12];
	unsigned int  gpio_pwr_active_level;
	unsigned int  gpio_pwr_bitnum;
	unsigned int  gpio_pwr_reg_gpio_en;
	unsigned int  gpio_pwr_reg_gpio_od;
	unsigned int  gpio_pwr_reg_gpio_oc;
	short  i2c_addr;
} cmos_dev_t;

typedef struct {
	unsigned int en;
	unsigned int dummy;
	
	unsigned int dev_tot_num;
	unsigned int dev_id;
	unsigned int is_two_same_dev;
	cmos_dev_t   dev[2];
	
	unsigned int v_flip;//v_mirror
	unsigned int h_flip;//h_mirror
	unsigned int i2c_gpio_en;

	unsigned int i2c_gpio_scl_binum;
	unsigned int i2c_gpio_sda_binum;
	
	unsigned int reg_i2c_gpio_scl_gpio_in;
	unsigned int reg_i2c_gpio_scl_gpio_en;
	unsigned int reg_i2c_gpio_scl_gpio_od;
	unsigned int reg_i2c_gpio_scl_gpio_oc;
	unsigned int reg_i2c_gpio_scl_gpio_pe;
	unsigned int reg_i2c_gpio_scl_gpio_pe_bitnum;
	
	unsigned int reg_i2c_gpio_sda_gpio_in;
	unsigned int reg_i2c_gpio_sda_gpio_en;
	unsigned int reg_i2c_gpio_sda_gpio_od;
	unsigned int reg_i2c_gpio_sda_gpio_oc;
	unsigned int reg_i2c_gpio_sda_gpio_pe;
	unsigned int reg_i2c_gpio_sda_gpio_pe_bitnum;
	
	char isMIPI;
} cmos_uboot_env_t;

typedef enum {
	VID_NTSC,
	VID_PAL
} vid_tvsys_e;


int wmt_vid_open(vid_mode mode,cmos_uboot_env_t *cmos_uboot_env);
int wmt_vid_close(vid_mode mode);
int wmt_vid_set_mode(int width, int height, unsigned int pix_format);
int wmt_vid_set_addr(unsigned int y_addr, unsigned int c_addr);
  
int wmt_vid_set_cur_fb(vid_fb_t *fb);
vid_fb_t * wmt_vid_get_cur_fb(void);
int write_array_2_i2c(int slave_addr, char *array_addr, int array_size);

int wmt_vid_i2c_write(int chipId ,unsigned int index,char data);
int wmt_vid_i2c_read(int chipId ,unsigned int index) ;
int wmt_vid_i2c_read_page(int chipId ,unsigned int index,char *pdata,int len);
int wmt_vid_i2c_write_page(int chipId ,unsigned int index,char *pdata,int len);
int wmt_vid_i2c_write16addr(int chipId ,unsigned int index,unsigned int data);
int wmt_vid_i2c_read16addr(int chipId ,unsigned int index);
int wmt_vid_i2c_write16data(int chipId ,unsigned int index,unsigned int data);
int wmt_vid_i2c_read16data(int chipId ,unsigned int index);
int wmt_vid_i2c_init( int dev_num, short dev_id);
int wmt_vid_i2c_release(void);
int wmt_vid_i2c_set_same_dev(void);

#endif

