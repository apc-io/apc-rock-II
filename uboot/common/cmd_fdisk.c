


#include <common.h>
#include <part.h>
//#include <config.h>
#include <command.h>
//#include <image.h>
//#include <linux/ctype.h>
//#include <asm/byteorder.h>
//#include <linux/stat.h>
//#include <malloc.h>

/* DOS partition types */

static const char *const i386_sys_types[] = {
	"\x00" "Empty",
	"\x01" "FAT12",
	"\x04" "FAT16 <32M",
	"\x05" "Extended",         /* DOS 3.3+ extended partition */
	"\x06" "FAT16",            /* DOS 16-bit >=32M */
	"\x07" "HPFS/NTFS",        /* OS/2 IFS, eg, HPFS or NTFS or QNX */
	"\x0a" "OS/2 Boot Manager",/* OS/2 Boot Manager */
	"\x0b" "Win95 FAT32",
	"\x0c" "Win95 FAT32 (LBA)",/* LBA really is 'Extended Int 13h' */
	"\x0e" "Win95 FAT16 (LBA)",
	"\x0f" "Win95 Ext'd (LBA)",
	"\x11" "Hidden FAT12",
	"\x12" "Compaq diagnostics",
	"\x14" "Hidden FAT16 <32M",
	"\x16" "Hidden FAT16",
	"\x17" "Hidden HPFS/NTFS",
	"\x1b" "Hidden Win95 FAT32",
	"\x1c" "Hidden W95 FAT32 (LBA)",
	"\x1e" "Hidden W95 FAT16 (LBA)",
	"\x3c" "Part.Magic recovery",
	"\x41" "PPC PReP Boot",
	"\x42" "SFS",
	"\x63" "GNU HURD or SysV", /* GNU HURD or Mach or Sys V/386 (such as ISC UNIX) */
	"\x80" "Old Minix",        /* Minix 1.4a and earlier */
	"\x81" "Minix / old Linux",/* Minix 1.4b and later */
	"\x82" "Linux swap",       /* also Solaris */
	"\x83" "Linux",
	"\x84" "OS/2 hidden C: drive",
	"\x85" "Linux extended",
	"\x86" "NTFS volume set",
	"\x87" "NTFS volume set",
	"\x8e" "Linux LVM",
	"\x9f" "BSD/OS",           /* BSDI */
	"\xa0" "Thinkpad hibernation",
	"\xa5" "FreeBSD",          /* various BSD flavours */
	"\xa6" "OpenBSD",
	"\xa8" "Darwin UFS",
	"\xa9" "NetBSD",
	"\xab" "Darwin boot",
	"\xb7" "BSDI fs",
	"\xb8" "BSDI swap",
	"\xbe" "Solaris boot",
	"\xeb" "BeOS fs",
	"\xee" "EFI GPT",                    /* Intel EFI GUID Partition Table */
	"\xef" "EFI (FAT-12/16/32)",         /* Intel EFI System Partition */
	"\xf0" "Linux/PA-RISC boot",         /* Linux/PA-RISC boot loader */
	"\xf2" "DOS secondary",              /* DOS 3.3+ secondary */
	"\xfd" "Linux raid autodetect",      /* New (2.2.x) raid partition with
						autodetect using persistent
						superblock */
#if 0 /* ENABLE_WEIRD_PARTITION_TYPES */
	"\x02" "XENIX root",
	"\x03" "XENIX usr",
	"\x08" "AIX",              /* AIX boot (AIX -- PS/2 port) or SplitDrive */
	"\x09" "AIX bootable",     /* AIX data or Coherent */
	"\x10" "OPUS",
	"\x18" "AST SmartSleep",
	"\x24" "NEC DOS",
	"\x39" "Plan 9",
	"\x40" "Venix 80286",
	"\x4d" "QNX4.x",
	"\x4e" "QNX4.x 2nd part",
	"\x4f" "QNX4.x 3rd part",
	"\x50" "OnTrack DM",
	"\x51" "OnTrack DM6 Aux1", /* (or Novell) */
	"\x52" "CP/M",             /* CP/M or Microport SysV/AT */
	"\x53" "OnTrack DM6 Aux3",
	"\x54" "OnTrackDM6",
	"\x55" "EZ-Drive",
	"\x56" "Golden Bow",
	"\x5c" "Priam Edisk",
	"\x61" "SpeedStor",
	"\x64" "Novell Netware 286",
	"\x65" "Novell Netware 386",
	"\x70" "DiskSecure Multi-Boot",
	"\x75" "PC/IX",
	"\x93" "Amoeba",
	"\x94" "Amoeba BBT",       /* (bad block table) */
	"\xa7" "NeXTSTEP",
	"\xbb" "Boot Wizard hidden",
	"\xc1" "DRDOS/sec (FAT-12)",
	"\xc4" "DRDOS/sec (FAT-16 < 32M)",
	"\xc6" "DRDOS/sec (FAT-16)",
	"\xc7" "Syrinx",
	"\xda" "Non-FS data",
	"\xdb" "CP/M / CTOS / ...",/* CP/M or Concurrent CP/M or
	                              Concurrent DOS or CTOS */
	"\xde" "Dell Utility",     /* Dell PowerEdge Server utilities */
	"\xdf" "BootIt",           /* BootIt EMBRM */
	"\xe1" "DOS access",       /* DOS access or SpeedStor 12-bit FAT
	                              extended partition */
	"\xe3" "DOS R/O",          /* DOS R/O or SpeedStor */
	"\xe4" "SpeedStor",        /* SpeedStor 16-bit FAT extended
	                              partition < 1024 cyl. */
	"\xf1" "SpeedStor",
	"\xf4" "SpeedStor",        /* SpeedStor large partition */
	"\xfe" "LANstep",          /* SpeedStor >1024 cyl. or LANstep */
	"\xff" "BBT",              /* Xenix Bad Block Table */
#endif
	NULL
};

