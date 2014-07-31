/// ===========================================
/// uG31xx_API.cpp
/// ===========================================

#include "stdafx.h"     //windows need this??
#include "uG31xx_API.h"

static _upi_bool_ Ug31DebugEnable;
static _upi_s32_ dsg_charge_before_suspend;
static _upi_s32_ delta_cap_during_suspend;
static _upi_u8_ wakeup_predict_rsoc;
static _upi_u8_ fix_et_at_initial_cnt;
#if defined (uG31xx_OS_WINDOWS)

  #ifdef DEBUG_LOG

    unsigned int debugViewLines = 0;
    CString debugViewFileName = _T("uG3100-1");
    CString BackupFile;

  #endif

#elif defined (uG31xx_OS_ANDROID)

  _upi_u32_ GetTickCount(void) 
  {
  	return jiffies_to_msecs(jiffies);      //20121121/jacky 
  }

  _upi_u32_ GetSysTickCount(void) 
  {

  	struct timeval current_tick;

  	do_gettimeofday(&current_tick);

  	return current_tick.tv_sec * 1000 + current_tick.tv_usec/1000;
  }

#endif

#if defined(uG31xx_OS_ANDROID)

  int ug31_printk(int level, const char *fmt, ...) 
  {
    #ifdef  UG31XX_DEBUG_ENABLE
      va_list args;
      int r;

      r = 0;
      if(Ug31DebugEnable == _UPI_TRUE_)
      {
        va_start(args, fmt);
        r = vprintk(fmt, args);
        va_end(args);
      }
      return (r);
    #else   ///< else of UG31XX_DEBUG_ENABLE
      return (0);
    #endif  ///< end of UG31XX_DEBUG_ENABLE
  }
  
#endif  ///< end of defined(uG31xx_OS_ANDROID)



/// ===========================================
/// uG31xx_API.cpp (VAR)
/// ===========================================


/* uPI ug31xx hardware control interface */
struct ug31xx_data {

  /// [AT-PM] : Following variables are used for uG31xx operation ; 11/01/2012
  _upi_u8_  totalCellNums;
  _upi_bool_ bFirstData;  

  // Global variable
  CELL_TABLE      cellTable;     // data from .GGB file
  CELL_PARAMETER  cellParameter;  // data from .GGB file
  GG_BATTERY_INFO batteryInfo;
  GG_DEVICE_INFO  deviceInfo;
  GG_USER_REG     userReg;			//user register 0x00 ~0x10
  GG_USER2_REG	  user2Reg;		//user register 0x40 ~0x4f
  GG_TI_BQ27520   bq27520Cmd;
  
  OtpDataType       otpData;
  MeasDataType      measData;
  CapacityDataType  capData;
  SystemDataType    sysData;
  BackupDataType    backupData;
  
  _upi_u8_ EncriptTableStatus;
  _upi_u16_ PreviousITAve;
};

/// ===========================================
/// End of uG31xx_API.cpp (VAR)
/// ===========================================

/**
 * @brief upiGG_GetAlarmStatus
 *
 *  Get alarm status
 *
 * @para  pAlarmStatus  address of alarm status
 * @return  UG_READ_DEVICE_ALARM_SUCCESS if success
 */
GGSTATUS upiGG_GetAlarmStatus(char *pObj, _upi_u8_ *pAlarmStatus)
{
  struct ug31xx_data *pUg31xx;

  pUg31xx = (struct ug31xx_data *)pObj;

  UpiMeasAlarmThreshold(&pUg31xx->measData);
  *pAlarmStatus = UpiAlarmStatus(&pUg31xx->sysData);

  pUg31xx->userReg.regAlarm1Status = (_upi_u8_)(pUg31xx->sysData.alarmSts & 0x00ff);
  pUg31xx->userReg.regAlarm2Status = (_upi_u8_)(pUg31xx->sysData.alarmSts >> 8);
  
  return (UG_READ_DEVICE_ALARM_SUCCESS);
}

// Read GG_USER_REG from device to global variable and output
GGSTATUS upiGG_ReadAllRegister(char *pObj, GG_USER_REG* pUserReg, GG_USER2_REG* pUser2Reg)
{
  struct ug31xx_data *pUg31xx;

  pUg31xx = (struct ug31xx_data *)pObj;

	if(!API_I2C_Read(NORMAL, 
                   UG31XX_I2C_HIGH_SPEED_MODE, 
                   UG31XX_I2C_TEM_BITS_MODE, 
                   REG_MODE, 
                   sizeof(GG_USER_REG),
                   &pUg31xx->userReg.regMode))
  {
		return (UG_READ_REG_FAIL);
  }
  if(!API_I2C_Read(NORMAL, 
                   UG31XX_I2C_HIGH_SPEED_MODE, 
                   UG31XX_I2C_TEM_BITS_MODE, 
                   REG_VBAT2_LOW, 
                   sizeof(GG_USER2_REG), 
                   (_upi_u8_* )&pUg31xx->user2Reg.regVbat2))    //read
  {
		return (UG_READ_REG_FAIL);
  }  
  
  return (UG_READ_REG_SUCCESS);
}

// 07/04/1022/Jacky
_upi_u16_ CalculateVoltageFromUserReg(struct ug31xx_data *pUg31xx, _upi_s16_ voltageAdcCode, _upi_s16_ curr, _upi_u16_ offsetR, _upi_u16_ deltaR)
{    
	_upi_u16_ voltage_return;

  voltage_return = (_upi_u16_)voltageAdcCode;
	if(curr < 0) {
		voltage_return = voltage_return + offsetR*abs(curr)/1000 + deltaR;
	} else{
		voltage_return = voltage_return - offsetR*abs(curr)/1000 + deltaR;
	}
	return (voltage_return);
}

// Read GG_USER_REG from device and calculate GG_DEVICE_INFO, then write to global variable and output
// TODO: offsetR and deltaR will input from .GGB in the future modify
GGSTATUS upiGG_ReadDeviceInfo(char *pObj, GG_DEVICE_INFO* pExtDeviceInfo)
{
	// Get current user register data
	GGSTATUS status = UG_READ_DEVICE_INFO_SUCCESS;
  struct ug31xx_data *pUg31xx;
  _upi_s32_ tmp;
  MEAS_RTN_CODE rtn;

  pUg31xx = (struct ug31xx_data *)pObj;

	if(!API_I2C_Read(NORMAL, 
                   UG31XX_I2C_HIGH_SPEED_MODE, 
                   UG31XX_I2C_TEM_BITS_MODE, 
                   REG_MODE, 
                   REG_AVE_RID_HIGH - REG_MODE + 1, 
                   &pUg31xx->userReg.regMode))
  {
		status = UG_READ_ADC_FAIL;
  } 
	else
  {
		if(!API_I2C_Read(NORMAL, 
                     UG31XX_I2C_HIGH_SPEED_MODE, 
                     UG31XX_I2C_TEM_BITS_MODE, 
                     REG_INTR_STATUS, 
                     REG_CTRL2 - REG_INTR_STATUS + 1, 
                     &pUg31xx->userReg.regIntrStatus))
    {
      status = UG_READ_ADC_FAIL;
    }  
    else
    {
      if(!API_I2C_Read(NORMAL, 
                       UG31XX_I2C_HIGH_SPEED_MODE, 
                       UG31XX_I2C_TEM_BITS_MODE, 
                       REG_VBAT2_LOW, 
                       sizeof(GG_USER2_REG), 
                       (_upi_u8_* )&pUg31xx->user2Reg.regVbat2))    //read
      {
        status = UG_READ_ADC_FAIL;
      }  
    }
	}

  /// [AT-PM] : Check IT AVE code, which should be continuous ; 12/28/2012
  if(pUg31xx->PreviousITAve != 0)
  {
    tmp = (_upi_s32_)pUg31xx->PreviousITAve;
    tmp = tmp - pUg31xx->userReg.regITAve;
    if((tmp > 1000) || (tmp < -1000))
    {
      #ifdef DEBUG_LOG
        wDebug::LOGE(debugViewFileName.GetBuffer(),debugViewLines++,_T("[%s]: IT AVE Code abnormal -> %d/%d"),
                     _L(__FUNCTION__), pUg31xx->userReg.regITAve, pUg31xx->PreviousITAve);
        debugViewFileName.ReleaseBuffer();
      #endif
      pUg31xx->userReg.regITAve = pUg31xx->PreviousITAve;
    }
  }
  pUg31xx->PreviousITAve = pUg31xx->userReg.regITAve;

  /// [AT-PM] : Check OTP is empty or not ; 01/25/2013
  if(pUg31xx->otpData.empty == OTP_IS_EMPTY)
  {
    return (UG_OTP_ISEMPTY);
  }

  /// [AT-PM] : Check product type ; 01/25/2013
  if(pUg31xx->otpData.productType != UG31XX_PRODUCT_TYPE_0)
  {
    return (UG_OTP_PRODUCT_DISMATCH);
  }

  pUg31xx->measData.sysData = &pUg31xx->sysData;
  pUg31xx->measData.otp = &pUg31xx->otpData;
  rtn = UpiMeasurement(&pUg31xx->measData);
  if(rtn != MEAS_RTN_PASS)
  {
    return ((GGSTATUS)(rtn + UG_MEAS_FAIL));
  }
  
	pUg31xx->deviceInfo.chargeRegister = pUg31xx->userReg.regCharge;					//coulomb counter
	pUg31xx->deviceInfo.AdcCounter = pUg31xx->userReg.regCounter;						//adc1 convert counter
	pUg31xx->deviceInfo.aveCurrentRegister = pUg31xx->userReg.regCurrentAve;    //2012/07/11
	pUg31xx->deviceInfo.current_mA = pUg31xx->measData.curr;
	pUg31xx->deviceInfo.AveCurrent_mA = pUg31xx->measData.curr;
	pUg31xx->deviceInfo.IT = pUg31xx->measData.intTemperature;
	pUg31xx->deviceInfo.ET = pUg31xx->measData.extTemperature;
	pUg31xx->deviceInfo.v1_mV = pUg31xx->measData.bat1Voltage;
	pUg31xx->deviceInfo.vCell1_mV = pUg31xx->measData.bat1Voltage;
	pUg31xx->deviceInfo.vBat1Average_mV = CalculateVoltageFromUserReg(pUg31xx, 
                                                                    pUg31xx->measData.bat1Voltage, 
                                                                    pUg31xx->measData.curr, 
                                                                    pUg31xx->cellParameter.offsetR, 
                                                                    pUg31xx->cellParameter.deltaR);
	pUg31xx->deviceInfo.voltage_mV = pUg31xx->deviceInfo.vBat1Average_mV;
	pUg31xx->deviceInfo.chargeData_mAh = pUg31xx->measData.deltaCap;

	pUg31xx->sysData.otpData = &pUg31xx->otpData;
	UpiCalibrationOsc(&pUg31xx->sysData);			//osc calibration

	UpiAdcStatus(&pUg31xx->sysData);

	if(fix_et_at_initial_cnt > 0)
	{
		fix_et_at_initial_cnt = fix_et_at_initial_cnt - 1;

		if(pUg31xx->deviceInfo.ET > UG31XX_MAX_TEMPERATURE_BEFORE_READY)
		{
			pUg31xx->deviceInfo.ET = UG31XX_MAX_TEMPERATURE_BEFORE_READY;
		}
		if(pUg31xx->deviceInfo.ET < UG31XX_MIN_TEMPERATURE_BEFORE_READY)
		{
			pUg31xx->deviceInfo.ET = UG31XX_MIN_TEMPERATURE_BEFORE_READY;
		}
	}
	memcpy(pExtDeviceInfo, &pUg31xx->deviceInfo, sizeof(GG_DEVICE_INFO));
	return (status);
}

