apc-rock-II
===========

Kernel and Bootloader codes for Rock II (&amp; Paper II)

Copyright © 2014 VIA Technologies, Inc.


BUILD SOURCE CODE
--------------------------
### DEVELOP ENVIRONMENT

OS: Ubuntu 12.04 x64
Linux kernel toolchain: apc-rock-toolchain

### Prepare

Download toolchain from [apc-io/apc-rock-toolchain] (https://github.com/apc-io/apc-rock-toolchain)

    $ export ARCH=arm
    $ export CROSS_COMPILE=apc-rock-toolchain/mybin/arm_1103_le-
    $ export PATH=$PATH:apc-rock-toolchain/mybin/

### Uboot

    $ cd uboot
    $ make distclean
    $ make wmt_config
    $ make zuboot
    $ Rename zuboot as u-boot.bin and put it into FirmwareInstall/firmware/


### Kernel

    $ make Android_defconfig
    $ make ubin
    $ Copy uzimage.bin into FirmwareInstall/firmware/
