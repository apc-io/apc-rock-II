#include <common.h>
//#include <ext_common.h>
//#include <ext4fs.h>
#include <malloc.h>
#include <stddef.h>
#include <linux/stat.h>
#include <linux/time.h>

//#include <ext4fs.h>
#include "ext2_fs.h"
#include "ext2fs.h"

#if defined(__linux__)    &&	defined(EXT2_OS_LINUX)
#define CREATOR_OS EXT2_OS_LINUX
#else
#if defined(__GNU__)     &&	defined(EXT2_OS_HURD)
#define CREATOR_OS EXT2_OS_HURD
#else
#if defined(__FreeBSD__) &&	defined(EXT2_OS_FREEBSD)
#define CREATOR_OS EXT2_OS_FREEBSD
#else
#if defined(LITES) 	   &&	defined(EXT2_OS_LITES)
#define CREATOR_OS EXT2_OS_LITES
#else
#define CREATOR_OS EXT2_OS_LINUX /* by default */
#endif /* defined(LITES) && defined(EXT2_OS_LITES) */
#endif /* defined(__FreeBSD__) && defined(EXT2_OS_FREEBSD) */
#endif /* defined(__GNU__)     && defined(EXT2_OS_HURD) */
#endif /* defined(__linux__)   && defined(EXT2_OS_LINUX) */

static struct struct_ext2_filsys *fs;
struct ext_filesystem {
	/* Total Sector of partition */
	uint32_t total_sect;//uint64_t
	/* Block size  of partition */
	uint32_t blksz;
	/* Inode size of partition */
	uint32_t inodesz;
	/* Sectors per Block */
	uint32_t sect_perblk;
	/* Group Descriptor Block Number */
	uint32_t gdtable_blkno;
	/* Total block groups of partition */
	uint32_t no_blkgrp;
	/* No of blocks required for bgdtable */
	uint32_t no_blk_pergdt;
	/* Superblock */
	struct ext2_sblock *sb;
	/* Block group descritpor table */
	struct ext2_block_group *bgd;
	char *gdtable;

	/* Block Bitmap Related */
	unsigned char **blk_bmaps;
	long int curr_blkno;
	uint16_t first_pass_bbmap;

	/* Inode Bitmap Related */
	unsigned char **inode_bmaps;
	int curr_inode_no;
	uint16_t first_pass_ibmap;

	/* Journal Related */

	/* Block Device Descriptor */
	block_dev_desc_t *dev_desc;
};

struct ext_filesystem *get_fs(void);
static int times=1;
static unsigned long writebyte=0;
extern unsigned long part_offset;

static errcode_t open(const char *name, int flags, io_channel *channel)
{
	//printf("open\n");
	times=1;
	writebyte=0;
	return 0;
}
static errcode_t close(io_channel channel)
{
	//printf("close\n");
	return 0;
}
static errcode_t set_blksize(io_channel channel, int blksize)
{	
	times= blksize/512;
	return 0;
}
static errcode_t read_blk(io_channel channel, unsigned long block,
	int count, const void *buf)
{
	struct ext_filesystem *tfs = get_fs();
	if(count<0) {
		//printf("read_blk form 0x%x to 0x%x\n", part_offset+block*times, part_offset+block*times+count*(-1)/512-1);
		
		if (tfs->dev_desc->block_read(tfs->dev_desc->dev, part_offset+block*times, count*(-1)/512, (unsigned long *)buf)!=(count*(-1)/512)) {
			printf("read_blk error\n");
			return 1;
		}
		return 0;
	}
	//printf("read_blk form 0x%x to 0x%x\n", part_offset+block*times, part_offset+(block+count-1)*times);
	if (tfs->dev_desc->block_read(tfs->dev_desc->dev, part_offset+block*times, count*times, (unsigned long *)buf)!=count*times) {
		printf("read_blk error\n");
		return 1;
	}
	return 0;
}


static errcode_t write_blk(io_channel channel, unsigned long block,
					int count, const void *buf)
{

	struct ext_filesystem *tfs = get_fs();	
		
	if (count<0){
		//printf("write_blk form 0x%x to 0x%x\n", part_offset+block*times, part_offset+block*times+count*(-1)/512-1);
		//printf("count<0, 0x%x\n",count*(-1));
		writebyte+=count*(-1);
		if (tfs->dev_desc->block_write(tfs->dev_desc->dev, part_offset+block*times, count*(-1)/512, (unsigned long *)buf)!=(count*(-1)/512)) {
			printf("write_blk error\n");
			return 1;
		}
		return 0;
	}
	//printf("write_blk form 0x%x to 0x%x\n", 0x3f+block*times, 0x3f+(block+count-1)*times);
	writebyte+=count*times*512;
	if (tfs->dev_desc->block_write(tfs->dev_desc->dev, part_offset+block*times, count*times, (unsigned long *)buf)!=count*times) {
		printf("write_blk error\n");
		return 1;
	}
	return 0;

}

static errcode_t flush(io_channel channel)
{
	//printf("flush\n");
	return 0;
}

static errcode_t write_byte(io_channel channel, unsigned long offset,
				 int size, const void *buf)
{
	//printf("write_byte\n");
	return 0;
}

static errcode_t set_option(io_channel channel, const char *option,
				 const char *arg)
{
	//printf("set_option\n");
	return 0;
}

