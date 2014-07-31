/* Source for:
 * Cypress TrueTouch(TM) Standard Product I2C touchscreen driver.
 * drivers/input/touchscreen/cyttsp-i2c.c
 *
 * Copyright (C) 2009, 2010 Cypress Semiconductor, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2, and only version 2, as published by the
 * Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * Cypress reserves the right to make changes without further notice
 * to the materials described herein. Cypress does not assume any
 * liability arising out of the application described herein.
 *
 * Contact Cypress Semiconductor at www.cypress.com
 *
 */

#include <linux/delay.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/slab.h>
#include <linux/gpio.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/timer.h>
#include <linux/workqueue.h>
#include <linux/byteorder/generic.h>
#include <linux/bitops.h>
#include <linux/pm_runtime.h>
#include <linux/firmware.h>
#include <linux/mutex.h>
#include <linux/regulator/consumer.h>
#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif /* CONFIG_HAS_EARLYSUSPEND */
//#include <mach/vreg.h>

#define CY_DECLARE_GLOBALS

#include "cyttsp.h"

#include <linux/miscdevice.h>
#include <asm/uaccess.h>

#include <linux/poll.h>

#define LOG_TP()  printk("++++%s, Line %d\n", __FUNCTION__, __LINE__);


#define CYTTSP_SUPPORT_READ_TP_VERSION

#define CYTTSP_SUPPORT_TP_SENSOR
// need upgrade,add MACRO
//#ifdef CONFIG_LCT_AW551_YL
#define CYTTSP_SUPPORT_SYS_NODE
//#endif

extern struct tpd_device *tpd;//add 2013-1-7

int cyttsp_vendor_id=20;
int cyttsp_firmware_version= -1;

int cyttsp_has_bootloader=0;

#ifdef CYTTSP_SUPPORT_TP_SENSOR

#define CYTTSP_DEBUG_TP_SENSOR

#define CYTTSP_SUPPORT_TP_SENSOR_FIRMWARE_VERSION (0xc) //if we change this value we should also change func apds9900_init_dev in file apds9000.c
static DEFINE_MUTEX(cyttsp_sensor_mutex);
static DECLARE_WAIT_QUEUE_HEAD(cyttsp_sensor_waitqueue);

//static char cyttsp_sensor_data = 0; // 0 near 1 far
//static int cyttsp_sensor_data_changed = 0;
//static struct i2c_client *tp_sensor_I2Cclient = NULL;
//static int cyttsp_sensor_opened = 0;



#endif



#define CYTTSP_AW551_OFILM  "cyttspfw_aw551_ofilm.fw"
#define CYTTSP_AW551_TRULY  "cyttspfw_aw551_truly.fw"
#define CYTTSP_AW550_TRULY  "cyttspfw_aw550_truly.fw"

uint32_t cyttsp_tsdebug1 = 0xff;

module_param_named(tsdebug1, cyttsp_tsdebug1, uint, 0664);

#define FW_FNAME_LEN 40
#define TP_ID_GPIO 85

//static u8 irq_cnt;		/* comparison counter with register valuw */
//static u32 irq_cnt_total;	/* total interrupts */
//static u32 irq_err_cnt;		/* count number of touch interrupts with err */
#define CY_IRQ_CNT_MASK	0x000000FF	/* mapped for sizeof count in reg */
#define CY_IRQ_CNT_REG	0x00		/* tt_undef[0]=reg 0x1B - Gen3 only */



/* ****************************************************************************
 * Prototypes for static functions
 * ************************************************************************** */

static int cyttsp_putbl(struct cyttsp *ts, int show,
			int show_status, int show_version, int show_cid);
/*static int __devinit cyttsp_probe(struct i2c_client *client,
			const struct i2c_device_id *id); */
//static int __devexit cyttsp_remove(struct i2c_client *client);
//static int cyttsp_resume(struct device *dev);
//static int cyttsp_suspend(struct device *dev);

#ifdef CYTTSP_SUPPORT_SYS_NODE
//static	int cyttsp_power_down(void);
#endif



/* Static variables */
//static struct cyttsp_gen3_xydata_t g_xy_data;
static struct cyttsp_bootloader_data_t g_bl_data;
static struct cyttsp_sysinfo_data_t g_sysinfo_data;
static const struct i2c_device_id cyttsp_id[] = {
	{ CY_I2C_NAME, 0 },  { }
};
static u8 bl_cmd[] = {
	CY_BL_FILE0, CY_BL_CMD, CY_BL_EXIT,
	CY_BL_KEY0, CY_BL_KEY1, CY_BL_KEY2,
	CY_BL_KEY3, CY_BL_KEY4, CY_BL_KEY5,
	CY_BL_KEY6, CY_BL_KEY7};

