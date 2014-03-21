/*++
	linux/arch/arm/mach-wmt/generic.c

	wmt generic architecture level codes
	Copyright (c) 2013  WonderMedia Technologies, Inc.

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

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/pm.h>
#include <linux/cpufreq.h>
#include <linux/ioport.h>
#include <linux/platform_device.h>

#include <mach/hardware.h>
#include <mach/wmt_secure.h>
#include <asm/system.h>
#include <asm/pgtable.h>
#include <asm/mach/map.h>
#include <asm/irq.h>
#include <asm/sizes.h>
#include <linux/i2c.h>

#include <linux/mfd/wm8994/pdata.h>
#include <linux/regulator/fixed.h>
#include <linux/regulator/machine.h>

#include "generic.h"
#include <linux/spi/spi.h>

#ifdef CONFIG_WMT_NEWSPI_SUPPORT 
#include <mach/wmt-spi.h>
#endif

#ifdef CONFIG_WMT_NEWSPI1_SUPPORT 
#include <mach/wmt-spi.h>
#endif

#include <asm/hardware/cache-l2x0.h>
#include <mach/wmt_env.h>

extern void enable_user_access(void);

/* TODO*/
#define PMHC_HIBERNATE 0x205
extern void __init wmt_gpio_init(void);

extern void wmt_power_up_debounce_value(void);
extern void wmt_restart(char mode, const char *cmd);
extern void (*arm_pm_restart)(char str, const char *cmd);
static void wmt_power_off(void)
{
#ifdef CONFIG_PM
	/*set power button debounce value*/
	wmt_power_up_debounce_value();
#endif
	mdelay(100);
	local_irq_disable();

	*(volatile unsigned int *)0xfe018008 |= 0x03030303; //scu output pm	
	
#ifndef CONFIG_SMP
	PMCEU_VAL |= 0x00800000;//sf boot, enable sf for single core pm
	mdelay(1);
#endif
	
	/*
	 * Set scratchpad to zero, just in case it is used as a restart
	 * address by the bootloader. Since PB_RESUME button has been
	 * set to be one of the wakeup sources, clean the resume address
	 * will cause zacboot to issue a SW_RESET, for design a behavior
	 * to let PB_RESUME button be a power on button.
	 *
	 * Also force to disable watchdog timer, if it has been enabled.
	 */
	HSP0_VAL = 0;
	OSTW_VAL &= ~OSTW_WE;

	/*
	 * Well, I cannot power-off myself,
	 * so try to enter power-off suspend mode.
	 */
	
	//for single core
#ifndef CONFIG_SMP
	HSP7_VAL = 0xffffffb8;
	while(HSP7_VAL != 0xffffffb8);
	asm("sev" : : "r" (0));
#endif		
		
	//PMWTC_VAL = 0x2000;//DCDET falling
	*(unsigned char *)0xfe13005c = 0x0;
#if 0	
	PMWT_VAL = 0x40000000;
	mdelay(1);
	PMWS_VAL = PMWS_VAL;
	mdelay(1);
	PMWE_VAL = 0x00004080;//DCDET + PWRBTN
	mdelay(1);
	WK_TRG_EN_VAL = 0x00004080;//DCDET + PWRBTN

	if (DCDET_STS_VAL & 0x100)
		PMHC_VAL = PMHC_SUSPEND;
//#else
	if (DCDET_STS_VAL & 0x100)
		PMSR_VAL = PMSR_SWR;
		
	else		
#endif		
	PMHC_VAL = PMHC_HIBERNATE;
	//asm("mcr%? p15, 0, %0, c7, c0, 4" : : "r" (0));		/* Force ARM to idle mode*/
	do {
		asm("wfi" : : "r" (0));		/* Force ARM to idle mode*/
	} while(1);
}

static struct resource wmt_uart0_resources[] = {
	[0] = {
		.start  = UART0_BASE_ADDR,
		.end    = (UART0_BASE_ADDR + 0xFFFF),
		.flags  = IORESOURCE_MEM,
	},
};