void dumpInfo(struct ug31xx_data *pUg31xx)
{
        int i=0;
        int j;
        int k;

/// dump parameter setting
        UG31_LOGI("/// 2012/12/16/1611====================================\n");
        UG31_LOGI("/// CELL_PARAMETER\n");
        UG31_LOGI("/// ====================================2012/12/16/1611\n");
        UG31_LOGI("Total struct size: %d\n", pUg31xx->cellParameter.totalSize);
        UG31_LOGI("firmware version: 0x%02X\n", pUg31xx->cellParameter.fw_ver)
        UG31_LOGI("customer: %s\n", pUg31xx->cellParameter.customer)
        UG31_LOGI("project: %s\n", pUg31xx->cellParameter.project)
        UG31_LOGI("ggb version: 0x%02X\n", pUg31xx->cellParameter.ggb_version)
        UG31_LOGI("customer self-define: %s\n", pUg31xx->cellParameter.customerSelfDef)
        UG31_LOGI("project self-define: %s\n", pUg31xx->cellParameter.projectSelfDef)
        UG31_LOGI("cell type : 0x%04X\n", pUg31xx->cellParameter.cell_type_code);
        UG31_LOGI("ICType: 0x%02X\n", pUg31xx->cellParameter.ICType);
        UG31_LOGI("gpio1: 0x%02X\n", pUg31xx->cellParameter.gpio1);
        UG31_LOGI("gpio2: 0x%02X\n", pUg31xx->cellParameter.gpio2);
        UG31_LOGI("gpio34: 0x%02X\n", pUg31xx->cellParameter.gpio34);
        UG31_LOGI("Chop control ?? : 0x%02X\n", pUg31xx->cellParameter.chopCtrl);
        UG31_LOGI("ADC1 offset ?? : %d\n", pUg31xx->cellParameter.adc1Offset);
        UG31_LOGI("Cell number ?? : %d\n", pUg31xx->cellParameter.cellNumber);
        UG31_LOGI("Assign cell one to: %d\n", pUg31xx->cellParameter.assignCellOneTo);
        UG31_LOGI("Assign cell two to: %d\n", pUg31xx->cellParameter.assignCellTwoTo);
        UG31_LOGI("Assign cell three to: %d\n", pUg31xx->cellParameter.assignCellThreeTo);
        UG31_LOGI("I2C Address: : 0x%02X\n", pUg31xx->cellParameter.i2cAddress);
        UG31_LOGI("I2C 10bit address: : 0x%02X\n", pUg31xx->cellParameter.tenBitAddressMode); 
        UG31_LOGI("I2C high speed: 0x%02X\n", pUg31xx->cellParameter.highSpeedMode);
        UG31_LOGI("clock(kHz): %d\n", pUg31xx->cellParameter.clock);
        UG31_LOGI("RSense(m ohm): %d\n", pUg31xx->cellParameter.rSense);
        UG31_LOGI("ILMD(mAH) ?? : %d\n", pUg31xx->cellParameter.ILMD);
        UG31_LOGI("EDV1 Voltage(mV): %d\n", pUg31xx->cellParameter.edv1Voltage);
        UG31_LOGI("Standby current ?? : %d\n", pUg31xx->cellParameter.standbyCurrent);
        UG31_LOGI("TP Current(mA)?? : %d\n", pUg31xx->cellParameter.TPCurrent);
        UG31_LOGI("TP Voltage(mV)?? : %d\n", pUg31xx->cellParameter.TPVoltage);
        UG31_LOGI("TP Time ?? : %d\n", pUg31xx->cellParameter.TPTime);
        UG31_LOGI("Offset R ?? : %d\n", pUg31xx->cellParameter.offsetR);
        UG31_LOGI("Delta R ?? : %d\n", pUg31xx->cellParameter.deltaR);
        UG31_LOGI("max delta Q(%%)  ?? : %d\n", pUg31xx->cellParameter.maxDeltaQ);
        UG31_LOGI("TP Bypass Current ?? : %d\n", pUg31xx->cellParameter.TpBypassCurrent);    //20121029/Jacky
        UG31_LOGI("time interval (s) : %d\n", pUg31xx->cellParameter.timeInterval);
        UG31_LOGI("ADC1 pgain: %d\n", pUg31xx->cellParameter.adc1_pgain);
        UG31_LOGI("ADC1 ngain: %d\n", pUg31xx->cellParameter.adc1_ngain);
        UG31_LOGI("ADC1 pos. offset: %d\n", pUg31xx->cellParameter.adc1_pos_offset);
        UG31_LOGI("ADC2 gain: %d\n", pUg31xx->cellParameter.adc2_gain);
        UG31_LOGI("ADC2 offset: %d\n", pUg31xx->cellParameter.adc2_offset);
        UG31_LOGI("R ?? : %d\n", pUg31xx->cellParameter.R);
        for (i=0; i<sizeof(pUg31xx->cellParameter.rtTable)/sizeof(_upi_u16_); i++) {
                UG31_LOGI("RTTable[%02d]: %d\n", i, pUg31xx->cellParameter.rtTable[i]);
        }
        for (i=0; i<sizeof(pUg31xx->cellParameter.SOV_TABLE)/sizeof(_upi_u16_); i++) {
                UG31_LOGI("SOV Table[%02d]: %d\n", i, pUg31xx->cellParameter.SOV_TABLE[i]/10);
        }
        UG31_LOGI("ADC d1: %d\n", pUg31xx->cellParameter.adc_d1);
        UG31_LOGI("ADC d2: %d\n", pUg31xx->cellParameter.adc_d2);
        UG31_LOGI("ADC d3: %d\n", pUg31xx->cellParameter.adc_d3);
        UG31_LOGI("ADC d4: %d\n", pUg31xx->cellParameter.adc_d4);
        UG31_LOGI("ADC d5: %d\n", pUg31xx->cellParameter.adc_d5);
        UG31_LOGI("NacLmdAdjustCfg: %d\n", pUg31xx->cellParameter.NacLmdAdjustCfg);    //20121124

        /// [AT-PM] : Dump NAC table ; 01/27/2013
        i = 0;
        while(i < TEMPERATURE_NUMS)
        {
          j = 0;
          while(j < C_RATE_NUMS)
          {
            k = 0;
            while(k < SOV_NUMS)
            {
              UG31_LOGI("NAC Table [%d][%d][%d] = %d\n", i, j, k, pUg31xx->cellTable.CELL_NAC_TABLE[i][j][k]);
              k = k + 1;
            }
            j = j + 1;
          }
          i = i + 1;
        }
}

/// count Time Elapsed in suspend/power Off  
_upi_u32_ CountTotalTime(_upi_u32_ savedTimeTag)
{
	_upi_u32_ totalTime;
	_upi_u32_ currentTime;
	
	totalTime = 0;
#if defined(uG31xx_OS_ANDROID)
	currentTime = GetSysTickCount();	
#else   ///< else of defined(uG31xx_OS_ANDROID)
	currentTime = GetTickCount();
#endif  ///< end of defined(uG31xx_OS_ANDROID)
	if(currentTime > savedTimeTag)
	{
			totalTime = currentTime - savedTimeTag;				//count the delta Time
	} 				
  else
  {
    totalTime = currentTime;
  }
	UG31_LOGE("[%s]current time/save Time/totalTime = %d/%d/%d \n",
							__func__,
							currentTime,
							savedTimeTag,
							totalTime
							);
  return(totalTime);
}

#define MS_IN_A_DAY                             (86400000)
#define INIT_CAP_FROM_CC_FACTOR                 (10)

/**
 * @brief CheckInitCapacityFromCC
 *
 *  Check the initial capacity from coulomb counter with time interval
 *  The delta RSOC should be less than n days x 0.1%
 *
 * @para  pUg31xx address of struct ug31xx_data
 * @para  lastRsoc  last RSOC before employ coulomb counter value
 * @return  _UPI_NULL_
 */
void CheckInitCapacityFromCC(struct ug31xx_data *pUg31xx, _sys_u8_ lastRsoc)
{
  _upi_s32_ tmp32;
  
  tmp32 = (_upi_s16_)pUg31xx->sysData.rsocFromIC;
  tmp32 = tmp32 - lastRsoc;
  if(tmp32 < 0)
  {
    tmp32 = (_upi_s32_)CountTotalTime(pUg31xx->sysData.timeTagFromIC)/MS_IN_A_DAY/INIT_CAP_FROM_CC_FACTOR;
    tmp32 = tmp32*(-1) + lastRsoc;
    if(tmp32 < 0)
    {
      tmp32 = 1;
    }
    UG31_LOGI("[%s]: RSOC should be limited to %d (%d <-> %d)\n", __func__, 
              tmp32, lastRsoc, pUg31xx->sysData.rsocFromIC);
    pUg31xx->sysData.rsocFromIC = (_sys_u8_)tmp32;
    tmp32 = tmp32*pUg31xx->sysData.fccFromIC/CONST_PERCENTAGE;
    pUg31xx->sysData.rmFromIC = (_sys_u16_)tmp32;
  }
}

#define MAX_DELTA_RSOC_THRESHOLD_FOR_WAKEUP     (25)
#define MIN_DELTA_RSOC_THRESHOLD_FOR_WAKEUP     (-25)
#define MAX_DELTA_TIME_THRESHOLD_FOR_WAKEUP     (MS_IN_A_DAY*5)
#define MAX_DELTA_RSOC_THRESHOLD_FOR_TABLE      (0)
#define MIN_DELTA_RSOC_THRESHOLD_FOR_TABLE      (-5)

