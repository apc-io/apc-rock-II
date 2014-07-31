/*++
 * linux/drivers/video/wmt/osif.c
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

#define VPP_OSIF_C
#undef DEBUG
/* #define DEBUG */
/* #define DEBUG_DETAIL */
/*----------------------- DEPENDENCE -----------------------------------------*/
#include "vpp.h"

/*----------------------- PRIVATE MACRO --------------------------------------*/

/*----------------------- PRIVATE CONSTANTS ----------------------------------*/
/* #define LVDS_XXXX    1     *//*Example*/

/*----------------------- PRIVATE TYPE  --------------------------------------*/
/* typedef  xxxx lvds_xxx_t; *//*Example*/

/*----------EXPORTED PRIVATE VARIABLES are defined in lvds.h  -------------*/
/*----------------------- INTERNAL PRIVATE VARIABLES - -----------------------*/
/* int  lvds_xxx;        *//*Example*/
#ifdef __KERNEL__
static DEFINE_SPINLOCK(vpp_irqlock);
static unsigned long vpp_lock_flags;
#endif
int vpp_dvi_i2c_id;

/*--------------------- INTERNAL PRIVATE FUNCTIONS ---------------------------*/
/* void lvds_xxx(void); *//*Example*/

/*----------------------- Function Body --------------------------------------*/
#ifdef __KERNEL__
#include <asm/io.h>
#include <linux/proc_fs.h>
#else
__inline__ U32 inl(U32 offset)
{
	return REG32_VAL(offset);
}

__inline__ void outl(U32 val, U32 offset)
{
	REG32_VAL(offset) = val;
}

static __inline__ U16 inw(U32 offset)
{
	return REG16_VAL(offset);
}

static __inline__ void outw(U16 val, U32 offset)
{
	REG16_VAL(offset) = val;
}

static __inline__ U8 inb(U32 offset)
{
	return REG8_VAL(offset);
}

static __inline__ void outb(U8 val, U32 offset)
{
	REG8_VAL(offset) = val;
}
#ifndef CFG_LOADER
int get_key(void)
{
	int key;

	key = get_num(0, 256, "Input:", 5);
	DPRINT("\n");
	return key;
}

void udelay(int us)
{
	vpp_post_delay(us);
}

void mdelay(int ms)
{
	udelay(ms * 1000);
}
#endif
#endif

extern unsigned int wmt_read_oscr(void);
void vpp_udelay(unsigned int us)
{
#if 1
	udelay(us);
#else
	unsigned int cur;
	unsigned int cross;

	if (us == 0)
		return;

	cur = wmt_read_oscr();
	us = cur + us * 3;
	cross = (us < cur) ? cur : 0; /* check overflow */
	while (1) {
		if (cross) {
			if (cur < cross)
				cross = 0;
		} else {
			if (us < cur)
				break;
		}
		cur = wmt_read_oscr();
	}
#endif
}

/* Internal functions */
U8 vppif_reg8_in(U32 offset)
{
	return inb(offset);
}

U8 vppif_reg8_out(U32 offset, U8 val)
{
	outb(val, offset);
	return val;
}

U16 vppif_reg16_in(U32 offset)
{
	return inw(offset);
}

U16 vppif_reg16_out(U32 offset, U16 val)
{
	outw(val, offset);
	return val;
}

U32 vppif_reg32_in(U32 offset)
{
	return inl(offset);
}

U32 vppif_reg32_out(U32 offset, U32 val)
{
	outl(val, offset);
	return val;
}

U32 vppif_reg32_write(U32 offset, U32 mask, U32 shift, U32 val)
{
	U32 new_val;

#if 0
	if (val > (mask >> shift))
		MSG("*E* check the parameter 0x%x 0x%x 0x%x 0x%x\n",
						offset, mask, shift, val);
#endif
	new_val = (inl(offset) & ~(mask)) | (((val) << (shift)) & mask);
	outl(new_val, offset);
	return new_val;
}

U32 vppif_reg32_read(U32 offset, U32 mask, U32 shift)
{
	return (inl(offset) & mask) >> shift;
}

U32 vppif_reg32_mask(U32 offset, U32 mask, U32 shift)
{
	return mask;
}

