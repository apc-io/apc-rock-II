/*
 * misc.c
 *
 * This is a collection of several routines from gzip-1.0.3
 * adapted for Linux.
 *
 * malloc by Hannu Savolainen 1993 and Matthias Urlichs 1994
 *
 * Modified for ARM Linux by Russell King
 *
 * Nicolas Pitre <nico@visuaide.com>  1999/04/14 :
 *  For this code to run directly from Flash, all constant variables must
 *  be marked with 'const' and all other variables initialized at run-time
 *  only.  This way all non constant variables will end up in the bss segment,
 *  which should point to addresses in RAM and cleared to 0 on start.
 *  This allows for a much quicker boot time.
 */

unsigned int __machine_arch_type;

#define _LINUX_STRING_H_

//#define LOGO_DISPLAY_SPEED_TEST //added by howayhuo to calculate the logo display speed

#ifdef LOGO_DISPLAY_SPEED_TEST
typedef struct
{
	volatile unsigned long	ostm[4];				// [Rx100-Rx10C] OS Timer Match Register0-3
	volatile unsigned long	ostct;					// [Rx110-113] OS Timer Counter Register
	volatile unsigned long	osts;					// [Rx114-117] OS Timer Status Register
	volatile unsigned long	ostwe;					// [Rx118-Rx11B]
	volatile unsigned long	ostie;					// [Rx11C-Rx11F]
	volatile unsigned long	ostctrl;				// [Rx120-Rx123] OS Timer Control Register
	volatile unsigned long	ostas;					// [Rx124-Rx127] OS Timer Access Status Register
} WMT_OST_REG;

static WMT_OST_REG *pWMTOST;
#define NULL 0

static int wmt_write_ostc_rdy(void)
{
	unsigned int sw_counter = 30000;

	while( pWMTOST->ostas & 0x10 ) {
		if ( --sw_counter == 0 ) {
			//printf("Count Write Active Busy\n");
			return -1;
		}
	}
	return 0;
}

static int wmt_init_ostimer(void)
{
	//printf("wmt_init_ostimer\n");

	if (pWMTOST == NULL)
		pWMTOST = (WMT_OST_REG *)0xd8130100;

	if (pWMTOST->ostctrl&0x01)
		return 0;

	pWMTOST->ostctrl = 0;
	pWMTOST->ostwe = 0;

	if (wmt_write_ostc_rdy())
		return -1;

	pWMTOST->ostct = 0;

	pWMTOST->ostctrl = 1;

	return 0;
}

static int wmt_read_ostc(unsigned int *val)
{
	unsigned int sw_counter = 300000;

	if (pWMTOST == NULL) {
		if (wmt_init_ostimer())
			return -1;
	}

	if ( (pWMTOST->ostctrl & 0x02 ) == 0 )
		pWMTOST->ostctrl |= 0x02;

	/* Check OS Timer Count Value is Valid*/
	while ((pWMTOST->ostas & 0x20) == 0x20) {
		if ( --sw_counter == 0 ) { /* Need to be considered*/
			//printf("Read Count Request Fail\n");
			break;
		}
	}
	*val = (int)pWMTOST->ostct;
	return 0;
}
#endif

/* static void putstr(const char *ptr); */
extern void error(char *x);

#include <mach/uncompress.h>

void *memcpy(void *__dest, __const void *__src, unsigned int __n)
{
	int i = 0;
	unsigned char *d = (unsigned char *)__dest, *s = (unsigned char *)__src;

	for (i = __n >> 3; i > 0; i--) {
		*d++ = *s++;
		*d++ = *s++;
		*d++ = *s++;
		*d++ = *s++;
		*d++ = *s++;
		*d++ = *s++;
		*d++ = *s++;
		*d++ = *s++;
	}

	if (__n & 1 << 2) {
		*d++ = *s++;
		*d++ = *s++;
		*d++ = *s++;
		*d++ = *s++;
	}

	if (__n & 1 << 1) {
		*d++ = *s++;
		*d++ = *s++;
	}

	if (__n & 1)
		*d++ = *s++;

	return __dest;
}

/*
 * gzip declarations
 */
extern char input_data[];
extern char input_data_end[];

unsigned char *output_data;

unsigned long free_mem_ptr;
unsigned long free_mem_end_ptr;

#ifndef arch_error
#define arch_error(x)
#endif

void error(char *x)
{
	arch_error(x);

	putstr("\n\n");
	putstr(x);
	putstr("\n\n -- System halted");

	while(1);	/* Halt */
}

void __div0(void)
{
	error("Attempting division by 0!");
}

extern int do_decompress(u8 *input, int len, u8 *output, void (*error)(char *x));

void
decompress_kernel(unsigned long output_start, unsigned long free_mem_ptr_p,
		unsigned long free_mem_ptr_end_p,
		int arch_id)
{
#ifdef LOGO_DISPLAY_SPEED_TEST
	{
		int start_decompress_time;
		volatile unsigned int *p_scratch4;

		p_scratch4 = (volatile unsigned int *)0xd8130040 ;
		wmt_read_ostc(&start_decompress_time);
		*p_scratch4 = start_decompress_time;
	}
#endif

	int ret;

	output_data		= (unsigned char *)output_start;
	free_mem_ptr		= free_mem_ptr_p;
	free_mem_end_ptr	= free_mem_ptr_end_p;
	__machine_arch_type	= arch_id;

	arch_decomp_setup();

	putstr("Uncompressing U-Boot...");
	ret = do_decompress(input_data, input_data_end - input_data,
			    output_data, error);
	if (ret)
		error("decompressor returned an error");
	else
		putstr(" done, booting U-Boot.\n");
}
