/*
 * Copyright (c) 2010, The Android Open Source Project.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Neither the name of The Android Open Source Project nor the names
 *    of its contributors may be used to endorse or promote products
 *    derived from this software without specific prior written
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED 
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#include <common.h>
#include <mmc.h>
#include <fastboot.h>
#include "../cpu/arm920t/wmt/sdc.h"   

#define EFI_VERSION 0x00010000
#define EFI_ENTRIES 128
#define EFI_NAMELEN 36
 int load_ptbl(void);
void mmc_part_init(void);
int mmc_part_update(unsigned char *buffer);
extern  int mmc_controller_no;

static const u8 partition_type[16] = {
	0xa2, 0xa0, 0xd0, 0xeb, 0xe5, 0xb9, 0x33, 0x44,
	0x87, 0xc0, 0x68, 0xb6, 0xb7, 0x26, 0x99, 0xc7,
};

static const u8 random_uuid[16] = {
	0xff, 0x1f, 0xf2, 0xf9, 0xd4, 0xa8, 0x0e, 0x5f,
	0x97, 0x46, 0x59, 0x48, 0x69, 0xae, 0xc3, 0x4e,
};
	
struct efi_entry {
	u8 type_uuid[16];
	u8 uniq_uuid[16];
	u64 first_lba;
	u64 last_lba;
	u64 attr;
	u16 name[EFI_NAMELEN];
};

struct efi_header {
	u8 magic[8];

	u32 version;
	u32 header_sz;

	u32 crc32;
	u32 reserved;

	u64 header_lba;
	u64 backup_lba;
	u64 first_lba;
	u64 last_lba;

	u8 volume_uuid[16];

	u64 entries_lba;

	u32 entries_count;
	u32 entries_size;
	u32 entries_crc32;
} __attribute__((packed));

struct ptable {
	u8 mbr[512];
	union {
		struct efi_header header;
		u8 block[512];
	};
	struct efi_entry entry[EFI_ENTRIES];	
};

static void init_mbr(u8 *mbr, u32 blocks)
{
	mbr[0x1be] = 0x00; // nonbootable
	mbr[0x1bf] = 0xFF; // bogus CHS
	mbr[0x1c0] = 0xFF;
	mbr[0x1c1] = 0xFF;

	mbr[0x1c2] = 0xEE; // GPT partition
	mbr[0x1c3] = 0xFF; // bogus CHS
	mbr[0x1c4] = 0xFF;
	mbr[0x1c5] = 0xFF;

	mbr[0x1c6] = 0x01; // start
	mbr[0x1c7] = 0x00;
	mbr[0x1c8] = 0x00;
	mbr[0x1c9] = 0x00;

	//memcpy(mbr + 0x1ca, &blocks, sizeof(u32));
	mbr[0x1ca] =  blocks & 0xff; // start
	mbr[0x1cb] = (blocks >> 8   ) & 0xff;
	mbr[0x1cc] = (blocks >> 16 ) & 0xff;
	mbr[0x1cd] = (blocks >> 24 ) & 0xff;

	mbr[0x1fe] = 0x55;
	mbr[0x1ff] = 0xaa;
}

static void start_ptbl(struct ptable *ptbl, unsigned int blocks)
{
	struct efi_header *hdr = &ptbl->header;
	/*printf("%s,blocks= 0x%x\n", __FUNCTION__,blocks);*/

	memset(ptbl, 0, sizeof(*ptbl));

	init_mbr(ptbl->mbr, blocks - 1);

	memcpy(hdr->magic, "EFI PART", 8);
	hdr->version = EFI_VERSION;
	hdr->header_sz = sizeof(struct efi_header);
	hdr->header_lba = 1;
	hdr->backup_lba = blocks - 1;
	hdr->first_lba = 34;
	hdr->last_lba = blocks - 1;
	memcpy(hdr->volume_uuid, random_uuid, 16);
	hdr->entries_lba = 2;
	hdr->entries_count = EFI_ENTRIES;
	hdr->entries_size = sizeof(struct efi_entry);
}

static void end_ptbl(struct ptable *ptbl)
{
	struct efi_header *hdr = &ptbl->header;
	u32 n;
	/*printf("%s\n", __FUNCTION__);*/

	n = crc32(0, 0, 0);
	n = crc32(n, (void*) ptbl->entry, sizeof(ptbl->entry));
	hdr->entries_crc32 = n;

	n = crc32(0, 0, 0);
	n = crc32(0, (void*) &ptbl->header, sizeof(ptbl->header));
	hdr->crc32 = n;
}

