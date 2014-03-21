

#include <common.h>
#include <part.h>
//#include <config.h>
#include <command.h>
//#include <image.h>
//#include <linux/ctype.h>
//#include <asm/byteorder.h>
//#include <linux/stat.h>
//#include <malloc.h>

int do_mkfsfat_32(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
	int dev=0;
	int part=1;
	block_dev_desc_t *dev_desc=NULL;
	disk_partition_t info;

	if (argc < 4) {
		printf ("Usage:\n%s\n", cmdtp->help);
		return(1);
	}
	dev = (int)simple_strtoul (argv[2], NULL, 16);
	dev_desc=get_dev(argv[1],dev);

	if (dev_desc == NULL) {
		printf ("\n** Block device %s %d not supported\n", argv[1], dev);
		return(1);
	}
	printf("Using device %s %d, \n", argv[1], dev);
	
	part = (int)simple_strtoul (argv[3], NULL, 10);
	if (part<=0) {
		printf("Please input a partition number which is from 1 !\n");
		return 1;
	}

	
	fatpre_register_device(dev_desc,part);

	return format_fat32();
}
int do_mkfsfat_16(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
	int dev=0;
	int part=1;
	block_dev_desc_t *dev_desc=NULL;
	disk_partition_t info;

	if (argc < 4) {
		printf ("Usage:\n%s\n", cmdtp->help);
		return(1);
	}
	dev = (int)simple_strtoul (argv[2], NULL, 16);
	dev_desc=get_dev(argv[1],dev);

	if (dev_desc == NULL) {
		printf ("\n** Block device %s %d not supported\n", argv[1], dev);
		return(1);
	}
	printf("Using device %s %d, \n", argv[1], dev);
	
	part = (int)simple_strtoul (argv[3], NULL, 10);
	if (part<=0) {
		printf("Please input a partition number which is from 1 !\n");
		return 1;
	}

	
	fatpre_register_device(dev_desc,part);

	return format_fat16();
}
int do_mkfsfat_12(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
	int dev=0;
	int part=1;
	block_dev_desc_t *dev_desc=NULL;

	if (argc < 4) {
		printf ("Usage:\n%s\n", cmdtp->help);
		return(1);
	}
	dev = (int)simple_strtoul (argv[2], NULL, 16);
	dev_desc=get_dev(argv[1],dev);

	if (dev_desc == NULL) {
		printf ("\n** Block device %s %d not supported\n", argv[1], dev);
		return(1);
	}
	
	part = (int)simple_strtoul (argv[3], NULL, 10);
	if (part<=0) {
		printf("Please input a partition number which is from 1 !\n");
		return 1;
	}
	
	fatpre_register_device(dev_desc,part);

	return format_fat12();
	
}

U_BOOT_CMD(mkfsfat12, 4, 1, do_mkfsfat_12,
	"format a partition in the storage device to FAT12",
	"<interface> <device number> <partition number>\n"
	"     - format a primary partition in the storage device to FAT12");

U_BOOT_CMD(mkfsfat16, 4, 1, do_mkfsfat_16,
	"format a partition in the storage device to FAT16",
        "<interface> <device number> <partition number>\n"
	"     - format a primary partition in the storage device to FAT16");

U_BOOT_CMD(mkfsfat32, 4, 1, do_mkfsfat_32,
	"format a partition in the storage device to FAT32",
	"<interface> <device number> <partition number>\n"
	"     - format a primary partition in the storage device to FAT32");

