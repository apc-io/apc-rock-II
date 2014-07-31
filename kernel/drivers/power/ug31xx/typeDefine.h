/// ===========================================
/// typeDefine.h
/// ===========================================

#ifndef _TYPE_DEFINE_H_
#define _TYPE_DEFINE_H_

typedef unsigned char       _upi_u8_;
typedef unsigned short      _upi_u16_;
typedef unsigned int        _upi_u32_;
typedef unsigned long long  _upi_u64_;
typedef char                _upi_s8_;
typedef short               _upi_s16_;
typedef int                 _upi_s32_;
typedef long long           _upi_s64_;
typedef char                _upi_bool_;

#define _UPI_TRUE_      (1)
#define _UPI_FALSE_     (0)
#define _UPI_NULL_      (NULL)


#ifdef  uG31xx_OS_WINDOWS
#pragma pack(push)
#pragma pack(1)
#endif

#define CELL_PARAMETER_ALARM_EN_UV      (1<<0)
#define CELL_PARAMETER_ALARM_EN_UET     (1<<1)
#define CELL_PARAMETER_ALARM_EN_OET     (1<<2)

typedef struct CELL_PARAMETER
{
  _upi_u16_ totalSize;     //Total struct size
  _upi_u16_ fw_ver;            //CellParameter struct version

  char customer[16];   //Customer name defined by uPI   //####2012/08/29#####
  char project[16];    //Project name defined by uPI
  _upi_u16_ ggb_version;       //0x0102 => 2.1

  char customerSelfDef[16];  //Customer name record by customer 
  char projectSelfDef[16];   //Project name record by customer
  _upi_u16_ cell_type_code; 

  _upi_u8_ ICType;  /*[2:0]=000 -> uG3100 [2:0]=001 -> uG3101
                            [2:0]=010 -> uG3102 [2:0]=100 -> uG3103_2
                            [2:0]=101 -> uG3103_3 */

  _upi_u8_ gpio1; /*bit[4] cbc_en32
                         bit[3] cbc_en21
                         bit[2] pwm
                         bit[1] alarm
                         bit[0] gpio */	   
  _upi_u8_ gpio2; /*bit[4] cbc_en32
                        bit[3] cbc_en21
                        bit[2] pwm
                        bit[1] alarm
                        bit[0] gpio */
  _upi_u8_ gpio34;			//11/22/2011 -->reg92  
  
  _upi_u8_ cellNumber;
  _upi_u8_ assignCellOneTo;
  _upi_u8_ assignCellTwoTo;
  _upi_u8_ assignCellThreeTo;

  _upi_u16_ i2cAddress;    //I2C Address(Hex)
  _upi_u16_ clock;

  _upi_u8_ tenBitAddressMode;
  _upi_u8_ highSpeedMode;
  _upi_u8_ chopCtrl;		//11/22/2011 -->regC1
  _upi_u8_ rSense;

  _upi_s16_ adc1Offset;		//11/22/2011 -->reg58/59
  _upi_u16_ ILMD;

  _upi_u16_ edv1Voltage;
  _upi_u16_ standbyCurrent;

  _upi_u16_ TPCurrent;
  _upi_u16_ TPVoltage;

  _upi_u16_ TPTime;
  _upi_u16_ offsetR;

  _upi_u16_ deltaR;
  _upi_u16_ TpBypassCurrent;			//20121029

  _upi_s16_ uvAlarm;
  _upi_s16_ uvRelease;

  _upi_s16_ uetAlarm;
  _upi_s16_ uetRelease;

  _upi_s16_ oetAlarm;
  _upi_s16_ oetRelease;

  _upi_s16_ oscTuneJ;
  _upi_s16_ oscTuneK;

  _upi_u8_ maxDeltaQ;
  _upi_u8_ timeInterval;
  _upi_u8_ alarm_timer;		//11/22/2011  00:*5,01:*10,10:*15,11:*20
  _upi_u8_ pwm_timer; /*[1:0]=00:32k [1:0]=01:16k
                                [1:0]=10:8k  [1:0]=11: 4k */
  
  _upi_u8_ clkDivA;			//11/22/2011
  _upi_u8_ clkDivB;			//11/22/2011	
  _upi_u8_ alarmEnable; /*[7]:COC [6]:DOC [5]:IT [4]:ET
                                    [3]:VP  [2]:V3  [1]:V2 [0]:V1 */
  _upi_u8_ cbcEnable; /*[1]:CBC_EN32  [0]:CBC_EN21 */

  _upi_u16_ vBat2_8V_IdealAdcCode;	//ideal ADC Code
  _upi_u16_ vBat2_6V_IdealAdcCode;

  _upi_u16_ vBat3_12V_IdealAdcCode;
  _upi_u16_ vBat3_9V_IdealAdcCode;

  _upi_s16_ adc1_pgain;
  _upi_s16_ adc1_ngain;

  _upi_s16_ adc1_pos_offset;
  _upi_u16_ adc2_gain;

  _upi_s16_ adc2_offset;
  _upi_u16_ R;
  
  _upi_u16_ rtTable[ET_NUMS];
  // SOV_TABLE %
  _upi_u16_ SOV_TABLE[SOV_NUMS]; 

  _upi_s16_ adc_d1;				//2012/06/06/update for IT25
  _upi_s16_ adc_d2;				//2012/06/06/update for IT80
  
  _upi_s16_ adc_d3;       ///< [AT-PM] : Used for ADC calibration IT code ; 08/15/2012
  _upi_s16_ adc_d4;
  
  _upi_s16_ adc_d5;
  _upi_u16_ NacLmdAdjustCfg;

  _upi_u8_ otp1Scale;
#if defined(uG31xx_OS_ANDROID)
}__attribute__((packed)) CELL_PARAMETER;
#else   ///< else of defined(uG31xx_OS_ANDROID)
}CELL_PARAMETER;
#endif  ///< end of defined(uG31xx_OS_ANDROID)

