/*++ 
 * linux/sound/soc/wmt/wmt_hwdep.h
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

/*
 * ioctls for Hardware Dependant Interface
 */
#ifndef __WMT_HWDEP_H__
#define __WMT_HWDEP_H__

#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>
#include <sound/hwdep.h>

#include <asm/mach-types.h>
#include <mach/hardware.h>

struct wmt_soc_vt1603_info {
    u16 reg_offset;
    u16 reg_value;
};//add 2013-9-2 for vt1603 eq apk

#define WMT_SOC_IOCTL_HDMI			_IOWR('H', 0x10, int)
#define WMT_SOC_IOCTL_WFD_START		_IOWR('H', 0x20, int)
#define WMT_SOC_IOCTL_GET_STRM		_IOWR('H', 0x30, int)
#define WMT_SOC_IOCTL_WFD_STOP		_IOWR('H', 0x40, int)
#define WMT_SOC_IOCTL_VT1603_RD		_IOWR('H', 0x50, int)
#define WMT_SOC_IOCTL_VT1603_WR		_IOWR('H', 0x60, int)
#define WMT_SOC_IOCTL_CH_SEL		_IOWR('H', 0x70, int)
void wmt_soc_hwdep_new(struct snd_soc_codec *codec);

extern int vt1603_hwdep_ioctl(u8 rw_flag, u16 offset, u16 value);
extern void wmt_i2s_ch_sel(int ch_sel_num);
#endif
