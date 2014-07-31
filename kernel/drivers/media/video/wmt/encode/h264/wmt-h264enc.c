/*++ 
 * WonderMedia H.264 hardware encoder driver
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

#ifndef WMT_H264ENC_C
/* To assert that only one occurrence is included */
#define WMT_H264ENC_C

/*----------------- MODULE DEPENDENCY ----------------------------------------*/
//------------------------------------------------
//  Include Header
//------------------------------------------------
#include "../../wmt-codec.h"
#include "wmt-h264enc.h"
#include "header.h"
#include <asm/cacheflush.h>


/*----------------- EXPORTED PRIVATE CONSTANTS -------------------------------*/
#define DEVICE_NAME "WMT-H264ENC"
#define THE_MB_USER	"WMT-H264ENC"

#define LAMBDA_ACCURACY_BITS         5
#define PRD_MAX_SIZE        (60*1024)  /* Max 64k */
#define H264ENC_IRQ_MODE


/*----------------- EXPORTED PRIVATE MACROS ----------------------------------*/
//#define LAMBDA_FACTOR(lambda)        ((int)((double)(1 << LAMBDA_ACCURACY_BITS) * lambda )+0.5)
//------------------------------------------------
//  Declaration Global
//------------------------------------------------
static const int QP2QUANT[40]=
{
   1, 1, 1, 1, 2, 2, 2, 2,
   3, 3, 3, 4, 4, 4, 5, 6,
   6, 7, 8, 9,10,11,13,14,
  16,18,20,23,25,29,32,36,
  40,45,51,57,64,72,81,91
};



h264enc_ctx_t h264enc_drv;
VEU veu;

static int h264enc_dev_ref = 0; /* is device open */
static spinlock_t h264enc_lock;
static spinlock_t h264enc_ste_lock;

DECLARE_WAIT_QUEUE_HEAD(h264enc_wait);


#ifdef THIS_DEBUG_DETAIL
static void dumpdata(unsigned char *buf, unsigned int size)
{
	unsigned int i = size;
	unsigned char *ptr = buf;

	if (!ptr) {
		ERR("input point is NULL!\n");
		return;
	}
	printk("\n[0]  ");
	for (i = 0;i < size; i++) {
		printk("%02x ", *(ptr+i));
		if (((i%16)==15) && (i<size-1))
			printk("\n[%d] ",i+1);
	}

	return;
}
#endif
/*!*************************************************************************
* print_prd_table
* 
*/
/*!
* \brief
*	Set source PRD table
*
* \retval  0 if success
*/ 
static int print_prd_table(h264enc_ctx_t *drv, int dump_data)
{
	unsigned int  prd_addr_in, prd_data_size = 0;
	unsigned int  addr, len;
	int i, j;

	DBG_DETAIL("Dma prd virt addr: 0x%08x\n", drv->prd_virt);
	DBG_DETAIL("Dma prd phy  addr: 0x%08x\n", drv->prd_phys);

	prd_addr_in = drv->prd_virt;
	for (i=0; ; i+=2) {
		addr = *(unsigned int *)(prd_addr_in + i * 4);
		len  = *(unsigned int *)(prd_addr_in + (i + 1) * 4);
		prd_data_size += (len & 0xFFFF);

		DBG_DETAIL("[%02d]Addr: 0x%08x\n", i, addr);
		DBG_DETAIL("    Len:  0x%08x (%d)\n", len, (len & 0xFFFF));

		if (dump_data) {
			unsigned char *ptr;

			ptr = (unsigned char *)addr;
			for (j=0; j<(len & 0xFFFF); j++ ) {
				if ((j%16) == 15) {
					DBG_DETAIL("0x%02x\n", *ptr);
				} else {
					DBG_DETAIL("0x%02x ", *ptr);
				}
				ptr++;
			}
		}
		if (len & 0x80000000)
			break;
	} /* for(i=0; ; i+=2) */
	DBG_DETAIL("prd_data_size: %d\n", prd_data_size);
	DBG_DETAIL("PRD_ADDR: 0x%x \n", REG32_VAL(REG_BASE_H264BSOUT));

	return 0;
} /* End of print_prd_table() */

