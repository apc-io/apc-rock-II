/**
 * @filename	ug31xx_boot.c
 *
 *	uG31xx operation in bootloader
 *
 * @author	AllenTeng <allen_teng@upi-semi.com>
 */

#include "../../../include/configs/wmt.h"
#ifdef CONFIG_BATT_UPI
#include <common.h>
//#include <app.h>
//#include <debug.h>
//#include <arch/arm.h>
//#include <dev/udc.h>
//#include <linux/string.h>
//#include <kernel/thread.h>
//#include <arch/ops.h>
//#include <malloc.h>

//#include <stdarg.h>

#include "ug31xx_boot_i2c.h"
#include "ug31xx_boot.h"
#include "uG31xx_API.h"
#include "../wmt_battery.h"

#define	UG31XX_BOOT_VERSION	(12)

#define	UG31XX_BOOT_STATUS_FCC_IS_0		(1<<0)
#define	UG31XX_BOOT_STATUS_IC_IS_NOT_ACTIVE	(1<<1)
#define	UG31XX_BOOT_STATUS_WRONG_PRODUCT_TYPE	(1<<2)
#define   DPRINTF(x,args...)	printf("UG31xx " x , ##args)

extern void ug31xx_init_i2c(int i2c_dev);

UG31xxDataType	ug31_data;

typedef struct UG31xxDataInternalST {
	UG31xxDataType	*info;

	CELL_PARAMETER	ggbParameter;
	CELL_TABLE	ggbCellTable;

	OtpDataType 	otpData;
	MeasDataType	measData;
	SystemDataType	sysData;

	int		tpTime;
	int		status;
} UG31xxDataInternalType;

#undef bool
typedef enum _bool {false, true} bool;

#include "ggb/ug31xx_ggb_data_uboot_wms8309_wm8_20130820_110949.h"
#include "ggb/ug31xx_ggb_data_uboot_wms8309_c7_20130725_164935.h"
#include "ggb/ug31xx_ggb_data_uboot_wms8309_c7_20130910_130553.h"
#include "ggb/ug31xx_ggb_data_uboot_wms7320_20130718_200031.h"
#include "ggb/ug31xx_ggb_data_uboot_cw500_20130801_103638.h"
#include "ggb/ug31xx_ggb_data_uboot_mp718_20131004_070110.h"
#include "ggb/ug31xx_ggb_data_uboot_t73v_20131120_001204.h"

struct ggb_info {
	char *name;
	char *data;
};

/* Extern Function */
static struct ggb_info ggb_arrays[] = {
	{
		.name = "wms8309wm8",
		.data = FactoryGGBXFile_wms8309_wm8,
	}, {
		.name = "wms7320",
		.data = FactoryGGBXFile_wms7320,
	}, {
		.name = "wms8309c7_3900mAh",
		.data = FactoryGGBXFile_wms8309_c7_3900mAh,
	}, {
		.name = "wms8309c7_3000mAh",
		.data = FactoryGGBXFile_wms8309_c7_3000mAh,
	}, {
		.name = "cw500",
		.data = FactoryGGBXFile_cw500, 
	}, {
		.name = "mp718",
		.data = FactoryGGBXFile_mp718,
	}, {
		.name = "t73v",
		.data = FactoryGGBXFile_t73v,
	},
};

struct ug31xx_param {
	int i2c_adapter;
	int external_temperature;
	int charge_temperature_range[2];
	struct ggb_info *ggb
};

static struct ug31xx_param ug31xx_param = {
	.i2c_adapter = 1,
	.external_temperature = 1,
	.charge_temperature_range = { -150, 600 },
	.ggb = &ggb_arrays[0],
};