typedef struct CELL_TABLE
{		
  _upi_s16_ INIT_OCV[TEMPERATURE_NUMS][OCV_TABLE_IDX_COUNT][OCV_NUMS];							//initial OCV Table,0.1C/0.2C OCV/charge table 
  _upi_s16_ CELL_VOLTAGE_TABLE[TEMPERATURE_NUMS][C_RATE_NUMS][OCV_NUMS];		//cell behavior Model,the voltage data
  _upi_s16_ CELL_NAC_TABLE[TEMPERATURE_NUMS][C_RATE_NUMS][OCV_NUMS];				//cell behavior Model,the deltaQ
#if defined(uG31xx_OS_ANDROID)
}  __attribute__((packed)) CELL_TABLE;
#else   ///< else of defined(uG31xx_OS_ANDROID)
}CELL_TABLE;
#endif  ///< end of defined(uG31xx_OS_ANDROID)

typedef struct CELL_DATA
{
  CELL_PARAMETER cellParameter;
  CELL_TABLE cellTable1;
#if defined(uG31xx_OS_ANDROID)
}  __attribute__((packed)) CELL_DATA;
#else   ///< else of defined(uG31xx_OS_ANDROID)
}CELL_DATA;
#endif  ///< end of defined(uG31xx_OS_ANDROID)

#if defined(uG31xx_OS_ANDROID)

  //<ASUS-WAD+>
  typedef struct _GGBX_FILE_HEADER
  {
    _upi_u32_ ggb_tag;        //'_GG_'
    _upi_u32_ sum16;          //16 bits checksum, but store as 4 bytes
    _upi_u64_ time_stamp;     //seconds pass since 1970 year, 00:00:00
    _upi_u64_ length;         //size that not only include ggb content. (multi-file support)
    _upi_u64_ num_ggb;        //number of ggb files. 
  }  __attribute__((packed)) GGBX_FILE_HEADER;
  //<ASUS-WAD->

#endif  ///< end of defined(uG31xx_OS_ANDROID)

typedef struct ADC_CHECK
{
	_upi_u16_ regCounter;	        //check adc counter
	_upi_u16_ regVbat1Ave;      //check average voltage
	_upi_u16_ lastCounter;
  _upi_u16_ lastVBat1Ave;
  _upi_u16_ failCounterCurrent;
  _upi_u16_ failCounterVoltage;
#if defined(uG31xx_OS_ANDROID)
}  __attribute__((packed)) ADC_CHECK;
#else   ///< else of defined(uG31xx_OS_ANDROID)
}ADC_CHECK;
#endif  ///< end of defined(uG31xx_OS_ANDROID)