MODULE_DEVICE_TABLE(i2c, cyttsp_id);



MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Cypress TrueTouch(R) Standard touchscreen driver");
MODULE_AUTHOR("Cypress");



#ifdef CYTTSP_SUPPORT_TP_SENSOR
#define CYTTSP_SENSOR_IOM	  'r'

#define CYTTSP_SENSOR_IOC_SET_PS_ENABLE		 _IOW(CYTTSP_SENSOR_IOM, 0, char *)
#define CYTTSP_SENSOR_READ_PS_DATA		_IOR(CYTTSP_SENSOR_IOM, 2, char *)




#endif









static void cyttsp_exit_bl_mode(struct cyttsp *ts);





#ifdef CYTTSP_SUPPORT_SYS_NODE
/* firmware flashing block */
#define BLK_SIZE     16
#define DATA_REC_LEN 64
#define START_ADDR   0x0880//0x0b00
#define BLK_SEED     0xff
#define RECAL_REG    0x1b

enum bl_commands {
	BL_CMD_WRBLK     = 0x39,
	BL_CMD_INIT      = 0x38,
	BL_CMD_TERMINATE = 0x3b,
};
/* TODO: Add key as part of platform data */
#define KEY_CS  (0 + 1 + 2 + 3 + 4 + 5 + 6 + 7)
#define KEY {0, 1, 2, 3, 4, 5, 6, 7}

static const  char _key[] = KEY;
#define KEY_LEN sizeof(_key)

static int rec_cnt;
struct fw_record {
	u8 seed;
	u8 cmd;
	u8 key[KEY_LEN];
	u8 blk_hi;
	u8 blk_lo;
	u8 data[DATA_REC_LEN];
	u8 data_cs;
	u8 rec_cs;
};
#define fw_rec_size (sizeof(struct fw_record))

struct cmd_record {
	u8 reg;
	u8 seed;
	u8 cmd;
	u8 key[KEY_LEN];
};
#define cmd_rec_size (sizeof(struct cmd_record))

static struct fw_record data_record = {
	.seed = BLK_SEED,
	.cmd = BL_CMD_WRBLK,
	.key = KEY,
};

static const struct cmd_record terminate_rec = {
	.reg = 0,
	.seed = BLK_SEED,
	.cmd = BL_CMD_TERMINATE,
	.key = KEY,
};
static const struct cmd_record initiate_rec = {
	.reg = 0,
	.seed = BLK_SEED,
	.cmd = BL_CMD_INIT,
	.key = KEY,
};

#define BL_REC1_ADDR          0x0780
#define BL_REC2_ADDR          0x07c0

#define ID_INFO_REC           ":40078000"
#define ID_INFO_OFFSET_IN_REC 77

#define REC_START_CHR     ':'
#define REC_LEN_OFFSET     1
#define REC_ADDR_HI_OFFSET 3
#define REC_ADDR_LO_OFFSET 5
#define REC_TYPE_OFFSET    7
#define REC_DATA_OFFSET    9
#define REC_LINE_SIZE	141

static int cyttsp_soft_reset(struct cyttsp *ts)
{
	int retval = 0, tries = 0;
	u8 host_reg = CY_SOFT_RESET_MODE;

	#if 0
          gpio_set_value(ts->platform_data->resout_gpio, 1); //reset high valid
	   msleep(100);
	   gpio_set_value(ts->platform_data->resout_gpio, 0);
          msleep(1000);
	#endif
	LOG_TP();

	#if 1// 0 modify 2013-1-7!!!!!!!!!!!
	do {
		retval = i2c_smbus_write_i2c_block_data(ts->client,
				CY_REG_BASE, sizeof(host_reg), &host_reg);
		if (retval < 0)
			msleep(20);
	} while (tries++ < 10 && (retval < 0));

	if (retval < 0) {
		pr_err("%s: failed\n", __func__);
		return retval;
	}
	#else
	cyttsp_hw_reset(); //!!!! 2013-1-7 may not go here !!!
	#endif

	LOG_TP();
	tries = 0;
	do {
		msleep(20);
		cyttsp_putbl(ts, 1, true, true, false);
	} while (g_bl_data.bl_status != 0x10 &&
		g_bl_data.bl_status != 0x11 &&
		tries++ < 100);
	LOG_TP();
	if (g_bl_data.bl_status != 0x11 && g_bl_data.bl_status != 0x10)
		return -EINVAL;
	LOG_TP();
	return 0;
}

