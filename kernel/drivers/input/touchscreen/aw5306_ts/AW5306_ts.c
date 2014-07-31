/* 
 * drivers/input/touchscreen/aw5306/aw5306.c
 *
 * FocalTech aw5306 TouchScreen driver. 
 *
 * Copyright (c) 2010  Focal tech Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 *
 *	note: only support mulititouch	Wenfs 2010-10-01
 */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/i2c.h>
#include <linux/input.h>
#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <mach/hardware.h>
#include <linux/platform_device.h>
#include <linux/suspend.h>
#include <linux/wait.h>
#include <asm/uaccess.h>
#include <linux/irq.h>
#include <linux/gpio.h>
#include <mach/wmt_iomux.h>
#include <linux/slab.h>
#include "AW5306_Drv.h"
#include "AW5306_userpara.h"
#include "irq_gpio.h"

#define CONFIG_AW5306_MULTITOUCH     (1)
#define DEV_AW5306	    "touch_aw5306"
#define TS_I2C_NAME	    "aw5306-ts"
#define AW5306_I2C_ADDR	    0x38
#define AW5306_I2C_BUS      0x01

//#define DEBUG_EN

#undef dbg
#ifdef DEBUG_EN
	#define dbg(fmt,args...)  printk("DBG:%s_%d:"fmt,__FUNCTION__,__LINE__,##args)
#else
	#define dbg(fmt,args...)  
#endif

#undef dbg_err
#define dbg_err(fmt,args...)  printk("ERR:%s_%d:"fmt,__FUNCTION__,__LINE__,##args)



struct ts_event {
	int	x[5];
	int	y[5];
	int	pressure;
	int touch_ID[5];
	int touch_point;
	int pre_point;
};

struct AW5306_ts_data {
    	const char *name;
	struct input_dev	*input_dev;
	struct ts_event		event;
	struct work_struct 	pen_event_work;
	struct workqueue_struct *ts_workqueue;
    	struct kobject *kobj;
#ifdef CONFIG_HAS_EARLYSUSPEND
	struct early_suspend	early_suspend;
#endif
	struct timer_list touch_timer;

    	int irq;
    	int irqgpio;
    	int rstgpio;

    	int reslx;
    	int resly;
    	int nt;
    	int nb;
    	int xch;
    	int ych;
    	int swap;
    	int dbg;
};


struct AW5306_ts_data *pContext=NULL;
static struct i2c_client *l_client=NULL;
static unsigned char suspend_flag=0; //0: sleep out; 1: sleep in
static short tp_idlecnt = 0;
static char tp_SlowMode = 0;
//static struct class *i2c_dev_class;

extern char AW5306_CLB(void);
extern void AW5306_CLB_GetCfg(void);
extern STRUCTCALI       AW_Cali;
extern AW5306_UCF   AWTPCfg;
extern STRUCTBASE		AW_Base;
extern short	Diff[NUM_TX][NUM_RX];
extern short	adbDiff[NUM_TX][NUM_RX];
extern short	AWDeltaData[32];

char	AW_CALI_FILENAME[50] = {0,};
char	AW_UCF_FILENAME[50] = {0,};

extern int wmt_setsyspara(char *varname, unsigned char *varval);
extern int wmt_getsyspara(char *varname, unsigned char *varval, int *varlen);


void __aeabi_unwind_cpp_pr0(void)
{
}

void __aeabi_unwind_cpp_pr1(void)
{
}


int AW_nvram_read(char *filename, char *buf, ssize_t len, int offset)
{	
    struct file *fd;
    //ssize_t ret;
    int retLen = -1;

    mm_segment_t old_fs = get_fs();
    set_fs(KERNEL_DS);

    fd = filp_open(filename, O_RDONLY, 0);
    
    if(IS_ERR(fd)) {
        printk("[AW5306][nvram_read] : failed to open!!\n");
        return -1;
    }

       do{
        if ((fd->f_op == NULL) || (fd->f_op->read == NULL))
    		{
            printk("[AW5306][nvram_read] : file can not be read!!\n");
            break;
    		} 
    		
        if (fd->f_pos != offset) {
            if (fd->f_op->llseek) {
        		    if(fd->f_op->llseek(fd, offset, 0) != offset) {
						printk("[AW5306][nvram_read] : failed to seek!!\n");
					    break;
        		    }
        	  } else {
        		    fd->f_pos = offset;
        	  }
        }    		
        
    		retLen = fd->f_op->read(fd,
    									  buf,
    									  len,
    									  &fd->f_pos);			
    		
    }while(false);
    
    filp_close(fd, NULL);
    
    set_fs(old_fs);
    

    return retLen;
}

int AW_nvram_write(char *filename, char *buf, ssize_t len, int offset)
{	
    struct file *fd;
    //ssize_t ret;
    int retLen = -1;

    mm_segment_t old_fs = get_fs();
    set_fs(KERNEL_DS);
    
    fd = filp_open(filename, O_WRONLY|O_CREAT, 0666);
    
    if(IS_ERR(fd)) {
        printk("[AW5306][nvram_write] : failed to open!!\n");
        return -1;
    }
        
    do{
        if ((fd->f_op == NULL) || (fd->f_op->write == NULL))
    		{
            printk("[AW5306][nvram_write] : file can not be write!!\n");
            break;
    		} /* End of if */
    		
        if (fd->f_pos != offset) {
            if (fd->f_op->llseek) {
        	    if(fd->f_op->llseek(fd, offset, 0) != offset) {
				    printk("[AW5306][nvram_write] : failed to seek!!\n");
                    break;
                }
            } else {
                fd->f_pos = offset;
            }
        }       		
        
        retLen = fd->f_op->write(fd,
                                 buf,
                                 len,
                                 &fd->f_pos);			
    		
    }while(false);
    
    filp_close(fd, NULL);
    
    set_fs(old_fs);
    
    return retLen;
}


int AW_I2C_WriteByte(u8 addr, u8 para)
{
	int ret;
	u8 buf[3];
	struct i2c_msg msg[] = {
		{
			.addr	= l_client->addr,
			.flags	= 0,
			.len	= 2,
			.buf	= buf,
		},
	};
	buf[0] = addr;
	buf[1] = para;
	ret = i2c_transfer(l_client->adapter, msg, 1);
	return ret;
}


unsigned char AW_I2C_ReadByte(u8 addr)
{
	int ret;
	u8 buf[2] = {0};
	struct i2c_msg msgs[] = {
		{
			.addr	= l_client->addr,
			.flags	= 0,
			.len	= 1,
			.buf	= buf,
		},
		{
			.addr	= l_client->addr,
			.flags	= I2C_M_RD,
			.len	= 1,
			.buf	= buf,
		},
	};
	buf[0] = addr;
	//msleep(1);
	ret = i2c_transfer(l_client->adapter, msgs, 2);
	return buf[0];
}

unsigned char AW_I2C_ReadXByte( unsigned char *buf, unsigned char addr, unsigned short len)
{
	int ret,i;
	u8 rdbuf[512] = {0};
	struct i2c_msg msgs[] = {
		{
			.addr	= l_client->addr,
			.flags	= 0,
			.len	= 1,
			.buf	= rdbuf,
		},
		{
			.addr	= l_client->addr,
			.flags	= I2C_M_RD,
			.len	= len,
			.buf	= rdbuf,
		},
	};
	rdbuf[0] = addr;
	//msleep(1);
	ret = i2c_transfer(l_client->adapter, msgs, 2);
	if (ret < 0)
		pr_err("msg %s i2c read error: %d\n", __func__, ret);
	for(i = 0; i < len; i++)
	{
		buf[i] = rdbuf[i];
	}
    	return ret;
}

