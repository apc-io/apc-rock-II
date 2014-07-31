#
# Makefile for the linux kernel.
#

# Common support
obj-y   := generic.o irq.o board.o wmt_clk.o dma.o wmt_time.o wmt_reset.o wmt_smc.o gpio.o \
	   wmt_misc.o \
	   pwm.o

plus_sec := $(call as-instr,.arch_extension sec,+sec)
AFLAGS_wmt-smc.o                       :=-Wa,-march=armv7-a$(plus_sec)
obj-$(CONFIG_PM)		+= pm.o pm_cpai.o sleep.o

# CPUFreq support
obj-$(CONFIG_ARM_WMT_CPUFREQ)	+= wmt_cpufreq.o
obj-$(CONFIG_CPU_IDLE)			+= wmt_cpuidle.o
obj-$(CONFIG_SMP)		+= platsmp.o headsmp.o
obj-$(CONFIG_HOTPLUG_CPU)	+= hotplug.o
obj-$(CONFIG_COMMON_CLK)	+= wmt_clock.o
