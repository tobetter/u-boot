/*
 * fat.c
 *
 * R/O (V)FAT 12/16/32 filesystem implementation by Marcus Sundberg
 *
 * 2002-07-28 - rjones@nexus-tech.net - ported to ppcboot v1.1.6
 * 2003-03-10 - kharris@nexus-tech.net - ported to uboot
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

#include "../v2_burning_i.h"
#include <part.h>
#include <fat.h>
#include <partition_table.h>
#include <mmc.h>

#undef  FAT_ERROR
#define FAT_ERROR(fmt...) printf("[FAT_ERR]L%d,", __LINE__),printf(fmt)

#define FAT_MSG(fmt...) printf("fat:"fmt)

extern int disk_read (__u32 startblock, __u32 getsize, __u8 * bufptr);
extern int device_boot_flag;

static int v2_ext_mmc_read(__u32 startblock, __u32 nBlk, __u8 * bufptr)
{
    int ret = 0;
    char* usb_update = getenv("usb_update");
    if(strcmp(usb_update,"1"))
    {
        //Attention: So far the work flow of sdc_burn or sdc_update after store_init(0), so device_boot_flag is setup yet!
        if((device_boot_flag == SPI_EMMC_FLAG) || (device_boot_flag == EMMC_BOOT_FLAG))
        {
           	static struct mmc *mmc = NULL;

        	if(!mmc)
        	{
            	mmc = find_mmc_device(0);
            	if(!mmc){
                	FAT_ERROR("Fail to find mmc 0 device");
                	return __LINE__;
            	}
        	}
        	ret = mmc_init(mmc);
        	if(ret){
            	FAT_ERROR("Fail to init mmc 0 device");
            	return __LINE__;
        	}
    	}
    }

    ret = disk_read(startblock, nBlk, bufptr);

    return ret;
}

/*
 * Convert a string to lowercase.
 */
static void
downcase(char *str)
{
	while (*str != '\0') {
		TOLOWER(*str);
		str++;
	}
}

/*
 * Get the first occurence of a directory delimiter ('/' or '\') in a string.
 * Return index into string if found, -1 otherwise.
 */
static int
dirdelim(char *str)
{
	char *start = str;

	while (*str != '\0') {
		if (ISDIRDELIM(*str)) return str - start;
		str++;
	}
	return -1;
}


/*
 * Match volume_info fs_type strings.
 * Return 0 on match, -1 otherwise.
 */
static int
compare_sign(char *str1, char *str2)
{
	char *end = str1+SIGNLEN;

	while (str1 != end) {
		if (*str1 != *str2) {
			return -1;
		}
		str1++;
		str2++;
	}

	return 0;
}


/*
 * Extract zero terminated short name from a directory entry.
 */
static void get_name (dir_entry *dirent, char *s_name)
{
	char *ptr;

	memcpy (s_name, dirent->name, 8);
	s_name[8] = '\0';
	ptr = s_name;
	while (*ptr && *ptr != ' ')
		ptr++;
	if (dirent->ext[0] && dirent->ext[0] != ' ') {
		*ptr = '.';
		ptr++;
		memcpy (ptr, dirent->ext, 3);
		ptr[3] = '\0';
		while (*ptr && *ptr != ' ')
			ptr++;
	}
	*ptr = '\0';
	if (*s_name == DELETED_FLAG)
		*s_name = '\0';
	else if (*s_name == aRING)
		*s_name = DELETED_FLAG;
	downcase (s_name);
}

/*
 * Get the cluster entry at index 'entry' in a FAT (12/16/32) table.
 * On failure 0x00 is returned.
 */
