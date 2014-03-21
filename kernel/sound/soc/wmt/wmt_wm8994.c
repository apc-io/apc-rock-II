/*
 * wmt_wm8994.c
 *
 * Copyright (C) 2010 Samsung Electronics Co.Ltd
 * Author: Chanwoo Choi <cw00.choi@samsung.com>
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 *
 */

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

#include "wmt-soc.h"
#include "wmt-pcm.h"
#include "wmt_hwdep.h"
#include "../codecs/wm8994.h"
#include <linux/mfd/wm8994/registers.h>
#include <linux/mfd/wm8994/core.h>

#include <linux/i2c.h>

static struct snd_soc_card wmt;
static struct platform_device *wmt_snd_device;
static int wmt_incall = 0;

extern int wmt_getsyspara(char *varname, unsigned char *varval, int *varlen);
extern void wmt_set_i2s_share_pin(void);

static const struct snd_pcm_hardware wmt_voice_hardware = {
	.info           = SNDRV_PCM_INFO_MMAP |
	                  SNDRV_PCM_INFO_MMAP_VALID |
	                  SNDRV_PCM_INFO_INTERLEAVED |
	                  SNDRV_PCM_INFO_PAUSE |
	                  SNDRV_PCM_INFO_RESUME,
	.formats        = SNDRV_PCM_FMTBIT_S16_LE,
	.rate_min       = 8000,
	.rate_max       = 8000,
	.period_bytes_min   = 16,
	.period_bytes_max   = 4 * 1024,
	.periods_min        = 2,
	.periods_max        = 16,
	.buffer_bytes_max   = 16 * 1024,
};

static int wmt_snd_suspend_post(struct snd_soc_card *card)
{
	/* Disable BIT15:I2S clock, BIT4:ARFP clock, BIT3:ARF clock */
	PMCEU_VAL &= ~(BIT15 | BIT4 | BIT3);
	return 0;
}

static int wmt_snd_resume_pre(struct snd_soc_card *card)
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

static int wmt_wm8994_init(struct snd_soc_pcm_runtime *rtd)
{
	struct snd_soc_dai *codec_dai = rtd->codec_dai;
	int ret = 0;
	unsigned int pll_in  = 48000;
	unsigned int pll_out = 12000000;

	/* Set the codec system clock for DAC and ADC */			
	ret = snd_soc_dai_set_pll(codec_dai, WM8994_FLL1, WM8994_FLL_SRC_LRCLK,
			pll_in, pll_out);
	if (ret < 0)
		return ret;
	ret = snd_soc_dai_set_sysclk(codec_dai, WM8994_SYSCLK_FLL1, pll_out,
			SND_SOC_CLOCK_IN);

	wmt_soc_hwdep_new(rtd->codec);

	return 0;
}

static int wmt_hifi_hw_params(struct snd_pcm_substream *substream,
		struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *codec_dai = rtd->codec_dai;
	struct snd_soc_dai *cpu_dai = rtd->cpu_dai;
	int ret = 0;
	unsigned int pll_in  = 48000;
	unsigned int pll_out = 12000000;

	/* Set codec DAI configuration */
	ret = snd_soc_dai_set_fmt(codec_dai, SND_SOC_DAIFMT_CBS_CFS | SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_NB_NF);
	if (ret< 0) {
		printk("<<ret:%d snd_soc_dai_set_fmt(codec) hifi\n", ret);
		return ret;
	}

	/* Set cpu DAI configuration for I2S */
	ret = snd_soc_dai_set_fmt(cpu_dai, SND_SOC_DAIFMT_I2S);
	if (ret < 0) {
		printk("<<ret:%d snd_soc_dai_set_fmt(cpu) hifi\n", ret);
		return ret;
	}

	/* Set the codec system clock for DAC and ADC */			
	ret = snd_soc_dai_set_pll(codec_dai, WM8994_FLL1, WM8994_FLL_SRC_LRCLK,
			pll_in, pll_out);
	if (ret < 0) {
		printk("<<ret:%d snd_soc_dai_set_pll hifi\n", ret);
		return ret;
	}
	ret = snd_soc_dai_set_sysclk(codec_dai, WM8994_SYSCLK_FLL1, pll_out,
			SND_SOC_CLOCK_IN);
	if (ret < 0)
		printk("<<ret:%d snd_soc_dai_set_sysclk hifi\n", ret);

	if (!wmt_incall)
		snd_soc_update_bits(codec_dai->codec, WM8994_CLOCKING_1, WM8994_SYSCLK_SRC, 0);
	
	return 0;
}

static int wmt_voice_startup(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	int ret;

	ret = snd_soc_set_runtime_hwparams(substream, &wmt_voice_hardware);

	/* Ensure that buffer size is a multiple of period size */
	ret = snd_pcm_hw_constraint_integer(runtime,SNDRV_PCM_HW_PARAM_PERIODS);
	return ret;
}

