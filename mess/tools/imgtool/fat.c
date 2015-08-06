/****************************************************************************

	fat.c

	PC FAT disk images

*****************************************************************************

  Master boot record format:

  Offset  Length  Description
  ------  ------  -----------
       0     446  Boot machine code
     446      16  Partion #1 info
     462      16  Partion #2 info
     478      16  Partion #3 info
     494      16  Partion #4 info
	 510       2  Magic bytes (0x55 0xAA)


  Partition info format:

  Offset  Length  Description
  ------  ------  -----------
       0       1  Active byte (0x80=active 0x00=inactive)
	   1       1  Starting head
	   2       1  Starting sector (bits 5-0) and high bits of starting track (bits 6-5)
	   3       1  Low bits of starting track
	   4       1  Partition type:
				       0x00		Unused
					   0x?1		FAT12	(0-15 MB)
					   0x?2		XENIX
					   0x?4		FAT16	(16-32 MB)
					   0x?6		FAT16`	(32 MB-2 GB)
					   0x?7		HPFS or NTFS
					   0x?A		Boot Manager
					   0x?B		FAT32	(512 MB-2 TB)
					   0x?C		FAT32	(512 MB-2 TB LBA)
					   0x1?		OS/2 Boot manager/Win95 hidden
					   0xC?		DR-DOS secured partition
					   0xD?		Multiuser DOS secured partition
					   0xE?		SpeedStor extended partition
	   5       1  Ending head
	   6       1  Ending sector (bits 5-0) and high bits of ending track (bits 6-5)
	   7       1  Low bits of ending track
	   8       4  Sector index of beginning of partition
	  12       4  Total sectors in partition


  Boot sector format:

  Offset  Length  Description
  ------  ------  -----------
       0       3  Jump instruction (to skip over header on boot)
	   3       8  OEM Name
      11       2  Bytes per sector
	  13       1  Sectors per cluster
	  14       2  Reserved sector count (including boot sector)
	  16       1  Number of FATs (file allocation tables)
	  17       2  Number of root directory entries
	  19       2  Total sectors (bits 0-15)
	  21       1  Media descriptor
	  22       2  Sectors per FAT
	  24       2  Sectors per track
	  26       2  Number of heads
	  28       4  Hidden sectors
	  32       4  Total sectors (bits 16-47)
	  36       1  Physical drive number
      37       1  Current head
	  38       1  Signature
	  39       4  ID
	  43      11  Volume Label
	  54       8  FAT file system type 
	  62     448  Boot machine code
	 510       2  Magic bytes (0x55 0xAA)

  For more information:
	http://support.microsoft.com/kb/q140418/


  Directory Entry Format:

  Offset  Length  Description
  ------  ------  -----------
       0       8  DOS File Name (padded with spaces)
	   8       3  DOS File Extension (padded with spaces)
	  11       1  File Attributes
	  12       2  Unknown
	  14       4  Time of Creation
	  18       2  Last Access Time
	  20       2  EA-Index (OS/2 stuff)
	  22       4  Last Modified Time
	  26       2  First Cluster
	  28       4  File Size


  Dates and times are stored in separate words; when together, the time is
  first and the date is second.

	Time:
		bits 15-11		Hour
		bits 10- 5		Minute
		bits  4- 0		Second / 2

	Date:
		bits 15- 9		Year - 1980
		bits  8- 5		Month
		bits  4- 0		Day

  LFN Entry Format:

  Offset  Length  Description
  ------  ------  -----------
       0       1  Sequence Number (bit 6 is set on highest sequence)
	   1      10  Name characters (five UTF-16LE chars)
	  11       1  Attributes (always 0x0F)
	  12       1  Reserved (always 0x00)
	  13       1  Checksum of short filename entry
	  14      12  Name characters (six UTF-16LE chars)
	  26       2  Entry Cluster (always 0x00)
	  28       4  Name characters (two UTF-16LE chars)
  
  Valid characters in DOS file names:
	- Upper case letters A-Z
	- Numbers 0-9
	- Space (though there is no way to identify a trailing space)
	- ! # $ % & ( ) - @ ^ _ ` { } ~ 
	- Characters 128-255 (though the code page is indeterminate)

  For more information:
	http://en.wikipedia.org/wiki/File_Allocation_Table

****************************************************************************/

#include <time.h>
#include <ctype.h>

#include "imgtool.h"
#include "formats/pc_dsk.h"
#include "iflopimg.h"
#include "unicode.h"
#include "imghd.h"

#define FAT_DIRENT_SIZE			32
#define FAT_SECLEN				512

#define LOG(x)

struct fat_diskinfo
{
	UINT32 fat_bits;
	UINT32 sectors_per_cluster;
	UINT32 cluster_size;
	UINT32 reserved_sectors;
	UINT32 fat_count;
	UINT32 root_entries;
	UINT32 sectors_per_fat;
	UINT32 sectors_per_track;
	UINT32 heads;
	UINT64 total_sectors;
	UINT32 total_clusters;
	UINT32 partition_sector_index;
	struct mess_hard_disk_file harddisk;
};

struct fat_partitiontableinfo
{
	UINT32 heads;
	UINT32 sectors;

	struct
	{
		unsigned int active : 1;
		unsigned int is_fat : 1;
		unsigned int corrupt : 1;
		UINT32 fat_bits;
		UINT32 starting_track;
		UINT32 starting_head;
		UINT32 starting_sector;
		UINT32 ending_track;
		UINT32 ending_head;
		UINT32 ending_sector;
		UINT32 sector_index;
		UINT32 total_sectors;
	} partitions[4];
};

struct fat_file
{
	unsigned int root : 1;
	unsigned int directory : 1;
	unsigned int eof : 1;
	UINT32 index;
	UINT32 filesize;
	UINT32 first_cluster;
	UINT32 parent_first_cluster;
	UINT32 cluster;
	UINT32 cluster_index;
	UINT32 dirent_sector_index;
	UINT32 dirent_sector_offset;
};

struct fat_dirent
{
	char long_filename[512];
	char short_filename[13];
	unsigned int directory : 1;
	unsigned int eof : 1;
	UINT32 filesize;
	UINT32 first_cluster;
	UINT32 dirent_sector_index;
	UINT32 dirent_sector_offset;
	time_t creation_time;
	time_t lastmodified_time;
};

struct fat_freeentry_info
{
	UINT32 required_size;
	UINT32 candidate_position;
	UINT32 position;
};

struct fat_mediatype
{
	UINT8 media_descriptor;
	UINT8 heads;
	UINT8 tracks;
	UINT8 sectors;
};

typedef enum
{
	CREATE_NONE,
	CREATE_FILE,
	CREATE_DIR
} creation_policy_t;



static struct fat_mediatype known_media[] =
{
	{ 0xF0, 2, 80, 36 },
	{ 0xF0, 2, 80, 18 },
	{ 0xF9, 2, 80,  9 },
	{ 0xF9, 2, 80, 15 },
	{ 0xFD, 2, 40,  9 },
	{ 0xFF, 2, 40,  8 },
	{ 0xFC, 1, 40,  9 },
	{ 0xFE, 1, 40,  8 },
	{ 0xF8, 0,  0,  0 }
};

/* boot sector code taken from FreeDOS */
static const UINT8 boot_sector_code[] =
{
	0xfa, 0xfc, 0x31, 0xc0, 0x8e, 0xd8, 0xbd, 0x00, 0x7c, 0xb8, 0xe0, 0x1f, 
	0x8e, 0xc0, 0x89, 0xee, 0x89, 0xef, 0xb9, 0x00, 0x01, 0xf3, 0xa5, 0xea, 
	0x5e, 0x7c, 0xe0, 0x1f, 0x00, 0x00, 0x60, 0x00, 0x8e, 0xd8, 0x8e, 0xd0, 
	0x8d, 0x66, 0xa0, 0xfb, 0x80, 0x7e, 0x24, 0xff, 0x75, 0x03, 0x88, 0x56, 
	0x24, 0xc7, 0x46, 0xc0, 0x10, 0x00, 0xc7, 0x46, 0xc2, 0x01, 0x00, 0x8b, 
	0x76, 0x1c, 0x8b, 0x7e, 0x1e, 0x03, 0x76, 0x0e, 0x83, 0xd7, 0x00, 0x89, 
	0x76, 0xd2, 0x89, 0x7e, 0xd4, 0x8a, 0x46, 0x10, 0x98, 0xf7, 0x66, 0x16, 
	0x01, 0xc6, 0x11, 0xd7, 0x89, 0x76, 0xd6, 0x89, 0x7e, 0xd8, 0x8b, 0x5e, 
	0x0b, 0xb1, 0x05, 0xd3, 0xeb, 0x8b, 0x46, 0x11, 0x31, 0xd2, 0xf7, 0xf3, 
	0x89, 0x46, 0xd0, 0x01, 0xc6, 0x83, 0xd7, 0x00, 0x89, 0x76, 0xda, 0x89, 
	0x7e, 0xdc, 0x8b, 0x46, 0xd6, 0x8b, 0x56, 0xd8, 0x8b, 0x7e, 0xd0, 0xc4, 
	0x5e, 0x5a, 0xe8, 0xac, 0x00, 0xc4, 0x7e, 0x5a, 0xb9, 0x0b, 0x00, 0xbe, 
	0xf1, 0x7d, 0x57, 0xf3, 0xa6, 0x5f, 0x26, 0x8b, 0x45, 0x1a, 0x74, 0x0b, 
	0x83, 0xc7, 0x20, 0x26, 0x80, 0x3d, 0x00, 0x75, 0xe7, 0x72, 0x68, 0x50, 
	0xc4, 0x5e, 0x5a, 0x8b, 0x7e, 0x16, 0x8b, 0x46, 0xd2, 0x8b, 0x56, 0xd4, 
	0xe8, 0x7e, 0x00, 0x58, 0x1e, 0x07, 0x8e, 0x5e, 0x5c, 0xbf, 0x00, 0x20, 
	0xab, 0x89, 0xc6, 0x01, 0xf6, 0x01, 0xc6, 0xd1, 0xee, 0xad, 0x73, 0x04, 
	0xb1, 0x04, 0xd3, 0xe8, 0x80, 0xe4, 0x0f, 0x3d, 0xf8, 0x0f, 0x72, 0xe8, 
	0x31, 0xc0, 0xab, 0x0e, 0x1f, 0xc4, 0x5e, 0x5a, 0xbe, 0x00, 0x20, 0xad, 
	0x09, 0xc0, 0x75, 0x05, 0x88, 0xd3, 0xff, 0x6e, 0x5a, 0x48, 0x48, 0x8b, 
	0x7e, 0x0d, 0x81, 0xe7, 0xff, 0x00, 0xf7, 0xe7, 0x03, 0x46, 0xda, 0x13, 
	0x56, 0xdc, 0xe8, 0x34, 0x00, 0xeb, 0xe0, 0x5e, 0xac, 0x56, 0xb4, 0x0e, 
	0xcd, 0x10, 0x3c, 0x2e, 0x75, 0xf5, 0xc3, 0xe8, 0xf1, 0xff, 0x45, 0x72, 
	0x72, 0x6f, 0x72, 0x21, 0x20, 0x48, 0x69, 0x74, 0x20, 0x61, 0x20, 0x6b, 
	0x65, 0x79, 0x20, 0x74, 0x6f, 0x20, 0x72, 0x65, 0x62, 0x6f, 0x6f, 0x74, 
	0x2e, 0x30, 0xe4, 0xcd, 0x13, 0xcd, 0x16, 0xcd, 0x19, 0x56, 0x89, 0x46, 
	0xc8, 0x89, 0x56, 0xca, 0x8c, 0x46, 0xc6, 0x89, 0x5e, 0xc4, 0xe8, 0xbe, 
	0xff, 0x2e, 0xb4, 0x41, 0xbb, 0xaa, 0x55, 0x8a, 0x56, 0x24, 0x84, 0xd2, 
	0x74, 0x19, 0xcd, 0x13, 0x72, 0x15, 0xd1, 0xe9, 0x81, 0xdb, 0x54, 0xaa, 
	0x75, 0x0d, 0x8d, 0x76, 0xc0, 0x89, 0x5e, 0xcc, 0x89, 0x5e, 0xce, 0xb4, 
	0x42, 0xeb, 0x26, 0x8b, 0x4e, 0xc8, 0x8b, 0x56, 0xca, 0x8a, 0x46, 0x18, 
	0xf6, 0x66, 0x1a, 0x91, 0xf7, 0xf1, 0x92, 0xf6, 0x76, 0x18, 0x89, 0xd1, 
	0x88, 0xc6, 0x86, 0xe9, 0xd0, 0xc9, 0xd0, 0xc9, 0x08, 0xe1, 0x41, 0xc4, 
	0x5e, 0xc4, 0xb8, 0x01, 0x02, 0x8a, 0x56, 0x24, 0xcd, 0x13, 0x0f, 0x82, 
	0x75, 0xff, 0x8b, 0x46, 0x0b, 0xf6, 0x76, 0xc0, 0x01, 0x46, 0xc6, 0x83, 
	0x46, 0xc8, 0x01, 0x83, 0x56, 0xca, 0x00, 0x4f, 0x75, 0x98, 0x8e, 0x46, 
	0xc6, 0x5e, 0xc3, 0x4d, 0x45, 0x54, 0x41, 0x4b, 0x45, 0x52, 0x4e, 0x53, 
	0x59, 0x53, 0x00, 0x00, 0x55, 0xaa
};