static __u32 get_fatent(fsdata *mydata, __u32 entry/*cluster index*/)
{
	__u32 bufnum;
	__u32 offset;
	__u32 ret = 0x00;//On failure 0x00 is returned.

	switch (mydata->fatsize) {
	case 32:
		bufnum = entry / FAT32BUFSIZE;
		offset = entry - bufnum * FAT32BUFSIZE;
		break;
	case 16:
		bufnum = entry / FAT16BUFSIZE;
		offset = entry - bufnum * FAT16BUFSIZE;
		break;
	case 12:
		bufnum = entry / FAT12BUFSIZE;
		offset = entry - bufnum * FAT12BUFSIZE;
		break;

	default:
		/* Unsupported FAT size */
		return ret;
	}

	/* Read a new block of FAT entries into the cache. */
    /* this block of FAT entries is not cached */
	if (bufnum != mydata->fatbufnum) {
		int getsize = FATBUFSIZE/FS_BLOCK_SIZE;//==6
		__u8 *bufptr = mydata->fatbuf;
		__u32 fatlength = mydata->fatlength;
		__u32 startblock = bufnum * FATBUFBLOCKS;

		fatlength *= SECTOR_SIZE;	/* We want it in bytes now */
		startblock += mydata->fat_sect;	/* Offset from start of disk */

		if (getsize > fatlength) getsize = fatlength;
		if (v2_ext_mmc_read(startblock, getsize, bufptr) < 0) {
			FAT_DPRINT("Error reading FAT blocks\n");
			return ret;
		}
		mydata->fatbufnum = bufnum;
	}

	/* Get the actual entry from the table */
	switch (mydata->fatsize) {
	case 32:
		ret = FAT2CPU32(((__u32*)mydata->fatbuf)[offset]);
		break;
	case 16:
		ret = FAT2CPU16(((__u16*)mydata->fatbuf)[offset]);
		break;
	case 12: {
		__u32 off16 = (offset*3)/4;
		__u16 val1, val2;

		switch (offset & 0x3) {
		case 0:
			ret = FAT2CPU16(((__u16*)mydata->fatbuf)[off16]);
			ret &= 0xfff;
			break;
		case 1:
			val1 = FAT2CPU16(((__u16*)mydata->fatbuf)[off16]);
			val1 &= 0xf000;
			val2 = FAT2CPU16(((__u16*)mydata->fatbuf)[off16+1]);
			val2 &= 0x00ff;
			ret = (val2 << 4) | (val1 >> 12);
			break;
		case 2:
			val1 = FAT2CPU16(((__u16*)mydata->fatbuf)[off16]);
			val1 &= 0xff00;
			val2 = FAT2CPU16(((__u16*)mydata->fatbuf)[off16+1]);
			val2 &= 0x000f;
			ret = (val2 << 8) | (val1 >> 8);
			break;
		case 3:
			ret = FAT2CPU16(((__u16*)mydata->fatbuf)[off16]);;
			ret = (ret & 0xfff0) >> 4;
			break;
		default:
			break;
		}
	}
	break;
	}
	FAT_DPRINT("ret: %d, offset: %d\n", ret, offset);

	return ret;
}


/*
 * Read at most 'size' bytes from the specified cluster into 'buffer'.
 * Return 0 on success, -1 otherwise.
 */
static int
get_cluster(fsdata *mydata, const __u32 clustnum, __u8 *buffer, const unsigned long size)
{
	int idx = 0;
	__u32 startsect;

	if (clustnum > 0) {
		startsect = mydata->data_begin + clustnum*mydata->clust_size;
	} else {
		startsect = mydata->rootdir_sect;
	}

	FAT_DPRINT("gc - clustnum: %d, startsect: %d\n", clustnum, startsect);
	if (v2_ext_mmc_read(startsect, size/FS_BLOCK_SIZE , buffer) < 0) {
		FAT_DPRINT("Error reading data\n");
		return -1;
	}
	if(size % FS_BLOCK_SIZE) {
		__u8 tmpbuf[FS_BLOCK_SIZE];
		idx= size/FS_BLOCK_SIZE;
		if (v2_ext_mmc_read(startsect + idx, 1, tmpbuf) < 0) {
			FAT_ERROR("Error reading data\n");
			return -1;
		}
		buffer += idx*FS_BLOCK_SIZE;

		memcpy(buffer, tmpbuf, size % FS_BLOCK_SIZE);
		return 0;
	}

	return 0;
}

#ifdef CONFIG_SUPPORT_VFAT
/*
 * Extract the file name information from 'slotptr' into 'l_name',
 * starting at l_name[*idx].
 * Return 1 if terminator (zero byte) is found, 0 otherwise.
 */
static int
slot2str(dir_slot *slotptr, char *l_name, int *idx)
{
	int j;

	for (j = 0; j <= 8; j += 2) {
		l_name[*idx] = slotptr->name0_4[j];
		if (l_name[*idx] == 0x00) return 1;
		(*idx)++;
	}
	for (j = 0; j <= 10; j += 2) {
		l_name[*idx] = slotptr->name5_10[j];
		if (l_name[*idx] == 0x00) return 1;
		(*idx)++;
	}
	for (j = 0; j <= 2; j += 2) {
		l_name[*idx] = slotptr->name11_12[j];
		if (l_name[*idx] == 0x00) return 1;
		(*idx)++;
	}

	return 0;
}


/*
 * Extract the full long filename starting at 'retdent' (which is really
 * a slot) into 'l_name'. If successful also copy the real directory entry
 * into 'retdent'
 * Return 0 on success, -1 otherwise.
 */
