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
#include <ethdbg.h>
#include <fmd.h>
#include "loader.h"
#include "usb.h"

#ifndef	DISPLAY_BROADSHEET
#include "s3c6410_ldi.h"
#include "s3c6410_display_con.h"
#include "InitialImage_rgb16_320x240.h"
#endif	DISPLAY_BROADSHEET

// header files for whimory - hmseo-061028
#include <WMRTypes.h>
#include <VFLBuffer.h>
//#include <WMR.h>
#include <FTL.h>
#include <VFL.h>
#include <FIL.h>
#include <config.h>
#include <WMR_Utils.h>
//#include <WMR_Eboot.h>

#ifdef	OMNIBOOK_VER
// +++ keypad settings +++
#include "keypad.h"
// --- keypad settings ---

// +++ i2c settings +++
#define	PMIC_ADDR	0xCC
extern void IICWriteByte(unsigned long slvAddr, unsigned long addr, unsigned char data);
extern void IICReadByte(unsigned long slvAddr, unsigned long addr, unsigned char *data);
// --- i2c settings ---

#ifdef	DISPLAY_BROADSHEET
// +++ epd settings +++
#include "display_epd.h"
// --- epd settings ---
#endif	DISPLAY_BROADSHEET

// +++ sdmmc settings +++
extern BOOL InitializeSDMMC(void);
extern BOOL ChooseImageFromSDMMC(void);
extern BOOL SDMMCReadData(DWORD cbData, LPBYTE pbData);
// --- sdmmc settings ---
#endif	OMNIBOOK_VER


// For USB Download function
extern BOOL InitializeUSB();
extern void InitializeInterrupt();
extern BOOL UbootReadData(DWORD cbData, LPBYTE pbData);
extern void OTGDEV_SetSoftDisconnect();
// To change Voltage for higher clock.
extern void LTC3714_Init();
extern void LTC3714_VoltageSet(UINT32,UINT32,UINT32);

// For Ethernet Download function.
char *inet_ntoa(DWORD dwIP);
DWORD inet_addr( char *pszDottedD );
BOOL EbootInitEtherTransport(EDBG_ADDR *pEdbgAddr, LPDWORD pdwSubnetMask,
									BOOL *pfJumpImg,
									DWORD *pdwDHCPLeaseTime,
									UCHAR VersionMajor, UCHAR VersionMinor,
									char *szPlatformString, char *szDeviceName,
									UCHAR CPUId, DWORD dwBootFlags);
BOOL EbootEtherReadData(DWORD cbData, LPBYTE pbData);
EDBG_OS_CONFIG_DATA *EbootWaitForHostConnect (EDBG_ADDR *pDevAddr, EDBG_ADDR *pHostAddr);
void	SaveEthernetAddress();

extern BOOL IsValidMBRSector();
extern BOOL CreateMBR();

// Eboot Internal static function
static void InitializeDisplay(void);
static void SpinForever(void);
static USHORT GetIPString(char *);

// Globals
//
DWORD			g_ImageType;
DWORD			g_dwMinImageStart;
MultiBINInfo		g_BINRegionInfo;
PBOOT_CFG		g_pBootCfg;
UCHAR			g_TOC[SECTOR_SIZE];
const PTOC 		g_pTOC = (PTOC)&g_TOC;
DWORD			g_dwImageStartBlock;
DWORD			g_dwTocEntry;
BOOL			g_bBootMediaExist = FALSE;
BOOL			g_bDownloadImage  = TRUE;
BOOL 			g_bWaitForConnect = TRUE;
BOOL			g_bUSBDownload = FALSE;
BOOL 			*g_bCleanBootFlag;
#ifdef	OMNIBOOK_VER
BOOL			g_bSDMMCDownload = FALSE;
BOOL			*g_bHiveCleanFlag;
BOOL			*g_bFormatPartitionFlag;
#endif	OMNIBOOK_VER

//for KITL Configuration Of Args
OAL_KITL_ARGS	*g_KITLConfig;

//for Device ID Of Args
UCHAR			*g_DevID;


EDBG_ADDR 		g_DeviceAddr; // NOTE: global used so it remains in scope throughout download process
                        // since eboot library code keeps a global pointer to the variable provided.

DWORD			wNUM_BLOCKS;

void main(void)
{
    //GPIOTest_Function();
#ifndef	OMNIBOOK_VER
    OTGDEV_SetSoftDisconnect();
#endif	//!OMNIBOOK_VER
    BootloaderMain();

    // Should never get here.
    //
    SpinForever();
}

#ifdef	OMNIBOOK_VER
#define	DEC2HEXCHAR(x)	((9 < x) ? ((x%10)+'A') : (x+'0'))
static void CvtMAC2UUID(PBOOT_CFG pBootCfg, BSP_ARGS *pArgs)
{
	UINT8 bValue, cnt=0;

	pArgs->uuid[cnt++] = 'S';
	pArgs->uuid[cnt++] = 'J';
	pArgs->uuid[cnt++] = 'M';
	pArgs->uuid[cnt++] = 'T';

	bValue = (UINT8)(pBootCfg->EdbgAddr.wMAC[0] & 0x00FF);
	pArgs->uuid[cnt++] = DEC2HEXCHAR(bValue / 16);
	pArgs->uuid[cnt++] = DEC2HEXCHAR(bValue % 16);
	bValue = (UINT8)(pBootCfg->EdbgAddr.wMAC[0] >> 8);
	pArgs->uuid[cnt++] = DEC2HEXCHAR(bValue / 16);
	pArgs->uuid[cnt++] = DEC2HEXCHAR(bValue % 16);

	bValue = (UINT8)(pBootCfg->EdbgAddr.wMAC[1] & 0x00FF);
	pArgs->uuid[cnt++] = DEC2HEXCHAR(bValue / 16);
	pArgs->uuid[cnt++] = DEC2HEXCHAR(bValue % 16);
	bValue = (UINT8)(pBootCfg->EdbgAddr.wMAC[1] >> 8);
	pArgs->uuid[cnt++] = DEC2HEXCHAR(bValue / 16);
	pArgs->uuid[cnt++] = DEC2HEXCHAR(bValue % 16);

	bValue = (UINT8)(pBootCfg->EdbgAddr.wMAC[2] & 0x00FF);
	pArgs->uuid[cnt++] = DEC2HEXCHAR(bValue / 16);
	pArgs->uuid[cnt++] = DEC2HEXCHAR(bValue % 16);
	bValue = (UINT8)(pBootCfg->EdbgAddr.wMAC[2] >> 8);
	pArgs->uuid[cnt++] = DEC2HEXCHAR(bValue / 16);
	pArgs->uuid[cnt++] = DEC2HEXCHAR(bValue % 16);
}
#endif	OMNIBOOK_VER

static USHORT GetIPString(char *szDottedD)
{
	USHORT InChar = 0;
	USHORT cwNumChars = 0;
	
  while(!((InChar == 0x0d) || (InChar == 0x0a)))
  {
      InChar = OEMReadDebugByte();
      if (InChar != OEM_DEBUG_COM_ERROR && InChar != OEM_DEBUG_READ_NODATA)
      {
          // If it's a number or a period, add it to the string.
          //
          if (InChar == '.' || (InChar >= '0' && InChar <= '9'))
          {
                if (cwNumChars < 16)        // IP string cannot over 15.  xxx.xxx.xxx.xxx
              {
                  szDottedD[cwNumChars++] = (char)InChar;
                  OEMWriteDebugByte((BYTE)InChar);
              }
          }
          // If it's a backspace, back up.
          //
          else if (InChar == 8)
          {
              if (cwNumChars > 0)
              {
                  cwNumChars--;
                  OEMWriteDebugByte((BYTE)InChar);
              }
          }
      }
  }

	return cwNumChars;

}

/*
    @func   void | SetIP | Accepts IP address from user input.
    @rdesc  N/A.
    @comm
    @xref
*/

static void SetIP(PBOOT_CFG pBootCfg)
{
    CHAR   szDottedD[16];   // The string used to collect the dotted decimal IP address.
    USHORT cwNumChars = 0;
//    USHORT InChar = 0;

    EdbgOutputDebugString("\r\nEnter new IP address: ");

		cwNumChars = GetIPString(szDottedD);

    // If it's a carriage return with an empty string, don't change anything.
    //
    if (cwNumChars)
    {
        szDottedD[cwNumChars] = '\0';
        pBootCfg->EdbgAddr.dwIP = inet_addr(szDottedD);
    }
}

/*
    @func   void | SetMask | Accepts subnet mask from user input.
    @rdesc  N/A.
    @comm
    @xref
*/
static void SetMask(PBOOT_CFG pBootCfg)
{
    CHAR szDottedD[16]; // The string used to collect the dotted masks.
    USHORT cwNumChars = 0;
//    USHORT InChar = 0;

    EdbgOutputDebugString("\r\nEnter new subnet mask: ");

		cwNumChars = GetIPString(szDottedD);

    // If it's a carriage return with an empty string, don't change anything.
    //
    if (cwNumChars)
    {
        szDottedD[cwNumChars] = '\0';
        pBootCfg->SubnetMask = inet_addr(szDottedD);
    }
}


/*
    @func   void | SetDelay | Accepts an autoboot delay value from user input.
    @rdesc  N/A.
    @comm
    @xref
*/
static void SetDelay(PBOOT_CFG pBootCfg)
{
    CHAR szCount[16];
    USHORT cwNumChars = 0;
    USHORT InChar = 0;

    EdbgOutputDebugString("\r\nEnter maximum number of seconds to delay [1-255]: ");

    while(!((InChar == 0x0d) || (InChar == 0x0a)))
    {
        InChar = OEMReadDebugByte();
        if (InChar != OEM_DEBUG_COM_ERROR && InChar != OEM_DEBUG_READ_NODATA)
        {
            // If it's a number or a period, add it to the string.
            //
            if ((InChar >= '0' && InChar <= '9'))
            {
                if (cwNumChars < 16)
                {
                    szCount[cwNumChars++] = (char)InChar;
                    OEMWriteDebugByte((BYTE)InChar);
                }
            }
            // If it's a backspace, back up.
            //
            else if (InChar == 8)
            {
                if (cwNumChars > 0)
                {
                    cwNumChars--;
                    OEMWriteDebugByte((BYTE)InChar);
                }
            }
        }
    }

    // If it's a carriage return with an empty string, don't change anything.
    //
    if (cwNumChars)
    {
        szCount[cwNumChars] = '\0';
        pBootCfg->BootDelay = atoi(szCount);
        if (pBootCfg->BootDelay > 255)
        {
            pBootCfg->BootDelay = 255;
        }
        else if (pBootCfg->BootDelay < 1)
        {
            pBootCfg->BootDelay = 1;
        }
    }
}


static ULONG mystrtoul(PUCHAR pStr, UCHAR nBase)
{
    UCHAR nPos=0;
    BYTE c;
    ULONG nVal = 0;
    UCHAR nCnt=0;
    ULONG n=0;

    // fulllibc doesn't implement isctype or iswctype, which are needed by
    // strtoul, rather than including coredll code, here's our own simple strtoul.

    if (pStr == NULL)
        return(0);

    for (nPos=0 ; nPos < strlen(pStr) ; nPos++)
    {
        c = tolower(*(pStr + strlen(pStr) - 1 - nPos));
        if (c >= '0' && c <= '9')
            c -= '0';
        else if (c >= 'a' && c <= 'f')
        {
            c -= 'a';
            c  = (0xa + c);
        }

        for (nCnt = 0, n = 1 ; nCnt < nPos ; nCnt++)
        {
            n *= nBase;
        }
        nVal += (n * c);
    }

    return(nVal);
}


