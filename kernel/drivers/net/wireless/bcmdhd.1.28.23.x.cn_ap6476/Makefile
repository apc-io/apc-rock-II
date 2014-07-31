MODULE_NAME = bcmdhd_ap6476

CONFIG_MACH_ODROID_4210 = y
CONFIG_BCMDHD_OOB = n
CONFIG_BCMDHD_AG = n
CONFIG_WIRELESS_EXT = y
CONFIG_CFG80211 = y
CONFIG_DHD_USE_SCHED_SCAN = n

ARCH := arm
CROSS_COMPILE := arm_1103_le-

# WL_SUPPORT_BACKPORTED_KPATCHES should be remove if kernel version is 3.0 and Android version is 4.1
DHDCFLAGS = -Wall -Wstrict-prototypes -Dlinux -DBCMDRIVER                 \
	-DBCMDONGLEHOST -DUNRELEASEDCHIP -DBCMDMA32 -DBCMFILEIMAGE            \
	-DDHDTHREAD -DDHD_DEBUG -DSDTEST -DBDC -DTOE                          \
	-DDHD_BCMEVENTS -DSHOW_EVENTS -DBCMDBG                                \
	-DMMC_SDIO_ABORT -DBCMSDIO -DBCMLXSDMMC -DBCMPLATFORM_BUS             \
	-DWIFI_ACT_FRAME -DARP_OFFLOAD_SUPPORT                                \
	-DKEEP_ALIVE -DPKT_FILTER_SUPPORT                                     \
	-DEMBEDDED_PLATFORM -DENABLE_INSMOD_NO_FW_LOAD -DPNO_SUPPORT          \
	-DDHD_USE_IDLECOUNT -DSET_RANDOM_MAC_SOFTAP -DVSDB                    \
	-DWL_CFG80211_VSDB_PRIORITIZE_SCAN_REQUEST -DSDIO_CRC_ERROR_FIX       \
	-DESCAN_RESULT_PATCH -DHT40_GO -DPASS_ARP_PACKET -DSUPPORT_PM2_ONLY   \
	-DDHD_DONOT_FORWARD_BCMEVENT_AS_NETWORK_PKT -DAMPDU_HOSTREORDER       \
	-DDISABLE_FW_ROAM_SUSPEND -DDISABLE_BUILTIN_ROAM                      \
	-DCUSTOM_SDIO_F2_BLKSIZE=128 -DWL_SDO -DWL_SUPPORT_BACKPORTED_KPATCHES\
	-Idrivers/net/wireless/bcmdhd.1.28.23.x.cn_ap6476 -Idrivers/net/wireless/bcmdhd.1.28.23.x.cn_ap6476/include


ifeq ($(CONFIG_MACH_ODROID_4210),y)
DHDCFLAGS += -DCUSTOMER_HW
endif

ifeq ($(CONFIG_BCMDHD_OOB),y)
DHDCFLAGS += -DOOB_INTR_ONLY -DHW_OOB -DCUSTOMER_OOB
else
DHDCFLAGS += -DSDIO_ISR_THREAD
endif

ifeq ($(CONFIG_BCMDHD_AG),y)
	DHDCFLAGS += -DBAND_AG
endif

ifeq ($(CONFIG_WIRELESS_EXT),y)
DHDCFLAGS += -DSOFTAP -DWL_WIRELESS_EXT -DUSE_IW
endif

ifeq ($(CONFIG_CFG80211),y)
DHDCFLAGS += -DWL_CFG80211 -DWLP2P -DWL_CFG80211_STA_EVENT -DWL_ENABLE_P2P_IF
DHDCFLAGS += -DCUSTOM_ROAM_TRIGGER_SETTING=-65
DHDCFLAGS += -DCUSTOM_ROAM_DELTA_SETTING=15
DHDCFLAGS += -DCUSTOM_KEEP_ALIVE_SETTING=28000
DHDCFLAGS += -DCUSTOM_PNO_EVENT_LOCK_xTIME=7
endif

ifneq ($(CONFIG_DHD_USE_SCHED_SCAN),)
DHDCFLAGS += -DWL_SCHED_SCAN
endif

EXTRA_CFLAGS = $(DHDCFLAGS)
EXTRA_LDFLAGS += --strip-debug


ifneq ($(KERNELRELEASE),)

DHDOFILES = aiutils.o bcmsdh_sdmmc_linux.o dhd_linux.o siutils.o bcmutils.o   \
	dhd_linux_sched.o dhd_sdio.o bcmwifi_channels.o bcmevent.o hndpmu.o   \
	bcmsdh.o dhd_cdc.o bcmsdh_linux.o dhd_common.o linux_osl.o            \
	bcmsdh_sdmmc.o dhd_custom_gpio.o sbutils.o wldev_common.o wl_android.o

$(MODULE_NAME)-y += $(DHDOFILES)

$(MODULE_NAME)-$(CONFIG_MACH_ODROID_4210) += dhd_gpio.o
$(MODULE_NAME)-$(CONFIG_WIRELESS_EXT) += wl_iw.o
$(MODULE_NAME)-$(CONFIG_CFG80211) += wl_cfg80211.o wl_cfgp2p.o wl_linux_mon.o dhd_cfg80211.o
	
obj-m := $(MODULE_NAME).o


else
export CONFIG_BCMDHD = m

all: modules

modules:
	$(MAKE) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KSRC) M=$(shell pwd)  modules

strip:
	$(CROSS_COMPILE)strip $(MODULE_NAME).ko --strip-unneeded

.PHONY: modules clean

clean:
	rm -fr *.mod.c *.mod *.o .*.cmd *.ko *~
	rm -fr .tmp_versions
	rm -fr Module.symvers ; rm -fr Module.markers ; rm -fr modules.order
endif	