/*!*************************************************************************
* dbg_dump_registers
* 
*/
/*!
* \brief
*   Dump important register values for debugging
*
* \parameter
*
* \retval  0 if success
*/ 
static void dbg_dump_registers(h264enc_ctx_t *ctx)
{
  
	#define Dump_RegisterValue(addr)   	DPRINT("REG(0x%x): 0x%x\n", addr, REG32_VAL(addr));

#if 0 // debug only
	print_prd_table(ctx, 1);
#else
	print_prd_table(ctx, 0);
#endif

	Dump_RegisterValue(REG_BASE_MSVD+0x000);
	Dump_RegisterValue(REG_BASE_MSVD+0x004);
	Dump_RegisterValue(REG_BASE_MSVD+0x010);

	DPRINT("h264enc Regsiters: \n");
	Dump_RegisterValue(WMT_H264ENC_BASE+0x000);
	Dump_RegisterValue(WMT_H264ENC_BASE+0x010);
	Dump_RegisterValue(WMT_H264ENC_BASE+0x014);
	Dump_RegisterValue(WMT_H264ENC_BASE+0x018);
	Dump_RegisterValue(WMT_H264ENC_BASE+0x01C);  /* debug register */
	Dump_RegisterValue(WMT_H264ENC_BASE+0x020);
	Dump_RegisterValue(WMT_H264ENC_BASE+0x024);
	Dump_RegisterValue(WMT_H264ENC_BASE+0x028);

	Dump_RegisterValue(WMT_H264ENC_BASE+0x100);
	Dump_RegisterValue(WMT_H264ENC_BASE+0x104);
	Dump_RegisterValue(WMT_H264ENC_BASE+0x110);
	Dump_RegisterValue(WMT_H264ENC_BASE+0x114);
	Dump_RegisterValue(WMT_H264ENC_BASE+0x118);
	Dump_RegisterValue(WMT_H264ENC_BASE+0x11C);  /* debug register */
	Dump_RegisterValue(WMT_H264ENC_BASE+0x120);  /* debug register */
	Dump_RegisterValue(WMT_H264ENC_BASE+0x124);  /* debug register */
	Dump_RegisterValue(WMT_H264ENC_BASE+0x130);
	Dump_RegisterValue(WMT_H264ENC_BASE+0x134);
	Dump_RegisterValue(WMT_H264ENC_BASE+0x140);
	Dump_RegisterValue(WMT_H264ENC_BASE+0x144);
	Dump_RegisterValue(WMT_H264ENC_BASE+0x150);
	Dump_RegisterValue(WMT_H264ENC_BASE+0x154);
	Dump_RegisterValue(WMT_H264ENC_BASE+0x158);
	Dump_RegisterValue(WMT_H264ENC_BASE+0x15C);
	Dump_RegisterValue(WMT_H264ENC_BASE+0x160);
	Dump_RegisterValue(WMT_H264ENC_BASE+0x164);
	Dump_RegisterValue(WMT_H264ENC_BASE+0x168);
	Dump_RegisterValue(WMT_H264ENC_BASE+0x16C);
	Dump_RegisterValue(WMT_H264ENC_BASE+0x170);
	Dump_RegisterValue(WMT_H264ENC_BASE+0x174);
	Dump_RegisterValue(WMT_H264ENC_BASE+0x180);
	Dump_RegisterValue(WMT_H264ENC_BASE+0x184);
	Dump_RegisterValue(WMT_H264ENC_BASE+0x188);
	Dump_RegisterValue(WMT_H264ENC_BASE+0x18C);
	Dump_RegisterValue(WMT_H264ENC_BASE+0x190);
	Dump_RegisterValue(WMT_H264ENC_BASE+0x194);
	Dump_RegisterValue(WMT_H264ENC_BASE+0x198);
	Dump_RegisterValue(WMT_H264ENC_BASE+0x19C);
	Dump_RegisterValue(WMT_H264ENC_BASE+0x1A0);
	Dump_RegisterValue(WMT_H264ENC_BASE+0x1A8);

	Dump_RegisterValue(WMT_H264ENC_BASE+0x200);
	Dump_RegisterValue(WMT_H264ENC_BASE+0x204);
	Dump_RegisterValue(WMT_H264ENC_BASE+0x210);
	Dump_RegisterValue(WMT_H264ENC_BASE+0x214);
	Dump_RegisterValue(WMT_H264ENC_BASE+0x240);
	Dump_RegisterValue(WMT_H264ENC_BASE+0x244);
	Dump_RegisterValue(WMT_H264ENC_BASE+0x248);
	Dump_RegisterValue(WMT_H264ENC_BASE+0x300);
	Dump_RegisterValue(WMT_H264ENC_BASE+0x304);
	Dump_RegisterValue(WMT_H264ENC_BASE+0x308);
	Dump_RegisterValue(WMT_H264ENC_BASE+0x310);
	Dump_RegisterValue(WMT_H264ENC_BASE+0x314);
	Dump_RegisterValue(WMT_H264ENC_BASE+0x318);

	Dump_RegisterValue(WMT_H264ENC_BASE+0x400);
	Dump_RegisterValue(WMT_H264ENC_BASE+0x404);
	Dump_RegisterValue(WMT_H264ENC_BASE+0x408);
	Dump_RegisterValue(WMT_H264ENC_BASE+0x410);
	Dump_RegisterValue(WMT_H264ENC_BASE+0x414);
	Dump_RegisterValue(WMT_H264ENC_BASE+0x418);

	Dump_RegisterValue(WMT_H264ENC_BASE+0x500);

	Dump_RegisterValue(WMT_H264ENC_BASE+0x600);
	Dump_RegisterValue(WMT_H264ENC_BASE+0x604);
	Dump_RegisterValue(WMT_H264ENC_BASE+0x608);
	Dump_RegisterValue(WMT_H264ENC_BASE+0x60C);
	Dump_RegisterValue(WMT_H264ENC_BASE+0x610);
	Dump_RegisterValue(WMT_H264ENC_BASE+0x614);
	Dump_RegisterValue(WMT_H264ENC_BASE+0x618);

	Dump_RegisterValue(WMT_H264ENC_BASE+0x700);
	Dump_RegisterValue(WMT_H264ENC_BASE+0x704);
	Dump_RegisterValue(WMT_H264ENC_BASE+0x708);
	Dump_RegisterValue(WMT_H264ENC_BASE+0x70C);
	Dump_RegisterValue(WMT_H264ENC_BASE+0x710);
	Dump_RegisterValue(WMT_H264ENC_BASE+0x714);

	Dump_RegisterValue(WMT_H264ENC_BASE+0x800);
	Dump_RegisterValue(WMT_H264ENC_BASE+0x804);
	Dump_RegisterValue(WMT_H264ENC_BASE+0x808);
	Dump_RegisterValue(WMT_H264ENC_BASE+0x80C);

	Dump_RegisterValue(WMT_H264ENC_BASE+0xF00);
	Dump_RegisterValue(WMT_H264ENC_BASE+0xF04);
	Dump_RegisterValue(WMT_H264ENC_BASE+0xF08);
	Dump_RegisterValue(WMT_H264ENC_BASE+0xF0C);
	Dump_RegisterValue(WMT_H264ENC_BASE+0xF10);
	Dump_RegisterValue(WMT_H264ENC_BASE+0xF14);
	Dump_RegisterValue(WMT_H264ENC_BASE+0xF18);
	Dump_RegisterValue(WMT_H264ENC_BASE+0xF1C);
	Dump_RegisterValue(WMT_H264ENC_BASE+0xF20);
	Dump_RegisterValue(WMT_H264ENC_BASE+0xF24);
	Dump_RegisterValue(WMT_H264ENC_BASE+0xF28);
	Dump_RegisterValue(WMT_H264ENC_BASE+0xF2C);
	Dump_RegisterValue(WMT_H264ENC_BASE+0xF30);
	Dump_RegisterValue(WMT_H264ENC_BASE+0xF34);
	Dump_RegisterValue(WMT_H264ENC_BASE+0xF38);
	Dump_RegisterValue(WMT_H264ENC_BASE+0xF40);
	Dump_RegisterValue(WMT_H264ENC_BASE+0xF44);
	Dump_RegisterValue(WMT_H264ENC_BASE+0xF48);
	Dump_RegisterValue(WMT_H264ENC_BASE+0xF50);
	Dump_RegisterValue(WMT_H264ENC_BASE+0xF54);
	Dump_RegisterValue(WMT_H264ENC_BASE+0xF58);
	Dump_RegisterValue(WMT_H264ENC_BASE+0xF5C);
	Dump_RegisterValue(WMT_H264ENC_BASE+0xF60);
	Dump_RegisterValue(WMT_H264ENC_BASE+0xF64);
	Dump_RegisterValue(WMT_H264ENC_BASE+0xF68);
	Dump_RegisterValue(WMT_H264ENC_BASE+0xF70);
	Dump_RegisterValue(WMT_H264ENC_BASE+0xF74);
	Dump_RegisterValue(WMT_H264ENC_BASE+0xF78);
	Dump_RegisterValue(WMT_H264ENC_BASE+0xF7C);
	Dump_RegisterValue(WMT_H264ENC_BASE+0xF80);
	Dump_RegisterValue(WMT_H264ENC_BASE+0xF84);
	Dump_RegisterValue(WMT_H264ENC_BASE+0xF88);
	Dump_RegisterValue(WMT_H264ENC_BASE+0xF90);
	Dump_RegisterValue(WMT_H264ENC_BASE+0xF94);

} /* End of dbg_dump_registers() */

/*!*************************************************************************
* set_prd_table
* 
*/
/*!
* \brief
*	Set source PRD table
*
* \retval  0 if success
*/ 
static int set_prd_table(h264enc_ctx_t *ctx)
{
	unsigned int  prd_addr_in;
	int i;

	DBG_DETAIL("Dma prd virt addr: 0x%08x\n", ctx->prd_virt);
	DBG_DETAIL("Dma prd phy  addr: 0x%08x\n", ctx->prd_phys);

	prd_addr_in = ctx->prd_virt;
	for (i=0; ; i+=2) {
		unsigned int  addr, len;

		addr = *(unsigned int *)(prd_addr_in + i * 4);
		len  = *(unsigned int *)(prd_addr_in + (i + 1) * 4);

		DBG_DETAIL("[%02d]Addr: 0x%08x\n", i, addr);
		DBG_DETAIL("    Len:  0x%08x (%d)\n", len, (len & 0xFFFF));

		if (len & 0x80000000) {
			break;
		} /* if(len & 0x80000000) */
	} /* for(i=0; ; i+=2) */

	H264_REG_SET32(REG_BASE_H264BSOUT+0x000, ctx->prd_phys);
	TRACE("PRD_ADDR: 0x%x \n", REG32_VAL(REG_BASE_H264BSOUT+0x000));

	return 0;
} /* End of set_prd_table() */

void frozen_screen_handle(void)
{
	int wr_data;
	//set the reference frame to source input
	H264_REG_SET32(REG_BASE_IP+0x010,REG_GET32(REG_BASE_IP+0x080)); 
	H264_REG_SET32(REG_BASE_IP+0x014,REG_GET32(REG_BASE_IP+0x084));

	//the reference frame is NV12 format and phy memory
	H264_REG_SET32(REG_BASE_IP+0x020, 0);
	wr_data = (8<<1)+1;
	H264_REG_SET32 (REG_BASE_IP+0x038, wr_data);

	//disable sub pixel prediction.
	H264_REG_SET32(REG_BASE_IP+0x074, veu.ip_fwp_inter_pred_max_err_sum);
}

