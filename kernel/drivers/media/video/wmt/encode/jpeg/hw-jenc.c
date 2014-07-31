/*++
 * WonderMedia common SoC hardware JPEG encoder driver
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
 * 10F, 529, Chung-Cheng Road, Hsin-Tien, Taipei 231, R.O.C
--*/

#define HW_JENC_C

#include <linux/module.h>

#include "hw-jenc.h"


/*!*************************************************************************
* jenc_ioctl
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
static long jenc_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	struct jenc_ctx *ctx;
	int ret = -EINVAL;
	int max_size;

	TRACE("Enter\n");
	/*----------------------------------------------------------------------
		IOCTL handler
	----------------------------------------------------------------------*/
	ctx = (struct jenc_ctx *)filp->private_data;

	switch (cmd) {
	case VEIOSET_ENCODE_LOOP:
		DBG_DETAIL("Receive IOCTL:VDIOSET_DECODE_LOOP\n");
		ret = copy_from_user((struct jenc_prop *)&ctx->prop,
				(const void *)arg, sizeof(struct jenc_prop));
		if (ret) {
			DBG_ERR("copy_from_user FAIL\n");
			break;
		}
		max_size = (PAGE_ALIGN(ctx->prop.buf_size)/PAGE_SIZE)*8;
		if (max_size > MAX_INPUT_BUF_SIZE) {
			DBG_ERR("prop size overflow. %x/%x\n",
			ctx->prop.buf_size, MAX_INPUT_BUF_SIZE*4096/8);
			ret = -EFAULT;
			goto EXIT_jenc_ioctl;
		}
		ret = wmt_codec_lock(CODEC_VE_JPEG, ctx->prop.timeout);
		if (ret)
			break;

		ret = wmt_jenc_encode_proc(ctx);
		if (ret == 0)
			ret = wmt_jenc_encode_finish(ctx);

		copy_to_user((struct jenc_prop *)arg, &ctx->prop,
			sizeof(struct jenc_prop));

		wmt_codec_unlock(CODEC_VE_JPEG);
		break;

	default:
		DBG_ERR("Unknown IOCTL:0x%x in jenc_ioctl()!\n", cmd);
		ret = -EINVAL;
		break;
	} /* switch(cmd) */
EXIT_jenc_ioctl:

	TRACE("Leave\n");

	return ret;
} /* End of jenc_ioctl() */

/*!*************************************************************************
* jenc_open
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
static int jenc_open(struct inode *inode, struct file *filp)
{
	struct ve_info *info;
	struct jenc_ctx *ctx;
	int ret;

	TRACE("Enter\n");
	/*----------------------------------------------------------------------
		Initial hardware decoder
	----------------------------------------------------------------------*/
	ctx = kmalloc(sizeof(struct jenc_ctx), GFP_KERNEL);
	if (ctx == 0) {
		DBG_ERR("Allocate %d bytes fail!\n", sizeof(struct jenc_ctx));
		ret = -EBUSY;
		goto EXIT_jenc_open;
	}
	memset(ctx, 0, sizeof(struct jenc_ctx));

	/*----------------------------------------------------------------------
		private_data from VD is "PRD table virtual & physical address"
	----------------------------------------------------------------------*/
	info = (struct ve_info *)filp->private_data;

	ctx->prd_virt = (unsigned int)info->prdt_virt;
	ctx->prd_phys = (unsigned int)info->prdt_phys;

	DBG_DETAIL("prd_virt: 0x%x, prd_phys: 0x%x\n",
		ctx->prd_virt, ctx->prd_phys);

	/* change privata_data to ctx */
	filp->private_data = ctx;

	ret = wmt_jenc_open(ctx);

EXIT_jenc_open:
	TRACE("Leave\n");

	return ret;
} /* End of jenc_open() */

/*!*************************************************************************
* jenc_release
*
* Private Function
*/
/*!
* \brief
*   Reset hardward and free resource
* \parameter
*   inode   [IN] a pointer point to struct inode
*   filp    [IN] a pointer point to struct file
* \retval  0 if success
*/
static int jenc_release(struct inode *inode, struct file *filp)
{
	struct jenc_ctx *ctx = (struct jenc_ctx *)filp->private_data;

	TRACE("Enter\n");
	/*----------------------------------------------------------------------
		Close hardware encoder
	----------------------------------------------------------------------*/
	if (ctx) {
		wmt_jenc_close(ctx);
		kfree(ctx);
	}
	TRACE("Leave\n");

	return 0;
} /* End of jenc_release() */

/*!*************************************************************************
* jenc_suspend
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
static int jenc_suspend(pm_message_t state)
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
} /* End of jenc_suspend() */

/*!*************************************************************************
* jenc_resume
*
* Private Function
*/
/*!
* \brief
*       resume jpeg module
* \retval  0 if success
*/
static int jenc_resume(void)
{
	TRACE("Enter\n");
	TRACE("Leave\n");

	return 0;
} /* End of jenc_resume() */

/*!*************************************************************************
* jenc_get_info
*
* Private Function
*/
/*!
* \brief
*       resume jpeg module
* \retval  0 if success
*/
static int jenc_get_info(char *buf, char **start, off_t offset, int len)
{
	return 0;
}

/*!*************************************************************************
	platform device struct define
****************************************************************************/

struct videoencoder jpeg_encoder = {
	.name    = "jenc",
	.id      = VE_JPEG,
	.suspend = jenc_suspend,
	.resume  = jenc_resume,
	.fops = {
				.owner   = THIS_MODULE,
				.open    = jenc_open,
				.unlocked_ioctl = jenc_ioctl,
				.release = jenc_release,
			},
	.get_info = jenc_get_info,
	.device = NULL,
	.irq_num      = 0,
	.isr          = NULL,
	.ve_class     = NULL,
};

/*!*************************************************************************
* hw_jenc_init
*
* Private Function
*/
/*!
* \brief
*       init jpeg module
* \retval  0 if success
*/
static int hw_jenc_init(void)
{
	int ret;

	TRACE("Enter\n");

	jpeg_encoder.id      = VE_JPEG;
	jpeg_encoder.irq_num = 0;
	jpeg_encoder.isr     = 0;

	ret = wmt_ve_register(&jpeg_encoder);
	TRACE("Leave\n");

	return ret;
} /* End of hw_jenc_init() */

/*!*************************************************************************
* hw_jenc_exit
*
* Private Function
*/
/*!
* \brief
*
* \retval  0 if success
*/
static void hw_jenc_exit(void)
{
	TRACE("Enter\n");

	wmt_ve_unregister(&jpeg_encoder);

	TRACE("Leave\n");

	return;
} /* End of hw_jenc_exit() */

module_init(hw_jenc_init);
module_exit(hw_jenc_exit);

MODULE_AUTHOR("WonderMedia Technologies, Inc.");
MODULE_DESCRIPTION("WMT JPEG encode device driver");
MODULE_LICENSE("GPL");

/*------------------------------------------------------------------------*/
/*--------------------End of Function Body -------------------------------*/

#undef HW_JENC_C
