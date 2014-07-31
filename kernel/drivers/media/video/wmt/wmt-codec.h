/*++
 * Common interface for WonderMedia SoC hardware encoder and decoder drivers
 *
 * Copyright (c) 2008-2013  WonderMedia  Technologies, Inc.
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
#ifndef CODEC_H
#define CODEC_H

/*-------------------- MODULE DEPENDENCY ---------------------------------*/

enum {
	CODEC_VD_JPEG,
	CODEC_VD_MSVD,
	CODEC_VE_H264,
	CODEC_VE_JPEG,
	CODEC_MAX
}; /* codec_type */

/*--------------------- EXPORTED PRIVATE FUNCTIONS  ----------------------*/

int wmt_get_codec_clock_count(void);
void wmt_reset_codec_clock_count(void);

int wmt_codec_pmc_ctl(int codec_type, int enable);
int wmt_codec_clock_en(int codec_type, int enable);
int wmt_codec_msvd_reset(int codec_type);

int wmt_codec_lock(int codec_type, int timeout);
int wmt_codec_unlock(int codec_type);

int wmt_codec_write_prd(
	unsigned int src_buf,
	unsigned int src_size,
	unsigned int prd_addr_in,
	unsigned int prd_buf_size);

int wmt_codec_dump_prd(unsigned int prd_virt_in, int dump_data);

int wmt_codec_timer_init(
	void **ppcodec,
	const char *name,
	unsigned int count,
	int threshold_ms);

int wmt_codec_timer_reset(
	void *phandle,
	unsigned int count,
	int threshold_ms);

int wmt_codec_timer_start(void *pcodec);
int wmt_codec_timer_stop(void  *pcodec);
int wmt_codec_timer_exit(void  *pcodec);

/*=== END wmt-codec.h ====================================================*/
#endif /* #ifndef CODEC_H */

