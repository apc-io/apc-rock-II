/*++ 
Copyright (c) 2010 WonderMedia Technologies, Inc.

This program is free software: you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free Software 
Foundation, either version 2 of the License, or (at your option) any later 
version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details. You
should have received a copy of the GNU General Public License along with this
program. If not, see http://www.gnu.org/licenses/>.

WonderMedia Technologies, Inc.
10F, 529, Chung-Cheng Road, Hsin-Tien, Taipei 231, R.O.C.
--*/


/*
 * RSA encode/decode Utilities
 */

#include <common.h>
#include <command.h>



#if 1/*(CONFIG_COMMANDS & CFG_CMD_RSA)*/
extern int rsa_check(unsigned int pub_key_addr, unsigned int pub_key_size, unsigned int sig_addr, unsigned int sig_size, u8 *out_buf);
extern void cypher_initialization(void);
extern int cypher_encode(unsigned int buf_in, unsigned int in_len, unsigned int buf_out);
extern void cipher_release(void);
/*-----------------------------------------------------------------------
 * Definitions
 */

/*
 * image_mem_addr  : boot.img memory address.
 * image _size     : boot.img size
 * sig_addr        : signature memory address.
 * sig_size        : signature size
 * return :        0 : rsa decode and compare pass, otherwise return nonzero.
 */

int image_rsa_check(
unsigned int image_mem_addr,
unsigned int image_size,
unsigned int sig_addr,
unsigned int sig_size)
{
	int ret = 0, i, j, k;
	char *key = NULL;
	unsigned int pub_key_addr, pub_key_size;
	unsigned char out_buf[128], hash_kernel[32], hash_signature[64], tmp;
	
	key = getenv("wmt.rsa.pem");
	if (!key) {
		printf("get public key fail\n");
		return 1;
	}
	pub_key_size = strlen(key);
#ifdef DEBUG_RSA_CMD
	printf("pub_key_size=0x%x, key=%s\n", pub_key_size, key);
#endif
	cypher_initialization();
	ret = cypher_encode(image_mem_addr, image_size, (unsigned int)hash_kernel);
	cipher_release();
	if (ret) {
		printf("cypher decode fail\n");
		return 1;
	}

	pub_key_addr = (u32) key;
	ret = rsa_check(pub_key_addr, pub_key_size, sig_addr, sig_size, out_buf);
	if (ret) {
		printf("decode signature fail\n");
		return 2;
	}
	for (i = 0, j = 0; i < 64; i=i+2,j++) {
		tmp = 0;
		for (k = 0; k < 2; k++) {
			if (out_buf[i+k] == '0')
				tmp += ((k == 0) ?(0<<4):0);
			else if (out_buf[i+k] == '1')
				tmp += ((k == 0) ?(1<<4):1);
			else if (out_buf[i+k] == '2')
				tmp += ((k == 0) ?(2<<4):2);
			else if (out_buf[i+k] == '3')
				tmp += ((k == 0) ?(3<<4):3);
			else if (out_buf[i+k] == '4')
				tmp += ((k == 0) ?(4<<4):4);
			else if (out_buf[i+k] == '5')
				tmp += ((k == 0) ?(5<<4):5);
			else if (out_buf[i+k] == '6')
				tmp += ((k == 0) ?(6<<4):6);
			else if (out_buf[i+k] == '7')
				tmp += ((k == 0) ?(7<<4):7);
			else if (out_buf[i+k] == '8')
				tmp += ((k == 0) ?(8<<4):8);
			else if (out_buf[i+k] == '9')
				tmp += ((k == 0) ?(9<<4):9);
			else if (out_buf[i+k] == 'a')
				tmp += ((k == 0) ?(0xa<<4):0xa);
			else if (out_buf[i+k] == 'b')
				tmp += ((k == 0) ?(0xb<<4):0xb);
			else if (out_buf[i+k] == 'c')
				tmp += ((k == 0) ?(0xc<<4):0xc);
			else if (out_buf[i+k] == 'd')
				tmp += ((k == 0) ?(0xd<<4):0xd);
			else if (out_buf[i+k] == 'e')
				tmp += ((k == 0) ?(0xe<<4):0xe);
			else if (out_buf[i+k] == 'f')
				tmp += ((k == 0) ?(0xf<<4):0xf);
			else {
				printf("change from character to digit fail out_buf[%d]=%c\n", i, out_buf[i]);
				ret = 3;
				break;
			}
		}
		//printf("i=%d ,j=%d tmp=%x out_buf=%d%c\n", i , j, tmp, out_buf[i+1], out_buf[i]);
		if (ret == 3)
			break;
		hash_signature[j] = tmp;
	}

	if (strncmp((char *)hash_signature, (char *)hash_kernel, 32) != 0) {
		printf("signature decode =");
		for (i = 0; i < 32; i++)
			printf("%2.2x", hash_signature[i]);
		printf("\nbootImage decode =");
		for (i = 0; i < 32; i++)
			printf("%x", hash_kernel[i]);
		printf("\n sha256 cmp fail\n");
		return 3;
	}

	return ret;
}

/*
 * image RSA verify
 * rsa verify <image_mem_addr> <image_size> <signature_mem_addr> <signature_size>
 * This command is used to verify the image signature by rsa/sha256.
 * The rsa public key will be get from env variable <wmt.rsa.pem>
 */

int do_rsa (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	int rcode = 0;

	if (strncmp(argv[1], "verify", 6) == 0 && argc == 6) {
		ulong image_mem_addr = simple_strtoul(argv[2], NULL, 16);
		ulong image_size = simple_strtoul(argv[3], NULL, 16);
		ulong sig_addr  = simple_strtoul(argv[4], NULL, 16);
		ulong sig_size = simple_strtoul(argv[5], NULL, 16);
#ifdef DEBUG_RSA_CMD
		printf("image_mem_addr=0x%x\r\n", image_mem_addr);
		printf("image_size=0x%x\r\n", image_size);
		printf("sig_addr=0x%x\r\n", sig_addr);
		printf("sig_size=0x%x\r\n", sig_size);
#endif
		rcode = image_rsa_check(image_mem_addr, image_size, sig_addr, sig_size);
	} else {
		rcode = 1;
		printf("please type \"rsa verify\" with parameter \n");
	}

	return rcode;
}

/***************************************************/

U_BOOT_CMD(
	rsa, 6,	1,	do_rsa,
	"rsa    - RSA utility commands\n",
	"- This command is used to verify the image signature by rsa/sha256.\n"
	"<image_mem_addr>       - boot.img address in memory "
	"<image_size>           - boot.img address in memory\n"
	"<signature_mem_addr>   - hashed rsa signature of address in memory\n"
	"<signature_size>       - signature size\n"
);

#endif	/* CFG_CMD_?? */

