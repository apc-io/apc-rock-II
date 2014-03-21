/*++ 
Copyright (c) 2010 WonderMedia Technologies, Inc.

This program is free software: you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free Software 
Foundation, either version 2 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details. You
should have received a copy of the GNU General Public License along with this
program. If not, see http://www.gnu.org/licenses/>.

WonderMedia Technologies, Inc.
cheney chen mce Shenzhen china
--*/

#include <common.h>
#include <linux/mtd/nand.h>
#include <mmc.h>
#include <part.h>

#define BIT24 (1 << 24)
#define REG32               *(volatile unsigned int *)
#define REG8                *(volatile unsigned char *)
#define REG_GET8(addr)      ( REG8(addr) )      /* Read  8 bits Register */
#define REG_GET32(addr)     ( REG32(addr) )     /* Read 32 bits Register */
#define REG_SET8(addr, val) ( REG8(addr)  = (val) ) /* Write  8 bits Register */
#define ENV_HARDWARE_RECOVERY "wmt.hardware.recovery"
#define ENV_RECOVERY_NAME "wmt.recovery.enable"
#define MAX_NUM 9
#define TIMEOUT_SECONDS4 40    //100ms * 40
#define TIMEOUT_SECONDS10 100    //100ms * 100
//#define HW_RECOVERY_DEBUG

#define ENV_MISC_PART "misc-TF_part"
#define ENV_MISC_OFFSET "misc-NAND_ofs"
#define WMT_BOOT_DEV "wmt.boot.dev"


struct gpio_control_t {
     unsigned int  gpio_num; // gpio number
	 unsigned int  delay;
     unsigned int  active;	 // 0 or 1 
     unsigned int  bitmap;   // bitmap
     unsigned int  ctraddr;   // enable gpio function 
     unsigned int  icaddr;   // input control address
     unsigned int  idaddr;   // input data address 
     unsigned int  ipcaddr;  // input pull up/down control address 
     unsigned int  ipdaddr;  // input pull data address 
};

static struct gpio_control_t commb_gpio_key[] = {
	//power and volume+ in default
	[0] = {
		.gpio_num = 8,
		.delay  =  10,
		.active = 0,
		.bitmap = 0,
		.idaddr = 0xd8110001,
		.ctraddr = 0xd8110041,
		.icaddr = 0xd8110081,
		.ipcaddr = 0xd8110481,
		.ipdaddr = 0xd81104c1,
	}
};

struct bootloader_message {
    char command[32];
    char status[32];
    char recovery[1024];
};



extern int WMTSaveImageToNAND(struct nand_chip *nand, unsigned long long naddr, unsigned int dwImageStart,
		unsigned int dwImageLength, int oob_offs, unsigned int need_erase);
struct nand_chip* get_current_nand_chip(void);


static const int NAND_MISC_COMMAND_PAGE = 1;  // bootloader command is this page

static int nand_set_bootloader_message(const struct bootloader_message *in){
    char * ofsStr;
	unsigned long miscOff;
	struct nand_chip * nand = NULL;
    unsigned long nandOff;

    if(in == NULL){
		printf("invalid argument\n");
		return -1;
	}
	
	if((ofsStr = getenv(ENV_MISC_OFFSET)) == NULL){
		printf("%s is undefined.\n", ENV_MISC_OFFSET);
		return -1;
	}

	miscOff = simple_strtoul(ofsStr, NULL, 16);

	if(miscOff == 0){
		printf("Invalid misc offset %x(%s)\n", miscOff, ofsStr);
		return -1;
	}

	nand = get_current_nand_chip();
	if(nand == NULL){
        printf("Current nand chip is NULL\n");
		return -1;
	}

	nandOff = miscOff + NAND_MISC_COMMAND_PAGE * nand->dwPageSize;

	printf("set bootloader message , nand off = 0x%x(%x, %x), memory off = %p, size = 0x%x\n", 
		    nandOff, miscOff, nand->dwPageSize, in, sizeof(struct bootloader_message));

	return WMTSaveImageToNAND(nand, nandOff, (unsigned int)in, sizeof(struct bootloader_message), 0, 1);
}

extern block_dev_desc_t *get_dev (char*, int);

static  unsigned long mmc_part_offset =0;