static void CvtMAC(USHORT MacAddr[3], char *pszDottedD )
{
    DWORD cBytes;
    char *pszLastNum;
    int atoi (const char *s);
    int i=0;
    BYTE *p = (BYTE *)MacAddr;

    // Replace the dots with NULL terminators
    pszLastNum = pszDottedD;
    for(cBytes = 0 ; cBytes < 6 ; cBytes++)
    {
        while(*pszDottedD != '.' && *pszDottedD != '\0')
        {
            pszDottedD++;
        }
        if (pszDottedD == '\0' && cBytes != 5)
        {
            // zero out the rest of MAC address
            while(i++ < 6)
            {
                *p++ = 0;
            }
            break;
        }
        *pszDottedD = '\0';
        *p++ = (BYTE)(mystrtoul(pszLastNum, 16) & 0xFF);
        i++;
        pszLastNum = ++pszDottedD;
    }
}


static void SetCS8900MACAddress(PBOOT_CFG pBootCfg)
{
    CHAR szDottedD[24];
    USHORT cwNumChars = 0;
    USHORT InChar = 0;

    memset(szDottedD, '0', 24);

    EdbgOutputDebugString ( "\r\nEnter new MAC address in hexadecimal (hh.hh.hh.hh.hh.hh): ");
#ifdef	OMNIBOOK_VER
	EPDOutputString("\r\nEnter new MAC address in hexadecimal (hh.hh.hh.hh.hh.hh): ");
	EPDOutputFlush();
#endif	OMNIBOOK_VER

    while(!((InChar == 0x0d) || (InChar == 0x0a)))
    {
        InChar = OEMReadDebugByte();
#ifdef	OMNIBOOK_VER
		if (((USHORT)OEM_DEBUG_READ_NODATA == InChar))
			InChar = (USHORT)GetKeypad2();
		else
#endif	OMNIBOOK_VER
        InChar = tolower(InChar);
        if (InChar != OEM_DEBUG_COM_ERROR && InChar != OEM_DEBUG_READ_NODATA)
        {
            // If it's a hex number or a period, add it to the string.
            //
            if (InChar == '.' || (InChar >= '0' && InChar <= '9') || (InChar >= 'a' && InChar <= 'f'))
            {
                if (cwNumChars < 17)
                {
                    szDottedD[cwNumChars++] = (char)InChar;
                    OEMWriteDebugByte((BYTE)InChar);
#ifdef	OMNIBOOK_VER
					EPDOutputChar((BYTE)InChar);
					EPDOutputFlush();
#endif	OMNIBOOK_VER
                }
            }
            else if (InChar == 8)       // If it's a backspace, back up.
            {
                if (cwNumChars > 0)
                {
                    cwNumChars--;
                    OEMWriteDebugByte((BYTE)InChar);
#ifdef	OMNIBOOK_VER
					EPDOutputChar((BYTE)InChar);
					EPDOutputFlush();
#endif	OMNIBOOK_VER
                }
            }
        }
    }

    EdbgOutputDebugString ( "\r\n");
#ifdef	OMNIBOOK_VER
	EPDOutputString("\r\n");
#endif	OMNIBOOK_VER

    // If it's a carriage return with an empty string, don't change anything.
    //
    if (cwNumChars)
    {
        szDottedD[cwNumChars] = '\0';
        CvtMAC(pBootCfg->EdbgAddr.wMAC, szDottedD);

        EdbgOutputDebugString("INFO: MAC address set to: %x:%x:%x:%x:%x:%x\r\n",
                  pBootCfg->EdbgAddr.wMAC[0] & 0x00FF, pBootCfg->EdbgAddr.wMAC[0] >> 8,
                  pBootCfg->EdbgAddr.wMAC[1] & 0x00FF, pBootCfg->EdbgAddr.wMAC[1] >> 8,
                  pBootCfg->EdbgAddr.wMAC[2] & 0x00FF, pBootCfg->EdbgAddr.wMAC[2] >> 8);
#ifdef	OMNIBOOK_VER
		CvtMAC2UUID(pBootCfg, pBSPArgs);
		EPDOutputString("INFO: MAC address set to: %B:%B:%B:%B:%B:%B\r\n",
				pBootCfg->EdbgAddr.wMAC[0] & 0x00FF, pBootCfg->EdbgAddr.wMAC[0] >> 8,
				pBootCfg->EdbgAddr.wMAC[1] & 0x00FF, pBootCfg->EdbgAddr.wMAC[1] >> 8,
				pBootCfg->EdbgAddr.wMAC[2] & 0x00FF, pBootCfg->EdbgAddr.wMAC[2] >> 8);
#endif	OMNIBOOK_VER
    }
    else
    {
        EdbgOutputDebugString("WARNING: SetCS8900MACAddress: Invalid MAC address.\r\n");
#ifdef	OMNIBOOK_VER
		EPDOutputString("WARNING: SetCS8900MACAddress: Invalid MAC address.\r\n");
#endif	OMNIBOOK_VER
    }
#ifdef	OMNIBOOK_VER
	EPDOutputFlush();
#endif	OMNIBOOK_VER
}


/*
    @func   BOOL | MainMenu | Manages the Samsung bootloader main menu.
    @rdesc  TRUE == Success and FALSE == Failure.
    @comm
    @xref
*/

