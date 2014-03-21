/*++
	linux/arch/arm/mach-wmt/include/mach/iomux.h

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

/*
 * Base address:	0xd8110000
 * register offset:
 * 	Data Input	- 0x0000
 * 	Gpio Enable	- 0x0040
 * 	Output Enable	- 0x0080
 * 	Output Data	- 0x00c0
 * 	Pull Enable	- 0x0480
 * 	Pull Control	- 0x04c0
 * 	IO Strength	- 0x0800
 */

/*       GPn  bit  irq   macro-name */

/* GP0 */
WMT_PIN(0x00, 0, 0x00, WMT_PIN_GP0_GPIO0)		/* 0 */
WMT_PIN(0x00, 1, 0x01, WMT_PIN_GP0_GPIO1)		/* 1 */
WMT_PIN(0x00, 2, 0x02, WMT_PIN_GP0_GPIO2)		/* 2 */
WMT_PIN(0x00, 3, 0x03, WMT_PIN_GP0_GPIO3)		/* 3 */
WMT_PIN(0x00, 4, 0x04, WMT_PIN_GP0_GPIO4)		/* 4 */
WMT_PIN(0x00, 5, 0x05, WMT_PIN_GP0_GPIO5)		/* 5 */
WMT_PIN(0x00, 6, 0x06, WMT_PIN_GP0_GPIO6)		/* 6 */
WMT_PIN(0x00, 7, 0x07, WMT_PIN_GP0_GPIO7)		/* 7 */

/* GP1 */
WMT_PIN(0x01, 0, 0x08, WMT_PIN_GP1_GPIO8)		/* 8  */
WMT_PIN(0x01, 1, 0x09, WMT_PIN_GP1_GPIO9)		/* 9  */
WMT_PIN(0x01, 2, 0x0a, WMT_PIN_GP1_GPIO10)		/* 10 */
WMT_PIN(0x01, 3, 0x0b, WMT_PIN_GP1_GPIO11)		/* 11 */
WMT_PIN(0x01, 4, 0x0c, WMT_PIN_GP1_GPIO12)		/* 12 */
WMT_PIN(0x01, 5, 0x0d, WMT_PIN_GP1_GPIO13)		/* 13 */
WMT_PIN(0x01, 6, 0x0e, WMT_PIN_GP1_GPIO14)		/* 14 */
WMT_PIN(0x01, 7, 0x0f, WMT_PIN_GP1_GPIO15)		/* 15 */

/* GP2 */
WMT_PIN(0x02, 0, 0x10, WMT_PIN_GP2_GPIO16)		/* 16 */
WMT_PIN(0x02, 1, 0x11, WMT_PIN_GP2_GPIO17)		/* 17 */
WMT_PIN(0x02, 2, 0x12, WMT_PIN_GP2_GPIO18)		/* 18 */
WMT_PIN(0x02, 3, 0x13, WMT_PIN_GP2_GPIO19)		/* 19 */

/* GP4 */
WMT_PIN(0x04, 0,   -1, WMT_PIN_GP4_VDOUT0)		/* 20 */
WMT_PIN(0x04, 1,   -1, WMT_PIN_GP4_VDOUT1)		/* 21 */
WMT_PIN(0x04, 2,   -1, WMT_PIN_GP4_VDOUT2)		/* 22 */
WMT_PIN(0x04, 3,   -1, WMT_PIN_GP4_VDOUT3)		/* 23 */
WMT_PIN(0x04, 4,   -1, WMT_PIN_GP4_VDOUT4)		/* 24 */
WMT_PIN(0x04, 5,   -1, WMT_PIN_GP4_VDOUT5)		/* 25 */
WMT_PIN(0x04, 6,   -1, WMT_PIN_GP4_VDOUT6)		/* 26 */
WMT_PIN(0x04, 7,   -1, WMT_PIN_GP4_VDOUT7)		/* 27 */

