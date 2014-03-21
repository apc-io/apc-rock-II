/*
 * Copyright (C) 2011 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <common.h>
#include "./include/bq_battery_i2c.h"
#include <mmc.h>
#include <command.h>
#include <linux/mtd/nand.h>

#ifndef max
#define max(a,b) ((a) > (b) ? (a) : (b))
#endif

#ifndef min
#define min(a,b) ((a) < (b) ? (a) : (b))
#endif

/*
#define ARRAY_SIZE(x)           (sizeof(x)/sizeof(x[0]))
*/

#define MSEC_PER_SEC            (1000LL)
#define NSEC_PER_MSEC           (1000000LL)

#define BATTERY_UNKNOWN_TIME    (2 * MSEC_PER_SEC)
#define POWER_ON_KEY_TIME       (2 * MSEC_PER_SEC)
#define UNPLUGGED_SHUTDOWN_TIME (10 * MSEC_PER_SEC)

#define BATTERY_FULL_THRESH     95
#define POWER_SUPPLY_NUM 2
#define KEY_MAX 10
/*KEY_TYPE*/
#define EV_KEY 0x01
/*KEY_VALUE*/
#define KEY_POWER 0x00
#define PMC_BASE_ADDR 0xD8130000
#define PMCPB_ADDR 0xD8130054
#define IC_BASE_ADDR 0xD8140000
#define PMWS_ADDR (PMC_BASE_ADDR + 0x0014)
#define PWHC (PMC_BASE_ADDR + 0x0012)
#define PWRESET (PMC_BASE_ADDR + 0x0060)
#define PMC_WAKEUPEVENT_EN (PMC_BASE_ADDR + 0x1C)
#define PMC_WAKEUPEVENT_TYPE (PMC_BASE_ADDR + 0x20)

/*GPIO*/
#define DEDICATED_GPIO_ID_ADDR 0xD8110000
#define DEDICATED_GPIO_CTRL_ADDR 0xD8110040
#define DEDICATED_GPIO_OC_ADDR 0xD8110080
#define DEDICATED_GPIO_PULL_EN_ADDR 0xD8110480
#define DEDICATED_GPIO_PULL_CTRL_ADDR 0xD81104C0

typedef enum _bool{false,true} bool;

extern void lcd_blt_enable(int no,int enable);
extern int wmt_read_oscr(unsigned int *val);
extern int wmt_init_ostimer(void);
extern int wmt_idle_us(int us);
extern int do_fat_fsload(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[]);
extern int display_clear(void);
extern int display_init(int on, int force);
extern int do_nand(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[]);

extern int wmt_bat_get_capacity(void);

static unsigned short curr_capacity = 0;
static unsigned int normal_boot_flag = 0;

struct wmt_img_header {
	unsigned char wmt_header[3];
	unsigned short number_pictures;
	unsigned int img_size;
	unsigned int pictures_offset[128];
};

static struct wmt_img_header img_header;
static unsigned int img_ram_addr = 0xa00000;

struct input_event {
	unsigned int time;
	unsigned short type;
	unsigned short code;
	int value;
};

struct key_state {
	bool pending;
	bool down;
	unsigned int timestamp;
};

struct power_supply {
	char name[256];
	char type[32];
	bool online;
	bool valid;
};

struct frame {
	unsigned int surface_addr;
	int disp_time;
	int min_capacity;
	bool level_only;
};

struct animation {
	bool run;

	struct frame *frames;
	int cur_frame;
	int num_frames;

	int cur_cycle;
	int num_cycles;

	/* current capacity being animated */
	int capacity;
};

struct charger {
	unsigned int next_screen_transition;
	unsigned int next_key_check;
	unsigned int next_pwr_check;

	struct key_state keys[KEY_MAX + 1];
	int uevent_fd;

	struct power_supply supplies[POWER_SUPPLY_NUM];
	int num_supplies;
	int num_supplies_online;

	struct animation *batt_anim;
	unsigned int surf_unknown;

	struct power_supply *battery;
	unsigned int check_pb_after_anim;
};

struct uevent {
	const char *action;
	const char *path;
	const char *subsystem;
	const char *ps_name;
	const char *ps_type;
	const char *ps_online;
};

