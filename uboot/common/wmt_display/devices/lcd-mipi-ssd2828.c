/*
 * ==========================================================================
 *
 *       Filename:  lcd-b079xan01.c
 *
 *    Description:
 *
 *        Version:  0.01
 *        Created:  Thursday, December 27, 2012 02:50:05 HKT
 *
 *         Author:  Sam Mei (),
 *        Company:
 *
 * ==========================================================================
 */

#include <common.h>
#include <command.h>
#include <asm/types.h>
#include <asm/byteorder.h>

#include "../../../board/wmt/include/wmt_spi.h"
#include "../../../board/wmt/include/wmt_iomux.h"

#include "../lcd.h"

#define DELAY_MASK  0xff000000
#define CHIPSELECT	0

extern void spi1_write_then_read_data(unsigned char *wbuf, unsigned char *rbuf,
				      int num, int clk_mode, int chip_sel);

static inline void __spi_write(u8 *buf, int len)
{
	spi1_write_then_read_data(buf, buf, sizeof(buf), SPI_MODE_3, CHIPSELECT);
	udelay(10);
}

static void ssd2828_read(uint8_t reg, uint16_t *data)
{
	uint8_t buf1[3] = { 0x70, 0x00, 0x00 };
	uint8_t buf2[3] = { 0x73, 0x00, 0x00 };

	buf1[2] = reg;

	spi1_write_then_read_data(buf1, buf1, sizeof(buf1), SPI_MODE_3, CHIPSELECT);
	udelay(10);

	spi1_write_then_read_data(buf2, buf1, sizeof(buf2), SPI_MODE_3, CHIPSELECT);
	udelay(10);

	*data = (buf1[1] << 8) | buf1[2];
}

static inline void spi_write_24bit(uint32_t data)
{
	uint8_t buf[3];

	buf[0] = (data >> 16) & 0xff;
	buf[1] = (data >> 8) & 0xff;
	buf[2] = data & 0xff;

	__spi_write(buf, 3);
}

static void ssd2828_reset(void)
{
	gpio_direction_output(14, 1);
	mdelay(10);

	gpio_direction_output(WMT_PIN_GP0_GPIO0, 1);
	mdelay(10);
	gpio_direction_output(WMT_PIN_GP0_GPIO0, 0);
	mdelay(20);
	gpio_direction_output(WMT_PIN_GP0_GPIO0, 1);
	mdelay(20);
}

// BP080WX7

static const uint32_t ssd2828_bp080wx7[] = {
	0x7000B1,		// VSA=02 , HSA=02
	0x720202,
	0x7000B2,		// VBP+VSA=8, HBP+HSA=40
	0x720828,
	0x7000B3,		// VFP=04 , HFP=10
	0x72040A,
	0x7000B4,		// HACT=1280
	0x720500,
	0x7000B5,		// vACT=800
	0x720320,
	0x7000B6,		// Non burst mode with sync event  24bpp
	0x720007,

	//DELAY_MASK + 10,

	0x7000DE,		// 4lanes
	0x720003,
	0x7000D6,		//BGR
	0x720005,
	0x7000B9,	
	0x720000,

	//DELAY_MASK + 10,
	0x7000BA,		//PLL=24*16 = 384MHz
	0x72C010,
	0x7000BB,		//LP CLK=8.3MHz
	0x720008,
	0x7000B9,	
	0x720001,

	//DELAY_MASK + 200,
	0x7000B8,		// Virtual Channel 0
	0x720000,

	0x7000B7,		//LP Mode 
	0x720342,

	//DELAY_MASK + 10,
	0x7000BC,		
	0x720000,
	0x700011,		//sleep out

	DELAY_MASK + 200,
	0x7000BC,	
	0x720000,
	0x700029,		//display on

	//    DELAY_MASK + 50,		//Video Mode
	0x7000B7,	
	0x72034B,

	DELAY_MASK + 200,
	0x7000BC,	
	0x720000,
	0x700029,		//display on
};

static int bp080wx7_init(void)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(ssd2828_bp080wx7); i++) {
		if ((ssd2828_bp080wx7[i] & DELAY_MASK) == DELAY_MASK) {
			mdelay(ssd2828_bp080wx7[i] & 0xff);
		} else {
			spi_write_24bit(ssd2828_bp080wx7[i]);
		}
	}

	return 0;
}

static void lcd_bp080wx7_on(void)
{
	uint16_t id;

	printf("%s\n", __FUNCTION__);
	ssd2828_reset();

	ssd2828_read(0xb0, &id);
	if (id == 0x2828) {
		printf("found SSD2828!\n");
		bp080wx7_init();
		lcd_enable_signal(1); /* singal on */    
	} else
		printf("Error: SSD2828 not found! (0x%x)\n", id);
}

static void lcd_bp080wx7_off(void)
{
	lcd_enable_signal(0); /* singal on */
	printf("lcd_b079xan01_power_off\n");
}

//Timing parameter for 3.0" QVGA LCD
#define VBPD 		(16)
#define VFPD 		(16)
#define VSPW 		(4)

#define HBPD 		(47)
#define HFPD 		(45)
#define HSPW 		(8)

static lcd_parm_t lcd_bp080wx7_parm = {
	.bits_per_pixel = 24,
	.capability = 0,
	.vmode = {
		.name = "BP080WX7",
		.refresh = 60,
		.xres = 1280,
		.yres = 800,
		.pixclock = KHZ2PICOS(64800),
		.left_margin = HBPD,
		.right_margin = HFPD,
		.upper_margin = VBPD,
		.lower_margin = VFPD,
		.hsync_len = HSPW,
		.vsync_len = VSPW,
		.sync = 0, //FB_SYNC_VERT_HIGH_ACT | FB_SYNC_HOR_HIGH_ACT,
		.vmode = 0,
		.flag = 0,
	},
	.initial   = lcd_bp080wx7_on,
	.uninitial = lcd_bp080wx7_off,
};