int vpp_request_irq(unsigned int irq_no, void *routine,
				unsigned int flags, char *name, void *arg)
{
#if 0	/* disable irq */
	return 0;
#endif

#ifdef __KERNEL__
	if (request_irq(irq_no, routine, flags, name, arg)) {
		DPRINT("[VPP] *E* request irq %s fail\n", name);
		return -1;
	}
#endif
	return 0;
}

void vpp_free_irq(unsigned int irq_no, void *arg)
{
#ifdef __KERNEL__
	free_irq(irq_no, arg);
#endif
}

#ifndef __KERNEL__
int wmt_getsyspara(char *varname, char *varval, int *varlen)
{
	int i = 0;
	char *p;

	p = getenv(varname);
	if (!p) {
		printf("## Warning: %s not defined\n", varname);
		return -1;
	}
	while (p[i] != '\0') {
		varval[i] = p[i];
		i++;
	}
	varval[i] = '\0';
	*varlen = i;
	/* printf("getsyspara: %s,len %d\n", p, *varlen); */
	return 0;
}
#endif

#ifndef CONFIG_VPOST
int vpp_parse_param(char *buf, unsigned int *param,
					int cnt, unsigned int hex_mask)
{
	char *p;
	char *endp;
	int i = 0;

	if (*buf == '\0')
		return 0;

	for (i = 0; i < cnt; i++)
		param[i] = 0;

	p = (char *)buf;
	for (i = 0; i < cnt; i++) {
		param[i] = simple_strtoul(p, &endp,
				(hex_mask & (0x1 << i)) ? 16 : 0);
		if (*endp == '\0')
			break;
		p = endp + 1;

		if (*p == '\0')
			break;
	}
	return i + 1;
}
#endif

unsigned int vpp_lock_cnt;
void vpp_lock_l(void)
{
#ifdef __KERNEL__
#if 0
	vpp_lock_cnt++;
	DPRINT("vpp_lock(%d)\n", vpp_lock_cnt);
#endif
	spin_lock_irqsave(&vpp_irqlock, vpp_lock_flags);
#else
#endif
}

void vpp_unlock(void)
{
#ifdef __KERNEL__
#if 0
	vpp_lock_cnt--;
	DPRINT("vpp_unlock(%d)\n", vpp_lock_cnt);
#endif
	spin_unlock_irqrestore(&vpp_irqlock, vpp_lock_flags);
#else
#endif
}

#ifdef __KERNEL__
struct i2c_adapter *vpp_i2c_adapter;
struct i2c_client *vpp_i2c_client;
struct vpp_i2c_priv {
	unsigned int var;
};
static int __devinit vpp_i2c_probe(struct i2c_client *i2c,
			    const struct i2c_device_id *id)
{
	struct vpp_i2c_priv *vpp_i2c;
	int ret = 0;

	DBGMSG("\n");
	if (vpp_i2c_client == 0)
		return -ENODEV;

	if (!i2c_check_functionality(vpp_i2c_client->adapter, I2C_FUNC_I2C)) {
		DBG_ERR("need I2C_FUNC_I2C\n");
		return -ENODEV;
	}

	vpp_i2c = kzalloc(sizeof(struct vpp_i2c_priv), GFP_KERNEL);
	if (vpp_i2c == NULL) {
		DBG_ERR("kzalloc fail\n");
		return -ENOMEM;
	}
	i2c_set_clientdata(i2c, vpp_i2c);
	return ret;
}

static int  vpp_i2c_remove(struct i2c_client *client)
{
	kfree(i2c_get_clientdata(client));
	return 0;
}

static const struct i2c_device_id vpp_i2c_id[] = {
	{ "vpp_i2c", 0},
	{ }
};
MODULE_DEVICE_TABLE(i2c, vpp_i2c_id);

static struct i2c_driver vpp_i2c_driver = {
	.probe = vpp_i2c_probe,
	.remove = vpp_i2c_remove,
	.id_table = vpp_i2c_id,
	.driver = { .name = "vpp_i2c", },
};