static struct resource wmt_uart1_resources[] = {
	[0] = {
		.start  = UART1_BASE_ADDR,
		.end    = (UART1_BASE_ADDR + 0xFFFF),
		.flags  = IORESOURCE_MEM,
	},
};

#ifdef CONFIG_UART_2_3_ENABLE
static struct resource wmt_uart2_resources[] = {
	[0] = {
		.start  = UART2_BASE_ADDR,
		.end    = (UART2_BASE_ADDR + 0xFFFF),
		.flags  = IORESOURCE_MEM,
	},
};

static struct resource wmt_uart3_resources[] = {
	[0] = {
		.start  = UART3_BASE_ADDR,
		.end    = (UART3_BASE_ADDR + 0xFFFF),
		.flags  = IORESOURCE_MEM,
	},
};
#endif

static struct platform_device wmt_uart0_device = {
	.name           = "uart",
	.id             = 0,
	.num_resources  = ARRAY_SIZE(wmt_uart0_resources),
	.resource       = wmt_uart0_resources,
};

static struct platform_device wmt_uart1_device = {
	.name           = "uart",
	.id             = 1,
	.num_resources  = ARRAY_SIZE(wmt_uart1_resources),
	.resource       = wmt_uart1_resources,
};

#ifdef CONFIG_UART_2_3_ENABLE
static struct platform_device wmt_uart2_device = {
	.name           = "uart",
	.id             = 2,
	.num_resources  = ARRAY_SIZE(wmt_uart2_resources),
	.resource       = wmt_uart2_resources,
};

static struct platform_device wmt_uart3_device = {
	.name           = "uart",
	.id             = 3,
	.num_resources  = ARRAY_SIZE(wmt_uart3_resources),
	.resource       = wmt_uart3_resources,
};
#endif

static struct resource wmt_sf_resources[] = {
	[0] = {
		.start  = SF_MEM_CTRL_CFG_BASE_ADDR,
		.end    = SF_MEM_CTRL_CFG_BASE_ADDR + 0x3FF,
		.flags  = IORESOURCE_MEM,
	},
};

static struct platform_device wmt_sf_device = {
	.name           = "sf",
	.id             = 0,
	.num_resources  = ARRAY_SIZE(wmt_sf_resources),
	.resource       = wmt_sf_resources,
};

#ifdef CONFIG_MTD_WMT_NOR
static struct resource wmt_nor_resources[] = {
	[0] = {
		.start  = NOR_CTRL_CFG_BASE_ADDR,
		.end    = NOR_CTRL_CFG_BASE_ADDR + 0x3FF,
		.flags  = IORESOURCE_MEM,
	},
};

static struct platform_device wmt_nor_device = {
	.name           = "nor",
	.id             = 0,
	.num_resources  = ARRAY_SIZE(wmt_nor_resources),
	.resource       = wmt_nor_resources,
};
#endif

static struct resource wmt_nand_resources[] = {
	[0] = {
		.start  = NF_CTRL_CFG_BASE_ADDR,
		.end    = NF_CTRL_CFG_BASE_ADDR + 0x3FF,
		.flags  = IORESOURCE_MEM,
	},
};

static u64 wmt_nand_dma_mask = 0xffffffffUL;

static struct platform_device wmt_nand_device = {
	.name           = "nand",
	.id             = 0,
	.dev            = {
	.dma_mask 		= &wmt_nand_dma_mask,
	.coherent_dma_mask = ~0,
	},
	.num_resources  = ARRAY_SIZE(wmt_nand_resources),
	.resource       = wmt_nand_resources,
};
static struct resource wmt_i2s_resources[] = {
    [0] = {
		.start  = 0xD80ED800,
		.end    = 0xD80EDBFF,
		.flags  = IORESOURCE_MEM,
    },
};

