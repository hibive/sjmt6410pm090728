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

//#include "global.h"
#include <windows.h>
#include <bsp.h>
#include <HSMMCDrv.h>

#include "sdmmc_fat32.h"



/**************************************************************************

향후 개선해야 할 사항들 - 2005/04

- FAT16/32 동시 지원
- 파일 입출력 함수의 완전한 구현
- 메모리가 충분한 시스템을 위해 동적 메모리 할당기법을 사용가능하도록!!
- AVR에 의존적인 함수들의 수정
- 섹터 크기가 512바이트가 아닌 디바이스의 지원
- 속도 향상을 위해 표준함수 memcpy(), memset(), memcmp()를 사용하지 말고 
  자체적인 인라인 함수를 쓰도록 한다
- 현재 모든 함수가 에러 처리에 취약함. 
- 빅 엔디안 시스템에 대한 고려
- 다중 파티션의 지원

***************************************************************************/



/**************************************************************************
*                                                                         *
*                                                                         *
*                                                                         *
***************************************************************************/
void fat32_get_device_partition_info(FAT32_DEVICE_PARTITION_DESCRIPTOR* p_desc, const uint8_t p_num)
{
	uint8_t i;
	uint8_t partition_info[16];
	
	fat32_device_read_sector(0, 1, buffer);
	
	// 파티션 정보는 16바이트로 구성되어 있다
	for (i=0; i<=15; i++)
	{
		partition_info[i] = buffer[446 + (p_num * 16) + i];
	}

	// 부트플래그	
	if (partition_info[0] == 0x80)
		p_desc->boot_flag = 1;
	else
		p_desc->boot_flag = 0;
		
	p_desc->type_code = partition_info[4];
	p_desc->start_lba = get_dword(partition_info + 8);
	p_desc->n_sector = get_dword(partition_info + 12);
	
	return;
}




/**************************************************************************
*                                                                         *
*                                                                         *
*                                                                         *
***************************************************************************/
void fat32_get_descriptor(FAT32_FAT_DESCRIPTOR* desc, const uint8_t partition_num)
{
	FAT32_DEVICE_PARTITION_DESCRIPTOR partition_info;
	
//#ifdef 	FAT32_USE_IDE
	fat32_get_device_partition_info(&partition_info, partition_num);
	fat32_device_read_sector(partition_info.start_lba, 1, buffer);
//#endif
/*
#ifdef FAT32_USE_MMC
	fat32_device_read_sector(0, 1, buffer);
	partition_info.start_lba = 0;
	partition_info.n_sector = get_dword(buffer + 0x20);
#endif
*/
	// FAT32
	desc->bytes_per_sector 			= get_word(buffer + 0x0b);
	desc->sector_per_cluster 		= buffer[0x0d];
	desc->n_reserved_sector 		= get_word(buffer + 0x0e);
	desc->n_fat 					= buffer[0x10];
	desc->sector_per_fat 			= get_dword(buffer + 0x24);
	desc->n_root_entry 				= get_dword(buffer + 0x11);
	desc->root_cluster 				= get_dword(buffer + 0x2c);
	desc->volume_signature 			= get_word(buffer + 0x01fe);
	desc->fat_start_sector 			= partition_info.start_lba + desc->n_reserved_sector;
	desc->fat_start_data_sector		= desc->fat_start_sector + (desc->n_fat * desc->sector_per_fat);
	desc->root_dir 					= (desc->root_cluster - 0x02) * desc->sector_per_cluster + (desc->fat_start_data_sector);
	
	desc->current_dir_start_lba 	= desc->root_dir;
	desc->current_dir_start_cluster = desc->root_cluster;
	
	//EdbgOutputDebugString("sector per cluster = %d\r\n", desc->sector_per_cluster);
	//EdbgOutputDebugString("bytes per sector = %d\r\n", desc->bytes_per_sector);
	return;
}