static errcode_t get_stats(io_channel channel, io_stats *stats)
{
	//printf("get_stats\n");
	(*stats)->bytes_written =writebyte;
	//memcpy(*stats, writebyte, sizeof(unsigned long long));
	//printf("Tina: writebyte 0x%x\n", writebyte);
	return 0;
}

static errcode_t read_blk64(io_channel channel, unsigned long long block,
			       int count, void *buf)
{
	//printf("read_blk64\n");
	return 0;
}

static errcode_t write_blk64(io_channel channel, unsigned long long block,
				int count, const void *buf)
{
	//printf("write_blk64\n");
	return 0;
}

static struct struct_io_manager struct_devio_manager = {
	EXT2_ET_MAGIC_IO_MANAGER,
	"Unix I/O Manager",
	open,
	close,
	set_blksize,
	read_blk,
	write_blk,
	flush,
	write_byte,
	set_option,
	get_stats,
	read_blk64,
	write_blk64,
};
#if 0
static void verbose_buffer(void* buf)
{
	int i;
	int offset=0;
	for(i=0;i<512;i++) {
		printf("offset 0x%x:  0x%x  0x%x  0x%x  0x%x\n",offset,
			*((unsigned int *)(buf+offset)), 
			*((unsigned int *)(buf+4)),
			*((unsigned int *)(buf+8)),
			*((unsigned int *)(buf+12)));
		offset+=16;
		i+=16;
	}
}
#endif
#if 0
static void verbose_superblock(struct ext2_super_block *param)
{
	int i;
	
	printf("s_inodes_count 0x%x\n", param->s_inodes_count);
	printf("s_blocks_count 0x%x\n", param->s_blocks_count);
	printf("s_r_blocks_count 0x%x\n", param->s_r_blocks_count);
	printf("s_free_blocks_count 0x%x\n", param->s_free_blocks_count);
	printf("s_free_inodes_count 0x%x\n", param->s_free_inodes_count);
	printf("s_first_data_block 0x%x\n", param->s_first_data_block);
	printf("s_log_block_size 0x%x\n", param->s_log_block_size);
	printf("s_log_frag_size 0x%x\n", param->s_log_frag_size);	
	printf("s_blocks_per_group 0x%x\n", param->s_blocks_per_group);	
	printf("s_frags_per_group 0x%x\n", param->s_frags_per_group);	
	printf("s_inodes_per_group 0x%x\n", param->s_inodes_per_group);
	printf("s_mtime 0x%x\n", param->s_mtime);
	printf("s_wtime 0x%x\n", param->s_wtime);
	printf("s_mnt_count 0x%x\n", param->s_mnt_count);
	printf("s_max_mnt_count 0x%x\n", param->s_max_mnt_count);

	printf("s_magic %d\n", param->s_magic);
	printf("s_state 0x%x\n", param->s_state);
	printf("s_errors 0x%x\n", param->s_errors);
	printf("s_minor_rev_level 0x%x\n", param->s_minor_rev_level);
	printf("s_lastcheck 0x%x\n", param->s_lastcheck);
	printf("s_checkinterval 0x%x\n", param->s_checkinterval);
	printf("s_creator_os 0x%x\n", param->s_creator_os);
	printf("s_rev_level 0x%x\n", param->s_rev_level);
	printf("s_def_resuid 0x%x\n", param->s_def_resuid);
	printf("s_def_resgid 0x%x\n", param->s_def_resgid);
	
		/*
		 * These fields are for EXT2_DYNAMIC_REV superblocks only.
		 *
		 * Note: the difference between the compatible feature set and
		 * the incompatible feature set is that if there is a bit set
		 * in the incompatible feature set that the kernel doesn't
		 * know about, it should refuse to mount the filesystem.
		 *
		 * e2fsck's requirements are more strict; if it doesn't know
		 * about a feature in either the compatible or incompatible
		 * feature set, it must abort and not try to meddle with
		 * things it doesn't understand...
		 */
		 printf("s_first_ino 0x%x\n", param->s_first_ino);
		printf("s_inode_size 0x%x\n", param->s_inode_size);
		printf("s_block_group_nr 0x%x\n", param->s_block_group_nr);
		printf("s_feature_compat 0x%x\n", param->s_feature_compat);
		printf("s_feature_incompat 0x%x\n", param->s_feature_incompat);
		printf("s_feature_ro_compat 0x%x\n", param->s_feature_ro_compat);
		for(i=0;i<16;i++)
			printf("s_uuid 0x%x\n", param->s_uuid[i]);
		printf("\n");
		printf("s_volume_name %s\n", param->s_volume_name);
		printf("s_last_mounted %s\n", param->s_last_mounted);
		printf("s_algorithm_usage_bitmap 0x%x\n", param->s_algorithm_usage_bitmap);
		printf("s_prealloc_blocks 0x%x\n", param->s_prealloc_blocks);
		printf("s_prealloc_dir_blocks 0x%x\n", param->s_prealloc_dir_blocks);
		printf("s_reserved_gdt_blocks 0x%x\n", param->s_reserved_gdt_blocks);
		/*
		 * Journaling support valid if EXT2_FEATURE_COMPAT_HAS_JOURNAL set.
		 */
		 
		for(i=0;i<16;i++)
			printf("s_journal_uuid 0x%x\n", param->s_journal_uuid[i]);
		printf("s_journal_inum 0x%x\n", param->s_journal_inum);
		printf("s_journal_dev 0x%x\n", param->s_journal_dev);
		printf("s_last_orphan 0x%x\n", param->s_last_orphan);
		for(i=0;i<4;i++)
			printf("s_hash_seed 0x%x\n", param->s_hash_seed[i]);
		printf("s_def_hash_version 0x%x\n", param->s_def_hash_version);
		printf("s_jnl_backup_type 0x%x\n", param->s_jnl_backup_type);
		printf("s_desc_size 0x%x\n", param->s_desc_size);
		printf("s_default_mount_opts 0x%x\n", param->s_default_mount_opts);
		printf("s_first_meta_bg 0x%x\n", param->s_first_meta_bg);
		printf("s_mkfs_time 0x%x\n", param->s_mkfs_time);
		for(i=0;i<17;i++)
			printf("s_jnl_blocks 0x%x\n", param->s_jnl_blocks[i]);


		
		printf("s_blocks_count_hi 0x%x\n", param->s_blocks_count_hi);
		printf("s_r_blocks_count_hi 0x%x\n", param->s_r_blocks_count_hi);
		printf("s_free_blocks_hi 0x%x\n", param->s_free_blocks_hi);
		printf("s_min_extra_isize 0x%x\n", param->s_min_extra_isize);
		printf("s_want_extra_isize 0x%x\n", param->s_want_extra_isize);
		printf("s_flags 0x%x\n", param->s_flags);


		printf("s_raid_stride 0x%x\n", param->s_raid_stride);
		printf("s_mmp_interval 0x%x\n", param->s_mmp_interval);
		printf("s_mmp_block[0] 0x%x\n", *((unsigned int *)&(param->s_mmp_block)));
		printf("s_mmp_block[1] 0x%x\n", *(((char *)(&(param->s_mmp_block))+4)));
		printf("s_raid_stripe_width 0x%x\n", param->s_raid_stripe_width);
		printf("s_log_groups_per_flex 0x%x\n", param->s_log_groups_per_flex);
		printf("s_reserved_char_pad 0x%x\n", param->s_reserved_char_pad);
		printf("s_reserved_pad 0x%x\n", param->s_reserved_pad);
		
		printf("s_kbytes_written[0] 0x%x\n", *((unsigned int *)&(param->s_kbytes_written)));
		printf("s_kbytes_written[1] 0x%x\n", *(((char *)(&(param->s_kbytes_written))+4)));
		
		

		printf("s_snapshot_inum 0x%x\n", param->s_snapshot_inum);
		printf("s_snapshot_id 0x%x\n", param->s_snapshot_id);
		printf("s_snapshot_r_blocks_count[0] 0x%x\n", *((unsigned int *)&(param->s_snapshot_r_blocks_count)));
		printf("s_snapshot_r_blocks_count[1] 0x%x\n", *(((char *)(&(param->s_snapshot_r_blocks_count))+4)));
		printf("s_snapshot_list 0x%x\n", param->s_snapshot_list);
		printf("s_error_count 0x%x\n", param->s_error_count);
		printf("s_first_error_time 0x%x\n", param->s_first_error_time);
		printf("s_first_error_ino 0x%x\n", param->s_first_error_ino);
		printf("s_first_error_block[0] 0x%x\n", *((unsigned int *)&(param->s_first_error_block)));
		printf("s_first_error_block[1] 0x%x\n", *(((char *)(&(param->s_first_error_block))+4)));
		for(i=0;i<32;i++)
			printf("s_first_error_func 0x%x\n", param->s_first_error_func[i]);
		printf("\n");
		printf("s_first_error_line 0x%x\n", param->s_first_error_line);
		printf("s_last_error_time 0x%x\n", param->s_last_error_time);
		

		printf("s_last_error_ino 0x%x\n", param->s_last_error_ino);
		printf("s_last_error_line 0x%x\n", param->s_last_error_line);
		
		printf("s_last_error_block[0] 0x%x\n", *((unsigned int *)&(param->s_last_error_block)));
		printf("s_last_error_block[1] 0x%x\n", *(((char *)(&(param->s_last_error_block))+4)));
		
		for(i=0;i<32;i++)
			printf("s_last_error_func 0x%x\n", param->s_last_error_func[i]);
		printf("\n");
		for(i=0;i<64;i++)
			printf("s_mount_opts 0x%x\n", param->s_mount_opts[i]);
		printf("\n");

}
#endif



