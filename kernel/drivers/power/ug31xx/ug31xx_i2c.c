/*
 * Copyright (c) 2012, ASUSTek, Inc. All Rights Reserved.
 */
#include <linux/module.h>
#include <linux/param.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <asm/unaligned.h>
#include <linux/interrupt.h>
#include <linux/idr.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include "ug31xx_i2c.h"

static struct i2c_client *ug31xx_client = NULL;

void ug31xx_i2c_client_set(struct i2c_client *client)
{
	ug31xx_client = client;
	dev_info(&ug31xx_client->dev, "%s: Ug31xx i2c client saved.\n", __func__);
}

int ug31xx_read_i2c(struct i2c_client *client, 
                u8 reg, int *rt_value, int b_single)
{
	struct i2c_msg msg[2];
	unsigned char data[4];
	int err;

	if (!client || !client->adapter)
		return -ENODEV;

	if (!rt_value) return -EINVAL;

	data[0] = reg;
	//err = i2c_transfer(client->adapter, msg, 1);

  msg[0].addr = client->addr;
  msg[0].flags = 0 | I2C_M_NOSTART;
  msg[0].len = 1;
  if (reg >= 0x80) {
          data[1] = SECURITY_KEY;
          msg[0].len++;
  }
  msg[0].buf = (unsigned char *)data;

  msg[1].addr = client->addr;
  msg[1].flags = (I2C_M_RD);
  msg[1].len = b_single ? 1 : 2;
  msg[1].buf = (unsigned char *)data;

  err = i2c_transfer(client->adapter, msg, sizeof(msg)/sizeof(struct i2c_msg));

  if (err < 0) return err;

  if (b_single)
		*rt_value = data[0];
  else 
		*rt_value = get_unaligned_le16(data);

  return 0;
}

int ug31xx_write_i2c(struct i2c_client *client, 
                u8 reg, int rt_value, int b_single)
{
	struct i2c_msg msg[1];
	unsigned char data[4];
	int err;
	int idx;
	int tmp_buf=0;

	if (!client || !client->adapter)
		return -ENODEV;

  idx = 0;
	data[idx++] = reg;
  if (reg >= 0x80) {
          data[idx++] = SECURITY_KEY;
  }
  data[idx++] = rt_value & 0x0FF;
  data[idx++] = (rt_value & 0x0FF00)>>8;

  msg[0].addr = client->addr;
  msg[0].flags = 0 | I2C_M_NOSTART;
  msg[0].len = b_single ? idx-1 : idx;
  msg[0].buf = (unsigned char *)data;

  err = i2c_transfer(client->adapter, msg, sizeof(msg)/sizeof(struct i2c_msg));
  
  if (err >= 0)
          err = ug31xx_read_i2c(client, reg, &tmp_buf, b_single);
  //dev_info(&client->dev, "%s:: 0x%02X, 0x%04X, 0x%04X. %s\n", 
  //        __func__, reg, rt_value, tmp_buf, err < 0 ? "FAIL" : "SUCCESS");

  return err < 0 ? err : 0;
}

bool _API_I2C_Write(u16 writeAddress, u8 writeLength, u8 *PWriteData)
{
  int i, ret;
  int byte_flag=0;

  if (!PWriteData) {
    dev_err(&ug31xx_client->dev, "%s: Write buffer pointer error.\n", __func__);
    return false;
  }

  byte_flag = ONE_BYTE;

  for (i=0; i<writeLength; i++) {
    int tmp_buf = PWriteData[i];

    ret = ug31xx_write_i2c(ug31xx_client, writeAddress+i, tmp_buf, byte_flag);
    if (ret) {
      dev_err(&ug31xx_client->dev, "%s: Write data(0x%02X) fail. %d\n", 
                      __func__, i, ret);
      return false;
    }
  }

  return true;
}

bool _API_I2C_Read(u16 readAddress, u8 readLength, u8 *pReadDataBuffer)
{
  int i, ret;
  int byte_flag=0;

  if (!pReadDataBuffer) {
    dev_err(&ug31xx_client->dev, "%s: Read buffer pointer error.\n", __func__);
    return false;
  }

  byte_flag = ONE_BYTE;

  for (i=0; i<readLength; i++) {
    int tmp_buf=0;

    ret = ug31xx_read_i2c(ug31xx_client, readAddress+i, &tmp_buf, byte_flag);
    if (ret) {
      dev_err(&ug31xx_client->dev, "%s: read data(0x%02X) fail. %d\n", 
              __func__, i, ret);
      return false;
    }
    pReadDataBuffer[i] = tmp_buf;
  }

  return true;
}