/**************************************************************************
*                                                                         *
*                                                                         *
*                                                                         *
***************************************************************************/
uint16_t fat32_get_dir_list(FAT32_FAT_DESCRIPTOR* fat_desc, 
						uint8_t dir_buf[FAT32_FILES_PER_DIR_MAX][11], const uint8_t* path)
{
	uint8_t			temp[32];
	uint16_t		dir_buf_pos;
	uint16_t		i;
	uint16_t		sec_offset;
	uint32_t		dir_lba;
	uint32_t		dir_cluster;
	uint8_t			flag_finish;
	
	dir_cluster = fat_desc->current_dir_start_cluster;
	dir_lba 	= fat_desc->current_dir_start_lba;
	
	flag_finish = 0;
	dir_buf_pos = 0;
	sec_offset = 0;
	
	while(flag_finish == 0)
	{
		fat32_device_read_sector(dir_lba + sec_offset, 1, buffer);
	
		for(i=0; i<(fat_desc->bytes_per_sector); i+=32)
		{
			memcpy(temp, buffer + i, 32);		

			if (temp[0] == FAT32_END_OF_ENTRY)		// 엔트리가 끝나는 경우 종료
			{
				flag_finish = 1;				
				break;
			}
			
			if (temp[FAT32_DIR_OFFSET_ATTR] & (FAT32_ATTR_DIRECTORY |  FAT32_ATTR_ARCHIVE | FAT32_ATTR_VOLUME_ID))
			{	
				// 파일명 엔트리 속성과 부합하는 경우 목록에 복사한다.
				if (((temp[FAT32_DIR_OFFSET_ATTR] != FAT32_ATTR_LONG_NAME) && temp[0] != FAT32_REMOVED_ENTRY))
				{
					memcpy(&(dir_buf[dir_buf_pos][0]), temp + FAT32_DIR_OFFSET_NAME, 11);
					
					dir_buf_pos++;
				}
			}
			
			if (dir_buf_pos >= FAT32_FILES_PER_DIR_MAX)
			{
				flag_finish = 1;				
				break;
			}
			
		}
		
		// 읽어둔 섹터의 끝에 도달한경우
		sec_offset++;
		
		if (sec_offset >= fat_desc->sector_per_cluster)
		{
			dir_lba = fat32_cluster2lba(fat_desc, fat32_get_next_cluster(fat_desc, dir_cluster));
			dir_cluster = fat32_get_next_cluster(fat_desc, dir_cluster);
			sec_offset = 0;
		}
		
		// FAT의 끝에 도달하는 경우
		if (dir_cluster >=0x0fffffff)
			return dir_buf_pos;
	}	
	
	return dir_buf_pos;
}

/**************************************************************************
*                                                                         *
*                                                                         *
*                                                                         *
***************************************************************************/
int8_t fat32_get_dir(FAT32_FAT_DESCRIPTOR* fat_desc, 
					uint8_t dir_buf[11], const uint8_t* path,
					uint16_t dir_pos)
{
	uint8_t			temp[32];
	uint16_t		i;
	uint16_t		sec_offset;
	uint32_t		dir_lba;
	uint32_t		dir_cluster;
	uint8_t			flag_finish;
	
	dir_cluster = fat_desc->current_dir_start_cluster;
	dir_lba 	= fat_desc->current_dir_start_lba;
	
	flag_finish = 0;
	sec_offset = 0;
	
	while(flag_finish == 0)
	{
		fat32_device_read_sector(dir_lba + sec_offset, 1, buffer);
	
		for(i=0; i<(fat_desc->bytes_per_sector); i+=32)
		{
			memcpy(temp, buffer + i, 32);		

			if (temp[0] == FAT32_END_OF_ENTRY)		// 엔트리가 끝나는 경우 종료
			{
				flag_finish = 1;				
				return -2;
			}
			
			if (temp[FAT32_DIR_OFFSET_ATTR] & (FAT32_ATTR_DIRECTORY |  FAT32_ATTR_ARCHIVE | FAT32_ATTR_VOLUME_ID))
			{	
				
				// 파일명 엔트리 속성과 부합하는 경우 
				if (((temp[FAT32_DIR_OFFSET_ATTR] != FAT32_ATTR_LONG_NAME) && temp[0] != FAT32_REMOVED_ENTRY))
				{
					// 지정된 디렉터리 순서에 도달할때까지 넘긴다
					if (dir_pos !=0)
					{
						dir_pos--;
						continue;
					}

					memcpy(&(dir_buf[0]), temp + FAT32_DIR_OFFSET_NAME, 11);
					flag_finish = 1;
					break;
				}
			}
		}
		
		// 읽어둔 섹터의 끝에 도달한경우
		sec_offset++;
		
		if (sec_offset >= fat_desc->sector_per_cluster)
		{
			dir_lba = fat32_cluster2lba(fat_desc, fat32_get_next_cluster(fat_desc, dir_cluster));
			dir_cluster = fat32_get_next_cluster(fat_desc, dir_cluster);
			sec_offset = 0;
		}
		
		// FAT의 끝에 도달하는 경우
		if (dir_cluster >=0x0fffffff)
			return -1;
	}	
	return 0;
}
						