/*in this function we assume some parameters.
All this parameters can be set by usr, or this time we 
make it a little easier to const value.*/
static void ext4fs_preinitialize(void)
{
	int blocksize=4096;
	int inode_ratio=16384;
	double reserved_ratio=5.0;
	struct ext_filesystem *fse = get_fs();
	//According to the mkfs.ext4, we have three choice:
	//floppy(<3M), small (<512M), default(>=512M)
	if (fse->total_sect < 3*1024*2) {
		//floppy
		fs->super->s_blocks_count = fse->total_sect/2;//sector size is 512, and block size=1K
		fs->super->s_inode_size=128;
		inode_ratio=8192;
		fs->super->s_blocks_count &=0xfffffffc;
		
		fs->super->s_log_block_size=0;
		fs->super->s_log_frag_size=0;
		blocksize=1024;
		
	} else if (fse->total_sect <512*1024*2) {
		//small
		
		fs->super->s_blocks_count = fse->total_sect/2;//sector size is 512, and block size=1K
		fs->super->s_inode_size=128;
		inode_ratio=4096;
		
		fs->super->s_blocks_count &=0xfffffffc;
		
		fs->super->s_log_block_size=0;
		fs->super->s_log_frag_size=0;
		blocksize=1024;
	} else {
		//default
		fs->super->s_blocks_count = fse->total_sect/8;//sector size is 512, and block size=4K
		fs->super->s_inode_size=256;
		inode_ratio=16384;
		
		fs->super->s_log_block_size=0x2;
		fs->super->s_log_frag_size=0x2;
		blocksize=4096;
	}

	/*
	 * Calculate number of blocks to reserve
	 */
	fs->super->s_r_blocks_count = (unsigned int) (reserved_ratio *
					fs->super->s_blocks_count / 100.0);

	fs->super->s_rev_level=1;
	fs->super->s_feature_compat = 0x3c;
	fs->super->s_feature_incompat=0x242;
	fs->super->s_feature_ro_compat=0x79;
	fs->super->s_log_groups_per_flex = 0x04;


	fs->super->s_inodes_count = ((__u64) fs->super->s_blocks_count*blocksize) / inode_ratio;	
	
	
		
}


