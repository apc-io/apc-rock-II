/*
 * (C) Copyright 2001
 * Raymond Lo, lo@routefree.com
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *
 * See file CREDITS for list of people who contributed to this
 * project.
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
 */

/*
 * Support for harddisk partitions.
 *
 * To be compatible with LinuxPPC and Apple we use the standard Apple
 * SCSI disk partitioning scheme. For more information see:
 * http://developer.apple.com/techpubs/mac/Devices/Devices-126.html#MARKER-14-92
 */

#include <common.h>
#include <command.h>
#include <ide.h>
#include "part_dos.h"

#if ((CONFIG_COMMANDS & CFG_CMD_IDE)	|| \
     (CONFIG_COMMANDS & CFG_CMD_SCSI)	|| \
     (CONFIG_COMMANDS & CFG_CMD_USB)	|| \
     defined(CONFIG_MMC) || \
     defined(CONFIG_SYSTEMACE) ) && defined(CONFIG_DOS_PARTITION)

/* Convert char[4] in little endian format to the host format integer
 */
static inline int le32_to_int(unsigned char *le32)
{
    return ((le32[3] << 24) +
	    (le32[2] << 16) +
	    (le32[1] << 8) +
	     le32[0]
	   );
}

static inline int is_extended(int part_type)
{
    return (part_type == 0x5 ||
	    part_type == 0xf ||
	    part_type == 0x85);
}

static int
valid_part_table_flag(const char *mbuffer)
{
	return (mbuffer[510] == 0x55 && (uint8_t)mbuffer[511] == 0xaa);
}

static void print_one_part (dos_partition_t *p, int ext_part_sector, int part_num)
{
	int lba_start = ext_part_sector + le32_to_int (p->start4);
	int lba_size  = le32_to_int (p->size4);

	printf ("%5d\t\t%10d\t%10d\t%2x%s\n",
		part_num, lba_start, lba_size, p->sys_ind,
		(is_extended (p->sys_ind) ? " Extd" : ""));
}

static int test_block_type(unsigned char *buffer)
{
	if((buffer[DOS_PART_MAGIC_OFFSET + 0] != 0x55) ||
	    (buffer[DOS_PART_MAGIC_OFFSET + 1] != 0xaa) ) {
		return (-1);
	} /* no DOS Signature at all */
	if(strncmp((char *)&buffer[DOS_PBR_FSTYPE_OFFSET],"FAT",3)==0)
		return DOS_PBR; /* is PBR */
	return DOS_MBR;	    /* Is MBR */
}


int test_part_dos (block_dev_desc_t *dev_desc)
{
	unsigned char buffer[DEFAULT_SECTOR_SIZE];

	if ((dev_desc->block_read(dev_desc->dev, 0, 1, (ulong *) buffer) != 1) ||
	    (buffer[DOS_PART_MAGIC_OFFSET + 0] != 0x55) ||
	    (buffer[DOS_PART_MAGIC_OFFSET + 1] != 0xaa) ) {
		return (-1);
	}
	return (0);
}

/*  Print a partition that is relative to its Extended partition table
 */
static void print_partition_extended (block_dev_desc_t *dev_desc, int ext_part_sector, int relative,
							   int part_num)
{
	unsigned char buffer[DEFAULT_SECTOR_SIZE];
	dos_partition_t *pt;
	int i;

	if (dev_desc->block_read(dev_desc->dev, ext_part_sector, 1, (ulong *) buffer) != 1) {
		printf ("** Can't read partition table on %d:%d **\n",
			dev_desc->dev, ext_part_sector);
		return;
	}
	i=test_block_type(buffer);
	if(i==-1) {
		printf ("bad MBR sector signature 0x%02x%02x\n",
			buffer[DOS_PART_MAGIC_OFFSET],
			buffer[DOS_PART_MAGIC_OFFSET + 1]);
		return;
	}
	if(i==DOS_PBR) {
		printf ("    1\t\t         0\t%10ld\t%2x\n",
			dev_desc->lba, buffer[DOS_PBR_MEDIA_TYPE_OFFSET]);
		return;
	}
	/* Print all primary/logical partitions */
	pt = (dos_partition_t *) (buffer + DOS_PART_TBL_OFFSET);
	for (i = 0; i < 4; i++, pt++) {
		/*
		 * fdisk does not show the extended partitions that
		 * are not in the MBR
		 */

		if ((pt->sys_ind != 0) &&
		    (ext_part_sector == 0 || !is_extended (pt->sys_ind)) ) {
			print_one_part (pt, ext_part_sector, part_num);
		}

		/* Reverse engr the fdisk part# assignment rule! */
		if ((ext_part_sector == 0) ||
		    (pt->sys_ind != 0 && !is_extended (pt->sys_ind)) ) {
			part_num++;
		}
	}

	/* Follows the extended partitions */
	pt = (dos_partition_t *) (buffer + DOS_PART_TBL_OFFSET);
	for (i = 0; i < 4; i++, pt++) {
		if (is_extended (pt->sys_ind)) {
			int lba_start = le32_to_int (pt->start4) + relative;

			print_partition_extended (dev_desc, lba_start,
						  ext_part_sector == 0  ? lba_start
									: relative,
						  part_num);
		}
	}

	return;
}

