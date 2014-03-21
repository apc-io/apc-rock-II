/*++ 
Copyright (c) 2012 WonderMedia Technologies, Inc.

This program is free software: you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free Software 
Foundation, either version 2 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details. You
should have received a copy of the GNU General Public License along with this
program. If not, see http://www.gnu.org/licenses/>.

WonderMedia Technologies, Inc.
4F, 531, Chung-Cheng Road, Hsin-Tien, Taipei 231, R.O.C.
--*/

#include <common.h>

#ifndef NULL
#define NULL 0
#endif

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

static int wmt_write_ostc_rdy(void)
{
	unsigned int sw_counter = 30000;
	
	while( pWMTOST->ostas & 0x10 ) {
		if ( --sw_counter == 0 ) {
			printf("Count Write Active Busy\n");
			return -1;
		}
	}
	return 0;
}

int wmt_init_ostimer(void)
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

int wmt_read_oscr(unsigned int *val)
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
			printf("Read Count Request Fail\n");
			break;
		}
	}
	*val = (int)pWMTOST->ostct;
	return 0;
}

int wmt_idle_us(int us)
{
	int count = 100;
	unsigned int before,after;

	us = us*3;	

	if (wmt_read_oscr(&before))
		return -1;
	while (1) {
		while(--count);
		if (wmt_read_oscr(&after))
			return -1;
		if ((int)(after - before) >= us) {
			//printf("request = %d , result = %d\n",us,(after - before));
			break;
		}
		count = 100;
	}
	return 0;
}