static BOOL MainMenu(PBOOT_CFG pBootCfg)
{
    BYTE KeySelect = 0;
    BOOL bConfigChanged = FALSE;
    BOOLEAN bDownload = TRUE;
    UINT32 nSyncRet;

#ifdef	OMNIBOOK_VER
	EPDOutputString("< Build Date : %s %s >\r\n", __DATE__, __TIME__);
	EPDOutputString("\t[Board] Revision(%B)\r\n", pBSPArgs->bBoardRevision);
	EPDOutputString("\t\tAPLL_CLK(%d), ACLK(%d)\r\n", APLL_CLK, S3C6410_ACLK);
	EPDOutputString("\t\tHCLK(%d), PCLK(%d), ECLK(%d)\r\n", S3C6410_HCLK, S3C6410_PCLK, S3C6410_ECLK);
	EPDOutputString("\t[Disp] Revision(%W), Product(%W)\r\n", pBSPArgs->BS_wRevsionCode, pBSPArgs->BS_wProductCode);
	EPDOutputString("\t[Disp] %W : Command Type\r\n", pBSPArgs->CMD_wType);
	EPDOutputString("\t[Disp] %B.%B : Command Version\r\n", pBSPArgs->CMD_bMajor, pBSPArgs->CMD_bMinor);
	EPDOutputString("\t[Disp] %X : Waveform File Size\r\n", pBSPArgs->WFM_dwFileSize);
	EPDOutputString("\t[Disp] %X : Waveform Serial Number\r\n", pBSPArgs->WFM_dwSerialNumber);
	EPDOutputString("\t[Disp] %B : Waveform Run Type\r\n", pBSPArgs->WFM_bRunType);
	EPDOutputString("\t[Disp] %B : Waveform FPL Platform\r\n", pBSPArgs->WFM_bFPLPlatform);
	EPDOutputString("\t[Disp] %W : Waveform FPL Lot\r\n", pBSPArgs->WFM_wFPLLot);
	EPDOutputString("\t[Disp] %B : Waveform Mode Version\r\n", pBSPArgs->WFM_bModeVersion);
	EPDOutputString("\t[Disp] %B : Waveform Version\r\n", pBSPArgs->WFM_bWaveformVersion);
	EPDOutputString("\t[Disp] %B : Waveform Subversion\r\n", pBSPArgs->WFM_bWaveformSubVersion);
	EPDOutputString("\t[Disp] %B : Waveform Type\r\n", pBSPArgs->WFM_bWaveformType);
	EPDOutputString("\t[Disp] %B : Waveform FPL Size\r\n", pBSPArgs->WFM_bFPLSize);
	EPDOutputString("\t[Disp] %B : Waveform MFG Code\r\n", pBSPArgs->WFM_bMFGCode);
	EPDOutputFlush();
#endif	OMNIBOOK_VER

    while(TRUE)
    {
        KeySelect = 0;

        EdbgOutputDebugString ( "\r\nEthernet Boot Loader Configuration:\r\n\r\n");
        EdbgOutputDebugString ( "0) IP address: %s\r\n",inet_ntoa(pBootCfg->EdbgAddr.dwIP));
        EdbgOutputDebugString ( "1) Subnet mask: %s\r\n", inet_ntoa(pBootCfg->SubnetMask));
        EdbgOutputDebugString ( "2) DHCP: %s\r\n", (pBootCfg->ConfigFlags & CONFIG_FLAGS_DHCP)?"Enabled":"Disabled");
        EdbgOutputDebugString ( "3) Boot delay: %d seconds\r\n", pBootCfg->BootDelay);
        EdbgOutputDebugString ( "4) Reset to factory default configuration\r\n");
        EdbgOutputDebugString ( "5) Startup image: %s\r\n", (g_pBootCfg->ConfigFlags & BOOT_TYPE_DIRECT) ? "LAUNCH EXISTING" : "DOWNLOAD NEW");
        EdbgOutputDebugString ( "6) Program disk image into SmartMedia card: %s\r\n", (pBootCfg->ConfigFlags & TARGET_TYPE_NAND)?"Enabled":"Disabled");
        EdbgOutputDebugString ( "7) Program CS8900 MAC address (%B:%B:%B:%B:%B:%B)\r\n",
                               g_pBootCfg->EdbgAddr.wMAC[0] & 0x00FF, g_pBootCfg->EdbgAddr.wMAC[0] >> 8,
                               g_pBootCfg->EdbgAddr.wMAC[1] & 0x00FF, g_pBootCfg->EdbgAddr.wMAC[1] >> 8,
                               g_pBootCfg->EdbgAddr.wMAC[2] & 0x00FF, g_pBootCfg->EdbgAddr.wMAC[2] >> 8);
        EdbgOutputDebugString ( "8) KITL Configuration: %s\r\n", (pBootCfg->ConfigFlags & CONFIG_FLAGS_KITL) ? "ENABLED" : "DISABLED");
//        EdbgOutputDebugString ( "9) Format Boot Media for BinFS\r\n");

        // N.B: we need this option here since BinFS is really a RAM image, where you "format" the media
        // with an MBR. There is no way to parse the image to say it's ment to be BinFS enabled.
        EdbgOutputDebugString ( "A) Format FIL (Erase All Blocks)\r\n");
        EdbgOutputDebugString ( "B) Format VFL (Format FIL + VFL Format)\r\n");
        EdbgOutputDebugString ( "C) Format FTL (Erase FTL Area + FTL Format)\r\n");
        EdbgOutputDebugString ( "E) Erase Physical Block 0\r\n");
#ifndef	OMNIBOOK_VER
        EdbgOutputDebugString ( "F) Make Initial Bad Block Information (Warning)\r\n");
#endif	//!OMNIBOOK_VER
        EdbgOutputDebugString ( "T) MLC Low level test \r\n");
        EdbgOutputDebugString ( "D) Download image now\r\n");
        EdbgOutputDebugString ( "L) LAUNCH existing Boot Media image\r\n");
        EdbgOutputDebugString ( "R) Read Configuration \r\n");
#ifdef	OMNIBOOK_VER
		EdbgOutputDebugString ( "H) Hive Clean on Boot-time      : [%s]\r\n", (pBootCfg->ConfigFlags & BOOT_OPTION_HIVECLEAN) ? "True" : "*False");
		EdbgOutputDebugString ( "P) Format Partition on Boot-time: [%s]\r\n", (pBootCfg->ConfigFlags & BOOT_OPTION_FORMATPARTITION) ? "True" : "*False");
		EdbgOutputDebugString ( "S) DOWNLOAD image now(SDMMCCard)\r\n");
#endif	OMNIBOOK_VER
        EdbgOutputDebugString ( "U) DOWNLOAD image now(USB)\r\n");
        EdbgOutputDebugString ( "W) Write Configuration Right Now\r\n");
#ifdef	DISPLAY_BROADSHEET
		EdbgOutputDebugString ( "X) Epson Instruction byte code update\r\n");
#endif	DISPLAY_BROADSHEET
        EdbgOutputDebugString ( "\r\nEnter your selection: ");

#ifdef	OMNIBOOK_VER
		EPDOutputString("\r\nEthernet Boot Loader Configuration:\r\n\r\n");
		EPDOutputString("7) Program CS8900 MAC address (%B:%B:%B:%B:%B:%B)\r\n",
							g_pBootCfg->EdbgAddr.wMAC[0] & 0x00FF, g_pBootCfg->EdbgAddr.wMAC[0] >> 8,
							g_pBootCfg->EdbgAddr.wMAC[1] & 0x00FF, g_pBootCfg->EdbgAddr.wMAC[1] >> 8,
							g_pBootCfg->EdbgAddr.wMAC[2] & 0x00FF, g_pBootCfg->EdbgAddr.wMAC[2] >> 8);
		EPDOutputString("H) Hive Clean on Boot-time : [%s]\r\n", (pBootCfg->ConfigFlags & BOOT_OPTION_HIVECLEAN) ? "True" : "*False");
		EPDOutputString("P) Format Partition on Boot-time : [%s]\r\n", (pBootCfg->ConfigFlags & BOOT_OPTION_FORMATPARTITION) ? "True" : "*False");
		EPDOutputString("C) Format FTL (Erase FTL Area + FTL Format)\r\n");
		EPDOutputString("L) LAUNCH existing Boot Media image\r\n");
		EPDOutputString("S) DOWNLOAD image now(SDMMCCard)\r\n");
		EPDOutputString("U) DOWNLOAD image now(USB)\r\n");
		EPDOutputString("X) Epson Instruction byte code update\r\n");
		EPDOutputString("\r\nEnter your selection: ");
		EPDOutputFlush();
#endif	OMNIBOOK_VER

        while (! ( ( (KeySelect >= '0') && (KeySelect <= '8') ) ||
                   ( (KeySelect == 'A') || (KeySelect == 'a') ) ||
                   ( (KeySelect == 'B') || (KeySelect == 'b') ) ||
                   ( (KeySelect == 'C') || (KeySelect == 'c') ) ||
                   ( (KeySelect == 'D') || (KeySelect == 'd') ) ||
                   ( (KeySelect == 'E') || (KeySelect == 'e') ) ||
#ifndef	OMNIBOOK_VER
                   ( (KeySelect == 'F') || (KeySelect == 'f') ) ||
#endif	OMNIBOOK_VER
                   ( (KeySelect == 'T') || (KeySelect == 't') ) ||
                   ( (KeySelect == 'L') || (KeySelect == 'l') ) ||
                   ( (KeySelect == 'R') || (KeySelect == 'r') ) ||
#ifdef	OMNIBOOK_VER
                   ( (KeySelect == 'H') || (KeySelect == 'h') ) ||
                   ( (KeySelect == 'P') || (KeySelect == 'p') ) ||
                   ( (KeySelect == 'S') || (KeySelect == 's') ) ||
#endif	OMNIBOOK_VER
                   ( (KeySelect == 'U') || (KeySelect == 'u') ) ||
#ifdef	DISPLAY_BROADSHEET
                   ( (KeySelect == 'X') || (KeySelect == 'x') ) ||
#endif	DISPLAY_BROADSHEET
                   ( (KeySelect == 'W') || (KeySelect == 'w') ) ))
        {
            KeySelect = OEMReadDebugByte();
#ifdef	OMNIBOOK_VER
			if (((BYTE)OEM_DEBUG_READ_NODATA == KeySelect))
			{
				EKEY_DATA KeyData = GetKeypad();
				switch (KeyData)
				{
				case KEY_7:
					KeySelect = '7';	break;
				case KEY_H:
					KeySelect = 'H';	break;
				case KEY_P:
					KeySelect = 'P';	break;
				case KEY_C:
					KeySelect = 'C';	break;
				case KEY_L:
					KeySelect = 'L';	break;
				case KEY_S:
					KeySelect = 'S';	break;
				case KEY_U:
					KeySelect = 'U';	break;
				case KEY_X:
					KeySelect = 'X';	break;
				default:
					KeySelect = OEM_DEBUG_READ_NODATA;
					break;
				}
			}
#endif	OMNIBOOK_VER
        }

        EdbgOutputDebugString ( "%c\r\n", KeySelect);
#ifdef	OMNIBOOK_VER
		EPDOutputString("%c\r\n", KeySelect);
		EPDOutputFlush();
#endif	OMNIBOOK_VER

        switch(KeySelect)
        {
        case '0':           // Change IP address.
            SetIP(pBootCfg);
            pBootCfg->ConfigFlags &= ~CONFIG_FLAGS_DHCP;   // clear DHCP flag
            bConfigChanged = TRUE;
            break;
        case '1':           // Change subnet mask.
            SetMask(pBootCfg);
            bConfigChanged = TRUE;
            break;
        case '2':           // Toggle static/DHCP mode.
            pBootCfg->ConfigFlags = (pBootCfg->ConfigFlags ^ CONFIG_FLAGS_DHCP);
            bConfigChanged = TRUE;
            break;
        case '3':           // Change autoboot delay.
            SetDelay(pBootCfg);
            bConfigChanged = TRUE;
            break;
        case '4':           // Reset the bootloader configuration to defaults.
            OALMSG(TRUE, (TEXT("Resetting default TOC...\r\n")));
            TOC_Init(DEFAULT_IMAGE_DESCRIPTOR, (IMAGE_TYPE_RAMIMAGE|IMAGE_TYPE_BINFS), 0, 0, 0);
            if ( !TOC_Write() ) {
                OALMSG(OAL_WARN, (TEXT("TOC_Write Failed!\r\n")));
            }
            OALMSG(TRUE, (TEXT("...TOC complete\r\n")));
            break;
        case '5':           // Toggle download/launch status.
            pBootCfg->ConfigFlags = (pBootCfg->ConfigFlags ^ BOOT_TYPE_DIRECT);
            bConfigChanged = TRUE;
            break;
        case '6':           // Toggle image storage to Smart Media.
            pBootCfg->ConfigFlags = (pBootCfg->ConfigFlags ^ TARGET_TYPE_NAND);
            bConfigChanged = TRUE;
            break;
        case '7':           // Configure Crystal CS8900 MAC address.
            SetCS8900MACAddress(pBootCfg);
            bConfigChanged = TRUE;
            break;
        case '8':           // Toggle KD
            pBootCfg->ConfigFlags = (pBootCfg->ConfigFlags ^ CONFIG_FLAGS_KITL);
            g_bWaitForConnect = (pBootCfg->ConfigFlags & CONFIG_FLAGS_KITL) ? TRUE : FALSE;
            bConfigChanged = TRUE;
            continue;
            break;
#if 0
        case '9':
            // format the boot media for BinFS
            // N.B: this does not destroy our OEM reserved sections (TOC, bootloaders, etc)
            if ( !g_bBootMediaExist ) {
				OALMSG(OAL_ERROR, (TEXT("ERROR: BootMonitor: boot media does not exist.\r\n")));
                continue;
            }
            // N.B: format offset by # of reserved blocks,
            // decrease the ttl # blocks available by that amount.
            if ( !BP_LowLevelFormat( g_dwImageStartBlock,
                                     wNUM_BLOCKS - g_dwImageStartBlock,
                                     FORMAT_SKIP_BLOCK_CHECK) )
            {
                OALMSG(OAL_ERROR, (TEXT("ERROR: BootMonitor: Low-level boot media format failed.\r\n")));
                continue;
            }
            break;
#endif
	case 'A' :
	case 'a' :
		{
			OALMSG(TRUE, (TEXT(" ++Format FIL (Erase All Blocks)\r\n")));

			if (VFL_Close() != VFL_SUCCESS)
			{
				OALMSG(TRUE, (TEXT("[ERR] VFL_Close() Failure\r\n")));
				break;
			}

			if (WMR_Format_FIL() == FALSE32)
			{
				OALMSG(TRUE, (TEXT("[ERR] WMR_Format_FIL() Failure\r\n")));
				break;
			}

			OALMSG(TRUE, (TEXT("[INF] You can not use VFL before Format VFL\r\n")));

			OALMSG(TRUE, (TEXT(" --Format FIL (Erase All Blocks)\r\n")));
		}
		break;
	case 'B' :
	case 'b' :
		{
			OALMSG(TRUE, (TEXT(" ++Format VFL (Format FIL + VFL Format)\r\n")));

			if (VFL_Close() != VFL_SUCCESS)
			{
				OALMSG(TRUE, (TEXT("[ERR] VFL_Close() Failure\r\n")));
				break;
			}

			if (WMR_Format_VFL() == FALSE32)
			{
				OALMSG(TRUE, (TEXT("[ERR] WMR_Format_VFL() Failure\r\n")));
				break;
			}

			if (VFL_Open() != VFL_SUCCESS)
			{
				OALMSG(TRUE, (TEXT("[ERR] VFL_Open() Failure\r\n")));
				break;
			}

			OALMSG(TRUE, (TEXT(" --Format VFL (Format FIL + VFL Format)\r\n")));
		}
		break;
	case 'C' :
	case 'c' :
		{
			OALMSG(TRUE, (TEXT(" ++Format FTL (Erase FTL Area + FTL Format)\r\n")));

			if (WMR_Format_FTL() == FALSE32)
			{
				OALMSG(TRUE, (TEXT("[ERR] WMR_Format_FTL() Failure\r\n")));
				break;
			}

			if (FTL_Close() != FTL_SUCCESS)
			{
				OALMSG(TRUE, (TEXT("[ERR] FTL_Close() Failure\r\n")));
				break;
			}

			OALMSG(TRUE, (TEXT(" --Format FTL (Erase FTL Area + FTL Format)\r\n")));
		}
		break;
	case 'E' :
	case 'e' :
		{
			LowFuncTbl *pLowFuncTbl;

			OALMSG(TRUE, (TEXT(" ++Erase Physical Block 0\r\n")));

			pLowFuncTbl = FIL_GetFuncTbl();
			pLowFuncTbl->Erase(0, 0, enuBOTH_PLANE_BITMAP);
			if (pLowFuncTbl->Sync(0, &nSyncRet))
			{
				OALMSG(TRUE, (TEXT("[ERR] Erase Block 0 Error\r\n")));
			}

			OALMSG(TRUE, (TEXT(" --Erase Physical Block 0\r\n")));
		}
		break;
#ifndef	OMNIBOOK_VER
	case 'F' :
	case 'f' :
		{
			UINT32 nBlock, nPage;
//			UCHAR *pTemp = (UCHAR *)prayer16bpp;
			UCHAR *pTemp = (UCHAR *)InitialImage_rgb16_320x240;
			UCHAR pDBuf[8192];
			UCHAR pSBuf[256];
			LowFuncTbl *pLowFuncTbl;

			OALMSG(TRUE, (TEXT(" ++Make Initial Bad Block Information (Warning))\r\n")));

			pLowFuncTbl = FIL_GetFuncTbl();

			OALMSG(TRUE, (TEXT("Initial Bad Block Check\r\n")));

			for (nBlock=1; nBlock<SUBLKS_TOTAL; nBlock++)
			{
				IS_CHECK_SPARE_ECC = FALSE32;
				pLowFuncTbl->Read(0, nBlock*PAGES_PER_BLOCK+(PAGES_PER_BLOCK-1), 0x0, enuBOTH_PLANE_BITMAP, NULL, pSBuf, TRUE32, FALSE32);
				IS_CHECK_SPARE_ECC = TRUE32;

				if (pSBuf[0] != 0xff)
				{
					OALMSG(TRUE, (TEXT("Initial Bad Block @ %d Block\r\n"), nBlock));

#if	0
					pLowFuncTbl->Erase(0, nBlock, enuNONE_PLANE_BITMAP);
					pLowFuncTbl->Sync(0, &nSyncRet);

					memset((void *)pSBuf, 0x00, BYTES_PER_SPARE_PAGE);
					IS_CHECK_SPARE_ECC = FALSE32;
					pLowFuncTbl->Write(0, nBlock*PAGES_PER_BLOCK+(PAGES_PER_BLOCK-1), 0x0, enuNONE_PLANE_BITMAP, NULL, pSBuf);
					IS_CHECK_SPARE_ECC = TRUE32;
					if (pLowFuncTbl->Sync(0, &nSyncRet))	// Write Error
					{
						OALMSG(TRUE, (TEXT("Bad marking Write Error @ %d Block\r\n"), nBlock));
					}
#endif
				}
#if	1
				else
				{
					pLowFuncTbl->Erase(0, nBlock, enuNONE_PLANE_BITMAP);
					if (pLowFuncTbl->Sync(0, &nSyncRet))	// Erase Error
					{
						OALMSG(TRUE, (TEXT("Erase Error @ %d Block -> Bad Marking\r\n"), nBlock));

						memset((void *)pSBuf, 0x00, BYTES_PER_SPARE_PAGE);
						IS_CHECK_SPARE_ECC = FALSE32;
						pLowFuncTbl->Write(0, nBlock*PAGES_PER_BLOCK+(PAGES_PER_BLOCK-1), 0x0, enuNONE_PLANE_BITMAP, NULL, pSBuf);
						IS_CHECK_SPARE_ECC = TRUE32;
						if (pLowFuncTbl->Sync(0, &nSyncRet))	// Write Error
						{
							OALMSG(TRUE, (TEXT("Bad marking Write Error @ %d Block\r\n"), nBlock));
						}
					}
					else
					{
						for (nPage=0; nPage<PAGES_PER_BLOCK; nPage++)		// Write and Read Test
						{
							// Write Page
							memset((void *)pSBuf, 0xff, BYTES_PER_SPARE_PAGE);
							pLowFuncTbl->Write(0, nBlock*PAGES_PER_BLOCK+nPage, LEFT_SECTOR_BITMAP_PAGE, enuNONE_PLANE_BITMAP, pTemp, pSBuf);
							if (pLowFuncTbl->Sync(0, &nSyncRet))	// Write Error
							{
								OALMSG(TRUE, (TEXT("Write Error @ %d Block %d Page -> Bad Marking\r\n"), nBlock, nPage));

								pLowFuncTbl->Erase(0, nBlock, enuNONE_PLANE_BITMAP);
								pLowFuncTbl->Sync(0, &nSyncRet);

								memset((void *)pSBuf, 0x00, BYTES_PER_SPARE_PAGE);
								IS_CHECK_SPARE_ECC = FALSE32;
								pLowFuncTbl->Write(0, nBlock*PAGES_PER_BLOCK+(PAGES_PER_BLOCK-1), 0x0, enuNONE_PLANE_BITMAP, NULL, pSBuf);
								IS_CHECK_SPARE_ECC = TRUE32;
								if (pLowFuncTbl->Sync(0, &nSyncRet))	// Write Error
								{
									OALMSG(TRUE, (TEXT("Bad marking Write Error @ %d Block\r\n"), nBlock));
								}
								break;	// Bad Block
							}

							// Read Page
							memset((void *)pSBuf, 0xff, BYTES_PER_SPARE_PAGE);
							if (pLowFuncTbl->Read(0, nBlock*PAGES_PER_BLOCK+nPage, LEFT_SECTOR_BITMAP_PAGE, enuNONE_PLANE_BITMAP, pDBuf, pSBuf, 0, 0))
							{
								OALMSG(TRUE, (TEXT("Read Error @ %d Block %d Page -> Bad Marking\r\n"), nBlock, nPage));

								pLowFuncTbl->Erase(0, nBlock, enuNONE_PLANE_BITMAP);
								pLowFuncTbl->Sync(0, &nSyncRet);

								memset((void *)pSBuf, 0x00, BYTES_PER_SPARE_PAGE);
								IS_CHECK_SPARE_ECC = FALSE32;
								pLowFuncTbl->Write(0, nBlock*PAGES_PER_BLOCK+(PAGES_PER_BLOCK-1), 0x0, enuNONE_PLANE_BITMAP, NULL, pSBuf);
								IS_CHECK_SPARE_ECC = TRUE32;
								if (pLowFuncTbl->Sync(0, &nSyncRet))	// Write Error
								{
									OALMSG(TRUE, (TEXT("Bad marking Write Error @ %d Block\r\n"), nBlock));
								}
								break;	// Bad Block
							}
						}
					}
				}
#endif
			OALMSG(TRUE, (TEXT(".")));

			}

			OALMSG(TRUE, (TEXT(" --Make Initial Bad Block Information (Warning)\r\n")));
		}
		break;
#endif	OMNIBOOK_VER
        case 'T':           // Download? Yes.
        case 't':
			MLC_LowLevelTest();
		break;
        case 'D':           // Download? Yes.
        case 'd':
            bDownload = TRUE;
            goto MENU_DONE;
        case 'L':           // Download? No.
        case 'l':
            bDownload = FALSE;
            goto MENU_DONE;
        case 'R':
        case 'r':
			TOC_Read();
			TOC_Print();
            // TODO
            break;
#ifdef	OMNIBOOK_VER
		case 'H':	// Toggle
		case 'h':
			pBootCfg->ConfigFlags= (pBootCfg->ConfigFlags ^ BOOT_OPTION_HIVECLEAN);
			//bConfigChanged = TRUE;
			break;
		case 'P':	// Toggle
		case 'p':
			pBootCfg->ConfigFlags= (pBootCfg->ConfigFlags ^ BOOT_OPTION_FORMATPARTITION);
			//bConfigChanged = TRUE;
			break;
		case 'S':
		case 's':
			if (FALSE == InitializeSDMMC())
			{
				OALMSG(OAL_ERROR, (L"ERROR: InitializeSDMMC call failed\r\n"));;
				SpinForever();
			}
			g_bSDMMCDownload = TRUE;
			bDownload = TRUE;
			goto MENU_DONE;
#endif	OMNIBOOK_VER
        case 'U':           // Download? No.
        case 'u':
            //bConfigChanged = TRUE;  // Write to NAND too frequently causes wearout
#ifdef	OMNIBOOK_VER
			{
				volatile S3C6410_SYSCON_REG *pSysConReg = (S3C6410_SYSCON_REG *)OALPAtoVA(S3C6410_BASE_REG_PA_SYSCON, FALSE);
				UINT8 data[2];

				// ELDO3, ELDO8 On
				data[0] = 0x32 | (1<<3);	// [bit3] ELDO3 - VDD_OTGI(1.2V)
				IICWriteByte(PMIC_ADDR, 0x00, data[0]);
				data[0] = 0x91 | (1<<5);	// [bit5] ELDO8 - VDD_OTG(3.3V)
				IICWriteByte(PMIC_ADDR, 0x01, data[0]);

				InitializeInterrupt();
			    pSysConReg->HCLK_GATE |= (1<<20);
				OTGDEV_SetSoftDisconnect();
				InitializeUSB();
			}
#else	//!OMNIBOOK_VER
/*        if (!InitializeUSB())
			{
				DEBUGMSG(1, (TEXT("OEMPlatformInit: Failed to initialize USB.\r\n")));
				return(FALSE);
        }*/
#endif	OMNIBOOK_VER

			g_bUSBDownload = TRUE;
            bDownload = TRUE;
            goto MENU_DONE;
        case 'W':           // Configuration Write
        case 'w':
            if (!TOC_Write())
            {
                OALMSG(OAL_WARN, (TEXT("WARNING: MainMenu: Failed to store updated eboot configuration in flash.\r\n")));
            }
            else
            {
                OALMSG(OAL_INFO, (TEXT("Successfully Written\r\n")));
                bConfigChanged = FALSE;
            }
            break;
#ifdef	DISPLAY_BROADSHEET
		case 'X':	// BroadSheet(Epson) Instruction byte code update
		case 'x':
			if (EPDSerialFlashWrite())
			{
				EdbgOutputDebugString("Successfully Written\r\n");
				SpinForever();
			}
			break;
#endif	DISPLAY_BROADSHEET
        default:
            break;
        }
    }

MENU_DONE:

    // If eboot settings were changed by user, save them to flash.
    //
    if (bConfigChanged && !TOC_Write())
    {
        OALMSG(OAL_WARN, (TEXT("WARNING: MainMenu: Failed to store updated bootloader configuration to flash.\r\n")));
    }

    return(bDownload);
}


