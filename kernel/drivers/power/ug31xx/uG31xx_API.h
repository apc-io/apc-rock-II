/// ===========================================
/// uG31xx_API.h
/// ===========================================

#ifndef _UG31XXAPI_H_
#define _UG31XXAPI_H_

#include "uG31xx_Platform.h"

#if defined (uG31xx_OS_WINDOWS)

  #include <windows.h>

#endif

#include "global.h"
#include "uG31xx.h"

#if defined (uG31xx_OS_WINDOWS)

  #include "../../uG31xx_I2C_DLL/uG3100Dll/uG31xx_I2C.h"
  #include <assert.h>
  
  #ifdef DEBUG_LOG
  
    #include "wDebug.h"
    
  #endif
  
#elif defined (uG31xx_OS_ANDROID)

  #ifdef  uG31xx_BOOT_LOADER

    #include <sys/types.h>
    #include <platform/timer.h>

    #include "ug31xx_boot_i2c.h"

  #else   ///< else of uG31xx_BOOT_LOADER

    #include <linux/module.h>
    #include <linux/slab.h>
    #include <linux/delay.h>
    #include <linux/time.h>
    #include <asm/div64.h>
    #include <linux/jiffies.h>
    #include <linux/types.h>
    #include <linux/fs.h>
    #include <asm/uaccess.h>

    #include "ug31xx_i2c.h"

  #endif  ///< end of uG31xx_BOOT_LOADER

#endif

#include "typeDefine.h"
#include "uG31xx_Reg_Def.h"
#include "uG31xx_API_Otp.h"
#include "uG31xx_API_System.h"
#include "uG31xx_API_Measurement.h"
#include "uG31xx_API_Capacity.h"
#include "uG31xx_API_Backup.h"

#define	UG31XX_API_RELEASE_VERSION	(20131108)

#define UG31XX_API_MAIN_VERSION     (13)
#define UG31XX_API_OTP_VERSION      ((UG31XX_OTP_VERSION_MAIN << 16) | UG31XX_OTP_VERSION_SUB)
#define UG31XX_API_SUB_VERSION      ((UG31XX_CAP_VERSION << 16) | (UG31XX_MEAS_VERSION << 8) | UG31XX_SYSTEM_VERSION)

#define UG31XX_I2C_HIGH_SPEED_MODE    (_UPI_FALSE_)
#define UG31XX_I2C_TEM_BITS_MODE      (_UPI_FALSE_)

#define UG31XX_MAX_TEMPERATURE_BEFORE_READY     (395)
#define UG31XX_MIN_TEMPERATURE_BEFORE_READY     (105)

#ifdef  UG31XX_DEBUG_ENABLE

  #if defined(uG31xx_OS_ANDROID)

    #ifdef  uG31xx_BOOT_LOADER
          
      #define __func__        

      #define UG31_LOGE(...)
      #define UG31_LOGI(...)
      #define UG31_LOGD(...)
      #define UG31_LOGV(...)

      extern int ug31_dprintk(int level, const char *fmt, ...);
      
    #else   ///< else of uG31xx_BOOT_LOADER
    
      #define LOG_NORMAL      (0)
      #define LOG_ERROR       (1)
      #define LOG_DATA        (2)
      #define LOG_VERBOSE     (3)
      
      #define UG31_TAG "UG31"
      #define UG31_LOGE(...)  ug31_printk(LOG_ERROR, "<"UG31_TAG"/E>"  __VA_ARGS__);
      #define UG31_LOGI(...)  ug31_printk(LOG_NORMAL, "<"UG31_TAG"/I>"  __VA_ARGS__);
      #define UG31_LOGD(...)  ug31_printk(LOG_DATA, "<"UG31_TAG"/D>"  __VA_ARGS__);
      #define UG31_LOGV(...)  ug31_printk(LOG_VERBOSE, "<"UG31_TAG"/V>"  __VA_ARGS__);
      
      extern int ug31_printk(int level, const char *fmt, ...); 

    #endif  ///< end of uG31xx_BOOT_LOADER
    
  #endif  ///< end of defined(uG31xx_OS_ANDROID)

  #if defined (uG31xx_OS_WINDOWS)

    #define DEBUG_FILE      (_T("uG3105"))
    #define __func__        (_T(__FUNCTION__))

    #define UG31_LOGE(...)  wDebug::LOGE(DEBUG_FILE, 0, _T(__VA_ARGS__));
    #define UG31_LOGI(...)  wDebug::LOGE(DEBUG_FILE, 0, _T(__VA_ARGS__));
    #define UG31_LOGD(...)  wDebug::LOGE(DEBUG_FILE, 0, _T(__VA_ARGS__));
    #define UG31_LOGV(...)  wDebug::LOGE(DEBUG_FILE, 0, _T(__VA_ARGS__));

  #endif  ///< end of defined (uG31xx_OS_WINDOWS)
  
