/*++ 
 * linux/sound/soc/wmt/wmt-pcm.c
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

#include <linux/module.h>
#include <linux/init.h>
#include <linux/platform_device.h>

#include <linux/dma-mapping.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <linux/delay.h>

#include <asm/dma.h>
#include "wmt-pcm.h"
#include "wmt-soc.h"

#define NULL_DMA                ((dmach_t)(-1))

#define AUDIO_NAME "WMT_PCM"
//#define WMT_PCM_DEBUG 1
//#define WMT_PCM_DEBUG_DETAIL 1

#ifdef WMT_PCM_DEBUG
#define DPRINTK(format, arg...) \
	printk(KERN_INFO AUDIO_NAME ": " format "\n" , ## arg)
#else
#define DPRINTK(format, arg...) do {} while (0)
#endif

#ifdef WMT_PCM_DEBUG_DETAIL
#define DBG_DETAIL(format, arg...) \
	printk(KERN_INFO AUDIO_NAME ": [%s]" format "\n" , __FUNCTION__, ## arg)
#else
#define DBG_DETAIL(format, arg...) do {} while (0)
#endif

#define err(format, arg...) \
	printk(KERN_ERR AUDIO_NAME ": " format "\n" , ## arg)
#define info(format, arg...) \
	printk(KERN_INFO AUDIO_NAME ": " format "\n" , ## arg)
#define warn(format, arg...) \
	printk(KERN_WARNING AUDIO_NAME ": " format "\n" , ## arg)

static const struct snd_pcm_hardware wmt_pcm_hardware = {
	.info           = SNDRV_PCM_INFO_MMAP |
	                  SNDRV_PCM_INFO_MMAP_VALID |
	                  SNDRV_PCM_INFO_INTERLEAVED |
	                  SNDRV_PCM_INFO_PAUSE |
	                  SNDRV_PCM_INFO_RESUME,
	.formats        = SNDRV_PCM_FMTBIT_S16_LE,
	.rate_min       = 8000,
	.rate_max       = 96000,
	.period_bytes_min = 32,
	.period_bytes_max = 4 * 1024,
	.periods_min      = 2,
	.periods_max      = 128,
	.buffer_bytes_max = 64 * 1024,
};

struct wmt_runtime_data {
	spinlock_t			lock;
	struct wmt_pcm_dma_data	*dma_data;
	int				dma_ch;
	int				period_index;
};

struct snd_wfd_buffer {
	struct device *dev;
	unsigned char *area;	/* virtual pointer */
	dma_addr_t addr;		/* physical address */
	size_t bytes;			/* buffer size in bytes */
	size_t valuable_sz;		/* valuable size in wfd buffer */
	unsigned char *wr_ptr;
	unsigned char *rd_ptr;
	int enable;
};
static struct snd_wfd_buffer wfd_audbuf;

static int audio_dma_free(struct audio_stream_a *s);

dmach_t pcm_out_dmach = 0xFF;
struct dma_device_cfg_s *pcm_out_dma_cfg = NULL;

void wmt_pcm_wfd_start(void)
{
	/* allocate buffer for WFD support */
	wfd_audbuf.area = dma_alloc_writecombine(wfd_audbuf.dev, wfd_audbuf.bytes,
					   &wfd_audbuf.addr, GFP_KERNEL);

	if (!wfd_audbuf.area) {
		err("WFD_Aud allocate buffer fail");
	}
	else {
		info("*WFD_Aud enable");
		wfd_audbuf.enable = 1;
	}
		
	memset(wfd_audbuf.area, 0x0, wfd_audbuf.bytes);
	//info("&wfd_audbuf.addr=0x%x wfd_audbuf.addr=0x%x", &wfd_audbuf.addr, (unsigned int)wfd_audbuf.addr);
}
EXPORT_SYMBOL(wmt_pcm_wfd_start);

unsigned int wmt_pcm_wfd_get_buf(void)
{
	wfd_audbuf.valuable_sz = 0;
	wfd_audbuf.wr_ptr = wfd_audbuf.rd_ptr = wfd_audbuf.area;
	return (unsigned int)&wfd_audbuf.addr;
}
EXPORT_SYMBOL(wmt_pcm_wfd_get_buf);

