/*++
 * WonderMedia common SoC hardware JPEG decoder driver
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

#define HW_JDEC_C


#include "hw-jdec.h"

#ifdef __KERNEL__
#define JDEC_OWNER_DBG
#endif

#define JDEC_GET_STATUS(ctx)       ((ctx)->_status)
#define JDEC_SET_STATUS(ctx, sta)  ((ctx)->_status = (sta))

static int jpeg_serial_no;

static spinlock_t s_jdec_lock = __SPIN_LOCK_UNLOCKED(s_jdec_lock);

#ifdef JDEC_OWNER_DBG
#define MAX_JDEC_NUM    8
struct Observer {
	struct jdec_ctx_t  *ctx;
	pid_t  tgid;
	pid_t  pid;
	char   comm[TASK_COMM_LEN];
} gObserver[MAX_JDEC_NUM];
#endif

static void add_owner(struct jdec_ctx_t *ctx)
{
#ifdef JDEC_OWNER_DBG
	int i;

	for (i = 0; i < MAX_JDEC_NUM; i++) {
		struct Observer *ob = &gObserver[i];
		if (ob->ctx == 0) {
			ob->ctx = ctx;
			ob->tgid   = current->tgid;
			ob->pid    = current->pid;
			memcpy(ob->comm, current->comm, TASK_COMM_LEN);
			INFO("Open (id: %d) from [%s, tgid: %d, pid: %d]\n",
				ctx->ref_num, ob->comm, ob->tgid, ob->pid);
			break;
		}
	}
	if (i >= MAX_JDEC_NUM)
		DBG_ERR("Observer overflow! id: %d\n", ctx->ref_num);
#endif
}

static void del_owner(struct jdec_ctx_t *ctx)
{
#ifdef JDEC_OWNER_DBG
	int i;

	for (i = 0; i < MAX_JDEC_NUM; i++) {
		struct Observer *ob = &gObserver[i];
		if (ob->ctx == ctx) {
			INFO("close(id: %d) from [%s, tgid: %d, pid: %d]\n",
				ctx->ref_num, ob->comm, ob->tgid, ob->pid);
			gObserver[i].ctx = 0;
			break;
		}
	}
	if (i >= MAX_JDEC_NUM)
		DBG_ERR("Can not find owner 0x%p (%d)\n", ctx, ctx->ref_num);
#endif
}
static void show_owner_info(struct Observer *ob)
{
#ifdef JDEC_OWNER_DBG
	struct jdec_ctx_t *ctx = ob->ctx;

	INFO("- state: 0x%08x\n", ctx->_status);
	INFO("- got_lock: %dx\n", ob->ctx->got_lock);
	INFO("- SOF (%d x %d)\n",
		ctx->prop.hdr.sof_w, ctx->prop.hdr.sof_h);
	INFO("- scale_ratio:   %d\n", ctx->scale_ratio);
	INFO("- mmu_mode:      %d\n", ctx->prop.mmu_mode);
	if (ctx->_is_mjpeg)
		INFO("- _is_mjpeg: %d\n", ctx->_is_mjpeg);
#endif
}

static void show_owner(struct jdec_ctx_t *ctx)
{
#ifdef JDEC_OWNER_DBG
	int i;

	for (i = 0; i < MAX_JDEC_NUM; i++) {
		struct Observer *ob = &gObserver[i];
		if (ob->ctx == ctx) {
			INFO("Owner(id: %d) from [%s, tgid: %d, pid: %d]\n",
				ctx->ref_num, ob->comm, ob->tgid, ob->pid);
			show_owner_info(ob);
			break;
		}
	}
	if (i >= MAX_JDEC_NUM)
		DBG_ERR("Could not find this owner 0x%p\n", ctx);
#endif
}

static void show_all_owner(struct jdec_ctx_t *ctx)
{
#ifdef JDEC_OWNER_DBG
	int owner = 0;
	int i;

	INFO("Current Owner(id: %d)\n", ctx->ref_num);
	for (i = 0; i < MAX_JDEC_NUM; i++) {
		struct Observer *ob = &gObserver[i];
		if (ob->ctx == 0)
			continue;

		owner++;
		INFO("Owner[%d] (%d) from [%s, tgid: %d, pid: %d]\n",
			owner, ob->ctx->ref_num, ob->comm, ob->tgid, ob->pid);
		show_owner_info(ob);
	}
#endif
}

/*!*************************************************************************
* ioctl_set_attr
*
* Private Function
*/
/*!
* \brief
*    JPEG hardware initialization
*
* \retval  0 if success
*/
static int ioctl_set_attr(struct jdec_ctx_t *ctx)
{
	jdec_prop_ex_t *jp = &ctx->prop;
	int ret;

	if (jp->flags & DECFLAG_MJPEG_BIT)
		ctx->_is_mjpeg = 1;

	JDEC_SET_STATUS(ctx, STA_ATTR_SET);

	DBG_MSG("scale_factor:  %d\n", jp->scale_factor);
	DBG_MSG("decoded_color: %d\n", jp->dst_color);
	DBG_MSG("buf_in:        %p\n", jp->buf_in);
	DBG_MSG("bufsize:       %d\n", jp->bufsize);
	DBG_MSG("timeout:       %d\n", jp->timeout);

	DBG_MSG("profile:       %x\n", jp->hdr.profile);
	DBG_MSG("sof_w:         %d\n", jp->hdr.sof_w);
	DBG_MSG("sof_h:         %d\n", jp->hdr.sof_h);
	DBG_MSG("filesize:      %d\n", jp->hdr.filesize);
	DBG_MSG("src_color:     %d\n", jp->hdr.src_color);

	DBG_MSG("mmu_mode:      %d\n",   jp->mmu_mode);
	DBG_MSG("y_addr_user: 0x%08x\n", jp->y_addr_user);
	DBG_MSG("c_addr_user:   %d\n",   jp->c_addr_user);
	DBG_MSG("dst_y_addr:  0x%08x\n", jp->dst_y_addr);
	DBG_MSG("dst_y_size:    %d\n",   jp->dst_y_size);
	DBG_MSG("dst_c_addr:  0x%08x\n", jp->dst_c_addr);
	DBG_MSG("dst_c_size:    %d\n",   jp->dst_c_size);
	DBG_MSG("flags:       0x%08x\n", jp->flags);

	/* Do decoded_color change if necessary */
	switch (jp->dst_color) {
	case VDO_COL_FMT_ARGB:
	case VDO_COL_FMT_ABGR:
	case VDO_COL_FMT_YUV420:
	case VDO_COL_FMT_YUV422H:
	case VDO_COL_FMT_YUV444:
		break;
	default:
		DBG_ERR("Not supported decoded color(%d)\n", jp->dst_color);
		return -1;
	}
	ret = wmt_jdec_set_attr(ctx);

	return ret;
} /* End of ioctl_set_attr() */

