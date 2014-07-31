/// ===========================================
/// uG31xx.h
/// ===========================================

#ifndef _UG31XX_H_
#define _UG31XX_H_

#define  SECURITY			1		//Security Mode enable
#define  NORMAL				0		//Security Mode OFF

#define  HIGH_SPEED			1		//HS Mode
#define  FULL_SPEED			0		//FIL speed

#define  TEN_BIT_ADDR		1		//10-bit address Mode
#define	 SEVEN_BIT_ADDR		0		//7-bit address Mode

#define  I2C_SUCESS			1		//
#define  I2C_FAIL			0

/// ===========================================================================
/// Constant for Calibration
/// ===========================================================================

#define IT_TARGET_CODE25	  (12155)
#define IT_TARGET_CODE80	  (14306)

#define IT_CODE_25		(23171)
#define IT_CODE_80		(27341)

//constant
//define IC type
#define uG3100		0
#define uG3101		1
#define uG3102		2
#define uG3103_2	4
#define uG3103_3	5

//constant
//GPIO1/2 define
#define FUN_GPIO				0x01
#define FUN_ALARM				0x02
#define FUN_PWM					0x04
#define FUN_CBC_EN21			0x08
#define FUN_CBC_EN32			0x10

#define BIT_MACRO(x)		((_upi_u8_)1 << (x))

#define MAX_CRATE_AVAILABLE     (20)
					
#define I2C_ADDRESS    0x70
#define I2C_CLOCK      0x100

//const for CELL_TABLE table
#define TEMPERATURE_NUMS  (4)
#define C_RATE_NUMS				(3)     ///< [AT-PM] : 0.5, 0.2, 0.1, 0.02 ; 12/17/2013
#define OCV_NUMS				  (21)			//include the 0% & 100%
#define SOV_NUMS				  (5)     ///< [AT-PM] : 100%, 70%, 45%, 20%, 0% ; 12/17/2012
#define ET_NUMS           (19)      ///< [AT-PM] : -10, -5, 0, 5, 10, 15, 20, 25, 30, 35, 40, 45, 50, 55, 60, 65, 70, 75, 80 ; 01/25/2013

#define NAC_LMD_ADJUST_CFG_NO_LMD_UPDATE_BETWEEN_10_90_EN     (1<<0)
#define NAC_LMD_ADJUST_CFG_LOCK_AND_SPEED_UP_START_SOV_MASK   (15<<1)
  #define NAC_LMD_ADJUST_CFG_LOCK_AND_SPEED_UP_START_SOV_00   (0<<1)
  #define NAC_LMD_ADJUST_CFG_LOCK_AND_SPEED_UP_START_SOV_01   (1<<1)
  #define NAC_LMD_ADJUST_CFG_LOCK_AND_SPEED_UP_START_SOV_02   (2<<1)
  #define NAC_LMD_ADJUST_CFG_LOCK_AND_SPEED_UP_START_SOV_03   (3<<1)
  #define NAC_LMD_ADJUST_CFG_LOCK_AND_SPEED_UP_START_SOV_04   (4<<1)
  #define NAC_LMD_ADJUST_CFG_LOCK_AND_SPEED_UP_START_SOV_05   (5<<1)
  #define NAC_LMD_ADJUST_CFG_LOCK_AND_SPEED_UP_START_SOV_06   (6<<1)
  #define NAC_LMD_ADJUST_CFG_LOCK_AND_SPEED_UP_START_SOV_07   (7<<1)
  #define NAC_LMD_ADJUST_CFG_LOCK_AND_SPEED_UP_START_SOV_08   (8<<1)
  #define NAC_LMD_ADJUST_CFG_LOCK_AND_SPEED_UP_START_SOV_09   (9<<1)
  #define NAC_LMD_ADJUST_CFG_LOCK_AND_SPEED_UP_START_SOV_10   (10<<1)
  #define NAC_LMD_ADJUST_CFG_LOCK_AND_SPEED_UP_START_SOV_11   (11<<1)
  #define NAC_LMD_ADJUST_CFG_LOCK_AND_SPEED_UP_START_SOV_12   (12<<1)
  #define NAC_LMD_ADJUST_CFG_LOCK_AND_SPEED_UP_START_SOV_13   (13<<1)
  #define NAC_LMD_ADJUST_CFG_LOCK_AND_SPEED_UP_START_SOV_14   (14<<1)
  #define NAC_LMD_ADJUST_CFG_LOCK_AND_SPEED_UP_START_SOV_15   (15<<1)
#define NAC_LMD_ADJUST_CFG_DISPLAY_CC_AS_FCC                  (1<<5)

#if defined(uG31xx_OS_ANDROID)

  #define GGBX_FILE_TAG 0x5F47475F // _GG_
  #define GGBX_FACTORY_FILE_TAG 0x5F67675F // _gg_

#endif  ///< end of defined(uG31xx_OS_ANDROID)

enum C_RATE_TABLE_VALUES {
  C_RATE_TABLE_VALUE_0 = 50,
  C_RATE_TABLE_VALUE_1 = 20,
  C_RATE_TABLE_VALUE_2 = 10,
  C_RATE_TABLE_VALUE_3 = 2
};

enum OCV_TABLE_IDX {
  OCV_TABLE_IDX_CHARGE = 0,
  OCV_TABLE_IDX_STAND_ALONE,
  OCV_TABLE_IDX_100MA,
  OCV_TABLE_IDX_COUNT,
};

#if defined (uG31xx_OS_WINDOWS)

  #define SleepMiniSecond(x)    Sleep(x)

  #ifdef DEBUG_LOG

    #define  _L(X) __L(X)
    #define __L(X) L##X

  #endif

#elif defined (uG31xx_OS_ANDROID)

  #define SleepMiniSecond(x) mdelay(x)

#endif

#define CONST_PERCENTAGE                  (100)
#define TIME_CONVERT_TIME_TO_MSEC         (10)
#define CONST_CONVERSION_COUNT_THRESHOLD  (300)

#endif

/// ===========================================
/// End of uG31xx.h
/// ===========================================