unsigned char AW_I2C_WriteXByte( unsigned char *buf, unsigned char addr, unsigned short len)
{
	int ret,i;
	u8 wdbuf[512] = {0};

	struct i2c_msg msgs[] = {
		{
			.addr	= l_client->addr,
			.flags	= 0,
			.len	= len+1,
			.buf	= wdbuf,
		}
	};

	wdbuf[0] = addr;
	for(i = 0; i < len; i++)
	{
		wdbuf[i+1] = buf[i];
	}
	//msleep(1);
	ret = i2c_transfer(l_client->adapter, msgs, 1);
	if (ret < 0)
		pr_err("msg %s i2c read error: %d\n", __func__, ret);
    	return ret;
}


void AW_Sleep(unsigned int msec)
{
	msleep(msec);
}

static ssize_t AW5306_get_Cali(struct device* cd,struct device_attribute *attr, char* buf);
static ssize_t AW5306_set_Cali(struct device* cd,struct device_attribute *attr, const char *buf, size_t count);
static ssize_t AW5306_get_reg(struct device* cd,struct device_attribute *attr, char* buf);
static ssize_t AW5306_write_reg(struct device* cd,struct device_attribute *attr, const char *buf, size_t count);
static ssize_t AW5306_get_Base(struct device* cd,struct device_attribute *attr, char* buf);
static ssize_t AW5306_get_Diff(struct device* cd,struct device_attribute *attr, char* buf);
static ssize_t AW5306_get_adbBase(struct device* cd,struct device_attribute *attr, char* buf);
static ssize_t AW5306_get_adbDiff(struct device* cd,struct device_attribute *attr, char* buf);
static ssize_t AW5306_get_FreqScan(struct device* cd,struct device_attribute *attr, char* buf);
static ssize_t AW5306_Set_FreqScan(struct device* cd, struct device_attribute *attr,const char* buf, size_t len);
static ssize_t AW5306_GetUcf(struct device* cd,struct device_attribute *attr, char* buf);



static DEVICE_ATTR(cali,  S_IRUGO | S_IWUGO, AW5306_get_Cali,    AW5306_set_Cali);
static DEVICE_ATTR(readreg,  S_IRUGO | S_IWUGO, AW5306_get_reg,    AW5306_write_reg);
static DEVICE_ATTR(base,  S_IRUGO | S_IWUSR, AW5306_get_Base,    NULL);
static DEVICE_ATTR(diff, S_IRUGO | S_IWUSR, AW5306_get_Diff,    NULL);
static DEVICE_ATTR(adbbase,  S_IRUGO | S_IWUSR, AW5306_get_adbBase,    NULL);
static DEVICE_ATTR(adbdiff, S_IRUGO | S_IWUSR, AW5306_get_adbDiff,    NULL);
static DEVICE_ATTR(freqscan, S_IRUGO | S_IWUGO, AW5306_get_FreqScan,    AW5306_Set_FreqScan);
static DEVICE_ATTR(getucf, S_IRUGO | S_IWUSR, AW5306_GetUcf,    NULL);


static ssize_t AW5306_get_Cali(struct device* cd,struct device_attribute *attr, char* buf)
{
	unsigned char i,j;
	ssize_t len = 0;

	len += snprintf(buf+len, PAGE_SIZE-len,"AWINIC RELEASE CODE VER = %d\n", Release_Ver);
	
	len += snprintf(buf+len, PAGE_SIZE-len,"*****AW5306 Calibrate data*****\n");
	len += snprintf(buf+len, PAGE_SIZE-len,"TXOFFSET:");
	
	for(i=0;i<11;i++)
	{
		len += snprintf(buf+len, PAGE_SIZE-len, "0x%02X ", AW_Cali.TXOFFSET[i]);
	}
	
	len += snprintf(buf+len, PAGE_SIZE-len,  "\n");
	len += snprintf(buf+len, PAGE_SIZE-len,  "RXOFFSET:");

	for(i=0;i<6;i++)
	{
		len += snprintf(buf+len, PAGE_SIZE-len, "0x%02X ", AW_Cali.RXOFFSET[i]);
	}

	len += snprintf(buf+len, PAGE_SIZE-len,  "\n");
	len += snprintf(buf+len, PAGE_SIZE-len,  "TXCAC:");

	for(i=0;i<21;i++)
	{
		len += snprintf(buf+len, PAGE_SIZE-len, "0x%02X ", AW_Cali.TXCAC[i]);
	}

	len += snprintf(buf+len, PAGE_SIZE-len,  "\n");
	len += snprintf(buf+len, PAGE_SIZE-len,  "RXCAC:");

	for(i=0;i<12;i++)
	{
		len += snprintf(buf+len, PAGE_SIZE-len, "0x%02X ", AW_Cali.RXCAC[i]);
	}

	len += snprintf(buf+len, PAGE_SIZE-len,  "\n");
	len += snprintf(buf+len, PAGE_SIZE-len,  "TXGAIN:");

	for(i=0;i<21;i++)
	{
		len += snprintf(buf+len, PAGE_SIZE-len, "0x%02X ", AW_Cali.TXGAIN[i]);
	}

	len += snprintf(buf+len, PAGE_SIZE-len,  "\n");

	for(i=0;i<AWTPCfg.TX_LOCAL;i++)
	{
		for(j=0;j<AWTPCfg.RX_LOCAL;j++)
		{
			len += snprintf(buf+len, PAGE_SIZE-len, "%4d ", AW_Cali.SOFTOFFSET[i][j]);
		}
		len += snprintf(buf+len, PAGE_SIZE-len,  "\n");
	}
	return len;
	
}

static ssize_t AW5306_set_Cali(struct device* cd,struct device_attribute *attr, const char* buf, size_t count)
{
	struct AW5306_ts_data *data = i2c_get_clientdata(l_client);
	
	unsigned long on_off = simple_strtoul(buf, NULL, 10);

	if(on_off == 1)
	{
	#ifdef INTMODE
		wmt_disable_gpirq(data ->irqgpio);
		AW5306_Sleep();
		suspend_flag = 1;
		AW_Sleep(50);

		TP_Force_Calibration();

		AW5306_TP_Reinit();
		wmt_enable_gpirq(data->irqgpio);
		suspend_flag = 0;
		
	#else
		suspend_flag = 1;
		AW_Sleep(50);
		
		TP_Force_Calibration();
		
		AW5306_TP_Reinit();
		tp_idlecnt = 0;
		tp_SlowMode = 0;
		suspend_flag = 0;
		data->touch_timer.expires = jiffies + HZ/AWTPCfg.FAST_FRAME;
		add_timer(&data->touch_timer);
	#endif
	}

	return count;
}


static ssize_t AW5306_get_adbBase(struct device* cd,struct device_attribute *attr, char* buf)
{
	unsigned char i,j;
	ssize_t len = 0;

	len += snprintf(buf+len, PAGE_SIZE-len, "base: \n");
	for(i=0;i< AWTPCfg.TX_LOCAL;i++)
	{
		for(j=0;j<AWTPCfg.RX_LOCAL;j++)
		{
			len += snprintf(buf+len, PAGE_SIZE-len, "%4d, ",AW_Base.Base[i][j]+AW_Cali.SOFTOFFSET[i][j]);
		}
		len += snprintf(buf+len, PAGE_SIZE-len, "\n");
	}
	
	return len;
}