#define get_sys_types() i386_sys_types
#define MAXIMUM_PARTS 60

static const char *
partition_type(unsigned char type)
{
	int i;
	const char *const *types = get_sys_types();
	for (i = 0; types[i]; i++)
		if ((unsigned char)types[i][0] == type)
			return types[i] + 1;

	return "Unknown";
}


#define PRIMARY_PART 1
#define EXTENDED_PART 2
#define LOGICAL_PART 3
#define PRIMARY_MAX 4

int do_fdisk_add_primary(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
	int dev=0;
	int part=0;
	int address=0;//the unit is block size
	int length=0;//the unit is block size
	block_dev_desc_t *dev_desc=NULL;
	disk_partition_t info;
	char *ep;
	
	if (argc < 5) {
		printf ("Usage:\n%s\n", cmdtp->help);
		return(1);
	}
	dev = (int)simple_strtoul (argv[2], &ep, 16);
	dev_desc=get_dev(argv[1],dev);
	

	if (dev_desc == NULL) {
		printf ("\n** Block device %s %d not supported\n", argv[1], dev);
		return(1);
	}
	//printf("Using device %s %d, \n", argv[1], dev);


	if (*ep) {
		if (*ep != ':') {
			puts ("\n** Invalid boot device, use `dev[:part]' **\n");
			return 1;
		}
		part = (int)simple_strtoul(++ep, NULL, 16);
	}

	if ((part<=0)||(part> PRIMARY_MAX)) {
		printf("Please input a partition number which is from 1 to 4 !\n");
		return 1;
	}
	if (get_partition_info(dev_desc, part, &info)==0) {
		printf("The partition exists, please delete it first\n");
		return 1;
	}


	address = (int)simple_strtoul (argv[3], NULL, 16);
	if ((address<0)||(address>=dev_desc->lba)) {
		printf("Please input a validate block address which should be small then 0x%x !\n",
			dev_desc->lba);
		return 1;
	}
	length = (int)simple_strtoul (argv[4], NULL, 16);
	if ((length<=0)||
		(length>=dev_desc->lba)||
		((address+length)>dev_desc->lba)) {
		printf("Please input a validate size !\n");
		return 1;
	}
	
	return add_partition(dev_desc, part, PRIMARY_PART, address, length);
}


int do_fdisk_add_extended(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
	int dev=0;
	int part=0;
	int address=0;//the unit is block size
	int length=0;//the unit is block size
	block_dev_desc_t *dev_desc=NULL;
	disk_partition_t info;
	char *ep;
	
	if (argc < 5) {
		printf ("Usage:\n%s\n", cmdtp->help);
		return(1);
	}
	dev = (int)simple_strtoul (argv[2], &ep, 16);
	dev_desc=get_dev(argv[1],dev);
	

	if (dev_desc == NULL) {
		printf ("\n** Block device %s %d not supported\n", argv[1], dev);
		return(1);
	}
	//printf("Using device %s %d, \n", argv[1], dev);

	
	if (*ep) {
		if (*ep != ':') {
			puts ("\n** Invalid boot device, use `dev[:part]' **\n");
			return 1;
		}
		part = (int)simple_strtoul(++ep, NULL, 16);
	}
	
	if ((part<=0)||(part> PRIMARY_MAX)) {
		printf("Please input a partition number which is from 1 to 4 !\n");
		return 1;
	}
	if (get_partition_info(dev_desc, part, &info)==0) {
		printf("The partition exists, please delete it first\n");
		return 1;
	}

	address = (int)simple_strtoul (argv[3], NULL, 16);
	if ((address<=0)||(address>=dev_desc->lba)) {
		printf("Please input a validate block address which should be small then 0x%x !\n",
			dev_desc->lba);
		return 1;
	}
	length = (int)simple_strtoul (argv[4], NULL, 16);
	if ((length<=0)||
		(length>=dev_desc->lba)||
		((address+length)>dev_desc->lba)) {
		printf("Please input a validate size !\n");
		return 1;
	}
	
	return add_partition(dev_desc, part, EXTENDED_PART, address, length);
}