int ip_fwp_reg_set(h264enc_ctx_t *ctx)
{
	struct h264enc_prop *hprop = &ctx->prop;

	TRACE("\n===================================================================\n");
	TRACE("frame_no = %d, mb_width = %d, mb_height = %d\n", hprop->frame_num , veu.mb_width, veu.mb_height);  

	//ref frame only need YUV420.
	if (hprop->frame_num == 0 && (void *)(veu.ip_fwp_recon_1st_Y_StartAddress) == NULL) {
		veu.ip_fwp_recon_1st_Y_StartAddress = (int) mb_alloc (veu.ip_lineoffset * veu.height);
		if ((void *)veu.ip_fwp_recon_1st_Y_StartAddress==NULL) {
			ERR("ip_fwp_recon_1st_Y_StartAddress memory allocate FAIL(%d bytes).\n",(veu.ip_lineoffset * veu.height));
			return -1;
		}
		veu.ip_fwp_recon_2nd_Y_StartAddress = (int) mb_alloc (veu.ip_lineoffset * veu.height);
		if ((void *)veu.ip_fwp_recon_2nd_Y_StartAddress==NULL) {
			ERR("ip_fwp_recon_2nd_Y_StartAddress memory allocate FAIL(%d bytes).\n",(veu.ip_lineoffset * veu.height));
			return -1;
		}

		veu.ip_fwp_recon_1st_C_StartAddress = (int) mb_alloc (veu.ip_lineoffset * veu.height/2);
		if ((void *)veu.ip_fwp_recon_1st_C_StartAddress==NULL) {
			ERR("ip_fwp_recon_1st_C_StartAddress memory allocate FAIL(%d bytes).\n",(veu.ip_lineoffset * veu.height)/2);
			return -1;
		}
		veu.ip_fwp_recon_2nd_C_StartAddress = (int) mb_alloc (veu.ip_lineoffset * veu.height/2);
		if ((void *)veu.ip_fwp_recon_2nd_C_StartAddress==NULL) {
			ERR("ip_fwp_recon_2nd_C_StartAddress memory allocate FAIL(%d bytes).\n",(veu.ip_lineoffset * veu.height)/2);
			return -1;
		}
		TRACE("veu.ip_fwp_recon_1st_Y_StartAddress = 0x%08x\n",veu.ip_fwp_recon_1st_Y_StartAddress);
		TRACE("veu.ip_fwp_recon_2nd_Y_StartAddress = 0x%08x\n",veu.ip_fwp_recon_2nd_Y_StartAddress);
		TRACE("veu.ip_fwp_recon_1st_C_StartAddress = 0x%08x\n",veu.ip_fwp_recon_1st_C_StartAddress);
		TRACE("veu.ip_fwp_recon_2nd_C_StartAddress = 0x%08x\n",veu.ip_fwp_recon_2nd_C_StartAddress);
	}

	//--------------------------------------------------------------------------------
	//--------------------------------------------------------------------------------
	TRACE("Start Setting HW H264_ENC/PCR Registers !!!\n");

	TRACE("set veu.pred_bs_adr(0x%08x)\n",veu.pred_bs_adr_phys);
	H264_REG_SET32(REG_BASE_H264BSOUT+0x018, veu.pred_bs_adr_phys ); // REG_PC_ALIGN64B_BASE[31:6]

	TRACE("Complete Setting HW H264_ENC/PCR Registers !!!\n\n");
	//--------------------------------------------------------------------------------
	//--------------------------------------------------------------------------------
	TRACE("Start Setting HW H264_ENC/IP Registers !!!\n");

	H264_REG_SET32(REG_BASE_IP+0x010, veu.ip_Y_StartAddress); // REG_IP_SRC_ALIGN64B_YBASE[31:6]
	H264_REG_SET32(REG_BASE_IP+0x014, veu.ip_C_StartAddress); // REG_IP_SRC_ALIGN64B_CBASE[31:6]
	H264_REG_SET32(REG_BASE_IP+0x018, veu.ip_lineoffset);     // REG_IP_SRC_ALIGN64B_Y_LINEWIDTH[31:6]
	if (hprop->src_phy.col_fmt == VDO_COL_FMT_NV21)
		H264_REG_SET32(REG_BASE_IP+0x020, ((1<<2) + (veu.ip_cbcr_hor_intp <<1) + veu.ip_cbcr_ver_intp ) );     // {28'h0, REG_IP_ENC_USE_CONSTRAINED_INTRA_PRED, REG_IP_ENC_C_FIRST, REG_IP_HOR_INTP, REG_IP_VER_INTP}
	else
		H264_REG_SET32(REG_BASE_IP+0x020, ( (veu.ip_cbcr_hor_intp <<1) + veu.ip_cbcr_ver_intp ) );     // {28'h0, REG_IP_ENC_USE_CONSTRAINED_INTRA_PRED, REG_IP_ENC_C_FIRST, REG_IP_HOR_INTP, REG_IP_VER_INTP}

	H264_REG_SET32(REG_BASE_IP+0x01c, ( veu.ip_lineoffset * ( 1 + veu.ip_cbcr_hor_intp ) ) );     // REG_IP_SRC_ALIGN64B_C_LINEWIDTH[31:6]
	H264_REG_SET32(REG_BASE_IP+0x030, 0x1fff);    // REG_IP_Y4X4_MODE_EN[8:0]
	H264_REG_SET32(REG_BASE_IP+0x034, 0x00f);    // REG_IP_C8X8_MODE_EN[3:0]


	TRACE("Complete Setting HW H264_ENC/IP Registers !!!\n\n");
	//--------------------------------------------------------------------------------
	//--------------------------------------------------------------------------------
	TRACE("Start Setting HW H264_ENC/FWP Registers !!!\n");


	H264_REG_SET32(REG_BASE_IP+0x070, 0x1);   // Set REG_PIC_FWD_RECON_EN to 0x1 , Turn-On IP/FWP Picture Reconstruction .

	if (hprop->slice_type == I_SLICE) {  //(is I_VOP)
		if (hprop->frame_num % 2 ) {
			H264_REG_SET32(REG_BASE_IP+0x090, veu.ip_fwp_recon_2nd_Y_StartAddress);  // REG_FWP_RECON_ALIGN64B_YBASE[31:6]
			H264_REG_SET32(REG_BASE_IP+0x094, veu.ip_fwp_recon_2nd_C_StartAddress);  // REG_FWP_RECON_ALIGN64B_CBASE[31:6]
			H264_REG_SET32(REG_BASE_IP+0x080, veu.ip_fwp_recon_1st_Y_StartAddress);  // REG_FWP_REF_ALIGN64B_YBASE[31:6]
			H264_REG_SET32(REG_BASE_IP+0x084, veu.ip_fwp_recon_1st_C_StartAddress);
		} else {
			H264_REG_SET32(REG_BASE_IP+0x090, veu.ip_fwp_recon_1st_Y_StartAddress);  // REG_FWP_RECON_ALIGN64B_YBASE[31:6]
			H264_REG_SET32(REG_BASE_IP+0x094, veu.ip_fwp_recon_1st_C_StartAddress);  // REG_FWP_RECON_ALIGN64B_CBASE[31:6]
			H264_REG_SET32(REG_BASE_IP+0x080, veu.ip_fwp_recon_2nd_Y_StartAddress);  // REG_FWP_REF_ALIGN64B_YBASE[31:6]
			H264_REG_SET32(REG_BASE_IP+0x084, veu.ip_fwp_recon_2nd_C_StartAddress);
		}
		H264_REG_SET32(REG_BASE_IP+0x088, veu.ip_lineoffset);  // REG_FWP_REF_ALIGN64B_Y_LINEWIDTH[31:6]
		H264_REG_SET32(REG_BASE_IP+0x08C, veu.ip_lineoffset);  // REG_FWP_REF_ALIGN64B_C_LINEWIDTH[31:6]
	} else if (hprop->slice_type == P_SLICE) {  //(is P_VOP)
		// {15'h0, REG_INTER_INTRA_PRED_SWITCH_IN_P , REG_INTER_PRED_MAX_ERR_SUM[15:0]}
		H264_REG_SET32(REG_BASE_IP+0x074, veu.ip_fwp_inter_pred_max_err_sum + ((0x1) << 16) );  
		TRACE("hw_veu.c: Set REG_INTER_PRED_MAX_ERR_SUM[15:0] = 0x%x !!!\n", veu.ip_fwp_inter_pred_max_err_sum );

		if (hprop->frame_num % 2 ) {
			H264_REG_SET32(REG_BASE_IP+0x080, veu.ip_fwp_recon_1st_Y_StartAddress);  // REG_FWP_REF_ALIGN64B_YBASE[31:6]
			H264_REG_SET32(REG_BASE_IP+0x084, veu.ip_fwp_recon_1st_C_StartAddress);  // REG_FWP_REF_ALIGN64B_CBASE[31:6]
			H264_REG_SET32(REG_BASE_IP+0x090, veu.ip_fwp_recon_2nd_Y_StartAddress);  // REG_FWP_RECON_ALIGN64B_YBASE[31:6]
			H264_REG_SET32(REG_BASE_IP+0x094, veu.ip_fwp_recon_2nd_C_StartAddress);  // REG_FWP_RECON_ALIGN64B_CBASE[31:6]
		} else {
			H264_REG_SET32(REG_BASE_IP+0x080, veu.ip_fwp_recon_2nd_Y_StartAddress);  // REG_FWP_REF_ALIGN64B_YBASE[31:6]
			H264_REG_SET32(REG_BASE_IP+0x084, veu.ip_fwp_recon_2nd_C_StartAddress);  // REG_FWP_REF_ALIGN64B_CBASE[31:6]
			H264_REG_SET32(REG_BASE_IP+0x090, veu.ip_fwp_recon_1st_Y_StartAddress);  // REG_FWP_RECON_ALIGN64B_YBASE[31:6]
			H264_REG_SET32(REG_BASE_IP+0x094, veu.ip_fwp_recon_1st_C_StartAddress);  // REG_FWP_RECON_ALIGN64B_CBASE[31:6]
		}

		H264_REG_SET32(REG_BASE_IP+0x088, veu.ip_lineoffset);  // REG_FWP_REF_ALIGN64B_Y_LINEWIDTH[31:6]
		H264_REG_SET32(REG_BASE_IP+0x08C, veu.ip_lineoffset);  // REG_FWP_REF_ALIGN64B_C_LINEWIDTH[31:6]
	}  //(is P_VOP)

	H264_REG_SET32(REG_BASE_IP+0x098, veu.ip_lineoffset);  // REG_FWP_RECON_ALIGN64B_Y_LINEWIDTH[31:6]
	H264_REG_SET32(REG_BASE_IP+0x09C, veu.ip_lineoffset);  // REG_FWP_RECON_ALIGN64B_C_LINEWIDTH[31:6]

	if (hprop->frame_not_changed)
		frozen_screen_handle();

	TRACE("hw_veu.c: Complete Setting HW H264_ENC/FWP Registers !!!\n\n");
	wmb();
	H264_REG_SET32(REG_BASE_IP+0x000, 1); // REG_IP_START
	wmb();
	return 0;
}

