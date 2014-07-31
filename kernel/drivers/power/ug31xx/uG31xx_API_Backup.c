/**
 * @filename  uG31xx_API_Backup.cpp
 *
 *  Backup data on uG31xx to a file in system
 *
 * @author  AllenTeng <allen_teng@upi-semi.com>
 */

#include "stdafx.h"     //windows need this??
#include "uG31xx_API.h"

#define UG31XX_BACKUP_FILE_ENABLE

#ifdef uG31xx_OS_WINDOWS

typedef char mm_segment_t;
typedef char loff_t;

typedef struct file
{
  FILE *fp;
} fileType;

#define O_RDONLY  (1<<0)
#define O_WRONLY  (1<<1)
#define O_RDWR    (3<<0)
#define O_CREAT   (1<<2)
#define O_APPEND  (1<<3)

static struct file BackupFile;

/**
 * @brief filp_open
 *
 *  Open file
 *
 * @para  path  address of file
 * @para  cntl  FILE_CNTL parameter
 * @para  misc  dummy parameter
 * @return  address of BackupFile
 */
struct file * filp_open(const wchar_t *path, int cntl, int misc)
{
  if(BackupFile.fp != _UPI_NULL_)
  {
    fclose(BackupFile.fp);
  }
  BackupFile.fp = _UPI_NULL_;

  switch(cntl & O_RDWR)
  {
    case  O_RDONLY:
      _wfopen_s(&BackupFile.fp, path, _T("rb, ccs=UTF-8"));
      break;
    case  O_WRONLY:
      _wfopen_s(&BackupFile.fp, path, _T("wb, ccs=UTF-8"));
      break;
    case  O_RDWR:
      _wfopen_s(&BackupFile.fp, path, _T("r+b, ccs=UTF-8"));
      break;
    default:
      _wfopen_s(&BackupFile.fp, path, _T("rb, ccs=UTF-8"));
      break;
  }
  return (&BackupFile);
}

/**
 * @brief filp_close
 *
 *  Close file
 *
 * @para  fp  address of struct file
 * @para  misc  dummy value
 * @return  _UPI_NULL_
 */
void filp_close(struct file *fp, int misc)
{
  if(fp->fp != _UPI_NULL_)
  {
    fclose(fp->fp);
  }
}

/**
 * @brief vfs_write
 *
 *  Write data to binary file
 *
 * @para  fp  address of struct file
 * @para  data  address of data to be written
 * @para  size  size to be written
 * @para  pos start position in the file
 * @return  size be written
 */
size_t vfs_write(struct file *fp, char *data, int size, loff_t *pos)
{
  return (fwrite(data, sizeof(char), size, fp->fp));
}

/**
 * @brief vfs_read
 *
 *  Read data from binary file
 *
 * @para  fp  address of struct file
 * @para  data  address of data to be read
 * @para  size  size to be read
 * @para  pos start position in the file
 * @return  size be read
 */
size_t vfs_read(struct file *fp, char *data, int size, loff_t *pos)
{
  return (fread(data, sizeof(char), size, fp->fp));
}

/**
 * @brief IS_ERR
 *
 *  Check file is opened or not
 *
 * @para  fp  address of struct file
 * @return  _UPI_TRUE_ if file is opened
 */
_upi_bool_ IS_ERR(struct file *fp)
{
  return ((fp->fp == _UPI_NULL_) ? _UPI_TRUE_ : _UPI_FALSE_);
}

/**
 * @brief get_fs
 *
 *  Dummy function
 *
 * @return  0
 */
mm_segment_t get_fs(void)
{
  return (0);
}

/**
 * @brief set_fs
 *
 *  Dummy function
 *
 * @para  value mm_segment_t value
 * @return  _UPI_NULL_
 */
void set_fs(mm_segment_t value)
{
}

/**
 * @brief get_ds
 *
 *  Dummy function
 *
 * @return  0
 */
mm_segment_t get_ds(void)
{
  return (0);
}