__attribute__ ((__aligned__(__alignof__(dir_entry))))
__u8 get_vfatname_block[MAX_CLUSTSIZE];
static int
get_vfatname(fsdata *mydata, int curclust, __u8 *cluster,
	     dir_entry *retdent, char *l_name)
{
	dir_entry *realdent;
	dir_slot  *slotptr = (dir_slot*) retdent;
	__u8	  *nextclust = cluster + mydata->clust_size * SECTOR_SIZE;
	__u8	   counter = (slotptr->id & ~LAST_LONG_ENTRY_MASK) & 0xff;
	int idx = 0;

	while ((__u8*)slotptr < nextclust) {
		if (counter == 0) break;
		if (((slotptr->id & ~LAST_LONG_ENTRY_MASK) & 0xff) != counter)
			return -1;
		slotptr++;
		counter--;
	}

	if ((__u8*)slotptr >= nextclust) {
		dir_slot *slotptr2;

		slotptr--;
		curclust = get_fatent(mydata, curclust);
		if (CHECK_CLUST(curclust, mydata->fatsize)) {
			FAT_ERROR("curclust: 0x%x\n", curclust);
			FAT_ERROR("Invalid FAT entry\n");
			return -1;
		}
		if (get_cluster(mydata, curclust, get_vfatname_block,
				mydata->clust_size * SECTOR_SIZE) != 0) {
            FAT_ERROR("Error: reading directory block\n");
			return -1;
		}
		slotptr2 = (dir_slot*) get_vfatname_block;
		while (slotptr2->id > 0x01) {
			slotptr2++;
		}
		/* Save the real directory entry */
		realdent = (dir_entry*)slotptr2 + 1;
		while ((__u8*)slotptr2 >= get_vfatname_block) {
			slot2str(slotptr2, l_name, &idx);
			slotptr2--;
		}
	} else {
		/* Save the real directory entry */
		realdent = (dir_entry*)slotptr;
	}

	do {
		slotptr--;
		if (slot2str(slotptr, l_name, &idx)) break;
	} while (!(slotptr->id & LAST_LONG_ENTRY_MASK));

	l_name[idx] = '\0';
	if (*l_name == DELETED_FLAG) *l_name = '\0';
	else if (*l_name == aRING) *l_name = DELETED_FLAG;
	downcase(l_name);

	/* Return the real directory entry */
	memcpy(retdent, realdent, sizeof(dir_entry));

	return 0;
}


/* Calculate short name checksum */
static __u8
mkcksum(const char *str)
{
	int i;
	__u8 ret = 0;

	for (i = 0; i < 11; i++) {
		ret = (((ret&1)<<7)|((ret&0xfe)>>1)) + str[i];
	}

	return ret;
}
#endif


/*
 * Get the directory entry associated with 'filename' from the directory
 * starting at 'startsect'
 */
__attribute__ ((__aligned__(__alignof__(dir_entry))))
__u8 get_dentfromdir_block[MAX_CLUSTSIZE];
static dir_entry *get_dentfromdir (fsdata * mydata, int startsect,
				   char *filename, dir_entry * retdent,
				   int dols)
{
    __u16 prevcksum = 0xffff;
    __u32 curclust = START (retdent);
    int files = 0, dirs = 0;

    FAT_DPRINT ("get_dentfromdir: %s\n", filename);
    while (1) {
	dir_entry *dentptr;
	int i;

	if (get_cluster (mydata, curclust, get_dentfromdir_block,
		 mydata->clust_size * SECTOR_SIZE) != 0) {
	    FAT_DPRINT ("Error: reading directory block\n");
	    return NULL;
	}
	dentptr = (dir_entry *) get_dentfromdir_block;
	for (i = 0; i < DIRENTSPERCLUST; i++) {
	    char s_name[14], l_name[256];

	    l_name[0] = '\0';
	    if (dentptr->name[0] == DELETED_FLAG) {
		    dentptr++;
		    continue;
	    }
	    if ((dentptr->attr & ATTR_VOLUME)) {
#ifdef CONFIG_SUPPORT_VFAT
		if ((dentptr->attr & ATTR_VFAT) &&
		    (dentptr->name[0] & LAST_LONG_ENTRY_MASK)) {
		    prevcksum = ((dir_slot *) dentptr)
			    ->alias_checksum;
		    get_vfatname (mydata, curclust, get_dentfromdir_block,
				  dentptr, l_name);
		    if (dols) {
			int isdir = (dentptr->attr & ATTR_DIR);
			char dirc;
			int doit = 0;

			if (isdir) {
			    dirs++;
			    dirc = '/';
			    doit = 1;
			} else {
			    dirc = ' ';
			    if (l_name[0] != 0) {
				files++;
				doit = 1;
			    }
			}
			if (doit) {
			    if (dirc == ' ') {
				printf (" %8ld   %s%c\n",
					(long) FAT2CPU32 (dentptr->size),
					l_name, dirc);
			    } else {
				printf ("            %s%c\n", l_name, dirc);
			    }
			}
			dentptr++;
			continue;
		    }
		    FAT_DPRINT ("vfatname: |%s|\n", l_name);
		} else
#endif
		{
		    /* Volume label or VFAT entry */
		    dentptr++;
		    continue;
		}
	    }
	    if (dentptr->name[0] == 0) {
		if (dols) {
		    printf ("\n%d file(s), %d dir(s)\n\n", files, dirs);
		}
		FAT_DPRINT ("Dentname == NULL - %d\n", i);
		return NULL;
	    }
#ifdef CONFIG_SUPPORT_VFAT
	    if (dols && mkcksum (dentptr->name) == prevcksum) {
		dentptr++;
		continue;
	    }
#endif
	    get_name (dentptr, s_name);
	    if (dols) {
		int isdir = (dentptr->attr & ATTR_DIR);
		char dirc;
		int doit = 0;

		if (isdir) {
		    dirs++;
		    dirc = '/';
		    doit = 1;
		} else {
		    dirc = ' ';
		    if (s_name[0] != 0) {
			files++;
			doit = 1;
		    }
		}
		if (doit) {
		    if (dirc == ' ') {
			printf (" %8ld   %s%c\n",
				(long) FAT2CPU32 (dentptr->size), s_name,
				dirc);
		    } else {
			printf ("            %s%c\n", s_name, dirc);
		    }
		}
		dentptr++;
		continue;
	    }
	    if (strcmp (filename, s_name) && strcmp (filename, l_name)) {
		FAT_DPRINT ("Mismatch: |%s|%s|\n", s_name, l_name);
		dentptr++;
		continue;
	    }
	    memcpy (retdent, dentptr, sizeof (dir_entry));

	    FAT_DPRINT ("DentName: %s", s_name);
	    FAT_DPRINT (", start: 0x%x", START (dentptr));
	    FAT_DPRINT (", size:  0x%x %s\n",
			FAT2CPU32 (dentptr->size),
			(dentptr->attr & ATTR_DIR) ? "(DIR)" : "");

	    return retdent;
	}
	curclust = get_fatent (mydata, curclust);
	if (CHECK_CLUST(curclust, mydata->fatsize)) {
	    FAT_DPRINT ("curclust: 0x%x\n", curclust);
	    FAT_ERROR ("Invalid FAT entry\n");
	    return NULL;
	}
    }

    return NULL;
}