#else   ///< else of UG31XX_DEBUG_ENABLE

  #define UG31_LOGE(...)
  #define UG31_LOGI(...)
  #define UG31_LOGD(...)
  #define UG31_LOGV(...)

#endif  ///< end of UG31XX_DEBUG_ENABLE

#if defined(uG31xx_OS_ANDROID)

  extern _upi_u32_ GetTickCount(void);
  extern _upi_u32_ GetSysTickCount(void);
  
#endif

/* data struct */
typedef enum _GGSTATUS{
  UG_SUCCESS                    = 0x00,
  UG_FAIL		                    = 0x01,
  UG_NOT_DEF                    = 0x02,
  UG_INIT_OCV_FAIL	            = 0x03,
  UG_READ_GGB_FAIL              = 0x04,
  UG_ACTIVE_FAIL                = 0x05,
  UG_INIT_SUCCESS               = 0x06,
  UG_OTP_ISEMPTY                = 0x07,
  UG_OTP_PRODUCT_DISMATCH       = 0x08,

  UG_I2C_INIT_FAIL              = 0x10,
  UG_I2C_READ_SUCCESS           = 0x11,
  UG_I2C_READ_FAIL              = 0x12,
  UG_I2C_WRITE_SUCCESS          = 0x13,
  UG_I2C_WRITE_FAIL             = 0x14,

  UG_READ_REG_SUCCESS           = 0x20,
  UG_READ_REG_FAIL              = 0x21,

  UG_READ_DEVICE_INFO_SUCCESS   = 0x22,
  UG_READ_DEVICE_INFO_FAIL      = 0x23,
  UG_READ_DEVICE_ALARM_SUCCESS  = 0x24,
  UG_READ_DEVICE_ALARM_FAIL     = 0x25,
  UG_READ_DEVICE_RID_SUCCESS	  = 0x26,
  UG_READ_DEVICE_RID_FAIL		    = 0x27,
  UG_READ_ADC_FAIL						  = 0x28,			//new add for filter ADC Error Code

  UG_TI_CMD_OVERFLOW            = 0x30,

  UG_MEAS_FAIL                  = 0x40,
  UG_MEAS_FAIL_BATTERY_REMOVED  = 0x41,
  UG_MEAS_FAIL_ADC_ABNORMAL     = 0x42,
  UG_MEAS_FAIL_NTC_SHORT        = 0x43,

  UG_CAP_DATA_READY             = 0x50,
  UG_CAP_DATA_NOT_READY         = 0x51,
}GGSTATUS;

/*
    GGSTATUS upiGG_Initial
    Description: Initial and active uG31xx function
    Input: .GGB(gas gauge battery) setting filename, need include complete path
	Output: UG_INIT_SUCCESS -> initial uG31xx success
	        UG_READ_GGB_FAIL -> read GGB file fail
			UG_INIT_I2C_FAIL -> initial I2C to open HID fail
			UG_ACTIVE_FAIL -> active uG31xx fail
*/
#if defined (uG31xx_OS_WINDOWS)

  EXPORTS GGSTATUS upiGG_Initial(char **pObj, const wchar_t* GGBFilename, const wchar_t* OtpFileName, const wchar_t* BackupFileName, char reset, unsigned char MaxETFixCnt);

#endif

#if defined(uG31xx_OS_ANDROID)

  GGSTATUS upiGG_Initial(char **pObj, GGBX_FILE_HEADER *pGGBXBuf, char reset, unsigned char MaxETFixCnt);

#endif

/*
    GGSTATUS upiGG_CountInitQmax
    Description:
    Input: None
	Output: None
*/
//EXPORTS void upiGG_CountInitQmax(void);

