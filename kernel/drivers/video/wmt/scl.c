/*++
 * linux/drivers/video/wmt/scl.c
 * WonderMedia video post processor (VPP) driver
 *
 * Copyright c 2013  WonderMedia  Technologies, Inc.
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

#define SCL_C
#undef DEBUG
/* #define DEBUG */
/* #define DEBUG_DETAIL */

#include "scl.h"

#ifdef WMT_FTBLK_SCL
void scl_reg_dump(void)
{
	vpp_set_clock_enable(DEV_SCL444U, 1, 0);

	DPRINT("========== SCL register dump ==========\n");
	vpp_reg_dump(REG_SCL_BASE1_BEGIN,
		REG_SCL_BASE1_END - REG_SCL_BASE1_BEGIN);
	vpp_reg_dump(REG_SCL_BASE2_BEGIN,
		REG_SCL_BASE2_END - REG_SCL_BASE2_BEGIN);
	DPRINT("---------- SCL scale ----------\n");
	DPRINT("scale enable %d\n", vppif_reg32_read(SCL_ALU_ENABLE));
	DPRINT("mode bilinear(H %d,V %d),recursive(H %d,V %d)\n",
		vppif_reg32_read(SCL_BILINEAR_H),
		vppif_reg32_read(SCL_BILINEAR_V),
		vppif_reg32_read(SCL_RECURSIVE_H),
		vppif_reg32_read(SCL_RECURSIVE_V));
	DPRINT("src(%d,%d),dst(%d,%d)\n",
		vppif_reg32_read(SCLR_YPXLWID),
		vppif_reg32_read(SCL_VXWIDTH),
		vppif_reg32_read(SCL_H_THR),
		vppif_reg32_read(SCL_V_THR));
	DPRINT("scale width H %d,V %d\n",
		vppif_reg32_read(SCL_HXWIDTH),
		vppif_reg32_read(SCL_VXWIDTH));
	DPRINT("H scale up %d,V scale up %d\n",
		vppif_reg32_read(SCL_HSCLUP_ENABLE),
		vppif_reg32_read(SCL_VSCLUP_ENABLE));
	DPRINT("H sub step %d,thr %d,step %d,sub step cnt %d,i step cnt %d\n",
		vppif_reg32_read(SCL_H_SUBSTEP),
		vppif_reg32_read(SCL_H_THR),
		vppif_reg32_read(SCL_H_STEP),
		vppif_reg32_read(SCL_H_I_SUBSTEPCNT),
		vppif_reg32_read(SCL_H_I_STEPCNT));
	DPRINT("V sub step %d,thr %d,step %d,sub step cnt %d,i step cnt %d\n",
		vppif_reg32_read(SCL_V_SUBSTEP),
		vppif_reg32_read(SCL_V_THR),
		vppif_reg32_read(SCL_V_STEP),
		vppif_reg32_read(SCL_V_I_SUBSTEPCNT),
		vppif_reg32_read(SCL_V_I_STEPCNT));

	DPRINT("---------- SCL filter ----------\n");
	DPRINT("DEBLOCK %d,boundary 1st 0x%x,2nd 0x%x\n,",
		vppif_reg32_read(SCL_DEBLOCK_ENABLE),
		vppif_reg32_read(SCL_1ST_LAYER_BOUNDARY),
		vppif_reg32_read(SCL_2ND_LAYER_BOUNDARY));
	DPRINT("FIELD DEFLICKER %d,up %s down,thr Y %d,C %d\n",
		vppif_reg32_read(SCL_FIELD_DEFLICKER),
		vppif_reg32_read(SCL_FIELD_DEFLICKER) ? "&" : "or",
		vppif_reg32_read(SCL_FIELD_FILTER_Y_THD),
		vppif_reg32_read(SCL_FIELD_FILTER_C_THD));
	DPRINT("FRAME DEFLICKER %d,%s,2^%d,scene chg %d\n",
		vppif_reg32_read(SCL_FRAME_DEFLICKER),
		vppif_reg32_read(SCL_FRAME_FILTER_RGB) ? "RGB" : "Y",
		vppif_reg32_read(SCL_FRAME_FILTER_SAMPLER),
		vppif_reg32_read(SCL_FR_FILTER_SCENE_CHG_THD));
	DPRINT("CSC enable %d,CSC clamp %d\n",
		vppif_reg32_read(SCL_CSC_ENABLE),
		vppif_reg32_read(SCL_CSC_CLAMP_ENABLE));
	DPRINT("---------- SCL TG ----------\n");
	DPRINT("TG source : %s\n",
		(vppif_reg32_read(SCL_TG_GOVWTG_ENABLE)) ? "GOVW" : "SCL");
	DPRINT("TG enable %d, wait ready enable %d\n",
		vppif_reg32_read(SCL_TG_ENABLE),
		vppif_reg32_read(SCL_TG_WATCHDOG_ENABLE));
	DPRINT("clk %d,Read cyc %d,1T %d\n",
		vpp_get_base_clock(VPP_MOD_SCL),
		vppif_reg32_read(SCL_TG_RDCYC),
		vppif_reg32_read(SCL_READCYC_1T));
	DPRINT("H total %d, beg %d, end %d\n",
		vppif_reg32_read(SCL_TG_H_ALLPIXEL),
		vppif_reg32_read(SCL_TG_H_ACTBG),
		vppif_reg32_read(SCL_TG_H_ACTEND));
	DPRINT("V total %d, beg %d, end %d\n",
		vppif_reg32_read(SCL_TG_V_ALLLINE),
		vppif_reg32_read(SCL_TG_V_ACTBG),
		vppif_reg32_read(SCL_TG_V_ACTEND));
	DPRINT("VBIE %d,PVBI %d\n",
		vppif_reg32_read(SCL_TG_VBIE),
		vppif_reg32_read(SCL_TG_PVBI));
	DPRINT("Watch dog 0x%x\n",
		vppif_reg32_read(SCL_TG_WATCHDOG_VALUE));
	DPRINT("---------- SCLR FB ----------\n");
	DPRINT("SCLR MIF enable %d\n",
		vppif_reg32_read(SCLR_MIF_ENABLE));
	DPRINT("color format %s\n", vpp_colfmt_str[sclr_get_color_format()]);
	DPRINT("color bar enable %d,mode %d,inv %d\n",
		vppif_reg32_read(SCLR_COLBAR_ENABLE),
		vppif_reg32_read(SCLR_COLBAR_MODE),
		vppif_reg32_read(SCLR_COLBAR_INVERSION));
	DPRINT("sourc mode : %s,H264 %d\n",
		(vppif_reg32_read(SCLR_TAR_DISP_FMT)) ? "field" : "frame",
		vppif_reg32_read(SCLR_MEDIAFMT_H264));
	DPRINT("Y addr 0x%x, C addr 0x%x\n",
		vppif_reg32_in(REG_SCLR_YSA),
		vppif_reg32_in(REG_SCLR_CSA));
	DPRINT("width %d, fb width %d\n",
		vppif_reg32_read(SCLR_YPXLWID),
		vppif_reg32_read(SCLR_YBUFWID));
	DPRINT("H crop %d, V crop %d\n",
		vppif_reg32_read(SCLR_HCROP),
		vppif_reg32_read(SCLR_VCROP));
	DPRINT("---------- SCLW FB ----------\n");
	DPRINT("SCLW MIF enable %d\n", vppif_reg32_read(SCLW_MIF_ENABLE));
	DPRINT("color format %s\n", vpp_colfmt_str[sclw_get_color_format()]);
	DPRINT("Y addr 0x%x, C addr 0x%x\n",
		vppif_reg32_in(REG_SCLW_YSA), vppif_reg32_in(REG_SCLW_CSA));
	DPRINT("Y width %d, fb width %d\n",
		vppif_reg32_read(SCLW_YPXLWID), vppif_reg32_read(SCLW_YBUFWID));
	DPRINT("C width %d, fb width %d\n",
		vppif_reg32_read(SCLW_CPXLWID), vppif_reg32_read(SCLW_CBUFWID));
	DPRINT("Y err %d, C err %d\n",
		vppif_reg32_read(SCLW_INTSTS_MIFYERR),
		vppif_reg32_read(SCLW_INTSTS_MIFCERR));
	DPRINT("---------- SCLR2 FB ----------\n");
	DPRINT("MIF enable %d\n", vppif_reg32_read(SCL_R2_MIF_EN));
	DPRINT("color format %s\n", vpp_colfmt_str[scl_R2_get_color_format()]);
	DPRINT("color bar enable %d,mode %d,inv %d\n",
		vppif_reg32_read(SCL_R2_COLOR_EN),
		vppif_reg32_read(SCL_R2_COLOR_EN),
		vppif_reg32_read(SCL_R2_COLOR_INV));
	DPRINT("sourc mode : %s,H264 %d\n",
		(vppif_reg32_read(SCL_R2_IOFMT)) ? "field" : "frame",
		vppif_reg32_read(SCL_R2_H264_FMT));
	DPRINT("Y addr 0x%x, C addr 0x%x\n",
		vppif_reg32_in(REG_SCLR2_YSA), vppif_reg32_in(REG_SCLR2_CSA));
	DPRINT("width %d, fb width %d\n",
		vppif_reg32_read(SCL_R2_LNSIZE), vppif_reg32_read(SCL_R2_FBW));
	DPRINT("H crop %d, V crop %d\n",
		vppif_reg32_read(SCL_R2_HCROP), vppif_reg32_read(SCL_R2_VCROP));
	DPRINT("---------- ALPHA ----------\n");
	DPRINT("src alpha %d,dst alpha %d,swap %d\n",
		vppif_reg32_read(SCL_ALPHA_SRC),
		vppif_reg32_read(SCL_ALPHA_DST),
		vppif_reg32_read(SCL_ALPHA_SWAP));
	DPRINT("src fix 0x%x,dst fix 0x%x\n",
		vppif_reg32_read(SCL_ALPHA_SRC_FIXED),
		vppif_reg32_read(SCL_ALPHA_DST_FIXED));
	DPRINT("---------- ColorKey ----------\n");
	DPRINT("enable %d\n", vppif_reg32_read(SCL_ALPHA_COLORKEY_ENABLE));
	DPRINT("from %s,comp %d,mode %d\n",
		(vppif_reg32_read(SCL_ALPHA_COLORKEY_FROM)) ? "mif2" : "mif1",
		vppif_reg32_read(SCL_ALPHA_COLORKEY_COMP),
		vppif_reg32_read(SCL_ALPHA_COLORKEY_MODE));
	DPRINT("R 0x%x,G 0x%x,B 0x%x\n",
		vppif_reg32_read(SCL_ALPHA_COLORKEY_R),
		vppif_reg32_read(SCL_ALPHA_COLORKEY_G),
		vppif_reg32_read(SCL_ALPHA_COLORKEY_B));
	DPRINT("---------- sw status ----------\n");
	DPRINT("complete %d\n", p_scl->scale_complete);

	vpp_set_clock_enable(DEV_SCL444U, 0, 0);
}