int add_ptn(struct ptable *ptbl, u64 first, u64 last, char *name)
{
	struct efi_header *hdr = &ptbl->header;
	struct efi_entry *entry = ptbl->entry;
	unsigned n;
	/*printf("%s\n", __FUNCTION__);*/

	if (first < 34) {
		printf("partition '%s' overlaps partition table\n", name);
		return -1;
	}

	if (last > hdr->last_lba) {
		printf("partition '%s' does not fit\n", name);
		return -1;
	}
	for (n = 0; n < EFI_ENTRIES; n++, entry++) {
		if (entry->last_lba)
			continue;
		memcpy(entry->type_uuid, partition_type, 16);
		memcpy(entry->uniq_uuid, random_uuid, 16);
		entry->uniq_uuid[0] = n;
		entry->first_lba = first;
		entry->last_lba = last;
		for (n = 0; (n < EFI_NAMELEN) && *name; n++)
			entry->name[n] = *name++;
		return 0;
	}
	printf("out of partition table entries\n");
	return -1;
}

void import_efi_partition(struct efi_entry *entry)
{
	struct fastboot_ptentry e;
	int n;
	/*printf("%s\n", __FUNCTION__);*/
	if (memcmp(entry->type_uuid, partition_type, sizeof(partition_type)))
		return;
	for (n = 0; n < (sizeof(e.name)-1); n++)
		e.name[n] = entry->name[n];
	e.name[n] = 0;
	e.start = entry->first_lba;
	e.length = (entry->last_lba - entry->first_lba + 1) * 512;
	e.flags = 0;

	if (!strcmp(e.name,"environment"))
		e.flags |= FASTBOOT_PTENTRY_FLAGS_WRITE_ENV;
	fastboot_flash_add_ptn(&e);
	//printf("c:start:%8d length:%7d name:%s\n", e.start, e.length, e.name);  //Charles
	if (e.length > 0x100000)
		printf("%8d %7dM %s\n", e.start, e.length/0x100000, e.name);
	else
		printf("%8d %7dK %s\n", e.start, e.length/0x400, e.name);
}
 int load_ptbl(void)
{
	static unsigned char data[512];
	static struct efi_entry entry[4];
	int n,m,r;
	/*printf("%s\n", __FUNCTION__);*/
	r = mmc_bread(mmc_controller_no, 1, 1, (void*)data);

	if (r == -1) {
		printf("error reading partition table\n");
		return -1;
	}
	if (memcmp(data, "EFI PART", 8)) {
		printf("efi partition table not found\n");
		return -1;
	}
	for (n = 0; n < (128/4); n++) {
		r = mmc_bread(mmc_controller_no, 1+n, 1, (void*) entry);
		if (r == -1) {
			printf("partition read failed\n");
			return 1;
		}
		for (m = 0; m < 4; m ++)  {
			//printf("entry : 0x%x, n : %d, m:%d\n",entry,n,m);
			import_efi_partition(entry + m);
		}	
	}
	return 0;
}

struct partition {
	char *name;
	unsigned size_kb;
};
/*WM3481 emmc partition*/
static struct partition partitions[] = {
	{ "-", 17 },
	{ "system", 1024*1024},
	{ "boot", 16*1024},
	{ "recovery", 16*1024},
	{ "cache", 512*1024},
	{ "swap", 128*1024},
	{ "u-boot-logo", 2*1024},
	{ "kernel-logo", 4*1024},
	{ "misc", 2*1024},
	{ "efs", 2*1024},
	{ "radio", 2*1024},
	{ "keydata", 2*1024},
	{ "userdata", 0},
	{ 0, 0 },
};
static struct ptable the_ptable;
extern sd_info_t *SDDevInfo;

extern ulong mmc_bwrite(int dev_num, ulong blknr, ulong blkcnt, ulong *src);

