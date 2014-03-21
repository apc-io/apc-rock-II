#ifndef _GLOBAL_H_
#define _GLOBAL_H_

#define UG31XX_DEBUG_ENABLE
//#define UG31XX_RESET_DATABASE     ///< [AT-PM] : DEFAULT off ; 04/13/2013

#if defined (uG31xx_OS_WINDOWS)

  #pragma pack(push)
  #pragma pack(1)

  #ifdef  UG31XX_DEBUG_ENABLE
    #define DEBUG_LOG 
    #define uG31xx_NAC_LMD_ADJUST_DEBUG_ENABLE
    #define TABLE_BACKUP_DEBUG_ENABLE
    #define DEBUG_LOG_AT_PM
    #define CALIBRATE_ADC_DEBUG_LOG
  #endif  ///< end of UG31XX_DEBUG_ENABLE
  
  #define EXPORTS _declspec(dllexport)

#elif defined(uG31xx_OS_ANDROID)

  #define EXPORTS 

#endif

#define ENABLE_BQ27520_SW_CMD
//#define ENABLE_NTC_CHECK

#endif
