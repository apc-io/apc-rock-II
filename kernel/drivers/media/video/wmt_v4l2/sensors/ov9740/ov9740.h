/*++
	linux/drivers/media/video/wmt_v4l2/sensors/ov9740/ov9740.h

	Copyright (c) 2013  WonderMedia Technologies, Inc.

	This program is free software: you can redistribute it and/or modify it under the
	terms of the GNU General Public License as published by the Free Software Foundation,
	either version 2 of the License, or (at your option) any later version.

	This program is distributed in the hope that it will be useful, but WITHOUT
	ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
	PARTICULAR PURPOSE.  See the GNU General Public License for more details.
	You should have received a copy of the GNU General Public License along with
	this program.  If not, see <http://www.gnu.org/licenses/>.

	WonderMedia Technologies, Inc.
	10F, 529, Chung-Cheng Road, Hsin-Tien, Taipei 231, R.O.C.
--*/

#ifndef OV9740_H
#define OV9740_H

#define STREAMING_DELAY_MS	300

#define OV9740_MODEL_ID_HI		0x0000
#define OV9740_MODEL_ID_LO		0x0001
#define OV9740_REVISION_NUMBER		0x0002
#define OV9740_MANUFACTURER_ID		0x0003
#define OV9740_SMIA_VERSION		0x0004

/* General Setup Registers */
#define OV9740_MODE_SELECT		0x0100
#define OV9740_IMAGE_ORT		0x0101
#define OV9740_SOFTWARE_RESET		0x0103
#define OV9740_GRP_PARAM_HOLD		0x0104
#define OV9740_MSK_CORRUP_FM		0x0105

/* Timing Setting */
#define OV9740_FRM_LENGTH_LN_HI		0x0340 /* VTS */
#define OV9740_FRM_LENGTH_LN_LO		0x0341 /* VTS */
#define OV9740_LN_LENGTH_PCK_HI		0x0342 /* HTS */
#define OV9740_LN_LENGTH_PCK_LO		0x0343 /* HTS */
#define OV9740_X_ADDR_START_HI		0x0344
#define OV9740_X_ADDR_START_LO		0x0345
#define OV9740_Y_ADDR_START_HI		0x0346
#define OV9740_Y_ADDR_START_LO		0x0347
#define OV9740_X_ADDR_END_HI		0x0348
#define OV9740_X_ADDR_END_LO		0x0349
#define OV9740_Y_ADDR_END_HI		0x034A
#define OV9740_Y_ADDR_END_LO		0x034B
#define OV9740_X_OUTPUT_SIZE_HI		0x034C
#define OV9740_X_OUTPUT_SIZE_LO		0x034D
#define OV9740_Y_OUTPUT_SIZE_HI		0x034E
#define OV9740_Y_OUTPUT_SIZE_LO		0x034F

/* IO Control Registers */
#define OV9740_IO_CREL00		0x3002
#define OV9740_IO_CREL01		0x3004
#define OV9740_IO_CREL02		0x3005
#define OV9740_IO_OUTPUT_SEL01		0x3026
#define OV9740_IO_OUTPUT_SEL02		0x3027

/* AWB Registers */
#define OV9740_AWB_MANUAL_CTRL		0x3406

/* Analog Control Registers */
#define OV9740_ANALOG_CTRL01		0x3601
#define OV9740_ANALOG_CTRL02		0x3602
#define OV9740_ANALOG_CTRL03		0x3603
#define OV9740_ANALOG_CTRL04		0x3604
#define OV9740_ANALOG_CTRL10		0x3610
#define OV9740_ANALOG_CTRL12		0x3612
#define OV9740_ANALOG_CTRL20		0x3620
#define OV9740_ANALOG_CTRL21		0x3621
#define OV9740_ANALOG_CTRL22		0x3622
#define OV9740_ANALOG_CTRL30		0x3630
#define OV9740_ANALOG_CTRL31		0x3631
#define OV9740_ANALOG_CTRL32		0x3632
#define OV9740_ANALOG_CTRL33		0x3633