static void parse_ug31xx_param(void)
{
	enum {
		idx_ada,
		idx_et,
		idx_temp0,
		idx_temp1,
		idx_max
	};
	long ps[idx_max];
	char *p;
	char *endp;
	int i = 0;

	p = getenv("wmt.ug31xx.param");
	if (!p)
		goto out;

	memset(ps, 0, sizeof(ps));
	while (i < idx_max) {
		ps[i++] = simple_strtol(p, &endp, 10);
		if (*endp == '\0')
			break;
		p = endp + 1;

		if (*p == '\0')
			break;
	}

	ug31xx_param.i2c_adapter = ps[idx_ada];
	ug31xx_param.external_temperature = ps[idx_et];
	if (ps[idx_temp1] > ps[idx_temp0]) {
		ug31xx_param.charge_temperature_range[0] = ps[idx_temp0];
		ug31xx_param.charge_temperature_range[1] = ps[idx_temp1];
	}

	for (i = 0; i < ARRAY_SIZE(ggb_arrays); i++) {
		if (strstr(p, ggb_arrays[i].name)) {
			ug31xx_param.ggb = &ggb_arrays[i];
			break;
		}
	}

out:
	printf("ug31xx i2c%d, ggb %s, %s, charge temperature range [%d, %d]\n",
		ug31xx_param.i2c_adapter,
		ug31xx_param.ggb->name,
		ug31xx_param.external_temperature ? "ET" : "IT",
		ug31xx_param.charge_temperature_range[0],
		ug31xx_param.charge_temperature_range[1]);
}


/**
 * @brief	GetTickCount
 *
 *	Get tick count in milli-second
 *
 * @return	tick count
 */
_upi_u32_ GetTickCount(void)
{
	return (0);
}

/**
 * @brief	GetSysTickCount
 *
 *	Get system tick count in milli-second
 *
 * @return	tick count
 */
_upi_u32_ GetSysTickCount(void)
{
	return (0);
}

/**
 * @brief	ug31_dprintk
 *
 *	print debug message
 *
 * @para	level	debug message level
 * @para	fmt	debug message
 * @return	message length
 */
#if 0
int ug31_dprintk(int level, const char *fmt, ...)
{
	va_list args;
	int r;

	va_start(args, fmt);
	//r = dprintk(level, fmt, args);
        r = NULL;
	va_end(args);
	return (r);
}
#endif


#define	TP_REACH_SOC	(100)

void CheckRsocBeforeTP(UG31xxDataInternalType *obj)
{
	int rm;

	if(obj->sysData.rsocFromIC >= TP_REACH_SOC)
	{
		obj->sysData.rsocFromIC = TP_REACH_SOC - 1;

		rm = (int)obj->sysData.rsocFromIC;
		rm = rm*obj->sysData.fccFromIC/100;
		obj->sysData.rmFromIC = (_sys_u16_)rm;
		return;
	}

	if(obj->sysData.rmFromIC > obj->sysData.fccFromIC)
	{
		obj->sysData.rmFromIC = obj->sysData.fccFromIC;
	}
}

/**
 * @brief	ResetFullCharge
 *
 *	Reset full charge checking module
 *
 * @para	obj	address of UG31xxDataInternalType
 * @return	NULL
 */
void ResetFullCharge(UG31xxDataInternalType *obj)
{
	obj->tpTime = 0;

	CheckRsocBeforeTP(obj);
}

#define	TP_COUNT_INTERVAL	(5)

/**
 * @brief	FullChargeCheck
 *
 *	Check full charge condition
 *
 * @para	obj	address of UG31xxDataInternalType
 * @return	NULL
 */