static void
get_partition_table_geometry(const unsigned char * mbrbuffer);


/*  Print a partition that is relative to its Extended partition table
 */
static int get_partition_info_extended (block_dev_desc_t *dev_desc, int ext_part_sector,
				 int relative, int part_num,
				 int which_part, disk_partition_t *info)
{   
	unsigned char buffer[DEFAULT_SECTOR_SIZE];
	dos_partition_t *pt;
	int i;
	if (dev_desc->block_read (dev_desc->dev, ext_part_sector, 1, (ulong *) buffer) != 1) {
		printf ("** Can't read partition table on %d:%d **\n",
			dev_desc->dev, ext_part_sector);
		return -1;
	}
	if (buffer[DOS_PART_MAGIC_OFFSET] != 0x55 ||
		buffer[DOS_PART_MAGIC_OFFSET + 1] != 0xaa) {
		printf ("bad MBR sector signature 0x%02x%02x\n",
			buffer[DOS_PART_MAGIC_OFFSET],
			buffer[DOS_PART_MAGIC_OFFSET + 1]);
		return -1;
	}
	/* Print all primary/logical partitions */
	pt = (dos_partition_t *) (buffer + DOS_PART_TBL_OFFSET);
	#if 0	
	printf("ext_part_sector is 0x%x, relative is 0x%x, part_num %d, which_part %d\n",
		ext_part_sector, relative, part_num, which_part);
	printf("pt->boot = %x\n",pt->boot_ind);
	printf("pt->head = %x\n",pt->head);
	printf("pt->sector = %x\n",pt->sector);
	printf("pt->cyl = %x\n",pt->cyl);
	printf("pt->sys_ind = %x\n",pt->sys_ind);
	printf("pt->end_head = %x\n",pt->end_head);
	printf("pt->end_sector = %x\n",pt->end_sector);
	printf("pt->end_cyl = %x\n",pt->end_cyl);
	printf("start4[0] = %x, %x, %x ,%x\n",pt->start4[0],pt->start4[1],pt->start4[2],pt->start4[3]);
	printf("size4[0] = %x ,%x ,%x ,%x \n",pt->size4[0],pt->size4[1],pt->size4[2],pt->size4[3]);
	#endif
	for (i = 0; i < 4; i++, pt++) {
		/* 
		 * fdisk does not show the extended partitions that
		 * are not in the MBR
		 */	
		 
		if (((pt->boot_ind & ~0x80) == 0) && //eason 
			(pt->sys_ind != 0) &&
		    (part_num == which_part) &&
		    (is_extended(pt->sys_ind) == 0)) {
			info->blksz = 512;
			info->start = ext_part_sector + le32_to_int (pt->start4);
			info->offset=le32_to_int (pt->start4);
			info->size  = le32_to_int (pt->size4);
			switch(dev_desc->if_type) {
				case IF_TYPE_IDE:
				case IF_TYPE_ATAPI:
					sprintf ((char *)info->name, "hd%c%d\n", 'a' + dev_desc->dev, part_num);
					break;
				case IF_TYPE_SCSI:
					sprintf ((char *)info->name, "sd%c%d\n", 'a' + dev_desc->dev, part_num);
					break;
				case IF_TYPE_USB:
					sprintf ((char *)info->name, "usbd%c%d\n", 'a' + dev_desc->dev, part_num);
					break;
				case IF_TYPE_DOC:
					sprintf ((char *)info->name, "docd%c%d\n", 'a' + dev_desc->dev, part_num);
					break;
				default:
					sprintf ((char *)info->name, "xx%c%d\n", 'a' + dev_desc->dev, part_num);
					break;
			}
			sprintf(info->type, "%d", pt->sys_ind);
			//sprintf ((char *)info->type, "U-Boot");
			return 0;
		}
		//if we meet the extended partition in MBR, we should show it.
		if (is_extended(pt->sys_ind) && (part_num<=4)&&(part_num == which_part)) {			
			info->blksz = 512;
			info->start = ext_part_sector + le32_to_int (pt->start4);
			info->offset=le32_to_int (pt->start4);
			info->size  = le32_to_int (pt->size4);
			sprintf(info->type, "%d", pt->sys_ind);
			g_extendPart=part_num;
			return 0;
		}
		/* Reverse engr the fdisk part# assignment rule! */
		if ((ext_part_sector == 0) ||
		    (pt->sys_ind != 0 && !is_extended (pt->sys_ind)) ) {
			part_num++;
		}
	}

	if (which_part<=4) {
		return -1;//cannot find it in MBR
	}
	/* Follows the extended partitions */

	pt = (dos_partition_t *) (buffer + DOS_PART_TBL_OFFSET);
	for (i = 0; i < 4; i++, pt++) {
		if (is_extended (pt->sys_ind)) {
			int lba_start = le32_to_int (pt->start4) + relative;
			return get_partition_info_extended (dev_desc, lba_start,
				 ext_part_sector == 0 ? lba_start : relative,
				 part_num, which_part, info);
		}
	}
	return -1;
}

