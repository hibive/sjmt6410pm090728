////////////////////////////////////////////////////////
//                                                    //
//  FAT16/32 Filesystem API                           //
//                                                    //
//  2005.  1                                          //
//                                                    //
//  by Yi, Sang-Jin                                   //
//  A.K.A jeneena (webmaster@project-hf.net)          //
//                                                    //
//  http://www.project-hf.net                         //
//                                                    //
////////////////////////////////////////////////////////


#ifndef _FAT32_H_
#define _FAT32_H_


typedef unsigned __int64 uint64_t;
typedef unsigned __int32 uint32_t;
typedef unsigned __int16 uint16_t;
typedef unsigned __int8 uint8_t;
typedef unsigned __int64 u_int64_t;
typedef unsigned __int32 u_int32_t;
typedef unsigned __int16 u_int16_t;
typedef unsigned __int8 u_int8_t;
typedef __int64 int64_t;
typedef __int32 int32_t;
typedef __int16 int16_t;
typedef __int8  int8_t;



/**************************************************************************
*                                                                         *
* Configurations                                                          *
*                                                                         *
***************************************************************************/

// IDE와 MMC중에서 하나만 선택
//#define FAT32_USE_IDE
#define FAT32_USE_MMC


// 메모리를 절약하려면 아래의 수치를 적절히 수정한다
#define FAT32_LONG_NAME_MAX							200		// 긴 파일이름 길이의 상한선 설정
#define FAT32_FAT_BUFFER_MAX						1500    // FAT테이블 캐시의 크기 설정
#define FAT32_FILES_PER_DIR_MAX						50     	// 한개의 디렉터리에 최대 파일 수 설정


/**************************************************************************
*                                                                         *
* 매크로 정의                                                             *
*                                                                         *
***************************************************************************/


// 디바이스에 맞는 로우레벨 함수 매핑
#ifdef FAT32_USE_MMC
	#define	fat32_device_read_sector					mmc_read_sector
	#define fat32_device_write_sector					mmc_write_sector
#endif

#ifdef FAT32_USE_IDE
	#define fat32_device_read_sector					ide_read_sector
	#define fat32_device_write_sector					ide_write_sector
#endif


#define FAT32_ATTR_READ_ONLY							0x01
#define FAT32_ATTR_HIDDEN								0x02
#define FAT32_ATTR_SYSTEM								0x04
#define FAT32_ATTR_VOLUME_ID							0x08
#define FAT32_ATTR_DIRECTORY							0x10
#define FAT32_ATTR_ARCHIVE								0x20
#define FAT32_ATTR_LONG_NAME							0x0f

#define FAT32_END_OF_ENTRY								0x00
#define FAT32_KANJI_0XE5								0x05
#define FAT32_REMOVED_ENTRY								0xe5

#define FAT32_DIR_OFFSET_NAME							0
#define FAT32_DIR_OFFSET_ATTR							11
#define FAT32_DIR_OFFSET_NTRES							12
#define FAT32_DIR_OFFSET_CRTTIME_TENTH					13
#define FAT32_DIR_OFFSET_CRTTIME						14
#define FAT32_DIR_OFFSET_CRTDATE						16
#define FAT32_DIR_OFFSET_LSTACCDATE						18
#define FAT32_DIR_OFFSET_FSTCLUSHI						20
#define FAT32_DIR_OFFSET_WRTTIME						22
#define FAT32_DIR_OFFSET_WRTDATE						24
#define FAT32_DIR_OFFSET_FSTCLUSLO						26
#define FAT32_DIR_OFFSET_FILESIZE						28

#define FAT32_LONG_NAME_OFFSET_ORDER			0
#define FAT32_LONG_NAME_OFFSET_NAME1			1
#define FAT32_LONG_NAME_OFFSET_ATTR				11
#define FAT32_LONG_NAME_OFFSET_TYPE				12
#define FAT32_LONG_NAME_OFFSET_CHECKSUM			13
#define FAT32_LONG_NAME_OFFSET_NAME2			14
#define FAT32_LONG_NAME_OFFSET_FSTCLUSLO		26
#define FAT32_LONG_NAME_OFFSET_NAME3			28


/**************************************************************************
*                                                                         *
* Struct definition                                                       *
*                                                                         *
***************************************************************************/

typedef struct
{
	uint8_t 	boot_flag;
	uint8_t 	type_code;
	uint32_t	start_lba;
	uint32_t	n_sector;
	uint32_t	serial_no;
	uint8_t*	volume_label;
} FAT32_DEVICE_PARTITION_DESCRIPTOR;

