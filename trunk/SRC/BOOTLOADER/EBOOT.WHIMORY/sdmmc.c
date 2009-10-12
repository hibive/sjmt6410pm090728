//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
//
// Use of this source code is subject to the terms of the Microsoft end-user
// license agreement (EULA) under which you licensed this SOFTWARE PRODUCT.
// If you did not accept the terms of the EULA, you are not authorized to use
// this source code. For a copy of the EULA, please see the LICENSE.RTF on your
// install media.
//

#include <windows.h>
#include <bsp.h>
#include "loader.h"

#include <HSMMCDrv.h>
#include "sdmmc_fat32.h"
#include "keypad.h"
#include "display_epd.h"


#define IMAGE_NB0		0
#define IMAGE_BIN		1
#define IMAGE_LST		2


static UINT32 makeManifestImage(UINT32 dwImageType, MultiBINInfo *pMultiBIN, UINT8 *pBuffer);
static BOOL parsingChainListFile(const char *sFileName, MultiBINInfo *pMultiBIN);
static BOOL parsingImageFromSD(UINT32 dwImageType, const char *sFileName);

volatile UINT32	readPtIndex;
volatile UINT8	*g_pDownPt;


BOOL InitializeSDMMC(void)
{
	return SDHC_INIT();
}

BOOL ChooseImageFromSDMMC(void)
{
	BYTE KeySelect = 0;
	const char file_name[][12] = {
		"BLOCK0  NB0",	// 0 : block0img.nb0 or stepldr.nb0("STEPLDR NB0")
		"EBOOT   BIN",	// 1
		"CHAIN   LST",	// 2 : chain.lst = (XIPKER.bin(XIPKERNEL.bin) + NK.bin + chain.bin) or nk.bin("NK      BIN")
	}, *pSelFile;
	BOOL bRet = FALSE;

	EPDWriteEngFont8x16("\r\nChoose Download Image:\r\n\r\n");
	EPDWriteEngFont8x16("0) BLOCK0.NB0\r\n");
	EPDWriteEngFont8x16("1) EBOOT.BIN\r\n");
	EPDWriteEngFont8x16("2) CHAIN.LST\r\n");
	EPDWriteEngFont8x16("3) Power Off ...\r\n");
	EPDWriteEngFont8x16("\r\nEnter your selection: ");
	EPDFlushEngFont8x16();
	while (!(((KeySelect >= '0') && (KeySelect <= '3'))))
	{
		KeySelect = OEMReadDebugByte();
		if ((BYTE)OEM_DEBUG_READ_NODATA == KeySelect)
		{
			switch (GetKeypad())
			{
			case KEY_F13:
				KeySelect = '0';
				break;
			case KEY_F14:
				KeySelect = '1';
				break;
			case KEY_F15:
				KeySelect = '2';
				break;
			case KEY_F16:
				KeySelect = '3';
				break;
			default:
				KeySelect = OEM_DEBUG_READ_NODATA;
				break;
			}
		}
	}
	EPDWriteEngFont8x16("%c\r\n", KeySelect);

	g_pDownPt = (UINT8 *)EBOOT_USB_BUFFER_CA_START;
	readPtIndex = (UINT32)EBOOT_USB_BUFFER_CA_START;

	switch (KeySelect)
	{
	case '0':	// BLOCK0.NB0
		pSelFile = file_name[0];
		bRet = parsingImageFromSD(IMAGE_NB0, pSelFile);
		break;
	case '1':	// EBOOT.BIN
		pSelFile = file_name[1];
		bRet = parsingImageFromSD(IMAGE_BIN, pSelFile);
		break;
	case '2':	// CHAIN.LST
		pSelFile = file_name[2];
		bRet = parsingImageFromSD(IMAGE_LST, pSelFile);
		break;
	case '3':
		return FALSE;
	}
	EPDWriteEngFont8x16("%s - %s\r\n", pSelFile, bRet ? "Success" : "Failure" );
	EPDFlushEngFont8x16();

	return bRet;
}