static struct frame batt_anim_frames[] = {
	{
		.surface_addr = 0x14000000,
		.disp_time = 750,
		.min_capacity = 0,
	},
	{
		.surface_addr = 0x14100000,
		.disp_time = 750,
		.min_capacity = 20,
	},
	{
		.surface_addr = 0x14200000,
		.disp_time = 750,
		.min_capacity = 40,
	},
	{
		.surface_addr = 0x14300000,
		.disp_time = 750,
		.min_capacity = 60,
	},
	{
		.surface_addr = 0x14400000,
		.disp_time = 750,
		.min_capacity = 80,
		.level_only = true,
	},
	{
		.surface_addr = 0x14500000,
		.disp_time = 750,
		.min_capacity = BATTERY_FULL_THRESH,
	},
};

static struct animation battery_animation = {
	.frames = batt_anim_frames,
	.num_frames = ARRAY_SIZE(batt_anim_frames),
	.num_cycles = 3,
};

static struct charger charger_state = {
	.batt_anim = &battery_animation,
};

static int check_battery_exist(struct charger *data);
static int input_callback(struct charger *data);
static int check_power_supply(int supply_num, struct charger *data);

/* current time in milliseconds */
static unsigned int curr_time_ms(void)
{
	static unsigned int last_time;
	static unsigned int overflow_count;
	unsigned int curr_time = 0;
	unsigned int step = 0;
	wmt_read_oscr(&curr_time);
	if (last_time == 0)
		last_time = curr_time;
	/*0xffffffff is used for special purpose*/
	while (curr_time == 0xffffffff)
		wmt_read_oscr(&curr_time);
	if (last_time > curr_time)
		++overflow_count;
	if (overflow_count != 0)
		step = overflow_count * (0xffffffff/3000);
	return (curr_time/3000 + step);
}
static void gr_fb_blank(bool blank)
{
	if (blank)
		lcd_blt_enable(0, 0);
	else
		lcd_blt_enable(0, 1);
}

#define MAX_KLOG_WRITE_BUF_SZ 256

static int read_battery_capacity(int *batt)
{
	curr_capacity = wmt_bat_get_capacity();
	*batt = curr_capacity;
	return 0;
}

static int get_battery_capacity(struct charger *charger)
{
	int ret;
	int batt_cap = -1;

	if (!charger->battery)
		return -1;

	ret = read_battery_capacity(&batt_cap);
	if (ret < 0 || batt_cap > 100) {
		batt_cap = -1;
	}

	return batt_cap;
}


#define UEVENT_MSG_LEN  1024

static int uevent_callback(void *data)
{
	struct charger *charger = data;
	int i = 0;
	check_battery_exist(charger);
	for (i = 1; i < POWER_SUPPLY_NUM; ++i)
		check_power_supply(i, charger);
	return 0;
}

static int check_battery_exist(struct charger *data)
{
	struct charger *charger = data;
	charger->battery  = &((charger->supplies)[0]);/*Always suppose battery is the first supply*/
	strcpy("Battery", charger->battery->name);
	charger->battery->online = true;
	return 0;
}

int read_power_supplies_status(int supply_num)
{
	wmt_idle_us(125);
	*(volatile unsigned int *)PMWS_ADDR = 0x20;
	if (*(volatile unsigned int *)DEDICATED_GPIO_ID_ADDR & 0x00400000)
		return 1;
	else
		return 0;

}
static int check_power_supply(int supply_num, struct charger *data)
{
	struct charger *charger = data;
	strcpy("AC", ((charger->supplies)[supply_num]).name);
	if (read_power_supplies_status(supply_num)) {
		if (!((charger->supplies)[supply_num]).online) {
			charger->num_supplies_online++;
			((charger->supplies)[supply_num]).online = true;
		}
	} else {
		if (((charger->supplies)[supply_num]).online) {
			charger->num_supplies_online--;
			((charger->supplies)[supply_num]).online = false;
		}
	}
	return 0;
}

static void init_power_supply(struct charger *charger)
{
	int supply_num = 0;

	while (supply_num < POWER_SUPPLY_NUM) {
		if (supply_num == 0)
			check_battery_exist(charger);
		else
			check_power_supply(supply_num, charger);
		++supply_num;
	}
}

