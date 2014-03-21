/*++
drivers/mtd/devices/wmt_sf.c

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

/*
#include <linux/config.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/string.h>
#include <linux/ioport.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/dma-mapping.h>


#include <linux/mtd/partitions.h>

#include <asm/io.h>
#include <asm/dma.h>
#include <asm/sizes.h>

#include <asm/arch/vt8610_pmc.h>
#include <asm/arch/vt8610_gpio.h>
#include <asm/arch/vt8610_dma.h>
*/

//#include <linux/config.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/ioport.h>
#include <linux/vmalloc.h>
#include <linux/init.h>
#include <linux/mutex.h>
#include <linux/mtd/mtd.h>
#include <asm/io.h>
#include <linux/mtd/partitions.h>
#include <linux/platform_device.h>
#include <mach/hardware.h>
#include <linux/delay.h>

#include "wmt_sf.h"

SF_FPTR wmt_sf_prot = 0;
EXPORT_SYMBOL(wmt_sf_prot);

unsigned int MTDSF_PHY_ADDR, erase_sleep;

typedef enum {
	FL_READY,
	FL_READING,
	FL_WRITING,
	FL_ERASING,
} sf_state_t;

struct sf_hw_control {
	spinlock_t	 lock;
	wait_queue_head_t wq;
	//struct sf_chip *active;
};

struct wmt_sf_info_t {
		struct mtd_info	*sfmtd;
		struct mtd_info	mtd;
		struct mutex lock;
		struct sfreg_t *reg ;
		void *io_base;
		struct sf_hw_control controller;
		sf_state_t state;
};
static struct sfreg_t *reg_sf;
extern int wmt_getsyspara(char *varname, unsigned char *varval, int *varlen);
extern int wmt_setsyspara(char *varname, char *varval);
/* only bootable devices have a default partitioning */
/*static*/ struct mtd_partition boot_partitions[] = {
	{
		.name		= "filesystem-SF",
		.offset		= 0x00000000,
		.size		= 0x00D00000,
	},
	{
		.name		= "kernel-SF",
		.offset         = 0x00D00000,
		.size		= 0x00280000,
	},
	{
		.name           = "u-boot-SF",
		.offset         = 0x00F80000,
		.size           = 0x00050000,
	},
	{
		.name           = "u-boot env. cfg. 1-SF",
		.offset         = 0x00FD0000,
		.size           = 0x00010000,
	},
	{
		.name           = "u-boot env. cfg. 2-SF",
		.offset         = 0x00FE0000,
		.size           = 0x00010000,
	},
	{
		.name           = "w-load-SF",
		.offset         = 0x00FF0000,
		.size           = 0x00010000,
	}

};
#define NUM_SF_PARTITIONS ARRAY_SIZE(boot_partitions)
static const char *part_probes[] = { "cmdlinepart", NULL };
static struct mtd_partition *parts;

static unsigned int g_sf_force_size;

static int __init wmt_force_sf_size(char *str)
{
	char dummy;

	sscanf(str, "%d%c", (int *)&g_sf_force_size, &dummy);

	return 1;
}

__setup("sf_mtd=", wmt_force_sf_size);

struct wmt_flash_info_t g_sf_info[2];
extern struct wm_sf_dev_t sf_ids[];

int get_sf_info(int index, struct wmt_flash_info_t *info)
{
	unsigned int i;

	if (info->id == FLASH_UNKNOW)
		return -1;
	for (i = 0; sf_ids[i].id != 0; i++) {
		if (sf_ids[i].id == info->id) {
			info->total = (sf_ids[i].size*1024);
			break;
		}
	}
	if (sf_ids[i].id == 0) {
		printk(KERN_WARNING "un-know id%d = 0x%x\n", index, info->id);
		if (index == 0 && info->id != 0) {
			// if not identified, set size 512K as default.
			info->id = sf_ids[2].id;
			info->total = (sf_ids[2].size*1024);
		} else {
			info->id = FLASH_UNKNOW;	
			info->total = 0;
		}
		return -1;
	}

	return 0;
}

int wmt_sfc_ccr(struct wmt_flash_info_t *info)
{
	unsigned int cnt = 0, size;

	size = info->total;
	while (size) {
		size >>= 1;
		cnt++;
	}
	cnt -= 16;
	cnt = cnt<<8;
	info->val = (info->phy|cnt);
	return 0;
}