#pragma optimize ("",off)
BOOL SDMMCReadData(DWORD cbData, LPBYTE pbData)
{
	while (1)
	{
		if ((DWORD)g_pDownPt >= readPtIndex + cbData)
		{
			memcpy((PVOID)pbData, (PVOID)readPtIndex, cbData);

			// Clear Partial Download Memory to 0xFF
			// Unless Gabage data will be written to Boot Media
			memset((PVOID)readPtIndex, 0xFF, cbData);
			readPtIndex += cbData;
			break;
		}
		else if ((DWORD)g_pDownPt == EBOOT_USB_BUFFER_CA_START)
		{
		}
	}

	return TRUE;
}
#pragma optimize ("",on)


static UINT32 FATFileRead(const char *sFileName, UINT8 *pStartBuf)
{
	FAT32_FAT_DESCRIPTOR fat_mmc;
	FAT32_DIR_DESCRIPTOR dir_desc;
	uint8_t dir_buffer[11+1], sector_offset, read_buf[512]={0,};
	uint32_t i, next_cluster;
	UINT8 *pBuffer = pStartBuf;
	BOOL bAlign = !((UINT32)pStartBuf % 4) ? TRUE : FALSE;

	fat32_get_descriptor(&fat_mmc, 0);
	for (i=0; i<FAT32_FILES_PER_DIR_MAX; i++)
	{
		if (fat32_get_dir(&fat_mmc, dir_buffer , "", i) == -2)
			break;
		dir_buffer[11] = 0;

		memset((void *)&dir_desc , 0x00, sizeof(dir_desc));
		next_cluster = fat32_find_file(&fat_mmc, "", dir_buffer, &dir_desc);
		if (!strncmp(dir_desc.short_name, sFileName, 11))
		{
			sector_offset = 0;
			while (next_cluster < 0x0ffffff8)
			{
				if ((uint32_t)(pBuffer - pStartBuf) >= dir_desc.size)
					break;

				if (bAlign)
					fat32_device_read_sector(fat32_cluster2lba(&fat_mmc, next_cluster) + sector_offset, 1, pBuffer);
				else
				{
					fat32_device_read_sector(fat32_cluster2lba(&fat_mmc, next_cluster) + sector_offset, 1, read_buf);
					memcpy(pBuffer, read_buf, 512);
				}
				pBuffer += 512;
				sector_offset++;

				if (sector_offset >= fat_mmc.sector_per_cluster)
				{
					next_cluster = fat32_get_next_cluster(&fat_mmc, next_cluster);
					sector_offset = 0;
				}
			}

			return dir_desc.size;
		}
	}

	EdbgOutputDebugString("ERROR: %s File not found\r\n", sFileName);
	return (UINT32)-1;
}


// read magic number(7) : "N000FF\x0A" == BL_IMAGE_TYPE_MANIFEST
// read packet checksum(4)
// read region number(4)
//  read region info start(4)
//  read region info length(4)
//  read region info filename(260)
static UINT32 makeManifestImage(UINT32 dwImageType, MultiBINInfo *pMultiBIN, UINT8 *pBuffer)
{
	UINT8 *pFileData, *pPtr;
	UINT32 dwFileSize, dwCheckSum, i, j;
	RegionInfo *pCurRegion;

	pFileData = (pBuffer + 7 + 4 + 4 + (pMultiBIN->dwNumRegions*sizeof(RegionInfo)));
	dwCheckSum = 0;
	for (i=0; i<pMultiBIN->dwNumRegions; i++)
	{
		pCurRegion = &pMultiBIN->Region[i];

		dwFileSize = FATFileRead(pCurRegion->szFileName, pFileData);
		if ((UINT32)-1 == dwFileSize)
			return (UINT32)-1;

		if (IMAGE_NB0 == dwImageType)
		{
			pCurRegion->dwRegionStart = 0;
			pCurRegion->dwRegionLength = dwFileSize;
		}
		else
		{
			memcpy((void *)&pCurRegion->dwRegionStart, (pFileData + 7), 4);
			memcpy((void *)&pCurRegion->dwRegionLength, (pFileData + 7 + 4), 4);
		}

		pPtr = (UINT8 *)pCurRegion;
		for (j=0; j<sizeof(RegionInfo); j++)
			dwCheckSum += pPtr[j];

		pFileData += dwFileSize;
	}

	pPtr = pBuffer;
	memcpy((void *)pPtr, "N000FF\x0A", 7);
	pPtr += 7;
	memcpy((void *)pPtr, &dwCheckSum, 4);
	pPtr += 4;
	memcpy((void *)pPtr, &pMultiBIN->dwNumRegions, 4);
	pPtr += 4;
	memcpy((void *)pPtr, &pMultiBIN->Region[0], (pMultiBIN->dwNumRegions*sizeof(RegionInfo)));
	pPtr += (pMultiBIN->dwNumRegions*sizeof(RegionInfo));

	return (UINT32)(pFileData - pBuffer);
}