void ip_fwp_max_err_sum_cal(void)
{
 
	veu.ip_y4x4_x4_cost_min = REG_GET32(REG_BASE_IP+0x028);
	veu.ip_y4x4_x4_cost_max = REG_GET32(REG_BASE_IP+0x02C);

	TRACE("ip_y4x4_x4_cost_min = %d,ip_y4x4_x4_cost_max = %d\n",veu.ip_y4x4_x4_cost_min,veu.ip_y4x4_x4_cost_max);

	veu.ip_fwp_inter_pred_max_err_sum = (int) ( (((veu.ip_y4x4_x4_cost_min + veu.ip_y4x4_x4_cost_max) >> 1) >> 5)*7/10  );//* (0.7)
}


/*!
*************************************************************************************
* Brief
*     set hardware SPS, PPS messages
*************************************************************************************
*/
void Set_HW_SPS_PPS( h264enc_ctx_t *ctx)
{
	struct h264enc_prop *hprop = &ctx->prop;


	veu.is_gop_first_unit = hprop->idr_flag;

	//set sps header
	H264_REG_SET32(REG_BASE_HEADER+0x00c, 1);	   // REG_HEADER_EN = 1
	veu.hw_header_wen = 1;
	veu.last_enc = 0;
	veu.last_len = 0;
	TRACE("Write Sequence NAL unit start code !!!\n");
	H264_REG_SET32(REG_BASE_HEADER + 0x000, 32);		  // REG_HEADER_LEN
	H264_REG_SET32(REG_BASE_HEADER + 0x004, 0x00000001); // REG_HEADER_CODE
	H264_REG_SET32(REG_BASE_HEADER + 0x000, 8);		  // REG_HEADER_LEN
	H264_REG_SET32(REG_BASE_HEADER + 0x004, 0x00000067); // REG_HEADER_CODE

	TRACE("\n=========================================\n");
	TRACE("  Enter setting Sequence Header !!!\n");
	TRACE("\n=========================================\n");
	GenerateSeq_parameter_set_rbsp (ctx);
	TRACE("\n=========================================\n");
	TRACE("  HW setting Sequence Header Complete !!!\n");
	TRACE("\n==========================================\n");


	//set pps header
	H264_REG_SET32(REG_BASE_HEADER+0x00c, 1);	   // REG_HEADER_EN = 1
	veu.hw_header_wen = 1;
	veu.last_enc = 0;
	veu.last_len = 0;
	TRACE("Write Picture parameter NAL unit start code !!!\n");
	H264_REG_SET32(REG_BASE_HEADER + 0x000, 32);		  // REG_HEADER_LEN
	H264_REG_SET32(REG_BASE_HEADER + 0x004, 0x00000001); // REG_HEADER_CODE
	H264_REG_SET32(REG_BASE_HEADER + 0x000, 8);		  // REG_HEADER_LEN
	H264_REG_SET32(REG_BASE_HEADER + 0x004, 0x00000068); // REG_HEADER_CODE
	TRACE("\n===========================================\n");
	TRACE("  Enter setting Picture Header !!!\n");
	TRACE("\n===========================================\n");
	GeneratePic_parameter_set_rbsp (ctx);
	TRACE("\n===========================================\n");
	TRACE("  HW setting Picture Header Complete !!!\n");
	TRACE("\n===========================================\n");

	
}

/*!*******************************************************************
* \brief
* 
* 
* \retval  0 if success
*/
static void h264enc_update_ste(h264enc_ctx_t *ctx, int status)
{
	unsigned long flags = 0;
	if (!ctx) {
		ERR("H264 encoder update fail (NULL context)\n");
		return;
	}
	DBG_DETAIL("change status from %d to %d\n", ctx->status, status);
	spin_lock_irqsave(&ctx->ste_lock, flags);
	ctx->status = status;
	spin_unlock_irqrestore(&ctx->ste_lock, flags);
	return;
}