static int fat_is_harddisk(imgtool_image *image)
{
	return !strcmp(img_module(image)->name, "pc_chd_fat");
}



static struct fat_diskinfo *fat_get_diskinfo(imgtool_image *image)
{
	void *ptr;
	if (fat_is_harddisk(image))
		ptr = img_extrabytes(image);
	else
		ptr = imgtool_floppy_extrabytes(image);
	return (struct fat_diskinfo *) ptr;
}



static void fat_get_sector_position(imgtool_image *image, UINT32 sector_index,
	int *head, int *track, int *sector)
{
	const struct fat_diskinfo *disk_info;

	disk_info = fat_get_diskinfo(image);

	if (sector_index == 0)
	{
		/* special case */
		*head = 0;
		*track = 0;
		*sector = 1;
	}
	else
	{
		*head = (sector_index / disk_info->sectors_per_track) % disk_info->heads;
		*track = sector_index / disk_info->sectors_per_track / disk_info->heads;
		*sector = 1 + (sector_index % disk_info->sectors_per_track);
	}
}



static imgtoolerr_t fat_read_sector(imgtool_image *image, UINT32 sector_index,
	int offset, void *buffer, size_t buffer_len)
{
	const struct fat_diskinfo *disk_info;
	imgtoolerr_t err;
	floperr_t ferr;
	int head, track, sector;
	UINT8 data[FAT_SECLEN];
	size_t len;

	disk_info = fat_get_diskinfo(image);
	sector_index += disk_info->partition_sector_index;

	if (fat_is_harddisk(image))
	{
		while(buffer_len > 0)
		{
			err = imghd_read(&fat_get_diskinfo(image)->harddisk, sector_index++, 1, data);
			if (err)
				return err;

			len = MIN(buffer_len, sizeof(data) - offset);
			memcpy(buffer, data + offset, len);

			buffer = ((UINT8 *) buffer) + len;
			buffer_len -= len;
			offset = 0;
		}
	}
	else
	{
		fat_get_sector_position(image, sector_index, &head, &track, &sector);
		ferr = floppy_read_sector(imgtool_floppy(image), head, track, sector, offset, buffer, buffer_len);
		if (ferr)
			return imgtool_floppy_error(ferr);
	}
	return IMGTOOLERR_SUCCESS;
}



static imgtoolerr_t fat_write_sector(imgtool_image *image, UINT32 sector_index,
	int offset, const void *buffer, size_t buffer_len)
{
	const struct fat_diskinfo *disk_info;
	imgtoolerr_t err;
	floperr_t ferr;
	int head, track, sector;
	UINT8 data[FAT_SECLEN];
	const void *write_data;
	size_t len;

	disk_info = fat_get_diskinfo(image);
	sector_index += disk_info->partition_sector_index;

	if (fat_is_harddisk(image))
	{
		while(buffer_len > 0)
		{
			len = MIN(buffer_len, sizeof(data) - offset);

			if ((offset != 0) || (buffer_len < sizeof(data)))
			{
				err = imghd_read(&fat_get_diskinfo(image)->harddisk, sector_index, 1, data);
				if (err)
					return err;
				memcpy(data + offset, buffer, len);
				write_data = data;
			}
			else
			{
				write_data = buffer;
			}

			err = imghd_write(&fat_get_diskinfo(image)->harddisk, sector_index++, 1, write_data);
			if (err)
				return err;

			buffer = ((const UINT8 *) buffer) + len;
			buffer_len -= len;
			offset = 0;
		}
	}
	else
	{
		fat_get_sector_position(image, sector_index, &head, &track, &sector);
		ferr = floppy_write_sector(imgtool_floppy(image), head, track, sector, offset, buffer, buffer_len);
		if (ferr)
			return imgtool_floppy_error(ferr);
	}
	return IMGTOOLERR_SUCCESS;
}



static imgtoolerr_t fat_clear_sector(imgtool_image *image, UINT32 sector_index, UINT8 data)
{
	char buf[FAT_SECLEN];
	memset(buf, data, sizeof(buf));
	return fat_write_sector(image, sector_index, 0, buf, sizeof(buf));
}



static imgtoolerr_t fat_get_partition_info(const UINT8 *boot_sector, struct fat_partitiontableinfo *pi)
{
	int i = 0;
	const UINT8 *partition_info;
	UINT32 heads, sectors;
	UINT32 expected_sector_index;
	UINT32 ending_sector_index;
	UINT32 expected_ending_track;
	UINT32 expected_ending_head;
	UINT32 expected_ending_sector;
	
	memset(pi, '\0', sizeof(*pi));

	/* magic bytes present? */
	if ((boot_sector[510] != 0x55) || (boot_sector[511] != 0xAA))
		return IMGTOOLERR_CORRUPTIMAGE;

	for (i = 0; i < sizeof(pi->partitions) / sizeof(pi->partitions[0]); i++)
	{
		partition_info = &boot_sector[446 + i * 16];

		pi->partitions[i].active			= (partition_info[0] & 0x80) ? 1 : 0;
		pi->partitions[i].starting_head		= partition_info[1];
		pi->partitions[i].starting_track	= ((partition_info[2] << 2) & 0xFF00) | partition_info[3];
		pi->partitions[i].starting_sector	= partition_info[2] & 0x3F;
		pi->partitions[i].ending_head		= partition_info[5];
		pi->partitions[i].ending_track		= ((partition_info[6] << 2) & 0xFF00) | partition_info[7];
		pi->partitions[i].ending_sector		= partition_info[6] & 0x3F;

		pi->partitions[i].sector_index		= pick_integer_le(partition_info,  8, 4);
		pi->partitions[i].total_sectors		= pick_integer_le(partition_info, 12, 4);

		switch(partition_info[4] & 0x0F)
		{
			case 1:
				pi->partitions[i].is_fat = 1;
				pi->partitions[i].fat_bits = 12;
				break;
			case 4:
			case 6:
				pi->partitions[i].is_fat = 1;
				pi->partitions[i].fat_bits = 16;
				break;
			case 11:
			case 12:
				pi->partitions[i].is_fat = 1;
				pi->partitions[i].fat_bits = 32;
				break;
		}
		if (pi->partitions[i].starting_track > pi->partitions[i].ending_track)
			return IMGTOOLERR_CORRUPTIMAGE;
	}

	/* based on this info, try to deduce the dimensions of this disk */
	for (sectors = 63; sectors >= 1; sectors--)
	{
		for (heads = 9; heads >= 1; heads--)
		{
			for (i = 0; i < sizeof(pi->partitions) / sizeof(pi->partitions[0]); i++)
			{
				expected_sector_index = (pi->partitions[i].starting_track * heads * sectors)
					+ (pi->partitions[i].starting_head * sectors)
					+ (pi->partitions[i].starting_sector - 1);
				if (pi->partitions[i].total_sectors == 0)
					continue;
				if (expected_sector_index != pi->partitions[i].sector_index)
					break;

				ending_sector_index = pi->partitions[i].total_sectors + pi->partitions[i].sector_index - 1;
				expected_ending_track = ending_sector_index / sectors / heads;
				expected_ending_head = (ending_sector_index / sectors) % heads;
				expected_ending_sector = (ending_sector_index % sectors) + 1;

				if (expected_ending_track != pi->partitions[i].ending_track)
					break;
				if (expected_ending_head != pi->partitions[i].ending_head)
					break;
				if (expected_ending_sector != pi->partitions[i].ending_sector)
					break;
			}
			if (i == sizeof(pi->partitions) / sizeof(pi->partitions[0]))
			{
				/* we have established the dimensions */
				pi->sectors = sectors;
				pi->heads = heads;
				return IMGTOOLERR_SUCCESS;
			}
		}
	}
	return IMGTOOLERR_CORRUPTIMAGE;
}



