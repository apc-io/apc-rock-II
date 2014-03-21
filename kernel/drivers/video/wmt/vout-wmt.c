/*++
 * linux/drivers/video/wmt/vout-wmt.c
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
#undef DEBUG
/* #define DEBUG */
/* #define DEBUG_DETAIL */

#include "vpp.h"

#ifndef CFG_LOADER
static int vo_plug_flag;
#endif
int vo_plug_vout;
int (*vo_plug_func)(int hotplug);
vout_mode_t dvo_vout_mode;
vout_mode_t int_vout_mode;
struct fb_videomode vo_oem_vmode;
int hdmi_cur_plugin;
vout_t *vo_poll_vout;

/* GPIO 10 & 11 */
swi2c_reg_t vo_gpio_scl = {
	.bit_mask = BIT10,
	.gpio_en = (__GPIO_BASE + 0x40),
	.out_en = (__GPIO_BASE + 0x80),
	.data_in = (__GPIO_BASE + 0x00),
	.data_out = (__GPIO_BASE + 0xC0),
	.pull_en = (__GPIO_BASE + 0x480),
	.pull_en_bit_mask = BIT10,
};

swi2c_reg_t vo_gpio_sda = {
	.bit_mask = BIT11,
	.gpio_en = (__GPIO_BASE + 0x40),
	.out_en = (__GPIO_BASE + 0x80),
	.data_in = (__GPIO_BASE + 0x00),
	.data_out = (__GPIO_BASE + 0xC0),
	.pull_en = (__GPIO_BASE + 0x480),
	.pull_en_bit_mask = BIT11,
};

swi2c_handle_t vo_swi2c_dvi = {
	.scl_reg = &vo_gpio_scl,
	.sda_reg = &vo_gpio_sda,
};

struct vout_init_parm_t {
	unsigned int virtual_display;
	unsigned int def_resx;
	unsigned int def_resy;
	unsigned int def_fps;
	unsigned int ub_resx;
	unsigned int ub_resy;
};

#define DVI_POLL_TIME_MS	100

extern void hdmi_config_audio(vout_audio_t *info);
extern vout_dev_t *lcd_get_dev(void);

/*---------------------------------- API ------------------------------------*/
int vo_i2c_proc(int id, unsigned int addr, unsigned int index,
			char *pdata, int len)
{
	swi2c_handle_t *handle = 0;
	int ret = 0;

	switch (id) {
	case 1:	/* dvi */
		if (lcd_get_type())	/* share pin with LVDS */
			return -1;
		handle = &vo_swi2c_dvi;
		break;
	default:
		break;
	}

	if (handle) {
		if (wmt_swi2c_check(handle))
			return -1;
		if (addr & 0x1) { /* read */
			*pdata = 0xff;
#ifdef CONFIG_WMT_EDID
			ret = wmt_swi2c_read(handle, addr & ~0x1,
						index, pdata, len);
#else
			ret = -1;
#endif
		} else { /* write */
			DBG_ERR("not support sw i2c write\n");
		}
	}
	return ret;
}

#ifndef CONFIG_UBOOT
static void vo_do_plug(struct work_struct *ptr)
{
	vout_t *vo;
	int plugin;

	if (vo_plug_func == 0)
		return;

	vo = vout_get_entry(vo_plug_vout);
	govrh_set_dvo_enable(vo->govr, 1);
	plugin = vo_plug_func(1);
	govrh_set_dvo_enable(vo->govr, plugin);
	vout_change_status(vo, VPP_VOUT_STS_PLUGIN, plugin);
	vo_plug_flag = 0;
	DBG_DETAIL("vo_do_plug %d\n", plugin);
	/* GPIO irq enable */
	vppif_reg32_write(GPIO_BASE_ADDR + 0x300 + VPP_VOINT_NO, 0x80, 7, 1);
	/* GPIO input mode */
	vppif_reg32_write(GPIO_BASE_ADDR + 0x80, 0x1 << VPP_VOINT_NO,
				VPP_VOINT_NO, 0x0);
#ifdef __KERNEL__
	vpp_netlink_notify_plug(VPP_VOUT_NUM_DVI, plugin);
#endif
	return;
}

DECLARE_DELAYED_WORK(vo_plug_work, vo_do_plug);

static irqreturn_t vo_plug_interrupt_routine
(
	int irq,		/*!<; // irq id */
	void *dev_id		/*!<; // device id */
)
{
	DBG_DETAIL("Enter\n");
	if ((vppif_reg8_in(GPIO_BASE_ADDR + 0x360) &
		(0x1 << VPP_VOINT_NO)) == 0)
		return IRQ_NONE;

	/* clear int status */
	vppif_reg8_out(GPIO_BASE_ADDR + 0x360, 0x1 << VPP_VOINT_NO);
#ifdef __KERNEL__
	/* if (vo_plug_flag == 0) { */
	/* GPIO irq disable */
	vppif_reg32_write(GPIO_BASE_ADDR + 0x300 + VPP_VOINT_NO, 0x80, 7, 0);
	schedule_delayed_work(&vo_plug_work, HZ/5);
	vo_plug_flag = 1;
	/* } */
#else
	if (vo_plug_func)
		vo_do_plug(0);
#endif
	return IRQ_HANDLED;
}

#define CONFIG_VO_POLL_WORKQUEUE
struct timer_list vo_poll_timer;
#ifdef CONFIG_VO_POLL_WORKQUEUE
static void vo_do_poll(struct work_struct *ptr)
{
	vout_t *vo;

	vo = vo_poll_vout;
	if (vo) {
		if (vo->dev)
			vo->dev->poll();
		mod_timer(&vo_poll_timer,
			jiffies + msecs_to_jiffies(vo_poll_timer.data));
	}
	return;
}

DECLARE_DELAYED_WORK(vo_poll_work, vo_do_poll);
#else
struct tasklet_struct vo_poll_tasklet;
static void vo_do_poll_tasklet
(
	unsigned long data		/*!<; // tasklet input data */
)
{
	vout_t *vo;

	vpp_lock();
	vo = vo_poll_vout;
	if (vo) {
		if (vo->dev)
			vo->dev->poll();
		mod_timer(&vo_poll_timer,
			jiffies + msecs_to_jiffies(vo_poll_timer.data));
	}
	vpp_unlock();
}
#endif

void vo_do_poll_tmr(int ms)
{
#ifdef CONFIG_VO_POLL_WORKQUEUE
	schedule_delayed_work(&vo_poll_work, msecs_to_jiffies(ms));
#else
	tasklet_schedule(&vo_poll_tasklet);
#endif
}