/*!*******************************************************************
* \brief
* 
* 
* \retval  0 if success
*/
static int h264_enc_open(h264enc_ctx_t *ctx)
{
	int ret = 0;  

	TRACE("Enter\n");
	DBG_DETAIL("H264_enccoder_open\n");

	if (!ctx) {
		ERR("H264 encoder open fail (NULL context)\n");
		return -EFAULT;
	}

	//initial veu and setting input parameter for veu.

	TRACE("\nInitial_VEU\n\n");
#if 1
	// IP
	veu.ip_first_mb = 1;
	veu.frame_cnt = 0;
	veu.width = 0;
	veu.height = 0;
	veu.sps_en = 1;
	veu.pps_en = 1;
	veu.hw_header_wen = 0;
	veu.bits_to_go = 8;
	veu.bsend_status = 0;
#endif 
	/* Power on */
	wmt_codec_pmc_ctl(CODEC_VE_H264, 1);

	// Enable NA Bridge
	//TRACE(" Enable NA Bridge\n");
	//H264_REG_SET32(REG_BASE_MSVD+0x008, 0x00);

	h264enc_update_ste(ctx, VE_STA_READY);
	ctx->irqevent_flag = 1;

	//open timer
	wmt_codec_timer_init(&ctx->codec_tm, "<h264enc>", 0, 45);

	DBG("H264 encoder open ok\n");
	TRACE("Leave\n");
	return ret;
}

/*!*******************************************************************
* \brief
* 
* 
* \retval  0 if success
*/
static int h264_enc_close(h264enc_ctx_t *ctx)
{

	TRACE("Enter\n");
	DBG_DETAIL("h264_enc_close\n");
	if( !ctx ){
		ERR("encoder close fail (NULL context)\n");
		return -EFAULT;
	}
	DBG("release mb memory allocated by IP\n");
	DBG("veu.ip_fwp_recon_1st_Y_StartAddress = 0x%08x\n",veu.ip_fwp_recon_1st_Y_StartAddress);
	DBG("veu.ip_fwp_recon_2nd_Y_StartAddress = 0x%08x\n",veu.ip_fwp_recon_2nd_Y_StartAddress);
	DBG("veu.ip_fwp_recon_1st_C_StartAddress = 0x%08x\n",veu.ip_fwp_recon_1st_C_StartAddress);
	DBG("veu.ip_fwp_recon_2nd_C_StartAddress = 0x%08x\n",veu.ip_fwp_recon_2nd_C_StartAddress);
	if (veu.ip_fwp_recon_1st_Y_StartAddress) {
		mb_free((unsigned long)veu.ip_fwp_recon_1st_Y_StartAddress);
		veu.ip_fwp_recon_1st_Y_StartAddress=0;
	}
	if (veu.ip_fwp_recon_1st_C_StartAddress) {
		mb_free((unsigned long)veu.ip_fwp_recon_1st_C_StartAddress);
		veu.ip_fwp_recon_1st_C_StartAddress=0;
	}
	if (veu.ip_fwp_recon_2nd_Y_StartAddress) {
		mb_free((unsigned long)veu.ip_fwp_recon_2nd_Y_StartAddress);
		veu.ip_fwp_recon_2nd_Y_StartAddress=0;
	}
	if (veu.ip_fwp_recon_2nd_C_StartAddress) {
		mb_free((unsigned long)veu.ip_fwp_recon_2nd_C_StartAddress);
		veu.ip_fwp_recon_2nd_C_StartAddress=0;
	}

	if (veu.pred_bs_adr_phys) {
		mb_free( (unsigned long)veu.pred_bs_adr_phys);
		veu.pred_bs_adr_phys = 0;
	}

	if (ctx->bs_buf) {
		mb_free((unsigned long)ctx->bs_buf);
		ctx->bs_buf = 0;
	}


	H264_REG_SET32(REG_BASE_MSVD+0x04C, 0x1);
	H264_REG_SET32(REG_BASE_MSVD+0x040,0x0);
	//wmt_codec_clock_en(CODEC_VE_H264, 0);


	wmt_codec_timer_exit(ctx->codec_tm);
	h264enc_update_ste(ctx, VE_STA_CLOSE);
	ctx->irqevent_flag = 1;

	/* Power off */
	wmt_codec_pmc_ctl(CODEC_VE_H264, 0);

	DBG("encoder close ok\n");
	TRACE("Leave\n");
	return 0;
}