extern int display_animation(unsigned int img_phy_addr);

static void redraw_screen(struct charger *charger)
{
	struct animation *batt_anim = charger->batt_anim;
	unsigned int img_phy_addr;
	img_phy_addr = (batt_anim->frames[batt_anim->cur_frame]).surface_addr;
	
	display_animation(img_phy_addr);
}

static void kick_animation(struct animation *anim)
{
	anim->run = true;
}

static void reset_animation(struct animation *anim)
{
	anim->cur_cycle = 0;
	anim->cur_frame = 0;
	anim->run = false;
}

static void update_screen_state(struct charger *charger, unsigned int now)
{
	struct animation *batt_anim = charger->batt_anim;
	int disp_time;

	/*
	printf("now = %x, next_screen_transition = %x\n", now, charger->next_screen_transition);
	*/
	if (!batt_anim->run || (int)(now - charger->next_screen_transition) < 0) {
		if (!batt_anim->run) 
			printf("%s return\n", __func__);
		return;
	}

	/* animation is over, blank screen and leave */
	if (batt_anim->cur_cycle == batt_anim->num_cycles) {
		reset_animation(batt_anim);
		charger->next_screen_transition = -1;
		charger->check_pb_after_anim = 1;
		gr_fb_blank(true);
		printf("[%x] animation done\n", now);
		return;
	}

	disp_time = batt_anim->frames[batt_anim->cur_frame].disp_time;

	/* animation starting, set up the animation */
	if (batt_anim->cur_frame == 0) {
		int batt_cap;

		printf("[%x] animation starting\n", now);
		batt_cap = get_battery_capacity(charger);
		if (batt_cap >= 0 && batt_anim->num_frames != 0) {
			int i;

			/* find first frame given current capacity */
			for (i = 1; i < batt_anim->num_frames; i++) {
				if (batt_cap < batt_anim->frames[i].min_capacity)
				break;
			}
			batt_anim->cur_frame = i - 1;

			/* show the first frame for twice as long */
			disp_time = batt_anim->frames[batt_anim->cur_frame].disp_time * 2;
		}

		batt_anim->capacity = batt_cap;
	}

	/* unblank the screen  on first cycle */
	if (batt_anim->cur_cycle == 0)
		gr_fb_blank(false);

	/* draw the new frame (@ cur_frame) */
	redraw_screen(charger);

	/* if we don't have anim frames, we only have one image, so just bump
	 * the cycle counter and exit
	 */
	if (batt_anim->num_frames == 0 || batt_anim->capacity < 0) {
		printf("[%x] animation missing or unknown battery status\n", now);
		charger->next_screen_transition = now + BATTERY_UNKNOWN_TIME;
		batt_anim->cur_cycle++;
		return;
	}

	/* schedule next screen transition */
	charger->next_screen_transition = now + disp_time;

	/* advance frame cntr to the next valid frame
	 * if necessary, advance cycle cntr, and reset frame cntr
	 */
	batt_anim->cur_frame++;

	/* if the frame is used for level-only, that is only show it when it's
	 * the current level, skip it during the animation.
	 */
	while (batt_anim->cur_frame < batt_anim->num_frames &&
		batt_anim->frames[batt_anim->cur_frame].level_only)
		batt_anim->cur_frame++;

	if (batt_anim->cur_frame >= batt_anim->num_frames) {
		batt_anim->cur_cycle++;
		batt_anim->cur_frame = 0;

		/* don't reset the cycle counter, since we use that as a signal
		 * in a test above to check if animation is over
		 */
	}
}

static int set_key_callback(int code, int value, void *data)
{
	struct charger *charger = data;
	unsigned int now = curr_time_ms();
	int down = !!value;

	if (code > KEY_MAX)
		return -1;

	/* ignore events that don't modify our state */
	if (charger->keys[code].down == down) {
		/*
		printf("the same event\n");
		*/
		return 0;
	}

	/* only record the down even timestamp, as the amount
	 * of time the key spent not being pressed is not useful */
	if (down)
		charger->keys[code].timestamp = now;

	charger->keys[code].down = down;
	charger->keys[code].pending = true;

	if (down) {
		printf("[%x] key[%d] down\n", now, code);
	} else {
		unsigned int duration = now - charger->keys[code].timestamp;
		printf("[%x] key[%d] up (was down for %x)\n", now,
			code, duration);
	}

	return 0;
}

