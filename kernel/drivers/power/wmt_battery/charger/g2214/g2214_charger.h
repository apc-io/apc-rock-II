/*++
	Copyright (c) 2008  WonderMedia Technologies, Inc.

	This program is free software: you can redistribute it and/or modify it under the
	terms of the GNU General Public License as published by the Free Software Foundation,
	either version 2 of the License, or (at your option) any later version.

	This program is distributed in the hope that it will be useful, but WITHOUT
	ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
	PARTICULAR PURPOSE.  See the GNU General Public License for more details.
	You should have received a copy of the GNU General Public License along with
	this program.  If not, see <http://www.gnu.org/licenses/>.

	WonderMedia Technologies, Inc.
	10F, 529, Chung-Cheng Road, Hsin-Tien, Taipei 231, R.O.C.
--*/

#ifndef __G2144_CHARGER_H__
#define __G2144_CHARGER_H__

#include <linux/types.h>
#include <linux/power_supply.h>

#define DEV_NAME                   "g2144_charger"
#define GPIO_INTERRUPT_NUM  22
#define UDEC_INTERRUPT_NUM  26
#define USB_BASE_ADD 0xD8007800+WMT_MMAP_OFFSET
#define GPIO_BASE_ADD 0xD8110000+WMT_MMAP_OFFSET
#define PC_MODE 1
#define g2144_dump 0
#define ADP_MODE 3
#define DCIN 0
#define USB_ADP 1
#define USB_PC 2
#define USB_OTG 3
#define CHG_CUR_LEV_HIGH (1800*15/100)
#define CHG_CUR_LEV_LOW  (500*15/100)
//#define G2214_DUMP

/* G2144 Register address */
#define REG_A0     0x00
#define REG_A1     0x01
#define REG_A2     0x02
#define REG_A3     0x03
#define REG_A4     0x04
#define REG_A5     0x05
#define REG_A6     0x06
#define REG_A7     0x07
#define REG_A8     0x08
#define REG_A9     0x09
#define REG_A10    0x0A
#define REG_A11    0x0B
#define REG_A12    0x0C
#define REG_A13    0x0D

/* G2144 Register address */
/*     A0     */
#define TSSEL  BIT0
#define NTCDET BIT1
#define ENDPM  BIT3
#define CHGRST BIT6
#define PMURST BIT7
/*     A1     */
#define ERRDC1 BIT4
#define ERRDC2 BIT5
#define ERRDC3 BIT6
#define ERRDC4 BIT7
/*     A3     */
#define VOLDO6 (BIT0|BIT1)
#define VOLDO5 (BIT2|BIT3)
/*     A4     */
#define FBDC2 (BIT0|BIT1|BIT2|BIT3)
#define FBDC1 (BIT4|BIT5|BIT6|BIT7)
/*     A5     */
#define ISETA (BIT0|BIT1|BIT2|BIT3)
#define ISET_VBUS (BIT4|BIT5)
#define ISET_DCIN (BIT6|BIT7)
/*     A6     */
#define CHGSTEN BIT0
#define INSUS   BIT1
#define USUS    BIT2
#define ENCH    BIT3
#define TIMER   (BIT4|BIT5|BIT6|BIT7)
/*     A7     */
#define _INT BIT0
#define TSHT (BIT1|BIT2)
#define JEITA BIT3
#define VSETH BIT4
#define VSETC BIT5
#define ISETH BIT6
#define ISETC BIT7
/*     A8     */
#define ENOTG BIT3
#define VSETA (BIT6|BIT7)
/*     A9     */
#define MASK_SAFE     BIT0
#define MASK_USBPG    BIT1
#define MASK_DCINPG   BIT2
#define MASK_EOC      BIT3
#define MASK_NOBAT    BIT4
#define MASK_JEITA    BIT7
/*     A10     */
#define SAFE          BIT0
#define VBUS_PG       BIT1
#define DCIN_PG       BIT2
#define EOC           BIT3
#define NO_BAT        BIT4
#define TS_METER      (BIT5|BIT6|BIT7)
/*     A11     */
#define MASK_BATV     BIT3
#define MASK_CHGDONE  BIT4
#define MASK_BATLOW   BIT5
#define MASK_THR      BIT6
#define MASK_DPM      BIT7
/*     A12     */
#define INT_TYPE      BIT0
#define BATV          (BIT1|BIT2|BIT3)
#define CHGDONE       BIT4
#define BATLOW        BIT5
#define THR           BIT6
#define DPM           BIT7
/*     A13     */
#define TSD     BIT0
#define BATSUP    BIT1
#define NTC100K   BIT2
#define DPPM      BIT3
#define DCDET    BIT6
#define PWRGD    BIT7


struct g2214_charger *gmt;

struct charge_current_set {
	int dcin;
	int otgpc;
	int otgadp;
};

/**
 * struct g2214_charger - g2214 charger instance
 * @lock: protects concurrent access to online variables
 * @client: pointer to i2c client
 * @mains: power_supply instance for AC/DC power
 * @usb: power_supply instance for USB power
 * @battery: power_supply instance for battery
 * @mains_online: is AC/DC input connected
 * @usb_online: is USB input connected
 * @charging_enabled: is charging enabled
 * @dentry: for debugfs
 * @pdata: pointer to platform data
 */
struct g2214_charger {
	struct mutex		lock;
	struct gmt2214_dev *gmt_dev;
	struct power_supply	mains;
	struct power_supply	usb;
	struct power_supply	battery;
	struct device *dev;		/* the device structure		*/
	struct work_struct g2214_work;
	struct work_struct otg_work;
	struct charge_current_set sus;
        struct charge_current_set resum;
	struct charge_current_set sus_set;
        struct charge_current_set resum_set;

	unsigned int		mains_current_limit;
	int			en_gpio;
        int                   g2144_irq;
	int                   otg_irq;
	//int                   voldo5; 
	//int                   voldo5_del_time;
	int                   iset_dcin;
	int                   iset_vbus;
	int                    vseta;
	int                   current_state;
	int			safety_time;
	bool			mains_online;
	bool			usb_online;
	bool			charging_enabled;
	bool			usb_hc_mode;
	bool			usb_otg_enabled;
	bool			is_fully_charged;
	bool			batt_dead;
	bool                 batt_low;
	bool                 otg_check;
	bool                 d_plus_check;
        bool                 chgen;
	bool                  g2214en;
	bool                usb_connected;
	bool                usb_con_sus;
	bool                no_batt;
};


#endif  /* __G2144_CHARGER_H__ */