// Read GGB file and initial
#ifdef uG31xx_OS_WINDOWS
GGSTATUS upiGG_Initial(char **pObj,const wchar_t* GGBFilename,const wchar_t* OtpFileName, const wchar_t* BackupFileName, char ForceReset, unsigned char MaxETFixCnt)
#elif defined(uG31xx_OS_ANDROID)
GGSTATUS upiGG_Initial(char **pObj,GGBX_FILE_HEADER *pGGBXBuf, char ForceReset, unsigned char MaxETFixCnt)
#endif
{
	_upi_bool_ firstPowerOn;
	struct ug31xx_data *pUg31xx;
  SYSTEM_RTN_CODE rtn;
  _upi_s16_ deltaQC = 0;
  _upi_s32_ tmp32;
  MEAS_RTN_CODE rtnMeas;
  _sys_u8_ lastRsocFromIC;

	UG31_LOGI("[%s]: uG31xx API Version = %d.%08x.%04x\n", __func__,
            UG31XX_API_MAIN_VERSION, UG31XX_API_OTP_VERSION, UG31XX_API_SUB_VERSION);
  
  Ug31DebugEnable = _UPI_TRUE_;
	firstPowerOn = _UPI_FALSE_;
  #if defined(uG31xx_OS_ANDROID)
    #ifdef  uG31xx_BOOT_LOADER
      *pObj = (char *)malloc(sizeof(struct ug31xx_data));
    #else   ///< else of uG31xx_BOOT_LOADER
      *pObj = (char *)kmalloc(sizeof(struct ug31xx_data),GFP_KERNEL);
    #endif  ///< end of uG31xx_BOOT_LOADER
  #else   ///< else of defined(uG31xx_OS_ANDROID)
	  *pObj = (char *)malloc(sizeof(struct ug31xx_data));
	#endif  ///< end of defined(uG31xx_OS_ANDROID)
	pUg31xx = (struct ug31xx_data *)(*pObj);

  memset(pUg31xx, 0, sizeof(struct ug31xx_data));

  #ifdef uG31xx_OS_WINDOWS
    pUg31xx->sysData.ggbFilename = GGBFilename;
    pUg31xx->sysData.otpFileName = OtpFileName;
    pUg31xx->sysData.backupFileName = BackupFileName;
    BackupFile = BackupFileName;
  #elif defined(uG31xx_OS_ANDROID)
    pUg31xx->sysData.ggbXBuf = pGGBXBuf;
  #endif
  pUg31xx->sysData.ggbParameter = &pUg31xx->cellParameter;
  pUg31xx->sysData.ggbCellTable = &pUg31xx->cellTable;
  rtn = UpiInitSystemData(&pUg31xx->sysData);
  if(rtn != SYSTEM_RTN_PASS)
  {
    if(rtn == SYSTEM_RTN_READ_GGB_FAIL)
    {
      return (UG_READ_GGB_FAIL);
    }
    return (UG_NOT_DEF);
  }

  // Initial I2C and Open HID
  #if defined(uG31xx_OS_WINDOWS)
  
  	if(!API_I2C_Init(pUg31xx->cellParameter.clock, pUg31xx->cellParameter.i2cAddress))
    {
  		return UG_I2C_INIT_FAIL;
  	}
    
  #endif  ///< end of defined(uG31xx_OS_WINDOWS)
  UpiLoadBatInfoFromIC(&pUg31xx->sysData);

  pUg31xx->capData.ggbTable = &pUg31xx->cellTable;
  pUg31xx->capData.ggbParameter = &pUg31xx->cellParameter;
  pUg31xx->capData.measurement = &pUg31xx->measData;
  UpiInitNacTable(&pUg31xx->capData);

  /// [AT-PM] : Check IC is active or not ; 01/28/2013
  #ifdef  UG31XX_RESET_DATABASE
    firstPowerOn = _UPI_TRUE_;
  #else   ///< else of UG31XX_RESET_DATABASE
    if(ForceReset == 0)
    {
      firstPowerOn = UpiCheckICActive();
    }
    else
    {
      firstPowerOn = _UPI_TRUE_;
      UG31_LOGI("[%s]: Force to reset uG3105 driver. (%d)\n", __func__, ForceReset);
    }
  #endif  ///< end of UG31XX_RESET_DATABASE
  if(firstPowerOn == _UPI_TRUE_)
  {
    UG31_LOGE("[%s]#####firstPowerOn= %d \n",__func__,firstPowerOn);
    rtn = UpiActiveUg31xx();
    if(rtn != SYSTEM_RTN_PASS)
    {
      return (UG_ACTIVE_FAIL);
    }

    UpiSetupAdc(&pUg31xx->sysData);
    UpiSetupSystem(&pUg31xx->sysData);

    pUg31xx->backupData.icDataAvailable = BACKUP_BOOL_FALSE;
  }
  else
  {
    UG31_LOGE("[%s]#####Last time tag = %d\n", __func__, pUg31xx->sysData.timeTagFromIC);
    pUg31xx->measData.lastTimeTick = pUg31xx->sysData.timeTagFromIC;
    pUg31xx->measData.lastDeltaCap = pUg31xx->sysData.deltaCapFromIC;
    pUg31xx->measData.adc1ConvertTime = pUg31xx->sysData.adc1ConvTime;

    pUg31xx->backupData.icDataAvailable = BACKUP_BOOL_TRUE;
  }

  /// [AT-PM] : Load OTP data ; 01/31/2013
 	API_I2C_Read(SECURITY, UG31XX_I2C_HIGH_SPEED_MODE, UG31XX_I2C_TEM_BITS_MODE, OTP1_BYTE1, OTP1_SIZE, pUg31xx->otpData.otp1);
  API_I2C_Read(SECURITY, UG31XX_I2C_HIGH_SPEED_MODE, UG31XX_I2C_TEM_BITS_MODE, OTP2_BYTE1, OTP2_SIZE, pUg31xx->otpData.otp2);
 	API_I2C_Read(NORMAL, UG31XX_I2C_HIGH_SPEED_MODE, UG31XX_I2C_TEM_BITS_MODE, OTP6_BYTE1, OTP3_SIZE, pUg31xx->otpData.otp3);
  UpiConvertOtp(&pUg31xx->otpData);

  /// [AT-PM] : Check product type ; 01/25/2013
  if(pUg31xx->otpData.productType != UG31XX_PRODUCT_TYPE_0)
  {
    return (UG_OTP_PRODUCT_DISMATCH);
  }

  if(firstPowerOn == _UPI_TRUE_)
  {
    SleepMiniSecond(1000);
    fix_et_at_initial_cnt = MaxETFixCnt;
  }
  
  UG31_LOGI("[%s]: Do measurement\n", __func__);

  pUg31xx->measData.sysData = &pUg31xx->sysData;  
  pUg31xx->measData.otp = &pUg31xx->otpData;
  rtnMeas = UpiMeasurement(&pUg31xx->measData);
  if(rtnMeas != MEAS_RTN_PASS)
  {
    return ((GGSTATUS)(rtnMeas + UG_MEAS_FAIL));
  }
  if(firstPowerOn == _UPI_TRUE_)
  {
    /// [AT-PM] : Initialize alarm function ; 04/08/2013
    UpiMeasAlarmThreshold(&pUg31xx->measData);
    UpiInitAlarm(&pUg31xx->sysData);
  }

  UG31_LOGI("[%s]: Current Status = %d mV / %d mA / %d 0.1oC\n", __func__, 
            pUg31xx->measData.bat1Voltage, pUg31xx->measData.curr, pUg31xx->measData.intTemperature);
  
  UpiInitCapacity(&pUg31xx->capData);    
  pUg31xx->capData.rsoc = CalculateRsoc(pUg31xx->capData.rm, pUg31xx->capData.fcc);
	if((firstPowerOn == _UPI_TRUE_) || (pUg31xx->sysData.fccFromIC == 0))
  {
    pUg31xx->batteryInfo.NAC = (_upi_u16_)pUg31xx->capData.rm;
    pUg31xx->batteryInfo.LMD = (_upi_u16_)pUg31xx->capData.fcc;
    pUg31xx->batteryInfo.RSOC = (_upi_u16_)pUg31xx->capData.rsoc;
    UG31_LOGI("[%s]: Init data from table -> %d/%d = %d\n", __func__, 
              pUg31xx->batteryInfo.NAC, pUg31xx->batteryInfo.LMD, pUg31xx->batteryInfo.RSOC);
	}
  else
  {
    pUg31xx->capData.tableUpdateIdx = pUg31xx->sysData.tableUpdateIdxFromIC;
    
    /// [AT-PM] : Calculate the RSOC/NAC/LMD from coulomb counter ; 01/27/2013
    deltaQC = (_upi_s16_)pUg31xx->measData.stepCap;
    pUg31xx->sysData.voltage = pUg31xx->measData.bat1Voltage;
    lastRsocFromIC = pUg31xx->sysData.rsocFromIC;
    UpiUpdateBatInfoFromIC(&pUg31xx->sysData, deltaQC);
    if(CountTotalTime(pUg31xx->sysData.timeTagFromIC) > MAX_DELTA_TIME_THRESHOLD_FOR_WAKEUP)
    {
      /// [AT-PM] : Check the data accuracy ; 01/27/2013
      deltaQC = (_upi_s16_)pUg31xx->sysData.rsocFromIC;
      deltaQC = deltaQC - pUg31xx->capData.rsoc;
      if((deltaQC > MAX_DELTA_RSOC_THRESHOLD_FOR_WAKEUP) || (deltaQC < MIN_DELTA_RSOC_THRESHOLD_FOR_WAKEUP))
      {
        deltaQC = (_upi_s16_)pUg31xx->capData.rsoc;
        deltaQC = deltaQC - pUg31xx->sysData.rsocFromICBackup;
        if((deltaQC > MAX_DELTA_RSOC_THRESHOLD_FOR_TABLE) || (deltaQC < MIN_DELTA_RSOC_THRESHOLD_FOR_TABLE))
        {
          if(deltaQC > MAX_DELTA_RSOC_THRESHOLD_FOR_TABLE)
          {
            deltaQC = MAX_DELTA_RSOC_THRESHOLD_FOR_TABLE;
          }
          if(deltaQC < MIN_DELTA_RSOC_THRESHOLD_FOR_TABLE)
          {
            deltaQC = MIN_DELTA_RSOC_THRESHOLD_FOR_TABLE;
          }
          deltaQC = deltaQC + pUg31xx->sysData.rsocFromICBackup;
          pUg31xx->capData.rsoc = (_cap_u8_)deltaQC;
          
        }
        pUg31xx->capData.fcc = pUg31xx->sysData.fccFromIC;
        tmp32 = (_upi_s32_)pUg31xx->capData.fcc;
        tmp32 = tmp32*pUg31xx->capData.rsoc/CONST_PERCENTAGE;
        pUg31xx->capData.rm = (_cap_u16_)tmp32;
        UG31_LOGI("[%s]: Coulomb counter is not available -> Use data from table (%d/%d = %d)\n", __func__,
                  pUg31xx->capData.rm, pUg31xx->capData.fcc, pUg31xx->capData.rsoc);
      }
      else
      {
        CheckInitCapacityFromCC(pUg31xx, lastRsocFromIC);
        pUg31xx->capData.rm = (_cap_u16_)pUg31xx->sysData.rmFromIC;
        pUg31xx->capData.fcc = (_cap_u16_)pUg31xx->sysData.fccFromIC;
        pUg31xx->capData.rsoc = (_cap_u8_)pUg31xx->sysData.rsocFromIC;
        UG31_LOGI("[%s]: Use data from coulomb counter (%d/%d = %d)\n", __func__,
                  pUg31xx->capData.rm, pUg31xx->capData.fcc, pUg31xx->capData.rsoc);
      }
    }
    else
    {
      CheckInitCapacityFromCC(pUg31xx, lastRsocFromIC);
      pUg31xx->capData.rm = (_cap_u16_)pUg31xx->sysData.rmFromIC;
      pUg31xx->capData.fcc = (_cap_u16_)pUg31xx->sysData.fccFromIC;
      pUg31xx->capData.rsoc = (_cap_u8_)pUg31xx->sysData.rsocFromIC;
      UG31_LOGI("[%s]: Use data from coulomb counter (%d/%d = %d)\n", __func__,
                pUg31xx->capData.rm, pUg31xx->capData.fcc, pUg31xx->capData.rsoc);
    }
    pUg31xx->batteryInfo.NAC = (_upi_u16_)pUg31xx->capData.rm;
    pUg31xx->batteryInfo.LMD = (_upi_u16_)pUg31xx->capData.fcc;
    pUg31xx->batteryInfo.RSOC = (_upi_u16_)pUg31xx->capData.rsoc;
    UpiResetCoulombCounter(&pUg31xx->measData);
  }
  UpiInitDsgCharge(&pUg31xx->capData);

  /// [AT-PM] : Save battery information to IC ; 01/31/2013
  pUg31xx->sysData.rmFromIC = pUg31xx->batteryInfo.NAC;
  pUg31xx->sysData.fccFromIC = pUg31xx->batteryInfo.LMD;
  pUg31xx->sysData.rsocFromIC = (_sys_u8_)pUg31xx->batteryInfo.RSOC;
  pUg31xx->sysData.tableUpdateIdxFromIC = pUg31xx->capData.tableUpdateIdx;
  pUg31xx->sysData.deltaCapFromIC = pUg31xx->measData.lastDeltaCap;
  pUg31xx->sysData.adc1ConvTime = pUg31xx->measData.adc1ConvertTime;
  UpiSaveBatInfoTOIC(&pUg31xx->sysData);

  UG31_LOGI("[%s]: Driver version = %d\n", __func__, UG31XX_API_RELEASE_VERSION);
  pUg31xx->backupData.targetFileVer = UG31XX_API_RELEASE_VERSION;
  pUg31xx->backupData.backupDataIdx = BACKUP_MAX_LOG_SUSPEND_DATA;
  while(pUg31xx->backupData.backupDataIdx)
  {
    pUg31xx->backupData.backupDataIdx = pUg31xx->backupData.backupDataIdx - 1;
    pUg31xx->backupData.logData[pUg31xx->backupData.backupDataIdx] = (BackupSuspendDataType *)kmalloc(sizeof(BackupSuspendDataType), GFP_KERNEL);
    memset(pUg31xx->backupData.logData[pUg31xx->backupData.backupDataIdx], 0, sizeof(BackupSuspendDataType));
  }

	dumpInfo(pUg31xx);
	return (UG_INIT_SUCCESS);
}

GGSTATUS upiGG_PreSuspend(char *pObj)
{
  GGSTATUS Status = UG_READ_DEVICE_INFO_SUCCESS;
  struct ug31xx_data *pUg31xx;

  pUg31xx = (struct ug31xx_data *)pObj;
  UG31_LOGE("[%s]:*****upiGG_PreSuspend *****\n",  __func__);

  UpiResetCoulombCounter(&pUg31xx->measData);

  /// [AT-PM] : Save battery information to IC ; 01/31/2013
  pUg31xx->sysData.rmFromIC = pUg31xx->batteryInfo.NAC;
  pUg31xx->sysData.fccFromIC = pUg31xx->batteryInfo.LMD;
  pUg31xx->sysData.rsocFromIC = (_sys_u8_)pUg31xx->batteryInfo.RSOC;
  pUg31xx->sysData.tableUpdateIdxFromIC = pUg31xx->capData.tableUpdateIdx;
  pUg31xx->sysData.deltaCapFromIC = pUg31xx->measData.lastDeltaCap;
  pUg31xx->sysData.adc1ConvTime = pUg31xx->measData.adc1ConvertTime;
  pUg31xx->sysData.voltage = pUg31xx->measData.bat1Voltage;
  UpiUpdateBatInfoFromIC(&pUg31xx->sysData, (_sys_s16_) pUg31xx->measData.stepCap);
  UpiSaveBatInfoTOIC(&pUg31xx->sysData);

  /// [AT-PM] : Save dsgCharge before suspend ; 08/14/2013
  dsg_charge_before_suspend = (_upi_s32_)pUg31xx->capData.dsgCharge;
  if(dsg_charge_before_suspend < 0)
  {
    dsg_charge_before_suspend = (_upi_s32_)pUg31xx->sysData.fccFromIC;
    dsg_charge_before_suspend = dsg_charge_before_suspend - pUg31xx->sysData.rmFromIC;
  }
  UG31_LOGI("[%s]: dsg_charge_before_suspend = %d\n", __func__, dsg_charge_before_suspend);

  pUg31xx->backupData.capData = &pUg31xx->capData;
  pUg31xx->backupData.sysData = &pUg31xx->sysData;
  pUg31xx->backupData.measData = &pUg31xx->measData;
  UpiUpdateSuspendData(&pUg31xx->backupData, _UPI_FALSE_);

  /// [AT-PM] : Set CAP_STS_NAC_UPDATE_DISQ ; 11/08/2013 
  pUg31xx->capData.status = pUg31xx->capData.status | 0x0400;

  #ifdef DEBUG_LOG
    wDebug::LOGE(debugViewFileName.GetBuffer(),debugViewLines++,_T("[%s]:Reset Coulumb counter"),	_L(__FUNCTION__));
    debugViewFileName.ReleaseBuffer();
  #endif
  return(Status);
}