static void cyttsp_exit_bl_mode(struct cyttsp *ts)
{
	int retval, tries = 0;

	do {
		retval = i2c_smbus_write_i2c_block_data(ts->client,
			CY_REG_BASE, sizeof(bl_cmd), bl_cmd);
		if (retval < 0)
			msleep(20);
	} while (tries++ < 10 && (retval < 0));
}

static void cyttsp_set_sysinfo_mode(struct cyttsp *ts)
{
	int retval, tries = 0;
	u8 host_reg = CY_SYSINFO_MODE;

	do {
		retval = i2c_smbus_write_i2c_block_data(ts->client,
			CY_REG_BASE, sizeof(host_reg), &host_reg);
		if (retval < 0)
			msleep(20);
	} while (tries++ < 10 && (retval < 0));

	/* wait for TTSP Device to complete switch to SysInfo mode */
	if (!(retval < 0)) {
		retval = i2c_smbus_read_i2c_block_data(ts->client,
				CY_REG_BASE,
				sizeof(struct cyttsp_sysinfo_data_t),
				(u8 *)&g_sysinfo_data);
	} else
		pr_err("%s: failed\n", __func__);
}

static void cyttsp_set_opmode(struct cyttsp *ts)
{
	int retval, tries = 0;
	u8 host_reg = CY_OP_MODE;

	do {
		retval = i2c_smbus_write_i2c_block_data(ts->client,
				CY_REG_BASE, sizeof(host_reg), &host_reg);
		if (retval < 0)
			msleep(20);
	} while (tries++ < 10 && (retval < 0));
}

static int str2uc(char *str, u8 *val)
{
	char substr[3];
	unsigned long ulval;
	int rc;

	if (!str && strlen(str) < 2)
		return -EINVAL;

	substr[0] = str[0];
	substr[1] = str[1];
	substr[2] = '\0';

	rc = strict_strtoul(substr, 16, &ulval);
	if (rc != 0)
		return rc;

	*val = (u8) ulval;

	return 0;
}

static int flash_block(struct cyttsp *ts, u8 *blk, int len)
{
	int retval, i, tries = 0;
	char buf[(2 * (BLK_SIZE + 1)) + 1];
	char *p = buf;

	for (i = 0; i < len; i++, p += 2)
		sprintf(p, "%02x", blk[i]);
	pr_debug("%s: size %d, pos %ld payload %s\n",
		       __func__, len, (long)0, buf);

	do {
		retval = i2c_smbus_write_i2c_block_data(ts->client,
			CY_REG_BASE, len, blk);
		if (retval < 0)
			msleep(20);
	} while (tries++ < 20 && (retval < 0));

	if (retval < 0) {
		pr_err("%s: failed\n", __func__);
		return retval;
	}

	return 0;
}

static int flash_command(struct cyttsp *ts, const struct cmd_record *record)
{
	return flash_block(ts, (u8 *)record, cmd_rec_size);
}

static void init_data_record(struct fw_record *rec, unsigned short addr)
{
	addr >>= 6;
	rec->blk_hi = (addr >> 8) & 0xff;
	rec->blk_lo = addr & 0xff;
	rec->rec_cs = rec->blk_hi + rec->blk_lo +
			(unsigned char)(BLK_SEED + BL_CMD_WRBLK + KEY_CS);
	rec->data_cs = 0;
}

static int check_record(u8 *rec)
{
	int rc;
	u16 addr;
	u8 r_len, type, hi_off, lo_off;

	rc = str2uc(rec + REC_LEN_OFFSET, &r_len);
	if (rc < 0)
		return rc;

	rc = str2uc(rec + REC_TYPE_OFFSET, &type);
	if (rc < 0)
		return rc;

	if (*rec != REC_START_CHR || r_len != DATA_REC_LEN || type != 0)
		return -EINVAL;

	rc = str2uc(rec + REC_ADDR_HI_OFFSET, &hi_off);
	if (rc < 0)
		return rc;

	rc = str2uc(rec + REC_ADDR_LO_OFFSET, &lo_off);
	if (rc < 0)
		return rc;

	addr = (hi_off << 8) | lo_off;

	if (addr >= START_ADDR || addr == BL_REC1_ADDR || addr == BL_REC2_ADDR)
		return 0;

	return -EINVAL;
}