/*
 * Read boot sector and volume info from a FAT filesystem
 */
static int
read_bootsectandvi(boot_sector *bs, volume_info *volinfo, int *fatsize)
{
	__u8 block[FS_BLOCK_SIZE];
	volume_info *vistart;
	char *fstype;

	if (disk_read(0, 1, block) < 0) {
		FAT_ERROR("Error: reading block\n");
		return -1;
	}

	memcpy(bs, block, sizeof(boot_sector));
	bs->reserved	= FAT2CPU16(bs->reserved);
	bs->fat_length	= FAT2CPU16(bs->fat_length);
	bs->secs_track	= FAT2CPU16(bs->secs_track);
	bs->heads	= FAT2CPU16(bs->heads);
#if 0 /* UNUSED */
	bs->hidden	= FAT2CPU32(bs->hidden);
#endif
	bs->total_sect	= FAT2CPU32(bs->total_sect);

	/* FAT32 entries */
	if (bs->fat_length == 0) {
		/* Assume FAT32 */
		bs->fat32_length = FAT2CPU32(bs->fat32_length);
		bs->flags	 = FAT2CPU16(bs->flags);
		bs->root_cluster = FAT2CPU32(bs->root_cluster);
		bs->info_sector  = FAT2CPU16(bs->info_sector);
		bs->backup_boot  = FAT2CPU16(bs->backup_boot);
		vistart = (volume_info*) (block + sizeof(boot_sector));
		*fatsize = 32;
	} else {
		vistart = (volume_info*) &(bs->fat32_length);
		*fatsize = 0;
	}
	memcpy(volinfo, vistart, sizeof(volume_info));

	/*
	 * Terminate fs_type string. Writing past the end of vistart
	 * is ok - it's just the buffer.
	 */
	fstype = vistart->fs_type;
	fstype[8] = '\0';

	if (*fatsize == 32) {
		if (compare_sign(FAT32_SIGN, vistart->fs_type) == 0) {
			return 0;
		}
	} else {
		if (compare_sign(FAT12_SIGN, vistart->fs_type) == 0) {
			*fatsize = 12;
			return 0;
		}
		if (compare_sign(FAT16_SIGN, vistart->fs_type) == 0) {
			*fatsize = 16;
			return 0;
		}
	}

	FAT_ERROR("Error: broken fs_type sign\n");
	return -1;
}

__attribute__ ((__aligned__(__alignof__(dir_entry))))
__u8 do_fat_read_block[MAX_CLUSTSIZE];

//////////////////////////////////////////////////////////////////////

__attribute__ ((__aligned__(__alignof__(dir_entry))))
__u8 _do_fat_read_block[MAX_CLUSTSIZE];