static imgtoolerr_t fat_diskimage_open(imgtool_image *image, imgtool_stream *stream)
{
	UINT8 header[FAT_SECLEN];
	struct fat_partitiontableinfo pi;
	imgtoolerr_t err;
	struct fat_diskinfo *info;
	UINT32 fat_bits, total_sectors_l, total_sectors_h, sector_size;
	UINT64 available_sectors;
	int i;
	int has_extended_bios_param_block = TRUE;

	info = fat_get_diskinfo(image);

	err = fat_read_sector(image, 0, 0, header, sizeof(header));
	if (err)
		return err;

	/* magic bytes present? */
	if ((header[510] != 0x55) || (header[511] != 0xAA))
		return IMGTOOLERR_CORRUPTIMAGE;

	/* is this a partitioned image? */
	err = fat_get_partition_info(header, &pi);
	if (err == IMGTOOLERR_SUCCESS)
	{
		for (i = 0; i < sizeof(pi.partitions) / sizeof(pi.partitions[0]); i++)
		{
			if (pi.partitions[i].active && pi.partitions[i].is_fat)
				break;
		}
		if (i >= sizeof(pi.partitions) / sizeof(pi.partitions[0]))
			return IMGTOOLERR_CORRUPTIMAGE;

		info->partition_sector_index = pi.partitions[i].sector_index;
		info->heads = pi.heads;
		info->sectors_per_track = pi.sectors;
		fat_bits = pi.partitions[i].fat_bits;
		
		err = fat_read_sector(image, 0, 0, header, sizeof(header));
		if (err)
			return err;
	}
	else
	{
		/* this disk is not partitioned; first step is to figure out which
		 * FAT type this is */
		if (!memcmp(&header[54], "FAT     ", 8))
			fat_bits = 8;
		else if (!memcmp(&header[54], "FAT12   ", 8))
			fat_bits = 12;
		else if (!memcmp(&header[54], "FAT16   ", 8))
			fat_bits = 16;
		else if (!memcmp(&header[54], "FAT32   ", 8))
			fat_bits = 32;
		else
		{
			fat_bits = 8;
			has_extended_bios_param_block = FALSE;
		}
	}
	
	info->fat_bits				= fat_bits;
	sector_size					= pick_integer_le(header, 11, 2);
	info->sectors_per_cluster	= pick_integer_le(header, 13, 1);
	info->reserved_sectors		= pick_integer_le(header, 14, 2);
	info->fat_count				= pick_integer_le(header, 16, 1);
	info->root_entries			= pick_integer_le(header, 17, 2);
	total_sectors_l				= pick_integer_le(header, 19, 2);
	info->sectors_per_fat		= pick_integer_le(header, 22, 2);
	info->sectors_per_track		= pick_integer_le(header, 24, 2);
	info->heads					= pick_integer_le(header, 26, 2);
	total_sectors_h				= pick_integer_le(header, 32, 4);

	info->total_sectors = total_sectors_l + (((UINT64) total_sectors_h) << 16);
	available_sectors = info->total_sectors - info->reserved_sectors
		- (info->sectors_per_fat * info->fat_count)
		- (info->root_entries * FAT_DIRENT_SIZE + FAT_SECLEN - 1) / FAT_SECLEN;
	info->total_clusters = (available_sectors + info->sectors_per_cluster - 1) / info->sectors_per_cluster;
	info->cluster_size = FAT_SECLEN * info->sectors_per_cluster;

	if (info->fat_count == 0)
		return IMGTOOLERR_CORRUPTIMAGE;
	if (sector_size != FAT_SECLEN)
		return IMGTOOLERR_CORRUPTIMAGE;
	if (info->sectors_per_fat == 0)
		return IMGTOOLERR_CORRUPTIMAGE;
	if (info->sectors_per_cluster == 0)
		return IMGTOOLERR_CORRUPTIMAGE;
	if (info->reserved_sectors == 0)
		return IMGTOOLERR_CORRUPTIMAGE;
	if (info->sectors_per_track == 0)
		return IMGTOOLERR_CORRUPTIMAGE;
	if (info->heads == 0)
		return IMGTOOLERR_CORRUPTIMAGE;
	if (info->total_sectors < info->heads * info->sectors_per_track)
		return IMGTOOLERR_CORRUPTIMAGE;
	if (info->total_clusters * info->fat_bits > info->sectors_per_fat * FAT_SECLEN * 8)
		return IMGTOOLERR_CORRUPTIMAGE;

	return IMGTOOLERR_SUCCESS;
}



static imgtoolerr_t fat_diskimage_create(imgtool_image *image, imgtool_stream *stream, option_resolution *opts)
{
	imgtoolerr_t err;
	struct fat_diskinfo *disk_info;
	UINT32 heads, tracks, sectors;
	UINT32 fat_bits, sectors_per_cluster, reserved_sectors, hidden_sectors;
	UINT32 root_dir_count, root_dir_sectors;
	UINT32 sectors_per_fat, fat_count, i;
	UINT32 boot_sector_offset;
	UINT64 total_sectors, total_clusters;
	UINT8 media_descriptor;
	const char *title;
	const char *fat_bits_string;
	UINT8 header[FAT_SECLEN];
	UINT64 first_fat_entries;

	heads = option_resolution_lookup_int(opts, 'H');
	tracks = option_resolution_lookup_int(opts, 'T');
	sectors = option_resolution_lookup_int(opts, 'S');
	
	total_sectors = ((UINT64) heads) * tracks * sectors;

	/* cap our sector count so that we only use FAT12/16 */
	sectors_per_cluster = (total_sectors + 65524 - 1) / 65524;

	/* compute the FAT file system type */
	if ((total_sectors / sectors_per_cluster) <= 4084)
	{
		fat_bits = 12;
		fat_bits_string = "FAT12   ";
	}
	else if ((total_sectors / sectors_per_cluster) <= 65524)
	{
		fat_bits = 16;
		fat_bits_string = "FAT16   ";
	}
	else
	{
		fat_bits = 32;
		fat_bits_string = "FAT32   ";
	}

	/* figure out media type */
	i = 0;
	while((known_media[i].heads > 0) && ((known_media[i].heads != heads)
		|| (known_media[i].tracks != tracks)
		|| (known_media[i].sectors != sectors)))
	{
		i++;
	}
	media_descriptor = known_media[i].media_descriptor;

	/* other miscellaneous settings */
	title = "";
	fat_count = 2;
	root_dir_count = 512;
	hidden_sectors = 0;
	reserved_sectors = 1;

	/* calculated settings */
	root_dir_sectors = (root_dir_count * FAT_DIRENT_SIZE + FAT_SECLEN - 1) / FAT_SECLEN;
	total_clusters = (total_sectors - reserved_sectors - hidden_sectors - root_dir_sectors)
		/ sectors_per_cluster;
	sectors_per_fat = (total_clusters * fat_bits + (FAT_SECLEN * 8) - 1)
		/ (FAT_SECLEN * 8);

	/* write just enough info to disk_info to get going */
	disk_info = fat_get_diskinfo(image);
	disk_info->sectors_per_track = sectors;
	disk_info->heads = heads;

	/* prepare the header */
	memset(header, 0, sizeof(header));
	memcpy(&header[3], "IMGTOOL ", 8);
	place_integer_le(header, 11, 2, FAT_SECLEN);
	place_integer_le(header, 13, 1, sectors_per_cluster);
	place_integer_le(header, 14, 1, reserved_sectors);
	place_integer_le(header, 16, 1, fat_count);
	place_integer_le(header, 17, 2, root_dir_count);
	place_integer_le(header, 19, 2, (UINT16) (total_sectors >> 0));
	place_integer_le(header, 21, 1, media_descriptor);
	place_integer_le(header, 22, 2, sectors_per_fat);
	place_integer_le(header, 24, 2, sectors);
	place_integer_le(header, 26, 2, heads);
	place_integer_le(header, 28, 4, hidden_sectors);
	place_integer_le(header, 32, 4, (UINT32) (total_sectors >> 16));
	place_integer_le(header, 36, 1, 0xFF);
	place_integer_le(header, 38, 1, 0x28);
	place_integer_le(header, 39, 4, rand());
	memcpy(&header[43], "           ", 11);
	memcpy(&header[54], fat_bits_string, 8);

	/* store boot code */
	boot_sector_offset = sizeof(header) - sizeof(boot_sector_code);
	if (boot_sector_offset < 62)
		return IMGTOOLERR_UNEXPECTED;	/* sanity check */
	if (boot_sector_offset > 510)
		return IMGTOOLERR_UNEXPECTED;	/* sanity check */
	memcpy(&header[boot_sector_offset], boot_sector_code, sizeof(boot_sector_code));

	/* specify jump instruction */
	if (boot_sector_offset <= 129)
	{
		header[0] = 0xEB;									 /* JMP rel8 */
		header[1] = (UINT8) (boot_sector_offset - 2);		 /* (offset) */
		header[2] = 0x90;									 /* NOP */
	}
	else
	{
		header[0] = 0xE9;									 /* JMP rel16 */
		header[1] = (UINT8) ((boot_sector_offset - 2) >> 0); /* (offset) */
		header[2] = (UINT8) ((boot_sector_offset - 2) >> 8); /* (offset) */
	}

	err = fat_write_sector(image, 0, 0, header, sizeof(header));
	if (err)
		return err;

	/* clear out file allocation table */
	for (i = reserved_sectors; i < (reserved_sectors + sectors_per_fat * fat_count + root_dir_sectors); i++)
	{
		err = fat_clear_sector(image, i, 0);
		if (err)
			return err;
	}

	/* set first two FAT entries */
	first_fat_entries = ((UINT64) media_descriptor) | 0xFFFFFF00;
	first_fat_entries &= (((UINT64) 1) << fat_bits) - 1;
	first_fat_entries |= ((((UINT64) 1) << fat_bits) - 1) << fat_bits;
	first_fat_entries = LITTLE_ENDIANIZE_INT64(first_fat_entries);

	for (i = 0; i < fat_count; i++)
	{
		err = fat_write_sector(image, 1 + (i * sectors_per_fat), 0, &first_fat_entries,
			fat_bits * 2 / 8);
		if (err)
			return err;
	}

	return IMGTOOLERR_SUCCESS;
}



