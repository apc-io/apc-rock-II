/*++
linux/include/asm-arm/arch-wmt/wmt_mmap.h

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

/* Be sure that virtual mapping is defined right */
#ifndef __ASM_ARCH_HARDWARE_H
#error "You must include hardware.h, not wmt_mmap.h"
#endif

#ifndef __WMT_MMAP_H
#define __WMT_MMAP_H

/**
 * WMT Memory Map for Physical Address 0xD8000000 will be mapped to 
 * Virtual Address 0xFE000000
 */
#define WMT_MMAP_OFFSET                         (0xFE000000-0xD8000000)


/**
 *  Internal AHB Slaves Memory Address Map
 */
#define MEMORY_CTRL_V4_CFG_BASE_ADDR            (0xD8000400 + WMT_MMAP_OFFSET)  /* 1K , 8/16/32 RW */
#define DMA_CTRL_V4_CFG_BASE_ADDR               (0xD8001800 + WMT_MMAP_OFFSET)  /* 1K , 8/16/32 RW */
#define SF_MEM_CTRL_CFG_BASE_ADDR               (0xD8002000 + WMT_MMAP_OFFSET)  /* 1K , 8/16/32 RW */

#define ETHERNET_MAC_0_CFG_BASE_ADDR            (0xD8004000 + WMT_MMAP_OFFSET)  /* 1K , 8/16/32 RW */
#define SECURITY_ENGINE_CFG_BASE_ADDR           (0xD8006000 + WMT_MMAP_OFFSET)  /* 1K , 8/16/32 RW */
#define USB20_HOST_DEVICE_CFG_BASE_ADDR         (0xD8007800 + WMT_MMAP_OFFSET)  /* 2K , 8/16/32 RW */
#define USB20_HOST_CFG_EXTENT_BASE_ADDR         (0xD8008C00 + WMT_MMAP_OFFSET)  /* 1K , 8/16/32 RW */
#define NF_CTRL_CFG_BASE_ADDR                   (0xD8009000 + WMT_MMAP_OFFSET)  /* 1K , 8/16/32 RW */
#define SD0_SDIO_MMC_BASE_ADDR                  (0xD800A000 + WMT_MMAP_OFFSET)  /* 1K , 8/16/32 RW */
#define SD1_SDIO_MMC_BASE_ADDR                  (0xD800A400 + WMT_MMAP_OFFSET)  /* 1K , 8/16/32 RW */
#define SD2_SDIO_MMC_BASE_ADDR                  (0xD800A800 + WMT_MMAP_OFFSET)  /* 1K , 8/16/32 RW */
#define MPCORE_PRIVATE_MEM                      (0xD8018000 + WMT_MMAP_OFFSET)


#define ASYNC_APB_BRIDGE_BASE_ADDR              (0xD802FC00 + WMT_MMAP_OFFSET)  /* 1K , 8/16/32 RW */

#define GOVM_BASE_ADDR                          (0xD8050300 + WMT_MMAP_OFFSET)  /* 256 , 8/16/32 RW */
#define GOVRH_BASE1_ADDR                        (0xD8050800 + WMT_MMAP_OFFSET)  /* 256 , 8/16/32 RW */
#define GOVRH_BASE2_ADDR                        (0xD8050900 + WMT_MMAP_OFFSET)  /* 256 , 8/16/32 RW */
#define VID_BASE_ADDR                           (0xD8050A00 + WMT_MMAP_OFFSET)  /* 256 , 8/16/32 RW */
#define SCL_BASE_ADDR                           (0xD8050D00 + WMT_MMAP_OFFSET)  /* 256 , 8/16/32 RW */
#define SCL_BASE2_ADDR                          (0xD8050000 + WMT_MMAP_OFFSET)  /* 256 , 8/16/32 RW */
#define VPP_BASE_ADDR                           (0xD8050F00 + WMT_MMAP_OFFSET)  /* 256 , 8/16/32 RW */
#define LVDS_BASE_ADDR                          (0xD8051000 + WMT_MMAP_OFFSET)  /* 256 , 8/16/32 RW */
#define GOVRH2_BASE1_ADDR                       (0xD8051700 + WMT_MMAP_OFFSET)  /* 256 , 8/16/32 RW */
#define GOVRH2_BASE2_ADDR                       (0xD8051800 + WMT_MMAP_OFFSET)  /* 256 , 8/16/32 RW */
#define HDMI_BASE2_ADDR                         (0xD8051F00 + WMT_MMAP_OFFSET)  /* 256 , 8/16/32 RW */

#define HDMI_TRANSMITTE_BASE_ADDR               (0xD8060000 + WMT_MMAP_OFFSET)  /* 64K , 8/16/32 RW */
#define HDMI_CP_BASE_ADDR                       (0xD8070000 + WMT_MMAP_OFFSET)  /* 64K , 8/16/32 RW */

