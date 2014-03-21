apc-rock-II
===========

Kernel and Bootloader codes for Rock II (&amp; Paper II)


Building
--------------------------
Prepare
#########
Download toolchain from [apc-io/apc-rock-toolchain] (https://github.com/apc-io/apc-rock-toolchain)

Uboot
########
1. export ARCH=arm
2. export CROSS_COMPILE=apc-rock-toolchain/mybin/
3. cd uboot
4. make distclean
5. make wmt_config
6. make zuboot
7. Rename zuboot as u-boot.bin and put it into FirmwareInstall/firmware/


Kernel
#######
1. export ARCH=arm
2. export CROSS_COMPILE=apc-rock-toolchain/mybin/
3. make Android_defconfig
4. make ubin
5. Copy uzimage.bin into FirmwareInstall/firmware/