static struct i2c_board_info vpp_i2c_board_info = {
	.type          = "vpp_i2c",
	.flags         = 0x00,
	.platform_data = NULL,
	.archdata      = NULL,
	.irq           = -1,
};
#endif
#ifdef __KERNEL__
int vpp_i2c_xfer(struct i2c_msg *msg, unsigned int num, int bus_id)
#else
int vpp_i2c_xfer(struct i2c_msg_s *msg, unsigned int num, int bus_id)
#endif
{
	int  ret = 1;

#ifdef __KERNEL__
	int i = 0;
#if 0
	if (bus_id == 1) {
		ret = wmt_i2c_xfer_continue_if_4(msg, num, bus_id);
		return ret;
	}
#endif

	if (num > 1) {
		for (i = 0; i < num - 1; ++i)
			msg[i].flags |= I2C_M_NOSTART;
	}
	ret = i2c_transfer(vpp_i2c_adapter, msg, num);
	if (ret <= 0) {
		DBG_ERR("i2c fail\n");
		return ret;
	}
#elif defined(CONFIG_VPOST)
	ret = i2c_transfer(msg, 1);
#else
	ret = wmt_i2c_transfer(msg, num, bus_id);
#endif
	return ret;
}

int vpp_i2c_init(int i2c_id, unsigned short addr)
{
#ifdef __KERNEL__
	struct i2c_board_info *vpp_i2c_bi = &vpp_i2c_board_info;
	int ret = 0;

	DBGMSG("id %d,addr 0x%x\n", i2c_id, addr);

	vpp_dvi_i2c_id = i2c_id;
	if (i2c_id & VPP_DVI_I2C_SW_BIT)
		return 0;

	i2c_id &= VPP_DVI_I2C_ID_MASK;
	vpp_i2c_adapter = i2c_get_adapter(i2c_id);
	if (vpp_i2c_adapter == NULL) {
		DBG_ERR("can not get i2c adapter, client address error\n");
		return -ENODEV;
	}

	vpp_i2c_bi->addr = addr >> 1;

	vpp_i2c_client = i2c_new_device(vpp_i2c_adapter, vpp_i2c_bi);
	if (vpp_i2c_client == NULL) {
		DBG_ERR("allocate i2c client failed\n");
		return -ENOMEM;
	}
	i2c_put_adapter(vpp_i2c_adapter);

	ret = i2c_add_driver(&vpp_i2c_driver);
	if (ret)
		DBG_ERR("i2c_add_driver fail\n");
	return ret;
#else
	vpp_dvi_i2c_id = i2c_id & 0xF;
	return 0;
#endif
}

int vpp_i2c_release(void)
{
#ifdef __KERNEL__
	if (vpp_i2c_client) {
		i2c_del_driver(&vpp_i2c_driver);
		i2c_unregister_device(vpp_i2c_client);
		vpp_i2c_remove(vpp_i2c_client);
		vpp_i2c_client = 0;
	}
#else

#endif
	return 0;
}

#ifdef __KERNEL__
static DEFINE_SEMAPHORE(vpp_i2c_sem);
#endif
void vpp_i2c_set_lock(int lock)
{
#ifdef __KERNEL__
	if (lock)
		down(&vpp_i2c_sem);
	else
		up(&vpp_i2c_sem);
#endif
}

int vpp_i2c_lock;
int vpp_i2c_enhanced_ddc_read(int id, unsigned int addr,
			unsigned int index, char *pdata, int len)
{
#ifdef __KERNEL__
	struct i2c_msg msg[3];
#else
	struct i2c_msg_s msg[3];
#endif
	unsigned char buf[len + 1];
	unsigned char buf2[2];
	int ret = 0;

	vpp_i2c_set_lock(1);

	if (vpp_i2c_lock)
		DBG_ERR("in lock\n");

	vpp_i2c_lock = 1;

	if (id & VPP_DVI_I2C_BIT)
		id = vpp_dvi_i2c_id;
	id = id & VPP_DVI_I2C_ID_MASK;
	buf2[0] = 0x1;
	buf2[1] = 0x0;
	msg[0].addr = (0x60 >> 1);
	msg[0].flags = 0 ;
	msg[0].flags &= ~(I2C_M_RD);
	msg[0].len = 1;
	msg[0].buf = buf2;

	addr = (addr >> 1);
	memset(buf, 0x55, len + 1);
	buf[0] = index;
	buf[1] = 0x0;

	msg[1].addr = addr;
	msg[1].flags = 0 ;
	msg[1].flags &= ~(I2C_M_RD);
	msg[1].len = 1;
	msg[1].buf = buf;

	msg[2].addr = addr;
	msg[2].flags = 0 ;
	msg[2].flags |= (I2C_M_RD);
	msg[2].len = len;
	msg[2].buf = buf;

	ret = vpp_i2c_xfer(msg, 3, id);
	memcpy(pdata, buf, len);
	vpp_i2c_lock = 0;
	vpp_i2c_set_lock(0);
	return ret;
} /* End of vpp_i2c_enhanced_ddc_read */
EXPORT_SYMBOL(vpp_i2c_enhanced_ddc_read);

