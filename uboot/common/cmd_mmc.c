/*
 * (C) Copyright 2003
 * Kyle Harris, kharris@nexus-tech.net
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

#include <common.h>
#include <command.h>


#if (CONFIG_COMMANDS & CFG_CMD_MMC)

#include <mmc.h>
#include <environment.h>
#include <fastboot.h>
#include <part.h>
#include <fastboot.h>

//#define	RSA_DEBUG 1
#ifdef	RSA_DEBUG
#define	RSA_PRINTF(fmt,args...)	printf (fmt ,##args)
#else
#define  RSA_PRINTF(fmt,args...)
#endif
static   block_dev_desc_t *mmc_cur_dev = NULL;
static  unsigned long mmc_part_offset =0;
static  int mmc_cur_part =1;
struct sig_header {
    unsigned int img_size;      //kernel signature size in bytes
    unsigned int reserved[3];   //reserved.
};
extern  int load_ptbl(void) ; //Charles

int mmc_register_device(block_dev_desc_t *dev_desc, int part_no);
extern int image_rsa_check(
unsigned int image_mem_addr,
unsigned int image_size,
unsigned int sig_addr,
unsigned int sig_size);

static int sd_check_ctrlc(void)
{
    extern int ctrlc (void);
    if( ctrlc()){
        printf("Abort\n");
        return 1;
    }
    return 0;
}

int do_mmc_wait_insert(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
    SD_Controller_Powerup();
	int insert = simple_strtoul(argv[1], NULL, 10);
	if(insert){
		printf("waiting insert SD card\n");
		while(SD_card_inserted()!=1){
			if( sd_check_ctrlc())
                    return -1;
		}
		return 0;

	}else{
		printf("wainting remove SD card\n");
		while(SD_card_inserted()!=0){
			if( sd_check_ctrlc())
                    return -1;
            udelay(500000);//delay 500ms
		}
		
		return 0;
	}
}

int do_mmc (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	int device_num = 1;
	ulong dev_id = 0;
	
	if (argc <= 1) {
		printf("Usage:\n%s\n", cmdtp->usage);
		return 1;
	}
	
	if (argc == 2)
		dev_id = simple_strtoul(argv[1], NULL, 16);
	if (dev_id == 0 || dev_id == 1 || dev_id == 2)
		device_num = dev_id;

	if (mmc_init (1, (int)device_num) != 0) {
		//printf ("No MMC card found\n");
		return 1;
	}
	return 0;
}

int do_mmc_read (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	int device_num = 1;
	ulong dev_id = 0;
	ulong addr,block_num,bytes,blk_cnt;
	ulong ret;
		
	if (argc < 4) {
		printf("Usage:\n%s\n", cmdtp->usage);
		return 1;
	}
	
	/*get device id*/
	dev_id = simple_strtoul(argv[1], NULL, 16);
	if (dev_id == 0 || dev_id == 1 || dev_id == 2)
		device_num = dev_id;
	else {
		printf("dev_id Invalid\n");
		return 1;
	}

	/*get memory address*/
	addr = simple_strtoul (argv[2], NULL, 16);
	if (addr < 0) {
		printf("addr Invalid\n");
		return 1;
	}

	/*get card block address*/
	block_num = simple_strtoul (argv[3], NULL, 16);
	if (block_num < 0) {
		printf("block_num Invalid\n");
		return 1;
	}

	/*get transfer size is bytes*/
	bytes = simple_strtoul (argv[4], NULL, 16);
	if (bytes < 0) {
		printf("bytes Invalid\n");
		return 1;
	}

	if (bytes == 0)
		return 0;

	/*calculate transfer block count*/
	blk_cnt = (bytes / 512);
	if (bytes % 512)
		blk_cnt++;
	
	//printf("device_num = %x block_num = %x addr =  %x bytes = %x blk_cnt =%x\n",device_num,block_num,addr,bytes,blk_cnt);

	ret = mmc_bread(device_num,block_num,blk_cnt,(ulong *)addr);

	if (ret != blk_cnt) {
		printf("Read Data Fail\n");
		return 1;
	} else {
		printf("Read Data Success\n");
	}
		

	return 0;
}