/**************************************************************************
*                                                                         *
*                                                                         *
*                                                                         *
***************************************************************************/
uint16_t	fat32_get_dir_count(FAT32_FAT_DESCRIPTOR* fat_desc, const uint8_t* path)
{
	uint8_t			temp[32];
	uint16_t		dir_buf_pos;
	uint16_t		i;
	uint16_t		sec_offset;
	uint32_t		dir_lba;
	uint32_t		dir_cluster;
	uint8_t			flag_finish;
	
	dir_cluster = fat_desc->current_dir_start_cluster;
	dir_lba 	= fat_desc->current_dir_start_lba;
	
	flag_finish = 0;
	dir_buf_pos = 0;
	sec_offset = 0;
	
	while(flag_finish == 0)
	{
		fat32_device_read_sector(dir_lba + sec_offset, 1, buffer);
	
		for(i=0; i<(fat_desc->bytes_per_sector); i+=32)
		{
			memcpy(temp, buffer + i, 32);		

			if (temp[0] == FAT32_END_OF_ENTRY)		// 엔트리가 끝나는 경우 종료
			{
				flag_finish = 1;				
				break;
			}
			
			if (temp[FAT32_DIR_OFFSET_ATTR] & (FAT32_ATTR_DIRECTORY |  FAT32_ATTR_ARCHIVE | FAT32_ATTR_VOLUME_ID))
			{	
				// 파일명 엔트리 속성과 부합하는 경우 목록에 복사한다.
				if (((temp[FAT32_DIR_OFFSET_ATTR] != FAT32_ATTR_LONG_NAME) && temp[0] != FAT32_REMOVED_ENTRY))
				{
					dir_buf_pos++;
				}
			}
			
			if (dir_buf_pos >= FAT32_FILES_PER_DIR_MAX)
			{
				flag_finish = 1;				
				break;
			}
		}
		
		// 읽어둔 섹터의 끝에 도달한경우
		sec_offset++;
		
		if (sec_offset >= fat_desc->sector_per_cluster)
		{
			dir_lba = fat32_cluster2lba(fat_desc, fat32_get_next_cluster(fat_desc, dir_cluster));
			dir_cluster = fat32_get_next_cluster(fat_desc, dir_cluster);
			sec_offset = 0;
		}
		
		// FAT의 끝에 도달하는 경우
		if (dir_cluster >=0x0fffffff)
			return dir_buf_pos;
	}	
	return dir_buf_pos;
}

