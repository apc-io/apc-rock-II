#
# Makefile for the video capture/playback device drivers.
#

LOCAL_MODULE_NAME = g_wmt_v4l2_cmos

## sensors
${LOCAL_MODULE_NAME}-objs += gc0307/gc0307.o
${LOCAL_MODULE_NAME}-objs += gc0308/gc0308.o
${LOCAL_MODULE_NAME}-objs += gc0309/gc0309.o
${LOCAL_MODULE_NAME}-objs += gc0328/gc0328.o
${LOCAL_MODULE_NAME}-objs += gc0329/gc0329.o
${LOCAL_MODULE_NAME}-objs += gc2035/gc2035.o
${LOCAL_MODULE_NAME}-objs += ov7675/ov7675.o
${LOCAL_MODULE_NAME}-objs += ov2659/ov2659.o
${LOCAL_MODULE_NAME}-objs += ov5640/ov5640.o
${LOCAL_MODULE_NAME}-objs += ov9740/ov9740.o
${LOCAL_MODULE_NAME}-objs += hi253/hi253.o
${LOCAL_MODULE_NAME}-objs += hi257/hi257.o
${LOCAL_MODULE_NAME}-objs += siv121d/siv121d.o
${LOCAL_MODULE_NAME}-objs += siv120d/siv120d.o
${LOCAL_MODULE_NAME}-objs += sid130b/sid130b.o
${LOCAL_MODULE_NAME}-objs += sp0718/sp0718.o
${LOCAL_MODULE_NAME}-objs += sp0a19/sp0a19.o
${LOCAL_MODULE_NAME}-objs += sp0838/sp0838.o
${LOCAL_MODULE_NAME}-objs += sp2518/sp2518.o
${LOCAL_MODULE_NAME}-objs += bf3a03/bf3a03.o
${LOCAL_MODULE_NAME}-objs += bf3920/bf3920.o

## flash
${LOCAL_MODULE_NAME}-objs += flash/flash.o
${LOCAL_MODULE_NAME}-objs += flash/flash_gpio.o
${LOCAL_MODULE_NAME}-objs += flash/flash_eup2471.o

${LOCAL_MODULE_NAME}-objs += wmt-cmos.o cmos-subdev.o
obj-m += ${LOCAL_MODULE_NAME}.o