void scl_set_enable(vpp_flag_t enable)
{
	vppif_reg32_write(SCL_ALU_ENABLE, enable);
}

void scl_set_reg_update(vpp_flag_t enable)
{
	vppif_reg32_write(SCL_REG_UPDATE, enable);
}

void scl_set_reg_level(vpp_reglevel_t level)
{
	switch (level) {
	case VPP_REG_LEVEL_1:
		vppif_reg32_write(SCL_REG_LEVEL, 0x0);
		break;
	case VPP_REG_LEVEL_2:
		vppif_reg32_write(SCL_REG_LEVEL, 0x1);
		break;
	default:
		DBGMSG("*E* check the parameter.\n");
		return;
	}
}

void scl_set_int_enable(vpp_flag_t enable, vpp_int_t int_bit)
{
	/* clean status first before enable/disable interrupt */
	scl_clean_int_status(int_bit);

	if (int_bit & VPP_INT_ERR_SCL_TG)
		vppif_reg32_write(SCLW_INT_TGERR_ENABLE, enable);
	if (int_bit & VPP_INT_ERR_SCLR1_MIF)
		vppif_reg32_write(SCLW_INT_R1MIF_ENABLE, enable);
	if (int_bit & VPP_INT_ERR_SCLR2_MIF)
		vppif_reg32_write(SCLW_INT_R2MIF_ENABLE, enable);
	if (int_bit & VPP_INT_ERR_SCLW_MIFRGB)
		vppif_reg32_write(SCLW_INT_WMIFRGB_ENABLE, enable);
	if (int_bit & VPP_INT_ERR_SCLW_MIFY)
		vppif_reg32_write(SCLW_INT_WMIFYERR_ENABLE, enable);
	if (int_bit & VPP_INT_ERR_SCLW_MIFC)
		vppif_reg32_write(SCLW_INT_WMIFCERR_ENABLE, enable);
}

vpp_int_err_t scl_get_int_status(void)
{
	vpp_int_err_t int_sts;

	int_sts = 0;
	if (vppif_reg32_read(SCL_INTSTS_TGERR))
		int_sts |= VPP_INT_ERR_SCL_TG;
	if (vppif_reg32_read(SCLR_INTSTS_R1MIFERR))
		int_sts |= VPP_INT_ERR_SCLR1_MIF;
	if (vppif_reg32_read(SCLR_INTSTS_R2MIFERR))
		int_sts |= VPP_INT_ERR_SCLR2_MIF;
	if (vppif_reg32_read(SCLW_INTSTS_MIFRGBERR))
		int_sts |= VPP_INT_ERR_SCLW_MIFRGB;
	if (vppif_reg32_read(SCLW_INTSTS_MIFYERR))
		int_sts |= VPP_INT_ERR_SCLW_MIFY;
	if (vppif_reg32_read(SCLW_INTSTS_MIFCERR))
		int_sts |= VPP_INT_ERR_SCLW_MIFC;

	return int_sts;
}

void scl_clean_int_status(vpp_int_err_t int_sts)
{
	if (int_sts & VPP_INT_ERR_SCL_TG)
		vppif_reg32_out(REG_SCL_TG_STS + 0x0, BIT0);
	if (int_sts & VPP_INT_ERR_SCLR1_MIF)
		vppif_reg8_out(REG_SCLR_FIFO_CTL + 0x1, BIT0);
	if (int_sts & VPP_INT_ERR_SCLR2_MIF)
		vppif_reg8_out(REG_SCLR_FIFO_CTL + 0x1, BIT1);
	if (int_sts & VPP_INT_ERR_SCLW_MIFRGB)
		vppif_reg32_out(REG_SCLW_FF_CTL, BIT16);
	if (int_sts & VPP_INT_ERR_SCLW_MIFY)
		vppif_reg32_out(REG_SCLW_FF_CTL, BIT8);
	if (int_sts & VPP_INT_ERR_SCLW_MIFC)
		vppif_reg32_out(REG_SCLW_FF_CTL, BIT0);
}

void scl_set_csc_mode(vpp_csc_t mode)
{
	vdo_color_fmt src_fmt, dst_fmt;

	src_fmt = sclr_get_color_format();
	dst_fmt = sclw_get_color_format();
	mode = vpp_check_csc_mode(mode, src_fmt, dst_fmt, 0);
	if (mode >= VPP_CSC_MAX)
		vppif_reg32_write(SCL_CSC_ENABLE, VPP_FLAG_DISABLE);
	else {
		vppif_reg32_out(REG_SCL_CSC1, vpp_csc_parm[mode][0]);
		vppif_reg32_out(REG_SCL_CSC2, vpp_csc_parm[mode][1]);
		vppif_reg32_out(REG_SCL_CSC3, vpp_csc_parm[mode][2]);
		vppif_reg32_out(REG_SCL_CSC4, vpp_csc_parm[mode][3]);
		vppif_reg32_out(REG_SCL_CSC5, vpp_csc_parm[mode][4]);
		vppif_reg32_out(REG_SCL_CSC6, vpp_csc_parm[mode][5]);
		vppif_reg32_out(REG_SCL_CSC_CTL, vpp_csc_parm[mode][6]);
		vppif_reg32_write(SCL_CSC_ENABLE, VPP_FLAG_ENABLE);
	}
}

void scl_set_scale_enable(vpp_flag_t vscl_enable, vpp_flag_t hscl_enable)
{
	DBGMSG("V %d,H %d\n", vscl_enable, hscl_enable);
	vppif_reg32_write(SCL_VSCLUP_ENABLE, vscl_enable);
	vppif_reg32_write(SCL_HSCLUP_ENABLE, hscl_enable);
}

void scl_set_V_scale(int A, int B) /* A dst,B src */
{
	unsigned int V_STEP;
	unsigned int V_SUB_STEP;
	unsigned int V_THR_DIV2;

	DBG_DETAIL("scl_set_V_scale(%d,%d)\r\n", A, B);
	if (A > B) {
		V_STEP = (B - 1) * 16 / A;
		V_SUB_STEP = (B - 1) * 16  % A;
	} else {
		V_STEP = (16 * B / A);
		V_SUB_STEP = ((16 * B) % A);
	}
	V_THR_DIV2 = A;

	DBG_DETAIL("V step %d,sub step %d, div2 %d\r\n",
				V_STEP, V_SUB_STEP, V_THR_DIV2);

#ifdef SCL_DST_VXWIDTH
	vppif_reg32_write(SCL_DST_VXWIDTH, (A > B) ? A : B);
#endif
	vppif_reg32_write(SCL_VXWIDTH, B);
	vppif_reg32_write(SCL_V_STEP, V_STEP);
	vppif_reg32_write(SCL_V_SUBSTEP, V_SUB_STEP);
	vppif_reg32_write(SCL_V_THR, V_THR_DIV2);
	vppif_reg32_write(SCL_V_I_SUBSTEPCNT, 0);
}