int wmt_sfc_init(struct sfreg_t *sfc)
{
	unsigned int tmp;
	int i, ret;

	tmp = STRAP_STATUS_VAL;
	if ((tmp & 0x4008) == (0x4000|SPI_FLASH_TYPE)) {
		MTDSF_PHY_ADDR = 0xFFFFFFFF;
		/* set default */
		sfc->SPI_RD_WR_CTR = 0x11;
		sfc->CHIP_SEL_0_CFG = 0xFF800800;
		sfc->SPI_INTF_CFG = 0x00030000;
	} else {
		/*MTDSF_PHY_ADDR = 0xEFFFFFFF;*/
		/* set default */
		/*sfc->SPI_RD_WR_CTR = 0x11;
		sfc->CHIP_SEL_0_CFG = 0xEF800800;
		sfc->SPI_INTF_CFG = 0x00030000;*/
		printk(KERN_WARNING "strapping not support sf\n");
		return -EIO;
	}
	memset(&g_sf_info[0], 0, 2*sizeof(struct wmt_flash_info_t));
	g_sf_info[0].id = FLASH_UNKNOW;
	g_sf_info[1].id = FLASH_UNKNOW;

	/* read id */
	sfc->SPI_RD_WR_CTR = 0x11;
	g_sf_info[0].id = sfc->SPI_MEM_0_SR_ACC;
	sfc->SPI_RD_WR_CTR = 0x01;
	sfc->SPI_RD_WR_CTR = 0x11;
	g_sf_info[1].id = sfc->SPI_MEM_1_SR_ACC;
	sfc->SPI_RD_WR_CTR = 0x01;

	for (i = 0; i < 2; i++) {
		ret = get_sf_info(i, &g_sf_info[i]);
		if (ret)
			break;
	}
	if (g_sf_info[0].id == FLASH_UNKNOW)
		return -1;
	g_sf_info[0].phy = (MTDSF_PHY_ADDR-g_sf_info[0].total+1);
	MTDSF_PHY_ADDR = MTDSF_PHY_ADDR-g_sf_info[0].total+1;
	if (g_sf_info[0].phy&0xFFFF) {
		printk(KERN_ERR "WMT SFC Err : start address must align to 64KByte\n");
		return -1;
	}
	wmt_sfc_ccr(&g_sf_info[0]);
	sfc->CHIP_SEL_0_CFG = g_sf_info[0].val;
	if (g_sf_info[1].id != FLASH_UNKNOW) {
		g_sf_info[1].phy = (g_sf_info[0].phy-g_sf_info[1].total);
		MTDSF_PHY_ADDR = MTDSF_PHY_ADDR-g_sf_info[1].total;
		tmp = g_sf_info[1].phy;
		g_sf_info[1].phy &= ~(g_sf_info[1].total-1);
		if (g_sf_info[0].phy&0xFFFF) {
			printk(KERN_ERR "WMT SFC Err : start address must align to 64KByte\n");
			printk(KERN_ERR "WMT SFC Err : CS1 could not be used\n");
			g_sf_info[1].id = FLASH_UNKNOW;
			return 0;
		}
		wmt_sfc_ccr(&g_sf_info[1]);
		sfc->CHIP_SEL_1_CFG = g_sf_info[1].val;
	}
	/*printk("CS0 : 0x%x ,  CS1 : 0x%x\n",g_sf_info[0].val,g_sf_info[1].val);*/
	
	if (g_sf_force_size) {
		tmp = (g_sf_force_size*1024*1024);
		MTDSF_PHY_ADDR = (0xFFFFFFFF-tmp)+1;
	}

	return 0;
}