int do_mmc_write (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	int device_num = 1;
	ulong dev_id = 0;
	ulong addr,block_num,bytes,blk_cnt;
	ulong ret;
		
	if (argc < 4) {
		printf("Usage:\n%s\n", cmdtp->usage);
		return 1;
	}
	
	/*get device id*/
	dev_id = simple_strtoul(argv[1], NULL, 16);
	if (dev_id == 0 || dev_id == 1 || dev_id == 2)
		device_num = dev_id;
	else {
		printf("dev_id Invalid\n");
		return 1;
	}

	/*get memory address*/
	addr = simple_strtoul (argv[2], NULL, 16);
	if (addr < 0) {
		printf("addr Invalid\n");
		return 1;
	}

	/*get card block address*/
	block_num = simple_strtoul (argv[3], NULL, 16);
	if (block_num < 0) {
		printf("block_num Invalid\n");
		return 1;
	}

	/*get transfer size is bytes*/
	bytes = simple_strtoul (argv[4], NULL, 16);
	if (bytes < 0) {
		printf("bytes Invalid\n");
		return 1;
	}
	
	if (bytes == 0)
		return 0;

	/*calculate transfer block count*/
	blk_cnt = (bytes / 512);
	if (bytes % 512)
		blk_cnt++;
	
	//printf("device_num = %x block_num = %x addr =  %x bytes = %x blk_cnt =%x\n",device_num,block_num,addr,bytes,blk_cnt);
	ret = mmc_bwrite(device_num,block_num,blk_cnt,(ulong *)addr);

	if (ret != blk_cnt) {
		printf("Write Data Fail\n");
		return 1;
	} else {
		printf("Write Data Success\n");
	}
		

	return 0;
}

 void memdump (void *pv, int num)
{
	unsigned int  tmp,ba;
	ba =(unsigned int) pv;
	for (tmp = 0;tmp < num/16; tmp++)
	printf("[%8.8x]%8.8x %8.8x %8.8x %8.8x\n",(0x10*tmp+ba),*(volatile unsigned int *)(ba+(tmp*0x10)),*(volatile unsigned int *)(ba+(tmp*0x10)+0x4),*(volatile unsigned int *)(ba+(tmp*0x10)+0x8),*(volatile unsigned int *)(ba+(tmp*0x10)+0xc));
}
extern int do_setenv ( cmd_tbl_t *cmdtp, int flag, int argc, char *argv[]);
int do_saveenv (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[]);
extern block_dev_desc_t *get_dev (char*, int);
extern fastboot_ptentry *fastboot_flash_find_ptn(const char *name);
extern int mmc_controller_no ; //Charles
int fastboot_part_type = 0;
int do_mmc_read_img (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	int device_num = 1;
	int dev_id = 0;
	ulong kernel_addr,ramdisk_addr;
	ulong blk_cnt_kenel, blk_cnt_ramdisk, page_cnt_kenel, page_cnt_ramdisk;
	ulong ret;
	static unsigned char data[512];
	block_dev_desc_t *dev_desc=NULL;
	ulong part=1;
	char *ep;
	unsigned int  page_size, kernel_size, ramdisk_size, total_size,total_page;
	
	char var[64], val[32];
	char *setenv[4]  = { "setenv", NULL, NULL, NULL, };
	setenv[1] = var;
	setenv[2] = val;

	char *key = NULL;
	unsigned int  pub_key_size;
	unsigned int image_mem_addr = 0x01E00000;  //addr 30M 
	unsigned int image_size;
	unsigned int sig_addr = 0x02E00000;
	unsigned int sig_size;
	int rcode = 0, pub_key_flag = 0;
	unsigned char idx_max = 8, i = 0;
	char *p = NULL;
	char ps[idx_max];
	char * endp;

	key = getenv("wmt.rsa.pem");
	pub_key_size = strlen(key);
	if (!key) {
		printf("No RSA public key !! \n");
		pub_key_flag = 0;
	} else {
		pub_key_flag = 1;
	}
	p = getenv("wmt.fb.param");
	if (p) {
		while (i < idx_max) {
			ps[i++] = simple_strtoul(p, &endp, 16);
			if (*endp == '\0')
				break;
			p = endp + 1;
			if (*p == '\0')
				break;        
		}
		fastboot_part_type= ps[3];//support MBR or GPT
	}
	/*RSA_PRINTF("pub_key_size=0x%x, key=%s\n", pub_key_size, key);*/	

	if (argc < 4) {
		printf("Usage:\n%s\n", cmdtp->usage);
		return 1;
	}
	/*get device id*/
	dev_id = (int)simple_strtoul (argv[2], &ep, 16);
	if (dev_id == 0 || dev_id == 1 || dev_id == 2 || dev_id == 3)  {
		device_num = dev_id;
		mmc_controller_no = dev_id;
	}	
	else {
		printf("dev_id Invalid\n");
		return 1;
	}
	/*get mmc dev descriptor*/
	dev_desc= get_dev(argv[1], dev_id);
	if (dev_desc == NULL) {
		puts ("\n** Invalid boot device **\n");
		return 1;
	}
	if (*ep) {
		if (*ep != ':') {
			puts ("\n** Invalid boot device, use `dev[:part]' **\n");
			return 1;
		}
		part = (int)simple_strtoul(++ep, NULL, 16);
	}

	/*printf("dev_id:%x,part:%x\n",dev_id,part);*/
	/* init mmc controller */	
	if (mmc_init(1, device_num)) {
		printf("mmc init failed?\n");
		return 1;
	}
	if (mmc_register_device(dev_desc, part) != 0) {
		printf ("\n** Unable to use %s %d:%d for fatload **\n", argv[1], dev_id, part);
		return 1;
	}
 		

	/*get kernel memory address*/
	kernel_addr = simple_strtoul (argv[3], NULL, 16);
	if (kernel_addr < 0) {
		printf("kernel addr Invalid\n");
		return 1;
	}

	/*get ramdisk memory address*/
	ramdisk_addr = simple_strtoul (argv[4], NULL, 16);
	if (ramdisk_addr < 0) {
		printf("ramdisk addr  Invalid\n");
		return 1;
	}
	/*mmc block read partition offset sectors*/
	ret = mmc_bread(device_num, mmc_part_offset, 1, (void *)data);
	if (ret == -1  ) {
		printf("Read Bootimg Header Fail\n");
		return 1;
	} else {
		RSA_PRINTF("Read Bootimg Header Success\n");
	}

	/*memdump ((void *)data, 512);*/
	struct fastboot_boot_img_hdr *fb_hdr = (struct fastboot_boot_img_hdr *)data;
	
	if (memcmp(data, "ANDROID!", 8)) {
		printf(" boot.img  partition table not found\n");
		return -1;
	}
	/*kernel size change to sectors*/
	kernel_size = fb_hdr->kernel_size ;
	ramdisk_size = fb_hdr->ramdisk_size;
	page_size = fb_hdr->page_size ; 
	total_size = 1*page_size + kernel_size + ramdisk_size;

	/* calculate transfer block count */
	blk_cnt_kenel = (fb_hdr->kernel_size / 512);
	if (fb_hdr->kernel_size % 512)
		blk_cnt_kenel++;
	
	page_cnt_kenel = (fb_hdr->kernel_size / page_size);
	if (fb_hdr->kernel_size % page_size)
		page_cnt_kenel++;
	
	blk_cnt_ramdisk = (fb_hdr->ramdisk_size / 512);
	if (fb_hdr->ramdisk_size % 512)
		blk_cnt_ramdisk++;
	
	page_cnt_ramdisk = (fb_hdr->ramdisk_size / page_size);
	if (fb_hdr->ramdisk_size % page_size)
		page_cnt_ramdisk++;
	
	total_page = 1 + page_cnt_kenel + page_cnt_ramdisk;
	image_size = total_page * page_size;
	
	RSA_PRINTF("rsa page_cnt_kenel :0x%x, page_cnt_ramdisk :0x%x \n", page_cnt_kenel, page_cnt_ramdisk);
	RSA_PRINTF("rsa total_page :0x%x, image_size :0x%x \n", total_page, image_size);
	/*if pub_key_flag are set ,then verify signed bootimg signature*/
	if (pub_key_flag == 1) { 
		/*read total signed boot image*/	
		ret = mmc_bread(device_num, mmc_part_offset , (image_size/512) + 1, (void *)image_mem_addr);
		if (ret == -1  ) {
			printf("Read Bootimg Fail\n");
			return 1;
		} else {
			printf("Read Bootimg Success\n");
		}
		/* image sign signature addr */
		sig_addr = image_mem_addr + image_size;
		/*memdump ((void *)sig_addr, 512);*/
		
	      	struct sig_header *sig_hdr = (struct sig_header *)sig_addr;
		sig_size = sig_hdr->img_size ;
		/*memdump ((void *)sig_addr + 0x10, 256);*/
	     
		rcode = image_rsa_check(image_mem_addr, image_size, sig_addr + 0x10, sig_size);
	       if (rcode != 0 ) {
	   		printf("\nImage RSA Check Fail, rcode :%x\n", rcode);	
		   	return 1;  /* rsa verify fail */
	        } else {
      	   			printf("\nImage RSA Check Success\n");	
					/*No need copy to kernel addr  */
				memcpy ((void *)kernel_addr, (void *)image_mem_addr + 1*page_size, page_cnt_kenel*page_size);
					/*copy to ramdisk addr  */
				memcpy ((void *)ramdisk_addr, (void *)image_mem_addr + 1*page_size + page_cnt_kenel*page_size, page_cnt_ramdisk*page_size);
				
	        }
	}else {
		/* image_mem_addr  0x01E00000 */
		/*ret = mmc_bread(device_num, mmc_part_offset , image_size/512 , image_mem_addr);*/

		/* load kernel image from mmc boot image to kernel_addr */
		RSA_PRINTF(" mmc_part_offset + page_size:0x%x, fb_hdr->kernel_size:0x%x, mem kernel_addr:0x%x\n", 
			mmc_part_offset + page_size / 512, fb_hdr->kernel_size, kernel_addr);
	    	ret = mmc_bread(device_num, mmc_part_offset + page_size / 512, blk_cnt_kenel, (void *)kernel_addr);
		if (ret == -1  ) {
			printf("Read Kernel Data Fail\n");
			return 1;
		} else {
			printf("Read Kernel Data Success\n");
		}

		/* load ramdisk image from mmc boot image to ramdisk_addr */
		RSA_PRINTF("mmc_part_offset + page_size + blk_cnt_kenel:0x%x, fb_hdr->ramdisk_size:0x%x, mem ramdisk_addr:0x%x\n",
				mmc_part_offset + page_size /512 + page_cnt_kenel *(page_size/512), fb_hdr->ramdisk_size, ramdisk_addr);
	    	ret = mmc_bread(device_num, mmc_part_offset + page_size /512 + page_cnt_kenel *(page_size /512), blk_cnt_ramdisk, (void *)ramdisk_addr);
		if (ret == -1  ) {
			printf("Read Ramdisk Data Fail\n");
			return 1;
		} else {
			printf("Read Ramdisk Data Success\n");
		}
	}
	/*save ramdisk size to env argv[4]*/
	sprintf (var, "%s", argv[5]);
	sprintf (val, "%x", fb_hdr->ramdisk_size);
	do_setenv (NULL, 0, 3, setenv);

	printf("%s %s %s\n", setenv[0], setenv[1], setenv[2]);
	/*do_saveenv (NULL, 0, 1, saveenv);*/

	return 0;
}
int do_mem_read_img (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	int device_num = 1;
	int dev_id = 0;
	ulong kernel_addr,ramdisk_addr;
	ulong blk_cnt_kenel, blk_cnt_ramdisk, page_cnt_kenel, page_cnt_ramdisk;
	ulong ret;
	static unsigned char data[512];
	block_dev_desc_t *dev_desc=NULL;
	ulong part=1;
	char *ep;
	unsigned int  page_size, kernel_size, ramdisk_size, total_size,total_page;
	
	char var[64], val[32];
	char *setenv[4]  = { "setenv", NULL, NULL, NULL, };
	setenv[1] = var;
	setenv[2] = val;

	char *key = NULL;
	unsigned int  pub_key_size;
	unsigned int image_mem_addr = 0x01E00000;  //addr 30M 
	unsigned int image_size;
	unsigned int sig_addr = 0x02E00000;
	unsigned int sig_size;
	int rcode = 0, pub_key_flag = 0;
	unsigned char idx_max = 8, i = 0;
	char *p = NULL;
	char ps[idx_max];
	char * endp;

	key = getenv("wmt.rsa.pem");
	pub_key_size = strlen(key);
	if (!key) {
		printf("No RSA public key !! \n");
		pub_key_flag = 0;
	} else {
		pub_key_flag = 1;
	}

	if (argc < 4) {
		printf("Usage:\n%s\n", cmdtp->usage);
		return 1;
	}
	/*get memory bootimg address */
	image_mem_addr= (int)simple_strtoul (argv[1], &ep, 16);
	if (image_mem_addr < 0) {
		printf("image_mem_addr Invalid\n");
		return 1;
	} 		

	/*get kernel memory address*/
	kernel_addr = simple_strtoul (argv[2], NULL, 16);
	if (kernel_addr < 0) {
		printf("kernel addr Invalid\n");
		return 1;
	}

	/*get ramdisk memory address*/
	ramdisk_addr = simple_strtoul (argv[3], NULL, 16);
	if (ramdisk_addr < 0) {
		printf("ramdisk addr Invalid\n");
		return 1;
	}

	/*memdump ((void *)data, 512);*/
	struct fastboot_boot_img_hdr *fb_hdr = (struct fastboot_boot_img_hdr *)image_mem_addr;
	
	if (memcmp(image_mem_addr, "ANDROID!", 8)) {
		printf(" boot.img  partition table not found\n");
		return -1;
	}
	/*kernel size change to sectors*/
	kernel_size = fb_hdr->kernel_size ;
	ramdisk_size = fb_hdr->ramdisk_size;
	page_size = fb_hdr->page_size ; 
	total_size = 1*page_size + kernel_size + ramdisk_size;

	/* calculate transfer block count */
	blk_cnt_kenel = (fb_hdr->kernel_size / 512);
	if (fb_hdr->kernel_size % 512)
		blk_cnt_kenel++;
	
	page_cnt_kenel = (fb_hdr->kernel_size / page_size);
	if (fb_hdr->kernel_size % page_size)
		page_cnt_kenel++;
	
	blk_cnt_ramdisk = (fb_hdr->ramdisk_size / 512);
	if (fb_hdr->ramdisk_size % 512)
		blk_cnt_ramdisk++;
	
	page_cnt_ramdisk = (fb_hdr->ramdisk_size / page_size);
	if (fb_hdr->ramdisk_size % page_size)
		page_cnt_ramdisk++;
	
	total_page = 1 + page_cnt_kenel + page_cnt_ramdisk;
	image_size = total_page * page_size;
	
	RSA_PRINTF("page_cnt_kenel :0x%x, page_cnt_ramdisk :0x%x \n", page_cnt_kenel, page_cnt_ramdisk);
	RSA_PRINTF("total_page :0x%x, image_size :0x%x \n", total_page, image_size);
	/*if pub_key_flag are set ,then verify signed bootimg signature*/
	if (pub_key_flag == 1) { 
		
		/* image sign signature addr */
		sig_addr = image_mem_addr + image_size;
		memdump ((void *)sig_addr, 512);
		
	    struct sig_header *sig_hdr = (struct sig_header *)sig_addr;
		sig_size = sig_hdr->img_size ;
		memdump ((void *)sig_addr + 0x10, 256);
	     
		rcode = image_rsa_check(image_mem_addr, image_size, sig_addr + 0x10, sig_size);
       if (rcode != 0 ) {
	   		printf("\nImage RSA Check Fail, rcode :%x\n", rcode);	
		   	return 1;  /* rsa verify fail */
        } else {
			printf("\nImage RSA Check Success\n");	
			/*No need copy to kernel addr  */
			memcpy ((void *)kernel_addr, (void *)image_mem_addr + 1*page_size, page_cnt_kenel*page_size);
			/*copy to ramdisk addr  */
			memcpy ((void *)ramdisk_addr, (void *)image_mem_addr + 1*page_size + page_cnt_kenel*page_size,
			page_cnt_ramdisk*page_size);			
        }
	}else {
		/* load kernel image from mem boot image to kernel_addr */
		RSA_PRINTF("fb_hdr->kernel_size:0x%x, mem kernel_addr:0x%x\n", 
		fb_hdr->kernel_size, kernel_addr);
		/*need copy to kernel addr  */
		memcpy ((void *)kernel_addr, (void *)image_mem_addr + 1*page_size, page_cnt_kenel*page_size);
		/*copy to ramdisk addr  */
		memcpy ((void *)ramdisk_addr, (void *)image_mem_addr + 1*page_size + page_cnt_kenel*page_size,
		page_cnt_ramdisk*page_size);
		/* load ramdisk image from mmc boot image to ramdisk_addr */
		RSA_PRINTF("fb_hdr->ramdisk_size:0x%x, mem ramdisk_addr:0x%x\n",
		fb_hdr->ramdisk_size, ramdisk_addr);
	}
	/*save ramdisk size to env argv[4]*/
	sprintf (var, "%s", argv[4]);
	sprintf (val, "%x", fb_hdr->ramdisk_size);
	do_setenv (NULL, 0, 3, setenv);

	printf("%s %s %s\n", setenv[0], setenv[1], setenv[2]);
	/*do_saveenv (NULL, 0, 1, saveenv);*/

	return 0;
}