static struct resource wmt_pcm_resources[] = {
    [0] = {
		.start  = 0xD82D0000,
		.end    = 0xD82D0000 + 0x4f,
		.flags  = IORESOURCE_MEM,
    },
};
static u64 wmt_i2s_dma_mask = 0xffffffffUL;
static u64 wmt_pcm_dma_mask = 0xffffffffUL;

static struct platform_device wmt_i2s_device = {
	.name           = "wmt-i2s",
	.id             = 0,
	.dev            = {
	.dma_mask 		= &wmt_i2s_dma_mask,
	.coherent_dma_mask = ~0,
	},
	.num_resources  = ARRAY_SIZE(wmt_i2s_resources),
	.resource       = wmt_i2s_resources,
};

static struct platform_device wmt_pcm_controller_device = {
	.name           = "wmt-pcm-controller",
	.id             = 0,
	.dev            = {
	.dma_mask 		= &wmt_pcm_dma_mask,
	.coherent_dma_mask = ~0,
	},
	.num_resources  = ARRAY_SIZE(wmt_pcm_resources),
	.resource       = wmt_pcm_resources,
};

static struct platform_device wmt_pcm_dma_device = {
	.name           = "wmt-pcm-dma",
	.id             = 0,
};
static struct platform_device wmt_aud_pcm_device = {
	.name           = "wmt-audio-pcm",
	.id             = 0,
};

static struct platform_device wmt_i2s_hwdac_device = {
	.name           = "wmt-i2s-hwdac",
	.id             = 0,
};

static struct platform_device wmt_switch_device = {
	.name = "wmt-switch",
	.id   = 0,
};

static struct resource wmt_pwm_resources[] = {
    [0] = {
		.start  = 0xD8220000,
		.end    = 0xD8220000 + 0x44,
		.flags  = IORESOURCE_MEM,
    },
};

struct platform_device wm8880_device_pwm = {
	.name		= "wm8880-pwm",
	.id		= 0,
	.num_resources     = ARRAY_SIZE(wmt_pwm_resources),
	.resource          = wmt_pwm_resources,
};

#ifdef CONFIG_WMT_NEWSPI_SUPPORT
static struct spi_board_info wmt_spi_board_info[] = {
};
#endif

#ifdef CONFIG_WMT_NEWSPI1_SUPPORT
static struct spi_board_info wmt_spi1_board_info[] = {
};
#endif

#ifdef CONFIG_WMT_NEWSPI_SUPPORT
static struct wmt_spi_hw wmt_spi_info = {
	/* spi on wmt can support dma */
	.dma_support       = SPI_DMA_ENABLE,
	/* can support 4 slaves when WMT spi as master */
	.num_chipselect    = MAX_SPI_SLAVE,
	/* wmt spi support 16bits_per_word? i'm not sure */
	.bits_per_word_en  = BITS8_PER_WORD_EN,
	/* wmt spi can support multi-master also, but it seems we do not need it */
	.port_mode         = PORT_MODE_PTP,
	/* ssn driven low when enable */
	.ssn_ctrl          = SSN_CTRL_HARDWARE,
	/* actual 36bytes, but we use 32bytes */
	.fifo_size         = SPI_FIFO_SIZE,
	/* 4Kbytes, same as the DMA */
	.max_transfer_length = SPI_MAX_TRANSFER_LENGTH,
	/* it's really needed? i'm not sure   */
	.min_freq_hz       = SPI_MIN_FREQ_HZ,
	/* max freq 100Mhz */
	.max_freq_hz       = SPI_MAX_FREQ_HZ,
};

static struct resource wmt_spi_resources[] = {
	[0] = {
		.start = SPI0_BASE_ADDR,
		.end   = SPI0_BASE_ADDR + 0xFFFF,
		.flags = IORESOURCE_MEM,
	      },
	[1] = {
		.start = IRQ_SPI0,
		.end   = IRQ_SPI0,
		.flags = IORESOURCE_IRQ,
	      },
};

static u64 wmt_spi_dma_mask = 0xFFFFFFFFUL;