int flash_error(unsigned long code)
{

	/* check Timeout */
	if (code & BIT_TIMEOUT) {
		printk(KERN_ERR "Serial Flash Timeout\n");/* For UBOOT */
		return ERR_TIMOUT;
	}

	if (code & SF_BIT_WR_PROT_ERR) {
		printk(KERN_ERR "Serial Flash Write Protect Error\n"); /* For UBOOT */
		/*return ERR_PROG_ERROR;*/
	}

	if (code & SF_BIT_MEM_REGION_ERR) {
		printk(KERN_ERR "Serial Flash Memory Region Error\n") ;/* For UBOOT */
		return ERR_PROG_ERROR;
	}

	if (code & SF_BIT_PWR_DWN_ACC_ERR) {
		printk(KERN_ERR "Serial Flash Power Down Access Error\n") ;/* For UBOOT */
		return ERR_PROG_ERROR;
	}

	if (code & SF_BIT_PCMD_OP_ERR)	{
		printk(KERN_ERR "Serial Flash Program CMD OP Error\n") ;/* For UBOOT */
		return ERR_PROG_ERROR;
	}

	if (code & SF_BIT_PCMD_ACC_ERR) {
		printk(KERN_ERR "Serial Flash Program CMD OP Access Error\n") ;/* For UBOOT */
		return ERR_PROG_ERROR;
	}

	if (code & SF_BIT_MASLOCK_ERR) {
		printk(KERN_ERR "Serial Flash Master Lock Error\n") ;/* For UBOOT */
		return ERR_PROG_ERROR;
	}

	/* OK, no error */
	return ERR_OK;
}
int spi_read_status(int chip)
{
	struct sfreg_t *sfreg = reg_sf;
	unsigned long temp, timeout = 0x30000000;
	int rc;

	REG32_VAL(PMCEU_ADDR) |= SF_CLOCK_EN;
	do {
			if (chip == 0)
				temp = sfreg->SPI_MEM_0_SR_ACC;
			else
				temp = sfreg->SPI_MEM_1_SR_ACC;
			/* please SPI flash data sheet */
			if ((temp & 0x1) == 0x0) {
				//printk(KERN_ERR "ok re flash status=0x%x\n", (unsigned int)sfreg->SPI_MEM_0_SR_ACC);
					break;
				}

			rc = flash_error(sfreg->SPI_ERROR_STATUS);
			if (rc != ERR_OK) {
				/*printk(KERN_ERR "re sts flash error rc = 0x%x\n", rc);*/
				sfreg->SPI_ERROR_STATUS = 0x3F; /* write 1 to clear status*/
				goto sf_err1;
			} else if (sfreg->SPI_ERROR_STATUS) {
				sfreg->SPI_ERROR_STATUS = 0x3F;
				printk(KERN_ERR "re flash error rc = 0x%x status=0x%x\n", rc, (unsigned int)sfreg->SPI_MEM_0_SR_ACC);
			}
			timeout--;

		} while (timeout);

		if (timeout == 0) {
			printk(KERN_ERR "Check SF status timeout\n");
			return ERR_TIMOUT;
		}
	return 0;

sf_err1:
	return rc;
}
EXPORT_SYMBOL(spi_read_status);

int spi_write_status(int chip, unsigned int value)
{
	struct sfreg_t *sfreg = reg_sf;
	int rc, index = 0, ii;
	unsigned int temp;
ii = *(volatile unsigned char *)(GPIO_BASE_ADDR + 0xDF);
	REG32_VAL(PMCEU_ADDR) |= SF_CLOCK_EN;

	rc = spi_read_status(chip);

wr_sts:
	if (chip == 0) {
		sfreg->SPI_WR_EN_CTR = SF_CS0_WR_EN;
		sfreg->SPI_MEM_0_SR_ACC = value;
	} else {
		sfreg->SPI_WR_EN_CTR = SF_CS1_WR_EN;
		sfreg->SPI_MEM_1_SR_ACC = value;
	}

	rc = spi_read_status(chip);
	temp = sfreg->SPI_MEM_0_SR_ACC;
	if ((temp&1) == 0 && (value&0x9C) != (temp&0x9C)) {
		printk(KERN_ERR "0x%x wr sf sts reg 0x%x fail i=%d gpio=0x%x\n",value, temp, index, ii);
		if (index < 10) {
			index++;
			goto wr_sts;
		} else
			printk(KERN_ERR "write sf status reg 0x%x fail\n", temp);
	}
	
	sfreg->SPI_WR_EN_CTR = SF_CS0_WR_DIS;

	rc = spi_read_status(chip);

	REG32_VAL(PMCEU_ADDR) &= ~(SF_CLOCK_EN);

	return rc;
}
EXPORT_SYMBOL_GPL(spi_write_status);

