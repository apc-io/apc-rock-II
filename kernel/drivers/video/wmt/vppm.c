/*++
 * linux/drivers/video/wmt/vppm.c
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

#define VPPM_C
/* #define DEBUG */
/* #define DEBUG_DETAIL */

#include "vppm.h"

void vppm_set_int_enable(vpp_flag_t enable, vpp_int_t int_bit)
{
#ifdef WMT_FTBLK_SCL
	if (int_bit & VPP_INT_SCL_VBIE)
		vppif_reg32_write(VPP_SCL_INTEN_VBIE, enable);
	if (int_bit & VPP_INT_SCL_VBIS)
		vppif_reg32_write(VPP_SCL_INTEN_VBIS, enable);
	if (int_bit & VPP_INT_SCL_PVBI)
		vppif_reg32_write(VPP_SCL_INTEN_PVBI, enable);
#endif
#ifdef WMT_FTBLK_GOVRH
	if (int_bit & VPP_INT_GOVRH_VBIE)
		vppif_reg32_write(VPP_GOVRH_INTEN_VBIE, enable);
	if (int_bit & VPP_INT_GOVRH_VBIS)
		vppif_reg32_write(VPP_GOVRH_INTEN_VBIS, enable);
	if (int_bit & VPP_INT_GOVRH_PVBI)
		vppif_reg32_write(VPP_GOVRH_INTEN_PVBI, enable);
#endif
#ifdef WMT_FTBLK_GOVRH
	if (int_bit & VPP_INT_GOVRH2_VBIE)
		vppif_reg32_write(VPP_GOVRH2_INTEN_VBIE, enable);
	if (int_bit & VPP_INT_GOVRH2_VBIS)
		vppif_reg32_write(VPP_GOVRH2_INTEN_VBIS, enable);
	if (int_bit & VPP_INT_GOVRH2_PVBI)
		vppif_reg32_write(VPP_GOVRH2_INTEN_PVBI, enable);
#endif
}

int vppm_get_int_enable(vpp_int_t int_bit)
{
	int ret = 0;

#ifdef WMT_FTBLK_SCL
	if (int_bit & VPP_INT_SCL_VBIE)
		ret = vppif_reg32_read(VPP_SCL_INTEN_VBIE);
	if (int_bit & VPP_INT_SCL_VBIS)
		ret = vppif_reg32_read(VPP_SCL_INTEN_VBIS);
	if (int_bit & VPP_INT_SCL_PVBI)
		ret = vppif_reg32_read(VPP_SCL_INTEN_PVBI);
#endif
#ifdef WMT_FTBLK_GOVRH
	if (int_bit & VPP_INT_GOVRH_VBIE)
		ret = vppif_reg32_read(VPP_GOVRH_INTEN_VBIE);
	if (int_bit & VPP_INT_GOVRH_VBIS)
		ret = vppif_reg32_read(VPP_GOVRH_INTEN_VBIS);
	if (int_bit & VPP_INT_GOVRH_PVBI)
		ret = vppif_reg32_read(VPP_GOVRH_INTEN_PVBI);
#endif
#ifdef WMT_FTBLK_GOVRH
	if (int_bit & VPP_INT_GOVRH2_VBIE)
		ret = vppif_reg32_read(VPP_GOVRH2_INTEN_VBIE);
	if (int_bit & VPP_INT_GOVRH2_VBIS)
		ret = vppif_reg32_read(VPP_GOVRH2_INTEN_VBIS);
	if (int_bit & VPP_INT_GOVRH2_PVBI)
		ret = vppif_reg32_read(VPP_GOVRH2_INTEN_PVBI);
#endif
	return ret;
}