static void vo_set_poll(vout_t *vo, int on, int ms)
{
	if (on) {
		vo_poll_vout = vo;
		if (vo_poll_timer.function) {
			vo_poll_timer.data = ms / 2;
			mod_timer(&vo_poll_timer,
				jiffies + msecs_to_jiffies(vo_poll_timer.data));
		} else {
			init_timer(&vo_poll_timer);
			vo_poll_timer.data = ms / 2;
			vo_poll_timer.function = (void *) vo_do_poll_tmr;
			vo_poll_timer.expires = jiffies +
				msecs_to_jiffies(vo_poll_timer.data);
			add_timer(&vo_poll_timer);
		}
#ifndef CONFIG_VO_POLL_WORKQUEUE
		tasklet_init(&vo_poll_tasklet, vo_do_poll_tasklet, 0);
#endif
	} else {
		del_timer(&vo_poll_timer);
#ifndef CONFIG_VO_POLL_WORKQUEUE
		tasklet_kill(&vo_poll_tasklet);
#endif
		vo_poll_vout = 0;
	}
}
#endif

void vout_set_int_type(int type)
{
	unsigned char reg;

	reg = vppif_reg8_in(GPIO_BASE_ADDR+0x300 + VPP_VOINT_NO);
	reg &= ~0x7;
	switch (type) {
	case 0:	/* low level */
	case 1:	/* high level */
	case 2:	/* falling edge */
	case 3:	/* rising edge */
	case 4:	/* rising edge or falling */
		reg |= type;
		break;
	default:
		break;
	}
	vppif_reg8_out(GPIO_BASE_ADDR + 0x300 + VPP_VOINT_NO, reg);
}
EXPORT_SYMBOL(vout_set_int_type);

void vout_set_int_enable(int enable)
{
	vppif_reg32_write(GPIO_BASE_ADDR + 0x300 +
		VPP_VOINT_NO, 0x80, 7, enable);	/* GPIO irq enable/disable */
}
EXPORT_SYMBOL(vout_set_int_enable);

int vout_get_clr_int(void)
{
	if ((vppif_reg8_in(GPIO_BASE_ADDR + 0x360) &
		(0x1 << VPP_VOINT_NO)) == 0)
		return 1;
	/* clear int status */
	vppif_reg8_out(GPIO_BASE_ADDR + 0x360, 0x1 << VPP_VOINT_NO);
	return 0;
}
EXPORT_SYMBOL(vout_get_clr_int);

static void vo_plug_enable(int enable, void *func, int no)
{
	vout_t *vo;

	DBG_DETAIL("%d\n", enable);
	vo_plug_vout = no;
	vo = vout_get_entry(no);
#ifdef CONFIG_WMT_EXT_DEV_PLUG_DISABLE
	vo_plug_func = 0;
	govrh_set_dvo_enable(vo->govr, enable);
#else
	vo_plug_func = func;
	if (vo_plug_func == 0)
		return;

	if (enable) {
		vppif_reg32_write(GPIO_BASE_ADDR + 0x40, 0x1 << VPP_VOINT_NO,
			VPP_VOINT_NO, 0x0); /* GPIO disable */
		vppif_reg32_write(GPIO_BASE_ADDR + 0x80, 0x1 << VPP_VOINT_NO,
			VPP_VOINT_NO, 0x0); /* GPIO input mode */
		vppif_reg32_write(GPIO_BASE_ADDR + 0x480, 0x1 << VPP_VOINT_NO,
			VPP_VOINT_NO, 0x1); /* GPIO pull enable */
		vppif_reg32_write(GPIO_BASE_ADDR + 0x4c0, 0x1 << VPP_VOINT_NO,
			VPP_VOINT_NO, 0x1); /* GPIO pull-up */
#ifndef CONFIG_UBOOT
		vo_do_plug(0);
		if (vpp_request_irq(IRQ_GPIO, vo_plug_interrupt_routine,
			IRQF_SHARED, "vo plug", (void *) &vo_plug_vout))
			DBG_ERR("request GPIO ISR fail\n");

		vppif_reg32_write(GPIO_BASE_ADDR + 0x300 + VPP_VOINT_NO,
			0x80, 7, 1); /* GPIO irq enable */
	} else {
		vpp_free_irq(IRQ_GPIO, (void *) &vo_plug_vout);
#endif
	}
#endif
}

/*--------------------------------- DVI ------------------------------------*/
#ifdef WMT_FTBLK_VOUT_DVI
static int vo_dvi_blank(vout_t *vo, vout_blank_t arg)
{
	DBG_DETAIL("(%d)\n", arg);
	if (vo->pre_blank == VOUT_BLANK_POWERDOWN) {
		if (vo->dev) {
			vo->dev->init(vo);
#ifdef __KERNEL__
			if (vo->dev->poll)
				vo_set_poll(vo, (vo->dev->poll) ? 1 : 0,
					DVI_POLL_TIME_MS);
#endif
		}
	}
#ifdef __KERNEL__
	if (arg == VOUT_BLANK_POWERDOWN)
		vo_set_poll(vo, 0, 0);

	/* patch for virtual fb,if HDMI plugin then DVI blank */
	if (g_vpp.virtual_display || (g_vpp.dual_display == 0)) {
		if (vout_chkplug(VPP_VOUT_NUM_HDMI)) {
			arg = VOUT_BLANK_NORMAL;
			vout_change_status(vo, VPP_VOUT_STS_BLANK, arg);
		}
	}
#endif
	if (!lcd_get_dev()) /* enable DVO not contain LCD */
		govrh_set_dvo_enable(vo->govr,
				(arg == VOUT_BLANK_UNBLANK) ? 1 : 0);
	vo->pre_blank = arg;
	return 0;
}

static int vo_dvi_config(vout_t *vo, int arg)
{
	vout_info_t *vo_info;

	DBG_DETAIL("Enter\n");

	vo_info = (vout_info_t *) arg;
	govrh_set_dvo_sync_polar(vo->govr,
		  (vo_info->option & VPP_DVO_SYNC_POLAR_HI) ? 0 : 1,
		  (vo_info->option & VPP_DVO_VSYNC_POLAR_HI) ? 0 : 1);
	return 0;
}