int spi_flash_sector_erase(unsigned long addr, struct sfreg_t *sfreg)
{
	unsigned long timeout = 0x600000;
	unsigned long temp ;
	int rc;

	REG32_VAL(PMCEU_ADDR) |= SF_CLOCK_EN;
	/*
	SPI module chip erase
	SPI flash write enable control register: write enable on chip sel 0
	*/
	if ((addr + MTDSF_PHY_ADDR) >= g_sf_info[0].phy) {
		sfreg->SPI_WR_EN_CTR = SF_CS0_WR_EN ;
		/* printk("sfreg->SPI_ER_START_ADDR = %x \n",sfreg->SPI_ER_START_ADDR);*/
		/*printk("!!!! Erase chip 0\n"); */

		/* select sector to erase */
		addr &= 0xFFFF0000;
		sfreg->SPI_ER_START_ADDR = (addr+MTDSF_PHY_ADDR);

		/*
		SPI flash erase control register: start chip erase
		Auto clear when transmit finishes.
		*/
		sfreg->SPI_ER_CTR = SF_SEC_ER_EN;
		/*printk("sfreg->SPI_ER_START_ADDR = %x \n",sfreg->SPI_ER_START_ADDR);*/

		/* poll status reg of chip 0 for chip erase */
		do {
			//printk("0s");
			if (erase_sleep == 1)
				msleep(60);
			REG32_VAL(PMCEU_ADDR) |= SF_CLOCK_EN;
			//printk(" 0e\n");
			udelay(1);
			temp = sfreg->SPI_MEM_0_SR_ACC;
			/* please SPI flash data sheet */
			if ((temp & 0x1) == 0x0)
				break;
			timeout--;

		} while (timeout);
		erase_sleep = 1;

		if (timeout == 0)
			goto er_err_timout;

		rc = flash_error(sfreg->SPI_ERROR_STATUS);
		if (rc != ERR_OK) {
			/*printk(KERN_ERR "flash error rc = 0x%x\n", rc);*/
			sfreg->SPI_ERROR_STATUS = 0x3F; /* write 1 to clear status*/
			goto sf_err;
		} else if (sfreg->SPI_ERROR_STATUS) {
			printk(KERN_ERR "flash error rc = 0x%x status=0x%x\n", rc, (unsigned int)sfreg->SPI_MEM_0_SR_ACC);
			sfreg->SPI_ERROR_STATUS = 0x3F;
			printk(KERN_ERR "1flash error rc = 0x%x status=0x%x\n", rc, (unsigned int)sfreg->SPI_MEM_0_SR_ACC);
		}

			sfreg->SPI_WR_EN_CTR = SF_CS0_WR_DIS;
			goto sf_OK;
	} else {
		sfreg->SPI_WR_EN_CTR = SF_CS1_WR_EN;
		/*  select sector to erase */
		addr &= 0xFFFF0000;
		sfreg->SPI_ER_START_ADDR = (addr+MTDSF_PHY_ADDR);

		/*
		SPI flash erase control register: start chip erase
		Auto clear when transmit finishes.
		*/
		sfreg->SPI_ER_CTR = SF_SEC_ER_EN;

		/* poll status reg of chip 0 for chip erase */
		do {
			//printk("1s");
			msleep(60);
			REG32_VAL(PMCEU_ADDR) |= SF_CLOCK_EN;
			//printk(" 1e\n");
			udelay(1);
			temp = sfreg->SPI_MEM_1_SR_ACC;
			/* please SPI flash data sheet */
			if ((temp & 0x1) == 0x0)
				break;

			rc = flash_error(sfreg->SPI_ERROR_STATUS);
			if (rc != ERR_OK) {
				sfreg->SPI_ERROR_STATUS = 0x3F ; /* write 1 to clear status*/
				goto sf_err;
			}
			timeout--;
		}  while (timeout);

		if (timeout == 0)
			goto er_err_timout;

		sfreg->SPI_WR_EN_CTR = SF_CS1_WR_DIS ;
		goto sf_OK;
	}
sf_OK:
	return ERR_OK;
sf_err:
	return rc;
er_err_timout:
	return ERR_TIMOUT;
}

/*
	We could store these in the mtd structure, but we only support 1 device..
	static struct mtd_info *mtd_info;
*/
static int sf_erase(struct mtd_info *mtd, struct erase_info *instr)
{
	int ret;
	struct wmt_sf_info_t *info = (struct wmt_sf_info_t *)mtd->priv;
	struct sfreg_t *sfreg = info->reg;

	mutex_lock(&info->lock);
	REG32_VAL(PMCEU_ADDR) |= SF_CLOCK_EN;
	ret = spi_flash_sector_erase((unsigned long)instr->addr, sfreg);
	REG32_VAL(PMCEU_ADDR) &= ~(SF_CLOCK_EN);
	mutex_unlock(&info->lock);

	if (ret != ERR_OK) {
		printk(KERN_ERR "sf_erase() error at address 0x%lx \n", (unsigned long)instr->addr);
		return -EINVAL;
	}
	instr->state = MTD_ERASE_DONE;
	mtd_erase_callback(instr);

	return 0;
}