static imgtoolerr_t fat_load_fat(imgtool_image *image, UINT8 **fat_table)
{
	imgtoolerr_t err = IMGTOOLERR_SUCCESS;
	const struct fat_diskinfo *disk_info;
	UINT8 *table;
	UINT32 table_size;
	UINT32 pos, len;
	UINT32 sector_index;

	disk_info = fat_get_diskinfo(image);

	table_size = disk_info->sectors_per_fat * disk_info->fat_count * FAT_SECLEN;

	/* allocate the table with extra bytes, in case we "overextend" our reads */
	table = malloc(table_size + sizeof(UINT64));
	if (!table)
	{
		err = IMGTOOLERR_OUTOFMEMORY;
		goto done;
	}
	memset(table, 0, table_size + sizeof(UINT64));

	pos = 0;
	sector_index = disk_info->reserved_sectors;

	while(pos < table_size)
	{
		len = MIN(table_size - pos, FAT_SECLEN);

		err = fat_read_sector(image, sector_index++, 0, &table[pos], len);
		if (err)
			goto done;

		pos += FAT_SECLEN;
	}

done:
	if (err && table)
	{
		free(table);
		table = NULL;
	}
	*fat_table = table;
	return err;
}



static imgtoolerr_t fat_save_fat(imgtool_image *image, const UINT8 *fat_table)
{
	imgtoolerr_t err = IMGTOOLERR_SUCCESS;
	const struct fat_diskinfo *disk_info;
	UINT32 table_size;
	UINT32 pos, len;
	UINT32 sector_index;

	disk_info = fat_get_diskinfo(image);

	table_size = disk_info->sectors_per_fat * disk_info->fat_count * FAT_SECLEN;

	pos = 0;
	sector_index = disk_info->reserved_sectors;

	while(pos < table_size)
	{
		len = MIN(table_size - pos, FAT_SECLEN);

		err = fat_write_sector(image, sector_index++, 0, &fat_table[pos], len);
		if (err)
			goto done;

		pos += FAT_SECLEN;
	}

done:
	return err;
}



static UINT32 fat_get_fat_entry(imgtool_image *image, const UINT8 *fat_table, UINT32 fat_entry)
{
	const struct fat_diskinfo *disk_info;
	UINT64 entry;
	UINT32 bit_index, i;
	UINT32 last_entry = 0;
	UINT32 bit_mask;

	disk_info = fat_get_diskinfo(image);
	bit_index = fat_entry * disk_info->fat_bits;
	bit_mask = 0xFFFFFFFF >> (32 - disk_info->fat_bits);

	if (fat_entry >= disk_info->total_clusters)
	{
		assert(0);
		return 1;
	}

	/* make sure that the cluster is free in all fats */
	for (i = 0; i < disk_info->fat_count; i++)
	{
		memcpy(&entry, fat_table + (i * FAT_SECLEN
			* disk_info->sectors_per_fat) + (bit_index / 8), sizeof(entry));

		/* we've extracted the bytes; we now need to normalize it */
		entry = LITTLE_ENDIANIZE_INT64(entry);
		entry >>= bit_index % 8;
		entry &= bit_mask;

		if (i == 0)
			last_entry = (UINT32) entry;
		else if (last_entry != (UINT32) entry)
			return 1;	/* if the FATs disagree; mark this as reserved */
	}
	
	/* normalize special clusters */
	if (last_entry >= (0xFFFFFFF0 & bit_mask))
	{
		last_entry |= 0xFFFFFFF0;
		if (last_entry >= 0xFFFFFFF8)
			last_entry = 0xFFFFFFFF;
	}
	return last_entry;
}



static void fat_set_fat_entry(imgtool_image *image, UINT8 *fat_table, UINT32 fat_entry, UINT32 value)
{
	const struct fat_diskinfo *disk_info;
	UINT64 entry;
	UINT32 bit_index, i;

	disk_info = fat_get_diskinfo(image);
	bit_index = fat_entry * disk_info->fat_bits;
	value &= 0xFFFFFFFF >> (32 - disk_info->fat_bits);

	for (i = 0; i < disk_info->fat_count; i++)
	{
		memcpy(&entry, fat_table + (i * FAT_SECLEN
			* disk_info->sectors_per_fat) + (bit_index / 8), sizeof(entry));

		entry = LITTLE_ENDIANIZE_INT64(entry);
		entry &= (~((UINT64) 0xFFFFFFFF >> (32 - disk_info->fat_bits)) << (bit_index % 8)) | ((1 << (bit_index % 8)) - 1);
		entry |= ((UINT64) value) << (bit_index % 8);
		entry = LITTLE_ENDIANIZE_INT64(entry);

		memcpy(fat_table + (i * FAT_SECLEN
			* disk_info->sectors_per_fat) + (bit_index / 8), &entry, sizeof(entry));
	}
}



static void fat_debug_integrity_check(imgtool_image *image, const UINT8 *fat_table, const struct fat_file *file)
{
#ifdef MAME_DEBUG
	/* debug function to test the integrity of a file */
	UINT32 cluster;
	const struct fat_diskinfo *disk_info;

	disk_info = fat_get_diskinfo(image);
	cluster = file->first_cluster ? file->first_cluster : 0xFFFFFFFF;

	if (!file->root)
	{
		while(cluster != 0xFFFFFFFF)
		{
			assert((cluster >= 2) && (cluster < disk_info->total_clusters));
			cluster = fat_get_fat_entry(image, fat_table, cluster);
		}
	}
#endif
}



static imgtoolerr_t fat_seek_file(imgtool_image *image, struct fat_file *file, UINT32 pos)
{
	imgtoolerr_t err = IMGTOOLERR_SUCCESS;
	const struct fat_diskinfo *disk_info;
	UINT32 new_cluster;
	UINT8 *fat_table = NULL;

	disk_info = fat_get_diskinfo(image);

	/* can't seek past end of file */
	if (!file->directory && (pos > file->filesize))
		pos = file->filesize;

	if (file->first_cluster == 0)
	{
		/* special case; the root directory */
		file->index = pos;
	}
	else
	{
		/* first, we need to check to see if we have to go back to the beginning */
		if (pos < file->index)
		{
			file->cluster = file->first_cluster;
			file->cluster_index = 0;
			file->eof = 0;
		}

		/* skip ahead clusters */
		while((file->cluster_index + disk_info->cluster_size) <= pos)
		{
			if (!fat_table)
			{
				err = fat_load_fat(image, &fat_table);
				if (err)
					goto done;
			}

			new_cluster = fat_get_fat_entry(image, fat_table, file->cluster);

			file->cluster = new_cluster;
			file->cluster_index += disk_info->cluster_size;

			/* are we at the end of the file? */
			if (new_cluster == 0xFFFFFFFF)
			{
				pos = file->cluster_index;
				file->eof = 1;
			}
		}
		file->index = pos;
	}

done:
	if (fat_table)
		free(fat_table);
	return err;
}



static UINT32 fat_get_filepos_sector_index(imgtool_image *image, struct fat_file *file)
{
	UINT32 sector_index;
	const struct fat_diskinfo *disk_info;

	disk_info = fat_get_diskinfo(image);

	sector_index = disk_info->reserved_sectors + (disk_info->sectors_per_fat * disk_info->fat_count);
	if (file->root)
	{
		/* special case for the root file */
		sector_index += file->index / FAT_SECLEN;
	}
	else
	{
		/* cluster out of range? */
		if ((file->cluster < 2) || (file->cluster >= disk_info->total_clusters))
			return 0;

		sector_index += (disk_info->root_entries * FAT_DIRENT_SIZE + FAT_SECLEN - 1) / FAT_SECLEN;
		sector_index += (file->cluster - 2) * disk_info->sectors_per_cluster;
		sector_index += (file->index / FAT_SECLEN) % disk_info->sectors_per_cluster;
	}
	return sector_index;
}



static imgtoolerr_t fat_corrupt_file_error(const struct fat_file *file)
{
	imgtoolerr_t err;
	if (file->root)
		err = IMGTOOLERR_CORRUPTIMAGE;
	else if (file->directory)
		err = IMGTOOLERR_CORRUPTDIR;
	else
		err = IMGTOOLERR_CORRUPTFILE;
	return err;
}



static imgtoolerr_t fat_readwrite_file(imgtool_image *image, struct fat_file *file,
	void *buffer, size_t buffer_len, size_t *bytes_read, int read_or_write)
{
	imgtoolerr_t err;
	const struct fat_diskinfo *disk_info;
	UINT32 sector_index;
	int offset;
	size_t len;

	disk_info = fat_get_diskinfo(image);
	if (bytes_read)
		*bytes_read = 0;
	if (!file->directory)
		buffer_len = MIN(buffer_len, file->filesize - file->index);

	while(!file->eof && (buffer_len > 0))
	{
		sector_index = fat_get_filepos_sector_index(image, file);
		if (sector_index == 0)
			return fat_corrupt_file_error(file);

		offset = file->index % FAT_SECLEN;
		len = MIN(buffer_len, FAT_SECLEN - offset);

		/* read or write the data from the disk */
		if (read_or_write)
			err = fat_write_sector(image, sector_index, offset, buffer, len);
		else
			err = fat_read_sector(image, sector_index, offset, buffer, len);
		if (err)
			return err;

		/* and move the file pointer ahead */
		err = fat_seek_file(image, file, file->index + len);
		if (err)
			return err;

		buffer = ((UINT8 *) buffer) + len;
		buffer_len -= len;
		if (bytes_read)
			*bytes_read += len;
	}
	return IMGTOOLERR_SUCCESS;
}



static imgtoolerr_t fat_read_file(imgtool_image *image, struct fat_file *file,
	void *buffer, size_t buffer_len, size_t *bytes_read)
{
	return fat_readwrite_file(image, file, buffer, buffer_len, bytes_read, 0);
}



static imgtoolerr_t fat_write_file(imgtool_image *image, struct fat_file *file,
	const void *buffer, size_t buffer_len, size_t *bytes_read)
{
	return fat_readwrite_file(image, file, (void *) buffer, buffer_len, bytes_read, 1);
}



static UINT32 fat_allocate_cluster(imgtool_image *image, UINT8 *fat_table)
{
	const struct fat_diskinfo *disk_info;
	UINT32 i, val;

	disk_info = fat_get_diskinfo(image);

	for (i = 2; i < disk_info->total_clusters; i++)
	{
		val = fat_get_fat_entry(image, fat_table, i);
		if (val == 0)
		{
			fat_set_fat_entry(image, fat_table, i, 1);
			return i;
		}
	}
	return 0;
}



