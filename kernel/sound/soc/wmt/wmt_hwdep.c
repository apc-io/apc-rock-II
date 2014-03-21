/*++ 
 * linux/sound/soc/wmt/wmt_hwdep.c
 * WonderMedia I2S audio driver for ALSA
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

#include <sound/soc.h>
#include <sound/jack.h>

#include <asm/mach-types.h>
#include <mach/gpio.h>

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/device.h>

#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/soc-dapm.h>
#include <sound/hwdep.h>

#include <asm/mach-types.h>
#include <mach/hardware.h>

#include "wmt_hwdep.h"
#include "wmt-pcm.h"

int WFD_flag = 0;
static int gmode = 2;
static char gstring[3] = "LR";

static char gSpdHdm[6] = "BOTH";
static int gSpHd = 6;

extern void wmt_i2s_dac0_ctrl(int HDMI_audio_enable);

static int wmt_hwdep_open(struct snd_hwdep *hw, struct file *file)
{
	if ((file->f_flags & O_RDWR) && (WFD_flag)) {
		return -EBUSY;
	}
	else if (file->f_flags & O_SYNC) {
		WFD_flag = 1;
	}	
	return 0;
}

static int wmt_hwdep_release(struct snd_hwdep *hw, struct file *file)
{
	WFD_flag = 0;
	return 0;
}

static int wmt_hwdep_mmap(struct snd_hwdep *hw, struct file *file, struct vm_area_struct *vma)
{
	vma->vm_flags |= VM_IO | VM_RESERVED;
	vma->vm_page_prot = pgprot_writecombine(vma->vm_page_prot);
	if (remap_pfn_range(vma, vma->vm_start, (vma->vm_pgoff),
			vma->vm_end - vma->vm_start, vma->vm_page_prot)) {
       	printk("*E* remap page range failed: vm_pgoff=0x%x ", (unsigned int)vma->vm_pgoff);
       	return -EAGAIN;    
    }
    return 0;
}

static int wmt_hwdep_ioctl(struct snd_hwdep *hw, struct file *file, unsigned int cmd, unsigned long arg)
{
	int *value;
	WFDStrmInfo_t *info;
	struct wmt_soc_vt1603_info vt1603_info;
	int ret = 0;
	
	switch (cmd) {
	case WMT_SOC_IOCTL_HDMI:
		value = (int __user *)arg;
		
		if (*value > 1) {
			printk("Not supported status for HDMI Audio %d", *value);
			return 0;
		}	
		wmt_i2s_dac0_ctrl(*value);
		return 0;
	
	case WMT_SOC_IOCTL_WFD_START:
		wmt_pcm_wfd_start();
		return copy_to_user( (void *)arg, (const void __user *) wmt_pcm_wfd_get_buf(), sizeof(unsigned int));
		
	case WMT_SOC_IOCTL_GET_STRM:
		info = (WFDStrmInfo_t *)wmt_pcm_wfd_get_strm((WFDStrmInfo_t *)arg);
		return __put_user((int)info, (unsigned int __user *) arg);

	case WMT_SOC_IOCTL_WFD_STOP:
		wmt_pcm_wfd_stop();
		return 0;
	
	case WMT_SOC_IOCTL_VT1603_RD:
		ret = copy_from_user(&vt1603_info, (void __user *)arg, sizeof(vt1603_info));
		
		if (ret == 0) {
			vt1603_info.reg_value = vt1603_hwdep_ioctl(0, vt1603_info.reg_offset, vt1603_info.reg_value);
			printk("<<<%s read reg 0x%x val 0x%x\n", __FUNCTION__, vt1603_info.reg_offset, vt1603_info.reg_value);
			ret = copy_to_user((void __user *)arg, &vt1603_info, sizeof(vt1603_info));
		}
		return ret;
	case WMT_SOC_IOCTL_VT1603_WR:  
		ret = copy_from_user(&vt1603_info, (void __user *)arg, sizeof(vt1603_info));
			printk("<<<%s write reg 0x%x val 0x%x\n", __FUNCTION__, vt1603_info.reg_offset, vt1603_info.reg_value);
		if (ret == 0)
			vt1603_hwdep_ioctl(1, vt1603_info.reg_offset, vt1603_info.reg_value);
		return ret;
	
	case WMT_SOC_IOCTL_CH_SEL:
		value = (int __user *)arg;
		
		if (*value > 2) {
			printk("Not supported for CH select %d", *value);
			return 0;
		}	
		wmt_i2s_ch_sel(*value);
		return 0;		
		
	default:
		break;
	}
	
	printk("Not supported ioctl for WMT-HWDEP");
	return -ENOIOCTLCMD;
}

static long wmt_hwdep_write(struct snd_hwdep *hw, const char __user *buf,
		      long count, loff_t *offset)
{
	char string[3];
	//int mode;
	memset(string, 0, sizeof(string));
	copy_from_user(&string, buf, sizeof(string));
	printk("<<<%s %s\n", __FUNCTION__, string);
	if (!memcmp(string, "LL", 2)) {
		gmode = 0;		
	}
	else if (!memcmp(string, "RR", 2)) {
		gmode = 1;
	}
	else if (!memcmp(string, "LR", 2)) {
		gmode = 2;
	}
	else {
		printk("Not supported for CH select");
		return count;
	}	
	
	memset(gstring, 0, sizeof(gstring));
	strncpy(gstring, string, sizeof(string));
	wmt_i2s_ch_sel(gmode);
	return count;
}

static long wmt_hwdep_read(struct snd_hwdep *hw, char __user *buf,
		     long count, loff_t *offset)
{
	int len = 0;
	printk("%s string %s --> mode %d\n", __FUNCTION__, gstring, gmode);
	len = copy_to_user(buf, gstring, sizeof(gstring));
	
	return sizeof(gstring);
}		 

static long wmt_hwdep_write_1(struct snd_hwdep *hw, const char __user *buf,
		      long count, loff_t *offset)
{
	char string[5];
	//int mode;

	copy_from_user(&string, buf, sizeof(string));
	printk("<<<%s %s\n", __FUNCTION__, string);
	if (!memcmp(string, "NONE", 4)) {
		gSpHd = 3;
	}
	else if (!memcmp(string, "HDMI", 4)) {
		gSpHd = 4;
	}
	else if (!memcmp(string, "SPDIF", 5)) {
		gSpHd = 5;
	}
	else if (!memcmp(string, "BOTH", 4)) {
		gSpHd = 6;
	}
	else {
		printk("Not supported for SPDIF/HDMI switch");
		return count;
	}
	
	memset(gSpdHdm, 0, sizeof(gSpdHdm));
	strncpy(gSpdHdm, string, sizeof(string));
	
	wmt_i2s_ch_sel(gSpHd);
	return count;
}


static long wmt_hwdep_read_1(struct snd_hwdep *hw, char __user *buf,
		     long count, loff_t *offset)
{
	int len = 0;
	printk("%s string %s --> mode %d\n", __FUNCTION__, gSpdHdm, gSpHd);
	len = copy_to_user(buf, gSpdHdm, sizeof(gSpdHdm));
	
	return sizeof(gstring);
}

void wmt_soc_hwdep_new(struct snd_soc_codec *codec)
{
	struct snd_hwdep *hwdep;
	struct snd_hwdep *hwdep_1;
	if (snd_hwdep_new(codec->card->snd_card, "WMT-HWDEP", 0, &hwdep) < 0) {
		printk("create WMT-HWDEP_0 fail");
		return;
	}

	sprintf(hwdep->name, "WMT-HWDEP %d", 0);
	
	hwdep->iface = SNDRV_HWDEP_IFACE_WMT;
	hwdep->ops.open = wmt_hwdep_open;
	hwdep->ops.ioctl = wmt_hwdep_ioctl;
	hwdep->ops.release = wmt_hwdep_release;
	hwdep->ops.mmap = wmt_hwdep_mmap;
	hwdep->ops.write = wmt_hwdep_write;
	hwdep->ops.read = wmt_hwdep_read;
	
	if (snd_hwdep_new(codec->card->snd_card, "WMT-HWDEP", 1, &hwdep_1) < 0) {
		printk("create WMT-HWDEP_1 fail");
		return;
	}

	sprintf(hwdep_1->name, "WMT-HWDEP %d", 1);
	printk("create %s success", hwdep_1->name);

	hwdep_1->iface = SNDRV_HWDEP_IFACE_WMT;
	hwdep_1->ops.write = wmt_hwdep_write_1;
	hwdep_1->ops.read = wmt_hwdep_read_1;
}

