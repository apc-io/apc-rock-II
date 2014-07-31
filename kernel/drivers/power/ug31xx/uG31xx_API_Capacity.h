/**
 * @filename  uG31xx_API_Capacity.h
 *
 *  Header of uG31xx capacity algorithm
 *
 * @author  AllenTeng <allen_teng@upi-semi.com>
 */

typedef _upi_bool_            _cap_bool_;
typedef unsigned char   _cap_u8_;
typedef signed char     _cap_s8_;
typedef unsigned short  _cap_u16_;
typedef signed short    _cap_s16_;
typedef unsigned long   _cap_u32_;
typedef signed long     _cap_s32_;

#define CAP_FC_RELEASE_RSOC     (99)
#define UG31XX_CAP_VERSION      (36)
#define CAP_ENCRIPT_TABLE_SIZE  (TEMPERATURE_NUMS*C_RATE_NUMS*(SOV_NUMS - 1))

typedef struct CapacityDataST {

  /// [AT-PM] : Data from GGB file ; 01/25/2013
  CELL_PARAMETER *ggbParameter;
  CELL_TABLE *ggbTable;
  
  /// [AT-PM] : Measurement data ; 01/25/2013
  MeasDataType *measurement;

  /// [AT-PM] : Data for table backup ; 01/31/2013
  TableBackupType tableBackup[SOV_NUMS];
  _cap_u8_ encriptTable[CAP_ENCRIPT_TABLE_SIZE];

  /// [AT-PM] : Capacity information ; 01/25/2013
  _cap_u16_ rm;
  _cap_u16_ fcc;
  _cap_u16_ fccBackup;
  _cap_u8_ rsoc;

  /// [AT-PM] : Capacity operation variables ; 01/25/2013
  _cap_u32_ status;

  _cap_u32_ selfDsgMilliSec;
  _cap_u8_ selfDsgSec;
  _cap_u8_ selfDsgMin;
  _cap_u8_ selfDsgHour;
  _cap_u8_ selfDsgResidual;

  _cap_u8_ lastRsoc;

  _cap_u32_ tpTime;

  _cap_s32_ dsgCharge;
  _cap_s32_ dsgChargeStart;
  _cap_u32_ dsgChargeTime;
  _cap_s32_ preDsgCharge;

  _cap_u8_ tableUpdateIdx;
  _cap_u32_ tableUpdateDisqTime;
  _cap_u8_ tableUpdateDelayCnt;

  _cap_s8_ parseRMResidual;

  _cap_s16_ reverseCap;
  _cap_u8_ avgCRate;  

  _cap_s16_ ccRecord[SOV_NUMS];
#if defined(uG31xx_OS_ANDROID)
  } __attribute__ ((packed)) CapacityDataType;
#else   ///< else of defined(uG31xx_OS_ANDROID)	      
  } CapacityDataType;
#endif  ///< end of defined(uG31xx_OS_ANDROID)

/**
 * @brief UpiInitCapacity
 *
 *  Initial capacity algorithm
 *
 * @para  data  address of CapacityDataType
 * @return  _UPI_NULL_
 */
extern void UpiInitCapacity(CapacityDataType *data);

/**
 * @brief UpiReadCapacity
 *
 *  Read capacity information
 *
 * @para  data  address of CapacityDataType
 * @return  _UPI_NULL_
 */
extern void UpiReadCapacity(CapacityDataType *data);

/**
 * @brief UpiTableCapacity
 *
 *  Look up capacity from table
 *
 * @para  data  address of CapacityDataType
 * @return  _UPI_NULL_
 */
extern void UpiTableCapacity(CapacityDataType *data);

/**
 * @brief UpiInitDsgCharge
 *
 *  Initialize data->dsgCharge value
 *
 * @para  data  address of CapacityDataType
 * @return  _UPI_NULL_
 */
extern void UpiInitDsgCharge(CapacityDataType *data);

/**
 * @brief UpiInitNacTable
 *
 *  Initialize NAC table
 *
 * @para  data  address of CapacityDataType
 * @return  _UPI_NULL_
 */
extern void UpiInitNacTable(CapacityDataType *data);

/**
 * @brief UpiSaveNacTable
 *
 *  Save NAC table to IC
 *
 * @para  data  address of CapacityDataType
 * @return  _UPI_NULL_
 */
extern void UpiSaveNacTable(CapacityDataType *data);

/**
 * @brief CalculateRsoc
 *
 *  RSOC = RM x 100 / FCC
 *
 * @para  rm  remaining capacity
 * @para  fcc full charged capacity
 * @return  relative state of charge
 */
extern _cap_u8_ CalculateRsoc(_cap_u16_ rm, _cap_u16_ fcc);