int mmc_register_device_recovery(block_dev_desc_t *dev_desc, int part_no)
{
	unsigned char buffer[0x200];
	disk_partition_t info;

	if (!dev_desc->block_read)
		return -1;

	/* check if we have a MBR (on floppies we have only a PBR) */
	if (dev_desc->block_read (dev_desc->dev, 0, 1, (ulong *) buffer) != 1) {
		printf ("** Can't read from device %d **\n", dev_desc->dev);
		return -1;
	}

	if(!get_partition_info(dev_desc, part_no, &info)) {
			mmc_part_offset = info.start;
			printf("part_offset : %x, cur_part : %x\n", mmc_part_offset, part_no);
	} else {
#if 1
		printf ("** Partition %d not valid on device %d **\n",part_no,dev_desc->dev);
		return -1;
#else

		/* FIXME we need to determine the start block of the
		 * partition where the boot.img partition resides. This can be done
		 * by using the get_partition_info routine. For this
		 * purpose the libpart must be included.
		 */
		part_offset=32;
		cur_part = 1;
#endif
	}
	return 0;
}




static int mmc_set_bootloader_message(const struct bootloader_message *in){
    char *partStr;
	unsigned long partNo;
    unsigned long deviceId;
	unsigned long writeSize, writeCnt;
	block_dev_desc_t *devDesc=NULL;
	char *ep;
	int ret;
 
	if((partStr = getenv(ENV_MISC_PART)) == NULL){
		printf("%s is undefined.\n", ENV_MISC_PART);
		return -1;
	}

	deviceId = simple_strtoul(partStr, &ep, 16);

	if (deviceId < 0 || deviceId > 3){
		printf("Invalid device Id %d\n", deviceId);
		return -1;
	}

	/*get mmc dev descriptor*/
	devDesc=get_dev("mmc", deviceId);
	if (devDesc == NULL) {
		printf("\n** Invalid boot device **\n");
		return 1;
	}

	if (*ep) {
		if (*ep != ':') {
			puts ("\n** Invalid boot device, use `dev[:part]' **\n");
			return -1;
		}
		partNo = (int)simple_strtoul(++ep, NULL, 16);
	}

	/* init mmc controller */	
	if (mmc_init(1, deviceId)) {
		printf("mmc init failed?\n");
		return -1;
	}
	if (mmc_register_device_recovery(devDesc, partNo) != 0) {
		printf ("\n** Unable to use mmc %d:%d for fatload **\n", deviceId, partNo);
		return -1;
	}

    writeSize = sizeof(struct bootloader_message);
	writeCnt = writeSize/512 + (writeSize % 512)?1:0;

    ret = mmc_bwrite(deviceId, mmc_part_offset, writeCnt, (ulong*)in);
    printf("set bootloader message, deviceId = %d, partId = %d, part_off=0x%x, write block cnt=%d, memory off = %p, ret = %d\n",
		   deviceId, partNo, mmc_part_offset, writeCnt, in, ret);
	
    return ret;
}


static int set_bootloader_message(const struct bootloader_message *in){
    char *bootDev;
	if((bootDev = getenv(WMT_BOOT_DEV)) == NULL){
	     printf("%s is undefined.\n", WMT_BOOT_DEV);
		 return -1;
	}

	if(strcmp(bootDev, "NAND") == 0){
		return nand_set_bootloader_message(in);
	}else if(strcmp(bootDev, "TF") == 0){
	   return mmc_set_bootloader_message(in);	          
	}
	
	printf("boot %s is not supported.\n", bootDev);
	return -1;
}


static void boot_into_recovery(void){
    struct bootloader_message msg;

	strcpy(msg.command, "boot-recovery");
	strcpy(msg.recovery, "recovery\n--wipe_data\n--execute_script=/system/.restore/restore.sh");

    if(set_bootloader_message(&msg) >=0 ){
		run_command("textout 10 65 \"Boot into system recovery mode... \" 0xff0000;", 0);
		run_command("run boot-nand-ota-recovery", 0);
		run_command("run boot-kernel", 0);
	}
}


static int parse_commb_param(char* s_in,  int len)
{
    char *s1 = s_in;
    char *endp;
    int i = 0;
	unsigned int *param[MAX_NUM] = {
		&commb_gpio_key[0].gpio_num,
		&commb_gpio_key[0].delay,
		&commb_gpio_key[0].active,
		&commb_gpio_key[0].bitmap,
		&commb_gpio_key[0].ctraddr,
		&commb_gpio_key[0].icaddr,
		&commb_gpio_key[0].idaddr,
		&commb_gpio_key[0].ipcaddr,
		&commb_gpio_key[0].ipdaddr,
	};

	if(s1 == NULL)
		return -1;
    
#ifdef HW_RECOVERY_DEBUG
	printf("parse_commb_param: %s\n", s_in);
#endif
    
    while(i < len) {
        if (*s1 == ':')
        {
                s1++;
        }
        *param[i++] = simple_strtoul(s1, &endp, 16);
        if (*endp == '\0')
            break;
        s1 = endp + 1;
        if (*s1 == '\0') 
			break;
    }
	
#ifdef HW_RECOVERY_DEBUG
	printf("parse_commb_param: %x:%x:%x:%x:%x:%x:%x:%x:%x", commb_gpio_key[0].gpio_num, commb_gpio_key[0].delay, commb_gpio_key[0].active, commb_gpio_key[0].bitmap, commb_gpio_key[0].ctraddr, commb_gpio_key[0].icaddr, commb_gpio_key[0].idaddr, commb_gpio_key[0].ipcaddr, commb_gpio_key[0].ipdaddr );
#endif
	if(i != MAX_NUM)
		return -1;
	return 0;
}