static struct fw_record *prepare_record(u8 *rec)
{
	int i, rc;
	u16 addr;
	u8 hi_off, lo_off;
	u8 *p;

	rc = str2uc(rec + REC_ADDR_HI_OFFSET, &hi_off);
	if (rc < 0)
		return ERR_PTR((long) rc);

	rc = str2uc(rec + REC_ADDR_LO_OFFSET, &lo_off);
	if (rc < 0)
		return ERR_PTR((long) rc);

	addr = (hi_off << 8) | lo_off;

	init_data_record(&data_record, addr);
	p = rec + REC_DATA_OFFSET;
	for (i = 0; i < DATA_REC_LEN; i++) {
		rc = str2uc(p, &data_record.data[i]);
		if (rc < 0)
			return ERR_PTR((long) rc);
		data_record.data_cs += data_record.data[i];
		data_record.rec_cs += data_record.data[i];
		p += 2;
	}
	data_record.rec_cs += data_record.data_cs;

	return &data_record;
}

static int flash_record(struct cyttsp *ts, const struct fw_record *record)
{
	int len = fw_rec_size;
	int blk_len, rc;
	u8 *rec = (u8 *)record;
	u8 data[BLK_SIZE + 1];
	u8 blk_offset;

	for (blk_offset = 0; len; len -= blk_len) {
		data[0] = blk_offset;
		blk_len = len > BLK_SIZE ? BLK_SIZE : len;
		memcpy(data + 1, rec, blk_len);
		rec += blk_len;
		rc = flash_block(ts, data, blk_len + 1);
		if (rc < 0)
			return rc;
		blk_offset += blk_len;
	}
	return 0;
}

static int flash_data_rec(struct cyttsp *ts, u8 *buf)
{
	struct fw_record *rec;
	int rc, tries;
LOG_TP();
	if (!buf)
		return -EINVAL;

	rc = check_record(buf);

	if (rc < 0) {
		pr_debug("%s: record ignored %s", __func__, buf);
		return 0;
	}

	rec = prepare_record(buf);
	if (IS_ERR_OR_NULL(rec))
		return PTR_ERR(rec);

	rc = flash_record(ts, rec);
	if (rc < 0)
		return rc;

	tries = 0;
	do {
//printk("++++%s, Line %d, tries=%d\n", __FUNCTION__, __LINE__,tries );	//wujinyou
		
		//if (rec_cnt%2)
			//msleep(20);
		if(tries >50) 
		{
			printk("++++%s, Line %d, tries=%d\n", __FUNCTION__, __LINE__,tries );	//wujinyou
			msleep(20);
		}
		cyttsp_putbl(ts, 4, true, false, false);
	} while (g_bl_data.bl_status != 0x10 &&
		g_bl_data.bl_status != 0x11 &&
		tries++ < 100);
	rec_cnt++;
	return rc;
}

static int cyttspfw_flash_firmware(struct cyttsp *ts, const u8 *data,
					int data_len)
{
	u8 *buf;
	int i, j;
	int rc, tries = 0;
	LOG_TP();

	/* initiate bootload: this will erase all the existing data */
	rc = flash_command(ts, &initiate_rec);
	if (rc < 0)
		return rc;

	do {
//		LOG_TP();
printk("++++%s, Line %d, tries=%d\n", __FUNCTION__, __LINE__,tries );
	msleep(60);
		cyttsp_putbl(ts, 4, true, false, false);
	} while (g_bl_data.bl_status != 0x10 &&
		g_bl_data.bl_status != 0x11 &&
		tries++ < 100);

	buf = kzalloc(REC_LINE_SIZE + 1, GFP_KERNEL);
	if (!buf) {
		pr_err("%s: no memory\n", __func__);
		return -ENOMEM;
	}
	LOG_TP();
	rec_cnt = 0;
	/* flash data records */
	for (i = 0, j = 0; i < data_len; i++, j++) {
		if ((data[i] == REC_START_CHR) && j) {
			buf[j] = 0;
			rc = flash_data_rec(ts, buf);
			if (rc < 0)
				return rc;
			j = 0;
		}
		buf[j] = data[i];
	}
	LOG_TP();
	/* flash last data record */
	if (j) {
		buf[j] = 0;
		rc = flash_data_rec(ts, buf);
		if (rc < 0)
			return rc;
	}
	LOG_TP();
	kfree(buf);

	/* termiate bootload */
	tries = 0;
	rc = flash_command(ts, &terminate_rec);
	do {
		msleep(100);
printk("++++%s, Line %d, tries=%d\n", __FUNCTION__, __LINE__,tries );
		
		cyttsp_putbl(ts, 4, true, false, false);
	} while (g_bl_data.bl_status != 0x10 &&
		g_bl_data.bl_status != 0x11 &&
		tries++ < 100);
	LOG_TP();
	return rc;
}

