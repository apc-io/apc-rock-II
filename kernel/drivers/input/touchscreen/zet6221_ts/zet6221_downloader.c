#include <linux/kernel.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/slab.h>
//#include "zet6221_fw.h"

#include "wmt_ts.h"

#define ZET6221_DOWNLOADER_NAME "zet6221_downloader"
#define FEATURE_FW_CHECK_SUM	1
//#define High_Impendence_Mode

#define TS_INT_GPIO		S3C64XX_GPN(9)   /*s3c6410*/
#define TS_RST_GPIO		S3C64XX_GPN(10)  /*s3c6410*/
#define RSTPIN_ENABLE

#define GPIO_LOW 	0
#define GPIO_HIGH 	1

//static u8 fw_version0;
//static u8 fw_version1;

//#define debug_mode 1
//#define DPRINTK(fmt,args...)	do { if (debug_mode) printk(KERN_EMERG "[%s][%d] "fmt"\n", __FUNCTION__, __LINE__, ##args);} while(0)

static unsigned char zeitec_zet6221_page[131] __initdata;
static unsigned char zeitec_zet6221_page_in[131] __initdata;
unsigned char* flash_buffer = NULL; 
int l_fwlen = 0;

//static u16 fb[8] = {0x3EEA,0x3EED,0x3EF0,0x3EF3,0x3EF6,0x3EF9,0x3EFC,0x3EFF};
static u16 fb[8] =   {0x3DF1,0x3DF4,0x3DF7,0x3DFA,0x3EF6,0x3EF9,0x3EFC,0x3EFF};
static u16 fb21[8] = {0x3DF1,0x3DF4,0x3DF7,0x3DFA,0x3EF6,0x3EF9,0x3EFC,0x3EFF}; 
static u16 fb23[8] = {0x7BFC,0x7BFD,0x7BFE,0x7BFF,0x7C04,0x7C05,0x7C06,0x7C07};
u8 ic_model = 0;

extern int zet6221_i2c_write_tsdata(struct i2c_client *client, u8 *data, u8 length);
extern int zet6221_i2c_read_tsdata(struct i2c_client *client, u8 *data, u8 length);
extern u8 pc[];




/************************load firmwre data from file************************/
int zet6221_load_fw(void)
{
	char fwname[256] = {0};
	int ret = -1;
	wmt_ts_get_firmwname(fwname);
	ret = wmt_ts_load_firmware(fwname, &flash_buffer, &l_fwlen);
	if(!ret) {
	printk("Success load fw_file: %s, length %d\n", fwname, l_fwlen);
	printk("%x,%x,%x,%x\n", flash_buffer[0], flash_buffer[1], flash_buffer[2], flash_buffer[3]);
	printk("%x,%x,%x,%x\n", flash_buffer[l_fwlen-4], flash_buffer[l_fwlen-3], flash_buffer[l_fwlen-2], flash_buffer[l_fwlen-1]);
	}
	return ret;
}

/***********************free firmware memory*******************************/
int zet6221_free_fwmem(void)
{
	if (l_fwlen > 0 && flash_buffer != NULL )
	{
		kfree(flash_buffer);
		flash_buffer = NULL;
		l_fwlen = 0;
	}
	return 0;
}
//#define I2C_CTPM_ADDRESS        (0x76)

/***********************************************************************
[function]: 
    callback: write data to ctpm by i2c interface,implemented by special user;
[parameters]:
 	client[in]			:i2c client structure;
    bt_ctpm_addr[in]    :the address of the ctpm;
    pbt_buf[in]         :data buffer;
    dw_lenth[in]        :the length of the data buffer;
[return]:
    1    :success;
    0    :fail;
************************************************************************/
int i2c_write_interface(struct i2c_client *client, u8 bt_ctpm_addr, u8* pbt_buf, u16 dw_lenth)
{
	struct i2c_msg msg;
	msg.addr = bt_ctpm_addr;
	msg.flags = 0;
	msg.len = dw_lenth;
	msg.buf = pbt_buf;
	return i2c_transfer(client->adapter,&msg, 1);
}

/***********************************************************************
[function]: 
    callback: read data from ctpm by i2c interface,implemented by special user;
[parameters]:
 	client[in]			:i2c client structure;
    bt_ctpm_addr[in]    :the address of the ctpm;
    pbt_buf[out]        :data buffer;
    dw_lenth[in]        :the length of the data buffer;
[return]:
    1    :success;
    0    :fail;
************************************************************************/
int i2c_read_interface(struct i2c_client *client, u8 bt_ctpm_addr, u8* pbt_buf, u16 dw_lenth)
{
	struct i2c_msg msg;
	msg.addr = bt_ctpm_addr;
	msg.flags = I2C_M_RD;
	msg.len = dw_lenth;
	msg.buf = pbt_buf;
	return i2c_transfer(client->adapter,&msg, 1);
}