/*!*******************************************************************
* \brief
* 
* 
* \retval  0 if success
*/
static int h264_enc_proc(h264enc_ctx_t *ctx)
{
	unsigned int PicSizeInMbs;
	struct h264enc_prop *hprop = &ctx->prop;
	int wr_data;
	int ret;

	TRACE("Enter\n");
	DBG_DETAIL("h264_enc_proc\n");
	if (!ctx) {
		ERR("encoder proc fail (NULL context or input)\n");
		return -EFAULT;
	}

	//veu.bsend_status=0;
	//H264_REG_SET32(WMT_H264ENC_BASE+0x010, 1); 	// clear bsend_status

	//set video encoder unit parameter
	veu.width = hprop->src_phy.img_w;
	veu.height= hprop->src_phy.img_h;

	if (veu.width%16)
		veu.mb_width = (veu.width / 16) + 1;
	else
		veu.mb_width = (veu.width / 16);
	if (veu.height%16)
		veu.mb_height = (veu.height / 16) + 1;
	else
		veu.mb_height = (veu.height / 16);

	veu.ip_Y_StartAddress = hprop->src_phy.y_addr;
	veu.ip_C_StartAddress = hprop->src_phy.c_addr;
	veu.h264bs_adr_init = hprop->buf_addr;
	veu.h264bs_len_init = hprop->buf_size;

	veu.ip_lineoffset = hprop->src_phy.fb_w;
	//NOW just surport YUV420 src
	veu.ip_cbcr_hor_intp = 0;
	veu.ip_cbcr_ver_intp = 0;

	switch (hprop->src_phy.col_fmt) {
	case VDO_COL_FMT_YUV420:
	case VDO_COL_FMT_NV21:
		veu.ip_cbcr_hor_intp = 0;
		veu.ip_cbcr_ver_intp = 0;
		break;
	case VDO_COL_FMT_YUV422H:
		veu.ip_cbcr_hor_intp = 0;
		veu.ip_cbcr_ver_intp = 1;
		break;
	case VDO_COL_FMT_YUV422V:
		veu.ip_cbcr_hor_intp = 1;
		veu.ip_cbcr_ver_intp = 0;
		break;
	case VDO_COL_FMT_YUV444:
		veu.ip_cbcr_hor_intp = 1;
		veu.ip_cbcr_ver_intp = 1;
		break;
	default :
		veu.ip_cbcr_hor_intp = 0;
		veu.ip_cbcr_ver_intp = 0;
		break;
	}

	TRACE("veu.width 				= %d\n",veu.width );
	TRACE("veu.height				= %d\n",veu.height );
	TRACE("veu.mb_width			= %d\n",veu.mb_width );
	TRACE("veu.mb_height 			= %d\n",veu.mb_height );
	TRACE("veu.ip_Y_StartAddress 	= %x\n",veu.ip_Y_StartAddress );
	TRACE("veu.ip_C_StartAddress	= %x\n",veu.ip_C_StartAddress );
	TRACE("veu.h264bs_adr_init	= %x\n",veu.h264bs_adr_init );
	TRACE("veu.h264bs_len_init 	= %d\n",veu.h264bs_len_init );
	TRACE("veu.ip_lineoffset 		= %d\n",veu.ip_lineoffset );
	TRACE("veu.ip_cbcr_hor_intp 	= %d\n",veu.ip_cbcr_hor_intp );
	TRACE("veu.ip_cbcr_ver_intp 	= %d\n",veu.ip_cbcr_ver_intp );

	wmt_codec_clock_en(CODEC_VE_H264, 1);
	wmt_codec_timer_start(ctx->codec_tm);
	//MSVD_SoftwareReset
	wmt_codec_msvd_reset(CODEC_VE_H264);

    wmt_ve_request_sram(VE_H264);

	//allocate a memory for intra prediction in HW
	if (veu.pred_bs_adr_phys == 0) {
		veu.pred_bs_adr_phys = (unsigned int)mb_alloc(veu.mb_width*64+64);
		printk("mb allocate addr(0x%x)\n",veu.pred_bs_adr_phys);
		if ((void *)veu.pred_bs_adr_phys ==NULL) {
			ERR("memory allocate FAIL(%d bytes).\n",veu.mb_width*64);
			return -1;
		}
	}

	TRACE("pred_ptr = 0x%08x\n",veu.pred_bs_adr_phys);

	if (ctx->bs_buf==0) {
		ctx->frame_size = veu.width * veu.height;
		if (ctx->frame_size > MAX_OUTPUT_BUF_SIZE)
			printk("'W' bsout size is too big(%d)\n",ctx->frame_size);
		ctx->bs_buf = mb_alloc(ctx->frame_size);
		wmt_codec_write_prd(ctx->bs_buf,ctx->frame_size,ctx->prd_virt, MAX_OUTPUT_BUF_SIZE);
	}
	set_prd_table(ctx);

	// set parameter for register
	H264_REG_SET32 (REG_BASE_H264BSOUT+0x004, 1);  // REG_H264ENCPRD_EN = 1
	H264_REG_SET32 (REG_BASE_H264BSOUT+0x008, 1);  // Set REG_H264BSOUT_EN = 1

	//create SPS PPS header
	if (hprop->idr_flag == 1)
		Set_HW_SPS_PPS(ctx);

	TRACE("hprop->slice_type = %d\n",hprop->slice_type);
	if (hprop->slice_type == P_SLICE) {
		H264_REG_SET32(WMT_H264ENC_BASE+0x000, 1);  // REG_PVOP = 1
	} else {
		H264_REG_SET32(WMT_H264ENC_BASE+0x000, 0);  // REG_PVOP = 0
	}

	TRACE("\n Start Setting HW H264_ENC TOP Registers !!!\n");
	H264_REG_SET32(WMT_H264ENC_BASE+0x014, 1);    // set disable interrupt

	//JUST TEST
	wr_data = (8 << 1) + 1;
    H264_REG_SET32 (REG_BASE_IP+0x038, wr_data);
    H264_REG_SET32(REG_BASE_MBENC+0x02c, 0x002);
    TRACE(" mb_width = %d, mb_height = %d\n", veu.mb_width, veu.mb_height);
    H264_REG_SET32(WMT_H264ENC_BASE+0x020, veu.mb_width);  // set REG_MB_WIDTH
    H264_REG_SET32(WMT_H264ENC_BASE+0x024, veu.mb_height); // set REG_MB_HEIGHT

	//H264_REG_SET32(REG_BASE_H264BSOUT+0x018, veu.pred_bs_adr_phys); // REG_PC_ALIGN64B_BASE[31:6],veu.pred_bs_adr

	TRACE("Complete Setting HW H264_ENC TOP Registers !!!\n\n");

	PicSizeInMbs = (veu.mb_width)*(veu.mb_height);

	TRACE(" PicSizeInMbs = %d\n", PicSizeInMbs);
	H264_REG_SET32(WMT_H264ENC_BASE+0x028, PicSizeInMbs); // set REG_SLICE_MBNUM
	H264_REG_SET32(WMT_H264ENC_BASE+0x01c, 1);    // set REG_RESYNC
	veu.luma_qp = hprop->QP;
	veu.chroma_qp = QP_SCALE_CR[hprop->QP];
	H264_REG_SET32(REG_BASE_FQIQ+0x000, (veu.luma_qp/6)); // set REG_QP_PER_LUMA[3:0]
	H264_REG_SET32(REG_BASE_FQIQ+0x004, (veu.luma_qp%6)); // set REG_QP_M_SEL_LUMA[2:0]
	H264_REG_SET32(REG_BASE_FQIQ+0x008, (veu.luma_qp%6)); // set REG_IQ_M_SEL_LUMA[2:0]
	H264_REG_SET32(REG_BASE_FQIQ+0x010, (veu.chroma_qp/6)); // set REG_QP_PER_CHROMA[3:0]
	H264_REG_SET32(REG_BASE_FQIQ+0x014, (veu.chroma_qp%6)); // set REG_QP_M_SEL_CHROMA[2:0]
	H264_REG_SET32(REG_BASE_FQIQ+0x018, (veu.chroma_qp%6)); // set REG_IQ_M_SEL_CHROMA[2:0]

	veu.lambda_md = hprop->QP<=12?QP2QUANT[0]:QP2QUANT[hprop->QP-12];
	veu.lambda_mdfp = veu.lambda_md*32;
	veu.ip_cost = veu.lambda_mdfp*1; //(int) floor( lambda );
	H264_REG_SET32(REG_BASE_IP+0x024, veu.ip_cost); // REG_IP_Y4X4_LAMBDA_X3_COST[15:0]

	//set reconstruct frame and reference frame address
	ret = ip_fwp_reg_set(ctx);
	if (ret) return ret;

	H264_REG_SET32(REG_BASE_HEADER+0x00c, 1);     // REG_HEADER_EN = 1

	veu.hw_header_wen = 1;
	veu.last_enc = 0;
	veu.last_len = 0;
	TRACE("Write IDR Slice NAL unit start code !!!\n");
	H264_REG_SET32(REG_BASE_HEADER + 0x000, 32);         // REG_HEADER_LEN
	H264_REG_SET32(REG_BASE_HEADER + 0x004, 0x00000001); // REG_HEADER_CODE
	H264_REG_SET32(REG_BASE_HEADER + 0x000, 8);          // REG_HEADER_LEN
	if (hprop->idr_flag) {
		H264_REG_SET32(REG_BASE_HEADER + 0x004, 0x00000065); // REG_HEADER_CODE for IDR
	} else {
		H264_REG_SET32(REG_BASE_HEADER + 0x004, 0x00000041); // REG_HEADER_CODE for I
	}
	TRACE("\n=========================================\n");
	TRACE("  Enter setting Slice Header !!!\n");
	TRACE("\n=========================================\n");

	SliceHeader (ctx);

	ctx->irqevent_flag = 0;
	h264enc_update_ste(ctx, VE_STA_ENCODEING);

	DBG("do proc ok\n");
	TRACE("Leave\n");

	return 0;
}