/* sets the size of a file; 0xFFFFFFFF means 'delete' */
static imgtoolerr_t fat_set_file_size(imgtool_image *image, struct fat_file *file,
	UINT32 new_size)
{
	imgtoolerr_t err = IMGTOOLERR_SUCCESS;
	const struct fat_diskinfo *disk_info;
	UINT32 new_cluster_count;
	UINT32 old_cluster_count;
	UINT32 cluster, write_cluster, last_cluster, new_pos, i;
	UINT8 *fat_table = NULL;
	UINT8 dirent[32];
	size_t clear_size;
	void *clear_buffer = NULL;
	int delete_file = FALSE;
	int rest_free = FALSE;

	disk_info = fat_get_diskinfo(image);

	LOG(("fat_set_file_size(): file->first_cluster=%d new_size=0x%08x\n", file->first_cluster, new_size));

	/* special case */
	if (new_size == 0xFFFFFFFF)
	{
		delete_file = TRUE;
		new_size = 0;
	}
	
	/* if this is the trivial case (not changing the size), succeed */
	if (!delete_file && (file->filesize == new_size))
	{
		err = IMGTOOLERR_SUCCESS;
		goto done;
	}

	/* what is the new position? */
	new_pos = MIN(file->index, new_size);

	if (file->root)
	{
		/* this is the root directory; this is a special case */
		if (new_size > (disk_info->root_entries * FAT_DIRENT_SIZE))
		{
			err = IMGTOOLERR_NOSPACE;
			goto done;
		}
	}
	else
	{
		old_cluster_count = (file->filesize + disk_info->cluster_size - 1) / disk_info->cluster_size;
		new_cluster_count = (new_size + disk_info->cluster_size - 1) / disk_info->cluster_size;
		cluster = 0;

		/* load the dirent */
		err = fat_read_sector(image, file->dirent_sector_index, file->dirent_sector_offset, dirent, sizeof(dirent));
		if (err)
			goto done;

		/* need to load the FAT whether we are growing or shrinking the file */
		if (old_cluster_count != new_cluster_count)
		{
			err = fat_load_fat(image, &fat_table);
			if (err)
				goto done;

			cluster = 0;
			i = 0;
			do
			{
				last_cluster = cluster;
				write_cluster = 0;

				/* identify the next cluster */
				if (cluster != 0)
					cluster = fat_get_fat_entry(image, fat_table, cluster);
				else
					cluster = file->first_cluster ? file->first_cluster : 0xFFFFFFFF;

				/* do we need to grow the file by a cluster? */
				if (i < new_cluster_count && ((cluster < 2) || (cluster >= disk_info->total_clusters)))
				{
					/* grow this file by a cluster */
					cluster = fat_allocate_cluster(image, fat_table);
					if (cluster == 0)
					{
						err = IMGTOOLERR_NOSPACE;
						goto done;
					}
					
					write_cluster = cluster;
				}
				else if (i >= new_cluster_count)
				{
					/* we are shrinking the file; we need to unlink this node */
					if ((cluster < 2) || (cluster >= disk_info->total_clusters))
						cluster = 0xFFFFFFFF; /* ack file is corrupt! recover */
					write_cluster = 0xFFFFFFFF;
				}

				/* write out the entry, if appropriate */
				if (write_cluster != 0)
				{
					/* are we tieing up loose ends? */
					if (rest_free && (write_cluster == 0xFFFFFFFF))
						write_cluster = 0;

					if (last_cluster == 0)
						file->first_cluster = (write_cluster != 0xFFFFFFFF) ? write_cluster : 0;
					else
						fat_set_fat_entry(image, fat_table, last_cluster, write_cluster);

					/* did we write the last cluster?  if so, the rest (if any) are free */
					if (write_cluster == 0xFFFFFFFF)
						rest_free = TRUE;
				}
			}
			while((++i < new_cluster_count) || (cluster != 0xFFFFFFFF));
		}

		/* record the new file size */
		place_integer_le(dirent, 26, 2, file->first_cluster);
		place_integer_le(dirent, 28, 4, new_size);

		/* delete the file, if appropriate */
		if (delete_file)
			dirent[0] = 0xE5;

		/* save the dirent */
		err = fat_write_sector(image, file->dirent_sector_index, file->dirent_sector_offset, dirent, sizeof(dirent));
		if (err)
			goto done;

		/* if we've modified the FAT, save it out */
		if (fat_table)
		{
			err = fat_save_fat(image, fat_table);
			if (err)
				goto done;
		}

		/* update the file structure */
		if (!file->directory)
			file->filesize = new_size;
		file->cluster = file->first_cluster;
		file->index = 0;
		file->cluster_index = 0;
		file->eof = (new_cluster_count == 0);
	}

	/* special case; clear out stale bytes on non-root directories */
	if (file->directory && !delete_file)
	{
		if (file->root)
			clear_size = MIN(file->filesize - new_size, FAT_DIRENT_SIZE);
		else
			clear_size = (disk_info->cluster_size - (new_size % disk_info->cluster_size)) % disk_info->cluster_size;

		if (clear_size > 0)
		{
			err = fat_seek_file(image, file, new_size);
			if (err)
				goto done;

			clear_buffer = malloc(clear_size);
			if (!clear_buffer)
			{
				err = IMGTOOLERR_OUTOFMEMORY;
				goto done;
			}
			memset(clear_buffer, '\0', clear_size);

			err = fat_write_file(image, file, clear_buffer, clear_size, NULL);
			if (err)
				goto done;
		}
	}

	/* seek back to original pos */
	err = fat_seek_file(image, file, new_pos);
	if (err)
		goto done;

	if (fat_table)
		fat_debug_integrity_check(image, fat_table, file);

done:
	if (fat_table)
		free(fat_table);
	if (clear_buffer)
		free(clear_buffer);
	return err;
}



static void prepend_lfn_bytes(utf16_char_t *lfn_buf, size_t lfn_buflen, size_t *lfn_len,
	const UINT8 *entry, int offset, int chars)
{
	UINT16 w;
	int i;
	size_t move_len;

	move_len = MIN(*lfn_len + 1, lfn_buflen - chars - 1);
	memmove(&lfn_buf[chars], &lfn_buf[0], move_len * sizeof(*lfn_buf));

	for (i = 0; i < chars; i++)
	{
		/* read the character */
		memcpy(&w, &entry[offset + i * 2], 2);
		w = LITTLE_ENDIANIZE_INT16(w);

		/* append to buffer */
		lfn_buf[i] = (w != 0xFFFF) ? w : 0;
	}
	*lfn_len += chars;
}



static UINT8 fat_calc_filename_checksum(const UINT8 *short_filename)
{
	UINT8 checksum;
	int i, j;

	checksum = 0;
	for (i = 0; i < 11; i++)
	{
		j = checksum & 1;
		checksum >>= 1;
		if (j)
			checksum |= 0x80;
		checksum += short_filename[i];
	}
	return checksum;
}



static void fat_calc_dirent_lfnchecksum(UINT8 *entry, size_t entry_len)
{
	UINT8 checksum;
	int i;

	checksum = fat_calc_filename_checksum(entry + entry_len - FAT_DIRENT_SIZE);

	for (i = 0; i < (entry_len / FAT_DIRENT_SIZE - 1); i++)
		entry[i * FAT_DIRENT_SIZE + 13] = checksum;
}



static char fat_cannonicalize_sfn_char(char ch)
{
	/* return the display version of this short file name character */
	return tolower(ch);
}



static void fat_cannonicalize_sfn(char *sfn, const UINT8 *sfn_bytes)
{
	/* return the display version of this short file name */
	int i;

	memset(sfn, '\0', 13);
	memcpy(sfn, sfn_bytes, 8);
	rtrim(sfn);
	if (sfn[0] == 0x05)
		sfn[0] = (char) 0xE5;
	if ((sfn_bytes[8] != ' ') || (sfn_bytes[9] != ' ') || (sfn_bytes[10] != ' '))
	{
		strcat(sfn, ".");
		memcpy(sfn + strlen(sfn), &sfn_bytes[8], 3);
		rtrim(sfn);
	}
	for (i = 0; sfn[i]; i++)
		sfn[i] = fat_cannonicalize_sfn_char(sfn[i]);
}



static time_t fat_crack_time(UINT32 fat_time)
{
	struct tm t;
	time_t now;

	time(&now);
	t = *localtime(&now);

	t.tm_sec	= ((fat_time >>  0) & 0x001F) * 2;
	t.tm_min	= ((fat_time >>  5) & 0x003F);
	t.tm_hour	= ((fat_time >> 11) & 0x001F);
	t.tm_mday	= ((fat_time >> 16) & 0x001F);
	t.tm_mon	= ((fat_time >> 21) & 0x000F);
	t.tm_year	= ((fat_time >> 25) & 0x007F) + 1980 - 1900;

	return mktime(&t);
}



static UINT32 fat_setup_time(time_t ansi_time)
{
	struct tm t;
	UINT32 result = 0;

	t = *localtime(&ansi_time);

	result |= (((UINT32) (t.tm_sec / 2))			& 0x001F) <<  0;
	result |= (((UINT32) t.tm_min)					& 0x003F) <<  5;
	result |= (((UINT32) t.tm_hour)					& 0x001F) << 11;
	result |= (((UINT32) t.tm_mday)					& 0x001F) << 16;
	result |= (((UINT32) t.tm_mon)					& 0x000F) << 21;
	result |= (((UINT32) (t.tm_year + 1900 - 1980))	& 0x007F) << 25;

	return result;
}



static imgtoolerr_t fat_read_dirent(imgtool_image *image, struct fat_file *file,
	struct fat_dirent *ent, struct fat_freeentry_info *freeent)
{
	imgtoolerr_t err;
	const struct fat_diskinfo *disk_info;
	UINT8 entry[FAT_DIRENT_SIZE];
	size_t bytes_read;
	int i, j;
	unicode_char_t ch;
	utf16_char_t lfn_buf[512];
	size_t lfn_len = 0;
	int lfn_lastentry = 0;
	UINT8 lfn_checksum = 0;
	UINT32 entry_index, entry_sector_index, entry_sector_offset;

	assert(file->directory);
	lfn_buf[0] = '\0';
	memset(ent, 0, sizeof(*ent));
	disk_info = fat_get_diskinfo(image);

