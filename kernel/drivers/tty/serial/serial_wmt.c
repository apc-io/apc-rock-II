/*++
linux/drivers/serial/serial_wmt.c

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

#include <linux/module.h>
#include <linux/tty.h>
#include <linux/tty_flip.h>
#include <linux/ioport.h>
#include <linux/init.h>
#include <linux/serial.h>
#include <linux/console.h>
#include <linux/sysrq.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/cpufreq.h>
#include <linux/platform_device.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <mach/hardware.h>
#include <mach/dma.h>
#include <linux/serial_core.h>
#include <linux/dma-mapping.h>
#include <linux/slab.h>


#define PORT_WMT 54
#if defined(CONFIG_SERIAL_WMT_CONSOLE) && defined(CONFIG_MAGIC_SYSRQ)
#define SUPPORT_SYSRQ
#endif


#ifdef CONFIG_SERIAL_WMT_DMA
/* * DMA processing */
#define DMA_RX_REQUEST(s, cb)	wmt_request_dma(&s->rx_dmach, s->id, s->dma_rx_dev, cb, s)
#define DMA_RX_FREE(s)		{wmt_free_dma(s->rx_dmach); s->rx_dmach = NULL_DMA; }
#define DMA_RX_START(s, d, l)	wmt_start_dma(s->rx_dmach, d, 0, l)
#define DMA_RX_POS(s)		wmt_get_dma_pos(s->rx_dmach)
#define DMA_RX_STOP(s)		wmt_stop_dma(s->rx_dmach)
#define DMA_RX_CLEAR(s)		wmt_clear_dma(s->rx_dmach)
#define DMA_RX_RESET(s)		wmt_reset_dma(s->rx_dmach)
#define IS_NULLDMA(ch)		((ch) == NULL_DMA)
#define CHK_DMACH(ch)		((ch) != NULL_DMA)
#define DMA_TX_REQUEST(s, cb)	wmt_request_dma(&s->tx_dmach, s->id, s->dma_tx_dev, cb, s)
#define DMA_TX_FREE(s)		{wmt_free_dma(s->tx_dmach); s->tx_dmach = NULL_DMA; }
#define DMA_TX_START(s, d, l)	wmt_start_dma(s->tx_dmach, d, 0, l)
#define DMA_TX_POS(s)		wmt_get_dma_pos(s->tx_dmach)
#define DMA_TX_STOP(s)		wmt_stop_dma(s->tx_dmach)
#define DMA_TX_CLEAR(s)		wmt_clear_dma(s->tx_dmach)
#define DMA_TX_RESET(s)		wmt_reset_dma(s->tx_dmach)
#define NULL_DMA                ((dmach_t)(-1))
#endif



#ifdef DEBUG_SERIAL_WMT
#define DEBUG_INTR(fmt...)	printk(fmt)
#else
#define DEBUG_INTR(fmt...)	do { } while (0)
#endif


#define UART_BR_115K2 115200

/*
 * This is for saving useful I/O registers
 */

/*
 * RS232 on DB9
 * Pin No.      Name    Notes/Description
 * 1            DCD     Data Carrier Detect
 * 2            RD      Receive Data (a.k.a RxD, Rx)
 * 3            TD      Transmit Data (a.k.a TxD, Tx)
 * 4            DTR     Data Terminal Ready
 * 5            SGND    Ground
 * 6            DSR     Data Set Ready
 * 7            RTS     Request To Send
 * 8            CTS     Clear To Send
 * 9            RI      Ring Indicator
 */

/*
 * We've been assigned a range on the "Low-density serial ports" major
 *
 * dev          Major           Minor
 * ttyVT0       204             90
 * ttyS0        4               64
 */
#ifdef CONFIG_SERIAL_WMT_TTYVT
		#define SERIAL_WMT_MAJOR     204
		#define MINOR_START          90      /* Start from ttyVT0 */
		#define CALLOUT_WMT_MAJOR    205     /* for callout device */
#else
		#define SERIAL_WMT_MAJOR     4
		#define MINOR_START          64      /* Start from ttyS0 */
		#define CALLOUT_WMT_MAJOR    5       /* for callout device */
#endif

#ifdef CONFIG_UART_2_3_ENABLE
#define NR_PORTS				4
#else
#define NR_PORTS				2
#endif
#define WMT_ISR_PASS_LIMIT   256

struct wmt_port {
	struct uart_port	port;
	struct timer_list	timer;
	struct timer_list	rx_timer;
#ifdef CONFIG_SERIAL_WMT_DMA
	/* RX Buffer0 physical address */
	dma_addr_t uart_rx_dma_phy0_org;
	/* RX Buffer1 physical address */
	dma_addr_t uart_rx_dma_phy1_org;
	/* RX Buffer2 physical address */
	dma_addr_t uart_rx_dma_phy2_org;
	/* RX Buffer0 virtual address */
	char *uart_rx_dma_buf0_org;
	/* RX Buffer1 virtual address */
	char *uart_rx_dma_buf1_org;
	/* RX Buffer2 virtual address */
	char *uart_rx_dma_buf2_org;
	/* RX Buffer0 virtual address, this address is the data should be read */
	char *uart_dma_tmp_buf0;
	/* RX Buffer1 virtual address, this address is the data should be read */
	char *uart_dma_tmp_buf1;
	/* RX Buffer2 virtual address, this address is the data should be read */
	char *uart_dma_tmp_buf2;
	/* RX Buffer0 physical address, this address is the data should be read */
	dma_addr_t uart_dma_tmp_phy0;
	/* RX Buffer1 physical address, this address is the data should be read */
	dma_addr_t uart_dma_tmp_phy1;
	/* RX Buffer2 physical address, this address is the data should be read */
	dma_addr_t uart_dma_tmp_phy2;
	/* RX device identifier for DMA */
	enum dma_device_e dma_rx_dev;
	/* TX device identifier for DMA */
	enum dma_device_e dma_tx_dev;
	/* RX DMA device config */
	struct dma_device_cfg_s dma_rx_cfg;
	/* TX DMA device config */
	struct dma_device_cfg_s dma_tx_cfg;
	/* RX DMA channel number */
	dmach_t rx_dmach;
	/* TX DMA channel number */
	dmach_t tx_dmach;
	/* RX dma callback function record which buffer the DMA used */
	unsigned int buffer_used;
	/* The buffer rx_char function should used (virtual address) */
	unsigned int buffer_selected;
	/* The buffer rx_count function should used (physical address) */
	unsigned int buffer_rx_count_selected;
	/* TX Buffer0 virtual address */
	char *uart_tx_dma_buf0_org;
	/* TX Buffer0 physical address */
	dma_addr_t uart_tx_dma_phy0_org;
	/* Record TX DMA running status : DMA_TX_END, DMA_TX_CHAR, DMA_TX_XMIT */
	unsigned int uart_tx_dma_flag;
	/* TX dma transmit counter*/
	unsigned int uart_tx_count;
	/* RX dma buffer last position*/
	unsigned int last_pos;
	/* Record the number of RX dma timeout function with no data transmit*/
	unsigned int uart_rx_dma_flag;
	unsigned int dma_tx_cnt;
#endif
	unsigned int old_status;
	/* identification string */
	char *id;
	unsigned int old_urdiv;
	unsigned int old_urlcr;
	unsigned int old_urier;
	unsigned int old_urfcr;
	unsigned int old_urtod;
};

#ifdef CONFIG_SERIAL_WMT_DMA
#define DMA_TX_END 0
#define DMA_TX_CHAR 1
#define DMA_TX_XMIT 2
#endif


/*
 * WMT UART registers set structure.
 */
struct wmt_uart {
	unsigned int volatile urtdr;            /* 0x00*/
	unsigned int volatile urrdr;            /* 0x04*/
	unsigned int volatile urdiv;            /* 0x08*/
	unsigned int volatile urlcr;            /* 0x0C*/
	unsigned int volatile uricr;            /* 0x10*/
	unsigned int volatile urier;            /* 0x14*/
	unsigned int volatile urisr;            /* 0x18*/
	unsigned int volatile urusr;            /* 0x1C*/
	unsigned int volatile urfcr;            /* 0x20*/
	unsigned int volatile urfidx;           /* 0x24*/
	unsigned int volatile urbkr;            /* 0x28*/
	unsigned int volatile urtod;            /* 0x2C*/
	unsigned int volatile resv30_FFF[0x3F4];	/* 0x0030 - 0x0FFF Reserved*/
	unsigned char volatile urtxf[32];       /* 0x1000 - 0x101F*/
	unsigned char volatile urrxf[32];      /* 0x1020 - 0x103F*/
};

struct baud_info_s {
	unsigned int baud;		/* baud rate */
	unsigned int brd;		/* baud rate divisor */
	unsigned int bcv;		/* break counter value at this baud rate
							 * simply be calculated by baud * 0.004096
							 */
};
#ifdef UART_DEBUG
unsigned int *DMA_pbuf =NULL;
unsigned int *COUNT_pbuf =NULL;
unsigned int *CPU_pbuf =NULL;
unsigned int dma_write_index = 0x00;
#endif
int mtk6622_tty = -1;
extern int wmt_getsyspara(char *varname, char *varval, int *varlen);

static struct baud_info_s baud_table[] = {
	{   3600,  0x100FF,    15 },
	{   7600,  0x1007F,    30 },
	{   9600,  0x2003F,    39 },
	{  14400,  0x1003F,    59 },
	{  19200,  0x2001F,    79 },
	{  28800,  0x1001F,   118 },
	{  38400,  0x2000F,   157 },
	{  57600,  0x1000F,   236 },
	{ 115200,   0x10007,   472 },
	{ 230400,   0x10003,   944 },
	{ 460800,   0x10001,  1920 },
	{ 921600,   0x10000,  3775 },
};

#define BAUD_TABLE_SIZE                 ARRAY_SIZE(baud_table)

#ifdef CONFIG_SERIAL_WMT_DMA
#define UART_BUFFER_SIZE	(1024*16)
#endif
/*
 * Macros to put URISR and URUSR into a 32-bit status variable
 * URISR in bit[ 0:15]
 * URUSR in bit[16:31]
 */
#define URISR_TO_SM(x)                  ((x) & URISR_MASK)
#define URUSR_TO_SM(x)                  (((x) & URUSR_MASK) << 16)
#define SM_TO_URISR(x)                  ((x) & 0xffff)
#define SM_TO_URUSR(x)                  ((x) >> 16)

/*
 * Following is a trick if we're interesting to listen break signal,
 * but due to WMT UART doesn't suppout this interrupt status.
 * So I make a fake interrupt status and use URISR_FER event to implement
 * break signal detect.
 */
#ifdef CONFIG_SERIAL_WMT_BKSIG
#define SW_BKSIG                        (BIT31 | URISR_FER)
#endif
/*
 * Macros to manipulate WMT UART module.
 *
 * s = sport, o = offset, v = value
 *
 * registers offset table as follows:
 *
 * URTDR           0x0000
 * URRDR           0x0004
 * URBRD           0x0008
 * URLCR           0x000C
 * URICR           0x0010
 * URIER           0x0014
 * URISR           0x0018
 * URUSR           0x001C
 * URFCR           0x0020
 * URFIDX          0x0024
 * URBKR           0x0028
 *
 * Offset 0x002C-0x002F reserved
 *
 * URTXF           0x0030
 * URRXF           0x0040
 */
#define PORT_TO_BASE(s)                 ((s)->port.membase)
#define WMT_UART_GET(s, o)           __raw_readl((s)->port.membase + o)
#define WMT_UART_PUT(s, o, v)        __raw_writel(v, (s)->port.membase + o)
#define WMT_UART_TXFIFO(s)           (volatile unsigned char *)((s)->port.membase + URTXF)
#define WMT_UART_RXFIFO(s)           (volatile unsigned short *)((s)->port.membase + URRXF)

/*
 * This is the size of our serial port register set.
 */
#define UART_PORT_SIZE  0x1040

/*
 * This determines how often we check the modem status signals
 * for any change.  They generally aren't connected to an IRQ
 * so we have to poll them.  We also check immediately before
 * filling the TX fifo incase CTS has been dropped.
 */
#define MCTRL_TIMEOUT   (250*HZ/1000)

/*{2007/11/10 JHT Support the VT8500 Serial Port Driver Because the*/
/*                definition of URSIRT Bit[0] & Bit[3] are different.*/
/*                Before VT8500 these bit are defined as RO, in VT8500*/
/*                they are changed into the W1C. Therefore the xmit function*/
/*                for the FIFO mode should be modified as well.*/
static void wmt_tx_chars(struct wmt_port *sport);
static void wmt_rx_chars(struct wmt_port *sport, unsigned int status);
static struct wmt_port wmt_ports[NR_PORTS];

/*}2007/11/10-JHT*/