/*!*******************************************************************
* \brief
* 
* 
* \retval  0 if success
*/
static int h264_enc_finish(h264enc_ctx_t *ctx)
{
	struct h264enc_prop *hprop = &ctx->prop;
	int ret = 0;
	unsigned char* ptr;
#ifndef H264ENC_IRQ_MODE
	int time;
#endif

	TRACE("Enter\n");
	DBG_DETAIL("h264_enc_finish\n");
	if (!ctx) {
		ERR("enc finish fail (NULL context or frameinfo)\n");
		return -EFAULT;
	}

	TRACE("wait enc finish\n");
	DBG("0x%08x\n",veu.h264bs_adr_init);


#ifdef H264ENC_IRQ_MODE
	if (wait_event_interruptible_timeout(h264enc_wait,veu.bsend_status, msecs_to_jiffies(100)) == 0) {

		ERR("H264 encoder time out(%d).\n",veu.tmp_data);
		h264enc_update_ste(ctx, VE_STA_ERR_UNKNOWN);
		return -1;
	} else {
		veu.bsend_status= 0;
		veu.tmp_data = 0;
		H264_REG_SET32(WMT_H264ENC_BASE+0x010, 1); 	// clear bsend_status
		if (veu.mem_buf_full) {
			ERR("out of memory for h264 encoder\n");
			dbg_dump_registers(ctx);
			h264enc_update_ste(ctx, VE_STA_ERR_UNKNOWN);
			return -2;
		}

		TRACE("veu.h264bs_adr_init addr =0x%08x, input bs buf addr = 0x%08x\n",veu.h264bs_adr_init, ctx->prop.buf_addr);

		veu.bs_bytecnt 	= REG_GET32(REG_BASE_H264BSOUT + 0x00c);
		hprop->NumberofTextureBits = REG_GET32(REG_BASE_H264BSOUT+0x020);
		hprop->TotalMAD = REG_GET32(REG_BASE_H264BSOUT+0x024);
		hprop->NumberofHeaderBits = REG_GET32(REG_BASE_H264BSOUT+0x01C);
		wmt_codec_clock_en(CODEC_VE_H264, 0);
//		hprop->buf_addr	= veu.h264bs_adr_init;
//		hprop->buf_size	= veu.h264bs_len_init;
		hprop->enc_size = veu.bs_bytecnt;
		ptr = (unsigned char*)veu.pred_bs_adr_init;

		//dmac_flush_range((void *)mb_phys_to_virt(ctx->bs_buf), (void *)(mb_phys_to_virt(ctx->bs_buf)+veu.h264bs_len_init));
		//outer_flush_range(ctx->bs_buf, ctx->bs_buf+veu.h264bs_len_init);
		copy_to_user((void *)veu.h264bs_adr_init,(const void *)mb_phys_to_virt(ctx->bs_buf),veu.bs_bytecnt);

		if (!access_ok(VERIFY_WRITE, (void __user *) veu.h264bs_adr_init,veu.h264bs_len_init)) {
			TRACE("L2 cache flush(0x%08x,0x%08x)",veu.h264bs_adr_init,__pa(veu.h264bs_adr_init));
			dmac_flush_range((void *)veu.h264bs_adr_init, (void *)(veu.h264bs_adr_init+veu.h264bs_len_init));
			outer_flush_range(__pa(veu.h264bs_adr_init), __pa(veu.h264bs_adr_init)+veu.h264bs_len_init);
		}
		DBG("veu.bs_bytecnt = %d\n",veu.bs_bytecnt);

		h264enc_update_ste(ctx, VE_STA_ENCODE_DONE);
	}
#else//POLLING MODE

	veu.bsend_status = 0;
	time=1000;
	while (time) {
		veu.rd_data = REG_GET32(WMT_H264ENC_BASE+0x010);
		veu.bsend_status = (veu.rd_data & 0x01);
		veu.mem_buf_full  = (veu.rd_data & 0x08) >> 3;
		if (veu.bsend_status)
			break;
		time--;
    }
	if (time==0) {
		ERR(" encode time out \n");
		dbg_dump_registers(ctx);
		h264enc_update_ste(ctx,H264ENC_FAIL);
		return -1;
	}
	// mem_buf_full hardware means still have data not write to memory, because of memory full
	// software need set new PRD_TABLE for hardware write ou
	if (veu.mem_buf_full) {
		ERR("out of memory for h264 encoder\n");
		h264enc_update_ste(ctx, VE_STA_ERR_MEM_FULL);
		H264_REG_SET32(WMT_H264ENC_BASE+0x010, 1); 	// clear bsend_status
		return -2;
	}

	veu.bs_bytecnt = REG_GET32(REG_BASE_H264BSOUT + 0x00c);
//	fi->buf_addr	= veu.h264bs_adr_init;
//	fi->buf_size	= veu.h264bs_len_init;
	hprop->enc_size = veu.bs_bytecnt;
	TRACE("veu.h264bs_adr_init = 0x%08x\n",veu.h264bs_adr_init);
	TRACE("veu.bs_bytecnt = %d\n",veu.bs_bytecnt);

	ptr = (unsigned char*)veu.pred_bs_adr_init;
	mb_free((unsigned long)ptr);

	if (!access_ok(VERIFY_WRITE, (void __user *) veu.h264bs_adr_init,veu.h264bs_len_init)) {
		TRACE("L2 cache flush(0x%08x,0x%08x)",veu.h264bs_adr_init,__pa(veu.h264bs_adr_init));
		dmac_flush_range(veu.h264bs_adr_init, veu.h264bs_adr_init+veu.h264bs_len_init);
		outer_flush_range(__pa(veu.h264bs_adr_init), __pa(veu.h264bs_adr_init)+veu.h264bs_len_init);
	}

	h264enc_update_ste(ctx, VE_STA_ENCODE_DONE);

	TRACE("Wait for HW H264ENC BSOUT_FINISH  !!!\n");
	veu.rd_data = REG_GET32(WMT_H264ENC_BASE+0x010);
	veu.bsend_status = (veu.rd_data & 0x01);
	veu.mem_buf_full = (veu.rd_data & 0x80000000) >> 31;
	ip_fwp_max_err_sum_cal();
	//H264_REG_SET32 (REG_BASE_H264BSOUT+0x004, 1);	// REG_H264ENCPRD_EN = 1, reset mem_buf_full condition
	H264_REG_SET32(WMT_H264ENC_BASE+0x010, 1); 	// clear bsend_status
	wmt_codec_clock_en(CODEC_VE_H264, 0);
#endif

	TRACE("wait finish ok\n");
	TRACE("Leave\n");
	return ret;
}

/*!*******************************************************************
* \brief
* 
* 
* \retval  0 if success
*/
irqreturn_t wmt_h264enc_isr(int irq, void *dev_id)
{
#ifdef H264ENC_IRQ_MODE
	h264enc_ctx_t *ctx = &h264enc_drv;
  
	TRACE("Enter\n");
	DBG("H264ENC isr\n");

	wmt_codec_timer_stop(ctx->codec_tm);
	H264_REG_SET32(WMT_H264ENC_BASE+0x014,REG_GET32(WMT_H264ENC_BASE+0x014)& ~(REG_GET32(WMT_H264ENC_BASE+0x010)));
	veu.rd_data = REG_GET32(WMT_H264ENC_BASE+0x010);
	H264_REG_SET32(WMT_H264ENC_BASE+0x010,veu.rd_data);

	veu.bsend_status = (veu.rd_data & 0x01);
	veu.ipend_status = (veu.rd_data & 0x04) >> 2;
	veu.mem_buf_full = (veu.rd_data & 0x08) >> 3;
    
    veu.tmp_data |= veu.rd_data;

#if 0
	if(veu.tmp_data !=5)
		return IRQ_NONE;
#endif
	if(!veu.bsend_status)
		return IRQ_NONE;

	//veu.bs_bytecnt = REG_GET32(REG_BASE_H264BSOUT + 0x00c);
	ip_fwp_max_err_sum_cal();

	wmt_ve_free_sram(VE_H264);

	wake_up_interruptible(&h264enc_wait);
#endif
	TRACE("Leave\n");
	return IRQ_HANDLED;
}

/*!*******************************************************************
* \brief
* 
* 
* \retval  0 if success
*/
static long h264enc_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	int err = 0, ret = 0;
	unsigned long flags =0;
	h264enc_ctx_t *ctx;
	struct h264enc_prop prop;

	TRACE("Enter\n");
	DBG_DETAIL("Check input arguments\n");
	if (_IOC_TYPE(cmd) != VE_IOC_MAGIC) return -ENOTTY;
	if (_IOC_NR(cmd) > VE_IOC_MAXNR)    return -ENOTTY;
	/* check argument area */
	if (_IOC_DIR(cmd) & _IOC_READ) {
		err = !access_ok(VERIFY_WRITE, (void __user *) arg, _IOC_SIZE(cmd));
	} else if (_IOC_DIR(cmd) & _IOC_WRITE) {
		err = !access_ok(VERIFY_READ, (void __user *) arg, _IOC_SIZE(cmd));
	}
	if (err) return -EFAULT;

	DBG_DETAIL("IOCTL handler\n");
	spin_lock_irqsave(&h264enc_lock, flags);
	ctx = (h264enc_ctx_t *)filp->private_data;

	switch (cmd) {
	case VEIOSET_ENCODE_LOOP:
		ret = copy_from_user((void *)&prop, (const void *)arg, sizeof(struct h264enc_prop));
		if (ret) {
			ERR("copy_from_user FAIL in VEIOSET_ENCODE_LOOP!\n");
			return -EFAULT;
		}
		if (((PAGE_ALIGN(prop.buf_size)/PAGE_SIZE)*8) > MAX_INPUT_BUF_SIZE) {
			ERR("input size overflow. %x/%x\n", prop.buf_size, MAX_INPUT_BUF_SIZE*4096/8);
			return -EFAULT;
		}
		ctx->prop = prop;
		spin_unlock_irqrestore(&h264enc_lock, flags);
		ret = h264_enc_proc(ctx);
		if (ret)
			return -EFAULT;

		ret = h264_enc_finish(ctx);
		copy_to_user((void *)arg, (const void *)&ctx->prop, sizeof(struct h264enc_prop));
		spin_lock_irqsave(&h264enc_lock, flags);
		break;

	default:
		ERR("Unknown IOCTL:0x%x in h264enc_ioctl()!\n", cmd);
		ret = -ENOTTY;
		break;
	} /* switch(cmd) */
	spin_unlock_irqrestore(&h264enc_lock, flags);
	TRACE("Leave\n");
	return ret;
}

