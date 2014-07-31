/*
 * Copyright (c) 2008-2011 WonderMedia Technologies, Inc. All Rights Reserved.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * WonderMedia Technologies, Inc.
 * 4F, 533, Chung-Cheng Road, Hsin-Tien, Taipei 231, R.O.C
 */

#ifndef MALI_H
#define MALI_H
struct mali_device {
	int (*suspend)(u32 cores);
	int (*resume)(u32 cores);
	void (*enable_clock)(int enable);
	void (*enable_power)(int enable);
	void (*set_memory_base)(unsigned int val);
	void (*set_memory_size)(unsigned int val);
	void (*set_mem_validation_base)(unsigned int val);
	void (*set_mem_validation_size)(unsigned int val);
	void (*get_memory_base)(unsigned int *val);
	void (*get_memory_size)(unsigned int *val);
	void (*get_mem_validation_base)(unsigned int *val);
	void (*get_mem_validation_size)(unsigned int *val);
};

struct mali_device *create_mali_device(void);
void release_mali_device(struct mali_device *dev);

int mali_platform_init_impl(void *data);
int mali_platform_deinit_impl(void *data);
int mali_platform_powerdown_impl(u32 cores);
int mali_platform_powerup_impl(u32 cores);
void mali_gpu_utilization_handler_impl(u32 utilization);
void set_mali_parent_power_domain(struct platform_device *dev);

extern unsigned long msleep_interruptible(unsigned int msecs);
#endif /* MALI_H */