static int identify_key(struct charger *data, struct input_event *ev)
{
	unsigned int state = 0;
	ev->type = EV_KEY;
	ev->code = KEY_POWER;
	if (*(volatile unsigned int *)PMWS_ADDR & 0x4000) {
		state = 1;
		*(volatile unsigned int *)PMWS_ADDR = 0x4000;
		ev->value = state;
		return 0;
	}
	if (*(volatile unsigned int *)PMCPB_ADDR & 0x01000000)
		state = 1;/*pressing*/
	else
		state = 0;/*release*/

	ev->value = state ;
	return 0;
}
static void update_input_state(struct charger *charger,
                               struct input_event *ev)
{
	identify_key(charger, ev);

	if (ev->type != EV_KEY)
		return;
	set_key_callback(ev->code, ev->value, charger);
}

static void set_next_key_check(struct charger *charger,
                               struct key_state *key,
                               int64_t timeout)
{
	unsigned int then = key->timestamp + timeout;

	if (charger->next_key_check == -1 || then < charger->next_key_check)
		charger->next_key_check = then;
}

/*
 * if fastboot or recovery mode trigger, do reboot
 * fastboot:power button + Vol+ + Vol-
 * recovery mode :power button + Vol+
 */
static int do_fastboot(void)
{
	/*Vol+ pressed*/
	if (!(*(volatile unsigned int *)(0xD8110018) & 0x40000))
		return 1;
	return 0;
}
/*
 * level: 0:reboot
 *        1:shutdown
 */
static void wmt_reboot(int level)
{
	if (level == 1) {
		printf("shutdown\n");
		*(volatile unsigned char *)(IC_BASE_ADDR+ 0x56) = 0x00;
		*(volatile unsigned short *)PWHC |= 0x205;
		asm("wfi");
	} else if (level == 0) {
		printf("reboot\n");
		if (do_fastboot() == 1)
			*(volatile unsigned int *)PWRESET = 0x1;
		normal_boot_flag = 1;
		*(volatile unsigned char *)(IC_BASE_ADDR+ 0x56) = 0x00;/*turn off interrupt*/
		gr_fb_blank(true);
		display_init(0, 1);
		/*
		*(volatile unsigned int *)PWRESET = 0x1;
		*/
	} else
		printf("Wrong reboot level\n");
}
static void process_key(struct charger *charger, int code, int64_t now)
{
	struct key_state *key = &charger->keys[code];

	if (code == KEY_POWER) {
		if (key->down) {
			unsigned int reboot_timeout = key->timestamp + POWER_ON_KEY_TIME;
			if (now >= reboot_timeout) {
				/*
				printf("[%x] rebooting\n", now);
				*/
				wmt_reboot(0);
			} else {
				/* if the key is pressed but timeout hasn't expired,
				 * make sure we wake up at the right-ish time to check
				 */
				set_next_key_check(charger, key, POWER_ON_KEY_TIME);
			}
		} else {
			/* if the power key got released, force screen state cycle */
			/*
			printf("key up\n");
			*/
			if (key->pending)
				kick_animation(charger->batt_anim);
		}
	}

	key->pending = false;
}

static void handle_input_state(struct charger *charger, int64_t now)
{
	process_key(charger, KEY_POWER, now);

	if (charger->next_key_check != -1 && now > charger->next_key_check)
		charger->next_key_check = -1;
}

static void handle_power_supply_state(struct charger *charger, int64_t now)
{
	if (charger->num_supplies_online == 0) {
		if (charger->next_pwr_check == -1) {
			charger->next_pwr_check = now + UNPLUGGED_SHUTDOWN_TIME;
			printf("[%x] device unplugged: shutting down in %x (@ %x)\n",
				now, UNPLUGGED_SHUTDOWN_TIME, charger->next_pwr_check);
		} else if (now >= charger->next_pwr_check) {
			printf("[%x] shutting down\n", now);
			wmt_reboot(1);
		} else {
			/* otherwise we already have a shutdown timer scheduled */
		}
	} else {
		/* online supply present, reset shutdown timer if set */
		if (charger->next_pwr_check != -1) {
			printf("[%x] device plugged in: shutdown cancelled\n", now);
			kick_animation(charger->batt_anim);
		}
		charger->next_pwr_check = -1;
	}
}