/*
    @func   BOOL | OEMPlatformInit | Initialize the Samsung SMD6410 platform hardware.
    @rdesc  TRUE = Success, FALSE = Failure.
    @comm
    @xref
*/
BOOL OEMPlatformInit(void)
{
	ULONG BootDelay;
	UINT8 KeySelect;
	UINT32 dwStartTime, dwPrevTime, dwCurrTime;
	BOOL bResult = FALSE;
	FlashInfo flashInfo;
#ifdef	OMNIBOOK_VER
	UINT8 data[2];
	volatile S3C6410_GPIO_REG *pGPIOReg = (S3C6410_GPIO_REG *)OALPAtoVA(S3C6410_BASE_REG_PA_GPIO, FALSE);
#endif	OMNIBOOK_VER

	OALMSG(OAL_FUNC, (TEXT("+OEMPlatformInit.\r\n")));

#ifdef	OMNIBOOK_VER
	// +++ i2c settings +++
	// 1.30V(B), 1.20V(9), 1.10V(7), 1.05V(6)
	data[0] = 0x79;	// DVSARM2[7:4]=1.10V, DVSARM1[3:0]=1.20V
	IICWriteByte(PMIC_ADDR, 0x04, data[0]);
	data[0] = 0xB6;	// DVSARM4[7:4]=1.30V, DVSARM3[3:0]=1.05V
	IICWriteByte(PMIC_ADDR, 0x05, data[0]);
	data[0] = 0x9B;	// DVSINT2[7:4]=1.20V, DVSINT1[3:0]=1.30V
	IICWriteByte(PMIC_ADDR, 0x06, data[0]);
	// --- i2c settings ---
#endif	OMNIBOOK_VER

	// Check if Current ARM speed is not matched to Target Arm speed
	// then To get speed up, set Voltage
#if (APLL_CLK == CLK_1332MHz)
    LTC3714_Init();
    LTC3714_VoltageSet(1,1200,100);     // ARM
    LTC3714_VoltageSet(2,1300,100);     // INT
#endif
#if (TARGET_ARM_CLK == CLK_800MHz)
    LTC3714_Init();
    LTC3714_VoltageSet(1,1300,100);     // ARM
    LTC3714_VoltageSet(2,1200,100);     // INT
#endif

#ifdef	OMNIBOOK_VER
	// +++ gpio settings +++
	// GPC[3] PWRHOLD
	pGPIOReg->GPCCON = (pGPIOReg->GPCCON & ~(0xF<<12)) | (0x1<<12);	// Output
	pGPIOReg->GPCPUD = (pGPIOReg->GPCPUD & ~(0x3<<6)) | (0x0<<6);	// Pull-up/down disable
	// GPC[3] PWRHOLD
	pGPIOReg->GPCDAT = (pGPIOReg->GPCDAT & ~(0xF<<0)) | (0x1<<3);

	// GPA[7] LED_R, GPA[6] LED_B
	pGPIOReg->GPACON = (pGPIOReg->GPACON & ~(0xFF<<24)) | (0x11<<24);	// Output
	pGPIOReg->GPAPUD = (pGPIOReg->GPAPUD & ~(0xF<<12)) | (0x0<<12);		// Pull-up/down disable
	// GPA[7] LED_R#, GPA[6] LED_B On
	pGPIOReg->GPADAT = (pGPIOReg->GPADAT & ~(0x3<<6)) | (1<<6);

	// GPE[2] TOUCH_SLP, GPE[1] WCDMA_PD#, GPE[0] WLAN_PD#
	pGPIOReg->GPECON = (pGPIOReg->GPECON & ~(0xFFF<<0)) | (0x111<<0);	// Output
	pGPIOReg->GPEPUD = (pGPIOReg->GPEPUD & ~(0x3F<<0)) | (0x0<<0);		// Pull-up/down disable
	// GPE[2] TOUCH_SLP
	pGPIOReg->GPEDAT = (pGPIOReg->GPEDAT & ~(0x1<<2)) | (1<<2);
	// --- gpio settings ---
#endif	OMNIBOOK_VER

    EdbgOutputDebugString("Microsoft Windows CE Bootloader for the Samsung SMDK6410 Version %d.%d Built %s\r\n\r\n",
	EBOOT_VERSION_MAJOR, EBOOT_VERSION_MINOR, __DATE__);

	// Initialize the BSP args structure.
	OALArgsInit(pBSPArgs);

	g_bCleanBootFlag = (BOOL*)OALArgsQuery(BSP_ARGS_QUERY_CLEANBOOT) ;
#ifdef	OMNIBOOK_VER
	g_bHiveCleanFlag = (BOOL*)OALArgsQuery(BSP_ARGS_QUERY_HIVECLEAN);
	g_bFormatPartitionFlag = (BOOL*)OALArgsQuery(BSP_ARGS_QUERY_FORMATPART);
#endif	OMNIBOOK_VER
	g_KITLConfig = (OAL_KITL_ARGS *)OALArgsQuery(OAL_ARGS_QUERY_KITL);
	g_DevID = (UCHAR *)OALArgsQuery( OAL_ARGS_QUERY_DEVID);

	// Initialize the display.
	InitializeDisplay();

#ifdef	OMNIBOOK_VER
	// GPM[2:0] BOARD_REV
	pGPIOReg->GPMCON = (pGPIOReg->GPMCON & ~(0xFFF<<0)) | (0x000<<0);	// Input
	pGPIOReg->GPMPUD = (pGPIOReg->GPMPUD & ~(0x3F<<0)) | (0x2A<<0);		// Pull-up enable
	pBSPArgs->bBoardRevision = (BYTE)(pGPIOReg->GPMDAT & 0x7);

	InitializeInterrupt();
#endif	OMNIBOOK_VER

	g_dwImageStartBlock = IMAGE_START_BLOCK;

	// Try to initialize the boot media block driver and BinFS partition.
	//
	///*
    OALMSG(TRUE, (TEXT("BP_Init\r\n")));
	if (!BP_Init((LPBYTE)BINFS_RAM_START, BINFS_RAM_LENGTH, NULL, NULL, NULL) )
	{
		OALMSG(OAL_WARN, (TEXT("WARNING: OEMPlatformInit failed to initialize Boot Media.\r\n")));
		g_bBootMediaExist = FALSE;
	}
	else
	{			
		g_bBootMediaExist = TRUE;
	}

	// Get flash info
	if (!FMD_GetInfo(&flashInfo))
	{
		OALMSG(OAL_ERROR, (L"ERROR: BLFlashDownload: FMD_GetInfo call failed\r\n"));
	}

	wNUM_BLOCKS = flashInfo.dwNumBlocks;
	RETAILMSG(1, (TEXT("wNUM_BLOCKS : %d(0x%x) \r\n"), wNUM_BLOCKS, wNUM_BLOCKS));
//	stDeviceInfo = GetNandInfo();

	// Try to retrieve TOC (and Boot config) from boot media
	if ( !TOC_Read( ) )
	{
		// use default settings
		TOC_Init(DEFAULT_IMAGE_DESCRIPTOR, (IMAGE_TYPE_RAMIMAGE), 0, 0, 0);
	}
#ifdef	OMNIBOOK_VER
	CvtMAC2UUID(g_pBootCfg, (BSP_ARGS *)pBSPArgs);
	EdbgOutputDebugString("pBSPArgs->uuid : %s\r\n", pBSPArgs->uuid);
	EdbgOutputDebugString("pBSPArgs->deviceId : %s\r\n", pBSPArgs->deviceId);
#endif	OMNIBOOK_VER

	// Display boot message - user can halt the autoboot by pressing any key on the serial terminal emulator.
	BootDelay = g_pBootCfg->BootDelay;

	if (g_pBootCfg->ConfigFlags & BOOT_TYPE_DIRECT)
	{
		OALMSG(TRUE, (TEXT("Press [ENTER] to launch image stored on boot media, or [SPACE] to enter boot monitor.\r\n")));
		OALMSG(TRUE, (TEXT("\r\nInitiating image launch in %d seconds. "),BootDelay--));
	}
	else
	{
		OALMSG(TRUE, (TEXT("Press [ENTER] to download image stored on boot media, or [SPACE] to enter boot monitor.\r\n")));
		OALMSG(TRUE, (TEXT("\r\nInitiating image download in %d seconds. "),BootDelay--));
	}

	dwStartTime = OEMEthGetSecs();
	dwPrevTime  = dwStartTime;
	dwCurrTime  = dwStartTime;
	KeySelect   = 0;

#ifndef	OMNIBOOK_VER
        if (!InitializeUSB())
        {
            DEBUGMSG(1, (TEXT("OEMPlatformInit: Failed to initialize USB.\r\n")));
            return(FALSE);
        }
#endif	//!OMNIBOOK_VER

#ifdef	OMNIBOOK_VER
	InitializeKeypad();
#endif	OMNIBOOK_VER

	// Allow the user to break into the bootloader menu.
	while((dwCurrTime - dwStartTime) < g_pBootCfg->BootDelay)
	{
		KeySelect = OEMReadDebugByte();
#ifdef	OMNIBOOK_VER
		if (((BYTE)OEM_DEBUG_READ_NODATA == KeySelect))
		{
			EKEY_DATA KeyData = GetKeypad();
			if (KeyData == (KEY_HOLD | KEY_HOME))
				KeySelect = 0x20;
		}
#endif	OMNIBOOK_VER

		if ((KeySelect == 0x20) || (KeySelect == 0x0d))
		{
			break;
		}

		dwCurrTime = OEMEthGetSecs();

		if (dwCurrTime > dwPrevTime)
		{
			int i, j;

			// 1 Second has elapsed - update the countdown timer.
			dwPrevTime = dwCurrTime;

            // for text alignment
			if (BootDelay < 9)
            {
				i = 11;
            }
			else if (BootDelay < 99)
            {
				i = 12;
            }
			else if (BootDelay < 999)
            {
				i = 13;
            }
            else 
            {
                i = 14;     //< we don't care about this value when BootDelay over 1000 (1000 seconds)
            }
                

			for(j = 0; j < i; j++)
			{
				OEMWriteDebugByte((BYTE)0x08); // print back space
			}

			EdbgOutputDebugString ( "%d seconds. ", BootDelay--);
		}
	}

	OALMSG(OAL_INFO, (TEXT("\r\n")));

	// Boot or enter bootloader menu.
	//
	switch(KeySelect)
	{
	case 0x20: // Boot menu.
		g_pBootCfg->ConfigFlags &= ~BOOT_OPTION_CLEAN;		// Always clear CleanBoot Flags before Menu
		g_bDownloadImage = MainMenu(g_pBootCfg);
		break;
	case 0x00: // Fall through if no keys were pressed -or-
	case 0x0d: // the user cancelled the countdown.
	default:
		if (g_pBootCfg->ConfigFlags & BOOT_TYPE_DIRECT)
		{
			OALMSG(TRUE, (TEXT("\r\nLaunching image from boot media ... \r\n")));
			g_bDownloadImage = FALSE;
		}
		else
		{
			OALMSG(TRUE, (TEXT("\r\nStarting auto-download ... \r\n")));
			g_bDownloadImage = TRUE;
		}
		break;
	}

	//Update  Argument Area Value(KITL, Clean Option)

	if(g_pBootCfg->ConfigFlags &  BOOT_OPTION_CLEAN)
	{
		*g_bCleanBootFlag =TRUE;
	}

	else
	{
		*g_bCleanBootFlag =FALSE;
	}
#ifdef	OMNIBOOK_VER
	if (g_pBootCfg->ConfigFlags &  BOOT_OPTION_HIVECLEAN)
	{
		*g_bHiveCleanFlag = TRUE;
	}
	else
	{
		*g_bHiveCleanFlag = FALSE;
	}
	if (g_pBootCfg->ConfigFlags &  BOOT_OPTION_FORMATPARTITION)
	{
		*g_bFormatPartitionFlag = TRUE;
	}
	else
	{
		*g_bFormatPartitionFlag = FALSE;
	}
#endif	OMNIBOOK_VER
	if(g_pBootCfg->ConfigFlags & CONFIG_FLAGS_KITL)
	{
		g_KITLConfig->flags=OAL_KITL_FLAGS_ENABLED | OAL_KITL_FLAGS_VMINI;
	}
	else
	{
		g_KITLConfig->flags&=~(OAL_KITL_FLAGS_ENABLED | OAL_KITL_FLAGS_VMINI);
	}

	g_KITLConfig->ipAddress= g_pBootCfg->EdbgAddr.dwIP;
	g_KITLConfig->ipMask     = g_pBootCfg->SubnetMask;
	g_KITLConfig->devLoc.IfcType    = Internal;
	g_KITLConfig->devLoc.BusNumber  = 0;
	g_KITLConfig->devLoc.LogicalLoc = BSP_BASE_REG_PA_CS8900A_IOBASE;

	memcpy(g_KITLConfig->mac, g_pBootCfg->EdbgAddr.wMAC, 6);

	OALKitlCreateName(BSP_DEVICE_PREFIX, g_KITLConfig->mac, g_DevID);

	if ( !g_bDownloadImage )
	{
		// User doesn't want to download image - load it from the boot media.
		// We could read an entire nk.bin or nk.nb0 into ram and jump.
		if ( !VALID_TOC(g_pTOC) )
		{
			OALMSG(OAL_ERROR, (TEXT("OEMPlatformInit: ERROR_INVALID_TOC, can not autoboot.\r\n")));
			return FALSE;
		}

		switch (g_ImageType)
		{
		case IMAGE_TYPE_STEPLDR:
			OALMSG(TRUE, (TEXT("Don't support launch STEPLDR.bin\r\n")));
			break;
		case IMAGE_TYPE_LOADER:
			OALMSG(TRUE, (TEXT("Don't support launch EBOOT.bin\r\n")));
			break;
		case IMAGE_TYPE_RAMIMAGE:
			OTGDEV_SetSoftDisconnect();
			OALMSG(TRUE, (TEXT("OEMPlatformInit: IMAGE_TYPE_RAMIMAGE\r\n")));
#ifdef	OMNIBOOK_VER
			EPDDisplayBitmap(BITMAP_BOOTUP);	// BootUp
#endif	OMNIBOOK_VER
			if ( !ReadOSImageFromBootMedia( ) )
			{
				OALMSG(OAL_ERROR, (TEXT("OEMPlatformInit ERROR: Failed to load kernel region into RAM.\r\n")));
				return FALSE;
			}
			break;

		default:
			OALMSG(OAL_ERROR, (TEXT("OEMPlatformInit ERROR: unknown image type: 0x%x \r\n"), g_ImageType));
			return FALSE;
		}
	}

	// Configure Ethernet controller.
#ifdef	OMNIBOOK_VER
	if (g_bDownloadImage && (g_bUSBDownload == FALSE) && (g_bSDMMCDownload == FALSE))
#else	//!OMNIBOOK_VER
	if ( g_bDownloadImage && (g_bUSBDownload == FALSE))
#endif	OMNIBOOK_VER
	{
		if (!InitEthDevice(g_pBootCfg))
		{
			OALMSG(OAL_ERROR, (TEXT("ERROR: OEMPlatformInit: Failed to initialize Ethernet controller.\r\n")));
			goto CleanUp;
		}
	}

	bResult = TRUE;

	CleanUp:

	OALMSG(OAL_FUNC, (TEXT("_OEMPlatformInit.\r\n")));

	return(bResult);
}