	/* The first eight bytes of a FAT directory entry is a blank padded name
	 *
	 * The first byte can be special:
	 *	0x00 - entry is available and no further entry is used
	 *	0x05 - first character is actually 0xe5
	 *	0x2E - dot entry; either '.' or '..'
	 *	0xE5 - entry has been erased and is available
	 *
	 * Byte 11 is the attributes; and 0x0F denotes a LFN entry
	 */
	do
	{
		entry_index = file->index;
		entry_sector_index = fat_get_filepos_sector_index(image, file);
		entry_sector_offset = file->index % FAT_SECLEN;

		err = fat_read_file(image, file, entry, sizeof(entry), &bytes_read);
		if (err)
			return err;
		if (bytes_read < sizeof(entry))
			memset(entry, 0, sizeof(entry));

		if (entry[11] == 0x0F)
		{
			/* this is an LFN entry */
			if ((lfn_lastentry == 0)
				|| ((entry[0] & 0x3F) != (lfn_lastentry - 1))
				|| (lfn_checksum != entry[13]))
			{
				lfn_buf[0] = 0;
				lfn_len = 0;
				lfn_checksum = entry[13];
			}
			lfn_lastentry = entry[0] & 0x3F;
			prepend_lfn_bytes(lfn_buf, sizeof(lfn_buf) / sizeof(lfn_buf[0]),
				&lfn_len, entry, 28, 2);
			prepend_lfn_bytes(lfn_buf, sizeof(lfn_buf) / sizeof(lfn_buf[0]),
				&lfn_len, entry, 14, 6);
			prepend_lfn_bytes(lfn_buf, sizeof(lfn_buf) / sizeof(lfn_buf[0]),
				&lfn_len, entry,  1, 5);
		}
		else if (freeent && (freeent->position == ~0))
		{
			/* do a quick check to find out if we found space */
			if ((entry[0] == '\0') || (entry[0] == 0xE5))
			{
				if (freeent->candidate_position > entry_index)
					freeent->candidate_position = entry_index;

				if ((entry[0] == '\0') || (freeent->candidate_position + freeent->required_size < file->index))
					freeent->position = freeent->candidate_position;
			}
			else
			{
				freeent->candidate_position = ~0;
			}
		}
	}
	while((entry[0] == 0x2E) || (entry[0] == 0xE5) || (entry[11] == 0x0F));

	/* no more directory entries? */
	if (entry[0] == '\0')
	{
		ent->eof = 1;
		return IMGTOOLERR_SUCCESS;
	}

	/* pick apart short filename */
	fat_cannonicalize_sfn(ent->short_filename, entry);

	/* and the long filename */
	if (lfn_lastentry == 1)
	{
		/* only use the LFN if the checksum passes */
		if (lfn_checksum == fat_calc_filename_checksum(entry))
		{
			i = 0;
			j = 0;
			do
			{
				i += uchar_from_utf16(&ch, &lfn_buf[i], sizeof(lfn_buf) / sizeof(lfn_buf[0]) - i);
				j += utf8_from_uchar(&ent->long_filename[j], sizeof(ent->long_filename) / sizeof(ent->long_filename[0]) - j, ch);
			}
			while(ch != 0);
		}
	}

	/* other attributes */
	ent->filesize				= pick_integer_le(entry, 28, 4);
	ent->directory				= (entry[11] & 0x10) ? 1 : 0;
	ent->first_cluster			= pick_integer_le(entry, 26, 2);
	ent->dirent_sector_index	= entry_sector_index;
	ent->dirent_sector_offset	= entry_sector_offset;
	ent->creation_time			= fat_crack_time(pick_integer_le(entry, 14, 4));
	ent->lastmodified_time		= fat_crack_time(pick_integer_le(entry, 22, 4));
	return IMGTOOLERR_SUCCESS;
}



typedef enum
{
	SFN_SUFFICIENT,	/* name fully representable in short file name */
	SFN_DERIVATIVE,	/* name not fully representable in short file name, but no need to tildize */
	SFN_MANGLED		/* name not representable in short file name; must tildize */
} sfn_disposition_t;

static imgtoolerr_t fat_construct_dirent(const char *filename, creation_policy_t create,
	UINT8 **entry, size_t *entry_len)
{
	imgtoolerr_t err = IMGTOOLERR_SUCCESS;
	UINT8 *created_entry = NULL;
	UINT8 *new_created_entry;
	UINT32 now;
	size_t created_entry_len = FAT_DIRENT_SIZE;
	size_t created_entry_pos = 0;
	unicode_char_t ch;
	char last_short_char = ' ';
	char short_char = '\0';
	char cannonical_short_char;
	utf16_char_t buf[UTF16_CHAR_MAX];
	int i, len;
	int sfn_pos = 0;
	sfn_disposition_t sfn_disposition = SFN_SUFFICIENT;
	int sfn_in_extension = 0;

	/* sanity check */
	if (*filename == '\0')
	{
		err = IMGTOOLERR_BADFILENAME;
		goto done;
	}

	/* construct intial entry */
	created_entry = (UINT8 *) malloc(FAT_DIRENT_SIZE);
	if (!created_entry)
	{
		err = IMGTOOLERR_OUTOFMEMORY;
		goto done;
	}

	/* set up the basics for the new dirent */
	memset(created_entry +  0, ' ', 11);
	memset(created_entry + 12, '\0', FAT_DIRENT_SIZE - 12);
	created_entry[11] = (create == CREATE_DIR) ? 0x10 : 0x00;
	
	/* set up file dates in the new dirent */
	now = fat_setup_time(time(NULL));
	place_integer_le(created_entry, 14, 4, now);
	place_integer_le(created_entry, 18, 2, now);
	place_integer_le(created_entry, 22, 4, now);

	while(*filename)
	{
		filename += uchar_from_utf8(&ch, filename, UTF8_CHAR_MAX);

		/* append to short filename, if possible */
		if ((ch < 32) || (ch > 128))
			short_char = '\0';
		else if (isalnum((char) ch))
			short_char = toupper((char) ch);
		else if (strchr(".!#$%^()-@^_`{}~", (char) ch))
			short_char = (char) ch;
		else
			short_char = '\0';	/* illegal SFN char */
		cannonical_short_char = fat_cannonicalize_sfn_char((char) ch);
		if (!short_char || (short_char != cannonical_short_char))
		{
			if (toupper(short_char) == toupper(cannonical_short_char))
				sfn_disposition = MAX(sfn_disposition, SFN_DERIVATIVE);
			else
				sfn_disposition = SFN_MANGLED;
		}

		/* append the short filename char */
		if (short_char == '.')
		{
			/* multiple extensions or trailing spaces? */
			if (sfn_in_extension)
				sfn_disposition = SFN_MANGLED;
			else if (last_short_char == ' ')
				sfn_disposition = MAX(sfn_disposition, SFN_DERIVATIVE);

			sfn_in_extension = 1;
			sfn_pos = 8;
			created_entry[created_entry_len - FAT_DIRENT_SIZE + 8] = ' ';
			created_entry[created_entry_len - FAT_DIRENT_SIZE + 9] = ' ';
			created_entry[created_entry_len - FAT_DIRENT_SIZE + 10] = ' ';
		}
		else if (sfn_pos == (sfn_in_extension ? 11 : 8))
		{
			/* ran out of characters for short filename */
			sfn_disposition = SFN_MANGLED;
		}
		else if (short_char != '\0')
		{
			created_entry[created_entry_len - FAT_DIRENT_SIZE + sfn_pos++] = short_char;
		}
		last_short_char = short_char;


		/* convert to UTF-16 and add a long filename entry */
		len = utf16le_from_uchar(buf, UTF16_CHAR_MAX, ch);
		for (i = 0; i < len; i++)
		{
			switch(created_entry_pos)
			{
				case 0:
				case 32:
					/* need to grow */
					new_created_entry = (UINT8 *) realloc(created_entry, created_entry_len + FAT_DIRENT_SIZE);
					if (!new_created_entry)
					{
						err = IMGTOOLERR_OUTOFMEMORY;
						goto done;
					}
					created_entry = new_created_entry;

					/* move existing entries forward */
					memmove(created_entry + 32, created_entry, created_entry_len);
					created_entry_len += 32;

					/* set up this LFN */
					memset(created_entry, '\0', 32);
					memset(&created_entry[1], '\xFF', 10);
					memset(&created_entry[14], '\xFF', 12);
					memset(&created_entry[28], '\xFF', 4);
					created_entry[11] = 0x0F;

					/* specify entry index */
					created_entry[0] = (created_entry_len / 32) - 1;
					if (created_entry[0] >= 0x40)
					{
						err = IMGTOOLERR_BADFILENAME;
						goto done;
					}
					created_entry_pos = 1;
					break;

				case 11:
					created_entry_pos = 14;
					break;

				case 26:
					created_entry_pos = 28;
					break;
			}

			memcpy(&created_entry[created_entry_pos], &buf[i], 2); 
			created_entry_pos += 2;
		}
	}

	/* trailing spaces? */
	if (short_char == ' ')
		sfn_disposition = MAX(sfn_disposition, SFN_DERIVATIVE);

	if (sfn_disposition == SFN_SUFFICIENT)
	{
		/* the short filename suffices; remove the LFN stuff */
		memcpy(created_entry, created_entry + created_entry_len - FAT_DIRENT_SIZE, FAT_DIRENT_SIZE);
		created_entry_len = FAT_DIRENT_SIZE;

		new_created_entry = (UINT8 *) realloc(created_entry, created_entry_len);
		if (!new_created_entry)
		{
			err = IMGTOOLERR_OUTOFMEMORY;
			goto done;
		}
		created_entry = new_created_entry;
	}
	else
	{
		/* need to do finishing touches on the LFN */
		created_entry[0] |= 0x40;

		/* if necessary, mangle the name */
		if (sfn_disposition == SFN_MANGLED)
		{
			i = 6;
			while((i > 0) && isspace(created_entry[created_entry_len - FAT_DIRENT_SIZE + i - 1]))
				i--;
			created_entry[created_entry_len - FAT_DIRENT_SIZE + i + 0] = '~';
			created_entry[created_entry_len - FAT_DIRENT_SIZE + i + 1] = '1';
		}
		fat_calc_dirent_lfnchecksum(created_entry, created_entry_len);
	}

done:
	if (err && created_entry)
	{
		free(created_entry);
		created_entry = NULL;
	}
	*entry = created_entry;
	*entry_len = created_entry_len;
	return err;
}



static void fat_bump_dirent(UINT8 *entry, size_t entry_len)
{
	UINT8 *sfn_entry;
	int pos, digit_count, i;
	UINT32 digit_place, val = 0;
	
	sfn_entry = &entry[entry_len - FAT_DIRENT_SIZE];

	digit_place = 1;
	for (pos = 7; (pos >= 0) && isdigit((char) sfn_entry[pos]); pos--)
	{
		val += (sfn_entry[pos] - '0') * digit_place;
		digit_place *= 10;
	}
	val++;
	pos++;

	/* count the digits */
	digit_place = 1;
	digit_count = 1;
	while(val >= digit_place * 10)
	{
		digit_count++;
		digit_place *= 10;
	}

	/* give us some more space, if necessary */
	while ((pos > 0) && ((pos + digit_count) > 8))
		pos--;

	/* have we actually ran out of digits */
	if ((pos + digit_count) > 8)
	{
		/* extreme degenerate case; simply randomize the filename */
		for (i = 0; i < 6; i++)
			sfn_entry[i] = 'A' + (rand() % 26);
		sfn_entry[6] = '~';
		sfn_entry[7] = '0';
	}
	else
	{
		/* write the tilde, if possible */
		if (pos > 0)
			sfn_entry[pos - 1] = '~';

		/* write out the number */
		while(digit_place > 0)
		{
			sfn_entry[pos++] = (val / digit_place) + '0';
			val %= digit_place;
			digit_place /= 10;
		}
	}

	/* since we changed the short file name, we need to recalc the checksums
	 * in the LFN entries */
	fat_calc_dirent_lfnchecksum(entry, entry_len);
}