static int sf_read(struct mtd_info *mtd, loff_t from, size_t len,
			 size_t *retlen, u_char *buf)
{
	struct wmt_sf_info_t *info = (struct wmt_sf_info_t *)mtd->priv;
	unsigned char *sf_base_addr = info->io_base;
	int rc;


//printk("re sf check");
	if ((from + MTDSF_PHY_ADDR) >= g_sf_info[0].phy) {
		rc = spi_read_status(0);
		if (rc)
			printk("sfread: sf0 is busy");
	} else {
		rc = spi_read_status(1);
		if (rc)
			printk("sfread: sf1 is busy");
	}
	//printk("end\n");
	/*printk("sf_read(pos:%x, len:%x)\n", (long)from, (long)len);*/
	if (from + len > mtd->size) {
		printk(KERN_ERR "sf_read() out of bounds (%lx > %lx)\n", (long)(from + len), (long)mtd->size);
		return -EINVAL;
	}

	mutex_lock(&info->lock);
	//printk("sfread: lock from%llx, len=%d\n", from, len);
	REG32_VAL(PMCEU_ADDR) |= SF_CLOCK_EN;
	_memcpy_fromio(buf, (sf_base_addr+from), len);
	REG32_VAL(PMCEU_ADDR) &= ~(SF_CLOCK_EN);
	mutex_unlock(&info->lock);

	*retlen = len;
	

	return 0;

}

int spi_flash_sector_write(struct sfreg_t *sfreg, unsigned char *sf_base_addr,
		loff_t to, size_t len, u_char *buf)
{
	unsigned long temp;
	unsigned int i = 0;
	int rc ;
	unsigned long timeout = 0x30000000;
	size_t retlen;

	REG32_VAL(PMCEU_ADDR) |= SF_CLOCK_EN;
	udelay(1);
	//printk("wr sf check");
	if ((to + MTDSF_PHY_ADDR) >= g_sf_info[0].phy) {
		rc = spi_read_status(0);
		if (rc)
			printk("wr c0 wait status ret=%d\n", rc);
	} else {
		rc = spi_read_status(1);
		if (rc)
			printk("wr c1 wait status ret=%d\n", rc);
	}
	//printk("end\n");
	sfreg->SPI_WR_EN_CTR = 0x03;

	while (len >= 8) {
		_memcpy_toio(((u_char *)(sf_base_addr+to+i)), buf+i, 4);
		i += 4;
		_memcpy_toio(((u_char *)(sf_base_addr+to+i)), (buf+i), 4);
		i += 4;
		len -= 8;
		timeout = 0x30000000;
		do {
			temp = sfreg->SPI_MEM_0_SR_ACC ;
			/* please see SPI flash data sheet */
			if ((temp & 0x1) == 0x0)
				break ;
			rc = flash_error(sfreg->SPI_ERROR_STATUS);
			if (rc != ERR_OK) {
				sfreg->SPI_ERROR_STATUS = 0x3F ; /* write 1 to clear status */
				goto  sf_wr_err;
			}
			timeout--;
		} while (timeout);

		if (timeout == 0) {
			printk(KERN_ERR "time out \n");
			goto  err_timeout;
		}
	}
	while (len >= 4) {
		_memcpy_toio(((u_char *)(sf_base_addr+to+i)), (u_char*)(buf+i), 4);
		i += 4;
		len -= 4;
		if (len) {
			_memcpy_toio(((u_char *)(sf_base_addr+to+i)), (u_char*)(buf+i), 1);
			i++;
			len--;
		}
		timeout = 0x30000000;
		do {
			temp = sfreg->SPI_MEM_0_SR_ACC ;
			/* please see SPI flash data sheet */
			if ((temp & 0x1) == 0x0)
				break;
			rc = flash_error(sfreg->SPI_ERROR_STATUS);
			if (rc != ERR_OK) {
				sfreg->SPI_ERROR_STATUS = 0x3F ; /* write 1 to clear status */
				goto sf_wr_err;
			}
			timeout--;
		} while (timeout);
		if (timeout == 0) {
			printk(KERN_ERR "time out \n");
			goto  err_timeout;
		}
	}
	while (len) {
		_memcpy_toio(((u_char *)(sf_base_addr+to+i)), (buf+i), 1);
		i++;
		len--;
		if (len) {
			_memcpy_toio(((u_char *)(sf_base_addr+to+i)), (buf+i), 1);
			i++;
			len--;
		}
		timeout = 0x30000000;
		do {
			temp = sfreg->SPI_MEM_0_SR_ACC ;
			/* please see SPI flash data sheet */
			if ((temp & 0x1) == 0x0)
				break;
			rc = flash_error(sfreg->SPI_ERROR_STATUS);
			if (rc != ERR_OK) {
				sfreg->SPI_ERROR_STATUS = 0x3F ; /* write 1 to clear status */
				goto sf_wr_err;
			}
				timeout--;
				} while (timeout);

			if (timeout == 0) {
				printk(KERN_ERR "time out \n");
				goto  err_timeout;
			}
		}
		retlen = i;
		sfreg->SPI_WR_EN_CTR = 0x00;
		
		//REG32_VAL(PMCEU_ADDR) &= ~(SF_CLOCK_EN);

		return retlen;

err_timeout:
		return ERR_TIMOUT;
sf_wr_err:
		return rc;
}