static int del_partition_dos_extended(block_dev_desc_t *dev_desc, int ext_part_sector,
				 int relative, int which_part)
{
	
	unsigned char buffer[DEFAULT_SECTOR_SIZE];
	unsigned char buffer2[DEFAULT_SECTOR_SIZE];
	dos_partition_t *pt, *qt;
	int i;
	unsigned int lbaSec=0;
	disk_partition_t info[60];
	if (dev_desc->block_read (dev_desc->dev, ext_part_sector, 1, (ulong *) buffer) != 1) {
		printf ("** Can't read partition table on %d:%d **\n",
			dev_desc->dev, ext_part_sector);
		return -1;
	}
  
	if (buffer[DOS_PART_MAGIC_OFFSET] != 0x55 ||
		buffer[DOS_PART_MAGIC_OFFSET + 1] != 0xaa) {
		printf ("bad MBR sector signature 0x%02x%02x\n",
			buffer[DOS_PART_MAGIC_OFFSET],
			buffer[DOS_PART_MAGIC_OFFSET + 1]);
		return -1;
	}
	
	pt = (dos_partition_t *) (buffer + DOS_PART_TBL_OFFSET);

	//del primary and extended partition
	if (which_part<=4) {
		pt+=(which_part-1);
		memset((void *)pt, 0, 16);
		if (dev_desc->block_write (dev_desc->dev, ext_part_sector, 1, (ulong *) buffer) != 1) {
			printf ("** Can't write partition table on %d:%d **\n",
			dev_desc->dev, ext_part_sector);
			return -1;
		}	
		if (which_part=g_extendPart) {
			g_extendPart = 0;
		}
		printf("The partition %d has been deleted!\n", which_part);
		return 0;
	}

	//del logical partition
	for(i=1;i<5;i++)
		get_partition_info_extended(dev_desc, 0,0,1, i, &info[i]);
	while(get_partition_info_extended(dev_desc, 0,0,1, i, &info[i])==0)
		i++;
	
	if (dev_desc->block_read (dev_desc->dev, info[which_part].start-info[which_part].offset, 1, (ulong *) buffer) != 1) {
		printf ("** Can't read partition table on %d:%d **\n",
			dev_desc->dev, ext_part_sector);
		return -1;
	}
	
	if (buffer[DOS_PART_MAGIC_OFFSET] != 0x55 ||
		buffer[DOS_PART_MAGIC_OFFSET + 1] != 0xaa) {
		printf ("bad MBR sector signature 0x%02x%02x\n",
			buffer[DOS_PART_MAGIC_OFFSET],
			buffer[DOS_PART_MAGIC_OFFSET + 1]);
		return -1;
	}
	
	pt = (dos_partition_t *) (buffer+DOS_PART_TBL_OFFSET);
	qt = pt+1;
	
	//the last logical
	if ((!qt->sys_ind)&&(which_part>5)) {

		lbaSec = info[which_part-1].start-info[which_part-1].offset;
	
		
		if (dev_desc->block_read (dev_desc->dev, lbaSec, 1, (ulong *) buffer) != 1) {
			printf ("** Can't read partition table on %d:%d **\n",
				dev_desc->dev, ext_part_sector);
			return -1;
		}
		if (buffer[DOS_PART_MAGIC_OFFSET] != 0x55 ||
			buffer[DOS_PART_MAGIC_OFFSET + 1] != 0xaa) {
			printf ("bad EBR sector signature 0x%02x%02x\n",
				buffer[DOS_PART_MAGIC_OFFSET],
				buffer[DOS_PART_MAGIC_OFFSET + 1]);
			return -1;
		
		}
		pt = (dos_partition_t *) (buffer + DOS_PART_TBL_OFFSET);
		qt = pt+1;
		memset(qt, 0 ,48);
		
		
		if (dev_desc->block_write (dev_desc->dev, lbaSec, 1, (ulong *) buffer) != 1) {
			printf ("** Can't write partition table on %d:%d **\n",
			dev_desc->dev, ext_part_sector);
			return -1;
		}  
	
	} else if(which_part>5) {//in the middle of the chain
		if (dev_desc->block_read (dev_desc->dev, info[which_part-1].start-info[which_part-1].offset, 1, (ulong *) buffer) != 1) {
			printf ("** Can't read partition table on %d:%d **\n",
				dev_desc->dev, ext_part_sector);
			return -1;
		}
		if (buffer[DOS_PART_MAGIC_OFFSET] != 0x55 ||
			buffer[DOS_PART_MAGIC_OFFSET + 1] != 0xaa) {
			printf ("bad EBR sector signature 0x%02x%02x\n",
				buffer[DOS_PART_MAGIC_OFFSET],
				buffer[DOS_PART_MAGIC_OFFSET + 1]);
			return -1;
		
		}
		pt = (dos_partition_t *) (dos_partition_t *) (buffer+DOS_PART_TBL_OFFSET);
		qt = ++pt;
		
		if (dev_desc->block_read (dev_desc->dev, info[which_part].start-info[which_part].offset, 1, (ulong *) buffer2) != 1) {
			printf ("** Can't read partition table on %d:%d **\n",
				dev_desc->dev, ext_part_sector);
			return -1;
		}
		if (buffer2[DOS_PART_MAGIC_OFFSET] != 0x55 ||
			buffer2[DOS_PART_MAGIC_OFFSET + 1] != 0xaa) {
			printf ("bad EBR sector signature 0x%02x%02x\n",
				buffer2[DOS_PART_MAGIC_OFFSET],
				buffer2[DOS_PART_MAGIC_OFFSET + 1]);
			return -1;
		
		}
		pt = (dos_partition_t *) (dos_partition_t *) (buffer2+DOS_PART_TBL_OFFSET);
		pt++;
		memcpy(qt, pt, 16);
		memset(++qt, 0 ,32);
		if (dev_desc->block_write (dev_desc->dev, info[which_part-1].start-info[which_part-1].offset, 1, (ulong *) buffer) != 1) {
			printf ("** Can't write partition table on %d:%d **\n",
			dev_desc->dev, ext_part_sector);
			return -1;
		}  
	} else{//the first logical partition
		if (dev_desc->block_read (dev_desc->dev, info[g_extendPart].start, 1, (ulong *) buffer) != 1) {
			printf ("** Can't read partition table on %d:%d **\n",
				dev_desc->dev, ext_part_sector);
			return -1;
		}
		if (buffer[DOS_PART_MAGIC_OFFSET] != 0x55 ||
			buffer[DOS_PART_MAGIC_OFFSET + 1] != 0xaa) {
			printf ("bad EBR sector signature 0x%02x%02x\n",
				buffer[DOS_PART_MAGIC_OFFSET],
				buffer[DOS_PART_MAGIC_OFFSET + 1]);
			return -1;
		
		}
		pt = (dos_partition_t *) (buffer + DOS_PART_TBL_OFFSET);
		if (g_partition==5) {//also the last one
			memset(pt, 0 ,64);
		} else {
			
			qt = pt+1;
			pt->start4[0]=qt->start4[0];
			pt->start4[1]=qt->start4[1];
			pt->start4[2]=qt->start4[2];
			pt->start4[3]=qt->start4[3];
			
			if (dev_desc->block_read (dev_desc->dev, info[which_part+1].start-info[which_part+1].offset, 1, (ulong *) buffer2) != 1) {
				printf ("** Can't read partition table on %d:%d **\n",
					dev_desc->dev, ext_part_sector);
				return -1;
			}
			if (buffer2[DOS_PART_MAGIC_OFFSET] != 0x55 ||
				buffer2[DOS_PART_MAGIC_OFFSET + 1] != 0xaa) {
				printf ("bad EBR sector signature 0x%02x%02x\n",
					buffer2[DOS_PART_MAGIC_OFFSET],
					buffer2[DOS_PART_MAGIC_OFFSET + 1]);
				return -1;
			
			}
			qt = (dos_partition_t *) (buffer2 + DOS_PART_TBL_OFFSET);
			pt->start4[0]+=qt->start4[0];
			pt->start4[1]+=qt->start4[1];
			pt->start4[2]+=qt->start4[2];
			pt->start4[3]+=qt->start4[3];
			pt->size4[0]=qt->size4[0];
			pt->size4[1]=qt->size4[1];
			pt->size4[2]=qt->size4[2];
			pt->size4[3]=qt->size4[3];
			qt++;
			pt++;
			memcpy(pt, qt, 16);
			pt++;
			memset(pt, 0 ,32);
		}
		
		if (dev_desc->block_write (dev_desc->dev, info[g_extendPart].start, 1, (ulong *) buffer) != 1) {
			printf ("** Can't write partition table on %d:%d **\n",
			dev_desc->dev, ext_part_sector);
			return -1;
		}  
	}
	
	
	if (g_partition>4)
		g_partition--;
	printf("The partition %d has been deleted!\n", which_part);
	return 0;

}

