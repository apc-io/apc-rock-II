/**
 * @filename  uG31xx_API_Backup.h
 *
 *  Header of uG31xx_API_Backup.cpp
 *
 * @author  AllenTeng <allen_teng@upi-semi.com>
 */

#define BACKUP_BOOL_TRUE      (1)
#define BACKUP_BOOL_FALSE     (0)

#define BACKUP_FILE_PATH              ("/sdcard/upi_gg")

enum BACKUP_FILE_STS {
  BACKUP_FILE_STS_CHECKING = 0,
  BACKUP_FILE_STS_NOT_EXIST,
  BACKUP_FILE_STS_EXIST,
  BACKUP_FILE_STS_COMPARE,
};

typedef unsigned char _backup_bool_;
typedef unsigned char _backup_u8_;
typedef unsigned long _backup_u32_;

typedef struct BackupSuspendDataST {
	CapacityDataType beforeCapData;
	MeasDataType beforeMeasData;

	CapacityDataType afterCapData;
	MeasDataType afterMeasData;
} __attribute__ ((packed)) BackupSuspendDataType;

#define	BACKUP_MAX_LOG_SUSPEND_DATA	(8)

typedef struct BackupDataST {

  CapacityDataType *capData;
  SystemDataType *sysData;
  MeasDataType *measData;

  _backup_bool_ icDataAvailable;
  _backup_u8_ backupFileSts;
  _backup_u32_ backupFileVer;
  _backup_u32_ targetFileVer;
  
  _backup_u8_ backupDataIdx;
  BackupSuspendDataType *logData[BACKUP_MAX_LOG_SUSPEND_DATA];

  #if defined (uG31xx_OS_WINDOWS)
    const wchar_t* backupFileName;
  #elif defined(uG31xx_OS_ANDROID)
    char *backupFileName;
  #endif

#if defined(uG31xx_OS_ANDROID)
    } __attribute__ ((packed)) BackupDataType;
#else   ///< else of defined(uG31xx_OS_ANDROID)	      
    } BackupDataType;
#endif  ///< end of defined(uG31xx_OS_ANDROID)

/**
 * @brief UpiBackupData
 *
 *  Backup data from IC to system routine
 *
 * @para  data  address of BackupDataType
 * @return  _UPI_NULL_
 */
extern void UpiBackupData(BackupDataType *data);

/**
 *	@brief	UpiUpdateSuspendData
 *
 *	Update data for suspend backup
 *
 *	@para	data	address of BackupDataType
 *	@return	NULL
 */
extern void UpiUpdateSuspendData(BackupDataType *data, _backup_bool_ is_resume);