void FullChargeCheck(UG31xxDataInternalType *obj)
{
	if(obj->tpTime >= obj->ggbParameter.TPTime)
	{
		return;
	}

	/// [AT-PM] : Check taper voltage ; 02/01/2013
	if(obj->measData.bat1Voltage < obj->ggbParameter.TPVoltage)
	{
		ResetFullCharge(obj);
		return;
	}

	/// [AT-PM] : Check taper current ; 02/01/2013
	if((obj->measData.curr < obj->ggbParameter.standbyCurrent) ||
	   (obj->measData.curr > obj->ggbParameter.TPCurrent))
	{
		ResetFullCharge(obj);
		return;
	}

	/// [AT-PM] : Check taper time ; 02/01/2013
	obj->tpTime = obj->tpTime + TP_COUNT_INTERVAL;
	if(obj->tpTime < obj->ggbParameter.TPTime)
	{
		CheckRsocBeforeTP(obj);
		return;
	}

	/// [AT-PM] : Set full charge condition ; 02/01/2013
	obj->sysData.rsocFromIC = TP_REACH_SOC;
	obj->sysData.rmFromIC = obj->sysData.fccFromIC;
}

/**
 * @brief	SetMemoryMapping
 *
 *	Set memory mapping
 *
 * @para	data	address of UG31xxDataType
 * @para	obj	address of UG31xxDataInternalType pointer
 * @return	NULL
 */
void SetMemoryMapping(UG31xxDataType *data, UG31xxDataInternalType **obj)
{
	(*obj) = (UG31xxDataInternalType *)data->buf;
	(*obj)->info = data;
	(*obj)->measData.sysData = &((*obj)->sysData);
	(*obj)->measData.otp = &((*obj)->otpData);
	(*obj)->sysData.ggbXBuf = (GGBX_FILE_HEADER *)(ug31xx_param.ggb->data);
	(*obj)->sysData.ggbParameter = &((*obj)->ggbParameter);
	(*obj)->sysData.ggbCellTable = &((*obj)->ggbCellTable);
	(*obj)->sysData.otpData = &((*obj)->otpData);
        //printf("[SetMemoryMapping] ggb_tag (%d) = %d\n", sizeof((*obj)->sysData.ggbXBuf->ggb_tag), (*obj)->sysData.ggbXBuf->ggb_tag);
        //printf("[SetMemoryMapping] sum16 (%d) = %d\n", sizeof((*obj)->sysData.ggbXBuf->sum16), (*obj)->sysData.ggbXBuf->sum16);
        //printf("[SetMemoryMapping] time_stamp (%d) = %d\n", sizeof((*obj)->sysData.ggbXBuf->time_stamp), (*obj)->sysData.ggbXBuf->time_stamp);
        //printf("[SetMemoryMapping] length (%d) = %d\n", sizeof((*obj)->sysData.ggbXBuf->length), (*obj)->sysData.ggbXBuf->length);
        //printf("[SetMemoryMapping] num_ggb (%d) = %d\n", sizeof((*obj)->sysData.ggbXBuf->num_ggb), (*obj)->sysData.ggbXBuf->num_ggb);
}

#define	CURRENT_MAGIC_NUMBER	(4)
#define	OCV_TABLE_INDEX		(1)

static int OcvTableSoc[] = {
	100,
	95,
	90,
	85,
	80,
	75,
	70,
	65,
	60,
	55,
	50,
	45,
	40,
	35,
	30,
	25,
	20,
	15,
	10,
	5,
	0,
};

/**
 * @brief	InitCharge
 *
 *	Look up initial charge from table
 *
 * @para	obj	address of UG31xxDataInternalType
 * @return	NULL
 */
