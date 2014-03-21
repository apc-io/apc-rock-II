/*++
 * linux/drivers/video/wmt/parse-edid.c
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

#define PARSE_EDID_C
#undef DEBUG
/* #define DEBUG */
/* #define DEBUG_DETAIL */

#include "vpp.h"
#include "edid.h"
#include "hdmi.h"

const char edid_v1_header[] = {
	0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00};

const char edid_v1_descriptor_flag[] = { 0x00, 0x00 };

#define COMBINE_HI_8LO(hi, lo) \
	((((unsigned)hi) << 8) | (unsigned)lo)

#define COMBINE_HI_4LO(hi, lo) \
	((((unsigned)hi) << 4) | (unsigned)lo)

#define UPPER_NIBBLE(x) \
	(((128|64|32|16) & (x)) >> 4)

#define LOWER_NIBBLE(x) \
	((1|2|4|8) & (x))

#define MONITOR_NAME            0xfc
#define MONITOR_LIMITS          0xfd
#define UNKNOWN_DESCRIPTOR      -1
#define DETAILED_TIMING_BLOCK   -2

edid_timing_t edid_establish_timing[] = {
	{ 800, 600, 60 }, { 800, 600, 56 }, { 640, 480, 75 }, { 640, 480, 72 },
	{ 640, 480, 67 }, { 640, 480, 60 }, { 720, 400, 88 }, { 720, 400, 70 },
	{ 1280, 1024, 75 }, { 1024, 768, 75 }, { 1024, 768, 70 },
	{ 1024, 768, 60 }, { 1024, 768, 87 }, { 832, 624, 75 },
	{ 800, 600, 75 }, { 800, 600, 72 }, { 1152, 870, 75 }
};
int edid_msg_enable;
int edid_disable;
edid_parsed_t edid_parsed;

