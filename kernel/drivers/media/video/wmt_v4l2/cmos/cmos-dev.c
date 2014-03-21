/*++ 
 * linux/drivers/media/video/wmt_v4l2/cmos/cmos-dev.c
 * WonderMedia v4l cmos device driver
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
#define CMOS_DEV_C

#include "../wmt-vid.h"
#include "cmos-dev.h"
#include "cmos-dev-ov.h"
#include "cmos-dev-gc.h"
#include "cmos-dev-hy.h"
#include "cmos-dev-siv.h"


#include <linux/module.h>
#include <linux/videodev2.h> // color format

//#define CMOS_OV_DEBUG    /* Flag to enable debug message */
//#define CMOS_OV_TRACE

#ifdef CMOS_OV_DEBUG
  #define DBG(fmt, args...)    PRINT("{%s} " fmt, __FUNCTION__ , ## args)
#else
  #define DBG(fmt, args...)
#endif


#ifdef CMOS_OV_TRACE
  #define TRACE(fmt, args...)      PRINT("{%s}:  " fmt, __FUNCTION__ , ## args)
#else
  #define TRACE(fmt, args...) 
#endif

#define DBG_ERR(fmt, args...)      PRINT("*E* {%s} " fmt, __FUNCTION__ , ## args)



typedef struct  {
	int (*init)(cmos_init_arg_t *);
	int (*exit)(void);
	int (*identify)(void);
	char dev_name[12];
	int dev_clk_invert;
	short dev_id;
	unsigned char  init_frame_delay_count;
} cmos_dev_ops_t;




cmos_dev_ops_t cmos_dev_ov7675_ops = {
	
	.init = cmos_init_ov7675,
	.identify = cmos_ov7675_identify,
	.exit = cmos_exit_ov7675,	
	.dev_name = "ov7675",
	.dev_clk_invert = 0,
	.dev_id = CMOS_OV7675_I2C_ADDR,
	.init_frame_delay_count = 4,
};
cmos_dev_ops_t cmos_dev_ov5640_ops = {
	
	.init = cmos_init_ov5640,
	.identify = cmos_ov5640_identify,
	.exit = cmos_exit_ov5640,	
	.dev_name = "ov5640",
	.dev_clk_invert = 0,
	.dev_id = CMOS_OV5640_I2C_ADDR,
	.init_frame_delay_count = 4,
};

cmos_dev_ops_t cmos_dev_gc0308_ops = {
	
	.init = cmos_init_gc0308,
	.identify = cmos_gc0308_identify,
	.exit = cmos_exit_gc0308,	
	.dev_name = "gc0308",
	.dev_clk_invert = 1,
	.dev_id = CMOS_GC0308_I2C_ADDR,
	.init_frame_delay_count = 4,
};

cmos_dev_ops_t cmos_dev_gc0307_ops = {
	
	.init = cmos_init_gc0307,
	.identify = cmos_gc0307_identify,
	.exit = cmos_exit_gc0307,
	.dev_name = "gc0307",
	.dev_clk_invert = 0,
	.dev_id = CMOS_GC0307_I2C_ADDR,
	.init_frame_delay_count = 4,
};

cmos_dev_ops_t cmos_dev_hy511_ops = {
	
	.init = cmos_init_hy511,
	.identify = cmos_hy511_identify,
	.exit = cmos_exit_hy511,
	.dev_name = "hy511",
	.dev_clk_invert = 0,
	.dev_id = CMOS_HY511_I2C_ADDR,
	.init_frame_delay_count = 4,
};
cmos_dev_ops_t cmos_dev_gc2035_ops = {
	
	.init = cmos_init_gc2035,
	.identify = cmos_gc2035_identify,
	.exit = cmos_exit_gc2035,
	.dev_name = "gc2035",
	.dev_clk_invert = 0,
	.dev_id = CMOS_GC2035_I2C_ADDR,
	.init_frame_delay_count = 4,
};

cmos_dev_ops_t cmos_dev_siv121d_ops = {
	
	.init = cmos_init_siv121d,
	.identify = cmos_siv121d_identify,
	.exit = cmos_exit_siv121d,
	.dev_name = "siv121d",
	.dev_clk_invert = 0,
	.dev_id = CMOS_SIV121D_I2C_ADDR,
	.init_frame_delay_count = 4,
};

#define CMOS_DEV_TOT_NUM 8

cmos_dev_ops_t *cmos_dev_array[CMOS_DEV_TOT_NUM] = 
{  
    &cmos_dev_ov7675_ops , &cmos_dev_gc0308_ops, &cmos_dev_gc0307_ops, 
    &cmos_dev_hy511_ops  , &cmos_dev_gc2035_ops,
    &cmos_dev_ov5640_ops , &cmos_dev_siv121d_ops

};

unsigned char cmos_v_flip = 0;
unsigned char cmos_h_flip = 0;
unsigned int  old_dev_num = 0xff;
cmos_dev_ops_t  *cmos_dev_ops[2];


int cmos_exit_device(int width, int height)
{
    return 0;
} /* End of cmos_exit_device() */

 short cmos_get_dev_id(int dev_num, char *dev_name)
{
	int i = 0;
	const short ret_fail = 0;
	
	for (i = 0 ; i< CMOS_DEV_TOT_NUM ;i++)
      	{      	
      		cmos_dev_ops[dev_num] = cmos_dev_array[i];
        	if (!strcmp(cmos_dev_ops[dev_num]->dev_name, dev_name)) {
      			printk("got dev_name %s dev_num %d cmos_dev_array %d\n", cmos_dev_ops[dev_num]->dev_name, dev_num,i);
      			return cmos_dev_ops[dev_num]->dev_id;
      		}      		
      	}
	printk("don't find any device \n");
	
	return ret_fail;
}


int cmos_get_dev_clk_invert(int dev_num)
{
	return cmos_dev_ops[dev_num]->dev_clk_invert;
}

unsigned char cmos_get_dev_init_frame_delay_count(int dev_num)
{
	return cmos_dev_ops[dev_num]->init_frame_delay_count;
}


int cmos_init_ext_device(int dev_num, cmos_init_arg_t  *init_arg)
{
    int ret =0;
    cmos_v_flip = init_arg->cmos_v_flip;
    cmos_h_flip = init_arg->cmos_h_flip;

    printk(">> cmos_init_ext_devices() dev_num %d\n",dev_num);

    if ( dev_num == old_dev_num)
        init_arg->is_full_init = 0;
    else
        init_arg->is_full_init = 1;

    if (cmos_dev_ops[dev_num]->identify() == 0){
        ret = cmos_dev_ops[dev_num]->init(init_arg);
        if(ret)
            old_dev_num = 0xff;
        else
            old_dev_num = dev_num;
    }else{
        printk("init cmos device NG\n");
        ret = -1;
    }
	

    return ret;
       

} /* End of cmos_exit_ov7670() */

void cmos_dev_resume(void){
    old_dev_num = 0xff;
}



/*------------------------------------------------------------------------------*/
/*--------------------End of Function Body -----------------------------------*/

#undef CMOS_DEV_C
