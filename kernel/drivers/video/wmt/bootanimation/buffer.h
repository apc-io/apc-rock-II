/*++
	linux/drivers/video/wmt/buffer.h

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

#ifndef ANIMATION_BUFFER_H_INCLUDED
#define ANIMATION_BUFFER_H_INCLUDED

#include "LzmaDec.h"
#include <linux/semaphore.h>

typedef struct 
{
    unsigned char *buffer;
    unsigned char *w_pos;
    unsigned char *r_pos;
    int frame_count;
    int frame_size;
    struct semaphore sem_writable;
    struct semaphore sem_readable;
    int     eof;
}animation_buffer;
    
                              
int animation_buffer_init(animation_buffer * buf, int size, int count, ISzAlloc *alloc);

int animation_buffer_stop(animation_buffer * buf);
int animation_buffer_release(animation_buffer * buf, ISzAlloc *alloc);


unsigned char * animation_buffer_get_writable(animation_buffer * buf, unsigned int * pSize);
void            animation_buffer_write_finish(animation_buffer * buf, unsigned char * addr);

unsigned char * animation_buffer_get_readable(animation_buffer * buf);
void            animation_buffer_read_finish(animation_buffer * buf, unsigned char * addr);


#endif /* ANIMATION_BUFFER_H_INCLUDED */