enum {
	SHARE_PIN_UART = 0,
	SHARE_PIN_SPI,
};
static int wmt_uart_spi_sel = SHARE_PIN_UART;  /* 0:uart, 1:spi */

void uart_dump_reg(struct wmt_port *sport)
{
	struct wmt_uart *uart = (struct wmt_uart *)PORT_TO_BASE(sport);

	unsigned int urtdr = uart->urtdr;            /* 0x00*/
	unsigned int urrdr = uart->urrdr;            /* 0x04*/
	unsigned int urdiv = uart->urdiv;            /* 0x08*/
	unsigned int urlcr = uart->urlcr;            /* 0x0C*/
	unsigned int uricr = uart->uricr;            /* 0x10*/
	unsigned int urier = uart->urier;            /* 0x14*/
	unsigned int urisr = uart->urisr;            /* 0x18*/
	unsigned int urusr = uart->urusr;            /* 0x1C*/
	unsigned int urfcr = uart->urfcr;            /* 0x20*/
	unsigned int urfidx = uart->urfidx;           /* 0x24*/
	unsigned int urbkr = uart->urbkr;            /* 0x28*/
	unsigned int urtod = uart->urtod;            /* 0x2C*/
#if 1

	printk("urtdr=0x%.8x  urrdr=0x%.8x  urdiv=0x%.8x  urlcr=0x%.8x\n" \
		"uricr=0x%.8x  urier=0x%.8x  urisr=0x%.8x  urusr=0x%.8x\n" \
		"urfcr=0x%.8x  uridx=0x%.8x  urbkr=0x%.8x  urtod=0x%.8x\n",
			urtdr, urrdr, urdiv, urlcr,
			uricr, urier, urisr, urusr,
			urfcr, urfidx, urbkr, urtod);
#endif
}

static void wmt_mctrl_check(struct wmt_port *sport)
{
	unsigned int status, changed;

	status = sport->port.ops->get_mctrl(&sport->port);
	changed = status ^ sport->old_status;

	if (changed == 0)
		return;

	sport->old_status = status;

	if (changed & TIOCM_RI)
		sport->port.icount.rng++;
	if (changed & TIOCM_DSR)
		sport->port.icount.dsr++;
	if (changed & TIOCM_CAR)
		uart_handle_dcd_change(&sport->port, status & TIOCM_CAR);
	if (changed & TIOCM_CTS)
		uart_handle_cts_change(&sport->port, status & TIOCM_CTS);

	wake_up_interruptible(&sport->port.state->port.delta_msr_wait);
}

/*
 * This is our per-port timeout handler, for checking the
 * modem status signals.
 */
static void wmt_timeout(unsigned long data)
{
	struct wmt_port *sport = (struct wmt_port *)data;
	unsigned long flags;



	if (sport->port.state) {
		spin_lock_irqsave(&sport->port.lock, flags);
		wmt_mctrl_check(sport);
		spin_unlock_irqrestore(&sport->port.lock, flags);
		mod_timer(&sport->timer, jiffies + MCTRL_TIMEOUT);
	}
}

unsigned int rx_timeout = 1;

static void wmt_rx_timeout(unsigned long data)
{
	struct wmt_port *sport = (struct wmt_port *)data;
	struct wmt_uart *uart = (struct wmt_uart *)PORT_TO_BASE(sport);
	unsigned long flags;
	unsigned int status;
	unsigned int pos = 0;


	spin_lock_irqsave(&sport->port.lock, flags);
	pos = DMA_RX_POS(sport);
	status = URISR_TO_SM(uart->urisr) | URUSR_TO_SM(uart->urusr);
	uart->urisr |= SM_TO_URISR(status);
	spin_unlock_irqrestore(&sport->port.lock, flags);

	/* DMA didn't transmit any data */
	if (sport->last_pos == pos) {
		sport->uart_rx_dma_flag++;

		/* RX DMA didn't trasmit any data in two times,    */
		/* enable interrupt and do the last time mod timer */
		if (sport->uart_rx_dma_flag == 2) {
			sport->uart_rx_dma_flag++;
			uart->urier = URIER_ERXFAF  |
				      URIER_ERXFF   |
				      URIER_ERXTOUT |
				      URIER_EPER    |
				      URIER_EFER    |
				      URIER_ERXDOVR;
			mod_timer(&sport->rx_timer, jiffies + rx_timeout);
			return;
		} else if (sport->uart_rx_dma_flag == 3) {
			sport->uart_rx_dma_flag = 0;
			return;
		}
		mod_timer(&sport->rx_timer, jiffies + rx_timeout);
		return;
	}

	sport->uart_rx_dma_flag = 0;

	wmt_rx_chars(sport, URISR_RXFAF | URISR_RXFF | status);

	mod_timer(&sport->rx_timer, jiffies + rx_timeout);

}


/*
 * Interrupts should be disabled on entry.
 */
static void wmt_stop_tx(struct uart_port *port)
{
	struct wmt_port *sport = (struct wmt_port *)port;
	struct wmt_uart *uart = (struct wmt_uart *)PORT_TO_BASE(sport);

	#ifndef CONFIG_SERIAL_WMT_DMA
	uart->urier &= ~(URIER_ETXFAE | URIER_ETXFE);
	sport->port.read_status_mask &= ~URISR_TO_SM(URISR_TXFAE | URISR_TXFE);
	#else
	if ((unsigned int)(sport->port.membase) == UART0_BASE_ADDR) {
		uart->urier &= ~(URIER_ETXFAE | URIER_ETXFE);
		sport->port.read_status_mask &= ~URISR_TO_SM(URISR_TXFAE | URISR_TXFE);
	}
	#endif
}
/*
 * Interrupts may not be disabled on entry.
 */
static void wmt_start_tx(struct uart_port *port)
{
	struct wmt_port *sport = (struct wmt_port *)port;
	struct wmt_uart *uart = (struct wmt_uart *)PORT_TO_BASE(sport);

	/*{2007/11/10 JHT Support the VT8500 Serial Port Driver Because the
	 *                definition of URSIRT Bit[0] & Bit[3] are different.
	 *                Before VT8500 these bit are defined as RO, in VT8500
	 *                they are changed into the W1C. Therefore the xmit function
	 *                for the FIFO mode should be modified as well.
	 */
#ifdef CONFIG_SERIAL_WMT_DMA
	if ((unsigned int)(sport->port.membase) != UART0_BASE_ADDR) {
		uart->urlcr |= URLCR_DMAEN;
		uart->urier &= ~(URIER_ETXFAE | URIER_ETXFE);
	} else
		uart->urier &= ~(URIER_ETXFAE | URIER_ETXFE);
#else
	uart->urier &= ~(URIER_ETXFAE | URIER_ETXFE);
	#endif
	wmt_tx_chars(sport);
	/*}2007/11/10-JHT*/
#ifndef CONFIG_SERIAL_WMT_DMA
	sport->port.read_status_mask |= URISR_TO_SM(URISR_TXFAE | URISR_TXFE);
	uart->urier |= URIER_ETXFAE | URIER_ETXFE;
#else
	if ((unsigned int)(sport->port.membase) == UART0_BASE_ADDR) {
		sport->port.read_status_mask |= URISR_TO_SM(URISR_TXFAE | URISR_TXFE);
		uart->urier |= URIER_ETXFAE | URIER_ETXFE;
	}
	#endif
}

/*
 * Interrupts enabled
 */
static void wmt_stop_rx(struct uart_port *port)
{
/*
	struct wmt_port *sport = (struct wmt_port *)port;
	struct wmt_uart *uart = (struct wmt_uart *)PORT_TO_BASE(sport);


	uart->urier &= ~URIER_ERXFAF;
*/
}

/*
 * No modem control lines
 */
static void wmt_enable_ms(struct uart_port *port)
{
	struct wmt_port *sport = (struct wmt_port *)port;

	mod_timer(&sport->timer, jiffies);
}

#ifdef CONFIG_SERIAL_WMT_DMA
static unsigned int wmt_rx_count(struct wmt_port *sport)
{
	unsigned int rx_count = 0;
	unsigned int pos;

	pos = DMA_RX_POS(sport);

	if (pos == sport->last_pos) /*have no data to drain */
		return 0;

	if (sport->buffer_rx_count_selected == 0) {
		sport->buffer_selected = 0; /*shoud read buffer 0*/

		/*pos in the range of the buffer0*/
		if ((pos >= sport->uart_dma_tmp_phy0) &&
		    (pos <= (sport->uart_rx_dma_phy0_org + UART_BUFFER_SIZE))) {
			rx_count = (pos - (unsigned int)sport->uart_dma_tmp_phy0);
		} else if (pos == 0) {
			if (sport->uart_dma_tmp_phy0 == sport->uart_rx_dma_phy0_org)
				rx_count = 0;
			else {
				rx_count = (sport->uart_rx_dma_phy0_org + UART_BUFFER_SIZE)
					   - (unsigned int)sport->uart_dma_tmp_phy0;
			}
		} else if (((pos >= sport->uart_rx_dma_phy1_org) &&
			    (pos <= (sport->uart_rx_dma_phy1_org + UART_BUFFER_SIZE)))
			    ||
			    ((pos >= sport->uart_rx_dma_phy2_org) &&
			    (pos <= (sport->uart_rx_dma_phy2_org + UART_BUFFER_SIZE)))) {
			/* Buffer is full, dma pos in buffer 1 or 2, read all left data*/
			rx_count = (sport->uart_rx_dma_phy0_org + UART_BUFFER_SIZE) -
				   (unsigned int)sport->uart_dma_tmp_phy0;
		}
		sport->uart_dma_tmp_phy0  += rx_count; /*update tmp physical address*/
		sport->last_pos = sport->uart_dma_tmp_phy0;

		/* Buffer0 has reach to end*/
		if (sport->uart_dma_tmp_phy0 == (sport->uart_rx_dma_phy0_org + UART_BUFFER_SIZE)) {
			sport->buffer_rx_count_selected = 1;
			sport->uart_dma_tmp_phy0 = sport->uart_rx_dma_phy0_org;
		}
	} else if (sport->buffer_rx_count_selected == 1) {
		sport->buffer_selected = 1; /*shoud read buffer 1*/

		/*pos in the range of the buffer1*/
		if ((pos >= sport->uart_dma_tmp_phy1) &&
		    (pos <= (sport->uart_rx_dma_phy1_org + UART_BUFFER_SIZE))) {
			rx_count = (pos - (unsigned int)sport->uart_dma_tmp_phy1);
		} else if (pos == 0) {
			if (sport->uart_dma_tmp_phy1 == sport->uart_rx_dma_phy1_org)
				rx_count = 0;
			else {
				rx_count = (sport->uart_rx_dma_phy1_org + UART_BUFFER_SIZE) -
					   (unsigned int)sport->uart_dma_tmp_phy1;
			}
		} else if (((pos >= sport->uart_rx_dma_phy0_org) &&
			    (pos <= (sport->uart_rx_dma_phy0_org + UART_BUFFER_SIZE)))
			   ||
			   ((pos >= sport->uart_rx_dma_phy2_org) &&
			    (pos <= (sport->uart_rx_dma_phy2_org + UART_BUFFER_SIZE)))) {
			/* Buffer is full, dma pos in buffer 0 or 2, read all left data*/
			rx_count = (sport->uart_rx_dma_phy1_org + UART_BUFFER_SIZE) -
				   (unsigned int)sport->uart_dma_tmp_phy1;
		}
		sport->uart_dma_tmp_phy1  += rx_count;
		sport->last_pos = sport->uart_dma_tmp_phy1;
		/* Buffer1 has reach to end*/
		if (sport->uart_dma_tmp_phy1 == (sport->uart_rx_dma_phy1_org + UART_BUFFER_SIZE)) {
			sport->buffer_rx_count_selected = 2;
			sport->uart_dma_tmp_phy1 = sport->uart_rx_dma_phy1_org;
		}
	} else if (sport->buffer_rx_count_selected == 2) {
		sport->buffer_selected = 2; /*shoud read buffer 2*/

				/*pos in the range of the buffer1*/
		if ((pos >= sport->uart_dma_tmp_phy2) &&
		    (pos <= (sport->uart_rx_dma_phy2_org + UART_BUFFER_SIZE))) {
			rx_count = (pos - (unsigned int)sport->uart_dma_tmp_phy2);
		} else if (pos == 0) {
			if (sport->uart_dma_tmp_phy2 == sport->uart_rx_dma_phy2_org)
				rx_count = 0;
			else {
				rx_count = (sport->uart_rx_dma_phy2_org + UART_BUFFER_SIZE) -
					   (unsigned int)sport->uart_dma_tmp_phy2;
			}
		} else if (((pos >= sport->uart_rx_dma_phy0_org) &&
			    (pos <= (sport->uart_rx_dma_phy0_org + UART_BUFFER_SIZE)))
			    ||
			   ((pos >= sport->uart_rx_dma_phy1_org) &&
			    (pos <= (sport->uart_rx_dma_phy1_org + UART_BUFFER_SIZE)))) {
			/* Buffer is full, dma pos in buffer 0 or 1, read all left data*/
			rx_count = (sport->uart_rx_dma_phy2_org + UART_BUFFER_SIZE) -
				   (unsigned int)sport->uart_dma_tmp_phy2;
		}
		sport->uart_dma_tmp_phy2  += rx_count;
		sport->last_pos = sport->uart_dma_tmp_phy2;

		/* Buffer2 has reach to end*/
		if (sport->uart_dma_tmp_phy2 == (sport->uart_rx_dma_phy2_org + UART_BUFFER_SIZE)) {
			sport->buffer_rx_count_selected = 0;
			sport->uart_dma_tmp_phy2 = sport->uart_rx_dma_phy2_org;
		}
	}
	return rx_count;
}
/*
* we can use mu command to check memory content
*/
int volatile last_submit = 0x0;
void dump_rx_dma_buf(struct wmt_port *sport)
{
	unsigned int rx_count = 0;
	unsigned int pos;
	int i;
	int size=0;
	unsigned char *tmpbuf;
	pos = DMA_RX_POS(sport);
	printk("sport->last_pos:0x%x,pos:0x%x,sport->buffer_rx_count_selected:0x%x,last_submit:0x%x\n",sport->last_pos,pos,sport->buffer_rx_count_selected,last_submit);
	if ((sport->uart_rx_dma_phy0_org) <= pos && pos <= (sport->uart_rx_dma_phy0_org + UART_BUFFER_SIZE)){
		i = pos - sport->uart_rx_dma_phy0_org;
		tmpbuf = sport->uart_rx_dma_buf0_org + i;
		size = (unsigned int)(sport->uart_rx_dma_buf0_org) + UART_BUFFER_SIZE - (unsigned int)tmpbuf;
		if(size > 16)
			size = 16;
	}else if ((sport->uart_rx_dma_phy1_org)  <= pos && pos <=  (sport->uart_rx_dma_phy1_org + UART_BUFFER_SIZE)){
		i = pos - sport->uart_rx_dma_phy1_org;
		tmpbuf = sport->uart_rx_dma_buf1_org + i;
		size =(unsigned int)( sport->uart_rx_dma_buf1_org) + UART_BUFFER_SIZE - (unsigned int)tmpbuf;
		if(size > 16)
			size = 16;
	}else if ((sport->uart_rx_dma_phy2_org) <= pos && pos <=  (sport->uart_rx_dma_phy2_org + UART_BUFFER_SIZE)){
		i = pos - sport->uart_rx_dma_phy2_org;
		tmpbuf = sport->uart_rx_dma_buf2_org + i;
		size = (unsigned int)(sport->uart_rx_dma_buf2_org) + UART_BUFFER_SIZE - (unsigned int)tmpbuf;
		if(size > 16)
			size = 16;
	}

	for(i=0;i<size;i++)
		printk("0x%x ",*tmpbuf++);
	
	printk("\nsport->uart_rx_dma_phy0_org:0x%x\n",sport->uart_rx_dma_phy0_org);
	printk("sport->uart_rx_dma_phy1_org:0x%x\n",sport->uart_rx_dma_phy1_org);
	printk("sport->uart_rx_dma_phy2_org:0x%x\n",sport->uart_rx_dma_phy2_org);
}
#endif
/*
 * Inside the UART interrupt service routine dut to following
 * reason:
 *
 * URISR_RXFAF:  RX FIFO almost full (FIFO mode)
 * URISR_RXDF:   RX data register full (Register mode)
 * URISR_RXTOUT: RX timeout
 */