/*!*******************************************************************
* \brief
* 
* 
* \retval  0 if success
*/
static int h264enc_open(struct inode *inode, struct file *filp)
{
	h264enc_ctx_t *ctx = &h264enc_drv;
	struct ve_info *info;
	int ret = 0;

	TRACE("Enter\n");
	DBG_DETAIL("open\n");

	/*------------------------------------------------------------------
		Step 1: initial linux settings
	------------------------------------------------------------------*/
	spin_lock(&h264enc_lock);
	if (h264enc_dev_ref) {
		/* Currently we do not support multi-open */
		ERR("multi-open:h264enc dev ref %d\n", h264enc_dev_ref);
		spin_unlock(&h264enc_lock);
		return -EBUSY;
	}
	h264enc_dev_ref++;

	DBG("open h264enc dev ref %d\n", h264enc_dev_ref);

	/*------------------------------------------------------------------
		Step 2: initial hardware decoder
	------------------------------------------------------------------*/
	DBG_DETAIL("reset enecoder context %p (%d bytes)\n", ctx, sizeof(h264enc_ctx_t));
	memset((void *)&h264enc_drv, 0x0, sizeof(h264enc_ctx_t));
	memset((void *)&veu, 0x0, sizeof(VEU));

	/*------------------------------------------------------------------
		Step 3: set PRD table for encoder
	------------------------------------------------------------------*/
	info =(struct ve_info *)filp->private_data;

	ctx->prd_virt= (unsigned int)info->prdt_virt;
	ctx->prd_phys= (unsigned int)info->prdt_phys;

	DBG_DETAIL("prd_virt: 0x%08x, prd_hys: 0x%08x\n", ctx->prd_virt, ctx->prd_phys);

	filp->private_data = ctx;
	ctx->size 		 = sizeof(h264enc_ctx_t);
	ctx->ste_lock 	 = h264enc_ste_lock;

	ctx->irqevent_flag = 1;
	DBG_DETAIL("open hardware encoder\n");
	ret = h264_enc_open(ctx);

	spin_unlock(&h264enc_lock);
	TRACE("Leave\n");
	return ret;
}

/*!*******************************************************************
* \brief
* 
* 
* \retval  0 if success
*/
static int h264enc_release(struct inode *inode, struct file *filp)
{
	h264enc_ctx_t *ctx = (h264enc_ctx_t *)filp->private_data;

	TRACE("Enter\n");
	DBG_DETAIL("release\n");

	/*--------------------------------------------------------------------------
		Step 1: Wait encode finish if current status is in decoding
	--------------------------------------------------------------------------*/
	if (ctx->status == VE_STA_ENCODEING) {
		ERR("H264 encoder closed warring, H264 is encoding...\n");
		h264_enc_finish(ctx);
	}
	/*------------------------------------------------------------------
		Step 2: release linux settings
	------------------------------------------------------------------*/
	spin_lock(&h264enc_lock);
	if (!h264enc_dev_ref) {
		spin_unlock(&h264enc_lock);
		return -EBUSY;
	}
	h264enc_dev_ref--;
	DBG("close h264 encoder dev ref %d\n", h264enc_dev_ref);

	/*------------------------------------------------------------------
		Step 3: close hardware decoder
	------------------------------------------------------------------*/
	DBG_DETAIL("close hardware encoder\n");
	if (ctx) {
		h264_enc_close(ctx);
	}

	spin_unlock(&h264enc_lock);
	module_put(THIS_MODULE);

	TRACE("Leave\n");
	return 0;
}

/*!*******************************************************************
* \brief
* 
* 
* \retval  0 if success
*/
static int h264enc_setup(void)
{
	TRACE("Enter\n");
	DBG_DETAIL("setup\n");
	spin_lock_init(&h264enc_lock);
	spin_lock_init(&h264enc_ste_lock);
	h264enc_drv.irqevent_flag = 1;
	TRACE("Leave\n");
	return 0;
}

/*!*******************************************************************
* \brief
* 
* 
* \retval  0 if success
*/
static int h264enc_remove(void)
{
	TRACE("Enter\n");
	DBG_DETAIL("remove\n");
	TRACE("Leave\n");
	return 0;
}

/*!*******************************************************************
* \brief
* 
* 
* \retval  0 if success
*/
static int h264enc_suspend(pm_message_t status)
{
	TRACE("Enter\n");
	DBG_DETAIL("suspend\n");
	switch (status.event) {
		case PM_EVENT_SUSPEND:
		case PM_EVENT_FREEZE:
		case PM_EVENT_PRETHAW:
		default:
			break;
	}
	TRACE("Leave\n");
	return 0;
}

/*!*******************************************************************
* \brief
* 
* 
* \retval  0 if success
*/
static int h264enc_resume(void)
{
	TRACE("Enter\n");
	DBG_DETAIL("resume\n");
	TRACE("Leave\n");
	return 0;
}

/*!*************************************************************************
    platform device struct define
****************************************************************************/
struct videoencoder h264_encoder = {
	.name    = "h264enc",
	.id      = VE_H264,
	.setup   = h264enc_setup,
	.remove  = h264enc_remove,
	.suspend = h264enc_suspend,
	.resume  = h264enc_resume,
#ifdef H264ENC_POLLING_MODE
	.irq_num  = 0,
#else
	.irq_num  = H264ENC_IRQ,
#endif
	.isr      = wmt_h264enc_isr,
	.isr_data = (void *)&h264enc_drv,
	.fops = {
		.owner   = THIS_MODULE,
		.open    = h264enc_open,
		.unlocked_ioctl = h264enc_ioctl,
		.release = h264enc_release,
	},
	//.get_info = h264enc_get_info,
	.device   = NULL,
	.ve_class = NULL,
};

/*!*******************************************************************
* \brief
* 
* 
* \retval  0 if success
*/
int h264enc_init(void)
{
	int ret = 0;
	TRACE("Enter\n");

	wmt_ve_register(&h264_encoder);

	DBG_DETAIL("register encoder\n");
	TRACE("Leave\n");
	return ret;
}

/*!*******************************************************************
* \brief
* 
* 
* \retval  0 if success
*/
void h264enc_exit(void)
{
	TRACE("Enter\n");
	wmt_ve_unregister(&h264_encoder);
	DBG_DETAIL("unregister encoder\n");
	TRACE("Leave\n");
	return;
}

module_init(h264enc_init);
module_exit(h264enc_exit);

MODULE_AUTHOR("WonderMedia Technologies, Inc.");
MODULE_DESCRIPTION("WMT h264 encoder device driver");
MODULE_LICENSE("GPL");

#undef WMT_H264ENC_C
/*----------------------------------------------------------------------------*/
/*=== END ====================================================================*/
#endif

