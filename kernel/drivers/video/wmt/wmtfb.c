/*++
 * linux/drivers/video/wmt/wmtfb.c
 * WonderMedia frame buffer driver
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

#define WMTFB_C
/* #define DEBUG */
/* #define DEBUG_DETAIL */

/*----------------------- DEPENDENCE -----------------------------------------*/
#include <linux/init.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/errno.h>
#include <asm/uaccess.h>
#include <linux/kdev_t.h>
#include <linux/cdev.h>
#include <linux/fb.h>
#include <linux/dma-mapping.h>
#include <asm/page.h>
#include <linux/mm.h>
#include <linux/proc_fs.h>

#include "vpp.h"

/*----------------------- PRIVATE MACRO --------------------------------------*/

/*----------------------- PRIVATE CONSTANTS ----------------------------------*/
/* #define FBUT_XXXX    1     *//*Example*/

/*----------------------- PRIVATE TYPE  --------------------------------------*/
/* typedef  xxxx fbut_xxx_t; *//*Example*/

/*----------------------- INTERNAL PRIVATE VARIABLES - -----------------------*/
/* int  fbut_xxx;        *//*Example*/

static struct fb_fix_screeninfo wmtfb_fix = {
	.id             = "wmtfb",
	.smem_start     = 0,
	.smem_len       = 0,
	.type           = FB_TYPE_PACKED_PIXELS,
	.type_aux       = 0,
	.visual         = FB_VISUAL_TRUECOLOR,
	.xpanstep       = 1,
	.ypanstep       = 1,
	.ywrapstep      = 1,
	.line_length    = 0,
	.mmio_start     = 0xD8050000,
	.mmio_len       = 0x0700,
	.accel          = FB_ACCEL_NONE
};

static struct fb_var_screeninfo wmtfb_var = {
	.xres           = VPP_HD_DISP_RESX,
	.yres           = VPP_HD_DISP_RESY,
	.xres_virtual   = VPP_HD_DISP_RESX,
	.yres_virtual   = VPP_HD_DISP_RESY,
#if 0
	.bits_per_pixel = 32,
	.red            = {16, 8, 0},
	.green          = {8, 8, 0},
	.blue           = {0, 8, 0},
	.transp         = {24, 8, 0},
#else
	.bits_per_pixel = 16,
	.red            = {11, 5, 0},
	.green          = {5, 6, 0},
	.blue           = {0, 5, 0},
	.transp         = {0, 0, 0},
#endif
	.activate       = FB_ACTIVATE_NOW | FB_ACTIVATE_FORCE,
	.height         = -1,
	.width          = -1,
	.pixclock       = (VPP_HD_DISP_RESX*VPP_HD_DISP_RESY*VPP_HD_DISP_FPS),
	.left_margin    = 40,
	.right_margin   = 24,
	.upper_margin   = 32,
	.lower_margin   = 11,
	.hsync_len      = 96,
	.vsync_len      = 2,
	.vmode          = FB_VMODE_NONINTERLACED
};

int wmtfb_fb1_probe;
/*--------------------- INTERNAL PRIVATE FUNCTIONS ---------------------------*/
/* void fbut_xxx(void); *//*Example*/

/*----------------------- Function Body --------------------------------------*/
/*----------------------- Linux Proc --------------------------------------*/
#ifdef DEBUG
void wmtfb_show_var(char *str, struct fb_var_screeninfo *var)
{
	DPRINT("----- %s ------------------------\n", str);
	DPRINT("res(%d,%d),vir(%d,%d),offset(%d,%d)\n",
		var->xres, var->yres, var->xres_virtual,
		var->yres_virtual, var->xoffset, var->yoffset);
	DPRINT("pixclk %d(%d),hsync %d,vsync %d\n", var->pixclock,
		(int)(PICOS2KHZ(var->pixclock) * 1000),
		var->hsync_len, var->vsync_len);
	DPRINT("left %d,right %d,upper %d,lower %d\n", var->left_margin,
		var->right_margin, var->upper_margin, var->lower_margin);
	DPRINT("bpp %d, grayscale %d\n", var->bits_per_pixel, var->grayscale);
	DPRINT("nonstd %d, activate %d, height %d, width %d\n",
		var->nonstd, var->activate, var->height, var->width);
	DPRINT("vmode 0x%x,sync 0x%x,rotate %d,accel %d\n",
		var->vmode, var->sync, var->rotate, var->accel_flags);
	DPRINT("-----------------------------\n");
	return;
}
#endif

#ifdef CONFIG_PROC_FS
#define CONFIG_VPP_PROC
#ifdef CONFIG_VPP_PROC
unsigned int vpp_proc_value;
char vpp_proc_str[16];
static ctl_table vpp_table[];
static int vpp_do_proc(ctl_table *ctl, int write,
				void *buffer, size_t *len, loff_t *ppos)
{
	int ret;
	int ctl_name;

	ctl_name = (((int)ctl - (int)vpp_table) / sizeof(ctl_table)) + 1;
	if (!write) {
		switch (ctl_name) {
		case 1:
			vpp_proc_value = g_vpp.dbg_msg_level;
			break;
		case 2:
			vpp_proc_value = g_vpp.dbg_wait;
			break;
		case 3:
			vpp_proc_value = g_vpp.dbg_flag;
			break;
		case 8:
		case 9:
			vpp_proc_value = vpp_get_base_clock((ctl_name == 8) ?
				VPP_MOD_GOVRH : VPP_MOD_GOVRH2);
			break;
		case 10:
			vpp_proc_value = p_scl->scale_mode;
			break;
		case 11:
			vpp_proc_value = p_scl->filter_mode;
			break;
#ifdef CONFIG_WMT_HDMI
		case 12:
			vpp_proc_value = g_vpp.hdmi_cp_enable;
			break;
		case 13:
			vpp_proc_value = g_vpp.hdmi_3d_type;
			break;
		case 14:
			vpp_proc_value = g_vpp.hdmi_certify_flag;
			break;
#endif
		case 15:
			vpp_proc_value = govrh_get_brightness(p_govrh);
			break;
		case 16:
			vpp_proc_value = govrh_get_contrast(p_govrh);
			break;
		case 17:
			vpp_proc_value = govrh_get_saturation(p_govrh);
			break;
		default:
			break;
		}
	}

	ret = proc_dointvec(ctl, write, buffer, len, ppos);
	if (write) {
		switch (ctl_name) {
		case 1:
			DPRINT("---------- VPP debug level ----------\n");
			DPRINT("0-disable,255-show all\n");
			DPRINT("1-scale,2-disp fb,3-interrupt,4-timer\n");
			DPRINT("5-ioctl,6-diag,7-stream\n");
			DPRINT("-------------------------------------\n");
			g_vpp.dbg_msg_level = vpp_proc_value;
			break;
		case 2:
			g_vpp.dbg_wait = vpp_proc_value;
			vpp_dbg_wake_up();
			break;
		case 3:
			g_vpp.dbg_flag = vpp_proc_value;
			break;
#ifdef CONFIG_WMT_EDID
		case 6:
			{
			vout_t *vo;

			vo = vout_get_entry_adapter(vpp_proc_value);
			if ((vo->inf) && (vo->inf->get_edid)) {
				vo->status &= ~VPP_VOUT_STS_EDID;
				if (vout_get_edid(vo->num)) {
					int i;

					vo->edid_info.option = 0;
					edid_dump(vo->edid);
					for (i = 1; i <= vo->edid[126]; i++)
						edid_dump(vo->edid + 128 * i);
					if (!edid_parse(vo->edid,
						&vo->edid_info))
						DBG_ERR("parse EDID\n");
				} else {
					DBG_ERR("read EDID\n");
				}
			}
			}
			break;
#endif
		case 8:
		case 9:
			govrh_set_clock((ctl_name == 8) ? p_govrh : p_govrh2,
				vpp_proc_value);
			DPRINT("set govr pixclk %d\n", vpp_proc_value);
			break;
		case 10:
			DPRINT("---------- scale mode ----------\n");
			DPRINT("0-recursive normal\n");
			DPRINT("1-recursive sw bilinear\n");
			DPRINT("2-recursive hw bilinear\n");
			DPRINT("3-realtime noraml (quality but x/32 limit)\n");
			DPRINT("4-realtime bilinear (fit edge w skip line)\n");
			DPRINT("-------------------------------------\n");
			p_scl->scale_mode = vpp_proc_value;
			break;
		case 11:
			p_scl->filter_mode = vpp_proc_value;
			break;
#ifdef CONFIG_WMT_HDMI
		case 12:
			g_vpp.hdmi_cp_enable = vpp_proc_value;
			hdmi_set_cp_enable(vpp_proc_value);
			break;
		case 13:
			g_vpp.hdmi_3d_type = vpp_proc_value;
			hdmi_tx_vendor_specific_infoframe_packet();
			break;
		case 14:
			g_vpp.hdmi_certify_flag = vpp_proc_value;
			break;
#endif
		case 15:
			govrh_set_brightness(p_govrh, vpp_proc_value);
			break;
		case 16:
			govrh_set_contrast(p_govrh, vpp_proc_value);
			break;
		case 17:
			govrh_set_saturation(p_govrh, vpp_proc_value);
			break;
		default:
			break;
		}
	}
	return ret;
}

struct proc_dir_entry *vpp_proc_dir;
static ctl_table vpp_table[] = {
	{ /* .ctl_name = 1, */
	.procname	= "dbg_msg",
	.data		= &vpp_proc_value,
	.maxlen		= sizeof(int),
	.mode		= 0666,
	.proc_handler = &vpp_do_proc,
	},
	{ /* .ctl_name = 2, */
	.procname	= "dbg_wait",
	.data		= &vpp_proc_value,
	.maxlen		= sizeof(int),
	.mode		= 0666,
	.proc_handler = &vpp_do_proc,
	},
	{ /* .ctl_name = 3, */
	.procname	= "dbg_flag",
	.data		= &vpp_proc_value,
	.maxlen		= sizeof(int),
	.mode		= 0666,
	.proc_handler = &vpp_do_proc,
	},
	{ /* .ctl_name = 4, */
	.procname	= "edid_disable",
	.data		= &edid_disable,
	.maxlen		= sizeof(int),
	.mode		= 0666,
	.proc_handler = &vpp_do_proc,
	},
	{ /* .ctl_name = 5, */
	.procname	= "edid_msg",
	.data		= &edid_msg_enable,
	.maxlen		= sizeof(int),
	.mode		= 0666,
	.proc_handler = &vpp_do_proc,
	},
	{ /* .ctl_name = 6, */
	.procname	= "vout_edid",
	.data		= &vpp_proc_value,
	.maxlen		= sizeof(int),
	.mode		= 0666,
	.proc_handler = &vpp_do_proc,
	},
	{ /* .ctl_name = 7, */
	.procname	= "vo_mode",
	.data		= vpp_proc_str,
	.maxlen		= 12,
	.mode		= 0666,
	.proc_handler = &proc_dostring,
	},
	{ /* .ctl_name = 8, */
	.procname	= "govr1_pixclk",
	.data		= &vpp_proc_value,
	.maxlen		= sizeof(int),
	.mode		= 0666,
	.proc_handler = &vpp_do_proc,
	},
	{ /* .ctl_name = 9, */
	.procname	= "govr2_pixclk",
	.data		= &vpp_proc_value,
	.maxlen		= sizeof(int),
	.mode		= 0666,
	.proc_handler = &vpp_do_proc,
	},
	{ /* .ctl_name = 10, */
	.procname	= "scl_scale_mode",
	.data		= &vpp_proc_value,
	.maxlen		= sizeof(int),
	.mode		= 0666,
	.proc_handler = &vpp_do_proc,
	},
	{ /* .ctl_name = 11, */
	.procname	= "scl_filter",
	.data		= &vpp_proc_value,
	.maxlen		= sizeof(int),
	.mode		= 0666,
	.proc_handler = &vpp_do_proc,
	},
	{ /* .ctl_name = 12 */
	.procname	= "hdmi_cp_enable",
	.data		= &vpp_proc_value,
	.maxlen		= sizeof(int),
	.mode		= 0666,
	.proc_handler = &vpp_do_proc,
	},
	{ /* .ctl_name = 13 */
	.procname	= "hdmi_3d",
	.data		= &vpp_proc_value,
	.maxlen		= sizeof(int),
	.mode		= 0666,
	.proc_handler = &vpp_do_proc,
	},
	{ /* .ctl_name = 14 */
	.procname	= "hdmi_certify",
	.data		= &vpp_proc_value,
	.maxlen		= sizeof(int),
	.mode		= 0666,
	.proc_handler = &vpp_do_proc,
	},
	{ /* .ctl_name = 15 */
	.procname	= "brightness",
	.data		= &vpp_proc_value,
	.maxlen		= sizeof(int),
	.mode		= 0666,
	.proc_handler = &vpp_do_proc,
	},
	{ /* .ctl_name = 16 */
	.procname	= "contrast",
	.data		= &vpp_proc_value,
	.maxlen		= sizeof(int),
	.mode		= 0666,
	.proc_handler = &vpp_do_proc,
	},
	{ /* .ctl_name = 17 */
	.procname	= "saturation",
	.data		= &vpp_proc_value,
	.maxlen		= sizeof(int),
	.mode		= 0666,
	.proc_handler = &vpp_do_proc,
	},
	{ /* end of table */
	}
};