#ifndef CONFIG_SERIAL_WMT_DMA
static void
wmt_rx_chars(struct wmt_port *sport,  unsigned int status)
{
	struct tty_struct *tty = sport->port.state->port.tty;
	struct wmt_uart *uart = (struct wmt_uart *)PORT_TO_BASE(sport);
	unsigned int  flg, urfidx, ignored = 0;
	char ch;

	urfidx = URFIDX_RXFIDX(uart->urfidx);

	/*
	 * Check if there is data ready to be read.
	 *
	 * Note: We only receive characters.
	 */
	while ((status & URUSR_TO_SM(URUSR_RXDRDY)) && (URFIDX_RXFIDX(uart->urfidx))) {
		ch = (uart->urrxf[0] & 0xFF);
		/*urfidx--;*/

		sport->port.icount.rx++;

		flg = TTY_NORMAL;

		/*
		 * Check interrupt status information using status[URISR_bits].
		 *
		 * Notice that the error handling code is out of
		 * the main execution path and the URISR has already
		 * been read by ISR.
		 */
		if (status & URISR_TO_SM(URISR_PER | URISR_FER | URISR_RXDOVR)) {
			if (urfidx > 1) {
				if (uart_handle_sysrq_char(&sport->port, ch))
					goto ignore_char;
				/*
				 * Pop all TTY_NORMAL data.
				 */
				goto error_return;
			} else {
				/*
				 * Now we have poped up to the data with
				 * parity error or frame error.
				 */
				goto handle_error;
			}
		}

		if (uart_handle_sysrq_char(&sport->port, ch))
			goto ignore_char;

error_return:

		uart_insert_char(&sport->port, (status & 0xFFFF), URISR_TO_SM(URISR_RXDOVR) , ch, flg);

ignore_char:
		status &= 0xffff;       /* Keep URISR field*/
		status |= URUSR_TO_SM(uart->urusr);
	}
out:
	tty_flip_buffer_push(tty);
	return;

handle_error:
	/*
	 * Update error counters.
	 */
	if (status & URISR_TO_SM(URISR_PER))
		sport->port.icount.parity++;
	else
		if (status & URISR_TO_SM(URISR_FER)) {

	#ifdef CONFIG_SERIAL_WMT_BKSIG
			/*
			 * Experimental software patch for break signal detection.
			 *
			 * When I got there is a frame error in next frame data,
			 * I check the next data to judge if it is a break signal.
			 *
			 * FIXME: Open these if Bluetooth or IrDA need this patch.
			 *        Dec.29.2004 by Harry.
			 */
			if ((ch & RX_PERMASK) == 0) {
				sport->port.icount.brk++;
				uart_handle_break(&sport->port);
			} else
				sport->port.icount.frame++;

	#else   /* Don't support break sinal detection */

			sport->port.icount.frame++;

	#endif

		}

	/*
	 * RX Over Run event
	 */
	if (status & URISR_TO_SM(URISR_RXDOVR))
		sport->port.icount.overrun++;

	if (status & sport->port.ignore_status_mask) {
		if (++ignored > 100)
			goto out;
		goto ignore_char;
	}

	/*
	 * Second, handle the events which we're interesting to listen.
	 */
	status &= sport->port.read_status_mask;

	if (status & URISR_TO_SM(URISR_PER))
		flg = TTY_PARITY;
	else
		if (status & URISR_TO_SM(URISR_FER)) {

	#ifdef CONFIG_SERIAL_WMT_BKSIG
			/* Software patch for break signal detection.
			 *
			 * When I got there is a frame error in next frame data,
			 * I check the next data to judge if it is a break signal.
			 *
			 * FIXME: Open these if Bluetooth or IrDA need this patch.
			 *        Dec.29.2004 by Harry.
			 */
			if (sport->port.read_status_mask & SW_BKSIG) {
				if ((ch & RX_PERMASK) == 0) {
					DEBUG_INTR("handling break....");
					flg = TTY_BREAK;
					/*goto error_return;*/
				} else {
					flg = TTY_FRAME;
					/*goto error_return;*/
				}
			} else {
				flg = TTY_FRAME;
				/*goto error_return;*/
			}

	#else   /* Don't support break sinal detection */

			flg = TTY_FRAME;

	#endif
		}

	if (status & URISR_TO_SM(URISR_RXDOVR)) {
		/*
		 * Overrun is special, since it's reported
		 * immediately, and doesn't affect the current
		 * character.
		 */

		ch = 0;
		flg	= TTY_OVERRUN;
	}
	#ifdef SUPPORT_SYSRQ
	sport->port.sysrq = 0;
	#endif
	goto error_return;
}
#else


static inline int put_strings_tty_io(struct tty_struct *tty,
			const unsigned char *chars, char flag, size_t size)
{
	int i;

	i = tty_insert_flip_string_fixed_flag(tty, chars, flag, size);
	tty_flip_buffer_push(tty);
	return i;
}

static void
wmt_rx_chars(struct wmt_port *sport, unsigned int status)
{
	struct tty_struct *tty = sport->port.state->port.tty;
	struct wmt_uart *uart = (struct wmt_uart *)PORT_TO_BASE(sport);
	unsigned int receive_rx_count = 0;
	unsigned long flags;

	unsigned int  flg, urfidx, ignored = 0;
	char ch;
	char *pchar = NULL;
	char rx_flag;

	urfidx = URFIDX_RXFIDX(uart->urfidx);

	/*
	 * Check if there is data ready to be read.
	 *
	 * Note: We only receive characters.
	 */

	if ((unsigned int)(sport->port.membase) != UART0_BASE_ADDR) {

		spin_lock_irqsave(&sport->port.lock, flags);
		receive_rx_count = wmt_rx_count(sport);
		spin_unlock_irqrestore(&sport->port.lock, flags);

		rx_flag = TTY_NORMAL;

		if (status & URISR_TO_SM(URISR_PER))
			rx_flag = TTY_PARITY;

		if (status & URISR_TO_SM(URISR_FER))
			rx_flag = TTY_FRAME;

		if (receive_rx_count > 0) {
			if (sport->buffer_selected == 0) {
				pchar = sport->uart_dma_tmp_buf0;
				sport->uart_dma_tmp_buf0 += receive_rx_count;
				put_strings_tty_io(tty, pchar, rx_flag, receive_rx_count);

				if (sport->uart_dma_tmp_buf0 ==
				  (sport->uart_rx_dma_buf0_org + UART_BUFFER_SIZE)) {
					sport->uart_dma_tmp_buf0 = sport->uart_rx_dma_buf0_org;
				}

			} else if (sport->buffer_selected == 1) {
				pchar = sport->uart_dma_tmp_buf1;
				sport->uart_dma_tmp_buf1 += receive_rx_count;
				put_strings_tty_io(tty, pchar, rx_flag, receive_rx_count);

				if (sport->uart_dma_tmp_buf1 ==
				  (sport->uart_rx_dma_buf1_org + UART_BUFFER_SIZE)) {
					sport->uart_dma_tmp_buf1 = sport->uart_rx_dma_buf1_org;
				}
			} else if (sport->buffer_selected == 2) {
				pchar = sport->uart_dma_tmp_buf2;
				sport->uart_dma_tmp_buf2 += receive_rx_count ;
				put_strings_tty_io(tty, pchar, rx_flag, receive_rx_count);

				if (sport->uart_dma_tmp_buf2 ==
				   (sport->uart_rx_dma_buf2_org + UART_BUFFER_SIZE)) {
					sport->uart_dma_tmp_buf2 = sport->uart_rx_dma_buf2_org;
				}
			}
			sport->port.icount.rx += receive_rx_count;

			if (rx_flag == TTY_PARITY)
				sport->port.icount.parity += receive_rx_count;

			if (rx_flag == TTY_FRAME)
				sport->port.icount.frame += receive_rx_count;

			receive_rx_count = 0;
		}
		tty_flip_buffer_push(tty);
		return;

	} else {

		while ((status & URUSR_TO_SM(URUSR_RXDRDY)) && (URFIDX_RXFIDX(uart->urfidx))) {

			ch = (unsigned int)(uart->urrxf[0] & 0xFF);
			sport->port.icount.rx++;

			flg = TTY_NORMAL;

			/*
			 * Check interrupt status information using status[URISR_bits].
			 *
			 * Notice that the error handling code is out of
			 * the main execution path and the URISR has already
			 * been read by ISR.
			 */
			if (status & URISR_TO_SM(URISR_PER | URISR_FER | URISR_RXDOVR)) {
				if (urfidx > 1) {
					if (uart_handle_sysrq_char(&sport->port, ch))
						goto ignore_char2;
					/*
					 * Pop all TTY_NORMAL data.
					 */
					goto error_return2;
				} else {
					/*
					 * Now we have poped up to the data with
					 * parity error or frame error.
					 */
					goto handle_error2;
				}
			}

			if (uart_handle_sysrq_char(&sport->port, ch))
				goto ignore_char2;

error_return2:
			uart_insert_char(&sport->port, (status & 0xFFFF),
				URISR_TO_SM(URISR_RXDOVR) , ch, flg);
ignore_char2:
			status &= 0xffff;       /* Keep URISR field*/
			status |= URUSR_TO_SM(uart->urusr);
		}
out2:
		tty_flip_buffer_push(tty);
		return;

handle_error2:
		/*
		 * Update error counters.
		 */
		if (status & URISR_TO_SM(URISR_PER))
			sport->port.icount.parity++;
		else {
			if (status & URISR_TO_SM(URISR_FER)) {
#ifdef CONFIG_SERIAL_WMT_BKSIG
				/*
				 * Experimental software patch for break signal detection.
				 *
				 * When I got there is a frame error in next frame data,
				 * I check the next data to judge if it is a break signal.
				 *
				 * FIXME: Open these if Bluetooth or IrDA need this patch.
				 *        Dec.29.2004 by Harry.
				 */
				if ((ch & RX_PERMASK) == 0) {
					sport->port.icount.brk++;
					uart_handle_break(&sport->port);
				} else
					sport->port.icount.frame++;

#else   /* Don't support break sinal detection */
				sport->port.icount.frame++;
#endif
			}
		}
		/*
		 * RX Over Run event
		 */
		if (status & URISR_TO_SM(URISR_RXDOVR))
			sport->port.icount.overrun++;

		if (status & sport->port.ignore_status_mask) {
			if (++ignored > 100)
				goto out2;
			goto ignore_char2;
		}

		/*
		 * Second, handle the events which we're interesting to listen.
		 */
		status &= sport->port.read_status_mask;

		if (status & URISR_TO_SM(URISR_PER))
			flg = TTY_PARITY;
		else
			if (status & URISR_TO_SM(URISR_FER)) {

#ifdef CONFIG_SERIAL_WMT_BKSIG
				/* Software patch for break signal detection.
				 *
				 * When I got there is a frame error in next frame data,
				 * I check the next data to judge if it is a break signal.
				 *
				 * FIXME: Open these if Bluetooth or IrDA need this patch.
				 *        Dec.29.2004 by Harry.
				 */
				if (sport->port.read_status_mask & SW_BKSIG) {
					if ((ch & RX_PERMASK) == 0) {
						DEBUG_INTR("handling break....");
						flg = TTY_BREAK;
						/*goto error_return;*/
					} else {
						flg = TTY_FRAME;
						/*goto error_return;*/
					}
				} else {
					flg = TTY_FRAME;
					/*goto error_return;*/
				}
#else   /* Don't support break sinal detection */
				flg = TTY_FRAME;
#endif
		}

		if (status & URISR_TO_SM(URISR_RXDOVR)) {
			/*
			 * Overrun is special, since it's reported
			 * immediately, and doesn't affect the current
			 * character.
			 */

			ch = 0;
			flg	= TTY_OVERRUN;
		}
		#ifdef SUPPORT_SYSRQ
		sport->port.sysrq = 0;
		#endif
		goto error_return2;
	}

}
#endif
/*
 * Inside the UART interrupt service routine dut to following
 * reason:
 *
 * URISR_TXFAE: TX FIFO almost empty (FIFO mode)
 * URISR_TXFE:  TX FIFO empty(FIFO mode)
 */
