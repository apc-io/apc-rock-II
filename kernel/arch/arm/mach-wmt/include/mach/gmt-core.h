/*++
	drivers/mtd/gmt/gmt-core.c - GMT Core driver

	Copyright (c) 2013  WonderMedia Technologies, Inc.

	This program is free software: you can redistribute it and/or modify it under the
	terms of the GNU General Public License as published by the Free Software Foundation,
	either version 2 of the License, or (at your option) any later version.

	This program is distributed in the hope that it will be useful, but WITHOUT
	ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
	PARTICULAR PURPOSE.  See the GNU General Public License for more details.
	You should have received a copy of the GNU General Public License along with
	this program.  If not, see <http://www.gnu.org/licenses/>.

	WonderMedia Technologies, Inc.
	10F, 529, Chung-Cheng Road, Hsin-Tien, Taipei 231, R.O.C.
--*/
enum gmt_device_type {
	GMT2214,
};

/**
 * @dev: master device of the chip (can be used to access platform data)
 * @i2c: i2c client private data for regulator
 * @iolock: mutex for serializing io access
 * @irqlock: mutex for buslock
 */
struct gmt2214_dev {
	struct device *dev;
	struct regmap *regmap;
	struct i2c_client *i2c;
	struct mutex iolock;

	int device_type;
};

extern int gmt2214_reg_read(struct gmt2214_dev *gmt2214, u8 reg, void *dest);
extern int gmt2214_reg_write(struct gmt2214_dev *gmt2214, u8 reg, u8 value);

struct gmt2214_platform_data {
	int				device_type;
};