//====================================================
//API Call to get the Battery Capacity
// charge full condition:
//	if((Iav <TP current) && (Voltage >= TP Voltage))
//====================================================
void upiGG_ReadCapacity(char *pObj, GG_CAPACITY *pExtCapacity)
{
  struct ug31xx_data *pUg31xx;

  pUg31xx = (struct ug31xx_data *)pObj;

  pUg31xx->capData.ggbTable = &pUg31xx->cellTable;
  pUg31xx->capData.ggbParameter = &pUg31xx->cellParameter;
  pUg31xx->capData.measurement = &pUg31xx->measData;
  UpiReadCapacity(&pUg31xx->capData);
  pUg31xx->capData.tableUpdateDelayCnt = 1;
  pUg31xx->batteryInfo.NAC = (_upi_u16_)pUg31xx->capData.rm;
  pUg31xx->batteryInfo.LMD = (_upi_u16_)pUg31xx->capData.fcc;
  pUg31xx->batteryInfo.RSOC = (_upi_u16_)pUg31xx->capData.rsoc;

	// Output result by assign value from global variable
	pExtCapacity->LMD = pUg31xx->batteryInfo.LMD;
	pExtCapacity->NAC = pUg31xx->batteryInfo.NAC;
	pExtCapacity->RSOC = pUg31xx->batteryInfo.RSOC;

  /// [AT-PM] : If fully charged and keeps charging, reset coulomb counter ; 02/11/2013
  if((pUg31xx->batteryInfo.RSOC == 100) && (pUg31xx->measData.curr >= pUg31xx->cellParameter.standbyCurrent))
  {
    pUg31xx->measData.sysData = &pUg31xx->sysData;
    pUg31xx->measData.otp = &pUg31xx->otpData;
    UpiResetCoulombCounter(&pUg31xx->measData);
  }
  
  /// [AT-PM] : Save battery information to IC ; 01/31/2013
  pUg31xx->sysData.rmFromIC = pUg31xx->batteryInfo.NAC;
  pUg31xx->sysData.fccFromIC = pUg31xx->batteryInfo.LMD;
  pUg31xx->sysData.rsocFromIC = (_sys_u8_)pUg31xx->batteryInfo.RSOC;
  pUg31xx->sysData.tableUpdateIdxFromIC = pUg31xx->capData.tableUpdateIdx;
  pUg31xx->sysData.deltaCapFromIC = pUg31xx->measData.lastDeltaCap;
  pUg31xx->sysData.adc1ConvTime = pUg31xx->measData.adc1ConvertTime;
  UpiSaveBatInfoTOIC(&pUg31xx->sysData);

  /// [AT-PM] : Check data from IC and file ; 06/19/2013
  if((pUg31xx->backupData.backupFileSts == BACKUP_FILE_STS_EXIST) &&
     (pUg31xx->backupData.icDataAvailable == BACKUP_BOOL_FALSE))
  {
    pUg31xx->backupData.icDataAvailable = BACKUP_BOOL_TRUE;
    pUg31xx->sysData.timeTagFromIC = pUg31xx->measData.lastTimeTick;
    pUg31xx->sysData.deltaCapFromIC = pUg31xx->measData.lastDeltaCap;
    UpiSaveBatInfoTOIC(&pUg31xx->sysData);
    pUg31xx->capData.rm = (_cap_u16_)pUg31xx->sysData.rmFromIC;
    pUg31xx->capData.fcc = (_cap_u16_)pUg31xx->sysData.fccFromIC;
    pUg31xx->capData.rsoc = (_cap_u8_)pUg31xx->sysData.rsocFromIC;
    pUg31xx->capData.tableUpdateIdx = pUg31xx->sysData.tableUpdateIdxFromIC;
    UpiSaveNacTable(&pUg31xx->capData);
    pUg31xx->batteryInfo.NAC = (_upi_u16_)pUg31xx->capData.rm;
    pUg31xx->batteryInfo.LMD = (_upi_u16_)pUg31xx->capData.fcc;
    pUg31xx->batteryInfo.RSOC = (_upi_u16_)pUg31xx->capData.rsoc;
    UpiInitDsgCharge(&pUg31xx->capData);
    pUg31xx->measData.adc1ConvertTime = pUg31xx->sysData.adc1ConvTime;
    UG31_LOGI("[%s]: Refresh driver information from file\n", __func__);
  }

  if(pUg31xx->backupData.icDataAvailable == BACKUP_BOOL_TRUE)
  {
    pExtCapacity->Ready = UG_CAP_DATA_READY;
  }
  else
  {
    pExtCapacity->Ready = UG_CAP_DATA_NOT_READY;
  }
}

_upi_u8_ upiGG_CheckBackupFile(char *pObj)
{
  struct ug31xx_data *pUg31xx;

  pUg31xx = (struct ug31xx_data *)pObj;

  pUg31xx->capData.ggbTable = &pUg31xx->cellTable;
  pUg31xx->capData.ggbParameter = &pUg31xx->cellParameter;
  pUg31xx->capData.measurement = &pUg31xx->measData;
  pUg31xx->measData.sysData = &pUg31xx->sysData;
  pUg31xx->measData.otp = &pUg31xx->otpData;

  pUg31xx->sysData.rmFromIC = pUg31xx->batteryInfo.NAC;
  pUg31xx->sysData.fccFromIC = pUg31xx->batteryInfo.LMD;
  pUg31xx->sysData.rsocFromIC = (_sys_u8_)pUg31xx->batteryInfo.RSOC;
  pUg31xx->sysData.tableUpdateIdxFromIC = pUg31xx->capData.tableUpdateIdx;
  pUg31xx->sysData.deltaCapFromIC = pUg31xx->measData.lastDeltaCap;
  pUg31xx->sysData.adc1ConvTime = pUg31xx->measData.adc1ConvertTime;

  /// [AT-PM] : Backup data to file routine ; 02/21/2013
  pUg31xx->backupData.capData = &pUg31xx->capData;
  pUg31xx->backupData.sysData = &pUg31xx->sysData;
  pUg31xx->backupData.measData = &pUg31xx->measData;
  #ifdef  uG31xx_OS_WINDOWS
    pUg31xx->sysData.backupFileName = BackupFile.GetBuffer();
  #endif  ///< end of uG31xx_OS_WINDOWS
  UpiBackupData(&pUg31xx->backupData);
  #ifdef  uG31xx_OS_WINDOWS
    BackupFile.ReleaseBuffer();
  #endif  ///< end of uG31xx_OS_WINDOWS

  if(pUg31xx->backupData.backupFileSts == BACKUP_FILE_STS_CHECKING)
  {
    UG31_LOGI("[%s]: Backup File check fail\n", __func__);
    return (UPI_CHECK_BACKUP_FILE_FAIL);
  }
  if(pUg31xx->backupData.backupFileSts == BACKUP_FILE_STS_COMPARE)
  {
    UG31_LOGI("[%s]: Backup File Version = %d\n", __func__, pUg31xx->backupData.backupFileVer);
    if(pUg31xx->backupData.backupFileVer != UG31XX_API_RELEASE_VERSION)
    {
      return (UPI_CHECK_BACKUP_FILE_MISMATCH);
    }
    return (UPI_CHECK_BACKUP_FILE_EXIST);
  }
  return (UPI_CHECK_BACKUP_FILE_FAIL);
}

//system wakeup
// to read back the preSuspend information from uG31xx RAM area
// re-calculate the deltaQmax( the charge/discharge) during the suspend time
GGSTATUS upiGG_Wakeup(char *pObj, int dc_in_before)
{
	GGSTATUS Status = UG_READ_DEVICE_INFO_SUCCESS;
  _upi_s16_ deltaQC = 0;							//coulomb counter's deltaQ
  _upi_u32_ totalTime;
  _upi_s32_ tmp32;
  MEAS_RTN_CODE rtn;
	_upi_u16_ rmBefore;
	_upi_u16_ fccBefore;
	_upi_u16_ rsocBefore;
     
  struct ug31xx_data *pUg31xx;
  pUg31xx = (struct ug31xx_data *)pObj;

  ///Load the Saved time tag NAC LMD
  UpiLoadBatInfoFromIC(&pUg31xx->sysData);
	rmBefore = (_upi_u16_)pUg31xx->sysData.rmFromIC;
	fccBefore = (_upi_u16_)pUg31xx->sysData.fccFromIC;
	rsocBefore = (_upi_u16_)pUg31xx->sysData.rsocFromIC;

  /// Count total Time
  totalTime = CountTotalTime(pUg31xx->sysData.timeTagFromIC);
  /// count the deltaQ during suspend 
  pUg31xx->measData.sysData = &pUg31xx->sysData;
  pUg31xx->measData.otp = &pUg31xx->otpData;
  pUg31xx->measData.lastTimeTick = pUg31xx->sysData.timeTagFromIC;
  pUg31xx->measData.lastDeltaCap = pUg31xx->sysData.deltaCapFromIC;
  pUg31xx->measData.adc1ConvertTime = pUg31xx->sysData.adc1ConvTime;
  rtn = UpiMeasurement(&pUg31xx->measData);
  pUg31xx->measData.adc1ConvertTime = pUg31xx->sysData.adc1ConvTime;
  if(rtn != MEAS_RTN_PASS)
  {
    pUg31xx->backupData.capData = &pUg31xx->capData;
    pUg31xx->backupData.sysData = &pUg31xx->sysData;
    pUg31xx->backupData.measData = &pUg31xx->measData;  
    UpiUpdateSuspendData(&pUg31xx->backupData, _UPI_TRUE_);
    return ((GGSTATUS)(rtn + UG_MEAS_FAIL));
  }

  /// [AT-PM] : Calculate delta capacity; 08/13/2013
  tmp32 = (_upi_s32_)pUg31xx->measData.deltaCap;
  tmp32 = tmp32*(pUg31xx->sysData.adc1ConvTime)/(pUg31xx->measData.adc1ConvertTime);
  pUg31xx->measData.deltaCap = (_upi_s16_)tmp32;
  pUg31xx->measData.lastDeltaCap = pUg31xx->measData.deltaCap;
  tmp32 = tmp32 - pUg31xx->sysData.deltaCapFromIC;
  pUg31xx->measData.stepCap = (_upi_s16_)tmp32;
  deltaQC = (_upi_s16_)pUg31xx->measData.stepCap;
  delta_cap_during_suspend = (_upi_s32_)deltaQC;

  /// [AT-PM] : Calculate the RSOC/NAC/LMD from coulomb counter ; 01/27/2013
  pUg31xx->sysData.voltage = pUg31xx->measData.bat1Voltage;
  UpiUpdateBatInfoFromIC(&pUg31xx->sysData, deltaQC);
	UG31_LOGE("[%s]: suspend time = %d ms,deltaQ = %d mAh, RSOC =%d, LMD = %d mAh, NAC=%d mAh\n",
							__func__,
							totalTime,
							deltaQC,
							pUg31xx->sysData.rsocFromIC,
							pUg31xx->sysData.fccFromIC,
							pUg31xx->sysData.rmFromIC);
  /// [AT-PM] : Calculate the RSOC/NAC/LMD from table ; 01/28/2013
  pUg31xx->capData.ggbTable = &pUg31xx->cellTable;
  pUg31xx->capData.ggbParameter = &pUg31xx->cellParameter;
  pUg31xx->capData.measurement = &pUg31xx->measData;
  pUg31xx->capData.tableUpdateIdx = pUg31xx->sysData.tableUpdateIdxFromIC;
  UpiTableCapacity(&pUg31xx->capData);
  pUg31xx->capData.rsoc = CalculateRsoc(pUg31xx->capData.rm, pUg31xx->capData.fcc);
  wakeup_predict_rsoc = (_upi_u8_)pUg31xx->capData.rsoc;
  /// [AT-PM] : Check the data accuracy ; 01/27/2013
  if(totalTime > MAX_DELTA_TIME_THRESHOLD_FOR_WAKEUP)
  {
    deltaQC = (_upi_s16_)pUg31xx->sysData.rsocFromIC;
    deltaQC = deltaQC - pUg31xx->capData.rsoc;
    if((deltaQC > MAX_DELTA_RSOC_THRESHOLD_FOR_WAKEUP) || (deltaQC < MIN_DELTA_RSOC_THRESHOLD_FOR_WAKEUP))
    {
      deltaQC = (_upi_s16_)pUg31xx->capData.rsoc;
      deltaQC = deltaQC - pUg31xx->sysData.rsocFromICBackup;
      if((deltaQC > MAX_DELTA_RSOC_THRESHOLD_FOR_TABLE) || (deltaQC < MIN_DELTA_RSOC_THRESHOLD_FOR_TABLE))
      {
        if(deltaQC > MAX_DELTA_RSOC_THRESHOLD_FOR_TABLE)
        {
          deltaQC = MAX_DELTA_RSOC_THRESHOLD_FOR_TABLE;
        }
        if(deltaQC < MIN_DELTA_RSOC_THRESHOLD_FOR_TABLE)
        {
          deltaQC = MIN_DELTA_RSOC_THRESHOLD_FOR_TABLE;
        }
        deltaQC = deltaQC + pUg31xx->sysData.rsocFromICBackup;
        pUg31xx->capData.rsoc = (_cap_u8_)deltaQC;
        
      }
      pUg31xx->capData.fcc = pUg31xx->sysData.fccFromIC;
      tmp32 = (_upi_s32_)pUg31xx->capData.fcc;
      tmp32 = tmp32*pUg31xx->capData.rsoc/CONST_PERCENTAGE;
      pUg31xx->capData.rm = (_cap_u16_)tmp32;
      UG31_LOGI("[%s]: Coulomb counter is not available -> Use data from table (%d/%d = %d)\n", __func__,
                pUg31xx->capData.rm, pUg31xx->capData.fcc, pUg31xx->capData.rsoc);
    }
    else
    {
      pUg31xx->capData.rm = (_cap_u16_)pUg31xx->sysData.rmFromIC;
      pUg31xx->capData.fcc = (_cap_u16_)pUg31xx->sysData.fccFromIC;
      pUg31xx->capData.rsoc = (_cap_u8_)pUg31xx->sysData.rsocFromIC;
      UG31_LOGI("[%s]: Use data from coulomb counter (%d/%d = %d)\n", __func__,
                pUg31xx->capData.rm, pUg31xx->capData.fcc, pUg31xx->capData.rsoc);
    }
  }
  else
  {
    pUg31xx->capData.rm = (_cap_u16_)pUg31xx->sysData.rmFromIC;
    pUg31xx->capData.fcc = (_cap_u16_)pUg31xx->sysData.fccFromIC;
    pUg31xx->capData.rsoc = (_cap_u8_)pUg31xx->sysData.rsocFromIC;
    UG31_LOGI("[%s]: Use data from coulomb counter (%d/%d = %d)\n", __func__,
              pUg31xx->capData.rm, pUg31xx->capData.fcc, pUg31xx->capData.rsoc);
  }

	/// [AT-PM] : Check dc in before suspend ; 10/22/2013
	if(dc_in_before == 0)
	{
		if(pUg31xx->batteryInfo.RSOC < pUg31xx->capData.rsoc)
		{
			UG31_LOGI("[%s]: Fix the RSOC the same value before suspend = %d (%d)\n", __func__, rsocBefore, pUg31xx->capData.rsoc);
			pUg31xx->capData.rm = (_cap_u16_)rmBefore;
			pUg31xx->capData.fcc = (_cap_u16_)fccBefore;
			pUg31xx->capData.rsoc = (_cap_u16_)rsocBefore;
		}

		tmp32 = (_upi_s32_)totalTime;
		tmp32 = tmp32/1000*(pUg31xx->cellParameter.standbyCurrent)/2/3600*(-1);
		UG31_LOGI("[%s]: Estimated capacity = %d\n", __func__, tmp32);

		if(deltaQC >= tmp32)
		{
			UG31_LOGI("[%s]: Apply static discharging current\n", __func__);

			tmp32 = tmp32 + deltaQC;
			tmp32 = tmp32/2;
			tmp32 = tmp32 + rmBefore;
			if(tmp32 < 0)
			{
				tmp32 = 0;
			}
			UG31_LOGI("[%s]: New RM = %d (%d)\n", __func__, tmp32, rmBefore);

			pUg31xx->capData.rm = (_cap_u16_)tmp32;
			pUg31xx->capData.rsoc = CalculateRsoc(pUg31xx->capData.rm, pUg31xx->capData.fcc);
			UG31_LOGI("[%s]: Battery status -> %d / %d = %d\n", __func__, pUg31xx->capData.rm, pUg31xx->capData.fcc, pUg31xx->capData.rsoc);
		}

		if(pUg31xx->measData.rawCodeCharge > 0)
		{
			pUg31xx->measData.ccOffsetAdj = pUg31xx->measData.ccOffsetAdj + 1;
			if(pUg31xx->measData.ccOffsetAdj > pUg31xx->cellParameter.standbyCurrent)
			{
				pUg31xx->measData.ccOffsetAdj = pUg31xx->cellParameter.standbyCurrent;
			}
			UG31_LOGI("[%s]: Adjust ADC1 offset = %d\n", __func__, pUg31xx->measData.ccOffsetAdj);
		}
	}

  pUg31xx->batteryInfo.NAC = (_upi_u16_)pUg31xx->capData.rm;
  pUg31xx->batteryInfo.LMD = (_upi_u16_)pUg31xx->capData.fcc;
  pUg31xx->batteryInfo.RSOC = (_upi_u16_)pUg31xx->capData.rsoc;
  UpiInitDsgCharge(&pUg31xx->capData);

  UpiResetCoulombCounter(&pUg31xx->measData);

  /// [AT-PM] : Save battery information to IC ; 01/31/2013
  pUg31xx->sysData.rmFromIC = pUg31xx->batteryInfo.NAC;
  pUg31xx->sysData.fccFromIC = pUg31xx->batteryInfo.LMD;
  pUg31xx->sysData.rsocFromIC = (_sys_u8_)pUg31xx->batteryInfo.RSOC;
  pUg31xx->sysData.tableUpdateIdxFromIC = pUg31xx->capData.tableUpdateIdx;
  pUg31xx->sysData.deltaCapFromIC = pUg31xx->measData.lastDeltaCap;
  pUg31xx->sysData.adc1ConvTime = pUg31xx->measData.adc1ConvertTime;
  UpiSaveBatInfoTOIC(&pUg31xx->sysData);

  pUg31xx->backupData.capData = &pUg31xx->capData;
  pUg31xx->backupData.sysData = &pUg31xx->sysData;
  pUg31xx->backupData.measData = &pUg31xx->measData;  
  UpiUpdateSuspendData(&pUg31xx->backupData, _UPI_TRUE_);

#if 0
  /// [AT-PM] : Backup data to file routine ; 02/21/2013
  pUg31xx->backupData.capData = &pUg31xx->capData;
  pUg31xx->backupData.sysData = &pUg31xx->sysData;
  pUg31xx->backupData.measData = &pUg31xx->measData;
  UpiBackupData(&pUg31xx->backupData);
#endif
  return (Status);
}