static lcd_parm_t *lcd_bp080wx7_get_parm(int arg)
{
	return &lcd_bp080wx7_parm;
}

// B079XAN01

static const uint32_t ssd2828_b079xan01[] = {
	0x7000B1,	 //VSA=50, HAS=64
	0x723240,	
	0x7000B2,	 //VBP=30+50, HBP=56+64
	0x725078,	
	0x7000B3,	 //VFP=36, HFP=60
	0x72243C,	
	0x7000B4,	 //HACT=768
	0x720300,	
	0x7000B5,	 //VACT=1024
	0x720400,	
	0x7000B6,	
	0x72000b,	  //burst mode, 24bpp loosely packed
	0x7000DE,	 //no of lane=4
	0x720003,	
	0x7000D6,	 //RGB order and packet number in blanking period
	0x720005,	
	0x7000B9,	 //disable PLL
	0x720000,	
	0x7000BA,	 //lane speed=576 (24MHz * 24 = 576)
	0x728018,	  //may modify according to requirement, 500Mbps to 560Mbps
	0x7000BB,	 //LP clock
	0x720008,	 // 576 / 9 / 8 = 8 MHz
	0x7000B9,	 //enable PPL
	0x720001,	
	0x7000C4,	 //enable BTA
	0x720001,	
	0x7000B7,	 //enter LP mode
	0x720342,	
	0x7000B8,	 //VC
	0x720000,	
	0x7000BC,	 //set packet size
	0x720000,	
	0x700011,	//sleep out cmd

	DELAY_MASK + 200,
	0x700029,	 //display on
	DELAY_MASK + 200,
	0x7000B7, 	//video mode on
	0x72030b, 	
};

static int b079xan01_init(void)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(ssd2828_b079xan01); i++) {
		if ((ssd2828_b079xan01[i] & DELAY_MASK) == DELAY_MASK) {
			mdelay(ssd2828_b079xan01[i] & 0xff);
		} else {
			spi_write_24bit(ssd2828_b079xan01[i]);
		}
	}
	return 0;
}

static void lcd_b079xan01_initial(void)
{
	uint16_t id;

	printf("%s\n", __FUNCTION__);
	ssd2828_reset();

	ssd2828_read(0xb0, &id);
	if (id == 0x2828) {
		printf("found SSD2828!\n");
		b079xan01_init();
		lcd_enable_signal(1); /* singal on */
	} else
		printf("Error: SSD2828 not found! (0x%x)\n", id);
}

static void lcd_b079xan01_uninitial(void)
{
	lcd_enable_signal(0); /* singal on */
	printf("lcd_b079xan01_power_off\n");
}

static lcd_parm_t lcd_b079xan01_parm = {
	.bits_per_pixel = 24,
	.capability = 0,
	.vmode = {
		.name = "B079XAN01",
		.refresh = 60,
		.xres = 768,
		.yres = 1024,
		.pixclock = KHZ2PICOS(64800),
		.left_margin = 56,
		.right_margin = 60,
		.upper_margin = 30,
		.lower_margin = 36,
		.hsync_len = 64,
		.vsync_len = 50,
		.sync = 0, //FB_SYNC_VERT_HIGH_ACT | FB_SYNC_HOR_HIGH_ACT,
		.vmode = 0,
		.flag = 0,
	},
	.initial = lcd_b079xan01_initial,
	.uninitial = lcd_b079xan01_uninitial,
};

static lcd_parm_t *lcd_b079xan01_get_parm(int arg)
{
	return &lcd_b079xan01_parm;
}

int lcd_ssd2828_init(void)
{
	lcd_panel_register(LCD_B079XAN01, (void *) lcd_b079xan01_get_parm);
	lcd_panel_register(LCD_BP080WX7,  (void *) lcd_bp080wx7_get_parm);
	return 0;
}

module_init(lcd_ssd2828_init);

static void ssd2828_register_dump(void)
{
	int reg;
	uint16_t data;

	for (reg = 0xb0; reg <= 0xff; reg++) {
		ssd2828_read(reg, &data);
		printf("reg 0x%02X : 0x%04x\n", reg, data);
	}
}

static int do_b0lcd(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	if (argc == 1)
		return 0;

	if (!strcmp(argv[1], "fill")) {
		mv_Rect rect;
		uint32_t color;

		if (argc != 7) {
			printf("hx fill left top right bottom color\n");
			return 0;
		}

		rect.left = simple_strtoul(argv[2], NULL, 10);
		rect.top = simple_strtoul(argv[3], NULL, 10);
		rect.right = simple_strtoul(argv[4], NULL, 10);
		rect.bottom = simple_strtoul(argv[5], NULL, 10);

		color = simple_strtoul(argv[6], NULL, 16);

		printf("fill %d %d %d %d 0x%x\n",
		       rect.left, rect.top, rect.right, rect.bottom, color);
		mv_fillRect(&rect, (color >> 16) & 0xff, (color >> 8) & 0xff, color & 0xff);
	} else if (!strcmp(argv[1], "dump")) {
		ssd2828_register_dump();
	}

	return 0;
}

U_BOOT_CMD(b0,	10,	1,	do_b0lcd,
	   "bmp     - manipulate BMP image data\n",
	   "info <imageAddr>          - display image info\n"
	   "bmp display <imageAddr> [x y] - display image at x,y\n");