static struct platform_device wmt_spi_device = {
	.name              = "wmt_spi_0",
	.id                = 0,
	.dev               = {
		.dma_mask          = &wmt_spi_dma_mask,
		.coherent_dma_mask = ~0,
		.platform_data     = &wmt_spi_info,
	},
	.num_resources     = ARRAY_SIZE(wmt_spi_resources),
	.resource          = wmt_spi_resources,
};
#endif

#ifdef CONFIG_WMT_NEWSPI1_SUPPORT
static struct wmt_spi_hw wmt_spi1_info = {
	/* spi on wmt can support dma */
	.dma_support       = SPI_DMA_ENABLE,
	/* can support 4 slaves when wmt spi as master */
	.num_chipselect    = MAX_SPI_SLAVE,
	/* wmt spi support 16bits_per_word? i'm not sure */
	.bits_per_word_en  = BITS8_PER_WORD_EN,
	/* wmt spi can support multi-master also, but it seems we do not need it */
	.port_mode         = PORT_MODE_PTP,
	/* ssn driven low when enable */
	.ssn_ctrl          = SSN_CTRL_HARDWARE,
	/* actual 36bytes, but we use 32bytes */
	.fifo_size         = SPI_FIFO_SIZE,
	/* 4Kbytes, same as the DMA */
	.max_transfer_length = SPI_MAX_TRANSFER_LENGTH,
	/* it's really needed? i'm not sure   */
	.min_freq_hz       = SPI_MIN_FREQ_HZ,
	/* max freq 100Mhz */
	.max_freq_hz       = SPI_MAX_FREQ_HZ,
};

static struct resource wmt_spi1_resources[] = {
	[0] = {
		.start = SPI1_BASE_ADDR,
		.end   = SPI1_BASE_ADDR + 0x0000FFFF,
		.flags = IORESOURCE_MEM,
	      },
	[1] = {
		.start = IRQ_SPI1,
		.end   = IRQ_SPI1,
		.flags = IORESOURCE_IRQ,
	      },
};

static u64 wmt_spi1_dma_mask = 0xFFFFFFFFUL;

static struct platform_device wmt_spi1_device = {
	.name              = "wmt_spi_1",
	.id                = 1,
	.dev               = {
		.dma_mask          = &wmt_spi1_dma_mask,
		.coherent_dma_mask = ~0,
		.platform_data     = &wmt_spi1_info,
	},
	.num_resources     = ARRAY_SIZE(wmt_spi1_resources),
	.resource          = wmt_spi1_resources,
};
#endif

#ifdef CONFIG_DRM_MALI
static struct platform_device wmt_mali_drm_device = {
	.name = "mali_drm",
	.id   = -1,
};
#endif

#ifdef CONFIG_I2S_CODEC_WM8994
static struct regulator_consumer_supply wm8994_fixed_voltage0_supplies[] = {
	REGULATOR_SUPPLY("DBVDD", "4-001a"),
	REGULATOR_SUPPLY("AVDD2", "4-001a"),
	REGULATOR_SUPPLY("CPVDD", "4-001a"),
};

static struct regulator_consumer_supply wm8994_fixed_voltage1_supplies[] = {
	REGULATOR_SUPPLY("SPKVDD1", "4-001a"),
	REGULATOR_SUPPLY("SPKVDD2", "4-001a"),
};

static struct regulator_init_data wm8994_fixed_voltage0_init_data = {
	.constraints = {
		.always_on = 1,
	},
	.num_consumer_supplies	= ARRAY_SIZE(wm8994_fixed_voltage0_supplies),
	.consumer_supplies	= wm8994_fixed_voltage0_supplies,
};

static struct regulator_init_data wm8994_fixed_voltage1_init_data = {
	.constraints = {
		.always_on = 1,
	},
	.num_consumer_supplies	= ARRAY_SIZE(wm8994_fixed_voltage1_supplies),
	.consumer_supplies	= wm8994_fixed_voltage1_supplies,
};