#define FILE_MAX 1
struct _fs_info
{
    fsdata datablock;
    volume_info volinfo;
    boot_sector bs;

    char *fat_buf;
    unsigned fat_buf_cluster_index;//remember the index that which cluster was cached
};

struct file
{
    dir_entry dent;
    unsigned long offset;
    unsigned long filesize;
    __u32 curclust;//next cluster to read
    __u32 headclust;

};

static struct file files[FILE_MAX];
static struct _fs_info fs_info[FILE_MAX];
static int _fd = -1;

static int get_fd(void)
{
    if(_fd >= 0)
        return -1;

    _fd = 0;
    return _fd;
} 

static void put_fd(int fd)
{
    if(fd>=0)
        _fd = -1;
} 

/* wherehence: 0 to seek from start of file; 1 to seek from current position from file */
int do_fat_fseek(int fd, const __u64 offset, int wherehence)
{
    unsigned long curoffset;
    unsigned long offset_in_clust;
	unsigned long seeked;
    const unsigned int bytesperclust = fs_info[fd].datablock.clust_size * SECTOR_SIZE;
    const unsigned long filesize = files[fd].filesize; 
    __u32 curclust;

    if(fd<0){
        FAT_ERROR("invalid fd %d\n", fd);
        return -1;
    }

    curclust = files[fd].curclust;
    curoffset = files[fd].offset;

    if(wherehence == 0)
    {
        const unsigned long curClusterOffset = curoffset - (curoffset & (bytesperclust - 1));

        if(offset > filesize){
            FAT_ERROR("offset %llx > filesize %lx\n", offset, filesize);
            return -1;
        }

        if(offset < curoffset)//seek from head if want to seek backwards
        {
            curclust = files[fd].headclust;
            seeked=0;
        }
        else if(curClusterOffset + bytesperclust > offset)//Not need to actual seek as just in the right cluster
        {
            files[fd].offset = offset;
            return 0;
        }
        else//seek from the current cluster
        {
            seeked = curoffset - (curoffset & (bytesperclust - 1));//curclust not need to change
            FAT_MSG("Seek 0x%llx from 0x%lx\n", offset, curoffset);
        }

        /* seek to offset */
        while(1) 
        {

            if(seeked + bytesperclust > offset)
            {
                fsdata* mydata = &fs_info[fd].datablock;
                __u8*   clusterCache = (__u8*)fs_info[fd].fat_buf;

                files[fd].curclust = curclust;
                files[fd].offset = offset;

                if((offset & (bytesperclust - 1)) && curclust != fs_info[fd].fat_buf_cluster_index)
                {//cache the cluster if want to read from the offset where not align in cluster, and not cached yet
                    if (get_cluster(mydata, curclust, clusterCache, (int)bytesperclust) != 0) {
                        FAT_ERROR("Error reading cluster\n");
                        return -1;
                    }
                    fs_info[fd].fat_buf_cluster_index = curclust;
                }

                break;
            }

            curclust = get_fatent(&fs_info[fd].datablock, curclust);

            seeked += bytesperclust;
        }
    }
    else if(wherehence == 1)//this branch not used and not checked!
    {
        if(offset + curoffset > filesize){
            DWN_ERR("offset 0x%llx + curoffset 0x%lx > filesize 0x%lx\n", offset, curoffset, filesize);
            return __LINE__;
        }
        if(offset == 0)
            return 0;

        curclust = files[fd].curclust;

        seeked=0;
        /* seek to offset */

        offset_in_clust = curoffset % bytesperclust;
        
        if(offset_in_clust + offset <= bytesperclust)
        {
            files[fd].offset += offset; 
            return 0;
        }
        else
        {
            //round down to cluster boundry.
            const __u64 aimOffset = offset + offset_in_clust;
            /*offset += offset_in_clust;*/

            while(1) {

                if(seeked + bytesperclust > aimOffset)
                {
                    files[fd].curclust = curclust;
                    files[fd].offset = aimOffset;
                    break;
                }

                curclust = get_fatent(&fs_info[fd].datablock, curclust);

                seeked += bytesperclust;
            }
        }

    }

    return 0;
}

