/*
 * Copyright (c) 2008 WonderMedia Technologies, Inc. All Rights Reserved.
 *
 * This PROPRIETARY SOFTWARE is the property of WonderMedia Technologies, Inc.
 * and may contain trade secrets and/or other confidential information of
 * WonderMedia Technologies, Inc. This file shall not be disclosed to any third
 * party, in whole or in part, without prior written consent of WonderMedia.
 *
 * THIS PROPRIETARY SOFTWARE AND ANY RELATED DOCUMENTATION ARE PROVIDED AS IS,
 * WITH ALL FAULTS, AND WITHOUT WARRANTY OF ANY KIND EITHER EXPRESS OR IMPLIED,
 * AND WonderMedia TECHNOLOGIES, INC. DISCLAIMS ALL EXPRESS OR IMPLIED WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR
 * NON-INFRINGEMENT.
 */

#include <common.h>
#include <command.h>
#include <malloc.h>
#include <asm/io.h>
#include <watchdog.h>
/* #include <env.h> */
#include <image.h>
#include <linux/stddef.h>
#include <asm/byteorder.h>

#include <search.h>
#include <environment.h>

#include <fat.h>


//#define DEBUG_CMD_ADDFWCENV

extern char fat_fwc[260];
#define FWC_BUFFER_SIZE	(64*1024)
#define SETENV_BEGIN ("<cmd>+")
#define SETENV_END ("</cmd>")
#define WMT_MODEL_NO ("wmt.model.no")

char modelname[260]; //for default get fwc filename from wmt.model.no

int do_addfwcenv (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[]);

U_BOOT_CMD(
	addfwcenv,	CFG_MAXARGS,	2,	do_addfwcenv,
	"addfwcenv - folder\n",
	"folder - find *.fwc in this folder with '+' as the first char in the file name\n"
	"         and do \"+setenv\" in this fwc file.\n"
);


int fwc_getenv(char *name)
{
    if (!name)
       return -1;

    ENTRY e, *ep;
    e.key = name;
    e.data = NULL;
    hsearch_r(e, FIND, &ep, &env_htab);
    if (ep == NULL)
    return -1;

    strcpy(modelname, ep->data);
    //size_t len = printf("%s=%s\n", ep->key, ep->data);
    return 0;
}


int
getfwc(char* folder, char* buffer, int buf_size)
{
	char *s_install_dev;
	int usb_install = 0;

	#ifdef DEBUG_CMD_ADDFWCENV
	printf("addfwcenv: %s\n", folder);
	#endif

	if((s_install_dev = getenv("wmt.install.dev")) && !strcmp(s_install_dev,"usb"))
		usb_install = 1;

	fat_fwc[0] = 0;
	modelname[0] =0;
	if(!fwc_getenv(WMT_MODEL_NO))
	{
		//printf("---read modle_no name: %s\n", modelname);
		sprintf(fat_fwc, "%s.fwc", modelname);

		#ifdef DEBUG_CMD_ADDFWCENV
		printf("modle_no file: %s\n", fat_fwc);
		#endif
	}

	if(usb_install == 0) {
		if (run_command("mmcinit 0", 0) == -1 ) {
			printf("[getfwc] run 'mmcinit 0' fail\n");
			return 0;
		} else
			sprintf(buffer, "fatls mmc 0 %s", folder);
	} else {
		if (run_command("usb reset", 0) == -1 ) {
			printf("[getfwc] run 'usb reset' fail\n");
			return 0;
		} else
			sprintf(buffer, "fatls usb 0 %s", folder);
	}

	run_command(buffer, 0);

	if (strlen(fat_fwc) == 0) {
		return 0;
	}

	#ifdef DEBUG_CMD_ADDFWCENV
	printf("file: %s\n", fat_fwc);
	#endif

	if(usb_install == 0)
		sprintf(buffer, "fatload mmc 0 %x %s/%s", buffer, folder, fat_fwc);
	else
		sprintf(buffer, "fatload usb 0 %x %s/%s", buffer, folder, fat_fwc);

	#ifdef DEBUG_CMD_ADDFWCENV
	printf("run: %s", buffer);
	#endif

	run_command(buffer, 0);

	#ifdef DEBUG_CMD_ADDFWCENV
	printf("content: %s\n", buffer);
	#endif

	return 1;
}

int
do_addfwcenv (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	char fwcbuf[FWC_BUFFER_SIZE];

	if (argc != 2)
	{
	    printf("Usage:\n%s\n", cmdtp->usage);
		return 0;
	}

	memset(fwcbuf, 0, FWC_BUFFER_SIZE);
	if (!getfwc(argv[1], fwcbuf, FWC_BUFFER_SIZE)) {
		printf("read fwc fail.\n");
		return -1;
	}

	char* begin = strstr(fwcbuf, SETENV_BEGIN);
	char* end = 0;
	while(begin > 0) {
		begin += strlen(SETENV_BEGIN);
		end = strstr(begin, SETENV_END);
		if (end < 0) break;
		*end = 0;

		//#ifdef DEBUG_CMD_ADDFWCENV
		printf("cmd: %s\n", begin);
		//#endif

		run_command(begin, 0);

		begin = strstr(end + 1, SETENV_BEGIN);
	}

  return 0;
}
