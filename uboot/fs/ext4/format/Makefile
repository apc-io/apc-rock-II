#
# (C) Copyright 2006
# Wolfgang Denk, DENX Software Engineering, wd@denx.de.
#
# (C) Copyright 2003
# Pavel Bartusek, Sysgo Real-Time Solutions AG, pba@sysgo.de
#
#
# See file CREDITS for list of people who contributed to this
# project.
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation; either version 2 of
# the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston,
# MA 02111-1307 USA
#


include $(TOPDIR)/config.mk

LIB	= libext4fsformat.a

AOBJS	=
COBJS	= ext4_format.o freefs.o bitmaps.o gen_bitmap.o bitops.o rw_bitmaps.o \
          csum.o crc16.o alloc_sb.o closefs.o io_manager.o alloc_tables.o alloc.o \
          alloc_stats.o mkjournal.o mkdir.o newdir.o inode.o i_block.o dirblock.o \
          lookup.o link.o dir_iterate.o block.o ind_block.o extent.o expanddir.o \
          res_gdt.o bb_inode.o badblocks.o
OBJS	= $(AOBJS) $(COBJS)

#CPPFLAGS +=

all:	$(LIB) $(AOBJS)

$(LIB):	.depend $(OBJS)
	$(AR) crv $@ $(OBJS)


#########################################################################

.depend:	Makefile $(AOBJS:.o=.S) $(COBJS:.o=.c)
		$(CC) -M $(CFLAGS) $(AOBJS:.o=.S) $(COBJS:.o=.c) > $@

sinclude .depend

#########################################################################
