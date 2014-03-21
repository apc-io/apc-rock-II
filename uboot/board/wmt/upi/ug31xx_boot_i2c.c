/*
 * Copyright (c) 2012, ASUSTek, Inc. All Rights Reserved.
 */

#include "../../../include/configs/wmt.h"
#ifdef CONFIG_BATT_UPI
//#include <debug.h>
//#include <gsbi.h>
//#include <platform/timer.h>
#include <common.h>
#include "ug31xx_boot_i2c.h"
extern int wmt_i2c_transfer(struct i2c_msg_s msgs[], int num, int adap_id);

//typedef unsigned char  u8;
//typedef unsigned short u16;

#define	UG31XX_I2C_SLAVE_ADDRESS	(0x70)

//static struct qup_i2c_dev *ug31xx_i2c_dev = NULL;
static int ug31xx_i2c_dev;

/**
 * @brief	ug31xx_init_i2c
 *
 *	Initialize I2C for uG31xx
 *
 * @para	ug31xx_i2c_dev	address of qup_i2c_dev
 * @return	NULL
 */
//void ug31xx_init_i2c(struct qup_i2c_dev *i2c_dev)
void ug31xx_init_i2c(int i2c_dev)
{
	ug31xx_i2c_dev = i2c_dev;
}

/**
 * @brief	ug31xx_read_i2c
 *
 *	I2C read from uG31xx
 *
 * @para	ug31xx_i2c_dev	address of qup_i2c_dev
 * @para	reg		register address
 * @para	rt_value	address of returned value
 * @return	0 if pass
 */
int ug31xx_read_i2c(u8 reg, int *rt_value)
{
	int status;
	unsigned char ret;
	unsigned char data[2] = 
	{
		reg,
		SECURITY_KEY,
	};
	struct i2c_msg_s msg_buf1[2] = 
	{
		//{UG31XX_I2C_SLAVE_ADDRESS,	I2C_M_WR | I2C_M_NOSTART,	1,	&data[0],	},
		{	UG31XX_I2C_SLAVE_ADDRESS,	I2C_M_WR,	1,	&data[0],	},
		{	UG31XX_I2C_SLAVE_ADDRESS,	I2C_M_RD,	1,	&ret,		},
	};
	struct i2c_msg_s msg_buf2[2] = 
	{
		//{UG31XX_I2C_SLAVE_ADDRESS,	I2C_M_WR | I2C_M_NOSTART,	2,	&data[0],	},
		{	UG31XX_I2C_SLAVE_ADDRESS,	I2C_M_WR,	2,	&data[0],	},
		{	UG31XX_I2C_SLAVE_ADDRESS,	I2C_M_RD,	1,	&ret,		},
	};

	if(reg < 0x80)
	{
		//status = qup_i2c_xfer(ug31xx_i2c_dev, msg_buf1, 2);
		status = wmt_i2c_transfer(msg_buf1, 2, ug31xx_i2c_dev);
	}
	else
	{
		//status = qup_i2c_xfer(ug31xx_i2c_dev, msg_buf2, 2);
		status = wmt_i2c_transfer(msg_buf2, 2, ug31xx_i2c_dev);
	}

	if(status > 0)
	{
		*rt_value = ret;
		return (0);
	}
	//dprintf(CRITICAL, "[ug31xx_read_i2c][reg=%02x] failed\n", reg);
	printf("[ug31xx_read_i2c][reg=%02x] failed\n", reg);
	return (-1);
}

/**
 * @brief	ug31xx_write_i2c
 *
 *	I2C write to uG31xx
 *
 * @para	ug31xx_i2c_dev	address of qup_i2c_dev
 * @para	reg		register address
 * @para	rt_value	value to be written
 * @return	0 if pass
 */
int ug31xx_write_i2c(u8 reg, int rt_value)
{
	int status;
	unsigned char data1[2] =
	{
		reg,
		rt_value,
	};
	unsigned char data2[3] =
	{
		reg,
		SECURITY_KEY,
		rt_value,
	};
	struct i2c_msg_s msg_buf[2] =
	{
		//{UG31XX_I2C_SLAVE_ADDRESS,I2C_M_WR | I2C_M_NOSTART,	2,	&data1[0],	},
		//{UG31XX_I2C_SLAVE_ADDRESS,I2C_M_WR | I2C_M_NOSTART,	3,	&data2[0],	}
		{	UG31XX_I2C_SLAVE_ADDRESS,	I2C_M_WR,	2,	&data1[0],	},
		{	UG31XX_I2C_SLAVE_ADDRESS,	I2C_M_WR,	3,	&data2[0],	}
	};

	if(reg < 0x80)
	{
		//status = qup_i2c_xfer(ug31xx_i2c_dev, &msg_buf[0], 1);
		status = wmt_i2c_transfer(&msg_buf[0], 1,ug31xx_i2c_dev);
	}
	else
	{
		//status = qup_i2c_xfer(ug31xx_i2c_dev, &msg_buf[1], 1);
		status = wmt_i2c_transfer(&msg_buf[1], 1,ug31xx_i2c_dev);
	}

	if(status > 0)
	{
		return (0);
	}
	//dprintf(CRITICAL, "[ug31xx_write_i2c][reg=%02x] failed.\n", reg);
	return (-1);
}

/** 
 * @brief	_API_I2C_Write
 *
 *	I2C write protocol for uG31xx
 *
 * @para	writeAddress	register address to be written
 * @para	writeLength	data length
 * @para	PWriteData	address of data buffer
 * @return	true if pass
 */
_i2c_bool _API_I2C_Write(u16 writeAddress, u8 writeLength, u8 *PWriteData)
{
  int i, ret;
  int tmp_buf;

  if (!PWriteData)
  {
   // dprintf(CRITICAL, "[_API_I2C_Write]Write buffer pointer error.\n");
      printf("[_API_I2C_Write]Write buffer pointer error.\n");
    return (_i2c_false);
  }

  for (i=0; i<writeLength; i++) {
    tmp_buf = PWriteData[i];

    ret = ug31xx_write_i2c(writeAddress+i, tmp_buf);
    if (ret)
    {
      //dprintf(CRITICAL, "[_API_I2C_Write]Write data (%02x) to address (%02x) fail.\n", tmp_buf, writeAddress + i);
      printf("[_API_I2C_Write]Write data (%02x) to address (%02x) fail.\n", tmp_buf, writeAddress + i);
      return (_i2c_false);
    }
  }

  return (_i2c_true);
}

/**
 * @brief	_API_I2C_Read
 *
 *	I2C read protocol for uG31xx
 * 
 * @para	readAddress	register address to be read
 * @para	readLength	data length
 * @para	pReadDataBuffer	address of data buffer
 * @return	true if pass
 */
_i2c_bool _API_I2C_Read(u16 readAddress, u8 readLength, u8 *pReadDataBuffer)
{
  int i, ret;

  if (!pReadDataBuffer)
  {
    //dprintf(CRITICAL, "[_API_I2C_Read]Read buffer pointer error.\n");
      printf("[_API_I2C_Read]Read buffer pointer error.\n");
    return (_i2c_false);
  }

  for (i=0; i<readLength; i++) {
    int tmp_buf=0;

    ret = ug31xx_read_i2c(readAddress+i, &tmp_buf);
    if (ret)
    {
      //dprintf(CRITICAL, "[_API_I2C_Read]Read data from address (%02x) fail.\n", readAddress + i);
      printf("[_API_I2C_Read]Read data from address (%02x) fail.\n", readAddress + i);
      return (_i2c_false);
    }
    pReadDataBuffer[i] = tmp_buf;
  }

  return (_i2c_true);
}
#endif