#endif  ///< end of uG31xx_OS_WINDOWS

/**
 * @brief ISFileExist
 *
 *  Check file existed or not
 *
 * @para  data  address of BackupDataType
 * @return  BACKUP_BOOL_TRUE if file existed
 */
_backup_bool_ ISFileExist(BackupDataType *data)
{
  struct file *fp;

  fp = filp_open(data->backupFileName, O_RDONLY, 0);
  if(IS_ERR(fp))
  {
    UG31_LOGI("[%s]: Backup file is not existed\n", __func__);
    return (BACKUP_BOOL_FALSE);
  }
  UG31_LOGI("[%s]: Backup file is existed\n", __func__);
  filp_close(fp, _UPI_NULL_);
  return (BACKUP_BOOL_TRUE);
}

/**
 * @brief WriteFile
 *
 *  Write data to file
 *
 * @para  data  address of BackupDataType
 * @para  fp  address of struct fp
 */
void WriteFile(BackupDataType *data, struct file *fp)
{
  mm_segment_t oldFS;
  loff_t pos;
  size_t size;

  oldFS = get_fs();
  set_fs(get_ds());

  pos = 0;
  size = vfs_write(fp, (char *)data->capData->encriptTable, sizeof(data->capData->encriptTable), &pos);
  UG31_LOGI("[%s]: Write table %d bytes\n", __func__, size);
  size = vfs_write(fp, (char *)(&data->sysData->rmFromIC), sizeof(data->sysData->rmFromIC), &pos);
  UG31_LOGI("[%s]: Write RM (%d) %d bytes\n", __func__, data->sysData->rmFromIC, size);
  size = vfs_write(fp, (char *)(&data->sysData->fccFromIC), sizeof(data->sysData->fccFromIC), &pos);
  UG31_LOGI("[%s]: Write FCC (%d) %d bytes\n", __func__, data->sysData->fccFromIC, size);
  size = vfs_write(fp, (char *)(&data->sysData->timeTagFromIC), sizeof(data->sysData->timeTagFromIC), &pos);
  UG31_LOGI("[%s]: Write Time Tag (%d) %d bytes\n", __func__, data->sysData->timeTagFromIC, size);
  size = vfs_write(fp, (char *)(&data->sysData->tableUpdateIdxFromIC), sizeof(data->sysData->tableUpdateIdxFromIC), &pos);
  UG31_LOGI("[%s]: Write Table Update Index (%d) %d bytes\n", __func__, data->sysData->tableUpdateIdxFromIC, size);
  size = vfs_write(fp, (char *)(&data->sysData->deltaCapFromIC), sizeof(data->sysData->deltaCapFromIC), &pos);
  UG31_LOGI("[%s]: Write Delta Capacity (%d) %d bytes\n", __func__, data->sysData->deltaCapFromIC, size);
  size = vfs_write(fp, (char *)(&data->sysData->adc1ConvTime), sizeof(data->sysData->adc1ConvTime), &pos);
  UG31_LOGI("[%s]: Write ADC1 Conversion Time (%d) %d bytes\n", __func__, data->sysData->adc1ConvTime, size);
  size = vfs_write(fp, (char *)(&data->sysData->rsocFromIC), sizeof(data->sysData->rsocFromIC), &pos);
  UG31_LOGI("[%s]: Write RSOC (%d) %d bytes\n", __func__, data->sysData->rsocFromIC, size);
  size = vfs_write(fp, (char *)(&data->targetFileVer), sizeof(_backup_u32_), &pos);
  UG31_LOGI("[%s]: Write version (%d) %d bytes\n", __func__, data->targetFileVer, size);

  set_fs(oldFS);
}

/**
 * @brief CreateBackupFile
 *
 *  Create backup file in system
 *
 * @para  data  address of BackupDataType
 * @return  BACKUP_BOOL_TRUE if success
 */