/*
 * Calculate the number of GDT blocks to reserve for online filesystem growth.
 * The absolute maximum number of GDT blocks we can reserve is determined by
 * the number of block pointers that can fit into a single block.
 */
static unsigned int calc_reserved_gdt_blocks(ext2_filsys fs)
{
	struct ext2_super_block *sb = fs->super;
	unsigned long bpg = sb->s_blocks_per_group;
	unsigned int gdpb = EXT2_DESC_PER_BLOCK(sb);
	unsigned long max_blocks = 0xffffffff;
	unsigned long rsv_groups;
	unsigned int rsv_gdb;

	/* We set it at 1024x the current filesystem size, or
	 * the upper block count limit (2^32), whichever is lower.
	 */
	if (sb->s_blocks_count < max_blocks / 1024)
		max_blocks = sb->s_blocks_count * 1024;
	rsv_groups = ext2fs_div_ceil(max_blocks - sb->s_first_data_block, bpg);
	rsv_gdb = ext2fs_div_ceil(rsv_groups, gdpb) - fs->desc_blocks;
	if (rsv_gdb > EXT2_ADDR_PER_BLOCK(sb))
		rsv_gdb = EXT2_ADDR_PER_BLOCK(sb);
#ifdef RES_GDT_DEBUG
	printf("max_blocks %lu, rsv_groups = %lu, rsv_gdb = %u\n",
	       max_blocks, rsv_groups, rsv_gdb);
#endif

	return rsv_gdb;
}


static int test_root(int a, int b)
{
	if (a == 0)
		return 1;
	while (1) {
		if (a == 1)
			return 1;
		if (a % b)
			return 0;
		a = a / b;
	}
}

static int ext2fs_bg_has_super(ext2_filsys fs, int group_block)
{
	if (!(fs->super->s_feature_ro_compat &
	      EXT2_FEATURE_RO_COMPAT_SPARSE_SUPER))
		return 1;

	if (test_root(group_block, 3) || (test_root(group_block, 5)) ||
	    test_root(group_block, 7))
		return 1;

	return 0;
}

#define EXT2_DFL_CHECKINTERVAL (86400L * 180L)

int ext2fs_initialize(void)
{
	int	retval;
	struct ext2_super_block *super;
	int		frags_per_block;
	unsigned int	rem;
	unsigned int	overhead = 0;
	unsigned int	ipg;
	dgrp_t		i;
	blk_t		numblocks;
	int		rsv_gdt;
	int		csum_flag;
	char		c;

	if (!fs){
    	retval = ext2fs_get_mem(sizeof(struct struct_ext2_filsys), &fs);
    	if (retval)
    		return retval;
	}
	

	memset(fs, 0, sizeof(struct struct_ext2_filsys));
	fs->image_io->manager=fs->io->manager= &struct_devio_manager;
	fs->magic = EXT2_ET_MAGIC_EXT2FS_FILSYS;
	fs->flags = EXT2_FLAG_RW;
	fs->umask = 022;
#ifdef WORDS_BIGENDIAN
	fs->flags |= EXT2_FLAG_SWAP_BYTES;
#endif

	retval = ext2fs_get_mem(SUPERBLOCK_SIZE, &super);
	if (retval)
		goto cleanup;
	fs->super = super;

	memset(super, 0, SUPERBLOCK_SIZE);
	fs->io->manager->open(NULL, 0, NULL);

	ext4fs_preinitialize();
	

	

#define set_field(field, default) (super->field=  super->field? \
				   super->field : (default))

	super->s_magic = EXT2_SUPER_MAGIC;
	super->s_state = EXT2_VALID_FS;

	set_field(s_log_block_size, 0);	/* default blocksize: 1024 bytes */
	set_field(s_log_frag_size, 0); /* default fragsize: 1024 bytes */
	set_field(s_first_data_block, super->s_log_block_size ? 0 : 1);
	set_field(s_max_mnt_count, EXT2_DFL_MAX_MNT_COUNT);
	set_field(s_errors, EXT2_ERRORS_DEFAULT);
	set_field(s_feature_compat, 0);
	set_field(s_feature_incompat, 0);
	set_field(s_feature_ro_compat, 0);
	set_field(s_first_meta_bg, 0);
	set_field(s_raid_stride, 0);		/* default stride size: 0 */
	set_field(s_raid_stripe_width, 0);	/* default stripe width: 0 */
	set_field(s_log_groups_per_flex, 0);
	set_field(s_flags, 0);
	if (super->s_feature_incompat & ~EXT2_LIB_FEATURE_INCOMPAT_SUPP) {
		retval = EXT2_ET_UNSUPP_FEATURE;
		goto cleanup;
	}
	if (super->s_feature_ro_compat & ~EXT2_LIB_FEATURE_RO_COMPAT_SUPP) {
		retval = EXT2_ET_RO_UNSUPP_FEATURE;
		goto cleanup;
	}
	set_field(s_rev_level, EXT2_GOOD_OLD_REV);
	if (super->s_rev_level >= EXT2_DYNAMIC_REV) {
		set_field(s_first_ino, EXT2_GOOD_OLD_FIRST_INO);
		set_field(s_inode_size, EXT2_GOOD_OLD_INODE_SIZE);
		if (super->s_inode_size >= sizeof(struct ext2_inode_large)) {
			int extra_isize = sizeof(struct ext2_inode_large) -
				EXT2_GOOD_OLD_INODE_SIZE;
			set_field(s_min_extra_isize, extra_isize);
			set_field(s_want_extra_isize, extra_isize);
		}
	} else {
		super->s_first_ino = EXT2_GOOD_OLD_FIRST_INO;
		super->s_inode_size = EXT2_GOOD_OLD_INODE_SIZE;
	}

	set_field(s_checkinterval, EXT2_DFL_CHECKINTERVAL);
	super->s_mkfs_time = super->s_lastcheck = 0x5105cd7b;//fs->now ? fs->now : time(NULL);

	super->s_creator_os = CREATOR_OS;

	fs->blocksize = EXT2_BLOCK_SIZE(super);
	fs->fragsize = EXT2_FRAG_SIZE(super);
	frags_per_block = fs->blocksize / fs->fragsize;

	/* default: (fs->blocksize*8) blocks/group, up to 2^16 (GDT limit) */
	set_field(s_blocks_per_group, fs->blocksize * 8);
	if (super->s_blocks_per_group > EXT2_MAX_BLOCKS_PER_GROUP(super))
		super->s_blocks_per_group = EXT2_MAX_BLOCKS_PER_GROUP(super);
	super->s_frags_per_group = super->s_blocks_per_group * frags_per_block;