/* GP5 */
WMT_PIN(0x05, 0,   -1, WMT_PIN_GP5_VDOUT8)		/* 28 */
WMT_PIN(0x05, 1,   -1, WMT_PIN_GP5_VDOUT9)		/* 29 */
WMT_PIN(0x05, 2,   -1, WMT_PIN_GP5_VDOUT10)		/* 30 */
WMT_PIN(0x05, 3, 0x14, WMT_PIN_GP5_VDOUT11)		/* 31 */
WMT_PIN(0x05, 4, 0x15, WMT_PIN_GP5_VDOUT12)		/* 32 */
WMT_PIN(0x05, 5,   -1, WMT_PIN_GP5_VDOUT13)		/* 33 */
WMT_PIN(0x05, 6,   -1, WMT_PIN_GP5_VDOUT14)		/* 34 */
WMT_PIN(0x05, 7,   -1, WMT_PIN_GP5_VDOUT15)		/* 35 */

/* GP6 */
WMT_PIN(0x06, 0,   -1, WMT_PIN_GP6_VDOUT16)		/* 36 */
WMT_PIN(0x06, 1,   -1, WMT_PIN_GP6_VDOUT17)		/* 37 */
WMT_PIN(0x06, 2, 0x16, WMT_PIN_GP6_VDOUT18)		/* 38 */
WMT_PIN(0x06, 3, 0x17, WMT_PIN_GP6_VDOUT19)		/* 39 */
WMT_PIN(0x06, 4, 0x18, WMT_PIN_GP6_VDOUT20)		/* 40 */
WMT_PIN(0x06, 5, 0x19, WMT_PIN_GP6_VDOUT21)		/* 41 */
WMT_PIN(0x06, 6,   -1, WMT_PIN_GP6_VDOUT22)		/* 42 */
WMT_PIN(0x06, 7,   -1, WMT_PIN_GP6_VDOUT23)		/* 43 */

/* GP7 */
WMT_PIN(0x07, 0,   -1, WMT_PIN_GP7_VDDEN)		/* 44 */
WMT_PIN(0x07, 1,   -1, WMT_PIN_GP7_VDHSYNC)		/* 45 */
WMT_PIN(0x07, 2,   -1, WMT_PIN_GP7_VDVSYNC)		/* 46 */
WMT_PIN(0x07, 3,   -1, WMT_PIN_GP7_VDCLK)		/* 47 */

/* GP8 */
WMT_PIN(0x08, 0,   -1, WMT_PIN_GP8_VDIN0)		/* 48 */
WMT_PIN(0x08, 1,   -1, WMT_PIN_GP8_VDIN1)		/* 49 */
WMT_PIN(0x08, 2,   -1, WMT_PIN_GP8_VDIN2)		/* 50 */
WMT_PIN(0x08, 3,   -1, WMT_PIN_GP8_VDIN3)		/* 51 */
WMT_PIN(0x08, 4,   -1, WMT_PIN_GP8_VDIN4)		/* 52 */
WMT_PIN(0x08, 5,   -1, WMT_PIN_GP8_VDIN5)		/* 53 */
WMT_PIN(0x08, 6,   -1, WMT_PIN_GP8_VDIN6)		/* 54 */
WMT_PIN(0x08, 7,   -1, WMT_PIN_GP8_VDIN7)		/* 55 */

/* GP9 */
WMT_PIN(0x09, 0,   -1, WMT_PIN_GP9_VHSYNC)		/* 56 */
WMT_PIN(0x09, 1,   -1, WMT_PIN_GP9_VVSYNC)		/* 57 */
WMT_PIN(0x09, 2,   -1, WMT_PIN_GP9_VCLK)		/* 58 */