typedef struct USER_REGISTER
{
	_upi_u8_ mode;
	_upi_u8_ ctrl1;
	_upi_u8_ charge_low;
	_upi_u8_ charge_high;
	_upi_u8_ counter_low;
	_upi_u8_ counter_high;
	_upi_u8_ current_low;
  _upi_u8_ current_high;
	_upi_u8_ vbat1_low;
	_upi_u8_ vbat1_high;
	_upi_u8_ intr_temper_low;
	_upi_u8_ intr_temper_high;
	_upi_u8_ ave_current_low;
	_upi_u8_ ave_current_high;
	_upi_u8_ extr_temper_low;
	_upi_u8_ extr_temper_high;
	_upi_u8_ rid_low;
	_upi_u8_ rid_high;
	_upi_u8_ alarm1_status;
	_upi_u8_ alarm2_status;
	_upi_u8_ intr_status;
	_upi_u8_ alram_en;
	_upi_u8_ ctrl2;
#if defined(uG31xx_OS_ANDROID)
}__attribute__((packed)) USER_REGISTER;
#else   ///< else of defined(uG31xx_OS_ANDROID)
}USER_REGISTER;
#endif  ///< end of defined(uG31xx_OS_ANDROID)

//2012/08/24/new add for system suspend 
typedef struct _GG_SUSPEND_INFO{	
	_upi_u16_     LMD;						//battery Qmax (maH)
	_upi_u16_     NAC;						//battery NAC(maH)
	_upi_u16_     RSOC;						//Battery Current RSOC(%)
	_upi_u32_	currentTime;			//the time tick	

#if defined(uG31xx_OS_ANDROID)
 }__attribute__((packed)) GG_SUSPEND_INFO,*PGG_SUSPEND_INFO;
#else   ///< else of defined(uG31xx_OS_ANDROID)
}GG_SUSPEND_INFO,*PGG_SUSPEND_INFO;
#endif  ///< end of defined(uG31xx_OS_ANDROID)

typedef struct _GG_BATTERY_INFO{	
	_upi_u16_     LMD;				//battery Qmax (maH)
	_upi_u16_     NAC;				//battery NAC(maH)
	_upi_u16_     RSOC;				//Battery Current RSOC(%)
#if defined(uG31xx_OS_ANDROID)
 }__attribute__((packed)) GG_BATTERY_INFO;
#else   ///< else of defined(uG31xx_OS_ANDROID)
}GG_BATTERY_INFO;
#endif  ///< end of defined(uG31xx_OS_ANDROID)

/// [AT-PM] : Used for TI bq27520 like command ; 10/11/2012
typedef struct GG_TI_BQ27520 {
  _upi_u16_ CntlControlStatus;
  _upi_u16_ CntlPrevMacWrite;

  _upi_s16_ AR;

  _upi_u16_ Temp;
  
  _upi_u16_ Flags;
  _upi_s16_ SINow;
  _upi_s32_ SIBuf;
  _upi_s16_ SISample;
  _upi_u16_ SIWindow;

  _upi_s16_ Mli;
  _upi_u8_ MliDsgSoc;

  _upi_u16_ AE;
  
  _upi_s16_ AP;
  _upi_u16_ APStartChgE;
  _upi_u16_ APStartDsgE;
  _upi_u32_ APChgTime;
  _upi_u32_ APDsgTime;

  _upi_u16_ CC;
  _upi_u16_ CCBuf;
  _upi_u16_ CCLastNac;

  _upi_u16_ Dli;

  _upi_u16_ Dlb;
  
  _upi_s8_ FCSet;
  _upi_s8_ FCClear;
  _upi_u8_ Soc1Set;
  _upi_u8_ Soc1Clear;
  _upi_s8_ InitSI;
  _upi_s16_ InitMaxLoadCurrent;
  _upi_s16_ CCThreshold;
  _upi_u32_ Opcfg;
  _upi_u16_ Dcap;

  _upi_u32_ LastTime;
  _upi_u16_ DeltaSec;
#if defined(uG31xx_OS_ANDROID)
}__attribute__((packed)) GG_TI_BQ27520, *PGG_TI_BQ27520;
#else   ///< else of defined(uG31xx_OS_ANDROID)
} GG_TI_BQ27520, *PGG_TI_BQ27520;
#endif  ///< end of defined(uG31xx_OS_ANDROID)