#ifndef CONFIG_SERIAL_WMT_DMA
static void wmt_tx_chars(struct wmt_port *sport)
{
	struct circ_buf *xmit = &sport->port.state->xmit;
	struct wmt_uart *uart = (struct wmt_uart *)PORT_TO_BASE(sport);

	if (sport->port.x_char) {
		/*
		 * Fill character to the TX FIFO entry.
		 */
		uart->urtxf[0] = sport->port.x_char;
		sport->port.icount.tx++;
		sport->port.x_char = 0;
		return;
	}

	/*Check the modem control lines before transmitting anything.*/
	wmt_mctrl_check(sport);

	if (uart_circ_empty(xmit) || uart_tx_stopped(&sport->port)) {
		wmt_stop_tx(&sport->port);
		return;
	}

	/*{2007/11/10 JHT Support the WMT Serial Port Driver Because the
	 *                definition of URSIRT Bit[0] & Bit[3] are different.
	 *                Before WMT these bit are defined as RO, in WMT
	 *                they are changed into the W1C. Therefore the xmit function
	 *                for the FIFO mode should be modified as well.
	 */
	while ((uart->urfidx & 0x1F) < 16) {
		if (uart_circ_empty(xmit))
			break;

		if (uart->urusr & URUSR_TXDBSY)
			continue;
		uart->urtxf[0] = xmit->buf[xmit->tail];
		xmit->tail = (xmit->tail + 1) & (UART_XMIT_SIZE - 1);
		sport->port.icount.tx++;
	}
	/*}2007/11/10-JHT*/

	if (uart_circ_chars_pending(xmit) < WAKEUP_CHARS)
		uart_write_wakeup(&sport->port);

	if (uart_circ_empty(xmit))
		wmt_stop_tx(&sport->port);
}
#else

static void wmt_tx_chars(struct wmt_port *sport)
{
	struct circ_buf *xmit = &sport->port.state->xmit;
	int head = xmit->head;
	int tail = xmit->tail;
	struct wmt_uart *uart = (struct wmt_uart *)PORT_TO_BASE(sport);
	char *dma_buf = sport->uart_tx_dma_buf0_org;
	unsigned int tx_count;

	if (sport->port.x_char) {
		/*
		 * Fill character to the TX FIFO entry.
		 */
		if ((unsigned int)(sport->port.membase) != UART0_BASE_ADDR) {
			/*Dma is still running*/
			if (sport->uart_tx_dma_flag != DMA_TX_END)
				return;

			*dma_buf = sport->port.x_char;
			sport->uart_tx_dma_flag = DMA_TX_CHAR;
			DMA_TX_START(sport, sport->uart_tx_dma_phy0_org, 1);
		} else {
			uart->urtxf[0] = sport->port.x_char;
			sport->port.icount.tx++;
			sport->port.x_char = 0;
		}
		return;
	}
	/*Check the modem control lines before transmitting anything.*/
	wmt_mctrl_check(sport);

	if (uart_circ_empty(xmit) || uart_tx_stopped(&sport->port)) {
		wmt_stop_tx(&sport->port);
		return;
	}

	/*{2007/11/10 JHT Support the WMT Serial Port Driver Because the
	 *                definition of URSIRT Bit[0] & Bit[3] are different.
	 *                Before WMT these bit are defined as RO, in WMT
	 *                they are changed into the W1C. Therefore the xmit function
	 *                for the FIFO mode should be modified as well.
	 */


	if ((unsigned int)(sport->port.membase) != UART0_BASE_ADDR) {
        while((uart->urusr  & URUSR_TXON));
		if(sport->port.line == mtk6622_tty){
			while ((uart->urusr & URUSR_TXDBSY));
		}

		if (sport->uart_tx_dma_flag == DMA_TX_END) {
			sport->uart_tx_dma_flag = DMA_TX_XMIT;
			tx_count = 0;
			head = xmit->head;
			tail = xmit->tail;
			sport->uart_tx_count = 0;
			while (head != tail) {
				*dma_buf = xmit->buf[tail];
				tail = (tail + 1) & (UART_XMIT_SIZE - 1);
				dma_buf++;
				tx_count++;
				sport->uart_tx_count++;

				if (tx_count == UART_BUFFER_SIZE)
					break;
			}
			DMA_TX_START(sport, sport->uart_tx_dma_phy0_org, tx_count);
		}
	} else {
		while ((uart->urfidx & 0x1F) < 16) {
			if (uart_circ_empty(xmit))
				break;

			if (uart->urusr & URUSR_TXDBSY)
				continue;
			uart->urtxf[0] = xmit->buf[xmit->tail];
			xmit->tail = (xmit->tail + 1) & (UART_XMIT_SIZE - 1);
			sport->port.icount.tx++;
		}


		if (uart_circ_chars_pending(xmit) < WAKEUP_CHARS)
			uart_write_wakeup(&sport->port);

		if (uart_circ_empty(xmit))
			wmt_stop_tx(&sport->port);
	}

}
#endif

static irqreturn_t wmt_int(int irq, void *dev_id)
{
	struct wmt_port *sport = dev_id;
	struct wmt_uart *uart = (struct wmt_uart *)PORT_TO_BASE(sport);
	unsigned int status, pass_counter = 0;
	unsigned long flags;

	spin_lock_irqsave(&sport->port.lock, flags);
	/*
	 * Put interrupts status information to status bit[0:15]
	 * Put UART status register to status bit[16:31].
	 */

	status = URISR_TO_SM(uart->urisr) | URUSR_TO_SM(uart->urusr);
	uart->urisr |= SM_TO_URISR(status);

	do {
		/*
		 * First, we handle RX events.
		 *
		 * RX FIFO Almost Full.         (URUSR_RXFAF)
		 * RX Timeout.                  (URISR_RXTOUT)
		 * Frame error                  (URISR_FER)
		 *
		 * Note that also allow URISR_FER and URISR_PER event to do rx.
		 */

		if (status & URISR_TO_SM(URISR_RXFAF | URISR_RXFF | URISR_RXTOUT |\
								 URISR_PER | URISR_FER)) {

			if ((unsigned int)(sport->port.membase) == UART0_BASE_ADDR)
				wmt_rx_chars(sport,  status);

			if ((unsigned int)(sport->port.membase) != UART0_BASE_ADDR) {
				uart->urier = 0;	/*close uart rx interruption */
				uart->urisr |= SM_TO_URISR(status);	/* clear uart rx int status */
				sport->uart_rx_dma_flag = 0;
				mod_timer(&sport->rx_timer, jiffies + rx_timeout);
				break;
			}
		}
		/*
		 * Second, we handle TX events.
		 *
		 * If there comes a TX FIFO Almost event, try to fill TX FIFO.
		 */

#ifndef CONFIG_SERIAL_WMT_DMA
		wmt_tx_chars(sport);
#else
		if ((unsigned int)(sport->port.membase) == UART0_BASE_ADDR)
			wmt_tx_chars(sport);
#endif
		if (pass_counter++ > WMT_ISR_PASS_LIMIT)
			break;

		/*
		 * Update UART interrupt status and general status information.
		 */
		status = (URISR_TO_SM(uart->urisr) | URUSR_TO_SM(uart->urusr));
		uart->urisr |= SM_TO_URISR(status);

		/*
		 * Inside the loop, we handle events that we're interesting.
		 */
		status &= sport->port.read_status_mask;

		/*
		 * Continue loop while following condition:
		 *
		 * TX FIFO Almost Empty.        (URISR_TXFAE)
		 * RX FIFO Almost Full.         (URISR_RXFAF)
		 * RX Receive Time Out.         (URISR_RXTOUT)
		 */
	} while (status & (URISR_TXFE   |
			   URISR_TXFAE  |
			   URISR_RXFAF  |
			   URISR_RXFF   |
			   URISR_RXTOUT |
			   URISR_RXDOVR));

	spin_unlock_irqrestore(&sport->port.lock, flags);
	return IRQ_HANDLED;
}

/* wmt_tx_empty()
 *
 * Return TIOCSER_TEMT when transmitter is not busy.
 */
static unsigned int wmt_tx_empty(struct uart_port *port)
{
	struct wmt_port *sport = (struct wmt_port *)port;
	struct wmt_uart *uart = (struct wmt_uart *)PORT_TO_BASE(sport);

	return (uart->urusr & URUSR_TXDBSY) ? 0 : TIOCSER_TEMT;
}

/* wmt_get_mctrl()
 *
 * Returns the current state of modem control inputs.
 *
 * Note: Only support CTS now.
 */
static u_int wmt_get_mctrl(struct uart_port *port)
{
	u_int ret = TIOCM_DSR | TIOCM_CAR;
	struct wmt_port *sport = (struct wmt_port *)port;
	struct wmt_uart *uart = (struct wmt_uart *)PORT_TO_BASE(sport);

	ret |= (uart->urusr & URUSR_CTS) ? TIOCM_CTS : 0;

	return ret;
}

/* wmt_set_mctrl()
 *
 * This function sets the modem control lines for port described
 * by 'port' to the state described by mctrl. More detail please
 * refer to Documentation/serial/driver.
 *
 * Note: Only support RTS now.
 */
static void wmt_set_mctrl(struct uart_port *port, u_int mctrl)
{
	struct wmt_port *sport = (struct wmt_port *)port;
	struct wmt_uart *uart = (struct wmt_uart *)PORT_TO_BASE(sport);

	if (mctrl & TIOCM_RTS)
		uart->urlcr |= URLCR_RTS;
	else
		uart->urlcr &= ~URLCR_RTS;

}

/*
 * Interrupts always disabled.
 */
