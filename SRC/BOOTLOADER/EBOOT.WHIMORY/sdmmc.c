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

#include <WMRTypes.h>
#include <VFLBuffer.h>
#include <FTL.h>
#include <VFL.h>
#include <FIL.h>
#include <config.h>
#include <WMR_Utils.h>

#include <HSMMCDrv.h>
#include "sdmmc_fat32.h"
#include "keypad.h"
#include "display_epd.h"


#define IMAGE_NB0		0
#define IMAGE_BIN		1
#define IMAGE_LST		2
#define IMAGE_WBF		3

#define BL_HDRSIG_SIZE	7


volatile UINT32	readPtIndex;
volatile UINT8	*g_pDownPt;

extern const PTOC g_pTOC;
static BOOL g_fOEMNotified = FALSE;
static BYTE g_hdr[BL_HDRSIG_SIZE];
static DownloadManifest g_DownloadManifest;
static BYTE g_downloadFilesRemaining = 1;
static DWORD g_dwROMOffset;


static BOOL fatFileExist(const char *sFileName);
static UINT32 fatFileRead(const char *sFileName, UINT8 *pStartBuf);
static UINT32 makeManifestImage(UINT32 dwImageType, MultiBINInfo *pMultiBIN, UINT8 *pBuffer);
static BOOL parsingChainListFile(const char *sFileName, MultiBINInfo *pMultiBIN);
static UINT32 parsingImageFromSD(UINT32 dwImageType, const char *sFileName);

static void HALT(DWORD dwReason);
static BOOL verifyChecksum(DWORD cbRecord, LPBYTE pbRecord, DWORD dwChksum);
static BL_IMAGE_TYPE getImageType(void);
static BOOL checkImageManifest(void);
static BOOL isKernelRegion(DWORD dwRegionStart, DWORD dwRegionLength);
static BOOL downloadBin(LPDWORD pdwImageStart, LPDWORD pdwImageLength, LPDWORD pdwLaunchAddr);
static BOOL downloadNB0(LPDWORD pdwImageStart, LPDWORD pdwImageLength, LPDWORD pdwLaunchAddr);
static BOOL downloadImage(LPDWORD pdwImageStart, LPDWORD pdwImageLength, LPDWORD pdwLaunchAddr);
static void writeImage(void);
#if	0
BOOL fatReadBin(PUCHAR FileName, PULONG JumpAddress, PULONG pulSize, PULONG pulImageAddress);
static void cardBoot(void);
#endif

