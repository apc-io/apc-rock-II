/*++
	linux/drivers/media/video/wmt_v4l2/sensors/siv120d/siv120d.h

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

#ifndef SIV120D_H
#define SIV120D_H


// Scene Mode
uint8_t siv120d_scene_mode_auto[] = {
	0x00,0x01,
	0x11,0x10,
	0x40,0x40,
};

uint8_t siv120d_scene_mode_night[] = {
	0x00,0x01,
	0x11,0x14,
	0x40,0x50,
};


// White Balance
uint8_t siv120d_wb_auto [] = {
	0x00,0x02,
	//0x60,0x96,
	//0x61,0x90,
	//0x62,0x60,
	0x10,0xd3,
};

uint8_t siv120d_wb_incandescent [] = {
	0x00,0x02,
	0x10,0x00,
	0x60,0x98,
	0x61,0xC8,
};

uint8_t siv120d_wb_fluorescent [] = {
	0x00,0x02,
	0x10,0x00,
	0x60,0xAA,
	0x61,0xBE,
};

uint8_t siv120d_wb_daylight [] = {
	0x00,0x02,
	0x10,0x00,
	0x60,0xb0,
	0x61,0x74,
};

uint8_t siv120d_wb_cloudy [] = {
	0x00,0x02,
	0x10,0x00,
	0x60,0xb4,
	0x61,0x64,
};

uint8_t siv120d_wb_tungsten [] = {
	0x00,0x02,
	0x10,0x00,
	0x60,0x90,
	0x61,0xC0,
};


// Exposure
uint8_t siv120d_exposure_neg6[] = {
	0x00,0x03,
	0xAB,0xa8,
};

uint8_t siv120d_exposure_neg3[] = {
	0x00,0x03,
	0xAB,0x00,
};

uint8_t siv120d_exposure_zero[] = {
	0x00,0x03,
	0xAB,0x18,
};

uint8_t siv120d_exposure_pos3[] = {
	0x00,0x03,
	0xAB,0x28,
};

uint8_t siv120d_exposure_pos6[] = {
	0x00,0x03,
	0xAB,0x38,
};

// Color Effect
uint8_t siv120d_colorfx_none[] = {
	0x00,0x03,
	0xB6,0x00,
};

uint8_t siv120d_colorfx_bw[] = {
	0x00,0x03,
	0xB6,0x40,
};

uint8_t siv120d_colorfx_sepia[] = {
	0x00,0x03,
	0xB6,0x80,
	0xB7,0x60,
	0xB8,0xA0,
};

uint8_t siv120d_colorfx_negative[] = {
	0x00,0x03,
	0xB6,0x20,

};

uint8_t siv120d_colorfx_emboss[] = {
	0x00,0x03,
	0xB6,0x08,
	0xB7,0x00,
	0xB8,0x00,
};

uint8_t siv120d_colorfx_sketch[] = {
	0x00,0x03,
	0xB6,0x04,
	0xB7,0x00,
	0xB8,0x00,
};

uint8_t siv120d_colorfx_sky_blue[] = {
	0x00,0x03,
	0xB6,0xa0,
	0xB7,0xc0,
	0xB8,0x50,
};

uint8_t siv120d_colorfx_grass_green[] = {
	0x00,0x03,
	0xB6,0x80,
	0xB7,0x50,
	0xB8,0x50,
};

uint8_t siv120d_colorfx_skin_whiten[] = {
};

// Resolution
uint8_t siv120d_320_240_regs[]={


0x00,0x00,
0x06,0x06,
0x12,0x3d,
0x00,0x03,
0xc0,0x10,
0xc1,0x00,
0xc2,0x40,
0xc3,0x00,
0xc4,0xf0,                

 };

uint8_t siv120d_640_480_regs[]={
0x00,0x00, 
0x06,0x04, 
0x12,0x3d, 
0x00,0x03, 
0xc0,0x24, 
0xc1,0x00, 
0xc2,0x80, 
0xc3,0x00, 
0xc4,0xe0, 

};

uint8_t siv120d_default_regs_init_120d[]={

	    0x00, 0x00,                                                                 
			0x04, 0x00,                                                              
			0x05, 0x03, //VGA Output                                                 
  		0x07, 0x70,                                                              
			0x10, 0x34,                                                              
			0x11, 0x27,                                                              
  		0x12, 0x21,//33}},                                                       
			0x16, 0xc6,                                                              
			0x17, 0xaa,                                                              
		                                                                                 
			// SIV120D 50Hz - 24MHz                                                        
  		0x20,0x10,                                                      
			0x21,0xc3,                                                      
			0x23,0x4d,                                                      
			0x00,0x01,                                                               
			0x34,0x60,                                                               
		                                                                                 
			// SIV120D 50Hz - Still Mode                                                   
			0x00,0x00,                                                               
			0x24,0x10,                                                               
			0x25,0xC3,                                                               
			0x27,0x4D,                                                               
		                                                                                 
			// Vendor recommended value ##Don't change##                                   
			0x00,0x00,                                                               
			0x40,0x00,                                                               
			0x41,0x00,                                                               
			0x42,0x00,                                                               
  		0x43,0x00,                                                               
		                                                                                 
			// AE                                                                          
			0x00, 0x01,			                                                         
			0x11, 0x14, // 6fps at lowlux 		                                       
			0x12, 0x78,//79}},//7d}},//80}},//78}},// D65 target 0x74                
			0x13, 0x78,//79}},//7d}},//80}},//78}},// CWF target 0x74                
			0x14, 0x78,//79}},//7d}},//80}},//78}},// A target ,	0x74               
			0x1E, 0x08,// ini gain, 0x08                                             
			0x34, 0x1f,//0x1A   //STST		                                     
			0x40, 0x36,//48}},//0x50 //0x38 // Max x6		                             
			/*                                                                         
			0x41, 0x20,	//AG_TOP1	0x28                                               
	    0x42, 0x20,	//AG_TOP0	0x28                                               
	    0x43, 0x00,	//AG_MIN1	0x08                                               
	    0x44, 0x00,	//AG_MIN0	0x08                                               
	    0x45, 0x00,	//G50_dec	0x09                                               
	    0x46, 0x0a,	//G33_dec	0x17                                               
	    0x47, 0x10,	//G25_dec	0x1d                                               
	    0x48, 0x13,	//G20_dec	0x21                                               
	    0x49, 0x15,	//G12_dec	0x23                                               
	    0x4a, 0x18,	//G09_dec	0x24                                               
	    0x4b, 0x1a,	//G06_dec	0x26                                               
	    0x4c, 0x1d,	//G03_dec	0x27                                               
      0x4d, 0x20,	//G100_inc	0x27                                             
	    0x4e, 0x10,	//G50_inc	0x1a                                               
	    0x4f, 0x0a,	//G33_inc	0x14                                               
	    0x50, 0x08,	//G25_inc	0x11                                               
	    0x51, 0x06,	//G20_inc	0x0f                                               
	    0x52, 0x05,	//G12_inc	0x0d                                               
	    0x53, 0x04,	//G09_inc	0x0c                                               
	    0x54, 0x02,//	//G06_inc	0x0a                                             
	    0x55, 0x01,//	//G03_inc	0x09                                             
  	   */                                                                              
			//AWB					                                                                 
			0x00, 0x02,			                                                         
			0x10, 0x00,                                                              
			0x11, 0xc0,                                                              
			0x12, 0x80,                                                              
			0x13, 0x80,                                                              
			0x14, 0x80,                                                              
			0x15, 0xfe, // R gain Top                                                
			0x16, 0x80, // R gain bottom                                             
			0x17, 0xcb, // B gain Top                                                
			0x18, 0x70, // B gain bottom 0x80                                 
			0x19, 0x94, // Cr top value 0x90                                         
			0x1a, 0x6c, // Cr bottom value 0x70                                      
			0x1b, 0x94, // Cb top value 0x90                                         
			0x1c, 0x6c, // Cb bottom value 0x70                                      
			0x1d, 0x94, // 0xa0                                                      
			0x1e, 0x6c, // 0x60                                                      
			0x20, 0xe8, // AWB luminous top value                                    
			0x21, 0x30, // AWB luminous bottom value 0x20                            
			0x22, 0xa4,                                                              
			0x23, 0x20,                                                              
			0x25, 0x20,                                                              
			0x26, 0x0f,                                                              
			0x27, 0x10, // 60 BRTSRT                                          
			0x28, 0xcb,//cb}}, // b4 BRTRGNTOP result 0xb2                           
			0x29, 0xc8, // b0 BRTRGNBOT                                              
  		0x2a, 0xa0, // 92 BRTBGNTOP result 0x90                                  
			0x2b, 0x98,  // 8e BRTBGNBOT                                             
			0x2c, 0x88, // RGAINCONT                                                 
			0x2d, 0x88, // BGAINCONT                                                 
		                                                                                 
			0x30, 0x00,                                                              
			0x31, 0x10,                                                              
			0x32, 0x00,                                                              
			0x33, 0x10,                                                              
			0x34, 0x02,                                                              
			0x35, 0x76,                                                              
			0x36, 0x01,                                                              
			0x37, 0xd6,                                                              
  		0x40, 0x01,                                                              
			0x41, 0x04,                                                              
			0x42, 0x08,                                                              
			0x43, 0x10,                                                              
			0x44, 0x12,                                                              
			0x45, 0x35,                                                              
			0x46, 0x64,                                                              
  		0x50, 0x33,                                                              
  		0x51, 0x20,                                                              
			0x52, 0xe5,                                                              
			0x53, 0xfb,                                                              
			0x54, 0x13,                                                              
			0x55, 0x26,                                                              
			0x56, 0x07,                                                              
			0x57, 0xf5,                                                              
			0x58, 0xea,                                                              
  		0x59, 0x21,                                                              
		                                                                           
			0x62, 0x88, // G gain                                                    
		                                                                                 
			0x63, 0xb3, // R D30 to D20                                              
			0x64, 0xb8,                                 
			0x65, 0xb3, // R D20 to D30                                              
  		0x66, 0xb8,                               
		                                                                           
			0x67, 0xdd, // R D65 to D30                                              
			0x68, 0xa0, // B D65 to D30                                              
			0x69, 0xdd, // R D30 to D65                                              
			0x6a, 0xa0, // B D30 to D65                                              
		                                                                                 
			//IDP                                                                          
		  0x00,0x03,                                                               
		  0x10,0xFF, // IDP function enable                                        
		  0x11,0x1D, // PCLK polarity                                       
		  0x12,0x7d, // Y,Cb,Cr order sequence                                     
		  0x14,0x04, // don't change                                               
		  0x8c,0x10,	//CMA select                                                 
		                                                                                 
  	                                                                                 
			// DPCNR				                                                               
			0x17, 0x28,	// DPCNRCTRL                                                 
			0x18, 0x04,	// DPTHR                                                     
			0x19, 0x50,	// C DP Number {{ Normal [7:6] Dark [5:4] }} | [3:0] DPTHRMIN
			0x1A, 0x50,	// G DP Number {{ Normal [7:6] Dark [5:4] }} | [3:0] DPTHRMAX
			0x1B, 0x12,	// DPTHRSLP{{ [7:4] @ Normal | [3:0] @ Dark }}               
			0x1C, 0x00,	// NRTHR                                                     
			0x1D, 0x0f,//08}},	// [5:0] NRTHRMIN 0x48                               
			0x1E, 0x0f,//08}},	// [5:0] NRTHRMAX 0x48                               
			0x1F, 0x3f,//28}},	// NRTHRSLP{{ [7:4] @ Normal | [3:0] @ Dark }}, 0x2f 
			0x20, 0x04,	// IllumiInfo STRTNOR                                        
			0x21, 0x0f,	// IllumiInfo STRTDRK                                        
		                                                                                 
			// Gamma	                                                                                                     
			//0x30, 0x00 ,	//0x0                                                        
			0x31, 0x03 ,//0x080x31, 0x04 },//0x08   0x03                              
  		       0x32, 0x0b ,//0x100x32, 0x0b },//0x10   0x08                              
			0x33, 0x1e,//0x1B0x33, 0x24 },//0x1B   0x16                              
			0x34, 0x46,//0x370x34, 0x49 },//0x37   0x36                              
			0x35, 0x62,//0x4D0x35, 0x66 },//0x4D   0x56                              
			0x36, 0x78,//0x600x36, 0x7C },//0x60   0x77                              
			0x37, 0x8b,//0x720x37, 0x8D },//0x72   0x88                              
			0x38, 0x9B,//0x820x38, 0x9B },//0x82   0x9a                              
			0x39, 0xA8,//0x910x39, 0xAA },//0x91   0xA9                              
			0x3a, 0xb6,//0xA00x3a, 0xb6 },//0xA0   0xb5                              
			0x3b, 0xcc,//0xBA0x3b, 0xca },//0xBA   0xca                              
			0x3c, 0xdf,//0xD30x3c, 0xdc },//0xD3   0xdd                              
  		0x3d, 0xf0,//0xEA0x3d, 0xef },//0xEA   0xee                              
  		                                                        
  		// Sha0x3f, 0xFFding Register Setting }}, 								                               
			0x40, 0x0a,													                                     
			0x41, 0xdc,													                                     
			0x42, 0x77,													                                     
			0x43, 0x66,													                                     
			0x44, 0x55,													                                     
			0x45, 0x44,													                                     
			0x46, 0x11,	// left R gain[7:4], right R gain[3:0]			                 
			0x47, 0x11,	// top R gain[7:4], bottom R gain[3:0]			                 
			0x48, 0x10,	// left Gr gain[7:4], right Gr gain[3:0] 0x21			           
			0x49, 0x00,	// top Gr gain[7:4], bottom Gr gain[3:0]		                 
			0x4a, 0x00,	// left Gb gain[7:4], right Gb gain[3:0] 0x02		             
			0x4b, 0x00,	// top Gb gain[7:4], bottom Gb gain[3:0]		                 
			0x4c, 0x00,	// left B gain[7:4], right B gain[3:0]			                 
			0x4d, 0x11,	// top B gain[7:4], bottom B gain[3:0]			                 
  		0x4e, 0x04,	// X-axis center high[3:2], Y-axis center high[1:0]          
			0x4f, 0x98,	// X-axis center low[7:0] 0x50					                     
			0x50, 0xd8,	// Y-axis center low[7:0] 0xf6					                     
			0x51, 0x80,	// Shading Center Gain							                         
  		0x52, 0x00,	// Shading R Offset 							                           
			0x53, 0x00,	// Shading Gr Offset							                           
			0x54, 0x00,	// Shading Gb Offset							                           
			0x55, 0x00,	// Shading B Offset                                          
		                                                                                
  		// Interpolation                                                               
  	 0x60, 0x57,  //INT outdoor condition                                     
	   0x61, 0x57,  //INT normal condition                                      
  	                                                                                 
	    0x62, 0x77,  //ASLPCTRL 7:4 GE, 3:0 YE                                 
  	  0x63, 0x38,  //YDTECTRL {{YE}} [7] fixed,                            
  	  0x64, 0x38,  //GPEVCTRL {{GE}} [7] fixed,                                
  	                                                                           
  	  0x66, 0x0c,  //SATHRMIN                                                  
  	  0x67, 0xff,                                                              
  	  0x68, 0x04,  //SATHRSRT                                                  
  	  0x69, 0x08,  //SATHRSLP                                                  
  	                                                                           
  	  0x6a, 0xaf,  //PTDFATHR [7] fixed, [5:0] value                           
  	  0x6b, 0x58,  //PTDLOTHR [6] fixed, [5:0] value                           
  	  0x6d, 0x88,  //PTDLOTHR [6] fixed, [5:0] value                           
  	                                                                             
  	  0x8f, 0x00,  //Cb/Cr coring                                              
  	                                                                                 
  		// Color matrix {{D65}} - Daylight	                                           
                                               

       0x71,	0x42,	                                                    
       0x72,	0xbf,	                                                    
       0x73,	0x00,	                                                    
       0x74,	0x0f,	                                                    
       0x75,	0x31,	                                                    
       0x76,	0x00,	                                                    
       0x77,	0x00,	                                                    
       0x78,	0xbc,	                                                    
       0x79,	0x44,	                                                    
                                                                                   
  // Color matrix (D30) - CWF                                                        
       0x7a,	0x3a,	                                            
       0x7b,	0xcd,                                              
       0x7c,	0xfa,                                              
       0x7d,	0x12,                                              
       0x7e,	0x2c,                                              
       0x7f,	0x02,                                              
       0x80,	0xf7,                                              
       0x81,	0xc7,                                              
       0x82,	0x42,                                              
                                                                                   
  // Color matrix (D20) - A                                                          
       0x83,	0x3a,	                                                 
       0x84,	0xcd,                                                   
       0x85,	0xfa,                                                   
       0x86,	0x12,                                                   
       0x87,	0x2c,                                                   
       0x88,	0x02,                                                   
       0x89,	0xf7,                                                   
       0x8a,	0xc7,                                                   
       0x8b,	0x42,                                                   
                                                                            
       0x8c, 0x10,                                                          
  		0x8d, 0x04,                 
  		0x8e, 0x02,                 
  		0x8f, 0x00,                 
  	                              
  		0x90, 0x10,                 
  		0x91, 0x10,                 
  		0x92, 0x02,                 
  	                        
  		0x96, 0x02,       
                                                                            
  	  0x9f, 0x0c,                            
  	  0xa0, 0x0c,                            
  	  0xa1, 0x22,                            
  	                                         
  	  0xa9, 0x12,                            
  	  0xaa, 0x12,                            
  	  0xab, 0x90,                          
  	  0xae, 0x40,                            
  	  0xaf, 0x86,                            
  	  0xb9, 0x10,                            
  	  0xba, 0x20,                            
                                                  
  		// inverse color space conversion                                              
  		0xcc, 0x40,                                                              
  		0xcd, 0x00,                                                              
  		0xce, 0x58,                                                              
  		0xcf, 0x40,                                                              
  		0xd0, 0xea,                                                              
  		0xd1, 0xd3,                                                              
  		0xd2, 0x40,                                                              
  		0xd3, 0x6f,                                                              
  		0xd4, 0x00,                                                              
  	                                                                                 
  		// ee/nr				                                                               
  		0xd9, 0x0b,                                                              
  		0xda, 0x1f,                                                              
  		0xdb, 0x05,                                                              
  		0xdc, 0x08,                                                              
  		0xdd, 0x3C,		                                                           
  		0xde, 0xF9,	//NOIZCTRL	                                                 
  			                                                                             
  	  //window                                                                       
  	    0xc0, 0x24,                                                            
  		0xC1, 0x00,                                                              
  		0xC2, 0x80,                                                              
  		0xC3, 0x00,                                                              
  		0xC4, 0xE0,                                                              
  		// memory speed 		                                                           
  		0xe5, 0x15,                                                              
  		0xe6, 0x20,                                                              
  		0xe7, 0x04,                                                              
  	                                                                           
  		0x00, 0x02,                                                              
  		0x10, 0xd3,                                                              
  	                                                                                 
  		//Sensor On 			                                                             
  		0x00, 0x00,                                                              
  		0x03, 0x05,      

};

#endif                                                        