retry:
	fs->group_desc_count = ext2fs_div_ceil(super->s_blocks_count -
					       super->s_first_data_block,
					       EXT2_BLOCKS_PER_GROUP(super));
	if (fs->group_desc_count == 0) {
		retval = EXT2_ET_TOOSMALL;
		goto cleanup;
	}
	fs->desc_blocks = ext2fs_div_ceil(fs->group_desc_count,
					  EXT2_DESC_PER_BLOCK(super));

	i = fs->blocksize >= 4096 ? 1 : 4096 / fs->blocksize;
	set_field(s_inodes_count, super->s_blocks_count / i);

	/*
	 * Make sure we have at least EXT2_FIRST_INO + 1 inodes, so
	 * that we have enough inodes for the filesystem(!)
	 */
	if (super->s_inodes_count < EXT2_FIRST_INODE(super)+1)
		super->s_inodes_count = EXT2_FIRST_INODE(super)+1;

	/*
	 * There should be at least as many inodes as the user
	 * requested.  Figure out how many inodes per group that
	 * should be.  But make sure that we don't allocate more than
	 * one bitmap's worth of inodes each group.
	 */
	ipg = ext2fs_div_ceil(super->s_inodes_count, fs->group_desc_count);
	if (ipg > fs->blocksize * 8) {
		if (super->s_blocks_per_group >= 256) {
			/* Try again with slightly different parameters */
			super->s_blocks_per_group -= 8;
			super->s_frags_per_group = super->s_blocks_per_group *
				frags_per_block;
			goto retry;
		} else {
			retval = EXT2_ET_TOO_MANY_INODES;
			goto cleanup;
		}
	}

	if (ipg > (unsigned) EXT2_MAX_INODES_PER_GROUP(super))
		ipg = EXT2_MAX_INODES_PER_GROUP(super);