int hw_recovery(void)
{
	char *s;
	int timeout = 0;
	int power_btn_status = 0;
	int volume_btn_value = 0;
	int times = 0;
	//int times_ptn_only = 0;
	int ret = 0;

	//decrease the delay time for system boot
	if((s = getenv(ENV_RECOVERY_NAME)) != NULL && !strcmp(s, "0"))
	{
		printf("No recovery.\n");
		return ret;
	}

	//default power button and volume+
	if((s = getenv(ENV_HARDWARE_RECOVERY)) != NULL)
	{
		ret = parse_commb_param(s, MAX_NUM);
		if(ret < 0) 
		{
			printf("Invalid Parameter wmt.hardware.recovery");
			return -1;
		}
	}

	REG_SET8(commb_gpio_key[0].ctraddr, REG_GET8(commb_gpio_key[0].ctraddr) | (1 << commb_gpio_key[0].bitmap));
	REG_SET8(commb_gpio_key[0].icaddr, REG_GET8(commb_gpio_key[0].icaddr) & ~(1 << commb_gpio_key[0].bitmap));
	REG_SET8(commb_gpio_key[0].ipcaddr, REG_GET8(commb_gpio_key[0].ipcaddr) | (1 << commb_gpio_key[0].bitmap));
	REG_SET8(commb_gpio_key[0].ipdaddr, REG_GET8(commb_gpio_key[0].ipdaddr) | (1 << commb_gpio_key[0].bitmap));

	udelay(1000);

	//commbined key, power button and vol+ in default
	power_btn_status = REG_GET32(0xd8130054);
	volume_btn_value = REG_GET8(commb_gpio_key[0].idaddr);
	if(!(power_btn_status & BIT24) || ((volume_btn_value & (1 << commb_gpio_key[0].bitmap)) != commb_gpio_key[0].active))
		return -1;

/*
	while(timeout < TIMEOUT_SECONDS4)
	{
		//hardware revoery check, check power button and volume+ 3 seconds
		power_btn_status = REG_GET32(0xd8130054);		
		volume_btn_value = REG_GET8(commb_gpio_key[0].idaddr);

#ifdef HW_RECOVERY_DEBUG
		printf("pwrbtnst: 0x%x, volume+: 0x%x\n", power_btn_status, volume_btn_value);
#endif
		
		if((power_btn_status & BIT24) && ((volume_btn_value & (1 << commb_gpio_key[0].bitmap)) == commb_gpio_key[0].active))
			times++;
		else { // shell  reboot or reset or power on
			times_ptn_only++;
		}

		if(times > commb_gpio_key[0].delay) //100ms * commb_gpio_key[0].delay = xx seconds
			break;     // go to comfirm
		else if(times_ptn_only > 9)  //100ms * 10 = 1s
		{
			return -1; // don't hardware recovery
		}
		timeout++;
		udelay(100000);
	}

	if(timeout >= TIMEOUT_SECONDS4)
		return -1;
*/
	//wait vol- comfirm  for 2 seconds
	timeout = 0;
	times = 0;

	run_command("textout 10 5 \"Are you sure system recovery?\" 0xff0000", 0);
	run_command("textout 10 25 \"Long press power button again to enter system recovery.\" 0xff0000", 0);
	while(timeout < TIMEOUT_SECONDS10)
	{
		//confirm_btn_value = REG_GET8(commb_gpio_key[1].idaddr);
		power_btn_status = REG_GET32(0xd8130054);		
		volume_btn_value = REG_GET8(commb_gpio_key[0].idaddr);

		timeout++;

#ifdef HW_RECOVERY_DEBUG
		printf("times = %d, timeout = %d\n", times, timeout);
#endif
		//for user's a mistake
		if(timeout >= TIMEOUT_SECONDS10)
			run_command("display clean", 0);

		if((volume_btn_value & (1 << commb_gpio_key[0].bitmap)) == commb_gpio_key[0].active) 
		{
			udelay(100000);
			continue;
		}
				
		if((power_btn_status & BIT24))
			times++;

		if(times >= commb_gpio_key[0].delay) { // 100ms * commb_gpio_key[0].delay = xx seconds
			run_command("textout 10 45 \"Recovery confirmed.\" 0xffff00;", 0);
			boot_into_recovery();
			return 0;
		}
		udelay(100000);
	}
	

	return 0;
}