int vpp_i2c_read(int id, unsigned int addr,
			unsigned int index, char *pdata, int len)
{
	int ret = 0;

	DBG_DETAIL("(%d,0x%x,%d,%d)\n", id, addr, index, len);
	vpp_i2c_set_lock(1);
	if (vpp_i2c_lock)
		DBG_ERR("in lock\n");

	vpp_i2c_lock = 1;

	if (id & VPP_DVI_I2C_BIT)
		id = vpp_dvi_i2c_id;
	id = id & VPP_DVI_I2C_ID_MASK;
	switch (id) {
	case 0 ... 0xF:	/* hw i2c */
		{
#ifdef CONFIG_KERNEL
		struct i2c_msg msg[2];
#else
		struct i2c_msg_s msg[2] ;
#endif
		unsigned char buf[len + 1];

		addr = (addr >> 1);

		buf[0] = index;
		buf[1] = 0x0;

		msg[0].addr = addr; /* slave address */
		msg[0].flags = 0 ;
		msg[0].flags &= ~(I2C_M_RD);
		msg[0].len = 1;
		msg[0].buf = buf;

		msg[1].addr = addr;
		msg[1].flags = I2C_M_RD;
		msg[1].len = len;
		msg[1].buf = buf;
		ret = vpp_i2c_xfer(msg, 2, id);
		memcpy(pdata, buf, len);
		}
		break;
	default:
#ifdef CONFIG_KERNEL
		ret = vo_i2c_proc((id & 0xF), (addr | BIT0), index, pdata, len);
#endif
		break;
	}
#ifdef DEBUG
	{
	int i;

	DBGMSG("vpp_i2c_read(addr 0x%x,index 0x%x,len %d\n", addr, index, len);
	for (i = 0; i < len; i += 8) {
		DBGMSG("%d : 0x%02x 0x%02x 0x%02x 0x%02x",
			i, pdata[i], pdata[i + 1], pdata[i + 2], pdata[i + 3]);
		DBGMSG(" 0x%02x 0x%02x 0x%02x 0x%02x\n",
			pdata[i + 4], pdata[i + 5], pdata[i + 6], pdata[i + 7]);
	}
	}
#endif
	vpp_i2c_lock = 0;
	vpp_i2c_set_lock(0);
	return ret;
}
EXPORT_SYMBOL(vpp_i2c_read);

int vpp_i2c_write(int id, unsigned int addr, unsigned int index,
				char *pdata, int len)
{
	int ret = 0;

	DBG_DETAIL("(%d,0x%x,%d,%d)\n", id, addr, index, len);
	vpp_i2c_set_lock(1);
	if (vpp_i2c_lock)
		DBG_ERR("in lock\n");

	vpp_i2c_lock = 1;

	if (id & VPP_DVI_I2C_BIT)
		id = vpp_dvi_i2c_id;
	id = id & VPP_DVI_I2C_ID_MASK;
	switch (id) {
	case 0 ... 0xF:	/* hw i2c */
		{
#ifdef CONFIG_KERNEL
		struct i2c_msg msg[1];
#else
		struct i2c_msg_s msg[1] ;
#endif
		unsigned char buf[len + 1];

		addr = (addr >> 1);
		buf[0] = index;
		memcpy(&buf[1], pdata, len);
		msg[0].addr = addr; /* slave address */
		msg[0].flags = 0 ;
		msg[0].flags &= ~(I2C_M_RD);
		msg[0].len = len + 1;
		msg[0].buf = buf;
		ret = vpp_i2c_xfer(msg, 1, id);
		}
		break;
	default:
#ifdef CONFIG_KERNEL
		vo_i2c_proc((id & 0xF), (addr & ~BIT0), index, pdata, len);
#endif
		break;
	}

#ifdef DEBUG
	{
	int i;

	DBGMSG("vpp_i2c_write(addr 0x%x,index 0x%x,len %d\n", addr, index, len);
	for (i = 0; i < len; i += 8) {
		DBGMSG("%d : 0x%02x 0x%02x 0x%02x 0x%02x",
			i, pdata[i], pdata[i + 1], pdata[i + 2], pdata[i + 3]);
		DBGMSG(" 0x%02x 0x%02x 0x%02x 0x%02x\n",
			pdata[i + 4], pdata[i + 5], pdata[i + 6], pdata[i + 7]);
	}
	}
#endif
	vpp_i2c_lock = 0;
	vpp_i2c_set_lock(0);
	return ret;
}
EXPORT_SYMBOL(vpp_i2c_write);