void scl_set_H_scale(int A, int B) /* A dst,B src */
{
	unsigned int H_STEP;
	unsigned int H_SUB_STEP;
	unsigned int H_THR_DIV2;

	DBG_DETAIL("scl_set_H_scale(%d,%d)\r\n", A, B);
	if (A > B) {
		H_STEP = (B - 1) * 16 / A;
		H_SUB_STEP = (B - 1) * 16  % A;
	} else {
		H_STEP = (16 * B / A);
		H_SUB_STEP = ((16 * B) % A);
	}
	H_THR_DIV2 = A;
	DBG_DETAIL("H step %d,sub step %d, div2 %d\r\n",
				H_STEP, H_SUB_STEP, H_THR_DIV2);

	vppif_reg32_write(SCL_HXWIDTH, ((A > B) ? A : B));
	vppif_reg32_write(SCL_H_STEP, H_STEP);
	vppif_reg32_write(SCL_H_SUBSTEP, H_SUB_STEP);
	vppif_reg32_write(SCL_H_THR, H_THR_DIV2);
	vppif_reg32_write(SCL_H_I_SUBSTEPCNT, 0);
}

void scl_set_crop(int offset_x, int offset_y)
{
	/* offset_x &= VPU_CROP_ALIGN_MASK; */ /* ~0x7 */
	offset_x &= ~0xf;

	vppif_reg32_write(SCL_H_I_STEPCNT, offset_x * 16);
	vppif_reg32_write(SCL_V_I_STEPCNT, offset_y * 16);
	/* vppif_reg32_write(VPU_SCA_THR, 0xFF); */
	DBGMSG("[VPU] crop - x : 0x%x, y : 0x%x \r\n",
				offset_x * 16, offset_y * 16);
}

void scl_set_tg_enable(vpp_flag_t enable)
{
	vppif_reg32_write(SCL_TG_ENABLE, enable);
}

unsigned int scl_set_clock(unsigned int pixel_clock)
{
	unsigned int rd_cyc;
	rd_cyc = vpp_get_base_clock(VPP_MOD_SCL) / pixel_clock;
	return rd_cyc;
}

void scl_set_timing(vpp_clock_t *timing, unsigned int pixel_clock)
{
#if 1
	timing->read_cycle = WMT_SCL_RCYC_MIN;
#else
	timing->read_cycle = scl_set_clock(pixel_clock * 2) - 1;
	timing->read_cycle = (timing->read_cycle < WMT_SCL_RCYC_MIN) ?
		WMT_SCL_RCYC_MIN : timing->read_cycle;
	timing->read_cycle = (timing->read_cycle > 255) ?
		0xFF : timing->read_cycle;
#endif
	vppif_reg32_write(SCL_TG_RDCYC, timing->read_cycle);
	vppif_reg32_write(SCL_READCYC_1T, (timing->read_cycle) ? 0 : 1);
	vppif_reg32_write(SCL_TG_H_ALLPIXEL, timing->total_pixel_of_line);
	vppif_reg32_write(SCL_TG_H_ACTBG, timing->begin_pixel_of_active);
	vppif_reg32_write(SCL_TG_H_ACTEND, timing->end_pixel_of_active);
	vppif_reg32_write(SCL_TG_V_ALLLINE, timing->total_line_of_frame);
	vppif_reg32_write(SCL_TG_V_ACTBG, timing->begin_line_of_active);
	vppif_reg32_write(SCL_TG_V_ACTEND, timing->end_line_of_active);
	vppif_reg32_write(SCL_TG_VBIE, timing->line_number_between_VBIS_VBIE);
	vppif_reg32_write(SCL_TG_PVBI, timing->line_number_between_PVBI_VBIS);

#ifdef DEBUG_DETAIL
	vpp_show_timing("scl set timing", 0, timing);
#endif
}

void scl_get_timing(vpp_clock_t *p_timing)
{
	p_timing->read_cycle = vppif_reg32_read(SCL_TG_RDCYC);
	p_timing->total_pixel_of_line = vppif_reg32_read(SCL_TG_H_ALLPIXEL);
	p_timing->begin_pixel_of_active = vppif_reg32_read(SCL_TG_H_ACTBG);
	p_timing->end_pixel_of_active = vppif_reg32_read(SCL_TG_H_ACTEND);
	p_timing->total_line_of_frame = vppif_reg32_read(SCL_TG_V_ALLLINE);
	p_timing->begin_line_of_active = vppif_reg32_read(SCL_TG_V_ACTBG);
	p_timing->end_line_of_active = vppif_reg32_read(SCL_TG_V_ACTEND);
	p_timing->line_number_between_VBIS_VBIE = vppif_reg32_read(SCL_TG_VBIE);
	p_timing->line_number_between_PVBI_VBIS = vppif_reg32_read(SCL_TG_PVBI);
}

void scl_set_watchdog(U32 count)
{
	if (0 != count) {
		vppif_reg32_write(SCL_TG_WATCHDOG_VALUE, count);
		vppif_reg32_write(SCL_TG_WATCHDOG_ENABLE, VPP_FLAG_TRUE);
	} else
		vppif_reg32_write(SCL_TG_WATCHDOG_ENABLE, VPP_FLAG_FALSE);
}

void scl_set_timing_master(vpp_mod_t mod_bit)
{
	if (VPP_MOD_SCL == mod_bit)
		vppif_reg32_write(SCL_TG_GOVWTG_ENABLE, VPP_FLAG_DISABLE);
	else if (VPP_MOD_GOVW == mod_bit)
		vppif_reg32_write(SCL_TG_GOVWTG_ENABLE, VPP_FLAG_ENABLE);
	else {
		DBGMSG("*E* check the parameter.\n");
		return;
	}
}

vpp_mod_t scl_get_timing_master(void)
{
	if (vppif_reg32_read(SCL_TG_GOVWTG_ENABLE))
		return VPP_MOD_GOVW;
	return VPP_MOD_SCL;
}

void scl_set_drop_line(vpp_flag_t enable)
{
	vppif_reg32_write(SCL_SCLDW_METHOD, enable);
}

/* only one feature can work, other should be disable */
void scl_set_filter_mode(vpp_filter_mode_t mode, int enable)
{
	DBG_DETAIL("(%d,%d)\n", mode, enable);
	if (mode != VPP_FILTER_SCALE) {
		if (vppif_reg32_read(SCL_VSCLUP_ENABLE) ||
			vppif_reg32_read(SCL_HSCLUP_ENABLE))
			DPRINT("[SCL] *W* filter can't work w scale\n");
	}
	vppif_reg32_write(SCL_DEBLOCK_ENABLE, 0);
	vppif_reg32_write(SCL_FIELD_DEFLICKER, 0);
	vppif_reg32_write(SCL_FRAME_DEFLICKER, 0);
	switch (mode) {
	default:
	case VPP_FILTER_SCALE: /* scale mode */
		break;
	case VPP_FILTER_DEBLOCK: /* deblock */
		vppif_reg32_write(SCL_DEBLOCK_ENABLE, enable);
		break;
	case VPP_FILTER_FIELD_DEFLICKER: /* field deflicker */
		vppif_reg32_write(SCL_FIELD_DEFLICKER, enable);
		break;
	case VPP_FILTER_FRAME_DEFLICKER: /* frame deflicker */
		vppif_reg32_write(SCL_FRAME_DEFLICKER, enable);
		break;
	}
}

vpp_filter_mode_t scl_get_filter_mode(void)
{
	if (vppif_reg32_read(SCL_VSCLUP_ENABLE) ||
		vppif_reg32_read(SCL_HSCLUP_ENABLE))
		return VPP_FILTER_SCALE;
	if (vppif_reg32_read(SCL_DEBLOCK_ENABLE))
		return VPP_FILTER_DEBLOCK;

	if (vppif_reg32_read(SCL_FIELD_DEFLICKER))
		return VPP_FILTER_FIELD_DEFLICKER;

	if (vppif_reg32_read(SCL_FRAME_DEFLICKER))
		return VPP_FILTER_FRAME_DEFLICKER;

	return VPP_FILTER_SCALE;
}

void sclr_set_mif_enable(vpp_flag_t enable)
{
	vppif_reg32_write(SCLR_MIF_ENABLE, enable);
}

void sclr_set_mif2_enable(vpp_flag_t enable)
{

}

void sclr_set_colorbar(vpp_flag_t enable, int width, int inverse)
{
	vppif_reg32_write(SCLR_COLBAR_MODE, width);
	vppif_reg32_write(SCLR_COLBAR_INVERSION, inverse);
	vppif_reg32_write(SCLR_COLBAR_ENABLE, enable);
}

void sclr_set_field_mode(vpp_display_format_t fmt)
{
	vppif_reg32_write(SCLR_SRC_DISP_FMT, fmt);
}