void wmt_pcm_wfd_stop(void)
{
	wfd_audbuf.wr_ptr = wfd_audbuf.area;
	wfd_audbuf.enable = 0;

	dma_free_writecombine(wfd_audbuf.dev, wfd_audbuf.bytes,
			      wfd_audbuf.area, wfd_audbuf.addr);
	info("*WFD_Aud disable");
	return;
}
EXPORT_SYMBOL(wmt_pcm_wfd_stop);

int wmt_pcm_wfd_get_strm(WFDStrmInfo_t *info)
{
	if ((info->req_sz > wfd_audbuf.bytes) || (!wfd_audbuf.valuable_sz)) {
		//info("WFD read size=%d, Too Large!", info->req_sz);
		info->avail_sz = 0;
		return (int)info;
	}
	else if (wfd_audbuf.valuable_sz > info->req_sz) {
		info->avail_sz = info->req_sz;
		wfd_audbuf.valuable_sz -= info->req_sz;
	}
	else {
		info->avail_sz = wfd_audbuf.valuable_sz;
		wfd_audbuf.valuable_sz = 0;
	}
	
	info->buf_offset = wfd_audbuf.rd_ptr - wfd_audbuf.area;
	wfd_audbuf.rd_ptr += info->avail_sz;
	
	if (wfd_audbuf.rd_ptr >= wfd_audbuf.area + wfd_audbuf.bytes)
		wfd_audbuf.rd_ptr = wfd_audbuf.rd_ptr - wfd_audbuf.bytes;
	
	return (int)info;
}
EXPORT_SYMBOL(wmt_pcm_wfd_get_strm);

void wmt_pcm_wfd_update(char *src_buf, unsigned int chunksize)
{
	//info("wmt_pcm_wfd_update, 0x%x", src_buf[1024]);
	memcpy(wfd_audbuf.wr_ptr, src_buf, chunksize);
	wfd_audbuf.wr_ptr += chunksize;
					
	if (wfd_audbuf.wr_ptr == wfd_audbuf.area + wfd_audbuf.bytes)
		wfd_audbuf.wr_ptr = wfd_audbuf.area;

	wfd_audbuf.valuable_sz += chunksize;
	if (wfd_audbuf.valuable_sz >= wfd_audbuf.bytes) {
		wfd_audbuf.valuable_sz = wfd_audbuf.bytes;
		wfd_audbuf.rd_ptr = wfd_audbuf.wr_ptr;
	}	
}

/*
 *  Main dma routine, requests dma according where you are in main alsa buffer
 */
static void audio_process_dma(struct audio_stream_a *s)
{
	struct snd_pcm_substream *substream = s->stream;
	struct snd_pcm_runtime *runtime;
	unsigned int dma_size;
	unsigned int offset;
	dma_addr_t dma_base;
	int ret = 0;
	
	//DBG_DETAIL();
	DPRINTK("s: %d, dmach: %d. active: %d", (int)s, s->dmach, s->active);
	
	if (s->active) {
		substream = s->stream;
		runtime = substream->runtime;
		dma_size = frames_to_bytes(runtime, runtime->period_size);
		/*DPRINTK("frame_bits=%d, period_size=%d, dma_size 1=%d",
			runtime->frame_bits, (int)runtime->period_size, dma_size);*/
		if (dma_size > MAX_DMA_SIZE)
			dma_size = CUT_DMA_SIZE;
		offset = dma_size * s->period;

		/*DPRINTK("offset: 0x%x, ->dma_area: 0x%x, ->dma_addr: 0x%x, final addr: 0x%x",
			offset, (unsigned int)runtime->dma_area, runtime->dma_addr, runtime->dma_addr+offset);*/

		dma_base = __virt_to_phys((dma_addr_t)runtime->dma_area);

		//DPRINTK("dma address: 0x%x", dma_base+offset);
		/*DPRINTK("hw_ptr_interrupt: 0x%x, state: %d, hwptr: %u, applptr: %u, avail_min: %u", 
			(unsigned int)runtime->hw_ptr_interrupt, runtime->status->state, (unsigned int)runtime->status->hw_ptr,
			(unsigned int)runtime->control->appl_ptr, (unsigned int)runtime->control->avail_min);*/
		//DPRINTK("dmach: %u, dma_addr: %x, dma_size: %u", s->dmach, dma_base+offset, dma_size);

		if ((runtime->channels == 2 || runtime->channels == 1) && 
				(runtime->format == SNDRV_PCM_FORMAT_S16_LE)) {
			ret = wmt_start_dma(s->dmach, runtime->dma_addr + offset, 0, dma_size);
		}

		if (ret) {
			printk(KERN_ERR  "audio_process_dma: cannot queue DMA buffer (%i) \n", ret);
			return;
		}

		s->period++;
		s->period %= runtime->periods;
		s->periods++;
		s->offset = offset;
	}
}