/* GP10 */
WMT_PIN(0x0a, 0,   -1, WMT_PIN_GP10_I2SDACDAT0)		/* 59 */
WMT_PIN(0x0a, 1,   -1, WMT_PIN_GP10_I2SDACDAT1)		/* 60 */
WMT_PIN(0x0a, 2,   -1, WMT_PIN_GP10_I2SDACDAT2)		/* 61 */
WMT_PIN(0x0a, 3,   -1, WMT_PIN_GP10_I2SDACDAT3)		/* 62 */
WMT_PIN(0x0a, 4,   -1, WMT_PIN_GP10_I2SADCDAT2)		/* 63 */
WMT_PIN(0x0a, 5,   -1, WMT_PIN_GP10_I2SDACLRC)		/* 64 */
WMT_PIN(0x0a, 6,   -1, WMT_PIN_GP10_I2SDACBCLK)		/* 65 */
WMT_PIN(0x0a, 7,   -1, WMT_PIN_GP10_I2SDACMCLK)		/* 66 */

/* GP11 */
WMT_PIN(0x0b, 0,   -1, WMT_PIN_GP11_I2SADCDATA0)	/* 67 */
WMT_PIN(0x0b, 1,   -1, WMT_PIN_GP11_I2SADCDATA1)	/* 68 */
WMT_PIN(0x0b, 2,   -1, WMT_PIN_GP11_I2SSPDIF0)		/* 69 */

/* GP12 */
WMT_PIN(0x0c, 0,   -1, WMT_PIN_GP12_SPI0CLK)		/* 70 */
WMT_PIN(0x0c, 1,   -1, WMT_PIN_GP12_SPI0MISO)		/* 71 */
WMT_PIN(0x0c, 2,   -1, WMT_PIN_GP12_SPI0MOSI)		/* 72 */
WMT_PIN(0x0c, 3,   -1, WMT_PIN_GP12_SD018SEL)		/* 73 */

/* GP13 */
WMT_PIN(0x0d, 0,   -1, WMT_PIN_GP13_SD0CLK)		/* 74 */
WMT_PIN(0x0d, 1,   -1, WMT_PIN_GP13_SD0CMD)		/* 75 */
WMT_PIN(0x0d, 2,   -1, WMT_PIN_GP13_SD0WP)		/* 76 */
WMT_PIN(0x0d, 3,   -1, WMT_PIN_GP13_SD0DATA0)		/* 77 */
WMT_PIN(0x0d, 4,   -1, WMT_PIN_GP13_SD0DATA1)		/* 78 */
WMT_PIN(0x0d, 5,   -1, WMT_PIN_GP13_SD0DATA2)		/* 79 */
WMT_PIN(0x0d, 6,   -1, WMT_PIN_GP13_SD0DATA3)		/* 80 */
WMT_PIN(0x0d, 7,   -1, WMT_PIN_GP13_SD0PWRSW)		/* 81 */

/* GP14 */
WMT_PIN(0x0e, 0,   -1, WMT_PIN_GP14_NANDALE)		/* 82 */
WMT_PIN(0x0e, 1,   -1, WMT_PIN_GP14_NANDCLE)		/* 83 */
WMT_PIN(0x0e, 2,   -1, WMT_PIN_GP14_NANDWE)		/* 84 */
WMT_PIN(0x0e, 3,   -1, WMT_PIN_GP14_NANDRE)		/* 85 */
WMT_PIN(0x0e, 4,   -1, WMT_PIN_GP14_NANDWP)		/* 86 */
WMT_PIN(0x0e, 5,   -1, WMT_PIN_GP14_NANDWPD)		/* 87 */
WMT_PIN(0x0e, 6,   -1, WMT_PIN_GP14_NANDRB0)		/* 88 */
WMT_PIN(0x0e, 7,   -1, WMT_PIN_GP14_NANDRB1)		/* 89 */

/* GP15 */
WMT_PIN(0x0f, 0,   -1, WMT_PIN_GP14_NANDCE0)		/* 90 */
WMT_PIN(0x0f, 1,   -1, WMT_PIN_GP14_NANDCE1)		/* 91 */
WMT_PIN(0x0f, 2,   -1, WMT_PIN_GP14_NANDCE2)		/* 92 */
WMT_PIN(0x0f, 3,   -1, WMT_PIN_GP14_NANDCE3)		/* 93 */
WMT_PIN(0x0f, 4,   -1, WMT_PIN_GP14_NANDDQS)		/* 94 */