/*!*************************************************************************
* ioctl_decode_proc
*
* Private Function
*/
/*!
* \brief
*    Open JPEG hardware
*
* \retval  0 if success
*/
static int ioctl_decode_proc(struct jdec_ctx_t *ctx)
{
	jdec_prop_ex_t *jp = &ctx->prop;
	int ret = -EINVAL;

	DBG_MSG("flags:           0x%x\n", jp->flags);
	DBG_MSG("dst_y_addr:      0x%x\n", jp->dst_y_addr);
	DBG_MSG("dst_y_size:      %d\n",   jp->dst_y_size);
	DBG_MSG("dst_c_addr:      0x%x\n", jp->dst_c_addr);
	DBG_MSG("dst_c_size:      %d\n",   jp->dst_c_size);

	/*----------------------------------------------------------------------
		Transfer the input address as PRD foramt
	----------------------------------------------------------------------*/
	ctx->mb_phy_addr = mb_alloc(jp->bufsize);
	if (ctx->mb_phy_addr == 0) {
		DBG_ERR("No MB buffer (request: %d bytes)!\n", jp->bufsize);
		return -1;
	}
	if (copy_from_user((void *)(mb_phys_to_virt(ctx->mb_phy_addr)),
					(void *)jp->buf_in, jp->bufsize)) {
		DBG_ERR("input buffer error\n");
		JDEC_SET_STATUS(ctx, STA_ERR_BAD_PRD);
		return -EFAULT;
	}
	wmt_codec_write_prd(ctx->mb_phy_addr, jp->bufsize, ctx->prd_virt,
						MAX_INPUT_BUF_SIZE);

/*	JDEC_SET_STATUS(ctx, STA_DECODEING);*/
	ret = wmt_jdec_decode_proc(ctx);
	if (ret)
		show_owner(ctx);

	return ret;
} /* End of ioctl_decode_proc() */