void DelayMS(int ms)
{
	mdelay(ms);
}
EXPORT_SYMBOL(DelayMS);

/*----------------------- VPP debug --------------------------------------*/
#define VPP_DEBUG_FUNC
#ifdef VPP_DEBUG_FUNC
#define VPP_DBG_TMR_NUM		3
/* #define VPP_DBG_DIAG_NUM	100 */
#ifdef VPP_DBG_DIAG_NUM
char vpp_dbg_diag_str[VPP_DBG_DIAG_NUM][100];
int vpp_dbg_diag_index;
int vpp_dbg_diag_delay;
#endif

int vpp_check_dbg_level(vpp_dbg_level_t level)
{
	if (level == VPP_DBGLVL_ALL)
		return 1;

	switch (g_vpp.dbg_msg_level) {
	case VPP_DBGLVL_DISABLE:
		break;
	case VPP_DBGLVL_ALL:
		return 1;
	default:
		if (g_vpp.dbg_msg_level == level)
			return 1;
		break;
	}
	return 0;
}

void vpp_dbg_show(int level, int tmr, char *str)
{
#ifdef __KERNEL__
	static struct timeval pre_tv[VPP_DBG_TMR_NUM];
	struct timeval tv;
	unsigned int tm_usec = 0;

	if (vpp_check_dbg_level(level) == 0)
		return;

	if (tmr && (tmr <= VPP_DBG_TMR_NUM)) {
		do_gettimeofday(&tv);
		if (pre_tv[tmr - 1].tv_sec)
			tm_usec = (tv.tv_sec == pre_tv[tmr - 1].tv_sec) ?
			(tv.tv_usec - pre_tv[tmr - 1].tv_usec) :
			(1000000 + tv.tv_usec - pre_tv[tmr - 1].tv_usec);
		pre_tv[tmr - 1] = tv;
	}

#ifdef VPP_DBG_DIAG_NUM
	if (level == VPP_DBGLVL_DIAG) {
		if (str) {
			char *ptr = &vpp_dbg_diag_str[vpp_dbg_diag_index][0];
			sprintf(ptr, "%s (%d,%d)(T%d %d usec)", str,
				(int)tv.tv_sec, (int)tv.tv_usec, tmr,
				(int) tm_usec);
			vpp_dbg_diag_index = (vpp_dbg_diag_index + 1)
						% VPP_DBG_DIAG_NUM;
		}

		if (vpp_dbg_diag_delay) {
			vpp_dbg_diag_delay--;
			if (vpp_dbg_diag_delay == 0) {
				int i;

				DPRINT("----- VPP DIAG -----\n");
				for (i = 0; i < VPP_DBG_DIAG_NUM; i++) {
					DPRINT("%02d : %s\n", i,
				&vpp_dbg_diag_str[vpp_dbg_diag_index][0]);
				vpp_dbg_diag_index = (vpp_dbg_diag_index + 1)
					% VPP_DBG_DIAG_NUM;
				}
			}
		}
		return;
	}
#endif

	if (str) {
		if (tmr)
			DPRINT("[VPP] %s (T%d period %d usec)\n", str,
					tmr - 1, (int) tm_usec);
		else
			DPRINT("[VPP] %s\n", str);
	}
#else
	if (vpp_check_dbg_level(level) == 0)
		return;

	if (str)
		DPRINT("[VPP] %s\n", str);
#endif
} /* End of vpp_dbg_show */

void vpp_dbg_show_val1(int level, int tmr, char *str, int val)
{
	if (vpp_check_dbg_level(level)) {
		char buf[50];

		sprintf(buf, "%s 0x%x", str, val);
		vpp_dbg_show(level, tmr, buf);
	}
}