struct partition {
	const char *name;
	unsigned size_kb;
};
#define GPT 1
int mmc_register_device(block_dev_desc_t *dev_desc, int part_no)
{
	unsigned char buffer[0x200];
	disk_partition_t info;
	struct fastboot_ptentry *ptn;
	char *name;
	if (!dev_desc->block_read)
		return -1;
	mmc_cur_dev=dev_desc;
	/* check if we have a MBR (on floppies we have only a PBR) */
	if (dev_desc->block_read (dev_desc->dev, 0, 1, (ulong *) buffer) != 1) {
		printf ("** Can't read from device %d **\n", dev_desc->dev);
		return -1;
	}
	if (part_no == 2)	{
		name ="boot";
	} else if (part_no == 7) {
		name ="u-boot-logo";
	} else if (part_no == 8) {
		name ="kernel-logo";
	} else {
		name ="undefine";
	}

	/*ext4 GUID partition */
	if (fastboot_part_type == GPT) {
		load_ptbl();
		ptn = fastboot_flash_find_ptn(name);
		if (ptn == 0) {
			printf("%s partition does not exist", name);
			return -1;
		}
		mmc_part_offset = ptn->start;
		mmc_cur_part = part_no;
	} else {
	/*FAT partition*/
     
	if(!get_partition_info(dev_desc, part_no, &info)) {
			mmc_part_offset = info.start;
			mmc_cur_part = part_no;
			printf("part_offset : %x, cur_part : %x\n", mmc_part_offset, mmc_cur_part);
	} else {
#if 1
			mmc_part_offset = info.start;
			mmc_cur_part = part_no;

			//printf ("**Partition %d not valid on device %d **\n",part_no,dev_desc->dev);
			//return -1;
#else

			part_offset=32;
			cur_part = 1;
#endif
		}
	}	
	return 0;
}

