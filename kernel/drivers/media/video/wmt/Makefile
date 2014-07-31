#
# Makefile for the video capture/playback device drivers.
#

g_wmt_vd_com-y      := wmt-vd.o wmt-codec.o
g_wmt_lock-y        := wmt-lock.o
g_wmt_msvd-y        := msvd/vpu.o
g_wmt_vd_jdec-y     := jpeg/hw-jdec.o jpeg/wmt-jdec.o

obj-$(CONFIG_WMT_VIDEO_LOCK) += g_wmt_lock.o
obj-$(CONFIG_WMT_VIDEO_DECODE) += g_wmt_vd_com.o
obj-$(CONFIG_WMT_VIDEO_DECODE_MSVD) += g_wmt_msvd.o
obj-$(CONFIG_WMT_VIDEO_DECODE_JPEG) += g_wmt_vd_jdec.o