#define EXTENDED                0x05
#define LINUX_NATIVE            0x83
#define pt_offset(b, n) \
	((struct dos_partition *)((b) + 0x1be + (n) * sizeof(struct dos_partition)))


static void
get_partition_table_geometry(const unsigned char * mbrbuffer)
{
	const unsigned char *bufp = mbrbuffer;
	struct dos_partition *p;
	int i, h, s, hh, ss;
	int first = 1;
	int bad = 0;


	hh = ss = 0;
	for (i = 0; i < 4; i++) {
		p = pt_offset(bufp, i);
		if (p->sys_ind != 0) {
			h = p->end_head + 1;
			s = (p->end_sector & 077);
			if (first) {
				hh = h;
				ss = s;
				first = 0;
			} else if (hh != h || ss != s)
				bad = 1;
		}
	}

	if (!first && !bad) {
		g_heads = hh;
		g_sectors = ss;
	}
	
	printf("g_heads 0x%x, g_sectors 0x%x g_cylinders 0x%x\n",
		g_heads, g_sectors, g_cylinders);
}

//in C/H/S address, sector is from 1 to 63, heads is from 0 to 254
//LBA address is from 0
static void 
setPartition(dos_partition_t *pt, unsigned int address, 
	unsigned int offset, unsigned int length, int type)
{
	int tempCyl=0;
	///printf("SetPartition add 0x%x, off 0x%x, length 0x%x, type %d\n",
	//	address, offset, length, type);
	//printf("g_sector 0x%x, g_heads 0x%x\n", g_sectors, g_heads);
	pt->sys_ind=type;
	
	pt->start4[0] = offset&0xff;
	pt->start4[1] = (offset&0xff00)>>8;
	pt->start4[2] = (offset&0xff0000)>>16;
	pt->start4[3] = (offset&0xff000000)>>24;
	pt->boot_ind = 0 ;		
	pt->head = (address/g_sectors)%g_heads;	
	tempCyl = address/g_heads/g_sectors;
	pt->cyl= (tempCyl&0xFF);	
	pt->sector = ((address%g_sectors)+1)|((tempCyl&0x0300)>>2);
		
	pt->end_head = ((address+length-1)/g_sectors)%g_heads;
	tempCyl = (address+length-1)/g_heads/g_sectors;
	pt->end_cyl = (tempCyl&0xFF);
	pt->end_sector = (((address+length-1)%g_sectors)+1)|((tempCyl&0x0300)>>2);
	pt->size4[0]= length&0xff;
	pt->size4[1]= (length&0xff00)>>8;
	pt->size4[2]= (length&0xff0000)>>16;
	pt->size4[3]= (length&0xff000000)>>24;
	//printf("setPartition SC:0x%x, SH:0x%x, SS:0x%x\n", pt->cyl, pt->head, pt->sector);
	//printf("setPartition EC:0x%x, EH:0x%x, ES:0x%x\n", pt->end_cyl, pt->end_head, pt->end_sector);
}