/*
    GGSTATUS upiGG_ReadDevieRegister
    Description: Read GG_USER_REG from device to global variable and output
    Input: Pointer of sturct GG_USER_REG 
	Output: UG_READ_REG_SUCCESS -> read success
	        UG_READ_REG_FAIL -> read fail
*/
EXPORTS GGSTATUS upiGG_ReadAllRegister(char *pObj,GG_USER_REG* pExtUserReg, GG_USER2_REG* pExtUserReg2);

/*
    GGSTATUS upiGG_ReadDeviceInfo
    Description: Read GG_USER_REG from device and calculate GG_DEVICE_INFO, then write to global variable and output
    Input: Pointer of struct GG_DEVICE_INFO
	Output: UG_READ_DEVICE_INFO_SUCCESS -> calculate derive information sucess
	        UG_READ_DEVICE_INFO_FAIL -> calculate derive information fail
*/
EXPORTS GGSTATUS upiGG_ReadDeviceInfo(char *pObj,GG_DEVICE_INFO* pExtDeviceInfo);

/* GGSTATUS upiGG_ReadCapacity
    Description:
    Input:
	Output: None
*/
EXPORTS void upiGG_ReadCapacity(char *pObj,GG_CAPACITY *pExtCapacity);

/**
 * @brief upiGG_GetAlarmStatus
 *
 *  Get alarm status
 *
 * @para  pAlarmStatus  address of alarm status
 * @return  UG_READ_DEVICE_ALARM_SUCCESS if success
 */
EXPORTS GGSTATUS upiGG_GetAlarmStatus(char *pObj, _upi_u8_ *pAlarmStatus);

/*
new add function for System suspend & wakeup

*/
EXPORTS GGSTATUS upiGG_PreSuspend(char *pObj);
EXPORTS GGSTATUS upiGG_Wakeup(char *pObj, int dc_in_before);

/**
 * @brief upiGG_DumpRegister
 *
 *  Dump whole register value
 *
 * @para  pBuf  address of register value buffer
 * @return  data size
 */
EXPORTS _upi_u16_ upiGG_DumpRegister(char *pObj, _upi_u8_ *pBuf);

/**
 * @brief upiGG_DumpCellTable
 *
 *  Dump cell NAC table
 *
 * @para  pTable address of cell table
 * @return  _UPI_NULL_
 */
EXPORTS void upiGG_DumpCellTable(char *pObj, CELL_TABLE *pTable);
EXPORTS GGSTATUS upiGG_UnInitial(char **pObj);
EXPORTS void upiGG_DumpParameter(char *pObj, CELL_PARAMETER *pTable);

#define upiGG_PrePowerOff (upiGG_PreSuspend)

#ifdef ENABLE_BQ27520_SW_CMD

/**
 * @brief upiGG_AccessMeasurementParameter
 *
 *  Access measurement parameter
 *
 * @para  read  set _UPI_TRUE_ to read data from API
 * @para  pMeasPara pointer of GG_MEAS_PARA_TYPE
 * @return  GGSTATUS
 */
EXPORTS GGSTATUS upiGG_AccessMeasurementParameter(char *pObj, _upi_bool_ read, GG_MEAS_PARA_TYPE *pMeasPara);

