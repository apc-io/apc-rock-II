/*
 * (C) Copyright 2011 - 2012 Samsung Electronics
 * EXT2/4 filesystem implementation in Uboot by
 * Uma Shankar <uma.shankar@samsung.com>
 * Manjunatha C Achar <a.manjunatha@samsung.com>
 *
 * Ext4fs support
 * made from existing cmd_ext2.c file of Uboot
 *
 * (C) Copyright 2004
 * esd gmbh <www.esd-electronics.com>
 * Reinhard Arlt <reinhard.arlt@esd-electronics.com>
 *
 * made from cmd_reiserfs by
 *
 * (C) Copyright 2003 - 2004
 * Sysgo Real-Time Solutions, AG <www.elinos.com>
 * Pavel Bartusek <pba@sysgo.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 *
 */

/*
 * Changelog:
 *	0.1 - Newly created file for ext4fs support. Taken from cmd_ext2.c
 *	        file in uboot. Added ext4fs ls load and write support.
 */

#include <common.h>
#include <part.h>
#include <config.h>
#include <command.h>
#include <image.h>
#include <linux/ctype.h>
#include <asm/byteorder.h>
#include <ext_common.h>
#include <ext4fs.h>
#include <linux/stat.h>
#include <malloc.h>

#if defined(CONFIG_CMD_USB) && defined(CONFIG_USB_STORAGE)
#include <usb.h>
#endif


#define DOS_PART_MAGIC_OFFSET		0x1fe
#define DOS_FS_TYPE_OFFSET		0x36
#define DOS_FS32_TYPE_OFFSET		0x52

int do_ext_load(cmd_tbl_t *cmdtp, int flag, int argc,
						char *const argv[])
{
	char *filename = NULL;
	char *ep;
	int dev, part = 1;
	ulong addr = 0, part_length, filelen;
	disk_partition_t info;
	block_dev_desc_t *dev_desc = NULL;
	char buf [12];
	unsigned long count;
	char *addr_str;

	switch (argc) {
	case 3:
		addr_str = getenv("loadaddr");
		if (addr_str != NULL) {
			addr = simple_strtoul (addr_str, NULL, 16);
		} else {
			addr = CFG_LOAD_ADDR;
		}
		filename = getenv ("bootfile");
		count = 0;
		break;
	case 4:
		addr = simple_strtoul (argv[3], NULL, 16);
		filename = getenv ("bootfile");
		count = 0;
		break;
	case 5:
		addr = simple_strtoul (argv[3], NULL, 16);
		filename = argv[4];
		count = 0;
		break;
	case 6:
		addr = simple_strtoul (argv[3], NULL, 16);
		filename = argv[4];
		count = simple_strtoul (argv[5], NULL, 16);
		break;

	default:
		printf ("Usage:\n%s\n", cmdtp->usage);
		return(1);
	}

	if (!filename) {
		puts ("\n** No boot file defined **\n");
		return(1);
	}

	dev = (int)simple_strtoul (argv[2], &ep, 16);
	dev_desc=get_dev(argv[1],dev);
	if (dev_desc==NULL) {
		printf ("\n** Block device %s %d not supported\n", argv[1], dev);
		return(1);
	}
	if (*ep) {
		if (*ep != ':') {
			puts ("\n** Invalid boot device, use `dev[:part]' **\n");
			return(1);
		}
		part = (int)simple_strtoul(++ep, NULL, 16);
	}

	printf("Using device %s%d, partition %d\n", argv[1], dev, part);

	if (part != 0) {
		if (get_partition_info (dev_desc, part, &info)) {
			printf ("** Bad partition %d **\n", part);
			return(1);
		}

		if (strncmp((char *)info.type, BOOT_PART_TYPE, sizeof(info.type)) != 0) {
			printf ("\n** Invalid partition type \"%.32s\""
				" (expect \"" BOOT_PART_TYPE "\")\n",
				info.type);
			return(1);
		}
		//PRINTF ("\nLoading from block device %s device %d, partition %d: "
		//	"Name: %.32s  Type: %.32s  File:%s\n",
		//	argv[1], dev, part, info.name, info.type, filename);
	} else {
		//PRINTF ("\nLoading from block device %s device %d, File:%s\n",
		//	argv[1], dev, filename);
		
		/* disk doesn't use partition table */
		info.start = 0;
		info.size = dev_desc->lba;
		info.blksz = dev_desc->blksz;
	}

	part_length = info.size;
	ext4fs_set_blk_dev(dev_desc, &info);

	if (!ext4fs_mount(part_length)) {
		printf ("** Bad ext2 partition or disk - %s %d:%d **\n",  argv[1], dev, part);
		ext4fs_close();
		return(1);
	}

	filelen = ext4fs_open(filename);
	if (filelen < 0) {
		printf("** File not found %s\n", filename);
		ext4fs_close();
		return(1);
	}
	if ((count < filelen) && (count != 0)) {
	    filelen = count;
	}

	if (ext4fs_read((char *)addr, filelen) != filelen) {
		printf("\n** Unable to read \"%s\" from %s %d:%d **\n", filename, argv[1], dev, part);
		ext4fs_close();
		return(1);
	}

	ext4fs_close();

	/* Loading ok, update default load address */
	load_addr = addr;

	printf ("\n%ld bytes read\n", filelen);
	sprintf(buf, "%lX", filelen);
	setenv("filesize", buf);

	return(filelen);

#if 0
	char *filename = NULL;
	int dev, part;
	ulong addr = 0;
	int filelen;
	disk_partition_t info;
	block_dev_desc_t *dev_desc;
	char buf[12];
	unsigned long count;
	const char *addr_str;

	count = 0;
	addr = simple_strtoul(argv[3], NULL, 16);
	filename = getenv("bootfile");
	switch (argc) {
	case 3:
		addr_str = getenv("loadaddr");
		if (addr_str != NULL)
			addr = simple_strtoul(addr_str, NULL, 16);
		else
			addr = CONFIG_SYS_LOAD_ADDR;

		break;
	case 4:
		break;
	case 5:
		filename = argv[4];
		break;
	case 6:
		filename = argv[4];
		count = simple_strtoul(argv[5], NULL, 16);
		break;

	default:
		return cmd_usage(cmdtp);
	}

	if (!filename) {
		puts("** No boot file defined **\n");
		return 1;
	}

	part = get_device_and_partition(argv[1], argv[2], &dev_desc, &info, 1);
	if (part < 0)
		return 1;

	dev = dev_desc->dev;
	printf("Loading file \"%s\" from %s device %d%c%c\n",
		filename, argv[1], dev,
		part ? ':' : ' ', part ? part + '0' : ' ');

	ext4fs_set_blk_dev(dev_desc, &info);

	if (!ext4fs_mount(info.size)) {
		printf("** Bad ext2 partition or disk - %s %d:%d **\n",
		       argv[1], dev, part);
		ext4fs_close();
		goto fail;
	}

	filelen = ext4fs_open(filename);
	if (filelen < 0) {
		printf("** File not found %s\n", filename);
		ext4fs_close();
		goto fail;
	}
	if ((count < filelen) && (count != 0))
		filelen = count;

	if (ext4fs_read((char *)addr, filelen) != filelen) {
		printf("** Unable to read \"%s\" from %s %d:%d **\n",
		       filename, argv[1], dev, part);
		ext4fs_close();
		goto fail;
	}

	ext4fs_close();
	/* Loading ok, update default load address */
	load_addr = addr;

	printf("%d bytes read\n", filelen);
	sprintf(buf, "%X", filelen);
	setenv("filesize", buf);

	return 0;
fail:
	return 1;
#endif
}

