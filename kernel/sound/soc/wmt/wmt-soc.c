/*++ 
 * linux/sound/soc/wmt/wmt-soc.c
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
#include <linux/moduleparam.h>
#include <linux/device.h>

#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>
#include <sound/hwdep.h>

#include <asm/mach-types.h>
#include <mach/hardware.h>

#include "wmt-soc.h"
#include "wmt-pcm.h"
#include "wmt_hwdep.h"
#include "../codecs/wmt_vt1602.h"
#include "../codecs/vt1603.h"

extern int wmt_getsyspara(char *varname, unsigned char *varval, int *varlen);
extern void wmt_set_i2s_share_pin();
char wmt_codec_name[80];
char wmt_dai_name[80];
char wmt_rate[10];

#define AUDIO_NAME "WMT_SOC"
//#define WMT_SOC_DEBUG 1
//#define WMT_SOC_DEBUG_DETAIL 1

#ifdef WMT_SOC_DEBUG
#define DPRINTK(format, arg...) \
	printk(KERN_INFO AUDIO_NAME ": " format "\n" , ## arg)
#else
#define DPRINTK(format, arg...) do {} while (0)
#endif

#ifdef WMT_SOC_DEBUG_DETAIL
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

#define WMT_I2S_RATES  SNDRV_PCM_RATE_8000_96000

static struct snd_soc_card snd_soc_machine_wmt;

static int wmt_soc_primary_startup(struct snd_pcm_substream *substream)
{
	DBG_DETAIL();
	return 0;
}

static void wmt_soc_primary_shutdown(struct snd_pcm_substream *substream)
{
	DBG_DETAIL();
}

static int wmt_soc_primary_hw_params(struct snd_pcm_substream *substream,
	struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *codec_dai = rtd->codec_dai;
	struct snd_soc_dai *cpu_dai = rtd->cpu_dai;
	int ret = 0;

	DBG_DETAIL();
	
	if (strcmp(wmt_codec_name, "hwdac")) {
		/* Set codec DAI configuration */
		ret = snd_soc_dai_set_fmt(codec_dai, SND_SOC_DAIFMT_CBS_CFS |
				SND_SOC_DAIFMT_I2S |SND_SOC_DAIFMT_NB_NF);
		if (ret < 0)
			return ret;
	}
	

	/* Set cpu DAI configuration for I2S */
	ret = snd_soc_dai_set_fmt(cpu_dai, SND_SOC_DAIFMT_I2S 
				|SND_SOC_DAIFMT_MASTER_MASK | SND_SOC_DAIFMT_NB_NF);  
	if (ret < 0)
		return ret;

	if ((!strcmp(wmt_codec_name, "vt1602")) || (!strcmp(wmt_codec_name, "vt1603"))) {
		/* Set the codec system clock for DAC and ADC */
		if (!(params_rate(params) % 11025)) {
			ret = snd_soc_dai_set_sysclk(codec_dai, 0, 11289600,
						    SND_SOC_CLOCK_IN);		    
		}				    
		else {
			ret = snd_soc_dai_set_sysclk(codec_dai, 0, 12288000,
						    SND_SOC_CLOCK_IN);				    
	  }					    
	}

	return ret;
}

static int wmt_soc_second_startup(struct snd_pcm_substream *substream)
{
	DBG_DETAIL();
	return 0;
}

static void wmt_soc_second_shutdown(struct snd_pcm_substream *substream)
{
	DBG_DETAIL();
}

static int wmt_soc_second_hw_params(struct snd_pcm_substream *substream,
	struct snd_pcm_hw_params *params)
{
	DBG_DETAIL();
	return 0;
}

static int wmt_soc_dai_init(struct snd_soc_pcm_runtime *rtd)
{
	DBG_DETAIL();
	wmt_soc_hwdep_new(rtd->codec);
	return 0;
}

static int wmt_suspend_pre(struct snd_soc_card *card)
{
	snd_soc_dapm_disable_pin(&card->rtd->codec->dapm, "Left HP");
	snd_soc_dapm_disable_pin(&card->rtd->codec->dapm, "Right HP");
	snd_soc_dapm_disable_pin(&card->rtd->codec->dapm, "Left SPK");
	snd_soc_dapm_disable_pin(&card->rtd->codec->dapm, "Right SPK");
}

static int wmt_suspend_post(struct snd_soc_card *card)
{
	DBG_DETAIL();

	/* Disable BIT15:I2S clock, BIT4:ARFP clock, BIT3:ARF clock */
	PMCEU_VAL &= ~(BIT15 | BIT4 | BIT3);

	snd_soc_dapm_enable_pin(&card->rtd->codec->dapm, "Left HP");
	snd_soc_dapm_enable_pin(&card->rtd->codec->dapm, "Right HP");
	snd_soc_dapm_enable_pin(&card->rtd->codec->dapm, "Left SPK");
	snd_soc_dapm_enable_pin(&card->rtd->codec->dapm, "Right SPK");
	
	return 0;
}