typedef struct
{
	uint16_t	bytes_per_sector;
	uint8_t		sector_per_cluster;
	uint16_t	n_reserved_sector;
	uint8_t		n_fat;
	uint16_t	n_root_entry;
	uint8_t		media_descriptor;
	uint32_t	sector_per_fat;
	uint32_t	root_cluster;
	uint16_t	volume_signature;
	uint32_t	fat_start_sector;
	uint32_t	fat_start_data_sector;
	uint32_t	root_dir;
	
	uint32_t	current_dir_start_lba;
	uint32_t	current_dir_start_cluster;

} FAT32_FAT_DESCRIPTOR;

typedef struct
{
	uint8_t 		name[12];
	uint8_t			attribute;
	uint8_t			winnt_reserved;
	uint8_t			create_time_tenth;
	uint16_t		create_time;
	uint16_t		create_date;
	uint16_t		last_access_date;
	uint16_t		first_cluster_high;
	uint16_t		write_time;
	uint16_t		write_date;
	uint16_t		first_cluster_low;
	uint32_t		size;
} FAT32_DIR_ENTRY;

typedef struct
{
	uint8_t			order;
	uint8_t			name1[11];
	uint8_t			attribute;
	uint8_t			type;
	uint8_t			checksum;
	uint8_t 		name2[13];
	uint16_t		first_cluster_low;
	uint8_t			name3[5];
} FAT32_LONG_DIR_ENTRY;

typedef struct
{
	uint32_t		pos;
	uint32_t		length;
	uint32_t		fat_cluster_chain[FAT32_FAT_BUFFER_MAX];
} FAT32_FAT_BUFFER;

typedef struct
{
	uint32_t		cluster_pos;
	uint8_t			attribute;
	uint32_t		size;
	uint8_t			unicode_long_name[FAT32_LONG_NAME_MAX];
	uint8_t			short_name[11];
} FAT32_DIR_DESCRIPTOR;

typedef struct
{
	FAT32_FAT_DESCRIPTOR* 	fat_desc;
	FAT32_DIR_DESCRIPTOR* 	dir;
	uint8_t					mode;
	uint32_t				current_pos;
	uint32_t				current_cluster;
	uint32_t				fat_chain_offset;
} FAT32_FILE_DESCRIPTOR;

typedef struct
{
	FAT32_FAT_DESCRIPTOR* 	fat_desc;
	FAT32_DIR_DESCRIPTOR* 	dir;
	uint8_t					mode;
	uint64_t				current_pos;
	uint32_t				current_cluster;
	uint32_t				fat_chain_offset;
} FAT32_FILE_DESCRIPTOR64;

/**************************************************************************
*                                                                         *
* Function Prototypes                                                     *
*                                                                         *
***************************************************************************/

// 유틸리티 함수
uint16_t 	fat32_get_dir_list(FAT32_FAT_DESCRIPTOR* fat_desc, uint8_t dir_buf[FAT32_FILES_PER_DIR_MAX][11], const uint8_t* path);
int8_t 		fat32_get_dir(FAT32_FAT_DESCRIPTOR* fat_desc, uint8_t dir_buf[11], const uint8_t* path, uint16_t dir_pos);
uint16_t	fat32_get_dir_count(FAT32_FAT_DESCRIPTOR* fat_desc, const uint8_t* path);
uint32_t 	fat32_chdir(FAT32_FAT_DESCRIPTOR* fat_desc, const uint8_t* path, FAT32_DIR_DESCRIPTOR*	dir_desc);
uint32_t 	fat32_cache_fat(FAT32_FAT_DESCRIPTOR* fat_desc, const uint8_t* path, FAT32_FAT_BUFFER* fat_cache);
int8_t		fat32_format_disk(FAT32_FAT_DESCRIPTOR* fat_desc, uint8_t flag_quick_format);

// 내부적으로 사용되는 함수들
void 		fat32_get_device_partition_info(FAT32_DEVICE_PARTITION_DESCRIPTOR* p_desc, const uint8_t p_num);
void 		fat32_get_descriptor(FAT32_FAT_DESCRIPTOR* desc, const uint8_t partition_num);
uint32_t 	fat32_get_next_cluster(FAT32_FAT_DESCRIPTOR* fat_desc, const uint32_t current_cluster);
uint32_t 	fat32_get_next_free_cluster(FAT32_FAT_DESCRIPTOR* fat_desc, const uint32_t current_cluster);
uint32_t 	fat32_find_file(FAT32_FAT_DESCRIPTOR* fat_desc, const uint8_t* path, const uint8_t* filename, FAT32_DIR_DESCRIPTOR*	dir);
uint32_t 	fat32_cluster2lba(FAT32_FAT_DESCRIPTOR* fat_desc, const uint32_t cluster_num);