/**
 * @brief upiGG_AccessMeasurementParameter
 *
 *  Access measurement parameter
 *
 * @para  read  set _UPI_TRUE_ to read data from API
 * @para  pMeasPara pointer of GG_MEAS_PARA_TYPE
 * @return  GGSTATUS
 */
GGSTATUS upiGG_AccessMeasurementParameter(char *pObj, _upi_bool_ read, GG_MEAS_PARA_TYPE *pMeasPara)
{
  struct ug31xx_data *pUg31xx;

  pUg31xx = (struct ug31xx_data *)pObj;

  /// [AT-PM] : Read data ; 08/29/2012
  if(read == _UPI_TRUE_)
  {
    pMeasPara->Adc1Gain = pUg31xx->cellParameter.adc1_ngain;
    pMeasPara->Adc1Offset = pUg31xx->cellParameter.adc1_pos_offset;
    pMeasPara->Adc2Gain = pUg31xx->cellParameter.adc2_gain;
    pMeasPara->Adc2Offset = pUg31xx->cellParameter.adc2_offset;
    pMeasPara->ITOffset = pUg31xx->cellParameter.adc_d5;
    pMeasPara->ETOffset = pUg31xx->cellParameter.adc_d4;
    pMeasPara->ProductType = pUg31xx->otpData.productType;
    return (UG_SUCCESS);
  }

  /// [AT-PM] : Write data ; 08/29/2012
  pUg31xx->cellParameter.adc1_ngain = pMeasPara->Adc1Gain;
  pUg31xx->cellParameter.adc1_pos_offset = pMeasPara->Adc1Offset;
  pUg31xx->cellParameter.adc2_gain = pMeasPara->Adc2Gain;
  pUg31xx->cellParameter.adc2_offset = pMeasPara->Adc2Offset;
  pUg31xx->cellParameter.adc_d5 = pMeasPara->ITOffset;
  pUg31xx->cellParameter.adc_d4 = pMeasPara->ETOffset;
  return (UG_SUCCESS);
}

#ifdef  ENABLE_BQ27520_SW_CMD

/**
 * @brief TI_Cntl
 *
 *  Control() command
 *
 * @para  pData address of data
 * @return  GGSTATUS
 */
GGSTATUS TI_Cntl(struct ug31xx_data *pUg31xx, _upi_u16_ *pData)
{
  _upi_u16_ CntlData;

  CntlData = *pData;
  switch(CntlData)
  {
    case UG_STD_CMD_CNTL_CONTROL_STATUS:
      *pData = pUg31xx->bq27520Cmd.CntlControlStatus;
      break;
    case UG_STD_CMD_CNTL_DEVICE_TYPE:
      *pData = 0x3103;
      break;
    case UG_STD_CMD_CNTL_FW_VERSION:
      *pData = 0x0001;
      break;
    case UG_STD_CMD_CNTL_PREV_MACWRITE:
      *pData = pUg31xx->bq27520Cmd.CntlPrevMacWrite;
      break;
    case UG_STD_CMD_CNTL_CHEM_ID:
      *pData = 0x0001;
      break;
    case UG_STD_CMD_CNTL_OCV_CMD:
      break;
    case UG_STD_CMD_CNTL_BAT_INSERT:
      if(!(pUg31xx->bq27520Cmd.Opcfg & UG_STD_CMD_OPCFG_BIE))
      {
        pUg31xx->bq27520Cmd.Flags = pUg31xx->bq27520Cmd.Flags | UG_STD_CMD_FLAGS_BAT_DET;
      }
      break;
    case UG_STD_CMD_CNTL_BAT_REMOVE:
      if(!(pUg31xx->bq27520Cmd.Opcfg & UG_STD_CMD_OPCFG_BIE))
      {
        pUg31xx->bq27520Cmd.Flags = pUg31xx->bq27520Cmd.Flags & (~UG_STD_CMD_FLAGS_BAT_DET);
      }
      break;
    case UG_STD_CMD_CNTL_SET_HIBERNATE:
      pUg31xx->bq27520Cmd.CntlControlStatus = pUg31xx->bq27520Cmd.CntlControlStatus | UG_STD_CMD_CNTL_CONTROL_STATUS_HIBERNATE;
      break;
    case UG_STD_CMD_CNTL_CLEAR_HIBERNATE:
      pUg31xx->bq27520Cmd.CntlControlStatus = pUg31xx->bq27520Cmd.CntlControlStatus & (~UG_STD_CMD_CNTL_CONTROL_STATUS_HIBERNATE);
      break;
    case UG_STD_CMD_CNTL_SET_SLEEP_PLUS:
      pUg31xx->bq27520Cmd.CntlControlStatus = pUg31xx->bq27520Cmd.CntlControlStatus | UG_STD_CMD_CNTL_CONTROL_STATUS_SNOOZE;
      break;
    case UG_STD_CMD_CNTL_CLEAR_SLEEP_PLUS:
      pUg31xx->bq27520Cmd.CntlControlStatus = pUg31xx->bq27520Cmd.CntlControlStatus & (~UG_STD_CMD_CNTL_CONTROL_STATUS_SNOOZE);
      break;
    case UG_STD_CMD_CNTL_FACTORY_RESTORE:
      break;
    case UG_STD_CMD_CNTL_ENABLE_DLOG:
      pUg31xx->bq27520Cmd.CntlControlStatus = pUg31xx->bq27520Cmd.CntlControlStatus | UG_STD_CMD_CNTL_CONTROL_STATUS_DLOGEN;
      break;
    case UG_STD_CMD_CNTL_DISABLE_DLOG:
      pUg31xx->bq27520Cmd.CntlControlStatus = pUg31xx->bq27520Cmd.CntlControlStatus & (~UG_STD_CMD_CNTL_CONTROL_STATUS_DLOGEN);
      break;
    case UG_STD_CMD_CNTL_DF_VERSION:
      *pData = 0x0000;
      break;
    case UG_STD_CMD_CNTL_SEALED:
      pUg31xx->bq27520Cmd.CntlControlStatus = pUg31xx->bq27520Cmd.CntlControlStatus | UG_STD_CMD_CNTL_CONTROL_STATUS_SS;
      break;
    case UG_STD_CMD_CNTL_RESET:
      if(!(pUg31xx->bq27520Cmd.CntlControlStatus & UG_STD_CMD_CNTL_CONTROL_STATUS_SS))
      {
      }
      break;
    default:
      *pData = 0x0000;
      break;
  }
  
  pUg31xx->bq27520Cmd.CntlPrevMacWrite = ((CntlData) > UG_STD_CMD_CNTL_PREV_MACWRITE) ? UG_STD_CMD_CNTL_PREV_MACWRITE : CntlData;
  return (UG_SUCCESS);
}

/**
 * @brief TI_AR
 *
 *  AtRate() command
 *
 * @para  pData address of data
 * @return  GGSTATUS
 */
GGSTATUS TI_AR(struct ug31xx_data *pUg31xx, _upi_u16_ *pData)
{
  _upi_s16_ AR;

  AR = (_upi_s16_)(*pData);
  if(AR != pUg31xx->bq27520Cmd.AR)
  {
    pUg31xx->bq27520Cmd.AR = AR;
  }
  return (UG_SUCCESS);
}

/**
 * @brief TI_Artte
 *
 *  AtRateTimeToEmpty() command
 *
 * @para  pData address of data
 * @return  GGSTATUS
 */
GGSTATUS TI_Artte(struct ug31xx_data *pUg31xx, _upi_u16_ *pData)
{
  _upi_s32_ Artte;

  if(pUg31xx->bq27520Cmd.AR >= 0)
  {
    *pData = 65535;
  }
  
  Artte = (_upi_s32_)pUg31xx->batteryInfo.NAC;
  Artte = Artte*60*(-1)/pUg31xx->bq27520Cmd.AR;
  *pData = (_upi_u16_)Artte;
  return (UG_SUCCESS);
}

/**
 * @brief TI_Temp
 *
 *  Temperature() command
 *
 * @para  pData address of data
 * @return  GGSTATUS
 */
GGSTATUS TI_Temp(struct ug31xx_data *pUg31xx, _upi_u16_ *pData)
{
  if(pUg31xx->bq27520Cmd.Opcfg & UG_STD_CMD_OPCFG_WRTEMP)
  {
    /// [AT-PM] : Temperature is from host ; 10/11/2012
    pUg31xx->bq27520Cmd.Temp = *pData;
  }
  else
  {
    /// [AT-PM] : Temperature is measured by uG31xx ; 10/11/2012
    if(pUg31xx->bq27520Cmd.Opcfg & UG_STD_CMD_OPCFG_TEMPS)
    {
      /// [AT-PM] : Report external temperature ; 10/11/2012
      *pData = pUg31xx->deviceInfo.ET;
    }
    else
    {
      /// [AT-PM] : Report internal temperature ; 10/11/2012
      *pData = pUg31xx->deviceInfo.IT;
    }
  }
  return (UG_SUCCESS);
}

/**
 * @brief TI_Volt
 *
 *  Voltage() command
 *
 * @para  pData address of data
 * @return  GGSTATUS
 */
GGSTATUS TI_Volt(struct ug31xx_data *pUg31xx, _upi_u16_ *pData)
{
  *pData = pUg31xx->deviceInfo.voltage_mV;
  return (UG_SUCCESS);
}

/**
 * @brief TI_Flags
 *
 *  Flags() command
 *
 * @para  pData address of data
 * @return  GGSTATUS
 */