void InitCharge(UG31xxDataInternalType *obj)
{
	int volt;
	int tmp;
	int idxTemp;
	int idxSoc;
	int ocv;

	/// [AT-PM] : FCC is equal to ILMD ; 02/24/2013
	obj->info->fcc = (int)obj->ggbParameter.ILMD;

	/// [AT-PM] : Real battery voltage ; 02/24/2013
	volt = (int)obj->measData.bat1Voltage;
	tmp = (int)obj->measData.curr;
	tmp = tmp/CURRENT_MAGIC_NUMBER;
	if(tmp > 0)
	{
		volt = volt - tmp;
	}

	/// [AT-PM] : Look up table ; 02/24/2013
	idxSoc = 0;
	while(idxSoc < OCV_NUMS)
	{
		idxTemp = 0;
		ocv = 0;
		while(idxTemp < TEMPERATURE_NUMS)
		{
			ocv = ocv + obj->ggbCellTable.INIT_OCV[idxTemp][OCV_TABLE_INDEX][idxSoc];
			idxTemp = idxTemp + 1;
		}
		ocv = ocv/TEMPERATURE_NUMS;

		if(volt >= ocv)
		{
			break;
		}

		idxSoc = idxSoc + 1;
	}
	if(idxSoc < OCV_NUMS)
	{
		obj->info->rsoc = OcvTableSoc[idxSoc];
	}
	else
	{
		obj->info->rsoc = 0;
	}

	/// [AT-PM] : Calculate RM ; 02/24/2013
	tmp = obj->info->rsoc;
	tmp = tmp*obj->info->fcc/CONST_PERCENTAGE;
	obj->info->rm = tmp;
}

/// ===========================================================================
/// Extern region
/// ===========================================================================

#define DEFAULT_ADC1_CONV_TIME  (1250)
#define DEFAULT_TIME_TICK       (0xFFFFFFFF)

/**
 * @brief       UpiBootInitial
 *
 *      Initialize uG31xx
 *
 * @para        data    address of UG31xxDataType
 * @return      UPI_BOOT_RTN
 */