/*
    @func   DWORD | OEMPreDownload | Complete pre-download tasks - get IP address, initialize TFTP, etc.
    @rdesc  BL_DOWNLOAD = Platform Builder is asking us to download an image, BL_JUMP = Platform Builder is requesting we jump to an existing image, BL_ERROR = Failure.
    @comm
    @xref
*/
DWORD OEMPreDownload(void)
{
    BOOL  bGotJump = FALSE;
    DWORD dwDHCPLeaseTime = 0;
    PDWORD pdwDHCPLeaseTime = &dwDHCPLeaseTime;
    DWORD dwBootFlags = 0;

    OALMSG(OAL_FUNC, (TEXT("+OEMPreDownload.\r\n")));

    // Create device name based on Ethernet address (this is how Platform Builder identifies this device).
    //
    OALKitlCreateName(BSP_DEVICE_PREFIX, pBSPArgs->kitl.mac, pBSPArgs->deviceId);
    OALMSG(OAL_INFO, (L"INFO: *** Device Name '%hs' ***\r\n", pBSPArgs->deviceId));

#ifdef	OMNIBOOK_VER
	if (g_bUSBDownload == FALSE && g_bSDMMCDownload == FALSE)
#else	//!OMNIBOOK_VER
	if ( g_bUSBDownload == FALSE)
#endif	OMNIBOOK_VER
	{
		// If the user wants to use a static IP address, don't request an address
		// from a DHCP server.  This is done by passing in a NULL for the DHCP
		// lease time variable.  If user specified a static IP address, use it (don't use DHCP).
		//
		if (!(g_pBootCfg->ConfigFlags & CONFIG_FLAGS_DHCP))
		{
			// Static IP address.
			pBSPArgs->kitl.ipAddress  = g_pBootCfg->EdbgAddr.dwIP;
			pBSPArgs->kitl.ipMask     = g_pBootCfg->SubnetMask;
			pBSPArgs->kitl.flags     &= ~OAL_KITL_FLAGS_DHCP;
			pdwDHCPLeaseTime = NULL;
			OALMSG(OAL_INFO, (TEXT("INFO: Using static IP address %s.\r\n"), inet_ntoa(pBSPArgs->kitl.ipAddress)));
			OALMSG(OAL_INFO, (TEXT("INFO: Using subnet mask %s.\r\n"),       inet_ntoa(pBSPArgs->kitl.ipMask)));
		}
		else
		{
			pBSPArgs->kitl.ipAddress = 0;
			pBSPArgs->kitl.ipMask    = 0;
		}

		if ( !g_bDownloadImage)
		{
			return(BL_JUMP);
		}

		// Initialize the the TFTP transport.
		//
		g_DeviceAddr.dwIP = pBSPArgs->kitl.ipAddress;
		memcpy(g_DeviceAddr.wMAC, pBSPArgs->kitl.mac, (3 * sizeof(UINT16)));
		g_DeviceAddr.wPort = 0;

		if (!EbootInitEtherTransport(&g_DeviceAddr,
									 &pBSPArgs->kitl.ipMask,
									 &bGotJump,
									 pdwDHCPLeaseTime,
									 EBOOT_VERSION_MAJOR,
									 EBOOT_VERSION_MINOR,
									 BSP_DEVICE_PREFIX,
									 pBSPArgs->deviceId,
									 EDBG_CPU_ARM720,
									 dwBootFlags))
		{
			OALMSG(OAL_ERROR, (TEXT("ERROR: OEMPreDownload: Failed to initialize Ethernet connection.\r\n")));
			return(BL_ERROR);
		}


		// If the user wanted a DHCP address, we presumably have it now - save it for the OS to use.
		//
		if (g_pBootCfg->ConfigFlags & CONFIG_FLAGS_DHCP)
		{
			// DHCP address.
			pBSPArgs->kitl.ipAddress  = g_DeviceAddr.dwIP;
			pBSPArgs->kitl.flags     |= OAL_KITL_FLAGS_DHCP;
		}

		OALMSG(OAL_FUNC, (TEXT("_OEMPreDownload.\r\n")));
	}
	else if (g_bUSBDownload == TRUE) // jylee
	{
		OALMSG(TRUE, (TEXT("Please send the Image through USB.\r\n")));
	}
#ifdef	OMNIBOOK_VER
	else if (g_bSDMMCDownload == TRUE)
	{
		OALMSG(TRUE, (TEXT("Please choose the Image on SDMMCCard.\r\n")));
		if (FALSE == ChooseImageFromSDMMC())
		{
			OALMSG(OAL_ERROR, (L"ERROR: ChooseImageFromSDMMC call failed\r\n"));;
			SpinForever();
		}
	}
#endif	OMNIBOOK_VER

    return(bGotJump ? BL_JUMP : BL_DOWNLOAD);
}