static int vo_dvi_init(vout_t *vo, int arg)
{
	unsigned int clk_delay;

	DBG_DETAIL("(%d)\n", arg);

	lvds_set_enable(0);
	govrh_set_dvo_color_format(vo->govr, vo->option[0]);
	/* bit0:0-12 bit,1-24bit */
	govrh_set_dvo_outdatw(vo->govr, vo->option[1] & BIT0);
	govrh_IGS_set_mode(vo->govr, 0, (vo->option[1] & 0x600) >> 9,
		(vo->option[1] & 0x800) >> 11);
	govrh_IGS_set_RGB_swap(vo->govr, (vo->option[1] & 0x3000) >> 12);
	clk_delay = (vo->option[1] & BIT0) ?
		VPP_GOVR_DVO_DELAY_24 : VPP_GOVR_DVO_DELAY_12;
	govrh_set_dvo_clock_delay(vo->govr, ((clk_delay & BIT14) != 0x0),
		clk_delay & 0x3FFF);
	if (vo->dev) {
		vo->dev->set_mode(&vo->option[0]);
		vo->dev->set_power_down(VPP_FLAG_DISABLE);
		if (vo->dev->interrupt)
			vo_plug_enable(VPP_FLAG_ENABLE,
					vo->dev->interrupt, vo->num);
#ifdef __KERNEL__
		if (vo->dev->poll)
			vo_set_poll(vo, (vo->dev->poll) ? 1 : 0,
					DVI_POLL_TIME_MS);
#endif
		vout_change_status(vo, VPP_VOUT_STS_PLUGIN,
				vo->dev->check_plugin(0));
	}
	vo->govr->fb_p->set_csc(vo->govr->fb_p->csc_mode);
	if (!lcd_get_dev()) {
		govrh_set_dvo_enable(vo->govr,
			(vo->status & VPP_VOUT_STS_BLANK) ? 0 : 1);
	}
	return 0;
}

static int vo_dvi_uninit(vout_t *vo, int arg)
{
	DBG_DETAIL("(%d)\n", arg);

	vo_plug_enable(VPP_FLAG_DISABLE, 0, VPP_VOUT_NUM);
	govrh_set_dvo_enable(vo->govr, VPP_FLAG_DISABLE);
#ifdef __KERNEL__
	vo_set_poll(vo, 0, DVI_POLL_TIME_MS);
#endif
	return 0;
}

static int vo_dvi_chkplug(vout_t *vo, int arg)
{
	int plugin = 1;

	DBG_MSG("plugin %d\n", plugin);
	return plugin;
}

static int vo_dvi_get_edid(vout_t *vo, int arg)
{
	char *buf;
	int i, cnt;

	DBG_DETAIL("Enter\n");

	buf = (char *) arg;
	memset(&buf[0], 0x0, 128 * EDID_BLOCK_MAX);
	if (vpp_i2c_read(VPP_DVI_EDID_ID, 0xA0, 0, &buf[0], 128)) {
		DBG_ERR("read edid\n");
		return 1;
	}

	if (edid_checksum(buf, 128)) {
		DBG_ERR("checksum\n");
		return 1;
	}

	cnt = buf[0x7E];
	if (cnt >= 3)
		cnt = 3;
	for (i = 1; i <= cnt; i++) {
		vpp_i2c_read(VPP_DVI_EDID_ID, 0xA0, 0x80 * i,
			&buf[128 * i], 128);
	}
	return 0;
}

vout_inf_t vo_dvi_inf = {
	.mode = VOUT_INF_DVI,
	.init = vo_dvi_init,
	.uninit = vo_dvi_uninit,
	.blank = vo_dvi_blank,
	.config = vo_dvi_config,
	.chkplug = vo_dvi_chkplug,
	.get_edid = vo_dvi_get_edid,
};

int vo_dvi_initial(void)
{
	vout_inf_register(VOUT_INF_DVI, &vo_dvi_inf);
	return 0;
}
module_init(vo_dvi_initial);

#endif /* WMT_FTBLK_VOUT_DVI */

/*---------------------------------- HDMI -----------------------------------*/
void vo_hdmi_set_clock(int enable)
{
	DBG_DETAIL("(%d)\n", enable);

	enable = (enable) ? CLK_ENABLE : CLK_DISABLE;
	vpp_set_clock_enable(DEV_HDMII2C, enable, 0);
	vpp_set_clock_enable(DEV_HDMI, enable, 0);
	vpp_set_clock_enable(DEV_HDCE, enable, 0);
}

#ifdef WMT_FTBLK_VOUT_HDMI
#ifdef __KERNEL__
struct timer_list hdmi_cp_timer;
static struct timer_list hdmi_plug_timer;
#endif

void vo_hdmi_cp_set_enable_tmr(int sec)
{
	int ms = sec * 1000;

	DBG_MSG("[HDMI] set enable tmr %d sec\n", sec);

	if (sec == 0) {
		hdmi_set_cp_enable(VPP_FLAG_ENABLE);
		return ;
	}
#ifdef __KERNEL__
	if (hdmi_cp_timer.function)
		del_timer(&hdmi_cp_timer);
	init_timer(&hdmi_cp_timer);
	hdmi_cp_timer.data = VPP_FLAG_ENABLE;
	hdmi_cp_timer.function = (void *) hdmi_set_cp_enable;
	hdmi_cp_timer.expires = jiffies + msecs_to_jiffies(ms);
	add_timer(&hdmi_cp_timer);
#else
	hdmi_set_cp_enable(VPP_FLAG_ENABLE);
#endif
}

static int vo_hdmi_blank(vout_t *vo, vout_blank_t arg)
{
	int enable;

	DBG_DETAIL("(%d)\n", arg);

	enable = (arg == VOUT_BLANK_UNBLANK) ? 1 : 0;
	if (g_vpp.hdmi_cp_enable && enable)
		vo_hdmi_cp_set_enable_tmr(2);
	else
		hdmi_set_cp_enable(VPP_FLAG_DISABLE);
	hdmi_set_enable(enable);
	hdmi_set_power_down((enable) ? 0 : 1);
	return 0;
}

#ifndef CFG_LOADER
static irqreturn_t vo_hdmi_cp_interrupt
(
	int irq,		/*!<; // irq id */
	void *dev_id		/*!<; // device id */
)
{
	vout_t *vo;

	DBG_DETAIL("%d\n", irq);
	vo = vout_get_entry(VPP_VOUT_NUM_HDMI);
	switch (hdmi_check_cp_int()) {
	case 1:
		if (hdmi_cp)
			hdmi_cp->enable(VPP_FLAG_DISABLE);
		vo_hdmi_cp_set_enable_tmr(HDMI_CP_TIME);
		vout_change_status(vo, VPP_VOUT_STS_CONTENT_PROTECT, 0);
		vpp_netlink_notify_cp(0);
		break;
	case 0:
		vout_change_status(vo, VPP_VOUT_STS_CONTENT_PROTECT, 1);
		vpp_netlink_notify_cp(1);
		break;
	case 2:
		hdmi_ri_tm_cnt = 3 * 30;
		break;
	default:
		break;
	}
	return IRQ_HANDLED;
}