static int ev_wait(int timeout, struct charger *data)
{
	struct charger *charger = data;
	unsigned int current_time = 0;
	unsigned int elasped_time = 0;

	wmt_read_oscr(&current_time);

	if (timeout == -1) {
		asm("wfi");
		wmt_idle_us(125);
		/*
		printf("PMWS_VAL = %x\n", *(volatile unsigned int *)PMWS_ADDR);
		*/
	} else {
		while (elasped_time < 3000 * timeout) {
			/*release*/
			if (!(*(volatile unsigned int *)PMCPB_ADDR & 0x01000000)) {
				if (charger->keys[KEY_POWER].down) {
					printf("release\n");
					break;
				}
			}
			wmt_read_oscr(&elasped_time);
			if (elasped_time < current_time)
				current_time = 0xffffffff - current_time;
			elasped_time -= current_time;
		}
		/*
		printf("wait %dms\n", elasped_time/3000);
		*/
	}
	return 0;
}

static int ev_dispatch(struct charger *data)
{
	struct charger *charger = data;
	input_callback(charger);
	uevent_callback(charger);
	return 0;
}

static void wait_next_event(struct charger *charger, unsigned int now)
{
	unsigned int next_event = -1;
	unsigned int timeout;
	int ret;

	printf("[%x] next screen: %x next key: %x next pwr: %x\n", now,
		charger->next_screen_transition, charger->next_key_check,
		charger->next_pwr_check);

	if (charger->next_screen_transition != -1)
		next_event = charger->next_screen_transition;
	if (charger->next_key_check != -1 && charger->next_key_check < next_event)
		next_event = charger->next_key_check;
	if (charger->next_pwr_check != -1 && charger->next_pwr_check < next_event)
		next_event = charger->next_pwr_check;

	if (next_event != -1 && next_event != 0xffffffff)
		timeout = max(0, next_event - now);
	else
		timeout = -1;
	/*
	printf("Before wait timeout = %x\n", timeout);
	*/
	ret = ev_wait((int)timeout, charger);
	if (!ret)
		ev_dispatch(charger);
}

static int input_callback(struct charger *data)
{
	struct charger *charger = data;
	struct input_event ev;

	update_input_state(charger, &ev);
	return 0;
}

static void event_loop(struct charger *charger)
{
	while (true) {
		unsigned int now = curr_time_ms();

		handle_input_state(charger, now);
		if (normal_boot_flag == 1)
			break;
		handle_power_supply_state(charger, now);

		/* do screen update last in case any of the above want to start
		 * screen transitions (animations, etc)
		 */
		update_screen_state(charger, now);
		/*
		if (!charger->batt_anim->run)
			printf("anim run false\n");
		*/

		wait_next_event(charger, now);
	}
}

static void ev_init_key_state(struct charger *data)
{
	struct charger *charger = data;
	((charger->keys)[KEY_POWER]).down = false;/*release*/
	((charger->keys)[KEY_POWER]).pending = false;/*release*/
}

static void init_pwbtn(void)
{
	volatile unsigned int value = 0;
	/*set debouce time*/
	value = *(volatile unsigned int *)PMCPB_ADDR;
	value &= ~0x00ff0000;
	value |= 0x00040000;
	*(volatile unsigned int *)PMCPB_ADDR = value;
	/*enable interrupt controller*/
	*(volatile unsigned char *)(IC_BASE_ADDR+ 0x56) = 0x08;
	*(volatile unsigned int *)PMWS_ADDR = 0x4000;
	
}

static void init_gpio(void)
{
	*(volatile unsigned int *)DEDICATED_GPIO_CTRL_ADDR |= 0x00400000;
	*(volatile unsigned int *)DEDICATED_GPIO_OC_ADDR &= ~0x00400000;
	*(volatile unsigned int *)PMC_WAKEUPEVENT_TYPE &= ~0x00F00000;
	*(volatile unsigned int *)PMC_WAKEUPEVENT_TYPE |= 0x00400000;/*double edge*/
	*(volatile unsigned int *)PMWS_ADDR = 0x20;/*clear susgpio interrupt*/
	*(volatile unsigned int *)PMC_WAKEUPEVENT_EN |= 0x20;/*enable interrupt*/ 
	
}