BOOL InitializeSDMMC(const char *sFileName)
{
	BOOL bRet;

	bRet = SDHC_INIT();
	if (FALSE == bRet)
		return FALSE;

	if (sFileName)
		bRet = fatFileExist(sFileName);

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

BOOL ChooseImageFromSDMMC(BYTE bUpdateKey)
{
	const char file_name[][12] = {
		"BLOCK0  NB0",	// 0 : block0img.nb0 or stepldr.nb0("STEPLDR NB0")
		"EBOOT   BIN",	// 1
		"NK      BIN",	// 2
		"DISPEINKBIN",	// 3
		"CHAIN   LST",	// 4 : chain.lst = (XIPKER.bin(XIPKERNEL.bin) + NK.bin + chain.bin) or nk.bin("NK      BIN")
	}, *pSelFile;
	BYTE KeySelect = bUpdateKey;
	BOOL bRet = FALSE;

	EdbgOutputDebugString("\r\nChoose Download Image:\r\n\r\n");
	EdbgOutputDebugString("B) BLOCK0.NB0\r\n");
	EdbgOutputDebugString("E) EBOOT.BIN\r\n");
	EdbgOutputDebugString("N) NK.BIN\r\n");
	EdbgOutputDebugString("D) DISPEINK.BIN\r\n");
	EdbgOutputDebugString("R) Repair Update(Nand Format And Image Update)\r\n");
	EdbgOutputDebugString("F) Factory Update(Nand Format And Image Update)\r\n");
	EdbgOutputDebugString("C) CHAIN.LST\r\n");
	EdbgOutputDebugString("\r\nEnter your selection: ");

	EPDOutputString("\r\nChoose Download Image:\r\n\r\n");
	EPDOutputString("B) BLOCK0.NB0\r\n");
	EPDOutputString("E) EBOOT.BIN\r\n");
	EPDOutputString("N) NK.BIN\r\n");
	EPDOutputString("D) DISPEINK.BIN\r\n");
	EPDOutputString("R) Repair Update(Nand Format And Image Update)\r\n");
	EPDOutputString("F) Factory Update(Nand Format And Image Update)\r\n");
	EPDOutputString("C) CHAIN.LST\r\n");
	EPDOutputString("\r\nEnter your selection: ");
	EPDOutputFlush();

	while (! (  ( (KeySelect == 'B') || (KeySelect == 'b') ) ||
				( (KeySelect == 'E') || (KeySelect == 'e') ) ||
				( (KeySelect == 'N') || (KeySelect == 'n') ) ||
				( (KeySelect == 'D') || (KeySelect == 'd') ) ||
				( (KeySelect == 'R') || (KeySelect == 'r') ) ||
				( (KeySelect == 'F') || (KeySelect == 'f') ) ||
				( (KeySelect == 'C') || (KeySelect == 'c') ) ))
    {
   		KeySelect = OEMReadDebugByte();
		if ((BYTE)OEM_DEBUG_READ_NODATA == KeySelect)
			KeySelect = GetKeypad2();
	}

	EdbgOutputDebugString("%c\r\n", KeySelect);

	EPDOutputString("%c\r\n", KeySelect);
	EPDOutputFlush();

	g_pDownPt = (UINT8 *)EBOOT_USB_BUFFER_CA_START;
	readPtIndex = (UINT32)EBOOT_USB_BUFFER_CA_START;
	switch (KeySelect)
	{
	case 'B':	// BLOCK0.NB0
	case 'b':	// BLOCK0.NB0
		pSelFile = file_name[0];
		if (fatFileExist(pSelFile))
		{
			EdbgOutputDebugString("+++ Block0.nb0 Read\r\n");
			EPDOutputString("+++ Block0.nb0 Read\r\n");
			EPDOutputFlush();
			bRet = parsingImageFromSD(IMAGE_NB0, pSelFile);
			EdbgOutputDebugString("--- Block0.nb0 Read\r\n");
			EPDOutputString("--- Block0.nb0 Read\r\n");
			EPDOutputFlush();
		}
		break;
	case 'E':	// EBOOT.BIN
	case 'e':	// EBOOT.BIN
		pSelFile = file_name[1];
		if (fatFileExist(pSelFile))
		{
			EdbgOutputDebugString("+++ Eboot.bin Read\r\n");
			EPDOutputString("+++ Eboot.bin Read\r\n");
			EPDOutputFlush();
			bRet = parsingImageFromSD(IMAGE_BIN, pSelFile);
			EdbgOutputDebugString("--- Eboot.bin Read\r\n");
			EPDOutputString("--- Eboot.bin Read\r\n");
			EPDOutputFlush();
		}
		break;
	case 'N':	// NK.BIN
	case 'n':	// NK.BIN
		pSelFile = file_name[2];
		if (fatFileExist(pSelFile))
		{
			EdbgOutputDebugString("+++ NK.bin Read\r\n");
			EPDOutputString("+++ NK.bin Read\r\n");
			EPDOutputFlush();
			bRet = parsingImageFromSD(IMAGE_BIN, pSelFile);
			EdbgOutputDebugString("--- NK.bin Read\r\n");
			EPDOutputString("--- NK.bin Read\r\n");
			EPDOutputFlush();
		}
		break;
	case 'D':	// DISPEINK.BIN
	case 'd':	// DISPEINK.BIN
		pSelFile = file_name[3];
		if (fatFileExist(pSelFile))
		{
			BLOB blob = {0,};

			EdbgOutputDebugString("+++ DispEink.bin Read\r\n");
			EPDOutputString("+++ DispEink.bin Read\r\n");
			EPDOutputFlush();
			blob.cbSize = parsingImageFromSD(IMAGE_WBF, pSelFile);
			blob.pBlobData = (PBYTE)readPtIndex;
			EdbgOutputDebugString("--- DispEink.bin Read\r\n");
			EPDOutputString("--- DispEink.bin Read\r\n");
			EPDOutputFlush();

			EdbgOutputDebugString("+++ DispEink.bin Write\r\n");
			EPDOutputString("+++ DispEink.bin Write\r\n");
			EPDOutputFlush();
			bRet = EPDSerialFlashWrite((void *)&blob);
			EdbgOutputDebugString("--- DispEink.bin Write\r\n");
			EPDOutputString("--- DispEink.bin Write\r\n");
			EPDOutputFlush();

			EdbgOutputDebugString("%s - %s\r\n", pSelFile, bRet ? "Success" : "Failure");

			EPDOutputString("%s - %s\r\n", pSelFile, bRet ? "Success" : "Failure");
			EPDOutputFlush();

			HALT(0);
		}
		return FALSE;
	case 'R':	// NK.BIN
	case 'r':	// NK.BIN
		pSelFile = file_name[2];
		if (fatFileExist(pSelFile))
		{
			EdbgOutputDebugString("+++ Nand Flash Format All\r\n");
			EPDOutputString("+++ Nand Flash Format All\r\n");
			EPDOutputFlush();
			FTL_Close();
			VFL_Close();
			WMR_Format_VFL();
			TOC_Init(DEFAULT_IMAGE_DESCRIPTOR, (IMAGE_TYPE_RAMIMAGE), 0, 0, 0);
			TOC_Write();
			EdbgOutputDebugString("--- Nand Flash Format All\r\n");
			EPDOutputString("--- Nand Flash Format All\r\n");
			EPDOutputFlush();

#if	0
			EdbgOutputDebugString("+++ NK.bin Read\r\n");
			EPDOutputString("+++ NK.bin Read\r\n");
			EPDOutputFlush();
			bRet = parsingImageFromSD(IMAGE_BIN, pSelFile);
			EdbgOutputDebugString("--- NK.bin Read\r\n");
			EPDOutputString("--- NK.bin Read\r\n");
			EPDOutputFlush();
#else
			pSelFile = file_name[3];
			if (fatFileExist(pSelFile))
			{
				BLOB blob = {0,};
				g_pDownPt = (UINT8 *)EBOOT_USB_BUFFER_CA_START;
				readPtIndex = (UINT32)EBOOT_USB_BUFFER_CA_START;
				EdbgOutputDebugString("+++ DispEink.bin Read\r\n");
				EPDOutputString("+++ DispEink.bin Read\r\n");
				EPDOutputFlush();
				blob.cbSize = parsingImageFromSD(IMAGE_WBF, pSelFile);
				blob.pBlobData = (PBYTE)readPtIndex;
				EdbgOutputDebugString("--- DispEink.bin Read\r\n");
				EPDOutputString("--- DispEink.wbf bin\r\n");
				EPDOutputFlush();

				EdbgOutputDebugString("+++ DispEink.bin Write\r\n");
				EPDOutputString("+++ DispEink.bin Write\r\n");
				EPDOutputFlush();
				bRet = EPDSerialFlashWrite((void *)&blob);
				EdbgOutputDebugString("--- DispEink.bin Write\r\n");
				EPDOutputString("--- DispEink.bin Write\r\n");
				EPDOutputFlush();
			}

			pSelFile = file_name[0];
			if (fatFileExist(pSelFile))
			{
				g_pDownPt = (UINT8 *)EBOOT_USB_BUFFER_CA_START;
				readPtIndex = (UINT32)EBOOT_USB_BUFFER_CA_START;
				EdbgOutputDebugString("+++ Block0.nb0 Write\r\n");
				EPDOutputString("+++ Block0.nb0 Write\r\n");
				EPDOutputFlush();
				parsingImageFromSD(IMAGE_NB0, pSelFile);
				writeImage();
				EdbgOutputDebugString("--- Block0.nb0 Write\r\n");
				EPDOutputString("--- Block0.nb0 Write\r\n");
				EPDOutputFlush();
			}

			pSelFile = file_name[1];
			if (fatFileExist(pSelFile))
			{
				g_pDownPt = (UINT8 *)EBOOT_USB_BUFFER_CA_START;
				readPtIndex = (UINT32)EBOOT_USB_BUFFER_CA_START;
				EdbgOutputDebugString("+++ Eboot.bin Write\r\n");
				EPDOutputString("+++ Eboot.bin Write\r\n");
				EPDOutputFlush();
				parsingImageFromSD(IMAGE_BIN, pSelFile);
				writeImage();
				EdbgOutputDebugString("--- Eboot.bin Write\r\n");
				EPDOutputString("--- Eboot.bin Write\r\n");
				EPDOutputFlush();
			}

			EPDOutputString("INFO: Please Reboot.  Halting...\r\n");
			HALT(0);
#endif
		}
		break;
	case 'F':	// (Format All -> DispEink.bin -> Block0.nb0 -> Eboot.bin)
	case 'f':	// (Format All -> DispEink.bin -> Block0.nb0 -> Eboot.bin)
		if (fatFileExist(file_name[3]) && fatFileExist(file_name[0]) && fatFileExist(file_name[1]))
		{
			EdbgOutputDebugString("+++ Nand Flash Format All\r\n");
			EPDOutputString("+++ Nand Flash Format All\r\n");
			EPDOutputFlush();
			VFL_Close();
			WMR_Format_FIL();
			EdbgOutputDebugString("--- Nand Flash Format All\r\n");
			EPDOutputString("--- Nand Flash Format All\r\n");
			EPDOutputFlush();

			pSelFile = file_name[3];
			g_pDownPt = (UINT8 *)EBOOT_USB_BUFFER_CA_START;
			readPtIndex = (UINT32)EBOOT_USB_BUFFER_CA_START;
			{
				BLOB blob = {0,};
				EdbgOutputDebugString("+++ DispEink.bin Read\r\n");
				EPDOutputString("+++ DispEink.bin Read\r\n");
				EPDOutputFlush();
				blob.cbSize = parsingImageFromSD(IMAGE_WBF, pSelFile);
				blob.pBlobData = (PBYTE)readPtIndex;
				EdbgOutputDebugString("--- DispEink.bin Read\r\n");
				EPDOutputString("--- DispEink.wbf bin\r\n");
				EPDOutputFlush();

				EdbgOutputDebugString("+++ DispEink.bin Write\r\n");
				EPDOutputString("+++ DispEink.bin Write\r\n");
				EPDOutputFlush();
				bRet = EPDSerialFlashWrite((void *)&blob);
				EdbgOutputDebugString("--- DispEink.bin Write\r\n");
				EPDOutputString("--- DispEink.bin Write\r\n");
				EPDOutputFlush();
			}

			pSelFile = file_name[0];
			g_pDownPt = (UINT8 *)EBOOT_USB_BUFFER_CA_START;
			readPtIndex = (UINT32)EBOOT_USB_BUFFER_CA_START;
			EdbgOutputDebugString("+++ Block0.nb0 Write\r\n");
			EPDOutputString("+++ Block0.nb0 Write\r\n");
			EPDOutputFlush();
			parsingImageFromSD(IMAGE_NB0, pSelFile);
			writeImage();
			EdbgOutputDebugString("--- Block0.nb0 Write\r\n");
			EPDOutputString("--- Block0.nb0 Write\r\n");
			EPDOutputFlush();

			pSelFile = file_name[1];
			g_pDownPt = (UINT8 *)EBOOT_USB_BUFFER_CA_START;
			readPtIndex = (UINT32)EBOOT_USB_BUFFER_CA_START;
			EdbgOutputDebugString("+++ Eboot.bin Write\r\n");
			EPDOutputString("+++ Eboot.bin Write\r\n");
			EPDOutputFlush();
			parsingImageFromSD(IMAGE_BIN, pSelFile);
			writeImage();
			EdbgOutputDebugString("--- Eboot.bin Write\r\n");
			EPDOutputString("--- Eboot.bin Write\r\n");
			EPDOutputFlush();

			EPDOutputString("INFO: Please Reboot.  Halting...\r\n");
			HALT(0);
		}
		else
			HALT(-800);
		return FALSE;

	case 'C':	// CHAIN.LST
	case 'c':	// CHAIN.LST
		pSelFile = file_name[4];
		if (fatFileExist(pSelFile))
			bRet = parsingImageFromSD(IMAGE_LST, pSelFile);
		break;
#if	0
	case '7':
		cardBoot();
		break;
#endif

	default:
		return FALSE;
	}

	EdbgOutputDebugString("%s - %s\r\n", pSelFile, bRet ? "Success" : "Failure");

	EPDOutputString("%s - %s\r\n", pSelFile, bRet ? "Success" : "Failure");
	EPDOutputFlush();

	return bRet;
}


static BOOL fatFileExist(const char *sFileName)
{
	FAT32_FAT_DESCRIPTOR fat_mmc;
	FAT32_DIR_DESCRIPTOR dir_desc;
	uint8_t dir_buffer[11+1] = {0,};
	uint32_t i, next_cluster;

	fat32_get_descriptor(&fat_mmc, 0);
	for (i=0; i<FAT32_FILES_PER_DIR_MAX; i++)
	{
		if (fat32_get_dir(&fat_mmc, dir_buffer , "", i) == -2)
			break;
		dir_buffer[11] = 0;

		memset((void *)&dir_desc , 0x00, sizeof(dir_desc));
		next_cluster = fat32_find_file(&fat_mmc, "", dir_buffer, &dir_desc);
		if (FAT32_ATTR_DIRECTORY & dir_desc.attribute)
			continue;
		if (dir_desc.size && !strncmp(dir_desc.short_name, sFileName, 11))
			return TRUE;
	}

	EdbgOutputDebugString("ERROR: %s File not found\r\n", sFileName);
	return FALSE;
}
static UINT32 fatFileRead(const char *sFileName, UINT8 *pStartBuf)
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
		if (FAT32_ATTR_DIRECTORY & dir_desc.attribute)
			continue;
		if (dir_desc.size && !strncmp(dir_desc.short_name, sFileName, 11))
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

		dwFileSize = fatFileRead(pCurRegion->szFileName, pFileData);
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
	dwFileSize = fatFileRead(sFileName, (UINT8 *)szLstBuf);
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
static UINT32 parsingImageFromSD(UINT32 dwImageType, const char *sFileName)
{
	MultiBINInfo MultiBin;
	UINT32 dwImageSize = 0;

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
		dwImageSize = fatFileRead(sFileName, (UINT8 *)g_pDownPt);
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

	case IMAGE_WBF:
		dwImageSize = fatFileRead(sFileName, (UINT8 *)g_pDownPt);
		if ((UINT32)-1 == dwImageSize)
			return FALSE;
		g_pDownPt += dwImageSize;
		break;

	default:
		return dwImageSize;
	}

	return dwImageSize;
}