/**************************************************************************
*                                                                         *
*                                                                         *
*                                                                         *
***************************************************************************/
uint32_t fat32_find_file(FAT32_FAT_DESCRIPTOR* fat_desc, const uint8_t* path, 
						const uint8_t* filename, FAT32_DIR_DESCRIPTOR*	dir)
{
	uint8_t 			short_name[11];
	uint8_t				temp[32];
	uint8_t				flag_found;
	int16_t				byte_pos;
	uint16_t			sec_offset;
	uint32_t			dir_lba;
	uint32_t			dir_cluster;
	uint16_t			cluster_traverse_count = 0;
	
	uint32_t			prev_dir_lba = 0;
	uint32_t			prev_dir_cluster = 0;

// jhlee +++
	uint8_t				order = 0;
	int16_t				long_filename_byte_pos = 0;	
// jhlee ---

	flag_found = 0;
	sec_offset = 0;


	// 버퍼 초기화
	memset(short_name, 0x20, 11);
	memcpy(short_name, filename, 11);
	memset(dir->unicode_long_name, 0, FAT32_LONG_NAME_MAX);
	
	dir_lba = fat_desc->current_dir_start_lba;
	dir_cluster = fat_desc->current_dir_start_cluster;

	prev_dir_lba = dir_lba;
	prev_dir_cluster = dir_cluster;

	while(flag_found == 0)
	{
		fat32_device_read_sector(dir_lba + sec_offset, 1, buffer);
		sec_offset ++;
	
		dir->cluster_pos = 0;

		flag_found = 0;
		
		for(byte_pos = 0; byte_pos <= 511; byte_pos += 32)			// 511 -> 수정(섹터사이즈 유동성)
		{
			memcpy(temp, buffer + byte_pos, 32);	
			
			if (temp[FAT32_DIR_OFFSET_ATTR] == FAT32_ATTR_LONG_NAME)
			{
				continue;
			}
	
			else if (temp[0] == FAT32_REMOVED_ENTRY)
			{
				continue;
			}
			
			else if (temp[0] == FAT32_END_OF_ENTRY)
			{
				return 0;
			}
			
			else if (temp[0] == FAT32_KANJI_0XE5)		// 첫글자가 0xe5인경우 삭제된 엔트리로 인식하게 되는데
			{																				// 일본어코드중에서는 0xe5로 시작되는것이 있어
				temp[0] = 0xE5;												// 이와 혼동된다. 이럴때는 해당 0xe5를 0x05로 표현한다.
			}																				// 즉 0x05의 실제 문자코드는 0xe5가 된다.
			
			else if (temp[FAT32_DIR_OFFSET_ATTR] & (FAT32_ATTR_DIRECTORY |  FAT32_ATTR_ARCHIVE | FAT32_ATTR_VOLUME_ID))
			{
				if(memcmp(short_name, temp + FAT32_DIR_OFFSET_NAME, 11) == 0)
				{
					flag_found = 1;
					break;
				}
				else
				{
					continue;
				}
			}
		}
	
		if (flag_found == 0)
		{
			if (sec_offset >= fat_desc->sector_per_cluster)
			{
				prev_dir_lba = dir_lba;
				prev_dir_cluster = dir_cluster;
				
				dir_lba = fat32_cluster2lba(fat_desc, fat32_get_next_cluster(fat_desc, dir_cluster));
				dir_cluster = fat32_get_next_cluster(fat_desc, dir_cluster);
				
				sec_offset = 0;
				cluster_traverse_count++;
			}
			continue;
		}
		
		else
		{
			// 찾았을때
			dir->attribute 		= temp[FAT32_DIR_OFFSET_ATTR];
			dir->size					= get_dword(temp + FAT32_DIR_OFFSET_FILESIZE);
			dir->cluster_pos 	= get_word(temp + FAT32_DIR_OFFSET_FSTCLUSLO);
			dir->cluster_pos 	+= ((uint32_t)get_word(temp + FAT32_DIR_OFFSET_FSTCLUSHI) << 16);
			memcpy(dir->short_name, temp + FAT32_DIR_OFFSET_NAME, 11);	
		}

		// jhlee +++
		//uint8_t		order = 0;
		//int16_t		long_filename_byte_pos = 0;
		order = 0;
		long_filename_byte_pos = 0;
		// jhlee ---
		
		if (dir->attribute & (FAT32_ATTR_DIRECTORY | FAT32_ATTR_ARCHIVE))
		{
			// 긴 파일이름의 처리

			while(1)
			{
				if (byte_pos == 0 && long_filename_byte_pos == 0)		// 섹터의 첫부분인 경우 긴 파일이름은 이전 섹터에 위치
				{
					fat32_device_read_sector(dir_lba + (sec_offset - 2), 1, buffer);
	
					long_filename_byte_pos = 512 - 32;	// 이전 섹터 마지막 엔트리
		
				}	
				else if (order == 0)
				{
					if (byte_pos >= 32)
						long_filename_byte_pos = byte_pos - 32;
				}

				memcpy(temp, buffer + long_filename_byte_pos, 32);

				if (temp[FAT32_DIR_OFFSET_ATTR] == FAT32_ATTR_LONG_NAME)
				{
					order = temp[FAT32_LONG_NAME_OFFSET_ORDER] & 0x0F;

					memcpy(dir->unicode_long_name + ((order - 1) * 26), temp + FAT32_LONG_NAME_OFFSET_NAME1, 10);
					memcpy(dir->unicode_long_name + ((order - 1) * 26) + 10, temp + FAT32_LONG_NAME_OFFSET_NAME2, 12);
					memcpy(dir->unicode_long_name + ((order - 1) * 26) + 10 + 12, temp + FAT32_LONG_NAME_OFFSET_NAME3, 4);			
				}
				
				else
				{
					break;
				}
				long_filename_byte_pos -= 32;
				
				if (long_filename_byte_pos <= 0)			// 섹터의 첫부분에 도달하는 경우 나머지 긴 파일이름은 이전 섹터에 위치
				{
					fat32_device_read_sector(dir_lba + (sec_offset - 2), 1, buffer);
					long_filename_byte_pos = 512 - 32;	// 이전 섹터 마지막 엔트리
				}
			}
		}
	}
	return (dir->cluster_pos);
}



