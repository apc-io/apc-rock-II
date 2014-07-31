/*++
	linux/drivers/video/wmt/buffer.c

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
#include "buffer.h"

#define assert(int )

int animation_buffer_init(animation_buffer * buf, int size, int count, ISzAlloc *alloc)
{
    buf->buffer = buf->w_pos = buf->r_pos = alloc->Alloc(alloc, size * count);
    if(!buf->buffer)
        return -1;
    
	buf->frame_count = count;
	buf->frame_size = size;
    buf->eof = 0;
    
    sema_init(&buf->sem_writable, count);
    sema_init(&buf->sem_readable, 0);
    
    return 0;
}

int animation_buffer_release(animation_buffer * buf, ISzAlloc *alloc)
{
    if (buf->buffer)
		alloc->Free(alloc, buf->buffer);
	buf->buffer = buf->w_pos = buf->r_pos = NULL;
    return 0;
}

unsigned char * animation_buffer_get_writable(animation_buffer * buf, unsigned int * pSize)
{
    down_interruptible(&buf->sem_writable);
	*pSize = buf->frame_size;
	return buf->w_pos;
}

int animation_buffer_stop(animation_buffer * buf)
{
	up(&buf->sem_readable);
    /* up twice for safey*/
	up(&buf->sem_writable);
	up(&buf->sem_writable);
    return 0;
}

void   animation_buffer_write_finish(animation_buffer * buf, unsigned char * addr)
{
    assert(addr == buf->w_pos);
    
 //   printk(KERN_INFO "add one buffer 0x%p\n", addr);
        
	buf->w_pos += buf->frame_size;
	if (buf->w_pos - buf->buffer == buf->frame_size * buf->frame_count) 
		buf->w_pos = buf->buffer;
  
	up(&buf->sem_readable);
 //   printk(KERN_INFO "write <<<<<<<======\n");
	return;
    
}


unsigned char * animation_buffer_get_readable(animation_buffer * buf)
{
    int ret = down_trylock(&buf->sem_readable);
	if (ret == 0) {		
		return buf->r_pos;
	}
	else {
		return NULL;
	}
}

void animation_buffer_read_finish(animation_buffer * buf, unsigned char * addr)
{
    //  Integrity check
	assert(addr == buf->r_pos);
  //  printk(KERN_INFO "read one buffer 0x%p\n", addr);
	buf->r_pos += buf->frame_size;	
	if (buf->r_pos - buf->buffer == buf->frame_size * buf->frame_count) 
		buf->r_pos = buf->buffer;
      
	up(&buf->sem_writable);
}