long do_fat_fopen(const char *filename)
{
#if CONFIG_NIOS /* NIOS CPU cannot access big automatic arrays */
    static
#endif
    unsigned int bytesperclust;
    char fnamecopy[2048];
    fsdata *mydata;
    int fd;
    dir_entry *dentptr;
    char *subname = "";
    int rootdir_size, cursect;
    int idx, isdir = 0;
    int firsttime;

    if((fd = get_fd()) < 0) {
        FAT_ERROR("get_fd failed\n");
        return -1;
    }
   
    mydata = &fs_info[fd].datablock;

    if (read_bootsectandvi (&fs_info[fd].bs, &fs_info[fd].volinfo, &mydata->fatsize)) {
        FAT_ERROR ("Error: reading boot sector\n");
        put_fd(fd);
        return -1;
    }
    if (mydata->fatsize == 32) {
        mydata->fatlength = fs_info[fd].bs.fat32_length;
    } else {
        mydata->fatlength = fs_info[fd].bs.fat_length;
    }
    mydata->fat_sect = fs_info[fd].bs.reserved;
    cursect = mydata->rootdir_sect
	    = mydata->fat_sect + mydata->fatlength * fs_info[fd].bs.fats;
    mydata->clust_size = fs_info[fd].bs.cluster_size;
    if (mydata->fatsize == 32) {
	rootdir_size = mydata->clust_size;
	mydata->data_begin = mydata->rootdir_sect   /* + rootdir_size */
		- (mydata->clust_size * 2);
    } else {
	rootdir_size = ((fs_info[fd].bs.dir_entries[1] * (int) 256 + fs_info[fd].bs.dir_entries[0])
			* sizeof (dir_entry)) / SECTOR_SIZE;
	mydata->data_begin = mydata->rootdir_sect + rootdir_size
		- (mydata->clust_size * 2);
    }
    mydata->fatbufnum = -1;

    FAT_DPRINT ("FAT%d, fatlength: %d\n", mydata->fatsize,
		mydata->fatlength);
    FAT_DPRINT ("Rootdir begins at sector: %d, offset: %x, size: %d\n"
		"Data begins at: %d\n",
		mydata->rootdir_sect, mydata->rootdir_sect * SECTOR_SIZE,
		rootdir_size, mydata->data_begin);
    FAT_DPRINT ("Cluster size: %d\n", mydata->clust_size);

    /* "cwd" is always the root... */
    while (ISDIRDELIM (*filename))
	filename++;
    /* Make a copy of the filename and convert it to lowercase */
    strcpy (fnamecopy, filename);
    downcase (fnamecopy);
    if (*fnamecopy == '\0') {
        put_fd(fd);        
	    return -1;
    } else if ((idx = dirdelim (fnamecopy)) >= 0) {
	isdir = 1;
	fnamecopy[idx] = '\0';
	subname = fnamecopy + idx + 1;
	/* Handle multiple delimiters */
	while (ISDIRDELIM (*subname))
	    subname++;
    }

    while (1) {
	int i;

	if (v2_ext_mmc_read(cursect, mydata->clust_size, _do_fat_read_block) < 0) {
	    FAT_ERROR ("Error: reading rootdir block\n");
        put_fd(fd);        
	    return -1;
	}
	dentptr = (dir_entry *) _do_fat_read_block;
	for (i = 0; i < DIRENTSPERBLOCK; i++) {
	    char s_name[14], l_name[256];

	    l_name[0] = '\0';
	    if ((dentptr->attr & ATTR_VOLUME)) {
#ifdef CONFIG_SUPPORT_VFAT
		if (((dentptr->attr & ATTR_VFAT) == ATTR_VFAT) &&
		    (dentptr->name[0] & LAST_LONG_ENTRY_MASK)) {
		    get_vfatname (mydata, 0, _do_fat_read_block, dentptr, l_name);
		} else
#endif
		{
		    /* Volume label or VFAT entry */
		    dentptr++;
		    continue;
		}
	    } else if (dentptr->name[0] == 0) {
		FAT_DPRINT ("RootDentname == NULL - %d\n", i);
        put_fd(fd);        
		return -1;
	    }

	    get_name (dentptr, s_name);

	    if (strcmp (fnamecopy, s_name) && strcmp (fnamecopy, l_name)) {
		FAT_DPRINT ("RootMismatch: |%s|%s|\n", s_name, l_name);
		dentptr++;
		continue;
	    }
	    if (isdir && !(dentptr->attr & ATTR_DIR))
        {
            put_fd(fd);        
            return -1;
        }
	    FAT_DPRINT ("RootName: %s", s_name);
	    FAT_DPRINT (", start: 0x%x", START (dentptr));
	    FAT_DPRINT (", size:  0x%x %s\n",
			FAT2CPU32 (dentptr->size), isdir ? "(DIR)" : "");

	    goto rootdir_done;  /* We got a match */
	}
	cursect++;
    }
  rootdir_done:

    firsttime = 1;
    while (isdir) {
	int startsect = mydata->data_begin
		+ START (dentptr) * mydata->clust_size;
	dir_entry dent;
	char *nextname = NULL;

	dent = *dentptr;
	dentptr = &dent;

	idx = dirdelim (subname);
	if (idx >= 0) {
	    subname[idx] = '\0';
	    nextname = subname + idx + 1;
	    /* Handle multiple delimiters */
	    while (ISDIRDELIM (*nextname))
		nextname++;
	} else {
		isdir = 0;
	}

	if (get_dentfromdir (mydata, startsect, subname, dentptr, 0) == NULL) {
        put_fd(fd);        
	    return -1;
	}

	if (idx >= 0) {
	    if (!(dentptr->attr & ATTR_DIR))
        {
            put_fd(fd);        
            return -1;
        }
	    subname = nextname;
	}
    }

    files[fd].dent = *dentptr;
    files[fd].offset = 0;
	files[fd].curclust = files[fd].headclust = START(dentptr);
	files[fd].filesize = FAT2CPU32(dentptr->size);
    FAT_MSG("Filesize is 0x%lxB[%luM]\n", files[fd].filesize, (files[fd].filesize>>20));

	bytesperclust = fs_info[fd].datablock.clust_size * SECTOR_SIZE;
    fs_info[fd].fat_buf_cluster_index = 0;//0 is invalid
    fs_info[fd].fat_buf = malloc(bytesperclust);
    if(!fs_info[fd].fat_buf)
    {
        if(fd>=0)
        {
            memset(&files[fd], 0, sizeof(struct file));
            memset(&fs_info[fd], 0, sizeof(struct _fs_info));
        }
        put_fd(fd);
       return -1; 
    }
    return fd;
}