/*
    @func   BOOL | OEMReadData | Generically read download data (abstracts actual transport read call).
    @rdesc  TRUE = Success, FALSE = Failure.
    @comm
    @xref
*/
BOOL OEMReadData(DWORD dwData, PUCHAR pData)
{
	BOOL ret;
	//DWORD i;
   	OALMSG(OAL_FUNC, (TEXT("+OEMReadData.\r\n")));
	//OALMSG(TRUE, (TEXT("\r\nINFO: dwData = 0x%x, pData = 0x%x \r\n"), dwData, pData));

#ifdef	OMNIBOOK_VER
	if (g_bUSBDownload == FALSE && g_bSDMMCDownload == FALSE)
#else	//!OMNIBOOK_VER
	if ( g_bUSBDownload == FALSE )
#endif	OMNIBOOK_VER
	{
		ret = EbootEtherReadData(dwData, pData);
	}
	else if ( g_bUSBDownload == TRUE ) // jylee

	{
		ret = UbootReadData(dwData, pData);
	}
#ifdef	OMNIBOOK_VER
	else if (g_bSDMMCDownload == TRUE)
	{
		ret = SDMMCReadData(dwData, pData);
	}
#endif	OMNIBOOK_VER

/*
	OALMSG(TRUE, (TEXT("\r\n")));
	for ( i = 0; i < dwData; i++ )
	{
		OALMSG(TRUE, (TEXT("<%x>"), *(pData+i)));
		if ( i % 16 == 15 )
			OALMSG(TRUE, (TEXT("\r\n")));
	}
	OALMSG(TRUE, (TEXT("\r\n")));
*/
	return(ret);
}

void OEMReadDebugString(CHAR * szString)
{
//    static CHAR szString[16];   // The string used to collect the dotted decimal IP address.
    USHORT cwNumChars = 0;
    USHORT InChar = 0;

    while(!((InChar == 0x0d) || (InChar == 0x0a)))
    {
        InChar = OEMReadDebugByte();
        if (InChar != OEM_DEBUG_COM_ERROR && InChar != OEM_DEBUG_READ_NODATA)
        {
            if ((InChar >= 'a' && InChar <='z') || (InChar >= 'A' && InChar <= 'Z') || (InChar >= '0' && InChar <= '9'))
            {
                if (cwNumChars < 16)
                {
                    szString[cwNumChars++] = (char)InChar;
                    OEMWriteDebugByte((BYTE)InChar);
                }
            }
            // If it's a backspace, back up.
            //
            else if (InChar == 8)
            {
                if (cwNumChars > 0)
                {
                    cwNumChars--;
                    OEMWriteDebugByte((BYTE)InChar);
                }
            }
        }
    }

    // If it's a carriage return with an empty string, don't change anything.
    //
    if (cwNumChars)
    {
        szString[cwNumChars] = '\0';
        EdbgOutputDebugString("\r\n");
    }
}


/*
    @func   void | OEMShowProgress | Displays download progress for the user.
    @rdesc  N/A.
    @comm
    @xref
*/
void OEMShowProgress(DWORD dwPacketNum)
{
    OALMSG(OAL_FUNC, (TEXT("+OEMShowProgress.\r\n")));
}


/*
    @func   void | OEMLaunch | Executes the stored/downloaded image.
    @rdesc  N/A.
    @comm
    @xref
*/

void OEMLaunch( DWORD dwImageStart, DWORD dwImageLength, DWORD dwLaunchAddr, const ROMHDR *pRomHdr )
{
    DWORD dwPhysLaunchAddr;
    EDBG_ADDR EshellHostAddr;
    EDBG_OS_CONFIG_DATA *pCfgData;

    OALMSG(OAL_FUNC, (TEXT("+OEMLaunch.\r\n")));


    // If the user requested that a disk image (stored in RAM now) be written to the SmartMedia card, so it now.
    //
    if (g_bDownloadImage && (g_pBootCfg->ConfigFlags & TARGET_TYPE_NAND))
    {
        // Since this platform only supports RAM images, the image cache address is the same as the image RAM address.
        //

        switch (g_ImageType)
        {
        	case IMAGE_TYPE_STEPLDR:
		        if (!WriteRawImageToBootMedia(dwImageStart, dwImageLength, dwLaunchAddr))
		        {
            		OALMSG(OAL_ERROR, (TEXT("ERROR: OEMLaunch: Failed to store image to Smart Media.\r\n")));
            		goto CleanUp;
        		}
		        OALMSG(TRUE, (TEXT("INFO: Step loader image stored to Smart Media.  Please Reboot.  Halting...\r\n")));
#ifdef	OMNIBOOK_VER
				SpinForever();
#else	//!OMNIBOOK_VER
	        	while(1)
	        	{
            		// Wait...
	        	}
#endif	OMNIBOOK_VER
        		break;

            case IMAGE_TYPE_LOADER:
				g_pTOC->id[0].dwLoadAddress = dwImageStart;
				g_pTOC->id[0].dwTtlSectors = FILE_TO_SECTOR_SIZE(dwImageLength);
		        if (!WriteRawImageToBootMedia(dwImageStart, dwImageLength, dwLaunchAddr))
		        {
            		OALMSG(OAL_ERROR, (TEXT("ERROR: OEMLaunch: Failed to store image to Smart Media.\r\n")));
            		goto CleanUp;
        		}
				if (dwLaunchAddr && (g_pTOC->id[0].dwJumpAddress != dwLaunchAddr))
				{
					g_pTOC->id[0].dwJumpAddress = dwLaunchAddr;
#if 0 // don't write TOC after download Eboot
					if ( !TOC_Write() ) {
	            		EdbgOutputDebugString("*** OEMLaunch ERROR: TOC_Write failed! Next boot may not load from disk *** \r\n");
					}
	        		TOC_Print();
#endif	// by hmseo - 061123

				}
		        OALMSG(TRUE, (TEXT("INFO: Eboot image stored to Smart Media.  Please Reboot.  Halting...\r\n")));
#ifdef	OMNIBOOK_VER
				SpinForever();
#else	//!OMNIBOOK_VER
		        while(1)
		        {
            		// Wait...
        		}
#endif	OMNIBOOK_VER
                break;

            case IMAGE_TYPE_RAMIMAGE:
				g_pTOC->id[g_dwTocEntry].dwLoadAddress = dwImageStart;
				g_pTOC->id[g_dwTocEntry].dwTtlSectors = FILE_TO_SECTOR_SIZE(dwImageLength);
		        if (!WriteOSImageToBootMedia(dwImageStart, dwImageLength, dwLaunchAddr))
		        {
            		OALMSG(OAL_ERROR, (TEXT("ERROR: OEMLaunch: Failed to store image to Smart Media.\r\n")));
            		goto CleanUp;
        		}

				if (dwLaunchAddr && (g_pTOC->id[g_dwTocEntry].dwJumpAddress != dwLaunchAddr))
				{
					g_pTOC->id[g_dwTocEntry].dwJumpAddress = dwLaunchAddr;
					if ( !TOC_Write() ) {
	            		EdbgOutputDebugString("*** OEMLaunch ERROR: TOC_Write failed! Next boot may not load from disk *** \r\n");
					}
	        		TOC_Print();
				}
				else
				{
					dwLaunchAddr= g_pTOC->id[g_dwTocEntry].dwJumpAddress;
					EdbgOutputDebugString("INFO: using TOC[%d] dwJumpAddress: 0x%x\r\n", g_dwTocEntry, dwLaunchAddr);
				}

                break;
        }
    }
    else if(g_bDownloadImage)
    {
        switch (g_ImageType)
        {
        	case IMAGE_TYPE_STEPLDR:
		        OALMSG(TRUE, (TEXT("Stepldr image can't launch from ram.\r\n")));
		        OALMSG(TRUE, (TEXT("You should program it into flash.\r\n")));
		        SpinForever();
				break;
            case IMAGE_TYPE_LOADER:
		        OALMSG(TRUE, (TEXT("Eboot image can't launch from ram.\r\n")));
		        OALMSG(TRUE, (TEXT("You should program it into flash.\r\n")));
		        SpinForever();
		        break;
            default:
            	break;
    	}
    }

    OALMSG(1, (TEXT("waitforconnect\r\n")));
    // Wait for Platform Builder to connect after the download and send us IP and port settings for service
    // connections - also sends us KITL flags.  This information is used later by the OS (KITL).
    //
    if (~g_bUSBDownload & g_bDownloadImage & g_bWaitForConnect)
    {
        memset(&EshellHostAddr, 0, sizeof(EDBG_ADDR));

        g_DeviceAddr.dwIP  = pBSPArgs->kitl.ipAddress;
        memcpy(g_DeviceAddr.wMAC, pBSPArgs->kitl.mac, (3 * sizeof(UINT16)));
        g_DeviceAddr.wPort = 0;

        if (!(pCfgData = EbootWaitForHostConnect(&g_DeviceAddr, &EshellHostAddr)))
        {
            OALMSG(OAL_ERROR, (TEXT("ERROR: OEMLaunch: EbootWaitForHostConnect failed.\r\n")));
            goto CleanUp;
        }

        // If the user selected "passive" KITL (i.e., don't connect to the target at boot time), set the
        // flag in the args structure so the OS image can honor it when it boots.
        //
        if (pCfgData->KitlTransport & KTS_PASSIVE_MODE)
        {
            pBSPArgs->kitl.flags |= OAL_KITL_FLAGS_PASSIVE;
        }
	}


    // save ethernet address for ethernet kitl // added by jjg 06.09.18
    SaveEthernetAddress();

    // If a launch address was provided, we must have downloaded the image, save the address in case we
    // want to jump to this image next time.  If no launch address was provided, retrieve the last one.
    //
	if (dwLaunchAddr && (g_pTOC->id[g_dwTocEntry].dwJumpAddress != dwLaunchAddr))
	{
		g_pTOC->id[g_dwTocEntry].dwJumpAddress = dwLaunchAddr;
	}
	else
	{
		dwLaunchAddr= g_pTOC->id[g_dwTocEntry].dwJumpAddress;
		OALMSG(OAL_INFO, (TEXT("INFO: using TOC[%d] dwJumpAddress: 0x%x\r\n"), g_dwTocEntry, dwLaunchAddr));
	}

#ifdef	OMNIBOOK_VER
	{
		unsigned char buf[2];
		IICReadByte(PMIC_ADDR, 0x00, buf);
		if ((buf[0] & (1<<3)))	// [bit3] ELDO3 - VDD_OTGI(1.2V)
		{
			buf[0] &= ~(1<<3);	// off
			IICWriteByte(PMIC_ADDR, 0x00, buf[0]);
			OALMSG(TRUE, (TEXT("VDD_OTGI(1.2V) OFF\r\n")));
		}
		IICReadByte(PMIC_ADDR, 0x01, buf);
		if ((buf[0] & (1<<5)))	// [bit5] ELDO8 - VDD_OTG(3.3V)
		{
			buf[0] &= ~(1<<5);	// off
			IICWriteByte(PMIC_ADDR, 0x01, buf[0]);
			OALMSG(TRUE, (TEXT("VDD_OTG(3.3V) OFF\r\n")));
		}
	}
#endif	OMNIBOOK_VER

    // Jump to downloaded image (use the physical address since we'll be turning the MMU off)...
    //
    dwPhysLaunchAddr = (DWORD)OALVAtoPA((void *)dwLaunchAddr);
    OALMSG(TRUE, (TEXT("INFO: OEMLaunch: Jumping to Physical Address 0x%Xh (Virtual Address 0x%Xh)...\r\n\r\n\r\n"), dwPhysLaunchAddr, dwLaunchAddr));

    // Jump...
    //
    Launch(dwPhysLaunchAddr);


CleanUp:

    OALMSG(TRUE, (TEXT("ERROR: OEMLaunch: Halting...\r\n")));
    SpinForever();
}