/* Sensor Control */
#define OV9740_SENSOR_CTRL03		0x3703
#define OV9740_SENSOR_CTRL04		0x3704
#define OV9740_SENSOR_CTRL05		0x3705
#define OV9740_SENSOR_CTRL07		0x3707

/* Timing Control */
#define OV9740_TIMING_CTRL17		0x3817
#define OV9740_TIMING_CTRL19		0x3819
#define OV9740_TIMING_CTRL33		0x3833
#define OV9740_TIMING_CTRL35		0x3835

/* Banding Filter */
#define OV9740_AEC_MAXEXPO_60_H		0x3A02
#define OV9740_AEC_MAXEXPO_60_L		0x3A03
#define OV9740_AEC_B50_STEP_HI		0x3A08
#define OV9740_AEC_B50_STEP_LO		0x3A09
#define OV9740_AEC_B60_STEP_HI		0x3A0A
#define OV9740_AEC_B60_STEP_LO		0x3A0B
#define OV9740_AEC_CTRL0D		0x3A0D
#define OV9740_AEC_CTRL0E		0x3A0E
#define OV9740_AEC_MAXEXPO_50_H		0x3A14
#define OV9740_AEC_MAXEXPO_50_L		0x3A15

/* AEC/AGC Control */
#define OV9740_AEC_ENABLE		0x3503
#define OV9740_GAIN_CEILING_01		0x3A18
#define OV9740_GAIN_CEILING_02		0x3A19
#define OV9740_AEC_HI_THRESHOLD		0x3A11
#define OV9740_AEC_3A1A			0x3A1A
#define OV9740_AEC_CTRL1B_WPT2		0x3A1B
#define OV9740_AEC_CTRL0F_WPT		0x3A0F
#define OV9740_AEC_CTRL10_BPT		0x3A10
#define OV9740_AEC_CTRL1E_BPT2		0x3A1E
#define OV9740_AEC_LO_THRESHOLD		0x3A1F

/* BLC Control */
#define OV9740_BLC_AUTO_ENABLE		0x4002
#define OV9740_BLC_MODE			0x4005

/* VFIFO */
#define OV9740_VFIFO_READ_START_HI	0x4608
#define OV9740_VFIFO_READ_START_LO	0x4609

/* DVP Control */
#define OV9740_DVP_VSYNC_CTRL02		0x4702
#define OV9740_DVP_VSYNC_MODE		0x4704
#define OV9740_DVP_VSYNC_CTRL06		0x4706

/* PLL Setting */
#define OV9740_PLL_MODE_CTRL01		0x3104
#define OV9740_PRE_PLL_CLK_DIV		0x0305
#define OV9740_PLL_MULTIPLIER		0x0307
#define OV9740_VT_SYS_CLK_DIV		0x0303
#define OV9740_VT_PIX_CLK_DIV		0x0301
#define OV9740_PLL_CTRL3010		0x3010
#define OV9740_VFIFO_CTRL00		0x460E

/* ISP Control */
#define OV9740_ISP_CTRL00		0x5000
#define OV9740_ISP_CTRL01		0x5001
#define OV9740_ISP_CTRL03		0x5003
#define OV9740_ISP_CTRL05		0x5005
#define OV9740_ISP_CTRL12		0x5012
#define OV9740_ISP_CTRL19		0x5019
#define OV9740_ISP_CTRL1A		0x501A
#define OV9740_ISP_CTRL1E		0x501E
#define OV9740_ISP_CTRL1F		0x501F
#define OV9740_ISP_CTRL20		0x5020
#define OV9740_ISP_CTRL21		0x5021

