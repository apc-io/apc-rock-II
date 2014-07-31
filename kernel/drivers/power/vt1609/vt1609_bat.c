/*
 * Copyright (c) 2008  WonderMedia Technologies, Inc.
 *
 * This program is free software: you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software Foundation,
 * either version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE.  See the GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * WonderMedia Technologies, Inc.
 * 10F, 529, Chung-Cheng Road, Hsin-Tien, Taipei 231, R.O.C.
 */

#include <linux/init.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/delay.h>
#include <mach/hardware.h>
#include <linux/platform_device.h>
#include <linux/version.h>
#include <linux/spi/spi.h>
#include <linux/i2c.h>

#include "vt1609_bat.h"

#undef pr_err
#define pr_err(fmt, args...) printk(KERN_ERR "## VT1603A/VT1609 BAT##: " fmt,  ##args)

#define DFLT_BAT_VAL_AVG           4

struct vt1603_fifo {
	int head;
	int full;
	int buf[VT1603_FIFO_LEN];
};

struct vt1603_bat_drvdata {
	struct delayed_work work;
	struct vt1603 *tdev;

	u16 bat_val;
	u16 bat_new;

	u8 alarm_threshold;
	int wakeup_src;
	int tch_enabled;
};

static int vt1603_set_reg8(struct vt1603_bat_drvdata *bat_drv, u8 reg, u8 val)
{
	int ret =0;

	if (bat_drv->tdev)
		ret = bat_drv->tdev->reg_write(bat_drv->tdev, reg, val);

	if(ret){
		pr_err("vt1603 battery write error, errno%d\n", ret);
		return ret;
	}

	return 0;
}

static u8 vt1603_get_reg8(struct vt1603_bat_drvdata *bat_drv, u8 reg)
{
	u8 val = 0;
	int ret = 0;

	if (bat_drv->tdev)
		ret = bat_drv->tdev->reg_read(bat_drv->tdev, reg, &val);

	if (ret < 0){
		pr_err("vt1603 battery read error, errno%d\n", ret);
		return 0;
	}

	return val;
}


static int vt1603_read8(struct vt1603_bat_drvdata *bat_drv, u8 reg,u8* data)
{
	int ret = 0;

	if (bat_drv->tdev)
		ret = bat_drv->tdev->reg_read(bat_drv->tdev, reg, data);

	if (ret) {
		pr_err("vt1603 battery read error, errno%d\n", ret);
		return ret;
	}

	return 0;
}

static void vt1603_setbits(struct vt1603_bat_drvdata *bat_drv, u8 reg, u8 mask)
{
	u8 tmp = vt1603_get_reg8(bat_drv, reg) | mask;
	vt1603_set_reg8(bat_drv, reg, tmp);
}

static void vt1603_clrbits(struct vt1603_bat_drvdata *bat_drv, u8 reg, u8 mask)
{
	u8 tmp = vt1603_get_reg8(bat_drv, reg) & (~mask);
	vt1603_set_reg8(bat_drv, reg, tmp);
}

#if 0
static void vt1603_reg_dump(struct vt1603_bat_drvdata *bat_drv, u8 addr, int len)
{
	u8 i;
	for (i = addr; i < addr + len; i += 2) {
		pr_debug("reg[%d]:0x%02X,  reg[%d]:0x%02X\n",
			 i, vt1603_get_reg8(bat_drv, i),
			 i + 1, vt1603_get_reg8(bat_drv, i + 1));
	}
}
#endif

static struct vt1603_bat_drvdata *vt1603_bat = NULL;

static int vt1603_get_bat_data(struct vt1603_bat_drvdata *bat_drv,int *data)
{
	int ret = 0;
	u8 data_l, data_h;

	ret |= vt1603_read8(bat_drv, VT1603_DATL_REG, &data_l);
	ret |= vt1603_read8(bat_drv, VT1603_DATH_REG, &data_h);

	*data = ADC_DATA(data_l, data_h);

	return ret;
}

static void vt1603_switch_to_bat_mode(struct vt1603_bat_drvdata *bat_drv)
{
	vt1603_set_reg8(bat_drv, VT1603_CR_REG, 0x00);
	vt1603_set_reg8(bat_drv, 0xc6, 0x00);
	vt1603_set_reg8(bat_drv, VT1603_AMCR_REG, BIT0);
}