/**************************************************************************
*                                                                         *
*                                                                         *
*                                                                         *
***************************************************************************/
uint32_t fat32_chdir(FAT32_FAT_DESCRIPTOR* fat_desc, const uint8_t* path, FAT32_DIR_DESCRIPTOR*	dir)
{
	uint32_t			cluster_pos;
	
	//cluster_pos = fat32_find_file(fat_desc, "", path, dir);
	cluster_pos = fat32_find_file(fat_desc, NULL, path, dir);
	
	if ((dir->attribute & FAT32_ATTR_DIRECTORY) == 0)
		return 0;
	else
		return cluster_pos;
}


/**************************************************************************
*                                                                         *
*                                                                         *
*                                                                         *
***************************************************************************/
uint32_t fat32_get_next_cluster(FAT32_FAT_DESCRIPTOR* fat_desc, const uint32_t current_cluster)
{
	
	static uint8_t 		fat_buffer[512];
	static uint32_t		prev_cluster = 0xffffffff;
	
	uint32_t		fat_pos;
	uint32_t		fat_offset;
	uint32_t		next_cluster;
	
	fat_offset = current_cluster / 0x80;
	
	// 최근에 읽은 클러스터를 다시 읽는 경우 기존의 데이터를 재활용한다(속도 향상)
	if (prev_cluster != fat_offset)	
	{
		fat32_device_read_sector(fat_desc->fat_start_sector + fat_offset, 1, fat_buffer);
		prev_cluster = fat_offset;
	}
		
	fat_pos = (current_cluster - (fat_offset * 0x80)) * 4;
	next_cluster = get_dword(fat_buffer + fat_pos);
	
	return next_cluster;
}


/**************************************************************************
*                                                                         *
*                                                                         *
*                                                                         *
***************************************************************************/
uint32_t fat32_get_next_free_cluster(FAT32_FAT_DESCRIPTOR* fat_desc, const uint32_t current_cluster)
{
//	uint8_t 	buffer[512];
	uint32_t	fat_pos;
	uint32_t	fat_offset;
//	uint32_t	next_cluster;
	
	fat_offset = current_cluster / 0x80;
	fat32_device_read_sector(fat_desc->fat_start_sector + fat_offset, 1, buffer);
	fat_pos = (current_cluster - (fat_offset * 0x80)) * 4;

	// to-do, not yet implemented

	return 0;
}


/**************************************************************************
*                                                                         *
*                                                                         *
*                                                                         *
***************************************************************************/
uint32_t fat32_cluster2lba(FAT32_FAT_DESCRIPTOR* fat_desc, const uint32_t cluster_num)
{
	return ((cluster_num - 2) * fat_desc->sector_per_cluster) + fat_desc->fat_start_data_sector;
}