static struct fixed_voltage_config wm8994_fixed_voltage0_config = {
	.supply_name	= "VCC_1.8V_PDA",
	.microvolts	= 1800000,
	.gpio		= -EINVAL,
	.init_data	= &wm8994_fixed_voltage0_init_data,
};

static struct fixed_voltage_config wm8994_fixed_voltage1_config = {
	.supply_name	= "V_BAT",
	.microvolts	= 3700000,
	.gpio		= -EINVAL,
	.init_data	= &wm8994_fixed_voltage1_init_data,
};

static struct platform_device wm8994_fixed_voltage0 = {
	.name		= "reg-fixed-voltage",
	.id		= 0,
	.dev		= {
		.platform_data	= &wm8994_fixed_voltage0_config,
	},
};

static struct platform_device wm8994_fixed_voltage1 = {
	.name		= "reg-fixed-voltage",
	.id		= 1,
	.dev		= {
		.platform_data	= &wm8994_fixed_voltage1_config,
	},
};

static struct regulator_consumer_supply wm8994_avdd1_supply =
	REGULATOR_SUPPLY("AVDD1", "4-001a");

static struct regulator_consumer_supply wm8994_dcvdd_supply =
	REGULATOR_SUPPLY("DCVDD", "4-001a");

static struct regulator_init_data wm8994_ldo1_data = {
	.constraints	= {
		.name		= "AVDD1_3.0V",
	},
	.num_consumer_supplies	= 1,
	.consumer_supplies	= &wm8994_avdd1_supply,
};

static struct regulator_init_data wm8994_ldo2_data = {
	.constraints	= {
		.name		= "DCVDD_1.0V",
	},
	.num_consumer_supplies	= 1,
	.consumer_supplies	= &wm8994_dcvdd_supply,
};

static struct wm8994_pdata wm8994_platform_data = {
	/* configure gpio1 function for ADCLRCLK */
	.gpio_defaults[0] = 0x0100,
	/* configure gpio2 & gpio6 function for gpio */
	.gpio_defaults[1] = 0x8101, // read-only
	.gpio_defaults[5] = 0x0001, // output
	/* configure gpio3/4/5/7 function for AIF2 voice */
	.gpio_defaults[2] = 0x8100,
	.gpio_defaults[3] = 0x8100,
	.gpio_defaults[4] = 0x8100,
	.gpio_defaults[6] = 0x0100,
	/* configure gpio8/9/10/11 function for AIF3 BT */
	.gpio_defaults[7] = 0x8100,
	.gpio_defaults[8] = 0x0100,
	.gpio_defaults[9] = 0x0100,
	.gpio_defaults[10] = 0x0100,
	.ldo[0]	= { 0, &wm8994_ldo1_data },	/* XM0FRNB_2 */
	.ldo[1]	= { 0, &wm8994_ldo2_data },
};

static struct i2c_board_info wm8994_i2c_dev[] __initdata = {
	{I2C_BOARD_INFO("wm8994", 0x1a), .platform_data	= &wm8994_platform_data,},
};
#endif

#ifdef CONFIG_CACHE_L2X0
extern int wmt_getsyspara(char *varname, unsigned char *varval, int *varlen);
static	void __iomem *l2x0_base;
static DEFINE_RAW_SPINLOCK(l2x0_lock);

static void wmt_l2x0_disable(void)
{
	unsigned long flags;

	raw_spin_lock_irqsave(&l2x0_lock, flags);
//	__l2x0_flush_all();
	wmt_smc(WMT_SMC_CMD_PL310CTRL, 0);
	dsb();
	raw_spin_unlock_irqrestore(&l2x0_lock, flags);
}
#endif