#define UG_STD_CMD_CNTL     (0x00)
  #define UG_STD_CMD_CNTL_CONTROL_STATUS    (0x0000)
    #define UG_STD_CMD_CNTL_CONTROL_STATUS_DLOGEN     (1<<15)
    #define UG_STD_CMD_CNTL_CONTROL_STATUS_FAS        (1<<14)
    #define UG_STD_CMD_CNTL_CONTROL_STATUS_SS         (1<<13)
    #define UG_STD_CMD_CNTL_CONTROL_STATUS_CSV        (1<<12)
    #define UG_STD_CMD_CNTL_CONTROL_STATUS_CCA        (1<<11)
    #define UG_STD_CMD_CNTL_CONTROL_STATUS_BCA        (1<<10)
    #define UG_STD_CMD_CNTL_CONTROL_STATUS_OCVCMDCOMP (1<<9)
    #define UG_STD_CMD_CNTL_CONTROL_STATUS_OCVFAIL    (1<<8)
    #define UG_STD_CMD_CNTL_CONTROL_STATUS_INITCOMP   (1<<7)
    #define UG_STD_CMD_CNTL_CONTROL_STATUS_HIBERNATE  (1<<6)
    #define UG_STD_CMD_CNTL_CONTROL_STATUS_SNOOZE     (1<<5)
    #define UG_STD_CMD_CNTL_CONTROL_STATUS_SLEEP      (1<<4)
    #define UG_STD_CMD_CNTL_CONTROL_STATUS_LDMD       (1<<3)
    #define UG_STD_CMD_CNTL_CONTROL_STATUS_RUP_DIS    (1<<2)
    #define UG_STD_CMD_CNTL_CONTROL_STATUS_VOK        (1<<1)
    #define UG_STD_CMD_CNTL_CONTROL_STATUS_QEN        (1<<0)
  #define UG_STD_CMD_CNTL_DEVICE_TYPE       (0x0001)
  #define UG_STD_CMD_CNTL_FW_VERSION        (0x0002)
  #define UG_STD_CMD_CNTL_PREV_MACWRITE     (0x0007)
  #define UG_STD_CMD_CNTL_CHEM_ID           (0x0008)
  #define UG_STD_CMD_CNTL_OCV_CMD           (0x000C)      ///< [AT-PM] : Not implemented ; 10/11/2012
  #define UG_STD_CMD_CNTL_BAT_INSERT        (0x000D)
  #define UG_STD_CMD_CNTL_BAT_REMOVE        (0x000E)
  #define UG_STD_CMD_CNTL_SET_HIBERNATE     (0x0011)
  #define UG_STD_CMD_CNTL_CLEAR_HIBERNATE   (0x0012)
  #define UG_STD_CMD_CNTL_SET_SLEEP_PLUS    (0x0013)
  #define UG_STD_CMD_CNTL_CLEAR_SLEEP_PLUS  (0x0014)
  #define UG_STD_CMD_CNTL_FACTORY_RESTORE   (0x0015)
  #define UG_STD_CMD_CNTL_ENABLE_DLOG       (0x0018)
  #define UG_STD_CMD_CNTL_DISABLE_DLOG      (0x0019)
  #define UG_STD_CMD_CNTL_DF_VERSION        (0x001F)
  #define UG_STD_CMD_CNTL_SEALED            (0x0020)
  #define UG_STD_CMD_CNTL_RESET             (0x0041)
#define UG_STD_CMD_AR       (0x02)
#define UG_STD_CMD_ARTTE    (0x04)
#define UG_STD_CMD_TEMP     (0x06)
#define UG_STD_CMD_VOLT     (0x08)
#define UG_STD_CMD_FLAGS    (0x0A)
  #define UG_STD_CMD_FLAGS_OTC            (1<<15)
  #define UG_STD_CMD_FLAGS_OTD            (1<<14)
  #define UG_STD_CMD_FLAGS_RSVD13         (1<<13)
  #define UG_STD_CMD_FLAGS_RSVD12         (1<<12)
  #define UG_STD_CMD_FLAGS_CHG_INH        (1<<11)
  #define UG_STD_CMD_FLAGS_XCHG           (1<<10)
  #define UG_STD_CMD_FLAGS_FC             (1<<9)
  #define UG_STD_CMD_FLAGS_CHG            (1<<8)
  #define UG_STD_CMD_FLAGS_RSVD7          (1<<7)
  #define UG_STD_CMD_FLAGS_RSVD6          (1<<6)
  #define UG_STD_CMD_FLAGS_OCV_GD         (1<<5)
  #define UG_STD_CMD_FLAGS_WAIT_ID        (1<<4)
  #define UG_STD_CMD_FLAGS_BAT_DET        (1<<3)
  #define UG_STD_CMD_FLAGS_SOC1           (1<<2)
  #define UG_STD_CMD_FLAGS_SYSDOWN        (1<<1)
  #define UG_STD_CMD_FLAGS_DSG            (1<<0)
