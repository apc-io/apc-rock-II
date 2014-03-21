/*++ 
Copyright (c) 2013 WonderMedia Technologies, Inc.

This program is free software: you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free Software 
Foundation, either version 2 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details. You
should have received a copy of the GNU General Public License along with this
program. If not, see http://www.gnu.org/licenses/>.

WonderMedia Technologies, Inc.
10F, 529, Chung-Cheng Road, Hsin-Tien, Taipei 231, R.O.C.
--*/

#include "./include/i2c.h"
#include "./include/bq_battery_i2c.h"
#define I2C_BUS_ID 3

static int bq_i2c_read(unsigned char reg, unsigned char *rt_value, unsigned int len)
{
	struct i2c_msg_s msg[1];
	unsigned char data[2];
	int err;

	msg->addr = BQ_I2C_DEFAULT_ADDR;
	msg->flags = 0;
	msg->len = 1;
	msg->buf = data;

	data[0] = reg;
	err = wmt_i2c_transfer(msg, 1, I2C_BUS_ID);

	if (err >= 0) {
		msg->len = len;
		msg->flags = I2C_M_RD; 
		msg->buf = rt_value;
		err = wmt_i2c_transfer(msg, 1, I2C_BUS_ID);
	}
	return err;
}

static unsigned short bq_battery_read_percentage(void)
{
	unsigned short ret = 0;
	unsigned char value[2];
	value[0] = 0;
	value[1] = 0;
        bq_i2c_read(BQ_REG_SOC, value, 2);
	ret = value[1] << 8 | value[0];
	
        return ret;
}