_backup_bool_ CreateBackupFile(BackupDataType *data)
{
  struct file *fp;

  fp = filp_open(data->backupFileName, O_CREAT | O_RDWR, 0);
  if(IS_ERR(fp))
  {
    UG31_LOGI("[%s]: Create backup file fail\n", __func__);
    return (BACKUP_BOOL_FALSE);
  }

  /// [AT-PM] : Write data to file ; 02/21/2013
  WriteFile(data, fp);

  filp_close(fp, _UPI_NULL_);
  return (BACKUP_BOOL_TRUE);
}

/**
 * @brief ReadFile
 *
 *  Read data from file
 *
 * @para  data  address of BackupDataType
 * @para  fp  address of struct fp
 */
void ReadFile(BackupDataType *data, struct file *fp)
{
  mm_segment_t oldFS;
  loff_t pos;
  size_t size;

  oldFS = get_fs();
  set_fs(get_ds());

  pos = 0;
  size = vfs_read(fp, (char *)&data->capData->encriptTable, sizeof(data->capData->encriptTable), &pos);
  UG31_LOGI("[%s]: Read table %d bytes\n", __func__, size);
  size = vfs_read(fp, (char *)(&data->sysData->rmFromIC), sizeof(data->sysData->rmFromIC), &pos);
  UG31_LOGI("[%s]: Read RM (%d) %d bytes\n", __func__, data->sysData->rmFromIC, size);
  size = vfs_read(fp, (char *)(&data->sysData->fccFromIC), sizeof(data->sysData->fccFromIC), &pos);
  UG31_LOGI("[%s]: Read FCC (%d) %d bytes\n", __func__, data->sysData->fccFromIC, size);
  size = vfs_read(fp, (char *)(&data->sysData->timeTagFromIC), sizeof(data->sysData->timeTagFromIC), &pos);
  UG31_LOGI("[%s]: Read Time Tag (%d) %d bytes\n", __func__, data->sysData->timeTagFromIC, size);
  size = vfs_read(fp, (char *)(&data->sysData->tableUpdateIdxFromIC), sizeof(data->sysData->tableUpdateIdxFromIC), &pos);
  UG31_LOGI("[%s]: Read Table Update Index (%d) %d bytes\n", __func__, data->sysData->tableUpdateIdxFromIC, size);
  size = vfs_read(fp, (char *)(&data->sysData->deltaCapFromIC), sizeof(data->sysData->deltaCapFromIC), &pos);
  UG31_LOGI("[%s]: Read Delta Capacity (%d) %d bytes\n", __func__, data->sysData->deltaCapFromIC, size);
  size = vfs_read(fp, (char *)(&data->sysData->adc1ConvTime), sizeof(data->sysData->adc1ConvTime), &pos);
  UG31_LOGI("[%s]: Read ADC1 Conversion Time (%d) %d bytes\n", __func__, data->sysData->adc1ConvTime, size);
  size = vfs_read(fp, (char *)(&data->sysData->rsocFromIC), sizeof(data->sysData->rsocFromIC), &pos);
  UG31_LOGI("[%s]: Read RSOC (%d) %d bytes\n", __func__, data->sysData->rsocFromIC, size);
  size = vfs_read(fp, (char *)(&data->backupFileVer), sizeof(_backup_u32_), &pos);
  UG31_LOGI("[%s]: Read version (%d) %d bytes\n", __func__, data->backupFileVer, size);

  set_fs(oldFS);
}

/**
 * @brief CheckBackupFile
 *
 *  Check backup file is consisted with IC or not
 *
 * @para  data  address of BackupDataType
 * @return  BACKUP_BOOL_TRUE if success
 */