static ssize_t AW5306_get_Base(struct device* cd,struct device_attribute *attr, char* buf)
{
	unsigned char i,j;
	ssize_t len = 0;

	*(buf+len) = AWTPCfg.TX_LOCAL;
	len++;
	*(buf+len) = AWTPCfg.RX_LOCAL;
	len++;
	
	for(i=0;i< AWTPCfg.TX_LOCAL;i++)
	{
		for(j=0;j<AWTPCfg.RX_LOCAL;j++)
		{
			*(buf+len) = (char)(((AW_Base.Base[i][j]+AW_Cali.SOFTOFFSET[i][j]) & 0xFF00)>>8);
			len++;
			*(buf+len) = (char)((AW_Base.Base[i][j]+AW_Cali.SOFTOFFSET[i][j]) & 0x00FF);
			len++;
		}
	}
	return len;

}

static ssize_t AW5306_get_adbDiff(struct device* cd,struct device_attribute *attr, char* buf)
{
	unsigned char i,j;
	ssize_t len = 0;

	len += snprintf(buf+len, PAGE_SIZE-len, "Diff: \n");
	for(i=0;i< AWTPCfg.TX_LOCAL;i++)
	{
		for(j=0;j<AWTPCfg.RX_LOCAL;j++)
		{
			len += snprintf(buf+len, PAGE_SIZE-len, "%4d, ",adbDiff[i][j]);
		}
		len += snprintf(buf+len, PAGE_SIZE-len, "\n");
	}
	
	return len;
}

static ssize_t AW5306_get_Diff(struct device* cd,struct device_attribute *attr, char* buf)
{
	unsigned char i,j;
	ssize_t len = 0;

	*(buf+len) = AWTPCfg.TX_LOCAL;
	len++;
	*(buf+len) = AWTPCfg.RX_LOCAL;
	len++;
	
	for(i=0;i< AWTPCfg.TX_LOCAL;i++)
	{
		for(j=0;j<AWTPCfg.RX_LOCAL;j++)
		{
			*(buf+len) = (char)((adbDiff[i][j] & 0xFF00)>>8);
			len++;
			*(buf+len) = (char)(adbDiff[i][j] & 0x00FF);
			len++;
		}
	}
	return len;
}

static ssize_t AW5306_get_FreqScan(struct device* cd,struct device_attribute *attr, char* buf)
{
	unsigned char i;
	ssize_t len = 0;

	for(i=0;i< 32;i++)
	{
		//*(buf+len) = (char)((AWDeltaData[i] & 0xFF00)>>8);
		//len++;
		//*(buf+len) = (char)(AWDeltaData[i] & 0x00FF);
		//len++;
		len += snprintf(buf+len, PAGE_SIZE-len, "%4d, ",AWDeltaData[i]);
	}

	len += snprintf(buf+len, PAGE_SIZE-len,  "\n");
	return len;
}

static ssize_t AW5306_Set_FreqScan(struct device* cd, struct device_attribute *attr,
		       const char* buf, size_t len)
{
	struct AW5306_ts_data *data = i2c_get_clientdata(l_client);
	unsigned long Basefreq = simple_strtoul(buf, NULL, 10);

	if(Basefreq < 16)
	{
	#ifdef INTMODE
		wmt_disable_gpirq(data ->irqgpio);
		AW5306_Sleep();
		suspend_flag = 1;
		AW_Sleep(50);

		FreqScan(Basefreq);

		AW5306_TP_Reinit();
		wmt_enable_gpirq(data ->irqgpio);
		suspend_flag = 0;
	#else
		suspend_flag = 1;
		AW_Sleep(50);

		FreqScan(Basefreq);

		AW5306_TP_Reinit();
		tp_idlecnt = 0;
		tp_SlowMode = 0;
		suspend_flag = 0;
		data->touch_timer.expires = jiffies + HZ/AWTPCfg.FAST_FRAME;
		add_timer(&data->touch_timer);
	#endif
	}

	return len;
}

static ssize_t AW5306_get_reg(struct device* cd,struct device_attribute *attr, char* buf)
{
	struct AW5306_ts_data *data = i2c_get_clientdata(l_client);
	u8 reg_val[128];
	ssize_t len = 0;
	u8 i;

	if(suspend_flag != 1)
	{
#ifdef INTMODE
	wmt_disable_gpirq(data ->irqgpio);
		AW5306_Sleep();
		suspend_flag = 1;
		AW_Sleep(50);
			
	AW_I2C_ReadXByte(reg_val,0,127);

	AW5306_TP_Reinit();
	wmt_enable_gpirq(data->irqgpio);
	suspend_flag = 0;
#else
	suspend_flag = 1;
	
	AW_Sleep(50);
			
	AW_I2C_ReadXByte(reg_val,0,127);

	AW5306_TP_Reinit();
	tp_idlecnt = 0;
	tp_SlowMode = 0;
	suspend_flag = 0;
	data->touch_timer.expires = jiffies + HZ/AWTPCfg.FAST_FRAME;
	add_timer(&data->touch_timer);
#endif
	}
	else
	{
		AW_I2C_ReadXByte(reg_val,0,127);
	}
	for(i=0;i<0x7F;i++)
	{
		len += snprintf(buf+len, PAGE_SIZE-len, "reg%02X = 0x%02X, ", i,reg_val[i]);
	}

	return len;

}

static ssize_t AW5306_write_reg(struct device* cd,struct device_attribute *attr, const char *buf, size_t count)
{
	struct AW5306_ts_data *data = i2c_get_clientdata(l_client);
	int databuf[2];
		
	if(2 == sscanf(buf, "%d %d", &databuf[0], &databuf[1]))
	{ 
		if(suspend_flag != 1)
		{
		#ifdef INTMODE
			wmt_disable_gpirq(data ->irqgpio);
			AW5306_Sleep();
			suspend_flag = 1;
			AW_Sleep(50);

			AW_I2C_WriteByte((u8)databuf[0],(u8)databuf[1]);
			
			AW5306_TP_Reinit();
			//ctp_enable_irq();
			wmt_enable_gpirq(data->irqgpio);
			suspend_flag = 0;
		#else
			suspend_flag = 1;
			AW_Sleep(50);
			
			AW_I2C_WriteByte((u8)databuf[0],(u8)databuf[1]);

			AW5306_TP_Reinit();
			tp_idlecnt = 0;
			tp_SlowMode = 0;
			suspend_flag = 0;
			data->touch_timer.expires = jiffies + HZ/AWTPCfg.FAST_FRAME;
			add_timer(&data->touch_timer);
		#endif
		}
		else
		{
			AW_I2C_WriteByte((u8)databuf[0],(u8)databuf[1]);
		}
	}
	else
	{
		printk("invalid content: '%s', length = %d\n", buf, count);
	}
	return count; 
}