static int add_partition_dos_extended(block_dev_desc_t * dev_desc, int part, int ptype, int address, int length)
{
	
	unsigned char buffer[DEFAULT_SECTOR_SIZE];
	dos_partition_t *pt;
	disk_partition_t info[5];
	int i;
	int offset;
	int tempCyl=0;
	unsigned int first[g_partition], last[g_partition];
	
	printf("add_partition_dos_extended: pre add 0x%x, length 0x%x, total 0x%x\n", address,
		length, address+length-1);
	//read MBR
	if (dev_desc->block_read (dev_desc->dev, 0, 1, (ulong *) buffer) != 1) {
		printf ("** Can't read partition table on %d:%d **\n",
			dev_desc->dev, 0);
		return -1;
	}
  	//test MBR
	if (!(valid_part_table_flag((char*)buffer)))
		return -1;

	//test if there is another extended partition
	pt = (dos_partition_t *) (buffer + DOS_PART_TBL_OFFSET);
	if (part==DOS_EXTENDED) {
		for(i=1;i<5;i++, pt++) {
			if (is_extended(pt->sys_ind))
				break;
		}
		
		if (i<5) {
			printf("Please delete the extended partition first!\n");
			return -1;
		}
		g_extendPart=i;
	}
	
	//find the geometry
	g_heads=255;
	g_sectors=0x3f;
	get_partition_table_geometry(buffer);
	g_heads = g_heads ? g_heads :255;
	g_sectors = g_sectors ? g_sectors : 63;
	g_cylinders = dev_desc->lba / (g_heads * g_sectors );

	//adjust the length in cylinder unit
	if (address<0x3f)
		address=0x3f;
	if ((address+length)%(g_sectors*g_heads)) {
		length+=g_sectors*g_heads-((length+address)%(g_sectors*g_heads));
	}
	printf("add_partition_dos_extended: add 0x%x, length 0x%x, total 0x%x\n", address,
		length, address+length-1);

	//test the address and length
	memset(info, 0, sizeof(info[0])*5);
	for(i=1;i<5;i++){
		get_partition_info_dos(dev_desc, i, &info[i]);
		if (((address>=info[i].start)&&(address<(info[i].start+info[i].size)))||
			(((address+length)>=info[i].start)&&((address+length)<(info[i].start+info[i].size)))||
			((address<=info[i].start)&&((address+length)>=(info[i].start+info[i].size))&&(info[i].size!=0))){
			printf("new address 0x%x, length 0x%x, which is overlapped with part %d(0x%x:0x%x)\n",
				address, length, i, info[i].start, info[i].size);
			printf("The address is overlapped, Please input a valid address and length!\n");
			return -1;
		}
	}
	

	pt = (dos_partition_t *) (buffer + DOS_PART_TBL_OFFSET);
	pt += (part-1);
	if (ptype ==DOS_PRIMARY ) {
		setPartition(pt, address, address, length, LINUX_NATIVE);
		}
	else {
		setPartition(pt, address, address, length, EXTENDED);
		g_extendPart=part;
	}
	
	if (dev_desc->block_write (dev_desc->dev, 0, 1, (ulong *) buffer) != 1) {
		printf ("** Can't write partition table on %d:%d **\n",
		dev_desc->dev, 0);
		return -1;
	}  
	
	if (ptype==DOS_EXTENDED) {
		memset(buffer, 0, 512);
		buffer[510]= 0x55;
		buffer[511]=0xaa;
		if (dev_desc->block_write (dev_desc->dev, address, 1, (ulong *) buffer) != 1) {
			printf ("** Can't read partition table on %d:%d **\n",
				dev_desc->dev, 0);
			return -1;
		}
	}

	return 0;
	
	
}