/***********************************************************************
    [function]: 
		        callback: check version;
    [parameters]:
    			void

    [return]:
			    0: different 1: same;
************************************************************************/
u8 zet6221_ts_version(void)
{	
	int i;

	if(pc == NULL){
		errlog(" pc is NULL\n");
		return 0;
	}
	if( flash_buffer == NULL ){
		errlog("flash_buffer \n");
		return 0;
	}

#if 1
	dbg("pc: ");
	for(i=0;i<8;i++){
		dbg("%02x ",pc[i]);
	}
	dbg("\n");
	
	dbg("src: ");
	for(i=0;i<8;i++){
		dbg("%02x ", flash_buffer[fb[i]]);
	}
	dbg("\n");
#endif

	mdelay(20);

	for(i=0;i<8;i++)
		if(pc[i]!= flash_buffer[fb[i]])
			return 0;
	return 1;
}


/***********************************************************************
    [function]: 
		        callback: send password 1K (ZET6223)
    [parameters]:
    			client[in]:  struct i2c_client — represent an I2C slave device;

    [return]:
			    1;
************************************************************************/
u8 zet6221_ts_sndpwd_1k(struct i2c_client *client)
{
	u8 ts_sndpwd_cmd[3] = {0x20,0xB9,0xA3};
	
	int ret;

#if defined(I2C_CTPM_ADDRESS)
	ret=i2c_write_interface(client, I2C_CTPM_ADDRESS, ts_sndpwd_cmd, 3);
#else
	ret=zet6221_i2c_write_tsdata(client, ts_sndpwd_cmd, 3);
#endif

	
	return 1;
}


/***********************************************************************
    [function]: 
		        callback: send password;
    [parameters]:
    			client[in]:  struct i2c_client ???represent an I2C slave device;

    [return]:
			    1;
************************************************************************/
u8 zet6221_ts_sndpwd(struct i2c_client *client)
{
	u8 ts_sndpwd_cmd[3] = {0x20,0xC5,0x9D};
	
	int ret;

#if defined(I2C_CTPM_ADDRESS)
	ret=i2c_write_interface(client, I2C_CTPM_ADDRESS, ts_sndpwd_cmd, 3);
#else
	ret=zet6221_i2c_write_tsdata(client, ts_sndpwd_cmd, 3);
#endif
	
	return 1;
}