/* GP16 */
WMT_PIN(0x10, 0,   -1, WMT_PIN_GP16_NANDDIO0)		/* 95 */
WMT_PIN(0x10, 1,   -1, WMT_PIN_GP16_NANDDIO1)		/* 96 */
WMT_PIN(0x10, 2,   -1, WMT_PIN_GP16_NANDDIO2)		/* 97 */
WMT_PIN(0x10, 3,   -1, WMT_PIN_GP16_NANDDIO3)		/* 98 */
WMT_PIN(0x10, 4,   -1, WMT_PIN_GP16_NANDDIO4)		/* 99 */
WMT_PIN(0x10, 5,   -1, WMT_PIN_GP16_NANDDIO5)		/* 100 */
WMT_PIN(0x10, 6,   -1, WMT_PIN_GP16_NANDDIO6)		/* 101 */
WMT_PIN(0x10, 7,   -1, WMT_PIN_GP16_NANDDIO7)		/* 102 */

/* GP17 */
WMT_PIN(0x11, 0,   -1, WMT_PIN_GP17_I2C0SCL)		/* 103 */
WMT_PIN(0x11, 1,   -1, WMT_PIN_GP17_I2C0SDA)		/* 104 */
WMT_PIN(0x11, 2,   -1, WMT_PIN_GP17_I2C1SCL)		/* 105 */
WMT_PIN(0x11, 3,   -1, WMT_PIN_GP17_I2C1SDA)		/* 106 */
WMT_PIN(0x11, 4,   -1, WMT_PIN_GP17_I2C2SCL)		/* 107 */
WMT_PIN(0x11, 5,   -1, WMT_PIN_GP17_I2C2SDA)		/* 108 */
WMT_PIN(0x11, 6,   -1, WMT_PIN_GP17_C24MOUT)		/* 109 */

/* GP18 */
WMT_PIN(0x12, 0,   -1, WMT_PIN_GP18_UART0TXD)		/* 110 */
WMT_PIN(0x12, 1,   -1, WMT_PIN_GP18_UART0RXD)		/* 111 */
WMT_PIN(0x12, 2,   -1, WMT_PIN_GP18_UART0RTS)		/* 112 */
WMT_PIN(0x12, 3,   -1, WMT_PIN_GP18_UART0CTS)		/* 113 */
WMT_PIN(0x12, 4,   -1, WMT_PIN_GP18_UART1TXD)		/* 114 */
WMT_PIN(0x12, 5,   -1, WMT_PIN_GP18_UART1RXD)		/* 115 */
WMT_PIN(0x12, 6,   -1, WMT_PIN_GP18_UART1RTS)		/* 116 */
WMT_PIN(0x12, 7,   -1, WMT_PIN_GP18_UART1CTS)		/* 117 */

/* GP19 */
WMT_PIN(0x13, 0,   -1, WMT_PIN_GP19_SD2DATA0)		/* 118 */
WMT_PIN(0x13, 1,   -1, WMT_PIN_GP19_SD2DATA1)		/* 119 */
WMT_PIN(0x13, 2,   -1, WMT_PIN_GP19_SD2DATA2)		/* 120 */
WMT_PIN(0x13, 3,   -1, WMT_PIN_GP19_SD2DATA3)		/* 121 */
WMT_PIN(0x13, 4,   -1, WMT_PIN_GP19_SD2CMD)		/* 122 */
WMT_PIN(0x13, 5,   -1, WMT_PIN_GP19_SD2CLK)		/* 123 */
WMT_PIN(0x13, 6,   -1, WMT_PIN_GP19_SD2PWRSW)		/* 124 */
WMT_PIN(0x13, 7,   -1, WMT_PIN_GP19_SD2WP)		/* 125 */

/* GP20 */
WMT_PIN(0x14, 0,   -1, WMT_PIN_GP20_C24MHZCLKI)		/* 126 */
WMT_PIN(0x14, 1,   -1, WMT_PIN_GP20_PWMOUT0)		/* 127 */