typedef struct GGAdcDeltaCodeMappingST {
  _upi_s32_ Adc1V100;
  _upi_s32_ Adc1V200;
  _upi_s32_ Adc2V100;
  _upi_s32_ Adc2V200;
#if defined(uG31xx_OS_ANDROID)
}__attribute__((packed)) GGAdcDeltaCodeMappingType;
#else   ///< else of defined(uG31xx_OS_ANDROID)
} GGAdcDeltaCodeMappingType;
#endif  ///< end of defined(uG31xx_OS_ANDROID)

typedef struct TableBackupST {
  _upi_u16_ lowerBound;
  _upi_u8_ resolution;
#if defined(uG31xx_OS_ANDROID)
} __attribute__((packed)) TableBackupType;
#else   ///< else of defined(uG31xx_OS_ANDROID)
} TableBackupType;
#endif  ///< end of defined(uG31xx_OS_ANDROID)

/* define the register of uG31xx */
typedef struct _GG_USER_REG{
	_upi_u8_ 	regMode;				      ///< [AT-PM] : 0x00 - MODE ; 04/08/2013
	_upi_u8_ 	regCtrl1;				      ///< [AT-PM] : 0x01 - CTRL1 ; 04/08/2013
	_upi_s16_	regCharge;				    ///< [AT-PM] : 0x02 - Charge ; 04/08/2013
	_upi_u16_	regCounter;				    ///< [AT-PM] : 0x04 - Counter ; 04/08/2013
	_upi_s16_	regCurrentAve;			  ///< [AT-PM] : 0x06 - Ave Current ; 04/08/2013
	_upi_s16_	regVbat1Ave;			    ///< [AT-PM] : 0x08 - Ave VBat1 ; 04/08/2013
	_upi_u16_	regITAve;				      ///< [AT-PM] : 0x0A - Ave IT ; 04/08/2013
	_upi_s16_ regOffsetCurrentAve;  ///< [AT-PM] : 0x0C - Ave Offset Current ; 04/08/2013
	_upi_u16_ regETAve;				      ///< [AT-PM] : 0x0E - Ave ET ; 04/08/2013
	_upi_u16_ regRidAve;				    ///< [AT-PM] : 0x10 - Ave RID ; 04/08/2013
	_upi_u8_  regAlarm1Status;      ///< [AT-PM] : 0x12 - Alarm1 Status ; 04/08/2013
	_upi_u8_  regAlarm2Status;      ///< [AT-PM] : 0x13 - Alarm2 Status ; 04/08/2013
	_upi_u8_  regIntrStatus;	      ///< [AT-PM] : 0x14 - INTR Status ; 04/08/2013
	_upi_u8_  regAlarmEnable;       ///< [AT-PM] : 0x15 - Alarm EN ; 04/08/2013
  _upi_u8_  regCtrl2;				      ///< [AT-PM] : 0x16 - CTRL2 ; 04/08/2013
#if defined(uG31xx_OS_ANDROID)
} __attribute__((packed)) GG_USER_REG, *PGG_USER_REG;
#else   ///< else of defined(uG31xx_OS_ANDROID)
} GG_USER_REG, *PGG_USER_REG;
#endif  ///< end of defined(uG31xx_OS_ANDROID)

/* define the register of uG31xx */
typedef struct _GG_USER2_REG{
		_upi_s16_ regVbat2;				//0x40,vBat2
		_upi_s16_ regVbat3;				//0x42,vBat3
		_upi_s16_ regVbat1;				//0x44,vBat1 Average
		_upi_s16_ regVbat2Ave;			//0x46,vBat2 Average
		_upi_s16_ regVbat3Ave;			//0x48,vBat3 Average
		_upi_u16_ regV1;					//0x4a,cell 1 Voltage
		_upi_u16_ regV2;					//0x4c,0xcell 2 Voltage
		_upi_u16_ regV3;					//0x4e,cell 3 Voltage
		_upi_s16_ regIT;					//0x50
		_upi_s16_ regET;					//0x52
		_upi_u16_ regRID;					//0x54
		_upi_s16_	regCurrent;				//0x56
		_upi_s16_ regAdc1Offset;			//0x58ADC1 offset
#if defined(uG31xx_OS_ANDROID)
}__attribute__((packed)) GG_USER2_REG, *PGG_USER2_REG;
#else   ///< else of defined(uG31xx_OS_ANDROID)
} GG_USER2_REG, *PGG_USER2_REG;
#endif  ///< end of defined(uG31xx_OS_ANDROID)