#ifdef __KERNEL__
static void vo_hdmi_do_plug(struct work_struct *ptr)
#else
static void vo_hdmi_do_plug(void)
#endif
{
	vout_t *vo;
	int plugin;
	int option = 0;

	plugin = hdmi_check_plugin(1);
	vo = vout_get_entry(VPP_VOUT_NUM_HDMI);
	vout_change_status(vo, VPP_VOUT_STS_PLUGIN, plugin);
	if (plugin) {
		option = vout_get_edid_option(VPP_VOUT_NUM_HDMI);
#ifdef CONFIG_VPP_DEMO
		option |= (EDID_OPT_HDMI + EDID_OPT_AUDIO);
#endif
		hdmi_set_option(option);
	} else {
		g_vpp.hdmi_bksv[0] = g_vpp.hdmi_bksv[1] = 0;
	}
	vo_hdmi_blank(vo, (vo->status & VPP_VOUT_STS_BLANK) ? 1 : !(plugin));
	if (!g_vpp.hdmi_certify_flag)
		hdmi_hotplug_notify(plugin);
	DBG_MSG("%d\n", plugin);
	return;
}
DECLARE_WORK(vo_hdmi_plug_work, vo_hdmi_do_plug);

static void hdmi_handle_plug(vpp_flag_t enable)
{
		schedule_work(&vo_hdmi_plug_work);
}

static void vo_hdmi_handle_plug_tmr(int ms)
{
	static int timer_init;

	if(timer_init == 0) {
		init_timer(&hdmi_plug_timer);
		hdmi_plug_timer.data = VPP_FLAG_ENABLE;
		hdmi_plug_timer.function = (void *) hdmi_handle_plug;
		timer_init = 1;
	}
	hdmi_plug_timer.expires = jiffies + msecs_to_jiffies(ms);
	mod_timer(&hdmi_plug_timer, hdmi_plug_timer.expires);
}

static irqreturn_t vo_hdmi_plug_interrupt
(
	int irq,		/*!<; // irq id */
	void *dev_id		/*!<; // device id */
)
{
	DBG_MSG("vo_hdmi_plug_interrupt %d\n", irq);
	hdmi_clear_plug_status();
	if (g_vpp.hdmi_certify_flag)
		vo_hdmi_do_plug(0);
	else	
		vo_hdmi_handle_plug_tmr(HDMI_PLUG_DELAY);
	return IRQ_HANDLED;
}
#endif

static int vo_hdmi_init(vout_t *vo, int arg)
{
	DBG_DETAIL("(%d)\n", arg);

	vo_hdmi_set_clock(1);
	vout_change_status(vout_get_entry(VPP_VOUT_NUM_HDMI),
		VPP_VOUT_STS_PLUGIN, hdmi_check_plugin(0));
	hdmi_enable_plugin(1);

	if (g_vpp.hdmi_disable)
        return 0;
#ifndef CONFIG_UBOOT
	if (vpp_request_irq(VPP_IRQ_HDMI_CP, vo_hdmi_cp_interrupt,
		SA_INTERRUPT, "hdmi cp", (void *) 0)) {
		DBG_ERR("*E* request HDMI ISR fail\n");
	}
	if (vpp_request_irq(VPP_IRQ_HDMI_HPDH, vo_hdmi_plug_interrupt,
		SA_INTERRUPT, "hdmi plug", (void *) 0)) {
		DBG_ERR("*E* request HDMI ISR fail\n");
	}
	if (vpp_request_irq(VPP_IRQ_HDMI_HPDL, vo_hdmi_plug_interrupt,
		SA_INTERRUPT, "hdmi plug", (void *) 0)) {
		DBG_ERR("*E* request HDMI ISR fail\n");
	}
#endif
	hdmi_set_enable((vo->status & VPP_VOUT_STS_BLANK) ?
		VPP_FLAG_DISABLE : VPP_FLAG_ENABLE);
	return 0;
}

static int vo_hdmi_uninit(vout_t *vo, int arg)
{
	DBG_DETAIL("(%d)\n", arg);
	hdmi_enable_plugin(0);
	hdmi_set_cp_enable(VPP_FLAG_DISABLE);
	hdmi_set_enable(VPP_FLAG_DISABLE);
#ifndef CONFIG_UBOOT
	vpp_free_irq(VPP_IRQ_HDMI_CP, (void *) 0);
	vpp_free_irq(VPP_IRQ_HDMI_HPDH, (void *) 0);
	vpp_free_irq(VPP_IRQ_HDMI_HPDL, (void *) 0);
#endif
	vo_hdmi_set_clock(0);
	return 0;
}

static int vo_hdmi_config(vout_t *vo, int arg)
{
	vout_info_t *vo_info;
	vdo_color_fmt colfmt;

	hdmi_set_enable(0);
	vo_info = (vout_info_t *) arg;

	DBG_DETAIL("(%dx%d@%d)\n", vo_info->resx, vo_info->resy, vo_info->fps);
	DPRINT("hdmi config (%dx%d@%d)\n", vo_info->resx, vo_info->resy, vo_info->fps);

	/* 1280x720@60, HDMI pixel clock 74250060 not 74500000 */
	if ((vo_info->resx == 1280)
		&& (vo_info->resy == 720) && (vo_info->pixclk == 74500000))
		vo_info->pixclk = 74250060;
	colfmt = (vo->option[0] == VDO_COL_FMT_YUV422V) ?
		VDO_COL_FMT_YUV422H : vo->option[0];
	hdmi_cur_plugin = hdmi_check_plugin(0);
	hdmi_info.option = (hdmi_cur_plugin) ?
		vout_get_edid_option(VPP_VOUT_NUM_HDMI) : 0;
	hdmi_info.outfmt = colfmt;
	hdmi_info.vic = hdmi_get_vic(vo_info->resx, vo_info->resy,
		vo_info->fps, (vo_info->option & VPP_OPT_INTERLACE) ? 1 : 0);

	govrh_set_csc_mode(vo->govr, vo->govr->fb_p->csc_mode);
	hdmi_set_sync_low_active((vo_info->option & VPP_DVO_SYNC_POLAR_HI) ?
		0 : 1, (vo_info->option & VPP_DVO_VSYNC_POLAR_HI) ? 0 : 1);
	hdmi_config(&hdmi_info);
#ifdef __KERNEL__
	mdelay(200);	/* patch for VIZIO change resolution issue */
#endif
	hdmi_cur_plugin = hdmi_check_plugin(0);
	vo_hdmi_blank(vo, (vo->status & VPP_VOUT_STS_BLANK) ?
		1 : !(hdmi_cur_plugin));
	return 0;
}

static int vo_hdmi_chkplug(vout_t *vo, int arg)
{
	int plugin;

	if (g_vpp.hdmi_disable)
        return 0;
	plugin = hdmi_get_plugin();
	DBG_DETAIL("%d\n", plugin);
	return plugin;
}