/* AWB */
#define OV9740_AWB_CTRL00		0x5180
#define OV9740_AWB_CTRL01		0x5181
#define OV9740_AWB_CTRL02		0x5182
#define OV9740_AWB_CTRL03		0x5183
#define OV9740_AWB_ADV_CTRL01		0x5184
#define OV9740_AWB_ADV_CTRL02		0x5185
#define OV9740_AWB_ADV_CTRL03		0x5186
#define OV9740_AWB_ADV_CTRL04		0x5187
#define OV9740_AWB_ADV_CTRL05		0x5188
#define OV9740_AWB_ADV_CTRL06		0x5189
#define OV9740_AWB_ADV_CTRL07		0x518A
#define OV9740_AWB_ADV_CTRL08		0x518B
#define OV9740_AWB_ADV_CTRL09		0x518C
#define OV9740_AWB_ADV_CTRL10		0x518D
#define OV9740_AWB_ADV_CTRL11		0x518E
#define OV9740_AWB_CTRL0F		0x518F
#define OV9740_AWB_CTRL10		0x5190
#define OV9740_AWB_CTRL11		0x5191
#define OV9740_AWB_CTRL12		0x5192
#define OV9740_AWB_CTRL13		0x5193
#define OV9740_AWB_CTRL14		0x5194

/* MIPI Control */
#define OV9740_MIPI_CTRL00		0x4800
#define OV9740_MIPI_3837		0x3837
#define OV9740_MIPI_CTRL01		0x4801
#define OV9740_MIPI_CTRL03		0x4803
#define OV9740_MIPI_CTRL05		0x4805
#define OV9740_VFIFO_RD_CTRL		0x4601
#define OV9740_MIPI_CTRL_3012		0x3012
#define OV9740_SC_CMMM_MIPI_CTR		0x3014

// Scene Mode
uint32_t ov9740_scene_mode_auto[] = {};

uint32_t ov9740_scene_mode_night[] = {};


// White Balance
uint32_t ov9740_wb_auto [] = {};

uint32_t ov9740_wb_incandescent [] = {};

uint32_t ov9740_wb_fluorescent [] = {};

uint32_t ov9740_wb_daylight [] = {};

uint32_t ov9740_wb_cloudy [] = {};

uint32_t ov9740_wb_tungsten [] = {};


// Exposure
uint32_t ov9740_exposure_neg6[] = {};

uint32_t ov9740_exposure_neg3[] = {};

uint32_t ov9740_exposure_zero[] = {};

uint32_t ov9740_exposure_pos3[] = {};

uint32_t ov9740_exposure_pos6[] = {};

/*
 * The color effect settings
 */
uint32_t ov9740_colorfx_none[] = {};

uint32_t ov9740_colorfx_bw[] = {};

uint32_t ov9740_colorfx_sepia[] = {};

uint32_t ov9740_colorfx_negative[] = {};

uint32_t ov9740_colorfx_emboss[] = {

};

uint32_t ov9740_colorfx_sketch[] = {
};

uint32_t ov9740_colorfx_sky_blue[] = {
};

uint32_t ov9740_colorfx_grass_green[] = {
};

uint32_t ov9740_colorfx_skin_whiten[] = {
};

uint32_t ov9740_colorfx_vivid[] = {
};

// Brightness
uint32_t ov9740_brightness_neg4[] = {};

uint32_t ov9740_brightness_neg3[] = {};

uint32_t ov9740_brightness_neg2[] = {};

uint32_t ov9740_brightness_neg1[] = {};

uint32_t ov9740_brightness_zero[] = {};

uint32_t ov9740_brightness_pos1[] = {};

uint32_t ov9740_brightness_pos2[] = {};

uint32_t ov9740_brightness_pos3[] = {};

uint32_t ov9740_brightness_pos4[] = {};

// Contrast
uint32_t ov9740_contrast_neg4[] = {};

uint32_t ov9740_contrast_neg3[] = {};

uint32_t ov9740_contrast_neg2[] = {};

uint32_t ov9740_contrast_neg1[] = {};

uint32_t ov9740_contrast_zero[] = {};

uint32_t ov9740_contrast_pos1[] = {};

uint32_t ov9740_contrast_pos2[] = {};

uint32_t ov9740_contrast_pos3[] = {};

uint32_t ov9740_contrast_pos4[] = {};

