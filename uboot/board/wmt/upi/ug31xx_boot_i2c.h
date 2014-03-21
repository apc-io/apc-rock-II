/**
 * @filename	ug31xx_boot_i2c.h
 *
 *	Interface of I2C in bootloader
 *
 * @author	AllenTeng <allen_teng@upi-semi.com>
 */

#ifndef	_UG31XX_BOOT_I2C_H_
#define	_UG31XX_BOOT_I2C_H_

//#include <i2c_qup.h>
/* I2C Message - used for pure i2c transaction*/
/**/
/* flags*/
#define I2C_M_RD                0x01
#define I2C_M_WR                0x00
unsigned short bq_battery_read_percentage(void);

struct i2c_msg_s {
    unsigned short addr;        /* slave address*/
    unsigned short flags;       /* flags*/
    unsigned short len;         /* msg length*/
    unsigned char *buf;        /* pointer to msg data*/
} ;


#define SECURITY_KEY	0x5A	//i2c read/write 
#define ONE_BYTE	0x1
#define TWO_BYTE	0x0

#define _i2c_bool		unsigned char
#define _i2c_true		(1)
#define _i2c_false	(0)

//void ug31xx_init_i2c(struct qup_i2c_dev *i2c_dev);
//void ug31xx_init_i2c(void);

int ug31xx_read_i2c(unsigned char reg, int *rt_value);
int ug31xx_write_i2c(unsigned char reg, int rt_value);

_i2c_bool _API_I2C_Write(unsigned short writeAddress, unsigned char writeLength, unsigned char *PWriteData);
_i2c_bool _API_I2C_Read(unsigned short readAddress, unsigned char readLength, unsigned char *pReadDataBuffer);

static inline _i2c_bool API_I2C_Read(_i2c_bool bSecurityMode, _i2c_bool bHighSpeedMode, 
                _i2c_bool bTenBitMode ,unsigned short readAddress, unsigned char readLength, unsigned char *pReadDataBuffer) 
{
        return _API_I2C_Read(readAddress, readLength, pReadDataBuffer);
}

static inline _i2c_bool API_I2C_Write(_i2c_bool bSecurityMode, _i2c_bool bHighSpeedMode, _i2c_bool bTenBitMode,
                unsigned short writeAddress, unsigned char writeLength,  unsigned char *PWriteData) 
{
        return _API_I2C_Write(writeAddress, writeLength, PWriteData);
}

static inline _i2c_bool API_I2C_SingleRead(_i2c_bool bSecurityMode,_i2c_bool bHighSpeedMode, _i2c_bool bTenBitMode ,
                unsigned short readAddress, unsigned char *ReadData) 
{
        return API_I2C_Read(bSecurityMode, bHighSpeedMode, bTenBitMode, readAddress, 1, ReadData);
}

static inline _i2c_bool API_I2C_SingleWrite(_i2c_bool bSecurityMode, _i2c_bool bHighSpeedMode, _i2c_bool bTenBitMode ,
                unsigned short writeAddress, unsigned char WriteData) 
{
        return API_I2C_Write(bSecurityMode, bHighSpeedMode, bTenBitMode, writeAddress, 1, &WriteData);
}

static inline _i2c_bool API_I2C_Init(unsigned long clockRate, unsigned short slaveAddr)
{
        return _i2c_true;
}

#endif	///< end of _UG31XX_BOOT_I2C_H_