#define UG_STD_CMD_NAC      (0x0C)
#define UG_STD_CMD_FAC      (0x0E)
#define UG_STD_CMD_RM       (0x10)
#define UG_STD_CMD_FCC      (0x12)
#define UG_STD_CMD_AI       (0x14)
#define UG_STD_CMD_TTE      (0x16)
#define UG_STD_CMD_TTF      (0x18)
#define UG_STD_CMD_SI       (0x1A)
#define UG_STD_CMD_STTE     (0x1C)
#define UG_STD_CMD_MLI      (0x1E)
#define UG_STD_CMD_MLTTE    (0x20)
#define UG_STD_CMD_AE       (0x22)
#define UG_STD_CMD_AP       (0x24)
#define UG_STD_CMD_TTECP    (0x26)
#define UG_STD_CMD_SOH      (0x28)
  #define UG_STD_CMD_SOH_VALUE_MASK   (0x00FF)
  #define UG_STD_CMD_SOH_STATUS_MASK  (0xFF00)
    #define UG_STD_CMD_SOH_STATUS_NOT_VALID     (0x0000)
    #define UG_STD_CMD_SOH_STATUS_INSTANT_READY (0x0100)
    #define UG_STD_CMD_SOH_STATUS_INITIAL_READY (0x0200)
    #define UG_STD_CMD_SOH_STATUS_READY         (0x0300)
#define UG_STD_CMD_CC       (0x2A)
#define UG_STD_CMD_SOC      (0x2C)
#define UG_STD_CMD_NIC      (0x2E)      ///< [AT-PM] : Not implemented ; 10/11/2012
#define UG_STD_CMD_ICR      (0x30)
#define UG_STD_CMD_DLI      (0x32)
#define UG_STD_CMD_DLB      (0x34)
#define UG_STD_CMD_ITEMP    (0x36)
#define UG_STD_CMD_OPCFG    (0x3A)
  #define UG_STD_CMD_OPCFG_RESCAP     (1<<31)
  #define UG_STD_CMD_OPCFG_BATG_OVR   (1<<30)
  #define UG_STD_CMD_OPCFG_INT_BERM   (1<<29)
  #define UG_STD_CMD_OPCFG_PFC_CFG1   (1<<28)
  #define UG_STD_CMD_OPCFG_PFC_CFG0   (1<<27)
  #define UG_STD_CMD_OPCFG_IWAKE      (1<<26)
  #define UG_STD_CMD_OPCFG_RSNS1      (1<<25)
  #define UG_STD_CMD_OPCFG_RSNS0      (1<<24)
  #define UG_STD_CMD_OPCFG_INT_FOCV   (1<<23)
  #define UG_STD_CMD_OPCFG_IDSELEN    (1<<22)
  #define UG_STD_CMD_OPCFG_SLEEP      (1<<21)
  #define UG_STD_CMD_OPCFG_RMFCC      (1<<20)
  #define UG_STD_CMD_OPCFG_SOCI_POL   (1<<19)
  #define UG_STD_CMD_OPCFG_BATG_POL   (1<<18)
  #define UG_STD_CMD_OPCFG_BATL_POL   (1<<17)
  #define UG_STD_CMD_OPCFG_TEMPS      (1<<16)
  #define UG_STD_CMD_OPCFG_WRTEMP     (1<<15)
  #define UG_STD_CMD_OPCFG_BIE        (1<<14)
  #define UG_STD_CMD_OPCFG_BL_INT     (1<<13)
  #define UG_STD_CMD_OPCFG_GNDSEL     (1<<12)
  #define UG_STD_CMD_OPCFG_FCE        (1<<11)
  #define UG_STD_CMD_OPCFG_DFWRINDBL  (1<<10)
  #define UG_STD_CMD_OPCFG_RFACTSTEP  (1<<9)
  #define UG_STD_CMD_OPCFG_INDFACRES  (1<<8)
  #define UG_STD_CMD_OPCFG_BATGSPUEN  (1<<7)
  #define UG_STD_CMD_OPCFG_BATGWPUEN  (1<<6)
  #define UG_STD_CMD_OPCFG_BATLSPUEN  (1<<5)
  #define UG_STD_CMD_OPCFG_BATLWSPUEN (1<<4)
  #define UG_STD_CMD_OPCFG_RSVD3      (1<<3)
  #define UG_STD_CMD_OPCFG_SLPWKCHG   (1<<2)
  #define UG_STD_CMD_OPCFG_DELTAVOPT1 (1<<1)
  #define UG_STD_CMD_OPCFG_DELTAVOPT0 (1<<0)
#define UG_EXT_CMD_DCAP     (0x3C)