static ctl_table vpp_root_table[] = {
	{
	.procname	= "vpp", /* create path ==> /proc/sys/vpp */
	.mode		= 0555,
	.child		= vpp_table
	},
	{ /* end of table */
	}
};
static struct ctl_table_header *vpp_table_header;
#endif

static int vpp_sts_read_proc(char *buf, char **start, off_t offset,
					int len, int *eof, void *data)
{
	volatile struct govrh_regs *regs =
		(volatile struct govrh_regs *) REG_GOVRH_BASE1_BEGIN;
	unsigned int yaddr, caddr;
	char *p = buf;
	unsigned int reg;

	p += sprintf(p, "--- VPP HW status ---\n");
#ifdef WMT_FTBLK_GOVRH
	p += sprintf(p, "GOVRH memory read underrun error %d,cnt %d,cnt2 %d\n",
		(regs->interrupt.val & 0x200) ? 1 : 0,
		p_govrh->underrun_cnt, p_govrh2->underrun_cnt);
	p_govrh->clr_sts(VPP_INT_ALL);
#endif

#ifdef WMT_FTBLK_SCL
	p += sprintf(p, "---------------------------------------\n");
	p += sprintf(p, "SCL TG error %d\n",
		vppif_reg32_read(SCL_INTSTS_TGERR));
	p += sprintf(p, "SCLR MIF1 read error %d\n",
		vppif_reg32_read(SCLR_INTSTS_R1MIFERR));
	p += sprintf(p, "SCLR MIF2 read error %d\n",
		vppif_reg32_read(SCLR_INTSTS_R2MIFERR));
	p += sprintf(p, "SCLW RGB fifo overflow %d\n",
		vppif_reg32_read(SCLW_INTSTS_MIFRGBERR));
	p += sprintf(p, "SCLW Y fifo overflow %d\n",
		vppif_reg32_read(SCLW_INTSTS_MIFYERR));
	p += sprintf(p, "SCLW C fifo overflow %d\n",
		vppif_reg32_read(SCLW_INTSTS_MIFCERR));
	p_scl->clr_sts(VPP_INT_ALL);
#endif

	p += sprintf(p, "---------------------------------------\n");
	p += sprintf(p, "(880.0)GOVRH Enable %d,(900.0)TG %d\n",
		regs->mif.b.enable, regs->tg_enable.b.enable);

	reg = REG32_VAL(PM_CTRL_BASE_ADDR + 0x258);
	p += sprintf(p, "--- POWER CONTROL ---\n");
	p += sprintf(p, "0x%x = 0x%x\n", PM_CTRL_BASE_ADDR + 0x258, reg);
	p += sprintf(p, "HDCP %d,VPP %d,SCL %d,HDMI I2C %d\n",
		(reg & BIT7) ? 1 : 0, (reg & BIT18) ? 1 : 0,
		(reg & BIT21) ? 1 : 0, (reg & BIT22) ? 1 : 0);
	p += sprintf(p, "HDMI %d,GOVR %d,NA12 %d\n",
		(reg & BIT23) ? 1 : 0, (reg & BIT25) ? 1 : 0,
		(reg & BIT16) ? 1 : 0);
	p += sprintf(p, "DVO %d,HDMI OUT %d,LVDS %d\n", (reg & BIT29) ? 1 : 0,
		(reg & BIT30) ? 1 : 0, (reg & BIT14) ? 1 : 0);

	p += sprintf(p, "--- VPP fb Address ---\n");
	p += sprintf(p, "GOV mb 0x%x 0x%x\n", g_vpp.mb[0], g_vpp.mb[1]);

#ifdef WMT_FTBLK_GOVRH
	govrh_get_fb_addr(p_govrh, &yaddr, &caddr);
	p += sprintf(p, "GOVRH fb addr Y(0x%x) 0x%x, C(0x%x) 0x%x\n",
		REG_GOVRH_YSA, yaddr, REG_GOVRH_CSA, caddr);
	govrh_get_fb_addr(p_govrh2, &yaddr, &caddr);
	p += sprintf(p, "GOVRH2 fb addr Y(0x%x) 0x%x, C(0x%x) 0x%x\n",
		REG_GOVRH2_YSA, yaddr, REG_GOVRH2_CSA, caddr);
#endif
	p_govrh->underrun_cnt = 0;
	p_govrh2->underrun_cnt = 0;
	return p - buf;
} /* End of vpp_sts_read_proc */

static int vpp_reg_read_proc(char *buf, char **start, off_t offset,
					int len, int *eof, void *data)
{
	char *p = buf;
	vpp_mod_base_t *mod_p;
	int i;

	DPRINT("Product ID:0x%x\n", vpp_get_chipid());
	for (i = 0; i < VPP_MOD_MAX; i++) {
		mod_p = vpp_mod_get_base(i);
		if (mod_p && mod_p->dump_reg)
			mod_p->dump_reg();
	}
#ifdef WMT_FTBLK_HDMI
	hdmi_reg_dump();
#endif
#ifdef WMT_FTBLK_LVDS
	lvds_reg_dump();
#endif
	return p - buf;
} /* End of vpp_reg_read_proc */
#endif