static ssize_t AW5306_GetUcf(struct device* cd,struct device_attribute *attr, char* buf)
{
	ssize_t len = 0;
	
	len += snprintf(buf+len, PAGE_SIZE-len,"*****AW5306 UCF DATA*****\n");
	len += snprintf(buf+len, PAGE_SIZE-len,"%d,%d,\n",AWTPCfg.TX_LOCAL,AWTPCfg.RX_LOCAL);
	len += snprintf(buf+len, PAGE_SIZE-len,"(%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d}\n",
		AWTPCfg.TX_ORDER[0],AWTPCfg.TX_ORDER[1],AWTPCfg.TX_ORDER[2],AWTPCfg.TX_ORDER[3],AWTPCfg.TX_ORDER[4],
		AWTPCfg.TX_ORDER[5],AWTPCfg.TX_ORDER[6],AWTPCfg.TX_ORDER[7],AWTPCfg.TX_ORDER[8],AWTPCfg.TX_ORDER[9],
		AWTPCfg.TX_ORDER[10],AWTPCfg.TX_ORDER[11],AWTPCfg.TX_ORDER[12],AWTPCfg.TX_ORDER[13],AWTPCfg.TX_ORDER[14],
		AWTPCfg.TX_ORDER[15],AWTPCfg.TX_ORDER[16],AWTPCfg.TX_ORDER[17],AWTPCfg.TX_ORDER[19],AWTPCfg.TX_ORDER[19],
		AWTPCfg.TX_ORDER[20]);
	len += snprintf(buf+len, PAGE_SIZE-len,"{%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d},\n",
					AWTPCfg.RX_ORDER[0],AWTPCfg.RX_ORDER[1],AWTPCfg.RX_ORDER[2],AWTPCfg.RX_ORDER[3],
					AWTPCfg.RX_ORDER[4],AWTPCfg.RX_ORDER[5],AWTPCfg.RX_ORDER[6],AWTPCfg.RX_ORDER[7],
					AWTPCfg.RX_ORDER[8],AWTPCfg.RX_ORDER[9],AWTPCfg.RX_ORDER[10],AWTPCfg.RX_ORDER[11]);
	len += snprintf(buf+len, PAGE_SIZE-len,"%d,%d,\n",AWTPCfg.RX_START,AWTPCfg.HAVE_KEY_LINE);
	len += snprintf(buf+len, PAGE_SIZE-len,"{%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d},\n",
		AWTPCfg.KeyLineValid[0],AWTPCfg.KeyLineValid[1],AWTPCfg.KeyLineValid[2],AWTPCfg.KeyLineValid[3],
		AWTPCfg.KeyLineValid[4],AWTPCfg.KeyLineValid[5],AWTPCfg.KeyLineValid[6],AWTPCfg.KeyLineValid[7],
		AWTPCfg.KeyLineValid[8],AWTPCfg.KeyLineValid[9],AWTPCfg.KeyLineValid[10],AWTPCfg.KeyLineValid[11]);
	len += snprintf(buf+len, PAGE_SIZE-len,"%d,%d,\n",AWTPCfg.MAPPING_MAX_X,AWTPCfg.MAPPING_MAX_Y);
	len += snprintf(buf+len, PAGE_SIZE-len,"%d,%d,\n",AWTPCfg.GainClbDeltaMax,AWTPCfg.GainClbDeltaMin);
	len += snprintf(buf+len, PAGE_SIZE-len,"%d,%d,\n",AWTPCfg.KeyLineDeltaMax,AWTPCfg.KeyLineDeltaMin);
	len += snprintf(buf+len, PAGE_SIZE-len,"%d,%d,\n",AWTPCfg.OffsetClbExpectedMax,AWTPCfg.OffsetClbExpectedMin);
	len += snprintf(buf+len, PAGE_SIZE-len,"%d,%d,\n",AWTPCfg.RawDataDeviation,AWTPCfg.CacMultiCoef);
	len += snprintf(buf+len, PAGE_SIZE-len,"%d,%d,\n",AWTPCfg.RawDataCheckMin,AWTPCfg.RawDataCheckMax);
	len += snprintf(buf+len, PAGE_SIZE-len,"%d,%d,%d,\n",AWTPCfg.FLYING_TH,AWTPCfg.MOVING_TH,AWTPCfg.MOVING_ACCELER);
	len += snprintf(buf+len, PAGE_SIZE-len,"%d,%d,\n",AWTPCfg.PEAK_TH,AWTPCfg.GROUP_TH);
	len += snprintf(buf+len, PAGE_SIZE-len,"%d,%d,%d,\n",AWTPCfg.BIGAREA_TH,AWTPCfg.BIGAREA_CNT,AWTPCfg.BIGAREA_FRESHCNT);
	len += snprintf(buf+len, PAGE_SIZE-len,"%d,%d,%d,\n",AWTPCfg.CACULATE_COEF,AWTPCfg.FIRST_CALI,AWTPCfg.RAWDATA_DUMP_SWITCH);
	len += snprintf(buf+len, PAGE_SIZE-len,"%d,%d,0x%x,\n",AWTPCfg.MULTI_SCANFREQ,AWTPCfg.BASE_FREQ,AWTPCfg.FREQ_OFFSET);
	len += snprintf(buf+len, PAGE_SIZE-len,"0x%x,0x%x,0x%x,\n",AWTPCfg.WAIT_TIME,AWTPCfg.CHAMP_CFG,AWTPCfg.POSLEVEL_TH);
	len += snprintf(buf+len, PAGE_SIZE-len,"%d,\n",AWTPCfg.ESD_PROTECT);
	len += snprintf(buf+len, PAGE_SIZE-len,"%d,%d,%d,%d,%d,\n",AWTPCfg.MARGIN_COMPENSATE,AWTPCfg.MARGIN_COMP_DATA_UP,
		AWTPCfg.MARGIN_COMP_DATA_DOWN,AWTPCfg.MARGIN_COMP_DATA_LEFT,AWTPCfg.MARGIN_COMP_DATA_RIGHT);
	len += snprintf(buf+len, PAGE_SIZE-len,"%d,%d,%d,%d,\n",AWTPCfg.POINT_RELEASEHOLD,AWTPCfg.MARGIN_RELEASEHOLD,
		AWTPCfg.POINT_PRESSHOLD,AWTPCfg.KEY_PRESSHOLD);
	len += snprintf(buf+len, PAGE_SIZE-len,"%d,%d,%d,\n",AWTPCfg.PEAK_ROW_COMPENSATE,AWTPCfg.PEAK_COL_COMPENSATE,
		AWTPCfg.PEAK_COMPENSATE_COEF);
	len += snprintf(buf+len, PAGE_SIZE-len,"%d,%d,\n",AWTPCfg.LCD_NOISE_PROCESS,AWTPCfg.LCD_NOISETH);
	len += snprintf(buf+len, PAGE_SIZE-len,"%d,%d,\n",AWTPCfg.FALSE_PEAK_PROCESS,AWTPCfg.FALSE_PEAK_TH);
	len += snprintf(buf+len, PAGE_SIZE-len,"%d,%d,\n",AWTPCfg.STABLE_DELTA_X,AWTPCfg.STABLE_DELTA_Y);
	len += snprintf(buf+len, PAGE_SIZE-len,"%d,%d,%d,\n",AWTPCfg.DEBUG_LEVEL,AWTPCfg.FAST_FRAME,AWTPCfg.SLOW_FRAME);
	len += snprintf(buf+len, PAGE_SIZE-len,"%d,%d,\n",AWTPCfg.GAIN_CLB_SEPERATE,AWTPCfg.MARGIN_PREFILTER);
	len += snprintf(buf+len, PAGE_SIZE-len,"%d,%d,\n",AWTPCfg.BIGAREA_HOLDPOINT,AWTPCfg.CHARGE_NOISE);
	len += snprintf(buf+len, PAGE_SIZE-len,"%d,%d,\n",AWTPCfg.FREQ_JUMP,AWTPCfg.PEAK_VALID_CHECK);
	len += snprintf(buf+len, PAGE_SIZE-len,"%d,%d,\n",AWTPCfg.WATER_REMOVE,AWTPCfg.INT_MODE);

	return len;

}