static int get_hex_fw_ver(u8 *p, u8 *ttspver_hi, u8 *ttspver_lo,
			u8 *appid_hi, u8 *appid_lo, u8 *appver_hi,
			u8 *appver_lo, u8 *cid_0, u8 *cid_1, u8 *cid_2)
{
	int rc;

	p = p + ID_INFO_OFFSET_IN_REC;
	rc = str2uc(p, ttspver_hi);
	if (rc < 0)
		return rc;
	p += 2;
	rc = str2uc(p, ttspver_lo);
	if (rc < 0)
		return rc;
	p += 2;
	rc = str2uc(p, appid_hi);
	if (rc < 0)
		return rc;
	p += 2;
	rc = str2uc(p, appid_lo);
	if (rc < 0)
		return rc;
	p += 2;
	rc = str2uc(p, appver_hi);
	if (rc < 0)
		return rc;
	p += 2;
	rc = str2uc(p, appver_lo);
	if (rc < 0)
		return rc;
	p += 2;
	rc = str2uc(p, cid_0);
	if (rc < 0)
		return rc;
	p += 2;
	rc = str2uc(p, cid_1);
	if (rc < 0)
		return rc;
	p += 2;
	rc = str2uc(p, cid_2);
	if (rc < 0)
		return rc;

	return 0;
}

static void cyttspfw_flash_start(struct cyttsp *ts, const u8 *data,
				int data_len, u8 *buf, bool force)
{
	int rc;
	u8 ttspver_hi = 0, ttspver_lo = 0, fw_upgrade = 0;
	u8 appid_hi = 0, appid_lo = 0;
	u8 appver_hi = 0, appver_lo = 0;
	u8 cid_0 = 0, cid_1 = 0, cid_2 = 0;
	char *p = buf;

	/* get hex firmware version */
	rc = get_hex_fw_ver(p, &ttspver_hi, &ttspver_lo,
		&appid_hi, &appid_lo, &appver_hi,
		&appver_lo, &cid_0, &cid_1, &cid_2);
printk("++++tpd-fw-ver: %x %x %x %x %x %x %x %x %x\n", ttspver_hi, ttspver_lo,	appid_hi, appid_lo, appver_hi,appver_lo, cid_0, cid_1,cid_2);
	if (rc < 0) {
		pr_err("%s: unable to get hex firmware version\n", __func__);
		return;
	}
#if 0	//wujinyou
	/* disable interrupts before flashing */
	if (ts->client->irq == 0)
		del_timer(&ts->timer);
	else
		disable_irq(ts->client->irq);

	rc = cancel_work_sync(&ts->work);

	if (rc && ts->client->irq)
		enable_irq(ts->client->irq);
#endif

	/* enter bootloader idle mode */
	rc = cyttsp_soft_reset(ts);
	//LOG_TP();
	printk("++++%s, Line %d, rc=%d\n", __FUNCTION__, __LINE__,rc);

	if (rc < 0) {
	LOG_TP();
		pr_err("%s: cyttsp_soft_reset try entering into idle mode"
				" second time\n", __func__);
		msleep(1000);
		
		rc = cyttsp_soft_reset(ts);
	}

	if (rc < 0) {
	LOG_TP();
		pr_err("%s:cyttsp_soft_reset  try again later\n", __func__);
		return;
	}

	LOG_TP();

	pr_info("Current firmware:lusongbai %d.%d.%d", g_bl_data.appid_lo,
				g_bl_data.appver_hi, g_bl_data.appver_lo);
	pr_info("New firmware: %d.%d.%d", appid_lo, appver_hi, appver_lo);
	LOG_TP();

	if (force)
		fw_upgrade = 1;
	else
		if ((appid_hi == g_bl_data.appid_hi) &&
			(appid_lo == g_bl_data.appid_lo)) {
			if (appver_hi > g_bl_data.appver_hi) {
				fw_upgrade = 1;
			} else if ((appver_hi == g_bl_data.appver_hi) &&
					 (appver_lo > g_bl_data.appver_lo)) {
					fw_upgrade = 1;
				} else {
					fw_upgrade = 0;
					pr_info("%s: Firmware version "
					"lesser/equal to existing firmware, "
					"upgrade not needed\n", __func__);
				}
		} else {
			fw_upgrade = 0;
			pr_info("%s: Firware versions do not match, "
						"cannot upgrade\n", __func__);
		}
		
	printk("++++%s, Line %d, fw_upgrade=%d\n", __FUNCTION__, __LINE__,fw_upgrade);

	if (fw_upgrade) {
		pr_info("%s: Starting firmware upgrade\n", __func__);
		rc = cyttspfw_flash_firmware(ts, data, data_len);
		if (rc < 0)
			pr_err("%s: firmware upgrade failed\n", __func__);
		else
			pr_info("%s: lusongbai  firmware upgrade success\n", __func__);
	}
	LOG_TP();

	/* enter bootloader idle mode */
	cyttsp_soft_reset(ts);
	LOG_TP();
	
	/* exit bootloader mode */
	cyttsp_exit_bl_mode(ts);
		LOG_TP();

	msleep(100);
	/* set sysinfo details */
	cyttsp_set_sysinfo_mode(ts);
	LOG_TP();
	
	/* enter application mode */
	cyttsp_set_opmode(ts);
	LOG_TP();

	if((fw_upgrade == 1) && (rc >= 0))
	{
		u8 tmpData[18] = {0};
		rc  = i2c_smbus_read_i2c_block_data(ts->client,CY_REG_BASE,18, tmpData);  
		if(rc  < 0)
		{
			printk(KERN_ERR"cyttspfw_flash_start read version and module error\n");
		}
		else
		{
	       cyttsp_vendor_id=tmpData[16];
		   cyttsp_firmware_version = tmpData[17];
		   printk(KERN_ERR"cyttspfw_flash_start tp module is:%x   firmware version is %x:\n",tmpData[16],tmpData[17]);
		}
	}
	LOG_TP();

	/* enable interrupts */
#if 0
	if (ts->client->irq == 0)
		mod_timer(&ts->timer, jiffies + TOUCHSCREEN_TIMEOUT);
	else
		enable_irq(ts->client->irq);
#endif
	LOG_TP();
	
}