#define AUDREGF_BASE_ADDR                       (0xD80ED800 + WMT_MMAP_OFFSET)  /* 1K , 8/16/32 RW */

#define MSVD_BASE_ADDR                          (0xD80F0000 + WMT_MMAP_OFFSET)  /* 1K , 8/16/32 RW */
#define JPEG_ENCODER_BASE_ADDR                  (0xD80F2000 + WMT_MMAP_OFFSET)  /* 4K , 8/16/32 RW */
#define JPEG_DECODER_BASE_ADDR                  (0xD80F4000 + WMT_MMAP_OFFSET)  /* 4K , 8/16/32 RW */
#define H264_ENCODER_BASE_ADDR                  (0xD80F6000 + WMT_MMAP_OFFSET)  /* 16K , 8/16/32 RW */
#define CNM_BIT_BASE_ADDR                       (0xD80F8000 + WMT_MMAP_OFFSET)  /* 4K , 8/16/32 RW */

/**
 *  Internal APB Slaves Memory Address Map
 */
#define RTC_BASE_ADDR                           (0xD8100000 + WMT_MMAP_OFFSET)  /* 64K  */
#define GPIO_BASE_ADDR                          (0xD8110000 + WMT_MMAP_OFFSET)  /* 64K  */
#define SYSTEM_CFG_CTRL_BASE_ADDR               (0xD8120000 + WMT_MMAP_OFFSET)  /* 64K  */
#define PM_CTRL_BASE_ADDR                       (0xD8130000 + WMT_MMAP_OFFSET)  /* 64K  */
#define INTERRUPT0_CTRL_BASE_ADDR               (0xD8140000 + WMT_MMAP_OFFSET)  /* 64K  */
#define INTERRUPT1_CTRL_BASE_ADDR               (0xD8150000 + WMT_MMAP_OFFSET)  /* 64K  */

#define UART0_BASE_ADDR                         (0xD8200000 + WMT_MMAP_OFFSET)  /* 64K  */
#define UART1_BASE_ADDR                         (0xD82b0000 + WMT_MMAP_OFFSET)  /* 64K  */
#define UART2_BASE_ADDR                         (0xD8210000 + WMT_MMAP_OFFSET)  /* 64K  */
#define UART3_BASE_ADDR                         (0xD82c0000 + WMT_MMAP_OFFSET)  /* 64K  */
#define PWM0_BASE_ADDR                          (0xD8220000 + WMT_MMAP_OFFSET)  /* 64K  */

#define SPI0_BASE_ADDR                          (0xD8240000 + WMT_MMAP_OFFSET)  /* 64K  */
#define SPI1_BASE_ADDR                          (0xD8250000 + WMT_MMAP_OFFSET)  /* 64K  */
#define SPI2_BASE_ADDR                          (0xD82A0000 + WMT_MMAP_OFFSET)  /* 64K  */
#define KPAD_BASE_ADDR                          (0xD8260000 + WMT_MMAP_OFFSET)  /* 64K  */
#define CIR_BASE_ADDR                           (0xD8270000 + WMT_MMAP_OFFSET)  /* 64K  */
#define I2C0_BASE_ADDR                          (0xD8280000 + WMT_MMAP_OFFSET)  /* 64K  */
#define I2C1_BASE_ADDR                          (0xD8320000 + WMT_MMAP_OFFSET)  /* 64K  */
#define PCM_BASE_ADDR                           (0xD82D0000 + WMT_MMAP_OFFSET)  /* 64K  */

#define I2C2_BASE_ADDR                          (0xD83A0000 + WMT_MMAP_OFFSET)  /* 64K  */
#define I2C3_BASE_ADDR                          (0xD83B0000 + WMT_MMAP_OFFSET)  /* 64K  */


#define ADC_BASE_ADDR                           (0xD8340000 + WMT_MMAP_OFFSET)  /* 64K  */
#define I2C4_BASE_ADDR                          (0xD8400000 + WMT_MMAP_OFFSET)  /* 64K  */
// check
#define DMA_CTRL_CFG_BASE_ADDR                  DMA_CTRL_V4_CFG_BASE_ADDR
#define INTERRUPT_CTRL_BASE_ADDR                INTERRUPT0_CTRL_BASE_ADDR
#define SPI_BASE_ADDR                           SPI0_BASE_ADDR
#define I2C_BASE_ADDR                           I2C0_BASE_ADDR
#define I2S_BASE_ADDR                           AUDREGF_BASE_ADDR


/* WMT Memory Map for Physical Address*/
#define UART0_PHY_BASE_ADDR                     0xD8200000  /* 64K  */
#define UART1_PHY_BASE_ADDR                     0xD82b0000  /* 64K  */

#endif	/* __WMT_MMAP_H */