/*!*************************************************************************
* ioctl_decode_finish
*
* Private Function
*/
/*!
* \brief
*    Open JPEG hardware
*
* \retval  0 if success
*/
int ioctl_decode_finish(struct jdec_ctx_t *ctx)
{
	int ret = -EINVAL;

	ret = wmt_jdec_decode_finish(ctx);
	if (ret)
		show_all_owner(ctx);

	if (ctx->mb_phy_addr) {
		mb_free(ctx->mb_phy_addr);
		ctx->mb_phy_addr = 0;
	}
	return ret;
} /* End of ioctl_decode_finish() */

/*!*************************************************************************
* jdec_ioctl
*
* Private Function
*/
/*!
* \brief
*       It implement io control for AP
* \parameter
*   inode   [IN] a pointer point to struct inode
*   filp    [IN] a pointer point to struct file
*   cmd
*   arg
* \retval  0 if success
*/
static long jdec_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	int ret = -EINVAL;
	struct jdec_ctx_t *ctx;
	int max_size;

	TRACE("Enter (ctx: %p, cmd: 0x%x)\n", (void *)ctx, cmd);

	ctx = (struct jdec_ctx_t *)filp->private_data;

	switch (cmd) {
	case VDIOSET_DECODE_LOOP:
		DBG_DETAIL("Receive IOCTL:VDIOSET_DECODE_LOOP\n");

		ret = copy_from_user((vd_ioctl_cmd *)&ctx->prop,
			(const void *)arg, sizeof(jdec_prop_ex_t));
		if (ret) {
			DBG_ERR("copy_from_user FAIL\n");
			break;
		}
		ret = wmt_codec_lock(CODEC_VD_JPEG, ctx->prop.timeout);
		if (ret)
			break;

		ctx->got_lock = 1;
		ret = ioctl_set_attr(ctx);
		if (ret == 0) {
			max_size = (PAGE_ALIGN(ctx->prop.bufsize)/PAGE_SIZE)*8;
			if (max_size > MAX_INPUT_BUF_SIZE) {
				DBG_ERR("input size overflow\n");
					ret = -EFAULT;
					goto JDEC_UNLOCK_CODEC;
			}
			/* Start decoding process */
			ret = ioctl_decode_proc(ctx);
			if (ret == 0) {
				/* Wait HW decoding finish */
				ret = ioctl_decode_finish(ctx);
				copy_to_user((jdec_prop_ex_t *)arg,
					&ctx->prop, sizeof(jdec_prop_ex_t));
			}
		}
JDEC_UNLOCK_CODEC:
		wmt_codec_unlock(CODEC_VD_JPEG);
		ctx->got_lock = 0;
		break;

	default:
		DBG_ERR("Unknown IOCTL:0x%x in jdec_ioctl()!\n", cmd);
		ret = -EINVAL;
		break;
	} /* switch(cmd) */

	TRACE("Leave\n");

	return ret;
} /* End of jdec_ioctl() */