static void wmt_break_ctl(struct uart_port *port, int break_state)
{
	struct wmt_port *sport = (struct wmt_port *)port;
	struct wmt_uart *uart = (struct wmt_uart *)PORT_TO_BASE(sport);
	unsigned long flags;

	spin_lock_irqsave(&sport->port.lock, flags);

	if (break_state == -1) {
		int i;
		unsigned int urbrd = URBRD_BRD(uart->urdiv);

		/*
		 * This looks something tricky.
		 * Anyway, we need to get current baud rate divisor,
		 * search bcv in baud_table[], program it into
		 * URBKR, then generate break signal.
		 */
		for (i = 0; i < BAUD_TABLE_SIZE; i++) {
			if ((baud_table[i].brd & URBRD_BRDMASK) == urbrd)
				break;
		}

		if (i < BAUD_TABLE_SIZE) {
			uart->urbkr = URBKR_BCV(baud_table[i].bcv);
			uart->urlcr |= URLCR_BKINIT;
		}
	}

	spin_unlock_irqrestore(&sport->port.lock, flags);
}

static char *wmt_uartname[] = {
	"uart0",
	"uart1",
#ifdef CONFIG_UART_2_3_ENABLE
	"uart2",
	"uart3",
#endif
};

#ifdef CONFIG_SERIAL_WMT_DMA
static void uart_dma_callback_rx(void *data)
{
	struct wmt_port	*sport = data;
#ifdef UART_DEBUG	
	static int out_range =0x00;
	if(dma_write_index >=4096){
		printk("out of 4096\n");
		dma_write_index = 0x00;
		out_range++;
	}
#endif
	if (sport->buffer_used == 0) {
		sport->buffer_used = 1;
		DMA_RX_START(sport, sport->uart_rx_dma_phy0_org, UART_BUFFER_SIZE);
	} else if (sport->buffer_used == 1) {
		sport->buffer_used = 2;
		DMA_RX_START(sport, sport->uart_rx_dma_phy1_org, UART_BUFFER_SIZE);
	} else if (sport->buffer_used == 2) {
		sport->buffer_used = 0;
		DMA_RX_START(sport, sport->uart_rx_dma_phy2_org, UART_BUFFER_SIZE);
	}
#ifdef UART_DEBUG		
	DMA_pbuf[dma_write_index] = sport->buffer_used + (out_range<<24);
	//printk("0x%x dma rx DMA_pbuf[%d]:0x%x\n",(unsigned int)DMA_pbuf,dma_write_index,DMA_pbuf[dma_write_index]);
	if(sport->buffer_rx_count_selected == 0){
		COUNT_pbuf[dma_write_index] = sport->uart_dma_tmp_phy0;
	}else if(sport->buffer_rx_count_selected == 1)	{
		COUNT_pbuf[dma_write_index] = sport->uart_dma_tmp_phy1;
	}else if(sport->buffer_rx_count_selected == 2)	{
		COUNT_pbuf[dma_write_index] = sport->uart_dma_tmp_phy2;
	}

	if(sport->buffer_selected == 0){
		CPU_pbuf[dma_write_index] = sport->uart_dma_tmp_buf0;
	}else if(sport->buffer_selected == 1){
		CPU_pbuf[dma_write_index] = sport->uart_dma_tmp_buf1;
	}else if(sport->buffer_selected == 2){
		CPU_pbuf[dma_write_index] = sport->uart_dma_tmp_buf2;
	}
	dma_write_index++;
#endif	

}
#ifdef UART_DEBUG
void print_dma_count_cpu_buf_pos(struct wmt_port *sport)
{
	int i;
	for(i=0;i<UART_BUFFER_SIZE;i++){
		//printk("0x%x DMA_pbuf[%d]:0x%x\n",DMA_pbuf,i,DMA_pbuf[i]);
		if(DMA_pbuf[i] == 0x5a5a5a5a)
			break;
		printk("dma buf index:0x%x ",DMA_pbuf[i]);
		if((sport->uart_rx_dma_phy0_org <= COUNT_pbuf[i]) && (COUNT_pbuf[i] <= (sport->uart_rx_dma_phy0_org+UART_BUFFER_SIZE))){
			printk("count buf index:0x00 offset:0x%x ",COUNT_pbuf[i]-sport->uart_rx_dma_phy0_org);
		}else if((sport->uart_rx_dma_phy1_org <= COUNT_pbuf[i]) && (COUNT_pbuf[i] <= (sport->uart_rx_dma_phy1_org+UART_BUFFER_SIZE))){
			printk("count buf index:0x01 offset:0x%x ",COUNT_pbuf[i]-sport->uart_rx_dma_phy1_org);
		}if((sport->uart_rx_dma_phy2_org <= COUNT_pbuf[i]) && (COUNT_pbuf[i] <= (sport->uart_rx_dma_phy2_org+UART_BUFFER_SIZE))){
			printk("count buf index:0x02 offset:0x%x ",COUNT_pbuf[i]-sport->uart_rx_dma_phy2_org);
		}

		if((sport->uart_rx_dma_buf0_org <= CPU_pbuf[i]) && (CPU_pbuf[i] <= (sport->uart_rx_dma_buf0_org+UART_BUFFER_SIZE))){
			printk("cpu buf index:0x00 offset:0x%x\n",CPU_pbuf[i]-(unsigned int)sport->uart_rx_dma_buf0_org);
		}else if((sport->uart_rx_dma_buf1_org <= CPU_pbuf[i]) && (CPU_pbuf[i] <= (sport->uart_rx_dma_buf1_org+UART_BUFFER_SIZE))){
			printk("cpu buf index:0x01 offset:0x%x\n",CPU_pbuf[i]-(unsigned int)sport->uart_rx_dma_buf1_org);
		}if((sport->uart_rx_dma_buf2_org <= CPU_pbuf[i]) && (CPU_pbuf[i] <= (sport->uart_rx_dma_buf2_org+UART_BUFFER_SIZE))){
			printk("cpu buf index:0x02 offset:0x%x\n",CPU_pbuf[i]-(unsigned int)sport->uart_rx_dma_buf2_org);
		}
		printk("\n");
	}
}
#endif
static void uart_dma_callback_tx(void *data)
{
	struct wmt_port	*sport = data;
	unsigned long flags;
	sport->dma_tx_cnt++;
	spin_lock_irqsave(&sport->port.lock, flags);

	if (sport->uart_tx_dma_flag == DMA_TX_CHAR) {
		sport->port.icount.tx++;
		sport->port.x_char = 0;
	} else {
		sport->port.state->xmit.tail =
			(sport->port.state->xmit.tail + sport->uart_tx_count) & (UART_XMIT_SIZE - 1);
		sport->port.icount.tx += sport->uart_tx_count;
	}
	sport->uart_tx_dma_flag = DMA_TX_END;

	wmt_tx_chars(sport);

	if (uart_circ_chars_pending(&sport->port.state->xmit) < WAKEUP_CHARS)
		uart_write_wakeup(&sport->port);

	if (uart_circ_empty(&sport->port.state->xmit))
		wmt_stop_tx(&sport->port);

	spin_unlock_irqrestore(&sport->port.lock, flags);

}
#endif

wmt_uart1_pre_init(void)
{
	GPIO_CTRL_GP18_UART_BYTE_VAL &= ~(BIT4 | BIT5);
	auto_pll_divisor(DEV_UART1, CLK_ENABLE, 0, 0);
	printk("wmt_uart1_pre_init\n");
}

wmt_uart1_post_deinit(void)
{
	GPIO_CTRL_GP18_UART_BYTE_VAL |= (BIT4 | BIT5);
	auto_pll_divisor(DEV_UART1, CLK_DISABLE, 0, 0);
	printk("wmt_uart1_post_deinit\n");
}

static int wmt_startup(struct uart_port *port)
{
	struct wmt_port *sport = (struct wmt_port *)port;
	struct wmt_uart *uart = (struct wmt_uart *)PORT_TO_BASE(sport);
	char *uartname = NULL;
	int retval;
	int i;
	unsigned long flags;

	switch (sport->port.irq) {

	case IRQ_UART0:
		uartname = wmt_uartname[0];
/*#ifdef CONFIG_SERIAL_WMT_DMA
		sport->port.dma_rx_dev = UART_0_RX_DMA_REQ;
		sport->port.dma_tx_dev = UART_0_TX_DMA_REQ;
		sport->id			= "uart0";
		sport->port.dma_rx_cfg = dma_device_cfg_table[UART_0_RX_DMA_REQ];
		sport->port.dma_tx_cfg = dma_device_cfg_table[UART_0_TX_DMA_REQ];
#endif*/
		break;

	case IRQ_UART1:
		wmt_uart1_pre_init();//added by rubbitxiao
		uartname = wmt_uartname[1];
#ifdef CONFIG_SERIAL_WMT_DMA
		sport->dma_rx_dev = UART_1_RX_DMA_REQ;
		sport->dma_tx_dev = UART_1_TX_DMA_REQ;
		sport->id = "uart1";
		sport->dma_rx_cfg = dma_device_cfg_table[UART_1_RX_DMA_REQ];
		sport->dma_tx_cfg = dma_device_cfg_table[UART_1_TX_DMA_REQ];
#endif
		break;

#ifdef CONFIG_UART_2_3_ENABLE
	case IRQ_UART2:
		uartname = wmt_uartname[2];
#ifdef CONFIG_SERIAL_WMT_DMA
		sport->dma_rx_dev = UART_2_RX_DMA_REQ;
		sport->dma_tx_dev = UART_2_TX_DMA_REQ;
		sport->id = "uart2";
		sport->dma_rx_cfg = dma_device_cfg_table[UART_2_RX_DMA_REQ];
		sport->dma_tx_cfg = dma_device_cfg_table[UART_2_TX_DMA_REQ];
#endif
		break;
	case IRQ_UART3:
		uartname = wmt_uartname[3];
#ifdef CONFIG_SERIAL_WMT_DMA
		sport->dma_rx_dev = UART_3_RX_DMA_REQ;
		sport->dma_tx_dev = UART_3_TX_DMA_REQ;
		sport->id = "uart3";
		sport->dma_rx_cfg = dma_device_cfg_table[UART_3_RX_DMA_REQ];
		sport->dma_tx_cfg = dma_device_cfg_table[UART_3_TX_DMA_REQ];
#endif
		break;
#endif

	}
#ifdef CONFIG_SERIAL_WMT_DMA
	if ((unsigned int)(sport->port.membase) != UART0_BASE_ADDR) {
		memset(sport->uart_rx_dma_buf0_org, 0x0, UART_BUFFER_SIZE);
		memset(sport->uart_rx_dma_buf1_org, 0x0, UART_BUFFER_SIZE);
		memset(sport->uart_rx_dma_buf2_org, 0x0, UART_BUFFER_SIZE);
		memset(sport->uart_tx_dma_buf0_org, 0x0, UART_BUFFER_SIZE);
		sport->uart_dma_tmp_buf0 = sport->uart_rx_dma_buf0_org;
		sport->uart_dma_tmp_phy0 = sport->uart_rx_dma_phy0_org;
		sport->uart_dma_tmp_buf1 = sport->uart_rx_dma_buf1_org;
		sport->uart_dma_tmp_buf2 = sport->uart_rx_dma_buf2_org;
		sport->uart_dma_tmp_phy1 = sport->uart_rx_dma_phy1_org;
		sport->uart_dma_tmp_phy2 = sport->uart_rx_dma_phy2_org;
		sport->buffer_used = 0;   /*to record which buf DMA hardware used*/
		sport->buffer_selected = 0; /* to record which buf software is used to put data to kernel*/
		sport->buffer_rx_count_selected = 0; /* to record which buf rx_count is used*/
		sport->rx_dmach = NULL_DMA;
		sport->tx_dmach = NULL_DMA;
		sport->last_pos = 0;
		sport->uart_tx_dma_flag = DMA_TX_END;
		sport->uart_tx_count = 0;
		sport->dma_tx_cnt = 0x00;
		init_timer(&sport->rx_timer);
		sport->rx_timer.function = wmt_rx_timeout;
		sport->rx_timer.data = (unsigned long)sport;
		DMA_RX_REQUEST(sport, uart_dma_callback_rx);
		DMA_TX_REQUEST(sport, uart_dma_callback_tx);
		wmt_setup_dma(sport->rx_dmach, sport->dma_rx_cfg);
		wmt_setup_dma(sport->tx_dmach, sport->dma_tx_cfg);
		DMA_RX_START(sport, sport->uart_rx_dma_phy0_org, UART_BUFFER_SIZE);
		DMA_RX_START(sport, sport->uart_rx_dma_phy1_org, UART_BUFFER_SIZE);
		DMA_RX_START(sport, sport->uart_rx_dma_phy2_org, UART_BUFFER_SIZE);
#ifdef UART_DEBUG
		DMA_pbuf  = kmalloc(UART_BUFFER_SIZE*sizeof(unsigned int*), GFP_KERNEL);
		COUNT_pbuf  = kmalloc(UART_BUFFER_SIZE*sizeof(unsigned int*), GFP_KERNEL);
		CPU_pbuf  = kmalloc(UART_BUFFER_SIZE*sizeof(unsigned int*), GFP_KERNEL);
		if(!DMA_pbuf || !COUNT_pbuf || !CPU_pbuf){
			printk("kmalloc buf for debug buf failed\n");
			return;
		}else{
			memset(DMA_pbuf,0x5a,UART_BUFFER_SIZE*sizeof(unsigned int*));
			memset(COUNT_pbuf,0x00,UART_BUFFER_SIZE*sizeof(unsigned int*));
			memset(CPU_pbuf,0x00,UART_BUFFER_SIZE*sizeof(unsigned int*));			
		}
		dma_write_index =0x00;
#endif
		{
			char uboot_buf[256];
			int varlen = sizeof(uboot_buf);
			if(wmt_getsyspara("wmt.bt.tty",uboot_buf,&varlen) == 0) 
			{
			  	sscanf(uboot_buf,"%d",&mtk6622_tty);
				printk("mtk6622_tty:%d\n",mtk6622_tty);
				if(1<=mtk6622_tty && mtk6622_tty <=3){
					printk("wmt.bt.tty is correct\n");
				}else{
					printk("wmt.bt.tty is illegal\n");
					mtk6622_tty = -1;
				}
			}else{
				printk("have not set uboot variant:wmt.bt.tty\n");
			}
		}
	}
#endif
	/*
	 * Allocate the IRQ
	 */
	retval = request_irq(sport->port.irq, wmt_int, 0, uartname, sport);
	if (retval)
		return retval;

	/*
	 * Setup the UART clock divisor
	 */
	for (i = 0; i < BAUD_TABLE_SIZE; i++) {
		if (baud_table[i].baud == 115200)
			break;
	}
	spin_lock_irqsave(&sport->port.lock, flags);

	uart->urdiv = baud_table[i].brd;

	/* Disable TX,RX*/
	uart->urlcr = 0;
	/* Disable all interrupt*/
	uart->urier = 0;

	/*Reset TX,RX Fifo*/
	uart->urfcr = URFCR_TXFRST | URFCR_RXFRST;

	while (uart->urfcr)
		;

	/* Disable Fifo*/
	uart->urfcr &= ~(URFCR_FIFOEN);

	uart->urlcr |=  (URLCR_DLEN & ~URLCR_STBLEN & ~URLCR_PTYEN);
#ifdef CONFIG_SERIAL_WMT_DMA
	if ((unsigned int)(sport->port.membase) != UART0_BASE_ADDR) {
		uart->urfcr = URFCR_FIFOEN | URFCR_TXFLV(8) | URFCR_RXFLV(1) | URFCR_TRAIL;
		uart->urtod = 0x0a;
	} else
		uart->urfcr = URFCR_FIFOEN | URFCR_TXFLV(8) | URFCR_RXFLV(4);
#else
	/* Enable Fifo, Tx 16 , Rx 16*/
	uart->urfcr = URFCR_FIFOEN | URFCR_TXFLV(8) | URFCR_RXFLV(4);
#endif
	/* Enable Fifo, Tx 8 , Rx 8*/
#ifdef CONFIG_SERIAL_WMT_DMA
	if ((unsigned int)(sport->port.membase) != UART0_BASE_ADDR) {
		uart->urlcr |= URLCR_RXEN | URLCR_TXEN | URLCR_DMAEN | URLCR_RCTSSW;
		uart->urier = URIER_ERXFAF | URIER_ERXFF | URIER_ERXTOUT |
			      URIER_EPER | URIER_EFER | URIER_ERXDOVR;
	} else {
		uart->urlcr |= URLCR_RXEN | URLCR_TXEN | URLCR_RCTSSW;
		uart->urier = URIER_ERXFAF | URIER_ERXFF | URIER_ERXTOUT |
			      URIER_EPER | URIER_EFER | URIER_ERXDOVR;
	}
#else
	uart->urlcr |= URLCR_RXEN | URLCR_TXEN | URLCR_RCTSSW;
	uart->urier = URIER_ERXFAF | URIER_ERXFF | URIER_ERXTOUT | URIER_EPER |
		      URIER_EFER | URIER_ERXDOVR;

#endif
	/*
	 * Enable RX FIFO almost full, timeout, and overrun interrupts.
	 */

	/*
	 * Enable modem status interrupts
	 */

	wmt_enable_ms(&sport->port);
	spin_unlock_irqrestore(&sport->port.lock, flags);

	return 0;
}