//------------------------------------------------------------------------------
//
//  Function Name:  OEMVerifyMemory( DWORD dwStartAddr, DWORD dwLength )
//  Description..:  This function verifies the passed address range lies
//                  within a valid region of memory. Additionally this function
//                  sets the g_ImageType if the image is a boot loader.
//  Inputs.......:  DWORD           Memory start address
//                  DWORD           Memory length
//  Outputs......:  BOOL - true if verified, false otherwise
//
//------------------------------------------------------------------------------

BOOL OEMVerifyMemory( DWORD dwStartAddr, DWORD dwLength )
{

    OALMSG(OAL_FUNC, (TEXT("+OEMVerifyMemory.\r\n")));

    // Is the image being downloaded the stepldr?
    if ((dwStartAddr >= STEPLDR_RAM_IMAGE_BASE) &&
        ((dwStartAddr + dwLength - 1) < (STEPLDR_RAM_IMAGE_BASE + STEPLDR_RAM_IMAGE_SIZE)))
    {
        OALMSG(OAL_INFO, (TEXT("Stepldr image\r\n")));
        g_ImageType = IMAGE_TYPE_STEPLDR;     // Stepldr image.
        return TRUE;
    }
    // Is the image being downloaded the bootloader?
    else if ((dwStartAddr >= EBOOT_STORE_ADDRESS) &&
        ((dwStartAddr + dwLength - 1) < (EBOOT_STORE_ADDRESS + EBOOT_STORE_MAX_LENGTH)))
    {
        OALMSG(OAL_INFO, (TEXT("Eboot image\r\n")));
        g_ImageType = IMAGE_TYPE_LOADER;     // Eboot image.
        return TRUE;
    }

    // Is it a ram image?
//    else if ((dwStartAddr >= ROM_RAMIMAGE_START) &&
//        ((dwStartAddr + dwLength - 1) < (ROM_RAMIMAGE_START + ROM_RAMIMAGE_SIZE)))  //for supporting MultipleXIP
    else if (dwStartAddr >= ROM_RAMIMAGE_START) 
    {
        OALMSG(OAL_INFO, (TEXT("RAM image\r\n")));
        g_ImageType = IMAGE_TYPE_RAMIMAGE;
        return TRUE;
    }
	else if (!dwStartAddr && !dwLength)
	{
        OALMSG(TRUE, (TEXT("Don't support raw image\r\n")));
		g_ImageType = IMAGE_TYPE_RAWBIN;
    	return FALSE;
	}

    // HACKHACK: get around MXIP images with funky addresses
    OALMSG(TRUE, (TEXT("BIN image type unknow\r\n")));

    OALMSG(OAL_FUNC, (TEXT("_OEMVerifyMemory.\r\n")));

    return FALSE;
}

/*
    @func   void | OEMMultiBINNotify | Called by blcommon to nofity the OEM code of the number, size, and location of one or more BIN regions,
                                       this routine collects the information and uses it when temporarily caching a flash image in RAM prior to final storage.
    @rdesc  N/A.
    @comm
    @xref
*/
void OEMMultiBINNotify(const PMultiBINInfo pInfo)
{
    BYTE nCount;
//	DWORD g_dwMinImageStart;

    OALMSG(OAL_FUNC, (TEXT("+OEMMultiBINNotify.\r\n")));

    if (!pInfo || !pInfo->dwNumRegions)
    {
        OALMSG(OAL_WARN, (TEXT("WARNING: OEMMultiBINNotify: Invalid BIN region descriptor(s).\r\n")));
        return;
    }

	if (!pInfo->Region[0].dwRegionStart && !pInfo->Region[0].dwRegionLength)
	{
    	return;
	}

    g_dwMinImageStart = pInfo->Region[0].dwRegionStart;

    OALMSG(TRUE, (TEXT("\r\nDownload BIN file information:\r\n")));
    OALMSG(TRUE, (TEXT("-----------------------------------------------------\r\n")));
    for (nCount = 0 ; nCount < pInfo->dwNumRegions ; nCount++)
    {
        OALMSG(TRUE, (TEXT("[%d]: Base Address=0x%x  Length=0x%x\r\n"),
            nCount, pInfo->Region[nCount].dwRegionStart, pInfo->Region[nCount].dwRegionLength));
        if (pInfo->Region[nCount].dwRegionStart < g_dwMinImageStart)
        {
            g_dwMinImageStart = pInfo->Region[nCount].dwRegionStart;
            if (g_dwMinImageStart == 0)
            {
                OALMSG(OAL_WARN, (TEXT("WARNING: OEMMultiBINNotify: Bad start address for region (%d).\r\n"), nCount));
                return;
            }
        }
    }

    memcpy((LPBYTE)&g_BINRegionInfo, (LPBYTE)pInfo, sizeof(MultiBINInfo));

    OALMSG(TRUE, (TEXT("-----------------------------------------------------\r\n")));
    OALMSG(OAL_FUNC, (TEXT("_OEMMultiBINNotify.\r\n")));
}

#if	0	// by dodan2
/////////////////////// START - Stubbed functions - START //////////////////////////////
/*
    @func   void | SC_WriteDebugLED | Write to debug LED.
    @rdesc  N/A.
    @comm
    @xref
*/

void SC_WriteDebugLED(USHORT wIndex, ULONG dwPattern)
{
    // Stub - needed by NE2000 EDBG driver...
    //
}


ULONG HalSetBusDataByOffset(IN BUS_DATA_TYPE BusDataType,
                            IN ULONG BusNumber,
                            IN ULONG SlotNumber,
                            IN PVOID Buffer,
                            IN ULONG Offset,
                            IN ULONG Length)
{
    return(0);
}


ULONG
HalGetBusDataByOffset(IN BUS_DATA_TYPE BusDataType,
                      IN ULONG BusNumber,
                      IN ULONG SlotNumber,
                      IN PVOID Buffer,
                      IN ULONG Offset,
                      IN ULONG Length)
{
    return(0);
}


BOOLEAN HalTranslateBusAddress(IN INTERFACE_TYPE  InterfaceType,
                               IN ULONG BusNumber,
                               IN PHYSICAL_ADDRESS BusAddress,
                               IN OUT PULONG AddressSpace,
                               OUT PPHYSICAL_ADDRESS TranslatedAddress)
{

    // All accesses on this platform are memory accesses...
    //
    if (AddressSpace)
        *AddressSpace = 0;

    // 1:1 mapping...
    //
    if (TranslatedAddress)
    {
        *TranslatedAddress = BusAddress;
        return(TRUE);
    }

    return(FALSE);
}


PVOID MmMapIoSpace(IN PHYSICAL_ADDRESS PhysicalAddress,
                   IN ULONG NumberOfBytes,
                   IN BOOLEAN CacheEnable)
{
    DWORD dwAddr = PhysicalAddress.LowPart;

    if (CacheEnable)
        dwAddr &= ~CACHED_TO_UNCACHED_OFFSET;
    else
        dwAddr |= CACHED_TO_UNCACHED_OFFSET;

    return((PVOID)dwAddr);
}


VOID MmUnmapIoSpace(IN PVOID BaseAddress,
                    IN ULONG NumberOfBytes)
{
}

VOID WINAPI SetLastError(DWORD dwErrCode)
{
}
/////////////////////// END - Stubbed functions - END //////////////////////////////
#endif