static inline int vt1603_get_pen_state(struct vt1603_bat_drvdata *bat_drv)
{
	u8 state ;

	if(!bat_drv->tch_enabled)
		return TS_PENUP_STATE;

	state = vt1603_get_reg8(bat_drv, VT1603_INTS_REG);
	return (((state & BIT4) == 0) ? TS_PENDOWN_STATE : TS_PENUP_STATE);
}

static inline void vt1603_bat_pen_manual(struct vt1603_bat_drvdata *bat_drv)
{
	vt1603_setbits(bat_drv, VT1603_INTCR_REG, BIT7);
}

static void vt1603_bat_power_up(struct vt1603_bat_drvdata *bat_drv)
{
	if (vt1603_get_reg8(bat_drv, VT1603_PWC_REG) != 0x08)
		vt1603_set_reg8(bat_drv, VT1603_PWC_REG, 0x08);

	return ;
}

static int vt1603_bat_avg(int *data, int num)
{
	int i = 0;
	int avg = 0;

	if(num == 0)
		return 0;

	for (i = 0; i < num; i++)
		avg += data[i];

	return (avg / num);
}

static int vt1603_fifo_push(struct vt1603_fifo *Fifo, int Data)
{
	Fifo->buf[Fifo->head] = Data;
	Fifo->head++;
	if(Fifo->head >= VT1603_FIFO_LEN){
		Fifo->head = 0;
		Fifo->full = 1;
	}

	return 0;
}

static int vt1603_fifo_avg(struct vt1603_fifo Fifo, int *Data)
{
	int i=0;
	int Sum=0,Max=0,Min=0;

	if(!Fifo.full && !Fifo.head)//FIFO is empty
		return 0;

	if(!Fifo.full ){
		for(i=0; i<Fifo.head; i++)
			Sum += Fifo.buf[i];

		*Data = Sum/Fifo.head;
		return 0;
	}

	Max = Fifo.buf[0];
	Min = Fifo.buf[0];
	for(i=0; i<VT1603_FIFO_LEN; i++){
		Sum += Fifo.buf[i];

		if(Max < Fifo.buf[i])
			Max = Fifo.buf[i];

		if(Min > Fifo.buf[i])
			Min = Fifo.buf[i];
	}
	Sum -= Max;
	Sum -= Min;
	*Data = Sum/(VT1603_FIFO_LEN-2);

	return 0;

}

static struct vt1603_fifo Bat_buf;

static void vt1603_bat_update(void)
{
	int tmp = 0;
	int timeout, i = 0;
	int bat_arrary[DFLT_BAT_VAL_AVG]={0};
	struct vt1603_bat_drvdata *bat_drv = vt1603_bat;

	if (!bat_drv) {
		pr_err("bat_drv NULL\n");
		return;
	}

	if (unlikely(vt1603_get_pen_state(bat_drv) == TS_PENDOWN_STATE)) {
		pr_debug("vt1603 pendown when battery detect\n");
		goto out;
	}

	/* enable sar-adc power and clock        */
	vt1603_bat_power_up(bat_drv);
	/* enable pen down/up to avoid miss irq  */
	vt1603_bat_pen_manual(bat_drv);
	/* switch vt1603 to battery detect mode  */
	vt1603_switch_to_bat_mode(bat_drv);
	/* do conversion use battery manual mode */
	vt1603_setbits(bat_drv, VT1603_INTS_REG, BIT0);
	vt1603_set_reg8(bat_drv, VT1603_CR_REG, BIT4);

	for(i = 0; i < DFLT_BAT_VAL_AVG; i++){
		timeout = 2000;
		while(--timeout && (vt1603_get_reg8(bat_drv, VT1603_INTS_REG) & BIT0)==0)
			;

		if(timeout){
			if(vt1603_get_bat_data(bat_drv,&bat_arrary[i]) < 0) {
				pr_err("vt1603 get bat adc data Failed!\n");
			}
		}else {
			pr_err("wait adc end timeout ?!\n");
			goto out;
		}

		pr_debug("%s: #%d: battery new val: %d\n", __func__, i, bat_arrary[i]);
		vt1603_setbits(bat_drv, VT1603_INTS_REG, BIT0);
		vt1603_set_reg8(bat_drv, VT1603_CR_REG, BIT4);//start manual ADC mode
	}

	tmp = vt1603_bat_avg(bat_arrary,DFLT_BAT_VAL_AVG);
	bat_drv->bat_new = tmp;
	vt1603_fifo_push(&Bat_buf, tmp);
	vt1603_fifo_avg(Bat_buf, &tmp);
	bat_drv->bat_val = tmp;

	//printk(KERN_ERR"reported battery val:%d, new val:%d\n",tmp, bat_drv->bat_new);
out:
	vt1603_clrbits(bat_drv, VT1603_INTCR_REG, BIT7);
	vt1603_setbits(bat_drv, VT1603_INTS_REG, BIT0 | BIT3);
	vt1603_set_reg8(bat_drv, VT1603_CR_REG, BIT1);
}