unsigned do_fat_get_bytesperclust(int fd)
{ 
	const unsigned bytesperclust = fs_info[fd].datablock.clust_size * SECTOR_SIZE;

    if(fd < 0){
        FAT_ERROR("Invalid fd %d\n", fd);
        return -1;
    }

    return bytesperclust;
}

long do_fat_fread(int fd, __u8 *buffer, unsigned long maxsize)
{
    __u32 gotsize = 0;
    __u32 curclust;
	unsigned long actsize;
    unsigned long offset;
    unsigned long offset_in_clust;
    struct _fs_info* theFsInfo = fs_info + fd;
	const unsigned bytesperclust = theFsInfo->datablock.clust_size * SECTOR_SIZE;
    fsdata* mydata = &fs_info[fd].datablock;

    if(fd < 0){
        FAT_ERROR("Invalid fd %d\n", fd);
        return -1;
    }
  
    offset = files[fd].offset;



#if 0
	seeked=0;
  
    /* seek to offset */
    while(1) {

        if(seeked + bytesperclust > offset)
        {
            printf("Seeked to %d, target offset %d, clust %d\n", seeked, offset, curclust);
            break;
        }

        curclust = get_fatent(&fs_info[fd].datablock, curclust);

        if (CHECK_CLUST(curclust, mydata->fatsize)) {
            FAT_DPRINT("curclust: 0x%x\n", curclust);
            FAT_DPRINT("Invalid FAT entry\n");
            return 0;
        }

        seeked += bytesperclust;
    }
#else

    curclust = files[fd].curclust;

#endif

    /* calc actual size to read */
    if(offset + maxsize > files[fd].filesize)
    {
        actsize = files[fd].filesize - offset;
    }
    else
    {
        actsize = maxsize;
    }

    /* Deal with partial data at the first cluster */

    /* Data occupation in cluster 1
    Case 1:

    cluster1 :   |####____|
    
    Case 2:
    cluster1 :   |########|
    
    Case 3:
    cluster1 :   |__####__|

    Case 4:
    cluster1 :   |__######|
    */ 

    offset_in_clust = (offset % bytesperclust);
    if(offset_in_clust != 0) 
    { 
        if(curclust != theFsInfo->fat_buf_cluster_index)
        {//should seldom reach here if the image item consecutive
            FAT_MSG("offset_in_clust 0x%lx\n", offset_in_clust);
            /* Use vfat_buf as intermedia buffer to deal with the situation that size of _buf_ is smaller than a cluster */
            //FIXME:check if the fat_buf cache the data user wanted, and copy directly if wanted!!
            if (get_cluster(mydata, curclust, (__u8*)fs_info[fd].fat_buf, (int)bytesperclust) != 0) {
                FAT_ERROR("Error reading cluster\n");
                return -1;
            }
            fs_info[fd].fat_buf_cluster_index = curclust;
        }

        if(actsize < (bytesperclust - offset_in_clust))
        {/* Case 3: the end of the cluster is not reached */
            memcpy(buffer, fs_info[fd].fat_buf+ offset_in_clust, actsize);
            offset += actsize;
            gotsize = actsize;
            goto exit;
        }
        else
        {/* Case 4 */
            memcpy(buffer, fs_info[fd].fat_buf+ offset_in_clust, bytesperclust - offset_in_clust);
            actsize -= (bytesperclust - offset_in_clust);
            gotsize = (bytesperclust - offset_in_clust);
            offset += (bytesperclust - offset_in_clust);
            buffer += gotsize;

            //update cluster index if seeked to next cluster
            curclust = get_fatent(mydata, curclust);//get next cluster index
            if (CHECK_CLUST(curclust, mydata->fatsize)) {
                FAT_ERROR("curclust: 0x%x\n", curclust);
                FAT_ERROR("Invalid FAT entry\n");
                goto exit;
            }
        }
    }

    //Following disposing data which 'offset % bytesperclust == 0', that is start from align offset
    while(actsize)//left data length
    {
        __u32 endclust; //last cluster index of this consecutive clusters
        __u32 newclust = 0; //new cluster index of next consecutive clusters
        unsigned thisConsecutiveLen = 0;

        endclust = curclust;
        /* search for consecutive clusters until get enghou size*/
        while(actsize >= bytesperclust)
        {
            newclust = get_fatent(mydata, endclust);//get next cluster index
            if (CHECK_CLUST(newclust, mydata->fatsize)) {
                FAT_ERROR("curclust: 0x%x\n", newclust);
                FAT_ERROR("Invalid FAT entry\n");
                goto exit;
            }
            thisConsecutiveLen+= bytesperclust;
            actsize     -= bytesperclust;
            
            //clusters not consecutive
            if((newclust -1)!= endclust) break;
            endclust     =newclust;//update curclust for next read
		}
        if(thisConsecutiveLen)
        {
            if (get_cluster(mydata, curclust, buffer, thisConsecutiveLen) != 0) {
                FAT_ERROR("Error reading cluster\n");
                return -1;
            }
            buffer  += thisConsecutiveLen;
            gotsize += thisConsecutiveLen;
            offset  += thisConsecutiveLen;
            curclust = newclust;
            /*DWN_DBG("thisConsecutiveLen 0x%x\n", thisConsecutiveLen);*/
        }
        //data not enough
        if(actsize < bytesperclust && actsize)//Left data in the 'next' cluster < bytesperclust
        {
            __u8*   clusterCache = (__u8*)theFsInfo->fat_buf;

            if (get_cluster(mydata, curclust, clusterCache, bytesperclust) != 0) {
                FAT_ERROR("Error reading cluster\n");
                return -1;
            }
            theFsInfo->fat_buf_cluster_index = curclust;
            memcpy(buffer, clusterCache, actsize);

            gotsize += actsize;
            offset  += actsize;

            //If this message printed when burning, we saied the bootloader is above data parts in image.cfg, or 
            //sdc_burn not burn data partitions in the order in image
            FAT_MSG("sz 0x%x gz 0x%x, bps 0x%x\n", (unsigned)actsize, gotsize, bytesperclust);//bpc:bytesperclust
            actsize  = 0;///end the loop
        }
    }

exit:
    
    files[fd].offset = offset;
    files[fd].curclust = curclust;
    return gotsize;
}

