/*++ 
 * linux/sound/soc/wmt/wmt-soc.h
 * WonderMedia audio driver for ALSA
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

 
#ifndef __WMT_ASOC_H__
#define __WMT_ASOC_H__
#include <mach/dma.h>

struct audio_stream_a {
	char *id;		/* identification string */
	int stream_id;		/* numeric identification */
	dmach_t dmach;	        /* DMA channel number */
	struct dma_device_cfg_s dma_cfg;   /* DMA device config */
	int dma_dev;		/* dma number of that device */
	int dma_q_head;		/* DMA Channel Q Head */
	int dma_q_tail;		/* DMA Channel Q Tail */
	char dma_q_count;	/* DMA Channel Q Count */
	int active:1;		/* we are using this stream for transfer now */
	int period;		/* current transfer period */
	int periods;		/* current count of periods registerd in the DMA engine */
	spinlock_t dma_lock;	/* for locking in DMA operations */
	struct snd_pcm_substream *stream;	/* the pcm stream */
	unsigned linked:1;	/* dma channels linked */
	int offset;		/* store start position of the last period in the alsa buffer */
};

#define NUM_LINKS	1

extern struct snd_soc_dai_driver wmt_i2s_dai;


#endif