GGSTATUS TI_Flags(struct ug31xx_data *pUg31xx, _upi_u16_ *pData)
{  
  /// [AT-PM] : OTC - Overtemperature in charge ; 10/11/2012
  
  /// [AT-PM] : OTD - Overtemperature in discharge ; 10/11/2012
  
  /// [AT-PM] : CHG_INH - Charge inhibit ; 10/11/2012
  
  /// [AT-PM] : XCHG - Charge suspend alert ; 10/11/2012
  
  /// [AT-PM] : FC - Full-charged ; 10/11/2012
  if(pUg31xx->batteryInfo.RSOC < pUg31xx->bq27520Cmd.FCClear)
  {
    pUg31xx->bq27520Cmd.Flags = pUg31xx->bq27520Cmd.Flags & (~UG_STD_CMD_FLAGS_FC);
  }
  if(pUg31xx->bq27520Cmd.FCSet < 0)
  {
    if(pUg31xx->batteryInfo.RSOC == 100)
    {
      pUg31xx->bq27520Cmd.Flags = pUg31xx->bq27520Cmd.Flags | UG_STD_CMD_FLAGS_FC;
    }
  }
  else
  {
    if(pUg31xx->batteryInfo.RSOC > pUg31xx->bq27520Cmd.FCSet)
    {
      pUg31xx->bq27520Cmd.Flags = pUg31xx->bq27520Cmd.Flags | UG_STD_CMD_FLAGS_FC;
    }
  }
  
  /// [AT-PM] : CHG - (Fast) charging allowed ; 10/11/2012
  if(pUg31xx->bq27520Cmd.Flags & UG_STD_CMD_FLAGS_FC)
  {
    pUg31xx->bq27520Cmd.Flags = pUg31xx->bq27520Cmd.Flags & (~UG_STD_CMD_FLAGS_CHG);
  }
  else
  {
    pUg31xx->bq27520Cmd.Flags = pUg31xx->bq27520Cmd.Flags | UG_STD_CMD_FLAGS_CHG;
  }
  
  /// [AT-PM] : OCV_GD - Good OCV measurement taken ; 10/11/2012
  
  /// [AT-PM] : WAIT_ID - Waiting to identify inserted battery ; 10/11/2012
  
  /// [AT-PM] : BAT_DET - Battery detected ; 10/11/2012
  if(pUg31xx->userReg.regAlarm2Status & ALARM2_STATUS_OV1_ALARM)
  {
    pUg31xx->bq27520Cmd.Flags = pUg31xx->bq27520Cmd.Flags | UG_STD_CMD_FLAGS_BAT_DET;
  }
  else
  {
    pUg31xx->bq27520Cmd.Flags = pUg31xx->bq27520Cmd.Flags & (~UG_STD_CMD_FLAGS_BAT_DET);
  }
  
  /// [AT-PM] : SOC1 - State-of-charge threshold 1 (SOC1 Set) reached ; 10/11/2012
  if(pUg31xx->batteryInfo.NAC > pUg31xx->bq27520Cmd.Soc1Clear)
  {
    pUg31xx->bq27520Cmd.Flags = pUg31xx->bq27520Cmd.Flags & (~UG_STD_CMD_FLAGS_SOC1);
  }
  if(pUg31xx->batteryInfo.NAC < pUg31xx->bq27520Cmd.Soc1Set)
  {
    pUg31xx->bq27520Cmd.Flags = pUg31xx->bq27520Cmd.Flags | UG_STD_CMD_FLAGS_SOC1;
  }
  
  /// [AT-PM] : SYSDOWN - System should shut down ; 10/11/2012
  
  /// [AT-PM] : DSG - Discharging detected ; 10/11/2012
  if(pUg31xx->deviceInfo.AveCurrent_mA <= 0)
  {
    pUg31xx->bq27520Cmd.Flags = pUg31xx->bq27520Cmd.Flags | UG_STD_CMD_FLAGS_DSG;
  }
  else
  {
    pUg31xx->bq27520Cmd.Flags = pUg31xx->bq27520Cmd.Flags & (~UG_STD_CMD_FLAGS_DSG);
  }

  *pData = pUg31xx->bq27520Cmd.Flags;
  return (UG_SUCCESS);
}

/**
 * @brief TI_Nac
 *
 *  NominalAvailableCapacity() command
 *
 * @para  pData address of data
 * @return  GGSTATUS
 */
GGSTATUS TI_Nac(struct ug31xx_data *pUg31xx, _upi_u16_ *pData)
{  
  *pData = pUg31xx->batteryInfo.NAC;
  return (UG_SUCCESS);
}

/**
 * @brief TI_Fac
 *
 *  FullAvailableCapacity() command
 *
 * @para  pData address of data
 * @return  GGSTATUS
 */
GGSTATUS TI_Fac(struct ug31xx_data *pUg31xx, _upi_u16_ *pData)
{
  *pData = pUg31xx->batteryInfo.LMD;
  return (UG_SUCCESS);
}

/**
 * @brief TI_RM
 *
 *  RemainingCapacity() command
 *
 * @para  pData address of data
 * @return  GGSTATUS
 */
GGSTATUS TI_RM(struct ug31xx_data *pUg31xx, _upi_u16_ *pData)
{
  *pData = pUg31xx->batteryInfo.NAC;
  return (UG_SUCCESS);
}

/**
 * @brief TI_Fcc
 *
 *  FullChargeCapacity() command
 *
 * @para  pData address of data
 * @return  GGSTATUS
 */
GGSTATUS TI_Fcc(struct ug31xx_data *pUg31xx, _upi_u16_ *pData)
{
  *pData = pUg31xx->batteryInfo.LMD;
  return (UG_SUCCESS);
}

/**
 * @brief TI_AI
 *
 *  AverageCurrent() command
 *
 * @para  pData address of data
 * @return  GGSTATUS
 */
GGSTATUS TI_AI(struct ug31xx_data *pUg31xx, _upi_u16_ *pData)
{
  *pData = pUg31xx->deviceInfo.AveCurrent_mA;
  return (UG_SUCCESS);
}

/**
 * @brief TI_Tte
 *
 *  TimeToEmpty() command
 *
 * @para  pData address of data
 * @return  GGSTATUS
 */
GGSTATUS TI_Tte(struct ug31xx_data *pUg31xx, _upi_u16_ *pData)
{
  _upi_s32_ Tte;

  if(pUg31xx->deviceInfo.AveCurrent_mA >= 0)
  {
    *pData = 65535;
    return (UG_SUCCESS);
  }
  
  Tte = (_upi_s32_)pUg31xx->batteryInfo.NAC;
  Tte = Tte*60*(-1)/pUg31xx->deviceInfo.AveCurrent_mA;
  *pData = (_upi_u16_)Tte;
  return (UG_SUCCESS);
}

/**
 * @brief TI_Ttf
 *
 *  TimeToFull() command
 *
 * @para  pData address of data
 * @return  GGSTATUS
 */
GGSTATUS TI_Ttf(struct ug31xx_data *pUg31xx, _upi_u16_ *pData)
{
  _upi_s32_ Ttf;

  if(pUg31xx->deviceInfo.AveCurrent_mA <= 0)
  {
    *pData = 65535;
    return (UG_SUCCESS);
  }
  
  Ttf = (_upi_s32_)pUg31xx->batteryInfo.LMD;
  Ttf = Ttf - pUg31xx->batteryInfo.NAC;
  Ttf = Ttf*90/pUg31xx->deviceInfo.AveCurrent_mA;
  *pData = (_upi_u16_)Ttf;
  return (UG_SUCCESS);
}

/**
 * @brief TI_SI
 *
 *  StandbyCurrent() command
 *
 * @para  pData address of data
 * @return  GGSTATUS
 */
GGSTATUS TI_SI(struct ug31xx_data *pUg31xx, _upi_u16_ *pData)
{
  _upi_s16_ LowerBound;
  _upi_s32_ NewSI;

  /// [AT-PM] : Set initial SI ; 10/11/2012
  if(pUg31xx->bq27520Cmd.SINow == 0)
  {
    pUg31xx->bq27520Cmd.SINow = pUg31xx->bq27520Cmd.InitSI;
  }
  
  LowerBound = pUg31xx->bq27520Cmd.InitSI*2;
  if(LowerBound > 0)
  {
    LowerBound = LowerBound*(-1);
  }

  /// [AT-PM] : SI criteria - 2 x InitSI < Current < 0 ; 10/11/2012
  if((pUg31xx->deviceInfo.AveCurrent_mA < 0) && (pUg31xx->deviceInfo.AveCurrent_mA > LowerBound))
  {
    /// [AT-PM] : Update SI every 1 minute ; 10/11/2012
    if((pUg31xx->bq27520Cmd.SIWindow >= 60) && (pUg31xx->bq27520Cmd.SISample > 0))
    {
      NewSI = pUg31xx->bq27520Cmd.SIBuf/pUg31xx->bq27520Cmd.SISample;
      NewSI = NewSI*7 + pUg31xx->bq27520Cmd.SINow*93;
      NewSI = NewSI/100;
      pUg31xx->bq27520Cmd.SINow = (_upi_s16_)NewSI;
      pUg31xx->bq27520Cmd.SISample = -1;
      pUg31xx->bq27520Cmd.SIBuf = 0;
      pUg31xx->bq27520Cmd.SIWindow = 0;
    }
    else
    {
      pUg31xx->bq27520Cmd.SISample = pUg31xx->bq27520Cmd.SISample + 1;
      pUg31xx->bq27520Cmd.SIWindow = pUg31xx->bq27520Cmd.SIWindow + pUg31xx->bq27520Cmd.DeltaSec;

      /// [AT-PM] : Ignore the first sample ; 10/11/2012
      if(pUg31xx->bq27520Cmd.SISample > 0)
      {
        pUg31xx->bq27520Cmd.SIBuf = pUg31xx->bq27520Cmd.SIBuf + pUg31xx->deviceInfo.AveCurrent_mA;
      }
    }
  }
  else
  {
    /// [AT-PM] : Ignore the last sample ; 10/11/2012
    if(pUg31xx->bq27520Cmd.SISample > 0)
    {
      NewSI = pUg31xx->bq27520Cmd.SIBuf/pUg31xx->bq27520Cmd.SISample;
      NewSI = NewSI*7 + pUg31xx->bq27520Cmd.SINow*93;
      NewSI = NewSI/100;
      pUg31xx->bq27520Cmd.SINow = (_upi_s16_)NewSI;
    }
    pUg31xx->bq27520Cmd.SISample = -1;
    pUg31xx->bq27520Cmd.SIBuf = 0;
    pUg31xx->bq27520Cmd.SIWindow = 0;
  }

  *pData = pUg31xx->bq27520Cmd.SINow;
  return (UG_SUCCESS);
}

/**
 * @brief TI_Stte
 *
 *  StandbyTimeToEmpty() command
 *
 * @para  pData address of data
 * @return  GGSTATUS
 */
GGSTATUS TI_Stte(struct ug31xx_data *pUg31xx, _upi_u16_ *pData)
{
  _upi_s32_ Stte;

  if(pUg31xx->bq27520Cmd.SINow >= 0)
  {
    *pData = 65535;
    return (UG_SUCCESS);
  }
  
  Stte = (_upi_s32_)pUg31xx->batteryInfo.NAC;
  Stte = Stte*60*(-1)/pUg31xx->bq27520Cmd.SINow;
  *pData = (_upi_u16_)Stte;
  return (UG_SUCCESS);
}

/**
 * @brief TI_Mli
 *
 *  MaxLoadCurrent() command
 *
 * @para  pData address of data
 * @return  GGSTATUS
 */
GGSTATUS TI_Mli(struct ug31xx_data *pUg31xx, _upi_u16_ *pData)
{
  _upi_s32_ NewMli;

  /// [AT-PM] : Set initial MLI ; 10/11/2012
  if(pUg31xx->bq27520Cmd.Mli == 0)
  {
    pUg31xx->bq27520Cmd.Mli = pUg31xx->bq27520Cmd.InitMaxLoadCurrent;
  }

  /// [AT-PM] : Get the start charging SOC ; 10/11/2012
  if(pUg31xx->bq27520Cmd.Flags & UG_STD_CMD_FLAGS_DSG)
  {
    pUg31xx->bq27520Cmd.MliDsgSoc = (_upi_u8_)pUg31xx->batteryInfo.RSOC;
  }

  /// [AT-PM] : MLI criteria - Current < MLI ; 10/11/2012
  if(pUg31xx->deviceInfo.AveCurrent_mA < pUg31xx->bq27520Cmd.Mli)
  {
    pUg31xx->bq27520Cmd.Mli = pUg31xx->deviceInfo.AveCurrent_mA;
  }

  /// [AT-PM] : Reduce MLI at FC ; 10/11/2012
  if((pUg31xx->bq27520Cmd.Flags & UG_STD_CMD_FLAGS_FC) && (pUg31xx->bq27520Cmd.MliDsgSoc < 50))
  {
    NewMli = (_upi_s32_)pUg31xx->bq27520Cmd.InitMaxLoadCurrent;
    NewMli = NewMli + pUg31xx->bq27520Cmd.Mli;
    NewMli = NewMli/2;
    pUg31xx->bq27520Cmd.Mli = (_upi_s16_)NewMli;
  }

  *pData = (_upi_u16_)pUg31xx->bq27520Cmd.Mli;
  return (UG_SUCCESS);
}

/**
 * @brief TI_Stte
 *
 *  MaxLoadTimeToEmpty() command
 *
 * @para  pData address of data
 * @return  GGSTATUS
 */
GGSTATUS TI_Mltte(struct ug31xx_data *pUg31xx, _upi_u16_ *pData)
{
  _upi_s32_ Mltte;
  
  if(pUg31xx->deviceInfo.AveCurrent_mA >= 0)
  {
    *pData = 65535;
    return (UG_SUCCESS);
  }

  Mltte = (_upi_s32_)pUg31xx->batteryInfo.NAC;
  Mltte = Mltte*60*(-1)/pUg31xx->bq27520Cmd.Mli;
  *pData = (_upi_u16_)Mltte;
  return (UG_SUCCESS);
}

/**
 * @brief TI_AE
 *
 *  AvailableEnergy() command
 *
 * @para  pData address of data
 * @return  GGSTATUS
 */