static int vo_hdmi_get_edid(vout_t *vo, int arg)
{
	char *buf;
#ifdef CONFIG_WMT_EDID
	int i, cnt;
#endif
	DBG_DETAIL("Enter\n");
	buf = (char *) arg;
#ifdef CONFIG_WMT_EDID
	memset(&buf[0], 0x0, 128*EDID_BLOCK_MAX);
	if (!hdmi_get_plugin())
		return 1;

	if (hdmi_DDC_read(0xA0, 0x0, &buf[0], 128)) {
		DBG_ERR("read edid\n");
		return 1;
	}

	if (edid_checksum(buf, 128)) {
		DBG_ERR("hdmi checksum\n");
/*		g_vpp.dbg_hdmi_ddc_crc_err++; */
		return 1;
	}

	cnt = buf[0x7E];
	if (cnt >= 3)
		cnt = 3;
	for (i = 1; i <= cnt; i++)
		hdmi_DDC_read(0xA0, 0x80 * i, &buf[128 * i], 128);
#endif
	return 0;
}

vout_inf_t vo_hdmi_inf = {
	.mode = VOUT_INF_HDMI,
	.init = vo_hdmi_init,
	.uninit = vo_hdmi_uninit,
	.blank = vo_hdmi_blank,
	.config = vo_hdmi_config,
	.chkplug = vo_hdmi_chkplug,
	.get_edid = vo_hdmi_get_edid,
};

int vo_hdmi_initial(void)
{
	vout_inf_register(VOUT_INF_HDMI, &vo_hdmi_inf);
	return 0;
}
module_init(vo_hdmi_initial);

#endif /* WMT_FTBLK_VOUT_HDMI */

/*--------------------------------- LVDS ------------------------------------*/
#ifdef WMT_FTBLK_VOUT_LVDS
int vo_lvds_init_flag;
static int vo_lvds_blank(vout_t *vo, vout_blank_t arg)
{
	DBG_DETAIL("(%d)\n", arg);
	if (arg == VOUT_BLANK_POWERDOWN) {
		vppif_reg32_write(LVDS_TRE_EN, 0);
	} else { /* avoid suspend signal not clear */
		lvds_set_enable((arg == VOUT_BLANK_UNBLANK) ? 1 : 0);
	}

	if (vo_lvds_init_flag)
		lvds_set_power_down(arg);
	return 0;
}

static int vo_lvds_config(vout_t *vo, int arg)
{
	DBG_DETAIL("(%d)\n", arg);
	lvds_set_power_down(VPP_FLAG_DISABLE);
	vo_lvds_init_flag = 1;
	return 0;
}

static int vo_lvds_init(vout_t *vo, int arg)
{
	DBG_DETAIL("(%d)\n", arg);

	vpp_set_clock_enable(DEV_LVDS, 1, 0);
	if (vo->dev)
		vo->dev->set_mode(&vo->option[0]);
	govrh_set_dvo_enable(p_govrh2, 0);
	govrh_set_csc_mode(vo->govr, vo->govr->fb_p->csc_mode);
	lvds_set_enable((vo->status & VPP_VOUT_STS_BLANK) ?
		VPP_FLAG_DISABLE : VPP_FLAG_ENABLE);
	return 0;
}

static int vo_lvds_uninit(vout_t *vo, int arg)
{
	DBG_DETAIL("(%d)\n", arg);
	lvds_set_enable(VPP_FLAG_DISABLE);
	if (vo->dev)
		vo->dev->set_mode(0);
	lvds_set_power_down(VPP_FLAG_ENABLE);
	vpp_set_clock_enable(DEV_LVDS, 0, 0);
	vo_lvds_init_flag = 0;
	return 0;
}

static int vo_lvds_chkplug(vout_t *vo, int arg)
{
	DBG_DETAIL("\n");
#if 0
	vo = vout_get_info(VOUT_LVDS);
	if (vo->dev)
		return vo->dev->check_plugin(0);
#endif
	return 1;
}

vout_inf_t vo_lvds_inf = {
	.mode = VOUT_INF_LVDS,
	.capability = VOUT_INF_CAP_FIX_PLUG,
	.init = vo_lvds_init,
	.uninit = vo_lvds_uninit,
	.blank = vo_lvds_blank,
	.config = vo_lvds_config,
	.chkplug = vo_lvds_chkplug,
#ifdef WMT_FTBLK_VOUT_HDMI
	.get_edid = vo_hdmi_get_edid,
#endif
};

int vo_lvds_initial(void)
{
	vout_inf_register(VOUT_INF_LVDS, &vo_lvds_inf);
	return 0;
}
module_init(vo_lvds_initial);

#endif /* WMT_FTBLK_VOUT_LVDS */
/*---------------------------------- API ------------------------------------*/
#ifndef CFG_LOADER
int vout_set_audio(vout_audio_t *arg)
{
	vout_t *vout;
	int ret = 0;

#if 0
	vout = vout_get_info(VPP_VOUT_DVO2HDMI);
	if (vout && (vout->status & VPP_VOUT_STS_PLUGIN)) {
		if (vout->dev->set_audio)
			vout->dev->set_audio(arg);
	}
#endif

#ifdef WMT_FTBLK_VOUT_HDMI
	vout = vout_get_entry(VPP_VOUT_NUM_HDMI);
	if (vout) {
		hdmi_config_audio(arg);
		ret = 1;
	}
#endif
	return ret;
}
#endif

/* 3445 port1 : DVI/SDD, port2 : VGA/SDA, port3 : HDMI/LVDS */
/* 3481 port1 : HDMI/LVDS, port2 : DVI */
/* 3498 port1 : HDMI, port2 : DVI/LVDS */
vout_t vout_entry_0 = {
	.fix_cap = BIT(VOUT_INF_HDMI),
	.option[0] = VDO_COL_FMT_ARGB,
	.option[1] = VPP_DATAWIDHT_24,
	.option[2] = 0,
};

vout_t vout_entry_1 = {
	.fix_cap = BIT(VOUT_INF_DVI) + BIT(VOUT_INF_LVDS) +
		VOUT_CAP_EXT_DEV + 0x100,	/* i2c bus 1,ext dev */
	.option[0] = VDO_COL_FMT_ARGB,
	.option[1] = VPP_DATAWIDHT_24,
	.option[2] = 0,
};