static int sf_write(struct mtd_info *mtd, loff_t to, size_t len,
				size_t *retlen, const u_char *buf)
{

	struct wmt_sf_info_t *info = (struct wmt_sf_info_t *)mtd->priv;
	unsigned char *sf_base_addr = info->io_base;
	struct sfreg_t *sfreg = info->reg;
	size_t ret;


	/*printk("sf_write(pos:0x%x, len:0x%x )\n", (long)to, (long)len);*/

	if (to + len > mtd->size) {
		printk(KERN_ERR "sf_write() out of bounds (%ld > %ld)\n", (long)(to + len), (long)mtd->size);
		return -EINVAL;
	}

	mutex_lock(&info->lock);
	REG32_VAL(PMCEU_ADDR) |= SF_CLOCK_EN;
	ret = spi_flash_sector_write(sfreg, sf_base_addr, to, len, (u_char *)buf);
	REG32_VAL(PMCEU_ADDR) &= ~(SF_CLOCK_EN);
	mutex_unlock(&info->lock);
	

	*retlen = ret;

	return 0;
}

#if 0
void print_reg()
{
		printk(KERN_INFO "sfreg->CHIP_SEL_0_CFG = %lx\n", sfreg->CHIP_SEL_0_CFG);
		printk(KERN_INFO "sfreg->CHIP_SEL_1_CFG = %lx\n", sfreg->CHIP_SEL_1_CFG);
		printk(KERN_INFO "sfreg->SPI_WR_EN_CTR = %lx \n", sfreg->SPI_WR_EN_CTR);
		printk(KERN_INFO "sfreg->SPI_ER_CTR = %lx \n", sfreg->SPI_ER_CTR);
		printk(KERN_INFO "sfreg->SPI_ER_START_ADDR = %lx \n", sfreg->SPI_ER_START_ADDR);
}

void identify_sf_device_id(int sf_num)
{
	sfreg->SPI_RD_WR_CTR = 0x10;
	if (sf_num == 0)
		printk(KERN_INFO "sfreg->SPI_MEM_0_SR_ACC=%lx\n", sfreg->SPI_MEM_0_SR_ACC);
	else if (sf_num == 1)
		printk(KERN_INFO "sfreg->SPI_MEM_0_SR_ACC=%lx\n", sfreg->SPI_MEM_0_SR_ACC);
	else
		printk(KERN_ERR "Unkown spi flash! \n");
}
#endif

void config_sf_reg(struct sfreg_t *sfreg)
{
#if 0
		sfreg->CHIP_SEL_0_CFG = (MTDSF_PHY_ADDR | 0x0800800); /*0xff800800;*/
		sfreg->CHIP_SEL_1_CFG = (MTDSF_PHY_ADDR | 0x0800);  /*0xff000800;*/
		sfreg->SPI_INTF_CFG   = 0x00030000;
		printk(KERN_INFO "Eric %s Enter chip0=%x chip1=%x\n"
			, __func__, sfreg->CHIP_SEL_0_CFG, sfreg->CHIP_SEL_1_CFG);
#else
		if (g_sf_info[0].val)
			sfreg->CHIP_SEL_0_CFG = g_sf_info[0].val;
		if (g_sf_info[1].val)
			sfreg->CHIP_SEL_1_CFG = g_sf_info[1].val;
		else
			sfreg->CHIP_SEL_1_CFG = 0xff780800;
		sfreg->SPI_INTF_CFG   = 0x00030000;
#endif
}
void shift_partition_content(int index)
{
	int i, j;
	for ( i = index, j = 0; i < 6; i++, j++) {
		boot_partitions[j].name = boot_partitions[i].name;
		boot_partitions[j].offset = boot_partitions[i].offset;
		boot_partitions[j].size = boot_partitions[i].size;
	}
}

