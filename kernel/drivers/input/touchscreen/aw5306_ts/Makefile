KERNELDIR=../../../../
CROSS = arm_1103_le-
CC=		$(CROSS)gcc
LD=		$(CROSS)ld
STRIP = $(CROSS)strip

DEBUG = n 

# Add your debugging flag (or not) to EXTRA_CFLAGS
ifeq ($(DEBUG),y)
#  DEBFLAGS = -O -g -DSCULL_DEBUG # "-O" is needed to expand inlines
DEBFLAGS = -O0 -g -DSCULL_DEBUG  # "-O" is needed to expand inlines

else
  DEBFLAGS = -O2 -Wall 
  #DEBFLAGS = -O2 -Wall  -L./libAW5306.a
endif

EXTRA_CFLAGS += $(DEBFLAGS)


MY_MODULE_NAME=s_wmt_ts_aw5306

#obj-$(CONFIG_TOUCHSCREEN_FT5X0X) := $(MY_MODULE_NAME).o
obj-m := $(MY_MODULE_NAME).o
$(MY_MODULE_NAME)-objs  := AW5306_ts.o irq_gpio.o AW5306_userpara.o AW5306_Base.b  AW5306_Clb.b  AW5306_Drv.b

default:
	$(MAKE) -C $(KERNELDIR) SUBDIRS=$(PWD) modules
	$(STRIP) --strip-debug $(MY_MODULE_NAME).ko
#	@rm -rf *.o *~ core .depend .*.cmd *.mod.c .tmp_versions *.order *.symvers

clean:
	@rm -rf *.o *~ core .depend .*.cmd *.ko *.mod.c .tmp_versions *.order *.symvers modules.builtin