vpp_int_t vppm_get_int_status(void)
{
	unsigned int int_enable_reg;
	unsigned int int_sts_reg;
	vpp_int_t int_sts = 0;

	int_enable_reg = vppif_reg32_in(REG_VPP_INTEN);
	int_sts_reg = vppif_reg32_in(REG_VPP_INTSTS);

#ifdef WMT_FTBLK_SCL
	if ((int_enable_reg & BIT18) && (int_sts_reg & BIT18))
		int_sts |= VPP_INT_SCL_VBIE;
	if ((int_enable_reg & BIT17) && (int_sts_reg & BIT17))
		int_sts |= VPP_INT_SCL_VBIS;
	if ((int_enable_reg & BIT16) && (int_sts_reg & BIT16))
		int_sts |= VPP_INT_SCL_PVBI;
#endif
#ifdef WMT_FTBLK_GOVRH
	if ((int_enable_reg & BIT10) && (int_sts_reg & BIT10))
		int_sts |= VPP_INT_GOVRH_VBIE;
	if ((int_enable_reg & BIT9) && (int_sts_reg & BIT9))
		int_sts |= VPP_INT_GOVRH_VBIS;
	if ((int_enable_reg & BIT8) && (int_sts_reg & BIT8))
		int_sts |= VPP_INT_GOVRH_PVBI;
#endif
#ifdef WMT_FTBLK_GOVRH
	if ((int_enable_reg & BIT14) && (int_sts_reg & BIT14))
		int_sts |= VPP_INT_GOVRH2_VBIE;
	if ((int_enable_reg & BIT13) && (int_sts_reg & BIT13))
		int_sts |= VPP_INT_GOVRH2_VBIS;
	if ((int_enable_reg & BIT12) && (int_sts_reg & BIT12))
		int_sts |= VPP_INT_GOVRH2_PVBI;
#endif
	return int_sts;
}

void vppm_clean_int_status(vpp_int_t int_sts)
{
#ifdef WMT_FTBLK_SCL
	if (int_sts & VPP_INT_SCL_VBIE)
		vppif_reg8_out(REG_VPP_INTSTS + 0x2, 0x4);
	if (int_sts & VPP_INT_SCL_VBIS)
		vppif_reg8_out(REG_VPP_INTSTS + 0x2, 0x2);
	if (int_sts & VPP_INT_SCL_PVBI)
		vppif_reg8_out(REG_VPP_INTSTS + 0x2, 0x1);
#endif
#ifdef WMT_FTBLK_GOVRH
	if (int_sts & VPP_INT_GOVRH_VBIE)
		vppif_reg8_out(REG_VPP_INTSTS + 0x1, 0x4);
	if (int_sts & VPP_INT_GOVRH_VBIS)
		vppif_reg8_out(REG_VPP_INTSTS + 0x1, 0x2);
	if (int_sts & VPP_INT_GOVRH_PVBI)
		vppif_reg8_out(REG_VPP_INTSTS + 0x1, 0x1);
#endif
#ifdef WMT_FTBLK_GOVRH
	if (int_sts & VPP_INT_GOVRH2_VBIE)
		vppif_reg32_out(REG_VPP_INTSTS, 0x00004000);
	if (int_sts & VPP_INT_GOVRH2_VBIS)
		vppif_reg32_out(REG_VPP_INTSTS, 0x00002000);
	if (int_sts & VPP_INT_GOVRH2_PVBI)
		vppif_reg32_out(REG_VPP_INTSTS, 0x00001000);
#endif
}

void vppm_set_module_reset(vpp_mod_t mod)
{
	unsigned int value1 = 0x00, value2 = 0x00;
	unsigned int value3 = 0x00;

#ifdef WMT_FTBLK_SCL
	if (mod == VPP_MOD_SCL)
		value1 |= BIT0;
#endif
#ifdef WMT_FTBLK_GOVRH
	if (mod == VPP_MOD_GOVRH)
		value2 |= (BIT0 | BIT8 | BIT9);
#endif
	vppif_reg32_out(REG_VPP_SWRST1_SEL, ~value1);
	vppif_reg32_out(REG_VPP_SWRST1_SEL, 0x1010101);
	vppif_reg32_out(REG_VPP_SWRST2_SEL, ~value2);
	vppif_reg32_out(REG_VPP_SWRST2_SEL, 0x1011311);
	vppif_reg32_out(REG_VPP_SWRST3_SEL, ~value3);
	vppif_reg32_out(REG_VPP_SWRST3_SEL, 0x10101);
}