int mtdsf_init_device(struct mtd_info *mtd, unsigned long size, char *name)
{
	int nr_parts = 0, cut_parts = 0, ret;

	mtd->name = name;
	mtd->type = MTD_NORFLASH;
	mtd->flags = MTD_CAP_NORFLASH;
	mtd->size = size;
	mtd->erasesize = MTDSF_ERASE_SIZE;
	mtd->owner = THIS_MODULE;
	mtd->_erase = sf_erase;
	mtd->_read = sf_read;
	mtd->_write = sf_write;
	mtd->writesize = 1;


	boot_partitions[5].offset = size - boot_partitions[5].size;
	boot_partitions[4].offset = boot_partitions[5].offset - boot_partitions[4].size;
	boot_partitions[3].offset = boot_partitions[4].offset - boot_partitions[3].size;
	boot_partitions[2].offset = boot_partitions[3].offset - boot_partitions[2].size;
	if (boot_partitions[2].offset > 0x00280000) {
		boot_partitions[1].offset = boot_partitions[2].offset - boot_partitions[1].size;
		if (boot_partitions[1].offset > 0)
			boot_partitions[0].size = boot_partitions[1].offset;
		else {
			shift_partition_content(1);
			cut_parts = 1;
		}
	} else if (boot_partitions[2].offset > 0) {
		boot_partitions[1].size = boot_partitions[2].offset;
		boot_partitions[1].offset = 0;
		shift_partition_content(1);
		cut_parts = 1;
	} else {
		shift_partition_content(2);
		cut_parts = 2;
	}

	parts = boot_partitions;
	nr_parts = NUM_SF_PARTITIONS;

	ret = mtd_device_parse_register(mtd, part_probes, NULL,	parts, nr_parts - cut_parts);
	/*if (ret) {
		dev_err(&dev->pdev->dev, "Err MTD partition=%d\n", ret);
	}*/

	return ret;
}

static int wmt_sf_probe(struct platform_device *pdev)
{
	int err;
	/*int retval, len = 40;
	char *buf[40];
	char *buf1 = "7533967";*/
/*	struct platform_device *pdev = to_platform_device(dev);*/
	struct wmt_sf_info_t	*info;
	unsigned int sfsize = 0;
	REG32_VAL(PMCEU_ADDR) |= SF_CLOCK_EN;
	erase_sleep = 1;
	//REG32_VAL(0xFE130314) = 0xc;
	printk("sf clock =0x%x \n", REG32_VAL(0xFE130314));
	info = kzalloc(sizeof(*info), GFP_KERNEL);
	if (!info)
		return -ENOMEM;

	mutex_init(&info->lock);
	
	info->sfmtd = &info->mtd;

	dev_set_drvdata(&pdev->dev, info);

	info->reg = (struct sfreg_t *)SF_BASE_ADDR;
	/*config_sf_reg(info->reg);*/
	if (info->reg)
		err = wmt_sfc_init(info->reg);
	else
		err = -EIO;

	if (err) {
		printk(KERN_ERR "wmt sf controller initial failed\n");
		goto exit_error;
	}

	if (g_sf_force_size)
		sfsize = (g_sf_force_size*1024*1024);
	else
		sfsize = (0xFFFFFFFF-MTDSF_PHY_ADDR)+1;//MTDSF_TOTAL_SIZE;
	printk("MTDSF_PHY_ADDR = %08X, sfsize = %08X\n",MTDSF_PHY_ADDR,sfsize);
	if (MTDSF_PHY_ADDR == 0xFFFFFFFF || MTDSF_PHY_ADDR == 0xEFFFFFFF) {
		MTDSF_PHY_ADDR = MTDSF_PHY_ADDR-sfsize+1;
		printk("MTDSF_PHY_ADDR = %08X, sfsize = %08X\n",MTDSF_PHY_ADDR,sfsize);
	}
	info->io_base = (unsigned char *)ioremap(MTDSF_PHY_ADDR, sfsize);
	if (info->io_base == NULL) {
		dev_err(&pdev->dev, "cannot reserve register region\n");
		 err = -EIO;
		 goto exit_error;
	}

	err = mtdsf_init_device(info->sfmtd, sfsize, "mtdsf device");
	if (err)
		goto exit_error;

	info->sfmtd->priv = info;
	reg_sf = info->reg; //for global use.

/*	retval = wmt_getsyspara("dan", buf, &len);
	printk(KERN_INFO "sf read env buf=%s\n", buf);
	retval = wmt_setsyspara("dan", buf1);
	retval = wmt_getsyspara("dan", buf, &len);
	printk(KERN_INFO "sf read env buf=%s\n", buf);*/
	REG32_VAL(PMCEU_ADDR) &= ~(SF_CLOCK_EN);

	printk(KERN_INFO "wmt sf controller initial ok\n");

exit_error:
	return err;
}