static void HALT(DWORD dwReason)
{
	EdbgOutputDebugString("SpinForever... (%d)\r\n", dwReason);

	EPDOutputString("\r\n\tSystem HALT (%d)\r\n", dwReason);
	EPDOutputFlush();

	VFL_Sync();
	{
		volatile S3C6410_GPIO_REG *pGPIOReg = (S3C6410_GPIO_REG *)OALPAtoVA(S3C6410_BASE_REG_PA_GPIO, FALSE);
		volatile int delay = 5000000;
		while (delay--);
		pGPIOReg->GPCDAT = (pGPIOReg->GPCDAT & ~(0xF<<0)) | (0x0<<3);	// GPC[3] PWRHOLD
	}
	while (1);
}
static BOOL verifyChecksum(DWORD cbRecord, LPBYTE pbRecord, DWORD dwChksum)
{
	// Check the CRC
	DWORD dwCRC = 0;
	DWORD i;

	for (i=0; i<cbRecord; i++)
		dwCRC += *pbRecord ++;
	if (dwCRC != dwChksum)
		EdbgOutputDebugString("ERROR: Checksum failure (expected=0x%x  computed=0x%x)\r\n", dwChksum, dwCRC);

	return (dwCRC == dwChksum);
}

static BL_IMAGE_TYPE getImageType(void)
{
	BL_IMAGE_TYPE rval = BL_IMAGE_TYPE_UNKNOWN;

	// read the 7 byte "magic number"
	if (!OEMReadData(BL_HDRSIG_SIZE, g_hdr))
	{
		EdbgOutputDebugString("\r\nERROR: Unable to read image signature.\r\n");
		return BL_IMAGE_TYPE_NOT_FOUND;
	}

	// The N000FF packet indicates a manifest, which is constructed by Platform 
	// Builder when we're downloading multiple .bin files or an .nb0 file.
	if (!memcmp (g_hdr, "N000FF\x0A", BL_HDRSIG_SIZE))
	{
		EdbgOutputDebugString("\r\nBL_IMAGE_TYPE_MANIFEST\r\n\r\n");
		rval =  BL_IMAGE_TYPE_MANIFEST;
	}
	else if (!memcmp (g_hdr, "X000FF\x0A", BL_HDRSIG_SIZE))
	{
		EdbgOutputDebugString("\r\nBL_IMAGE_TYPE_MULTIXIP\r\n\r\n");
		rval =  BL_IMAGE_TYPE_MULTIXIP;
	}
	else if (!memcmp (g_hdr, "B000FF\x0A", BL_HDRSIG_SIZE))
	{
		EdbgOutputDebugString("\r\nBL_IMAGE_TYPE_BIN\r\n\r\n");
		rval =  BL_IMAGE_TYPE_BIN;
	}
	else if (!memcmp (g_hdr, "S000FF\x0A", BL_HDRSIG_SIZE))
	{
		EdbgOutputDebugString("\r\nBL_IMAGE_TYPE_SIGNED_BIN\r\n\r\n");
		rval =  BL_IMAGE_TYPE_SIGNED_BIN;
	}
	else if (!memcmp (g_hdr, "R000FF\x0A", BL_HDRSIG_SIZE))
	{
		EdbgOutputDebugString("\r\nBL_IMAGE_TYPE_SIGNED_NB0\r\n\r\n");
		rval =  BL_IMAGE_TYPE_SIGNED_NB0;
	}
	else
	{
		EdbgOutputDebugString("\r\nBL_IMAGE_TYPE_UNKNOWN\r\n\r\n");
		rval =  BL_IMAGE_TYPE_UNKNOWN;
	}

	return rval;  
}
static BOOL checkImageManifest(void)
{
	DWORD dwRecChk;

	// read the packet checksum.
	if (!OEMReadData(sizeof(DWORD), (LPBYTE) &dwRecChk))
	{
		EdbgOutputDebugString("\r\nERROR: Unable to read download manifest checksum.\r\n");
		HALT(BLERR_MAGIC);
		return FALSE;
	}

	// read region descriptions (start address and length).
	if (!OEMReadData(sizeof(DWORD), (LPBYTE) &g_DownloadManifest.dwNumRegions) ||
		!OEMReadData((g_DownloadManifest.dwNumRegions * sizeof(RegionInfo)), (LPBYTE) &g_DownloadManifest.Region[0]))
	{
		EdbgOutputDebugString("\r\nERROR: Unable to read download manifest information.\r\n");
		HALT(BLERR_MAGIC);
		return FALSE;
	}

	// verify the packet checksum.
	if (!verifyChecksum((g_DownloadManifest.dwNumRegions * sizeof(RegionInfo)), (LPBYTE)&g_DownloadManifest.Region[0], dwRecChk))
	{
		EdbgOutputDebugString("\r\nERROR: Download manifest packet failed checksum verification.\r\n");
		HALT(BLERR_CHECKSUM);
		return FALSE;
	}

	return TRUE;
}
static BOOL isKernelRegion(DWORD dwRegionStart, DWORD dwRegionLength)
{
	DWORD dwCacheAddress = 0;
	ROMHDR *pROMHeader;
	DWORD dwNumModules = 0;
	TOCentry *plTOC;

	if (dwRegionStart == 0 || dwRegionLength == 0)
		return(FALSE);

	if (*(LPDWORD) OEMMapMemAddr(dwRegionStart, dwRegionStart + ROM_SIGNATURE_OFFSET) != ROM_SIGNATURE)
		return (FALSE);

	// A pointer to the ROMHDR structure lives just past the ROM_SIGNATURE (which is a longword value).  Note that
	// this pointer is remapped since it might be a flash address (image destined for flash), but is actually cached
	// in RAM.
	dwCacheAddress = *(LPDWORD) OEMMapMemAddr(dwRegionStart, dwRegionStart + ROM_SIGNATURE_OFFSET + sizeof(ULONG));
	pROMHeader     = (ROMHDR *) OEMMapMemAddr(dwRegionStart, dwCacheAddress + g_dwROMOffset);

	// Make sure sure are some modules in the table of contents.
	if ((dwNumModules = pROMHeader->nummods) == 0)
		return (FALSE);

	// Locate the table of contents and search for the kernel executable and the TOC immediately follows the ROMHDR.
	plTOC = (TOCentry *)(pROMHeader + 1);

	while (dwNumModules--) {
		LPBYTE pFileName = OEMMapMemAddr(dwRegionStart, (DWORD)plTOC->lpszFileName + g_dwROMOffset);
		if (!strcmp(pFileName, "nk.exe")) {
			return TRUE;
		}
		++plTOC;
	}
	return FALSE;
}
static BOOL downloadBin(LPDWORD pdwImageStart, LPDWORD pdwImageLength, LPDWORD pdwLaunchAddr)
{
	RegionInfo *pCurDownloadFile;
	LPBYTE      lpDest = NULL;
	DWORD       dwImageStart, dwImageLength, dwRecAddr, dwRecLen, dwRecChk;
	DWORD       dwRecNum = 0;

	if (!OEMReadData(sizeof(DWORD), (LPBYTE)&dwImageStart)
		|| !OEMReadData(sizeof(DWORD), (LPBYTE)&dwImageLength))
	{
		EdbgOutputDebugString("Unable to read image start/length\r\n");
		HALT(BLERR_MAGIC);
	}

	// If Platform Builder didn't provide a manifest (i.e., we're only 
	// downloading a single .bin file), manufacture a manifest so we
	// can notify the OEM.
	if (!g_DownloadManifest.dwNumRegions)
	{
		g_DownloadManifest.dwNumRegions             = 1;
		g_DownloadManifest.Region[0].dwRegionStart  = dwImageStart;
		g_DownloadManifest.Region[0].dwRegionLength = dwImageLength;
	}

	// Provide the download manifest to the OEM.
	if (!g_fOEMNotified && g_pOEMMultiBINNotify)
	{
		g_pOEMMultiBINNotify((PDownloadManifest)&g_DownloadManifest);
		g_fOEMNotified = TRUE;
	}

	// Locate the current download manifest entry (current download file).
	pCurDownloadFile = &g_DownloadManifest.Region[g_DownloadManifest.dwNumRegions - g_downloadFilesRemaining];

	// give the OEM a chance to verify memory
	if (!OEMVerifyMemory(pCurDownloadFile->dwRegionStart, pCurDownloadFile->dwRegionLength))
	{
		EdbgOutputDebugString("!OEMVERIFYMEMORY: Invalid image\r\n");
		HALT(BLERR_OEMVERIFY);
	}

	//------------------------------------------------------------------------
	//  Download .bin records
	//------------------------------------------------------------------------
	while ( OEMReadData(sizeof(DWORD), (LPBYTE)&dwRecAddr) &&
			OEMReadData(sizeof(DWORD), (LPBYTE)&dwRecLen)  &&
			OEMReadData(sizeof(DWORD), (LPBYTE)&dwRecChk) )
	{
		// last record of .bin file uses sentinel values for address and checksum.
		if (!dwRecAddr && !dwRecChk)
			break;

		// map the record address (FLASH data is cached, for example)
		lpDest = OEMMapMemAddr(pCurDownloadFile->dwRegionStart, dwRecAddr);

		// read data block
		if (!OEMReadData(dwRecLen, lpDest))
		{
			EdbgOutputDebugString("****** Data record %d corrupted, ABORT!!! ******\r\n", dwRecNum);
			HALT(BLERR_CORRUPTED_DATA);
		}

		if (!verifyChecksum(dwRecLen, lpDest, dwRecChk))
		{
			EdbgOutputDebugString("****** Checksum failure on record %d, ABORT!!! ******\r\n", dwRecNum);
			HALT(BLERR_CHECKSUM);
		}

		// Look for ROMHDR to compute ROM offset.  NOTE: romimage guarantees that the record containing
		// the TOC signature and pointer will always come before the record that contains the ROMHDR contents.
		if (dwRecLen == sizeof(ROMHDR) && (*(LPDWORD)OEMMapMemAddr(pCurDownloadFile->dwRegionStart, pCurDownloadFile->dwRegionStart + ROM_SIGNATURE_OFFSET) == ROM_SIGNATURE))
		{
			DWORD dwTempOffset = (dwRecAddr - *(LPDWORD)OEMMapMemAddr(pCurDownloadFile->dwRegionStart, pCurDownloadFile->dwRegionStart + ROM_SIGNATURE_OFFSET + sizeof(ULONG)));
			ROMHDR *pROMHdr = (ROMHDR *)lpDest;

			// Check to make sure this record really contains the ROMHDR.
			if ((pROMHdr->physfirst == (pCurDownloadFile->dwRegionStart - dwTempOffset)) &&
				(pROMHdr->physlast  == (pCurDownloadFile->dwRegionStart - dwTempOffset + pCurDownloadFile->dwRegionLength)) &&
				(DWORD)(HIWORD(pROMHdr->dllfirst << 16) <= pROMHdr->dlllast) &&
				(DWORD)(LOWORD(pROMHdr->dllfirst << 16) <= pROMHdr->dlllast))
			{
				g_dwROMOffset = dwTempOffset;
				EdbgOutputDebugString("rom_offset=0x%x.\r\n", g_dwROMOffset);
			}
		}

		// verify partial checksum
		OEMShowProgress(dwRecNum++);
	}  // while ( records remaining )
    

	//------------------------------------------------------------------------
	//  Determine the image entry point
	//------------------------------------------------------------------------

	// Does this .bin file contain a TOC?
	if (*(LPDWORD)OEMMapMemAddr(pCurDownloadFile->dwRegionStart, pCurDownloadFile->dwRegionStart + ROM_SIGNATURE_OFFSET) == ROM_SIGNATURE)
	{
		// Contain the kernel?
		if (isKernelRegion(pCurDownloadFile->dwRegionStart, pCurDownloadFile->dwRegionLength))
		{
			*pdwImageStart  = pCurDownloadFile->dwRegionStart;
			*pdwImageLength = pCurDownloadFile->dwRegionLength;
			*pdwLaunchAddr  = dwRecLen;
		}
	}
	// No TOC - not made by romimage.  
	else if (g_DownloadManifest.dwNumRegions == 1)
	{
		*pdwImageStart  = pCurDownloadFile->dwRegionStart;
		*pdwImageLength = pCurDownloadFile->dwRegionLength;
		*pdwLaunchAddr  = dwRecLen;
	}
	else
	{
		// If we're downloading more than one .bin file, it's probably 
		// chain.bin which doesn't have a TOC (and which isn't
		// going to be downloaded on its own) and we should ignore it.
	}

	EdbgOutputDebugString("ImageStart = 0x%x, ImageLength = 0x%x, LaunchAddr = 0x%x\r\n",
		*pdwImageStart, *pdwImageLength, *pdwLaunchAddr);

	return TRUE;
}
static BOOL downloadNB0(LPDWORD pdwImageStart, LPDWORD pdwImageLength, LPDWORD pdwLaunchAddr)
{
	RegionInfo *pCurDownloadFile;
	LPBYTE      lpDest = NULL;

	// Provide the download manifest to the OEM.  This gives the OEM the
	// opportunity to provide start addresses for the .nb0 files (which 
	// don't contain placement information like .bin files do).
	if (!g_fOEMNotified && g_pOEMMultiBINNotify)
	{
		g_pOEMMultiBINNotify((PDownloadManifest)&g_DownloadManifest);
		g_fOEMNotified = TRUE;
	}

	// Locate the current download manifest entry (current download file).
	pCurDownloadFile = &g_DownloadManifest.Region[g_DownloadManifest.dwNumRegions - g_downloadFilesRemaining];

	// give the OEM a chance to verify memory
	if (!OEMVerifyMemory(pCurDownloadFile->dwRegionStart, pCurDownloadFile->dwRegionLength))
	{
		EdbgOutputDebugString("!OEMVERIFYMEMORY: Invalid image\r\n");
		HALT(BLERR_OEMVERIFY);
	}

	//------------------------------------------------------------------------
	//  Download the file
	//
	//  If we're downloading an UNSIGNED .nb0 file, we've already read the 
	//  start of the file in GetImageType().
	//  Copy what we've read so far to the destination, then finish downloading.
	//------------------------------------------------------------------------
	lpDest = OEMMapMemAddr(pCurDownloadFile->dwRegionStart, pCurDownloadFile->dwRegionStart);
	memcpy(lpDest, g_hdr, BL_HDRSIG_SIZE);
	lpDest += BL_HDRSIG_SIZE;
	if (!OEMReadData((pCurDownloadFile->dwRegionLength - BL_HDRSIG_SIZE), lpDest))
	{
		EdbgOutputDebugString("ERROR: failed when reading raw binary file.\r\n");
		HALT(BLERR_CORRUPTED_DATA);
	}

	//------------------------------------------------------------------------
	//  Determine the image entry point
	//------------------------------------------------------------------------
	*pdwImageStart  = pCurDownloadFile->dwRegionStart;
	*pdwLaunchAddr  = pCurDownloadFile->dwRegionStart;
	*pdwImageLength = pCurDownloadFile->dwRegionLength;

	EdbgOutputDebugString("ImageStart = 0x%x, ImageLength = 0x%x, LaunchAddr = 0x%x\r\n",
		*pdwImageStart, *pdwImageLength, *pdwLaunchAddr);

	return TRUE;
}
static BOOL downloadImage(LPDWORD pdwImageStart, LPDWORD pdwImageLength, LPDWORD pdwLaunchAddr)
{
	BOOL  rval = TRUE;
	DWORD dwImageType;

	*pdwImageStart = *pdwImageLength = *pdwLaunchAddr = 0;
	g_downloadFilesRemaining = 1;
	g_fOEMNotified = FALSE;
	memset(&g_DownloadManifest, 0x00, sizeof(DownloadManifest));

	// Download each region (multiple can be sent)
	do
	{
		dwImageType = getImageType();
		switch (dwImageType) 
		{
		case BL_IMAGE_TYPE_MANIFEST:
			// Platform Builder sends a manifest to indicate the following 
			// data consists of multiple .bin files /OR/ one .nb0 file.
			if (!checkImageManifest()) {
				HALT(BLERR_MAGIC);
			}

			// Continue with download of next file
			// +1 to account for the manifest
			g_downloadFilesRemaining = (BYTE)(g_DownloadManifest.dwNumRegions + 1);
			continue;

		case BL_IMAGE_TYPE_BIN:
			rval &= downloadBin(pdwImageStart, pdwImageLength, pdwLaunchAddr);
			break;

		case BL_IMAGE_TYPE_UNKNOWN:
			// Assume files without a "type" header (e.g. raw data) are unsigned .nb0
			rval &= downloadNB0(pdwImageStart, pdwImageLength, pdwLaunchAddr);
			break;

		default:
			// should never get here
			return (FALSE);
		}
	}
	while (--g_downloadFilesRemaining);

	return rval;
}
static void writeImage(void)
{
	DWORD dwImageStart = 0, dwImageLength = 0, dwLaunchAddr = 0, dwpToc = 0;
	if (!downloadImage(&dwImageStart, &dwImageLength, &dwLaunchAddr))
	{
		// error already reported in DownloadImage
		HALT(-900);
	}
	// Check for pTOC signature ("CECE") here, after image in place
	if (*(LPDWORD)OEMMapMemAddr(dwImageStart, dwImageStart + ROM_SIGNATURE_OFFSET) == ROM_SIGNATURE)
	{
		dwpToc = *(LPDWORD)OEMMapMemAddr(dwImageStart, dwImageStart + ROM_SIGNATURE_OFFSET + sizeof(ULONG));
		// need to map the content again since the pointer is going to be in a fixup address
		dwpToc = (DWORD)OEMMapMemAddr(dwImageStart, dwpToc + g_dwROMOffset);
		EdbgOutputDebugString("ROMHDR at Address %Xh\r\n", dwImageStart + ROM_SIGNATURE_OFFSET + sizeof(DWORD)); // right after signature
	}

	if (!WriteRawImageToBootMedia(dwImageStart, dwImageLength, dwLaunchAddr))
	{
		OALMSG(OAL_ERROR, (TEXT("ERROR: OEMLaunch: Failed to store image to Smart Media.\r\n")));
		HALT(-901);
	}
}