int do_fdisk_add_logical(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
	int dev=0;
	int address=0;//the unit is block size
	int length=0;//the unit is block size
	block_dev_desc_t *dev_desc=NULL;
	
	if (argc < 5) {
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

	address = (int)simple_strtoul (argv[3], NULL, 16);
	if ((address<=0)||(address>=dev_desc->lba)) {
		printf("Please input a validate block address which should be small then 0x%x !\n",
			dev_desc->lba);
		return 1;
	}
	length = (int)simple_strtoul (argv[4], NULL, 16);
	if ((length<=0)||
		(length>=dev_desc->lba)||
		((address+length)>dev_desc->lba)) {
		printf("Please input a validate size !\n");
		return 1;
	}
	
	return add_partition(dev_desc, 1, LOGICAL_PART, address, length);
}

int do_fdisk_del(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
	int dev=0;
	int part=0;
	block_dev_desc_t *dev_desc=NULL;
	char *ep;

	if (argc < 3) {
		printf ("Usage:\n%s\n", cmdtp->help);
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
			return 1;
		}
		part = (int)simple_strtoul(++ep, NULL, 16);
	}

	if (part<=0) {
		printf("Please input a partition number which is from 1 !\n");
		return 1;
	}
	if (del_partition(dev_desc, part)==0)
		return 0;
	else 
		return 1;
}

int do_fdisk_ls(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
	int dev=0;
	int part=1;
	block_dev_desc_t *dev_desc=NULL;
	disk_partition_t info[MAXIMUM_PARTS];

	if (argc < 3) {
		printf ("Usage:\n%s\n", cmdtp->help);
		return(1);
	}
	dev = (int)simple_strtoul (argv[2], NULL, 16);
	dev_desc=get_dev(argv[1],dev);

	if (dev_desc == NULL) {
		printf ("\n** Block device %s %d not supported\n", argv[1], dev);
		return(1);
	}

	//Show the total size in the Device
	printf("Total size(blocks): 0x%x, the block size(byte): 0x%x\n", dev_desc->lba, 
		dev_desc->blksz);
	//show the partitions in the MBR
	for(part=1; part<5; part++) {
		if (get_partition_info(dev_desc, part, &info[part])==0) {
			printf("----------------------------------\n");
			printf("Partition: %d\n", part);
			printf("Type: %s\n", info[part].type);
			printf("System: %s\n", partition_type((unsigned char)(simple_strtoul (info[part].type, NULL, 10))));
			printf("Start from (block): 0x%x\n", info[part].start);
			printf("End of (block): 0x%x\n", info[part].start+info[part].size-1);
			printf("Size (blocks): 0x%x\n", info[part].size);
			printf("Offset (blocks): 0x%x\n", info[part].offset);			
			printf("total size (Kbytes): %d\n", info[part].size>>1);
			printf("----------------------------------\n");
		}
	}

	//show the logical partitions
	while(get_partition_info(dev_desc, part, &info[part])==0){
		printf("----------------------------------\n");
		printf("Partition: %d\n", part);
		printf("Type: %s\n", info[part].type);
		printf("System: %s\n", partition_type((unsigned char)(simple_strtoul (info[part].type, NULL, 10))));
		printf("Start from (block): 0x%x\n", info[part].start);		
		printf("End of (block): 0x%x\n", info[part].start+info[part].size-1);
		printf("Size (blocks): 0x%x\n", info[part].size);
		printf("Offset (blocks): 0x%x\n", info[part].offset);			
		printf("total size (Kbytes): %d\n", info[part].size>>1);
		printf("----------------------------------\n");
		part++;
	}

	return 0;
}

U_BOOT_CMD(fdiskaddp, 5, 1, do_fdisk_add_primary,
	"add a primary partition in the storage device",
	"<interface> <dev:part> <address(hex)> <length(hex)>\n"
	"     - add a primary partition in the storage device");

U_BOOT_CMD(fdiskadde, 5, 1, do_fdisk_add_extended,
	"add a extended partition in the storage device",
	"<interface> <dev:part> <address(hex)> <length(hex)>\n"
	"     - add a extended partition in the storage device");

U_BOOT_CMD(fdiskaddl, 5, 1, do_fdisk_add_logical,
	"add a logical partition in the storage device",
	"<interface> <dev> <address(hex)> <length(hex)>\n"
	"     - add a logical partition in the storage device");


U_BOOT_CMD(fdiskdel, 3, 1, do_fdisk_del,
	"delete a partition in the storage device",
	"<interface> <dev:part> \n"
	"     - delete the partition in the storage device");

U_BOOT_CMD(fdiskls, 3, 1, do_fdisk_ls,
	"list the partitions in the storage device",
	"<interface> <dev> \n"
	"	  - list all the partiitons in the storage device");