_backup_bool_ CheckBackupFile(BackupDataType *data)
{
  struct file *fp;
  CapacityDataType *bufCapData;
  SystemDataType *bufSysData;

  fp = filp_open(data->backupFileName, O_RDWR, 0);
  if(IS_ERR(fp))
  {
    UG31_LOGI("[%s]: Open backup file fail\n", __func__);
    return (BACKUP_BOOL_FALSE);
  }

  /// [AT-PM] : Create buffer ; 02/21/2013
  #if defined(uG31xx_OS_ANDROID)
    #ifdef  uG31xx_BOOT_LOADER
      bufCapData = (CapacityDataType *)malloc(sizeof(CapacityDataType));
      bufSysData = (SystemDataType *)malloc(sizeof(SystemDataType));
    #else   ///< else of uG31xx_BOOT_LOADER
      bufCapData = (CapacityDataType *)kmalloc(sizeof(CapacityDataType), GFP_KERNEL);
      bufSysData = (SystemDataType *)kmalloc(sizeof(SystemDataType), GFP_KERNEL);
    #endif  ///< end of uG31xx_BOOT_LOADER
  #else   ///< else of defined(uG31xx_OS_ANDROID)
    bufCapData = (CapacityDataType *)malloc(sizeof(CapacityDataType));
    bufSysData = (SystemDataType *)malloc(sizeof(SystemDataType));
  #endif  ///< end of defined(uG31xx_OS_ANDROID)
  memcpy(bufCapData, data->capData, sizeof(CapacityDataType));
  memcpy(bufSysData, data->sysData, sizeof(SystemDataType));

  /// [AT-PM] : Get data from file ; 02/21/2013
  ReadFile(data, fp);

  /// [AT-PM] : Following information is not checked ; 02/21/2013
  data->sysData->rmFromIC = bufSysData->rmFromIC;
  data->sysData->timeTagFromIC = bufSysData->timeTagFromIC;
  data->sysData->deltaCapFromIC = bufSysData->deltaCapFromIC;
  data->sysData->adc1ConvTime = bufSysData->adc1ConvTime;
  
  /// [AT-PM] : Check data ; 02/21/2013
  if((memcmp(bufCapData, data->capData, sizeof(CapacityDataType)) != 0) ||
     (memcmp(bufSysData, data->sysData, sizeof(SystemDataType)) != 0) ||
     (data->backupFileVer != data->targetFileVer))
  {
    UG31_LOGI("[%s]: Backup file needs to be updated\n", __func__);
    /// [AT-PM] : Write data to file ; 02/21/2013
    memcpy(data->capData, bufCapData, sizeof(CapacityDataType));
    memcpy(data->sysData, bufSysData, sizeof(SystemDataType));    
    WriteFile(data, fp);
  }
  
  filp_close(fp, _UPI_NULL_);

  #if defined(uG31xx_OS_ANDROID)
    #ifdef  uG31xx_BOOT_LOADER
      free(bufCapData);
      free(bufSysData);
    #else   ///< else of uG31xx_BOOT_LOADER
      kfree(bufCapData);
      kfree(bufSysData);
    #endif  ///< end of uG31xx_BOOT_LOADER
  #else   ///< else of defined(uG31xx_OS_ANDROID)
    free(bufCapData);
    free(bufSysData);
  #endif  ///< end of defined(uG31xx_OS_ANDROID)
  return (BACKUP_BOOL_TRUE);
}

/**
 *	@brief	WriteSuspendBackupData
 *
 *	Write suspend backup data to file
 *
 * 	@para	data	address of BackupDataType
 *	@return	BACKUP_BOOL_TRUE if success
 */