void sclr_set_display_format(vpp_display_format_t source,
				vpp_display_format_t target)
{
	/* source */
	switch (source) {
	case VPP_DISP_FMT_FRAME:
		vppif_reg32_write(SCLR_SRC_DISP_FMT, 0x0);
		break;
	case VPP_DISP_FMT_FIELD:
		vppif_reg32_write(SCLR_SRC_DISP_FMT, 0x1);
		break;
	default:
		DBGMSG("*E* check the parameter.\n");
		return;
	}

	/* target */
	switch (target) {
	case VPP_DISP_FMT_FRAME:
		vppif_reg32_write(SCLR_TAR_DISP_FMT, 0x0);
		break;
	case VPP_DISP_FMT_FIELD:
		vppif_reg32_write(SCLR_TAR_DISP_FMT, 0x1);
		break;
	default:
		DBGMSG("*E* check the parameter.\n");
		return;
	}
}

void sclr_set_color_format(vdo_color_fmt format)
{
	if (format >= VDO_COL_FMT_ARGB) {
		if (format == VDO_COL_FMT_RGB_565)
			vppif_reg32_write(SCLR_RGB_MODE, 0x1);
		else
			vppif_reg32_write(SCLR_RGB_MODE, 0x3);
		return;
	}
	vppif_reg32_write(SCLR_RGB_MODE, 0x0);
	switch (format) {
	case VDO_COL_FMT_ARGB:
		vppif_reg32_write(SCLR_COLFMT_RGB, 0x1);
		break;
	case VDO_COL_FMT_YUV444:
		vppif_reg32_write(SCLR_COLFMT_RGB, 0x0);
		vppif_reg32_write(SCLR_COLFMT_YUV, 0x2);
		break;
	case VDO_COL_FMT_YUV422H:
		vppif_reg32_write(SCLR_COLFMT_RGB, 0x0);
		vppif_reg32_write(SCLR_COLFMT_YUV, 0x0);
		break;
	case VDO_COL_FMT_YUV420:
		vppif_reg32_write(SCLR_COLFMT_RGB, 0x0);
		vppif_reg32_write(SCLR_COLFMT_YUV, 0x1);
		break;
	default:
		DBGMSG("*E* check the parameter.\n");
		return;
	}
}

vdo_color_fmt sclr_get_color_format(void)
{
	switch (vppif_reg32_read(SCLR_RGB_MODE)) {
	case 0x1:
		return VDO_COL_FMT_RGB_565;
	case 0x3:
		return VDO_COL_FMT_ARGB;
	default:
		break;
	}
	switch (vppif_reg32_read(SCLR_COLFMT_YUV)) {
	case 0:
		return VDO_COL_FMT_YUV422H;
	case 1:
		return VDO_COL_FMT_YUV420;
	case 2:
		return VDO_COL_FMT_YUV444;
	default:
		break;
	}
	return VDO_COL_FMT_YUV444;
}

void sclr_set_media_format(vpp_media_format_t format)
{
	switch (format) {
	case VPP_MEDIA_FMT_MPEG:
		vppif_reg32_write(SCLR_MEDIAFMT_H264, 0x0);
		break;
	case VPP_MEDIA_FMT_H264:
		vppif_reg32_write(SCLR_MEDIAFMT_H264, 0x1);
		break;
	default:
		DBGMSG("*E* check the parameter.\n");
		return;
	}
}

void sclr_set_fb_addr(U32 y_addr, U32 c_addr)
{
	unsigned int line_y, line_c;
	unsigned int offset_y, offset_c;
	unsigned int pre_y, pre_c;

	DBGMSG("y_addr:0x%08x, c_addr:0x%08x\n", y_addr, c_addr);

	offset_y = offset_c = 0;
	line_y = line_c = vppif_reg32_read(SCLR_YBUFWID);
	switch (sclr_get_color_format()) {
	case VDO_COL_FMT_YUV420:
		offset_c /= 2;
		line_c = 0;
		break;
	case VDO_COL_FMT_YUV422H:
		break;
	case VDO_COL_FMT_ARGB:
		offset_y *= 4;
		line_y *= 4;
		break;
	case VDO_COL_FMT_RGB_565:
		offset_y *= 2;
		line_y *= 2;
		break;
	default:
		offset_c *= 2;
		line_c *= 2;
		break;
	}
	pre_y = vppif_reg32_in(REG_SCLR_YSA);
	pre_c = vppif_reg32_in(REG_SCLR_CSA);
	vppif_reg32_out(REG_SCLR_YSA, y_addr + offset_y);
	vppif_reg32_out(REG_SCLR_CSA, c_addr + offset_c);
}

void sclr_get_fb_addr(U32 *y_addr, U32 *c_addr)
{
	*y_addr = vppif_reg32_in(REG_SCLR_YSA);
	*c_addr = vppif_reg32_in(REG_SCLR_CSA);
/*	DBGMSG("y_addr:0x%08x, c_addr:0x%08x\n", *y_addr, *c_addr); */
}

void sclr_set_width(U32 y_pixel, U32 y_buffer)
{
	vppif_reg32_write(SCLR_YPXLWID, y_pixel);
	vppif_reg32_write(SCLR_YBUFWID, y_buffer);
}

void sclr_get_width(U32 *p_y_pixel, U32 *p_y_buffer)
{
	*p_y_pixel = vppif_reg32_read(SCLR_YPXLWID);
	*p_y_buffer = vppif_reg32_read(SCLR_YBUFWID);
}

void sclr_set_crop(U32 h_crop, U32 v_crop)
{
	vppif_reg32_write(SCLR_HCROP, h_crop);
	vppif_reg32_write(SCLR_VCROP, v_crop);
}

void sclr_get_fb_info(U32 *width, U32 *act_width,
				U32 *x_offset, U32 *y_offset)
{
	*width = vppif_reg32_read(SCLR_YBUFWID);
	*act_width = vppif_reg32_read(SCLR_YPXLWID);
	*x_offset = vppif_reg32_read(SCLR_HCROP);
	*y_offset = vppif_reg32_read(SCLR_VCROP);
}

void sclr_set_threshold(U32 value)
{
	vppif_reg32_write(SCLR_FIFO_THR, value);
}

void sclw_set_mif_enable(vpp_flag_t enable)
{
	vppif_reg32_write(SCLW_MIF_ENABLE, enable);
}

void sclw_set_color_format(vdo_color_fmt format)
{
	/* 0-888(4 byte), 1-5515(2 byte), 2-666(4 byte), 3-565(2 byte) */
	switch (format) {
	case VDO_COL_FMT_RGB_666:
		vppif_reg32_write(SCLW_COLFMT_RGB, 1);
		vppif_reg32_write(SCL_IGS_MODE, 2);
		break;
	case VDO_COL_FMT_RGB_565:
		vppif_reg32_write(SCLW_COLFMT_RGB, 1);
		vppif_reg32_write(SCL_IGS_MODE, 3);
		break;
	case VDO_COL_FMT_RGB_1555:
		vppif_reg32_write(SCLW_COLFMT_RGB, 1);
		vppif_reg32_write(SCL_IGS_MODE, 1);
		break;
	case VDO_COL_FMT_ARGB:
		vppif_reg32_write(SCLW_COLFMT_RGB, 1);
		vppif_reg32_write(SCL_IGS_MODE, 0);
		break;
	case VDO_COL_FMT_YUV444:
		vppif_reg32_write(SCLW_COLFMT_RGB, 0);
		vppif_reg32_write(SCLW_COLFMT_YUV, 0);
		vppif_reg32_write(SCL_IGS_MODE, 0);
		break;
	case VDO_COL_FMT_YUV422H:
	case VDO_COL_FMT_YUV420:
		vppif_reg32_write(SCLW_COLFMT_RGB, 0);
		vppif_reg32_write(SCLW_COLFMT_YUV, 1);
		vppif_reg32_write(SCL_IGS_MODE, 0);
		break;
	default:
		DBGMSG("*E* check the parameter.\n");
		return;
	}
}

vdo_color_fmt sclw_get_color_format(void)
{
	if (vppif_reg32_read(SCLW_COLFMT_RGB)) {
		switch (vppif_reg32_read(SCL_IGS_MODE)) {
		case 0:
			return VDO_COL_FMT_ARGB;
		case 1:
			return VDO_COL_FMT_RGB_1555;
		case 2:
			return VDO_COL_FMT_RGB_666;
		case 3:
			return VDO_COL_FMT_RGB_565;
		}
	}

	if (vppif_reg32_read(SCLW_COLFMT_YUV))
		return VDO_COL_FMT_YUV422H;
	return VDO_COL_FMT_YUV444;
}

void sclw_set_alpha(int enable, char data)
{
	vppif_reg32_write(SCL_FIXED_ALPHA_DATA, data);
	vppif_reg32_write(SCL_FIXED_ALPHA_ENABLE, enable);
}

void sclw_set_field_mode(vpp_display_format_t fmt)
{
	vppif_reg32_write(SCLR_TAR_DISP_FMT, fmt);
}

void sclw_set_fb_addr(U32 y_addr, U32 c_addr)
{
	DBGMSG("y_addr:0x%08x, c_addr:0x%08x\n", y_addr, c_addr);
/*	if( (y_addr & 0x3f) || (c_addr & 0x3f) ){
		DPRINT("[SCL] *E* addr should align 64\n");
	} */
	vppif_reg32_out(REG_SCLW_YSA, y_addr);
	vppif_reg32_out(REG_SCLW_CSA, c_addr);
}