static void wmt_shutdown(struct uart_port *port)
{
	struct wmt_port *sport = (struct wmt_port *)port;
	struct wmt_uart *uart = (struct wmt_uart *)PORT_TO_BASE(sport);
	unsigned long flags;
	//added begin by rubbitxiao
	spin_lock_irqsave(&sport->port.lock, flags);
	/* Disable TX,RX*/
	uart->urlcr = 0;
	/* Disable all interrupt*/
	uart->urier = 0;
	/*Reset TX,RX Fifo*/
	uart->urfcr = URFCR_TXFRST | URFCR_RXFRST;
	while (uart->urfcr)
		;
	/* Disable Fifo*/
	uart->urfcr &= ~(URFCR_FIFOEN);
	uart->urlcr |=  (URLCR_DLEN & ~URLCR_STBLEN & ~URLCR_PTYEN);
	spin_unlock_irqrestore(&sport->port.lock, flags);
	//added end by rubbitxiao
	/*
	 * Stop our timer.
	 */
	del_timer_sync(&sport->timer);

	/*
	 * Free the allocated interrupt
	 */
	free_irq(sport->port.irq, sport);

	/*
	 * Disable all interrupts, port and break condition.
	 */
	//spin_lock_irqsave(&sport->port.lock, flags);
	//uart->urier &= ~(URIER_ETXFE | URIER_ETXFAE | URIER_ERXFF | URIER_ERXFAF);
	//spin_unlock_irqrestore(&sport->port.lock, flags);

#ifdef CONFIG_SERIAL_WMT_DMA
	if ((unsigned int)(sport->port.membase) != UART0_BASE_ADDR) {
		del_timer_sync(&sport->rx_timer);
		DMA_RX_STOP(sport);
		DMA_RX_CLEAR(sport);
		DMA_RX_FREE(sport);
		while (sport->uart_tx_dma_flag != DMA_TX_END)
			msleep(1);

		DMA_TX_STOP(sport);
		DMA_TX_CLEAR(sport);
		DMA_TX_FREE(sport);
	}
#endif
	if ((unsigned int)(sport->port.membase) == UART1_BASE_ADDR)
		wmt_uart1_post_deinit();
}

/* wmt_uart_pm()
 *
 * Switch on/off uart in powersave mode.
 *
 * Hint: Identify port by irq number.
 */
static void wmt_uart_pm(struct uart_port *port, u_int state, u_int oldstate)
{
	return;
}

static void
wmt_set_termios(struct uart_port *port, struct ktermios *termios, struct ktermios *old)
{
	struct wmt_port *sport = (struct wmt_port *)port;
	struct wmt_uart *uart = (struct wmt_uart *)PORT_TO_BASE(sport);
	unsigned long flags;
	unsigned int new_urlcr, old_urlcr, old_urier, tmp_urisr, baud;
	unsigned int old_csize = old ? old->c_cflag & CSIZE : CS8;
	int i;

	/*
	 * If we don't support modem control lines, don't allow
	 * these to be set.
	 */
	if (0) {
		termios->c_cflag &= ~(HUPCL | CRTSCTS | CMSPAR);
		termios->c_cflag |= CLOCAL;
	}
	/*
	 * Only support CS7 and CS8.
	 */
	while ((termios->c_cflag & CSIZE) != CS7 && (termios->c_cflag & CSIZE) != CS8) {
		termios->c_cflag &= ~CSIZE;
		termios->c_cflag |= old_csize;
		old_csize = CS8;
	}

	if ((termios->c_cflag & CSIZE) == CS8)
		new_urlcr = URLCR_DLEN;
	else
		new_urlcr = 0;

	if (termios->c_cflag & CRTSCTS)
		new_urlcr &= ~URLCR_RCTSSW;
	else
		new_urlcr |= URLCR_RCTSSW;

	if (termios->c_cflag & CSTOPB)
		new_urlcr |= URLCR_STBLEN;

	if (termios->c_cflag & PARENB) {
		/*
		 * Enable parity.
		 */
		new_urlcr |= URLCR_PTYEN;

		/*
		 * Parity mode select.
		 */
		if (termios->c_cflag & PARODD)
			new_urlcr |= URLCR_PTYMODE;
	}

	/*
	 * Ask the core to get baud rate, but we need to
	 * calculate quot by ourself.
	 */
	baud = uart_get_baud_rate(port, termios, old, 9600, 921600);

	/*
	 * We need to calculate quot by ourself.
	 *
	 * FIXME: Be careful, following result is not an
	 *        interger quotient, fix it if need.
	 */
	/*quot = port->uartclk / (13 * baud);*/

	spin_lock_irqsave(&sport->port.lock, flags);

	/*
	 * Mask out other interesting to listen expect TX FIFO almost empty event.
	 */
	sport->port.read_status_mask &= URISR_TO_SM(URISR_TXFAE | URISR_TXFE);

	/*
	 * We're also interested in receiving RX FIFO events.
	 */
	sport->port.read_status_mask |= URISR_TO_SM(URISR_RXDOVR | URISR_RXFAF | URISR_RXFF);

	/*
	 * Check if we need to enable frame and parity error events
	 * to be passed to the TTY layer.
	 */
	if (termios->c_iflag & INPCK)
		sport->port.read_status_mask |= URISR_TO_SM(URISR_FER | URISR_PER);

#ifdef CONFIG_SERIAL_WMT_BKSIG
	/*
	 * check if we need to enable break events to be passed to the TTY layer.
	 */
	if (termios->c_iflag & (BRKINT | PARMRK))
		/*
		 * WMT UART doesn't support break signal detection interrupt.
		 *
		 * I try to implement this using URISR_FER.
		 */
		sport->port.read_status_mask |= SW_BKSIG;
#endif
	/*
	 * Characters to ignore
	 */
		sport->port.ignore_status_mask = 0;

	if (termios->c_iflag & IGNPAR)
		sport->port.ignore_status_mask |= URISR_TO_SM(URISR_FER | URISR_PER);

	if (termios->c_iflag & IGNBRK) {
#ifdef CONFIG_SERIAL_WMT_BKSIG
		/*
		 * WMT UART doesn't support break signal detection interrupt.
		 *
		 * I try to implement this using URISR_FER.
		 */
		sport->port.ignore_status_mask |= BIT31;/*FIXME*/
#endif

		/*
		 * If we're ignoring parity and break indicators,
		 * ignore overruns too (for real raw support).
		 */
		if (termios->c_iflag & IGNPAR)
			sport->port.ignore_status_mask |= URISR_TO_SM(URISR_RXDOVR);
	}

	del_timer_sync(&sport->timer);

	/*
	 * Update the per-port timeout.
	 */
	 uart_update_timeout(port, termios->c_cflag, baud);

	/*
	 * Disable FIFO request interrupts and drain transmitter
	 */
	old_urlcr = uart->urlcr;
	old_urier = uart->urier;
	uart->urier = old_urier & ~(URIER_ETXFAE | URIER_ERXFAF);

	/*
	 * Two step polling, first step polling the remaining
	 * entries in TX FIFO. This step make it safe to drain
	 * out all of remaining data in FIFO.
	 */
	while (URFIDX_TXFIDX(uart->urfidx))
		barrier();

	/*
	 * Second step to make sure the last one data has been sent.
	 */
	while (uart->urusr & URUSR_TXDBSY)
		barrier();

	/*
	 * Disable this UART port.
	 */
	uart->urier = 0;

	/*
	 * Set the parity, stop bits and data size
	 */
	uart->urlcr = new_urlcr;

	/*
	 * Set baud rate
	 */

	for (i = 0; i < BAUD_TABLE_SIZE; i++) {
		if (baud_table[i].baud == baud)
			break;
	}
	uart->urdiv = baud_table[i].brd;

	/*
	 * Read to clean any pending pulse interrupts.
	 */
	tmp_urisr = uart->urisr;

	/*
	 * Restore FIFO interrupt, TXEN bit, RXEN bit settings.
	 */
	uart->urier = old_urier;
#ifdef CONFIG_SERIAL_WMT_DMA
	uart->urlcr |= old_urlcr & (URLCR_TXEN | URLCR_RXEN | URLCR_DMAEN);
#else
	uart->urlcr |= old_urlcr & (URLCR_TXEN | URLCR_RXEN);
#endif

	if (UART_ENABLE_MS(&sport->port, termios->c_cflag))
		wmt_enable_ms(&sport->port);

	spin_unlock_irqrestore(&sport->port.lock, flags);
}

static const char *wmt_type(struct uart_port *port)
{
	struct wmt_port *sport = (struct wmt_port *)port;

	return (sport->port.type == PORT_WMT) ? "wmt serial" : NULL;
}

/*
 * Release the memory region(s) being used by 'port'.
 */