static int AW5306_create_sysfs(struct i2c_client *client)
{
	int err;
	struct device *dev = &(client->dev);

	//TS_DBG("%s", __func__);
	
	err = device_create_file(dev, &dev_attr_cali);
	err = device_create_file(dev, &dev_attr_readreg);
	err = device_create_file(dev, &dev_attr_base);
	err = device_create_file(dev, &dev_attr_diff);
	err = device_create_file(dev, &dev_attr_adbbase);
	err = device_create_file(dev, &dev_attr_adbdiff);
	err = device_create_file(dev, &dev_attr_freqscan);
	err = device_create_file(dev, &dev_attr_getucf);
	return err;
}

static void AW5306_ts_release(void)
{
	struct AW5306_ts_data *data = pContext;
#ifdef CONFIG_AW5306_MULTITOUCH	
	#ifdef TOUCH_KEY_SUPPORT
	if(1 == key_tp){
		if(key_val == 1){
			input_report_key(data->input_dev, KEY_MENU, 0);
			input_sync(data->input_dev);  
        }
        else if(key_val == 2){
			input_report_key(data->input_dev, KEY_BACK, 0);
			input_sync(data->input_dev);  
		//	printk("===KEY 2   upupupupupu===++=\n");     
        }
        else if(key_val == 3){
			input_report_key(data->input_dev, KEY_SEARCH, 0);
			input_sync(data->input_dev);  
		//	printk("===KEY 3   upupupupupu===++=\n");     
        }
        else if(key_val == 4){
			input_report_key(data->input_dev, KEY_HOMEPAGE, 0);
			input_sync(data->input_dev);  
		//	printk("===KEY 4   upupupupupu===++=\n");     
        }
        else if(key_val == 5){
			input_report_key(data->input_dev, KEY_VOLUMEDOWN, 0);
			input_sync(data->input_dev);  
		//	printk("===KEY 5   upupupupupu===++=\n");     
        }
        else if(key_val == 6){
			input_report_key(data->input_dev, KEY_VOLUMEUP, 0);
			input_sync(data->input_dev);  
		//	printk("===KEY 6   upupupupupu===++=\n");     
        }
//		input_report_key(data->input_dev, key_val, 0);
		//printk("Release Key = %d\n",key_val);		
		//printk("Release Keyi+++++++++++++++++++++++++++++\n");		
	} else{
		input_report_abs(data->input_dev, ABS_MT_TOUCH_MAJOR, 0);
	}
	#else
	input_report_abs(data->input_dev, ABS_MT_TOUCH_MAJOR, 0);
	#endif

#else
	input_report_abs(data->input_dev, ABS_PRESSURE, 0);
	input_report_key(data->input_dev, BTN_TOUCH, 0);
#endif
	
	input_mt_sync(data->input_dev);
	input_sync(data->input_dev);
	return;

}


static void Point_adjust(int *x, int *y)
{
	struct AW5306_ts_data *AW5306_ts = pContext;
	int temp;

	if (AW5306_ts->swap) {
		temp = *x;
		*x = *y;
		*y = temp;
	}
	if (AW5306_ts->xch) 
		*x = AW5306_ts->reslx - *x;
	if (AW5306_ts->ych) 
		*y = AW5306_ts->resly - *y;
}


static int AW5306_read_data(void)
{
	struct AW5306_ts_data *data = pContext;
	struct ts_event *event = &data->event;
	int Pevent;
    	int i = 0;
	
	AW5306_TouchProcess();
	
	//memset(event, 0, sizeof(struct ts_event));
	event->touch_point = AW5306_GetPointNum();

	for(i=0;i<event->touch_point;i++)
	{
		AW5306_GetPoint(&event->x[i],&event->y[i],&event->touch_ID[i],&Pevent,i);
		//swap(event->x[i], event->y[i]);
		Point_adjust(&event->x[i], &event->y[i]);
//		printk("key%d = %d,%d,%d \n",i,event->x[i],event->y[i],event->touch_ID[i] );
	}
    	
	if (event->touch_point == 0) 
	{
		if(tp_idlecnt <= AWTPCfg.FAST_FRAME*5)
		{
			tp_idlecnt++;
		}
		if(tp_idlecnt > AWTPCfg.FAST_FRAME*5)
		{
			tp_SlowMode = 1;
		}
		
		if (event->pre_point != 0)
		{
		    	AW5306_ts_release();
			event->pre_point = 0;
		}
		return 1; 
	}
	else
	{
		tp_SlowMode = 0;
		tp_idlecnt = 0;
		event->pre_point = event->touch_point; 
		event->pressure = 200;
		dbg("%s: 1:%d %d 2:%d %d \n", __func__,
		event->x[0], event->y[0], event->x[1], event->y[1]);
	
		return 0;
	}
}

static void AW5306_report_multitouch(void)
{
	struct AW5306_ts_data *data = pContext;
	struct ts_event *event = &data->event;

#ifdef TOUCH_KEY_SUPPORT
	if(1 == key_tp){
		return;
	}
#endif

	switch(event->touch_point) {
	case 5:
		input_report_abs(data->input_dev, ABS_MT_TRACKING_ID, event->touch_ID[4]);	
		//input_report_abs(data->input_dev, ABS_MT_TOUCH_MAJOR, event->pressure);
		input_report_abs(data->input_dev, ABS_MT_POSITION_X, event->x[4]);
		input_report_abs(data->input_dev, ABS_MT_POSITION_Y, event->y[4]);
		//input_report_abs(data->input_dev, ABS_MT_WIDTH_MAJOR, 1);
		input_mt_sync(data->input_dev);
	//	printk("=++==x5 = %d,y5 = %d ====\n",event->x[4],event->y[4]);
	case 4:
		input_report_abs(data->input_dev, ABS_MT_TRACKING_ID, event->touch_ID[3]);	
		//input_report_abs(data->input_dev, ABS_MT_TOUCH_MAJOR, event->pressure);
		input_report_abs(data->input_dev, ABS_MT_POSITION_X, event->x[3]);
		input_report_abs(data->input_dev, ABS_MT_POSITION_Y, event->y[3]);
		//input_report_abs(data->input_dev, ABS_MT_WIDTH_MAJOR, 1);
		input_mt_sync(data->input_dev);
	//	printk("===x4 = %d,y4 = %d ====\n",event->x[3],event->y[3]);
	case 3:
		input_report_abs(data->input_dev, ABS_MT_TRACKING_ID, event->touch_ID[2]);	
		//input_report_abs(data->input_dev, ABS_MT_TOUCH_MAJOR, event->pressure);
		input_report_abs(data->input_dev, ABS_MT_POSITION_X, event->x[2]);
		input_report_abs(data->input_dev, ABS_MT_POSITION_Y, event->y[2]);
		//input_report_abs(data->input_dev, ABS_MT_WIDTH_MAJOR, 1);
		input_mt_sync(data->input_dev);
	//	printk("===x3 = %d,y3 = %d ====\n",event->x[2],event->y[2]);
	case 2:
		input_report_abs(data->input_dev, ABS_MT_TRACKING_ID, event->touch_ID[1]);	
		//input_report_abs(data->input_dev, ABS_MT_TOUCH_MAJOR, event->pressure);
		input_report_abs(data->input_dev, ABS_MT_POSITION_X, event->x[1]);
		input_report_abs(data->input_dev, ABS_MT_POSITION_Y, event->y[1]);
		//input_report_abs(data->input_dev, ABS_MT_WIDTH_MAJOR, 1);
		input_mt_sync(data->input_dev);
	//	printk("===x2 = %d,y2 = %d ====\n",event->x[1],event->y[1]);
	case 1:
		input_report_abs(data->input_dev, ABS_MT_TRACKING_ID, event->touch_ID[0]);	
		//input_report_abs(data->input_dev, ABS_MT_TOUCH_MAJOR, event->pressure);
		input_report_abs(data->input_dev, ABS_MT_POSITION_X, event->x[0]);
		input_report_abs(data->input_dev, ABS_MT_POSITION_Y, event->y[0]);
		//input_report_abs(data->input_dev, ABS_MT_WIDTH_MAJOR, 1);
		input_mt_sync(data->input_dev);
	//	printk("===x1 = %d,y1 = %d ====\n",event->x[0],event->y[0]);
		break;
	default:
//		print_point_info("==touch_point default =\n");
		break;
	}
	
	input_sync(data->input_dev);
	dbg("%s: 1:%d %d 2:%d %d \n", __func__,
		event->x[0], event->y[0], event->x[1], event->y[1]);
	return;
}