GGSTATUS TI_AE(struct ug31xx_data *pUg31xx, _upi_u16_ *pData)
{
  _upi_u32_ AE;
  
  AE = (_upi_u32_)pUg31xx->batteryInfo.NAC;
  AE = AE*pUg31xx->deviceInfo.voltage_mV/1000;
  pUg31xx->bq27520Cmd.AE = (_upi_u16_)AE;
  *pData = pUg31xx->bq27520Cmd.AE;
  return (UG_SUCCESS);
}

/**
 * @brief TI_AP
 *
 *  AveragePower() command
 *
 * @para  pData address of data
 * @return  GGSTATUS
 */
GGSTATUS TI_AP(struct ug31xx_data *pUg31xx, _upi_u16_ *pData)
{
  _upi_s32_ AP;
  
  if((pUg31xx->deviceInfo.AveCurrent_mA == 0) || (pUg31xx->bq27520Cmd.APDsgTime == 0))
  {
    pUg31xx->bq27520Cmd.AP = 0;
    *pData = 0;
    return (UG_SUCCESS);
  }

  AP = (_upi_s32_)pUg31xx->batteryInfo.NAC;
  AP = AP*pUg31xx->deviceInfo.voltage_mV/1000;

  /// [AT-PM] : Average discharging power ; 10/11/2012
  if(pUg31xx->deviceInfo.AveCurrent_mA > 0)
  {
    pUg31xx->bq27520Cmd.APStartDsgE = AP;
    pUg31xx->bq27520Cmd.APDsgTime = 0;

    pUg31xx->bq27520Cmd.APChgTime = pUg31xx->bq27520Cmd.APChgTime + pUg31xx->bq27520Cmd.DeltaSec;
    AP = AP - pUg31xx->bq27520Cmd.APStartChgE;
    AP = AP*3600/pUg31xx->bq27520Cmd.APChgTime;
    pUg31xx->bq27520Cmd.AP = (_upi_s16_)AP;
  }

  /// [AT-PM] : Average charging power ; 10/11/2012
  if(pUg31xx->deviceInfo.AveCurrent_mA < 0)
  {
    pUg31xx->bq27520Cmd.APStartChgE = AP;
    pUg31xx->bq27520Cmd.APChgTime = 0;

    pUg31xx->bq27520Cmd.APDsgTime = pUg31xx->bq27520Cmd.APDsgTime + pUg31xx->bq27520Cmd.DeltaSec;
    AP = AP - pUg31xx->bq27520Cmd.APStartDsgE;
    AP = AP*3600/pUg31xx->bq27520Cmd.APDsgTime;
    pUg31xx->bq27520Cmd.AP = (_upi_s16_)AP;
  }

  *pData = (_upi_u16_)pUg31xx->bq27520Cmd.AP;
  return (UG_SUCCESS);
}

/**
 * @brief TI_Ttecp
 *
 *  TimeToEmptyAtConstantPower() command
 *
 * @para  pData address of data
 * @return  GGSTATUS
 */
GGSTATUS TI_Ttecp(struct ug31xx_data *pUg31xx, _upi_u16_ *pData)
{
  _upi_s32_ Ttecp;
  
  if(pUg31xx->bq27520Cmd.AP >= 0)
  {
    *pData = 65535;
    return (UG_SUCCESS);
  }

  Ttecp = (_upi_s32_)pUg31xx->bq27520Cmd.AE;
  Ttecp = Ttecp*60*(-1)/pUg31xx->bq27520Cmd.AP;
  *pData = (_upi_u16_)Ttecp;
  return (UG_SUCCESS);  
}

/**
 * @brief TI_Soh
 *
 *  StateOfHealth() command
 *
 * @para  pData address of data
 * @return  GGSTATUS
 */
GGSTATUS TI_Soh(struct ug31xx_data *pUg31xx, _upi_u16_ *pData)
{
  _upi_u32_ Soh;

  Soh = (_upi_u32_)pUg31xx->batteryInfo.LMD;
  Soh = Soh*100/pUg31xx->bq27520Cmd.Dcap;
  
  Soh = Soh & UG_STD_CMD_SOH_VALUE_MASK;
  Soh = Soh | UG_STD_CMD_SOH_STATUS_READY;
  *pData = (_upi_u16_)Soh;
  return (UG_SUCCESS);
}

/**
 * @brief TI_CC
 *
 *  CycleCount() command
 *
 * @para  pData address of data
 * @return  GGSTATUS
 */
GGSTATUS TI_CC(struct ug31xx_data *pUg31xx, _upi_u16_ *pData)
{
  if(pUg31xx->deviceInfo.AveCurrent_mA < 0)
  {
    pUg31xx->bq27520Cmd.CCBuf = pUg31xx->bq27520Cmd.CCBuf + pUg31xx->bq27520Cmd.CCLastNac - pUg31xx->batteryInfo.NAC;
  }
  pUg31xx->bq27520Cmd.CCLastNac = pUg31xx->batteryInfo.NAC;

  if(pUg31xx->bq27520Cmd.CCBuf >= pUg31xx->bq27520Cmd.CCThreshold)
  {
    pUg31xx->bq27520Cmd.CC = pUg31xx->bq27520Cmd.CC + 1;
    pUg31xx->bq27520Cmd.CCBuf = pUg31xx->bq27520Cmd.CCBuf - pUg31xx->bq27520Cmd.CCThreshold;
  }

  *pData = pUg31xx->bq27520Cmd.CC;
  return (UG_SUCCESS);
}

/**
 * @brief TI_Soc
 *
 *  StateOfCharge() command
 *
 * @para  pData address of data
 * @return  GGSTATUS
 */
GGSTATUS TI_Soc(struct ug31xx_data *pUg31xx, _upi_u16_ *pData)
{
  *pData = pUg31xx->batteryInfo.RSOC;
  return (UG_SUCCESS);
}

/**
 * @brief TI_Nic
 *
 *  NormalizedImpedanceCal() command
 *
 * @para  pData address of data
 * @return  GGSTATUS
 */
GGSTATUS TI_Nic(struct ug31xx_data *pUg31xx, _upi_u16_ *pData)
{
  *pData = 0;
  return (UG_SUCCESS);
}

/**
 * @brief TI_Icr
 *
 *  InstantaneousCurrentReading() command
 *
 * @para  pData address of data
 * @return  GGSTATUS
 */
GGSTATUS TI_Icr(struct ug31xx_data *pUg31xx, _upi_u16_ *pData)
{
  *pData = pUg31xx->deviceInfo.current_mA;
  return (UG_SUCCESS);
}

/**
 * @brief TI_Dli
 *
 *  DataLogIndex() command
 *
 * @para  pData address of data
 * @return  GGSTATUS
 */
GGSTATUS TI_Dli(struct ug31xx_data *pUg31xx, _upi_u16_ *pData)
{
  *pData = pUg31xx->bq27520Cmd.Dli;
  return (UG_SUCCESS);
}

/**
 * @brief TI_Dlb
 *
 *  DataLogBuffer() command
 *
 * @para  pData address of data
 * @return  GGSTATUS
 */
GGSTATUS TI_Dlb(struct ug31xx_data *pUg31xx, _upi_u16_ *pData)
{
  *pData = pUg31xx->bq27520Cmd.Dlb;
  return (UG_SUCCESS);
}

/**
 * @brief TI_Itemp
 *
 *  InternalTemperature() command
 *
 * @para  pData address of data
 * @return  GGSTATUS
 */
GGSTATUS TI_Itemp(struct ug31xx_data *pUg31xx, _upi_u16_ *pData)
{
  *pData = pUg31xx->deviceInfo.IT;
  return (UG_SUCCESS);
}

/**
 * @brief TI_Opcfg
 *
 *  OperationConfiguration() command
 *
 * @para  pData address of data
 * @return  GGSTATUS
 */
GGSTATUS TI_Opcfg(struct ug31xx_data *pUg31xx, _upi_u16_ *pData)
{
  *pData = (_upi_u16_)pUg31xx->bq27520Cmd.Opcfg;
  return (UG_SUCCESS);
}

/**
 * @brief TI_Dcap
 *
 *  DesignCapacity() command
 *
 * @para  pData address of data
 * @return  GGSTATUS
 */
GGSTATUS TI_Dcap(struct ug31xx_data *pUg31xx, _upi_u16_ *pData)
{
  *pData = pUg31xx->bq27520Cmd.Dcap;
  return (UG_SUCCESS);
}

typedef GGSTATUS (*TIBq27520FuncPtr)(struct ug31xx_data *pUg31xx, _upi_u16_ *pData);
typedef struct TIBq27520FuncTableST {
  TIBq27520FuncPtr pFunc;
  _upi_u8_ CmdCode;
} TIBq27520FuncTableType;


TIBq27520FuncTableType TI_Command[] = {
  { TI_Cntl,  UG_STD_CMD_CNTL,  },
  { TI_AR,    UG_STD_CMD_AR,    },
  { TI_Artte, UG_STD_CMD_ARTTE, },
  { TI_Temp,  UG_STD_CMD_TEMP,  },
  { TI_Volt,  UG_STD_CMD_VOLT,  },
  { TI_Flags, UG_STD_CMD_FLAGS, },
  { TI_Nac,   UG_STD_CMD_NAC,   },
  { TI_Fac,   UG_STD_CMD_FAC,   },
  { TI_RM,    UG_STD_CMD_RM,    },
  { TI_Fcc,   UG_STD_CMD_FCC,   },
  { TI_AI,    UG_STD_CMD_AI,    },
  { TI_Tte,   UG_STD_CMD_TTE,   },
  { TI_Ttf,   UG_STD_CMD_TTF,   },
  { TI_SI,    UG_STD_CMD_SI,    },
  { TI_Stte,  UG_STD_CMD_STTE,  },
  { TI_Mli,   UG_STD_CMD_MLI,   },
  { TI_Mltte, UG_STD_CMD_MLTTE, },
  { TI_AE,    UG_STD_CMD_AE,    },
  { TI_AP,    UG_STD_CMD_AP,    },
  { TI_Ttecp, UG_STD_CMD_TTECP, },
  { TI_Soh,   UG_STD_CMD_SOH,   },
  { TI_CC,    UG_STD_CMD_CC,    },
  { TI_Soc,   UG_STD_CMD_SOC,   },
  { TI_Nic,   UG_STD_CMD_NIC,   },
  { TI_Icr,   UG_STD_CMD_ICR,   },
  { TI_Dli,   UG_STD_CMD_DLI,   },
  { TI_Dlb,   UG_STD_CMD_DLB,   },
  { TI_Itemp, UG_STD_CMD_ITEMP, },
  { TI_Opcfg, UG_STD_CMD_OPCFG, },
  { TI_Dcap,  UG_EXT_CMD_DCAP,  },
};

/**
 * @brief upiGG_FetchDataCommand
 *
 *  Read the gas gauge status following TI bq27520's interface
 *
 * @para  CommandCode command code
 * @para  pData address of returned data
 * @return  GGSTATUS
 */
GGSTATUS upiGG_FetchDataCommand(char *pObj, _upi_u8_ CommandCode, _upi_u16_ *pData)
{
  GGSTATUS Rtn;
  int TotalCmd;
  GG_DEVICE_INFO DevInfo;
  GG_CAPACITY CapData;
  _upi_u32_ DeltaT;
  int CmdIdx;
  struct ug31xx_data *pUg31xx;

  pUg31xx = (struct ug31xx_data *)pObj;

  Rtn = upiGG_ReadDeviceInfo(pObj, &DevInfo);
  if(Rtn != UG_READ_DEVICE_INFO_SUCCESS)
  {
    return (Rtn);
  }

  DeltaT = pUg31xx->measData.deltaTime;
  if(DeltaT >= 5000000)
  {
    upiGG_ReadCapacity(pObj, &CapData);
  }

  DeltaT = GetTickCount();
  DeltaT = DeltaT - pUg31xx->bq27520Cmd.LastTime;
  pUg31xx->bq27520Cmd.LastTime = GetTickCount();
  pUg31xx->bq27520Cmd.DeltaSec = (_upi_u16_)(DeltaT/1000);

  Rtn = UG_SUCCESS;
  TotalCmd = sizeof(TI_Command)/sizeof(TIBq27520FuncTableType);
  CmdIdx = 0;
  while(1)
  {
    if(TI_Command[CmdIdx].CmdCode == CommandCode)
    {
      Rtn = (*TI_Command[CmdIdx].pFunc)(pUg31xx, pData);
      break;
    }
    
    CmdIdx = CmdIdx + 1;
    if(CmdIdx >= TotalCmd)
    {
      Rtn = UG_TI_CMD_OVERFLOW;
    }
  }

  return (Rtn);
}

/**
 * @brief upiGG_FetchDataParameter
 *
 *  Set the parameter for bq27520 like command
 *
 * @para  data  parameters of GG_FETCH_DATA_PARA_TYPE
 * @return  GGSTATUS
 */
GGSTATUS upiGG_FetchDataParameter(char *pObj, GG_FETCH_DATA_PARA_TYPE data)
{
  GGSTATUS Rtn;
  struct ug31xx_data *pUg31xx;

  pUg31xx = (struct ug31xx_data *)pObj;

  Rtn = UG_SUCCESS;
  
  pUg31xx->bq27520Cmd.FCSet = data.FCSet;
  pUg31xx->bq27520Cmd.FCClear = data.FCClear;
  pUg31xx->bq27520Cmd.Soc1Set = data.Soc1Set;
  pUg31xx->bq27520Cmd.Soc1Clear = data.Soc1Clear;
  pUg31xx->bq27520Cmd.InitSI = data.InitSI;
  pUg31xx->bq27520Cmd.InitMaxLoadCurrent = data.InitMaxLoadCurrent;
  pUg31xx->bq27520Cmd.CCThreshold = data.CCThreshold;
  pUg31xx->bq27520Cmd.Opcfg = data.Opcfg;
  pUg31xx->bq27520Cmd.Dcap = data.Dcap;
  return (Rtn);
}

