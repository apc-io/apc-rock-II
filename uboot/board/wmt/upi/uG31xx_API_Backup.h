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

typedef struct BackupDataST {

  CapacityDataType *capData;
  SystemDataType *sysData;
  MeasDataType *measData;

  _backup_bool_ icDataAvailable;
  _backup_u8_ backupFileSts;
  
  #if defined (uG31xx_OS_WINDOWS)
    const wchar_t* backupFileName;
  #elif defined(uG31xx_OS_ANDROID)
    char *backupFileName;
  #endif

#if defined(uG31xx_OS_ANDROID)
    } __attribute__ ((aligned(4))) BackupDataType;
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