void vppm_reg_dump(void)
{
	unsigned int reg1, reg2;

	DPRINT("========== VPPM register dump ==========\n");
	vpp_reg_dump(REG_VPP_BEGIN, REG_VPP_END - REG_VPP_BEGIN);

	DPRINT("---------- VPP Interrupt ----------\n");
	reg1 = vppif_reg32_in(REG_VPP_INTSTS);
	reg2 = vppif_reg32_in(REG_VPP_INTEN);
	DPRINT("GOVRH PVBI(En %d,%d),VBIS(En %d,%d),VBIE(En %d,%d)\n",
		vppif_reg32_read(VPP_GOVRH_INTEN_PVBI),
		vppif_reg32_read(VPP_GOVRH_INTSTS_PVBI),
		vppif_reg32_read(VPP_GOVRH_INTEN_VBIS),
		vppif_reg32_read(VPP_GOVRH_INTSTS_VBIS),
		vppif_reg32_read(VPP_GOVRH_INTEN_VBIE),
		vppif_reg32_read(VPP_GOVRH_INTSTS_VBIE));

	DPRINT("GOVRH2 PVBI(En %d,%d),VBIS(En %d,%d),VBIE(En %d,%d)\n",
		vppif_reg32_read(VPP_GOVRH2_INTEN_PVBI),
		vppif_reg32_read(VPP_GOVRH2_INTSTS_PVBI),
		vppif_reg32_read(VPP_GOVRH2_INTEN_VBIS),
		vppif_reg32_read(VPP_GOVRH2_INTSTS_VBIS),
		vppif_reg32_read(VPP_GOVRH2_INTEN_VBIE),
		vppif_reg32_read(VPP_GOVRH2_INTSTS_VBIE));

	DPRINT("SCL PVBI(En %d,%d),VBIS(En %d,%d),VBIE(En %d,%d)\n",
		vppif_reg32_read(VPP_SCL_INTEN_PVBI),
		vppif_reg32_read(VPP_SCL_INTSTS_PVBI),
		vppif_reg32_read(VPP_SCL_INTEN_VBIS),
		vppif_reg32_read(VPP_SCL_INTSTS_VBIS),
		vppif_reg32_read(VPP_SCL_INTEN_VBIE),
		vppif_reg32_read(VPP_SCL_INTSTS_VBIE));
}

#ifdef CONFIG_PM
void vppm_suspend(int sts)
{
	switch (sts) {
	case 0:	/* disable module */
		break;
	case 1: /* disable tg */
		break;
	case 2:	/* backup register */
		p_vppm->reg_bk = vpp_backup_reg(REG_VPP_BEGIN,
			(REG_VPP_END - REG_VPP_BEGIN));
		break;
	default:
		break;
	}
}

void vppm_resume(int sts)
{
	switch (sts) {
	case 0:	/* restore register */
		vpp_restore_reg(REG_VPP_BEGIN, (REG_VPP_END - REG_VPP_BEGIN),
			p_vppm->reg_bk);
		p_vppm->reg_bk = 0;
		break;
	case 1:	/* enable module */
		break;
	case 2: /* enable tg */
		break;
	default:
		break;
	}
}
#else
#define vppm_suspend NULL
#define vppm_resume NULL
#endif

void vppm_init(void *base)
{
	vppm_mod_t *mod_p;

	mod_p = (vppm_mod_t *) base;

	vppm_set_module_reset(0);
	vppm_set_int_enable(VPP_FLAG_ENABLE, mod_p->int_catch);
	vppm_clean_int_status(~0);
}

int vppm_mod_init(void)
{
	vppm_mod_t *mod_p;

	mod_p = (vppm_mod_t *) vpp_mod_register(VPP_MOD_VPPM,
		sizeof(vppm_mod_t), 0);
	if (!mod_p) {
		DPRINT("*E* VPP module register fail\n");
		return -1;
	}

	/* module member variable */
	mod_p->int_catch = VPP_INT_NULL;

	/* module member function */
	mod_p->init = vppm_init;
	mod_p->dump_reg = vppm_reg_dump;
	mod_p->get_sts = vppm_get_int_status;
	mod_p->clr_sts = vppm_clean_int_status;
	mod_p->suspend = vppm_suspend;
	mod_p->resume = vppm_resume;

	p_vppm = mod_p;
	return 0;
}
module_init(vppm_mod_init);