#endif  ///< end of ENABLE_BQ27520_SW_CMD

/**
 * @brief upiGG_DumpRegister
 *
 *  Dump whole register value
 *
 * @para  pBuf  address of register value buffer
 * @return  data size
 */
_upi_u16_ upiGG_DumpRegister(char *pObj, _upi_u8_ * pBuf)
{
  _upi_u16_ idx;
  struct ug31xx_data *pUg31xx;

  pUg31xx = (struct ug31xx_data *)pObj;

  idx = 0;
  memcpy(pBuf + idx, pUg31xx->otpData.otp1, OTP1_SIZE);
  idx = idx + OTP1_SIZE;
  memcpy(pBuf + idx, pUg31xx->otpData.otp2, OTP2_SIZE);
  idx = idx + OTP2_SIZE;
  memcpy(pBuf + idx, pUg31xx->otpData.otp3, OTP3_SIZE);
  idx = idx + OTP3_SIZE;
  memcpy(pBuf + idx, &pUg31xx->userReg, sizeof(GG_USER_REG));
  idx = idx + sizeof(GG_USER_REG);
  memcpy(pBuf + idx, &pUg31xx->user2Reg, sizeof(GG_USER2_REG));
  idx = idx + sizeof(GG_USER2_REG);
  return (idx);
}

/**
 * @brief upiGG_DumpCellTable
 *
 *  Dump cell NAC table
 *
 * @para  pTable address of cell table
 * @return  _UPI_NULL_
 */
void upiGG_DumpCellTable(char *pObj, CELL_TABLE *pTable)
{  
  struct ug31xx_data *pUg31xx;

  pUg31xx = (struct ug31xx_data *)pObj;
  memcpy(pTable, &pUg31xx->cellTable, sizeof(CELL_TABLE));
}

/**
 * @brief upiGG_UnInitial
 *
 *  Un-initialize uG31xx
 *
 * @para pObj address of memory buffer allocated for uG31xx
 * @return GGSTATUS
 */
GGSTATUS upiGG_UnInitial(char **pObj)
{
	struct ug31xx_data *pUg31xx;
	pUg31xx = (struct ug31xx_data *)(*pObj);

	UG31_LOGE("[%s]***** upiGG_UnInitial() to free memory ***** \n",   __func__);  

	pUg31xx->backupData.backupDataIdx = BACKUP_MAX_LOG_SUSPEND_DATA;
	while(pUg31xx->backupData.backupDataIdx)
	{
		pUg31xx->backupData.backupDataIdx = pUg31xx->backupData.backupDataIdx - 1;
		kfree(pUg31xx->backupData.logData[pUg31xx->backupData.backupDataIdx]);
	}

	#if defined(uG31xx_OS_ANDROID)
    #ifdef  uG31xx_BOOT_LOADER
      free(*pObj);
    #else   ///< else of uG31xx_BOOT_LOADER
  		kfree(*pObj);
    #endif  ///< end of uG31xx_BOOT_LOADER
	#else   ///< else of defined(uG31xx_OS_ANDROID)
		free(*pObj);
	#endif	///< end of defined(uG31xx_OS_ANDROID)
  return (UG_SUCCESS);
}

/**
 * @brief upiGG_DumpParameter 
 *
 *  Dump all parameter setting
 *
 * @para  pTable address of cell parameter
 * @return  _UPI_NULL_
 */
void upiGG_DumpParameter(char *pObj, CELL_PARAMETER *pTable)
{  
  struct ug31xx_data *pUg31xx;

  pUg31xx = (struct ug31xx_data *)pObj;
  memcpy(pTable, &pUg31xx->cellParameter, sizeof(CELL_PARAMETER));
  
 // memcpy(&pTable->CELL_NAC_TABLE, &pUg31xx->realData, sizeof(GG_REAL_DATA));
}

/**
 * @brief upiGG_FetchDebugData
 *
 *  Fetch debug information data
 *
 * @para  pObj  address of memory buffer
 * @para  data  address of GG_FETCH_CAP_DATA_TYPE
 * @return  _UPI_NULL_
 */
void upiGG_FetchDebugData(char *pObj, GG_FETCH_DEBUG_DATA_TYPE *data)
{
  struct ug31xx_data *pUg31xx;

  pUg31xx = (struct ug31xx_data *)pObj;

  data->versionMain = UG31XX_API_MAIN_VERSION;
  data->versionOtp = UG31XX_API_OTP_VERSION;
  data->versionSub = UG31XX_API_SUB_VERSION;

  data->capDelta = pUg31xx->measData.stepCap;
  data->capDsgCharge = pUg31xx->capData.dsgCharge;
  data->capDsgChargeStart = pUg31xx->capData.dsgChargeStart;
  data->capDsgChargeTime = pUg31xx->capData.dsgChargeTime;
  data->capPreDsgCharge = pUg31xx->capData.preDsgCharge;
  data->capSelfHour = pUg31xx->capData.selfDsgHour;
  data->capSelfMilliSec = pUg31xx->capData.selfDsgMilliSec;
  data->capSelfMin = pUg31xx->capData.selfDsgMin;
  data->capSelfSec = pUg31xx->capData.selfDsgSec;
  data->capStatus = pUg31xx->capData.status;
  data->capTableUpdateIdx = pUg31xx->capData.tableUpdateIdx;
  data->capTPTime = pUg31xx->capData.tpTime;

  data->measAdc1ConvertTime = pUg31xx->measData.adc1ConvertTime;
  data->measAdc1Gain = pUg31xx->measData.adc1Gain;
  data->measAdc1Offset = pUg31xx->measData.adc1Offset;
  data->measAdc2Gain = pUg31xx->measData.adc2Gain;
  data->measAdc2Offset = pUg31xx->measData.adc2Offset;
  data->measCCOffset = pUg31xx->measData.ccOffset;
  data->measCharge = pUg31xx->measData.codeCharge;
  data->measCodeBat1 = pUg31xx->measData.codeBat1;
  data->measCodeCurrent = pUg31xx->measData.codeCurrent;
  data->measCodeET = pUg31xx->measData.codeExtTemperature;
  data->measCodeIT = pUg31xx->measData.codeIntTemperature;
  data->measLastCounter = pUg31xx->measData.lastCounter;
  data->measLastDeltaQ = pUg31xx->measData.lastDeltaCap;
  data->measLastTimeTick = pUg31xx->measData.lastTimeTick;
}

/**
 * @brief upiGG_DebugSwitch
 *
 *  Enable/disable debug information to UART
 *
 * @para  Enable  set _UPI_TRUE_ to enable it
 * @return  NULL
 */
void upiGG_DebugSwitch(_upi_bool_ enable)
{
  Ug31DebugEnable = enable;
}

/**
 * @brief	upiGG_ForceTaper
 *
 *	Force taper condition reached
 *
 * @para	pObj		address of memory buffer
 * @para	charger_full	1 if charger detects full
 * @para	dc_in_before	1 if dc in before suspend mode
 * @para	dc_in_now	1 if dc in after suspend mode
 * @return	NULL
 */
void upiGG_ForceTaper(char *pObj, int charger_full, int dc_in_before, int dc_in_now)
{
	struct ug31xx_data *pUg31xx;
	_meas_s16_ now_step_cap;
	_meas_s16_ now_current;
	_meas_u16_ now_voltage;
	_meas_u32_ now_delta_time;
	_upi_u8_ cnt;
	_upi_s32_ tmp32;
	
	pUg31xx = (struct ug31xx_data *)pObj;
	pUg31xx->capData.ggbTable = &pUg31xx->cellTable;
	pUg31xx->capData.ggbParameter = &pUg31xx->cellParameter;
	pUg31xx->capData.measurement = &pUg31xx->measData;
	pUg31xx->capData.status = pUg31xx->capData.status | 0x0100;

	if(dc_in_before == 0)
	{
		UG31_LOGI("[%s]: Enter suspend without charger -> no force taper\n", __func__);
		return;
	}

	tmp32 = dsg_charge_before_suspend - delta_cap_during_suspend;
	if(dc_in_now == 0)
	{
		now_current = pUg31xx->measData.curr;
		now_current = now_current/10;
		now_voltage = pUg31xx->measData.bat1Voltage;
		now_voltage = now_voltage - now_current;

		UG31_LOGI("[%s]: No dc in now (%d)\n", __func__, now_voltage);
		tmp32 = tmp32*100;
		if((pUg31xx->capData.status & 0x0000000c) == 0x00000008)	///< [AT-PM] : Check current status is dischargign ; 10/11/2013
		{
			if((wakeup_predict_rsoc < 95) && 
			   (tmp32 > pUg31xx->cellParameter.ILMD) && 
			   (now_voltage < pUg31xx->cellParameter.TPVoltage))
			{
				UG31_LOGI("[%s]: Predicted RSOC < 95 and charged capacity is not enough (%d < %d) and voltage is low (%d < %d ) in discharging -> no force taper\n",
					__func__, delta_cap_during_suspend, dsg_charge_before_suspend, now_voltage, pUg31xx->cellParameter.TPVoltage);
				return;
			}	
		}
		else
		{
			if((wakeup_predict_rsoc < 98) && (tmp32 > pUg31xx->cellParameter.ILMD))
			{
				UG31_LOGI("[%s]: Predicted RSOC (%d) < 98 and charged capacity is not enough (%d < %d) -> no force taper\n", 
					__func__, wakeup_predict_rsoc, delta_cap_during_suspend, dsg_charge_before_suspend);
				return;	
			}
		}
	}
	else
	{
		UG31_LOGI("[%s]: DC in now\n", __func__);
		if(charger_full == 0)
		{
			UG31_LOGI("[%s]: No charger detect full event -> no force taper\n", __func__);
			return;
		}
		if(wakeup_predict_rsoc < 90)
		{
			UG31_LOGI("[%s]: Predicted RSOC (%d) < 90 -> no force taper\n", __func__, wakeup_predict_rsoc);
			return;
		}
	}

	now_step_cap = pUg31xx->measData.stepCap;
	now_current = pUg31xx->measData.curr;
	now_voltage = pUg31xx->measData.bat1Voltage;
	now_delta_time = pUg31xx->measData.deltaTime;

	UG31_LOGI("[%s]: Force to taper START\n", __func__);

	cnt = 100;
	while(cnt)
	{
		/// [AT-PM] : Enter charging mode ; 08/14/2013
		pUg31xx->measData.stepCap = 0;
		pUg31xx->measData.deltaTime = 0;
		pUg31xx->measData.curr = (_meas_s16_)pUg31xx->cellParameter.TPCurrent - 1;

		UpiReadCapacity(&pUg31xx->capData);
		pUg31xx->capData.tableUpdateDelayCnt = 1;
		pUg31xx->batteryInfo.NAC = (_upi_u16_)pUg31xx->capData.rm;
		pUg31xx->batteryInfo.LMD = (_upi_u16_)pUg31xx->capData.fcc;
		pUg31xx->batteryInfo.RSOC = (_upi_u16_)pUg31xx->capData.rsoc;

		/// [AT-PM] : Force to TP ; 08/14/2013
		pUg31xx->measData.bat1Voltage = (_meas_u16_)pUg31xx->cellParameter.TPVoltage + 1;
		pUg31xx->capData.tpTime = (_cap_u32_)pUg31xx->cellParameter.TPTime + 1;
		pUg31xx->capData.tpTime = pUg31xx->capData.tpTime*1000;

		UpiReadCapacity(&pUg31xx->capData);
		pUg31xx->capData.tableUpdateDelayCnt = 1;
		pUg31xx->capData.rm = pUg31xx->capData.fcc;
		pUg31xx->capData.rsoc = 100;
		pUg31xx->batteryInfo.NAC = (_upi_u16_)pUg31xx->capData.rm;
		pUg31xx->batteryInfo.LMD = (_upi_u16_)pUg31xx->capData.fcc;
		pUg31xx->batteryInfo.RSOC = (_upi_u16_)pUg31xx->capData.rsoc;

		/// [AT-PM] : Recover to initial state ; 08/14/2013
		pUg31xx->measData.curr = now_current;
		pUg31xx->measData.bat1Voltage = now_voltage;
		pUg31xx->measData.deltaTime = now_delta_time;

		UpiReadCapacity(&pUg31xx->capData);
		pUg31xx->capData.tableUpdateDelayCnt = 1;
		pUg31xx->batteryInfo.NAC = (_upi_u16_)pUg31xx->capData.rm;
		pUg31xx->batteryInfo.LMD = (_upi_u16_)pUg31xx->capData.fcc;
		pUg31xx->batteryInfo.RSOC = (_upi_u16_)pUg31xx->capData.rsoc;

		UG31_LOGI("[%s]: Force to taper loop %d -> %d / %d = %d\n", __func__,
			cnt, pUg31xx->batteryInfo.NAC, pUg31xx->batteryInfo.LMD, pUg31xx->batteryInfo.RSOC);
		if(pUg31xx->capData.rsoc >= 100)
		{
			break;
		}

		cnt = cnt - 1;
	}

	pUg31xx->measData.stepCap = now_step_cap;

	UG31_LOGI("[%s]: Force to taper END (%d / %d = %d)\n", __func__, 
		pUg31xx->batteryInfo.NAC, pUg31xx->batteryInfo.LMD, pUg31xx->batteryInfo.RSOC);
}

/// ===========================================
/// End of uG31xx_API.cpp
/// ===========================================

