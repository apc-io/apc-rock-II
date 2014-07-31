/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein
 * is confidential and proprietary to MediaTek Inc. and/or its licensors.
 * Without the prior written permission of MediaTek inc. and/or its licensors,
 * any reproduction, modification, use or disclosure of MediaTek Software,
 * and information contained herein, in whole or in part, shall be strictly prohibited.
 *
 * MediaTek Inc. (C) 2010. All rights reserved.
 *
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
 * RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER ON
 * AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
 * NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
 * SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
 * SUPPLIED WITH THE MEDIATEK SOFTWARE, AND RECEIVER AGREES TO LOOK ONLY TO SUCH
 * THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. RECEIVER EXPRESSLY ACKNOWLEDGES
 * THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO OWIFIAIN FROM ANY THIRD PARTY ALL PROPER LICENSES
 * CONTAINED IN MEDIATEK SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK
 * SOFTWARE RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND
 * CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
 * AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT ISSUE,
 * OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY RECEIVER TO
 * MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 *
 * The following software/firmware and/or related documentation ("MediaTek Software")
 * have been modified by MediaTek Inc. All revisions are subject to any receiver's
 * applicable license agreements with MediaTek Inc.
 */

/*
*  from kevin and simplicity it by rubbitxiao
*/

#include <linux/init.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/sched.h>
#include <asm/current.h>
#include <asm/uaccess.h>
#include <linux/fcntl.h>
#include <linux/poll.h>
#include <linux/time.h>
#include <linux/delay.h>
#include <linux/miscdevice.h>
#include <linux/gpio.h>
#include <mach/wmt_iomux.h>


MODULE_LICENSE("Dual BSD/GPL");

#define WIFI_POWER_DRIVER_NAME "wmtWifi"

extern void wifi_power_ctrl_comm(int open,int mdelay);
extern void force_remove_sdio2(void);
extern void wmt_detect_sdio2(void);

static struct semaphore wr_mtx;

enum LEVEL {
	LOW = 0,
	HIGH,
};

static int WIFI_open(struct inode *inode, struct file *file)
{
    printk("%s: major %d minor %d (pid %d)\n", __func__,
        imajor(inode),
        iminor(inode),
        current->pid
        );

    return 0;
}

static int WIFI_close(struct inode *inode, struct file *file)
{
    printk("%s: major %d minor %d (pid %d)\n", __func__,
        imajor(inode),
        iminor(inode),
        current->pid
        );

    return 0;
}

ssize_t WIFI_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos)
{
    int retval = -EIO;
    char local[4] = {0};
    static int opened = 0;

    down(&wr_mtx);

    if (count > 0) {

        if (0 == copy_from_user(local, buf, (count > 4) ? 4 : count)) {
            printk("WIFI_write %c\n", local[0]);
            if (local[0] == '0' && opened == 1) {
					force_remove_sdio2();          					
					msleep(100);
					//wifi_power_ctrl_comm(0,0);
					gpio_direction_output(WMT_PIN_GP1_GPIO14, HIGH);
					msleep(10);
					gpio_direction_output(WMT_PIN_GP62_SUSGPIO1,LOW);
                    printk("WMT turn off WIFI OK!\n");
                    opened = 0;
                    retval = count;
            }
            else if (local[0] == '1' && opened == 0) {//reset pin is reversed
					printk("rest / pmu_en - low\n");
					gpio_direction_output(WMT_PIN_GP1_GPIO14, HIGH);
					//wifi_power_ctrl_comm(0,00);
					gpio_direction_output(WMT_PIN_GP62_SUSGPIO1,LOW);
					msleep(100);
					printk("pmu_en high with 200ms delay\n");
					//wifi_power_ctrl_comm(1,100);
					gpio_direction_output(WMT_PIN_GP62_SUSGPIO1,HIGH);
					msleep(3);
					//printk("rst pin high, with 200ms delay\n");
					gpio_direction_output(WMT_PIN_GP1_GPIO14, LOW);
					msleep(12);			
					
					wmt_detect_sdio2();          					
					wmt_detect_sdio2();          					
					msleep(3000);
                    opened = 1;
                    retval = count;
                    printk("WMT turn on WIFI success!\n");
            }

        }
    }

    up(&wr_mtx);
    return (retval);
}


struct file_operations wifi_fops = {
    .open = WIFI_open,
    .release = WIFI_close,
    .write = WIFI_write,
};

static struct miscdevice wifi_power = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = WIFI_POWER_DRIVER_NAME,
	.fops = &wifi_fops,
};

static int WIFI_init(void)
{
	int ret;
	int ret1;
	sema_init(&wr_mtx, 1);
	ret = misc_register(&wifi_power);
	ret1 = gpio_request(WMT_PIN_GP1_GPIO14, "wifi reset pin");
	if(ret1 < 0) {
		printk("reques gpio:%x failed!!! for wifi\n",WMT_PIN_GP1_GPIO14);
		return -1;
	}else{
		printk("request gpio:%d for wifi success!!!\n",WMT_PIN_GP1_GPIO14);
	}
	//WMT_PIN_GP62_SUSGPIO1
	ret1 = gpio_request(WMT_PIN_GP62_SUSGPIO1, "wifi power pin");
	if(ret1 < 0) {
		printk("reques gpio:%x failed!!! for wifi\n",WMT_PIN_GP62_SUSGPIO1);
		return -1;
	}else{
		printk("request gpio:%d for wifi success!!!\n",WMT_PIN_GP62_SUSGPIO1);
	}
	gpio_direction_output(WMT_PIN_GP1_GPIO14, HIGH);
	gpio_direction_output(WMT_PIN_GP62_SUSGPIO1, LOW);	
	return ret;			
}

static void WIFI_exit(void)
{
	misc_deregister(&wifi_power);
	gpio_free(WMT_PIN_GP1_GPIO14);
	gpio_free(WMT_PIN_GP62_SUSGPIO1);
}

module_init(WIFI_init);
module_exit(WIFI_exit);