#ifdef TOUCH_KEY_SUPPORT
static void AW5306_report_touchkey(void)
{
	struct AW5306_ts_data *data = pContext;
	struct ts_event *event = &data->event;
	//printk("x=%d===Y=%d\n",event->x[0],event->y[0]);

#ifdef TOUCH_KEY_FOR_ANGDA
	if((1==event->touch_point)&&(event->x1 > TOUCH_KEY_X_LIMIT)){
		key_tp = 1;
		if(event->x1 < 40){
			key_val = 1;
			input_report_key(data->input_dev, key_val, 1);
			input_sync(data->input_dev);  
		//	print_point_info("===KEY 1====\n");
		}else if(event->y1 < 90){
			key_val = 2;
			input_report_key(data->input_dev, key_val, 1);
			input_sync(data->input_dev);     
		//	print_point_info("===KEY 2 ====\n");
		}else{
			key_val = 3;
			input_report_key(data->input_dev, key_val, 1);
			input_sync(data->input_dev);     
		//	print_point_info("===KEY 3====\n");	
		}
	} else{
		key_tp = 0;
	}
#endif
#ifdef TOUCH_KEY_FOR_EVB13
	if((1==event->touch_point)&&((event->y[0] > 510)&&(event->y[0]<530)))
	{
		if(key_tp != 1)
		{
			input_report_abs(data->input_dev, ABS_MT_TOUCH_MAJOR, 0);
			input_sync(data->input_dev);
		}
		else
		{
			//printk("===KEY touch ++++++++++====++=\n");     
		
			if(event->x[0] < 90){
				key_val = 1;
				input_report_key(data->input_dev, KEY_MENU, 1);
				input_sync(data->input_dev);  
			//	printk("===KEY 1===++=\n");     
			}else if((event->x[0] < 230)&&(event->x[0]>185)){
				key_val = 2;
				input_report_key(data->input_dev, KEY_BACK, 1);
				input_sync(data->input_dev);     
			//	printk("===KEY 2 ====\n");
			}else if((event->x[0] < 355)&&(event->x[0]>305)){
				key_val = 3;
				input_report_key(data->input_dev, KEY_SEARCH, 1);
				input_sync(data->input_dev);     
			//	print_point_info("===KEY 3====\n");
			}else if ((event->x[0] < 497)&&(event->x[0]>445))	{
				key_val = 4;
				input_report_key(data->input_dev, KEY_HOMEPAGE, 1);
				input_sync(data->input_dev);     
			//	print_point_info("===KEY 4====\n");	
			}else if ((event->x[0] < 615)&&(event->x[0]>570))	{
				key_val = 5;
				input_report_key(data->input_dev, KEY_VOLUMEDOWN, 1);
				input_sync(data->input_dev);     
			//	print_point_info("===KEY 5====\n");	
			}else if ((event->x[0] < 750)&&(event->x[0]>705))	{
				key_val = 6;
				input_report_key(data->input_dev, KEY_VOLUMEUP, 1);
				input_sync(data->input_dev);     
			//	print_point_info("===KEY 6====\n");	
			}
		}
		key_tp = 1;
	}
	else
	{
		key_tp = 0;
	}
#endif

#ifdef TOUCH_KEY_LIGHT_SUPPORT
	AW5306_lighting();
#endif
	return;
}
#endif


static void AW5306_report_value(void)
{
	AW5306_report_multitouch();
#ifdef TOUCH_KEY_SUPPORT
	AW5306_report_touchkey();
#endif
	return;
}	/*end AW5306_report_value*/


#ifdef INTMODE
static void AW5306_ts_pen_irq_work(struct work_struct *work)
{
	int ret = -1;
    
	ret = AW5306_read_data();
	if (ret == 0) 
		AW5306_report_value();

    	wmt_enable_gpirq(pContext->irqgpio);

    	return;
}
#else
static void AW5306_ts_pen_irq_work(struct work_struct *work)
{
	int ret = -1;
	
	if(suspend_flag != 1)
	{
		ret = AW5306_read_data();
		if (ret == 0) {
			AW5306_report_value();
		}
	}
	else
	{
			AW5306_Sleep(); 
	}
}

#endif



static irqreturn_t aw5306_interrupt(int irq, void *dev)
{
    	struct AW5306_ts_data *AW5306_ts= dev;

//printk("I\n");
	if (wmt_is_tsint(AW5306_ts->irqgpio))
	{
		wmt_clr_int(AW5306_ts->irqgpio);
		if (wmt_is_tsirq_enable(AW5306_ts->irqgpio))
		{
			wmt_disable_gpirq(AW5306_ts->irqgpio);
			#ifdef CONFIG_HAS_EARLYSUSPEND
				if(!AW5306_ts->earlysus) queue_work(AW5306_ts->ts_workqueue , &AW5306_ts->pen_event_work);
			#else
    				queue_work(AW5306_ts->ts_workqueue , &AW5306_ts->pen_event_work);
			#endif

		}
		return IRQ_HANDLED;
	}
	return IRQ_NONE;
}
/*
static void aw5306_reset(struct AW5306_ts_data *aw5306)
{
    gpio_set_value(aw5306->rstgpio, 0);
    mdelay(5);
    gpio_set_value(aw5306->rstgpio, 1);
    mdelay(5);
    gpio_set_value(aw5306->rstgpio, 0);
    mdelay(5);

    return;
}
*/

void AW5306_tpd_polling(unsigned long data)
 {
	struct AW5306_ts_data *AW5306_ts = i2c_get_clientdata(l_client);
 	
#ifdef INTMODE 
 	if (!work_pending(&AW5306_ts->pen_event_work)) {
    queue_work(AW5306_ts->ts_workqueue, &AW5306_ts->pen_event_work);
    }
#else
    
 	if (!work_pending(&AW5306_ts->pen_event_work)) {
    queue_work(AW5306_ts->ts_workqueue, &AW5306_ts->pen_event_work);
    }
	if(suspend_flag != 1)
	{
	#ifdef AUTO_RUDUCEFRAME
		if(tp_SlowMode)
		{  	
			AW5306_ts->touch_timer.expires = jiffies + HZ/AWTPCfg.SLOW_FRAME;
		}
		else
		{
			AW5306_ts->touch_timer.expires = jiffies + HZ/AWTPCfg.FAST_FRAME;
		}
	#else
		AW5306_ts->touch_timer.expires = jiffies + HZ/AWTPCfg.FAST_FRAME;
	#endif
		add_timer(&AW5306_ts->touch_timer);
	}
#endif
 }