/**********************************************************
*      향후 규현예정 함수                                 *
***********************************************************/

uint32_t	fat32_mkdir(FAT32_FAT_DESCRIPTOR* fat_desc, const uint8_t* path, FAT32_DIR_DESCRIPTOR*	dir_desc);
uint32_t	fat32_rmdir(FAT32_FAT_DESCRIPTOR* fat_desc, const uint8_t* path, FAT32_DIR_DESCRIPTOR*	dir_desc);

// 파일 입출력 함수들
uint8_t		fat32_fgetc(FAT32_FILE_DESCRIPTOR* fd);
uint8_t		fat32_fputc(FAT32_FILE_DESCRIPTOR* fd);

int8_t		fat32_fprintf(FAT32_FILE_DESCRIPTOR* fd, const uint8_t fmt, ...);
int8_t		fat32_fscanf(FAT32_FILE_DESCRIPTOR* fd, const uint8_t fmt, ...);
int8_t		fat32_fputs(FAT32_FILE_DESCRIPTOR* fd);

int8_t 		fat32_fread(FAT32_FILE_DESCRIPTOR* fd, uint8_t* buffer, uint16_t length);
int8_t 		fat32_fwrite(FAT32_FILE_DESCRIPTOR* fd,	uint8_t* buffer, uint16_t length);
int8_t 		fat32_fseek(FAT32_FILE_DESCRIPTOR* fd, uint32_t position);

FAT32_FILE_DESCRIPTOR* 
					fat32_fopen(FAT32_FAT_DESCRIPTOR* fat_desc,	uint8_t* filename);
int8_t		fat32_fclose(FAT32_FAT_DESCRIPTOR* fat_desc, FAT32_FILE_DESCRIPTOR* fd);
int8_t 		fat32_create(uint8_t* filename);
int8_t 		fat32_delete(uint8_t* filename);

//64비트 파일포인터 지원함수들
FAT32_FILE_DESCRIPTOR64* 
					fat32_fopen64(FAT32_FAT_DESCRIPTOR* fat_desc,	uint8_t* filename);
int8_t		fat32_fclose64(FAT32_FAT_DESCRIPTOR* fat_desc, FAT32_FILE_DESCRIPTOR64* fd);

uint8_t		fat32_fgetc64(FAT32_FILE_DESCRIPTOR64* fd);
uint8_t		fat32_fputc64(FAT32_FILE_DESCRIPTOR64* fd);

int8_t		fat32_fprintf64(FAT32_FILE_DESCRIPTOR64* fd, const uint8_t fmt, ...);
int8_t		fat32_fscanf64(FAT32_FILE_DESCRIPTOR64* fd, const uint8_t fmt, ...);
int8_t		fat32_fputs64(FAT32_FILE_DESCRIPTOR64* fd);

int8_t 		fat32_fread64(FAT32_FILE_DESCRIPTOR64* fd, uint8_t* buffer, uint16_t length);
int8_t 		fat32_fwrite64(FAT32_FILE_DESCRIPTOR64* fd,	uint8_t* buffer, uint16_t length);
int8_t		fat32_seek64(FAT32_FILE_DESCRIPTOR64* fd, uint64_t position);


/**************************************************************************
*                                                                         *
* Global Variables                                                        *
*                                                                         *
***************************************************************************/

uint8_t buffer[512];	// General purpose buffer


/**************************************************************************
*                                                                         *
* Inline functions                                                        *
*                                                                         *
***************************************************************************/

// C99를 제대로 지원하지 않는 컴파일러는 인라인 함수가 제대로 컴파일 되지 않는 경우가 있다.
// 그런 경우 아래의 함수들에서 "extern inline"키워드를 제거하고 보통의 함수처럼 변경한다.

// Optimization Level이 0인경우 인라인함수를 사용할 수 없다.
 
//extern inline uint32_t get_dword(uint8_t* buffer)
uint32_t get_dword(uint8_t* buffer);
/*{
	uint32_t temp;
	
	temp = ((uint32_t)buffer[3] << 24);
	temp += ((uint32_t)buffer[2] << 16);
	temp += ((uint32_t)buffer[1] << 8);
	temp +=(uint32_t)buffer[0];

	return temp;
}*/

//extern inline uint16_t get_word(uint8_t* buffer)
uint16_t get_word(uint8_t* buffer);
/*{
	uint16_t temp;

	temp = ((uint16_t)buffer[1] << 8);
	temp +=(uint16_t)buffer[0];

	return temp;
}*/


uint8_t 	mmc_read_sector(uint32_t sector,  uint16_t count, uint8_t* buffer);
void 		mmc_write_sector(uint32_t sector, uint8_t* buffer);

#endif