int UpiBootInitial(UG31xxDataType *data)
{
	UG31xxDataInternalType *obj;

	//dprintf(INFO, "[UpiBootInitial] Version = %d\n", UG31XX_BOOT_VERSION);
	printf("[UpiBootInitial] Version = %d\n", UG31XX_BOOT_VERSION);

	/// [AT-PM] : Create memory buffer ; 02/01/2013
	data->buf = (char *)malloc(sizeof(UG31xxDataInternalType));
	memset(data->buf, 0, sizeof(UG31xxDataInternalType));
	obj = _UPI_NULL_;
	SetMemoryMapping(data, &obj);

	data->version = UG31XX_BOOT_VERSION;
	obj->status = 0;

	//dprintf(INFO, "[UpiBootInitial] data->ug31xx_i2c_dev = %d\n",data->ug31xx_i2c_dev);
	/// [AT-PM] : Initial I2C device ; 02/04/2013
	//ug31xx_init_i2c(data->ug31xx_i2c_dev);

	/// [AT-PM] : Load GGB data ; 02/24/2013
	UpiInitSystemData(&obj->sysData);
	printf("[UPI Boot GGB]: %d (0x%04x)\n", obj->ggbCellTable.INIT_OCV[0][0][0], obj->ggbCellTable.INIT_OCV[0][0][0]);
	printf("[UPI Boot GGB]: %d (0x%04x)\n", obj->ggbParameter.adc1_pgain, obj->ggbParameter.adc1_pgain);
	printf("[UPI Boot GGB]: %d (0x%04x)\n", obj->ggbParameter.adc1_ngain, obj->ggbParameter.adc1_ngain);
	printf("[UPI Boot GGB]: %d (0x%04x)\n", obj->ggbParameter.adc1_pos_offset, obj->ggbParameter.adc1_pos_offset);
	printf("[UPI Boot GGB]: %d (0x%04x)\n", obj->ggbParameter.adc2_gain, obj->ggbParameter.adc2_gain);	
	printf("[UPI Boot GGB]: %d (0x%04x)\n", obj->ggbParameter.adc2_offset, obj->ggbParameter.adc2_offset);

	/// [AT-PM] : Get capacity information from IC ; 02/24/2013
	UpiLoadBatInfoFromIC(&obj->sysData);
	if(obj->sysData.fccFromIC == 0)
	{
		//dprintf(CRITICAL, "[UpiBootInitial] Capacity in uG31xx is invalid.\n");
		printf("[UpiBootInitial] Capacity in uG31xx is invalid.\n");
		obj->status = obj->status | UG31XX_BOOT_STATUS_FCC_IS_0;
	}

	/// [AT-PM] : Check IC is active or not ; 02/24/2013
	if(UpiCheckICActive() == _UPI_TRUE_)
	{
		//dprintf(CRITICAL, "[UpiBootInitial] uG31xx is not actived.\n");
		printf("[UpiBootInitial] uG31xx is not actived.\n");
		obj->status = obj->status | UG31XX_BOOT_STATUS_IC_IS_NOT_ACTIVE;

		/// [AT-PM] : Active uG31xx ; 02/24/2013
		if(UpiActiveUg31xx() != SYSTEM_RTN_PASS)
		{
			return (UPI_BOOT_RTN_UG31XX_NOT_ACTIVE);
		}

		/// [AT-PM] : Initialize ADC ; 02/24/2013
		UpiSetupAdc(&obj->sysData);

		/// [AT-PM] : Initialize system ; 02/24/2013
		UpiSetupSystem(&obj->sysData);
	}

	/// [AT-PM] : Get OTP data ; 02/24/2013
	API_I2C_Read(SECURITY, UG31XX_I2C_HIGH_SPEED_MODE, UG31XX_I2C_TEM_BITS_MODE, OTP1_BYTE1, OTP1_SIZE, obj->otpData.otp1);
       	API_I2C_Read(SECURITY, UG31XX_I2C_HIGH_SPEED_MODE, UG31XX_I2C_TEM_BITS_MODE, OTP2_BYTE1, OTP2_SIZE, obj->otpData.otp2);
        API_I2C_Read(NORMAL, UG31XX_I2C_HIGH_SPEED_MODE, UG31XX_I2C_TEM_BITS_MODE, OTP6_BYTE1, OTP3_SIZE, obj->otpData.otp3);
       	#ifdef  UPI_UBOOT_DEBUG_MSG
                printf("[UpiBootInitial] %02x %02x %02x %02x\n",
               	        obj->otpData.otp1[0], obj->otpData.otp1[1], obj->otpData.otp1[2], obj->otpData.otp1[3]);
       	        printf("[UpiBootInitial] %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
                        obj->otpData.otp2[0], obj->otpData.otp2[1], obj->otpData.otp2[2], obj->otpData.otp2[3],
                       	obj->otpData.otp2[4], obj->otpData.otp2[5], obj->otpData.otp2[6], obj->otpData.otp2[7],
               	        obj->otpData.otp2[8], obj->otpData.otp2[9], obj->otpData.otp2[10], obj->otpData.otp2[11],
       	                obj->otpData.otp2[12], obj->otpData.otp2[13], obj->otpData.otp2[14], obj->otpData.otp2[15]);
                printf("[UpiBootInitial] %02x %02x %02x %02x\n",
               	        obj->otpData.otp3[0], obj->otpData.otp3[1], obj->otpData.otp3[2], obj->otpData.otp3[3]);
       	#endif  ///< end of UPI_UBOOT_DEBUG_MSG
	UpiConvertOtp(&obj->otpData);
	if(obj->otpData.productType != UG31XX_PRODUCT_TYPE_0)
	{
		//dprintf(CRITICAL, "[UpiBootInitial] uG31xx product type is not %d (%d)\n", UG31XX_PRODUCT_TYPE_0, obj->otpData.productType);
		printf("[UpiBootInitial] uG31xx product type is not %d (%d)\n", UG31XX_PRODUCT_TYPE_0, obj->otpData.productType);
		obj->status = obj->status | UG31XX_BOOT_STATUS_WRONG_PRODUCT_TYPE;
		return (UPI_BOOT_RTN_INVALID_PRODUCT_TYPE);
	}

	/// [AT-PM] : Initialize UG31xxInternalDataType ; 02/24/2013
	if(obj->status & (UG31XX_BOOT_STATUS_FCC_IS_0 | UG31XX_BOOT_STATUS_IC_IS_NOT_ACTIVE))
	{
		/// [AT-PM] : Data for IC not active ; 02/24/2013
		obj->measData.lastDeltaCap = 0;
		obj->measData.adc1ConvertTime = DEFAULT_ADC1_CONV_TIME;
		/// [AT-PM] : Do measurement ; 02/24/2013
		obj->measData.lastTimeTick = DEFAULT_TIME_TICK;
		UpiMeasurement(&obj->measData);
                data->volt = (int)obj->measData.bat1Voltage;
                data->curr = (int)obj->measData.curr;
                data->intTemp = (int)obj->measData.intTemperature;
                data->extTemp = (int)obj->measData.extTemperature;
		/// [AT-PM] : Look up initial capacity from table ; 02/24/2013
		InitCharge(obj);
		obj->sysData.rmFromIC = (_sys_u16_)obj->info->rm;
		obj->sysData.fccFromIC = (_sys_u16_)obj->info->fcc;
		obj->sysData.rsocFromIC = (_sys_u8_)obj->info->rsoc;
		//dprintf(INFO, "[UpiBootInitial] (Lookup Table) %d / %d / %d - %d / %d = %d\n", data->volt, data->curr, data->intTemp, data->rm, data->fcc, data->rsoc);
		printf("[UpiBootInitial] (Lookup Table) %d / %d / %d - %d / %d = %d\n", data->volt, data->curr, data->intTemp, data->rm, data->fcc, data->rsoc);
	}
	else
	{
		/// [AT-PM] : Data for IC active ; 02/24/2013
		obj->measData.lastDeltaCap = obj->sysData.deltaCapFromIC;
		obj->measData.adc1ConvertTime = obj->sysData.adc1ConvTime;
		/// [AT-PM] : Do measurement ; 02/24/2013
		obj->measData.lastTimeTick = DEFAULT_TIME_TICK;
		UpiMeasurement(&obj->measData);
		data->volt = (int)obj->measData.bat1Voltage;
		data->curr = (int)obj->measData.curr;
		data->intTemp = (int)obj->measData.intTemperature;
		data->extTemp = (int)obj->measData.extTemperature;
		/// [AT-PM] : Set capacity information ; 02/24/2013
		obj->info->rm = (int)obj->sysData.rmFromIC;
		obj->info->fcc = (int)obj->sysData.fccFromIC;
		obj->info->rsoc = (int)obj->sysData.rsocFromIC;
		//dprintf(INFO, "[UpiBootInitial] (Load From IC) %d / %d / %d - %d / %d = %d\n", data->volt, data->curr, data->intTemp, data->rm, data->fcc, data->rsoc);
		printf("[UpiBootInitial] (Load From IC) %d / %d / %d - %d / %d = %d\n", data->volt, data->curr, data->intTemp, data->rm, data->fcc, data->rsoc);
	}
	return (UPI_BOOT_RTN_PASS);
}