void sclw_get_fb_addr(U32 *y_addr, U32 *c_addr)
{
	*y_addr = vppif_reg32_in(REG_SCLW_YSA);
	*c_addr = vppif_reg32_in(REG_SCLW_CSA);
	DBGMSG("y_addr:0x%08x, c_addr:0x%08x\n", *y_addr, *c_addr);
}

void sclw_set_fb_width(U32 width, U32 buf_width)
{
	vppif_reg32_write(SCLW_YPXLWID, width);
	vppif_reg32_write(SCLW_YBUFWID, buf_width);
	if (sclw_get_color_format() == VDO_COL_FMT_YUV444) {
		vppif_reg32_write(SCLW_CPXLWID, width);
		vppif_reg32_write(SCLW_CBUFWID, buf_width * 2);
	} else {
		vppif_reg32_write(SCLW_CPXLWID, width / 2);
		vppif_reg32_write(SCLW_CBUFWID, buf_width);
	}
}

void sclw_get_fb_width(U32 *width, U32 *buf_width)
{
	*width = vppif_reg32_read(SCLW_YPXLWID);
	*buf_width = vppif_reg32_read(SCLW_YBUFWID);
}

void scl_R2_set_mif_enable(int enable)
{
	vppif_reg32_write(SCL_R2_MIF_EN, enable);
}

void scl_R2_set_colorbar(int enable, int wide, int inv)
{
	vppif_reg32_write(SCL_R2_COLOR_EN, enable);
	vppif_reg32_write(SCL_R2_COLOR_WIDE, wide);
	vppif_reg32_write(SCL_R2_COLOR_INV, inv);
}

void scl_R2_set_color_format(vdo_color_fmt colfmt)
{
	if (colfmt >= VDO_COL_FMT_ARGB) {
		vppif_reg32_write(SCL_R2_RGB_MODE,
			(colfmt == VDO_COL_FMT_RGB_565) ? 0x1 : 0x3);
		return;
	}
	vppif_reg32_write(SCL_R2_RGB_MODE, 0x0);
	switch (colfmt) {
	case VDO_COL_FMT_YUV444:
		vppif_reg32_write(SCL_R2_VFMT, 0x2);
		break;
	case VDO_COL_FMT_YUV422H:
		vppif_reg32_write(SCL_R2_VFMT, 0x0);
		break;
	case VDO_COL_FMT_YUV420:
		vppif_reg32_write(SCL_R2_VFMT, 0x1);
		break;
	default:
		DBGMSG("*E* check the parameter.\n");
		return;
	}
}

vdo_color_fmt scl_R2_get_color_format(void)
{
	switch (vppif_reg32_read(SCL_R2_RGB_MODE)) {
	case 0:
		switch (vppif_reg32_read(SCL_R2_VFMT)) {
		case 0:
			return VDO_COL_FMT_YUV422H;
		case 1:
			return VDO_COL_FMT_YUV420;
		case 2:
		default:
			return VDO_COL_FMT_YUV444;
		}
		break;
	case 1:
		return VDO_COL_FMT_RGB_565;
	case 3:
	default:
		break;
	}
	return VDO_COL_FMT_ARGB;
}

void scl_R2_set_csc_mode(vpp_csc_t mode)
{
	vdo_color_fmt src_fmt, dst_fmt;

	src_fmt = scl_R2_get_color_format();
	dst_fmt = sclw_get_color_format();
	mode = vpp_check_csc_mode(mode, src_fmt, dst_fmt, 0);

	if (mode >= VPP_CSC_MAX)
		vppif_reg32_write(SCL_R2_CSC_EN, VPP_FLAG_DISABLE);
	else {
		vppif_reg32_out(REG_SCL_R2_CSC1, vpp_csc_parm[mode][0]);
		vppif_reg32_out(REG_SCL_R2_CSC2, vpp_csc_parm[mode][1]);
		vppif_reg32_out(REG_SCL_R2_CSC3, vpp_csc_parm[mode][2]);
		vppif_reg32_out(REG_SCL_R2_CSC4, vpp_csc_parm[mode][3]);
		vppif_reg32_out(REG_SCL_R2_CSC5, vpp_csc_parm[mode][4]);
		vppif_reg32_out(REG_SCL_R2_CSC6, vpp_csc_parm[mode][5]);
		vppif_reg32_out(REG_SCL_R2_CSC, vpp_csc_parm[mode][6]);
		vppif_reg32_write(SCL_R2_CSC_EN, VPP_FLAG_ENABLE);
	}
}

void scl_R2_set_framebuffer(vdo_framebuf_t *fb)
{
	vppif_reg32_write(SCL_R2_IOFMT,
		(fb->flag & VDO_FLAG_INTERLACE) ? 1 : 0);
	scl_R2_set_color_format(fb->col_fmt);
	vppif_reg32_out(REG_SCLR2_YSA, fb->y_addr);
	vppif_reg32_out(REG_SCLR2_CSA, fb->c_addr);
	vppif_reg32_write(SCL_R2_FBW, fb->fb_w);
	vppif_reg32_write(SCL_R2_LNSIZE, fb->img_w);
	vppif_reg32_write(SCL_R2_HCROP, fb->h_crop);
	vppif_reg32_write(SCL_R2_VCROP, fb->v_crop);
	scl_R2_set_csc_mode(p_scl->fb_p->csc_mode);
}

void scl_ALPHA_set_enable(int enable)
{
	vppif_reg32_write(SCL_ALPHA_COLORKEY_ENABLE, enable);
}

void scl_ALPHA_set_swap(int enable)
{
	/* 0-(alpha,1-alpha),1:(1-alpha,alpha) */
	vppif_reg32_write(SCL_ALPHA_SWAP, enable);
}

void scl_ALPHA_set_src(int mode, int fixed)
{
	/* 0-RMIF1,1-RMIF2,2-Fixed ALPHA */
	vppif_reg32_write(SCL_ALPHA_SRC, mode);
	vppif_reg32_write(SCL_ALPHA_SRC_FIXED, fixed);
}

void scl_ALPHA_set_dst(int mode, int fixed)
{
	/* 0-RMIF1,1-RMIF2,2-Fixed ALPHA */
	vppif_reg32_write(SCL_ALPHA_DST, mode);
	vppif_reg32_write(SCL_ALPHA_DST_FIXED, fixed);
}

void scl_ALPHA_set_color_key(int rmif2, int comp, int mode, int colkey)
{
	/* 0-RMIF1,1-RMIF2 */
	vppif_reg32_write(SCL_ALPHA_COLORKEY_FROM, rmif2);
	/* 0-888,1-777,2-666,3-555 */
	vppif_reg32_write(SCL_ALPHA_COLORKEY_COMP, comp);
	/* (Non-Hit,Hit):0/1-(alpha,alpha),
	2-(alpha,pix1),3-(pix1,alpha),4-(alpha,pix2),
	5-(pix2,alpha),6-(pix1,pix2),7-(pix2,pix1) */
	vppif_reg32_write(SCL_ALPHA_COLORKEY_MODE, mode);
	vppif_reg32_out(REG_ALFA_COLORKEY_RGB, colkey);
}

void scl_set_overlap(vpp_overlap_t *p)
{
#if 0
	DPRINT("alpha src %d,0x%x,dst %d,0x%x\n",
		p->alpha_src_type, p->alpha_src,
		p->alpha_dst_type, p->alpha_dst);
	DPRINT("colkey from %d,comp %d,mode %d,0x%x\n",
		p->color_key_from, p->color_key_comp,
		p->color_key_mode, p->color_key);
#endif
	scl_ALPHA_set_src(p->alpha_src_type, p->alpha_src);
	scl_ALPHA_set_dst(p->alpha_dst_type, p->alpha_dst);
	scl_ALPHA_set_swap(p->alpha_swap);
	scl_ALPHA_set_color_key(p->color_key_from, p->color_key_comp,
		p->color_key_mode, p->color_key);
}

void scl_set_req_num(int ynum, int cnum)
{
	vppif_reg32_write(SCL_R_Y_REQ_NUM, ynum);
	vppif_reg32_write(SCL_R_C_REQ_NUM, cnum);
}

static void scl_set_scale_PP(unsigned int src, unsigned int dst,
					int horizontal)
{
	int gcd;

/*	DBGMSG("scale PP(s %d,d %d,is H %d)\n",src,dst,horizontal); */

	/* gcd = scl_get_gcd(src,dst); */
	gcd = 1;
	src /= gcd;
	dst /= gcd;

	if (horizontal)
		scl_set_H_scale(dst, src);
	else
		scl_set_V_scale(dst, src);
}