int do_mmc_load_guid_img (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	int device_num = 1;
	int dev_id = 0;
	ulong logo_addr,logo_size;
	ulong blk_cnt_logo;
	ulong ret;
	block_dev_desc_t *dev_desc=NULL;
	ulong part=1;
	char *ep;
	
	char var[64], val[32];
	char *setenv[4]  = { "setenv", NULL, NULL, NULL, };
	setenv[1] = var;
	setenv[2] = val;

	unsigned char idx_max = 8, i = 0;
	char *p = NULL;
	char ps[idx_max];
	char * endp;
	
	p = getenv("wmt.fb.param");
	if (p) {
		while (i < idx_max) {
			ps[i++] = simple_strtoul(p, &endp, 16);
			if (*endp == '\0')
				break;
			p = endp + 1;
			if (*p == '\0')
				break;        
		}
		fastboot_part_type= ps[3];//support MBR or GPT
	}

	if (argc < 4) {
		printf("Usage:\n%s\n", cmdtp->usage);
		return 1;
	}
	/*get device id*/
	dev_id = (int)simple_strtoul (argv[2], &ep, 16);
	if (dev_id == 0 || dev_id == 1 || dev_id == 2 || dev_id == 3)  {
		device_num = dev_id;
		mmc_controller_no = dev_id;
	}	
	else {
		printf("dev_id Invalid\n");
		return 1;
	}
	/*get mmc dev descriptor*/
	dev_desc= get_dev(argv[1], dev_id);
	if (dev_desc == NULL) {
		puts ("\n** Invalid boot device **\n");
		return 1;
	}
	if (*ep) {
		if (*ep != ':') {
			puts ("\n** Invalid boot device, use `dev[:part]' **\n");
			return 1;
		}
		part = (int)simple_strtoul(++ep, NULL, 16);
	}
	/*printf("dev_id=%d,part=%d\n", dev_id, part);*/
	/* init mmc controller */	
	if (mmc_init(1, device_num)) {
		printf("mmc init failed?\n");
		return 1;
	}
	if (mmc_register_device(dev_desc, part) != 0) {
		printf ("\n** Unable to use %s %d:%d for fatload or guidload **\n", argv[1], dev_id, part);
		return 1;
	}
 		
	/*get logo memory address*/
	logo_addr = simple_strtoul (argv[3], NULL, 16);
	if (logo_addr < 0) {
		printf("logo addr Invalid\n");
		return 1;
	}

	/*get logo size*/
	logo_size = simple_strtoul (argv[4], NULL, 16);
	if (logo_size < 0) {
		printf("logo_size  Invalid\n");
		return 1;
	}
	blk_cnt_logo = (logo_size / 512);
	if (logo_size % 512)
		blk_cnt_logo++;

	/*mmc block read partition offset sectors*/
	ret = mmc_bread(device_num, mmc_part_offset, blk_cnt_logo, (void *)logo_addr);
	if (ret == -1  ) {
		printf("Read Logo Fail\n");
		return 1;
	} else {
		RSA_PRINTF("Read Logo Success\n");
	}

	/*memdump ((void *)logo_addr, 512);*/

	return 0;
}