#ifdef CONFIG_HAS_EARLYSUSPEND
static void aw5306_early_suspend(struct early_suspend *handler)
{
#ifdef INTMODE
	if(suspend_flag != 1)
	{
		wmt_disable_gpirq(AW5306_ts->irqgpio);
		AW5306_Sleep();
		suspend_flag = 1;
	}
#else
	if(suspend_flag != 1)
	{
		printk("AW5306 SLEEP!!!");
		suspend_flag = 1;
	}  
#endif

    	return;
}

static void aw5306_late_resume(struct early_suspend *handler)
{
    	struct AW5306_ts_data *AW5306_ts= container_of(handler, struct AW5306_ts_data , early_suspend);
#ifdef INTMODE
	if(suspend_flag != 0)
	{
		gpio_direction_output(AW5306_ts->rstgpio, 0);
		AW5306_User_Cfg1();
		AW5306_TP_Reinit();
    		wmt_enable_gpirq(AW5306_ts->irqgpio);
		suspend_flag = 0;
	}
#else
	if(suspend_flag != 0)
	{
		gpio_direction_output(AW5306_ts->rstgpio, 0);
		AW5306_User_Cfg1();
		AW5306_TP_Reinit();
		tp_idlecnt = 0;
		tp_SlowMode = 0;
		suspend_flag = 0;
		printk("AW5306 WAKE UP!!!");
		AW5306_ts->touch_timer.expires = jiffies + HZ/AWTPCfg.FAST_FRAME;
		add_timer(&AW5306_ts->touch_timer);
	}
#endif

    	return;
}
#endif  //CONFIG_HAS_EARLYSUSPEND

#ifdef CONFIG_PM
static int aw5306_suspend(struct platform_device *pdev, pm_message_t state)
{
#ifdef INTMODE
	if(suspend_flag != 1)
	{
		wmt_disable_gpirq(pContext->irqgpio);
		AW5306_Sleep();
		suspend_flag = 1;
	}
#else
	if(suspend_flag != 1)
	{
		printk("AW5306 SLEEP!!!");
		suspend_flag = 1;
	}  
#endif
	return 0;

}

static int aw5306_resume(struct platform_device *pdev)
{
    	struct AW5306_ts_data *AW5306_ts= dev_get_drvdata(&pdev->dev);
	
#ifdef INTMODE
	if(suspend_flag != 0)
	{
		gpio_direction_output(AW5306_ts->rstgpio, 0);
		AW5306_User_Cfg1();
		AW5306_TP_Reinit();
		suspend_flag = 0;
		printk("AW5306 WAKE UP_intmode!!!");
    		wmt_enable_gpirq(AW5306_ts->irqgpio);
	}
#else
	if(suspend_flag != 0)
	{
		gpio_direction_output(AW5306_ts->rstgpio, 0);
		AW5306_User_Cfg1();
		AW5306_TP_Reinit();
		tp_idlecnt = 0;
		tp_SlowMode = 0;
		suspend_flag = 0;
		printk("AW5306 WAKE UP!!!");
		AW5306_ts->touch_timer.expires = jiffies + HZ/AWTPCfg.FAST_FRAME;
		add_timer(&AW5306_ts->touch_timer);
	}
#endif

	return 0;
}

#else
#define aw5306_suspend NULL
#define aw5306_resume NULL
#endif

static int aw5306_probe(struct platform_device *pdev)
{
	int err = 0;
	struct AW5306_ts_data *AW5306_ts = platform_get_drvdata( pdev);
    	u8 reg_value;

    	//aw5306_reset(AW5306_ts);

	reg_value = AW_I2C_ReadByte(0x01);
	if(reg_value != 0xA8)
	{
		//l_client->addr = 0x39;
		dbg_err("AW5306_ts_probe: CHIP ID NOT CORRECT\n");
		return -ENODEV;
	}
	
	i2c_set_clientdata(l_client, AW5306_ts);

	INIT_WORK(&AW5306_ts->pen_event_work, AW5306_ts_pen_irq_work);
    	AW5306_ts->ts_workqueue = create_singlethread_workqueue(AW5306_ts->name);
	if (!AW5306_ts->ts_workqueue ) {
		err = -ESRCH;
		goto exit_create_singlethread;
	}
    
	AW5306_ts->input_dev = input_allocate_device();
	if (!AW5306_ts->input_dev) {
		err = -ENOMEM;
		dbg_err("failed to allocate input device\n");
		goto exit_input_dev_alloc_failed;
	}
	
	AW5306_ts->input_dev->name = AW5306_ts->name;
	AW5306_ts->input_dev->evbit[0] = BIT_MASK(EV_SYN) | BIT_MASK(EV_KEY) | BIT_MASK(EV_ABS);
	set_bit(INPUT_PROP_DIRECT, AW5306_ts->input_dev->propbit);

	input_set_abs_params(AW5306_ts->input_dev,
			     ABS_MT_POSITION_X, 0, AW5306_ts->reslx, 0, 0);
	input_set_abs_params(AW5306_ts->input_dev,
			     ABS_MT_POSITION_Y, 0, AW5306_ts->resly, 0, 0);
    	input_set_abs_params(AW5306_ts->input_dev,
                 	     ABS_MT_TRACKING_ID, 0, 4, 0, 0);

	err = input_register_device(AW5306_ts->input_dev);
	if (err) {
		dbg_err("aw5306_ts_probe: failed to register input device.\n");
		goto exit_input_register_device_failed;
	}
        
#ifdef CONFIG_HAS_EARLYSUSPEND
	AW5306_ts->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
	AW5306_ts->early_suspend.suspend = aw5306_early_suspend;
	AW5306_ts->early_suspend.resume = aw5306_late_resume;
	register_early_suspend(&AW5306_ts->early_suspend);
#endif

  	AW5306_create_sysfs(l_client);
	memcpy(AW_CALI_FILENAME,"/data/tpcali",12);
	//memcpy(AW_UCF_FILENAME,"/data/AWTPucf",13);
	printk("ucf file: %s\n", AW_UCF_FILENAME);
  
  	AW5306_TP_Init();  

	AW5306_ts->touch_timer.function = AW5306_tpd_polling;
	AW5306_ts->touch_timer.data = 0;
	init_timer(&AW5306_ts->touch_timer);
	AW5306_ts->touch_timer.expires = jiffies + HZ*10;
	add_timer(&AW5306_ts->touch_timer);
	
#ifdef INTMODE

	if(request_irq(AW5306_ts->irq, aw5306_interrupt, IRQF_SHARED, AW5306_ts->name, AW5306_ts) < 0){
		dbg_err("Could not allocate irq for ts_aw5306 !\n");
		err = -1;
		goto exit_register_irq;
	}	
	
    	wmt_set_gpirq(AW5306_ts->irqgpio, IRQ_TYPE_EDGE_FALLING); 
    	wmt_enable_gpirq(AW5306_ts->irqgpio);

#endif
    
   


    	return 0;
    
exit_register_irq:
#ifdef CONFIG_HAS_EARLYSUSPEND
	unregister_early_suspend(&AW5306_ts->early_suspend);
#endif
exit_input_register_device_failed:
	input_free_device(AW5306_ts->input_dev);
exit_input_dev_alloc_failed:
//exit_create_group:
	cancel_work_sync(&AW5306_ts->pen_event_work);
	destroy_workqueue(AW5306_ts->ts_workqueue );  
exit_create_singlethread:
	return err;
}