#define MAX_LST_SIZE	512
static BOOL parsingChainListFile(const char *sFileName, MultiBINInfo *pMultiBIN)
{
	char szLstBuf[MAX_LST_SIZE];
	UINT32 dwFileSize, i, j;
	UINT8 *pPtr;

	memset((void *)szLstBuf, 0, MAX_LST_SIZE);
	dwFileSize = FATFileRead(sFileName, (UINT8 *)szLstBuf);
	if ((UINT32)-1 == dwFileSize || MAX_LST_SIZE < dwFileSize)
		return FALSE;

	for (i=0, pPtr=NULL; i<MAX_LST_SIZE; i++)
	{
		if (('+' == szLstBuf[i]) || ('\t' == szLstBuf[i])
			|| ('\r' == szLstBuf[i]) || ('\n' == szLstBuf[i]))
		{
			if (pPtr)
			{
				if (BL_MAX_BIN_REGIONS > pMultiBIN->dwNumRegions)
					pMultiBIN->Region[pMultiBIN->dwNumRegions++].szFileName[j] = '\0';
				else
					i = MAX_LST_SIZE;	// exit

				pPtr = NULL;
			}

			continue;
		}

		if (NULL == pPtr)
		{
			pPtr = &szLstBuf[i];
			j = 0;
		}

		if (MAX_PATH > j)
			pMultiBIN->Region[pMultiBIN->dwNumRegions].szFileName[j++] = toupper(szLstBuf[i]);
	}

	return TRUE;
}

static BOOL parsingImageFromSD(UINT32 dwImageType, const char *sFileName)
{
	MultiBINInfo MultiBin;
	UINT32 dwImageSize;

	switch (dwImageType)
	{
	case IMAGE_NB0:
		memset((void *)&MultiBin, 0, sizeof(MultiBINInfo));
		MultiBin.dwNumRegions = 1;
		strcpy(MultiBin.Region[0].szFileName, sFileName);

		dwImageSize = makeManifestImage(dwImageType, &MultiBin, (UINT8 *)g_pDownPt);
		if ((UINT32)-1 == dwImageSize)
			return FALSE;
		g_pDownPt += dwImageSize;
		break;

	case IMAGE_BIN:
		dwImageSize = FATFileRead(sFileName, (UINT8 *)g_pDownPt);
		if ((UINT32)-1 == dwImageSize)
			return FALSE;

		g_pDownPt += dwImageSize;
		break;

	case IMAGE_LST:
		memset((void *)&MultiBin, 0, sizeof(MultiBINInfo));
		if (FALSE == parsingChainListFile(sFileName, &MultiBin))
			return FALSE;

		dwImageSize = makeManifestImage(dwImageType, &MultiBin, (UINT8 *)g_pDownPt);
		if ((UINT32)-1 == dwImageSize)
			return FALSE;
		g_pDownPt += dwImageSize;
		break;

	default:
		return FALSE;
	}

	//EdbgOutputDebugString("\r\n\tjhlee build date(%s) time(%s)\r\n", __DATE__, __TIME__);
	return TRUE;
}