static int add_logical(block_dev_desc_t * dev_desc, int address, int length)
{
	disk_partition_t info[60];
	dos_partition_t *pt,*qt;
	int tempCyl=0;
	unsigned char buffer[DEFAULT_SECTOR_SIZE];
	unsigned int offset=0;
	int i=0;
	
	//read MBR
	if (dev_desc->block_read (dev_desc->dev, 0, 1, (ulong *) buffer) != 1) {
		printf ("** Can't read partition table on %d:%d **\n",
			dev_desc->dev, 0);
		return -1;
	}
  	//test MBR
	if (!(valid_part_table_flag((char*)buffer)))
		return -1;
	
	pt = (dos_partition_t *) (buffer + DOS_PART_TBL_OFFSET);
	//find the extended partition
	for (i=1;i<5;i++,pt++) {
		if (is_extended(pt->sys_ind)) {
			g_extendPart=i;
			break;
		}
	}
	if (i==5) {
		printf("Please add extended patition first!\n");
		return -1;
	}
	
	g_heads=255;
	g_sectors=0x3f;
	get_partition_table_geometry(buffer);
	g_heads = g_heads ? g_heads :255;
	g_sectors = g_sectors ? g_sectors : 63;
	g_cylinders = dev_desc->lba / (g_heads * g_sectors );

	//adjust the length in cylinder unit
	if (address<0x3f)
		address=0x3f;
	if ((address+length)%(g_sectors*g_heads)) {
		length+=g_sectors*g_heads-((length+address)%(g_sectors*g_heads));
	}
	printf("add_logical: add 0x%x, length 0x%x, total 0x%x\n", address,
		length, address+length-1);
	
	//check the address and length is valid
	if (get_partition_info_dos(dev_desc, g_extendPart, &info[g_extendPart])==0) {
		if ((info[g_extendPart].start>address)||
			((info[g_extendPart].size+info[g_extendPart].start)<(length+address))) {
			printf("The logical address exceeds the extend address!\n");
			printf("The extended part is 0x%x\n", g_extendPart);
			printf("The extend address is from 0x%x to 0x%x\n", 
				info[g_extendPart].start, info[g_extendPart].size+info[g_extendPart].start);
			return -1;
		}
	} else {
		printf("Cannot find extendPart\n");
		return -1;
	}
	
	i=5;
	memset(&info[5], 0, sizeof(info[0])*55);
	while (get_partition_info_dos(dev_desc, i, &info[i])==0) {
		if (((address>=info[i].start)&&(address<(info[i].start+info[i].size)))||
			(((address+length)>=info[i].start)&&((address+length)<(info[i].start+info[i].size)))||
			((address<=info[i].start)&&((address+length)>=(info[i].start+info[i].size))&&(info[i].size!=0))){
			printf("new address 0x%x, length 0x%x, which is overlapped with part %d(0x%x:0x%x)\n",
				address, length, i, info[i].start, info[i].size);
			printf("The address is overlapped, Please input a valid address and length!\n");
			return -1;
		}
		i++;
	}
	g_partition=i-1;
	//printf("g_partition is 0x%x\n", g_partition);
		

	if (g_partition==4)	{//the first logical part
		if (dev_desc->block_read (dev_desc->dev, info[g_extendPart].start, 1, (ulong *) buffer) != 1) {
			printf ("** Can't read partition table on %d:%d **\n",
				dev_desc->dev, 0);
			return -1;
		}
		buffer[DOS_PART_MAGIC_OFFSET] = 0x55;
		buffer[DOS_PART_MAGIC_OFFSET + 1] = 0xaa;
		
		pt = (dos_partition_t *) (buffer + DOS_PART_TBL_OFFSET);
		memset(pt, 0, 64);
		
		setPartition(pt, address, g_sectors, length-g_sectors, LINUX_NATIVE);
		if (dev_desc->block_write (dev_desc->dev, info[g_extendPart].start, 1, (ulong *) buffer) != 1) {
			printf ("** Can't write partition table on %d:%d **\n",
			dev_desc->dev, 0);
			return -1;
		}  
	}else if (g_partition==5){//the second logical part	
		if (dev_desc->block_read (dev_desc->dev, info[g_extendPart].start, 1, (ulong *) buffer) != 1) {
			printf ("** Can't read partition table on %d:%d **\n",
				dev_desc->dev, 0);
			return -1;
		}	
		
		if (!(valid_part_table_flag((char*)buffer)))
			return -1;
		
		pt = (dos_partition_t *) (buffer + DOS_PART_TBL_OFFSET);
		setPartition(++pt, address, address-info[g_extendPart].start, length, EXTENDED);
		memset(++pt, 0, 32);
		
		if (dev_desc->block_write (dev_desc->dev, info[g_extendPart].start, 1, (ulong *) buffer) != 1) {
			printf ("** Can't write partition table on %d:%d **\n",
			dev_desc->dev, 0);
			return -1;
		}  
		
		if (dev_desc->block_read (dev_desc->dev, address, 1, (ulong *) buffer) != 1) {
			printf ("** Can't read partition table on %d:%d **\n",
				dev_desc->dev, 0);
			return -1;
		}
		
		pt = (dos_partition_t *) (buffer + DOS_PART_TBL_OFFSET);
		memset(pt, 0, 66);
		buffer[510]=0x55;
		buffer[511]=0xaa;
		setPartition(pt, address, g_sectors, length-g_sectors, LINUX_NATIVE);
				
		if (dev_desc->block_write (dev_desc->dev, address, 1, (ulong *) buffer) != 1) {
			printf ("** Can't write partition table on %d:%d **\n",
			dev_desc->dev, 0);
			return -1;
		}  

		
	} else {
		printf("Read from 0x%x\n", info[g_partition].start-info[g_partition].offset);
		if (dev_desc->block_read (dev_desc->dev, info[g_partition].start-info[g_partition].offset, 1, (ulong *) buffer) != 1) {
			printf ("** Can't read partition table on %d:%d **\n",
				dev_desc->dev, 0);
			return -1;
		}
		//test MBR
		if (!(valid_part_table_flag((char*)buffer))){
			return -1;
		}
		
		pt = (dos_partition_t *) (buffer + DOS_PART_TBL_OFFSET);
		
		setPartition(++pt, address, address-info[g_extendPart].start, length, EXTENDED);
		memset(++pt, 0, 32);
		
		if (dev_desc->block_write (dev_desc->dev, info[g_partition].start-info[g_partition].offset, 1, (ulong *) buffer) != 1) {
			printf ("** Can't write partition table on %d:%d **\n",
			dev_desc->dev, 0);
			return -1;
		}  
		
		if (dev_desc->block_read (dev_desc->dev, address, 1, (ulong *) buffer) != 1) {
			printf ("** Can't read partition table on %d:%d **\n",
				dev_desc->dev, 0);
			return -1;
		}
		

		pt = (dos_partition_t *) (buffer + DOS_PART_TBL_OFFSET);	
		memset(pt, 0, 66);
		buffer[510]=0x55;
		buffer[511]=0xaa;
		setPartition(pt, address, g_sectors, length-g_sectors, LINUX_NATIVE);
				
		if (dev_desc->block_write (dev_desc->dev, address, 1, (ulong *) buffer) != 1) {
			printf ("** Can't write partition table on %d:%d **\n",
			dev_desc->dev, 0);
			return -1;
		}  

	}
	
	g_partition++;
	return 0;
}

void print_part_dos (block_dev_desc_t *dev_desc)
{
	printf ("Partition     Start Sector     Num Sectors     Type\n");
	print_partition_extended (dev_desc, 0, 0, 1);
}

int get_partition_info_dos (block_dev_desc_t *dev_desc, int part, disk_partition_t * info)
{
	return get_partition_info_extended (dev_desc, 0, 0, 1, part, info);
}

int del_partition_dos(block_dev_desc_t *dev_desc, int part)
{
	return del_partition_dos_extended (dev_desc, 0, 0, part);
}

int add_partition_dos(block_dev_desc_t * dev_desc, int part, int ptype, int address, int length)
{
	switch (ptype) {
		case DOS_PRIMARY:
		case DOS_EXTENDED:
			return add_partition_dos_extended(dev_desc, part, ptype, address, length);
			break;
		case DOS_LOGICAL:
			return add_logical(dev_desc, address, length);
			break;
		default:
			printf("Is not a validate partition type!\n");
			break;
			
	}
	return -1;
}

#endif	/* (CONFIG_COMMANDS & CFG_CMD_IDE) && CONFIG_DOS_PARTITION */
