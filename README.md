apc-rock-II
===========

Kernel and Bootloader codes for Rock II (&amp; Paper II)


Building
--------------------------

Prepare
#########

Download toolchain from [apc-io/apc-rock-toolchain] (https://github.com/apc-io/apc-rock-toolchain)
1. export ARCH=arm
2. export CROSS_COMPILE=apc-rock-toolchain/mybin/arm_1103_le-
3. export PATH=$PATH:apc-rock-toolchain/mybin/

Uboot
########

1. cd uboot
2. make distclean
3. make wmt_config
4. make zuboot
5. Rename zuboot as u-boot.bin and put it into FirmwareInstall/firmware/


Kernel
#######

1. make Android_defconfig
2. make ubin
3. Copy uzimage.bin into FirmwareInstall/firmware/