/**
 * @brief       UpiBootMain
 *
 *      Main function of uG31xx
 *
 * @para        data    address of UG31xxDataType
 * @return      UPI_BOOT_RTN
 */
int UpiBootMain(UG31xxDataType *data)
{
	UG31xxDataInternalType *obj;
	MEAS_RTN_CODE rtn;

	//dprintf(INFO, "[UpiBootMain]\n");
	//printf("[UpiBootMain]\n");
	obj = _UPI_NULL_;
	SetMemoryMapping(data, &obj);

	/// [AT-PM] : Do measurement ; 02/24/2013
	obj->measData.lastTimeTick = DEFAULT_TIME_TICK;
	rtn = UpiMeasurement(&obj->measData);
	if(rtn != MEAS_RTN_PASS)
	{
		//dprintf(INFO, "[UpiBootMain] uG31xx measurement routine fail 0x%02x\n", rtn);
		printf("[UpiBootMain] uG31xx measurement routine fail 0x%02x\n", rtn);

		data->rm = 0;
		data->fcc = 0;
		data->rsoc = 0;
		return (rtn);
	}

        /// [AT-PM] : Update capacity ; 02/13/2013
        obj->sysData.voltage = obj->measData.bat1Voltage;
        if((obj->measData.curr < 0) && (obj->measData.stepCap > 0))
	{
		obj->measData.stepCap = 0;
	}
	else if((obj->measData.curr > 0) && (obj->measData.stepCap < 0))
	{
		obj->measData.stepCap = 0;
	}
        UpiUpdateBatInfoFromIC(&obj->sysData, obj->measData.stepCap);

	/// [AT-PM] : Reset coulomb counter ; 02/24/2013
	obj->measData.lastTimeTick = DEFAULT_TIME_TICK;
	if(obj->measData.deltaCap > 10)
	{
		UpiResetCoulombCounter(&obj->measData);
	}

	//dprintf(INFO, "[UpiBootMain] %d / %d / %d\n", obj->measData.codeBat1, obj->measData.codeCurrent, obj->measData.codeIntTemperature);
	//dprintf(INFO, "[UpiBootMain] %d - %d / %d - %d\n", obj->measData.adc1Gain, obj->measData.adc1Offset, obj->measData.adc2Gain, obj->measData.adc2Offset);
	//dprintf(INFO, "[UpiBootMain] Voltage : %d - %d\n", obj->ggbParameter.adc2_gain, obj->ggbParameter.adc2_offset);
	//dprintf(INFO, "[UpiBootMain] Current : %d - %d - %d\n", obj->ggbParameter.adc1_pgain, obj->ggbParameter.adc1_ngain, obj->ggbParameter.adc1_pos_offset);
	//dprintf(INFO, "[UpiBootMain] Int. T : %d\n", obj->ggbParameter.adc_d3);

	//printf("[UpiBootMain] %d / %d / %d\n", obj->measData.codeBat1, obj->measData.codeCurrent, obj->measData.codeIntTemperature);
	//printf("[UpiBootMain] %d - %d / %d - %d\n", obj->measData.adc1Gain, obj->measData.adc1Offset, obj->measData.adc2Gain, obj->measData.adc2Offset);
	//printf("[UpiBootMain] Voltage : %d - %d\n", obj->ggbParameter.adc2_gain, obj->ggbParameter.adc2_offset);
	//printf("[UpiBootMain] Current : %d - %d - %d\n", obj->ggbParameter.adc1_pgain, obj->ggbParameter.adc1_ngain, obj->ggbParameter.adc1_pos_offset);
	//printf("[UpiBootMain] Int. T : %d\n", obj->ggbParameter.adc_d3);

	data->volt = (int)obj->measData.bat1Voltage;
	data->curr = (int)obj->measData.curr;
	data->intTemp = (int)obj->measData.intTemperature;
	data->extTemp = (int)obj->measData.extTemperature;

	/// [AT-PM] : Full charge determination ; 02/01/2013
	FullChargeCheck(obj);

	/// [AT-PM] : Refresh capacity information ; 02/24/2013
	data->rm = (int)obj->sysData.rmFromIC;
	data->fcc = (int)obj->sysData.fccFromIC;
	data->rsoc = (int)obj->sysData.rsocFromIC;
	//dprintf(INFO, "[UpiBootMain] %d / %d / %d - %d / %d = %d\n", data->volt, data->curr, data->intTemp, data->rm, data->fcc, data->rsoc);
	//printf("[UpiBootMain] %d / %d / %d - %d / %d = %d\n", data->volt, data->curr, data->intTemp, data->rm, data->fcc, data->rsoc);

	/// [AT-PM] : Save battery information back to IC ; 02/24/2013
	if(obj->status & (UG31XX_BOOT_STATUS_FCC_IS_0 | UG31XX_BOOT_STATUS_IC_IS_NOT_ACTIVE))
	{
		obj->sysData.fccFromIC = 0;
	}
	UpiSaveBatInfoTOIC(&obj->sysData);
	obj->sysData.fccFromIC = (_sys_u16_)data->fcc;
#if 0
	printf("battery volt = %d\n", data->volt);
	printf("battery curr = %d\n", data->curr);
	printf("battery intTemp = %d\n", data->intTemp);
	printf("battery extTemp = %d\n", data->extTemp);
	printf("battery rm = %d\n", data->rm);
	printf("battery fcc = %d\n", data->fcc);
	printf("battery rsoc = %d\n", data->rsoc);
#endif
	return (UPI_BOOT_RTN_PASS);
}