ipg_retry:
	super->s_inodes_per_group = ipg;//Tina:we have to make sure how many inodes per group

	/*
	 * Make sure the number of inodes per group completely fills
	 * the inode table blocks in the descriptor.  If not, add some
	 * additional inodes/group.  Waste not, want not...
	 */
	fs->inode_blocks_per_group = (((super->s_inodes_per_group *
					EXT2_INODE_SIZE(super)) +
				       EXT2_BLOCK_SIZE(super) - 1) /
				      EXT2_BLOCK_SIZE(super));
	super->s_inodes_per_group = ((fs->inode_blocks_per_group *
				      EXT2_BLOCK_SIZE(super)) /
				     EXT2_INODE_SIZE(super));
	/*
	 * Finally, make sure the number of inodes per group is a
	 * multiple of 8.  This is needed to simplify the bitmap
	 * splicing code.
	 */
	super->s_inodes_per_group &= ~7;
	fs->inode_blocks_per_group = (((super->s_inodes_per_group *
					EXT2_INODE_SIZE(super)) +
				       EXT2_BLOCK_SIZE(super) - 1) /
				      EXT2_BLOCK_SIZE(super));

	/*
	 * adjust inode count to reflect the adjusted inodes_per_group
	 */
	if ((__u64)super->s_inodes_per_group * fs->group_desc_count > ~0U) {
		ipg--;
		goto ipg_retry;
	}
	super->s_inodes_count = super->s_inodes_per_group *
		fs->group_desc_count;
	super->s_free_inodes_count = super->s_inodes_count;

	/*
	 * check the number of reserved group descriptor table blocks
	 */
	if (super->s_feature_compat & EXT2_FEATURE_COMPAT_RESIZE_INODE)
		rsv_gdt = calc_reserved_gdt_blocks(fs);
	else
		rsv_gdt = 0;
	set_field(s_reserved_gdt_blocks, rsv_gdt);
	if (super->s_reserved_gdt_blocks > EXT2_ADDR_PER_BLOCK(super)) {
		retval = EXT2_ET_RES_GDT_BLOCKS;
		goto cleanup;
	}

	/*
	 * Calculate the maximum number of bookkeeping blocks per
	 * group.  It includes the superblock, the block group
	 * descriptors, the block bitmap, the inode bitmap, the inode
	 * table, and the reserved gdt blocks.
	 */
	overhead = (int) (3 + fs->inode_blocks_per_group +
			  fs->desc_blocks + super->s_reserved_gdt_blocks);

	//printf("overhead 0x%x\n",overhead);
	//printf("0x%x, 0x%x, 0x%x\n", fs->inode_blocks_per_group, 
	//	fs->desc_blocks, super->s_reserved_gdt_blocks);
	//printf("0x%x\n", super->s_blocks_per_group);
	/* This can only happen if the user requested too many inodes */
	if (overhead > super->s_blocks_per_group) {
		retval = EXT2_ET_TOO_MANY_INODES;
		goto cleanup;
	}

	/*
	 * See if the last group is big enough to support the
	 * necessary data structures.  If not, we need to get rid of
	 * it.  We need to recalculate the overhead for the last block
	 * group, since it might or might not have a superblock
	 * backup.
	 */
	overhead = (int) (2 + fs->inode_blocks_per_group);
	if (ext2fs_bg_has_super(fs, fs->group_desc_count - 1))
		overhead += 1 + fs->desc_blocks + super->s_reserved_gdt_blocks;
	rem = ((super->s_blocks_count - super->s_first_data_block) %
	       super->s_blocks_per_group);
	if ((fs->group_desc_count == 1) && rem && (rem < overhead)) {
		retval = EXT2_ET_TOOSMALL;
		goto cleanup;
	}
	if (rem && (rem < overhead+50)) {
		super->s_blocks_count -= rem;
		goto retry;
	}

	/*
	 * At this point we know how big the filesystem will be.  So
	 * we can do any and all allocations that depend on the block
	 * count.
	 */


	retval = ext2fs_allocate_block_bitmap(fs, NULL, &fs->block_map);
	if (retval) {
		printf("ext2fs_allocate_block_bitmap cannot allocate\n");
		goto cleanup;
	}
	retval = ext2fs_allocate_inode_bitmap(fs, NULL, &fs->inode_map);
	if (retval) {
		printf("ext2fs_allocate_inode_bitmap cannot allocate\n");
		goto cleanup;

	}


	retval = ext2fs_get_array(fs->desc_blocks, fs->blocksize,
				&fs->group_desc);
	if (retval){
		printf("ext2fs_get_array cannot get array\n");
		goto cleanup;
	}
	memset(fs->group_desc, 0, (size_t) fs->desc_blocks * fs->blocksize);


	/*
	 * Reserve the superblock and group descriptors for each
	 * group, and fill in the correct group statistics for group.
	 * Note that although the block bitmap, inode bitmap, and
	 * inode table have not been allocated (and in fact won't be
	 * by this routine), they are accounted for nevertheless.
	 *
	 * If FLEX_BG meta-data grouping is used, only account for the
	 * superblock and group descriptors (the inode tables and
	 * bitmaps will be accounted for when allocated).
	 */
	//Tina: set s_free_blocks_count
	//Tina: set block group descriptors
	super->s_free_blocks_count = 0;
	csum_flag = EXT2_HAS_RO_COMPAT_FEATURE(fs->super,
					       EXT4_FEATURE_RO_COMPAT_GDT_CSUM);
	for (i = 0; i < fs->group_desc_count; i++) {//Tina: for each group 
		/*
		 * Don't set the BLOCK_UNINIT group for the last group
		 * because the block bitmap needs to be padded.
		 */
		if (csum_flag) {
			if (i != fs->group_desc_count - 1)
				fs->group_desc[i].bg_flags |=
					EXT2_BG_BLOCK_UNINIT;
			fs->group_desc[i].bg_flags |= EXT2_BG_INODE_UNINIT;
			numblocks = super->s_inodes_per_group;
			if (i == 0)
				numblocks -= super->s_first_ino;
			fs->group_desc[i].bg_itable_unused = numblocks;//how many inode are free in the group
		}
		numblocks = ext2fs_reserve_super_and_bgd(fs, i, fs->block_map);
		if (fs->super->s_log_groups_per_flex)
			numblocks += 2 + fs->inode_blocks_per_group;//data blocks+two bitmap blocks+inode table blocks

		super->s_free_blocks_count += numblocks;
		fs->group_desc[i].bg_free_blocks_count = numblocks;
		fs->group_desc[i].bg_free_inodes_count =
			fs->super->s_inodes_per_group;
		fs->group_desc[i].bg_used_dirs_count = 0;
		ext2fs_group_desc_csum_set(fs, i);
	}

	c = (char) 255;
	if (((int) c) == -1) {
		super->s_flags |= EXT2_FLAGS_SIGNED_HASH;
	} else {
		super->s_flags |= EXT2_FLAGS_UNSIGNED_HASH;
	}


	ext2fs_mark_super_dirty(fs);
	ext2fs_mark_bb_dirty(fs);
	ext2fs_mark_ib_dirty(fs);
	//printf("Tina: hehe\n");
	//printf("Tina: set_blksize 0x%x\n", fs->io->manager->set_blksize);
	io_channel_set_blksize(fs->io, fs->blocksize);

	return 0;