/**************************************************************************
*                                                                         *
*                                                                         *
*                                                                         *
***************************************************************************/
uint32_t fat32_cache_fat(FAT32_FAT_DESCRIPTOR* fat_desc, const uint8_t* path, 
						FAT32_FAT_BUFFER* fat_cache)
{
	uint32_t				start_cluster;
	uint16_t				count;
	
	FAT32_DIR_DESCRIPTOR	dir;
	
	//start_cluster = fat32_find_file(fat_desc, "", path, &dir);
	start_cluster = fat32_find_file(fat_desc, NULL, path, &dir);
	
	if (start_cluster == 0)
		return 0;
	
	fat_cache->pos = 0;
	fat_cache->fat_cluster_chain[0] = start_cluster;
	
	for (count=1; count<=FAT32_FAT_BUFFER_MAX; count++)		
	{
		fat_cache->fat_cluster_chain[count] = fat32_get_next_cluster(fat_desc, fat_cache->fat_cluster_chain[count-1]);
		if (fat_cache->fat_cluster_chain[count] >= 0x0fffffff)
			break;
	}
	fat_cache->length = count - 1;
	
	return start_cluster;
}


/**************************************************************************
*                                                                         *
*                                                                         *
*                                                                         *
***************************************************************************/
uint8_t		fat32_fgetc(FAT32_FILE_DESCRIPTOR* fd)
{
	
	// to-do, not yet implemented
	return 0;
}



/**************************************************************************
*                                                                         *
*                                                                         *
*                                                                         *
***************************************************************************/
uint8_t		fat32_fputc(FAT32_FILE_DESCRIPTOR* fd)
{

	// to-do, not yet implemented	
	return 0;
}



/**************************************************************************
*                                                                         *
*                                                                         *
*                                                                         *
***************************************************************************/
int8_t		fat32_fprintf(FAT32_FILE_DESCRIPTOR* fd, const uint8_t fmt, ...)
{
	
	// to-do, not yet implemented
	return 0;
}



/**************************************************************************
*                                                                         *
*                                                                         *
*                                                                         *
***************************************************************************/
int8_t		fat32_fscanf(FAT32_FILE_DESCRIPTOR* fd, const uint8_t fmt, ...)
{
	
	// to-do, not yet implemented
	return 0;
}


/**************************************************************************
*                                                                         *
*                                                                         *
*                                                                         *
***************************************************************************/
int8_t		fat32_fputs(FAT32_FILE_DESCRIPTOR* fd)
{
	// to-do, not yet implemented	
	return 0;
}



/**************************************************************************
*                                                                         *
*                                                                         *
*                                                                         *
***************************************************************************/
int8_t fat32_fread(FAT32_FILE_DESCRIPTOR* fd, uint8_t* buffer, uint16_t length)
{
	uint16_t	offset;
	uint16_t	sec_offset;
	uint16_t	byte_pos;
	uint16_t	cluster_size;
	uint16_t	i;
	uint16_t	n_loop;
//	uint8_t 	temp_buffer[512];

	fat32_fseek(fd, fd->current_pos);
	
	cluster_size = ((fd->fat_desc->sector_per_cluster) * (fd->fat_desc->bytes_per_sector));
	offset = fd->current_pos - (fd->fat_chain_offset * cluster_size);
	sec_offset = (int)(offset / (fd->fat_desc->bytes_per_sector));
	byte_pos = 0;
	
	n_loop = (int)((cluster_size - (offset)) / (fd->fat_desc->bytes_per_sector));
	
	for (i=0; i<=n_loop; i++)
	{
		fat32_device_read_sector(fat32_cluster2lba((fd->fat_desc), 
				fd->current_cluster) + sec_offset + i, 1, buffer);
			
		if (i == 0)
		{
			// to-do, not yet implemented
		}
		
		else if (i>=n_loop)
		{
			// to-do, not yet implemented
		}
		
		else
		{
			// to-do, not yet implemented
		}
	}
	return 0;
}