void scl_set_scale(unsigned int SRC_W, unsigned int SRC_H,
			unsigned int DST_W, unsigned int DST_H)
{
	int h_scale_up;
	int v_scale_up;

	DBGMSG("[SCL] src(%dx%d),dst(%dx%d)\n", SRC_W, SRC_H, DST_W, DST_H);

	h_scale_up = (DST_W > SRC_W) ? 1 : 0;
	v_scale_up = (DST_H > SRC_H) ? 1 : 0;

	if (((DST_W / SRC_W) >= 32) || ((DST_W / SRC_W) < 1/32))
		DBGMSG("*W* SCL H scale rate invalid\n");

	if (((DST_H / SRC_H) >= 32) || ((DST_H / SRC_H) < 1/32))
		DBGMSG("*W* SCL V scale rate invalid\n");

/*	DBGMSG("scale H %d,V %d\n",h_scale_up,v_scale_up); */

	sclr_set_mif2_enable(VPP_FLAG_DISABLE);
	scl_set_scale_PP(SRC_W, DST_W, 1);
	scl_set_scale_PP(SRC_H, DST_H, 0);
	scl_set_scale_enable(v_scale_up, h_scale_up);

	{
		int rec_h, rec_v;
		int h, v;

		h = rec_h = 0;
		if (SRC_W > DST_W) { /* scale down */
			switch (p_scl->scale_mode) {
			case VPP_SCALE_MODE_ADAPTIVE:
				if ((DST_W * 2) > SRC_W) /* 1 > mode(3) > 1/2 */
					h = 1; /* bilinear mode */
				else
					rec_h = 1; /* recursive mode */
				break;
			case VPP_SCALE_MODE_BILINEAR:
				h = 1; /* bilinear mode */
				break;
			case VPP_SCALE_MODE_RECURSIVE:
				rec_h = 1;	/* recursive mode */
				break;
			default:
				break;
			}
		}

		v = rec_v = 0;
		if (SRC_H > DST_H) { /* scale down */
			switch (p_scl->scale_mode) {
			case VPP_SCALE_MODE_ADAPTIVE:
				if ((DST_H * 2) > SRC_H) /* 1 > mode(3) > 1/2 */
					v = 1; /* bilinear mode */
				else
					rec_v = 1; /* recursive mode */
				break;
			case VPP_SCALE_MODE_BILINEAR:
				v = 1; /* bilinear mode */
				break;
			case VPP_SCALE_MODE_RECURSIVE:
				rec_v = 1;	/* recursive mode */
				break;
			default:
				break;
			}
		}

		if (SRC_W == DST_W)
			rec_h = 1;
		if (SRC_H == DST_H)
			rec_v = 1;
		vppif_reg32_write(SCL_BILINEAR_H, h);
		vppif_reg32_write(SCL_BILINEAR_V, v);
		vppif_reg32_write(SCL_RECURSIVE_H, rec_h);
		vppif_reg32_write(SCL_RECURSIVE_V, rec_v);

		/* vertical bilinear mode */
		if (v) {
			vppif_reg32_write(SCL_VXWIDTH,
				vppif_reg32_read(SCL_VXWIDTH) - 1);
			vppif_reg32_write(SCL_DST_VXWIDTH,
				vppif_reg32_read(SCL_VXWIDTH));
		}
		sclr_set_mif2_enable((v) ? VPP_FLAG_ENABLE : VPP_FLAG_DISABLE);
	}
}

void sclr_set_framebuffer(vdo_framebuf_t *inbuf)
{
	sclr_set_color_format(inbuf->col_fmt);
	sclr_set_crop(inbuf->h_crop, inbuf->v_crop);
	sclr_set_width(inbuf->img_w, inbuf->fb_w);
	sclr_set_fb_addr(inbuf->y_addr, inbuf->c_addr);
	sclr_set_field_mode(vpp_get_fb_field(inbuf));
}

void scl_set_scale_timing(vdo_framebuf_t *s, vdo_framebuf_t *d)
{
	vpp_clock_t timing;
	unsigned int pixel_clock;

	/* scl TG */
	timing.total_pixel_of_line = (d->img_w > s->img_w) ?
					d->img_w : s->img_w;
	timing.total_line_of_frame = (d->img_h > s->img_h) ?
					d->img_h : s->img_h;
	timing.begin_pixel_of_active = 60;
	timing.end_pixel_of_active = timing.total_pixel_of_line + 60;
	timing.total_pixel_of_line = timing.total_pixel_of_line + 120;
	timing.begin_line_of_active = 8;
	timing.end_line_of_active = timing.total_line_of_frame + 8;
	timing.total_line_of_frame = timing.total_line_of_frame + 16;
	timing.line_number_between_VBIS_VBIE = 4;
	timing.line_number_between_PVBI_VBIS = 1;
	pixel_clock = timing.total_pixel_of_line *
		timing.total_line_of_frame * p_scl->fb_p->framerate;
	scl_set_timing(&timing, pixel_clock);
}

void sclw_set_framebuffer(vdo_framebuf_t *fb)
{
	unsigned int yaddr, caddr;
	int y_bpp, c_bpp;

	vpp_get_colfmt_bpp(fb->col_fmt, &y_bpp, &c_bpp);
	yaddr = fb->y_addr + ((fb->fb_w * fb->v_crop + fb->h_crop) * y_bpp / 8);
	caddr = (c_bpp) ? (fb->c_addr + (((fb->fb_w * fb->v_crop +
		fb->h_crop) / 2) * 2 * c_bpp / 8)) : 0;
	sclw_set_fb_addr(yaddr, caddr);
	sclw_set_color_format(fb->col_fmt);
	sclw_set_fb_width(fb->img_w, fb->fb_w);
	sclw_set_field_mode(vpp_get_fb_field(fb));
	scl_set_csc_mode(p_scl->fb_p->csc_mode);
}

void scl_init(void *base)
{
	scl_mod_t *mod_p;
	vpp_fb_base_t *fb_p;

	mod_p = (scl_mod_t *) base;
	fb_p = mod_p->fb_p;

	scl_set_reg_level(VPP_REG_LEVEL_1);
	scl_set_tg_enable(VPP_FLAG_DISABLE);
	scl_set_enable(VPP_FLAG_DISABLE);
	scl_set_int_enable(VPP_FLAG_DISABLE, VPP_INT_ALL);
	sclr_set_mif_enable(VPP_FLAG_DISABLE);
	sclr_set_mif2_enable(VPP_FLAG_DISABLE);
	sclr_set_colorbar(VPP_FLAG_DISABLE, 0, 0);

	scl_set_int_enable(VPP_FLAG_ENABLE, mod_p->int_catch);
	scl_set_watchdog(fb_p->wait_ready);
	scl_set_csc_mode(fb_p->csc_mode);
	sclr_set_media_format(fb_p->media_fmt);
	sclr_set_threshold(0xf);

	/* filter default value */
	vppif_reg32_write(SCL_1ST_LAYER_BOUNDARY, 48);
	vppif_reg32_write(SCL_2ND_LAYER_BOUNDARY, 16);

	vppif_reg32_write(SCL_FIELD_FILTER_Y_THD, 8);
	vppif_reg32_write(SCL_FIELD_FILTER_C_THD, 8);
	vppif_reg32_write(SCL_FIELD_FILTER_CONDITION, 0);

	vppif_reg32_write(SCL_FRAME_FILTER_RGB, 0);
	vppif_reg32_write(SCL_FRAME_FILTER_SAMPLER, 14);
	vppif_reg32_write(SCL_FR_FILTER_SCENE_CHG_THD, 32);

	scl_set_reg_update(VPP_FLAG_ENABLE);
	scl_set_tg_enable(VPP_FLAG_DISABLE);
}

void sclw_init(void *base)
{
	sclw_set_mif_enable(VPP_FLAG_DISABLE);
	sclw_set_fb_width(VPP_HD_DISP_RESX, VPP_HD_MAX_RESX);
/*	vppif_reg32_write(SCL_SCLDW_METHOD,0x1); */	/* drop line enable */
}

#ifdef __KERNEL__
/* static struct work_struct scl_proc_scale_wq; */
DECLARE_WAIT_QUEUE_HEAD(scl_proc_scale_event);
static void scl_proc_scale_complete_work(struct work_struct *work)
#else
static void scl_proc_scale_complete_work(int arg)
#endif
{
/*	DPRINT("[SCL] scl_proc_scale_complete_work\n"); */
	p_scl->scale_complete = 1;
#ifdef __KERNEL__
	wake_up_interruptible(&scl_proc_scale_event);
#endif
#if 0 /* avoid mutex in irq */
	vpp_mod_set_clock(VPP_MOD_SCL, VPP_FLAG_DISABLE, 0);
#endif
}