/*
    @func   PVOID | GetKernelExtPointer | Locates the kernel region's extension area pointer.
    @rdesc  Pointer to the kernel's extension area.
    @comm
    @xref
*/
PVOID GetKernelExtPointer(DWORD dwRegionStart, DWORD dwRegionLength)
{
    DWORD dwCacheAddress = 0;
    ROMHDR *pROMHeader;
    DWORD dwNumModules = 0;
    TOCentry *pTOC;

    if (dwRegionStart == 0 || dwRegionLength == 0)
        return(NULL);

    if (*(LPDWORD) OEMMapMemAddr (dwRegionStart, dwRegionStart + ROM_SIGNATURE_OFFSET) != ROM_SIGNATURE)
        return NULL;

    // A pointer to the ROMHDR structure lives just past the ROM_SIGNATURE (which is a longword value).  Note that
    // this pointer is remapped since it might be a flash address (image destined for flash), but is actually cached
    // in RAM.
    //
    dwCacheAddress = *(LPDWORD) OEMMapMemAddr (dwRegionStart, dwRegionStart + ROM_SIGNATURE_OFFSET + sizeof(ULONG));
    pROMHeader     = (ROMHDR *) OEMMapMemAddr (dwRegionStart, dwCacheAddress);

    // Make sure sure are some modules in the table of contents.
    //
    if ((dwNumModules = pROMHeader->nummods) == 0)
        return NULL;

    // Locate the table of contents and search for the kernel executable and the TOC immediately follows the ROMHDR.
    //
    pTOC = (TOCentry *)(pROMHeader + 1);

    while(dwNumModules--) {
        LPBYTE pFileName = OEMMapMemAddr(dwRegionStart, (DWORD)pTOC->lpszFileName);
        if (!strcmp((const char *)pFileName, "nk.exe")) {
            return ((PVOID)(pROMHeader->pExtensions));
        }
        ++pTOC;
    }
    return NULL;
}


/*
    @func   BOOL | OEMDebugInit | Initializes the serial port for debug output message.
    @rdesc  TRUE == Success and FALSE == Failure.
    @comm
    @xref
*/
BOOL OEMDebugInit(void)
{

    // Set up function callbacks used by blcommon.
    //
    g_pOEMVerifyMemory   = OEMVerifyMemory;      // Verify RAM.
    g_pOEMMultiBINNotify = OEMMultiBINNotify;

    // Call serial initialization routine (shared with the OAL).
    //
    OEMInitDebugSerial();

    return(TRUE);
}

/*
    @func   void | SaveEthernetAddress | Save Ethernet Address on IMAGE_SHARE_ARGS_UA_START for Ethernet KITL
    @rdesc
    @comm
    @xref
*/
void	SaveEthernetAddress()
{
    memcpy(pBSPArgs->kitl.mac, g_pBootCfg->EdbgAddr.wMAC, 6);
	if (!(g_pBootCfg->ConfigFlags & CONFIG_FLAGS_DHCP))
	{
		// Static IP address.
		pBSPArgs->kitl.ipAddress  = g_pBootCfg->EdbgAddr.dwIP;
		pBSPArgs->kitl.ipMask     = g_pBootCfg->SubnetMask;
		pBSPArgs->kitl.flags     &= ~OAL_KITL_FLAGS_DHCP;
	}
	else
	{
		pBSPArgs->kitl.ipAddress  = g_DeviceAddr.dwIP;
		pBSPArgs->kitl.flags     |= OAL_KITL_FLAGS_DHCP;
	}
}

//
//
//
//

static void InitializeDisplay(void)
{
#ifdef	DISPLAY_BROADSHEET
	EPDInitialize();
#else	DISPLAY_BROADSHEET
    tDevInfo RGBDevInfo;

    volatile S3C6410_GPIO_REG *pGPIOReg = (S3C6410_GPIO_REG *)OALPAtoVA(S3C6410_BASE_REG_PA_GPIO, FALSE);
    volatile S3C6410_DISPLAY_REG *pDispReg = (S3C6410_DISPLAY_REG *)OALPAtoVA(S3C6410_BASE_REG_PA_DISPLAY, FALSE);
    volatile S3C6410_SPI_REG *pSPIReg = (S3C6410_SPI_REG *)OALPAtoVA(S3C6410_BASE_REG_PA_SPI0, FALSE);
    volatile S3C6410_MSMIF_REG *pMSMIFReg = (S3C6410_MSMIF_REG *)OALPAtoVA(S3C6410_BASE_REG_PA_MSMIF_SFR, FALSE);

    volatile S3C6410_SYSCON_REG *pSysConReg = (S3C6410_SYSCON_REG *)OALPAtoVA(S3C6410_BASE_REG_PA_SYSCON, FALSE);

    EdbgOutputDebugString("[Eboot] ++InitializeDisplay()\r\n");

#ifdef	OMNIBOOK_VER
	{
		UINT8 data[2];
		volatile int loop = 5000;

		IICReadByte(PMIC_ADDR, 0x01, data);
		if (!(data[0] & (1<<6)))	// [bit6] ELDO7 on
			IICWriteByte(PMIC_ADDR, 0x01, (data[0] |(1<<6)));
		while (loop--);
	}
#endif	OMNIBOOK_VER

    // Initialize Display Power Gating
    if(!(pSysConReg->BLK_PWR_STAT & (1<<4))) {
        pSysConReg->NORMAL_CFG |= (1<<14);
        while(!(pSysConReg->BLK_PWR_STAT & (1<<4)));
        }

    // Initialize Virtual Address
    LDI_initialize_register_address((void *)pSPIReg, (void *)pDispReg, (void *)pGPIOReg);
    Disp_initialize_register_address((void *)pDispReg, (void *)pMSMIFReg, (void *)pGPIOReg);

    // Set LCD Module Type
#if        (SMDK6410_LCD_MODULE == LCD_MODULE_LTS222)
    LDI_set_LCD_module_type(LDI_LTS222QV_RGB);
#elif    (SMDK6410_LCD_MODULE == LCD_MODULE_LTV350)
    LDI_set_LCD_module_type(LDI_LTV350QV_RGB);
#elif    (SMDK6410_LCD_MODULE == LCD_MODULE_LTE480)
    LDI_set_LCD_module_type(LDI_LTE480WV_RGB);
#elif    (SMDK6410_LCD_MODULE == LCD_MODULE_EMUL48_D1)
    LDI_set_LCD_module_type(LDI_LTE480WV_RGB);
#elif    (SMDK6410_LCD_MODULE == LCD_MODULE_EMUL48_QV)
    LDI_set_LCD_module_type(LDI_LTE480WV_RGB);
#elif    (SMDK6410_LCD_MODULE == LCD_MODULE_EMUL48_PQV)
    LDI_set_LCD_module_type(LDI_LTE480WV_RGB);
#elif    (SMDK6410_LCD_MODULE == LCD_MODULE_EMUL48_ML)
    LDI_set_LCD_module_type(LDI_LTE480WV_RGB);
#elif    (SMDK6410_LCD_MODULE == LCD_MODULE_EMUL48_MP)
    LDI_set_LCD_module_type(LDI_LTE480WV_RGB);
#elif    (SMDK6410_LCD_MODULE == LCD_MODULE_LTP700)
    LDI_set_LCD_module_type(LDI_LTP700WV_RGB);
#else
    EdbgOutputDebugString("[Eboot:ERR] InitializeDisplay() : Unknown Module Type [%d]\r\n", SMDK6410_LCD_MODULE);
#endif

    // Get RGB Interface Information from LDI Library
    LDI_fill_output_device_information(&RGBDevInfo);

    // Setup Output Device Information
    Disp_set_output_device_information(&RGBDevInfo);

    // Initialize Display Controller
    Disp_initialize_output_interface(DISP_VIDOUT_RGBIF);

#if        (LCD_BPP == 16)
    Disp_set_window_mode(DISP_WIN1_DMA, DISP_16BPP_565, LCD_WIDTH, LCD_HEIGHT, 0, 0);
#elif    (LCD_BPP == 32)    // XRGB format (RGB888)
    Disp_set_window_mode(DISP_WIN1_DMA, DISP_24BPP_888, LCD_WIDTH, LCD_HEIGHT, 0, 0);
#else
    EdbgOutputDebugString("[Eboot:ERR] InitializeDisplay() : Unknown Color Depth %d bpp\r\n", LCD_BPP);
#endif

    Disp_set_framebuffer(DISP_WIN1, EBOOT_FRAMEBUFFER_PA_START);
    Disp_window_onfoff(DISP_WIN1, DISP_WINDOW_ON);

#if    (SMDK6410_LCD_MODULE == LCD_MODULE_LTS222)
    // This type of LCD need MSM I/F Bypass Mode to be Disabled
    pMSMIFReg->MIFPCON &= ~(0x1<<3);    // SEL_BYPASS -> Normal Mode
#endif

    // Initialize LCD Module
    LDI_initialize_LCD_module();

    // LCD Clock Source as MPLL_Dout
    pSysConReg->CLK_SRC = (pSysConReg->CLK_SRC & ~(0xFFFFFFF0))
                            |(0<<31)    // TV27_SEL    -> 27MHz
                            |(0<<30)    // DAC27        -> 27MHz
                            |(0<<28)    // SCALER_SEL    -> MOUT_EPLL
                            |(1<<26)    // LCD_SEL    -> Dout_MPLL
                            |(0<<24)    // IRDA_SEL    -> MOUT_EPLL
                            |(0<<22)    // MMC2_SEL    -> MOUT_EPLL
                            |(0<<20)    // MMC1_SEL    -> MOUT_EPLL
                            |(0<<18)    // MMC0_SEL    -> MOUT_EPLL
                            |(0<<16)    // SPI1_SEL    -> MOUT_EPLL
                            |(0<<14)    // SPI0_SEL    -> MOUT_EPLL
                            |(0<<13)    // UART_SEL    -> MOUT_EPLL
                            |(0<<10)    // AUDIO1_SEL    -> MOUT_EPLL
                            |(0<<7)        // AUDIO0_SEL    -> MOUT_EPLL
                            |(0<<5)        // UHOST_SEL    -> 48MHz
                            |(0<<4);        // MFCCLK_SEL    -> HCLKx2 (0:HCLKx2, 1:MoutEPLL)

    // Video Output Enable
    Disp_envid_onoff(DISP_ENVID_ON);

    // Fill Framebuffer
#if    (SMDK6410_LCD_MODULE == LCD_MODULE_LTV350)
    memcpy((void *)EBOOT_FRAMEBUFFER_UA_START, (void *)InitialImage_rgb16_320x240, 320*240*2);
#elif    (SMDK6410_LCD_MODULE == LCD_MODULE_EMULQV)
    memcpy((void *)EBOOT_FRAMEBUFFER_UA_START, (void *)InitialImage_rgb16_320x240, 320*240*2);
#elif    (LCD_BPP == 16)
    {
        int i;
        unsigned short *pFB;
        pFB = (unsigned short *)EBOOT_FRAMEBUFFER_UA_START;

        for (i=0; i<LCD_WIDTH*LCD_HEIGHT; i++)
            *pFB++ = 0x001F;        // Blue
    }
#elif    (LCD_BPP == 32)
    {
        int i;
        unsigned int *pFB;
        pFB = (unsigned int *)EBOOT_FRAMEBUFFER_UA_START;

        for (i=0; i<LCD_WIDTH*LCD_HEIGHT; i++)
            *pFB++ = 0x000000FF;        // Blue
    }
#endif
    // Backlight Power On
    //Set PWM GPIO to control Back-light  Regulator  Shotdown Pin (GPF[15])
    pGPIOReg->GPFDAT |= (1<<15);
    pGPIOReg->GPFCON = (pGPIOReg->GPFCON & ~(3<<30)) | (1<<30);    // set GPF[15] as Output

    EdbgOutputDebugString("[Eboot] --InitializeDisplay()\r\n");
#endif	DISPLAY_BROADSHEET
}

static void SpinForever(void)
{
	EdbgOutputDebugString("SpinForever...\r\n");
#ifdef	OMNIBOOK_VER
	EPDOutputString("SpinForever...\r\n");
	EPDOutputFlush();
#endif	OMNIBOOK_VER

#ifdef	OMNIBOOK_VER
	{
		volatile S3C6410_GPIO_REG *pGPIOReg = (S3C6410_GPIO_REG *)OALPAtoVA(S3C6410_BASE_REG_PA_GPIO, FALSE);
		// GPC[3] PWRHOLD
		pGPIOReg->GPCDAT = (pGPIOReg->GPCDAT & ~(0xF<<0)) | (0x0<<3);
	}
#endif	OMNIBOOK_VER
	while(1)
	{
		;
	}
}

