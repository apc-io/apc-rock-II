/*
 * Copyright (c) 2012, ASUSTek, Inc. All Rights Reserved.
 */
#include <linux/i2c.h>

#define SECURITY_KEY	0x5A	//i2c read/write 
#define ONE_BYTE	0x1
#define TWO_BYTE	0x0

void ug31xx_i2c_client_set(struct i2c_client *client);
int ug31xx_read_i2c(struct i2c_client *client, u8 reg, int *rt_value, int b_single);
int ug31xx_write_i2c(struct i2c_client *client, u8 reg, int rt_value, int b_single);

bool _API_I2C_Write(u16 writeAddress, u8 writeLength, u8 *PWriteData);
bool _API_I2C_Read(u16 readAddress, u8 readLength, u8 *pReadDataBuffer);

static inline bool API_I2C_Read(bool bSecurityMode, bool bHighSpeedMode, 
                bool bTenBitMode ,u16 readAddress, u8 readLength, u8 *pReadDataBuffer) 
{
        return _API_I2C_Read(readAddress, readLength, pReadDataBuffer);
}

static inline bool API_I2C_Write(bool bSecurityMode, bool bHighSpeedMode, bool bTenBitMode,
                u16 writeAddress, u8 writeLength,  u8 *PWriteData) 
{
        return _API_I2C_Write(writeAddress, writeLength, PWriteData);
}

static inline bool API_I2C_SingleRead(bool bSecurityMode,bool bHighSpeedMode, bool bTenBitMode ,
                u16 readAddress, u8 *ReadData) 
{
        return API_I2C_Read(bSecurityMode, bHighSpeedMode, bTenBitMode, readAddress, 1, ReadData);
}

static inline bool API_I2C_SingleWrite(bool bSecurityMode, bool bHighSpeedMode, bool bTenBitMode ,
                u16 writeAddress, u8 WriteData) 
{
        return API_I2C_Write(bSecurityMode, bHighSpeedMode, bTenBitMode, writeAddress, 1, &WriteData);
}
static inline bool API_I2C_Init(u32 clockRate, u16 slaveAddr)
{
        return true;
} 