static void vt1603_battery_poll(struct work_struct *work)
{
	struct vt1603_bat_drvdata *bat = vt1603_bat;
	const int poll_interval = 1;

	if (!bat) {
		pr_err("bat_drv NULL\n");
		return;
	}

	vt1603_bat_update();

	/* The timer does not have to be accurate. */
	set_timer_slack(&bat->work.timer, poll_interval * HZ / 4);
	schedule_delayed_work(&bat->work, poll_interval * HZ);
}

unsigned short vt1603_get_bat_info(int dcin_status)
{
	if (vt1603_bat == NULL)
		return 4444;

	if (dcin_status){
		Bat_buf.full = 0;
		Bat_buf.head = 0;
		vt1603_bat_update();
		return vt1603_bat->bat_new;
	} else
		return vt1603_bat->bat_val;

}
EXPORT_SYMBOL_GPL(vt1603_get_bat_info);

static int __devinit vt1603_bat_probe(struct platform_device *pdev)
{
	struct vt1603_bat_drvdata *bat;

	bat = kzalloc(sizeof(*bat), GFP_KERNEL);
	if (!bat) {
		pr_err("vt160x: alloc driver data failed\n");
		return -ENOMEM;
	}

	bat->alarm_threshold  = 0xc0;
	bat->wakeup_src = BA_WAKEUP_SRC_0;
	bat->tdev = dev_get_platdata(&pdev->dev);
	bat->bat_val  = 3500;

	vt1603_bat = bat;
	platform_set_drvdata(pdev, bat);

	Bat_buf.full = 0;
	Bat_buf.head = 0;

	INIT_DELAYED_WORK(&bat->work, vt1603_battery_poll);
	schedule_delayed_work(&bat->work, HZ);

	printk("VT160X SAR-ADC Battery Driver Installed!\n");
	return 0;
}

static int __devexit vt1603_bat_remove(struct platform_device *pdev)
{
	struct vt1603_bat_drvdata *bat = platform_get_drvdata(pdev);

	cancel_delayed_work_sync(&bat->work);
	kfree(bat);
	vt1603_bat = NULL;
	return 0;
}

static int vt1603_bat_suspend(struct platform_device *pdev, pm_message_t message)
{
	struct vt1603_bat_drvdata *bat = dev_get_drvdata(&pdev->dev);
	cancel_delayed_work_sync(&bat->work);
	return 0;
}

static int vt1603_bat_resume(struct platform_device *pdev)
{
	struct vt1603_bat_drvdata *bat = dev_get_drvdata(&pdev->dev);

	Bat_buf.full = 0;
	Bat_buf.head  = 0;
	schedule_delayed_work(&bat->work, 5 * HZ);
	return 0;
}

static struct platform_driver vt1603_bat_driver = {
	.driver    = {
		.name  = VT1603_BAT_DRIVER,
		.owner = THIS_MODULE,
	},
	.probe     	= vt1603_bat_probe,
	.remove    	= vt1603_bat_remove,
	.suspend   	= vt1603_bat_suspend,
	.resume   	= vt1603_bat_resume,
};

int vt1603_bat_init(void)
{
	return platform_driver_register(&vt1603_bat_driver);
}

void vt1603_bat_exit(void)
{
	platform_driver_unregister(&vt1603_bat_driver);
}