#define ALARM_STATUS_UV     (1<<0)
#define ALARM_STATUS_UET    (1<<1)
#define ALARM_STATUS_OET    (1<<2)

/* define device information */
typedef struct _GG_DEVICE_INFO{

	_upi_s16_	   oldRegCurrent;				//for skip the ADC code Error
	_upi_s16_	   oldRegVbat1;					//for skip the ADC code Error

	_upi_s16_	   vBat1_AdcCode;				//debug use
	_upi_s16_	   vBat1_AveAdcCode;

	_upi_s16_	   fwCalAveCurrent_mA;			//f/w calculate average current	
	_upi_u32_	   lastTime;					//	
	_upi_s16_	   chargeRegister;				//coulomb counter register  
	_upi_u16_	   AdcCounter;					//ADC convert counter

	_upi_s16_	   preChargeRegister;			//coulomb counter register  
	_upi_s16_    aveCurrentRegister;			//2012/0711/jacky
	_upi_u16_	   preAdcCounter;				
	_upi_s32_	   fwChargeData_mAH;			//fw calculate maH (Q= I * T)

	_upi_s32_    chargeData_mAh;				//maH calculate from charge register
	_upi_u16_    voltage_mV;		            //total voltage
	_upi_s16_    current_mA;			        // now current
	_upi_s16_	   AveCurrent_mA;				// average current
	_upi_s16_    IT;							// internal temperature
	_upi_s16_    ET;							// external temperature

	_upi_u16_	   v1_mV;						//v1   from hw register
	_upi_u16_	   v2_mV;						//v2
	_upi_u16_	   v3_mV;						//v3

	_upi_u16_	   vBat1Average_mV;				//vbat1 	 
	_upi_u16_	   vBat2Average_mV;		
	_upi_u16_	   vBat3Average_mV;		

	
	_upi_u16_    vCell1_mV;					//v Cell1
	_upi_u16_	   vCell2_mV;					//v Cell2
	_upi_u16_	   vCell3_mV;					//v Cell3

	_upi_s16_ CaliAdc1Code;				//2012/08/29/Jacky
	_upi_s16_ CaliAdc2Code;

	_upi_s16_ CoulombCounter;

  _upi_s32_ CaliChargeReg;
  _upi_u16_ Adc1ConvTime;

  _upi_u8_ alarmStatus;
#if defined(uG31xx_OS_ANDROID)
} __attribute__((packed)) GG_DEVICE_INFO, *PGG_DEVICE_INFO;
#else   ///< else of defined(uG31xx_OS_ANDROID)
} GG_DEVICE_INFO, *PGG_DEVICE_INFO;
#endif  ///< end of defined(uG31xx_OS_ANDROID)


/* define battery capacity */
typedef struct _GG_CAPACITY {
	_upi_u16_     LMD;						    //battery Qmax (maH)
	_upi_u16_     NAC;						    //battery NAC(maH)
	_upi_u16_     RSOC;					    //Battery Current RSOC(%)
	
  _upi_u8_      Ready;
#if defined(uG31xx_OS_ANDROID)
} __attribute__((packed)) GG_CAPACITY, *PGG_CAPACITY;
#else   ///< else of defined(uG31xx_OS_ANDROID)
}GG_CAPACITY, *PGG_CAPACITY;
#endif  ///< end of defined(uG31xx_OS_ANDROID)

typedef struct GG_MEAS_PARA_ST
{
  _upi_s16_ Adc1Gain;
  _upi_s16_ Adc1Offset;

  _upi_s16_ Adc2Gain;
  _upi_s16_ Adc2Offset;

  _upi_s16_ ITOffset;
  _upi_s16_ ETOffset;

  _upi_u8_ ProductType;
#if defined(uG31xx_OS_ANDROID)
}__attribute__((packed)) GG_MEAS_PARA_TYPE;
#else   ///< else of defined(uG31xx_OS_ANDROID)
} GG_MEAS_PARA_TYPE;
#endif  ///< end of defined(uG31xx_OS_ANDROID)

#endif

/// ===========================================
/// End of typeDefine.h
/// ===========================================