void vout_init_param(struct vout_init_parm_t *init_parm)
{
	char buf[100];
	int varlen = 100;
	unsigned int parm[10];

	memset(init_parm, 0, sizeof(struct vout_init_parm_t));

	/* register vout & set default */
	vout_register(0, &vout_entry_0);
	vout_entry_0.inf = vout_inf_get_entry(VOUT_INF_HDMI);
	vout_register(1, &vout_entry_1);
	vout_entry_1.inf = vout_inf_get_entry(VOUT_INF_DVI);

	/* [uboot parameter] up bound : resx:resy */
	if (wmt_getsyspara("wmt.display.upbound", buf, &varlen) == 0) {
		vpp_parse_param(buf, parm, 2, 0);
		init_parm->ub_resx = parm[0];
		init_parm->ub_resy = parm[1];
		MSG("up bound(%d,%d)\n", init_parm->ub_resx,
			init_parm->ub_resy);
	}

	/* [uboot parameter] default resolution : resx:resy:fps */
	init_parm->def_resx = VOUT_INFO_DEFAULT_RESX;
	init_parm->def_resy = VOUT_INFO_DEFAULT_RESY;
	init_parm->def_fps = VOUT_INFO_DEFAULT_FPS;
	if (wmt_getsyspara("wmt.display.default.res", buf, &varlen) == 0) {
		vpp_parse_param(buf, parm, 3, 0);
		init_parm->def_resx = parm[0];
		init_parm->def_resy = parm[1];
		init_parm->def_fps = parm[2];
		MSG("default res(%d,%d,%d)\n", init_parm->def_resx,
			init_parm->def_resy, init_parm->def_fps);
	} else if (wmt_getsyspara("wmt.display.param", buf, &varlen) == 0) {
		vpp_parse_param(buf, parm, 6, 0);
		init_parm->def_resx = parm[3];
		init_parm->def_resy = parm[4];
		init_parm->def_fps = parm[5];
		MSG("param2 res(%d,%d,%d)\n", init_parm->def_resx,
			init_parm->def_resy, init_parm->def_fps);
    }
}

const char *vout_sys_parm_str[] = {"wmt.display.param", "wmt.display.param2"};
int vout_check_display_info(struct vout_init_parm_t *init_parm)
{
	char buf[100];
	int varlen = 100;
	unsigned int parm[10];
	vout_t *vo = 0;
	int ret = 1;
	int i;
	int info_no;

	DBG_DETAIL("Enter\n");

	/* [uboot parameter] display param : type:op1:op2:resx:resy:fps */
	for (i = 0; i < 2; i++) {
		if (wmt_getsyspara((char *)vout_sys_parm_str[i], buf, &varlen))
			continue;

		DBG_MSG("display param %d : %s\n", i, buf);
		vpp_parse_param(buf, (unsigned int *)parm, 6, 0);
		MSG("boot parm vo %d opt %d,%d, %dx%d@%d\n", parm[0],
			parm[1], parm[2], parm[3], parm[4], parm[5]);
		vo = vout_get_entry_adapter(parm[0]);
		if (vo == 0) {
			vout_t *vo_boot;

			if (parm[0] != VOUT_BOOT) {
				ret = 1;
				DBG_ERR("uboot param invalid\n");
				break;
			}

  			g_vpp.virtual_display_mode = parm[1];
			if (parm[1] == 1) {
                                init_parm->def_resx = parm[3];
                                init_parm->def_resy = parm[4];
                                init_parm->def_fps = parm[5];
                                parm[3] = 1920;
                                parm[4] = 1080;
                                parm[5] = 60;
			} else if (parm[1] == 2) {
				init_parm->def_resx = parm[3];
				init_parm->def_resy = parm[4];
				init_parm->def_fps = parm[5];
				g_vpp.dual_display = 0;
				break;
			}

			MSG("[VOUT] virtual display\n");
			init_parm->virtual_display = 1;
			g_vpp.virtual_display = 1;
			g_vpp.fb0_bitblit = 1;
			vo_boot = kmalloc(sizeof(vout_t), GFP_KERNEL);
			if (vo_boot == 0) {
				ret = 1;
				break;
			}
			vo_boot->resx = parm[3];
			vo_boot->resy = parm[4];
			vo_boot->pixclk = parm[5];
			vo_boot->inf = 0;
			vo_boot->num = VPP_VOUT_INFO_NUM;
			vo_boot->govr = 0;
			info_no = vout_info_add_entry(vo_boot);
			kfree(vo_boot);
		} else {
			vo->inf = vout_get_inf_entry_adapter(parm[0]);
			vo->option[0] = parm[1];
			vo->option[1] = parm[2];
			vo->resx = parm[3];
			vo->resy = parm[4];
			vo->pixclk = parm[5];
			vout_change_status(vo, VPP_VOUT_STS_BLANK,
				(parm[2] & VOUT_OPT_BLANK) ?
				VOUT_BLANK_NORMAL : VOUT_BLANK_UNBLANK);
			info_no = vout_info_add_entry(vo);
		}

		switch (parm[0]) {
		case VOUT_LVDS:
			{
			struct fb_videomode *vmode;
			vout_info_t *info;

			info = vout_info_get_entry(info_no);
			vmode = 0;
			if ((parm[1] == 0) && (parm[3] == 0) &&
				(parm[4] == 0)) { /* auto detect */
				if (vout_get_edid_option(vo->num)) {
					vmode = &vo->edid_info.detail_timing[0];
					if (vmode->pixclock == 0) {
						vmode = 0;
						DBG_ERR("LVDS timing\n");
					}
				}

				if (vo->inf->get_edid(vo,
					(int)vo->edid) == 0) {
					if (edid_parse(vo->edid,
						&vo->edid_info) == 0)
						DBG_ERR("LVDS edid parse\n");
				} else {
					DBG_ERR("LVDS edid read\n");
				}
			}

			if (vmode == 0) { /* use internal timing */
				lcd_parm_t *p = 0;

				if (parm[1]) {
					p = lcd_get_parm(parm[1], parm[2]);
					if (p)
						lcd_set_lvds_id(parm[1]);
				}

				if (p == 0)
					p = lcd_get_oem_parm(parm[3], parm[4]);
				vmode = &p->vmode;
			}
			memcpy(&vo_oem_vmode, vmode,
				sizeof(struct fb_videomode));
			vo->option[2] = vmode->vmode;
			info->resx = vmode->xres;
			info->resy = vmode->yres;
			info->fps  = vmode->refresh;
			vout_info_set_fixed_timing(info_no, &vo_oem_vmode);
			lcd_set_type(1);
			}
		case VOUT_LCD:
			{
			vout_dev_t *dev;

			lcd_set_parm(parm[1], parm[2] & 0xFF);
			dev = lcd_get_dev();
			vo->ext_dev = dev;
			vo->dev = dev;
			dev->vout = vo;
			vo->option[0] = VDO_COL_FMT_ARGB;
			vo->option[1] &= ~0xFF;
			vo->option[1] |= VPP_DATAWIDHT_24;
			}
			break;
		case VOUT_DVI:
		case VOUT_HDMI:
			vout_info[info_no].option = (parm[2] & 0x2) ?
				VPP_OPT_INTERLACE : 0;
			break;
		default:
			break;
		}
		ret = 0;
	}

	if (g_vpp.virtual_display)
		ret = 1;

	/* add vout entry to info */
	for (i = 0; i < VPP_VOUT_NUM; i++) {
		vo = vout_get_entry(i);
		if (vo) {
			if (vo->resx == 0) {
				vo->resx = init_parm->def_resx;
				vo->resy = init_parm->def_resy;
				vo->pixclk = init_parm->def_fps;
			}
			vout_info_add_entry(vo);
		}
	}
	DBG_DETAIL("Leave\n");
	return ret;
}