#if	0
#include <pshpack1.h>		// byte packing
typedef struct _BINFILE_HEADER {
	UCHAR	SyncBytes[7];
	ULONG	ImageAddress;
	ULONG	ImageLength;
} BINFILE_HEADER, *PBINFILE_HEADER;
typedef struct _BINFILE_RECORD_HEADER {
	ULONG	LoadAddress;
	ULONG	Length;
	ULONG	CheckSum;
} BINFILE_RECORD_HEADER, *PBINFILE_RECORD_HEADER;
#include <poppack.h>
BOOL fatReadBin(PUCHAR FileName, PULONG JumpAddress, PULONG pulSize, PULONG pulImageAddress)
{
	UINT32 dwImageSize = 0;
	ULONG StartAddress, ImageLength, BytesProcessed;
	BINFILE_HEADER BinFileHeader;
	BINFILE_RECORD_HEADER BinRecordHeader;
	LONG CheckSum;
	PUCHAR pData;

	if (FALSE == fatFileExist(FileName))
		return FALSE;

	EdbgOutputDebugString("+++ NK.bin Read\r\n");
	dwImageSize = fatFileRead(FileName, (UINT8 *)g_pDownPt);
	EdbgOutputDebugString("--- NK.bin Read\r\n");
	if ((UINT32)-1 == dwImageSize)
		return FALSE;
	g_pDownPt += dwImageSize;

	if (dwImageSize < sizeof(BINFILE_HEADER) + 2*sizeof(BINFILE_RECORD_HEADER))
	{
		EdbgOutputDebugString("FATReadBin: BIN file size: %u bytes is too small.\r\n", dwImageSize);
		return FALSE;
	}

	SDMMCReadData(sizeof(BINFILE_HEADER), (PUCHAR)&BinFileHeader);
	EdbgOutputDebugString("INFO: Image Address =  : %X\r\n", BinFileHeader.ImageAddress);
	EdbgOutputDebugString("INFO: Image Size =     : %X\r\n", BinFileHeader.ImageLength);

	if (pulImageAddress)
		*pulImageAddress  = BinFileHeader.ImageAddress;
	StartAddress = BinFileHeader.ImageAddress;
	ImageLength  = BinFileHeader.ImageLength;

	BytesProcessed = sizeof(BINFILE_HEADER);
	while (BytesProcessed < (dwImageSize - sizeof(BINFILE_RECORD_HEADER)))
	{
		SDMMCReadData(sizeof(BINFILE_RECORD_HEADER), (PUCHAR)&BinRecordHeader);
		//EdbgOutputDebugString("INFO: Record Address = : %X\r\n", BinRecordHeader.LoadAddress);
		//EdbgOutputDebugString("INFO: Record Length  = : %X\r\n", BinRecordHeader.Length);

		SDMMCReadData(BinRecordHeader.Length, (PUCHAR)BinRecordHeader.LoadAddress);
		CheckSum = 0;
		for (pData=(PUCHAR)BinRecordHeader.LoadAddress;	pData<(PUCHAR)BinRecordHeader.LoadAddress+BinRecordHeader.Length; pData++)
			CheckSum += *pData;
		if ((ULONG)CheckSum != BinRecordHeader.CheckSum)
		{
			EdbgOutputDebugString("FATReadBin: ERROR: Record checksum failure. Aborting. %X != %X\r\n",
				CheckSum, BinRecordHeader.CheckSum);
			return FALSE;
		}
		BytesProcessed += (BinRecordHeader.Length + sizeof(BINFILE_RECORD_HEADER));
	}

	EdbgOutputDebugString("\r\nProcess the termination record which contains the jump address\r\n");
	SDMMCReadData(sizeof(BINFILE_RECORD_HEADER), (PUCHAR)&BinRecordHeader);
	if ((BinRecordHeader.LoadAddress != 0) || (BinRecordHeader.CheckSum != 0))
		EdbgOutputDebugString("FATReadBin: WARNING: Termination record invalid format.\r\n");
	*JumpAddress = BinRecordHeader.Length;
	*pulSize = BinFileHeader.ImageLength;
	EdbgOutputDebugString("JumpAddress(%X), pulSize(%X), pulImageAddress(%X)\r\n",
		*JumpAddress, *pulSize, *pulImageAddress);

	EdbgOutputDebugString("\r\nINFO: Copied BIN file from card to RAM \r\n");

	return TRUE;
}
static void cardBoot(void)
{
	ULONG ulJumpAddr;	// The image boot address.
	ULONG ulImageSize;	// The image size.
	ULONG ulImageAddr;	// The image address.

	if (fatReadBin("NK      BIN", &ulJumpAddr, &ulImageSize, &ulImageAddr))
	{
		EdbgOutputDebugString("Start CE at 0x%X\r\n", ulJumpAddr);
		OEMLaunch(ulImageAddr, ulImageSize, ulJumpAddr, NULL);
	}
}
#endif