struct timer_list scl_scale_timer;
int scl_proc_scale_complete(void *arg)
{
	del_timer(&scl_scale_timer);

/*	DPRINT("[SCL] scl_proc_scale_complete\n"); */
	if (vppif_reg32_read(SCL_INTSTS_TGERR)) {
		DPRINT("[SCL] scale TG err 0x%x,0x%x\n",
			vppif_reg32_in(REG_SCL_TG_STS),
			vppif_reg32_in(REG_SCLW_FF_CTL));
		vppif_reg32_out(REG_SCL_TG_STS+0x0, BIT0);
		vppif_reg32_out(REG_SCLW_FF_CTL, 0x10101);
	}
	scl_set_tg_enable(VPP_FLAG_DISABLE);
	vppm_set_int_enable(VPP_FLAG_DISABLE, SCL_COMPLETE_INT);
	sclw_set_mif_enable(VPP_FLAG_DISABLE);
	sclr_set_mif_enable(VPP_FLAG_DISABLE);
	sclr_set_mif2_enable(VPP_FLAG_DISABLE);
	scl_set_enable(VPP_FLAG_DISABLE);

/* #ifdef __KERNEL__ */
#if 0
	INIT_WORK(&scl_proc_scale_wq, scl_proc_scale_complete_work);
	schedule_work(&scl_proc_scale_wq);
#else
	scl_proc_scale_complete_work(0);
#endif
	return 0;
}

void scl_scale_timeout(int arg)
{
	DBG_ERR("scale timeout\n");
#if 0
	scl_reg_dump();
	g_vpp.dbg_msg_level = VPP_DBGLVL_STREAM;
	g_vpp.dbg_cnt = 1000;
#endif
	scl_proc_scale_complete(0);
}

void scl_set_scale_timer(int ms)
{
	if (scl_scale_timer.function)
		del_timer(&scl_scale_timer);
	init_timer(&scl_scale_timer);
	scl_scale_timer.function = (void *) scl_scale_timeout;
	scl_scale_timer.expires = jiffies + msecs_to_jiffies(ms);
	add_timer(&scl_scale_timer);
}

int scl_proc_scale_finish(void)
{
#ifdef __KERNEL__
	int ret;

/*	DPRINT("[SCL] scl_proc_scale_finish\n"); */
	ret = wait_event_interruptible_timeout(scl_proc_scale_event,
				(p_scl->scale_complete != 0), 3 * HZ);
	if (ret == 0) { /* timeout */
		DPRINT("[SCL] *E* wait scale timeout\n");
		ret = -1;
	} else {
		ret = 0;
	}
#endif
#if 1 /* avoid mutex in irq */
	vpp_mod_set_clock(VPP_MOD_SCL, VPP_FLAG_DISABLE, 0);
#endif
	return ret;
}

void scl_check_framebuf(vdo_framebuf_t *s,
				vdo_framebuf_t *in, vdo_framebuf_t *out)
{
	if (s) {
		if (s->img_w > WMT_SCL_H_DIV_MAX)
			DBG_ERR("src w %d over %d\n",
					s->img_w, WMT_SCL_H_DIV_MAX);
		if (s->img_h > WMT_SCL_V_DIV_MAX)
			DBG_ERR("src h %d over %d\n",
					s->img_h, WMT_SCL_V_DIV_MAX);
		if (s->col_fmt >= VDO_COL_FMT_ARGB) {
			if (s->y_addr % 4)
				DBG_ERR("src addr 0x%x not align 4\n",
						s->y_addr);
		}
		if (s->fb_w > WMT_SCL_H_DIV_MAX)
			DBG_ERR("src fb w %d over %d\n",
				s->fb_w, WMT_SCL_H_DIV_MAX);
		if (s->y_addr % 64)
			DBG_ERR("src fb addr 0x%x no align 64\n", s->y_addr);
	} else {
		DBG_ERR("src null\n");
	}

	if (in) {
		if (in->img_w > WMT_SCL_H_DIV_MAX)
			DBG_ERR("in w %d over %d\n",
					in->img_w, WMT_SCL_H_DIV_MAX);
		if (in->img_h > WMT_SCL_V_DIV_MAX)
			DBG_ERR("in h %d over %d\n",
					in->img_h, WMT_SCL_V_DIV_MAX);
		if (in->col_fmt >= VDO_COL_FMT_ARGB) {
			if (in->y_addr % 4)
				DBG_ERR("in addr 0x%x not align 4\n",
						in->y_addr);
		}
		if (in->fb_w > WMT_SCL_H_DIV_MAX)
			DBG_ERR("in fb w %d over %d\n",
				in->fb_w, WMT_SCL_H_DIV_MAX);
		if (in->y_addr % 64)
			DBG_ERR("in fb addr 0x%x no align 64\n", in->y_addr);
	}

	if (out) {
		if (s && (s->img_w == out->img_w)) {
			if (out->img_w > WMT_SCL_H_DIV_MAX)
				DBG_ERR("out w %d over %d\n", out->img_w,
					WMT_SCL_H_DIV_MAX);
		} else {
			if (out->img_w > WMT_SCL_SCALE_DST_H_MAX)
				DBG_ERR("out w %d over %d\n", out->img_w,
					WMT_SCL_SCALE_DST_H_MAX);

		}
		if (out->col_fmt >= VDO_COL_FMT_ARGB) {
			if (out->y_addr % 4)
				DBG_ERR("out addr 0x%x not align 4\n",
						out->y_addr);
		}
		if (out->fb_w > WMT_SCL_H_DIV_MAX)
			DBG_ERR("out fb w %d over %d\n",
				out->fb_w, WMT_SCL_H_DIV_MAX);
		if (out->y_addr % 64)
			DBG_ERR("out fb addr 0x%x no align 64\n", out->y_addr);
	} else {
		DBG_ERR("out null\n");
	}
}

#define CONFIG_VPP_CHECK_SCL_STATUS
int scl_set_scale_overlap(vdo_framebuf_t *s,
				vdo_framebuf_t *in, vdo_framebuf_t *out)
{
	int ret = 0;

	if (vpp_check_dbg_level(VPP_DBGLVL_SCALE)) {
		scl_check_framebuf(s, in, out);

		if (s)
			vpp_show_framebuf("src1", s);
		if (in)
			vpp_show_framebuf("src2", in);
		if (out)
			vpp_show_framebuf("dst", out);
	}

	if (p_scl->scale_sync)
		vpp_mod_set_clock(VPP_MOD_SCL, VPP_FLAG_ENABLE, 0);

	scl_set_timing_master(VPP_MOD_SCL);

	if (s) {
		p_scl->fb_p->fb = *s;
		sclr_set_framebuffer(s);
		sclr_set_mif_enable(VPP_FLAG_ENABLE);
	} else {
		DPRINT("[SCL] *E* no source\n");
		return -1;
	}

	if (in && (in->y_addr == 0))
		in = 0;

	scl_ALPHA_set_enable((in) ? VPP_FLAG_ENABLE : VPP_FLAG_DISABLE);
	scl_R2_set_mif_enable((in) ? VPP_FLAG_ENABLE : VPP_FLAG_DISABLE);
	if (in)
		scl_R2_set_framebuffer(in);
	if (out) {
		p_sclw->fb_p->fb = *out;
		sclw_set_framebuffer(out);
	} else {
		DPRINT("[SCL] *E* no dest\n");
		return -1;
	}

	scl_set_scale(s->img_w, s->img_h, out->img_w, out->img_h);
	scl_set_scale_timing(s, out);

	/* scale process */
	scl_set_enable(VPP_FLAG_ENABLE);
	vppif_reg32_write(SCL_ONESHOT_ENABLE, 1);
	sclw_set_mif_enable(VPP_FLAG_ENABLE);
	scl_set_tg_enable(VPP_FLAG_ENABLE);
#ifdef CONFIG_VPP_CHECK_SCL_STATUS
	vppif_reg32_out(REG_SCL_TG_STS+0x0, BIT0);
	vppif_reg32_out(REG_SCLW_FF_CTL, 0x10101);
#endif
	p_scl->scale_complete = 0;
	vppm_set_int_enable(VPP_FLAG_ENABLE, SCL_COMPLETE_INT);

#if 0   /* for debug scale */
	scl_reg_dump();
#endif

	if (p_scl->scale_sync) {
		ret = vpp_irqproc_work(SCL_COMPLETE_INT,
			(void *)scl_proc_scale_complete, 0, 100, 1);
		scl_proc_scale_finish();
	} else {
		vpp_irqproc_work(SCL_COMPLETE_INT,
			(void *)scl_proc_scale_complete, 0, 0, 1);
		scl_set_scale_timer(100);
	}
	return ret;
}

int scl_proc_scale(vdo_framebuf_t *src_fb, vdo_framebuf_t *dst_fb)
{
	int ret = 0;

	if (dst_fb->col_fmt == VDO_COL_FMT_YUV420) {
		int size;
		unsigned int buf;
		vdo_framebuf_t dfb;

		dfb = *dst_fb;	/* backup dst fb */

		/* alloc memory */
		size = dst_fb->img_w * dst_fb->img_h + 64;
		buf = (unsigned int) kmalloc(size, GFP_KERNEL);
		if (!buf) {
			DPRINT("[SCL] *E* malloc fail %d\n", size);
			return -1;
		}

		if (buf % 64)
			buf = buf + (64 - (buf % 64));

		/* scale for Y */
		dst_fb->c_addr = buf;
		ret = scl_set_scale_overlap(src_fb, 0, dst_fb);
		if (ret == 0) {
			/* V 1/2 scale for C */
			dst_fb->y_addr = buf;
			dst_fb->c_addr = dfb.c_addr;
			dst_fb->img_h = dfb.img_h / 2;
			ret = scl_set_scale_overlap(src_fb, 0, dst_fb);
		}
		kfree((void *)buf);
		*dst_fb = dfb;	/* restore dst fb */
	} else {
		ret = scl_set_scale_overlap(src_fb, 0, dst_fb);
	}
	return ret;
}