/* 
 *  This is called when dma IRQ occurs at the end of each transmited block
 */
static void audio_dma_callback(void *data)
{
	struct audio_stream_a *s = data;
	struct snd_pcm_substream *substream;
	struct snd_pcm_runtime *runtime;
	unsigned int dma_size;
	unsigned int offset;
	int stream_id;
	
	//DBG_DETAIL();

	substream = s->stream;
	runtime = substream->runtime;
	dma_size = frames_to_bytes(runtime, runtime->period_size);
	stream_id = substream->pstr->stream;
	
	if (s->period > 0)
		offset = dma_size * (s->period - 1);
	else
		offset = dma_size * (s->periods - 1);

	if ((stream_id == SNDRV_PCM_STREAM_PLAYBACK) && (wfd_audbuf.enable)) {
		wmt_pcm_wfd_update(runtime->dma_area + offset, dma_size);
	}
	
	/* 
	 * If we are getting a callback for an active stream then we inform
	 * the PCM middle layer we've finished a period
	 */
	if (s->active)
		snd_pcm_period_elapsed(s->stream);

	spin_lock(&s->dma_lock);
	if (s->periods > 0) 
		s->periods--;
	
	audio_process_dma(s);
	spin_unlock(&s->dma_lock);
}

static int audio_dma_request(struct audio_stream_a *s, void (*callback) (void *))
{
	int err;
	err = 0;

	DBG_DETAIL();
	
	//DPRINTK("s pointer: %d, dmach: %d, id: %s, dma_dev: %d", (int)s, s->dmach, s->id, s->dma_dev);
	err = wmt_request_dma(&s->dmach, s->id, s->dma_dev, callback, s);
	if (err < 0)
		printk(KERN_ERR "Unable to grab audio dma 0x%x\n", s->dmach);
	
	if (!strcmp(s->id, "WMT I2S out")) {
		pcm_out_dmach =  s->dmach;
	}
	return err;
}

static void audio_setup_dma(struct audio_stream_a *s, int stream_id)
{
	struct snd_pcm_runtime *runtime = s->stream->runtime;

	if (stream_id == SNDRV_PCM_STREAM_PLAYBACK) {
		/* From memory to device */
		switch (runtime->channels * runtime->format) {
		case 1:
			s->dma_cfg.DefaultCCR = I2S_TX_DMA_8BITS_CFG;     /* setup 1 bytes*/
			break ;
		case 2:
			s->dma_cfg.DefaultCCR = I2S_TX_DMA_16BITS_CFG;    /* setup 2 bytes*/
			break ;
		case 4:
			s->dma_cfg.DefaultCCR = I2S_TX_DMA_32BITS_CFG;    /* setup 4 byte*/
			break ;
		}
	}
	else {
		/* From device to memory */
		switch (runtime->channels * runtime->format) {
		case 1:
			s->dma_cfg.DefaultCCR = I2S_RX_DMA_8BITS_CFG ;     /* setup 1 bytes*/
			break ;
		case 2:
			s->dma_cfg.DefaultCCR = I2S_RX_DMA_16BITS_CFG ;    /* setup 2 bytes*/
			break ;
		case 4:
			s->dma_cfg.DefaultCCR = I2S_RX_DMA_32BITS_CFG ;    /* setup 4 byte*/
			break ;
		}
	}

	s->dma_cfg.ChunkSize = 1;
	if (stream_id == SNDRV_PCM_STREAM_PLAYBACK) {
		pcm_out_dma_cfg = &s->dma_cfg;
	}
	
	//DPRINTK("s pointer: %d. audio dma %d cfg.DefaultCCR 0x%x ", (int)s, s->dmach, (unsigned int)s->dma_cfg.DefaultCCR);
	//DPRINTK("cfg.ChunkSize 0x%x ", s->dma_cfg.ChunkSize);
	wmt_setup_dma(s->dmach, s->dma_cfg) ;
}