static void wmt_release_port(struct uart_port *port)
{
	struct wmt_port *sport = (struct wmt_port *)port;

	release_mem_region(sport->port.mapbase, UART_PORT_SIZE);
}

/*
 * Request the memory region(s) being used by 'port'.
 */
static int wmt_request_port(struct uart_port *port)
{
	struct wmt_port *sport = (struct wmt_port *)port;

	return request_mem_region(sport->port.mapbase,
							  UART_PORT_SIZE,
							  "uart") != NULL ? 0 : -EBUSY;
}

/*
 * Configure/autoconfigure the port.
 */
static void wmt_config_port(struct uart_port *port, int flags)
{
	struct wmt_port *sport = (struct wmt_port *)port;

	if (flags & UART_CONFIG_TYPE && wmt_request_port(&sport->port) == 0)
		sport->port.type = PORT_WMT;
}

/*
 * Verify the new serial_struct (for TIOCSSERIAL).
 * The only change we allow are to the flags and type, and
 * even then only between PORT_WMT and PORT_UNKNOWN
 */
static int wmt_verify_port(struct uart_port *port, struct serial_struct *ser)
{
	struct wmt_port *sport = (struct wmt_port *)port;
	int ret = 0;

	if (ser->type != PORT_UNKNOWN && ser->type != PORT_WMT)
		ret = -EINVAL;
	if (sport->port.irq != ser->irq)
		ret = -EINVAL;
	if (ser->io_type != SERIAL_IO_MEM)
		ret = -EINVAL;
	if (sport->port.uartclk / 16 != ser->baud_base)
		ret = -EINVAL;

	if ((void *)sport->port.mapbase != ser->iomem_base)
		ret = -EINVAL;
	if (sport->port.iobase != ser->port)
		ret = -EINVAL;
	if (ser->hub6 != 0)
		ret = -EINVAL;
	return ret;
}

static struct uart_ops wmt_pops = {
	.tx_empty	= wmt_tx_empty,
	.set_mctrl	= wmt_set_mctrl,
	.get_mctrl	= wmt_get_mctrl,
	.stop_tx	= wmt_stop_tx,
	.start_tx	= wmt_start_tx,
	.stop_rx	= wmt_stop_rx,
	.enable_ms	= wmt_enable_ms,
	.break_ctl	= wmt_break_ctl,
	.startup	= wmt_startup,
	.shutdown	= wmt_shutdown,
	.pm		= wmt_uart_pm,
	.set_termios	= wmt_set_termios,
	.type		= wmt_type,
	.release_port	= wmt_release_port,
	.request_port	= wmt_request_port,
	.config_port	= wmt_config_port,
	.verify_port	= wmt_verify_port,
};

static int parse_spi1_param(void)
{
	char buf[64];
	size_t l = sizeof(buf);
	int uart_spi_sel = 0;

	if (wmt_getsyspara("wmt.spi1.param", buf, &l) == 0) {
		sscanf(buf, "%d", &uart_spi_sel);
	}
	return uart_spi_sel;
}

/* Setup the WMT serial ports.  Note that we don't include the IrDA
 * port here since we have our own SIR/FIR driver (see drivers/net/irda)
 *
 * Note also that we support "console=ttyVTx" where "x" is either 0 to 2.
 * Which serial port this ends up being depends on the machine you're
 * running this kernel on.
 */
static void wmt_init_ports(void)
{
	static int first = 1;
	int i;

	if (!first)
		return;

	first = 0;

	wmt_uart_spi_sel = parse_spi1_param();

	for (i = 0; i < NR_PORTS; i++) {
		wmt_ports[i].port.uartclk    = 24000000;
		wmt_ports[i].port.ops        = &wmt_pops;
		wmt_ports[i].port.fifosize   = 16;
		wmt_ports[i].port.line       = i;
		wmt_ports[i].port.iotype     = SERIAL_IO_MEM;
		init_timer(&wmt_ports[i].timer);
		wmt_ports[i].timer.function  = wmt_timeout;
		wmt_ports[i].timer.data      = (unsigned long)&wmt_ports[i];
	}

	/*
	 * Make sure all UARTs are not configured as GPIO function.
	 *
	 * This step may be redundant due to bootloader has already
	 * done this for us.
	 */


	/* Switch the Uart's pin from default GPIO into uart function pins for UART0 ~ UART3 */
	GPIO_CTRL_GP18_UART_BYTE_VAL &= ~(BIT0 | BIT1 | BIT2 | BIT3 |
					  BIT4 | BIT5 | BIT6 | BIT7); /*UART0 UART1*/
#ifdef CONFIG_UART_2_3_ENABLE
	GPIO_CTRL_GP18_UART_BYTE_VAL &= ~(BIT2 | BIT3 | BIT6 | BIT7); /*UART2 UART3*/
#endif

	/*Set Uart0 and Uart1, Uart2 and Uart3  pin share*/
	PIN_SHARING_SEL_4BYTE_VAL  &= ~(BIT9 | BIT8);
	if (wmt_uart_spi_sel == SHARE_PIN_UART)
		PIN_SHARING_SEL_4BYTE_VAL  &= ~(BIT10);

#ifdef CONFIG_UART_2_3_ENABLE
	//kevin modify    uart0,uart1(hw flow control),uart2
    PIN_SHARING_SEL_4BYTE_VAL |= (BIT8);
#endif

	auto_pll_divisor(DEV_UART0, CLK_ENABLE, 0, 0);
	auto_pll_divisor(DEV_UART1, CLK_ENABLE, 0, 0);
#ifdef CONFIG_UART_2_3_ENABLE
	auto_pll_divisor(DEV_UART2, CLK_ENABLE, 0, 0);
	auto_pll_divisor(DEV_UART3, CLK_ENABLE, 0, 0);
#endif
}

void __init wmt_register_uart_fns(struct wmt_port_fns *fns)
{
	if (fns->get_mctrl)
		wmt_pops.get_mctrl = fns->get_mctrl;
	if (fns->set_mctrl)
		wmt_pops.set_mctrl = fns->set_mctrl;

	wmt_pops.pm = fns->pm;
	wmt_pops.set_wake = fns->set_wake;
}

void __init wmt_register_uart(int idx, int port)
{
	if (idx >= NR_PORTS) {
		printk(KERN_ERR "%s: bad index number %d\n", __func__, idx);
		return;
	}

	switch (port) {
	case 0:
		wmt_ports[idx].port.membase = (void *)(REG32_PTR(UART0_BASE_ADDR));
		wmt_ports[idx].port.mapbase = UART0_BASE_ADDR;
		wmt_ports[idx].port.irq     = IRQ_UART0;
		wmt_ports[idx].port.flags   = ASYNC_BOOT_AUTOCONF;
		break;
	case 1:
		wmt_ports[idx].port.membase = (void *)(REG32_PTR(UART1_BASE_ADDR));
		wmt_ports[idx].port.mapbase = UART1_BASE_ADDR;
		wmt_ports[idx].port.irq     = IRQ_UART1;
		wmt_ports[idx].port.flags   = ASYNC_BOOT_AUTOCONF;
		break;
#ifdef CONFIG_UART_2_3_ENABLE
	case 2:
		wmt_ports[idx].port.membase = (void *)(REG32_PTR(UART2_BASE_ADDR));
		wmt_ports[idx].port.mapbase = UART2_BASE_ADDR;
		wmt_ports[idx].port.irq     = IRQ_UART2;
		wmt_ports[idx].port.flags   = ASYNC_BOOT_AUTOCONF;
		break;
	case 3:
		wmt_ports[idx].port.membase = (void *)(REG32_PTR(UART3_BASE_ADDR));
		wmt_ports[idx].port.mapbase = UART3_BASE_ADDR;
		wmt_ports[idx].port.irq     = IRQ_UART3;
		wmt_ports[idx].port.flags   = ASYNC_BOOT_AUTOCONF;
		break;
#endif

	default:
		printk(KERN_ERR "%s: bad port number %d\n", __func__, port);
	}
}

#ifdef CONFIG_SERIAL_WMT_CONSOLE

/*
 * Interrupts are disabled on entering
 *
 * Note: We do console writing with UART register mode.
 */

static void wmt_console_write(struct console *co, const char *s, u_int count)
{
	struct wmt_port *sport = &wmt_ports[co->index];
	struct wmt_uart *uart = (struct wmt_uart *)PORT_TO_BASE(sport);
	unsigned int i, old_urlcr, old_urier;
	/*{JHT*/
	unsigned int old_urfcr;
	unsigned long flags;

	spin_lock_irqsave(&sport->port.lock, flags);
	/*}JHT*/
	/*
	 * First, save URLCR and URIER.
	 */
	old_urlcr = uart->urlcr;
	old_urier = uart->urier;
	/*{JHT*/
	old_urfcr = uart->urfcr;
	/*}JHT*/

	/*
	 * Second, switch to register mode with follows method:
	 *
	 * Disable FIFO threshold interrupts, and enable transmitter.
	 */
	uart->urier &= ~(URIER_ETXFAE | URIER_ERXFAF);
	uart->urlcr |= URLCR_TXEN;
	/*{JHT*/
	uart->urfcr &= ~URFCR_FIFOEN;
	/*}JHT*/
	/*
	 * Now, do each character
	 */
	for (i = 0; i < count; i++) {
		/*
		 * Polling until free for transmitting.
		 */
		while (uart->urusr & URUSR_TXDBSY)
			;

		uart->urtdr = (unsigned int)s[i];

		/*
		 * Do CR if there comes a LF.
		 */
		if (s[i] == '\n') {
			/*
			 * Polling until free for transmitting.
			 */
			while (uart->urusr & URUSR_TXDBSY)
				;

			uart->urtdr = (unsigned int)'\r';
		}
	}

	/*
	 * Finally, wait for transmitting done and restore URLCR and URIER.
	 */
	while (uart->urusr & URUSR_TXDBSY)
		;

	uart->urlcr = old_urlcr;
	uart->urier = old_urier;
	/*{JHT*/
	uart->urfcr = old_urfcr;
	/*}JHT*/
	spin_unlock_irqrestore(&sport->port.lock, flags);
}

/*
 * If the port was already initialised (eg, by a boot loader), try to determine
 * the current setup.
 */
static void __init wmt_console_get_options(struct wmt_port *sport, int *baud, int *parity, int *bits)
{
	int i;
	struct wmt_uart *uart = (struct wmt_uart *)PORT_TO_BASE(sport);

	if ((uart->urlcr & (URLCR_RXEN | URLCR_TXEN)) == (URLCR_RXEN | URLCR_TXEN)) {
		/*
		 * Port was enabled.
		 */
		unsigned quot;

		*parity = 'n';
		/*
		 * Check parity mode, 0:evev 1:odd
		 */
		if (uart->urlcr & URLCR_PTYEN) {
			if (uart->urlcr & URLCR_PTYMODE)
				*parity = 'o';
			else
				*parity = 'e';
		}

		/*
		 * Check data length, 0:7-bit 1:8-bit
		 */
		if (uart->urlcr & URLCR_DLEN)
			*bits = 8;
		else
			*bits = 7;

		/*
		 * Get baud rate divisor.
		 */
		quot = (uart->urdiv & URBRD_BRDMASK);
		/*
		 * FIXME: I didn't trace the console driver want me
		 * report baud rate whether actual baud rate or ideal
		 * target baud rate, current I report baud as actual
		 * one, if it need value as target baud rate, just
		 * creat an array to fix it, Dec.23 by Harry.
		 */

		for (i = 0; i < BAUD_TABLE_SIZE; i++) {
			if ((baud_table[i].brd & URBRD_BRDMASK) == quot) {
				*baud = baud_table[i].baud;
				break;
			}
		}

		/*
		 * If this condition is true, something might be wrong.
		 * I reprot the actual baud rate temporary.
		 * Check the printk information then fix it.
		 */
		if (i >= BAUD_TABLE_SIZE)
			*baud = sport->port.uartclk / (13 * (quot + 1));
	}
}

#ifndef CONFIG_WMT_DEFAULT_BAUDRATE
#define CONFIG_WMT_DEFAULT_BAUDRATE  115200
#endif

static int __init
wmt_console_setup(struct console *co, char *options)
{
	struct wmt_port *sport;
	int bits = 8;
	int parity = 'n';
	int flow = 'n';
	int baud = CONFIG_WMT_DEFAULT_BAUDRATE;

	/*
	 * Check whether an invalid uart number has been specified, and
	 * if so, search for the first available port that does have
	 * console support.
	*/
	if (co->index == -1 || co->index >= NR_PORTS)
		co->index = 0;

	sport = &wmt_ports[co->index];

	if (options)
		uart_parse_options(options, &baud, &parity, &bits, &flow);
	else
		wmt_console_get_options(sport, &baud, &parity, &bits);

	return uart_set_options(&sport->port, co, baud, parity, bits, flow);
}

static struct uart_driver wmt_reg;