#ifdef CONFIG_PM
static unsigned int *scl_pm_bk2;
static unsigned int scl_pm_enable, scl_pm_tg;
static unsigned int scl_pm_r_mif1, scl_pm_r_mif2, scl_pm_w_mif;
void scl_suspend(int sts)
{
	switch (sts) {
	case 0:	/* disable module */
		vpp_mod_set_clock(VPP_MOD_SCL, VPP_FLAG_ENABLE, 1);
		scl_pm_enable = vppif_reg32_read(SCL_ALU_ENABLE);
		vppif_reg32_write(SCL_ALU_ENABLE, 0);
		scl_pm_r_mif1 = vppif_reg32_read(SCLR_MIF_ENABLE);
		scl_pm_r_mif2 = vppif_reg32_read(SCL_R2_MIF_EN);
		vppif_reg32_write(SCL_R2_MIF_EN, 0);
		vppif_reg32_write(SCLR_MIF_ENABLE, 0);
		scl_pm_w_mif = vppif_reg32_read(SCLW_MIF_ENABLE);
		vppif_reg32_write(SCLW_MIF_ENABLE, 0);
		break;
	case 1: /* disable tg */
		scl_pm_tg = vppif_reg32_read(SCL_TG_ENABLE);
		vppif_reg32_write(SCL_TG_ENABLE, 0);
		break;
	case 2:	/* backup register */
		p_scl->reg_bk = vpp_backup_reg(REG_SCL_BASE1_BEGIN,
			(REG_SCL_BASE1_END - REG_SCL_BASE1_BEGIN));
		scl_pm_bk2 = vpp_backup_reg(REG_SCL_BASE2_BEGIN,
			(REG_SCL_BASE2_END - REG_SCL_BASE2_BEGIN));
		break;
	default:
		break;
	}
}

void scl_resume(int sts)
{
	switch (sts) {
	case 0:	/* restore register */
		vpp_restore_reg(REG_SCL_BASE1_BEGIN,
			(REG_SCL_BASE1_END - REG_SCL_BASE1_BEGIN),
			p_scl->reg_bk);
		vpp_restore_reg(REG_SCL_BASE2_BEGIN,
			(REG_SCL_BASE2_END - REG_SCL_BASE2_BEGIN), scl_pm_bk2);
		p_scl->reg_bk = 0;
		scl_pm_bk2 = 0;
		break;
	case 1:	/* enable module */
		vppif_reg32_write(SCLW_MIF_ENABLE, scl_pm_w_mif);
		vppif_reg32_write(SCLR_MIF_ENABLE, scl_pm_r_mif1);
		vppif_reg32_write(SCL_R2_MIF_EN, scl_pm_r_mif2);
		vppif_reg32_write(SCL_ALU_ENABLE, scl_pm_enable);
		break;
	case 2: /* enable tg */
		vppif_reg32_write(SCL_TG_ENABLE, scl_pm_tg);
		vpp_mod_set_clock(VPP_MOD_SCL, VPP_FLAG_DISABLE, 1);
		break;
	default:
		break;
	}
}
#else
#define scl_suspend NULL
#define scl_resume NULL
#endif

int scl_mod_init(void)
{
	vpp_fb_base_t *mod_fb_p;
	vdo_framebuf_t *fb_p;

	/* -------------------- SCL module -------------------- */
	{
	scl_mod_t *scl_mod_p;

	scl_mod_p = (scl_mod_t *) vpp_mod_register(VPP_MOD_SCL,
				sizeof(scl_mod_t), VPP_MOD_FLAG_FRAMEBUF);
	if (!scl_mod_p) {
		DPRINT("*E* SCL module register fail\n");
		return -1;
	}

	/* module member variable */
	scl_mod_p->int_catch = VPP_INT_NULL;
	scl_mod_p->scale_mode = VPP_SCALE_MODE_ADAPTIVE;
	scl_mod_p->pm = DEV_SCL444U;
	scl_mod_p->filter_mode = VPP_FILTER_SCALE;

	/* module member function */
	scl_mod_p->init = scl_init;
	scl_mod_p->set_enable = scl_set_enable;
	scl_mod_p->set_colorbar = sclr_set_colorbar;
	scl_mod_p->dump_reg = scl_reg_dump;
	scl_mod_p->get_sts = scl_get_int_status;
	scl_mod_p->clr_sts = scl_clean_int_status;
	scl_mod_p->scale = scl_proc_scale;
	scl_mod_p->scale_finish = scl_proc_scale_finish;
	scl_mod_p->suspend = scl_suspend;
	scl_mod_p->resume = scl_resume;

	/* module frame buffer variable */
	mod_fb_p = scl_mod_p->fb_p;
	fb_p = &mod_fb_p->fb;

	fb_p->y_addr = 0;
	fb_p->c_addr = 0;
	fb_p->col_fmt = VDO_COL_FMT_YUV422H;
	fb_p->img_w = VPP_HD_DISP_RESX;
	fb_p->img_h = VPP_HD_DISP_RESY;
	fb_p->fb_w = VPP_HD_MAX_RESX;
	fb_p->fb_h = VPP_HD_MAX_RESY;
	fb_p->h_crop = 0;
	fb_p->v_crop = 0;

	/* module frame buffer member function */
	mod_fb_p->csc_mode = VPP_CSC_RGB2YUV_SDTV_0_255;
	mod_fb_p->set_framebuf = sclr_set_framebuffer;
	mod_fb_p->set_addr = sclr_set_fb_addr;
	mod_fb_p->get_addr = sclr_get_fb_addr;
	mod_fb_p->set_csc = scl_set_csc_mode;
	mod_fb_p->framerate = 0x7fffffff;
	mod_fb_p->wait_ready = 0xffffffff;
	mod_fb_p->capability = BIT(VDO_COL_FMT_YUV420)
		| BIT(VDO_COL_FMT_YUV422H) | BIT(VDO_COL_FMT_YUV444)
		| BIT(VDO_COL_FMT_ARGB) | BIT(VDO_COL_FMT_RGB_565)
		| VPP_FB_FLAG_CSC | VPP_FB_FLAG_FIELD;
	p_scl = scl_mod_p;
	p_scl->scale_complete = 1;
	p_scl->scale_sync = 1;
	}

	/* -------------------- SCLW module -------------------- */
	{
	sclw_mod_t *sclw_mod_p;

	sclw_mod_p = (sclw_mod_t *) vpp_mod_register(VPP_MOD_SCLW,
				sizeof(sclw_mod_t), VPP_MOD_FLAG_FRAMEBUF);
	if (!sclw_mod_p) {
		DPRINT("*E* SCLW module register fail\n");
		return -1;
	}

	/* module member variable */
	sclw_mod_p->int_catch = VPP_INT_NULL;

	/* module member function */
	sclw_mod_p->init = sclw_init;
	sclw_mod_p->set_enable = sclw_set_mif_enable;

	/* module frame buffer */
	mod_fb_p = sclw_mod_p->fb_p;
	fb_p = &mod_fb_p->fb;

	fb_p->y_addr = 0;
	fb_p->c_addr = 0;
	fb_p->col_fmt = VDO_COL_FMT_YUV422H;
	fb_p->img_w = VPP_HD_DISP_RESX;
	fb_p->img_h = VPP_HD_DISP_RESY;
	fb_p->fb_w = VPP_HD_MAX_RESX;
	fb_p->fb_h = VPP_HD_MAX_RESY;
	fb_p->h_crop = 0;
	fb_p->v_crop = 0;

	/* module frame buffer member function */
	mod_fb_p->csc_mode = VPP_CSC_RGB2YUV_SDTV_0_255;
	mod_fb_p->set_framebuf = sclw_set_framebuffer;
	mod_fb_p->set_addr = sclw_set_fb_addr;
	mod_fb_p->get_addr = sclw_get_fb_addr;
	mod_fb_p->set_csc = scl_set_csc_mode;
	mod_fb_p->wait_ready = 0xffffffff;
	mod_fb_p->capability = BIT(VDO_COL_FMT_YUV422H)
		| BIT(VDO_COL_FMT_YUV444) | BIT(VDO_COL_FMT_ARGB)
		| BIT(VDO_COL_FMT_RGB_565) | VPP_FB_FLAG_CSC
		| BIT(VDO_COL_FMT_YUV420);
	p_sclw = sclw_mod_p;
	}
	return 0;
}
module_init(scl_mod_init);
#endif /* WMT_FTBLK_SCL */