static int audio_dma_free(struct audio_stream_a *s)
{
	int err = 0;
	DBG_DETAIL();
	wmt_free_dma(s->dmach);
	s->dmach = NULL_DMA;
	pcm_out_dma_cfg = NULL;
	return err;
}

/*
 * this stops the dma and clears the dma ptrs
 */
static void audio_stop_dma(struct audio_stream_a *s)
{
	unsigned long flags;
	DBG_DETAIL();
	local_irq_save(flags);
	s->active = 0;
	s->period = 0;
	s->periods = 0;
	s->offset = 0;
	wmt_stop_dma(s->dmach);
	wmt_clear_dma(s->dmach);
	local_irq_restore(flags);
}

/* this may get called several times by oss emulation */
static int wmt_pcm_hw_params(struct snd_pcm_substream *substream,
			      struct snd_pcm_hw_params *params)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	int err = 0;
	DBG_DETAIL();
	snd_pcm_set_runtime_buffer(substream, &substream->dma_buffer);
	runtime->dma_bytes = params_buffer_bytes(params);
	return err;
}

static int wmt_pcm_hw_free(struct snd_pcm_substream *substream)
{
	DBG_DETAIL();
	snd_pcm_set_runtime_buffer(substream, NULL);
	return 0;
}

static int wmt_pcm_prepare(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	int stream_id = substream->pstr->stream;
	struct audio_stream_a *prtd = runtime->private_data;
	struct audio_stream_a *s = &prtd[stream_id];

	DBG_DETAIL();

	s->period = 0;
	s->periods = 0;
	s->offset = 0;
	audio_setup_dma(s, stream_id);

	return 0;
}

static int wmt_pcm_trigger(struct snd_pcm_substream *substream, int cmd)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	int stream_id = substream->pstr->stream;
	struct audio_stream_a *prtd = runtime->private_data;
	struct audio_stream_a *s = &prtd[stream_id];
	int ret = 0;

	DPRINTK("wmt_pcm_trigger Enter, cmd=%d", cmd);
	
	spin_lock(&s->dma_lock);
	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
	case SNDRV_PCM_TRIGGER_RESUME:
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
		s->active = 1;
		audio_process_dma(s);
		break;

	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_SUSPEND:
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
		s->active = 0;
		audio_stop_dma(s);
		break;
	default:
		ret = -EINVAL;
	}
	spin_unlock(&s->dma_lock);

	return ret;
}

static snd_pcm_uframes_t wmt_pcm_pointer(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct audio_stream_a *prtd = runtime->private_data;
	int stream_id = substream->pstr->stream;
	struct audio_stream_a *s = &prtd[stream_id];
	dma_addr_t ptr;
	snd_pcm_uframes_t offset = 0;
	static snd_pcm_uframes_t last_offset = 0;
	
	ptr = wmt_get_dma_pos(s->dmach);

	if ((runtime->channels == 1 || runtime->channels == 2) && (runtime->format == SNDRV_PCM_FORMAT_S16_LE)) {
		offset = bytes_to_frames(runtime, ptr - runtime->dma_addr);
	}
	
	if ((offset < last_offset) && ((last_offset - offset) < runtime->period_size) &&
			(last_offset != runtime->buffer_size)) {
        snd_pcm_uframes_t old_offset = offset;
		if (last_offset < runtime->period_size)
			offset = runtime->period_size;
		else {
			offset = runtime->period_size *
				((last_offset / runtime->period_size) + 1);
		}
        printk("last_offset %d, old offset %d, new offset %d\n", (int)last_offset, (int)old_offset, (int)offset);
	}
	last_offset = offset;	

	if (offset >= runtime->buffer_size)
		offset = 0;

	spin_lock(&s->dma_lock);

	if (s->periods > 0 && s->periods < 2) {
		if (stream_id == SNDRV_PCM_STREAM_PLAYBACK) {
			if (snd_pcm_playback_hw_avail(runtime) >= 2 * runtime->period_size)
				audio_process_dma(s);
		}
		else {
			if (snd_pcm_capture_hw_avail(runtime) >= 2* runtime->period_size)
				audio_process_dma(s);
		}
		
	}
	spin_unlock(&s->dma_lock);

	//DPRINTK("offset = %x", (unsigned int)offset);
	return offset;
}