U_BOOT_CMD(
	mmcinit,	2,	1,	do_mmc,
	"mmcinit - init mmc card\n"
	"  mmcinit 0 -- init mmc device 0 \n"
	"  mmcinit 1 -- init mmc device 1 \n"
	"  mmcinit 2 -- init mmc device 2 \n",
	"mmcinit - init mmc card\n"
	"  mmcinit 0 -- init mmc device 0 \n"
	"  mmcinit 1 -- init mmc device 1 \n"
	"  mmcinit 2 -- init mmc device 2 \n"
);

U_BOOT_CMD(
	mmcread,	5,	1,	do_mmc_read,
	"mmcread - read data from SD/MMC card\n"
	"  <dev_id> <addr> <block_num> <bytes>\n"
	"   -read data from SD/MMC card block address 'block_num' on 'dev_id'\n"
	"    to memory address 'addr' size is 'bytes'\n",
	"mmcread - read data from SD/MMC card\n"
	"  <dev_id> <addr> <block_num> <bytes>\n"
	"   -read data from SD/MMC card block address 'block_num' on 'dev_id'\n"
	"    to memory address 'addr' size is 'bytes'\n"
);

U_BOOT_CMD(
	mmcwrite,	5,	1,	do_mmc_write,
	"mmcwrite - write data to SD/MMC card\n"
	"  <dev_id> <addr> <block_num> <bytes>\n"
	"   -write data to SD/MMC card block address 'block_num' on 'dev_id'\n"
	"    from memory address 'addr' size is 'bytes'\n",
	"mmcwrite - write data to SD/MMC card\n"
	"  <dev_id> <addr> <block_num> <bytes>\n"
	"   -write data to SD/MMC card block address 'block_num' on 'dev_id'\n"
	"    from memory address 'addr' size is 'bytes'\n"
);