static imgtoolerr_t fat_lookup_path(imgtool_image *image, const char *path,
	creation_policy_t create, struct fat_file *file)
{
	imgtoolerr_t err;
	const struct fat_diskinfo *disk_info;
	struct fat_dirent ent;
	struct fat_freeentry_info freeent = { 0, };
	const char *next_path_part;
	UINT8 *created_entry = NULL;
	size_t created_entry_len = 0;
	UINT32 entry_sector_index, entry_sector_offset;
	UINT32 parent_first_cluster;
	int bumped_sfn;
	char sfn[13];

	disk_info = fat_get_diskinfo(image);

	memset(file, 0, sizeof(*file));
	file->root = 1;
	file->directory = 1;
	file->filesize = disk_info->root_entries * FAT_DIRENT_SIZE;

	while(*path)
	{
		if (!file->directory)
		{
			err = IMGTOOLERR_PATHNOTFOUND;
			goto done;
		}

		next_path_part = path + strlen(path) + 1;
		if (create && (*next_path_part == '\0'))
		{
			/* this is the last entry, and we are creating a file */
			err = fat_construct_dirent(path, create, &created_entry, &created_entry_len);
			if (err)
				goto done;

			freeent.required_size = created_entry_len;
			freeent.candidate_position = ~0;
			freeent.position = ~0;
		}

		do
		{
			err = fat_read_dirent(image, file, &ent, created_entry ? &freeent : NULL);
			if (err)
				goto done;

			LOG(("fat_lookup_path(): %s/%s: %d\n", ent.short_filename, ent.long_filename, ent.dirent_sector_offset));
		}
		while(!ent.eof && mame_stricmp(path, ent.short_filename) && mame_stricmp(path, ent.long_filename));

		parent_first_cluster = file->first_cluster;

		if (ent.eof)
		{
			/* it seems that we have reached the end of this directory */
			if (!created_entry)
			{
				err = IMGTOOLERR_FILENOTFOUND;
				goto done;
			}

			if (created_entry_len > FAT_DIRENT_SIZE)
			{
				/* must ensure uniqueness of the short filename */
				do
				{
					/* rewind to the beginning of the directory */
					err = fat_seek_file(image, file, 0);
					if (err)
						goto done;

					bumped_sfn = FALSE;
					fat_cannonicalize_sfn(sfn, &created_entry[created_entry_len - FAT_DIRENT_SIZE]);

					do
					{
						err = fat_read_dirent(image, file, &ent, NULL);
						if (err)
							goto done;

						if (!mame_stricmp(sfn, ent.short_filename))
						{
							bumped_sfn = TRUE;
							fat_bump_dirent(created_entry, created_entry_len);
							fat_cannonicalize_sfn(sfn, &created_entry[created_entry_len - FAT_DIRENT_SIZE]);
						}
					}
					while(!ent.eof);
				}
				while(bumped_sfn);
			}

			LOG(("fat_lookup_path(): creating entry; pos=%u length=%u\n", freeent.position, freeent.required_size));

			err = fat_set_file_size(image, file, MAX(file->filesize, freeent.position + created_entry_len));
			if (err)
				goto done;

			err = fat_seek_file(image, file, freeent.position);
			if (err)
				goto done;
			
			err = fat_write_file(image, file, created_entry, created_entry_len, NULL);
			if (err)
				goto done;

			/* we have to do a special seek operation to get the main dirent */
			err = fat_seek_file(image, file, freeent.position + created_entry_len - FAT_DIRENT_SIZE);
			if (err)
				goto done;
			entry_sector_index = fat_get_filepos_sector_index(image, file);
			entry_sector_offset = file->index % FAT_SECLEN;

			/* build the file struct for the newly created file/directory */
			memset(file, 0, sizeof(*file));
			file->directory = (created_entry[created_entry_len - FAT_DIRENT_SIZE + 11] & 0x10) ? 1 : 0;
			file->dirent_sector_index = entry_sector_index;
			file->dirent_sector_offset = entry_sector_offset;
		}
		else
		{
			/* update the current file */
			memset(file, 0, sizeof(*file));
			file->directory = ent.directory;
			file->filesize = ent.filesize;
			file->cluster = ent.first_cluster;
			file->first_cluster = ent.first_cluster;
			file->dirent_sector_index = ent.dirent_sector_index;
			file->dirent_sector_offset = ent.dirent_sector_offset;
		}

		path = next_path_part;
		file->parent_first_cluster = parent_first_cluster;
	}

	err = IMGTOOLERR_SUCCESS;

done:
	if (created_entry)
		free(created_entry);
	return err;
}



static imgtoolerr_t fat_diskimage_beginenum(imgtool_imageenum *enumeration, const char *path)
{
	imgtoolerr_t err;
	struct fat_file *file;

	file = (struct fat_file *) img_enum_extrabytes(enumeration);

	err = fat_lookup_path(img_enum_image(enumeration), path, CREATE_NONE, file);
	if (err)
		return err;
	if (!file->directory)
		return IMGTOOLERR_PATHNOTFOUND;
	return IMGTOOLERR_SUCCESS;
}



static imgtoolerr_t fat_diskimage_nextenum(imgtool_imageenum *enumeration, imgtool_dirent *ent)
{
	imgtoolerr_t err;
	struct fat_file *file;
	struct fat_dirent fatent;

	file = (struct fat_file *) img_enum_extrabytes(enumeration);
	err = fat_read_dirent(img_enum_image(enumeration), file, &fatent, NULL);
	if (err)
		return err;

	/* copy stuff from the FAT dirent to the Imgtool dirent */
	snprintf(ent->filename, sizeof(ent->filename) / sizeof(ent->filename[0]), "%s", fatent.long_filename[0]
		? fatent.long_filename : fatent.short_filename);
	ent->filesize = fatent.filesize;
	ent->directory = fatent.directory;
	ent->eof = fatent.eof;
	ent->creation_time = fatent.creation_time;
	ent->lastmodified_time = fatent.lastmodified_time;
	return IMGTOOLERR_SUCCESS;
}



static imgtoolerr_t fat_read_bootblock(imgtool_image *image, imgtool_stream *stream)
{
	imgtoolerr_t err;
	UINT8 block[FAT_SECLEN];

	err = fat_read_sector(image, 0, 0, block, sizeof(block));
	if (err)
		return err;

	stream_write(stream, block, sizeof(block));
	return IMGTOOLERR_SUCCESS;
}



static imgtoolerr_t fat_write_bootblock(imgtool_image *image, imgtool_stream *stream)
{
	imgtoolerr_t err;
	UINT8 block[FAT_SECLEN];
	UINT8 new_block[FAT_SECLEN];

	if (stream_size(stream) != sizeof(new_block))
		return IMGTOOLERR_UNEXPECTED;
	stream_read(stream, new_block, sizeof(new_block));

	if (new_block[510] != 0x55)
		return IMGTOOLERR_UNEXPECTED;
	if (new_block[511] != 0xAA)
		return IMGTOOLERR_UNEXPECTED;

	/* read current boot sector */
	err = fat_read_sector(image, 0, 0, block, sizeof(block));
	if (err)
		return err;

	/* merge in the new stuff */
	memcpy(&block[ 0], &new_block[ 0],   3);
	memcpy(&block[62], &new_block[62], 448);

	/* and write it out */
	err = fat_write_sector(image, 0, 0, block, sizeof(block));
	if (err)
		return err;

	return IMGTOOLERR_SUCCESS;
}



static imgtoolerr_t fat_diskimage_readfile(imgtool_image *image, const char *filename, const char *fork, imgtool_stream *destf)
{
	imgtoolerr_t err;
	struct fat_file file;
	size_t bytes_read;
	char buffer[1024];

	/* special case for bootblock */
	if (filename == FILENAME_BOOTBLOCK)
		return fat_read_bootblock(image, destf);

	err = fat_lookup_path(image, filename, CREATE_NONE, &file);
	if (err)
		return err;

	if (file.directory)
		return IMGTOOLERR_FILENOTFOUND;

	do
	{
		err = fat_read_file(image, &file, buffer, sizeof(buffer), &bytes_read);
		if (err)
			return err;

		stream_write(destf, buffer, bytes_read);
	}
	while(bytes_read > 0);
	return IMGTOOLERR_SUCCESS;
}



static imgtoolerr_t fat_diskimage_writefile(imgtool_image *image, const char *filename, const char *fork, imgtool_stream *sourcef, option_resolution *opts)
{
	imgtoolerr_t err;
	struct fat_file file;
	UINT32 bytes_left, len;
	char buffer[1024];

	/* special case for bootblock */
	if (filename == FILENAME_BOOTBLOCK)
		return fat_write_bootblock(image, sourcef);

	err = fat_lookup_path(image, filename, CREATE_FILE, &file);
	if (err)
		return err;

	if (file.directory)
		return IMGTOOLERR_FILENOTFOUND;

	bytes_left = (UINT32) stream_size(sourcef);

	err = fat_set_file_size(image, &file, bytes_left);
	if (err)
		return err;

	while(bytes_left > 0)
	{
		len = MIN(bytes_left, sizeof(buffer));
		stream_read(sourcef, buffer, len);

		err = fat_write_file(image, &file, buffer, len, NULL);
		if (err)
			return err;

		bytes_left -= len;
	}
	return IMGTOOLERR_SUCCESS;
}



static imgtoolerr_t fat_diskimage_delete(imgtool_image *image, const char *filename, unsigned int dir)
{
	imgtoolerr_t err;
	struct fat_file file;
	struct fat_dirent ent;

	err = fat_lookup_path(image, filename, CREATE_NONE, &file);
	if (err)
		return err;
	if (file.directory != dir)
		return IMGTOOLERR_FILENOTFOUND;

	if (dir)
	{
		err = fat_read_dirent(image, &file, &ent, NULL);
		if (err)
			return err;
		if (!ent.eof)
			return IMGTOOLERR_DIRNOTEMPTY;
	}

	err = fat_set_file_size(image, &file, 0xFFFFFFFF);
	if (err)
		return err;

	return IMGTOOLERR_SUCCESS;
}