static void cyttspfw_upgrade_start(struct cyttsp *ts, const u8 *data,
					int data_len, bool force)
{
	int i, j;
	u8 *buf;

	buf = kzalloc(REC_LINE_SIZE + 1, GFP_KERNEL);
	if (!buf) {
		pr_err("%s: no memory\n", __func__);
		return;
	}

	for (i = 0, j = 0; i < data_len; i++, j++) {
		if ((data[i] == REC_START_CHR) && j) {
			buf[j] = 0;
			j = 0;
			if (!strncmp(buf, ID_INFO_REC, strlen(ID_INFO_REC))) {
				cyttspfw_flash_start(ts, data, data_len,
							buf, force);
				break;
			}
		}
		buf[j] = data[i];
	}

	/* check in the last record of firmware */
	if (j) {
		buf[j] = 0;
		if (!strncmp(buf, ID_INFO_REC, strlen(ID_INFO_REC))) {
			cyttspfw_flash_start(ts, data, data_len,
						buf, force);
		}
	}

	kfree(buf);
}
static bool cyttsp_IsTpInBootloader(struct cyttsp *ts)
{	  
    int retval = -1;
	u8 tmpData[18] = {0};
	retval = i2c_smbus_read_i2c_block_data(ts->client,CY_REG_BASE,18, tmpData);  
	if(retval < 0)
	{
		printk(KERN_ERR"cyttsp_IsTpInBootloader read version and module error\n");
		return false;
	}
	else
	{
		retval = 0;
		retval  = ((tmpData[1] & 0x10) >> 4);

		printk(KERN_ERR"cyttsp_IsTpInBootloader tmpData[1]:%x  retval:%x\n",tmpData[1],retval);
	}
	if(retval == 0)
	{
		return false;
	}
	return true;

}