u8 zet622x_ts_option(struct i2c_client *client)
{
	u8 ts_cmd[1] = {0x27};
	u8 ts_cmd_erase[1] = {0x28};
	u8 ts_in_data[16] = {0};
	u8 ts_out_data[18] = {0};
	int ret;
	u16 model;
	int i;
	u8	high_impendence_data = 0;
	const u8 HIGH_IMPENDENCE_MODE_DATA = 0xf1;
	const u8 NOT_HIGH_IMPENDENCE_MODE_DATA = 0xf2;


	dbg("zet622x_ts_option++\n");	

	wmt_rst_output(0);
	msleep(10);
	//send password
	zet6221_ts_sndpwd(client);
	msleep(100);



#if defined(I2C_CTPM_ADDRESS)
	ret=i2c_write_interface(client, I2C_CTPM_ADDRESS, ts_cmd, dim(ts_cmd));
#else
	ret=zet6221_i2c_write_tsdata(client, ts_cmd, dim(ts_cmd));
#endif
	msleep(2);


#if defined(I2C_CTPM_ADDRESS)
	ret=i2c_read_interface(client, I2C_CTPM_ADDRESS, ts_in_data, dim(ts_in_data));
#else
	ret=zet6221_i2c_read_tsdata(client, ts_in_data, dim(ts_in_data));
#endif
	//msleep(2);

	dbg("command %02x recv:\n",ts_cmd[0]);
	for(i=0;i<16;i++)
	{
		dbg("%02x ",ts_in_data[i]); 
	}
	dbg("\n"); 
	// zet6231 recv: ff ff fc 30 ff 80 31 62 ff ff ff ff ff ff ff ff

	model = 0x0;
	model = ts_in_data[7];
	model = (model << 8) | ts_in_data[6];
	
	switch(model) { 
        case 0xFFFF: 
        	ret = 1;
            ic_model = ZET6221;
			for(i=0;i<8;i++)
			{
				fb[i]=fb21[i];
			}

			if( is_high_impendence_mode() == HIGH_IMPENDENCE_MODE ){
				high_impendence_data = HIGH_IMPENDENCE_MODE_DATA;
			}else if( is_high_impendence_mode() == NOT_HIGH_IMPENDENCE_MODE  ) {
				high_impendence_data = NOT_HIGH_IMPENDENCE_MODE_DATA;
			}
			
			//#if defined(High_Impendence_Mode)
			if(ts_in_data[2] != high_impendence_data)
			{
			
				if(zet6221_ts_sfr(client)==0)
				{
					return 0;
				}
			
				#if defined(I2C_CTPM_ADDRESS)
				ret=i2c_write_interface(client, I2C_CTPM_ADDRESS, ts_cmd_erase, dim(ts_cmd_erase));
				#else
				ret=zet6221_i2c_write_tsdata(client, ts_cmd_erase, dim(ts_cmd_erase));
				#endif
				
				msleep(100);
				dbg("erase ret=%d \n",ret);
				
			
				for(i=2;i<18;i++)
				{
					ts_out_data[i]=ts_in_data[i-2];
				}
				ts_out_data[0] = 0x21;
				ts_out_data[1] = 0xc5;
				ts_out_data[4] = high_impendence_data;
			
				#if defined(I2C_CTPM_ADDRESS)
				ret=i2c_write_interface(client, I2C_CTPM_ADDRESS, ts_out_data, 18);
				#else
				ret=zet6221_i2c_write_tsdata(client, ts_out_data, 18);
				#endif
				
				msleep(100);
				dbg("write out data, ret=%d\n",ret);

				

				#if defined(I2C_CTPM_ADDRESS)
				ret=i2c_write_interface(client, I2C_CTPM_ADDRESS, ts_cmd, 1);
				#else
				ret=zet6221_i2c_write_tsdata(client, ts_cmd, 1);
				#endif

				msleep(2);
	
				dbg("send %02x\n",ts_cmd[0]); 
	

				#if defined(I2C_CTPM_ADDRESS)
				ret=i2c_read_interface(client, I2C_CTPM_ADDRESS, ts_in_data, 16);
				#else
				ret=zet6221_i2c_read_tsdata(client, ts_in_data, 16);
				#endif
				//msleep(2);
				dbg("command %02x recv:\n",ts_cmd[0]); 
				for(i=0;i<16;i++)
				{
					dbg("%02x ",ts_in_data[i]); 
				}
				dbg("\n"); 
				
			}
												
			//#endif	


				
            break; 
        case 0x6223:
        	ret = 1;
			ic_model = ZET6223;
			for(i=0;i<8;i++)
			{
				fb[i]=fb23[i];
			}
            break; 
		case 0x6231:
			ret = 1;
			ic_model = ZET6231;
			for(i=0;i<8;i++)
			{
				fb[i]=fb23[i];
			}
			break; 
    		case 0x6251:
			ic_model = ZET6251;
			for(i=0;i<8;i++)
			{
				fb[i] = fb23[i];
			}
			break;
		default: 
			errlog("Notice: can't detect the TP IC,use ZET6231 default\n");
			ret = 1;
			ic_model = ZET6231;
			for(i=0;i<8;i++)
			{
				fb[i]=fb23[i];
			}
			break; 

    } 

	wmt_rst_output(1);
	msleep(10);	

	dbg("zet622x_ts_option--  ret:%d\n",ret);
	return ret;
}
/***********************************************************************
    [function]: 
		        callback: set/check sfr information;
    [parameters]:
    			client[in]:  struct i2c_client ???represent an I2C slave device;

    [return]:
			    1;
************************************************************************/
u8 zet6221_ts_sfr(struct i2c_client *client)
{
	u8 ts_cmd[1] = {0x2C};
	u8 ts_in_data[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
	u8 ts_cmd17[17] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
	//u8 ts_sfr_data[16] = {0x18,0x76,0x27,0x27,0xFF,0x03,0x8E,0x14,0x00,0x38,0x82,0xEC,0x00,0x00,0x7d,0x03};
	int ret;
	int i;
	
	dbg("zet6221_ts_sfr++");
#if defined(I2C_CTPM_ADDRESS)
	ret=i2c_write_interface(client, I2C_CTPM_ADDRESS, ts_cmd, 1);
#else
	ret=zet6221_i2c_write_tsdata(client, ts_cmd, 1);
#endif
	msleep(10);
	dbg("sfr cmd : 0x%02x \n",ts_cmd[0]); 



	dbg("sfr rcv : \n"); 

#if defined(I2C_CTPM_ADDRESS)
	ret=i2c_read_interface(client, I2C_CTPM_ADDRESS, ts_in_data, 16);
#else
	ret=zet6221_i2c_read_tsdata(client, ts_in_data, 16);
#endif
	msleep(10);

	if(ts_in_data[14]!=0x3D && ts_in_data[14]!=0x7D)
	{
			return 0;
	}

	for(i=0;i<16;i++)
	{
		ts_cmd17[i+1]=ts_in_data[i];
		dbg("[%d]%02x\n",i,ts_in_data[i]); 
	}
	
	dbg("\n"); 

	// need to check 0x3D to open write function
	if(ts_in_data[14]!=0x3D)
	{
		ts_cmd17[15]=0x3D;
		
		ts_cmd17[0]=0x2B;	
		
#if defined(I2C_CTPM_ADDRESS)
		ret=i2c_write_interface(client, I2C_CTPM_ADDRESS, ts_cmd17, 17);
#else
		ret=zet6221_i2c_write_tsdata(client, ts_cmd17, 17);
#endif

		if(ret<0)
		{
			errlog("enable sfr(0x3D) failed!\n"); 
			return 0;
		}

	}
	dbg("zet6221_ts_sfr--");
	return 1;
}

/***********************************************************************
    [function]: 
		        callback: mass erase flash;
    [parameters]:
    			client[in]:  struct i2c_client ???represent an I2C slave device;

    [return]:
			    1;
************************************************************************/
u8 zet6221_ts_masserase(struct i2c_client *client)
{
	u8 ts_cmd[1] = {0x24};
	
	int ret;

#if defined(I2C_CTPM_ADDRESS)
	ret=i2c_write_interface(client, I2C_CTPM_ADDRESS, ts_cmd, 1);
#else
	ret=zet6221_i2c_write_tsdata(client, ts_cmd, 1);
#endif
	
	return 1;
}

/***********************************************************************
    [function]: 
		        callback: erase flash by page;
    [parameters]:
    			client[in]:  struct i2c_client ???represent an I2C slave device;

    [return]:
			    1;
************************************************************************/
u8 zet6221_ts_pageerase(struct i2c_client *client,int npage)
{
	u8 ts_cmd[3] = {0x23,0x00,0x00};
	u8 len = 0;
	int ret;

	switch(ic_model)
	{
		case ZET6221:
			ts_cmd[1]=npage;
			len=2;
			break;
		case ZET6231: 
		case ZET6223:
		case ZET6251:
			ts_cmd[1]=npage & 0xff;
			ts_cmd[2]=npage >> 8;
			len=3;
			break;
		default:
			ts_cmd[1]=npage & 0xff;
			ts_cmd[2]=npage >> 8;
			len=3;
			break;
	}
#if defined(I2C_CTPM_ADDRESS)
	ret=i2c_write_interface(client, I2C_CTPM_ADDRESS, ts_cmd, len);
#else
	ret=zet6221_i2c_write_tsdata(client, ts_cmd, len);
#endif
	msleep(2);
	
	return 1;
}

/***********************************************************************
    [function]: 
		        callback: reset mcu;
    [parameters]:
    			client[in]:  struct i2c_client ???represent an I2C slave device;

    [return]:
			    1;
************************************************************************/
u8 zet6221_ts_resetmcu(struct i2c_client *client)
{
	u8 ts_cmd[1] = {0x29};
	
	int ret;

#if defined(I2C_CTPM_ADDRESS)
	ret=i2c_write_interface(client, I2C_CTPM_ADDRESS, ts_cmd, 1);
#else
	ret=zet6221_i2c_write_tsdata(client, ts_cmd, 1);
#endif
	
	return 1;
}


#define CMD_PROG_CHECK_SUM		(0x36)
#define CMD_PROG_GET_CHECK_SUM	(0x37)
///***********************************************************************
///   [function]:  zet622x_cmd_read_check_sum
///   [parameters]: client, page_id, buf
///   [return]: int
///************************************************************************
int zet622x_cmd_read_check_sum(struct i2c_client *client, int page_id, u8 * buf)
{
	int ret;
	int cmd_len = 3;

	buf[0]= CMD_PROG_CHECK_SUM;
	buf[1]= (u8)(page_id) & 0xff; 
	buf[2]= (u8)(page_id >> 8);   		
	ret=zet6221_i2c_write_tsdata(client, buf, cmd_len);
	if(ret<=0)
	{
		printk("[ZET]: Read check sum fail");
		return ret;
	}

	buf[0]= CMD_PROG_GET_CHECK_SUM;
	cmd_len = 1;
	ret=zet6221_i2c_write_tsdata(client, buf, cmd_len);
	if(ret<=0)
	{
		printk("[ZET]: Read check sum fail");
		return ret;
	}
	
	cmd_len = 1;
	ret = zet6221_i2c_read_tsdata(client, buf, cmd_len);
	if(ret<=0)		
	{
		printk("[ZET]: Read check sum fail");
		return ret;
	}
	return 1;
}


/***********************************************************************
    [function]: 
		        callback: start HW function;
    [parameters]:
    			client[in]:  struct i2c_client ???represent an I2C slave device;

    [return]:
			    1;
************************************************************************/
u8 zet6221_ts_hwcmd(struct i2c_client *client)
{
	u8 ts_cmd[1] = {0xB9};
	
	int ret;

#if defined(I2C_CTPM_ADDRESS)
	ret=i2c_write_interface(client, I2C_CTPM_ADDRESS, ts_cmd, 1);
#else
	ret=zet6221_i2c_write_tsdata(client, ts_cmd, 1);
#endif
	
	return 1;
}

/***********************************************************************
update FW
************************************************************************/
int __init zet6221_downloader( struct i2c_client *client )
{
	int BufLen=0;
	int BufPage=0;
	int BufIndex=0;
	int ret;
	int i;
	
	int nowBufLen=0;
	int nowBufPage=0;
	int nowBufIndex=0;
	int retryCount=0;
	int retryTimes = 0;

	int i2cLength=0;
	int bufOffset=0;

	dbg("zet6221_downloader++\n");
	
begin_download:
	
#if defined(RSTPIN_ENABLE)
	//reset mcu
	//gpio_direction_output(TS_RST_GPIO, GPIO_LOW);
	wmt_rst_output(0);
	msleep(5);
#else
	zet6221_ts_hwcmd(client);
	msleep(200);
#endif
	//send password
	//send password
	ret = zet6221_ts_sndpwd(client);
	dbg("zet6221_ts_sndpwd ret=%d\n",ret);
	msleep(100);
	
/*****compare version*******/

	//0~3
	memset(zeitec_zet6221_page_in,0x00,131);
	switch(ic_model)
	{
		case ZET6221:
			zeitec_zet6221_page_in[0]=0x25;
			zeitec_zet6221_page_in[1]=(fb[0] >> 7);//(fb[0]/128);
			
			i2cLength=2;
			break;
		case ZET6231:
		case ZET6223: 
		case ZET6251: 
		default:
			zeitec_zet6221_page_in[0]=0x25;
			zeitec_zet6221_page_in[1]=(fb[0] >> 7) & 0xff; //(fb[0]/128);
			zeitec_zet6221_page_in[2]=(fb[0] >> 7) >> 8; //(fb[0]/128);
			
			i2cLength=3;
			break;
	}
#if defined(I2C_CTPM_ADDRESS)
		ret=i2c_write_interface(client, I2C_CTPM_ADDRESS, zeitec_zet6221_page_in, i2cLength);
#else
		ret=zet6221_i2c_write_tsdata(client, zeitec_zet6221_page_in, i2cLength);
		dbg("write_ret =%d, i2caddr=0x%x\n", ret, client->addr);
#endif
		msleep(2);
	
	zeitec_zet6221_page_in[0]=0x0;
	zeitec_zet6221_page_in[1]=0x0;
	zeitec_zet6221_page_in[2]=0x0;
#if defined(I2C_CTPM_ADDRESS)
	ret=i2c_read_interface(client, I2C_CTPM_ADDRESS, zeitec_zet6221_page_in, 128);
#else
	ret=zet6221_i2c_read_tsdata(client, zeitec_zet6221_page_in, 128);
	dbg("read_ret =%d, i2caddr=0x%x\n", ret, client->addr);
#endif

	//printk("page=%d ",(fb[0] >> 7));             //(fb[0]/128));
	for(i=0;i<4;i++)
	{
		pc[i]=zeitec_zet6221_page_in[(fb[i] & 0x7f)];     //[(fb[i]%128)];
		dbg("offset[%d]=%d ",i,(fb[i] & 0x7f));        //(fb[i]%128));
	}
	dbg("\n");
	
	
	// 4~7
	memset(zeitec_zet6221_page_in,0x00,131);
	switch(ic_model)
	{
		case ZET6221: 
			zeitec_zet6221_page_in[0]=0x25;
			zeitec_zet6221_page_in[1]=(fb[4] >> 7);//(fb[4]/128);
			
			i2cLength=2;
			break;
		case ZET6231:
		case ZET6223: 
		case ZET6251: 
			zeitec_zet6221_page_in[0]=0x25;
			zeitec_zet6221_page_in[1]=(fb[4] >> 7) & 0xff; //(fb[4]/128);
			zeitec_zet6221_page_in[2]=(fb[4] >> 7) >> 8; //(fb[4]/128);
			
			i2cLength=3;
			break;
	}
#if defined(I2C_CTPM_ADDRESS)
	ret=i2c_write_interface(client, I2C_CTPM_ADDRESS, zeitec_zet6221_page_in, i2cLength);
#else
	ret=zet6221_i2c_write_tsdata(client, zeitec_zet6221_page_in, i2cLength);
	dbg("write_ret =%d, i2caddr=0x%x\n", ret, client->addr);
#endif
	
	zeitec_zet6221_page_in[0]=0x0;
	zeitec_zet6221_page_in[1]=0x0;
	zeitec_zet6221_page_in[2]=0x0;
#if defined(I2C_CTPM_ADDRESS)
		ret=i2c_read_interface(client, I2C_CTPM_ADDRESS, zeitec_zet6221_page_in, 128);
#else
		ret=zet6221_i2c_read_tsdata(client, zeitec_zet6221_page_in, 128);
		dbg("read_ret =%d, i2caddr=0x%x\n", ret, client->addr);
#endif

	//printk("page=%d ",(fb[4] >> 7)); //(fb[4]/128));
	for(i=4;i<8;i++)
	{
		pc[i]=zeitec_zet6221_page_in[(fb[i] & 0x7f)]; 		//[(fb[i]%128)];
		dbg("offset[%d]=%d ",i,(fb[i] & 0x7f));  		//(fb[i]%128));
	}
	dbg("\n");
	
#if 1 // need to check
	//page 127
	memset(zeitec_zet6221_page_in,0x00,130);
	zeitec_zet6221_page_in[0]=0x25;
	zeitec_zet6221_page_in[1]=127;
#if defined(I2C_CTPM_ADDRESS)
	ret=i2c_write_interface(client, I2C_CTPM_ADDRESS, zeitec_zet6221_page_in, 2);
#else
	ret=zet6221_i2c_write_tsdata(client, zeitec_zet6221_page_in, 2);
#endif
	
	zeitec_zet6221_page_in[0]=0x0;
	zeitec_zet6221_page_in[1]=0x0;
#if defined(I2C_CTPM_ADDRESS)
		ret=i2c_read_interface(client, I2C_CTPM_ADDRESS, zeitec_zet6221_page_in, 128);
#else
		ret=zet6221_i2c_read_tsdata(client, zeitec_zet6221_page_in, 128);
#endif

	for(i=0;i<128;i++)
	{	
		// 0x3F80 = 16256 = 128x127, means skipped the first 127 page (0-126) ,use the page 127.
		if(0x3F80+i < l_fwlen/*sizeof(flash_buffer)/sizeof(char)*/) //l_fwlen: the bytes of data read from firmware file
		{
			if(zeitec_zet6221_page_in[i]!=flash_buffer[0x3F80+i])
			{
				errlog("page 127 [%d] doesn't match! continue to download! retry times:%d\n",i,retryTimes);
				if( retryTimes++ >= 20){ // retry 20 times ,quit
					errlog("May be I2C comunication is error\n");
					goto exit_download;
				}			
				goto proc_sfr;
			}
		}
	}
	
#endif

	if( get_download_option() == FORCE_DOWNLOAD ){
		errlog("FORCE_DOWNLOAD\n");
		goto proc_sfr;
	}
	if( get_download_option() == FORCE_CANCEL_DOWNLOAD ){
		errlog("FORCE_CANCEL_DOWNLOAD\n");
		goto exit_download;
	}	
	if(zet6221_ts_version()!=0){
		klog("tp version is the same,no need to download\n");
		goto exit_download;
	}

	
	
/*****compare version*******/
proc_sfr:
	//sfr
	if(zet6221_ts_sfr(client)==0)
	{

#if 1

#if defined(RSTPIN_ENABLE)
	
		//gpio_direction_output(TS_RST_GPIO, GPIO_HIGH);
		wmt_rst_output(1);
		msleep(20);
		
		//gpio_direction_output(TS_RST_GPIO, GPIO_LOW);
		wmt_rst_output(0);
		msleep(20);
		
		//gpio_direction_output(TS_RST_GPIO, GPIO_HIGH);
		wmt_rst_output(1);
#else
		zet6221_ts_resetmcu(client);
#endif	
		msleep(20);
		errlog("zet6221_ts_sfr error, download again\n");
		goto begin_download;
		
#endif

	}
	msleep(20);

	/// Fix the bug that  page#504~#512 failed to write
	if(ic_model == ZET6223)
	{
		zet6221_ts_sndpwd_1k(client);
	}
		
	//erase
	if(BufLen==0)
	{
		//mass erase
		dbg( "mass erase\n");
		zet6221_ts_masserase(client);
		msleep(200);

		BufLen=l_fwlen;/*sizeof(flash_buffer)/sizeof(char)*/;
	}else
	{
		zet6221_ts_pageerase(client,BufPage);
		msleep(200);
	}

	
	while(BufLen>0)
	{
download_page:

		memset(zeitec_zet6221_page,0x00,131);
		
		klog( "Start: write page%d\n",BufPage);
		nowBufIndex=BufIndex;
		nowBufLen=BufLen;
		nowBufPage=BufPage;
		
		switch(ic_model)
		{
			case ZET6221: 
				bufOffset = 2;
				i2cLength=130;
				
				zeitec_zet6221_page[0]=0x22;
				zeitec_zet6221_page[1]=BufPage;				
				break;
			case ZET6231: 
			case ZET6223: 
			case ZET6251: 
			default:
				bufOffset = 3;
				i2cLength=131;
				
				zeitec_zet6221_page[0]=0x22;
				zeitec_zet6221_page[1]=BufPage & 0xff;
				zeitec_zet6221_page[2]=BufPage >> 8;
				break;
		}
		if(BufLen>128)
		{
			for(i=0;i<128;i++)
			{
				zeitec_zet6221_page[i+bufOffset]=flash_buffer[BufIndex];
				BufIndex+=1;
			}
			zeitec_zet6221_page[0]=0x22;
			zeitec_zet6221_page[1]=BufPage;
			BufLen-=128;
		}
		else
		{
			for(i=0;i<BufLen;i++)
			{
				zeitec_zet6221_page[i+bufOffset]=flash_buffer[BufIndex];
				BufIndex+=1;
			}
			zeitec_zet6221_page[0]=0x22;
			zeitec_zet6221_page[1]=BufPage;
			BufLen=0;
		}
		
#if defined(I2C_CTPM_ADDRESS)
		ret=i2c_write_interface(client, I2C_CTPM_ADDRESS, zeitec_zet6221_page, i2cLength);
#else
		ret=zet6221_i2c_write_tsdata(client, zeitec_zet6221_page, i2cLength);
#endif
		//msleep(200);
		msleep(2);
		
#if 1

		memset(zeitec_zet6221_page_in,0x00,131);
		switch(ic_model)
		{
			case ZET6221: 
				zeitec_zet6221_page_in[0]=0x25;
				zeitec_zet6221_page_in[1]=BufPage;
				
				i2cLength=2;
				break;
			case ZET6231: 
			case ZET6223: 
			case ZET6251: 
			default:
				zeitec_zet6221_page_in[0]=0x25;
				zeitec_zet6221_page_in[1]=BufPage & 0xff;
				zeitec_zet6221_page_in[2]=BufPage >> 8;

				i2cLength=3;
				break;
		}		
#if defined(I2C_CTPM_ADDRESS)
		ret=i2c_write_interface(client, I2C_CTPM_ADDRESS, zeitec_zet6221_page_in, i2cLength);
#else
		ret=zet6221_i2c_write_tsdata(client, zeitec_zet6221_page_in, i2cLength);
#endif
		msleep(2);
	
		zeitec_zet6221_page_in[0]=0x0;
		zeitec_zet6221_page_in[1]=0x0;
		zeitec_zet6221_page_in[2]=0x0;
#if defined(I2C_CTPM_ADDRESS)
		ret=i2c_read_interface(client, I2C_CTPM_ADDRESS, zeitec_zet6221_page_in, 128);
#else
		ret=zet6221_i2c_read_tsdata(client, zeitec_zet6221_page_in, 128);
#endif
		
		for(i=0;i<128;i++)
		{
			if(i < nowBufLen)
			{
				if(zeitec_zet6221_page[i+bufOffset]!=zeitec_zet6221_page_in[i])
				{
					BufIndex=nowBufIndex;
					BufLen=nowBufLen;
					BufPage=nowBufPage;
				
					if(retryCount < 5)
					{
						retryCount++;
						goto download_page;
					}else
					{
						//BufIndex=0;
						//BufLen=0;
						//BufPage=0;
						retryCount=0;
						
#if defined(RSTPIN_ENABLE)
						//gpio_direction_output(TS_RST_GPIO, GPIO_HIGH);
						wmt_rst_output(1);
						msleep(20);
		
						//gpio_direction_output(TS_RST_GPIO, GPIO_LOW);
						wmt_rst_output(0);
						msleep(20);
		
						//gpio_direction_output(TS_RST_GPIO, GPIO_HIGH);
						wmt_rst_output(1);
#else
						zet6221_ts_resetmcu(client);
#endif	
						msleep(20);
						goto begin_download;
					}

				}
			}
		}
		
#endif
		retryCount=0;
		BufPage+=1;
	}
	
exit_download:

#if defined(RSTPIN_ENABLE)
	//gpio_direction_output(TS_RST_GPIO, GPIO_HIGH);
	wmt_rst_output(1);
	msleep(100);
#endif

	zet6221_ts_resetmcu(client);
	msleep(100);

	dbg("zet6221_downloader--\n");
	return 1;


}

int zet622x_resume_downloader(struct i2c_client *client)
{
	int ret = 0;

	int BufLen=0;
	int BufPage=0;
	int BufIndex=0;
	int bufOffset = 0;

	int nowBufLen=0;
	int nowBufPage=0;
	int nowBufIndex=0;

	int i2cLength = 0;

	int i = 0;

	u8 bPageBuf[256];
	
#ifdef FEATURE_FW_CHECK_SUM
	u8 get_check_sum		= 0;
	u8 check_sum 			= 0;
	int retry_count			= 0;
	u8 tmp_data[16];
#endif ///< for FEATURE_FW_CHECK_SUM


	///-------------------------------------------------------------///
	///   1. Set RST=LOW
	///-------------------------------------------------------------///
	wmt_rst_output(0);
	msleep(20);
	//printk("RST = LOW\n");

	///-------------------------------------------------------------///
	/// Send password
	///-------------------------------------------------------------///
	ret = zet6221_ts_sndpwd(client);
	if(ret<=0)
	{
		return ret;
	}
	
	//printk("AAA\n");
	BufLen=l_fwlen;/*sizeof(flash_buffer)/sizeof(char)*/;
	//printk("BBB%d\n",BufLen);
  
	while(BufLen>0)
	{
		///	memset(zeitec_zet622x_page, 0x00, 131);		
		nowBufIndex=BufIndex;
		nowBufLen=BufLen;
		nowBufPage=BufPage;
		
		switch(ic_model)
		{
			case ZET6251: 
				bufOffset = 3;
				i2cLength=131;
				
				bPageBuf[0]=0x22;
				bPageBuf[1]=BufPage & 0xff;
				bPageBuf[2]=BufPage >> 8;
				break;
		}
		
		if(BufLen>128)
		{
			for(i=0;i<128;i++)
			{
				bPageBuf[i + bufOffset] = flash_buffer[BufIndex];
				BufIndex += 1;
			}

			BufLen = BufLen - 128;
		}
		else
		{
			for(i=0;i<BufLen;i++)
			{
				bPageBuf[i+bufOffset]=flash_buffer[BufIndex];
				BufIndex+=1;
			}

			BufLen=0;
		}
		
#ifdef FEATURE_FW_CHECK_SUM
LABEL_RETRY_DOWNLOAD_PAGE:
#endif  ///< for FEATURE_FW_CHECK_SUM

		ret=zet6221_i2c_write_tsdata(client, bPageBuf, i2cLength);

		if(ic_model!= ZET6251)
		{
			msleep(50);
		}
		
#ifdef FEATURE_FW_CHECK_SUM
		///---------------------------------///
		///  Get check sum
		///---------------------------------///
		for(i=0;i<128;i++)
		{
			if(i == 0)
			{
				check_sum = bPageBuf[i + bufOffset];
			}
			else
			{
				check_sum = check_sum ^ bPageBuf[i + bufOffset];
			}
		}
		
		///---------------------------------///
		/// Read check sum
		///---------------------------------///
		memset(tmp_data, 0, 16);
		ret = zet622x_cmd_read_check_sum(client, BufPage, &tmp_data[0]);	
		if(ret<=0)
		{
			return ret;
		}
		get_check_sum = tmp_data[0];
		
		//printk("[ZET]: page=%3d  ,Check sum : %x ,get check sum : %x\n", BufPage, check_sum, get_check_sum);
		if(check_sum != get_check_sum)
		{
		
			if(retry_count < 5)
			{
				retry_count++;
				goto LABEL_RETRY_DOWNLOAD_PAGE;
			}
			else
			{
				retry_count = 0;						
				wmt_rst_output(1);
				msleep(20);		
				wmt_rst_output(0);
				msleep(20);
				wmt_rst_output(1);
				msleep(20);
				printk("[ZET] zet622x_resume_downloader fail\n");
				return ret;
			}
			
		}
		retry_count  = 0;	
#endif  ///< for FEATURE_FW_CHECK_SUM

		BufPage+=1;
	}

	printk("[ZET] zet622x_resume_downloader OK\n");
	//printk("RST = HIGH\n");

	///-------------------------------------------------------------///
	/// reset_mcu command
	///-------------------------------------------------------------///
	zet6221_ts_resetmcu(client);
	msleep(10);	

	///-------------------------------------------------------------///
	///   SET RST=HIGH
	///-------------------------------------------------------------///
	wmt_rst_output(1);
	msleep(20);

	///-------------------------------------------------------------///
	/// RST toggle 	
	///-------------------------------------------------------------///
	wmt_rst_output(0);
	msleep(2);

	wmt_rst_output(1);
 	msleep(2);

	return ret;
}