static int do_format(void)
{
	struct ptable *ptbl = &the_ptable;
	unsigned int sector_sz, blocks;
	unsigned next;
	unsigned long block_cnt=0;
	int n;
	//printf("%s,mmc_controller_no:%x\n", __FUNCTION__,mmc_controller_no);

	if (mmc_init(1,mmc_controller_no)) {
		printf("mmc init failed?\n");
		return -1;
	}
     	blocks = SDDevInfo->SDCard_Size;
	sector_sz = 512;
	/*printf("total blocks %d , sector_sz %d\n", blocks, sector_sz);*/
	start_ptbl(ptbl, blocks);
	n = 0;
	next = 0;
	for (n = 0, next = 0; partitions[n].name; n++) {
		unsigned sz = partitions[n].size_kb * 2;
		if (!strcmp(partitions[n].name,"-")) {
			next += sz;
			continue;
		}
		if (sz == 0)
			sz = blocks - next;
		//printf("next:%d ,  next + sz - 1 : %d, n:%d, %s\n", next,  next + sz - 1 , n, partitions[n].name);	
		if (add_ptn(ptbl, next, next + sz - 1, partitions[n].name))
			return -1;
		next += sz;
	}
	end_ptbl(ptbl);

	block_cnt = sizeof(struct ptable) /512;
	if ( sizeof(struct ptable) % 512)
		block_cnt++ ;
	//printf(" block_cnt =%d,sizeof(struct ptable) =%d\n", block_cnt, sizeof(struct ptable));

	if (mmc_bwrite(mmc_controller_no, 0, block_cnt, (void*) ptbl) == -1) {
		printf(" mmc bwrite fail\n");
		return -1;
	}
	//printf("new partition table: block_cnt =%d,sizeof(struct ptable) =%d\n",block_cnt,sizeof(struct ptable) );
	load_ptbl();

	return 0;
}

int fastboot_oem(const char *cmd)
{
	if(memcmp(cmd, "format", 6) == 0) 
		return do_format();
	return -1;
}

/*reserved function*/
void mmc_part_init(void)
{
	char *buf;
	char size[8];
	int len_name,len_size;
      char *name;
	unsigned int min_partition_size;
	unsigned int value,i;
	char *tmp;
	int delim;
	buf = getenv("wmt.mmcpart.param");
	if (!buf) {
		min_partition_size = 0x200000; //default 2MB
		printf("default min_partition_size :0x%x\n", min_partition_size);
	} else {
		for (i = 1; i < 16; i++) {
			delim = ':';
			if (delim) {
				char *p;
				char *size_mb;
				memset(size, 0, 8);
				name= buf;
				//printf(" partition name for name \'%s\'\n", name);
				p  = strchr((char *)name, delim);
				//printf(" partition name for p \'%s\'\n",p);
				if (!p) {
					printf(" closing %c not found in partition name\n", delim);
					printf("no partition name for \'%s\'\n", name);
				}
				len_name =p - name;
				strncpy(partitions[i].name, buf, len_name);
				buf = buf + 1 + len_name;
				//printf(" partition name for buf  \'%s\'\n",buf);
				size_mb = strchr((char *)name, ',');
				//printf(" partition name for size_mb \'%s\'\n",size_mb);
				len_size =  size_mb-buf;
				strncpy(size, buf, len_size);	
				//printf(" len_size %d ,size %s\n",len_size, size);
				buf = buf +  len_size;
				value = simple_strtoul(size, &tmp, 16);
				//printf(" partition name  \'%s\'\n", partitions[i].name);
				//printf(" partition size_mb  \'0x%x KB\'\n",value);

				} else {
					printf("no partition name for \'%s\'\n", name);
			}

			partitions[i].size_kb = value ; 
			printf(" mmc part init %s 0x%x KB \n" , partitions[i].name , partitions[i].size_kb);

			if (*buf != ',') {
				break;
			}
			buf += 1; // skip ','
		}

	}
}

int mmc_part_update(unsigned char *buffer)
{
	char *buf;
	char size[8];
	int len_name,len_size;
      char *name;
	unsigned int value,i;
	char *tmp;
	int delim;
	buf = (char *)buffer;
	if (!buf) {
		printf("mmc_part_update fail !\n");
		return 1;
	} else {
	
		for (i = 1; i < 16; i++) {
			delim = ':';
			if (delim) {
				char *p;
				char *size_mb;
				memset(size, 0, 8);
				name= buf;
				//printf(" partition name for name \'%s\'\n", name);				
				p  = strchr((char *)name, delim);
				if (!p) {
					printf(" closing %c not found in partition name\n", delim);
					printf("no partition name for \'%s\'\n", name);
				}
				len_name = p - name;
				strncpy(partitions[i].name, buf, len_name);
				buf = buf + 1 + len_name;
				size_mb = strchr((char *)name, ',');
				len_size =  size_mb-buf;
				strncpy(size, buf, len_size);	
				buf = buf +  len_size;
				value = simple_strtoul(size, &tmp, 16);
				} else {
					printf("no partition name for \'%s\'\n", name);
			}
			partitions[i].size_kb = value ; 
			printf(" mmc part update %s 0x%x KB \n" , partitions[i].name , partitions[i].size_kb);
			if (*buf != ',') {
				break;
			}
			buf += 1; // skip ','
		}
	}

	return 0;
}

int board_late_init(void)
{
	if (mmc_init(1,mmc_controller_no)) {
		printf("mmc init failed?\n");
		return 1;
	}
	printf("\nefi partition table:\n");
	return load_ptbl();
}