void
do_fat_fclose(int fd)
{

    if(fd>=0)
    {
        memset(&files[fd], 0, sizeof(struct file));
        memset(&fs_info[fd], 0, sizeof(struct _fs_info));
    }

    if(fs_info[fd].fat_buf)
    {
        free(fs_info[fd].fat_buf);
        fs_info[fd].fat_buf=0;
    }

    put_fd(fd);
}

// added by scy
//if image not exist, return 0
s64 do_fat_get_fileSz(const char* imgItemPath)
{
    char cmdBuf[256] = "";
    int rcode = 0;
    const char* envFileSz = NULL; 
    const char* usb_update = getenv("usb_update");

	if(!strcmp(usb_update,"1"))
    {
    	//fatexist usb host 0 imgItemPath
    	sprintf(cmdBuf, "fatexist usb 0 %s", imgItemPath);
    }
    else
    {
	sprintf(cmdBuf, "fatexist mmc 0 %s", imgItemPath);
    }
	/*SDC_DBG("to run cmd [%s]\n", cmdBuf);*/
    rcode = run_command(cmdBuf, 0);
    if(rcode){
        printf("fail in cmd [%s], rcode %d\n", cmdBuf, rcode);
        return 0;//item size is 0
    }
    envFileSz = getenv("filesize");
    /*SDC_DBG("size of item %s is 0x%s\n", imgItemPath, envFileSz);*/

    return simple_strtoull(envFileSz, NULL, 16);
}

//<0 if failed, 0 is normal, 1 is sparse, others reserved
int do_fat_get_file_format(const char* imgFilePath, unsigned char* pbuf, const unsigned bufSz)
{
    int readSz = 0;

    int hFile = do_fat_fopen(imgFilePath);
    if(hFile < 0){
        printf("Fail to open file (%s)\n", imgFilePath);
        return -1;
    }

    readSz = do_fat_fread(hFile, pbuf, bufSz);
    if(readSz <= 0){
        printf("Fail to read file(%s), readSz=%d\n", imgFilePath, readSz);
        do_fat_fclose(hFile);
        return -1;
    }

    readSz = optimus_simg_probe(pbuf, readSz);

    do_fat_fclose(hFile);

    return readSz;
}