_backup_bool_ WriteSuspendBackupData(BackupDataType *data)
{
	struct file *fp;
	_backup_u8_ idx;
	mm_segment_t oldFS;
	loff_t pos;
	size_t size;
	_backup_u8_ *buf;

	/// [AT-PM] : Check backup file content ; 10/12/2013
	fp = filp_open("/sdcard/upi_table", O_RDWR, 0);
	if(!IS_ERR(fp))
	{
		oldFS = get_fs();
		set_fs(get_ds());

		idx = 0;
		pos = 0;
		buf = (_backup_u8_ *)kmalloc(sizeof(BackupSuspendDataType), GFP_KERNEL);
		while(idx < BACKUP_MAX_LOG_SUSPEND_DATA)
		{
			size = vfs_read(fp, (char *)buf, sizeof(BackupSuspendDataType), &pos);

			if(memcmp((char *)buf, (char *)data->logData[idx], sizeof(BackupSuspendDataType)) != 0)
			{
				UG31_LOGI("[%s]: Need to write suspend data to file (%d)\n", __func__, idx);
				break;
			}

			idx = idx + 1;
		}
		kfree(buf);

		set_fs(oldFS);

		filp_close(fp, _UPI_NULL_);

		if(idx >= BACKUP_MAX_LOG_SUSPEND_DATA)
		{
			UG31_LOGI("[%s]: No need to update suspend data.\n", __func__, idx);
			return (BACKUP_BOOL_TRUE);
		}
	}

        fp = filp_open("/sdcard/upi_table", O_CREAT | O_RDWR, 0);
        if(IS_ERR(fp))
        {
                UG31_LOGI("[%s]: Open backup file fail\n", __func__);
                return (BACKUP_BOOL_FALSE);
        }

	oldFS = get_fs();
	set_fs(get_ds());

	idx = 0;
	pos = 0;
	while(idx < BACKUP_MAX_LOG_SUSPEND_DATA)
	{		
		size = vfs_write(fp, (char *)data->logData[idx], sizeof(BackupSuspendDataType), &pos);
		UG31_LOGI("[%s]: Write suspend data %d with %d bytes\n", __func__, idx, size);

		idx = idx + 1;
	}

	set_fs(oldFS);

	filp_close(fp, _UPI_NULL_);
	return (BACKUP_BOOL_TRUE);
}

/// =============================================
/// Extern Function Region
/// =============================================

#define RETRY_CHECKING_THRESHOLD      (CONST_CONVERSION_COUNT_THRESHOLD)

/**
 * @brief UpiBackupData
 *
 *  Backup data from IC to system routine
 *
 * @para  data  address of BackupDataType
 * @return  _UPI_NULL_
 */