void vout_check_ext_device(void)
{
	char buf[100];
	int varlen = 100;
	vout_t *vo = 0;
	vout_dev_t *dev = 0;
	int i;
	vout_info_t *info;

	DBG_DETAIL("Enter\n");
	vpp_i2c_init(1, 0xA0);

	/* [uboot parameter] reg operation : addr op val */
	if (wmt_getsyspara("wmt.display.regop", buf, &varlen) == 0) {
		unsigned int addr;
		unsigned int val;
		char op;
		char *p, *endp;

		p = buf;
		while (1) {
			addr = simple_strtoul(p, &endp, 16);
			if (*endp == '\0')
				break;

			op = *endp;
			if (endp[1] == '~') {
				val = simple_strtoul(endp + 2, &endp, 16);
				val = ~val;
			} else {
				val = simple_strtoul(endp + 1, &endp, 16);
			}

			DBG_DETAIL("  reg op: 0x%X %c 0x%X\n", addr, op, val);
			switch (op) {
			case '|':
				REG32_VAL(addr) |= val;
				break;
			case '=':
				REG32_VAL(addr) = val;
				break;
			case '&':
				REG32_VAL(addr) &= val;
				break;
			default:
				DBG_ERR("Error, Unknown operator %c\n", op);
			}

			if (*endp == '\0')
				break;
			p = endp + 1;
		}
	}

	/* [uboot parameter] dvi device : name:i2c id */
	vo = vout_get_entry(VPP_VOUT_NUM_DVI);
	info = vout_get_info_entry(VPP_VOUT_NUM_DVI);
	if (lcd_get_dev()) {
		vo->dev->init(vo);
		vout_info_set_fixed_timing(info->num, &p_lcd->vmode);
	} else if (vo->ext_dev == 0) {
		if (wmt_getsyspara("wmt.display.dvi.dev", buf, &varlen) == 0) {
			unsigned int param[1];
			char *p;

			p = strchr(buf, ':');
			*p = 0;
			vpp_parse_param(p + 1, param, 1, 0x1);
			vpp_i2c_release();
			vpp_i2c_init(param[0], 0xA0);
			MSG("dvi dev %s : %x\n", buf, param[0]);
			do {
				dev = vout_get_device(dev);
				if (dev == 0)
					break;
				if (strcmp(buf, dev->name) == 0) {
					vo->ext_dev = dev;
					vo->dev = dev;
					dev->vout = vo;

					/* probe & init external device */
					if (dev->init(vo)) {
						DBG_ERR("%s not exist\n",
							dev->name);
						vo->dev = vo->ext_dev = 0;
						break;
					} else {
						MSG("[VOUT] dvi dev %s\n", buf);
					}

					/* if LCD then set fixed timing */
					if (vo->dev == lcd_get_dev())
						vout_info_set_fixed_timing(
						info->num, &p_lcd->vmode);
				}
			} while (1);
		}
	}

	for (i = 0; i < VPP_VOUT_NUM; i++) {
		vo = vout_get_entry(i);
		if (vo == 0)
			continue;

		if ((vo->fix_cap & VOUT_CAP_EXT_DEV) && !(vo->ext_dev)) {
			dev = 0;
			do {
				dev = vout_get_device(dev);
				if (dev == 0)
					break;
				if (vo->fix_cap & BIT(dev->mode)) {
					vo->inf = vout_inf_get_entry(dev->mode);
					if (dev->init(vo) == 0) {
						vo->ext_dev = dev;
						vo->dev = dev;
						dev->vout = vo;
						break;
					}
				}
			} while (1);
		}
		DBG_MSG("vout %d ext dev : %s\n", i,
			(vo->dev) ? vo->dev->name : "NO");
	}

	/* [uboot parameter] oem timing :
		pixclk:option:hsync:hbp:hpixel:hfp:vsync:vbp:vpixel:vfp */
	if (wmt_getsyspara("wmt.display.tmr", buf, &varlen) == 0) {
		unsigned int param[10];
		struct fb_videomode *p;
		int xres, yres;

		p = &vo_oem_vmode;
		DBG_MSG("tmr %s\n", buf);
		vpp_parse_param(buf, param, 10, 0);
		p->pixclock = param[0];
		p->vmode = param[1];
		p->hsync_len = param[2];
		p->left_margin = param[3];
		p->xres = param[4];
		p->right_margin = param[5];
		p->vsync_len = param[6];
		p->upper_margin = param[7];
		p->yres = param[8];
		p->lower_margin = param[9];
		p->pixclock *= 1000;
		xres = p->hsync_len + p->left_margin + p->xres +
			p->right_margin;
		yres = p->vsync_len + p->upper_margin + p->yres +
			p->lower_margin;
		p->refresh = vpp_calc_refresh(p->pixclock, xres, yres);
		if (p->refresh == 59)
			p->refresh = 60;
		DBG_MSG("tmr pixclk %d,option 0x%x\n",
			p->pixclock, p->vmode);
		DBG_MSG("H sync %d,bp %d,pixel %d,fp %d\n", p->hsync_len,
			p->left_margin,	p->xres, p->right_margin);
		DBG_MSG("V sync %d,bp %d,pixel %d,fp %d\n", p->vsync_len,
			p->upper_margin, p->yres, p->lower_margin);
		p->pixclock = KHZ2PICOS(p->pixclock / 1000);
		vout_info_set_fixed_timing(0, &vo_oem_vmode);
		vout_info[0].resx = p->xres;
		vout_info[0].resy = p->yres;
		vout_info[0].fps = p->refresh;
	}
	DBG_DETAIL("Leave\n");
}