static int wmt_pcm_open(struct snd_pcm_substream *substream)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *cpu_dai = rtd->cpu_dai;
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct audio_stream_a *s = runtime->private_data;
	int ret;

	DBG_DETAIL();
	
	if (!cpu_dai->active) {
		audio_dma_request(&s[0], audio_dma_callback);
		audio_dma_request(&s[1], audio_dma_callback);
	}

	snd_soc_set_runtime_hwparams(substream, &wmt_pcm_hardware);

	/* Ensure that buffer size is a multiple of period size */
	ret = snd_pcm_hw_constraint_integer(runtime, SNDRV_PCM_HW_PARAM_PERIODS);
	if (ret < 0)
		goto out;

out:
	return ret;
}

static int wmt_pcm_close(struct snd_pcm_substream *substream)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *cpu_dai = rtd->cpu_dai;
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct audio_stream_a *s = runtime->private_data;

	DBG_DETAIL();
	
	if (!cpu_dai->active) {
		audio_dma_free(&s[0]);
		audio_dma_free(&s[1]);
	}

	return 0;
}

static int wmt_pcm_mmap(struct snd_pcm_substream *substream,
	struct vm_area_struct *vma)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	
	DBG_DETAIL();
	
	return dma_mmap_writecombine(substream->pcm->card->dev, vma,
				     runtime->dma_area,
				     runtime->dma_addr,
				     runtime->dma_bytes);
}

static struct snd_pcm_ops wmt_pcm_ops = {
	.open		= wmt_pcm_open,
	.close		= wmt_pcm_close,
	.ioctl		= snd_pcm_lib_ioctl,
	.hw_params	= wmt_pcm_hw_params,
	.hw_free	= wmt_pcm_hw_free,
	.prepare	= wmt_pcm_prepare,
	.trigger	= wmt_pcm_trigger,
	.pointer	= wmt_pcm_pointer,
	.mmap		= wmt_pcm_mmap,
};

static u64 wmt_pcm_dmamask = DMA_BIT_MASK(32);

static int wmt_pcm_preallocate_dma_buffer(struct snd_pcm *pcm,
	int stream)
{
	struct snd_pcm_substream *substream = pcm->streams[stream].substream;
	struct snd_dma_buffer *buf = &substream->dma_buffer;
	size_t size = wmt_pcm_hardware.buffer_bytes_max;

	DBG_DETAIL();
	
	buf->dev.type = SNDRV_DMA_TYPE_DEV;
	buf->dev.dev = pcm->card->dev;
	buf->private_data = NULL;
	buf->area = dma_alloc_writecombine(pcm->card->dev, size,
					&buf->addr, GFP_KERNEL);
	
	if (stream == SNDRV_PCM_STREAM_PLAYBACK) {
		info("stream = %d, buf_addr = %x", stream, buf->addr);

		wfd_audbuf.bytes = size;
		wfd_audbuf.dev = pcm->card->dev;
		wfd_audbuf.enable = 0;
	}
	
	if (!buf->area)
		return -ENOMEM;

	buf->bytes = size;
	
	return 0;
}

static void wmt_pcm_free_dma_buffers(struct snd_pcm *pcm)
{
	struct snd_pcm_substream *substream;
	struct snd_dma_buffer *buf;
	int stream;

	DBG_DETAIL();
	
	for (stream = 0; stream < 2; stream++) {
		substream = pcm->streams[stream].substream;
		if (!substream)
			continue;

		buf = &substream->dma_buffer;
		if (!buf->area)
			continue;

		dma_free_writecombine(pcm->card->dev, buf->bytes, buf->area, buf->addr);
		
		buf->area = NULL;
	}
}