void UpiBackupData(BackupDataType *data)
{
  _backup_bool_ rtnBool;

  #ifndef UG31XX_BACKUP_FILE_ENABLE
    rtnBool = _UPI_TRUE_;
    data->backupFileSts = BACKUP_FILE_STS_EXIST;
  #endif  ///< end of UG31XX_BACKUP_FILE_ENABLE
  
  #if defined(uG31xx_OS_ANDROID)
    #ifdef  uG31xx_BOOT_LOADER
      data->backupFileName = (char *)malloc(strlen(BACKUP_FILE_PATH));
    #else   ///< else of uG31xx_BOOT_LOADER
      data->backupFileName = (char *)kmalloc(strlen(BACKUP_FILE_PATH), GFP_KERNEL);
    #endif  ///< end of uG31xx_BOOT_LOADER
    sprintf(data->backupFileName, "%s", BACKUP_FILE_PATH);
  #else   ///< else of defined(uG31xx_OS_ANDROID)
    data->backupFileName = data->sysData->backupFileName;
  #endif  ///< end of defined(uG31xx_OS_ANDROID)

  switch(data->backupFileSts)
  {
    case  BACKUP_FILE_STS_CHECKING:
      /// [AT-PM] : Check backup file existed or not ; 02/21/2013
      rtnBool = ISFileExist(data);
      if(rtnBool == BACKUP_BOOL_TRUE)
      {
        #ifdef  BACKUP_FILE_STS_NOT_EXIST
          data->backupFileSts = BACKUP_FILE_STS_NOT_EXIST;
        #else   ///< else of BACKUP_FILE_STS_NOT_EXIST
          data->backupFileSts = BACKUP_FILE_STS_EXIST;
        #endif  ///< end of BACKUP_FILE_STS_NOT_EXIST
      }
      else
      {
        if(data->measData->lastCounter > RETRY_CHECKING_THRESHOLD)
        {
          data->backupFileSts = BACKUP_FILE_STS_NOT_EXIST;
        }
      }
      break;
    case  BACKUP_FILE_STS_NOT_EXIST:
      /// [AT-PM] : Create backup file ; 02/21/2013
      rtnBool = CreateBackupFile(data);
      if(rtnBool == BACKUP_BOOL_TRUE)
      {
        data->backupFileSts = BACKUP_FILE_STS_EXIST;
      }
      break;
    case  BACKUP_FILE_STS_EXIST:
      rtnBool = CheckBackupFile(data);
      if(rtnBool != BACKUP_BOOL_TRUE)
      {
        data->backupFileSts = BACKUP_FILE_STS_NOT_EXIST;
      }
      else
      {
        data->backupFileSts = BACKUP_FILE_STS_COMPARE;
      }
      break;
    case  BACKUP_FILE_STS_COMPARE:
      if(data->icDataAvailable == BACKUP_BOOL_TRUE)
      {
        /// [AT-PM] : Check content of file is consist with IC or not ; 02/21/2013
        rtnBool = CheckBackupFile(data);
        if(rtnBool != BACKUP_BOOL_TRUE)
        {
          data->backupFileSts = BACKUP_FILE_STS_NOT_EXIST;
        }
        rtnBool = WriteSuspendBackupData(data);
        if(rtnBool != BACKUP_BOOL_TRUE)
        {
          data->backupFileSts = BACKUP_FILE_STS_NOT_EXIST;
        }
      }
      else
      {
        data->backupFileSts = BACKUP_FILE_STS_CHECKING;
      }
      break;
    default:
      /// [AT-PM] : Un-known state ; 02/21/2013
      data->backupFileSts = BACKUP_FILE_STS_NOT_EXIST;
      break;
  }

  #if defined(uG31xx_OS_ANDROID)
    #ifdef  uG31xx_BOOT_LOADER
      free(data->backupFileName);
    #else   ///< else of uG31xx_BOOT_LOADER
      kfree(data->backupFileName);
    #endif  ///< end of uG31xx_BOOT_LOADER
  #endif  ///< end of defined(uG31xx_OS_ANDROID)

  UG31_LOGI("[%s]: Backp file status = %d\n", __func__, data->backupFileSts);  
}

/**
 *      @brief  UpiUpdateSuspendData
 *
 *      Update data for suspend backup
 *
 *      @para   data    	address of BackupDataType
 *	@para	is_resume	UPI_TRUE if called in resume
 *      @return NULL
 */
void UpiUpdateSuspendData(BackupDataType *data, _backup_bool_ is_resume)
{
	if(is_resume == _UPI_TRUE_)
	{
		data->logData[data->backupDataIdx]->afterCapData = *(data->capData);
		data->logData[data->backupDataIdx]->afterMeasData = *(data->measData);
		UG31_LOGI("[%s]: Backup resume data to buffer %d\n", __func__, data->backupDataIdx);

		data->backupDataIdx = data->backupDataIdx + 1;
		if(data->backupDataIdx >= BACKUP_MAX_LOG_SUSPEND_DATA)
		{
			data->backupDataIdx = BACKUP_MAX_LOG_SUSPEND_DATA;
		}
	}
	else
	{
		if((data->backupDataIdx >= BACKUP_MAX_LOG_SUSPEND_DATA) && (is_resume == _UPI_FALSE_))
		{
			data->backupDataIdx = 0;
			while(data->backupDataIdx < (BACKUP_MAX_LOG_SUSPEND_DATA - 1))
			{
				memcpy(data->logData[data->backupDataIdx], data->logData[data->backupDataIdx + 1], sizeof(BackupSuspendDataType));
				data->backupDataIdx = data->backupDataIdx + 1;
			}
		}

		data->logData[data->backupDataIdx]->beforeCapData = *(data->capData);
		data->logData[data->backupDataIdx]->beforeMeasData = *(data->measData);
		UG31_LOGI("[%s]: Backup suspend data to buffer %d\n", __func__, data->backupDataIdx);
	}
}