cleanup:
	ext2fs_free(fs);
	
	return retval;
}

static void write_inode_tables(ext2_filsys fs, int lazy_flag, int itable_zeroed)
{
	errcode_t	retval;
	blk_t		blk;
	dgrp_t		i;
	int		num, ipb;


	for (i = 0; i < fs->group_desc_count; i++) {

		blk = fs->group_desc[i].bg_inode_table;
		num = fs->inode_blocks_per_group;

		if (lazy_flag) {
			ipb = fs->blocksize / EXT2_INODE_SIZE(fs->super);
			num = ((((fs->super->s_inodes_per_group -
				  fs->group_desc[i].bg_itable_unused) *
				 EXT2_INODE_SIZE(fs->super)) +
				EXT2_BLOCK_SIZE(fs->super) - 1) /
			       EXT2_BLOCK_SIZE(fs->super));
		}
		if (!lazy_flag || itable_zeroed) {
			/* The kernel doesn't need to zero the itable blocks */
			fs->group_desc[i].bg_flags |= EXT2_BG_INODE_ZEROED;
			ext2fs_group_desc_csum_set(fs, i);
		}
		retval = ext2fs_zero_blocks(fs, blk, num, &blk, &num);
		if (retval) {
			printf("Could not write %d blocks in inode table starting at %u \n",num, blk);
			return;
		}
	}
	ext2fs_zero_blocks(0, 0, 0, 0, 0);
}

static void create_root_dir(ext2_filsys fs)
{
	errcode_t		retval;

	retval = ext2fs_mkdir(fs, EXT2_ROOT_INO, EXT2_ROOT_INO, 0);
	if (retval) {
		printf("error: while creating root dir\n");
		return;
	}
}

static void create_lost_and_found(ext2_filsys fs)
{
	unsigned int		lpf_size = 0;
	errcode_t		retval;
	ext2_ino_t		ino;
	const char		*name = "lost+found";
	int			i;

	fs->umask = 077;
	retval = ext2fs_mkdir(fs, EXT2_ROOT_INO, 0, name);
	if (retval) {
		printf("create_lost_and_found: ext2fs_mkdir error\n");
		return ;
	}

	retval = ext2fs_lookup(fs, EXT2_ROOT_INO, name, strlen(name), 0, &ino);
	if (retval) {
		printf("create_lost_and_found: ext2fs_lookup\n");
		return ;
	}

	for (i=1; i < EXT2_NDIR_BLOCKS; i++) {
		/* Ensure that lost+found is at least 2 blocks, so we always
		 * test large empty blocks for big-block filesystems.  */
		if ((lpf_size += fs->blocksize) >= 16*1024 &&
		    lpf_size >= 2 * fs->blocksize)
			break;
		retval = ext2fs_expand_dir(fs, ino);
		if (retval) {
			printf("create_lost_and_found: ext2fs_expand_dir\n");
			return;
		}
	}
}

static void reserve_inodes(ext2_filsys fs)
{
	ext2_ino_t	i;

	for (i = EXT2_ROOT_INO + 1; i < EXT2_FIRST_INODE(fs->super); i++)
		ext2fs_inode_alloc_stats2(fs, i, +1, 0);
	ext2fs_mark_ib_dirty(fs);
}

static void create_bad_block_inode(ext2_filsys fs, badblocks_list bb_list)
{
	errcode_t	retval;

	ext2fs_mark_inode_bitmap(fs->inode_map, EXT2_BAD_INO);
	ext2fs_inode_alloc_stats2(fs, EXT2_BAD_INO, +1, 0);
	retval = ext2fs_update_bb_inode(fs, bb_list);
	if (retval) {
		printf("create_bad_block_inode: ext2fs_update_bb_inode error\n");
	}

}

/*
 * Determine the number of journal blocks to use, either via
 * user-specified # of megabytes, or via some intelligently selected
 * defaults.
 *
 * Find a reasonable journal file size (in blocks) given the number of blocks
 * in the filesystem.  For very small filesystems, it is not reasonable to
 * have a journal that fills more than half of the filesystem.
 */
static unsigned int figure_journal_size(int size, ext2_filsys fs)
{
	int j_blocks;

	j_blocks = ext2fs_default_journal_size(fs->super->s_blocks_count);
	if (j_blocks < 0) {
		printf("\nFilesystem too small for a journal\n");
		return 0;
	}

	if (size > 0) {
		j_blocks = size * 1024 / (fs->blocksize	/ 1024);
		if (j_blocks < 1024 || j_blocks > 10240000) {
			printf("\nThe requested journal "
				"size is %d blocks; it must be\n"
				"between 1024 and 10240000 blocks.  "
				"Aborting.\n");
			return 1;
		}
		if ((unsigned) j_blocks > fs->super->s_free_blocks_count / 2) {
			printf("\nJournal size too big for filesystem.\n");
			return 1;
		}
	}
	return j_blocks;
}