/*static int wmt_sf_remove(struct device *dev)*/
static int wmt_sf_remove(struct platform_device *pdev)
{
	struct wmt_sf_info_t *info = dev_get_drvdata(&pdev->dev);
	int		status;

	pr_debug("%s: remove\n", dev_name(&pdev->dev));

	status = mtd_device_unregister(&info->mtd);
	if (status == 0) {
		dev_set_drvdata(&pdev->dev, NULL);
		if (info->io_base)
			iounmap(info->io_base);
		kfree(info);
	}

	return 0;
}

#ifdef CONFIG_PM
int wmt_sf_suspend(struct platform_device *pdev, pm_message_t state)
{
	unsigned int boot_value = STRAP_STATUS_VAL;
	int rc = 0;

	/*Judge whether boot from SF in order to implement power self management*/
	if ((boot_value & 0x4008) == (0x4000|SPI_FLASH_TYPE)) {
		REG32_VAL(PMCEU_ADDR) |= SF_CLOCK_EN;
		rc = spi_read_status(0);
	if (rc)
		printk("sfread: sf0 is busy");
	}
	
	printk("suspend pllc=0x%x, div0x%x\n", 
	*(volatile unsigned int *)(0xfe130208), *(volatile unsigned int *)(0xfe13036c));

	printk(KERN_INFO "wmt_sf_suspend\n");

	return 0;
}

int wmt_sf_resume(struct platform_device *pdev)
{
	struct wmt_sf_info_t *info = dev_get_drvdata(&pdev->dev);
	struct sfreg_t *sfreg = info->reg;

	REG32_VAL(PMCEU_ADDR) |= SF_CLOCK_EN;
	if (info->reg)
		config_sf_reg(info->reg);
	else
		printk(KERN_ERR "wmt sf restore state error\n");

	if (g_sf_info[0].id == SF_IDALL(ATMEL_MANUF, AT_25DF041A_ID)) {
		printk(KERN_INFO "sf resume and set Global Unprotect\n");
		sfreg->SPI_INTF_CFG |= SF_MANUAL_MODE; /* enter programmable command mode */
		sfreg->SPI_PROG_CMD_WBF[0] = SF_CMD_WREN;
		sfreg->SPI_PROG_CMD_CTR = (0x01000000 | (0<<1)); /* set size and chip select */
		sfreg->SPI_PROG_CMD_CTR |= SF_RUN_CMD;  /* enable programmable command */
		while ((sfreg->SPI_PROG_CMD_CTR & SF_RUN_CMD) != 0)
			;
		sfreg->SPI_PROG_CMD_WBF[0] = SF_CMD_WRSR;
		sfreg->SPI_PROG_CMD_WBF[1] = 0x00; /* Global Unprotect */
		sfreg->SPI_PROG_CMD_CTR = (0x02000000 | (0<<1)); /* set size and chip select */
		sfreg->SPI_PROG_CMD_CTR |= SF_RUN_CMD;  /* enable programmable command */
		while ((sfreg->SPI_PROG_CMD_CTR & SF_RUN_CMD) != 0)
			;
		sfreg->SPI_PROG_CMD_CTR = 0; /* reset programmable command register*/
		sfreg->SPI_INTF_CFG &= ~SF_MANUAL_MODE; /* leave programmable mode */
	}

	REG32_VAL(PMCEU_ADDR) &= ~(SF_CLOCK_EN);/*Turn off the clock*/
	
	printk("resume pllc=0x%x, div0x%x\n", 
	*(volatile unsigned int *)(0xfe130208), *(volatile unsigned int *)(0xfe13036c));

	return 0;
}

#else
#define wmt_sf_suspend NULL
#define wmt_sf_resume NULL
#endif

/*
struct device_driver wmt_sf_driver = {
		.name       = "sf",
		.bus          = &platform_bus_type,
		.probe       = wmt_sf_probe,
		.remove     = wmt_sf_remove,
		.suspend    = wmt_sf_suspend,
		.resume     = wmt_sf_resume
};
*/

struct platform_driver wmt_sf_driver = {
	.driver.name	= "sf",
	.probe = wmt_sf_probe,
	.remove = wmt_sf_remove,
	.suspend = wmt_sf_suspend,
	.resume = wmt_sf_resume
};


static int __init wmt_sf_init(void)
{
	//printk(KERN_INFO "WMT SPI Flash Driver, WonderMedia Technologies, Inc\n");
	return platform_driver_register(&wmt_sf_driver);
}

static void __exit wmt_sf_exit(void)
{
	platform_driver_unregister(&wmt_sf_driver);
}

module_init(wmt_sf_init);
module_exit(wmt_sf_exit);

MODULE_AUTHOR("WonderMedia Technologies, Inc.");
MODULE_DESCRIPTION("WMT [SF] driver");
MODULE_LICENSE("GPL");