int do_ext_ls(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{

	char *filename = "/";
	int dev=0;
	int part=1;
	char *ep;
	block_dev_desc_t *dev_desc=NULL;
	int part_length;
	disk_partition_t info;

	if (argc < 3) {
		printf ("Usage:\n%s\n", cmdtp->usage);
		return(1);
	}
	dev = (int)simple_strtoul (argv[2], &ep, 16);
	dev_desc=get_dev(argv[1],dev);

	if (dev_desc == NULL) {
		printf ("\n** Block device %s %d not supported\n", argv[1], dev);
		return(1);
	}

	if (*ep) {
		if (*ep != ':') {
			puts ("\n** Invalid boot device, use `dev[:part]' **\n");
			return(1);
		}
		part = (int)simple_strtoul(++ep, NULL, 16);
	}

	if (argc == 4) {
	    filename = argv[3];
	}

	printf("Using device %s %d:%d, directory: %s\n", argv[1], dev, part, filename);

	if (part == 0) {
		/* disk doesn't use partition table */
		info.start = 0;
		info.size = dev_desc->lba;
		info.blksz = dev_desc->blksz;
	} else {
		if (get_partition_info
		    (dev_desc, part, &info)) {
		}
	}

	part_length = info.size;
	if (part_length == 0) {
		printf ("** Bad partition - %s %d:%d **\n",  argv[1], dev, part);
		ext4fs_close();
		return(1);
	}

	dev = dev_desc->dev;
	ext4fs_set_blk_dev(dev_desc, &info);
	
	if (!ext4fs_mount(info.size)) {
		printf("** Bad ext2 partition or disk - %s %d:%d **\n",
			   argv[1], dev, part);
		ext4fs_close();
		goto fail;
	}
	
	if (ext4fs_ls(filename)) {
		printf("** Error extfs_ls() **\n");
		ext4fs_close();
		goto fail;
	};
	
	ext4fs_close();
	return 0;
	
fail:

	return 1;

}
