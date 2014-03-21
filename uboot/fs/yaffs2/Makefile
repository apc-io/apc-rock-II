# Makefile for YAFFS direct test
#
#
# YAFFS: Yet another Flash File System. A NAND-flash specific file system.
#
# Copyright (C) 2003 Aleph One Ltd.
#
#
# Created by Charles Manning <charles@aleph1.co.uk>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 2 as
# published by the Free Software Foundation.
#
# NB Warning this Makefile does not include header dependencies.
#
# $Id: Makefile,v 1.15 2007/07/18 19:40:38 charles Exp $

#EXTRA_COMPILE_FLAGS = -DYAFFS_IGNORE_TAGS_ECC
include $(TOPDIR)/config.mk

LIB = libyaffs2.a

AOBJS	=
COBJS	= \
	yaffscfg.o yaffsfs.o yaffs_guts.o \
	yaffs_packedtags2.o yaffs_tagsvalidity.o \
	yaffs_nand.o yaffs_checkptrw.o \
	yaffs_mtdif2.o

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