#undef DBGMSG
#define DBGMSG(fmt, args...)	if (edid_msg_enable) \
					DPRINT(fmt, ## args)

static int block_type(char *block)
{
	if (!memcmp(edid_v1_descriptor_flag, block, 2)) {
		/* descriptor */
		if (block[2] != 0)
			return UNKNOWN_DESCRIPTOR;
		return block[3];
	}
	/* detailed timing block */
	return DETAILED_TIMING_BLOCK;
} /* End of block_type() */

static char *get_vendor_sign(char *block, char *sign)
{
	unsigned short h;

	/*
	08h	WORD	big-endian manufacturer ID (see #00136)
		    bits 14-10: first letter (01h='A', 02h='B', etc.)
		    bits 9-5: second letter
		    bits 4-0: third letter
	*/
	h = COMBINE_HI_8LO(block[0], block[1]);
	sign[0] = ((h >> 10) & 0x1f) + 'A' - 1;
	sign[1] = ((h >> 5) & 0x1f) + 'A' - 1;
	sign[2] = (h & 0x1f) + 'A' - 1;
	sign[3] = 0;
	return sign;
} /* End of get_vendor_sign() */

static char *get_monitor_name(char *block, char *name)
{
#define DESCRIPTOR_DATA         5

	char *ptr = block + DESCRIPTOR_DATA;
	unsigned i;

	for (i = 0; i < 13; i++, ptr++) {
		if (*ptr == 0xa) {
			name[i] = 0;
			return name;
		}
		name[i] = *ptr;
	}
	return name;
} /* End of get_monitor_name() */

static int parse_timing_description(char *dtd, edid_info_t *info)
{
#define PIXEL_CLOCK_LO     ((unsigned)dtd[0])
#define PIXEL_CLOCK_HI     ((unsigned)dtd[1])
#define PIXEL_CLOCK  (COMBINE_HI_8LO(PIXEL_CLOCK_HI, PIXEL_CLOCK_LO) * 10000)
#define H_ACTIVE_LO        ((unsigned)dtd[2])
#define H_BLANKING_LO      ((unsigned)dtd[3])
#define H_ACTIVE_HI        UPPER_NIBBLE((unsigned)dtd[4])
#define H_ACTIVE           COMBINE_HI_8LO(H_ACTIVE_HI, H_ACTIVE_LO)
#define H_BLANKING_HI      LOWER_NIBBLE((unsigned)dtd[4])
#define H_BLANKING         COMBINE_HI_8LO(H_BLANKING_HI, H_BLANKING_LO)
#define V_ACTIVE_LO        ((unsigned)dtd[5])
#define V_BLANKING_LO      ((unsigned)dtd[6])
#define V_ACTIVE_HI        UPPER_NIBBLE((unsigned)dtd[7])
#define V_ACTIVE           COMBINE_HI_8LO(V_ACTIVE_HI, V_ACTIVE_LO)
#define V_BLANKING_HI      LOWER_NIBBLE((unsigned)dtd[7])
#define V_BLANKING         COMBINE_HI_8LO(V_BLANKING_HI, V_BLANKING_LO)
#define H_SYNC_OFFSET_LO   ((unsigned)dtd[8])
#define H_SYNC_WIDTH_LO    ((unsigned)dtd[9])
#define V_SYNC_OFFSET_LO   UPPER_NIBBLE((unsigned)dtd[10])
#define V_SYNC_WIDTH_LO    LOWER_NIBBLE((unsigned)dtd[10])
#define V_SYNC_WIDTH_HI    ((unsigned)dtd[11] & (1|2))
#define V_SYNC_OFFSET_HI   (((unsigned)dtd[11] & (4|8)) >> 2)
#define H_SYNC_WIDTH_HI    (((unsigned)dtd[11] & (16|32)) >> 4)
#define H_SYNC_OFFSET_HI   (((unsigned)dtd[11] & (64|128)) >> 6)
#define V_SYNC_WIDTH       COMBINE_HI_4LO(V_SYNC_WIDTH_HI, V_SYNC_WIDTH_LO)
#define V_SYNC_OFFSET      COMBINE_HI_4LO(V_SYNC_OFFSET_HI, V_SYNC_OFFSET_LO)
#define H_SYNC_WIDTH       COMBINE_HI_4LO(H_SYNC_WIDTH_HI, H_SYNC_WIDTH_LO)
#define H_SYNC_OFFSET      COMBINE_HI_4LO(H_SYNC_OFFSET_HI, H_SYNC_OFFSET_LO)
#define H_SIZE_LO          ((unsigned)dtd[12])
#define V_SIZE_LO          ((unsigned)dtd[13])
#define H_SIZE_HI          UPPER_NIBBLE((unsigned)dtd[14])
#define V_SIZE_HI          LOWER_NIBBLE((unsigned)dtd[14])
#define H_SIZE             COMBINE_HI_8LO(H_SIZE_HI, H_SIZE_LO)
#define V_SIZE             COMBINE_HI_8LO(V_SIZE_HI, V_SIZE_LO)
#define H_BORDER           ((unsigned)dtd[15])
#define V_BORDER           ((unsigned)dtd[16])
#define FLAGS              ((unsigned) dtd[17])
#define INTERLACED         (FLAGS & 128)
#define SYNC_TYPE	   (FLAGS & 3 << 3)  /* bits 4,3 */
#define SYNC_SEPARATE	   (3 << 3)
#define HSYNC_POSITIVE	   (FLAGS & 4)
#define VSYNC_POSITIVE     (FLAGS & 2)

	int htotal, vtotal;
	int i;
	struct fb_videomode *t;
	int fps;
	int vmul;

	htotal = H_ACTIVE + H_BLANKING;
	vtotal = V_ACTIVE + V_BLANKING;

	DBGMSG("Detail Timing: \"%dx%d\"\n", H_ACTIVE, V_ACTIVE);
	DBGMSG("\tVfreq %dHz, Hfreq %dkHz\n",
	PIXEL_CLOCK / (vtotal * htotal),
	PIXEL_CLOCK / (htotal * 1000));
	DBGMSG("\tDotClock\t%d\n", PIXEL_CLOCK / 1000000);
	DBGMSG("\tHTimings\t%u %u %u %u\n", H_ACTIVE,
		H_ACTIVE + H_SYNC_OFFSET,
		H_ACTIVE + H_SYNC_OFFSET + H_SYNC_WIDTH,
		htotal);

	DBGMSG("\tVTimings\t%u %u %u %u\n", V_ACTIVE,
	V_ACTIVE + V_SYNC_OFFSET,
	V_ACTIVE + V_SYNC_OFFSET + V_SYNC_WIDTH,
	vtotal);

	if (INTERLACED || (SYNC_TYPE == SYNC_SEPARATE)) {
		DBGMSG("\tFlags\t%s\"%sHSync\" \"%sVSync\"\n",
		INTERLACED ? "\"Interlace\" " : "",
		HSYNC_POSITIVE ? "+" : "-",
		VSYNC_POSITIVE ? "+" : "-");
	}

	for (i = 0; i < 4; i++) {
		t = &info->detail_timing[i];
		if (t->pixclock == 0)
			break;
	}

	if (i >= 4) {
		DBGMSG("*W* slot full\n");
		return 0;
	}

	t->pixclock = KHZ2PICOS(PIXEL_CLOCK / 1000);
	t->right_margin = H_SYNC_OFFSET;
	t->hsync_len = H_SYNC_WIDTH;
	t->xres = H_ACTIVE;
	t->left_margin = H_BLANKING - (H_SYNC_WIDTH + H_SYNC_OFFSET);
	t->vmode = INTERLACED ? FB_VMODE_INTERLACED : FB_VMODE_NONINTERLACED;
	vmul = (INTERLACED) ? 2 : 1;
	t->lower_margin = V_SYNC_OFFSET * vmul;
	t->vsync_len = V_SYNC_WIDTH * vmul;
	t->yres = V_ACTIVE * vmul;
	t->upper_margin = V_BLANKING - (V_SYNC_WIDTH + V_SYNC_OFFSET);

	fps = vpp_calc_refresh(PIXEL_CLOCK, htotal, vtotal);
	t->refresh = fps;
	t->sync = HSYNC_POSITIVE ? (FB_SYNC_HOR_HIGH_ACT) : 0;
	t->sync |= VSYNC_POSITIVE ? (FB_SYNC_VERT_HIGH_ACT) : 0;

	if (vout_check_ratio_16_9(H_ACTIVE, V_ACTIVE))
		info->option |= EDID_OPT_16_9;
#if 0
	DBGMSG("%dx%d,%d,%s\n", H_ACTIVE, V_ACTIVE, t->pixel_clock,
		(info->option & EDID_OPT_16_9) ? "16:9" : "4:3");
#endif
	return 0;
} /* End of parse_timing_description() */

static int parse_dpms_capabilities(char flags)
{
#define DPMS_ACTIVE_OFF		(1 << 5)
#define DPMS_SUSPEND		(1 << 6)
#define DPMS_STANDBY		(1 << 7)

	DBGMSG("# DPMS capabilities: Active off:%s  Suspend:%s  Standby:%s\n\n",
	    (flags & DPMS_ACTIVE_OFF) ? "yes" : "no",
	    (flags & DPMS_SUSPEND)    ? "yes" : "no",
	    (flags & DPMS_STANDBY)    ? "yes" : "no");
	return 0;
} /* End of parse_dpms_capabilities() */

static int parse_monitor_limits(char *block, edid_info_t *info)
{
#define V_MIN_RATE              block[5]
#define V_MAX_RATE              block[6]
#define H_MIN_RATE              block[7]
#define H_MAX_RATE              block[8]
#define MAX_PIXEL_CLOCK         (((int)block[9]) * 10)
#define GTF_SUPPORT             block[10]

	DBGMSG("Monitor limit\n");
	DBGMSG("\tHorizontal Frequency: %u-%u Hz\n", H_MIN_RATE, H_MAX_RATE);
	DBGMSG("\tVertical   Frequency: %u-%u kHz\n", V_MIN_RATE, V_MAX_RATE);
	if (MAX_PIXEL_CLOCK == 10 * 0xff) {
		DBGMSG("\tMax dot clock not given\n");
	} else {
		DBGMSG("\tMax dot clock (video bandwidth) %u MHz\n",
			(int)MAX_PIXEL_CLOCK);
		info->pixel_clock_limit = MAX_PIXEL_CLOCK;
	}

	if (GTF_SUPPORT)
		DBGMSG("\tEDID version 3 GTF given: contact author\n");
	return 0;
} /* End of parse_monitor_limits() */

static int get_established_timing(char *edid, edid_info_t *info)
{
	char time_1, time_2;

	time_1 = edid[ESTABLISHED_TIMING_I];
	time_2 = edid[ESTABLISHED_TIMING_II];
	info->establish_timing = time_1 + (time_2 << 8);

	/*---------------------------------------------------------------------
	35: ESTABLISHED TIMING I
	bit 7-0: 720x400@70 Hz, 720x400@88 Hz, 640x480@60 Hz, 640x480@67 Hz,
	640x480@72 Hz, 640x480@75 Hz, 800x600@56 Hz, 800x600@60 Hz
	---------------------------------------------------------------------*/
	DBGMSG("Established Timimgs I: 0x%x\n", time_1);
	if (time_1 & 0x80)
		DBGMSG("\t%dx%d@%dHz\n", 720, 400, 70);
	if (time_1 & 0x40)
		DBGMSG("\t%dx%d@%dHz\n", 720, 400, 88);
	if (time_1 & 0x20)
		DBGMSG("\t%dx%d@%dHz\n", 640, 480, 60);
	if (time_1 & 0x10)
		DBGMSG("\t%dx%d@%dHz\n", 640, 480, 67);
	if (time_1 & 0x08)
		DBGMSG("\t%dx%d@%dHz\n", 640, 480, 72);
	if (time_1 & 0x04)
		DBGMSG("\t%dx%d@%dHz\n", 640, 480, 75);
	if (time_1 & 0x02)
		DBGMSG("\t%dx%d@%dHz\n", 800, 600, 56);
	if (time_1 & 0x01)
		DBGMSG("\t%dx%d@%dHz\n", 800, 600, 60);

	/*---------------------------------------------------------------------
	36: ESTABLISHED TIMING II
	    bit 7-0: 800x600@72 Hz, 800x600@75 Hz, 832x624@75 Hz,
	    1024x768@87 Hz (Interlaced), 1024x768@60 Hz, 1024x768@70 Hz,
	    1024x768@75 Hz, 1280x1024@75 Hz
	---------------------------------------------------------------------*/
	DBGMSG("Established Timimgs II: 0x%x\n", time_2);
	if (time_2 & 0x80)
		DBGMSG("\t%dx%d@%dHz\n", 800, 600, 72);
	if (time_2 & 0x40)
		DBGMSG("\t%dx%d@%dHz\n", 800, 600, 75);
	if (time_2 & 0x20)
		DBGMSG("\t%dx%d@%dHz\n", 832, 624, 75);
	if (time_2 & 0x10)
		DBGMSG("\t%dx%d@%dHz (Interlace)\n", 1024, 768, 87);
	if (time_2 & 0x08)
		DBGMSG("\t%dx%d@%dHz\n", 1024, 768, 60);
	if (time_2 & 0x04)
		DBGMSG("\t%dx%d@%dHz\n", 1024, 768, 70);
	if (time_2 & 0x02)
		DBGMSG("\t%dx%d@%dHz\n", 1024, 768, 75);
	if (time_2 & 0x01)
		DBGMSG("\t%dx%d@%dHz\n", 1280, 1024, 75);
	return 0;
} /* End of get_established_timing() */

static int get_standard_timing(char *edid, edid_info_t *info)
{
	char *ptr = edid + STANDARD_TIMING_IDENTIFICATION_START;
	int h_res, v_res, v_freq;
	int byte_1, byte_2, aspect, i;

	/*---------------------------------------------------------------------
	First byte
	Horizontal resolution.  Multiply by 8, then add 248 for actual value.
	Second byte
	bit 7-6: Aspect ratio. Actual vertical resolution depends on horizontal
	resolution.
	00=16:10, 01=4:3, 10=5:4, 11=16:9 (00=1:1 prior to v1.3)
	bit 5-0: Vertical frequency. Add 60 to get actual value.
	---------------------------------------------------------------------*/
	DBGMSG("Standard Timing Identification\n");
	for (i = 0; i < STANDARD_TIMING_IDENTIFICATION_SIZE / 2; i++) {
		byte_1 = *ptr++;
		byte_2 = *ptr++;
		if ((byte_1 == 0x01) && (byte_2 == 0x01))
			break;
		h_res = (byte_1 * 8) + 248;
		aspect = byte_2 & 0xC0;
		switch (aspect) {
		default:
		case 0x00:
			v_res = h_res * 10/16;
			break;
		case 0x40:
			v_res = h_res * 3/4;
			break;
		case 0x80:
			v_res = h_res * 4/5;
			break;
		case 0xC0:
			v_res = h_res * 9/16;
			break;
		}
		v_freq = (byte_2 & 0x1F) + 60;
		DBGMSG("\t%dx%d@%dHz\n", h_res, v_res, v_freq);
		info->standard_timing[i].resx = h_res;
		info->standard_timing[i].resy = v_res;
		info->standard_timing[i].freq = v_freq;
	}
	return 0;
} /* End of get_standard_timing() */

static int edid_parse_v1(char *edid, edid_info_t *info)
{
	char *block;
	char *monitor_name = 0;
	char  monitor_alt_name[100];
	char  vendor_sign[4];
	int   i, ret = 0;

	if (edid_checksum(edid, EDID_LENGTH)) {
		DBG_ERR("checksum failed\n");
		ret = -1;
		goto parse_end;
	}

	if (memcmp(edid+EDID_HEADER, edid_v1_header, EDID_HEADER_END+1)) {
		DBGMSG("*E* first bytes don't match EDID version 1 header\n");
		ret = -1;
		goto parse_end;
	}

//#ifdef DEBUG
	if(edid_msg_enable)
		edid_dump(edid);
//#endif

	DBGMSG("[EDID] EDID version:  %d.%d\n",
		(int)edid[EDID_STRUCT_VERSION],
		(int)edid[EDID_STRUCT_REVISION]);

	get_vendor_sign(edid + ID_MANUFACTURER_NAME, (char *) &vendor_sign);

	/*---------------------------------------------------------------------
	Parse Monitor name
	---------------------------------------------------------------------*/
	block = edid + DETAILED_TIMING_DESCRIPTIONS_START;
	for (i = 0; i < NO_DETAILED_TIMING_DESCRIPTIONS; i++,
	block += DETAILED_TIMING_DESCRIPTION_SIZE) {
		if (block_type(block) == MONITOR_NAME) {
			monitor_name =
				get_monitor_name(block, monitor_alt_name);
			break;
		}
	}

	if (!monitor_name) {
		/* Stupid djgpp hasn't snDBGMSG so we have to
			hack something together */
		if (strlen(vendor_sign) + 10 > sizeof(monitor_alt_name))
			vendor_sign[3] = 0;

		sprintf(monitor_alt_name, "%s:%02x%02x",
		vendor_sign, edid[ID_MODEL], edid[ID_MODEL+1]);
		monitor_name = monitor_alt_name;
	}

	DBGMSG("Identifier \"%s\"\n", monitor_name);
	DBGMSG("VendorName \"%s\"\n", vendor_sign);
	DBGMSG("ModelName  \"%s\"\n", monitor_name);

	memset(edid_parsed.tv_name.vendor_name, 0, sizeof(edid_parsed.tv_name.vendor_name));
	strcpy(edid_parsed.tv_name.vendor_name, vendor_sign);

	memset(edid_parsed.tv_name.monitor_name, 0, sizeof(edid_parsed.tv_name.monitor_name));
	if(strlen(monitor_name) < MONITOR_NAME_LEN)
		strcpy(edid_parsed.tv_name.monitor_name, monitor_name);
	else
		strncpy(edid_parsed.tv_name.monitor_name, monitor_name, MONITOR_NAME_LEN - 1);

	parse_dpms_capabilities(edid[DPMS_FLAGS]);

	/*---------------------------------------------------------------------
	Parse ESTABLISHED TIMING I and II
	---------------------------------------------------------------------*/
	get_established_timing(edid, info);

	/*---------------------------------------------------------------------
	Parse STANDARD TIMING IDENTIFICATION
	---------------------------------------------------------------------*/
	get_standard_timing(edid, info);

	block = edid + DETAILED_TIMING_DESCRIPTIONS_START;
	for (i = 0; i < NO_DETAILED_TIMING_DESCRIPTIONS; i++,
		block += DETAILED_TIMING_DESCRIPTION_SIZE) {
		if (block_type(block) == MONITOR_LIMITS)
			parse_monitor_limits(block, info);
	}

	block = edid + DETAILED_TIMING_DESCRIPTIONS_START;
	for (i = 0; i < NO_DETAILED_TIMING_DESCRIPTIONS; i++,
		block += DETAILED_TIMING_DESCRIPTION_SIZE) {
		if (block_type(block) == DETAILED_TIMING_BLOCK)
			parse_timing_description(block, info);
	}
parse_end:
	return ret;
}

void edid_parse_CEA_VendorSpecDataBlock(char *block, int len,
						edid_info_t *info)
{
	int index;
	char temp;

	DBGMSG("Vendor Spec Data Block\n");
	if (len < 5) /* min size */
		return;
	/* IEEE Registration Identifier 0x000C03 */
	if ((block[1] == 0x03) && (block[2] == 0x0C) &&
		(block[3] == 0x0)) {
		info->option |= EDID_OPT_HDMI;
		DBGMSG("\t support HDMI\n");
	}
	DBGMSG("\t Source Physical Addr %d.%d.%d.%d\n",
		(block[4] & 0xF0) >> 4, block[4] & 0x0F,
		(block[5] & 0xF0) >> 4, block[5] & 0x0F);
	info->hdmi_phy_addr = (block[4] << 8) + block[5];

	/* extersion fields */
	if (len < 8)
		return;
	DBGMSG("\t%s support AI\n",
		(block[6] & 0x80) ? "" : "no");
	DBGMSG("\t%s support 30 bits/pixel(10 bits/color)\n",
		(block[6] & 0x40) ? "" : "no");
	DBGMSG("\t%s support 36 bits/pixel(12 bits/color)\n",
		(block[6] & 0x20) ? "" : "no");
	DBGMSG("\t%s support 48 bits/pixel(16 bits/color)\n",
		(block[6] & 0x10) ? "" : "no");
	DBGMSG("\t%s support YUV444 in Deep Color mode\n",
		(block[6] & 0x08) ? "" : "no");
	DBGMSG("\t%s support DVI dual-link\n",
		(block[6] & 0x01) ? "" : "no");
	DBGMSG("\tMax TMDS Clock %d MHz\n", block[7] * 5);
	temp = block[8];
	index = 9;

	if (temp & BIT(7)) {
		DBGMSG("\tVideo Latency %d,Audio Latency %d\n",
			block[index], block[index+1]);
		index += 2;
	}

	if (temp & BIT(6)) {
		DBGMSG("\tInterlaced Video Latency %d,Audio Latency %d\n",
			block[index], block[index+1]);
		index += 2;
	}

	if (temp & BIT(5)) {
		int hdmi_xx_len, hdmi_3d_len;

		DBGMSG("\tHDMI Video present\n");
		temp = block[index];
		if (temp & 0x80)
			DBGMSG("\t\t3D present support Mandatory formats\n");
		if (temp & 0x60)
			DBGMSG("\t\t3D Multi present %d\n", (temp & 0x60) >> 5);
		DBGMSG("\t\tImage size %d\n", (temp & 0x18) >> 3);
		hdmi_xx_len = (block[index + 1] & 0xE0) >> 5;
		hdmi_3d_len = block[index + 1] & 0x1F;
		DBGMSG("\t\thdmi_xx_len %d,hdmi_3d_len %d\n",
			hdmi_xx_len, hdmi_3d_len);
		index += 2;

		if (hdmi_xx_len) {
			/* Refer to HDMI Spec Ver 1.4a */
			index += hdmi_xx_len;
		}

		if (hdmi_3d_len) {
			int struct_all_3d = 0;
			int mask_3d = 0;
			char vic_order_2d, struct_3d, detail_3d;

			hdmi_3d_len += index;
			switch (temp & 0x60) {
			case 0x40:
				struct_all_3d = (block[index] << 8) +
					block[index + 1];
				mask_3d = (block[index + 2] << 8) +
					block[index + 3]; /* 3D support mask */
				DBGMSG("\t\t3D struct 0x%x,mask 0x%x\n",
					struct_all_3d, mask_3d);
				index += 4;
				break;
			case 0x20:
				struct_all_3d = (block[index] << 8) +
					block[index + 1];
				DBGMSG("\t\t3D struct 0x%x\n", struct_all_3d);
				index += 2;
				break;
			default:
				break;
			}
			/* 3D support type */
			if (struct_all_3d & BIT0)
				DBGMSG("\t\tSupport Frame packing\n");
			if (struct_all_3d & BIT6)
				DBGMSG("\t\tSupport Top and Bottom\n");
			if (struct_all_3d & BIT8)
				DBGMSG("\t\tSupport Side by Side\n");

			DBGMSG("\t\t[3D Structure type 0-Frame packing");
			DBGMSG(",6-Top and Bottom,8-Side by Side]\n");
			while (index < hdmi_3d_len) {
				vic_order_2d = (block[index] & 0xF0) >> 4;
				struct_3d = block[index] & 0x0F;
				index++;
				if (struct_3d & 0x8) {
					detail_3d = (block[index] & 0xF0) >> 4;
					index++;
				} else {
					detail_3d = 0;
				}
				DBGMSG("\t\tVIC %d,3D struct %d,detail %d\n",
					vic_order_2d, struct_3d, detail_3d);
			}
		}
	}
}

static int edid_parse_CEA(char *edid, edid_info_t *info)
{
	char *block, *block_end;
	char checksum = 0;
	int i, len;
	char audio_format;
	int num = 0, sadcnt_in_block;

	memset(edid_parsed.sad, 0, sizeof(edid_parsed.sad));

	if (edid[0] != 0x2)
		return -1;

	for (i = 0; i < EDID_LENGTH; i++)
		checksum += edid[i];

	if (checksum != 0) {
		DPRINT("*E* CEA EDID checksum (0x%02x) failed - data is corrupt\n", checksum);
		info->option |= (EDID_OPT_HDMI + EDID_OPT_AUDIO);
		return -1;
	}

//#ifdef DEBUG
	if(edid_msg_enable)
		edid_dump(edid);
//#endif

	DBGMSG("[EDID] CEA EDID Version %d.%d\n", edid[0], edid[1]);

	if (edid[1] >= 2) {
		info->option |= (edid[3] & 0xF0);
		DBGMSG("\t%s support 422\n", (edid[3] & 0x10) ? "" : "no");
		DBGMSG("\t%s support 444\n", (edid[3] & 0x20) ? "" : "no");
		DBGMSG("\t%s support audio\n", (edid[3] & 0x40) ? "" : "no");
		DBGMSG("\t%s support underscan\n",
			(edid[3] & 0x80) ? "" : "no");
	}

	block_end = edid + edid[2];
	block = edid + 4;
	do {
		len = block[0] & 0x1F;
		switch (((block[0] & 0xE0)>>5)) {
		case 1:	/* Audio Data Block */
			DBGMSG("Audio Data Block (0x%02X)\n", block[0]);
			info->option |= EDID_OPT_AUDIO;
			sadcnt_in_block = len / 3;

			for(i = 0; i < sadcnt_in_block; i++){
				if(num < AUD_SAD_NUM) {
					edid_parsed.sad[num].flag = 1;
					edid_parsed.sad[num].sad_byte[0] = block[i * 3 + 1];
					edid_parsed.sad[num].sad_byte[1] = block[i * 3 + 2];
					edid_parsed.sad[num].sad_byte[2] = block[i * 3 + 3];
					num++;
				} else {
					DPRINT("Lose SAD info: 0x%02X 0x%02X 0x%02X\n",
						block[i * 3 + 1], block[i * 3 + 2], block[i * 3 + 3]);
				}

				DBGMSG("\t ======== SDA %d ========\n", i);
				DBGMSG("\t SDA Data: 0x%02X 0x%02X 0x%02X\n",
					block[i * 3 + 1], block[i * 3 + 2], block[i * 3 + 3]);

				audio_format = (block[i * 3 + 1] & 0x78) >> 3;
				switch (audio_format) {
				default:
				case 0: /* reserved */
				case 15:/* reserved */
					DBGMSG("\t Reserved Audio Fmt\n");
				break;
				case 1: /* LPCM */
					DBGMSG("\t Audio Fmt: LPCM\n");
				break;
				case 2: /* AC3 */
					DBGMSG("\t Audio Fmt: AC3\n");
				break;
				case 3: /* MPEG1 */
					DBGMSG("\t Audio Fmt: MPEG1\n");
				break;
				case 4: /* MP3 */
					DBGMSG("\t Audio Fmt: MP3\n");
				break;
				case 5: /* MPEG2 */
					DBGMSG("\t Audio Fmt: MPEG2\n");
				break;
				case 6: /* AAC */
					DBGMSG("\t Audio Fmt: AAC\n");
				break;
				case 7: /* DTS */
					DBGMSG("\t Audio Fmt: DTS\n");
				break;
				case 8: /* ATRAC */
					DBGMSG("\t Audio Fmt: ATRAC\n");
				break;
				case 9: /* One bit audio */
					DBGMSG("\t Audio Fmt: ONE BIT AUDIO\n");
				break;
				case 10:/* Dolby */
					DBGMSG("\t Audio Fmt: DOLBY\n");
				break;
				case 11:/* DTS-HD */
					DBGMSG("\t Audio Fmt: DTS-HD\n");
				break;
				case 12:/* MAT (MLP) */
					DBGMSG("\t Audio Fmt: MAT\n");
				break;
				case 13:/* DST */
					DBGMSG("\t Audio Fmt: DST\n");
				break;
				case 14:/* WMA Pro */
					DBGMSG("\t Audio Fmt: WMA Pro\n");
				break;
				}

				DBGMSG("\t Max channel: %d\n", (block[i * 3 + 1] & 0x7) + 1);
				DBGMSG("\t %s support 32 KHz\n",
					(block[i * 3 + 2] & 0x1) ? "" : "no");
				DBGMSG("\t %s support 44 KHz\n",
					(block[i * 3 + 2] & 0x2) ? "" : "no");
				DBGMSG("\t %s support 48 KHz\n",
					(block[i * 3 + 2] & 0x4) ? "" : "no");
				DBGMSG("\t %s support 88 KHz\n",
					(block[i * 3 + 2] & 0x8) ? "" : "no");
				DBGMSG("\t %s support 96 KHz\n",
					(block[i * 3 + 2] & 0x10) ? "" : "no");
				DBGMSG("\t %s support 176 KHz\n",
					(block[i * 3 + 2] & 0x20) ? "" : "no");
				DBGMSG("\t %s support 192 KHz\n",
					(block[i * 3 + 2] & 0x40) ? "" : "no");
				if(audio_format == 1) { /* For LPCM */
					DBGMSG("\t %s support 16 bit\n",
						(block[i * 3 + 3] & 0x1) ? "" : "no");
					DBGMSG("\t %s support 20 bit\n",
						(block[i * 3 + 3] & 0x2) ? "" : "no");
					DBGMSG("\t %s support 24 bit\n",
						(block[i * 3 + 3] & 0x4) ? "" : "no");
				} else if(audio_format >= 2 && audio_format <= 8) { /* From AC3 to ATRAC */
					DBGMSG("\t Max bitrate: %d kbit/s\n", block[i * 3 + 3] * 8);
				}
				else if(audio_format >= 9 && audio_format <= 14) { /* From One-bit-audio to WMA Pro*/
					DBGMSG("\t Audio Format Code dependent value: 0x%02X\n", block[i * 3 + 3]);
				}
				DBGMSG("\t ========================\n");
			}
			break;
		case 2:	/* Video Data Block */
			DBGMSG("Video Data Block\n");
			for (i = 0; i < len; i++) {
				unsigned int vic;

				vic = block[1 + i] & 0x7F;
				info->cea_vic[vic / 8] |= (0x1 << (vic % 8));
				DBGMSG("\t %2d : VIC %2d %dx%d@%d%s %s\n", i,
					vic, hdmi_vic_info[vic].resx,
					hdmi_vic_info[vic].resy,
					hdmi_vic_info[vic].freq,
					(hdmi_vic_info[vic].option
					& HDMI_VIC_INTERLACE) ? "I" : "P",
					(hdmi_vic_info[vic].option
					& HDMI_VIC_4x3) ? "4:3" : "16:9");
			}
			break;
		case 3: /* Vendor Spec Data Block */
			edid_parse_CEA_VendorSpecDataBlock(block, len, info);
			break;
		case 4:	/* Speaker Allocation Data Block */
			DBGMSG("Speaker Allocation Data Block\n");
			break;
		case 5:	/* VESA DTC Data Block */
			DBGMSG("VESA DTC Data Block\n");
			break;
		case 7:	/* Use Extended Tag */
			DBGMSG("Use Extended Tag\n");
			break;
		case 0:	/* Reserved */
		default:
			len = 0;
			break;
		}
		block += (1 + len);
	} while (block < block_end);

	{
	struct fb_videomode *p;
	unsigned int fps;

	DBGMSG("Detail Timing\n");
	block = edid + edid[2];
	len = (128 - edid[2]) / 18;
	for (i = 0; i < len; i++, block += 18) {
		p = &info->cea_timing[i];
		p->pixclock = ((block[1] << 8) + block[0]) * 10000;
		if (p->pixclock == 0)
			break;
		p->xres = ((block[4] & 0xF0) << 4) + block[2];
		p->left_margin = ((block[4] & 0x0F) << 8) + block[3];
		p->yres = ((block[7] & 0xF0) << 4) + block[5];
		p->upper_margin = ((block[7] & 0x0F) << 8) + block[6];
		fps = vpp_calc_refresh(p->pixclock, p->xres + p->left_margin,
			p->yres + p->upper_margin);
		p->right_margin = ((block[11] & 0xC0) << 2) + block[8];
		p->hsync_len = ((block[11] & 0x30) << 4) + block[9];
		p->left_margin = p->left_margin -
			p->right_margin - p->hsync_len;
		p->lower_margin = ((block[11] & 0x0C) << 2) +
			((block[10] & 0xF0) >> 4);
		p->vsync_len = ((block[11] & 0x03) << 4) + (block[10] & 0x0F);
		p->upper_margin = p->upper_margin -
			p->lower_margin - p->vsync_len;
		p->refresh = fps;
		p->vmode = (block[17] & 0x80) ?
			FB_VMODE_INTERLACED : FB_VMODE_NONINTERLACED;
		p->sync = (block[17] & 0x04) ? FB_SYNC_VERT_HIGH_ACT : 0;
		p->sync |= (block[17] & 0x02) ? FB_SYNC_HOR_HIGH_ACT : 0;
		if (p->vmode & FB_VMODE_INTERLACED) {
			p->yres *= 2;
			p->upper_margin *= 2;
			p->lower_margin *= 2;
			p->vsync_len *= 2;
		}
		DBGMSG("\t%dx%d%s@%d,clk %d\n", p->xres, p->yres,
			(block[17] & 0x80) ? "I" : "P", fps, p->pixclock);
		DBGMSG("\t\tH bp %d,sync %d,fp %d\n",
			p->left_margin, p->hsync_len, p->right_margin);
		DBGMSG("\t\tV bp %d,sync %d,fp %d\n",
			p->upper_margin, p->vsync_len, p->lower_margin);
		DBGMSG("\t\thsync %d,vsync %d\n",
			(p->sync & FB_SYNC_HOR_HIGH_ACT) ? 1 : 0,
			(p->sync & FB_SYNC_VERT_HIGH_ACT) ? 1 : 0);
			p->pixclock = KHZ2PICOS(p->pixclock / 1000);
	}
	}
	return 0;
}

void edid_dump(char *edid)
{
	int i;

	DPRINT("===================== EDID BlOCK =====================");
	for (i = 0; i < 128; i++) {
		if ((i % 16) == 0)
			DPRINT("\n");
		DPRINT("%02x ", edid[i]);
	}
	DPRINT("\n");
	DPRINT("======================================================\n");
}

int edid_checksum(char *edid, int len)
{
	char checksum = 0;
	int i;

	for (i = 0; i < len; i++)
		checksum += edid[i];

	if (checksum) {
#ifdef DEBUG
		edid_dump(edid);
#endif
	}
	return checksum;
}

int edid_parse(char *edid, edid_info_t *info)
{
	int ext_cnt = 0;

	if (edid == 0)
		return 0;

	if (info->option & EDID_OPT_VALID) {
		DBG_MSG("[EDID] parse exist\n");
		return info->option;
	}

	DBG_MSG("[EDID] Enter\n");

	memset(info, 0, sizeof(edid_info_t));
	info->option = EDID_OPT_VALID;
	if (edid_parse_v1(edid, info) == 0) {
		ext_cnt = edid[0x7E];
		if (ext_cnt >= EDID_BLOCK_MAX) {
			DPRINT("[EDID] *W* ext block cnt %d\n", ext_cnt);
			ext_cnt = EDID_BLOCK_MAX - 1;
		}
	} else {
		info->option = 0;
		return 0;
	}

	while (ext_cnt) {
		edid += 128;
		ext_cnt--;
		if (edid_parse_CEA(edid, info) == 0)
			continue;

		DPRINT("*W* not support EDID\n");
		edid_dump(edid);
	}
	return info->option;
}

int edid_find_support(edid_info_t *info, unsigned int resx,
	unsigned int resy, int freq, struct fb_videomode **vmode)
{
	struct fb_videomode *p;
	int temp;
	int ret;
	int i;

	ret = 0;
	*vmode = 0;

	if (!info)
		return 0;

	/* find cea timing */
	for (i = 0; i < 6; i++) {
		p = &info->cea_timing[i];

		if (p->pixclock == 0)
			continue;
		if (resx != p->xres)
			continue;
		if (resy != p->yres)
			continue;

		temp = p->refresh;
		temp |= (p->vmode & FB_VMODE_INTERLACED) ?
			EDID_TMR_INTERLACE : 0;
		if (freq != temp)
			continue;

		*vmode = p;
		ret = 4;
		goto find_end;
	}

	/* find vic timing */
	for (i = 0; i < 64; i++) {
		if ((info->cea_vic[i / 8] & (0x1 << (i % 8))) == 0)
			continue;

		if (i >= HDMI_VIDEO_CODE_MAX)
			continue;

		if (resx != hdmi_vic_info[i].resx)
			continue;
		if (resy != hdmi_vic_info[i].resy)
			continue;
		temp = hdmi_vic_info[i].freq;
		temp |= (hdmi_vic_info[i].option & HDMI_VIC_INTERLACE) ?
			EDID_TMR_INTERLACE : 0;
		if (freq != temp)
			continue;
		ret = 5;
		goto find_end;
	}

	/* find detail timing */
	for (i = 0; i < 4; i++) {
		p = &info->detail_timing[i];
		if (p->pixclock == 0)
			continue;
		if (resx != p->xres)
			continue;
		if (resy != p->yres)
			continue;

		temp = p->refresh;
		temp |= (p->vmode & FB_VMODE_INTERLACED) ?
			EDID_TMR_INTERLACE : 0;
		if (freq != temp)
			continue;
		*vmode = p;
		ret = 3;
		goto find_end;
	}

	/* find established timing */
	if (info->establish_timing) {
		for (i = 0; i < 17; i++) {
			if (info->establish_timing & (0x1 << i)) {
				if ((resx == edid_establish_timing[i].resx) &&
				(resy == edid_establish_timing[i].resy)) {
					if (freq ==
						edid_establish_timing[i].freq) {
						ret = 1;
						goto find_end;
					}
				}
			}
		}
	}

	/* find standard timing */
	for (i = 0; i < 8; i++) {
		if (info->standard_timing[i].resx == 0)
			continue;
		if ((resx == info->standard_timing[i].resx) &&
			(resy == info->standard_timing[i].resy)) {
			if (freq == info->standard_timing[i].freq) {
				ret = 2;
				goto find_end;
			}
		}
	}

find_end:
#if 0
	if ((resx == 1920) && (resy == 1080) && !(freq & EDID_TMR_INTERLACE))
		ret = 0;
#endif
#if 0
	if (!(freq & EDID_TMR_INTERLACE))
		ret = 0;
#endif
#if 0
	DPRINT("[EDID] %s support %dx%d@%d%s(ret %d)\n",
		(ret) ? "" : "No", resx, resy, freq & EDID_TMR_FREQ,
		(freq & EDID_TMR_INTERLACE) ? "I" : "P", ret);
#endif
	return ret;
}

unsigned int edid_get_hdmi_phy_addr(void)
{
	vout_t *vo;

	vo = vout_get_entry(VPP_VOUT_NUM_HDMI);
	return vo->edid_info.hdmi_phy_addr;
}

