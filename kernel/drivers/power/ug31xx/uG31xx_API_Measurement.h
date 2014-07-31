/**
 * @filename  uG31xx_API_Measurement.h
 *
 *  Header for uG31xx measurement API
 *
 * @author  AllenTeng <allen_teng@upi-semi.com>
 */

typedef signed char       _meas_s8_;
typedef unsigned char     _meas_u8_;
typedef signed short      _meas_s16_;
typedef unsigned short    _meas_u16_;
typedef signed long       _meas_s32_;
typedef unsigned long     _meas_u32_;
typedef signed long long  _meas_s64_;

#define UG31XX_MEAS_VERSION     (9)

#define BOARD_FACTOR_CONST          (1000)
#define BOARD_FACTOR_VOLTAGE_GAIN   (1000)  ///< [AT-PM] : VBat1 board factor - gain ; 01/25/2013
#define BOARD_FACTOR_VOLTAGE_OFFSET (0)     ///< [AT-PM] : VBat1 board factor - offset ; 01/25/2013
#define BOARD_FACTOR_CURR_GAIN      (1014)  ///< [AT-PM] : Current board factor - gain ; 01/25/2013
#define BOARD_FACTOR_CURR_OFFSET    (-7)    ///< [AT-PM] : Current board factor - offset ; 01/25/2013
#define BOARD_FACTOR_INTT_OFFSET    (-23)   ///< [AT-PM] : Internal Temperature board factor - offset ; 01/25/2013
#define BOARD_FACTOR_EXTT_OFFSET    (13)    ///< [AT-PM] : External Temperature board factor - offset ; 01/25/2013

#define CALIBRATION_FACTOR_CONST    (1000)

#define COULOMB_COUNTER_RESET_THRESHOLD_COUNTER     (10000)
#define COULOMB_COUNTER_RESET_THRESHOLD_CHARGE_CHG  (30000)
#define COULOMB_COUNTER_RESET_THREDHOLD_CHARGE_DSG  (-30000)

typedef enum _MEAS_RTN_CODE {
  MEAS_RTN_PASS = 0,
  MEAS_RTN_BATTERY_REMOVED,
  MEAS_RTN_ADC_ABNORMAL,
  MEAS_RTN_NTC_SHORT,
} MEAS_RTN_CODE;

typedef struct MeasDataST {

  /// [AT-PM] : System data ; 04/08/2013
  SystemDataType *sysData;
  
  /// [AT-PM] : OTP data ; 01/23/2013
  OtpDataType *otp;
  
  /// [AT-PM] : Physical value ; 01/23/2013
  _meas_u16_ bat1Voltage;
  _meas_s16_ curr;
  _meas_s16_ intTemperature;
  _meas_s16_ extTemperature;
  _meas_s16_ deltaCap;
  _meas_s16_ stepCap;
  _meas_u32_ deltaTime;


  /// [AT-PM] : ADC code ; 01/23/2013
  _meas_u16_ codeBat1;
  _meas_s16_ codeCurrent;
  _meas_u16_ codeIntTemperature;
  _meas_u16_ codeExtTemperature;
  _meas_s32_ codeCharge;
  _meas_s16_ rawCodeCharge;

  /// [AT-PM] : Coulomb counter offset ; 01/23/2013
  _meas_s16_ ccOffset;
  _meas_s16_ ccOffsetAdj;
  
  /// [AT-PM] : ADC1 characteristic ; 01/23/2013
  _meas_u16_ adc1ConvertTime;
  _meas_s32_ adc1Gain;
  _meas_s32_ adc1GainSlope;
  _meas_s32_ adc1GainFactorB;
  _meas_s32_ adc1Offset;
  _meas_s32_ adc1OffsetSlope;
  _meas_s32_ adc1OffsetFactorO;

  /// [AT-PM] : ADC2 characteristic ; 01/23/2013
  _meas_s32_ adc2Gain;
  _meas_s32_ adc2GainSlope;
  _meas_s32_ adc2GainFactorB;
  _meas_s32_ adc2Offset;
  _meas_s32_ adc2OffsetSlope;
  _meas_s32_ adc2OffsetFactorO;

  /// [AT-PM] : Previous information ; 01/25/2013
  _meas_u16_ lastCounter;
  _meas_u32_ lastTimeTick;
  _meas_s16_ lastDeltaCap;
  
#if defined(uG31xx_OS_ANDROID)
} __attribute__ ((packed)) MeasDataType;
#else   ///< else of defined(uG31xx_OS_ANDROID)	      
} MeasDataType;
#endif  ///< end of defined(uG31xx_OS_ANDROID)

/**
 * @brief UpiResetCoulombCounter
 *
 *  Reset coulomb counter
 *
 * @para  data  address of MeasDataType
 * @return  _UPI_NULL_
 */
extern void UpiResetCoulombCounter(MeasDataType *data);

/**
 * @brief UpiMeasurement
 *
 *  Measurement routine
 *
 * @para  data  address of MeasDataType
 * @return  MEAS_RTN_CODE
 */
extern MEAS_RTN_CODE UpiMeasurement(MeasDataType *data);

/**
 * @brief UpiMeasAlarmThreshold
 *
 *  Get alarm threshold
 *
 * @para  data  address of MeasDataType
 * @return  MEAS_RTN_CODE
 */
extern MEAS_RTN_CODE UpiMeasAlarmThreshold(MeasDataType *data);