int wmt_pcm_new(struct snd_soc_pcm_runtime *rtd)
{
	struct snd_card *card = rtd->card->snd_card;
	struct snd_pcm *pcm = rtd->pcm;
	int ret = 0;

	DBG_DETAIL();
	
	if (!card->dev->dma_mask)
		card->dev->dma_mask = &wmt_pcm_dmamask;
	if (!card->dev->coherent_dma_mask)
		card->dev->coherent_dma_mask = DMA_BIT_MASK(32);

	if (pcm->streams[SNDRV_PCM_STREAM_PLAYBACK].substream) {
		ret = wmt_pcm_preallocate_dma_buffer(pcm,
			SNDRV_PCM_STREAM_PLAYBACK);
		if (ret)
			goto out;
	}

	if (pcm->streams[SNDRV_PCM_STREAM_CAPTURE].substream) {
		ret = wmt_pcm_preallocate_dma_buffer(pcm,
			SNDRV_PCM_STREAM_CAPTURE);
		if (ret)
			goto out;
	}

out:
	/* free preallocated buffers in case of error */
	if (ret)
		wmt_pcm_free_dma_buffers(pcm);
	
	return ret;
}

#ifdef CONFIG_PM
static int wmt_pcm_suspend(struct snd_soc_dai *dai)
{
	struct snd_pcm_runtime *runtime = dai->runtime;
	struct audio_stream_a *prtd;
	struct audio_stream_a *s;

	DBG_DETAIL();
	
	if (!runtime)
		return 0;

	prtd = runtime->private_data;
	s = &prtd[SNDRV_PCM_STREAM_PLAYBACK];

	if (s->active) {
		udelay(5);
		wmt_stop_dma(s->dmach);
		/*
		wmt_clear_dma(s->dmach);
		audio_stop_dma(s);
		*/
	}

	s = &prtd[SNDRV_PCM_STREAM_CAPTURE];

	if (s->active) {
		udelay(5);
		wmt_stop_dma(s->dmach);
		/*
		wmt_clear_dma(s->dmach);
		audio_stop_dma(s);
		*/
	}

	return 0;
}

static int wmt_pcm_resume(struct snd_soc_dai *dai)
{
	struct snd_pcm_runtime *runtime = dai->runtime;
	struct audio_stream_a *prtd;
	struct audio_stream_a *s;

	DBG_DETAIL();
	
	if (!runtime) {
		if ((pcm_out_dmach != 0xFF) && (pcm_out_dma_cfg != NULL)) {
			wmt_setup_dma(pcm_out_dmach, *pcm_out_dma_cfg);
		}
		return 0;
	}
	
	prtd = runtime->private_data;
	s = &prtd[SNDRV_PCM_STREAM_PLAYBACK];
	audio_setup_dma(s, SNDRV_PCM_STREAM_PLAYBACK);
	
	if (s->active) {
		wmt_resume_dma(s->dmach) ;
	}
	
	s = &prtd[SNDRV_PCM_STREAM_CAPTURE];
	audio_setup_dma(s, SNDRV_PCM_STREAM_CAPTURE);

	if (s->active) {
		wmt_resume_dma(s->dmach) ;
	}

	return 0;
}
#else
#define wmt_pcm_suspend	NULL
#define wmt_pcm_resume		NULL
#endif

static struct snd_soc_platform_driver wmt_soc_platform = {
	.ops		= &wmt_pcm_ops,
	.pcm_new	= wmt_pcm_new,
	.pcm_free	= wmt_pcm_free_dma_buffers,
	.suspend	= wmt_pcm_suspend,
	.resume		= wmt_pcm_resume,
};

static int __devinit wmt_pcm_platform_probe(struct platform_device *pdev)
{
	DBG_DETAIL();
	
	return snd_soc_register_platform(&pdev->dev, &wmt_soc_platform);
}

static int __devexit wmt_pcm_platform_remove(struct platform_device *pdev)
{
	DBG_DETAIL();
	
	snd_soc_unregister_platform(&pdev->dev);
	return 0;
}

static struct platform_driver wmt_pcm_driver = {
	.driver = {
			.name = "wmt-audio-pcm",
			.owner = THIS_MODULE,
	},

	.probe = wmt_pcm_platform_probe,
	.remove = __devexit_p(wmt_pcm_platform_remove),
};

module_platform_driver(wmt_pcm_driver);

MODULE_AUTHOR("WonderMedia Technologies, Inc.");
MODULE_DESCRIPTION("WMT [ALSA SoC/pcm] driver");
MODULE_LICENSE("GPL");
