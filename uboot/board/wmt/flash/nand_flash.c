/*
 * (C) Copyright 2000
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * Copyright (c) 2008 WonderMedia Technologies, Inc.
 *
 * This program is free software: you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software Foundation,
 * either version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE.  See the GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * WonderMedia Technologies, Inc.
 * 10F, 529, Chung-Cheng Road, Hsin-Tien, Taipei 231, R.O.C.
 */

#include <common.h>
#include <mpc8xx.h>
#include "nand_flash.h"

extern flash_info_t	flash_info_nand[CFG_MAX_NAND_FLASH_BANKS]; /* info for FLASH chips	*/

/*-----------------------------------------------------------------------
 * Functions
 */
#ifdef CFG_FLASH_PROTECTION
static void flash_sync_real_protect(flash_info_t *info);
#endif

/*-----------------------------------------------------------------------
 * nand_flash_init()
 *
 * sets up flash_info and returns size of FLASH (bytes)
 */
unsigned long nand_flash_init(void)
{
	unsigned long size_b = 0;
	return size_b;
}

/*-----------------------------------------------------------------------
 */
void nand_flash_print_info(flash_info_t *info)
{
}

/*
 * The following code cannot be run from FLASH!
 */

#ifdef CFG_FLASH_PROTECTION
/*-----------------------------------------------------------------------
 */
static void flash_sync_real_protect(flash_info_t *info)
{
}
#endif

/*-----------------------------------------------------------------------
 */

int	nand_flash_erase(flash_info_t *info, int s_first, int s_last)
{
	int rcode = 0;
	return rcode;
}

/*-----------------------------------------------------------------------
 * Copy memory to flash, returns:
 * 0 - OK
 * 1 - write timeout
 * 2 - Flash not erased
 */
int nand_write_buff(flash_info_t *info, uchar *src, ulong addr, ulong cnt)
{
	int res = 0;
	return res;
}

#ifdef CFG_FLASH_PROTECTION
/*-----------------------------------------------------------------------
 */
int nand_flash_real_protect(flash_info_t *info, long sector, int prot)
{
	int rcode = 0;		/* assume success */
	return rcode;
}
#endif