/*----------------------- fb ioctl --------------------------------------*/
int vpp_common_ioctl(unsigned int cmd, unsigned long arg)
{
	vpp_mod_base_t *mod_p;
	vpp_fb_base_t *mod_fb_p;
	int retval = 0;

	switch (cmd) {
	case VPPIO_VPPGET_INFO:
		{
		int i;
		vpp_cap_t parm;

		parm.chip_id = vpp_get_chipid();
		parm.version = 0x01;
		parm.resx_max = VPP_HD_MAX_RESX;
		parm.resy_max = VPP_HD_MAX_RESY;
		parm.pixel_clk = 400000000;
		parm.module = 0x0;
		for (i = 0; i < VPP_MOD_MAX; i++) {
			mod_p = vpp_mod_get_base(i);
			if (mod_p)
				parm.module |= (0x01 << i);
		}
		parm.option = VPP_CAP_DUAL_DISPLAY;
		copy_to_user((void *)arg, (void *) &parm, sizeof(vpp_cap_t));
		}
		break;
	case VPPIO_VPPSET_INFO:
		{
		vpp_cap_t parm;

		copy_from_user((void *)&parm, (const void *)arg,
			sizeof(vpp_cap_t));
		}
		break;
	case VPPIO_I2CSET_BYTE:
		{
		vpp_i2c_t parm;
		unsigned int id;

		copy_from_user((void *) &parm, (const void *)arg,
			sizeof(vpp_i2c_t));
		id = (parm.addr & 0x0000FF00) >> 8;
		vpp_i2c_write(id, (parm.addr & 0xFF), parm.index,
			(char *)&parm.val, 1);
		}
		break;
	case VPPIO_I2CGET_BYTE:
		{
		vpp_i2c_t parm;
		unsigned int id;
		int len;

		copy_from_user((void *) &parm, (const void *)arg,
			sizeof(vpp_i2c_t));
		id = (parm.addr & 0x0000FF00) >> 8;
		len = parm.val;
		{
			unsigned char buf[len];

			vpp_i2c_read(id, (parm.addr & 0xFF), parm.index,
				buf, len);
			parm.val = buf[0];
		}
		copy_to_user((void *)arg, (void *) &parm, sizeof(vpp_i2c_t));
		}
		break;
	case VPPIO_VPPSET_FBDISP:
		break;
	case VPPIO_VPPGET_FBDISP:
		{
		vpp_dispfb_info_t parm;
		copy_to_user((void *)arg, (void *) &parm,
			sizeof(vpp_dispfb_info_t));
		}
		break;
	case VPPIO_WAIT_FRAME:
		vpp_wait_vsync(0, arg);
		break;
	case VPPIO_MODULE_FRAMERATE:
		{
		vpp_mod_arg_t parm;

		copy_from_user((void *) &parm, (const void *)arg,
			sizeof(vpp_mod_arg_t));
		mod_fb_p = vpp_mod_get_fb_base(parm.mod);
		if (!mod_fb_p)
			break;
		if (parm.read) {
			parm.arg1 = mod_fb_p->framerate;
			copy_to_user((void *)arg, (void *) &parm,
				sizeof(vpp_mod_arg_t));
		} else {
			mod_fb_p->framerate = parm.arg1;
		}
		}
		break;
	case VPPIO_MODULE_ENABLE:
		{
		vpp_mod_arg_t parm;

		copy_from_user((void *) &parm, (const void *)arg,
			sizeof(vpp_mod_arg_t));
		mod_p = vpp_mod_get_base(parm.mod);
		if (!mod_p)
			break;

		if (parm.read) {
			/* not used */
		} else {
			mod_p->set_enable(parm.arg1);
#ifdef WMT_FTBLK_GOVRH_CURSOR
			if (parm.mod == VPP_MOD_CURSOR) {
				p_cursor->enable = parm.arg1;
				if (p_cursor->enable) {
					vpp_irqproc_work(VPP_INT_GOVRH_PVBI,
						(void *)govrh_CUR_irqproc, 0,
						0, 0);
				} else {
					vpp_irqproc_del_work(VPP_INT_GOVRH_PVBI,
						(void *)govrh_CUR_irqproc);
				}
			}
#endif
		}
		}
		break;
	case VPPIO_MODULE_TIMING:
		DPRINT("remove for new arch\n");
		break;
	case VPPIO_MODULE_FBADDR:
		{
		vpp_mod_arg_t parm;

		copy_from_user((void *) &parm, (const void *)arg,
			sizeof(vpp_mod_arg_t));
		mod_fb_p = vpp_mod_get_fb_base(parm.mod);
		if (!mod_fb_p)
			break;
		mod_p = vpp_mod_get_base(parm.mod);
		if (parm.read) {
			mod_fb_p->get_addr(&parm.arg1, &parm.arg2);
			copy_to_user((void *)arg, (void *) &parm,
				sizeof(vpp_mod_arg_t));
		} else {
			mod_fb_p->set_addr(parm.arg1, parm.arg2);
		}
		}
		break;
	case VPPIO_MODULE_FBINFO:
		{
		vpp_mod_fbinfo_t parm;

		copy_from_user((void *) &parm, (const void *)arg,
			sizeof(vpp_mod_fbinfo_t));

		if (g_vpp.virtual_display)
			parm.mod = (hdmi_get_plugin()) ?
				VPP_MOD_GOVRH : VPP_MOD_GOVRH2;
		mod_fb_p = vpp_mod_get_fb_base(parm.mod);
		if (!mod_fb_p)
			break;
		mod_p = vpp_mod_get_base(parm.mod);
		if (parm.read) {
			parm.fb = mod_fb_p->fb;
			switch (parm.mod) {
			case VPP_MOD_GOVRH:
			case VPP_MOD_GOVRH2:
				govrh_get_framebuffer((govrh_mod_t *)mod_p,
					&parm.fb);
				break;
			default:
				break;
			}
			copy_to_user((void *)arg, (void *) &parm,
				sizeof(vpp_mod_fbinfo_t));
		} else {
			mod_fb_p->fb = parm.fb;
			mod_fb_p->set_framebuf(&parm.fb);
			
			if (g_vpp.virtual_display) {
#ifdef CONFIG_VPP_STREAM_CAPTURE
				if (g_vpp.stream_enable)
					vpp_mb_scale_bitblit(&mod_fb_p->fb);
#endif
				if (vpp_check_dbg_level(VPP_DBGLVL_FPS))
					vpp_dbg_timer(
						&vout_info[1].pandisp_timer,
						"fbinfo", 2);
			}
		}
		}
		break;
	case VPPIO_MODULE_VIEW:
		{
		vpp_mod_view_t parm;
		copy_from_user((void *) &parm, (const void *)arg,
			sizeof(vpp_mod_view_t));

		mod_fb_p = vpp_mod_get_fb_base(parm.mod);
		if (!mod_fb_p)
			break;
		mod_p = vpp_mod_get_base(parm.mod);
		if (parm.read) {
			mod_fb_p->fn_view(VPP_FLAG_RD, &parm.view);
			copy_to_user((void *)arg, (void *) &parm,
				sizeof(vpp_mod_view_t));
		} else {
			mod_fb_p->fn_view(0, &parm.view);
		}
		}
		break;
#ifdef CONFIG_VPP_STREAM_CAPTURE
	case VPPIO_STREAM_ENABLE:
		g_vpp.stream_enable = arg;
		g_vpp.stream_mb_sync_flag = 0;

		MSG("[VPP] VPPIO_STREAM_ENABLE %d\n", g_vpp.stream_enable);
#ifdef CONFIG_VPP_STREAM_FIX_RESOLUTION
		memset(&g_vpp.stream_fb, 0, sizeof(vdo_framebuf_t));
		g_vpp.stream_fb.img_w = 1280;
		g_vpp.stream_fb.img_h = 720;
		g_vpp.stream_fb.fb_w =
			vpp_calc_align(g_vpp.stream_fb.img_w, 64);
		g_vpp.stream_fb.fb_h = g_vpp.stream_fb.img_h;
		g_vpp.stream_fb.bpp = 16;
		g_vpp.stream_fb.col_fmt = VDO_COL_FMT_YUV422H;
		g_vpp.stream_fb.y_size = g_vpp.stream_fb.fb_w *
			g_vpp.stream_fb.fb_h;
		g_vpp.stream_fb.c_size = g_vpp.stream_fb.fb_w *
			g_vpp.stream_fb.fb_h;
#endif
#ifdef CONFIG_VPP_STREAM_ROTATE
		g_vpp.stream_mb_lock= 0;
		g_vpp.stream_mb_index = 0xFF;
		vpp_netlink_notify(WP_PID, DEVICE_STREAM,
				(g_vpp.stream_enable) ? 1 : 0);
#else
#ifndef CONFIG_VPP_DYNAMIC_ALLOC
		if (g_vpp.alloc_framebuf == 0) {
#endif
		{
			if (arg) {
#ifdef CONFIG_VPP_STREAM_FIX_RESOLUTION
				vpp_alloc_framebuffer(g_vpp.stream_fb.fb_w,
							g_vpp.stream_fb.fb_h);
#else
				vout_info_t *info;

				info = vout_info_get_entry(0);
				vpp_alloc_framebuffer(info->resx, info->resy);
#endif
			} else {
				vpp_free_framebuffer();
			}
		}
		vpp_mod_set_clock(VPP_MOD_SCL, g_vpp.stream_enable, 0);
		vpp_pan_display(0, 0);
		if (!g_vpp.stream_enable) {
			vpp_lock();
			vpp_mb_put(0);
			vpp_unlock();
		}
#endif
		{
#if 0		
		vpp_int_t type;

		type = (vout_info[0].govr_mod == VPP_MOD_GOVRH) ?
			VPP_INT_GOVRH_VBIS : VPP_INT_GOVRH2_VBIS;
		if (g_vpp.stream_enable) {
			vpp_irqproc_work(type, (void *)vpp_mb_irqproc_sync,
				vout_info[0].govr, 0, 0);
		} else {
			vpp_irqproc_del_work(type, (void *)vpp_mb_irqproc_sync);
		}
#endif
		wmt_enable_mmfreq(WMT_MMFREQ_MIRACAST, g_vpp.stream_enable);
		}
		break;
	case VPPIO_STREAM_GETFB:
		{
		vdo_framebuf_t fb;

		vpp_lock();
#ifdef CONFIG_VPP_STREAM_ROTATE
		if (g_vpp.stream_mb_index == 0xFF) { /* not avail */
			retval = -1;
			vpp_unlock();
			break;
		}
		fb = vout_info[1].fb;
#else
#ifdef CONFIG_VPP_STREAM_FIX_RESOLUTION
		fb = g_vpp.stream_fb;		
#else
		fb = vout_info[0].fb;
#endif
#endif
		fb.fb_w = vpp_calc_align(fb.fb_w, 64);
		fb.y_addr = g_vpp.stream_mb[g_vpp.stream_mb_index];
		fb.y_size = fb.fb_w * fb.img_h;
		fb.c_addr = fb.y_addr + fb.y_size;
		fb.c_size = fb.y_size;
		fb.col_fmt = VDO_COL_FMT_YUV422H;

		copy_to_user((void *)arg, (void *) &fb, sizeof(vdo_framebuf_t));
		retval = vpp_mb_get(fb.y_addr);
		vpp_unlock();
		}
		break;
	case VPPIO_STREAM_PUTFB:
		{
		vdo_framebuf_t fb;
		copy_from_user((void *) &fb, (const void *)arg,
			sizeof(vdo_framebuf_t));
		vpp_lock();
		vpp_mb_put(fb.y_addr);
		vpp_unlock();
		}
		break;
#endif
	case VPPIO_MODULE_CSC:
		{
		vpp_mod_arg_t parm;
		copy_from_user((void *) &parm, (const void *)arg,
			sizeof(vpp_mod_arg_t));
		mod_fb_p = vpp_mod_get_fb_base(parm.mod);
		if (!mod_fb_p)
			break;
		mod_p = vpp_mod_get_base(parm.mod);
		if (parm.read) {
			parm.arg1 = mod_fb_p->csc_mode;
			copy_to_user((void *)arg, (void *) &parm,
				sizeof(vpp_mod_arg_t));
		} else {
			mod_fb_p->csc_mode = parm.arg1;
			mod_fb_p->set_csc(mod_fb_p->csc_mode);
		}
		}
		break;
	case VPPIO_MULTIVD_ENABLE:
		wmt_enable_mmfreq(WMT_MMFREQ_MULTI_VD, arg);
		break;
	default:
		retval = -ENOTTY;
		break;
	}
	return retval;
} /* End of vpp_common_ioctl */

int vout_ioctl(unsigned int cmd, unsigned long arg)
{
	int retval = 0;

	switch (cmd) {
	case VPPIO_VOGET_INFO:
		{
		vpp_vout_info_t parm;
		vout_t *vo;
		int num;
		vout_inf_t *inf;
		int fb_num;
		vout_info_t *vo_info;

		copy_from_user((void *) &parm, (const void *)arg,
			sizeof(vpp_vout_info_t));
		fb_num = (parm.num & 0xF0) >> 4;
		num = parm.num & 0xF;
		DBGMSG("VPPIO_VOGET_INFO %d,%d\n", fb_num, num);
		if ((fb_num >= VPP_VOUT_INFO_NUM) || (num >= VOUT_MODE_MAX)) {
			retval = -ENOTTY;
			DBG_MSG("fb_num or num invalid\n");
			break;
		}
		memset(&parm, 0, sizeof(vpp_vout_info_t));
		vo = vout_get_entry_adapter(num);
		if (vo == 0) {
			retval = -ENOTTY;
			DBG_MSG("vo null\n");
			break;
		}
		inf = vout_get_inf_entry_adapter(num);
		if (inf == 0) {
			retval = -ENOTTY;
			DBG_MSG("inf null\n");
			break;
		}
		vo_info = vout_get_info_entry(vo->num);
		if (vo_info == 0) {
			retval = -ENOTTY;
			DBG_MSG("vo_info null\n");
			break;
		}
		if ((g_vpp.dual_display == 1) && (g_vpp.virtual_display == 0)
			&& (fb_num != vo_info->num)) {
			retval = -ENOTTY;
			DBG_MSG("fb num not match\n");
			break;
		}
		DBGMSG("VPPIO_VOGET_INFO %d,fb%d,0x%x,0x%x\n", num,
			vo_info->num, (int)vo, (int)inf);
		if ((vo == 0) || (inf == 0))
			goto label_get_info;

		parm.num = num;
		parm.status = vo->status | VPP_VOUT_STS_REGISTER;
		strncpy(parm.name, vout_adpt_str[num], 10);
		switch (inf->mode) {
		case VOUT_INF_DVI:
			parm.status &= ~VPP_VOUT_STS_ACTIVE;
			if (vo->dev) {
				/* check current DVI is dvi/dvo2hdmi/lcd */
				switch (num) {
				case VOUT_DVI:
					if (strcmp("VT1632",
						vo->dev->name) == 0)
						parm.status |=
							VPP_VOUT_STS_ACTIVE;
					parm.status |= VPP_VOUT_STS_ACTIVE;
					break;
				case VOUT_LCD:
					if (strcmp("LCD",
						vo->dev->name) == 0)
						parm.status |=
							VPP_VOUT_STS_ACTIVE;
					break;
				case VOUT_DVO2HDMI:
					if (strcmp("SIL902X",
						vo->dev->name) == 0)
						parm.status |=
							VPP_VOUT_STS_ACTIVE;
					break;
				default:
					break;
				}
			} else { /* dvi hw mode */
				if (num == VOUT_DVI)
					parm.status |= VPP_VOUT_STS_ACTIVE;
			}

			if (g_vpp.virtual_display) {
				if (vout_chkplug(VPP_VOUT_NUM_HDMI))
					parm.status &= ~VPP_VOUT_STS_ACTIVE;
			}
			break;
		case VOUT_INF_HDMI:
		case VOUT_INF_LVDS:
			/* check current HDMI is HDMI or LVDS */
			if (vo->inf != inf)
				parm.status = VPP_VOUT_STS_REGISTER;
			break;
		default:
			break;
		}

		if (parm.status & VPP_VOUT_STS_ACTIVE) {
			if (vout_chkplug(vo->num))
				parm.status |= VPP_VOUT_STS_PLUGIN;
			else
				parm.status &= ~VPP_VOUT_STS_PLUGIN;
		} else {
			parm.status = VPP_VOUT_STS_REGISTER;
		}
label_get_info:
		copy_to_user((void *)arg, (const void *) &parm,
			sizeof(vpp_vout_info_t));
		}
		break;
	case VPPIO_VOSET_MODE:
		{
		vpp_vout_parm_t parm;
		vout_t *vo;
		vout_inf_t *inf;

		copy_from_user((void *) &parm, (const void *)arg,
			sizeof(vpp_vout_parm_t));
		vo = vout_get_entry_adapter(parm.num);
		inf = vout_get_inf_entry_adapter(parm.num);
		if (vo && inf) {
			DBG_DETAIL("VPPIO_VOSET_MODE %d %d\n",
				vo->num, parm.arg);
			if (g_vpp.dual_display == 0) {
				int plugin;

				plugin = vout_chkplug(VPP_VOUT_NUM_HDMI);
				vout_set_blank((0x1 << VPP_VOUT_NUM_HDMI),
					(plugin) ? 0 : 1);
				vout_set_blank((0x1 << VPP_VOUT_NUM_DVI),
					(plugin) ? 1 : 0);
				break;
			}
			vout_set_blank((0x1 << vo->num), (parm.arg) ? 0 : 1);
			retval = vout_set_mode(vo->num, inf->mode);
		}
		}
		break;
	case VPPIO_VOSET_BLANK:
		{
		vpp_vout_parm_t parm;
		vout_t *vo;

		copy_from_user((void *) &parm, (const void *)arg,
			sizeof(vpp_vout_parm_t));
		vo = vout_get_entry_adapter(parm.num);
		if (vo)
			retval = vout_set_blank((0x1 << vo->num), parm.arg);
		}
		break;
	case VPPIO_VOSET_OPTION:
		{
		vpp_vout_option_t option;
		vout_t *vo;
		int num;
		vout_inf_t *inf;

		copy_from_user((void *) &option, (const void *)arg,
			sizeof(vpp_vout_option_t));
		num = option.num;
		if (num >= VOUT_MODE_MAX) {
			retval = -ENOTTY;
			break;
		}
		vo = vout_get_entry_adapter(num);
		inf = vout_get_inf_entry_adapter(num);
		if (vo && inf) {
			vo->option[0] = option.option[0];
			vo->option[1] = option.option[1];
			vo->option[2] = option.option[2];
			vout_set_mode(vo->num, inf->mode);
		}
		}
		break;
	case VPPIO_VOGET_OPTION:
		{
		vpp_vout_option_t option;
		vout_t *vo;
		int num;

		copy_from_user((void *) &option, (const void *)arg,
			sizeof(vpp_vout_option_t));
		num = option.num;
		if (num >= VOUT_MODE_MAX) {
			retval = -ENOTTY;
			break;
		}
		memset(&option, 0, sizeof(vpp_vout_info_t));
		vo = vout_get_entry_adapter(num);
		if (vo) {
			option.num = num;
			option.option[0] = vo->option[0];
			option.option[1] = vo->option[1];
			option.option[2] = vo->option[2];
		}
		copy_to_user((void *)arg, (const void *) &option,
			sizeof(vpp_vout_option_t));
		}
		break;
	case VPPIO_VOUT_VMODE:
		{
		vpp_vout_vmode_t parm;
		int i;
		struct fb_videomode *vmode;
		unsigned int resx, resy, fps;
		unsigned int pre_resx, pre_resy, pre_fps;
		int index, from_index;
		int support;
		unsigned int option, pre_option;
#ifdef CONFIG_WMT_EDID
		edid_info_t *edid_info;
#endif
		struct fb_videomode *edid_vmode;
		vpp_vout_t mode;

		copy_from_user((void *) &parm, (const void *)arg,
			sizeof(vpp_vout_vmode_t));
		from_index = parm.num;
		parm.num = 0;
		mode = parm.mode & 0xF;
#ifdef CONFIG_VPP_DEMO
		parm.parm[parm.num].resx = 1920;
		parm.parm[parm.num].resy = 1080;
		parm.parm[parm.num].fps = 60;
		parm.parm[parm.num].option = 0;
		parm.num++;
#else
#ifdef CONFIG_WMT_EDID
		{
		vout_t *vo;

		vo = vout_get_entry_adapter(mode);
		if (!vo)
			goto vout_vmode_end;

		if (!(vo->status & VPP_VOUT_STS_PLUGIN)) {
			DPRINT("*W* not plugin\n");
			goto vout_vmode_end;
		}

		if (vout_get_edid(vo->num) == 0) {
			DPRINT("*W* read EDID fail\n");
			goto vout_vmode_end;
		}
		if (edid_parse(vo->edid, &vo->edid_info) == 0) {
			DPRINT("*W* parse EDID fail\n");
			goto vout_vmode_end;
		}
		edid_info = &vo->edid_info;
		}
#endif
		index = 0;
		resx = resy = fps = option = 0;
		pre_resx = pre_resy = pre_fps = pre_option = 0;
		for (i = 0; ; i++) {
			vmode = (struct fb_videomode *) &vpp_videomode[i];
			if (vmode->pixclock == 0)
				break;
			resx = vmode->xres;
			resy = vmode->yres;
			fps = vmode->refresh;
			option = fps & EDID_TMR_FREQ;
			option |= (vmode->vmode & FB_VMODE_INTERLACED) ?
				EDID_TMR_INTERLACE : 0;
			if ((pre_resx == resx) && (pre_resy == resy) &&
				(pre_fps == fps) && (pre_option == option))
				continue;
			pre_resx = resx;
			pre_resy = resy;
			pre_fps = fps;
			pre_option = option;
			support = 0;
#ifdef CONFIG_WMT_EDID
			if (edid_find_support(edid_info, resx, resy,
				option, &edid_vmode))
#else
			if (1)
#endif
				support = 1;

			switch (mode) {
			case VPP_VOUT_HDMI:
			case VPP_VOUT_DVO2HDMI:
				if (g_vpp.hdmi_video_mode) {
					if (resy > g_vpp.hdmi_video_mode)
						support = 0;
				}
				break;
			default:
				break;
			}

			if (support) {
				if (index >= from_index) {
					parm.parm[parm.num].resx = resx;
					parm.parm[parm.num].resy = resy;
					parm.parm[parm.num].fps = fps;
					parm.parm[parm.num].option =
						vmode->vmode;
					parm.num++;
				}
				index++;
				if (parm.num >= VPP_VOUT_VMODE_NUM)
					break;
			}
		}
#ifdef CONFIG_WMT_EDID
vout_vmode_end:
#endif
#endif
		DBG_MSG("[VPP] get support vmode %d\n", parm.num);
		copy_to_user((void *)arg, (const void *) &parm,
			sizeof(vpp_vout_vmode_t));
		}
		break;
	case VPPIO_VOGET_EDID:
		{
		vpp_vout_edid_t parm;
		char *edid;
		vout_t *vo;
		int size;

		copy_from_user((void *) &parm, (const void *)arg,
			sizeof(vpp_vout_edid_t));
		size = 0;
#ifdef CONFIG_WMT_EDID
		vo = vout_get_entry_adapter(parm.mode);
		if (!vo)
			goto vout_edid_end;

		if (!(vo->status & VPP_VOUT_STS_PLUGIN)) {
			DBG_ERR("*W* not plugin\n");
			goto vout_edid_end;
		}

		edid = vout_get_edid(vo->num);
		if (edid == 0) {
			DBG_ERR("*W* read EDID fail\n");
			goto vout_edid_end;
		}
		size = (edid[0x7E] + 1) * 128;
		if (size > parm.size)
			size = parm.size;
		copy_to_user((void *) parm.buf, (void *) edid, size);
vout_edid_end:
#endif
		parm.size = size;
		copy_to_user((void *)arg, (const void *) &parm,
			sizeof(vpp_vout_edid_t));
		}
		break;
	case VPPIO_VOGET_CP_INFO:
		{
		vpp_vout_cp_info_t parm;
		int num;

		copy_from_user((void *) &parm, (const void *)arg,
			sizeof(vpp_vout_cp_info_t));
		num = parm.num;
		if (num >= VOUT_MODE_MAX) {
			retval = -ENOTTY;
			break;
		}
		memset(&parm, 0, sizeof(vpp_vout_cp_info_t));
		switch (num) {
		case VOUT_HDMI:
			if (!g_vpp.hdmi_certify_flag) {
				if (g_vpp.hdmi_bksv[0] == 0) {
					hdmi_get_bksv(&g_vpp.hdmi_bksv[0]);
					DBG_MSG("get BKSV 0x%x 0x%x\n",
						g_vpp.hdmi_bksv[0],
						g_vpp.hdmi_bksv[1]);
				}
			}
		case VOUT_DVO2HDMI:
			parm.bksv[0] = g_vpp.hdmi_bksv[0];
			parm.bksv[1] = g_vpp.hdmi_bksv[1];
			break;
		default:
			parm.bksv[0] = parm.bksv[1] = 0;
			break;
		}
		copy_to_user((void *)arg, (const void *) &parm,
			sizeof(vpp_vout_cp_info_t));
		}
		break;
	case VPPIO_VOSET_CP_KEY:
		if (g_vpp.hdmi_cp_p == 0) {
			g_vpp.hdmi_cp_p =
				(char *)kmalloc(sizeof(vpp_vout_cp_key_t),
				GFP_KERNEL);
		}
		if (g_vpp.hdmi_cp_p) {
			copy_from_user((void *) g_vpp.hdmi_cp_p,
				(const void *)arg, sizeof(vpp_vout_cp_key_t));
			if (hdmi_cp)
				hdmi_cp->init();
		}
		break;
#ifdef WMT_FTBLK_HDMI
	case VPPIO_VOSET_AUDIO_PASSTHRU:
		vppif_reg32_write(HDMI_AUD_SUB_PACKET, (arg) ? 0xF : 0x0);
		break;
#endif
#ifdef CONFIG_VPP_VIRTUAL_DISPLAY
	case VPPIO_VOSET_VIRTUAL_FBDEV:
		g_vpp.fb0_bitblit = (arg) ? 0 : 1;
		MSG("[VPP] virtual display %d\n", (int)arg);
		break;
#endif
	default:
		retval = -ENOTTY;
		break;
	}
	return retval;
} /* End of vout_ioctl */

int govr_ioctl(unsigned int cmd, unsigned long arg)
{
	int retval = 0;

	switch (cmd) {
#ifdef WMT_FTBLK_GOVRH
	case VPPIO_GOVRSET_DVO:
		{
		vdo_dvo_parm_t parm;

		copy_from_user((void *) &parm, (const void *)arg,
			sizeof(vdo_dvo_parm_t));
		govrh_set_dvo_enable(p_govrh, parm.enable);
		govrh_set_dvo_color_format(p_govrh, parm.color_fmt);
		govrh_set_dvo_clock_delay(p_govrh, parm.clk_inv,
			parm.clk_delay);
		govrh_set_dvo_outdatw(p_govrh, parm.data_w);
		govrh_set_dvo_sync_polar(p_govrh, parm.sync_polar,
			parm.vsync_polar);
		p_govrh->fb_p->set_csc(p_govrh->fb_p->csc_mode);
		}
		break;
#endif
#ifdef WMT_FTBLK_GOVRH_CURSOR
	case VPPIO_GOVRSET_CUR_COLKEY:
		{
		vpp_mod_arg_t parm;

		copy_from_user((void *) &parm, (const void *)arg,
			sizeof(vdo_dvo_parm_t));
		govrh_CUR_set_color_key(p_cursor, VPP_FLAG_ENABLE,
			0, parm.arg1);
		}
		break;
	case VPPIO_GOVRSET_CUR_HOTSPOT:
		{
		vpp_mod_arg_t parm;
		vdo_view_t view;

		copy_from_user((void *) &parm, (const void *)arg,
			sizeof(vdo_dvo_parm_t));
		p_cursor->hotspot_x = parm.arg1;
		p_cursor->hotspot_y = parm.arg2;
		view.posx = p_cursor->posx;
		view.posy = p_cursor->posy;
		p_cursor->fb_p->fn_view(0, &view);
		}
		break;
#endif
	default:
		retval = -ENOTTY;
		break;
	}
	return retval;
} /* End of govr_ioctl */

int scl_ioctl(unsigned int cmd, unsigned long arg)
{
	int retval = 0;
	extern int ge_do_alpha_bitblt(vdo_framebuf_t *src,
		vdo_framebuf_t *src2, vdo_framebuf_t *dst);

	switch (cmd) {
	case VPPIO_SCL_SCALE_OVERLAP:
		{
		vpp_scale_overlap_t parm;

		copy_from_user((void *) &parm, (const void *)arg,
			sizeof(vpp_scale_overlap_t));
		if (vpp_check_dbg_level(VPP_DBGLVL_FPS))
			vpp_dbg_timer(&p_scl->overlap_timer, 0, 1);

		p_scl->scale_sync = 1;
		retval = scl_set_scale_overlap(&parm.src_fb,
			&parm.src2_fb, &parm.dst_fb);

		if (vpp_check_dbg_level(VPP_DBGLVL_FPS))
			vpp_dbg_timer(&p_scl->overlap_timer, "overlap", 2);
		}
		break;
	case VPPIO_SCL_SCALE_ASYNC:
	case VPPIO_SCL_SCALE:
		{
		vpp_scale_t parm;

		p_scl->scale_sync = (cmd == VPPIO_SCL_SCALE) ? 1 : 0;
		copy_from_user((void *) &parm, (const void *)arg,
			sizeof(vpp_scale_t));
		if (vpp_check_dbg_level(VPP_DBGLVL_FPS))
			vpp_dbg_timer(&p_scl->scale_timer, 0, 1);

		vpp_set_NA12_hiprio(1);
		retval = vpp_set_recursive_scale(&parm.src_fb, &parm.dst_fb);
		vpp_set_NA12_hiprio(0);

		if (vpp_check_dbg_level(VPP_DBGLVL_FPS))
			vpp_dbg_timer(&p_scl->scale_timer, "scale", 2);
		copy_to_user((void *)arg, (void *)&parm, sizeof(vpp_scale_t));
		}
		break;
#ifdef WMT_FTBLK_SCL
	case VPPIO_SCL_DROP_LINE_ENABLE:
		scl_set_drop_line(arg);
		break;
#endif
	case VPPIO_SCL_SCALE_FINISH:
		retval = p_scl->scale_finish();
		break;
	case VPPIO_SCLSET_OVERLAP:
		{
		vpp_overlap_t parm;

		copy_from_user((void *) &parm, (const void *)arg,
			sizeof(vpp_overlap_t));
		vpp_mod_set_clock(VPP_MOD_SCL, VPP_FLAG_ENABLE, 0);
		scl_set_overlap(&parm);
		vpp_mod_set_clock(VPP_MOD_SCL, VPP_FLAG_DISABLE, 0);
		}
		break;
	default:
		retval = -ENOTTY;
		break;
	}
	return retval;
} /* End of scl_ioctl */

int vpp_ioctl(unsigned int cmd, unsigned long arg)
{
	int retval = 0;
	int err = 0;

/*	DBGMSG("vpp_ioctl\n"); */
	switch (_IOC_TYPE(cmd)) {
	case VPPIO_MAGIC:
		break;
	default:
		return -ENOTTY;
	}

	/* check argument area */
	if (_IOC_DIR(cmd) & _IOC_READ)
		err = !access_ok(VERIFY_WRITE, (void __user *) arg,
		_IOC_SIZE(cmd));
	else if (_IOC_DIR(cmd) & _IOC_WRITE)
		err = !access_ok(VERIFY_READ, (void __user *) arg,
		_IOC_SIZE(cmd));

	if (err)
		return -EFAULT;

	if (vpp_check_dbg_level(VPP_DBGLVL_IOCTL)) {
		switch (cmd) {
		case VPPIO_VPPSET_FBDISP:
		case VPPIO_VPPGET_FBDISP:
		case VPPIO_MODULE_VIEW:	/* cursor pos */
			break;
		default:
			DPRINT("[VPP] ioctl cmd 0x%x,arg 0x%x\n",
				_IOC_NR(cmd), (int)arg);
			break;
		}
	}

	vpp_set_mutex(1, 1);
	switch (_IOC_NR(cmd)) {
	case VPPIO_VPP_BASE ... (VPPIO_VOUT_BASE-1):
		/* DBGMSG("VPP command ioctl\n"); */
		retval = vpp_common_ioctl(cmd, arg);
		break;
	case VPPIO_VOUT_BASE ... (VPPIO_GOVR_BASE-1):
		/* DBGMSG("VOUT ioctl\n"); */
		retval = vout_ioctl(cmd, arg);
		break;
	case VPPIO_GOVR_BASE ... (VPPIO_SCL_BASE-1):
		/* DBGMSG("GOVR ioctl\n"); */
		retval = govr_ioctl(cmd, arg);
		break;
	case VPPIO_SCL_BASE ... (VPPIO_MAX-1):
		/* DBGMSG("SCL ioctl\n"); */
		vpp_set_mutex(1, 0);
		retval = scl_ioctl(cmd, arg);
		vpp_set_mutex(1, 1);
		break;
	default:
		retval = -ENOTTY;
		break;
	}
	vpp_set_mutex(1, 0);

	if (vpp_check_dbg_level(VPP_DBGLVL_IOCTL)) {
		switch (cmd) {
		case VPPIO_VPPSET_FBDISP:
		case VPPIO_VPPGET_FBDISP:
		case VPPIO_MODULE_VIEW:
			break;
		default:
			DPRINT("[VPP] ioctl cmd 0x%x,ret 0x%x\n",
				_IOC_NR(cmd), (int)retval);
			break;
		}
	}
	return retval;
} /* End of vpp_ioctl */

int vpp_set_blank(struct fb_info *info, int blank)
{
	vout_info_t *vo_info;
	unsigned int vout_blank_mask = 0, vout_unblank_mask = 0;

	if ((g_vpp.dual_display == 0) && (info->node == 1))
		return 0;

	DBG_MSG("(%d,%d)\n", info->node, blank);

	vo_info = vout_info_get_entry(info->node);
	if (blank)
		vout_blank_mask = vout_get_mask(vo_info);
	else
		vout_unblank_mask = vout_get_mask(vo_info);

	if (g_vpp.virtual_display || (g_vpp.dual_display == 0)) {
		int plugin;

		plugin = vout_chkplug(VPP_VOUT_NUM_HDMI);
		vout_blank_mask &= ~((0x1 << VPP_VOUT_NUM_DVI) +
			(0x1 << VPP_VOUT_NUM_HDMI));
		vout_unblank_mask &= ~((0x1 << VPP_VOUT_NUM_DVI) +
			(0x1 << VPP_VOUT_NUM_HDMI));
		vout_blank_mask |= (plugin) ? (0x1 << VPP_VOUT_NUM_DVI) :
			(0x1 << VPP_VOUT_NUM_HDMI);
		vout_unblank_mask |= (plugin) ? (0x1 << VPP_VOUT_NUM_HDMI) :
			(0x1 << VPP_VOUT_NUM_DVI);
	}
	vout_set_blank(vout_blank_mask, 1);
	vout_set_blank(vout_unblank_mask, 0);
	return 0;
}

void vpp_set_lvds_blank(int blank)
{
	if (lcd_get_lvds_id() != LCD_LVDS_1024x600)
		return;

	if (blank == 0) {
		REG32_VAL(GPIO_BASE_ADDR+0xC0) |= 0x400;
		REG32_VAL(GPIO_BASE_ADDR+0x80) |= 0x400;
		mdelay(6);
	}

	if (blank)
		msleep(50);

	vout_set_blank((0x1 << VPP_VOUT_NUM_LVDS),
		(blank) ? VOUT_BLANK_POWERDOWN : VOUT_BLANK_UNBLANK);
	lvds_set_power_down((blank) ? 1 : 0);

	if (blank) {
		mdelay(6);
		/* GPIO10 off  8ms -> clock -> off */
		REG32_VAL(GPIO_BASE_ADDR + 0xC0) &= ~0x400;
	}
}

irqreturn_t vpp_interrupt_routine(int irq, void *dev_id)
{
	vpp_int_t int_sts;

	switch (irq) {
	case VPP_IRQ_VPPM: /* VPP */
		int_sts = p_vppm->get_sts();
		p_vppm->clr_sts(int_sts);

		vpp_dbg_show_val1(VPP_DBGLVL_INT, 0, "[VPP] VPPM INT", int_sts);
		{
		int i;
		unsigned int mask;
		vpp_irqproc_t *irqproc;

		for (i = 0, mask = 0x1; (i < 32) && int_sts; i++, mask <<= 1) {
			if ((int_sts & mask) == 0)
				continue;

			irqproc = vpp_irqproc_get_entry(mask);
			if (irqproc) {
				if (list_empty(&irqproc->list) == 0)
					tasklet_schedule(&irqproc->tasklet);
			} else {
				irqproc = vpp_irqproc_get_entry(VPP_INT_MAX);
				if (list_empty(&irqproc->list) == 0) {
					vpp_proc_t *entry;
					struct list_head *ptr;

					ptr = (&irqproc->list)->next;
					entry = list_entry(ptr, vpp_proc_t,
						list);
					if (entry->type == mask)
						tasklet_schedule(
							&irqproc->tasklet);
				}
			}
			int_sts &= ~mask;
		}
		}
		break;
#ifdef WMT_FTBLK_SCL
	case VPP_IRQ_SCL: /* SCL */
		int_sts = p_scl->get_sts();
		p_scl->clr_sts(int_sts);
		vpp_dbg_show_val1(VPP_DBGLVL_INT, 0, "[VPP] SCL INT", int_sts);
		break;
#endif
#ifdef WMT_FTBLK_GOVRH
	case VPP_IRQ_GOVR:	/* GOVR */
	case VPP_IRQ_GOVR2:
		{
		govrh_mod_t *govr;

		govr = (irq == VPP_IRQ_GOVR) ? p_govrh : p_govrh2;
		int_sts = govr->get_sts();
		govr->clr_sts(int_sts);
		vpp_dbg_show_val1(VPP_DBGLVL_INT, 0, "[VPP] GOVR INT", int_sts);
		govr->underrun_cnt++;
#ifdef VPP_DBG_DIAG_NUM
		vpp_dbg_show(VPP_DBGLVL_DIAG, 3, "GOVR MIF Err");
		vpp_dbg_diag_delay = 10;
#endif
		}
		break;
#endif
	default:
		DPRINT("*E* invalid vpp isr\n");
		break;
	}
	return IRQ_HANDLED;
} /* End of vpp_interrupt_routine */

int vpp_dev_init(void)
{
	g_vpp.alloc_framebuf = vpp_alloc_framebuffer;
	vpp_irqproc_init();
	vpp_netlink_init();
	vpp_init();

#ifdef CONFIG_VPP_PROC
	/* init system proc */
	if (vpp_proc_dir == 0) {
		struct proc_dir_entry *res;

		vpp_proc_dir = proc_mkdir("driver/vpp", NULL);
		res = create_proc_entry("sts", 0, vpp_proc_dir);
		if (res)
			res->read_proc = vpp_sts_read_proc;
		res = create_proc_entry("reg", 0, vpp_proc_dir);
		if (res)
			res->read_proc = vpp_reg_read_proc;
		vpp_table_header = register_sysctl_table(vpp_root_table);
	}
#endif

	/* init interrupt service routine */
#ifdef WMT_FTBLK_SCL
	if (vpp_request_irq(VPP_IRQ_SCL, vpp_interrupt_routine,
		SA_INTERRUPT, "scl", (void *)&g_vpp)) {
		DPRINT("*E* request VPP ISR fail\n");
		return -1;
	}
#endif
	if (vpp_request_irq(VPP_IRQ_VPPM, vpp_interrupt_routine,
		SA_INTERRUPT, "vpp", (void *)&g_vpp)) {
		DPRINT("*E* request VPP ISR fail\n");
		return -1;
	}
#ifdef WMT_FTBLK_GOVRH
	if (vpp_request_irq(VPP_IRQ_GOVR, vpp_interrupt_routine,
		SA_INTERRUPT, "govr", (void *)&g_vpp)) {
		DPRINT("*E* request VPP ISR fail\n");
		return -1;
	}

	if (vpp_request_irq(VPP_IRQ_GOVR2, vpp_interrupt_routine,
		SA_INTERRUPT, "govr2", (void *)&g_vpp)) {
		DPRINT("*E* request VPP ISR fail\n");
		return -1;
	}
#endif
	vpp_switch_state_init();
	return 0;
}

int vpp_exit(struct fb_info *info)
{
	DBG_MSG("vpp_exit\n");

	vout_exit();
#ifdef CONFIG_VPP_PROC
	unregister_sysctl_table(vpp_table_header);
#endif
	return 0;
}

void vpp_wait_vsync(int no, int cnt)
{
	int govr_mask = 0;

	if (g_vpp.virtual_display || (g_vpp.dual_display == 0)) {
		if (govrh_get_MIF_enable(p_govrh))
			govr_mask |= BIT0;
		if (govrh_get_MIF_enable(p_govrh2))
			govr_mask |= BIT1;
	} else {
		vout_info_t *vo_info;
		vout_t *vo;
		int vo_mask;
		int i;

		vo_info = vout_info_get_entry(no);
		vo_mask = vout_get_mask(vo_info);
		for (i = 0; i < VPP_VOUT_NUM; i++) {
			if ((vo_mask & (0x1 << i)) == 0)
				continue;
			vo = vout_get_entry(i);
			if (vo) {
				if (vo->govr == p_govrh)
					govr_mask |= BIT0;
				if (vo->govr == p_govrh2)
					govr_mask |= BIT1;
			}
		}
	}

	if (govr_mask) {
#if 0
		if (vpp_check_dbg_level(VPP_DBGLVL_FPS))
			MSG("[VPP] vpp_wait_vsync %d\n", no);
#endif
		vpp_irqproc_work((govr_mask & BIT0) ? VPP_INT_GOVRH_VBIS :
			VPP_INT_GOVRH2_VBIS, 0, 0, 100 * cnt, cnt);
	}
} /* End of vpp_wait_vsync */

/* struct list_head *vpp_modelist; */
void vpp_get_info(int fbn, struct fb_var_screeninfo *var)
{
	static int vpp_init_flag = 1;
	vout_info_t *info;

	if (vpp_init_flag) {
#if 0
		int num;

		INIT_LIST_HEAD(vpp_modelist);
		for (num = 0; ; num++) {
			if (vpp_videomode[num].xres == 0)
				break;
		}
		fb_videomode_to_modelist(vpp_videomode, num, vpp_modelist);
#endif
		vpp_dev_init();
		vpp_init_flag = 0;
	}

	info = vout_info_get_entry(fbn);
	if (info->govr) {
		struct fb_videomode vmode;

		govrh_get_framebuffer(info->govr, &info->fb);
		govrh_get_videomode(info->govr, &vmode);
		fb_videomode_to_var(var, &vmode);
	} else {
		var->pixclock =
			KHZ2PICOS((info->resx * info->resy * 60) / 1000);
	}
	var->xres = info->resx;
	var->yres = info->resy;
	var->xres_virtual = info->fb.fb_w;
	var->yres_virtual = var->yres * VPP_MB_ALLOC_NUM;
	if (g_vpp.mb_colfmt == VDO_COL_FMT_ARGB) {
		var->bits_per_pixel = 32;
		var->red.offset = 16;
		var->red.length = 8;
		var->red.msb_right = 0;
		var->green.offset = 8;
		var->green.length = 8;
		var->green.msb_right = 0;
		var->green.offset = 0;
		var->green.length = 8;
		var->green.msb_right = 0;
		var->transp.offset = 24;
		var->transp.length = 8;
		var->transp.msb_right = 0;
	}
	DBG_MSG("(%d,%dx%d,%dx%d,%d,%d)\n", fbn, var->xres, var->yres,
		var->xres_virtual, var->yres_virtual,
		var->pixclock, var->bits_per_pixel);
#ifdef DEBUG
	wmtfb_show_var("get_info", var);
#endif
}

void vpp_var_to_fb(struct fb_var_screeninfo *var,
				struct fb_info *info, vdo_framebuf_t *fb)
{
	extern unsigned int fb_egl_swap;
	unsigned int addr;
	int y_bpp, c_bpp;

	if (var) {
		fb->col_fmt = WMT_FB_COLFMT(var->nonstd);
		if (!fb->col_fmt) {
			switch (var->bits_per_pixel) {
			case 16:
				if ((info->var.red.length == 5) &&
					(info->var.green.length == 6) &&
					(info->var.blue.length == 5)) {
					fb->col_fmt = VDO_COL_FMT_RGB_565;
				} else if ((info->var.red.length == 5) &&
					(info->var.green.length == 5) &&
					(info->var.blue.length == 5)) {
					fb->col_fmt = VDO_COL_FMT_RGB_1555;
				} else {
					fb->col_fmt = VDO_COL_FMT_RGB_5551;
				}
				break;
			case 32:
				fb->col_fmt = VDO_COL_FMT_ARGB;
				break;
			default:
				fb->col_fmt = VDO_COL_FMT_RGB_565;
				break;
			}
			y_bpp = var->bits_per_pixel;
			c_bpp = 0;
		} else {
			y_bpp = 8;
			c_bpp = 8;
		}
		var->xres_virtual = vpp_calc_fb_width(fb->col_fmt, var->xres);

		fb->img_w = var->xres;
		fb->img_h = var->yres;
		fb->fb_w = var->xres_virtual;
		fb->fb_h = var->yres_virtual;
		fb->h_crop = 0;
		fb->v_crop = 0;
		fb->flag = 0;
		fb->bpp = var->bits_per_pixel;

		addr = info->fix.smem_start +
		(var->yoffset * var->xres_virtual * ((y_bpp + c_bpp) >> 3));
		addr += var->xoffset * ((y_bpp) >> 3);
		fb->y_addr = addr;
		fb->y_size = var->xres_virtual * var->yres * (y_bpp >> 3);
		fb->c_addr = fb->y_addr + fb->y_size;
		fb->c_size = var->xres_virtual * var->yres * (c_bpp >> 3);
	}

	if (info && (info->node == 0) && (fb_egl_swap != 0)) /* for Android */
		fb->y_addr = fb_egl_swap;

}

int vpp_pan_display(struct fb_var_screeninfo *var, struct fb_info *info)
{
	vout_info_t *vo_info;

	if (g_vpp.hdmi_certify_flag)
		return 0;

	if (wmtfb_fb1_probe == 0)
		return 0;

	vo_info = vout_info_get_entry((info) ? info->node : 0);

	DBG_DETAIL("fb %d,govr %d\n", (info) ? info->node : 0,
		vo_info->govr->mod);

	vpp_var_to_fb(var, info, &vo_info->fb);
	vout_set_framebuffer(vout_get_mask(vo_info), &vo_info->fb);

	if (g_vpp.fb0_bitblit) {
		vdo_framebuf_t src, dst;

		info = vout_info_get_entry(1);
		g_vpp.stream_mb_index = (g_vpp.stream_mb_index) ? 0 : 1;
		p_scl->scale_sync = 1;
		src = vo_info->fb;
		govrh_get_framebuffer(p_govrh, &dst);
		dst.fb_w = vpp_calc_align(dst.fb_w, 64);
		dst.y_size = dst.fb_w * dst.img_h * dst.bpp / 8;
#ifdef CONFIG_VPP_DYNAMIC_ALLOC
		if (g_vpp.mb[0] == 0) {
			vpp_alloc_framebuffer(dst.img_w, dst.img_h);
		}
#endif
		dst.y_addr = g_vpp.mb[0] + (dst.y_size * g_vpp.stream_mb_index);
		dst.c_addr = 0;
//		MSG("fb0 bitblit Y 0x%x,C 0x%x,(%dx%d)\n",
//			dst.y_addr, dst.c_addr, dst.fb_w, dst.img_h);
		vpp_set_recursive_scale(&src, &dst);
		vout_set_framebuffer(VPP_VOUT_ALL, &dst);
	}

#ifdef CONFIG_VPP_STREAM_CAPTURE
	if (g_vpp.stream_enable) {
#ifdef CONFIG_VPP_STREAM_ROTATE
		if (info && (info->node == 1)) {
			g_vpp.stream_mb_index = var->yoffset / var->yres;
			vpp_dbg_show_val1(VPP_DBGLVL_STREAM, 0,
				"stream pan disp", g_vpp.stream_mb_index);
		}
#else
		if ((info && (info->node == 0)) || (info == 0))
			vpp_mb_scale_bitblit(&vo_info->fb);
#endif
	}
#endif
	if (vo_info->govr && vpp_check_dbg_level(VPP_DBGLVL_DISPFB)) {
		char buf[50];
		unsigned int yaddr, caddr;

		govrh_get_fb_addr(vo_info->govr, &yaddr, &caddr);
		sprintf(buf, "pan_display %d,%s,0x%x", vo_info->num,
			vpp_mod_str[vo_info->govr_mod], yaddr);
		vpp_dbg_show(VPP_DBGLVL_DISPFB, vo_info->num + 1, buf);
	}

	if (vo_info->govr && vpp_check_dbg_level(VPP_DBGLVL_FPS))
		vpp_dbg_timer(&vo_info->pandisp_timer,
			(vo_info->num == 0) ? "fb0" : "fb1", 2);
	return 0;
}

int vpp_set_par(struct fb_info *info)
{
	vout_info_t *vo_info;
	unsigned int mask;

	if (g_vpp.hdmi_certify_flag)
		return 0;

	if ((g_vpp.dual_display == 0) && (info->node == 1))
		return 0;

	if (g_vpp.govrh_preinit)
		g_vpp.govrh_preinit = 0;

	vpp_set_mutex(info->node, 1);

	vo_info = vout_info_get_entry(info->node);
	mask = vout_get_mask(vo_info);

	/* set frame buffer */
	if (mask) {
		vdo_framebuf_t fb;

		vpp_var_to_fb(&info->var, info, &fb);
		vo_info->fb.fb_h = fb.fb_h;
		if (memcmp(&fb.img_w, &vo_info->fb.img_w, 32)) {
#ifdef DEBUG
			DPRINT("[wmtfb_set_par] set_par %d : set framebuf\n",
				info->node);
			vpp_show_framebuf("cur", &vo_info->fb);
			vpp_show_framebuf("new", &fb);
#endif
			vo_info->fb = fb;
			vout_set_framebuffer(mask, &vo_info->fb);
		}
	}

	/* set timing */
	if (vo_info->govr && !(g_vpp.virtual_display && (info->node == 0))) {
		struct fb_videomode var, cur;
		govrh_mod_t *govr;
		unsigned int cur_pixclk, new_pixclk;

		if (g_vpp.virtual_display)
			govr = (vout_chkplug(VPP_VOUT_NUM_HDMI)) ?
				p_govrh : p_govrh2;
		else
			govr = vo_info->govr;
		fb_var_to_videomode(&var, &info->var);
		govrh_get_videomode(govr, &cur);
		/* diff less then 500K */
		cur_pixclk = PICOS2KHZ(cur.pixclock);
		new_pixclk = PICOS2KHZ(var.pixclock);
		if (abs(new_pixclk - cur_pixclk) < 500) {
			var.pixclock = cur.pixclock;
			var.refresh = cur.refresh;
		}
		if (abs(var.refresh - cur.refresh) <= 2) /* diff less then 2 */
			var.refresh = cur.refresh;
		if (memcmp(&var, &cur, sizeof(struct fb_videomode))) {
#ifdef DEBUG
			DPRINT("[wmtfb] set_par %d: set timing\n", info->node);
			vpp_show_timing("cur", &cur, 0);
			vpp_show_timing("new", &var, 0);
#endif
			vout_config(mask, vo_info, &var);
		}
	}
	vpp_set_mutex(info->node, 0);
	return 0;
}

static int wmtfb_open
(
	struct fb_info *info,   /*!<; // a pointer point to struct fb_info */
	int user                /*!<; // user space mode */
)
{
	DBG_MSG("Enter wmtfb_open\n");
	return 0;
} /* End of wmtfb_open */

static int wmtfb_release
(
	struct fb_info *info,   /*!<; // a pointer point to struct fb_info */
	int user                /*!<; // user space mode */
)
{
	DBG_MSG("Enter wmtfb_release\n");
	return 0;
} /* End of wmtfb_release */

int wmtfb_check_var(struct fb_var_screeninfo *var, struct fb_info *info)
{
	int temp;
	int force = 0;

	DBG_DETAIL("Enter %d\n", info->node);

	var->xres_virtual = vpp_calc_fb_width((var->bits_per_pixel == 16) ?
		VDO_COL_FMT_RGB_565 : VDO_COL_FMT_ARGB, var->xres);

	if ((info->node == 1) && g_vpp.alloc_framebuf) {
		if( (g_vpp.mb[0] == 0) || ((var->xres != info->var.xres) ||
			(var->yres != info->var.yres))) {
			if (var->nonstd) {
				g_vpp.mb_colfmt = WMT_FB_COLFMT(var->nonstd);
			} else {
				g_vpp.mb_colfmt = (var->bits_per_pixel == 16) ?
					VDO_COL_FMT_RGB_565 : VDO_COL_FMT_ARGB;
			}
			if (g_vpp.alloc_framebuf(var->xres, var->yres))
				return -ENOMEM;
			info->fix.smem_start = g_vpp.mb[0];
			info->fix.smem_len =
				g_vpp.mb_fb_size * VPP_MB_ALLOC_NUM;
			info->screen_base =
				mb_phys_to_virt(info->fix.smem_start);
		}
	}
	temp = (info->fix.smem_len /
		(var->xres_virtual * var->yres * (var->bits_per_pixel >> 3)));
	if (temp < 2) {
		DBG_MSG("smem_len %d,%d\n", info->fix.smem_len, temp);
		temp = 2;
	}
	if (var->yres_virtual > (var->yres * temp))
		var->yres_virtual = var->yres * temp;

	/* more than 1M is khz not picos (for ut_vpp) */
	if (var->pixclock > 1000000) {
		temp = KHZ2PICOS(var->pixclock / 1000);
		DBG_MSG("pixclock patch(>1000000)%d-->%d(%d)\n",
			var->pixclock, temp, (int)PICOS2KHZ(temp) * 1000);
		var->pixclock = temp;
	}

	/* less than 100 is fps not picos (for ut_vpp) */
	if ((var->pixclock > 0) && (var->pixclock < 100)) {
		unsigned int htotal, vtotal;

		htotal = var->xres + var->right_margin + var->hsync_len +
			var->left_margin;
		vtotal = var->yres + var->lower_margin + var->vsync_len +
			var->upper_margin;
		temp = htotal * vtotal * var->pixclock;
		temp = KHZ2PICOS(temp / 1000);
		DBG_MSG("pixclock patch(<100)%d-->%d(%d)\n",
			var->pixclock, temp, (int)PICOS2KHZ(temp) * 1000);
		var->pixclock = temp;
	}

#ifdef DEBUG_DETAIL
	wmtfb_show_var("cur var", &info->var);
	wmtfb_show_var("new var", var);
#endif
	switch (var->bits_per_pixel) {
	case 1:
	case 8:
		if (var->red.offset > 8) {
			var->red.offset = 0;
			var->red.length = 8;
			var->green.offset = 0;
			var->green.length = 8;
			var->blue.offset = 0;
			var->blue.length = 8;
			var->transp.offset = 0;
			var->transp.length = 0;
		}
		break;
	case 16:                /* ARGB 1555 */
		if (var->transp.length) {
			var->red.offset = 10;
			var->red.length = 5;
			var->green.offset = 5;
			var->green.length = 5;
			var->blue.offset = 0;
			var->blue.length = 5;
			var->transp.offset = 15;
			var->transp.length = 1;
		} else {        /* RGB 565 */
			var->red.offset = 11;
			var->red.length = 5;
			var->green.offset = 5;
			var->green.length = 6;
			var->blue.offset = 0;
			var->blue.length = 5;
			var->transp.offset = 0;
			var->transp.length = 0;
		}
		break;
	case 24:                /* RGB 888 */
	case 32:                /* ARGB 8888 */
		var->red.offset = 16;
		var->red.length = 8;
		var->green.offset = 8;
		var->green.length = 8;
		var->blue.offset = 0;
		var->blue.length = 8;
		var->transp.offset = 0;
		var->transp.length = 0;
		break;
	}

	if (g_vpp.fb_manual)
		return 0;

	if (g_vpp.fb_recheck & (0x1 << info->node)) {
		force = 1;
		g_vpp.fb_recheck &= ~(0x1 << info->node);
	}

	if ((var->xres != info->var.xres) || (var->yres != info->var.yres) ||
		memcmp(&info->var.pixclock, &var->pixclock, 4 * 9) || force) {
		struct fb_videomode varfbmode;
		unsigned int yres_virtual;

		DPRINT("[wmtfb_check_var] fb%d res(%d,%d)->(%d,%d),force %d\n",
			info->node, info->var.xres, info->var.yres,
			var->xres, var->yres, force);
#ifdef DEBUG
		wmtfb_show_var("cur var", &info->var);
		wmtfb_show_var("new var", var);
#endif
		yres_virtual = var->yres_virtual;
		fb_var_to_videomode(&varfbmode, var);
#ifdef DEBUG
		DPRINT("new fps %d\n", varfbmode.refresh);
#endif
		if (vout_find_match_mode(info->node, &varfbmode, 1)) {
			DPRINT("[wmtfb] not support\n");
			return -1;
		}
		fb_videomode_to_var(var, &varfbmode);
		var->yres_virtual = yres_virtual;
#ifdef DEBUG
		wmtfb_show_var("[wmtfb] time change", var);
#endif
	}
	return 0;
} /* End of wmtfb_check_var */

static int wmtfb_set_par
(
	struct fb_info *info /*!<; // a pointer point to struct fb_info */
)
{
	struct fb_var_screeninfo *var = &info->var;

	DBG_DETAIL("Enter fb%d(%dx%d)\n", info->node, var->xres, var->yres);

	/* init your hardware here */
	/* ... */
	if (var->bits_per_pixel == 8)
		info->fix.visual = FB_VISUAL_PSEUDOCOLOR;
	else
		info->fix.visual = FB_VISUAL_TRUECOLOR;
	info->node = 1;
	vpp_set_par(info);
	info->fix.line_length = var->xres_virtual * var->bits_per_pixel / 8;
	return 0;
} /* End of vt8430fb_set_par */

static int wmtfb_setcolreg
(
	unsigned regno,         /*!<; // register no */
	unsigned red,           /*!<; // red color map */
	unsigned green,         /*!<; // green color map */
	unsigned blue,          /*!<; // blue color map */
	unsigned transp,        /*!<; // transp map */
	struct fb_info *info    /*!<; // a pointer point to struct fb_info */
)
{
	return 0;

} /* End of wmtfb_setcolreg */

static int wmtfb_pan_display
(
	struct fb_var_screeninfo *var,  /*!<; // a pointer fb_var_screeninfo */
	struct fb_info *info            /*!<; // a pointer fb_info */
)
{
	static struct timeval tv1 = {0, 0};

	DBG_DETAIL("Enter wmtfb_pan_display\n");

	vpp_set_mutex(1, 1);
	if (var->activate & FB_ACTIVATE_VBL) {
		struct timeval tv2;

		do_gettimeofday(&tv2);
		if (tv1.tv_sec) {
			int us;

			us = (tv2.tv_sec == tv1.tv_sec) ?
				(tv2.tv_usec - tv1.tv_usec) :
				(1000000 + tv2.tv_usec - tv1.tv_usec);
			if (us < 16667)
				vpp_wait_vsync(1, 1);
		}
	}
	vpp_pan_display(var, info);
	do_gettimeofday(&tv1);
	vpp_set_mutex(1, 0);

	DBG_DETAIL("Exit wmtfb_pan_display\n");
	return 0;
} /* End of wmtfb_pan_display */

static int wmtfb_ioctl
(
	struct fb_info *info,    /*!<; // a pointer point to struct fb_info */
	unsigned int cmd,       /*!<; // ioctl command */
	unsigned long arg       /*!<; // a argument pointer */
)
{
	int retval = 0;

/*	printk("Enter wmtfb_ioctl %x\n",cmd); */
	if (_IOC_TYPE(cmd) != VPPIO_MAGIC)
		return retval;

	unlock_fb_info(info);
	retval = vpp_ioctl(cmd, arg);
	lock_fb_info(info);
	return retval;
} /* End of wmtfb_ioctl */

static int wmtfb_mmap
(
	struct fb_info *info,           /*!<; // a pointer fb_info */
	struct vm_area_struct *vma      /*!<; // a pointer vm_area_struct */
)
{
	unsigned long off;
	unsigned long start;
	u32 len;

	DBGMSG("Enter wmtfb_mmap\n");

	/* frame buffer memory */
	start = info->fix.smem_start;
	off = vma->vm_pgoff << PAGE_SHIFT;
	len = PAGE_ALIGN((start & ~PAGE_MASK) + info->fix.smem_len);
	if (off >= len) {
		/* memory mapped io */
		off -= len;
		if (info->var.accel_flags)
			return -EINVAL;
		start = info->fix.mmio_start;
		len = PAGE_ALIGN((start & ~PAGE_MASK) + info->fix.mmio_len);
	}

	start &= PAGE_MASK;
	if ((vma->vm_end - vma->vm_start + off) > len)
		return -EINVAL;
	off += start;
	vma->vm_pgoff = off >> PAGE_SHIFT;
	vma->vm_flags |= VM_IO | VM_RESERVED;
	vma->vm_page_prot = pgprot_writecombine(vma->vm_page_prot);
	if (remap_pfn_range(vma, vma->vm_start, vma->vm_pgoff,
		vma->vm_end - vma->vm_start, vma->vm_page_prot))
		return -EAGAIN;
	return 0;
} /* End of wmtfb_mmap */

int wmtfb_hw_cursor(struct fb_info *info, struct fb_cursor *cursor)
{
	return 0;
} /* End of wmtfb_hw_cursor */

int wmtfb_blank(int blank, struct fb_info *info)
{
	DBGMSG("(%d,%d)\n", info->node, blank);
	vpp_set_blank(info, blank);
	return 0;
}

/***************************************************************************
	driver file operations struct define
****************************************************************************/
static struct fb_ops wmtfb_ops = {
	.owner          = THIS_MODULE,
	.fb_open        = wmtfb_open,
	.fb_release     = wmtfb_release,
#if 0
	.fb_read      = wmtfb_read,
	.fb_write     = wmtfb_write,
#endif
	.fb_check_var   = wmtfb_check_var,
	.fb_set_par     = wmtfb_set_par,
	.fb_setcolreg   = wmtfb_setcolreg,
	.fb_pan_display = wmtfb_pan_display,
	.fb_fillrect    = cfb_fillrect,
	.fb_copyarea    = cfb_copyarea,
	.fb_imageblit   = cfb_imageblit,
	.fb_blank       = wmtfb_blank,
	.fb_cursor      = wmtfb_hw_cursor,
	.fb_ioctl       = wmtfb_ioctl,
	.fb_mmap        = wmtfb_mmap,
};

static int __init wmtfb_probe
(
	struct platform_device *dev /*!<; // a pointer point to struct device */
)
{
	struct fb_info *info;
	int cmap_len;
	u32 map_size;
	char mode_option[20];

	sprintf(mode_option, "%dx%d@%d",
		VPP_HD_DISP_RESX, VPP_HD_DISP_RESY, VPP_HD_DISP_FPS);

	/*  Dynamically allocate memory for fb_info and par.*/
	info = framebuffer_alloc(sizeof(u32) * 16, &dev->dev);
	if (!info) {
		release_mem_region(info->fix.smem_start, info->fix.smem_len);
		return -ENOMEM;
	}

	/* Set default fb_info */
	info->fbops = &wmtfb_ops;
	info->fix = wmtfb_fix;

#if 1
	if (g_vpp.alloc_framebuf) {
		info->fix.smem_start = g_vpp.mb[0];
		info->fix.smem_len = g_vpp.mb_fb_size * VPP_MB_ALLOC_NUM;
		info->screen_base = mb_phys_to_virt(info->fix.smem_start);
	}
#endif
#if 0
	/* Set video memory */
	if (!request_mem_region(info->fix.smem_start,
		info->fix.smem_len, "wmtfb")) {
		printk(KERN_WARNING
			"wmtfb: abort, cannot reserve video memory at 0x%lx\n",
			info->fix.smem_start);
	}

	info->screen_base = ioremap(info->fix.smem_start, info->fix.smem_len);
	if (!info->screen_base) {
		printk(KERN_ERR
		"wmtfb: abort, cannot ioremap video memory 0x%x @ 0x%lx\n",
			info->fix.smem_len, info->fix.smem_start);
		return -EIO;
	}
#endif
	printk(KERN_INFO "wmtfb: framebuffer at 0x%lx, mapped to 0x%p, "
		"using %d, total %d\n",
		info->fix.smem_start, info->screen_base,
		info->fix.smem_len, info->fix.smem_len);

	/*
	 *  Do as a normal fbdev does, but allocate a larger memory for GE.
	 */
	map_size = PAGE_ALIGN(info->fix.smem_len);

	/*
	 * The pseudopalette is an 16-member array for fbcon.
	 */
	info->pseudo_palette = info->par;
	info->par = NULL;
	info->flags = FBINFO_DEFAULT; /* flag for fbcon. */

	/*
	 *  This has to been done !!!
	 */
	cmap_len = 256; /* Be the same as VESA. */
	fb_alloc_cmap(&info->cmap, cmap_len, 0);

	/*
	 *  The following is done in the case of
	 *  having hardware with a static mode.
	 */
	info->var = wmtfb_var;
	vpp_get_info(1, &info->var);

	/*
	 * This should give a reasonable default video mode.
	 */

	/*
	 *  For drivers that can...
	 */
	wmtfb_check_var(&info->var, info);

	/*
	 *  It's safe to allow fbcon to do it for you.
	 *  But in this case, we need it here.
	 */
	wmtfb_set_par(info);

	if (register_framebuffer(info) < 0)
		return -EINVAL;
	info->dev->power.async_suspend = 1;
	printk(KERN_INFO "fb%d: %s frame buffer device\n",
		info->node, info->fix.id);
	dev_set_drvdata(&dev->dev, info);
	wmtfb_fb1_probe = 1;
	return 0;
} /* End of wmtfb_probe */

static int wmtfb_remove
(
	struct platform_device *dev /*!<; // a pointer point to struct device */
)
{
	struct fb_info *info = dev_get_drvdata(&dev->dev);

	if (info) {
		unregister_framebuffer(info);
		fb_dealloc_cmap(&info->cmap);
		framebuffer_release(info);
	}
	return 0;
}

#ifdef CONFIG_PM
unsigned int vpp_vout_blank_mask;

static int wmtfb_suspend
(
	struct platform_device *pDev,   /*!<; // a pointer struct device */
	pm_message_t state		/*!<; // suspend state */
)
{
	vpp_mod_base_t *mod_p;
	vout_t *vo;
	int i;

	DBGMSG("Enter wmtfb_suspend\n");
	vpp_vout_blank_mask = 0;
	for (i = 0; i <= VPP_VOUT_NUM; i++) {
		vo = vout_get_entry(i);
		if (vo && !(vo->status & VPP_VOUT_STS_BLANK))
			vpp_vout_blank_mask |= (0x1 << i);
	}
	vout_set_blank(VPP_VOUT_ALL, VOUT_BLANK_POWERDOWN);
	if (vout_check_plugin(1))
		vpp_netlink_notify_plug(VPP_VOUT_ALL ,0);
	else
		wmt_set_mmfreq(0);

	/* disable module */
	for (i = 0; i < VPP_MOD_MAX; i++) {
		mod_p = vpp_mod_get_base(i);
		if (mod_p && mod_p->suspend)
			mod_p->suspend(0);
	}
#ifdef WMT_FTBLK_HDMI
	hdmi_suspend(0);
#endif
	wmt_suspend_mmfreq();
#ifdef WMT_FTBLK_LVDS
	lvds_suspend(0);
#endif
	/* disable tg */
	for (i = 0; i < VPP_MOD_MAX; i++) {
		mod_p = vpp_mod_get_base(i);
		if (mod_p && mod_p->suspend)
			mod_p->suspend(1);
	}
#ifdef WMT_FTBLK_HDMI
	hdmi_suspend(1);
#endif
#ifdef WMT_FTBLK_LVDS
	lvds_suspend(1);
#endif
	/* backup registers */
	for (i = 0; i < VPP_MOD_MAX; i++) {
		mod_p = vpp_mod_get_base(i);
		if (mod_p && mod_p->suspend)
			mod_p->suspend(2);
	}
#ifdef WMT_FTBLK_HDMI
	hdmi_suspend(2);
#endif
#ifdef WMT_FTBLK_LVDS
	lvds_suspend(2);
#endif
	if (lcd_get_lvds_id() == LCD_LVDS_1024x600) {
		mdelay(5);
		/* GPIO10 off  8ms -> clock -> off */
		REG32_VAL(GPIO_BASE_ADDR + 0xC0) &= ~0x400;
	}
	return 0;
} /* End of wmtfb_suspend */

static int wmtfb_resume
(
	struct platform_device *pDev	/*!<; // a pointer struct device */
)
{
	vpp_mod_base_t *mod_p;
	int i;

#if 0
	if (lcd_get_lvds_id() == LCD_LVDS_1024x600) {
		/* GPIO10 6ms -> clock r0.02.04 */
		REG32_VAL(GPIO_BASE_ADDR+0x80) |= 0x400;
		REG32_VAL(GPIO_BASE_ADDR+0xC0) |= 0x400;
	}
#endif

	DBGMSG("Enter wmtfb_resume\n");

	/* restore registers */
	for (i = 0; i < VPP_MOD_MAX; i++) {
		mod_p = vpp_mod_get_base(i);
		if (mod_p && mod_p->resume)
			mod_p->resume(0);
	}
#ifdef WMT_FTBLK_LVDS
	lvds_resume(0);
#endif
#ifdef WMT_FTBLK_HDMI
	hdmi_check_plugin(0);
	hdmi_resume(0);
#endif
	/* enable tg */
	for (i = 0; i < VPP_MOD_MAX; i++) {
		mod_p = vpp_mod_get_base(i);
		if (mod_p && mod_p->resume)
			mod_p->resume(1);
	}
#ifdef WMT_FTBLK_LVDS
	lvds_resume(1);
#endif
#ifdef WMT_FTBLK_HDMI
	hdmi_resume(1);
#endif
	/* wait */
	msleep(150);

	/* enable module */
	for (i = 0; i < VPP_MOD_MAX; i++) {
		mod_p = vpp_mod_get_base(i);
		if (mod_p && mod_p->resume)
			mod_p->resume(2);
	}
#ifdef WMT_FTBLK_LVDS
	lvds_resume(2);
#endif
#ifdef WMT_FTBLK_HDMI
	hdmi_resume(2);
#endif
	if (lcd_get_lvds_id() != LCD_LVDS_1024x600)
		vout_set_blank(vpp_vout_blank_mask, VOUT_BLANK_UNBLANK);
	wmt_resume_mmfreq();
	if (vout_check_plugin(0))
		vpp_netlink_notify_plug(VPP_VOUT_ALL, 1);
	else
		wmt_set_mmfreq(0);
	return 0;
} /* End of wmtfb_resume */

static void wmtfb_shutdown
(
	struct platform_device *pDev	/*!<; // a pointer struct device */
)
{
	DPRINT("wmtfb_shutdown\n");
	hdmi_set_power_down(1);
	lvds_set_power_down(1);
}

#else
#define wmtfb_suspend NULL
#define wmtfb_resume NULL
#define wmtfb_shutdown NULL
#endif

/***************************************************************************
	device driver struct define
****************************************************************************/
static struct platform_driver wmtfb_driver = {
	.driver.name    = "wmtfb",
	.driver.bus     = &platform_bus_type,
	.probe          = wmtfb_probe,
	.remove         = wmtfb_remove,
	.suspend        = wmtfb_suspend,
	.resume         = wmtfb_resume,
	.shutdown 	= wmtfb_shutdown,
};

/***************************************************************************
	platform device struct define
****************************************************************************/
static u64 wmtfb_dma_mask = 0xffffffffUL;
static struct platform_device wmtfb_device = {
	.name   = "wmtfb",
	.dev    = {
		.dma_mask = &wmtfb_dma_mask,
		.coherent_dma_mask = ~0,
		.power.async_suspend = 1,
	},

#if 0
	.id     = 0,
	.dev    = {
		.release = wmtfb_platform_release,
	},
	.num_resources  = 0,    /* ARRAY_SIZE(wmtfb_resources), */
	.resource       = NULL, /* wmtfb_resources, */
#endif
};

static int __init wmtfb_init(void)
{
	int ret;

	/*
	 *  For kernel boot options (in 'video=wmtfb:<options>' format)
	 */
	ret = platform_driver_register(&wmtfb_driver);
	if (!ret) {
		ret = platform_device_register(&wmtfb_device);
		if (ret)
			platform_driver_unregister(&wmtfb_driver);
	}
	return ret;

} /* End of wmtfb_init */
module_init(wmtfb_init);

static void __exit wmtfb_exit(void)
{
	printk(KERN_ALERT "Enter wmtfb_exit\n");

	platform_driver_unregister(&wmtfb_driver);
	platform_device_unregister(&wmtfb_device);
	return;
} /* End of wmtfb_exit */
module_exit(wmtfb_exit);

MODULE_AUTHOR("WonderMedia SW Team");
MODULE_DESCRIPTION("wmtfb device driver");
MODULE_LICENSE("GPL");
/*--------------------End of Function Body -----------------------------------*/
#undef WMTFB_C
