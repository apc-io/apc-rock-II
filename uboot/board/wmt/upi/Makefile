#
# See file CREDITS for list of people who contributed to this
# project.
#
# Some descriptions of such software. Copyright (c) 2008 WonderMedia Technologies, Inc.
#
# This program is free software: you can redistribute it and/or modify it under the
# terms of the GNU General Public License as published by the Free Software Foundation,
# either version 2 of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
# PARTICULAR PURPOSE.  See the GNU General Public License for more details.
# You should have received a copy of the GNU General Public License along with
# this program.  If not, see <http://www.gnu.org/licenses/>.
#
# WonderMedia Technologies, Inc.
# 10F, 529, Chung-Cheng Road, Hsin-Tien, Taipei 231, R.O.C.
#

include $(TOPDIR)/config.mk

LIB	= libbattupi.a

OBJS	:= uG31xx_API_Measurement.o \
	uG31xx_API_Otp.o uG31xx_API_System.o \
	ug31xx_boot.o ug31xx_boot_i2c.o
	
#SOBJS	:= lowlevel_init.o

#$(LIB):	$(OBJS) $(SOBJS)
#	$(AR) crv $@ $^

clean:
	rm -f $(OBJS)

distclean:	clean
	rm -f $(LIB) core *.bak .depend

#########################################################################

.depend:	Makefile $(SOBJS:.o=.S) $(OBJS:.o=.c)
		$(CC) -M $(CPPFLAGS) $(SOBJS:.o=.S) $(OBJS:.o=.c) > $@

-include .depend

#########################################################################