void vout_check_monitor_resolution(struct vout_init_parm_t *init_parm)
{
	vout_info_t *p;
	vout_t *vo;
	int i;

	DBG_DETAIL("Check monitor support\n");

	if (g_vpp.virtual_display) {
		p = &vout_info[0];
		if ((p->resx == 0) || (p->resy == 0)) {
			struct fb_videomode vmode;
			int edid_option;

			for (i = 0; i < VPP_VOUT_NUM; i++) {
				if (vout_chkplug(i) == 0) {
					DBG_MSG("no plugin %d\n", i);
					continue;
				}

				edid_option = vout_get_edid_option(i);
				if ((edid_option & EDID_OPT_VALID) == 0) {
					DBG_MSG("no EDID %d\n", i);
					continue;
				}
				break;
			}
			vmode.xres = (i == VPP_VOUT_NUM) ? 1280 : 1920;
			vmode.yres = (i == VPP_VOUT_NUM) ? 720 : 1080;
			vmode.refresh = 60;
			vmode.vmode = 0;
			vout_find_match_mode(1, &vmode, 1);
			init_parm->def_resx = vmode.xres;
			init_parm->def_resy = vmode.yres;
			init_parm->def_fps = vmode.refresh;

			for (i = 0; i < VPP_VOUT_INFO_NUM; i++) {
				p = &vout_info[i];
				p->resx = init_parm->def_resx;
				p->resy = init_parm->def_resy;
				p->fps = init_parm->def_fps;
			}
			DBG_MSG("virtual display %dx%d@%d\n",
				vmode.xres, vmode.yres, vmode.refresh);
		}
	}

	for (i = 0; i < VPP_VOUT_INFO_NUM; i++) {
		int support;
		int vo_num;

		p = &vout_info[i];

		DBG_MSG("info %d (%d,%d,%d)\n", i, p->resx, p->resy, p->fps);

		if (p->vo_mask == 0)
			break;
		if (p->govr == 0)
			continue;
		if (p->fixed_vmode)
			continue;
		if (init_parm->ub_resx)
			p->resx = init_parm->ub_resx;
		if (init_parm->ub_resy)
			p->resy = init_parm->ub_resy;
		if (!p->resx)
			p->resx = init_parm->def_resx;
		if (!p->resy)
			p->resy = init_parm->def_resy;
		if (!p->fps)
			p->fps = init_parm->def_fps;

		DBG_DETAIL("info %d (%d,%d,%d)\n", i, p->resx, p->resy, p->fps);

		support = 0;
		for (vo_num = 0; vo_num < VPP_VOUT_NUM; vo_num++) {
			int edid_option;

			if ((p->vo_mask & (0x1 << vo_num)) == 0)
				continue;
			vo = vout_get_entry(vo_num);
			if (vo == 0)
				continue;
#ifdef CONFIG_WMT_EDID
			if (vout_chkplug(vo_num) == 0) {
				DBG_MSG("no plugin %d\n", vo_num);
				continue;
			}

			vout_change_status(vo, VPP_VOUT_STS_BLANK, 0);
			edid_option = vout_get_edid_option(vo_num);
			if (edid_option & EDID_OPT_VALID) {
				struct fb_videomode *vmode;

				if (edid_find_support(&vo->edid_info, p->resx,
					p->resy, p->fps, &vmode)) {
					support = 1;
					break;
				} else {
					if (vout_find_edid_support_mode(
						&vo->edid_info,
						(unsigned int *)&p->resx,
						(unsigned int *)&p->resy,
						(unsigned int *)&p->fps,
						(edid_option & EDID_OPT_16_9) ?
						1 : 0)) {
						support = 1;
						break;
					}
				}
			}
#else
			break;
#endif
		}

		if (support == 0) {
			if (vout_chkplug(VPP_VOUT_NUM_HDMI)) {
				init_parm->def_resx = 1280;
				init_parm->def_resy = 720;
				init_parm->def_fps = 60;
			} else {
				char buf[40];
				int varlen = 40;

				if (wmt_getsyspara("wmt.display.tvformat",
					buf, &varlen) == 0) {
					if (strnicmp(buf, "PAL", 3) == 0) {
						init_parm->def_resx = 720;
						init_parm->def_resy = 576;
						init_parm->def_fps = 50;
					} else if (strnicmp(buf,"NTSC",4) == 0) {
						init_parm->def_resx = 720;
						init_parm->def_resy = 480;
						init_parm->def_fps = 60;
					}
				} else {
					init_parm->def_resx = 1024;
					init_parm->def_resy = 768;
					init_parm->def_fps = 60;
				}
			}
			p->resx = init_parm->def_resx;
			p->resy = init_parm->def_resy;
			p->fps = init_parm->def_fps;
			DBG_MSG("use default(%dx%d@%d)\n", p->resx,
				p->resy, p->fps);
		}
		DBG_MSG("fb%d(%dx%d@%d)\n", i, p->resx, p->resy, p->fps);
	}
	DBG_DETAIL("Leave\n");
}

int vout_init(void)
{
	vout_t *vo;
	int flag;
	int i;
	struct vout_init_parm_t init_parm;

	DBG_DETAIL("Enter\n");

	vout_init_param(&init_parm);

	/* check vout info */
	memset(vout_info, 0, sizeof(vout_info_t) * VPP_VOUT_INFO_NUM);
	flag = vout_check_display_info(&init_parm);

	/* probe external device */
	vout_check_ext_device();

	/* check plug & EDID for resolution */
	if (flag)
		vout_check_monitor_resolution(&init_parm);

	if ((init_parm.virtual_display) || (g_vpp.dual_display == 0)) {
		int plugin;

		plugin = vout_chkplug(VPP_VOUT_NUM_HDMI);
		vout_change_status(vout_get_entry(VPP_VOUT_NUM_HDMI),
			VPP_VOUT_STS_BLANK, (plugin) ? 0 : 1);
		vout_change_status(vout_get_entry(VPP_VOUT_NUM_DVI),
			VPP_VOUT_STS_BLANK, (plugin) ? 1 : 0);
	}

	DBG_DETAIL("Set mode\n");
	for (i = 0; i < VPP_VOUT_NUM; i++) {
		vo = vout_get_entry(i);
		if (vo && (vo->inf)) {
			vout_inf_mode_t mode;

			mode = vo->inf->mode;
			vo->inf = 0; /* force interface init */
			vout_set_mode(i, mode);
			DBG_DETAIL("vout %d : inf %s, ext dev %s,status 0x%x\n",
				i,
				(vo->inf) ? vout_inf_str[vo->inf->mode] : "No",
				(vo->dev) ? vo->dev->name : "No", vo->status);
		}
	}

#ifndef CONFIG_UBOOT
	for (i = 0; i < VPP_VOUT_NUM; i++) {
		vo = vout_get_entry(i);
		if (vo)
			vout_set_blank((0x1 << i),
				(vo->status & VPP_VOUT_STS_BLANK) ? 1 : 0);
	}
#endif

	/* show vout info */
	for (i = 0; i < VPP_VOUT_INFO_NUM; i++) {
		vout_info_t *info;

		info = &vout_info[i];
		if (info->vo_mask == 0)
			break;
		if (i == 0) {
			if (info->govr) {
#ifdef WMT_FTBLK_GOVRH_CURSOR
				p_cursor->mmio = info->govr->mmio;
#endif
			}
		}
		MSG("[VOUT] info %d,mask 0x%x,%s,%dx%d@%d\n", i, info->vo_mask,
			(info->govr) ? vpp_mod_str[info->govr_mod] : "VIR",
			info->resx, info->resy, info->fps);
	}
	DBG_DETAIL("Leave\n");
	return 0;
}

int vout_exit(void)
{
	return 0;
}