static struct platform_device *wmt_devices[] __initdata = {
	&wmt_uart0_device,
	&wmt_uart1_device,
#ifdef CONFIG_UART_2_3_ENABLE
	&wmt_uart2_device,
	&wmt_uart3_device,
#endif
	&wmt_sf_device,
#ifdef CONFIG_MTD_WMT_NOR
	&wmt_nor_device,
#endif
	&wmt_nand_device,
	&wmt_i2s_device,
	&wmt_pcm_controller_device,
	&wmt_pcm_dma_device,
	&wmt_aud_pcm_device,
	&wmt_i2s_hwdac_device,
//	&wmt_pcm_device,
	&wmt_switch_device,
#ifdef CONFIG_WMT_NEWSPI_SUPPORT
	&wmt_spi_device,
#endif
#ifdef CONFIG_WMT_NEWSPI1_SUPPORT
	&wmt_spi1_device,
#endif
#ifdef CONFIG_DRM_MALI
	&wmt_mali_drm_device,
#endif
#ifdef CONFIG_I2S_CODEC_WM8994
	&wm8994_fixed_voltage0,
	&wm8994_fixed_voltage1,
#endif
	&wm8880_device_pwm,
};

#ifdef CONFIG_VT1603_IOCTRL_SPI
static struct wmt_spi_slave vt1603_codec_info = {
	.dma_en        = SPI_DMA_DISABLE,
	.bits_per_word = 8,
};
static struct spi_board_info vt1603_spi_board_info[] __initdata = {
	{
		.modalias               = "vt1609",
		.bus_num                = 0,
		.chip_select            = 0,
		.max_speed_hz           = 12000000,
		.irq                    = -1,
		.mode = SPI_CLK_MODE3,
		.controller_data = &vt1603_codec_info,
	},
};
#endif


static struct i2c_board_info cp2682_i2c_dev[] __initdata = {
	{I2C_BOARD_INFO("cp2682", (0x2c)),},
};

static char ns_printk_buf[1024];
void sprintk(unsigned int buf, unsigned int len)
{
	local_irq_disable();
        printk(ns_printk_buf);
	wmt_smc(WMT_SMC_CMD_PRINTK_RET, 0);
}
void notify_log_buf()
{
	wmt_smc(WMT_SMC_CMD_LOGBUFOK, (unsigned int)sprintk);
	wmt_smc(WMT_SMC_CMD_LOGBUF_ADDR, (unsigned int)virt_to_phys(ns_printk_buf));
}