/**
 * @brief       UpiBootUnInitial
 *
 *      Un-initialize uG31xx
 *
 * @para        data    address of UG31xxDataType
 * @return      UPI_BOOT_RTN
 */
int UpiBootUnInitial(UG31xxDataType *data)
{
	printf("[UpiBootUninitial]\n");

	free(data->buf);
	return (UPI_BOOT_RTN_PASS);
}

/*
 * WMT MCE: Use gpio3 on ug31xx as a switch to control charger.
 */
static void hw_charging_set(bool enable)
{
	u8 data = 0;
	API_I2C_SingleWrite(0, 0, 0, 0x16, enable ? 0x2 : 0x0);

//	API_I2C_SingleRead(0, 0, 0, 0x16, &data);
//	printf("enable %d, 0x%x, temperature %d\n", enable, data, gauge_dev_info.ET);
}

static void inline hw_charging_disable(void)
{
	hw_charging_set(false);
}

static void inline hw_charging_enable(void)
{
	hw_charging_set(true);
}

static unsigned short percentage = -1;

int upi_read_percentage(void)
{
	UpiBootMain(&ug31_data);
	percentage = ug31_data.rsoc;

	printf("%s: percentage = %d, temperature %d\n", __FUNCTION__, percentage, ug31_data.extTemp);
	if (ug31xx_param.external_temperature) {
		if (ug31_data.extTemp < ug31xx_param.charge_temperature_range[0] ||
		    ug31_data.extTemp > ug31xx_param.charge_temperature_range[1]) {
			hw_charging_disable();
		} else {
			hw_charging_enable();
		}
		printf("%s: percentage = %d, temperature %d\n", __FUNCTION__, percentage, ug31_data.extTemp);
	}

	return percentage;
}