U_BOOT_CMD(
	sdwaitins,	2,	1,	do_mmc_wait_insert,
	"sdwaitins - wait sd card inserted or removed\n"
	"sdwaitins 0 -- waiting removed\n"
	"sdwaitins 1 -- waiting inserted\n",
	"sdwaitins - wait sd card inserted or removed\n"
	"sdwaitins 0 -- waiting removed\n"
	"sdwaitins 1 -- waiting inserted\n"
);
//Charles
U_BOOT_CMD(
	mmcreadimg,	6,	1,	do_mmc_read_img,
	"mmcreadimg - read boot.img or recovery.img from SD/MMC card\n"
	"  <dev_id[:partition_no]> <kernel_addr> <ramdisk_addr> <ramdisk_sizes>\n"
	"   -read boot.img from SD/MMC card partition on 'dev_id'\n"
	"    to memory address 'addr' size is 'bytes'\n",
	"mmcreadimg - read boot.img or recovery.img from SD/MMC card\n"
	"  <dev_id[:partition_no]> <kernel_addr> <ramdisk_addr> <ramdisk_sizes>\n"
	"   -read boot.img from SD/MMC card partition on 'dev_id'\n"
	"    to memory address 'addr' size is 'bytes'\n"
);
U_BOOT_CMD(
	guidload,	5,	1,	do_mmc_load_guid_img,
	"guidload - read u-boot-logo or kernel-logo partition from SD/MMC card\n"
	"  <dev_id[:partition_no]> <load_addr> <filesize> \n"
	"   -read u-boot-logo or kernel-logo partition from SD/MMC card partition on 'dev_id'\n"
	"    to memory address 'load_addr' in bytes'\n",
	"guidload - read u-boot-logo or kernel-logo partition from SD/MMC card\n"
	"  <dev_id[:partition_no]> <load_addr> <filesize> \n"
	"   -read u-boot-logo or kernel-logo partition from SD/MMC card partition on 'dev_id'\n"
	"    to memory address 'load_addr' in 'bytes'\n"
);
U_BOOT_CMD(
	memreadimg,	5,	1,	do_mem_read_img,
	"memreadimg - read boot.img or recovery.img from memory\n"
	"  <bootimg_mem_addr> <kernel_addr> <ramdisk_addr> <ramdisk_sizes>\n"
	"   -read boot.img from memory'\n"
	"    to memory address 'addr' size is 'bytes'\n",
	"memreadimg - read boot.img or recovery.img from memory\n"
	"  <bootimg_mem_addr> <kernel_addr> <ramdisk_addr> <ramdisk_sizes>\n"
	"   -read boot.img from bootimg_mem_addr\n"
	"    to memory address 'addr' size is 'bytes'\n"
);
#endif	/* CFG_CMD_MMC */