#ifdef __KERNEL__
static DECLARE_WAIT_QUEUE_HEAD(vpp_dbg_wq);
void vpp_dbg_wait(char *str)
{
	DPRINT("[VPP] vpp_dbg_wait(%s)\n", str);
	wait_event_interruptible(vpp_dbg_wq, (g_vpp.dbg_wait));
	g_vpp.dbg_wait = 0;
	DPRINT("[VPP] Exit vpp_dbg_wait\n");
}

void vpp_dbg_wake_up(void)
{
	wake_up(&vpp_dbg_wq);
}

int vpp_dbg_get_period_usec(vpp_dbg_period_t *p, int cmd)
{
	struct timeval tv;
	int tm_usec = 0;

	if (p == 0)
		return 0;

	do_gettimeofday(&tv);
	if (p->pre_tv.tv_sec)
		tm_usec = (tv.tv_sec == p->pre_tv.tv_sec) ?
		(tv.tv_usec - p->pre_tv.tv_usec) :
		(1000000 + tv.tv_usec - p->pre_tv.tv_usec);
	p->pre_tv = tv;
	if (cmd == 0) { /* reset */
		p->index = 0;
		memset(&p->period_us, 0, VPP_DBG_PERIOD_NUM);
	} else if (p->index < VPP_DBG_PERIOD_NUM) {
		p->period_us[p->index] = tm_usec;
		p->index++;
	}

	if (cmd == 2) { /* show */
		int i, sum = 0;

		DPRINT("[VPP] period");
		for (i = 0; i < VPP_DBG_PERIOD_NUM; i++) {
			DPRINT(" %d", p->period_us[i]);
			sum += p->period_us[i];
		}
		DPRINT(",sum %d\n", sum);
	}
	return tm_usec;
}

void vpp_dbg_timer(vpp_dbg_timer_t *p, char *str, int cmd)
{
	struct timeval tv;
	int tm_usec = 0;
	int initial = 0;

	if (p == 0)
		return;

	if (p->reset == 0) { /* default */
		p->reset = 150;
		initial = 1;
	}

	do_gettimeofday(&tv);
	if (p->pre_tv.tv_sec)
		tm_usec = (tv.tv_sec == p->pre_tv.tv_sec) ?
			(tv.tv_usec - p->pre_tv.tv_usec) :
			(1000000 + tv.tv_usec - p->pre_tv.tv_usec);
	p->pre_tv = tv;
	switch (cmd) {
	case 0: /* initial */
		initial = 1;
		break;
	case 1: /* start */
		break;
	case 2: /* end */
		p->cnt++;
		p->sum += tm_usec;
		if (p->min > tm_usec)
			p->min = tm_usec;
		if (p->max < tm_usec)
			p->max = tm_usec;
		if (p->threshold && (tm_usec >= p->threshold))
			MSG("%s tmr %d over %d\n", str, tm_usec, p->threshold);
		if (p->cnt >= p->reset) {
			int us_1t;

			us_1t = p->sum / p->cnt;
			MSG("%s(Cnt %d)Sum %d us,Avg %d,Min %d,Max %d,fps %d.%02d\n",
				str, p->cnt, p->sum, us_1t,
				p->min, p->max, 1000000 / us_1t,
				(100000000 / us_1t) % 100);
			initial = 1;
		}
		break;
	default:
		break;
	}

	if (initial) {
		p->cnt = 0;
		p->sum = 0;
		p->min = ~0;
		p->max = 0;
	}
}
#endif
#else
void vpp_dbg_show(int level, int tmr, char *str) {}
static void vpp_dbg_show_val1(int level, int tmr, char *str, int val) {}
void vpp_dbg_wait(char *str) {}
#endif

#if 0
static void load_regs(struct pt_regs *ptr)
{
	asm volatile(
	"stmia %0, {r0 - r15}\n\t"
	:
	: "r" (ptr)
	: "memory"
	);
}

void vpp_dbg_back_trace(void)
{
	struct pt_regs *ptr;
	unsigned int fp;
	unsigned long flags;

	ptr = kmalloc(sizeof(struct pt_regs), GFP_KERNEL);

	local_irq_save(flags);

	MSG("\n\nstart back trace...\n");
	load_regs(ptr);
	fp = ptr->ARM_fp;
	c_backtrace(fp, 0x1f);
	MSG("back trace end...\n\n");

	local_irq_restore(flags);

	kfree(ptr);
}
EXPORT_SYMBOL(vpp_dbg_back_trace);
#endif