// Saturation
uint32_t ov9740_saturation_neg4[] = {};

uint32_t ov9740_saturation_neg3[] = {};

uint32_t ov9740_saturation_neg2[] = {};

uint32_t ov9740_saturation_neg1[] = {};

uint32_t ov9740_saturation_zero[] = {};

uint32_t ov9740_saturation_pos1[] = {};

uint32_t ov9740_saturation_pos2[] = {};

uint32_t ov9740_saturation_pos3[] = {};

uint32_t ov9740_saturation_pos4[] = {};


// Resolution
uint32_t ov9740_640_480_regs[]={
	0x0101,	0x01 ,
	0x3104, 0x20 , 
	0x0305, 0x03 ,  
	0x0307, 0x5b ,  
	0x0303, 0x02 ,  
	0x0301, 0x0a ,  
	0x3010, 0x01 ,  
	0x300e, 0x12 ,   
	0x0340, 0x03 ,  
	0x0341, 0x07 ,  
	0x0342, 0x06 ,  
	0x0343, 0x62 ,  
	0x0344, 0x00 ,  
	0x0345, 0xa8 ,  
	0x0346, 0x00 ,  
	0x0347, 0x04 ,  
	0x0348, 0x04 ,  
	0x0349, 0x67 ,  
	0x034a, 0x02 ,  
	0x034b, 0xd8 ,   
	0x034c, 0x02 ,  
	0x034d, 0x80 ,  
	0x034e, 0x01 ,  
	0x034f, 0xe0 ,                    
	0x3707, 0x11 ,
	0x3833, 0x04 , 
	0x3835, 0x04 , 
	0x3819, 0x6e , 
	0x3817, 0x94 , 
	0x3831, 0x40 ,
	0x381a, 0x00 ,
	0x4608, 0x02 ,
	0x4609, 0x70 ,
	0x5001, 0xff ,  
	0x5003, 0xff ,
	0x501e, 0x03 ,
	0x501f, 0xc0 , 
	0x5020, 0x02 ,
	0x5021, 0xd0 , 
	0x530d, 0x06 ,
	0x0100, 0x01 ,
};

uint32_t ov9740_800_600_regs[]={};

uint32_t ov9740_1280_720_regs[]={
	0x0101, 0x01 ,  
	0x3104, 0x20 ,  
	0x0305, 0x03 ,  
	0x0307, 0x5f ,  
	0x0303, 0x01 ,  
	0x0301, 0x0a ,  
	0x3010, 0x01 ,  
	0x300e, 0x01 ,    
	0x0340, 0x03 ,  
	0x0341, 0x07 ,  
	0x0342, 0x06 ,  
	0x0343, 0x62 ,  
	0x0344, 0x00 ,  
	0x0345, 0x08 ,  
	0x0346, 0x00 ,  
	0x0347, 0x04 ,  
	0x0348, 0x05 ,  
	0x0349, 0x0c ,  
	0x034a, 0x02 ,  
	0x034b, 0xd8 ,   
	0x034c, 0x05 ,  
	0x034d, 0x00 ,  
	0x034e, 0x02 ,  
	0x034f, 0xd0 ,  
	0x3707, 0x11 ,   
	0x3833, 0x04 ,   
	0x3835, 0x04 ,   
	0x3819, 0x6e ,   
	0x3817, 0x94 ,   
	0x3831, 0x40 ,   
	0x381a, 0x00 ,   
	0x4608, 0x00 ,   
	0x4609, 0x04 ,    
	0x5001, 0xff ,  
	0x5003, 0xff ,  
	0x501e, 0x05 ,  
	0x501f, 0x00 ,   
	0x5020, 0x02 ,  
	0x5021, 0xd0 ,   
	0x530d, 0x06 ,  
	0x0100, 0x01 ,
};

uint32_t ov9740_1600_1200_regs[]={

};

// init