int upi_read_voltage(void)
{
   int volt_mV;

   UpiBootMain(&ug31_data);
   volt_mV = ug31_data.volt;
   printf("voltage = %d mV\n", volt_mV);

    return volt_mV;
}

int upi_read_current(void)
{
   int current;

   UpiBootMain(&ug31_data);
   current = ug31_data.curr;
   printf("current = %d mA\n", current);

    return current;
}

int upi_boot_init(void)
{
    int ret;

    parse_ug31xx_param();
    ug31xx_init_i2c(ug31xx_param.i2c_adapter);

    ret = UpiBootInitial(&ug31_data);

    return (ret == UPI_BOOT_RTN_PASS) ? 0 : -1;
}

int upi_check_bl(void)
{
	int ret;

	if (percentage < 0) {
		upi_read_percentage();
	}

	ret = (percentage < 3);
	printf("%s: percentage = %d, temperature %d, ret %d\n",
	       __FUNCTION__, ug31_data.rsoc, ug31_data.extTemp, ret);

	return ret;
}

struct bat_dev ug31xx_battery_dev = {
		.name		= "ug31xx",
		.is_gauge	= 1,
		.init		= upi_boot_init,
		.get_capacity	= upi_read_percentage,
		.get_voltage	= upi_read_voltage,
		.get_current	= upi_read_current,
		.check_bl	= upi_check_bl,
};
#endif