/**
 * @brief upiGG_FetchDataCommand
 *
 *  Fetch bq27520 like command
 *
 * @para  read  set _UPI_TRUE_ to read data from API
 * @para  pMeasPara pointer of GG_MEAS_PARA_TYPE
 * @return  GGSTATUS
 */
EXPORTS GGSTATUS upiGG_FetchDataCommand(char *pObj, _upi_u8_ CommandCode, _upi_u16_ *pData);

typedef struct GG_FETCH_DATA_PARA_ST {
  _upi_s8_ FCSet;
  _upi_s8_ FCClear;
  _upi_u8_ Soc1Set;
  _upi_u8_ Soc1Clear;
  _upi_s8_ InitSI;
  _upi_s16_ InitMaxLoadCurrent;
  _upi_u16_ CCThreshold;
  _upi_u32_ Opcfg;
  _upi_u16_ Dcap;
#if defined(uG31xx_OS_ANDROID)
}__attribute__((packed)) GG_FETCH_DATA_PARA_TYPE;
#else   ///< else of defined(uG31xx_OS_ANDROID)
} GG_FETCH_DATA_PARA_TYPE;
#endif  ///< end of defined(uG31xx_OS_ANDROID)

/**
 * @brief upiGG_FetchDataParameter
 *
 *  Set the parameter for bq27520 like command
 *
 * @para  data  parameters of GG_FETCH_DATA_PARA_TYPE
 * @return  GGSTATUS
 */
EXPORTS GGSTATUS upiGG_FetchDataParameter(char *pObj, GG_FETCH_DATA_PARA_TYPE data);

#endif     //endif ENABLE_BQ27520_SW_CMD

typedef struct GG_FETCH_DEBUG_DATA_ST {
  /// [AT-PM] : Driver version ; 01/30/2013
  int versionMain;
  int versionOtp;
  int versionSub;

  /// [AT-PM] : Capacity related ; 01/30/2013
  int capStatus;
  int capSelfHour;
  int capSelfMin;
  int capSelfSec;
  int capSelfMilliSec;
  int capTPTime;
  int capDelta;
  int capDsgCharge;
  int capDsgChargeStart;
  int capDsgChargeTime;
  int capPreDsgCharge;
  int capTableUpdateIdx;

  /// [AT-PM] : Measurement related ; 01/30/2013
  int measCodeBat1;
  int measCodeCurrent;
  int measCodeIT;
  int measCodeET;
  int measCharge;
  int measCCOffset;
  int measAdc1ConvertTime;
  int measAdc1Gain;
  int measAdc1Offset;
  int measAdc2Gain;
  int measAdc2Offset;
  int measLastCounter;
  int measLastTimeTick;
  int measLastDeltaQ;
  
#if defined(uG31xx_OS_ANDROID)
  }__attribute__((packed)) GG_FETCH_DEBUG_DATA_TYPE;
#else   ///< else of defined(uG31xx_OS_ANDROID)
  } GG_FETCH_DEBUG_DATA_TYPE;
#endif  ///< end of defined(uG31xx_OS_ANDROID)

/**
 * @brief upiGG_FetchDebugData
 *
 *  Fetch debug information data
 *
 * @para  pObj  address of memory buffer
 * @para  data  address of GG_FETCH_CAP_DATA_TYPE
 * @return  _UPI_NULL_
 */
EXPORTS void upiGG_FetchDebugData(char *pObj, GG_FETCH_DEBUG_DATA_TYPE *data);

/**
 * @brief upiGG_DebugSwitch
 *
 *  Enable/disable debug information to UART
 *
 * @para  Enable  set _UPI_TRUE_ to enable it
 * @return  NULL
 */
EXPORTS void upiGG_DebugSwitch(_upi_bool_ enable);

#define	UPI_CHECK_BACKUP_FILE_FAIL	(0)
#define	UPI_CHECK_BACKUP_FILE_EXIST	(1)
#define	UPI_CHECK_BACKUP_FILE_MISMATCH	(2)

EXPORTS _upi_u8_ upiGG_CheckBackupFile(char *pObj);

EXPORTS void upiGG_ForceTaper(char *pObj, int charger_full, int dc_in_before, int dc_in_now);

#endif  ///< end of _UG31XXAPI_H_

/// ===========================================
/// End of uG31xx_API.h
/// ===========================================