uint32_t ov9740_default_regs_init[]={
// Software RESET
	0x0103, 0x01 ,
//Mirror on, flip off
	0x0101, 0x01 ,
// PLL setting
	0x3104, 0x20 , // Reset PLL2
	0x0305, 0x03 , // PLL1 pre_divider
	0x0307, 0x5f , // PLL1 multiplier
	0x0303, 0x01 , // PLL1 video pclk divider
	0x0301, 0x0a , // PLL1 video sysclk divider
	0x3010, 0x01 , // PLL1 scale divider
//Timing setting
	0x0340, 0x03 , // VTS
	0x0341, 0x07 , // VTS
	0x0342, 0x06 , // HTS
	0x0343, 0x62 , // HTS
	0x0344, 0x00 , // X start
	0x0345, 0x08 , // X start
	0x0346, 0x00 , // Y start
	0x0347, 0x04 , // Y start
	0x0348, 0x05 , // X end
	0x0349, 0x0c , // X end
	0x034a, 0x02 , // Y end
	0x034b, 0xd8 , // Y end
	0x034c, 0x05 , // H output size
	0x034d, 0x00 , // H output size
	0x034e, 0x02 , // V output size
	0x034f, 0xd0 , // V output size
// Output select 
	0x3002, 0xe8 , // Vsync, Href, PCLK outout
	0x3004, 0x03 , // D[9:8] output
	0x3005, 0xff , // D[7:0] output
	0x3013, 0x60 , // MIPI control?
	0x3026, 0x00 , // D[9:8] data path
	0x3027, 0x00 , // D[7:0] data path
// Analog control
	0x3601, 0x40 , // Analog control
	0x3602, 0x16 , // Analog control
	0x3603, 0xaa , // Analog control
	0x3604, 0x0c , // Analog control
	0x3610, 0xa1 , // Analog control
	0x3612, 0x24 , // Analog control
	0x3620, 0x66 , // Analog control
	0x3621, 0xc0 , // Analog control
	0x3622, 0x9f , // Analog control
	0x3630, 0xd2 , // Analog control
	0x3631, 0x5e , // Analog control
	0x3632, 0x27 , // Analog control
	0x3633, 0x50 , // Analog control
// Sensor control
	0x3703, 0x42 , // Sensor control 
	0x3704, 0x10 , // Sensor control 
	0x3705, 0x45 , // Sensor control 
	0x3707, 0x11 , // Sensor control 
// Timing control
	0x3817, 0x94 , // Internal timing control
	0x3819, 0x6e , // Internal timing control
	0x3831, 0x40 , // Digital gain enable
	0x3833, 0x04 , // Internal timing control
	0x3835, 0x04 , // Internal timing control
// AEC/AGC control
	0x3503, 0x10 , // AEC/AGC off
	0x3a18, 0x01 , // Gain ceiling
	0x3a19, 0xb5 , // Gain ceiling = 27.3x
	0x3a1a, 0x05 , // Max diff
	0x3a11, 0x90 , // control zone High
	0x3a1b, 0x4a , // stable high out
	0x3a0f, 0x48 , // stable high in  
	0x3a10, 0x44 , // stable low in
	0x3a1e, 0x42 , // stable low out
	0x3a1f, 0x22 ,   // control zone low 
// Banding filter
	0x3a08, 0x00 , // 50Hz banding step
	0x3a09, 0xe8 , // 50Hz banding step
	0x3a0e, 0x03 , // 50Hz banding Max
	0x3a14, 0x09 , // 50Hz Max exposure
	0x3a15, 0x15 , // 50Hz Max exposure
	0x3a0a, 0x00 , // 60Hz banding step
	0x3a0b, 0xc0 , // 60Hz banding step
	0x3a0d, 0x04 , // 60Hz banding Max
	0x3a02, 0x09 , // 60Hz Max exposure
	0x3a03, 0x15 , // 60Hz Max exposure
//50/60 detection
	0x3c0a, 0x9c , // Number of samples
	0x3c0b, 0x3f , // Number of samples
// BLC control
	0x4002, 0x45 , // BLC auto enable
	0x4005, 0x18 , // BLC mode
// DVP control
	0x4702, 0x04 , // Vsync length high
	0x4704, 0x00 , // Vsync mode 
	0x4706, 0x08 , // SOF/EOF negative edge to Vsync positive edge
// ISP control
	0x5000, 0xff , // [7]LC [6]Gamma [3]DNS [2]BPC [1]WPC [0]CIP
	0x5001, 0xff , // [7]SDE [6]UV adjust [4]scale [3]contrast [2]UV average [1]CMX [0]AWB
	0x5003, 0xff , // [7]PAD [5]Buffer [3]Vario [1]BLC [0]AWB gain
//AWB
	0x5180, 0xf0 , //AWB setting
	0x5181, 0x00 ,   //AWB setting
	0x5182, 0x41 , //AWB setting 
	0x5183, 0x42 , //AWB setting
	0x5184, 0x80 , //AWB setting
	0x5185, 0x68 , //AWB setting
	0x5186, 0x93 , //AWB setting 
	0x5187, 0xa8 , //AWB setting
	0x5188, 0x17 , //AWB setting
	0x5189, 0x45 , //AWB setting
	0x518a, 0x27 , //AWB setting
	0x518b, 0x41 , //AWB setting
	0x518c, 0x2d , //AWB setting
	0x518d, 0xf0 , //AWB setting
	0x518e, 0x10 ,   //AWB setting
	0x518f, 0xff ,  //AWB setting
	0x5190, 0x00 ,   //AWB setting
	0x5191, 0xff ,  //AWB setting 
	0x5192, 0x00 ,   //AWB setting
	0x5193, 0xff ,  //AWB setting 
	0x5194, 0x00 , //AWB setting 
// DNS
	0x529a, 0x02 , //noise Y list 0
	0x529b, 0x08 , //noise Y list 1
	0x529c, 0x0a , //noise Y list 2
	0x529d, 0x10 , //noise Y list 3
	0x529e, 0x10 , //noise Y list 4
	0x529f, 0x28 ,   //noise Y list 5
	0x52a0, 0x32 , //noise Y list 6
	0x52a2, 0x00 , //noise UV list 0[8] 
	0x52a3, 0x02 , //noise UV list 0[7:0]
	0x52a4, 0x00 , //noise UV list 1[8] 
	0x52a5, 0x04 , //noise UV list 1[7:0] 
	0x52a6, 0x00 , //noise UV list 2[8]
	0x52a7, 0x08 , //noise UV list 2[7:0]
	0x52a8, 0x00 , //noise UV list 3[8] 
	0x52a9, 0x10 ,   //noise UV list 3[7:0]
	0x52aa, 0x00 , //noise UV list 4[8]
	0x52ab, 0x38 ,   //noise UV list 4[7:0]
	0x52ac, 0x00 , //noise UV list 5[8]
	0x52ad, 0x3c , //noise UV list 5[7:0]
	0x52ae, 0x00 , //noise UV list 6[8]
	0x52af, 0x4c ,  //noise UV list 6[7:0]
//CIP
	0x530d, 0x06 , //max sharpen
//CMX
	0x5380, 0x01 , //CMX00[9:8]  
	0x5381, 0x00 , //CMX00[7:0]  
	0x5382, 0x00 , //CMX01[9:8]
	0x5383, 0x0d , //CMX01[7:0]  
	0x5384, 0x00 , //CMX02[9:8]
	0x5385, 0x2f , //CMX02[7:0]   
	0x5386, 0x00 , //CMX10[9:8]
	0x5387, 0x00 , //CMX10[7:0]  
	0x5388, 0x00 , //CMX11[9:8]
	0x5389, 0xd3 , //CMX11[7:0] 
	0x538a, 0x00 , //CMX12[9:8]
	0x538b, 0x0f , //CMX12[7:0]   
	0x538c, 0x00 , //CMX20[9:8] 
	0x538d, 0x00 , //CMX20[7:0] 
	0x538e, 0x00 , //CMX21[9:8]
	0x538f, 0x32 , //CMX21[7:0] 
	0x5390, 0x00 , //CMX22[9:8]
	0x5391, 0x94 , //CMX22[7:0] 
	0x5392, 0x00 , //bit[0] CMX22 sign  
	0x5393, 0xa4 , //CMX 21,20, 12, 11, 10, 02, 01, 00 sign
	0x5394, 0x18 , //CMX shift
// Contrast
	0x5401, 0x2c , // bit[7:4] th1, birt[3:0]th2
	0x5403, 0x28 , // maximum high level
	0x5404, 0x06 , // minimum high level
	0x5405, 0xe0 , // minimum high level
//Y Gamma
	0x5480, 0x04 , //Y List 0
	0x5481, 0x12 , //Y List 1
	0x5482, 0x27 , //Y List 2
	0x5483, 0x49 , //Y List 3
	0x5484, 0x57 , //Y List 4
	0x5485, 0x66 , //Y List 5
	0x5486, 0x75 , //Y List 6
	0x5487, 0x81 , //Y List 7
	0x5488, 0x8c , //Y List 8
	0x5489, 0x95 , //Y List 9
	0x548a, 0xa5 , //Y List a
	0x548b, 0xb2 , //Y List b
	0x548c, 0xc8 , //Y List c
	0x548d, 0xd9 , //Y List d
	0x548e, 0xec , //Y List e
//UV Gamma
	0x5490, 0x01 ,   //UV List 0[11:8] 
	0x5491, 0xc0 ,   //UV List 0[7:0] 
	0x5492, 0x03 ,   //UV List 1[11:8] 
	0x5493, 0x00 ,   //UV List 1[7:0] 
	0x5494, 0x03 ,   //UV List 2[11:8] 
	0x5495, 0xe0 ,   //UV List 2[7:0] 
	0x5496, 0x03 ,   //UV List 3[11:8] 
	0x5497, 0x10 ,   //UV List 3[7:0] 
	0x5498, 0x02 ,   //UV List 4[11:8] 
	0x5499, 0xac ,   //UV List 4[7:0] 
	0x549a, 0x02 ,   //UV List 5[11:8] 
	0x549b, 0x75 ,   //UV List 5[7:0] 
	0x549c, 0x02 ,    //UV List 6[11:8] 
	0x549d, 0x44 ,   //UV List 6[7:0] 
	0x549e, 0x02 ,   //UV List 7[11:8] 
	0x549f, 0x20 ,   //UV List 7[7:0] 
	0x54a0, 0x02 ,   //UV List 8[11:8] 
	0x54a1, 0x07 ,   //UV List 8[7:0] 
	0x54a2, 0x01 ,   //UV List 9[11:8] 
	0x54a3, 0xec ,   //UV List 9[7:0] 
	0x54a4, 0x01 ,   //UV List 10[11:8] 
	0x54a5, 0xc0 ,   //UV List 10[7:0] 
	0x54a6, 0x01 ,   //UV List 11[11:8] 
	0x54a7, 0x9b ,   //UV List 11[7:0] 
	0x54a8, 0x01 ,   //UV List 12[11:8] 
	0x54a9, 0x63 ,   //UV List 12[7:0] 
	0x54aa, 0x01 ,   //UV List 13[11:8] 
	0x54ab, 0x2b ,   //UV List 13[7:0] 
	0x54ac, 0x01 ,   //UV List 14[11:8] 
	0x54ad, 0x22 ,   //UV List 14[7:0] 
 
// UV adjusti
	0x4708, 0x01 ,
	0x4300, 0x30 ,
	0x5501, 0x1c ,   //UV adjust manual
	0x5502, 0x00 ,    //UV adjust th1[8]
	0x5503, 0x40 ,   //UV adjust th1[7:0] 
	0x5504, 0x00 ,   //UV adjust th2[8]
	0x5505, 0x80 ,   //UV adjust th2[7:0] 
// Lens correction
	0x5800, 0x1c , // Lenc g00
	0x5801, 0x16 , // Lenc g01
	0x5802, 0x15 , // Lenc g02
	0x5803, 0x16 , // Lenc g03
	0x5804, 0x18 , // Lenc g04
	0x5805, 0x1a , // Lenc g05
	0x5806, 0x0c , // Lenc g10
	0x5807, 0x0a , // Lenc g11
	0x5808, 0x08 , // Lenc g12
	0x5809, 0x08 , // Lenc g13
	0x580a, 0x0a , // Lenc g14
	0x580b, 0x0b , // Lenc g15
	0x580c, 0x05 , // Lenc g20
	0x580d, 0x02 , // Lenc g21
	0x580e, 0x00 , // Lenc g22
	0x580f, 0x00 , // Lenc g23
	0x5810, 0x02 , // Lenc g24
	0x5811, 0x05 , // Lenc g25
	0x5812, 0x04 , // Lenc g30
	0x5813, 0x01 , // Lenc g31
	0x5814, 0x00 , // Lenc g32
	0x5815, 0x00 , // Lenc g33
	0x5816, 0x02 , // Lenc g34
	0x5817, 0x03 , // Lenc g35
	0x5818, 0x0a , // Lenc g40
	0x5819, 0x07 , // Lenc g41
	0x581a, 0x05 , // Lenc g42
	0x581b, 0x05 , // Lenc g43
	0x581c, 0x08 , // Lenc g44
	0x581d, 0x0b , // Lenc g45
	0x581e, 0x15 , // Lenc g50
	0x581f, 0x14 , // Lenc g51
	0x5820, 0x14 , // Lenc g52
	0x5821, 0x13 , // Lenc g53
	0x5822, 0x17 , // Lenc g54
	0x5823, 0x16 , // Lenc g55
	0x5824, 0x46 , // Lenc b00, r00
	0x5825, 0x4c , // Lenc b01, r01
	0x5826, 0x6c , // Lenc b02, r02
	0x5827, 0x4c , // Lenc b03, r03
	0x5828, 0x80 , // Lenc b04, r04
	0x5829, 0x2e , // Lenc b10, r10
	0x582a, 0x48 , // Lenc b11, r11
	0x582b, 0x46 , // Lenc b12, r12
	0x582c, 0x2a , // Lenc b13, r13
	0x582d, 0x68 , // Lenc b14, r14
	0x582e, 0x08 , // Lenc b20, r20
	0x582f, 0x26 , // Lenc b21, r21
	0x5830, 0x44 , // Lenc b22, r22
	0x5831, 0x46 , // Lenc b23, r23
	0x5832, 0x62 , // Lenc b24, r24
	0x5833, 0x0c , // Lenc b30, r30
	0x5834, 0x28 , // Lenc b31, r31
	0x5835, 0x46 , // Lenc b32, r32
	0x5836, 0x28 , // Lenc b33, r33
	0x5837, 0x88 , // Lenc b34, r34
	0x5838, 0x0e , // Lenc b40, r40
	0x5839, 0x0e , // Lenc b41, r41
	0x583a, 0x2c , // Lenc b42, r42
	0x583b, 0x2e , // Lenc b43, r43
	0x583c, 0x46 , // Lenc b44, r44
	0x583d, 0xca , // Lenc b offset, r offset
	0x583e, 0xf0 , // max gain
	0x5842, 0x02 , // br hscale[10:8]
	0x5843, 0x5e , // br hscale[7:0]
	0x5844, 0x04 , // br vscale[10:8]
	0x5845, 0x32 , // br vscale[7:0]
	0x5846, 0x03 , // g hscael[11:8]
	0x5847, 0x29 , // g hscale[7:0]
	0x5848, 0x02 , // g vscale[11:8]
	0x5849, 0xcc , // g vscale[7:0]
	0x5580, 0x06 , // sharpness on, saturation on
	0x3c00, 0x04 , // band 50hz
	0x3c01, 0x80 , // manual band on
// Start streaming
	0x0100, 0x01 , // start streaming
};
uint32_t ov9740_default_regs_exit[]={};

#endif