static int wmt_resume_pre(struct snd_soc_card *card)
{
	/* Enable MCLK before VT1602 codec enable, otherwise the codec will be disabled. */
	
	/* set to 24.576MHz */
	auto_pll_divisor(DEV_I2S, CLK_ENABLE , 0, 0);
	auto_pll_divisor(DEV_I2S, SET_PLLDIV, 1, 24576);
	/* Enable BIT4:ARFP clock, BIT3:ARF clock */
	PMCEU_VAL |= (BIT4 | BIT3);
	/* Enable BIT2:AUD clock */
	PMCE3_VAL |= BIT2;

	wmt_set_i2s_share_pin();
	return 0;
}

static struct snd_soc_ops wmt_soc_primary_ops = {
	.startup = wmt_soc_primary_startup,
	.hw_params = wmt_soc_primary_hw_params,
	.shutdown = wmt_soc_primary_shutdown,
};

static struct snd_soc_ops wmt_soc_second_ops = {
	.startup = wmt_soc_second_startup,
	.hw_params = wmt_soc_second_hw_params,
	.shutdown = wmt_soc_second_shutdown,
};

/* Digital audio interface glue - connects codec <--> CPU */
static struct snd_soc_dai_link wmt_dai[] = {
	{
		.name = "HiFi",
		.stream_name = "HiFi",
		.platform_name = "wmt-audio-pcm.0",
		.init = wmt_soc_dai_init,
		.ops = &wmt_soc_primary_ops,
	},
	{
		.name = "Voice",
		.stream_name = "Voice",
		.platform_name = "wmt-pcm-dma.0",
		.cpu_dai_name = "wmt-pcm-controller.0",
		.codec_dai_name = "HWDAC",
		.codec_name = "wmt-i2s-hwdac.0",
		.ops = &wmt_soc_second_ops,
	},
};

/* Audio machine driver */
static struct snd_soc_card snd_soc_machine_wmt = {
	.name = "WMT_VT1609",
	.dai_link = wmt_dai,
	.num_links = ARRAY_SIZE(wmt_dai),
	.suspend_pre = wmt_suspend_pre,
	.suspend_post = wmt_suspend_post,
	.resume_pre = wmt_resume_pre,
};

static struct platform_device *wmt_snd_device;

static int __init wmt_soc_init(void)
{
	int ret, i;
	char buf[64];
	int len = ARRAY_SIZE(buf);
	
	if (wmt_getsyspara("wmt.audio.i2s", buf, &len) != 0) {
		strcpy(wmt_dai_name, "null");	
		strcpy(wmt_codec_name, "null");
	}
	else {
		strcpy(wmt_dai_name, "i2s");
	}

	if (strcmp(wmt_dai_name, "null")) {
		for (i = 0; i < 80; ++i) {
			if (buf[i] == ':')
				break;
			else
				wmt_codec_name[i] = buf[i];
		}
	}
	else {
		return -EINVAL;
	}

	// is wm8994, return and load wmt_wm8994 module
	if (strcmp(wmt_codec_name, "wm8994") == 0)
		return -ENODEV;

	info("dai_name=%s, codec_name=%s", wmt_dai_name, wmt_codec_name);
	

	wmt_i2s_dai.playback.rates = (SNDRV_PCM_RATE_48000 | SNDRV_PCM_RATE_8000);
	wmt_i2s_dai.capture.rates = (SNDRV_PCM_RATE_48000 | SNDRV_PCM_RATE_8000);
	wmt_dai[0].cpu_dai_name = "wmt-i2s.0";

	if (!strcmp(wmt_codec_name, "vt1602")) {		
		wmt_dai[0].codec_dai_name = "VT1602";
		wmt_dai[0].codec_name = "vt1602.1-001a";
	}
	else if (!strcmp(wmt_codec_name, "hwdac")) {
		wmt_dai[0].codec_dai_name = "HWDAC";
		wmt_dai[0].codec_name = "wmt-i2s-hwdac.0";
	}
	else if (!strcmp(wmt_codec_name, "vt1603")) {
		wmt_dai[0].codec_dai_name = "VT1603";
		wmt_dai[0].codec_name = "vt1603-codec";
	}

	/* Doing register process after plug-in */
	wmt_snd_device = platform_device_alloc("soc-audio", -1);
	if (!wmt_snd_device)
		return -ENOMEM;

	platform_set_drvdata(wmt_snd_device, &snd_soc_machine_wmt);
	
	ret = platform_device_add(wmt_snd_device);
	if (ret)
		goto err1;

	return 0;
	
err1:
	platform_device_put(wmt_snd_device);
	return ret;

}

static void __exit wmt_soc_exit(void)
{
	DBG_DETAIL();
	
	platform_device_unregister(wmt_snd_device);
}

module_init(wmt_soc_init);
module_exit(wmt_soc_exit);

MODULE_AUTHOR("WonderMedia Technologies, Inc.");
MODULE_DESCRIPTION("WMT [ALSA SoC/Machine] driver");
MODULE_LICENSE("GPL");