extern struct nand_chip* get_current_nand_chip(void);

int parse_img_header(struct charger *data)
{
	struct charger *charger = data;
	struct animation *batt_anim = charger->batt_anim;
	int ret = 0;
	int i = 0;
	img_header.wmt_header[0] = *(unsigned char *)img_ram_addr;
	img_header.wmt_header[1] = *(unsigned char *)(img_ram_addr + 1);
	img_header.wmt_header[2] = *(unsigned char *)(img_ram_addr + 2);
	img_header.number_pictures = *(unsigned short *)(img_ram_addr + 4);
	/*
	printf("number_pictures = %d\n", img_header.number_pictures);
	*/
	img_header.img_size = *(unsigned int *)(img_ram_addr + 8);
	/*
	printf("img_size = %x\n", img_header.img_size);
	*/
	for (i = 0; i < img_header.number_pictures; ++i) {
		img_header.pictures_offset[i] = *(unsigned int *)(img_ram_addr + 0x10 + i*0x4) + img_ram_addr;
		(batt_anim->frames[i]).surface_addr = img_header.pictures_offset[i];
		/*
		printf("[%d]offset = 0x%x\n", i, img_header.pictures_offset[i]);
		*/
	}
	return ret;
}
static void load_img(struct charger *data,
			unsigned int storage_id,
			char *dev_id,
			char *ram_offset,
			char *storage_offset)
{
	/*emmc command*/
	char **loadcmd = 0;
	char *cmd_line = "fatload";
	char *dev_name = "mmc";
	/*nand flash cmd*/
	char *nand_cmd_line = "nandrw";
	char *nand_rw = "r";
	char img_size[80];
	int dev = 0;
	char *ep;
	/*
	printf("storage id = %d\n", storage_id);
	*/
	*loadcmd = cmd_line;
	*(loadcmd+1) = dev_name;
	*(loadcmd+2) = dev_id;
	*(loadcmd+3) = ram_offset;
	*(loadcmd+4) = storage_offset;
	/*
	printf("assign command finish\n");
	*/
	
	if (storage_id == 0) {
		*loadcmd = nand_cmd_line;
		*(loadcmd+1) = nand_rw;
		*(loadcmd+2) = storage_offset;
		*(loadcmd+3) = ram_offset;
		sprintf(img_size, "%x", 0x4000);
		*(loadcmd+4) = img_size;
		do_nand(NULL, 0, 5, loadcmd);
		parse_img_header(data);
		sprintf(img_size, "%x", img_header.img_size);
		*(loadcmd+4) = img_size;
		do_nand(NULL, 0, 5, loadcmd);
	} else if (storage_id == 1) {
		dev = (int)simple_strtoul (dev_id, &ep, 16);
		/*
		printf("dev = %d\n", dev);
		*/
		mmc_init(0, dev);
		do_fat_fsload(NULL, 0, 5, loadcmd);
		parse_img_header(data);
	}
}

int show_charger_main(unsigned int storage_id,
			char *dev_id,
			char *ram_offset,
			char *storage_offset)
{
	struct charger *charger = &charger_state;
	unsigned int now;

	normal_boot_flag = 0;
	init_gpio();
	init_pwbtn();
	init_power_supply(charger);

	wmt_init_ostimer();
	now = curr_time_ms() - 1;
	load_img(charger, storage_id, dev_id, ram_offset, storage_offset);
	/*read full capacity*/
	curr_capacity = wmt_bat_get_capacity();
	printf("curr capacity = 0x%x\n", curr_capacity);

	ev_init_key_state(charger);

	charger->next_screen_transition = now - 1;
	charger->next_key_check = -1;
	charger->next_pwr_check = -1;
	charger->check_pb_after_anim = 0;
	reset_animation(charger->batt_anim);
	kick_animation(charger->batt_anim);

	event_loop(charger);

	return 0;
}