static int aw5306_remove(struct platform_device *pdev)
{
    	struct AW5306_ts_data *AW5306_ts= platform_get_drvdata( pdev);

	del_timer(&AW5306_ts->touch_timer);
    

#ifdef INTMODE
    	wmt_disable_gpirq(AW5306_ts->irqgpio);
    	free_irq(AW5306_ts->irq, AW5306_ts);
#endif

    
#ifdef CONFIG_HAS_EARLYSUSPEND
    	unregister_early_suspend(&AW5306_ts->early_suspend);
#endif
	input_unregister_device(AW5306_ts->input_dev);
	input_free_device(AW5306_ts->input_dev);
    
    	cancel_work_sync(&AW5306_ts->pen_event_work);
    	flush_workqueue(AW5306_ts->ts_workqueue);
	destroy_workqueue(AW5306_ts->ts_workqueue);

	dbg("remove...\n");
	return 0;
}

static void aw5306_release(struct device *device)
{
    return;
}

static struct platform_device aw5306_device = {
	.name  	    = DEV_AW5306,
	.id       	= 0,
	.dev    	= {.release = aw5306_release},
};

static struct platform_driver aw5306_driver = {
	.driver = {
	           .name   	= DEV_AW5306,
		   .owner	= THIS_MODULE,
	 },
	.probe    = aw5306_probe,
	.remove   = aw5306_remove,
	.suspend  = aw5306_suspend,
	.resume   = aw5306_resume,
};

static int check_touch_env(struct AW5306_ts_data *AW5306_ts)
{
	int ret = 0;
	int len = 96;
	int Enable;
    	char retval[96] = {0};
	char ucfname[20] = {0};
	char *p=NULL, *s=NULL;

    // Get u-boot parameter
	ret = wmt_getsyspara("wmt.io.touch", retval, &len);
	if(ret){
		//printk("MST FT5x0x:Read wmt.io.touch Failed.\n");
		return -EIO;
	}
	sscanf(retval,"%d:",&Enable);
	//check touch enable
	if(Enable == 0){
		//printk("FT5x0x Touch Screen Is Disabled.\n");
		return -ENODEV;
	}
	
	p = strchr(retval,':');
	p++;
    	if(strncmp(p,"aw5306",6)) return -ENODEV;
	AW5306_ts->name = DEV_AW5306;
	s = strchr(p, ':');
	p = p + 7;
	if (s <= p)
		return -ENODEV;
	strncpy(ucfname, p, s-p);
	sprintf(AW_UCF_FILENAME, "/lib/firmware/%s", ucfname);
	
	s++;
	sscanf(s,"%d:%d:%d:%d:%d:%d:%d:%d", &AW5306_ts->irqgpio, &AW5306_ts->reslx, &AW5306_ts->resly, &AW5306_ts->rstgpio, &AW5306_ts->swap, &AW5306_ts->xch, &AW5306_ts->ych, &AW5306_ts->nt);

    	AW5306_ts->irq = IRQ_GPIO;

	printk("%s irqgpio=%d, reslx=%d, resly=%d, rstgpio=%d, swap=%d, xch=%d, ych=%d, nt=%d\n", AW5306_ts->name,  AW5306_ts->irqgpio, AW5306_ts->reslx, AW5306_ts->resly, AW5306_ts->rstgpio, AW5306_ts->swap, AW5306_ts->xch, AW5306_ts->ych, AW5306_ts->nt);
	return 0;
}

struct i2c_board_info ts_i2c_board_info = {
	.type          = TS_I2C_NAME,
	.flags         = 0x00,
	.addr          = AW5306_I2C_ADDR,
	.platform_data = NULL,
	.archdata      = NULL,
	.irq           = -1,
};

static int ts_i2c_register_device (void)
{
	struct i2c_board_info *ts_i2c_bi;
	struct i2c_adapter *adapter = NULL;
	//struct i2c_client *client   = NULL;

	//ts_i2c_board_info.addr =  AW5306_I2C_ADDR;
	ts_i2c_bi = &ts_i2c_board_info;
	adapter = i2c_get_adapter(AW5306_I2C_BUS);/*in bus 1*/

	if (NULL == adapter) {
		printk("can not get i2c adapter, client address error\n");
		return -1;
	}
	l_client = i2c_new_device(adapter, ts_i2c_bi);
	if (l_client == NULL) {
		printk("allocate i2c client failed\n");
		return -1;
	}
	i2c_put_adapter(adapter);
	return 0;
}

static void ts_i2c_unregister_device(void)
{
	if (l_client != NULL)
	{
		i2c_unregister_device(l_client);
		l_client = NULL;
	}
}

static int __init aw5306_init(void)
{
	int ret = -ENOMEM;
	struct AW5306_ts_data *AW5306_ts=NULL;

	if (ts_i2c_register_device()<0)
	{
		dbg("Error to run ts_i2c_register_device()!\n");
		return -1;
	}

	AW5306_ts = kzalloc(sizeof(struct AW5306_ts_data), GFP_KERNEL);
    	if(!AW5306_ts){
        	dbg_err("mem alloc failed.\n");
        	return -ENOMEM;
    	}

    	pContext = AW5306_ts;
	ret = check_touch_env(AW5306_ts);
    	if(ret < 0)
        	goto exit_free_mem;
	
	ret = gpio_request(AW5306_ts->irqgpio, "ts_irq");
	if (ret < 0) {
		printk("gpio(%d) touchscreen irq request fail\n", AW5306_ts->irqgpio);
		goto exit_free_mem;
	}
	//wmt_gpio_setpull(AW5306_ts->irqgpio, WMT_GPIO_PULL_UP);
	gpio_direction_input(AW5306_ts->irqgpio);

	ret = gpio_request(AW5306_ts->rstgpio, "ts_rst");
	if (ret < 0) {
		printk("gpio(%d) touchscreen reset request fail\n", AW5306_ts->rstgpio);
		goto exit_free_irqgpio;
	}
	gpio_direction_output(AW5306_ts->rstgpio, 0);


	ret = platform_device_register(&aw5306_device);
	if(ret){
		dbg_err("register platform drivver failed!\n");
		goto exit_free_gpio;
	}
    	platform_set_drvdata(&aw5306_device, AW5306_ts);
	
	ret = platform_driver_register(&aw5306_driver);
	if(ret){
		dbg_err("register platform device failed!\n");
		goto exit_unregister_pdev;
	}

/*
	i2c_dev_class = class_create(THIS_MODULE,"aw_i2c_dev");
	if (IS_ERR(i2c_dev_class)) {		
		ret = PTR_ERR(i2c_dev_class);		
		class_destroy(i2c_dev_class);	
	}
*/


	return ret;
	
exit_unregister_pdev:
	platform_device_unregister(&aw5306_device);
exit_free_gpio:
	gpio_free(AW5306_ts->rstgpio);
exit_free_irqgpio:
	gpio_free(AW5306_ts->irqgpio);
exit_free_mem:
    kfree(AW5306_ts);
    pContext = NULL;
	return ret;
}

static void aw5306_exit(void)
{
    if(!pContext) return;
    
	platform_driver_unregister(&aw5306_driver);
	platform_device_unregister(&aw5306_device);
	gpio_free(pContext->rstgpio);
	gpio_free(pContext->irqgpio);
    	kfree(pContext);
	ts_i2c_unregister_device();
	return;
}

late_initcall(aw5306_init);
module_exit(aw5306_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("FocalTech.Touch");