static struct console wmt_console = {

#ifdef CONFIG_SERIAL_WMT_TTYVT
	.name	= "ttyVT",
#else
	.name	= "ttyS",
#endif
	.write	= wmt_console_write,
	.device	= uart_console_device,
	.setup	= wmt_console_setup,
	.flags	= CON_PRINTBUFFER,
	.index	= -1,
	.data	= &wmt_reg,
};

static int __init wmt_rs_console_init(void)
{
	wmt_init_ports();
	register_console(&wmt_console);
	return 0;
}

console_initcall(wmt_rs_console_init);

#define WMT_CONSOLE  (&wmt_console)

#else   /* CONFIG_SERIAL_WMT_CONSOLE */

#define WMT_CONSOLE  NULL

#endif

static struct uart_driver wmt_reg = {
	.owner          = THIS_MODULE,

#ifdef CONFIG_SERIAL_WMT_TTYVT
	.driver_name    = "ttyVT",
	.dev_name       = "ttyVT",
#else
	.driver_name    = "ttyS",
	.dev_name       = "ttyS",
#endif
	.major          = SERIAL_WMT_MAJOR,
	.minor          = MINOR_START,
	.nr             = NR_PORTS,
	.cons           = WMT_CONSOLE,
};


void wmt_serial_set_reg(void)
{
	*(volatile unsigned int *) (UART0_BASE_ADDR + 0x00000008) = BRD_115200BPS;
	*(volatile unsigned int *) (UART0_BASE_ADDR + 0x0000000c) = URLCR_TXEN |
								    URLCR_RXEN |
								    URLCR_DLEN |
								    URLCR_RCTSSW;

	*(volatile unsigned int *) (UART0_BASE_ADDR + 0x00000014) = URIER_ERXFAF  |
								    URIER_ERXFF   |
								    URIER_ERXTOUT |
								    URIER_EPER    |
								    URIER_EFER    |
								    URIER_ERXDOVR;

	*(volatile unsigned int *) (UART0_BASE_ADDR + 0x00000020) = URFCR_FIFOEN   |
								    URFCR_TXFLV(8) |
								    URFCR_RXFLV(8);
}

static int wmt_serial_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct device *dev = &pdev->dev;
	struct wmt_port *sport = dev_get_drvdata(dev);
	struct wmt_uart *uart = (struct wmt_uart *)PORT_TO_BASE(sport);
	unsigned long flags;

	if (sport) {
		if (sport->port.irq != IRQ_UART0)
			uart_suspend_port(&wmt_reg, &sport->port);
	}
	if (!sport)
		return 0;

	spin_lock_irqsave(&sport->port.lock, flags);

	/* save host register */
	sport->old_urdiv = uart->urdiv;
	sport->old_urlcr = uart->urlcr;
	sport->old_urier = uart->urier;
	sport->old_urfcr = uart->urfcr;
	sport->old_urtod = uart->urtod;
	
	uart->urier = 0;
	spin_unlock_irqrestore(&sport->port.lock, flags);

	switch (sport->port.irq) {
	case IRQ_UART0:
		/*auto_pll_divisor(DEV_UART0, CLK_DISABLE, 0, 0);*/
		break;
	case IRQ_UART1:
		auto_pll_divisor(DEV_UART1, CLK_DISABLE, 0, 0);
		break;
#ifdef CONFIG_UART_2_3_ENABLE
	case IRQ_UART2:
		auto_pll_divisor(DEV_UART2, CLK_DISABLE, 0, 0);
		break;

	case IRQ_UART3:
		auto_pll_divisor(DEV_UART3, CLK_DISABLE, 0, 0);
		break;
#endif
	default:
		break;

	}
	return 0;
}

static int wmt_serial_resume(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct wmt_port *sport = dev_get_drvdata(dev);
	struct wmt_uart *uart = (struct wmt_uart *)PORT_TO_BASE(sport);
	unsigned long flags;

	if (!sport)
		return 0;

	GPIO_CTRL_GP18_UART_BYTE_VAL &= ~(BIT0 | BIT1 | BIT2 | BIT3 |
					  BIT4 | BIT5 | BIT6 | BIT7); /*UART0 UART1*/
#ifdef CONFIG_UART_2_3_ENABLE
	GPIO_CTRL_GP18_UART_BYTE_VAL &= ~(BIT2 | BIT3 | BIT6 | BIT7); /*UART2 UART3*/
#endif

	/*Set Uart0 and Uart1, Uart2 and Uart3  pin share*/
	PIN_SHARING_SEL_4BYTE_VAL  &= ~(BIT9 | BIT8);
	if (wmt_uart_spi_sel == SHARE_PIN_UART)
		PIN_SHARING_SEL_4BYTE_VAL  &= ~(BIT10);

#ifdef CONFIG_UART_2_3_ENABLE
	//kevin modify,    uart0,uart1(hw flow control),uart2
    PIN_SHARING_SEL_4BYTE_VAL |= (BIT8);
#endif

	switch (sport->port.irq) {
	case IRQ_UART0:
		auto_pll_divisor(DEV_UART0, CLK_ENABLE, 0, 0);
		break;
	case IRQ_UART1:
		auto_pll_divisor(DEV_UART1, CLK_ENABLE, 0, 0);
		break;
#ifdef CONFIG_UART_2_3_ENABLE
	case IRQ_UART2:
		auto_pll_divisor(DEV_UART2, CLK_ENABLE, 0, 0);
		break;
	case IRQ_UART3:
		auto_pll_divisor(DEV_UART3, CLK_ENABLE, 0, 0);
		break;
#endif
	default:
		break;
	}
	
	if (sport->port.irq != IRQ_UART0) {
		/* Disable TX,RX */
		uart->urlcr = 0;
		/* Disable all interrupt */
		uart->urier = 0;
		/* Clear all interrupt */
		uart->urisr = 0xffffffff;
	
		/* Disable Fifo */
		uart->urfcr &= ~(URFCR_FIFOEN);

		/* Reset TX,RX Fifo */
		uart->urfcr = URFCR_TXFRST | URFCR_RXFRST;

		while (uart->urfcr)
			;
	}
	
	spin_lock_irqsave(&sport->port.lock, flags);
	/*store back the interrupt enable status*/
	uart->urdiv = sport->old_urdiv;
	uart->urfcr = sport->old_urfcr;
	uart->urtod = sport->old_urtod;
	uart->urier = sport->old_urier;
	uart->urlcr = sport->old_urlcr;
	spin_unlock_irqrestore(&sport->port.lock, flags);

	if (sport) {
		if (sport->port.irq != IRQ_UART0)
			uart_resume_port(&wmt_reg, &sport->port);

	}

	return 0;
}
#if 1
void print_dma_position(struct wmt_port *sport)
{
	unsigned int *rx_dma = (unsigned int *)(0xfe001904 + (0x20*sport->rx_dmach));
	printk("address 0x%p = 0x%x(value)\n",rx_dma,*rx_dma);
}

void print_dma_buf_pointer(struct wmt_port *sport)
{
	printk("buf0:0x%p;  buf1:0x%p;   buf2:0x%p\n",sport->uart_dma_tmp_buf0,sport->uart_dma_tmp_buf1,sport->uart_dma_tmp_buf2);
	printk("phy0:0x%x; phy1:0x%x;  phy2:0x%x\n",sport->uart_dma_tmp_phy0,sport->uart_dma_tmp_phy1,sport->uart_dma_tmp_phy2);
}
void dump_uart_info(void)
{
     unsigned long flags;
     struct wmt_port * p_wmt_port = &wmt_ports[1];
     uart_dump_reg(p_wmt_port);
     printk("sport1->port.icount.rx0:0x%x,uart_tx_dma_phy0_org:0x%x\n",p_wmt_port->port.icount.rx,p_wmt_port->uart_tx_dma_phy0_org);
     print_dma_position(p_wmt_port);

     //spin_lock_irqsave(&(p_wmt_port->port.lock), flags);
     wmt_rx_chars(p_wmt_port,URISR_RXFAF | URISR_RXFF);
     //spin_unlock_irqrestore(&(p_wmt_port->port.lock), flags);

     print_dma_position(p_wmt_port);
     printk("sport1->port.icount.rx1:0x%x,uart_tx_dma_phy0_org:0x%x\n",p_wmt_port->port.icount.rx,p_wmt_port->uart_tx_dma_phy0_org);
     printk("uart rxxx dma channel register:\n");
     wmt_dump_dma_regs(p_wmt_port->rx_dmach);
     printk("uart tttx dma channel register:\n");
     wmt_dump_dma_regs(p_wmt_port->tx_dmach);
     printk("buf pointer position\n");
     print_dma_buf_pointer(p_wmt_port);
     printk("#######################################\n");
     dump_rx_dma_buf(p_wmt_port);
#ifdef UART_DEBUG	 
     printk("##########################################\n");
     print_dma_count_cpu_buf_pos(p_wmt_port);
#endif
    printk("#############debug dma tx failed begin#############\n");
    printk("sport->port.icount.tx:0x%x,sport->uart_tx_count:0x%x\n",p_wmt_port->port.icount.tx,
	p_wmt_port->uart_tx_count);
    printk("dma_tx_cnt:0x%x\n",p_wmt_port->dma_tx_cnt);
    printk("uart_tx_dma_flag:0x%x\n",p_wmt_port->uart_tx_dma_flag);
	printk("uart_tx_stopped:%d\n",uart_tx_stopped(&p_wmt_port->port));
	if(uart_tx_stopped(&p_wmt_port->port))
	{
		printk("stopped:%d\n",(&p_wmt_port->port)->state->port.tty->stopped);
		printk("hw_stopped:%d\n",(&p_wmt_port->port)->state->port.tty->hw_stopped);
	}
    printk("#############debug dma tx failed end#############\n");	

}
EXPORT_SYMBOL(dump_uart_info);
#endif
static int wmt_serial_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct resource *res = pdev->resource;
	int i;

	for (i = 0; i < pdev->num_resources; i++, res++)
		if (res->flags & IORESOURCE_MEM)
			break;

	if (i < pdev->num_resources) {
		for (i = 0; i < NR_PORTS; i++) {
			if (wmt_ports[i].port.mapbase != res->start)
					continue;

			wmt_ports[i].port.dev = dev;
			uart_add_one_port(&wmt_reg, &wmt_ports[i].port);
			dev_set_drvdata(dev, &wmt_ports[i]);
			if (i >= 1) {
				wmt_ports[i].uart_rx_dma_buf0_org =
					dma_alloc_coherent(NULL,
							   UART_BUFFER_SIZE,
							   &wmt_ports[i].uart_rx_dma_phy0_org,
							   GFP_KERNEL);
				wmt_ports[i].uart_rx_dma_buf1_org =
					dma_alloc_coherent(NULL,
							   UART_BUFFER_SIZE,
							   &wmt_ports[i].uart_rx_dma_phy1_org,
							   GFP_KERNEL);
				wmt_ports[i].uart_rx_dma_buf2_org =
					dma_alloc_coherent(NULL,
							   UART_BUFFER_SIZE,
							   &wmt_ports[i].uart_rx_dma_phy2_org,
							   GFP_KERNEL);
				wmt_ports[i].uart_tx_dma_buf0_org =
					dma_alloc_coherent(NULL,
							   UART_BUFFER_SIZE,
							   &wmt_ports[i].uart_tx_dma_phy0_org,
							   GFP_KERNEL);
			}
			break;
		}
	}
	return 0;
}

static int wmt_serial_remove(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct wmt_port *sport = dev_get_drvdata(dev);

	dev_set_drvdata(dev, NULL);

	if (sport)
		uart_remove_one_port(&wmt_reg, &sport->port);

	return 0;
}

static struct platform_driver wmt_serial_driver = {
	.driver.name    = "uart",
	.probe          = wmt_serial_probe,
	.remove         = wmt_serial_remove,
	.suspend        = wmt_serial_suspend,
	.resume         = wmt_serial_resume,
};

static int __init wmt_serial_init(void)
{
	int ret;

	wmt_init_ports();
	ret = uart_register_driver(&wmt_reg);

	if (ret == 0) {
		ret = platform_driver_register(&wmt_serial_driver);
		if (ret)
			uart_unregister_driver(&wmt_reg);
	}
#ifndef CONFIG_SKIP_DRIVER_MSG
	printk(KERN_INFO "WMT Serial driver initialized: %s\n",
			(ret == 0) ? "ok" : "failed");
#endif
	return ret;
}

static void __exit wmt_serial_exit(void)
{
	platform_driver_unregister(&wmt_serial_driver);
	uart_unregister_driver(&wmt_reg);
}

module_init(wmt_serial_init);
module_exit(wmt_serial_exit);

MODULE_AUTHOR("WonderMedia Technologies, Inc.");
MODULE_DESCRIPTION("WMT [generic serial port] driver");
MODULE_LICENSE("GPL");