const u8 fw_hex_of[] = {
//	#include "BOOT_AG500_OF_DW_2802_V2_20120702.i"
//	#include "BOOT_AG500_OF_DW_2803_V3_20120711.i"
};
const u8 fw_hex_hhx[] = {
//	#include "BOOT_AG500_F5_HHX_2503_V3_20120711.i"
};
struct cyttsp cust_ts;
const u8* fw_hex = fw_hex_hhx;
extern void cyttsp_print_reg(struct i2c_client *client);

int read_vender_id(void)
{
	char buffer[32];
	int ret =0;

	ret = i2c_smbus_read_i2c_block_data(cust_ts.client, 0x00, 24, &(buffer[0]));
	if(ret<0)
	{
		return -1;
	}
	cyttsp_vendor_id = buffer[3];
	printk("++++cyttp read_vender_id=0x%x\n", cyttsp_vendor_id);
	cyttsp_print_reg(cust_ts.client);
	return 0;
}


void cyttsp_fw_upgrade(void)
{
	struct cyttsp *ts = &cust_ts;
	bool force = 1;
	/* check and start upgrade */
	//if upgrade failed we should force upgrage when next power up

	if(read_vender_id() != 0)
	{
		printk("++++cyttspfw_upgrade read vender id failed!\n");
		return;
	}
	switch(cyttsp_vendor_id)
	{
		case 0x28:
			fw_hex = fw_hex_of;
			break;
		case 0x25:
			fw_hex = fw_hex_hhx;
			break;
		case 0x0:
			break;
		default:
			break;
	}
	
	if(cyttsp_IsTpInBootloader(ts) == true)
	{
	LOG_TP();
		printk(KERN_ERR"cyttspfw_upgrade we should force upgrade tp fw\n");
		cyttspfw_upgrade_start(ts, fw_hex,
			sizeof(fw_hex_of), true);
	}
	else
	{
	LOG_TP();
		cyttspfw_upgrade_start(ts, fw_hex,
			sizeof(fw_hex_of), force);
	}
}

#endif



static int cyttsp_putbl(struct cyttsp *ts, int show,
			int show_status, int show_version, int show_cid)
{
	int retval = CY_OK;

	int num_bytes = (show_status * 3) + (show_version * 6) + (show_cid * 3);

	if (show_cid)
		num_bytes = sizeof(struct cyttsp_bootloader_data_t);
	else if (show_version)
		num_bytes = sizeof(struct cyttsp_bootloader_data_t) - 3;
	else
		num_bytes = sizeof(struct cyttsp_bootloader_data_t) - 9;
	LOG_TP();

	if (show) {
		retval = i2c_smbus_read_i2c_block_data(ts->client,
			CY_REG_BASE, num_bytes, (u8 *)&g_bl_data);

			{
				int i = 0;
				printk("cyttsp_putbl:");
				for(i=0; i<num_bytes; i++)
					printk(" 0x%x",*((u8 *)&g_bl_data+i));
				printk("\n");
			}
		if (show_status) {
			cyttsp_debug("BL%d: f=%02X s=%02X err=%02X bl=%02X%02X bld=%02X%02X\n", \
				show, \
				g_bl_data.bl_file, \
				g_bl_data.bl_status, \
				g_bl_data.bl_error, \
				g_bl_data.blver_hi, g_bl_data.blver_lo, \
				g_bl_data.bld_blver_hi, g_bl_data.bld_blver_lo);
		}
		if (show_version) {
			cyttsp_debug("BL%d: ttspver=0x%02X%02X appid=0x%02X%02X appver=0x%02X%02X\n", \
				show, \
				g_bl_data.ttspver_hi, g_bl_data.ttspver_lo, \
				g_bl_data.appid_hi, g_bl_data.appid_lo, \
				g_bl_data.appver_hi, g_bl_data.appver_lo);
		}
		if (show_cid) {
			cyttsp_debug("BL%d: cid=0x%02X%02X%02X\n", \
				show, \
				g_bl_data.cid_0, \
				g_bl_data.cid_1, \
				g_bl_data.cid_2);
		}
	}
	//LOG_TP();

	return retval;
}


#ifndef CYTTSP_SUPPORT_SYS_NODE
static void cyttsp_exit_bl_mode(struct cyttsp *ts)
{
	int retval, tries = 0;

	do {
		retval = i2c_smbus_write_i2c_block_data(ts->client,
			CY_REG_BASE, sizeof(bl_cmd), bl_cmd);
		if (retval < 0)
			msleep(20);
	} while (tries++ < 10 && (retval < 0));
}
#endif