static void wmt_default_idle(void)
{
	if (!need_resched()) {
		asm("dsb");
		asm("wfi");
	}
	local_irq_enable();
}
static int __init wmt_init(void)
{
	/* Add for enable user access to pmu */
	unsigned char buf[40];
	int varlen=40;
	unsigned int pmu_param;
	/* Add End */

#ifdef CONFIG_CACHE_L2X0
	__u32 power_ctrl = 0;
	unsigned int onoff = 0;
	unsigned int aux = 0x3E440000;
	unsigned int prefetch_ctrl = 0x70000007;
	unsigned int en_static_address_filtering = 0;
	unsigned int address_filtering_start = 0xD8000000;
	unsigned int address_filtering_end = 0xD9000000;
	unsigned int cpu_trustzone_enabled = 0;
	unsigned long flags;
#endif

	wmt_gpio_init();

	pm_power_off = wmt_power_off;
	pm_idle = wmt_default_idle;
	arm_pm_restart = wmt_restart;

#ifdef CONFIG_WMT_NEWSPI_SUPPORT
	spi_register_board_info(wmt_spi_board_info, ARRAY_SIZE(wmt_spi_board_info));
#endif
#ifdef CONFIG_WMT_NEWSPI1_SUPPORT
	spi_register_board_info(wmt_spi1_board_info, ARRAY_SIZE(wmt_spi1_board_info));
#endif
#ifdef CONFIG_VT1603_IOCTRL_SPI
	spi_register_board_info(vt1603_spi_board_info, ARRAY_SIZE(vt1603_spi_board_info));
#endif

#ifdef CONFIG_I2S_CODEC_WM8994
	i2c_register_board_info(4, wm8994_i2c_dev, ARRAY_SIZE(wm8994_i2c_dev));
#endif


#ifdef CONFIG_CACHE_L2X0
	if (wmt_getsyspara("wmt.l2c.param",buf,&varlen) == 0)
		sscanf(buf,"%d:%x:%x:%d:%x:%x",&onoff, &aux, &prefetch_ctrl, &en_static_address_filtering, &address_filtering_start, &address_filtering_end);

	if (wmt_getsyspara("wmt.secure.param",buf,&varlen) == 0)
		sscanf(buf,"%d",&cpu_trustzone_enabled);
	if(cpu_trustzone_enabled != 1)
		cpu_trustzone_enabled = 0;

	if (onoff == 1) {
		l2x0_base = ioremap(0xD9000000, SZ_4K);


		if(cpu_trustzone_enabled == 0)
		{
			if (en_static_address_filtering == 1) {
				writel_relaxed(address_filtering_end, l2x0_base + 0xC04);
				writel_relaxed((address_filtering_start | 0x01), l2x0_base + 0xC00);
			}
			
			writel_relaxed(0x110, l2x0_base + L2X0_TAG_LATENCY_CTRL);
			writel_relaxed(0x110, l2x0_base + L2X0_DATA_LATENCY_CTRL);
	
			power_ctrl = readl_relaxed(l2x0_base + L2X0_POWER_CTRL) | L2X0_DYNAMIC_CLK_GATING_EN | L2X0_STNDBY_MODE_EN;
			writel_relaxed(power_ctrl, l2x0_base + L2X0_POWER_CTRL);        
	
			writel_relaxed(prefetch_ctrl, l2x0_base + L2X0_PREFETCH_CTRL);         
		}
		else
		{
			if (en_static_address_filtering == 1) {
				wmt_smc(WMT_SMC_CMD_PL310FILTER_END, address_filtering_end);
				wmt_smc(WMT_SMC_CMD_PL310FILTER_START, (address_filtering_start | 0x01));
			}

			wmt_smc(WMT_SMC_CMD_PL310TAG_LATENCY, 0x110);
			wmt_smc(WMT_SMC_CMD_PL310DATA_LATENCY, 0x110);
			power_ctrl = readl_relaxed(l2x0_base + L2X0_POWER_CTRL) | L2X0_DYNAMIC_CLK_GATING_EN | L2X0_STNDBY_MODE_EN;
			wmt_smc(WMT_SMC_CMD_PL310POWER, power_ctrl);

			wmt_smc(WMT_SMC_CMD_PL310PREFETCH, prefetch_ctrl);

			raw_spin_lock_irqsave(&l2x0_lock, flags);
			writel_relaxed(0xffff, l2x0_base + L2X0_INV_WAY);
			while ( readl_relaxed(l2x0_base + L2X0_INV_WAY)  & 0xffff)
				cpu_relax();
			writel_relaxed(0, l2x0_base + L2X0_CACHE_SYNC);
			raw_spin_unlock_irqrestore(&l2x0_lock, flags);
			
			/* enable L2X0 */
			wmt_smc(WMT_SMC_CMD_PL310CTRL, 1);
		}

		/* 512KB (32KB/way) 16-way associativity */
		l2x0_init(l2x0_base, aux, 0);
		
		if(cpu_trustzone_enabled != 0)
			outer_cache.disable = wmt_l2x0_disable;
	}
#endif

#ifdef CONFIG_MTD_WMT_SF
/* Add for enable user access to ARM11 performance monitor */
	if(wmt_getsyspara("wmt.pmu.param",buf,&varlen) == 0)
		sscanf(buf,"%d",&pmu_param );         
	if(pmu_param & 0x1){
		//enable_user_access();
	}
#endif
/* Add End */

	if(cpu_trustzone_enabled == 1)
		notify_log_buf();//Lch for SecureOS_printk
	return platform_add_devices(wmt_devices, ARRAY_SIZE(wmt_devices));
}


arch_initcall(wmt_init);