/**************************************************************************
*                                                                         *
*                                                                         *
*                                                                         *
***************************************************************************/
int8_t fat32_fseek(FAT32_FILE_DESCRIPTOR* fd, uint32_t position)
{
	uint32_t fat_chain_offset;
	uint32_t cluster_num;
	uint16_t i;

	if (fd == NULL)
		return -1;
	
	fat_chain_offset = ((int)(position /((fd->fat_desc->sector_per_cluster) * (fd->fat_desc->bytes_per_sector))));

	//printf_P(PSTR("seek : fat_chain_offset : %ld\r\n"), fat_chain_offset);

	if (fat_chain_offset == 0)
		cluster_num = fd->dir->cluster_pos;
		
	else
	{
		cluster_num = fd->dir->cluster_pos;
		for (i=0; i<=fat_chain_offset - 1; i++)
		{
			cluster_num = fat32_get_next_cluster((fd->fat_desc), cluster_num);
		}
	}
	
	fd->current_pos = position;
	fd->current_cluster = cluster_num;
	fd->fat_chain_offset = fat_chain_offset;
	
	return 0;	
}



/**************************************************************************
*                                                                         *
*                                                                         *
*                                                                         *
***************************************************************************/
int8_t fat32_fwrite(FAT32_FILE_DESCRIPTOR* fd, uint8_t* buffer, uint16_t length)
{
	// to-do, not yet implemented
	return 0;
}



/**************************************************************************
*                                                                         *
*                                                                         *
*                                                                         *
***************************************************************************/
int8_t 		fat32_create(uint8_t* filename)
{
	// to-do, not yet implemented
	return 0;	
}




/**************************************************************************
*                                                                         *
*                                                                         *
*                                                                         *
***************************************************************************/
int8_t 		fat32_delete(uint8_t* filename)
{
	// to-do, not yet implemented
	return 0;
}


/**************************************************************************
*                                                                         *
*                                                                         *
*                                                                         *
***************************************************************************/
FAT32_FILE_DESCRIPTOR* fat32_fopen(FAT32_FAT_DESCRIPTOR* fat_desc,	uint8_t* filename)
{
	// to-do, not yet implemented
	return 0;	
}


/**************************************************************************
*                                                                         *
*                                                                         *
*                                                                         *
***************************************************************************/
int8_t	fat32_fclose(FAT32_FAT_DESCRIPTOR* fat_desc, FAT32_FILE_DESCRIPTOR* fd)
{
	// to-do, not yet implemented
	return 0;	
}



/**************************************************************************
*                                                                         *
*                                                                         *
*                                                                         *
***************************************************************************/
uint32_t	fat32_mkdir(FAT32_FAT_DESCRIPTOR* fat_desc, const uint8_t* path, 
								FAT32_DIR_DESCRIPTOR*	dir_desc)
{
	// to-do, not yet implemented
	return 0;
}






/**************************************************************************
*                                                                         *
*                                                                         *
*                                                                         *
***************************************************************************/
uint32_t	fat32_rmdir(FAT32_FAT_DESCRIPTOR* fat_desc, const uint8_t* path, 
								FAT32_DIR_DESCRIPTOR*	dir_desc)
{
	// to-do, not yet implemented
	return 0;
}





uint32_t get_dword(uint8_t* buffer)
{
	uint32_t temp;
	
	temp = ((uint32_t)buffer[3] << 24);
	temp += ((uint32_t)buffer[2] << 16);
	temp += ((uint32_t)buffer[1] << 8);
	temp +=(uint32_t)buffer[0];

	return temp;
}

//extern inline uint16_t get_word(uint8_t* buffer)
uint16_t get_word(uint8_t* buffer)
{
	uint16_t temp;

	temp = ((uint16_t)buffer[1] << 8);
	temp +=(uint16_t)buffer[0];

	return temp;
}



uint8_t 	mmc_read_sector(uint32_t sector,  uint16_t count, uint8_t* buffer)
{
	return (uint8_t)SDHC_READ(sector, (UINT32)count, (UINT32)buffer);
}
void 		mmc_write_sector(uint32_t sector, uint8_t* buffer)
{
}