static int wmt_voice_hw_params(struct snd_pcm_substream *substream,
		struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *codec_dai = rtd->codec_dai;
	int ret = 0;
	unsigned int pll_in  = 2048000;
	unsigned int pll_out = 12288000;
	
	if (params_rate(params) != 8000)
		return -EINVAL;

	/* Set codec DAI configuration */
	ret = snd_soc_dai_set_fmt(codec_dai, SND_SOC_DAIFMT_DSP_A |
			SND_SOC_DAIFMT_IB_NF | SND_SOC_DAIFMT_CBS_CFS);
	if (ret < 0) {
		printk("<<ret:%d snd_soc_dai_set_fmt voice\n", ret);
		return ret;
	}

	/* Set the codec system clock for DAC and ADC */			
	ret = snd_soc_dai_set_pll(codec_dai, WM8994_FLL2, WM8994_FLL_SRC_BCLK,
			pll_in, pll_out);
	if (ret < 0) {
		printk("<<ret:%d snd_soc_dai_set_pll voice\n", ret);
		return ret;
	}
	ret = snd_soc_dai_set_sysclk(codec_dai, WM8994_SYSCLK_FLL2, pll_out,
			SND_SOC_CLOCK_IN);
	if (ret < 0)
		printk("<<ret:%d snd_soc_dai_set_sysclk voice\n", ret);

	wmt_incall = 1;
	return ret;
}

static int wmt_voice_hw_free(struct snd_pcm_substream *substream)
{
	wmt_incall = 0;
	return 0;
}

static struct snd_soc_ops wmt_hifi_ops = {
	.hw_params = wmt_hifi_hw_params,
};

static struct snd_soc_ops wmt_voice_ops = {
	.startup   = wmt_voice_startup,
	.hw_params = wmt_voice_hw_params,
	.hw_free   = wmt_voice_hw_free,
};

static struct snd_soc_dai_driver voice_dai[] = {
	{
		.name = "wmt-voice-dai",
		.playback = {
			.channels_min = 1,
			.channels_max = 2,
			.rates = SNDRV_PCM_RATE_8000,
			.formats = SNDRV_PCM_FMTBIT_S16_LE,},
		.capture = {
			.channels_min = 1,
			.channels_max = 2,
			.rates = SNDRV_PCM_RATE_8000,
			.formats = SNDRV_PCM_FMTBIT_S16_LE,},
	},
};

static struct snd_soc_dai_link wmt_dai[] = {
	{
		.name = "WM8994",
		.stream_name = "WM8994 HiFi",
		.cpu_dai_name = "wmt-i2s.0",
		.codec_dai_name = "wm8994-aif1",
		.platform_name = "wmt-audio-pcm.0",
		.codec_name = "wm8994-codec",
		.init = wmt_wm8994_init,
		.ops = &wmt_hifi_ops,
	},
	{
		.name = "WM8994 Voice",
		.stream_name = "Voice",
		.cpu_dai_name = "wmt-voice-dai",
		.codec_dai_name = "wm8994-aif2",
		.codec_name = "wm8994-codec",
		.ops = &wmt_voice_ops,
	},
};

static struct snd_soc_card wmt = {
	.name = "WMT_WM8994",
	.dai_link = wmt_dai,
	.num_links = ARRAY_SIZE(wmt_dai),
	.suspend_post = wmt_snd_suspend_post,
	.resume_pre = wmt_snd_resume_pre,
};

static int __init wmt_init(void)
{
	int ret;
	char buf[128];
	int len = ARRAY_SIZE(buf);

	if (wmt_getsyspara("wmt.audio.i2s", buf, &len) != 0)
		return -EINVAL;

	if (strncmp(buf, "wm8994", strlen("wm8994")))
		return -ENODEV;

	wmt_snd_device = platform_device_alloc("soc-audio", -1);
	if (!wmt_snd_device)
		return -ENOMEM;

	/* register voice DAI here */
	ret = snd_soc_register_dais(&wmt_snd_device->dev, voice_dai, ARRAY_SIZE(voice_dai));
	if (ret) {
		platform_device_put(wmt_snd_device);
		return ret;
	}

	platform_set_drvdata(wmt_snd_device, &wmt);
	ret = platform_device_add(wmt_snd_device);

	if (ret) {
		snd_soc_unregister_dai(&wmt_snd_device->dev);
		platform_device_put(wmt_snd_device);
	}

	return ret;
}

static void __exit wmt_exit(void)
{
	snd_soc_unregister_dai(&wmt_snd_device->dev);
	platform_device_unregister(wmt_snd_device);
}

module_init(wmt_init);
module_exit(wmt_exit);

MODULE_DESCRIPTION("ALSA SoC WM8994 WMT(wm8950)");
MODULE_AUTHOR("Loonzhong <Loonzhong@wondermedia.com.cn>");
MODULE_LICENSE("GPL");