/* GP21 */
WMT_PIN(0x15, 0,   -1, WMT_PIN_GP21_HDMIDCSDA)		/* 128 */
WMT_PIN(0x15, 1,   -1, WMT_PIN_GP21_HDMIDCSCL)		/* 129 */
WMT_PIN(0x15, 2,   -1, WMT_PIN_GP21_HDMIHPD)		/* 130 */

/* GP23 */
WMT_PIN(0x17, 0,   -1, WMT_PIN_GP23_I2C3SDA)		/* 131 */
WMT_PIN(0x17, 1,   -1, WMT_PIN_GP23_I2C3SCL)		/* 132 */
WMT_PIN(0x17, 2,   -1, WMT_PIN_GP23_HDMICEC)		/* 133 */

/* GP24 */
WMT_PIN(0x18, 0,   -1, WMT_PIN_GP24_SFCS0)		/* 134 */
WMT_PIN(0x18, 1,   -1, WMT_PIN_GP24_SFCS1)		/* 135 */
WMT_PIN(0x18, 2,   -1, WMT_PIN_GP24_SFCLK)		/* 136 */
WMT_PIN(0x18, 3,   -1, WMT_PIN_GP24_SFDI)		/* 137 */
WMT_PIN(0x18, 4,   -1, WMT_PIN_GP24_SFDO)		/* 138 */

/* GP25 */

/* Reversed */

/* GP26 */
WMT_PIN(0x1a, 0,   -1, WMT_PIN_GP26_PCM1MCLK)		/* 139 */
WMT_PIN(0x1a, 1,   -1, WMT_PIN_GP26_PCM1CLK)		/* 140 */
WMT_PIN(0x1a, 2,   -1, WMT_PIN_GP26_PCM1SYNC)		/* 141 */
WMT_PIN(0x1a, 3,   -1, WMT_PIN_GP26_PCM1OUT)		/* 142 */
WMT_PIN(0x1a, 4,   -1, WMT_PIN_GP26_PCM1IN)		/* 143 */

/* GP60 */
WMT_PIN(0x3c, 0,   -1, WMT_PIN_GP60_USBSW0)		/* 144 */
WMT_PIN(0x3c, 1,   -1, WMT_PIN_GP60_USBATTTA0)		/* 145 */
WMT_PIN(0x3c, 2,   -1, WMT_PIN_GP60_USB0C0)		/* 146 */
WMT_PIN(0x3c, 3,   -1, WMT_PIN_GP60_USB0C1)		/* 147 */
WMT_PIN(0x3c, 4,   -1, WMT_PIN_GP60_USB0C2)		/* 148 */

/* GP62 */
WMT_PIN(0x3e, 0,   -1, WMT_PIN_GP62_WAKEUP0)		/* 149 */
WMT_PIN(0x3e, 1,   -1, WMT_PIN_GP62_CIRIN)		/* 150 */
WMT_PIN(0x3e, 2,   -1, WMT_PIN_GP62_WAKEUP2)		/* 151 */
WMT_PIN(0x3e, 3,   -1, WMT_PIN_GP62_WAKEUP3)		/* 152 */
WMT_PIN(0x3e, 4,   -1, WMT_PIN_GP62_WAKEUP4)		/* 153 */
WMT_PIN(0x3e, 5,   -1, WMT_PIN_GP62_WAKEUP5)		/* 154 */
WMT_PIN(0x3e, 6,   -1, WMT_PIN_GP62_SUSGPIO0)		/* 155 */
WMT_PIN(0x3e, 7,   -1, WMT_PIN_GP62_SUSGPIO1)		/* 156 */

/* GP63 */
WMT_PIN(0x3f, 2,   -1, WMT_PIN_GP63_SD0CD)		/* 157 */
WMT_PIN(0x3f, 4,   -1, WMT_PIN_GP63_SD2CD)		/* 158 */