/*!*************************************************************************
* jdec_open
*
* Private Function
*/
/*!
* \brief
*    It is called when driver is opened, and initial hardward and resource
* \parameter
*   inode   [IN] a pointer point to struct inode
*   filp    [IN] a pointer point to struct file
* \retval  0 if success
*/
static int jdec_open(struct inode *inode, struct file *filp)
{
	struct jdec_ctx_t *ctx;
	struct vd_mem_set *vd_info;
	int ret = 0;

	TRACE("Enter\n");

	/*----------------------------------------------------------------------
		Initial hardware decoder
	----------------------------------------------------------------------*/
	ctx = kmalloc(sizeof(struct jdec_ctx_t), GFP_KERNEL);
	if (ctx == 0) {
		DBG_ERR("Allocate %d bytes fail!\n", sizeof(struct jdec_ctx_t));
		ret = -EBUSY;
		goto EXIT_jdec_open;
	}
	memset(ctx, 0, sizeof(struct jdec_ctx_t));
	/*----------------------------------------------------------------------
		private_data from VD is "PRD table virtual & physical address"
	----------------------------------------------------------------------*/
	vd_info = (struct vd_mem_set *)filp->private_data;

	ctx->prd_virt = (unsigned int)vd_info->virt;
	ctx->prd_addr = (unsigned int)vd_info->phys;

	DBG_DETAIL("prd_virt: 0x%x, prd_addr: 0x%x\n",
		ctx->prd_virt, ctx->prd_addr);

	spin_lock(&s_jdec_lock);
	jpeg_serial_no++;
	spin_unlock(&s_jdec_lock);

	/* change privata_data to ctx */
	filp->private_data = ctx;
	ctx->ref_num       = jpeg_serial_no;

	ret = wmt_jdec_open(ctx);
	JDEC_SET_STATUS(ctx, STA_READY);

EXIT_jdec_open:
	if (ret == 0)
		add_owner(ctx);
	TRACE("Leave\n");

	return ret;
} /* End of jdec_open() */

/*!*************************************************************************
* jdec_release
*
* Private Function
*/
/*!
* \brief
*    It is called when driver is released, and reset hardward and free resource
* \parameter
*   inode   [IN] a pointer point to struct inode
*   filp    [IN] a pointer point to struct file
* \retval  0 if success
*/
static int jdec_release(struct inode *inode, struct file *filp)
{
	struct jdec_ctx_t *ctx;

	TRACE("Enter\n");

	ctx = (struct jdec_ctx_t *)filp->private_data;
	if (ctx) {
		if (ctx->got_lock == 1)
			wmt_codec_unlock(CODEC_VD_JPEG);
		wmt_jdec_close(ctx);
		del_owner(ctx);
		kfree(ctx);
	}
	module_put(THIS_MODULE);

	TRACE("Leave\n");

	return 0;
} /* End of jdec_release() */

/*!*************************************************************************
* jdec_probe
*
* Private Function
*/
/*!
* \brief
*       Probe
* \retval  0 if success
*/
static int jdec_probe(struct videodecoder *jdec)
{
	TRACE("Enter\n");

	jdec->id       = VD_JPEG;
	jdec->irq_num = 0;
	jdec->isr     = 0;

	TRACE("Leave\n");

	return 0;
} /* End of jdec_probe() */

/*!*************************************************************************
* jdec_remove
*
* Private Function
*/
/*!
* \brief
*       remove jpeg module
* \retval  0 if success
*/
static int jdec_remove(void)
{
	TRACE("Enter\n");
	TRACE("Leave\n");

	return 0;
} /* End of jdec_remove() */

/*!*************************************************************************
* jdec_suspend
*
* Private Function
*/
/*!
* \brief
*       suspend jpeg module
* \parameter
*   dev
*   state
*   level
* \retval  0 if success
*/
static int jdec_suspend(pm_message_t state)
{
	TRACE("Enter\n");
	switch (state.event) {
	case PM_EVENT_SUSPEND:
	case PM_EVENT_FREEZE:
	case PM_EVENT_PRETHAW:
	default:
		/* TO DO */
		break;
	}
	TRACE("Leave\n");

	return 0;
} /* End of jdec_suspend() */