#if 0
static void zap_sector(ext2_filsys fs, int sect, int nsect)
{
	char *buf;
	int retval;

	buf = malloc(512*nsect);
	if (!buf) {
		printf("Out of memory erasing sectors %d-%d\n",
		       sect, sect + nsect - 1);
		return;
	}

	memset(buf, 0, 512*nsect);
	io_channel_set_blksize(fs->io, 512);
	retval = io_channel_write_blk(fs->io, sect, nsect, buf);
	io_channel_set_blksize(fs->io, fs->blocksize);
	free(buf);
	if (retval)
		printf("could not erase sector %d\n",sect);
}
#endif

int ext4_format(void)
{
	errcode_t	retval;
	int val;
	unsigned int i;
	badblocks_list	bb_list = 0;
	unsigned int	journal_blocks;
	int	journal_flags=0;	
	int journal_size=0;;
	if (ext2fs_initialize()){
		printf("error cannot format\n");
		return -1;
	}
	
	if ((fs->super->s_feature_incompat &
	     (EXT3_FEATURE_INCOMPAT_EXTENTS|EXT4_FEATURE_INCOMPAT_FLEX_BG)) ||
	    (fs->super->s_feature_ro_compat &
	     (EXT4_FEATURE_RO_COMPAT_HUGE_FILE|EXT4_FEATURE_RO_COMPAT_GDT_CSUM|
	      EXT4_FEATURE_RO_COMPAT_DIR_NLINK|
	      EXT4_FEATURE_RO_COMPAT_EXTRA_ISIZE)))
		fs->super->s_kbytes_written = 1;

	//Parse or generate a UUID
	//Since we don't know how to generate a UUID in uboot, we give it the fixed number temporarily
	*((unsigned int *)(&fs->super->s_uuid[0]))=0x2b8761f7;
	*((unsigned int *)(&fs->super->s_uuid[4]))=0xf344e580;
	*((unsigned int *)(&fs->super->s_uuid[8]))=0x16b499a6;
	*((unsigned int *)(&fs->super->s_uuid[12]))=0xd29fb741;

		//initialize the directory index variables
	fs->super->s_def_hash_version=EXT2_HASH_HALF_MD4;
	fs->super->s_hash_seed[0]=0xae1ba9fb;
	fs->super->s_hash_seed[1]=0x2145bde9;
	fs->super->s_hash_seed[2]=0x1182c581;
	fs->super->s_hash_seed[3]=0xfc00aa81;

	/*
	 * Add "jitter" to the superblock's check interval so that we
	 * don't check all the filesystems at the same time.  We use a
	 * kludgy hack of using the UUID to derive a random jitter value.
	 */
	for (i = 0, val = 0 ; i < sizeof(fs->super->s_uuid); i++)
		val += fs->super->s_uuid[i];
	fs->super->s_max_mnt_count += val % EXT2_DFL_MAX_MNT_COUNT;


	/*
	 * For the Hurd, we will turn off filetype since it doesn't
	 * support it.
	 */
	if (fs->super->s_creator_os == EXT2_OS_HURD)
		fs->super->s_feature_incompat &=
			~EXT2_FEATURE_INCOMPAT_FILETYPE;
	//printf("Tina: allocate tables\n");
	retval = ext2fs_allocate_tables(fs);
	if (retval) {
		printf("Error: while trying to allocate filesystem tables\n");
		return 1;
	}
	

	{
			/* rsv must be a power of two (64kB is MD RAID sb alignment) */
			unsigned int rsv = 65536 / fs->blocksize;
			unsigned long blocks = fs->super->s_blocks_count;
			unsigned long start;
			blk_t ret_blk;
			//zap_sector(fs, 0, 2);//clear the first two sectors
	
			/*
			 * Wipe out any old MD RAID (or other) metadata at the end
			 * of the device.  This will also verify that the device is
			 * as large as we think.  Be careful with very small devices.
			 */
			start = (blocks & ~(rsv - 1));
			if (start > rsv)
				start -= rsv;
			if (start > 0)
				retval = ext2fs_zero_blocks(fs, start, blocks - start,
								&ret_blk, NULL);
	
			if (retval) {
				printf("error while zeroing block %u at end of filesystem\n", ret_blk);
			}
			printf("write inode tables\n");
			write_inode_tables(fs, 1, 0);
			create_root_dir(fs);
			create_lost_and_found(fs);
			printf("reserve inodes\n");
			reserve_inodes(fs);
			printf("creating bad block inode\n");
			create_bad_block_inode(fs, bb_list);
			
			if (fs->super->s_feature_compat &
				EXT2_FEATURE_COMPAT_RESIZE_INODE) {
				printf("creating resize inode\n");
				retval = ext2fs_create_resize_inode(fs);
				if (retval) {
					printf("ext2fs_create_resize_inode error\n");
					return 1;
				}
			}
	}
	
	//about the journal
	//printf("figure_journal_size\n");
	journal_blocks = figure_journal_size(journal_size, fs);
	if (!journal_blocks) {
		fs->super->s_feature_compat &=
			~EXT3_FEATURE_COMPAT_HAS_JOURNAL;
		goto no_journal;
	}

	printf("Do the journal and the journal size is 0x%x blocks: \n", journal_blocks);
	
	retval = ext2fs_add_journal_inode(fs, journal_blocks,
					  journal_flags);
	if (retval) {
		printf("ext2fs_add_journal_inode error\n");
		return 1;
	}
	
	//verbose_superblock(fs->super);
	
no_journal:
	printf("\nWriting superblocks and filesystem accounting information: \n");
	
	retval = ext2fs_flush(fs);
	if (retval) {
		printf("\nWarning, had trouble writing out superblocks.\n");
	}
	
	val = ext2fs_close(fs);

	return 0;
}