static imgtoolerr_t fat_diskimage_deletefile(imgtool_image *image, const char *filename)
{
	return fat_diskimage_delete(image, filename, 0);
}



static imgtoolerr_t fat_diskimage_freespace(imgtool_image *image, UINT64 *size)
{
	imgtoolerr_t err;
	const struct fat_diskinfo * disk_info;
	UINT8 *fat_table;
	UINT32 i;

	disk_info = fat_get_diskinfo(image);

	err = fat_load_fat(image, &fat_table);
	if (err)
		goto done;

	*size = 0;
	for (i = 2; i < disk_info->total_clusters; i++)
	{
		if (fat_get_fat_entry(image, fat_table, i) == 0)
			*size += disk_info->cluster_size;
	}

done:
	if (fat_table)
		free(fat_table);
	return err;
}



static imgtoolerr_t fat_diskimage_createdir(imgtool_image *image, const char *path)
{
	imgtoolerr_t err;
	struct fat_file file;
	UINT8 initial_data[64];

	err = fat_lookup_path(image, path, CREATE_DIR, &file);
	if (err)
		return err;
	if (!file.directory)
		return IMGTOOLERR_FILENOTFOUND;

	err = fat_set_file_size(image, &file, sizeof(initial_data));
	if (err)
		return err;

	memset(initial_data, 0, sizeof(initial_data));
	memcpy(&initial_data[0], ".          ", 11);
	place_integer_le(initial_data, 11, 1, 0x10);
	place_integer_le(initial_data, 26, 2, file.first_cluster);
	memcpy(&initial_data[32], ".          ", 11);
	place_integer_le(initial_data, 43, 1, 0x10);
	place_integer_le(initial_data, 58, 2, file.parent_first_cluster);

	err = fat_write_file(image, &file, initial_data, sizeof(initial_data), NULL);
	if (err)
		return err;

	return IMGTOOLERR_SUCCESS;
}



static imgtoolerr_t fat_diskimage_deletedir(imgtool_image *image, const char *path)
{
	return fat_diskimage_delete(image, path, 1);
}



/*********************************************************************
	Imgtool module declaration
*********************************************************************/

static void fat_module_populate(UINT32 state, union imgtoolinfo *info)
{
	switch(state)
	{
		case IMGTOOLINFO_INT_INITIAL_PATH_SEPARATOR:		info->i = 1; break;
		case IMGTOOLINFO_INT_OPEN_IS_STRICT:				info->i = 1; break;
		case IMGTOOLINFO_INT_SUPPORTS_CREATION_TIME:		info->i = 1; break;
		case IMGTOOLINFO_INT_SUPPORTS_LASTMODIFIED_TIME:	info->i = 1; break;
		case IMGTOOLINFO_INT_SUPPORTS_BOOTBLOCK:			info->i = 1; break;
		case IMGTOOLINFO_INT_PATH_SEPARATOR:				info->i = '\\'; break;
		case IMGTOOLINFO_INT_ALTERNATE_PATH_SEPARATOR:		info->i = '/'; break;
		case IMGTOOLINFO_STR_EOLN:							strcpy(info->s = imgtool_temp_str(), "\r\n"); break;
		case IMGTOOLINFO_INT_IMAGE_EXTRA_BYTES:				info->i = sizeof(struct fat_diskinfo); break;
		case IMGTOOLINFO_INT_ENUM_EXTRA_BYTES:				info->i = sizeof(struct fat_file); break;
		case IMGTOOLINFO_PTR_CREATE:						info->create = fat_diskimage_create; break;
		case IMGTOOLINFO_PTR_OPEN:							info->open = fat_diskimage_open; break;
		case IMGTOOLINFO_PTR_BEGIN_ENUM:					info->begin_enum = fat_diskimage_beginenum; break;
		case IMGTOOLINFO_PTR_NEXT_ENUM:						info->next_enum = fat_diskimage_nextenum; break;
		case IMGTOOLINFO_PTR_READ_FILE:						info->read_file = fat_diskimage_readfile; break;
		case IMGTOOLINFO_PTR_WRITE_FILE:					info->write_file = fat_diskimage_writefile; break;
		case IMGTOOLINFO_PTR_DELETE_FILE:					info->delete_file = fat_diskimage_deletefile; break;
		case IMGTOOLINFO_PTR_FREE_SPACE:					info->free_space = fat_diskimage_freespace; break;
		case IMGTOOLINFO_PTR_CREATE_DIR:					info->create_dir = fat_diskimage_createdir; break;
		case IMGTOOLINFO_PTR_DELETE_DIR:					info->delete_dir = fat_diskimage_deletedir; break;
	}
}



FLOPPYMODULE(fat, "FAT format", pc, fat_module_populate)


/* ----------------------------------------------------------------------- *
 * CHD disk images                                                         *
 * ----------------------------------------------------------------------- */

OPTION_GUIDE_START( fat_chd_create_optionguide )
	OPTION_INT('T', "cylinders",	"Cylinders" )
	OPTION_INT('H', "heads",		"Heads" )
	OPTION_INT('S', "sectors",		"Sectors" )
OPTION_GUIDE_END

static const char fat_chd_create_optionspec[] = "H1-[16]S1-[32]-63T10/20/30/40/50/60/70/80/90/[100]/110/120/130/140/150/160/170/180/190/200";



static imgtoolerr_t fat_chd_diskimage_open(imgtool_image *image, imgtool_stream *stream)
{
	imgtoolerr_t err;
	struct fat_diskinfo *disk_info;

	disk_info = fat_get_diskinfo(image);
	err = imghd_open(stream, &disk_info->harddisk);
	if (err)
		goto done;

	err = fat_diskimage_open(image, NULL);
	if (err)
		goto done;

done:
	if (err)
		imghd_close(&disk_info->harddisk);
	return err;
}



static imgtoolerr_t fat_chd_diskimage_create(imgtool_image *image, imgtool_stream *f, option_resolution *opts)
{
	imgtoolerr_t err;
	UINT32 cylinders, heads, sectors;
	struct fat_diskinfo *disk_info;

	cylinders = option_resolution_lookup_int(opts, 'T');
	heads = option_resolution_lookup_int(opts, 'H');
	sectors = option_resolution_lookup_int(opts, 'S');

	disk_info = fat_get_diskinfo(image);

	/* create the hard disk image */
	err = imghd_create(f, 0, cylinders, heads, sectors, FAT_SECLEN);
	if (err)
		goto done;

	err = imghd_open(f, &disk_info->harddisk);
	if (err)
		goto done;

	err = fat_diskimage_create(image, NULL, opts);
	if (err)
		goto done;

	err = fat_diskimage_open(image, NULL);
	if (err)
		goto done;

done:
	if (err)
		imghd_close(&disk_info->harddisk);
	return err;
}



static void fat_chd_diskimage_close(imgtool_image *image)
{
	struct fat_diskinfo *disk_info;
	disk_info = fat_get_diskinfo(image);
	imghd_close(&disk_info->harddisk);
}



static imgtoolerr_t	fat_chd_diskimage_getsectorsize(imgtool_image *image, UINT32 track, UINT32 head, UINT32 sector, UINT32 *sector_size)
{
	struct fat_diskinfo *disk_info;
	disk_info = fat_get_diskinfo(image);
	*sector_size = imghd_get_header(&disk_info->harddisk)->sectorbytes;
	return IMGTOOLERR_SUCCESS;
}



static UINT32 fat_chd_calc_lbasector(struct fat_diskinfo *disk_info, UINT32 track, UINT32 head, UINT32 sector)
{
	UINT32 lbasector;
	const hard_disk_info *hd_info;

	hd_info = imghd_get_header(&disk_info->harddisk);
	lbasector = track;
	lbasector *= hd_info->heads;
	lbasector += head;
	lbasector *= hd_info->sectors;
	lbasector += sector;
	return lbasector;
}



static imgtoolerr_t	fat_chd_diskimage_readsector(imgtool_image *image, UINT32 track, UINT32 head, UINT32 sector, void *buffer, size_t len)
{
	struct fat_diskinfo *disk_info;
	disk_info = fat_get_diskinfo(image);
	return imghd_read(&disk_info->harddisk,
		fat_chd_calc_lbasector(disk_info, track, head, sector),
		1, buffer);
}



static imgtoolerr_t	fat_chd_diskimage_writesector(imgtool_image *image, UINT32 track, UINT32 head, UINT32 sector, const void *buffer, size_t len)
{
	struct fat_diskinfo *disk_info;
	disk_info = fat_get_diskinfo(image);
	return imghd_write(&disk_info->harddisk,
		fat_chd_calc_lbasector(disk_info, track, head, sector),
		1, buffer);
}



imgtoolerr_t pc_chd_createmodule(imgtool_library *library)
{
	imgtoolerr_t err;
	struct ImageModule *module;

	err = imgtool_library_createmodule(library, "pc_chd_fat", &module);
	if (err)
		return err;

	module->name						= "pc_chd_fat";
	module->description					= "PC CHD disk image (FAT format)";
	module->extensions					= "chd\0";
	module->initial_path_separator		= 1;
	module->open_is_strict				= 1;
	module->supports_creation_time		= 1;
	module->supports_lastmodified_time	= 1;
	module->tracks_are_called_cylinders	= 1;
	module->supports_bootblock			= 1;
	module->path_separator				= '\\';
	module->alternate_path_separator	= '/';
	module->eoln						= EOLN_CRLF;
	module->image_extra_bytes			+= sizeof(struct fat_diskinfo);
	module->imageenum_extra_bytes		+= sizeof(struct fat_file);
	module->open						= fat_chd_diskimage_open;
	module->close						= fat_chd_diskimage_close;
	module->create						= fat_chd_diskimage_create;
	module->begin_enum					= fat_diskimage_beginenum;
	module->next_enum					= fat_diskimage_nextenum;
	module->read_file					= fat_diskimage_readfile;
	module->write_file					= fat_diskimage_writefile;
	module->delete_file					= fat_diskimage_deletefile;
	module->free_space					= fat_diskimage_freespace;
	module->create_dir					= fat_diskimage_createdir;
	module->delete_dir					= fat_diskimage_deletedir;
	module->get_sector_size				= fat_chd_diskimage_getsectorsize;
	module->read_sector					= fat_chd_diskimage_readsector;
	module->write_sector				= fat_chd_diskimage_writesector;
	module->createimage_optguide		= fat_chd_create_optionguide;
	module->createimage_optspec			= fat_chd_create_optionspec;
	return IMGTOOLERR_SUCCESS;
}
