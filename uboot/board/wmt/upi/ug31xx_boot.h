/**
 * @filename	ug31xx_boot.h
 *
 *	uG31xx API for bootloader
 *
 * @author	AllenTeng <allen_teng@upi-semi.com>
 */

#ifndef	_UG31XX_BOOT_H_
#define	_UG31XX_BOOT_H_

enum UPI_BOOT_RTN {
	UPI_BOOT_RTN_PASS = 0,
	UPI_BOOT_RTN_UG31XX_NOT_ACTIVE,
	UPI_BOOT_RTN_INVALID_CAPACITY,
	UPI_BOOT_RTN_INVALID_PRODUCT_TYPE,
};

typedef struct UG31xxDataST {
	//struct qup_i2c_dev *ug31xx_i2c_dev;
	
	int	version;

	int	rm;		///< [AT-PM] : in unit mAh ; 02/01/2013
	int	fcc;		///< [AT-PM] : in unit mAh ; 02/01/2013
	int	rsoc;		///< [AT-PM] : in unit % ; 02/01/2013

	int	volt;		///< [AT-PM] : in unit mV ; 02/01/2013
	int	curr;		///< [AT-PM] : in unit mA ; 02/01/2013
	int	intTemp;	///< [AT-PM] : in unit 0.1oC ; 02/01/2013
	int	extTemp;	///< [AT-PM] : in unit 0.1oC ; 02/01/2013

	char	*buf;
} UG31xxDataType;

/**
 * @brief	UpiBootInitial
 *
 *	Initialize uG31xx
 *
 * @para	data	address of UG31xxDataType
 * @return	UPI_BOOT_RTN_PASS
 */
extern int UpiBootInitial(UG31xxDataType *data);

/**
 * @brief	UpiBootMain
 *
 *	Main function of uG31xx
 *
 * @para	data	address of UG31xxDataType
 * @return	UPI_BOOT_RTN_PASS
 */
extern int UpiBootMain(UG31xxDataType *data);

/**
 * @brief	UpiBootUnInitial
 *
 *	Un-initialize uG31xx
 *
 * @para	data	address of UG31xxDataType
 * @return	UPI_BOOT_RTN_PASS
 */
extern int UpiBootUnInitial(UG31xxDataType *data);

extern int upi_read_percentage(void);

extern int upi_boot_init(void);

#endif	///< end of _UG31XX_BOOT_H_