/*!*************************************************************************
* jdec_resume
*
* Private Function
*/
/*!
* \brief
*       resume jpeg module
* \retval  0 if success
*/
static int jdec_resume(void)
{
	TRACE("Enter\n");

	/* TO DO */

	TRACE("Leave\n");

	return 0;
} /* End of jdec_resume() */

/*!*************************************************************************
* jdec_get_info
*
* Private Function
*/
/*!
* \brief
*       resume jpeg module
* \retval  0 if success
*/
static int jdec_get_info(char *buf, char **start, off_t offset, int len)
{
	char *p = buf;
#ifdef CONFIG_PROC_FS
	struct jdec_ctx_t *ctx;
	int i, owner = 0;

	p += sprintf(p, "[WMT JDEC]\n");
	p += sprintf(p, "Latest Serial: %d\n", jpeg_serial_no);

	for (i = 0; i < MAX_JDEC_NUM; i++) {
		struct Observer *ob = &gObserver[i];
		jdec_prop_ex_t *jp;

		if (ob->ctx == 0)
			continue;

		owner++;
		ctx = ob->ctx;
		jp = &ctx->prop;
		p += sprintf(p, "Owner[%d] from [%s, tgid: %d, pid: %d]\n",
			owner, ob->comm, ob->tgid, ob->pid);
		p += sprintf(p, " - serial id:   %d\n", ctx->ref_num);
		p += sprintf(p, " - status:  0x%08x\n", ctx->_status);
		p += sprintf(p, " - got_lock:    %d\n", ctx->got_lock);
		p += sprintf(p, " - SOF (%d x %d)\n",
			jp->hdr.sof_w, jp->hdr.sof_h);
		p += sprintf(p, " - scale: %d\n", jp->scale_factor);
		p += sprintf(p, " - mmu_mode:    %d\n", jp->mmu_mode);
		p += sprintf(p, " - is_mjpeg:    %d\n", ctx->_is_mjpeg);
	}
	p += sprintf(p, "[WMT JDEC] End\n");
#endif /* CONFIG_PROC_FS */
	return p - buf;
} /* End of jdec_get_info() */

/*!*************************************************************************
* jdec_setup
*
* Private Function
*/
/*!
* \brief
*
* \retval  0 if success
*/
static int jdec_setup(void)
{
	return 0;
} /* End of jdec_setup() */

/*!*************************************************************************
    platform device struct define
***************************************************************************/

struct videodecoder jpeg_decoder = {
	.name    = "jdec",
	.id      = VD_JPEG,
	.setup   = jdec_setup,
	.remove  = jdec_remove,
	.suspend = jdec_suspend,
	.resume  = jdec_resume,
	.fops = {
				.owner   = THIS_MODULE,
				.open    = jdec_open,
				.unlocked_ioctl = jdec_ioctl,
				.release = jdec_release,
			},
	.get_info = jdec_get_info,
	.device = NULL,
	.irq_num      = 0,
	.isr          = NULL,
	.isr_data     = NULL,
	.vd_class     = NULL,
};

/*!*************************************************************************
* hw_jdec_init
*
* Private Function
*/
/*!
* \brief
*       init jpeg module
* \retval  0 if success
*/
static int hw_jdec_init(void)
{
	int ret;

	TRACE("Enter\n");

	jdec_probe(&jpeg_decoder);

	ret = wmt_vd_register(&jpeg_decoder);

	TRACE("Leave\n");

	return ret;
} /* End of hw_jdec_init() */

module_init(hw_jdec_init);

/*!*************************************************************************
* hw_jdec_exit
*
* Private Function
*/
/*!
* \brief
*
* \retval  0 if success
*/
static void hw_jdec_exit(void)
{
	TRACE("Enter\n");

	wmt_vd_unregister(&jpeg_decoder);

	TRACE("Leave\n");

	return;
} /* End of hw_jdec_exit() */

module_exit(hw_jdec_exit);

MODULE_AUTHOR("WonderMedia Technologies, Inc.");
MODULE_DESCRIPTION("WMT JPEG decode device driver");
MODULE_LICENSE("GPL");

/*--------------------End of Function Body -------------------------------*/

#undef HW_JDEC_C
